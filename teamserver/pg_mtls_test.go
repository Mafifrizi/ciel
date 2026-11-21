package main

import (
	"bufio"
	"bytes"
	"context"
	"crypto/rand"
	"crypto/tls"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/Mafifrizi/shuna/pkg/envelope"
	"github.com/Mafifrizi/shuna/pkg/mtls"
	"github.com/Mafifrizi/shuna/pkg/pqc"
	"github.com/Mafifrizi/shuna/pkg/vault"
	"github.com/jackc/pgx/v5/pgxpool"
)

func findPG16Bin(t *testing.T, name string) string {
	t.Helper()
	p := filepath.Join("/usr/lib/postgresql/16/bin", name)
	if _, err := os.Stat(p); err == nil {
		return p
	}
	if lp, err := exec.LookPath(name); err == nil {
		return lp
	}
	t.Skipf("PostgreSQL 16 binary %s not installed; skipping real DB test", name)
	return ""
}

func freeTCPPort(t *testing.T) int {
	t.Helper()
	ln, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("freeTCPPort: %v", err)
	}
	port := ln.Addr().(*net.TCPAddr).Port
	_ = ln.Close()
	return port
}

type ephemeralPG struct {
	dsn      string
	adminDSN string
	sockDir  string
	port     int
	cmd      *exec.Cmd
	cancel   context.CancelFunc
}

func startEphemeralPG16(t *testing.T, pkiDir string) *ephemeralPG {
	t.Helper()
	if os.Getuid() == 0 {
		t.Skip("PostgreSQL initdb/postgres cannot run as root")
	}
	initdbBin := findPG16Bin(t, "initdb")
	postgresBin := findPG16Bin(t, "postgres")

	baseDir, err := os.MkdirTemp("", "ciel-pg16-*")
	if err != nil {
		t.Fatalf("MkdirTemp pg: %v", err)
	}
	t.Cleanup(func() { _ = os.RemoveAll(baseDir) })

	dataDir := filepath.Join(baseDir, "pgdata")
	sockDir := filepath.Join(baseDir, "sock")
	if err := os.MkdirAll(sockDir, 0o700); err != nil {
		t.Fatal(err)
	}
	pwFile := filepath.Join(baseDir, "pw.txt")
	pgPass := "ciel-test-pg-pass-2026"
	if err := os.WriteFile(pwFile, []byte(pgPass+"\n"), 0o600); err != nil {
		t.Fatal(err)
	}

	initCmd := exec.Command(initdbBin,
		"-D", dataDir,
		"-U", "ciel",
		"--auth-host=scram-sha-256",
		"--auth-local=scram-sha-256",
		"--pwfile="+pwFile,
		"-E", "UTF8",
	)
	if out, err := initCmd.CombinedOutput(); err != nil {
		t.Fatalf("initdb failed: %v\n%s", err, string(out))
	}

	// Stage TLS certs inside dataDir with 0600 key permissions so postgres accepts them on any FS.
	copyFile := func(src, dst string, mode os.FileMode) {
		b, rerr := os.ReadFile(src)
		if rerr != nil {
			t.Fatalf("read %s: %v", src, rerr)
		}
		if werr := os.WriteFile(dst, b, mode); werr != nil {
			t.Fatalf("write %s: %v", dst, werr)
		}
		_ = os.Chmod(dst, mode)
	}
	pgCA := filepath.Join(dataDir, "ca.crt")
	pgCert := filepath.Join(dataDir, "pg-server.crt")
	pgKey := filepath.Join(dataDir, "pg-server.key")
	copyFile(filepath.Join(pkiDir, "ca.crt"), pgCA, 0o644)
	copyFile(filepath.Join(pkiDir, "pg-server.crt"), pgCert, 0o644)
	copyFile(filepath.Join(pkiDir, "pg-server.key"), pgKey, 0o600)

	hbaPath := filepath.Join(dataDir, "pg_hba.conf")
	hbaBody := "local all all scram-sha-256\nhostssl all all 127.0.0.1/32 scram-sha-256\nhostssl all all ::1/128 scram-sha-256\n"
	if err := os.WriteFile(hbaPath, []byte(hbaBody), 0o600); err != nil {
		t.Fatal(err)
	}

	port := freeTCPPort(t)
	ctx, cancel := context.WithCancel(context.Background())
	pgCmd := exec.CommandContext(ctx, postgresBin,
		"-D", dataDir,
		"-p", fmt.Sprintf("%d", port),
		"-h", "127.0.0.1",
		"-k", sockDir,
		"-c", "ssl=on",
		"-c", "ssl_cert_file="+pgCert,
		"-c", "ssl_key_file="+pgKey,
		"-c", "ssl_ca_file="+pgCA,
		"-c", "ssl_min_protocol_version=TLSv1.3",
		"-c", "password_encryption=scram-sha-256",
		"-c", "hba_file="+hbaPath,
	)
	var pgLog bytes.Buffer
	pgCmd.Stdout = &pgLog
	pgCmd.Stderr = &pgLog
	if err := pgCmd.Start(); err != nil {
		cancel()
		t.Fatalf("start postgres: %v", err)
	}
	t.Cleanup(func() {
		cancel()
		_ = pgCmd.Wait()
	})

	dsn := fmt.Sprintf("postgres://ciel:%s@127.0.0.1:%d/postgres?sslmode=verify-full&sslrootcert=%s",
		pgPass, port, filepath.Join(pkiDir, "ca.crt"))

	deadline := time.Now().Add(15 * time.Second)
	for time.Now().Before(deadline) {
		cfg, _, err := ValidateAndBuildPGConfig(dsn, pkiDir, false)
		if err == nil {
			pool, perr := pgxpool.NewWithConfig(context.Background(), cfg)
			if perr == nil {
				pctx, pcancel := context.WithTimeout(context.Background(), 500*time.Millisecond)
				pingErr := pool.Ping(pctx)
				pcancel()
				pool.Close()
				if pingErr == nil {
					return &ephemeralPG{
						dsn:      dsn,
						adminDSN: dsn,
						sockDir:  sockDir,
						port:     port,
						cmd:      pgCmd,
						cancel:   cancel,
					}
				}
			}
		}
		time.Sleep(150 * time.Millisecond)
	}
	t.Fatalf("ephemeral PostgreSQL 16 failed to become ready on port %d:\n%s", port, pgLog.String())
	return nil
}

