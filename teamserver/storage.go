package main

import (
	"bytes"
	"context"
	"crypto/hmac"
	"crypto/sha256"
	"crypto/tls"
	"crypto/x509"
	"embed"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/url"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/Mafifrizi/shuna/pkg/contract"
	"github.com/Mafifrizi/shuna/pkg/envelope"
	"github.com/Mafifrizi/shuna/pkg/kdf"
	"github.com/Mafifrizi/shuna/pkg/pqc"
	"github.com/Mafifrizi/shuna/pkg/vault"
	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgxpool"
)

//go:embed migrations/001_init.sql
var migrationsFS embed.FS

const rotationBatchSize = 1000

var (
	ErrDBNotConnected = errors.New("postgresql is not configured or connected (standalone mode)")
	ErrRotationActive = errors.New("vault key rotation is already in progress")
)

type HostMeta struct {
	Hostname     string  `json:"hostname"`
	OSName       string  `json:"os_name"`
	Arch         string  `json:"arch"`
	UID          int     `json:"uid"`
	Username     string  `json:"username"`
	InternalAddr *string `json:"internal_addr"`
}

type HostView struct {
	HostID      string    `json:"host_id"`
	Status      string    `json:"status"`
	KeyVersion  uint32    `json:"key_version"`
	FirstSeenAt time.Time `json:"first_seen_at"`
	LastSeenAt  time.Time `json:"last_seen_at"`
	Meta        HostMeta  `json:"meta"`
}

type TargetProfile struct {
	HostID       string    `json:"host_id"`
	CcachePath   string    `json:"ccache_path,omitempty"`
	KeytabPath   string    `json:"keytab_path,omitempty"`
	Krb5ConfPath string    `json:"krb5_conf_path,omitempty"`
	KcmSocket    string    `json:"kcm_socket,omitempty"`
	UpdatedAt    time.Time `json:"updated_at,omitempty"`
}

type SnapshotPayload struct {
	Operative string `json:"operative"`
	Module    string `json:"module"`
	Target    string `json:"target"`
	Detail    string `json:"detail"`
	PermOrTTL string `json:"perm_or_ttl"`
	SkewOrAux string `json:"skew_or_aux"`
	RawJSON   any    `json:"raw_json,omitempty"`
}

type SnapshotView struct {
	SnapshotID  int64           `json:"snapshot_id"`
	HostID      string          `json:"host_id"`
	OperativeID string          `json:"operative_id"`
	ExitCode    int             `json:"exit_code"`
	Status      string          `json:"status"`
	KeyVersion  uint32          `json:"key_version"`
	ObservedAt  time.Time       `json:"observed_at"`
	Payload     SnapshotPayload `json:"payload"`
}

type AuditDetail struct {
	Action  string `json:"action"`
	Summary string `json:"summary"`
	Body    string `json:"body"`
}

type AuditEventView struct {
	EventID     int64       `json:"event_id"`
	HostID      string      `json:"host_id"`
	OperativeID string      `json:"operative_id"`
	Severity    string      `json:"severity"`
	ExitCode    int         `json:"exit_code"`
	Status      string      `json:"status"`
	KeyVersion  uint32      `json:"key_version"`
	CreatedAt   time.Time   `json:"created_at"`
	Detail      AuditDetail `json:"detail"`
}

type OperatorCommandPayload struct {
	OperatorName string   `json:"operator_name"`
	Action       string   `json:"action"`
	Command      string   `json:"command"`
	Args         []string `json:"args,omitempty"`
}

type vaultDiskMeta struct {
	Format            string                `json:"format"`
	Algorithm         byte                  `json:"algorithm"`
	ActiveVersion     uint32                `json:"active_version"`
	StrictLockoutWipe bool                  `json:"strict_lockout_wipe,omitempty"`
	RecoveryPublicKey *pqc.HybridPublicKey  `json:"recovery_public_key"`
	Keys              map[uint32]*vault.KeyRecord `json:"keys"`
}

// pgLockoutStore persists vault_lockout state durably in PostgreSQL (and mirrors to fileStore if set).
type pgLockoutStore struct {
	pool              *pgxpool.Pool
	strictLockoutWipe bool
	fileStore         *vault.FileLockoutStore
	lockoutFilePath   string
}

func (s *pgLockoutStore) Load() (vault.LockoutState, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()

	var failed int
	var lockedUntil, lastFailure *time.Time
	err := s.pool.QueryRow(ctx, `
		SELECT failed_attempts, locked_until, last_failure_at
		FROM vault_lockout
		WHERE id = 1
	`).Scan(&failed, &lockedUntil, &lastFailure)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return vault.LockoutState{}, nil
		}
		return vault.LockoutState{}, fmt.Errorf("load vault_lockout: %w", err)
	}

	st := vault.LockoutState{
		FailedAttempts: failed,
	}
	if lockedUntil != nil {
		st.LockedUntil = lockedUntil.UTC()
	}
	if lastFailure != nil {
		st.LastFailureAt = lastFailure.UTC()
	}
	if st.FailedAttempts >= 1 && st.FailedAttempts < vault.MaxFailedAttempts && !st.LastFailureAt.IsZero() {
		backoff := time.Duration(1<<(st.FailedAttempts-1)) * time.Second
		st.CooldownUntil = st.LastFailureAt.Add(backoff)
	}
	if s.strictLockoutWipe && st.FailedAttempts >= vault.MaxFailedAttempts {
		st.WipedLocked = true
	}

	// If local administrator cleared lockout via `shuna unlock-reset --vault ...` offline,
	// lockout.json will have failed_attempts == 0 with a newer mtime than last_failure_at.
	if s.fileStore != nil && s.lockoutFilePath != "" && st.FailedAttempts >= vault.MaxFailedAttempts {
		if info, serr := os.Stat(s.lockoutFilePath); serr == nil {
			if st.LastFailureAt.IsZero() || info.ModTime().UTC().After(st.LastFailureAt.Add(time.Second)) {
				if fst, ferr := s.fileStore.Load(); ferr == nil && fst.FailedAttempts == 0 && !fst.WipedLocked {
					_ = s.Save(vault.LockoutState{})
					return vault.LockoutState{}, nil
				}
			}
		}
	}
	return st, nil
}

func (s *pgLockoutStore) Save(st vault.LockoutState) error {
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()

	var lockedPtr, lastPtr *time.Time
	if !st.LockedUntil.IsZero() {
		t := st.LockedUntil.UTC()
		lockedPtr = &t
	}
	if !st.LastFailureAt.IsZero() {
		t := st.LastFailureAt.UTC()
		lastPtr = &t
	}

	_, err := s.pool.Exec(ctx, `
		INSERT INTO vault_lockout (id, failed_attempts, locked_until, last_failure_at)
		VALUES (1, $1, $2, $3)
		ON CONFLICT (id) DO UPDATE SET
			failed_attempts = EXCLUDED.failed_attempts,
			locked_until = EXCLUDED.locked_until,
			last_failure_at = EXCLUDED.last_failure_at
	`, st.FailedAttempts, lockedPtr, lastPtr)
	if err != nil {
		return fmt.Errorf("save vault_lockout: %w", err)
	}
	if s.fileStore != nil {
		_ = s.fileStore.Save(st)
	}
	return nil
}

type Storage struct {
	pool              *pgxpool.Pool
	poolCfg           *pgxpool.Config
	vault             *vault.Vault
	vaultDir          string
	pkiDir            string
	sslMode           string
	strictMlock       bool
	strictLockoutWipe bool

	blindMu     sync.RWMutex
	blindKey    []byte
	localHostID string

	passMu    sync.Mutex
	cachedPwd []byte

	diffMu   sync.Mutex
	lastDiff map[string]string

	rotMu      sync.Mutex
	rotating   bool
	lastRotErr string
}

func ResolvePGDSN(flagDSN string) (string, string) {
	if d := strings.TrimSpace(flagDSN); d != "" {
		return d, "flag"
	}
	if d := strings.TrimSpace(os.Getenv("CIEL_PG_DSN")); d != "" {
		return d, "env"
	}
	if home, err := os.UserHomeDir(); err == nil && home != "" {
		p := filepath.Join(home, ".config", "ciel", "pg.dsn")
		if raw, rerr := os.ReadFile(p); rerr == nil {
			if d := strings.TrimSpace(string(raw)); d != "" {
				return d, p
			}
		}
	}
	return "", ""
}

func parseRawDSNParams(rawDSN string) map[string]string {
	params := make(map[string]string)
	trimmed := strings.TrimSpace(rawDSN)
	if strings.HasPrefix(trimmed, "postgres://") || strings.HasPrefix(trimmed, "postgresql://") {
		if u, err := url.Parse(trimmed); err == nil {
			if u.Hostname() != "" {
				params["host"] = u.Hostname()
			}
			for k, vals := range u.Query() {
				if len(vals) > 0 {
					params[strings.ToLower(k)] = vals[0]
				}
			}
			return params
		}
	}
	// Keyword=value DSN
	i := 0
	for i < len(trimmed) {
		for i < len(trimmed) && (trimmed[i] == ' ' || trimmed[i] == '\t' || trimmed[i] == '\n') {
			i++
		}
		if i >= len(trimmed) {
			break
		}
		eq := strings.IndexByte(trimmed[i:], '=')
		if eq <= 0 {
			break
		}
		key := strings.ToLower(strings.TrimSpace(trimmed[i : i+eq]))
		i += eq + 1
		var val strings.Builder
		if i < len(trimmed) && trimmed[i] == '\'' {
			i++
			for i < len(trimmed) {
				if trimmed[i] == '\\' && i+1 < len(trimmed) {
					val.WriteByte(trimmed[i+1])
					i += 2
					continue
				}
				if trimmed[i] == '\'' {
					i++
					break
				}
				val.WriteByte(trimmed[i])
				i++
			}
		} else {
			for i < len(trimmed) && trimmed[i] != ' ' && trimmed[i] != '\t' && trimmed[i] != '\n' {
				val.WriteByte(trimmed[i])
				i++
			}
		}
		params[key] = val.String()
	}
	return params
}

func isLoopbackHost(host string) bool {
	h := strings.ToLower(strings.TrimSpace(host))
	return h == "" || h == "127.0.0.1" || h == "localhost" || h == "::1"
}

func isUnixSocketHost(host string) bool {
	return strings.HasPrefix(host, "/") || strings.HasPrefix(host, "@")
}

func enrichDSNWithTLS(rawDSN, pkiDir string) (string, string, error) {
	params := parseRawDSNParams(rawDSN)
	host := params["host"]
	sslMode := strings.ToLower(strings.TrimSpace(params["sslmode"]))
	sslRootCert := strings.TrimSpace(params["sslrootcert"])

	if isUnixSocketHost(host) {
		return rawDSN, "unix-socket", nil
	}

	if sslMode != "" && sslMode != "verify-full" {
		return "", "", fmt.Errorf("refusing PostgreSQL sslmode=%q on TCP host %q (sslmode=verify-full is mandatory)", sslMode, host)
	}

	caPath := filepath.Join(pkiDir, "ca.crt")
	trimmed := strings.TrimSpace(rawDSN)
	if strings.HasPrefix(trimmed, "postgres://") || strings.HasPrefix(trimmed, "postgresql://") {
		u, err := url.Parse(trimmed)
		if err != nil {
			return "", "", fmt.Errorf("parse pg dsn url: %w", err)
		}
		q := u.Query()
		if q.Get("sslmode") == "" {
			q.Set("sslmode", "verify-full")
		}
		if q.Get("sslrootcert") == "" && sslRootCert == "" {
			if _, statErr := os.Stat(caPath); statErr == nil {
				q.Set("sslrootcert", caPath)
			}
		}
		u.RawQuery = q.Encode()
		return u.String(), "verify-full", nil
	}

	var b strings.Builder
	b.WriteString(trimmed)
	if sslMode == "" {
		b.WriteString(" sslmode=verify-full")
	}
	if sslRootCert == "" {
		if _, statErr := os.Stat(caPath); statErr == nil {
			b.WriteString(" sslrootcert='")
			b.WriteString(strings.ReplaceAll(caPath, "'", "\\'"))
			b.WriteString("'")
		}
	}
	return b.String(), "verify-full", nil
}

func ValidateAndBuildPGConfig(rawDSN, pkiDir string, allowInsecureLocal bool) (*pgxpool.Config, string, error) {
	params := parseRawDSNParams(rawDSN)
	host := params["host"]
	sslModeParam := strings.ToLower(strings.TrimSpace(params["sslmode"]))

	if isUnixSocketHost(host) {
		cfg, err := pgxpool.ParseConfig(rawDSN)
		if err != nil {
			return nil, "", fmt.Errorf("parse unix socket pg dsn: %w", err)
		}
		return cfg, "unix-socket", nil
	}

	if allowInsecureLocal {
		if !isLoopbackHost(host) {
			return nil, "", fmt.Errorf("--pg-insecure-local is only permitted for loopback PostgreSQL hosts, got %q", host)
		}
		fmt.Fprintf(os.Stderr, "[ciel-teamserver] WARNING: --pg-insecure-local enabled for loopback host %q (TLS verification relaxed)\n", host)
		cfg, err := pgxpool.ParseConfig(rawDSN)
		if err != nil {
			return nil, "", fmt.Errorf("parse insecure-local pg dsn: %w", err)
		}
		mode := sslModeParam
		if mode == "" {
			mode = "insecure-local"
		}
		return cfg, mode, nil
	}

	enrichedDSN, mode, err := enrichDSNWithTLS(rawDSN, pkiDir)
	if err != nil {
		return nil, "", err
	}
	cfg, err := pgxpool.ParseConfig(enrichedDSN)
	if err != nil {
		return nil, "", fmt.Errorf("parse pg dsn: %w", err)
	}

	if cfg.ConnConfig.TLSConfig == nil {
		caPath := filepath.Join(pkiDir, "ca.crt")
		caPEM, rerr := os.ReadFile(caPath)
		if rerr != nil {
			return nil, "", fmt.Errorf("sslmode=verify-full requires TLSConfig or readable CA at %s: %w", caPath, rerr)
		}
		pool := x509.NewCertPool()
		if !pool.AppendCertsFromPEM(caPEM) {
			return nil, "", fmt.Errorf("invalid CA PEM at %s", caPath)
		}
		cfg.ConnConfig.TLSConfig = &tls.Config{
			RootCAs:    pool,
			ServerName: cfg.ConnConfig.Host,
			MinVersion: tls.VersionTLS12,
		}
	} else {
		cfg.ConnConfig.TLSConfig.InsecureSkipVerify = false
		if cfg.ConnConfig.TLSConfig.ServerName == "" {
			cfg.ConnConfig.TLSConfig.ServerName = cfg.ConnConfig.Host
		}
	}
	if cfg.ConnConfig.ConnectTimeout == 0 {
		cfg.ConnConfig.ConnectTimeout = 5 * time.Second
	}
	return cfg, mode, nil
}

func canonicalMachineID() string {
	if raw, err := os.ReadFile("/etc/machine-id"); err == nil {
		if mid := strings.TrimSpace(string(raw)); mid != "" {
			return mid
		}
	}
	host, err := os.Hostname()
	if err != nil || host == "" {
		host = "localhost"
	}
	return fmt.Sprintf("%s:%d", host, os.Getuid())
}

func currentHostMeta() HostMeta {
	uname, uid, hostname := hostUserInfo()
	return HostMeta{
		Hostname:     hostname,
		OSName:       runtime.GOOS,
		Arch:         runtime.GOARCH,
		UID:          uid,
		Username:     uname,
		InternalAddr: nil,
	}
}

func NewStorage(ctx context.Context, rawDSN, pkiDir, vaultDir string, initialPassphrase []byte, pgInsecureLocal, strictMlock, strictLockoutWipe bool) (*Storage, error) {
	st := &Storage{
		vaultDir:          vaultDir,
		pkiDir:            pkiDir,
		sslMode:           "standalone",
		strictMlock:       strictMlock,
		strictLockoutWipe: strictLockoutWipe,
		lastDiff:          make(map[string]string),
	}

	if rawDSN != "" {
		cfg, mode, err := ValidateAndBuildPGConfig(rawDSN, pkiDir, pgInsecureLocal)
		if err != nil {
			return nil, err
		}
		pool, err := pgxpool.NewWithConfig(ctx, cfg)
		if err != nil {
			return nil, fmt.Errorf("connect postgresql: %w", err)
		}
		pingCtx, cancel := context.WithTimeout(ctx, 5*time.Second)
		err = pool.Ping(pingCtx)
		cancel()
		if err != nil {
			pool.Close()
			return nil, fmt.Errorf("postgresql unreachable: %w", err)
		}
		st.pool = pool
		st.poolCfg = cfg
		st.sslMode = mode

		if err := st.applyMigrations(ctx); err != nil {
			pool.Close()
			return nil, err
		}
	}

	if err := st.initOrOpenVault(ctx, initialPassphrase); err != nil {
		if st.pool != nil {
			st.pool.Close()
		}
		return nil, err
	}

	return st, nil
}