func TestPostgreSQLTLSValidationAndFailClosed(t *testing.T) {
	pkiDir := t.TempDir()
	if _, err := mtls.InitPKI(pkiDir, []string{"127.0.0.1", "localhost"}, []string{"127.0.0.1", "localhost"}, 24*time.Hour); err != nil {
		t.Fatalf("InitPKI: %v", err)
	}

	// Reject sslmode=disable, require, prefer on TCP hosts unless --pg-insecure-local
	for _, mode := range []string{"disable", "allow", "prefer", "require", "verify-ca"} {
		dsn := fmt.Sprintf("postgres://ciel:pw@127.0.0.1:5432/ciel?sslmode=%s", mode)
		if _, _, err := ValidateAndBuildPGConfig(dsn, pkiDir, false); err == nil {
			t.Fatalf("expected sslmode=%s on TCP host to be rejected", mode)
		}
	}

	// Reject --pg-insecure-local on non-loopback host
	if _, _, err := ValidateAndBuildPGConfig("postgres://ciel:pw@10.10.10.10:5432/ciel?sslmode=disable", pkiDir, true); err == nil {
		t.Fatal("expected --pg-insecure-local on non-loopback host to be rejected")
	}

	// Allow --pg-insecure-local on loopback
	if _, mode, err := ValidateAndBuildPGConfig("postgres://ciel:pw@127.0.0.1:5432/ciel?sslmode=disable", pkiDir, true); err != nil || mode != "disable" {
		t.Fatalf("expected loopback --pg-insecure-local to succeed, got mode=%q err=%v", mode, err)
	}

	// Allow UNIX socket
	if _, mode, err := ValidateAndBuildPGConfig("host=/var/run/postgresql user=ciel dbname=ciel sslmode=disable", pkiDir, false); err != nil || mode != "unix-socket" {
		t.Fatalf("expected unix socket DSN to succeed, got mode=%q err=%v", mode, err)
	}

	// Enrich TCP DSN lacking sslmode with verify-full + ca.crt
	cfg, mode, err := ValidateAndBuildPGConfig("postgres://ciel:pw@127.0.0.1:5432/ciel", pkiDir, false)
	if err != nil || mode != "verify-full" || cfg.ConnConfig.TLSConfig == nil || cfg.ConnConfig.TLSConfig.ServerName != "127.0.0.1" {
		t.Fatalf("expected enriched verify-full TLSConfig, got mode=%q cfg=%+v err=%v", mode, cfg, err)
	}

	// Standalone mode when DSN is empty
	workDir := t.TempDir()
	st, err := NewStorage(context.Background(), "", pkiDir, filepath.Join(workDir, "vault"), nil, false, false, false)
	if err != nil {
		t.Fatalf("standalone NewStorage: %v", err)
	}
	if st.DBConnected() || st.SSLMode() != "standalone" {
		t.Fatalf("expected standalone mode, got connected=%v sslMode=%q", st.DBConnected(), st.SSLMode())
	}
	st.Close()

	// Unreachable configured PG DSN must cause binary startup to fail closed with exit code 3
	binPath := filepath.Join(t.TempDir(), "ciel-teamserver-test")
	buildCmd := exec.Command("go", "build", "-o", binPath, ".")
	if out, err := buildCmd.CombinedOutput(); err != nil {
		t.Fatalf("build teamserver binary: %v\n%s", err, out)
	}
	deadPort := freeTCPPort(t)
	failWork := t.TempDir()
	failCmd := exec.Command(binPath,
		"--addr", fmt.Sprintf("127.0.0.1:%d", freeTCPPort(t)),
		"--work-dir", failWork,
		"--pki-dir", pkiDir,
		"--pg-dsn", fmt.Sprintf("postgres://ciel:pw@127.0.0.1:%d/ciel?sslmode=verify-full&sslrootcert=%s", deadPort, filepath.Join(pkiDir, "ca.crt")),
	)
	failCmd.Env = append(os.Environ(), "CIEL_VAULT_PASSPHRASE=test-pass")
	err = failCmd.Run()
	var ee *exec.ExitError
	if !errors.As(err, &ee) || ee.ExitCode() != 3 {
		t.Fatalf("expected unreachable --pg-dsn to exit 3, got err=%v", err)
	}
}