func (st *Storage) applyMigrations(ctx context.Context) error {
	sqlBytes, err := migrationsFS.ReadFile("migrations/001_init.sql")
	if err != nil {
		return fmt.Errorf("read embedded migration: %w", err)
	}
	tx, err := st.pool.Begin(ctx)
	if err != nil {
		return fmt.Errorf("begin migration tx: %w", err)
	}
	defer func() { _ = tx.Rollback(ctx) }()

	if _, err := tx.Exec(ctx, string(sqlBytes)); err != nil {
		return fmt.Errorf("apply 001_init.sql: %w", err)
	}
	if _, err := tx.Exec(ctx, `INSERT INTO schema_migrations (version) VALUES (1) ON CONFLICT DO NOTHING`); err != nil {
		return fmt.Errorf("record schema_migrations v1: %w", err)
	}
	return tx.Commit(ctx)
}

func (st *Storage) lockoutStore() vault.LockoutStore {
	lockPath := filepath.Join(st.vaultDir, vault.LockoutFileName)
	fileStore := vault.NewFileLockoutStore(lockPath)
	if st.pool != nil {
		return &pgLockoutStore{
			pool:              st.pool,
			strictLockoutWipe: st.strictLockoutWipe,
			fileStore:         fileStore,
			lockoutFilePath:   lockPath,
		}
	}
	return fileStore
}

func (st *Storage) initOrOpenVault(ctx context.Context, initialPassphrase []byte) error {
	if st.vaultDir == "" {
		return nil
	}
	vaultPath := filepath.Join(st.vaultDir, vault.VaultFileName)
	opts := vault.Options{
		StrictMlock:       st.strictMlock,
		StrictLockoutWipe: st.strictLockoutWipe,
		LockoutStore:      st.lockoutStore(),
	}

	_, statErr := os.Stat(vaultPath)
	if statErr == nil {
		v, err := vault.OpenVault(st.vaultDir, opts)
		if err != nil {
			return fmt.Errorf("open shuna vault: %w", err)
		}
		st.vault = v
		if st.pool != nil {
			if err := st.SyncCryptoKeys(ctx); err != nil {
				return err
			}
		}
		if len(initialPassphrase) > 0 {
			if err := st.UnlockVault(ctx, initialPassphrase); err != nil {
				return err
			}
		}
		return nil
	}
	if !os.IsNotExist(statErr) {
		return fmt.Errorf("stat vault.json: %w", statErr)
	}

	// Vault does not exist yet on disk.
	if len(initialPassphrase) > 0 {
		v, _, err := vault.InitVault(st.vaultDir, initialPassphrase, opts)
		if err != nil {
			return fmt.Errorf("init shuna vault: %w", err)
		}
		st.vault = v
		st.setCachedPassphrase(initialPassphrase)
		if err := st.refreshDerivedHostState(); err != nil {
			return err
		}
		if st.pool != nil {
			if err := st.SyncCryptoKeys(ctx); err != nil {
				return err
			}
		}
		return nil
	}

	// PostgreSQL configured without vault.json or initial passphrase -> fail closed.
	if st.pool != nil {
		return contract.NewError(contract.ExitMissing, "MISSING", fmt.Sprintf("vault not initialized at %s (run shuna keygen or set CIEL_VAULT_PASSPHRASE)", st.vaultDir), nil)
	}
	// Ephemeral standalone mode without vault.json stays usable without encryption vault.
	return nil
}

func (st *Storage) setCachedPassphrase(pass []byte) {
	st.passMu.Lock()
	defer st.passMu.Unlock()
	if len(st.cachedPwd) > 0 {
		kdf.WipeBytes(st.cachedPwd)
		_ = kdf.UnlockSensitiveMemory(st.cachedPwd)
		st.cachedPwd = nil
	}
	if len(pass) > 0 {
		buf := make([]byte, len(pass))
		copy(buf, pass)
		_, _ = kdf.LockSensitiveMemory(buf, false)
		st.cachedPwd = buf
	}
}

func (st *Storage) getCachedPassphraseCopy() []byte {
	st.passMu.Lock()
	defer st.passMu.Unlock()
	if len(st.cachedPwd) == 0 {
		return nil
	}
	cp := make([]byte, len(st.cachedPwd))
	copy(cp, st.cachedPwd)
	return cp
}

func (st *Storage) Close() {
	st.passMu.Lock()
	if len(st.cachedPwd) > 0 {
		kdf.WipeBytes(st.cachedPwd)
		_ = kdf.UnlockSensitiveMemory(st.cachedPwd)
		st.cachedPwd = nil
	}
	st.passMu.Unlock()

	st.blindMu.Lock()
	if len(st.blindKey) > 0 {
		kdf.WipeBytes(st.blindKey)
		_ = kdf.UnlockSensitiveMemory(st.blindKey)
		st.blindKey = nil
	}
	st.blindMu.Unlock()

	if st.vault != nil {
		st.vault.Zeroize()
	}
	if st.pool != nil {
		st.pool.Close()
	}
}

func (st *Storage) DBConnected() bool {
	return st.pool != nil
}

func (st *Storage) SSLMode() string {
	return st.sslMode
}

func (st *Storage) ActiveKeyVersion() uint32 {
	if st.vault == nil {
		return 0
	}
	return st.vault.ActiveKeyVersion()
}

func (st *Storage) VaultLocked() bool {
	if st.vault == nil {
		return false
	}
	bk, err := st.vault.DeriveBlindIndexKey()
	if err != nil {
		return true
	}
	kdf.WipeBytes(bk)
	return false
}

func (st *Storage) VaultLockoutStatus() vault.LockoutState {
	if st.vault == nil {
		return vault.LockoutState{}
	}
	ls, err := st.vault.LockoutStatus()
	if err != nil {
		return vault.LockoutState{}
	}
	return ls
}

func (st *Storage) IsRotating() bool {
	st.rotMu.Lock()
	defer st.rotMu.Unlock()
	return st.rotating
}

func (st *Storage) LocalHostID() string {
	st.blindMu.RLock()
	defer st.blindMu.RUnlock()
	return st.localHostID
}

func (st *Storage) refreshDerivedHostState() error {
	if st.vault == nil {
		return nil
	}
	bk, err := st.vault.DeriveBlindIndexKey()
	if err != nil {
		return err
	}
	_, _ = kdf.LockSensitiveMemory(bk, false)

	mac := hmac.New(sha256.New, bk)
	mac.Write([]byte(canonicalMachineID()))
	hostID := hex.EncodeToString(mac.Sum(nil))

	st.blindMu.Lock()
	if len(st.blindKey) > 0 {
		kdf.WipeBytes(st.blindKey)
		_ = kdf.UnlockSensitiveMemory(st.blindKey)
	}
	st.blindKey = bk
	st.localHostID = hostID
	st.blindMu.Unlock()
	return nil
}

func (st *Storage) ComputeHostID(raw string) (string, error) {
	trimmed := strings.TrimSpace(raw)
	if trimmed == "" {
		hid := st.LocalHostID()
		if hid == "" {
			return "", vault.ErrVaultLocked
		}
		return hid, nil
	}
	if len(trimmed) == 64 && isLowerHex(trimmed) {
		return trimmed, nil
	}
	st.blindMu.RLock()
	defer st.blindMu.RUnlock()
	if len(st.blindKey) == 0 {
		return "", vault.ErrVaultLocked
	}
	mac := hmac.New(sha256.New, st.blindKey)
	mac.Write([]byte(trimmed))
	return hex.EncodeToString(mac.Sum(nil)), nil
}

func isLowerHex(s string) bool {
	for i := 0; i < len(s); i++ {
		c := s[i]
		if !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) {
			return false
		}
	}
	return true
}

func (st *Storage) UnlockVault(ctx context.Context, passphrase []byte) error {
	if st.vault == nil {
		return vault.ErrVaultNotFound
	}
	if err := st.vault.Unlock(passphrase); err != nil {
		if errors.Is(err, vault.ErrVaultLocked) {
			st.setCachedPassphrase(nil)
		}
		return err
	}
	st.setCachedPassphrase(passphrase)
	if err := st.refreshDerivedHostState(); err != nil {
		return err
	}
	if st.pool != nil {
		return st.SyncCryptoKeys(ctx)
	}
	return nil
}

func (st *Storage) SyncCryptoKeys(ctx context.Context) error {
	if st.pool == nil || st.vaultDir == "" {
		return nil
	}
	raw, err := os.ReadFile(filepath.Join(st.vaultDir, vault.VaultFileName))
	if err != nil {
		return fmt.Errorf("read vault.json for crypto_keys sync: %w", err)
	}
	var meta vaultDiskMeta
	if err := json.Unmarshal(raw, &meta); err != nil {
		return fmt.Errorf("parse vault.json: %w", err)
	}
	algLabel := "MLKEM1024-X25519-XCHACHA20POLY1305"
	if meta.Algorithm == envelope.AlgAES256GCM {
		algLabel = "MLKEM1024-X25519-AES256GCM"
	}
	fpPrefix := "local"
	if meta.RecoveryPublicKey != nil && len(meta.RecoveryPublicKey.Fingerprint) >= 12 {
		fpPrefix = meta.RecoveryPublicKey.Fingerprint[:12]
	}

	for ver, rec := range meta.Keys {
		if rec == nil {
			continue
		}
		wrapped := rec.WrappedDEK
		if wrapped == nil {
			wrapped = []byte{}
		}
		keyID := fmt.Sprintf("shuna-v%d-%s", ver, fpPrefix)
		_, execErr := st.pool.Exec(ctx, `
			INSERT INTO crypto_keys (key_version, key_id, algorithm, wrapped_dek, kem_ciphertext, active, decrypt_only, created_at, rotated_at, retired_at)
			VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)
			ON CONFLICT (key_version) DO UPDATE SET
				wrapped_dek = CASE WHEN octet_length(EXCLUDED.wrapped_dek) > 0 THEN EXCLUDED.wrapped_dek ELSE crypto_keys.wrapped_dek END,
				kem_ciphertext = COALESCE(EXCLUDED.kem_ciphertext, crypto_keys.kem_ciphertext),
				active = EXCLUDED.active,
				decrypt_only = EXCLUDED.decrypt_only,
				rotated_at = EXCLUDED.rotated_at,
				retired_at = EXCLUDED.retired_at
		`, int32(ver), keyID, algLabel, wrapped, rec.RecoveryKEMCiphertext, rec.Active, rec.DecryptOnly, rec.CreatedAt, rec.RotatedAt, rec.RetiredAt)
		if execErr != nil {
			return fmt.Errorf("sync crypto_keys v%d: %w", ver, execErr)
		}
	}
	return nil
}

func (st *Storage) LoadRevokedSerials(ctx context.Context) (map[string]bool, error) {
	out := make(map[string]bool)
	if st.pool == nil {
		return out, nil
	}
	rows, err := st.pool.Query(ctx, `SELECT serial_hex FROM revoked_certs`)
	if err != nil {
		return nil, fmt.Errorf("query revoked_certs: %w", err)
	}
	defer rows.Close()
	for rows.Next() {
		var s string
		if err := rows.Scan(&s); err != nil {
			return nil, err
		}
		out[strings.ToLower(s)] = true
	}
	return out, rows.Err()
}

func (st *Storage) RecordRevokedCert(ctx context.Context, serialHex, subjectCN, reason string) error {
	if st.pool == nil {
		return nil
	}
	if st.VaultLocked() || st.vault == nil {
		return vault.ErrVaultLocked
	}
	kv := st.vault.ActiveKeyVersion()
	aad := []byte(fmt.Sprintf("revoked_certs|global|%s|%d", serialHex, kv))
	pt, err := json.Marshal(map[string]string{
		"subject_cn": subjectCN,
		"reason":     reason,
	})
	if err != nil {
		return err
	}
	defer kdf.WipeBytes(pt)

	enc, err := st.vault.Encrypt(aad, pt)
	if err != nil {
		return err
	}
	_, err = st.pool.Exec(ctx, `
		INSERT INTO revoked_certs (serial_hex, subject_enc, key_version, revoked_at)
		VALUES ($1, $2, $3, NOW())
		ON CONFLICT (serial_hex) DO UPDATE SET
			subject_enc = EXCLUDED.subject_enc,
			key_version = EXCLUDED.key_version,
			revoked_at = EXCLUDED.revoked_at
	`, serialHex, enc, int32(kv))
	return err
}

func (st *Storage) UpsertHost(ctx context.Context, hostID, status string, meta HostMeta) error {
	if st.pool == nil {
		return ErrDBNotConnected
	}
	if st.VaultLocked() || st.vault == nil {
		return vault.ErrVaultLocked
	}
	if status == "" {
		status = "ONLINE"
	}
	kv := st.vault.ActiveKeyVersion()
	aad := []byte(fmt.Sprintf("hosts|%s|1|%d", hostID, kv))
	pt, err := json.Marshal(meta)
	if err != nil {
		return err
	}
	defer kdf.WipeBytes(pt)

	enc, err := st.vault.Encrypt(aad, pt)
	if err != nil {
		return err
	}
	_, err = st.pool.Exec(ctx, `
		INSERT INTO hosts (host_id, status, host_meta_enc, key_version, first_seen_at, last_seen_at)
		VALUES ($1, $2, $3, $4, NOW(), NOW())
		ON CONFLICT (host_id) DO UPDATE SET
			status = EXCLUDED.status,
			host_meta_enc = EXCLUDED.host_meta_enc,
			key_version = EXCLUDED.key_version,
			last_seen_at = NOW()
	`, hostID, status, enc, int32(kv))
	return err
}

func (st *Storage) RecordFullSync(ctx context.Context, hostID string, meta HostMeta, overallStatus string, rows []Row, logs []string) error {
	if st.pool == nil {
		return nil
	}
	if st.VaultLocked() || st.vault == nil {
		return vault.ErrVaultLocked
	}
	if err := st.UpsertHost(ctx, hostID, overallStatus, meta); err != nil {
		return err
	}

	kv := st.vault.ActiveKeyVersion()
	for i, r := range rows {
		var snapID int64
		if err := st.pool.QueryRow(ctx, `SELECT nextval('telemetry_snapshots_snapshot_id_seq')`).Scan(&snapID); err != nil {
			return fmt.Errorf("nextval telemetry_snapshots: %w", err)
		}
		payload := SnapshotPayload{
			Operative: r.Operative,
			Module:    r.Module,
			Target:    r.Target,
			Detail:    r.Detail,
			PermOrTTL: r.PermOrTTL,
			SkewOrAux: r.SkewOrAux,
			RawJSON:   r.RawJSON,
		}
		pt, err := json.Marshal(payload)
		if err != nil {
			return err
		}
		aad := []byte(fmt.Sprintf("telemetry_snapshots|%s|%d|%d", hostID, snapID, kv))
		enc, err := st.vault.Encrypt(aad, pt)
		kdf.WipeBytes(pt)
		if err != nil {
			return err
		}
		if _, err := st.pool.Exec(ctx, `
			INSERT INTO telemetry_snapshots (snapshot_id, host_id, operative_id, exit_code, status, payload_enc, key_version, observed_at)
			VALUES ($1, $2, $3, $4, $5, $6, $7, NOW())
		`, snapID, hostID, r.ID, r.ExitCode, r.Status, enc, int32(kv)); err != nil {
			return fmt.Errorf("insert telemetry_snapshot: %w", err)
		}

		// Diff audit_events when operative status/exit/target changed since last observation.
		stable := r.Target
		if r.ID == "OP-04" {
			stable = r.PermOrTTL
		}
		sig := fmt.Sprintf("%s|%d|%s", r.Status, r.ExitCode, stable)
		diffKey := hostID + "|" + r.ID

		st.diffMu.Lock()
		changed := st.lastDiff[diffKey] != sig
		if changed {
			st.lastDiff[diffKey] = sig
		}
		st.diffMu.Unlock()

		if changed {
			sev := severityForRow(r.Status, r.ExitCode)
			logBody := ""
			if i < len(logs) {
				logBody = logs[i]
			}
			if err := st.RecordAuditEvent(ctx, hostID, r.ID, sev, r.ExitCode, r.Status, AuditDetail{
				Action:  "full_sync",
				Summary: fmt.Sprintf("%s %s -> %s", r.Operative, r.Module, r.Status),
				Body:    logBody,
			}); err != nil {
				return err
			}
		}
	}
	return nil
}

func severityForRow(status string, exitCode int) string {
	switch status {
	case "MISSING", "NO_CREDENTIALS", "SHARED_KEYRING", "ATTENTION", "FINDINGS", "HIGH", "MEDIUM", "MODERATE", "EXPIRING_SOON", "WARN", "VAULT_LOCKED":
		return "WARN"
	case "ERROR", "CRITICAL", "POLICY_STOP", "REFUSED", "CLOCK_SKEW_VIOLATION", "EXPIRED", "WEAK_ENCTYPE", "RENEW_FAILED", "CONNECTION_FAILED", "BIND_FAILED", "PARSE_ERROR", "PURGE_FAILED", "USAGE_ERROR", "UNAUTHORIZED":
		return "ALERT"
	}
	if exitCode == 0 {
		return "OK"
	}
	return "INFO"
}