func TestRealPostgresEncryptedFleetRotationLockoutAndBackup(t *testing.T) {
	pkiDir := t.TempDir()
	if _, err := mtls.InitPKI(pkiDir, []string{"127.0.0.1", "localhost"}, []string{"127.0.0.1", "localhost"}, 24*time.Hour); err != nil {
		t.Fatalf("InitPKI: %v", err)
	}
	pg := startEphemeralPG16(t, pkiDir)

	// Verify wrong CA is rejected under sslmode=verify-full
	otherPKI := t.TempDir()
	if _, err := mtls.InitPKI(otherPKI, []string{"127.0.0.1"}, []string{"127.0.0.1"}, 24*time.Hour); err != nil {
		t.Fatal(err)
	}
	badCADSN := fmt.Sprintf("postgres://ciel:ciel-test-pg-pass-2026@127.0.0.1:%d/postgres?sslmode=verify-full&sslrootcert=%s",
		pg.port, filepath.Join(otherPKI, "ca.crt"))
	if _, err := NewStorage(context.Background(), badCADSN, otherPKI, filepath.Join(t.TempDir(), "v"), []byte("pass"), false, false, false); err == nil {
		t.Fatal("expected sslmode=verify-full with wrong CA to fail")
	}

	vaultDir := filepath.Join(t.TempDir(), "vault")
	pass1 := []byte("super-secret-vault-pass-1")
	ctx := context.Background()

	st, err := NewStorage(ctx, pg.dsn, pkiDir, vaultDir, pass1, false, false, true)
	if err != nil {
		t.Fatalf("NewStorage with real PostgreSQL 16 TLS: %v", err)
	}
	if !st.DBConnected() || st.SSLMode() != "verify-full" || st.VaultLocked() {
		t.Fatalf("unexpected storage state: connected=%v ssl=%s locked=%v", st.DBConnected(), st.SSLMode(), st.VaultLocked())
	}

	hostID, err := st.ComputeHostID("")
	if err != nil || len(hostID) != 64 {
		t.Fatalf("ComputeHostID failed: %q err=%v", hostID, err)
	}

	meta := HostMeta{
		Hostname:     "secret-workstation-alpha",
		OSName:       "linux",
		Arch:         "amd64",
		UID:          1000,
		Username:     "secret_operator",
		InternalAddr: nil,
	}
	rows := []Row{
		{
			ID: "OP-01", Operative: "Souei", Module: "keyring-probe",
			Target: "uid=1000", Detail: "keyring=998877 isolated=true",
			PermOrTTL: "perm=0x3f000000", SkewOrAux: "keys=2", ExitCode: 0, Status: "OK",
			RawJSON: map[string]any{"secret_token_field": "sensitive-op01-json"},
		},
		{
			ID: "OP-02", Operative: "Chronoa", Module: "ccache-watchdog",
			Target: "svc_secret@CORP.LOCAL", Detail: "aes256-cts-hmac-sha1-96",
			PermOrTTL: "ttl=3600s", SkewOrAux: "skew=0s", ExitCode: 0, Status: "HEALTHY",
			RawJSON: map[string]any{"principal": "svc_secret@CORP.LOCAL"},
		},
	}
	if err := st.RecordFullSync(ctx, hostID, meta, "SYNCED", rows, []string{"log1", "log2"}); err != nil {
		t.Fatalf("RecordFullSync: %v", err)
	}
	if err := st.SaveProfile(ctx, hostID, meta, TargetProfile{
		HostID:       hostID,
		CcachePath:   "/tmp/secret_krb5cc_1000",
		KeytabPath:   "/home/secret_operator/svc.keytab",
		Krb5ConfPath: "/etc/secret_krb5.conf",
		KcmSocket:    "/run/secret-kcm.sock",
	}); err != nil {
		t.Fatalf("SaveProfile: %v", err)
	}
	if err := st.RecordOperatorCommand(ctx, hostID, OperatorCommandPayload{
		OperatorName: "secret_operator",
		Action:       "tanuki_exec",
		Command:      "blast --keytab /home/secret_operator/svc.keytab",
	}, 0, "LOW"); err != nil {
		t.Fatalf("RecordOperatorCommand: %v", err)
	}

	// Verify decrypted round-trips & internal_addr == nil
	hosts, err := st.ListHosts(ctx)
	if err != nil || len(hosts) != 1 || hosts[0].Meta.Hostname != "secret-workstation-alpha" || hosts[0].Meta.InternalAddr != nil {
		t.Fatalf("ListHosts mismatch: %+v err=%v", hosts, err)
	}
	prof, found, err := st.GetProfile(ctx, hostID)
	if err != nil || !found || prof.CcachePath != "/tmp/secret_krb5cc_1000" {
		t.Fatalf("GetProfile mismatch: %+v found=%v err=%v", prof, found, err)
	}
	events, snaps, err := st.ListHistory(ctx, hostID, "", 50)
	if err != nil || len(events) != 2 || len(snaps) != 2 {
		t.Fatalf("ListHistory mismatch: events=%d snaps=%d err=%v", len(events), len(snaps), err)
	}

	// Verify ZERO plaintext sensitive metadata in raw PostgreSQL BYTEA columns and SHUNA1 header present
	var rawHostEnc, rawCCEnc, rawSnapEnc, rawEvEnc, rawCmdEnc []byte
	if err := st.pool.QueryRow(ctx, `SELECT host_meta_enc FROM hosts WHERE host_id = $1`, hostID).Scan(&rawHostEnc); err != nil {
		t.Fatal(err)
	}
	if err := st.pool.QueryRow(ctx, `SELECT ccache_enc FROM target_profiles WHERE host_id = $1`, hostID).Scan(&rawCCEnc); err != nil {
		t.Fatal(err)
	}
	if err := st.pool.QueryRow(ctx, `SELECT payload_enc FROM telemetry_snapshots WHERE host_id = $1 LIMIT 1`, hostID).Scan(&rawSnapEnc); err != nil {
		t.Fatal(err)
	}
	if err := st.pool.QueryRow(ctx, `SELECT detail_enc FROM audit_events WHERE host_id = $1 LIMIT 1`, hostID).Scan(&rawEvEnc); err != nil {
		t.Fatal(err)
	}
	if err := st.pool.QueryRow(ctx, `SELECT command_enc FROM operator_commands WHERE host_id = $1 LIMIT 1`, hostID).Scan(&rawCmdEnc); err != nil {
		t.Fatal(err)
	}
	for label, blob := range map[string][]byte{
		"hosts.host_meta_enc":              rawHostEnc,
		"target_profiles.ccache_enc":       rawCCEnc,
		"telemetry_snapshots.payload_enc":  rawSnapEnc,
		"audit_events.detail_enc":          rawEvEnc,
		"operator_commands.command_enc":    rawCmdEnc,
	} {
		if !bytes.HasPrefix(blob, []byte(envelope.Magic)) {
			t.Fatalf("%s missing SHUNA1 magic header", label)
		}
		for _, forbidden := range []string{"secret-workstation-alpha", "secret_operator", "secret_krb5cc_1000", "svc_secret@CORP.LOCAL", "sensitive-op01-json"} {
			if bytes.Contains(blob, []byte(forbidden)) {
				t.Fatalf("%s leaked plaintext %q", label, forbidden)
			}
		}
	}

	// AAD Tamper Test: swap payload_enc between snapshot_id 1 and 2 -> ListHistory MUST fail AEAD auth
	if _, err := st.pool.Exec(ctx, `
		UPDATE telemetry_snapshots SET payload_enc = $1 WHERE snapshot_id = 1
	`, rawSnapEnc); err != nil {
		t.Fatal(err)
	}
	var snap1ID int64
	var snap1Blob, snap2Blob []byte
	_ = st.pool.QueryRow(ctx, `SELECT snapshot_id, payload_enc FROM telemetry_snapshots ORDER BY snapshot_id ASC LIMIT 1`).Scan(&snap1ID, &snap1Blob)
	_ = st.pool.QueryRow(ctx, `SELECT payload_enc FROM telemetry_snapshots ORDER BY snapshot_id DESC LIMIT 1`).Scan(&snap2Blob)
	_, _ = st.pool.Exec(ctx, `UPDATE telemetry_snapshots SET payload_enc = $1 WHERE snapshot_id = $2`, snap2Blob, snap1ID)
	if _, _, err := st.ListHistory(ctx, hostID, "", 50); err == nil {
		t.Fatal("expected swapped BYTEA across snapshot rows to fail AAD verification")
	}
	// Restore snapshot 1
	_, _ = st.pool.Exec(ctx, `UPDATE telemetry_snapshots SET payload_enc = $1 WHERE snapshot_id = $2`, snap1Blob, snap1ID)

	// Insert 2,500 rows to exercise multi-batch (LIMIT 1000 FOR UPDATE SKIP LOCKED) online rotation
	for batch := 0; batch < 25; batch++ {
		var batchRows []Row
		for j := 0; j < 100; j++ {
			batchRows = append(batchRows, Row{
				ID: "OP-01", Operative: "Souei", Module: "keyring-probe",
				Target: fmt.Sprintf("uid=1000-%d-%d", batch, j), Detail: "ok",
				PermOrTTL: "perm=0x3f000000", SkewOrAux: "keys=2", ExitCode: 0, Status: "OK",
			})
		}
		if err := st.RecordFullSync(ctx, hostID, meta, "SYNCED", batchRows, nil); err != nil {
			t.Fatalf("seed batch %d: %v", batch, err)
		}
	}

	// Start online batched rotation while issuing concurrent RecordFullSync calls
	oldVer, newVer, err := st.StartRotation(ctx, []byte("rotated-pass-2"))
	if err != nil || oldVer != 1 || newVer != 2 {
		t.Fatalf("StartRotation failed: old=%d new=%d err=%v", oldVer, newVer, err)
	}

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()
		for k := 0; k < 5; k++ {
			_ = st.RecordFullSync(ctx, hostID, meta, "SYNCED", rows, []string{"concurrent-1", "concurrent-2"})
			time.Sleep(15 * time.Millisecond)
		}
	}()
	wg.Wait()

	rotDeadline := time.Now().Add(20 * time.Second)
	for st.IsRotating() && time.Now().Before(rotDeadline) {
		time.Sleep(50 * time.Millisecond)
	}
	if st.IsRotating() {
		t.Fatal("batched rotation did not complete within 20s")
	}

	// Verify zero rows remain on key_version < 2 across ALL encrypted tables and v1 is retired
	var rem int64
	if err := st.pool.QueryRow(ctx, `
		SELECT
			(SELECT COUNT(*) FROM hosts WHERE key_version < 2) +
			(SELECT COUNT(*) FROM target_profiles WHERE key_version < 2) +
			(SELECT COUNT(*) FROM telemetry_snapshots WHERE key_version < 2) +
			(SELECT COUNT(*) FROM audit_events WHERE key_version < 2) +
			(SELECT COUNT(*) FROM operator_commands WHERE key_version < 2) +
			(SELECT COUNT(*) FROM revoked_certs WHERE key_version < 2)
	`).Scan(&rem); err != nil || rem != 0 {
		t.Fatalf("expected 0 unrotated rows after vault_rotate, got %d (err=%v)", rem, err)
	}
	var retired bool
	if err := st.pool.QueryRow(ctx, `SELECT (retired_at IS NOT NULL AND NOT active AND NOT decrypt_only) FROM crypto_keys WHERE key_version = 1`).Scan(&retired); err != nil || !retired {
		t.Fatalf("expected crypto_keys v1 retired, got %v (err=%v)", retired, err)
	}

	// Export SHUNAS1 encrypted pg_dump backup and verify offline unseal
	backupPath := filepath.Join(t.TempDir(), "fleet.shunas1")
	nBytes, err := st.ExportBackup(ctx, backupPath)
	if err != nil || nBytes <= 32 {
		t.Fatalf("ExportBackup failed: bytes=%d err=%v", nBytes, err)
	}
	rawBackup, err := os.ReadFile(backupPath)
	if err != nil || !bytes.HasPrefix(rawBackup, []byte(envelope.StreamMagic)) {
		t.Fatalf("backup missing SHUNAS1 magic header: err=%v", err)
	}
	var unsealedSQL bytes.Buffer
	if err := st.vault.UnsealStream(bytes.NewReader(rawBackup), &unsealedSQL, []byte("ciel:pg_dump:v1")); err != nil {
		t.Fatalf("UnsealStream backup failed: %v", err)
	}
	if !strings.Contains(unsealedSQL.String(), "CREATE TABLE public.hosts") {
		t.Fatalf("unsealed pg_dump SQL missing hosts DDL")
	}
	st.Close()

	// Test persisted brute-force lockout across teamserver restart (NewStorage without passphrase -> 5 wrong unlocks)
	stLocked, err := NewStorage(ctx, pg.dsn, pkiDir, vaultDir, nil, false, false, true)
	if err != nil {
		t.Fatalf("reopen locked storage: %v", err)
	}
	if !stLocked.VaultLocked() {
		t.Fatal("expected reopened storage without passphrase to be locked")
	}
	for i := 0; i < 5; i++ {
		stepCtx, stepCancel := context.WithTimeout(context.Background(), 30*time.Second)
		err = stLocked.UnlockVault(stepCtx, []byte("wrong-pass"))
		stepCancel()
		if err == nil {
			t.Fatalf("expected wrong passphrase attempt %d to fail", i+1)
		}
	}
	if !errors.Is(err, vault.ErrVaultLocked) {
		t.Fatalf("expected 5th failure to return ErrVaultLocked, got %v", err)
	}
	stLocked.Close()

	// Restart storage again -> lockout MUST persist in PostgreSQL vault_lockout and reject valid passphrase!
	stRestarted, err := NewStorage(context.Background(), pg.dsn, pkiDir, vaultDir, nil, false, false, true)
	if err != nil {
		t.Fatalf("reopen after lockout: %v", err)
	}
	unlockCtx, unlockCancel := context.WithTimeout(context.Background(), 30*time.Second)
	err = stRestarted.UnlockVault(unlockCtx, []byte("rotated-pass-2"))
	unlockCancel()
	if !errors.Is(err, vault.ErrVaultLocked) {
		t.Fatalf("expected lockout to survive restart and block valid passphrase, got %v", err)
	}
	if ls := stRestarted.VaultLockoutStatus(); ls.FailedAttempts != 5 || !ls.WipedLocked {
		t.Fatalf("unexpected persisted lockout status: %+v", ls)
	}

	// Admin offline recovery reset clears persisted lockout
	recPriv, err := pqc.LoadRecoveryPrivateKey(filepath.Join(vaultDir, "recovery.key"))
	if err != nil {
		t.Fatalf("LoadRecoveryPrivateKey: %v", err)
	}
	defer recPriv.Zeroize()
	if err := stRestarted.vault.UnlockWithRecoveryKey(recPriv, []byte("recovered-pass-3")); err != nil {
		t.Fatalf("UnlockWithRecoveryKey: %v", err)
	}
	stRestarted.Close()

	recCtx, recCancel := context.WithTimeout(context.Background(), 30*time.Second)
	stRecovered, err := NewStorage(recCtx, pg.dsn, pkiDir, vaultDir, []byte("recovered-pass-3"), false, false, true)
	recCancel()
	if err != nil || stRecovered.VaultLocked() {
		t.Fatalf("expected storage unlockable after recovery reset: locked=%v err=%v", stRecovered != nil && stRecovered.VaultLocked(), err)
	}
	stRecovered.Close()
}