func (st *Storage) RecordAuditEvent(ctx context.Context, hostID, operativeID, severity string, exitCode int, status string, detail AuditDetail) error {
	if st.pool == nil || st.VaultLocked() || st.vault == nil {
		return nil
	}
	if hostID == "" {
		hostID = st.LocalHostID()
		if hostID == "" {
			return nil
		}
	}
	kv := st.vault.ActiveKeyVersion()
	var eventID int64
	if err := st.pool.QueryRow(ctx, `SELECT nextval('audit_events_event_id_seq')`).Scan(&eventID); err != nil {
		return fmt.Errorf("nextval audit_events: %w", err)
	}
	pt, err := json.Marshal(detail)
	if err != nil {
		return err
	}
	defer kdf.WipeBytes(pt)

	aad := []byte(fmt.Sprintf("audit_events|%s|%d|%d", hostID, eventID, kv))
	enc, err := st.vault.Encrypt(aad, pt)
	if err != nil {
		return err
	}
	_, err = st.pool.Exec(ctx, `
		INSERT INTO audit_events (event_id, host_id, operative_id, severity, exit_code, status, detail_enc, key_version, created_at)
		VALUES ($1, $2, $3, $4, $5, $6, $7, $8, NOW())
	`, eventID, hostID, operativeID, severity, exitCode, status, enc, int32(kv))
	return err
}

func (st *Storage) RecordOperatorCommand(ctx context.Context, hostID string, payload OperatorCommandPayload, exitCode int, status string) error {
	if st.pool == nil || st.VaultLocked() || st.vault == nil {
		return nil
	}
	if hostID == "" {
		hostID = st.LocalHostID()
		if hostID == "" {
			return nil
		}
	}
	kv := st.vault.ActiveKeyVersion()
	var cmdID int64
	if err := st.pool.QueryRow(ctx, `SELECT nextval('operator_commands_cmd_id_seq')`).Scan(&cmdID); err != nil {
		return fmt.Errorf("nextval operator_commands: %w", err)
	}
	pt, err := json.Marshal(payload)
	if err != nil {
		return err
	}
	defer kdf.WipeBytes(pt)

	aad := []byte(fmt.Sprintf("operator_commands|%s|%d|%d", hostID, cmdID, kv))
	enc, err := st.vault.Encrypt(aad, pt)
	if err != nil {
		return err
	}
	_, err = st.pool.Exec(ctx, `
		INSERT INTO operator_commands (cmd_id, host_id, exit_code, status, command_enc, key_version, executed_at)
		VALUES ($1, $2, $3, $4, $5, $6, NOW())
	`, cmdID, hostID, exitCode, status, enc, int32(kv))
	return err
}

func (st *Storage) ListHosts(ctx context.Context) ([]HostView, error) {
	if st.pool == nil {
		return nil, ErrDBNotConnected
	}
	if st.VaultLocked() || st.vault == nil {
		return nil, vault.ErrVaultLocked
	}
	rows, err := st.pool.Query(ctx, `
		SELECT host_id, status, host_meta_enc, key_version, first_seen_at, last_seen_at
		FROM hosts
		ORDER BY last_seen_at DESC
	`)
	if err != nil {
		return nil, fmt.Errorf("query hosts: %w", err)
	}
	defer rows.Close()

	var out []HostView
	for rows.Next() {
		var hv HostView
		var enc []byte
		var kv int32
		if err := rows.Scan(&hv.HostID, &hv.Status, &enc, &kv, &hv.FirstSeenAt, &hv.LastSeenAt); err != nil {
			return nil, err
		}
		hv.KeyVersion = uint32(kv)
		aad := []byte(fmt.Sprintf("hosts|%s|1|%d", hv.HostID, kv))
		pt, err := st.vault.Decrypt(aad, enc)
		if err != nil {
			return nil, fmt.Errorf("decrypt host %s: %w", hv.HostID, err)
		}
		err = json.Unmarshal(pt, &hv.Meta)
		kdf.WipeBytes(pt)
		if err != nil {
			return nil, fmt.Errorf("unmarshal host_meta %s: %w", hv.HostID, err)
		}
		out = append(out, hv)
	}
	return out, rows.Err()
}

func (st *Storage) ListHistory(ctx context.Context, hostID, operativeID string, limit int) ([]AuditEventView, []SnapshotView, error) {
	if st.pool == nil {
		return nil, nil, ErrDBNotConnected
	}
	if st.VaultLocked() || st.vault == nil {
		return nil, nil, vault.ErrVaultLocked
	}
	if limit <= 0 || limit > 500 {
		limit = 50
	}

	evQuery := `
		SELECT event_id, host_id, operative_id, severity, exit_code, status, detail_enc, key_version, created_at
		FROM audit_events
		WHERE ($1 = '' OR host_id = $1) AND ($2 = '' OR operative_id = $2)
		ORDER BY event_id DESC
		LIMIT $3
	`
	evRows, err := st.pool.Query(ctx, evQuery, hostID, operativeID, limit)
	if err != nil {
		return nil, nil, fmt.Errorf("query audit_events: %w", err)
	}
	defer evRows.Close()

	var events []AuditEventView
	for evRows.Next() {
		var ev AuditEventView
		var enc []byte
		var kv int32
		if err := evRows.Scan(&ev.EventID, &ev.HostID, &ev.OperativeID, &ev.Severity, &ev.ExitCode, &ev.Status, &enc, &kv, &ev.CreatedAt); err != nil {
			return nil, nil, err
		}
		ev.KeyVersion = uint32(kv)
		aad := []byte(fmt.Sprintf("audit_events|%s|%d|%d", ev.HostID, ev.EventID, kv))
		pt, err := st.vault.Decrypt(aad, enc)
		if err != nil {
			return nil, nil, fmt.Errorf("decrypt audit_event %d: %w", ev.EventID, err)
		}
		err = json.Unmarshal(pt, &ev.Detail)
		kdf.WipeBytes(pt)
		if err != nil {
			return nil, nil, fmt.Errorf("unmarshal audit_event %d: %w", ev.EventID, err)
		}
		events = append(events, ev)
	}
	if err := evRows.Err(); err != nil {
		return nil, nil, err
	}

	snapQuery := `
		SELECT snapshot_id, host_id, operative_id, exit_code, status, payload_enc, key_version, observed_at
		FROM telemetry_snapshots
		WHERE ($1 = '' OR host_id = $1) AND ($2 = '' OR operative_id = $2)
		ORDER BY snapshot_id DESC
		LIMIT $3
	`
	snapRows, err := st.pool.Query(ctx, snapQuery, hostID, operativeID, limit)
	if err != nil {
		return nil, nil, fmt.Errorf("query telemetry_snapshots: %w", err)
	}
	defer snapRows.Close()

	var snaps []SnapshotView
	for snapRows.Next() {
		var sv SnapshotView
		var enc []byte
		var kv int32
		if err := snapRows.Scan(&sv.SnapshotID, &sv.HostID, &sv.OperativeID, &sv.ExitCode, &sv.Status, &enc, &kv, &sv.ObservedAt); err != nil {
			return nil, nil, err
		}
		sv.KeyVersion = uint32(kv)
		aad := []byte(fmt.Sprintf("telemetry_snapshots|%s|%d|%d", sv.HostID, sv.SnapshotID, kv))
		pt, err := st.vault.Decrypt(aad, enc)
		if err != nil {
			return nil, nil, fmt.Errorf("decrypt telemetry_snapshot %d: %w", sv.SnapshotID, err)
		}
		err = json.Unmarshal(pt, &sv.Payload)
		kdf.WipeBytes(pt)
		if err != nil {
			return nil, nil, fmt.Errorf("unmarshal telemetry_snapshot %d: %w", sv.SnapshotID, err)
		}
		snaps = append(snaps, sv)
	}
	return events, snaps, snapRows.Err()
}

func (st *Storage) SaveProfile(ctx context.Context, hostID string, meta HostMeta, prof TargetProfile) error {
	if st.pool == nil {
		return ErrDBNotConnected
	}
	if st.VaultLocked() || st.vault == nil {
		return vault.ErrVaultLocked
	}
	kv := st.vault.ActiveKeyVersion()
	metaBytes, err := json.Marshal(meta)
	if err != nil {
		return fmt.Errorf("marshal host meta for profile: %w", err)
	}
	hostAAD := []byte(fmt.Sprintf("hosts|%s|1|%d", hostID, kv))
	hostMetaEnc, err := st.vault.Encrypt(hostAAD, metaBytes)
	kdf.WipeBytes(metaBytes)
	if err != nil {
		return fmt.Errorf("encrypt host meta for profile: %w", err)
	}
	// Ensure host row exists first (FK constraint).
	if _, err := st.pool.Exec(ctx, `
		INSERT INTO hosts (host_id, status, host_meta_enc, key_version, first_seen_at, last_seen_at)
		SELECT $1, 'ONLINE', $2, $3, NOW(), NOW()
		WHERE NOT EXISTS (SELECT 1 FROM hosts WHERE host_id = $1)
	`, hostID, hostMetaEnc, int32(kv)); err != nil {
		return fmt.Errorf("ensure host for profile: %w", err)
	}

	aad := []byte(fmt.Sprintf("target_profiles|%s|1|%d", hostID, kv))
	encOpt := func(s string) ([]byte, error) {
		if strings.TrimSpace(s) == "" {
			return nil, nil
		}
		return st.vault.Encrypt(aad, []byte(strings.TrimSpace(s)))
	}
	ccEnc, err := encOpt(prof.CcachePath)
	if err != nil {
		return err
	}
	ktEnc, err := encOpt(prof.KeytabPath)
	if err != nil {
		return err
	}
	k5Enc, err := encOpt(prof.Krb5ConfPath)
	if err != nil {
		return err
	}
	kcmEnc, err := encOpt(prof.KcmSocket)
	if err != nil {
		return err
	}

	_, err = st.pool.Exec(ctx, `
		INSERT INTO target_profiles (host_id, ccache_enc, keytab_enc, krb5_conf_enc, kcm_socket_enc, key_version, updated_at)
		VALUES ($1, $2, $3, $4, $5, $6, NOW())
		ON CONFLICT (host_id) DO UPDATE SET
			ccache_enc = EXCLUDED.ccache_enc,
			keytab_enc = EXCLUDED.keytab_enc,
			krb5_conf_enc = EXCLUDED.krb5_conf_enc,
			kcm_socket_enc = EXCLUDED.kcm_socket_enc,
			key_version = EXCLUDED.key_version,
			updated_at = NOW()
	`, hostID, ccEnc, ktEnc, k5Enc, kcmEnc, int32(kv))
	return err
}

func (st *Storage) GetProfile(ctx context.Context, hostID string) (TargetProfile, bool, error) {
	if st.pool == nil {
		return TargetProfile{}, false, ErrDBNotConnected
	}
	if st.VaultLocked() || st.vault == nil {
		return TargetProfile{}, false, vault.ErrVaultLocked
	}
	var ccEnc, ktEnc, k5Enc, kcmEnc []byte
	var kv int32
	var updated time.Time
	err := st.pool.QueryRow(ctx, `
		SELECT ccache_enc, keytab_enc, krb5_conf_enc, kcm_socket_enc, key_version, updated_at
		FROM target_profiles
		WHERE host_id = $1
	`, hostID).Scan(&ccEnc, &ktEnc, &k5Enc, &kcmEnc, &kv, &updated)
	if err != nil {
		if errors.Is(err, pgx.ErrNoRows) {
			return TargetProfile{HostID: hostID}, false, nil
		}
		return TargetProfile{}, false, fmt.Errorf("query target_profile: %w", err)
	}

	aad := []byte(fmt.Sprintf("target_profiles|%s|1|%d", hostID, kv))
	decOpt := func(b []byte) (string, error) {
		if len(b) == 0 {
			return "", nil
		}
		pt, derr := st.vault.Decrypt(aad, b)
		if derr != nil {
			return "", derr
		}
		s := string(pt)
		kdf.WipeBytes(pt)
		return s, nil
	}
	var prof TargetProfile
	prof.HostID = hostID
	prof.UpdatedAt = updated
	if prof.CcachePath, err = decOpt(ccEnc); err != nil {
		return prof, false, err
	}
	if prof.KeytabPath, err = decOpt(ktEnc); err != nil {
		return prof, false, err
	}
	if prof.Krb5ConfPath, err = decOpt(k5Enc); err != nil {
		return prof, false, err
	}
	if prof.KcmSocket, err = decOpt(kcmEnc); err != nil {
		return prof, false, err
	}
	return prof, true, nil
}

func (st *Storage) StartRotation(ctx context.Context, newPass []byte) (uint32, uint32, error) {
	if st.vault == nil {
		return 0, 0, vault.ErrVaultNotFound
	}
	if st.VaultLocked() {
		return 0, 0, vault.ErrVaultLocked
	}

	st.rotMu.Lock()
	if st.rotating {
		st.rotMu.Unlock()
		return 0, 0, ErrRotationActive
	}
	st.rotating = true
	st.lastRotErr = ""
	st.rotMu.Unlock()

	cleanupFlag := func() {
		st.rotMu.Lock()
		st.rotating = false
		st.rotMu.Unlock()
	}

	pass := newPass
	var borrowed []byte
	if len(pass) == 0 {
		borrowed = st.getCachedPassphraseCopy()
		pass = borrowed
	}
	defer kdf.WipeBytes(borrowed)
	if len(pass) == 0 {
		cleanupFlag()
		return 0, 0, contract.NewError(contract.ExitUsage, "USAGE", "passphrase required to rotate vault key", nil)
	}

	oldVer := st.vault.ActiveKeyVersion()
	newVer, err := st.vault.RotatePrepare(pass)
	if err != nil {
		cleanupFlag()
		return 0, 0, err
	}
	st.setCachedPassphrase(pass)

	if st.pool == nil {
		// Standalone mode: retire old key immediately after preparation.
		if err := st.vault.RetireKey(oldVer); err != nil {
			cleanupFlag()
			return oldVer, newVer, err
		}
		cleanupFlag()
		return oldVer, newVer, nil
	}

	if err := st.SyncCryptoKeys(ctx); err != nil {
		cleanupFlag()
		return oldVer, newVer, err
	}

	go st.runBatchedRotation(oldVer, newVer)
	return oldVer, newVer, nil
}

func (st *Storage) runBatchedRotation(oldVer, newVer uint32) {
	defer func() {
		st.rotMu.Lock()
		st.rotating = false
		st.rotMu.Unlock()
	}()

	ctx := context.Background()
	rotSteps := []func(context.Context, uint32) (int, error){
		st.rotateHostsBatch,
		st.rotateProfilesBatch,
		st.rotateSnapshotsBatch,
		st.rotateAuditEventsBatch,
		st.rotateOperatorCommandsBatch,
		st.rotateRevokedCertsBatch,
	}

	for _, step := range rotSteps {
		for {
			n, err := step(ctx, newVer)
			if err != nil {
				st.rotMu.Lock()
				st.lastRotErr = err.Error()
				st.rotMu.Unlock()
				return
			}
			if n == 0 {
				break
			}
		}
	}

	// Verify zero rows remain with key_version < newVer across all encrypted tables before retiring old keys.
	var rem int64
	err := st.pool.QueryRow(ctx, `
		SELECT
			(SELECT COUNT(*) FROM hosts WHERE key_version < $1) +
			(SELECT COUNT(*) FROM target_profiles WHERE key_version < $1) +
			(SELECT COUNT(*) FROM telemetry_snapshots WHERE key_version < $1) +
			(SELECT COUNT(*) FROM audit_events WHERE key_version < $1) +
			(SELECT COUNT(*) FROM operator_commands WHERE key_version < $1) +
			(SELECT COUNT(*) FROM revoked_certs WHERE key_version < $1)
	`, int32(newVer)).Scan(&rem)
	if err != nil {
		st.rotMu.Lock()
		st.lastRotErr = err.Error()
		st.rotMu.Unlock()
		return
	}
	if rem == 0 {
		for v := uint32(1); v < newVer; v++ {
			if err := st.vault.RetireKey(v); err != nil && !errors.Is(err, vault.ErrKeyNotFound) {
				st.rotMu.Lock()
				st.lastRotErr = err.Error()
				st.rotMu.Unlock()
				return
			}
		}
		if _, err := st.pool.Exec(ctx, `
			UPDATE crypto_keys
			SET active = FALSE, decrypt_only = FALSE, retired_at = COALESCE(retired_at, NOW())
			WHERE key_version < $1
		`, int32(newVer)); err != nil {
			st.rotMu.Lock()
			st.lastRotErr = err.Error()
			st.rotMu.Unlock()
			return
		}
		if err := st.SyncCryptoKeys(ctx); err != nil {
			st.rotMu.Lock()
			st.lastRotErr = err.Error()
			st.rotMu.Unlock()
			return
		}
	}
	_ = oldVer
}