func TestMTLS13PQCAuthBindingAndCertRevocation(t *testing.T) {
	binDir := t.TempDir()
	tanukiBin := buildMockBinary(t, binDir)
	_ = os.WriteFile(filepath.Join(binDir, "libranga.so"), []byte("mock"), 0o755)
	workDir := t.TempDir()
	pkiDir := filepath.Join(workDir, "pki")

	srv := &Server{
		tempestDir:        binDir,
		tanukiBin:         tanukiBin,
		workDir:           workDir,
		selfExe:           tanukiBin,
		requireTLSBinding: true,
		connSem:           make(chan struct{}, 32),
		revokedSerials:    make(map[string]bool),
		activeConns:       make(map[net.Conn]peerConnInfo),
	}
	if err := srv.initPKI(pkiDir); err != nil {
		t.Fatalf("initPKI: %v", err)
	}
	if err := srv.initAuthToken(""); err != nil {
		t.Fatalf("initAuthToken: %v", err)
	}
	tlsCfg, err := srv.tlsConfig()
	if err != nil {
		t.Fatalf("tlsConfig: %v", err)
	}
	tcpLn, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	ln := tls.NewListener(tcpLn, tlsCfg)
	defer ln.Close()

	go func() {
		for {
			c, aerr := ln.Accept()
			if aerr != nil {
				return
			}
			go srv.serveConn(c)
		}
	}()

	addr := ln.Addr().String()
	caPath := filepath.Join(pkiDir, "ca.crt")
	cli1Cert := filepath.Join(pkiDir, "client.crt")
	cli1Key := filepath.Join(pkiDir, "client.key")

	// 1. Classical-only client (X25519 without X25519MLKEM768) must be rejected
	cli1TLS, err := mtls.ClientTLSConfig(cli1Cert, cli1Key, caPath, nil, mtls.TLSOptions{ServerName: "127.0.0.1"})
	if err != nil {
		t.Fatal(err)
	}
	classicalOnly := cli1TLS.Clone()
	classicalOnly.CurvePreferences = []tls.CurveID{tls.X25519}
	if badConn, derr := tls.Dial("tcp", addr, classicalOnly); derr == nil {
		var tmp [1]byte
		_, rerr := badConn.Read(tmp[:])
		_ = badConn.Close()
		if rerr == nil {
			t.Fatal("expected classical-only TLS client to be rejected")
		}
	}

	// 2. Valid X25519MLKEM768 mTLS client + bound auth_token succeeds
	conn1, err := tls.Dial("tcp", addr, cli1TLS)
	if err != nil {
		t.Fatalf("tls.Dial client1: %v", err)
	}
	defer conn1.Close()

	call := func(c net.Conn, req Request) Response {
		t.Helper()
		b, _ := json.Marshal(req)
		b = append(b, '\n')
		if _, werr := c.Write(b); werr != nil {
			t.Fatalf("write RPC %s: %v", req.Action, werr)
		}
		rd := bufio.NewReader(c)
		line, rerr := rd.ReadBytes('\n')
		if rerr != nil {
			t.Fatalf("read RPC %s: %v", req.Action, rerr)
		}
		var resp Response
		if jerr := json.Unmarshal(line, &resp); jerr != nil {
			t.Fatalf("unmarshal %s: %v", req.Action, jerr)
		}
		return resp
	}

	pingResp := call(conn1, Request{AuthToken: srv.authToken, Action: "ping"})
	if !pingResp.OK || pingResp.Data["pqc_group"] != "X25519MLKEM768" {
		t.Fatalf("unexpected ping response: %+v", pingResp)
	}

	// 3. Issue operator2 cert -> presenting operator1's auth_token MUST fail with UNAUTHORIZED (cert fingerprint binding)
	op2Issued, err := mtls.IssueCert(pkiDir, "operator2", mtls.RoleClient, nil, 24*time.Hour)
	if err != nil {
		t.Fatalf("IssueCert operator2: %v", err)
	}
	cli2TLS, err := mtls.ClientTLSConfig(op2Issued.CertPath, op2Issued.KeyPath, caPath, nil, mtls.TLSOptions{ServerName: "127.0.0.1"})
	if err != nil {
		t.Fatal(err)
	}
	conn2, err := tls.Dial("tcp", addr, cli2TLS)
	if err != nil {
		t.Fatalf("tls.Dial operator2: %v", err)
	}
	defer conn2.Close()

	stolenTokResp := call(conn2, Request{AuthToken: srv.authToken, Action: "ping"})
	if stolenTokResp.OK || stolenTokResp.Status != "UNAUTHORIZED" {
		t.Fatalf("expected stolen token from different mTLS cert to be UNAUTHORIZED, got %+v", stolenTokResp)
	}

	// Issue a dedicated session token bound to operator2's fingerprint
	issResp := call(conn1, Request{
		AuthToken: srv.authToken,
		Action:    "session_issue",
		CertFP:    op2Issued.Fingerprint,
		SerialHex: op2Issued.SerialHex,
	})
	if !issResp.OK {
		t.Fatalf("session_issue failed: %+v", issResp)
	}
	op2Token := issResp.Data["auth_token"].(string)
	op2Ping := call(conn2, Request{AuthToken: op2Token, Action: "ping"})
	if !op2Ping.OK {
		t.Fatalf("operator2 ping with bound token failed: %+v", op2Ping)
	}

	// Non-bootstrap operator2 must NOT be allowed to call session_issue
	unauthIssue := call(conn2, Request{
		AuthToken: op2Token,
		Action:    "session_issue",
		CertFP:    strings.Repeat("a", 64),
	})
	if unauthIssue.OK || unauthIssue.Status != "UNAUTHORIZED" || unauthIssue.ExitCode != 1 {
		t.Fatalf("expected non-bootstrap operator2 session_issue to fail with UNAUTHORIZED, got %+v", unauthIssue)
	}

	// 4. token_rotate issues new token while keeping old token valid during 30s grace
	oldTok := srv.authToken
	rotResp := call(conn1, Request{AuthToken: oldTok, Action: "token_rotate"})
	if !rotResp.OK {
		t.Fatalf("token_rotate failed: %+v", rotResp)
	}
	newTok := rotResp.Data["auth_token"].(string)
	if newTok == "" || newTok == oldTok {
		t.Fatalf("token_rotate did not produce new token")
	}
	if p := call(conn1, Request{AuthToken: oldTok, Action: "ping"}); !p.OK {
		t.Fatalf("old token rejected during 30s grace: %+v", p)
	}
	if p := call(conn1, Request{AuthToken: newTok, Action: "ping"}); !p.OK {
		t.Fatalf("new rotated token rejected: %+v", p)
	}

	// 5. cert_revoke immediately closes active conn2 socket AND rejects new connections from operator2
	revResp := call(conn1, Request{
		AuthToken: newTok,
		Action:    "cert_revoke",
		SerialHex: op2Issued.SerialHex,
		Reason:    "key_compromise",
	})
	if !revResp.OK || revResp.Status != "REVOKED" {
		t.Fatalf("cert_revoke failed: %+v", revResp)
	}

	// Active conn2 must see EOF/close immediately
	_ = conn2.SetReadDeadline(time.Now().Add(2 * time.Second))
	var drain [16]byte
	if _, rerr := io.ReadFull(conn2, drain[:]); rerr == nil {
		t.Fatal("expected active revoked socket conn2 to be closed by server")
	}

	// New handshake with operator2 cert must fail
	if conn2New, derr := tls.Dial("tcp", addr, cli2TLS); derr == nil {
		_ = conn2New.SetReadDeadline(time.Now().Add(2 * time.Second))
		_, rerr := conn2New.Read(drain[:])
		_ = conn2New.Close()
		if rerr == nil {
			t.Fatal("expected new connection with revoked operator2 cert to fail")
		}
	}
	_ = rand.Reader
}