func (st *Storage) rotateHostsBatch(ctx context.Context, newVer uint32) (int, error) {
	tx, err := st.pool.Begin(ctx)
	if err != nil {
		return 0, err
	}
	defer func() { _ = tx.Rollback(ctx) }()

	rows, err := tx.Query(ctx, `
		SELECT host_id, host_meta_enc, key_version
		FROM hosts
		WHERE key_version < $1
		ORDER BY host_id
		LIMIT $2
		FOR UPDATE SKIP LOCKED
	`, int32(newVer), rotationBatchSize)
	if err != nil {
		return 0, err
	}
	type item struct {
		id  string
		enc []byte
		kv  int32
	}
	var batch []item
	for rows.Next() {
		var it item
		if err := rows.Scan(&it.id, &it.enc, &it.kv); err != nil {
			rows.Close()
			return 0, err
		}
		batch = append(batch, it)
	}
	rows.Close()
	if err := rows.Err(); err != nil {
		return 0, err
	}
	for _, it := range batch {
		oldAAD := []byte(fmt.Sprintf("hosts|%s|1|%d", it.id, it.kv))
		pt, err := st.vault.Decrypt(oldAAD, it.enc)
		if err != nil {
			return 0, err
		}
		newAAD := []byte(fmt.Sprintf("hosts|%s|1|%d", it.id, newVer))
		reEnc, err := st.vault.Encrypt(newAAD, pt)
		kdf.WipeBytes(pt)
		if err != nil {
			return 0, err
		}
		if _, err := tx.Exec(ctx, `UPDATE hosts SET host_meta_enc = $1, key_version = $2 WHERE host_id = $3`, reEnc, int32(newVer), it.id); err != nil {
			return 0, err
		}
	}
	if err := tx.Commit(ctx); err != nil {
		return 0, err
	}
	return len(batch), nil
}

func (st *Storage) rotateProfilesBatch(ctx context.Context, newVer uint32) (int, error) {
	tx, err := st.pool.Begin(ctx)
	if err != nil {
		return 0, err
	}
	defer func() { _ = tx.Rollback(ctx) }()

	rows, err := tx.Query(ctx, `
		SELECT host_id, ccache_enc, keytab_enc, krb5_conf_enc, kcm_socket_enc, key_version
		FROM target_profiles
		WHERE key_version < $1
		ORDER BY host_id
		LIMIT $2
		FOR UPDATE SKIP LOCKED
	`, int32(newVer), rotationBatchSize)
	if err != nil {
		return 0, err
	}
	type item struct {
		id              string
		cc, kt, k5, kcm []byte
		kv              int32
	}
	var batch []item
	for rows.Next() {
		var it item
		if err := rows.Scan(&it.id, &it.cc, &it.kt, &it.k5, &it.kcm, &it.kv); err != nil {
			rows.Close()
			return 0, err
		}
		batch = append(batch, it)
	}
	rows.Close()
	if err := rows.Err(); err != nil {
		return 0, err
	}
	reEncryptOpt := func(oldAAD, newAAD, blob []byte) ([]byte, error) {
		if len(blob) == 0 {
			return nil, nil
		}
		pt, err := st.vault.Decrypt(oldAAD, blob)
		if err != nil {
			return nil, err
		}
		out, err := st.vault.Encrypt(newAAD, pt)
		kdf.WipeBytes(pt)
		return out, err
	}
	for _, it := range batch {
		oldAAD := []byte(fmt.Sprintf("target_profiles|%s|1|%d", it.id, it.kv))
		newAAD := []byte(fmt.Sprintf("target_profiles|%s|1|%d", it.id, newVer))
		cc, err := reEncryptOpt(oldAAD, newAAD, it.cc)
		if err != nil {
			return 0, err
		}
		kt, err := reEncryptOpt(oldAAD, newAAD, it.kt)
		if err != nil {
			return 0, err
		}
		k5, err := reEncryptOpt(oldAAD, newAAD, it.k5)
		if err != nil {
			return 0, err
		}
		kcm, err := reEncryptOpt(oldAAD, newAAD, it.kcm)
		if err != nil {
			return 0, err
		}
		if _, err := tx.Exec(ctx, `
			UPDATE target_profiles
			SET ccache_enc = $1, keytab_enc = $2, krb5_conf_enc = $3, kcm_socket_enc = $4, key_version = $5
			WHERE host_id = $6
		`, cc, kt, k5, kcm, int32(newVer), it.id); err != nil {
			return 0, err
		}
	}
	if err := tx.Commit(ctx); err != nil {
		return 0, err
	}
	return len(batch), nil
}

func (st *Storage) rotateSnapshotsBatch(ctx context.Context, newVer uint32) (int, error) {
	tx, err := st.pool.Begin(ctx)
	if err != nil {
		return 0, err
	}
	defer func() { _ = tx.Rollback(ctx) }()

	rows, err := tx.Query(ctx, `
		SELECT snapshot_id, host_id, payload_enc, key_version
		FROM telemetry_snapshots
		WHERE key_version < $1
		ORDER BY snapshot_id
		LIMIT $2
		FOR UPDATE SKIP LOCKED
	`, int32(newVer), rotationBatchSize)
	if err != nil {
		return 0, err
	}
	type item struct {
		id     int64
		hostID string
		enc    []byte
		kv     int32
	}
	var batch []item
	for rows.Next() {
		var it item
		if err := rows.Scan(&it.id, &it.hostID, &it.enc, &it.kv); err != nil {
			rows.Close()
			return 0, err
		}
		batch = append(batch, it)
	}
	rows.Close()
	if err := rows.Err(); err != nil {
		return 0, err
	}
	for _, it := range batch {
		oldAAD := []byte(fmt.Sprintf("telemetry_snapshots|%s|%d|%d", it.hostID, it.id, it.kv))
		pt, err := st.vault.Decrypt(oldAAD, it.enc)
		if err != nil {
			return 0, err
		}
		newAAD := []byte(fmt.Sprintf("telemetry_snapshots|%s|%d|%d", it.hostID, it.id, newVer))
		reEnc, err := st.vault.Encrypt(newAAD, pt)
		kdf.WipeBytes(pt)
		if err != nil {
			return 0, err
		}
		if _, err := tx.Exec(ctx, `UPDATE telemetry_snapshots SET payload_enc = $1, key_version = $2 WHERE snapshot_id = $3`, reEnc, int32(newVer), it.id); err != nil {
			return 0, err
		}
	}
	if err := tx.Commit(ctx); err != nil {
		return 0, err
	}
	return len(batch), nil
}

func (st *Storage) rotateAuditEventsBatch(ctx context.Context, newVer uint32) (int, error) {
	tx, err := st.pool.Begin(ctx)
	if err != nil {
		return 0, err
	}
	defer func() { _ = tx.Rollback(ctx) }()

	rows, err := tx.Query(ctx, `
		SELECT event_id, host_id, detail_enc, key_version
		FROM audit_events
		WHERE key_version < $1
		ORDER BY event_id
		LIMIT $2
		FOR UPDATE SKIP LOCKED
	`, int32(newVer), rotationBatchSize)
	if err != nil {
		return 0, err
	}
	type item struct {
		id     int64
		hostID string
		enc    []byte
		kv     int32
	}
	var batch []item
	for rows.Next() {
		var it item
		if err := rows.Scan(&it.id, &it.hostID, &it.enc, &it.kv); err != nil {
			rows.Close()
			return 0, err
		}
		batch = append(batch, it)
	}
	rows.Close()
	if err := rows.Err(); err != nil {
		return 0, err
	}
	for _, it := range batch {
		oldAAD := []byte(fmt.Sprintf("audit_events|%s|%d|%d", it.hostID, it.id, it.kv))
		pt, err := st.vault.Decrypt(oldAAD, it.enc)
		if err != nil {
			return 0, err
		}
		newAAD := []byte(fmt.Sprintf("audit_events|%s|%d|%d", it.hostID, it.id, newVer))
		reEnc, err := st.vault.Encrypt(newAAD, pt)
		kdf.WipeBytes(pt)
		if err != nil {
			return 0, err
		}
		if _, err := tx.Exec(ctx, `UPDATE audit_events SET detail_enc = $1, key_version = $2 WHERE event_id = $3`, reEnc, int32(newVer), it.id); err != nil {
			return 0, err
		}
	}
	if err := tx.Commit(ctx); err != nil {
		return 0, err
	}
	return len(batch), nil
}

func (st *Storage) rotateOperatorCommandsBatch(ctx context.Context, newVer uint32) (int, error) {
	tx, err := st.pool.Begin(ctx)
	if err != nil {
		return 0, err
	}
	defer func() { _ = tx.Rollback(ctx) }()

	rows, err := tx.Query(ctx, `
		SELECT cmd_id, host_id, command_enc, key_version
		FROM operator_commands
		WHERE key_version < $1
		ORDER BY cmd_id
		LIMIT $2
		FOR UPDATE SKIP LOCKED
	`, int32(newVer), rotationBatchSize)
	if err != nil {
		return 0, err
	}
	type item struct {
		id     int64
		hostID string
		enc    []byte
		kv     int32
	}
	var batch []item
	for rows.Next() {
		var it item
		if err := rows.Scan(&it.id, &it.hostID, &it.enc, &it.kv); err != nil {
			rows.Close()
			return 0, err
		}
		batch = append(batch, it)
	}
	rows.Close()
	if err := rows.Err(); err != nil {
		return 0, err
	}
	for _, it := range batch {
		oldAAD := []byte(fmt.Sprintf("operator_commands|%s|%d|%d", it.hostID, it.id, it.kv))
		pt, err := st.vault.Decrypt(oldAAD, it.enc)
		if err != nil {
			return 0, err
		}
		newAAD := []byte(fmt.Sprintf("operator_commands|%s|%d|%d", it.hostID, it.id, newVer))
		reEnc, err := st.vault.Encrypt(newAAD, pt)
		kdf.WipeBytes(pt)
		if err != nil {
			return 0, err
		}
		if _, err := tx.Exec(ctx, `UPDATE operator_commands SET command_enc = $1, key_version = $2 WHERE cmd_id = $3`, reEnc, int32(newVer), it.id); err != nil {
			return 0, err
		}
	}
	if err := tx.Commit(ctx); err != nil {
		return 0, err
	}
	return len(batch), nil
}

func (st *Storage) rotateRevokedCertsBatch(ctx context.Context, newVer uint32) (int, error) {
	tx, err := st.pool.Begin(ctx)
	if err != nil {
		return 0, err
	}
	defer func() { _ = tx.Rollback(ctx) }()

	rows, err := tx.Query(ctx, `
		SELECT serial_hex, subject_enc, key_version
		FROM revoked_certs
		WHERE key_version < $1
		ORDER BY serial_hex
		LIMIT $2
		FOR UPDATE SKIP LOCKED
	`, int32(newVer), rotationBatchSize)
	if err != nil {
		return 0, err
	}
	type item struct {
		serial string
		enc    []byte
		kv     int32
	}
	var batch []item
	for rows.Next() {
		var it item
		if err := rows.Scan(&it.serial, &it.enc, &it.kv); err != nil {
			rows.Close()
			return 0, err
		}
		batch = append(batch, it)
	}
	rows.Close()
	if err := rows.Err(); err != nil {
		return 0, err
	}
	for _, it := range batch {
		oldAAD := []byte(fmt.Sprintf("revoked_certs|global|%s|%d", it.serial, it.kv))
		pt, err := st.vault.Decrypt(oldAAD, it.enc)
		if err != nil {
			return 0, err
		}
		newAAD := []byte(fmt.Sprintf("revoked_certs|global|%s|%d", it.serial, newVer))
		reEnc, err := st.vault.Encrypt(newAAD, pt)
		kdf.WipeBytes(pt)
		if err != nil {
			return 0, err
		}
		if _, err := tx.Exec(ctx, `UPDATE revoked_certs SET subject_enc = $1, key_version = $2 WHERE serial_hex = $3`, reEnc, int32(newVer), it.serial); err != nil {
			return 0, err
		}
	}
	if err := tx.Commit(ctx); err != nil {
		return 0, err
	}
	return len(batch), nil
}

func findPgDumpBin() (string, error) {
	if p, err := exec.LookPath("pg_dump"); err == nil {
		return p, nil
	}
	for _, c := range []string{"/usr/lib/postgresql/16/bin/pg_dump", "/usr/bin/pg_dump"} {
		if _, err := os.Stat(c); err == nil {
			return c, nil
		}
	}
	return "", errors.New("pg_dump binary not found on PATH or /usr/lib/postgresql/16/bin/pg_dump")
}

func (st *Storage) ExportBackup(ctx context.Context, outPath string) (int64, error) {
	if st.pool == nil || st.poolCfg == nil {
		return 0, ErrDBNotConnected
	}
	if st.VaultLocked() || st.vault == nil {
		return 0, vault.ErrVaultLocked
	}
	if err := validatePurgeTarget(outPath); err != nil {
		return 0, contract.NewError(contract.ExitUsage, "USAGE_ERROR", err.Error(), err)
	}
	for _, seg := range strings.FieldsFunc(outPath, func(r rune) bool { return r == '/' || r == '\\' }) {
		if seg == ".." {
			return 0, contract.NewError(contract.ExitUsage, "USAGE_ERROR", "backup destination must not contain '..' traversal", nil)
		}
	}
	if info, err := os.Lstat(outPath); err == nil && info.Mode()&os.ModeSymlink != 0 {
		return 0, contract.NewError(contract.ExitPolicyStop, "POLICY_STOP", fmt.Sprintf("refusing symlink backup destination: %s", outPath), nil)
	}

	pgDumpBin, err := findPgDumpBin()
	if err != nil {
		return 0, err
	}

	cc := st.poolCfg.ConnConfig
	env := sanitizedChildEnv(
		"PGHOST="+cc.Host,
		"PGPORT="+strconv.Itoa(int(cc.Port)),
		"PGUSER="+cc.User,
		"PGDATABASE="+cc.Database,
	)
	if cc.Password != "" {
		env = append(env, "PGPASSWORD="+cc.Password)
	}
	if st.sslMode == "verify-full" {
		env = append(env, "PGSSLMODE=verify-full")
		caPath := filepath.Join(st.pkiDir, "ca.crt")
		if _, statErr := os.Stat(caPath); statErr == nil {
			env = append(env, "PGSSLROOTCERT="+caPath)
		}
	} else if st.sslMode == "unix-socket" {
		env = append(env, "PGSSLMODE=disable")
	}

	dir := filepath.Dir(outPath)
	if err := validateSecureDir(dir); err != nil {
		return 0, contract.NewError(contract.ExitPolicyStop, "POLICY_STOP", err.Error(), err)
	}

	cmdCtx, cancel := context.WithTimeout(ctx, 2*time.Minute)
	defer cancel()
	cmd := exec.CommandContext(cmdCtx, pgDumpBin, "--no-owner", "--no-acl")
	cmd.Env = env
	var stderr bytes.Buffer
	cmd.Stderr = &stderr
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		return 0, fmt.Errorf("pg_dump stdout pipe: %w", err)
	}
	if err := cmd.Start(); err != nil {
		return 0, fmt.Errorf("start pg_dump: %w", err)
	}

	tmp, err := os.CreateTemp(dir, ".ciel-backup-*.shunas1")
	if err != nil {
		_ = cmd.Process.Kill()
		_ = cmd.Wait()
		return 0, err
	}
	tmpName := tmp.Name()
	cleanupTmp := func() {
		_ = tmp.Close()
		_ = os.Remove(tmpName)
	}

	cw := &countingWriter{w: tmp}
	sealErr := st.vault.SealStream(stdout, cw, []byte("ciel:pg_dump:v1"))
	_, _ = io.Copy(io.Discard, stdout)
	waitErr := cmd.Wait()
	if sealErr != nil {
		cleanupTmp()
		return 0, fmt.Errorf("seal pg_dump stream: %w", sealErr)
	}
	if waitErr != nil {
		cleanupTmp()
		return 0, fmt.Errorf("pg_dump failed: %w (%s)", waitErr, strings.TrimSpace(stderr.String()))
	}
	if err := tmp.Chmod(0o600); err != nil {
		cleanupTmp()
		return 0, err
	}
	if err := tmp.Close(); err != nil {
		_ = os.Remove(tmpName)
		return 0, err
	}
	defer os.Remove(tmpName)
	if err := installTempFileNoFollow(tmpName, outPath, 0o600); err != nil {
		return 0, contract.NewError(contract.ExitPolicyStop, "POLICY_STOP", err.Error(), err)
	}
	return cw.n, nil
}

type countingWriter struct {
	w io.Writer
	n int64
}

func (c *countingWriter) Write(p []byte) (int, error) {
	n, err := c.w.Write(p)
	c.n += int64(n)
	return n, err
}
