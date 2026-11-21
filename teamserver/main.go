package main

import (
	"bufio"
	"bytes"
	"context"
	"crypto/rand"
	"crypto/sha256"
	"crypto/subtle"
	"crypto/tls"
	"encoding/hex"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"os/user"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/Mafifrizi/shuna/pkg/contract"
	"github.com/Mafifrizi/shuna/pkg/kdf"
	"github.com/Mafifrizi/shuna/pkg/mtls"
	"github.com/Mafifrizi/shuna/pkg/vault"
)

const defaultTempestDir = "../tempest/bin"

var allowedTanukiSubcmds = map[string]bool{
	"doctor": true,
	"fix":    true,
	"keytab": true,
	"config": true,
	"triage": true,
	"ladder": true,
	"nhi":    true,
	"token":  true,
	"blast":  true,
	"adcs":   true,
	"pac":    true,
	"shadow": true,
	"kcm":    true,
	"auth":   true,
	"tgs":    true,
	"ldap":   true,
	"purge":  true,
	"skill":  true,
}

var systemKcmSocketCandidates = []string{
	"/run/.heim_org.h5l.kcm-socket",
	"/var/run/.heim_org.h5l.kcm-socket",
	"/var/run/sssd-kcm.socket",
	"/run/sssd-kcm.socket",
	"/var/lib/sss/pipes/kcm",
}

type Request struct {
	AuthToken      string   `json:"auth_token,omitempty"`
	Action         string   `json:"action"`
	CcachePath     string   `json:"ccache_path,omitempty"`
	KeytabPath     string   `json:"keytab_path,omitempty"`
	Krb5ConfPath   string   `json:"krb5_conf_path,omitempty"`
	KcmSocket      string   `json:"kcm_socket,omitempty"`
	WatchPath      string   `json:"watch_path,omitempty"`
	WatchTimeoutMs int      `json:"watch_timeout_ms,omitempty"`
	KdcTimeEpoch   int64    `json:"kdc_time_epoch,omitempty"`
	WarnWindowSec  int64    `json:"warn_window_sec,omitempty"`
	Renew          bool     `json:"renew,omitempty"`
	WatchInotify   bool     `json:"watch_inotify,omitempty"`
	IsolateMode    string   `json:"isolate_mode,omitempty"`
	PurgePath      string   `json:"purge_path,omitempty"`
	RevokeSession  bool     `json:"revoke_session,omitempty"`
	Subcommand     string   `json:"subcommand,omitempty"`
	TargetArg      string   `json:"target_arg,omitempty"`
	Args           []string `json:"args,omitempty"`
	HostID         string   `json:"host_id,omitempty"`
	OperativeID    string   `json:"operative_id,omitempty"`
	Limit          int      `json:"limit,omitempty"`
	Passphrase     string   `json:"passphrase,omitempty"`
	NewPassphrase  string   `json:"new_passphrase,omitempty"`
	SerialHex      string   `json:"serial_hex,omitempty"`
	Reason         string   `json:"reason,omitempty"`
	OutPath        string   `json:"out_path,omitempty"`
	CertFP         string   `json:"cert_fp,omitempty"`
}

type Row struct {
	ID        string `json:"id"`
	Operative string `json:"operative"`
	Module    string `json:"module"`
	Target    string `json:"target"`
	Detail    string `json:"detail"`
	PermOrTTL string `json:"perm_or_ttl"`
	SkewOrAux string `json:"skew_or_aux"`
	ExitCode  int    `json:"exit_code"`
	Status    string `json:"status"`
	RawJSON   any    `json:"raw_json,omitempty"`
}

type Response struct {
	OK        bool           `json:"ok"`
	Action    string         `json:"action"`
	ExitCode  int            `json:"exit_code"`
	Status    string         `json:"status"`
	Timestamp string         `json:"timestamp"`
	Error     string         `json:"error,omitempty"`
	Data      map[string]any `json:"data,omitempty"`
	Rows      []Row          `json:"rows,omitempty"`
	Logs      []string       `json:"logs,omitempty"`
}

const (
	defaultTokenTTL   = 12 * time.Hour
	tokenRotateGrace  = 30 * time.Second
)

type authSession struct {
	TokenHash  [32]byte
	CertFP     string
	CertSerial string
	IssuedAt   time.Time
	ExpiresAt  time.Time
}

type peerConnInfo struct {
	CertFP     string
	SerialHex  string
	CommonName string
	PQCGroup   string
}

type Server struct {
	tempestDir             string
	tanukiBin              string
	workDir                string
	selfExe                string
	kcmSockPath            string
	authToken              string
	tokenFile              string
	pkiDir                 string
	storage                *Storage
	crlMgr                 *mtls.CRLManager
	bootstrapCertFP        string
	bootstrapSerial        string
	requireTLSBinding      bool
	allowClassicalFallback bool
	connSem                chan struct{}
	mu                     sync.Mutex

	sessMu   sync.Mutex
	sessions []*authSession

	revokedMu      sync.RWMutex
	revokedSerials map[string]bool

	connsMu     sync.Mutex
	activeConns map[net.Conn]peerConnInfo
}

func validateCleanArg(s string, allowLeadingDash bool) error {
	if s == "" {
		return errors.New("value is empty")
	}
	if !allowLeadingDash && strings.HasPrefix(s, "-") {
		return errors.New("value must not begin with '-'")
	}
	for i := 0; i < len(s); i++ {
		c := s[i]
		if c < 0x20 || c == 0x7f {
			return fmt.Errorf("value contains control character 0x%02x", c)
		}
	}
	return nil
}

func validatePathInput(p string) error {
	if err := validateCleanArg(p, false); err != nil {
		return err
	}
	if strings.ContainsRune(p, 0) {
		return errors.New("path contains NUL byte")
	}
	return nil
}

func validatePurgeTarget(p string) error {
	if err := validatePathInput(p); err != nil {
		return err
	}
	cleaned := filepath.Clean(p)
	protected := map[string]bool{
		"/":            true,
		"/bin":         true,
		"/boot":        true,
		"/dev":         true,
		"/etc":         true,
		"/etc/group":   true,
		"/etc/gshadow": true,
		"/etc/passwd":  true,
		"/etc/shadow":  true,
		"/etc/sudoers": true,
		"/home":        true,
		"/lib":         true,
		"/lib64":       true,
		"/proc":        true,
		"/root":        true,
		"/run":         true,
		"/sbin":        true,
		"/sys":         true,
		"/usr":         true,
		"/var":         true,
	}
	if protected[cleaned] {
		return fmt.Errorf("refusing to purge protected system path: %s", cleaned)
	}
	protectedPrefixes := []string{
		"/bin/", "/boot/", "/dev/", "/lib/", "/lib64/",
		"/proc/", "/sbin/", "/sys/", "/usr/",
	}
	for _, pref := range protectedPrefixes {
		if strings.HasPrefix(cleaned, pref) {
			return fmt.Errorf("refusing to purge protected system path: %s", cleaned)
		}
	}
	return nil
}

func splitQuotedArgs(line string) ([]string, error) {
	var out []string
	var cur strings.Builder
	inSingle := false
	inDouble := false
	escaped := false
	hasToken := false

	for i := 0; i < len(line); i++ {
		ch := line[i]
		if escaped {
			cur.WriteByte(ch)
			escaped = false
			hasToken = true
			continue
		}
		if ch == '\\' && !inSingle {
			escaped = true
			hasToken = true
			continue
		}
		if ch == '\'' && !inDouble {
			inSingle = !inSingle
			hasToken = true
			continue
		}
		if ch == '"' && !inSingle {
			inDouble = !inDouble
			hasToken = true
			continue
		}
		if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') && !inSingle && !inDouble {
			if hasToken {
				out = append(out, cur.String())
				cur.Reset()
				hasToken = false
			}
			continue
		}
		cur.WriteByte(ch)
		hasToken = true
	}
	if escaped || inSingle || inDouble {
		return nil, errors.New("unterminated quote or escape in argument string")
	}
	if hasToken {
		out = append(out, cur.String())
	}
	return out, nil
}

func validateTanukiArgs(args []string) error {
	for i, a := range args {
		if err := validateCleanArg(a, true); err != nil {
			return fmt.Errorf("invalid argument at index %d: %w", i, err)
		}
		if a == "--interactive" {
			return errors.New("interactive TUI mode is not supported over RPC")
		}
		if a == "-i" && (i+1 >= len(args) || strings.HasPrefix(args[i+1], "-")) {
			return errors.New("interactive -i flag is not supported over RPC")
		}
	}
	return nil
}

func (s *Server) resolveKcmSocket(explicit string) string {
	cand := strings.TrimSpace(explicit)
	if cand == "" {
		cand = strings.TrimSpace(s.kcmSockPath)
	}
	if cand != "" {
		return cand
	}
	for _, p := range systemKcmSocketCandidates {
		if _, err := os.Stat(p); err == nil {
			return p
		}
	}
	return ""
}

func parseCCacheSpec(raw string) (string, string, error) {
	cand := strings.TrimSpace(raw)
	if strings.HasPrefix(cand, "FILE:") {
		p := strings.TrimPrefix(cand, "FILE:")
		if err := validatePathInput(p); err != nil {
			return "", "", err
		}
		return p, "FILE", nil
	}
	if strings.HasPrefix(cand, "/") {
		if err := validatePathInput(cand); err != nil {
			return "", "", err
		}
		return cand, "FILE", nil
	}
	if idx := strings.IndexByte(cand, ':'); idx > 0 {
		scheme := strings.ToUpper(cand[:idx])
		if err := validateCleanArg(cand, false); err != nil {
			return "", "", err
		}
		return cand, scheme, nil
	}
	if err := validatePathInput(cand); err != nil {
		return "", "", err
	}
	return cand, "FILE", nil
}

func resolveCCachePath(explicit string) (string, string, error) {
	if cand := strings.TrimSpace(explicit); cand != "" {
		return parseCCacheSpec(cand)
	}
	if env := strings.TrimSpace(os.Getenv("KRB5CCNAME")); env != "" {
		return parseCCacheSpec(env)
	}
	return fmt.Sprintf("/tmp/krb5cc_%d", os.Getuid()), "FILE", nil
}

func resolveKrb5ConfPath(explicit string) string {
	if c := strings.TrimSpace(explicit); c != "" {
		return c
	}
	if env := strings.TrimSpace(os.Getenv("KRB5_CONFIG")); env != "" {
		return env
	}
	return "/etc/krb5.conf"
}

func resolveKeytabPath(explicit string) string {
	if k := strings.TrimSpace(explicit); k != "" {
		return k
	}
	if env := strings.TrimSpace(os.Getenv("KRB5_KTNAME")); env != "" {
		return strings.TrimPrefix(env, "FILE:")
	}
	return "/etc/krb5.keytab"
}

func sanitizedChildEnv(extraEnv ...string) []string {
	base := os.Environ()
	out := make([]string, 0, len(base)+len(extraEnv))
	for _, kv := range base {
		if strings.HasPrefix(kv, "CIEL_AUTH_TOKEN=") {
			continue
		}
		out = append(out, kv)
	}
	out = append(out, extraEnv...)
	return out
}

func (s *Server) runJSONProcess(binPath string, extraEnv []string, args ...string) (map[string]any, int, string, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 15*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, binPath, args...)
	if s.workDir != "" {
		cmd.Dir = s.workDir
	}
	cmd.Env = sanitizedChildEnv(extraEnv...)
	var stdout, stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr

	err := cmd.Run()
	exitCode := 0
	if err != nil {
		var ee *exec.ExitError
		if errors.As(err, &ee) {
			exitCode = ee.ExitCode()
		} else {
			return nil, -1, stderr.String(), err
		}
	}

	var parsed map[string]any
	trimmed := bytes.TrimSpace(stdout.Bytes())
	if len(trimmed) > 0 {
		if trimmed[0] == '[' {
			var arr []any
			if jErr := json.Unmarshal(trimmed, &arr); jErr != nil {
				return nil, exitCode, stdout.String() + " " + stderr.String(), jErr
			}
			parsed = map[string]any{
				"items": arr,
				"count": len(arr),
			}
		} else if jErr := json.Unmarshal(trimmed, &parsed); jErr != nil {
			return nil, exitCode, stdout.String() + " " + stderr.String(), jErr
		}
	}
	return parsed, exitCode, stderr.String(), nil
}

func (s *Server) runBinaryJSON(binName string, extraEnv []string, args ...string) (map[string]any, int, string, error) {
	return s.runJSONProcess(filepath.Join(s.tempestDir, binName), extraEnv, args...)
}

func strField(m map[string]any, key string) string {
	if m == nil {
		return ""
	}
	if v, ok := m[key]; ok {
		return fmt.Sprintf("%v", v)
	}
	return ""
}

func intField(m map[string]any, key string) int64 {
	if m == nil {
		return 0
	}
	switch v := m[key].(type) {
	case float64:
		return int64(v)
	case int:
		return int64(v)
	case int64:
		return v
	case int32:
		return int64(v)
	case uint:
		return int64(v)
	case uint64:
		return int64(v)
	case json.Number:
		if n, err := v.Int64(); err == nil {
			return n
		}
		if f, err := v.Float64(); err == nil {
			return int64(f)
		}
	}
	return 0
}

func boolField(m map[string]any, key string) bool {
	if m == nil {
		return false
	}
	if v, ok := m[key].(bool); ok {
		return v
	}
	return false
}

func errRow(id, op, mod, target string, rc int, err error) Row {
	return Row{
		ID:        id,
		Operative: op,
		Module:    mod,
		Target:    target,
		Detail:    "execution failed",
		PermOrTTL: "-",
		SkewOrAux: err.Error(),
		ExitCode:  rc,
		Status:    "ERROR",
	}
}

func (s *Server) execSoueiProbe(req Request, watchInotify bool) (Row, map[string]any, int, string) {
	args := []string{"--probe"}
	kcmSock := s.resolveKcmSocket(req.KcmSocket)
	if kcmSock != "" {
		if err := validatePathInput(kcmSock); err != nil {
			return errRow("OP-01", "Souei", "keyring-probe", kcmSock, 1, err),
				nil, 1, fmt.Sprintf("[SOUEI] invalid kcm_socket: %v", err)
		}
		args = append(args, "--kcm-socket", kcmSock)
	}

	modName := "keyring-probe"
	if watchInotify {
		modName = "inotify-watch"
		watchTarget := strings.TrimSpace(req.WatchPath)
		if watchTarget == "" {
			ccPath, scheme, err := resolveCCachePath(req.CcachePath)
			if err == nil && scheme == "FILE" {
				watchTarget = ccPath
			}
		}
		if watchTarget == "" {
			err := errors.New("watch_path is required when no FILE ccache path is available")
			return errRow("OP-01", "Souei", modName, "-", 1, err),
				nil, 1, fmt.Sprintf("[SOUEI] %v", err)
		}
		if err := validatePathInput(watchTarget); err != nil {
			return errRow("OP-01", "Souei", modName, watchTarget, 1, err),
				nil, 1, fmt.Sprintf("[SOUEI] invalid watch_path: %v", err)
		}
		timeoutMs := req.WatchTimeoutMs
		if timeoutMs <= 0 {
			timeoutMs = 500
		} else if timeoutMs < 100 {
			timeoutMs = 100
		} else if timeoutMs > 10000 {
			timeoutMs = 10000
		}
		args = append(args, "--watch-file", watchTarget, "--timeout-ms", strconv.Itoa(timeoutMs))
	}

	args = append(args, "--json")
	data, exitCode, stderrStr, err := s.runBinaryJSON("souei", nil, args...)
	if err != nil {
		return errRow("OP-01", "Souei", modName, "/proc/keys", exitCode, err),
			nil, exitCode, fmt.Sprintf("[SOUEI] error executing souei: %v (%s)", err, stderrStr)
	}

	sessID := intField(data, "session_keyring_id")
	permHex := strField(data, "session_perm_hex")
	iso := boolField(data, "isolated_session_keyring")
	entries := intField(data, "keyring_entries_count")
	status := strField(data, "status")

	kcmChecked := false
	kcmConn := false
	var peerPid int64
	if kcm, ok := data["kcm_socket"].(map[string]any); ok && kcm != nil {
		kcmChecked = boolField(kcm, "checked")
		kcmConn = boolField(kcm, "connected")
		peerPid = intField(kcm, "peer_pid")
	}
	inoCount := 0
	if ino, ok := data["inotify"].(map[string]any); ok && ino != nil {
		if evs, ok := ino["events"].([]any); ok {
			inoCount = len(evs)
		}
	}

	kcmLabel := "none"
	if kcmChecked {
		kcmLabel = fmt.Sprintf("%v", kcmConn)
	}
	detail := fmt.Sprintf("keyring=%d isolated=%v kcm=%s", sessID, iso, kcmLabel)
	aux := fmt.Sprintf("keys=%d", entries)
	if kcmChecked {
		aux = fmt.Sprintf("keys=%d kcm_pid=%d", entries, peerPid)
	}
	if watchInotify {
		detail = fmt.Sprintf("inotify_events=%d kcm=%s", inoCount, kcmLabel)
		aux = fmt.Sprintf("ino=%d", inoCount)
		if kcmChecked {
			aux = fmt.Sprintf("ino=%d kcm_pid=%d", inoCount, peerPid)
		}
	}

	row := Row{
		ID:        "OP-01",
		Operative: "Souei",
		Module:    modName,
		Target:    fmt.Sprintf("uid=%d", intField(data, "uid")),
		Detail:    detail,
		PermOrTTL: fmt.Sprintf("perm=0x%s", permHex),
		SkewOrAux: aux,
		ExitCode:  exitCode,
		Status:    status,
		RawJSON:   data,
	}
	logLine := fmt.Sprintf("[SOUEI] status=%s exit=%d session_keyring=%d perm=0x%s entries=%d kcm=%s peer_pid=%d inotify_events=%d",
		status, exitCode, sessID, permHex, entries, kcmLabel, peerPid, inoCount)
	return row, data, exitCode, logLine
}

func runKinitRenew(ccPath, scheme, krb5Conf string) (int, string, error) {
	kinitBin, err := exec.LookPath("kinit")
	if err != nil {
		return 3, "", errors.New("kinit binary not found on PATH (install krb5-user)")
	}
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, kinitBin, "-R")
	ccVal := ccPath
	if scheme == "FILE" && !strings.HasPrefix(ccVal, "FILE:") {
		ccVal = "FILE:" + ccPath
	}
	extraEnv := []string{"KRB5CCNAME=" + ccVal}
	if strings.TrimSpace(krb5Conf) != "" {
		extraEnv = append(extraEnv, "KRB5_CONFIG="+strings.TrimSpace(krb5Conf))
	}
	cmd.Env = sanitizedChildEnv(extraEnv...)
	var stderr bytes.Buffer
	cmd.Stderr = &stderr

	runErr := cmd.Run()
	if runErr != nil {
		var ee *exec.ExitError
		if errors.As(runErr, &ee) {
			return ee.ExitCode(), strings.TrimSpace(stderr.String()), runErr
		}
		return -1, strings.TrimSpace(stderr.String()), runErr
	}
	return 0, strings.TrimSpace(stderr.String()), nil
}

func (s *Server) execChronoaCheck(req Request) (Row, map[string]any, int, string) {
	ccPath, scheme, err := resolveCCachePath(req.CcachePath)
	if err != nil {
		return errRow("OP-02", "Chronoa", "ccache-watchdog", req.CcachePath, 1, err),
			nil, 1, fmt.Sprintf("[CHRONOA] invalid ccache_path: %v", err)
	}

	var krb5Conf string
	if strings.TrimSpace(req.Krb5ConfPath) != "" {
		if err := validatePathInput(strings.TrimSpace(req.Krb5ConfPath)); err != nil {
			return errRow("OP-02", "Chronoa", "ccache-watchdog", req.Krb5ConfPath, 1, err),
				nil, 1, fmt.Sprintf("[CHRONOA] invalid krb5_conf_path: %v", err)
		}
		krb5Conf = strings.TrimSpace(req.Krb5ConfPath)
	}

	renewAttempted := false
	renewExit := -1
	renewErrMsg := ""
	renewed := false
	if req.Renew {
		renewAttempted = true
		rc, rStderr, rErr := runKinitRenew(ccPath, scheme, krb5Conf)
		renewExit = rc
		if rErr != nil {
			if rStderr != "" {
				renewErrMsg = rStderr
			} else {
				renewErrMsg = rErr.Error()
			}
		} else {
			renewed = true
		}
	}

	warnWin := req.WarnWindowSec
	if warnWin <= 0 {
		warnWin = 300
	}

	args := []string{}
	var chronoaEnv []string
	if scheme == "FILE" {
		args = append(args, "--ccache", ccPath)
	} else {
		chronoaEnv = append(chronoaEnv, "KRB5CCNAME="+ccPath)
	}
	if req.KdcTimeEpoch > 0 {
		args = append(args, "--kdc-time", strconv.FormatInt(req.KdcTimeEpoch, 10))
	}
	args = append(args,
		"--warn-window-sec", strconv.FormatInt(warnWin, 10),
		"--once",
		"--json",
	)

	data, exitCode, stderrStr, execErr := s.runBinaryJSON("chronoa", chronoaEnv, args...)
	if execErr != nil {
		return errRow("OP-02", "Chronoa", "ccache-watchdog", ccPath, exitCode, execErr),
			nil, exitCode, fmt.Sprintf("[CHRONOA] error executing chronoa: %v (%s)", execErr, stderrStr)
	}
	if data == nil {
		data = map[string]any{}
	}

	if renewAttempted {
		data["renew_attempted"] = true
		data["renew_exit_code"] = renewExit
		if renewErrMsg != "" {
			data["renew_error"] = renewErrMsg
		}
		if renewed && exitCode == 0 {
			data["renewed"] = true
			data["status"] = "RENEWED"
		} else if !renewed {
			data["renewed"] = false
			if exitCode == 0 {
				exitCode = 2
				data["exit_code"] = 2
				data["status"] = "RENEW_FAILED"
			}
		}
	}

	princ := strField(data, "default_principal")
	if princ == "" {
		princ = ccPath
	}
	encName := "-"
	if pt, ok := data["primary_ticket"].(map[string]any); ok && pt != nil {
		encName = strField(pt, "enctype_name")
	}
	ttl := intField(data, "ttl_sec")
	skew := intField(data, "skew_sec")
	status := strField(data, "status")
	if status == "" {
		status = "ERROR"
	}

	permOrTTL := fmt.Sprintf("ttl=%ds", ttl)
	if status == "MISSING" || status == "NO_CREDENTIALS" {
		permOrTTL = "ttl=-"
	}

	detail := encName
	if renewErrMsg != "" {
		if detail == "" || detail == "-" {
			detail = "renew failed"
		} else {
			detail += " (renew failed)"
		}
	}

	row := Row{
		ID:        "OP-02",
		Operative: "Chronoa",
		Module:    "ccache-watchdog",
		Target:    princ,
		Detail:    detail,
		PermOrTTL: permOrTTL,
		SkewOrAux: fmt.Sprintf("skew=%ds", skew),
		ExitCode:  exitCode,
		Status:    status,
		RawJSON:   data,
	}
	logLine := fmt.Sprintf("[CHRONOA] status=%s exit=%d principal=%s enctype=%s ttl=%ds skew=%ds renewed=%v",
		status, exitCode, princ, encName, ttl, skew, boolField(data, "renewed"))
	if renewErrMsg != "" {
		logLine += fmt.Sprintf(" renew_error=%q", renewErrMsg)
	}
	return row, data, exitCode, logLine
}

func (s *Server) execRangaStatus(isolate bool, mode string) (Row, map[string]any, int, string) {
	var extraEnv []string
	rowID := "OP-03"
	modName := "keyring-baseline"
	args := []string{"--status", "--json"}
	if isolate {
		rowID = "OP-04"
		if mode == "wrapper" {
			modName = "wrapper-isolate"
			selfBin := filepath.Join(s.tempestDir, "ranga")
			args = []string{"--isolate", "--", selfBin, "--status", "--json"}
		} else {
			modName = "ldpreload-isolate"
			libPath := filepath.Join(s.tempestDir, "libranga.so")
			extraEnv = []string{
				"LD_PRELOAD=" + libPath,
				"RANGA_REVOKE_ON_EXIT=1",
			}
		}
	}

	data, exitCode, stderrStr, err := s.runBinaryJSON("ranga", extraEnv, args...)
	if err != nil {
		return errRow(rowID, "Ranga", modName, "session-keyring", exitCode, err),
			nil, exitCode, fmt.Sprintf("[RANGA] error executing ranga: %v (%s)", err, stderrStr)
	}

	kid := intField(data, "session_keyring_id")
	permHex := strField(data, "perm_hex")
	possOnly := boolField(data, "possessor_only")
	desc := strField(data, "description")
	status := "SHARED_KEYRING"
	if possOnly {
		status = "ISOLATED"
	} else if exitCode != 0 {
		status = "ERROR"
	}

	row := Row{
		ID:        rowID,
		Operative: "Ranga",
		Module:    modName,
		Target:    fmt.Sprintf("kid=%d (%s)", kid, desc),
		Detail:    fmt.Sprintf("possessor_only=%v", possOnly),
		PermOrTTL: fmt.Sprintf("perm=0x%s", permHex),
		SkewOrAux: fmt.Sprintf("keys=%d", intField(data, "child_key_count")),
		ExitCode:  exitCode,
		Status:    status,
		RawJSON:   data,
	}
	logLine := fmt.Sprintf("[RANGA:%s] status=%s exit=%d keyring_id=%d perm=0x%s possessor_only=%v",
		modName, status, exitCode, kid, permHex, possOnly)
	return row, data, exitCode, logLine
}

func (s *Server) execRangaPurge(req Request) (Row, map[string]any, int, string) {
	target := strings.TrimSpace(req.PurgePath)
	if err := validatePurgeTarget(target); err != nil {
		return Row{
				ID:        "OP-05",
				Operative: "Ranga",
				Module:    "nist-purge",
				Target:    target,
				Detail:    "validation failed",
				PermOrTTL: "-",
				SkewOrAux: err.Error(),
				ExitCode:  1,
				Status:    "USAGE_ERROR",
			},
			nil, 1, fmt.Sprintf("[RANGA:purge] rejected target: %v", err)
	}

	args := []string{"--purge", target}
	if req.RevokeSession {
		args = append(args, "--revoke-session")
	}
	args = append(args, "--json")

	data, exitCode, stderrStr, err := s.runBinaryJSON("ranga", nil, args...)
	if err != nil {
		return errRow("OP-05", "Ranga", "nist-purge", target, exitCode, err),
			nil, exitCode, fmt.Sprintf("[RANGA:purge] error executing ranga: %v (%s)", err, stderrStr)
	}

	removed := boolField(data, "removed")
	passes := intField(data, "passes")
	wiped := intField(data, "wiped_bytes")
	status := "PURGE_FAILED"
	if exitCode == 0 && removed {
		status = "PURGED"
	} else if exitCode == 3 {
		status = "MISSING"
	} else if exitCode == 2 {
		status = "POLICY_STOP"
	}

	row := Row{
		ID:        "OP-05",
		Operative: "Ranga",
		Module:    "nist-purge",
		Target:    target,
		Detail:    fmt.Sprintf("passes=%d removed=%v", passes, removed),
		PermOrTTL: fmt.Sprintf("wiped=%dB", wiped),
		SkewOrAux: fmt.Sprintf("revoked=%v", boolField(data, "session_revoked")),
		ExitCode:  exitCode,
		Status:    status,
		RawJSON:   data,
	}
	logLine := fmt.Sprintf("[RANGA:purge] status=%s exit=%d path=%s passes=%d wiped_bytes=%d removed=%v",
		status, exitCode, target, passes, wiped, removed)
	return row, data, exitCode, logLine
}

func hasFlag(args []string, flags ...string) bool {
	for _, a := range args {
		for _, f := range flags {
			if a == f || strings.HasPrefix(a, f+"=") {
				return true
			}
		}
	}
	return false
}

func (s *Server) buildTanukiArgs(subcmd string, req Request, includeOpsec bool) ([]string, string, error) {
	arg := strings.TrimSpace(req.TargetArg)
	if arg != "" {
		if err := validateCleanArg(arg, true); err != nil {
			return nil, arg, err
		}
		if err := validateTanukiArgs([]string{arg}); err != nil {
			return nil, arg, err
		}
	}

	kt := strings.TrimSpace(req.KeytabPath)
	if kt != "" {
		if err := validatePathInput(kt); err != nil {
			return nil, kt, err
		}
	}
	cc := strings.TrimSpace(req.CcachePath)
	if cc != "" {
		if strings.HasPrefix(cc, "FILE:") {
			cc = strings.TrimPrefix(cc, "FILE:")
		}
		if err := validatePathInput(cc); err != nil {
			return nil, cc, err
		}
	}
	krb5Conf := strings.TrimSpace(req.Krb5ConfPath)
	if krb5Conf != "" {
		if err := validatePathInput(krb5Conf); err != nil {
			return nil, krb5Conf, err
		}
	}
	kcmSock := strings.TrimSpace(req.KcmSocket)
	if kcmSock != "" {
		if err := validatePathInput(kcmSock); err != nil {
			return nil, kcmSock, err
		}
	}

	if len(req.Args) > 0 {
		if err := validateTanukiArgs(req.Args); err != nil {
			return nil, strings.Join(req.Args, " "), err
		}
		out := []string{subcmd}
		if subcmd == "fix" {
			applyMode := hasFlag(req.Args, "--apply")
			dryRunMode := hasFlag(req.Args, "--dry-run")
			for _, a := range req.Args {
				if a == "--apply" {
					continue
				}
				out = append(out, a)
			}
			if !dryRunMode && !applyMode {
				out = append(out, "--dry-run")
			}
		} else {
			out = append(out, req.Args...)
		}
		switch subcmd {
		case "doctor":
			if kt != "" && !hasFlag(req.Args, "--keytab", "-k") {
				out = append(out, "--keytab", kt)
			}
			if cc != "" && !hasFlag(req.Args, "--ccache", "-c") {
				out = append(out, "--ccache", cc)
			}
			if krb5Conf != "" && !hasFlag(req.Args, "--krb5-conf") {
				out = append(out, "--krb5-conf", krb5Conf)
			}
			if kcmSock != "" && !hasFlag(req.Args, "--sssd-pipe") {
				out = append(out, "--sssd-pipe", kcmSock)
			}
		case "fix":
			if kt != "" && !hasFlag(req.Args, "--keytab", "-k") {
				out = append(out, "--keytab", kt)
			}
			if cc != "" && !hasFlag(req.Args, "--ccache", "-c") {
				out = append(out, "--ccache", cc)
			}
			if krb5Conf != "" && !hasFlag(req.Args, "--krb5-conf") {
				out = append(out, "--krb5-conf", krb5Conf)
			}
		case "config":
			if !hasFlag(req.Args, "-o", "--out", "--stdout") {
				out = append(out, "--stdout")
			}
		case "kcm":
			if !hasFlag(req.Args, "-o", "--out") {
				out = append(out, "-o", filepath.Join(s.workDir, "kcm_out"))
			}
		case "purge":
			for _, a := range req.Args {
				if a == "--all" {
					continue
				}
				if strings.HasPrefix(a, "-") {
					if eq := strings.IndexByte(a, '='); eq > 0 {
						if err := validatePurgeTarget(a[eq+1:]); err != nil {
							return nil, a[eq+1:], err
						}
					}
					continue
				}
				if err := validatePurgeTarget(a); err != nil {
					return nil, a, err
				}
			}
		case "blast":
			if kt != "" && !hasFlag(req.Args, "--keytab", "-k") {
				out = append(out, "--keytab", kt)
			}
			if cc != "" && !hasFlag(req.Args, "--ccache", "-c") {
				out = append(out, "--ccache", cc)
			}
		case "auth":
			if kt != "" && !hasFlag(req.Args, "--keytab", "-k") {
				out = append(out, "--keytab", kt)
			}
			if krb5Conf != "" && !hasFlag(req.Args, "--krb5-conf") {
				out = append(out, "--krb5-conf", krb5Conf)
			}
		case "tgs":
			if cc != "" && !hasFlag(req.Args, "--ccache", "-c") {
				out = append(out, "--ccache", cc)
			}
			if krb5Conf != "" && !hasFlag(req.Args, "--krb5-conf") {
				out = append(out, "--krb5-conf", krb5Conf)
			}
		}
		if !hasFlag(req.Args, "--json", "-j") {
			out = append(out, "--json")
		}
		return out, strings.Join(req.Args, " "), nil
	}

	switch subcmd {
	case "doctor":
		out := []string{"doctor"}
		if kt != "" {
			out = append(out, "--keytab", kt)
		}
		if cc != "" {
			out = append(out, "--ccache", cc)
		}
		if krb5Conf != "" {
			out = append(out, "--krb5-conf", krb5Conf)
		}
		if kcmSock != "" {
			out = append(out, "--sssd-pipe", kcmSock)
		}
		if includeOpsec {
			out = append(out, "--opsec")
		}
		out = append(out, "--json")
		lbl := "host"
		if kt != "" || cc != "" || krb5Conf != "" {
			lbl = "configured"
		}
		return out, lbl, nil

	case "fix":
		out := []string{"fix", "--dry-run"}
		if kt != "" {
			out = append(out, "--keytab", kt)
		}
		if cc != "" {
			out = append(out, "--ccache", cc)
		}
		if krb5Conf != "" {
			out = append(out, "--krb5-conf", krb5Conf)
		}
		out = append(out, "--json")
		return out, "dry-run", nil

	case "keytab":
		t := arg
		if t == "" {
			t = kt
		}
		if t == "" {
			t = resolveKeytabPath("")
		}
		if err := validatePathInput(t); err != nil {
			return nil, t, err
		}
		return []string{"keytab", t, "--json"}, filepath.Base(t), nil

	case "config":
		if arg == "" {
			return nil, "-", errors.New("config requires explicit arguments (e.g. --realm CORP.LOCAL --kdc 192.168.56.106)")
		}
		parts, err := splitQuotedArgs(arg)
		if err != nil {
			return nil, arg, err
		}
		if err := validateTanukiArgs(parts); err != nil {
			return nil, arg, err
		}
		out := append([]string{"config"}, parts...)
		if !hasFlag(parts, "-o", "--out", "--stdout") {
			out = append(out, "--stdout")
		}
		if !hasFlag(parts, "--json", "-j") {
			out = append(out, "--json")
		}
		return out, arg, nil

	case "triage":
		if arg != "" {
			if strings.HasPrefix(arg, "-") {
				return nil, arg, errors.New("triage query must not start with '-'")
			}
			return []string{"triage", arg, "--json"}, arg, nil
		}
		return []string{"triage", "--json"}, "all", nil

	case "ladder":
		return []string{"ladder", "--json"}, "5-rungs", nil

	case "nhi":
		if arg != "" {
			parts, err := splitQuotedArgs(arg)
			if err != nil {
				return nil, arg, err
			}
			if err := validateTanukiArgs(parts); err != nil {
				return nil, arg, err
			}
			out := append([]string{"nhi"}, parts...)
			if !hasFlag(parts, "--json", "-j") {
				out = append(out, "--json")
			}
			return out, arg, nil
		}
		return []string{"nhi", "scan", "--json"}, "scan", nil

	case "token":
		if arg == "" {
			return nil, "-", errors.New("token path or JWT string is required")
		}
		if strings.HasPrefix(arg, "-") {
			return nil, arg, errors.New("token path/value must not start with '-'")
		}
		return []string{"token", arg, "--json"}, filepath.Base(arg), nil

	case "blast":
		if arg != "" {
			parts, err := splitQuotedArgs(arg)
			if err != nil {
				return nil, arg, err
			}
			if err := validateTanukiArgs(parts); err != nil {
				return nil, arg, err
			}
			out := append([]string{"blast"}, parts...)
			if kt != "" && !hasFlag(parts, "--keytab", "-k") {
				out = append(out, "--keytab", kt)
			}
			if cc != "" && !hasFlag(parts, "--ccache", "-c") {
				out = append(out, "--ccache", cc)
			}
			if !hasFlag(parts, "--json", "-j") {
				out = append(out, "--json")
			}
			return out, arg, nil
		}
		out := []string{"blast"}
		if kt != "" {
			out = append(out, "--keytab", kt)
		}
		if cc != "" {
			out = append(out, "--ccache", cc)
		}
		if kt == "" && cc == "" {
			return nil, "-", errors.New("blast requires target principal/source, configured keytab, or ccache")
		}
		out = append(out, "--json")
		return out, "configured", nil

	case "adcs":
		if arg == "" {
			return nil, "-", errors.New("adcs templates JSON/LDIF or certificate file path is required")
		}
		if err := validatePathInput(arg); err != nil {
			return nil, arg, err
		}
		return []string{"adcs", "-f", arg, "--json"}, filepath.Base(arg), nil

	case "pac":
		if arg == "" {
			return nil, "-", errors.New("pac file path or hex/base64 payload is required")
		}
		if strings.HasPrefix(arg, "-") {
			return nil, arg, errors.New("pac path/payload must not start with '-'")
		}
		return []string{"pac", arg, "--json"}, filepath.Base(arg), nil

	case "shadow":
		if arg == "" {
			return nil, "-", errors.New("shadow credential file path, hex, or DN-Binary input is required")
		}
		if strings.HasPrefix(arg, "-") {
			return nil, arg, errors.New("shadow input must not start with '-'")
		}
		return []string{"shadow", arg, "--json"}, "keyCredLink", nil

	case "kcm":
		outDir := filepath.Join(s.workDir, "kcm_out")
		if arg != "" {
			if err := validatePathInput(arg); err != nil {
				return nil, arg, err
			}
			return []string{"kcm", "-f", arg, "-o", outDir, "--json"}, filepath.Base(arg), nil
		}
		return []string{"kcm", "-o", outDir, "--json"}, "local-kcm", nil

	case "purge":
		if arg == "" {
			return nil, "-", errors.New("purge target file path or --all is required")
		}
		if arg == "--all" {
			return []string{"purge", "--all", "--json"}, "--all", nil
		}
		if err := validatePurgeTarget(arg); err != nil {
			return nil, arg, err
		}
		return []string{"purge", arg, "--json"}, filepath.Base(arg), nil

	case "skill":
		return []string{"skill", "--json"}, "manifest", nil

	default:
		out := []string{subcmd}
		var parts []string
		if arg != "" {
			var err error
			parts, err = splitQuotedArgs(arg)
			if err != nil {
				return nil, arg, err
			}
			if err := validateTanukiArgs(parts); err != nil {
				return nil, arg, err
			}
			out = append(out, parts...)
		}
		if subcmd == "auth" {
			if kt != "" && !hasFlag(parts, "--keytab", "-k") {
				out = append(out, "--keytab", kt)
			}
			if krb5Conf != "" && !hasFlag(parts, "--krb5-conf") {
				out = append(out, "--krb5-conf", krb5Conf)
			}
		}
		if subcmd == "tgs" {
			if cc != "" && !hasFlag(parts, "--ccache", "-c") {
				out = append(out, "--ccache", cc)
			}
			if krb5Conf != "" && !hasFlag(parts, "--krb5-conf") {
				out = append(out, "--krb5-conf", krb5Conf)
			}
		}
		if !hasFlag(parts, "--json", "-j") {
			out = append(out, "--json")
		}
		return out, arg, nil
	}
}

func (s *Server) execTanuki(req Request) (Row, map[string]any, int, string) {
	return s.execTanukiWithOpsec(req, true)
}

func (s *Server) execTanukiWithOpsec(req Request, includeOpsec bool) (Row, map[string]any, int, string) {
	subcmd := strings.ToLower(strings.TrimSpace(req.Subcommand))
	if subcmd == "" {
		subcmd = "doctor"
	}
	if !allowedTanukiSubcmds[subcmd] {
		err := fmt.Errorf("unknown or disallowed tanuki subcommand: %s", subcmd)
		return Row{
				ID:        "OP-06",
				Operative: "Tanuki",
				Module:    subcmd,
				Target:    "-",
				Detail:    "validation failed",
				PermOrTTL: "tanuki-nhi",
				SkewOrAux: err.Error(),
				ExitCode:  1,
				Status:    "USAGE_ERROR",
			},
			nil, 1, fmt.Sprintf("[TANUKI:%s] rejected: %v", subcmd, err)
	}
	if s.tanukiBin == "" {
		err := errors.New("tanuki binary not found")
		return errRow("OP-06", "Tanuki", subcmd, "-", 3, err),
			nil, 3, "[TANUKI:" + subcmd + "] error: tanuki binary not found"
	}

	args, targetLabel, buildErr := s.buildTanukiArgs(subcmd, req, includeOpsec)
	if buildErr != nil {
		return Row{
				ID:        "OP-06",
				Operative: "Tanuki",
				Module:    subcmd,
				Target:    targetLabel,
				Detail:    "usage error",
				PermOrTTL: "tanuki-nhi",
				SkewOrAux: buildErr.Error(),
				ExitCode:  1,
				Status:    "USAGE_ERROR",
			},
			nil, 1, fmt.Sprintf("[TANUKI:%s] %v", subcmd, buildErr)
	}

	data, exitCode, stderrStr, err := s.runJSONProcess(s.tanukiBin, nil, args...)
	if err != nil {
		return errRow("OP-06", "Tanuki", subcmd, targetLabel, exitCode, err),
			nil, exitCode, fmt.Sprintf("[TANUKI:%s] error: %v (%s)", subcmd, err, stderrStr)
	}

	status := strings.ToUpper(strField(data, "status"))
	if status == "" {
		switch subcmd {
		case "keytab", "triage", "ladder", "kcm":
			if exitCode == 0 {
				status = "PARSED"
			} else {
				status = "ERROR"
			}
		case "skill":
			if exitCode == 0 {
				status = "READY"
			} else {
				status = "ERROR"
			}
		default:
			if exitCode == 0 {
				status = "OK"
			} else {
				status = "ERROR"
			}
		}
	}

	count := intField(data, "count")
	aux := fmt.Sprintf("rc=%d", exitCode)
	perm := "tanuki-nhi"
	switch subcmd {
	case "doctor":
		if sum, ok := data["summary"].(map[string]any); ok && sum != nil {
			aux = fmt.Sprintf("ok=%d warn=%d fail=%d", intField(sum, "passed"), intField(sum, "warnings"), intField(sum, "failures"))
		}
		perm = "pre-flight"
	case "keytab", "kcm":
		if exitCode == 0 && status != "ERROR" && status != "REFUSED" {
			aux = fmt.Sprintf("entries=%d", count)
		} else if rc := strField(data, "reason_code"); rc != "" {
			aux = rc
		}
		perm = "rfc4120"
	case "ladder":
		if exitCode == 0 && status != "ERROR" && status != "REFUSED" {
			aux = fmt.Sprintf("rungs=%d", count)
		}
		perm = "decision"
	case "adcs":
		if findings, ok := data["findings"].([]any); ok {
			aux = fmt.Sprintf("findings=%d", len(findings))
			if len(findings) > 0 {
				status = "FINDINGS"
			} else {
				status = "CLEAN"
			}
		} else if rc := strField(data, "reason_code"); rc != "" {
			aux = rc
		}
		perm = "esc1-11"
	case "triage":
		if code := strField(data, "code"); code != "" {
			aux = code
			if status == "" || status == "PARSED" {
				status = "MATCHED"
			}
		} else if exitCode == 0 && status != "ERROR" && status != "REFUSED" {
			aux = fmt.Sprintf("codes=%d", count)
		}
		perm = "protocol"
	case "pac":
		if bc := intField(data, "buffer_count"); bc > 0 {
			aux = fmt.Sprintf("buffers=%d", bc)
		} else if rc := strField(data, "reason_code"); rc != "" {
			aux = rc
		}
		perm = "ms-pac"
	case "shadow":
		if ver := strField(data, "version_str"); ver != "" {
			aux = "v" + ver
		} else if rc := strField(data, "reason_code"); rc != "" {
			aux = rc
		}
		perm = "ms-adts"
	case "blast":
		if exitCode == 0 && status != "ERROR" && status != "REFUSED" {
			sev := strings.ToUpper(strField(data, "severity"))
			score := intField(data, "blast_score")
			if tierVal, hasTier := data["tier"]; hasTier && tierVal != nil {
				aux = fmt.Sprintf("tier=%d score=%d", intField(data, "tier"), score)
			} else if tl := strField(data, "tier_label"); tl != "" {
				aux = tl
			}
			if sev != "" {
				status = sev
			}
		} else if rc := strField(data, "reason_code"); rc != "" {
			aux = rc
		}
		perm = "ad-tier"
	case "token":
		if exitCode != 0 || status == "ERROR" || status == "REFUSED" {
			if rc := strField(data, "reason_code"); rc != "" {
				aux = rc
			}
		} else {
			risk := ""
			if se, ok := data["security_evaluation"].(map[string]any); ok && se != nil {
				risk = strings.ToUpper(strField(se, "risk_level"))
			}
			if risk == "" {
				risk = strings.ToUpper(strField(data, "risk_level"))
			}
			if risk != "" {
				aux = "risk=" + risk
				if status == "OK" || status == "" {
					status = risk
				}
			} else {
				aux = "jwt"
			}
		}
		perm = "rfc8693"
	case "nhi":
		if exitCode != 0 || status == "ERROR" || status == "REFUSED" {
			if rc := strField(data, "reason_code"); rc != "" {
				aux = rc
			} else {
				aux = fmt.Sprintf("rc=%d", exitCode)
			}
			if status == "" || status == "OK" {
				status = "ERROR"
			}
		} else if tokens, ok := data["discovered_tokens"].([]any); ok {
			aux = fmt.Sprintf("tokens=%d", len(tokens))
			if status == "OK" || status == "" {
				if len(tokens) > 0 {
					status = "SUCCESS"
				} else {
					status = "NONE_DISCOVERED"
				}
			}
		} else if _, hasCount := data["count"]; hasCount {
			aux = fmt.Sprintf("identities=%d", intField(data, "count"))
		} else if idents, ok := data["discovered_identities"].([]any); ok {
			aux = fmt.Sprintf("identities=%d", len(idents))
		}
		if targetLabel == "scan" || strings.HasPrefix(targetLabel, "scan ") {
			perm = "nhi-scan"
		} else if targetLabel == "mesh" || strings.HasPrefix(targetLabel, "mesh ") {
			perm = "mesh"
		} else {
			perm = "nhi"
		}
	case "purge":
		if exitCode == 0 && status != "ERROR" && status != "REFUSED" {
			aux = fmt.Sprintf("shredded=%d", intField(data, "shredded_count"))
		} else if rc := strField(data, "reason_code"); rc != "" {
			aux = rc
		}
		perm = "nist-800-88"
	case "config":
		if r := strField(data, "realm"); r != "" {
			aux = r
		}
		perm = "zero-dns"
	case "fix":
		if exitCode == 0 && status != "ERROR" && status != "REFUSED" {
			aux = "dry-run"
		}
		perm = "self-heal"
	case "auth", "tgs":
		if rc := strField(data, "reason_code"); rc != "" {
			aux = rc
		} else if p := strField(data, "principal"); p != "" {
			aux = p
		} else if spn := strField(data, "spn"); spn != "" {
			aux = spn
		}
		if subcmd == "auth" {
			perm = "as-req"
		} else {
			perm = "tgs-req"
		}
	case "ldap":
		if rc := strField(data, "reason_code"); rc != "" {
			aux = rc
		} else if bd := strField(data, "base_dn"); bd != "" {
			aux = bd
		}
		perm = "ldap"
	}

	row := Row{
		ID:        "OP-06",
		Operative: "Tanuki",
		Module:    subcmd,
		Target:    targetLabel,
		Detail:    fmt.Sprintf("subcmd=%s %s", subcmd, aux),
		PermOrTTL: perm,
		SkewOrAux: aux,
		ExitCode:  exitCode,
		Status:    status,
		RawJSON:   data,
	}
	logLine := fmt.Sprintf("[TANUKI:%s] status=%s exit=%d target=%s %s",
		subcmd, status, exitCode, targetLabel, aux)
	return row, data, exitCode, logLine
}

func hostUserInfo() (string, int, string) {
	username := os.Getenv("USER")
	if u, err := user.Current(); err == nil && u.Username != "" {
		username = u.Username
	}
	if username == "" {
		username = "operator"
	}
	host, err := os.Hostname()
	if err != nil || host == "" {
		host = "localhost"
	}
	return username, os.Getuid(), host
}

func (s *Server) registerSession(token, certFP, certSerial string, ttl time.Duration) authSession {
	sum := sha256.Sum256([]byte(token))
	now := time.Now().UTC()
	sess := &authSession{
		TokenHash:  sum,
		CertFP:     strings.ToLower(strings.TrimSpace(certFP)),
		CertSerial: strings.ToLower(strings.TrimSpace(certSerial)),
		IssuedAt:   now,
		ExpiresAt:  now.Add(ttl),
	}
	s.sessMu.Lock()
	s.pruneExpiredSessionsLocked(now)
	s.sessions = append(s.sessions, sess)
	s.sessMu.Unlock()
	return *sess
}

func (s *Server) pruneExpiredSessionsLocked(now time.Time) {
	kept := s.sessions[:0]
	for _, sess := range s.sessions {
		if sess == nil || !now.Before(sess.ExpiresAt) {
			if sess != nil {
				kdf.WipeBytes(sess.TokenHash[:])
			}
			continue
		}
		kept = append(kept, sess)
	}
	s.sessions = kept
}

func (s *Server) isSerialRevoked(serialHex string) bool {
	canon := strings.ToLower(strings.TrimSpace(serialHex))
	if canon == "" {
		return false
	}
	s.revokedMu.RLock()
	rev := s.revokedSerials[canon]
	s.revokedMu.RUnlock()
	if rev {
		return true
	}
	if s.crlMgr != nil {
		if err := s.crlMgr.ReloadIfChanged(); err != nil {
			return true
		}
		if s.crlMgr.IsRevokedHex(canon) {
			return true
		}
	}
	return false
}

func (s *Server) authorizeRequest(token, peerFP, peerSerial string) (*authSession, bool) {
	if token == "" {
		return nil, false
	}
	if peerSerial != "" {
		if s.isSerialRevoked(peerSerial) {
			return nil, false
		}
	} else if s.crlMgr != nil {
		if err := s.crlMgr.ReloadIfChanged(); err != nil {
			return nil, false
		}
	}
	tokHash := sha256.Sum256([]byte(token))
	defer kdf.WipeBytes(tokHash[:])

	now := time.Now().UTC()
	s.sessMu.Lock()
	defer s.sessMu.Unlock()
	s.pruneExpiredSessionsLocked(now)

	var matched *authSession
	for _, sess := range s.sessions {
		if subtle.ConstantTimeCompare(tokHash[:], sess.TokenHash[:]) != 1 {
			continue
		}
		if s.requireTLSBinding || peerFP != "" {
			if sess.CertFP == "" || peerFP == "" {
				continue
			}
			if subtle.ConstantTimeCompare([]byte(strings.ToLower(peerFP)), []byte(sess.CertFP)) != 1 {
				continue
			}
		}
		cp := *sess
		matched = &cp
		break
	}
	if matched != nil {
		return matched, true
	}

	// Unit-test compatibility when Server{} is constructed directly without TLS/sessions.
	if !s.requireTLSBinding && peerFP == "" && s.authToken != "" && len(s.sessions) == 0 {
		if subtle.ConstantTimeCompare([]byte(token), []byte(s.authToken)) == 1 {
			return &authSession{IssuedAt: now, ExpiresAt: now.Add(defaultTokenTTL)}, true
		}
	}
	return nil, false
}

func (s *Server) isAuthorized(token string) bool {
	_, ok := s.authorizeRequest(token, "", "")
	return ok
}

func (s *Server) storageStatusFields() map[string]any {
	dbConn := false
	sslMode := "standalone"
	var keyVer uint32
	vaultLocked := false
	vaultWiped := false
	failedAttempts := 0
	retrySec := 0
	rotating := false
	hostID := ""
	if s.storage != nil {
		dbConn = s.storage.DBConnected()
		sslMode = s.storage.SSLMode()
		keyVer = s.storage.ActiveKeyVersion()
		vaultLocked = s.storage.VaultLocked()
		ls := s.storage.VaultLockoutStatus()
		vaultWiped = ls.WipedLocked
		failedAttempts = ls.FailedAttempts
		if !ls.LockedUntil.IsZero() {
			if rem := int(time.Until(ls.LockedUntil).Seconds()); rem > 0 {
				retrySec = rem
			}
		} else if !ls.CooldownUntil.IsZero() {
			if rem := int(time.Until(ls.CooldownUntil).Seconds()); rem > 0 {
				retrySec = rem
			}
		}
		rotating = s.storage.IsRotating()
		hostID = s.storage.LocalHostID()
	}
	return map[string]any{
		"db_connected":         dbConn,
		"ssl_mode":             sslMode,
		"shuna_key_version":    keyVer,
		"vault_locked":         vaultLocked,
		"vault_wiped":          vaultWiped,
		"failed_attempts":      failedAttempts,
		"retry_after_sec":      retrySec,
		"rotation_in_progress": rotating,
		"host_id":              hostID,
	}
}

func mergeMap(dst, src map[string]any) {
	for k, v := range src {
		dst[k] = v
	}
}

func mapStorageErr(resp *Response, err error, fallbackStatus string) {
	resp.OK = false
	if errors.Is(err, vault.ErrVaultLocked) {
		resp.ExitCode = contract.ExitPolicyStop
		resp.Status = "VAULT_LOCKED"
		resp.Error = err.Error()
		return
	}
	if errors.Is(err, ErrDBNotConnected) || errors.Is(err, vault.ErrVaultNotFound) {
		resp.ExitCode = contract.ExitMissing
		resp.Status = "MISSING"
		resp.Error = err.Error()
		return
	}
	var cerr *contract.ExitError
	if errors.As(err, &cerr) {
		resp.ExitCode = cerr.Code
		if cerr.Status != "" {
			if cerr.Status == "USAGE" {
				resp.Status = "USAGE_ERROR"
			} else {
				resp.Status = cerr.Status
			}
		} else if fallbackStatus != "" {
			resp.Status = fallbackStatus
		} else {
			resp.Status = "ERROR"
		}
		resp.Error = err.Error()
		return
	}
	resp.ExitCode = contract.ExitPolicyStop
	if fallbackStatus != "" {
		resp.Status = fallbackStatus
	} else {
		resp.Status = "ERROR"
	}
	resp.Error = err.Error()
}

func (s *Server) resolveBackupOutPath(rawOut string) (string, error) {
	backupsDir := filepath.Join(s.workDir, "backups")
	if s.workDir == "" {
		backupsDir = filepath.Join(defaultWorkDir(), "backups")
	}
	if absDir, err := filepath.Abs(backupsDir); err == nil {
		backupsDir = absDir
	}
	trimmed := strings.TrimSpace(rawOut)
	if trimmed == "" {
		return filepath.Join(backupsDir, fmt.Sprintf("ciel-backup-%s.shunas1", time.Now().UTC().Format("20060102-150405"))), nil
	}
	if err := validatePathInput(trimmed); err != nil {
		return "", contract.NewError(contract.ExitUsage, "USAGE_ERROR", err.Error(), err)
	}
	for _, seg := range strings.FieldsFunc(trimmed, func(r rune) bool { return r == '/' || r == '\\' }) {
		if seg == ".." {
			return "", contract.NewError(contract.ExitUsage, "USAGE_ERROR", "backup out_path must not contain '..' traversal", nil)
		}
	}
	var candidate string
	if filepath.IsAbs(trimmed) {
		candidate = filepath.Clean(trimmed)
	} else {
		candidate = filepath.Join(backupsDir, trimmed)
	}
	if absCand, err := filepath.Abs(candidate); err == nil {
		candidate = absCand
	}
	rel, err := filepath.Rel(filepath.Clean(backupsDir), candidate)
	if err != nil || rel == "." || rel == ".." || strings.HasPrefix(rel, ".."+string(filepath.Separator)) {
		return "", contract.NewError(contract.ExitUsage, "USAGE_ERROR", fmt.Sprintf("backup out_path must reside inside %s", backupsDir), nil)
	}
	return candidate, nil
}

func (s *Server) closeConnsForSerial(serialHex string, exclude net.Conn) int {
	canon := strings.ToLower(strings.TrimSpace(serialHex))
	var toClose []net.Conn
	s.connsMu.Lock()
	for c, info := range s.activeConns {
		if strings.ToLower(info.SerialHex) == canon && c != exclude {
			toClose = append(toClose, c)
		}
	}
	s.connsMu.Unlock()
	for _, c := range toClose {
		_ = c.Close()
	}
	return len(toClose)
}

func (s *Server) revokeSessionsForSerial(serialHex string) int {
	canon := strings.ToLower(strings.TrimSpace(serialHex))
	s.sessMu.Lock()
	defer s.sessMu.Unlock()
	removed := 0
	kept := s.sessions[:0]
	for _, sess := range s.sessions {
		if sess != nil && strings.ToLower(sess.CertSerial) == canon {
			kdf.WipeBytes(sess.TokenHash[:])
			removed++
			continue
		}
		kept = append(kept, sess)
	}
	s.sessions = kept
	return removed
}

func (s *Server) handleRequest(req Request) Response {
	resp, _ := s.handleRequestWithPeer(req, peerConnInfo{}, nil)
	return resp
}

func (s *Server) handleRequestWithPeer(req Request, peer peerConnInfo, currentConn net.Conn) (Response, bool) {
	ts := time.Now().Format("15:04:05")
	resp := Response{
		Action:    req.Action,
		Timestamp: ts,
	}
	closeAfterWrite := false

	sess, ok := s.authorizeRequest(req.AuthToken, peer.CertFP, peer.SerialHex)
	if !ok {
		resp.OK = false
		resp.ExitCode = 1
		resp.Status = "UNAUTHORIZED"
		resp.Error = "Invalid, expired, or cert-mismatched auth_token"
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] unauthorized request action=%s", ts, req.Action)}
		return resp, false
	}

	resolvedCC, ccScheme, _ := resolveCCachePath(req.CcachePath)
	resolvedKCM := s.resolveKcmSocket(req.KcmSocket)
	resolvedKT := resolveKeytabPath(req.KeytabPath)
	resolvedKrb5 := resolveKrb5ConfPath(req.Krb5ConfPath)
	_, kinitErr := exec.LookPath("kinit")

	pqcGroup := peer.PQCGroup
	if pqcGroup == "" {
		pqcGroup = "X25519MLKEM768"
	}

	switch req.Action {
	case "ping":
		bins := []string{"souei", "chronoa", "ranga", "libranga.so"}
		present := make(map[string]bool, len(bins)+1)
		allReady := true
		for _, b := range bins {
			_, err := os.Stat(filepath.Join(s.tempestDir, b))
			ok := err == nil
			present[b] = ok
			if !ok {
				allReady = false
			}
		}
		tanukiOK := s.tanukiBin != ""
		present["tanuki"] = tanukiOK

		resp.OK = allReady && tanukiOK
		resp.Status = "READY"
		if !resp.OK {
			resp.Status = "MISSING_BINARIES"
			resp.ExitCode = 3
		}
		uname, uid, hostname := hostUserInfo()
		data := map[string]any{
			"server":              "ciel-teamserver",
			"tempest_dir":         s.tempestDir,
			"tanuki_bin":          s.tanukiBin,
			"work_dir":            s.workDir,
			"pki_dir":             s.pkiDir,
			"resolved_ccache":     resolvedCC,
			"ccache_scheme":       ccScheme,
			"resolved_kcm_socket": resolvedKCM,
			"resolved_keytab":     resolvedKT,
			"resolved_krb5_conf":  resolvedKrb5,
			"kinit_available":     kinitErr == nil,
			"auth_enforced":       true,
			"mtls_enforced":       true,
			"pqc_group":           pqcGroup,
			"token_expires_at":    sess.ExpiresAt.UTC().Format(time.RFC3339),
			"client_cert_serial":  peer.SerialHex,
			"client_cert_fp":      peer.CertFP,
			"binaries_present":    present,
			"pid":                 os.Getpid(),
			"user":                uname,
			"uid":                 uid,
			"hostname":            hostname,
			"arch":                runtime.GOARCH,
		}
		mergeMap(data, s.storageStatusFields())
		resp.Data = data
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] ping status=%s pqc=%s db=%v vault_locked=%v",
			ts, resp.Status, pqcGroup, data["db_connected"], data["vault_locked"])}

	case "souei_probe":
		row, data, rc, logLine := s.execSoueiProbe(req, req.WatchInotify)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}
		if s.storage != nil {
			_ = s.storage.RecordAuditEvent(context.Background(), req.HostID, "OP-01", severityForRow(row.Status, rc), rc, row.Status, AuditDetail{
				Action:  "souei_probe",
				Summary: row.Detail,
				Body:    logLine,
			})
		}

	case "souei_watch":
		row, data, rc, logLine := s.execSoueiProbe(req, true)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}
		if s.storage != nil {
			_ = s.storage.RecordAuditEvent(context.Background(), req.HostID, "OP-01", severityForRow(row.Status, rc), rc, row.Status, AuditDetail{
				Action:  "souei_watch",
				Summary: row.Detail,
				Body:    logLine,
			})
		}

	case "chronoa_check":
		row, data, rc, logLine := s.execChronoaCheck(req)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}
		if s.storage != nil {
			_ = s.storage.RecordAuditEvent(context.Background(), req.HostID, "OP-02", severityForRow(row.Status, rc), rc, row.Status, AuditDetail{
				Action:  "chronoa_check",
				Summary: row.Detail,
				Body:    logLine,
			})
		}

	case "ranga_status":
		row, data, rc, logLine := s.execRangaStatus(false, "")
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}

	case "ranga_isolate":
		row, data, rc, logLine := s.execRangaStatus(true, req.IsolateMode)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}
		if s.storage != nil {
			_ = s.storage.RecordAuditEvent(context.Background(), req.HostID, "OP-04", severityForRow(row.Status, rc), rc, row.Status, AuditDetail{
				Action:  "ranga_isolate",
				Summary: row.Detail,
				Body:    logLine,
			})
		}

	case "ranga_purge":
		row, data, rc, logLine := s.execRangaPurge(req)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}
		if s.storage != nil {
			_ = s.storage.RecordAuditEvent(context.Background(), req.HostID, "OP-05", severityForRow(row.Status, rc), rc, row.Status, AuditDetail{
				Action:  "ranga_purge",
				Summary: row.Target,
				Body:    logLine,
			})
			_ = s.storage.RecordOperatorCommand(context.Background(), req.HostID, OperatorCommandPayload{
				OperatorName: peer.CommonName,
				Action:       "ranga_purge",
				Command:      req.PurgePath,
			}, rc, row.Status)
		}

	case "tanuki_exec":
		row, data, rc, logLine := s.execTanuki(req)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}
		if s.storage != nil {
			_ = s.storage.RecordAuditEvent(context.Background(), req.HostID, "OP-06", severityForRow(row.Status, rc), rc, row.Status, AuditDetail{
				Action:  "tanuki_exec:" + req.Subcommand,
				Summary: row.Detail,
				Body:    logLine,
			})
			_ = s.storage.RecordOperatorCommand(context.Background(), req.HostID, OperatorCommandPayload{
				OperatorName: peer.CommonName,
				Action:       "tanuki_exec",
				Command:      req.Subcommand + " " + req.TargetArg,
				Args:         req.Args,
			}, rc, row.Status)
		}

	case "full_sync":
		r1, d1, rc1, l1 := s.execSoueiProbe(req, false)
		r2, d2, rc2, l2 := s.execChronoaCheck(req)
		r3, d3, rc3, l3 := s.execRangaStatus(false, "")
		r4, d4, rc4, l4 := s.execRangaStatus(true, req.IsolateMode)
		docReq := Request{
			Subcommand:   "doctor",
			KeytabPath:   req.KeytabPath,
			CcachePath:   req.CcachePath,
			Krb5ConfPath: req.Krb5ConfPath,
			KcmSocket:    req.KcmSocket,
		}
		r5, d5, rc5, l5 := s.execTanukiWithOpsec(docReq, false)
		resp.OK = true
		resp.Status = "SYNCED"
		if rc1 != 0 || rc3 != 0 || rc4 != 0 || rc5 != 0 || (rc2 != 0 && rc2 != 3) {
			resp.Status = "ATTENTION"
		}
		uname, uid, hostname := hostUserInfo()
		resp.Rows = []Row{r1, r2, r3, r4, r5}
		logs := []string{ts + " " + l1, ts + " " + l2, ts + " " + l3, ts + " " + l4, ts + " " + l5}
		data := map[string]any{
			"souei":               d1,
			"chronoa":             d2,
			"ranga_baseline":      d3,
			"ranga_isolated":      d4,
			"tanuki_doctor":       d5,
			"work_dir":            s.workDir,
			"pki_dir":             s.pkiDir,
			"resolved_ccache":     resolvedCC,
			"ccache_scheme":       ccScheme,
			"resolved_kcm_socket": resolvedKCM,
			"resolved_keytab":     resolvedKT,
			"resolved_krb5_conf":  resolvedKrb5,
			"kinit_available":     kinitErr == nil,
			"auth_enforced":       true,
			"mtls_enforced":       true,
			"pqc_group":           pqcGroup,
			"token_expires_at":    sess.ExpiresAt.UTC().Format(time.RFC3339),
			"client_cert_serial":  peer.SerialHex,
			"user":                uname,
			"uid":                 uid,
			"hostname":            hostname,
			"arch":                runtime.GOARCH,
		}
		if s.storage != nil && s.storage.DBConnected() && !s.storage.VaultLocked() {
			hostID, hidErr := s.storage.ComputeHostID(req.HostID)
			if hidErr == nil {
				meta := currentHostMeta()
				if syncErr := s.storage.RecordFullSync(context.Background(), hostID, meta, resp.Status, resp.Rows, logs); syncErr != nil {
					data["db_sync_error"] = syncErr.Error()
				}
			}
		}
		mergeMap(data, s.storageStatusFields())
		resp.Data = data
		resp.Logs = logs

	case "host_list":
		if s.storage == nil {
			mapStorageErr(&resp, ErrDBNotConnected, "MISSING")
			resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] host_list failed: %s", ts, resp.Error)}
			break
		}
		hosts, err := s.storage.ListHosts(context.Background())
		if err != nil {
			mapStorageErr(&resp, err, "ERROR")
			resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] host_list status=%s err=%s", ts, resp.Status, resp.Error)}
			break
		}
		resp.OK = true
		resp.Status = "OK"
		resp.Data = map[string]any{
			"hosts": hosts,
			"count": len(hosts),
		}
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] host_list count=%d", ts, len(hosts))}

	case "history_list":
		if s.storage == nil {
			mapStorageErr(&resp, ErrDBNotConnected, "MISSING")
			resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] history_list failed: %s", ts, resp.Error)}
			break
		}
		hostID := strings.TrimSpace(req.HostID)
		if hostID != "" {
			computed, hidErr := s.storage.ComputeHostID(hostID)
			if hidErr != nil {
				mapStorageErr(&resp, hidErr, "VAULT_LOCKED")
				break
			}
			hostID = computed
		}
		events, snaps, err := s.storage.ListHistory(context.Background(), hostID, strings.TrimSpace(req.OperativeID), req.Limit)
		if err != nil {
			mapStorageErr(&resp, err, "ERROR")
			resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] history_list status=%s err=%s", ts, resp.Status, resp.Error)}
			break
		}
		resp.OK = true
		resp.Status = "OK"
		resp.Data = map[string]any{
			"events":         events,
			"snapshots":      snaps,
			"event_count":    len(events),
			"snapshot_count": len(snaps),
		}
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] history_list events=%d snapshots=%d", ts, len(events), len(snaps))}

	case "profile_save":
		if s.storage == nil {
			mapStorageErr(&resp, ErrDBNotConnected, "MISSING")
			break
		}
		for _, p := range []string{req.CcachePath, req.KeytabPath, req.Krb5ConfPath, req.KcmSocket} {
			if strings.TrimSpace(p) != "" {
				if err := validatePathInput(strings.TrimSpace(p)); err != nil {
					resp.OK = false
					resp.ExitCode = 1
					resp.Status = "USAGE_ERROR"
					resp.Error = err.Error()
					resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] profile_save rejected: %v", ts, err)}
					return resp, false
				}
			}
		}
		hostID, err := s.storage.ComputeHostID(req.HostID)
		if err != nil {
			mapStorageErr(&resp, err, "VAULT_LOCKED")
			break
		}
		prof := TargetProfile{
			HostID:       hostID,
			CcachePath:   strings.TrimSpace(req.CcachePath),
			KeytabPath:   strings.TrimSpace(req.KeytabPath),
			Krb5ConfPath: strings.TrimSpace(req.Krb5ConfPath),
			KcmSocket:    strings.TrimSpace(req.KcmSocket),
		}
		if err := s.storage.SaveProfile(context.Background(), hostID, currentHostMeta(), prof); err != nil {
			mapStorageErr(&resp, err, "ERROR")
			break
		}
		resp.OK = true
		resp.Status = "SAVED"
		resp.Data = map[string]any{
			"host_id":        hostID,
			"ccache_path":    prof.CcachePath,
			"keytab_path":    prof.KeytabPath,
			"krb5_conf_path": prof.Krb5ConfPath,
			"kcm_socket":     prof.KcmSocket,
		}
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] profile_save host_id=%s", ts, hostID)}

	case "profile_get":
		if s.storage == nil {
			mapStorageErr(&resp, ErrDBNotConnected, "MISSING")
			break
		}
		hostID, err := s.storage.ComputeHostID(req.HostID)
		if err != nil {
			mapStorageErr(&resp, err, "VAULT_LOCKED")
			break
		}
		prof, found, err := s.storage.GetProfile(context.Background(), hostID)
		if err != nil {
			mapStorageErr(&resp, err, "ERROR")
			break
		}
		resp.OK = true
		resp.Status = "OK"
		if !found {
			resp.Status = "NOT_FOUND"
		}
		resp.Data = map[string]any{
			"found":          found,
			"host_id":        prof.HostID,
			"ccache_path":    prof.CcachePath,
			"keytab_path":    prof.KeytabPath,
			"krb5_conf_path": prof.Krb5ConfPath,
			"kcm_socket":     prof.KcmSocket,
		}
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] profile_get host_id=%s found=%v", ts, hostID, found)}

	case "vault_unlock":
		if s.storage == nil {
			mapStorageErr(&resp, vault.ErrVaultNotFound, "MISSING")
			break
		}
		pass := []byte(req.Passphrase)
		defer kdf.WipeBytes(pass)
		err := s.storage.UnlockVault(context.Background(), pass)
		if err != nil {
			mapStorageErr(&resp, err, "POLICY_STOP")
			ls := s.storage.VaultLockoutStatus()
			retrySec := 0
			if !ls.LockedUntil.IsZero() {
				rem := int(time.Until(ls.LockedUntil).Seconds())
				if rem > 0 {
					retrySec = rem
				}
			}
			resp.Data = map[string]any{
				"vault_locked":    s.storage.VaultLocked(),
				"vault_wiped":     ls.WipedLocked,
				"failed_attempts": ls.FailedAttempts,
				"retry_after_sec": retrySec,
				"wiped_locked":    ls.WipedLocked,
			}
			resp.Logs = []string{fmt.Sprintf("%s [SHUNA] vault_unlock status=%s attempts=%d retry_after=%ds",
				ts, resp.Status, ls.FailedAttempts, retrySec)}
			break
		}
		resp.OK = true
		resp.Status = "UNLOCKED"
		resp.Data = s.storageStatusFields()
		resp.Logs = []string{fmt.Sprintf("%s [SHUNA] vault_unlock status=UNLOCKED key_version=%d", ts, s.storage.ActiveKeyVersion())}

	case "vault_rotate":
		if s.storage == nil {
			mapStorageErr(&resp, vault.ErrVaultNotFound, "MISSING")
			break
		}
		var pass []byte
		if req.NewPassphrase != "" {
			pass = []byte(req.NewPassphrase)
		} else if req.Passphrase != "" {
			pass = []byte(req.Passphrase)
		}
		defer kdf.WipeBytes(pass)
		oldVer, newVer, err := s.storage.StartRotation(context.Background(), pass)
		if err != nil {
			mapStorageErr(&resp, err, "POLICY_STOP")
			resp.Logs = []string{fmt.Sprintf("%s [SHUNA] vault_rotate failed status=%s err=%s", ts, resp.Status, resp.Error)}
			break
		}
		_ = s.storage.RecordAuditEvent(context.Background(), req.HostID, "OP-06", "INFO", 0, "ROTATING", AuditDetail{
			Action:  "vault_rotate",
			Summary: fmt.Sprintf("key rotation %d -> %d", oldVer, newVer),
			Body:    fmt.Sprintf("dual-key online rotation started %d -> %d", oldVer, newVer),
		})
		resp.OK = true
		resp.Status = "ROTATING"
		resp.Data = map[string]any{
			"previous_key_version": oldVer,
			"shuna_key_version":    newVer,
			"rotation_in_progress": s.storage.IsRotating(),
		}
		resp.Logs = []string{fmt.Sprintf("%s [SHUNA] vault_rotate %d -> %d (in_progress=%v)", ts, oldVer, newVer, s.storage.IsRotating())}

	case "session_issue":
		if (s.requireTLSBinding || s.bootstrapCertFP != "" || peer.CertFP != "") &&
			(peer.CertFP == "" || s.bootstrapCertFP == "" || !strings.EqualFold(peer.CertFP, s.bootstrapCertFP)) {
			resp.OK = false
			resp.ExitCode = 1
			resp.Status = "UNAUTHORIZED"
			resp.Error = "session_issue is restricted to the bootstrap/admin mTLS client certificate"
			resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] unauthorized session_issue from cert_fp=%s", ts, peer.CertFP)}
			break
		}
		targetFP := strings.ToLower(strings.TrimSpace(req.CertFP))
		if targetFP == "" {
			targetFP = peer.CertFP
		}
		if len(targetFP) != 64 || !isLowerHex(targetFP) {
			resp.OK = false
			resp.ExitCode = 1
			resp.Status = "USAGE_ERROR"
			resp.Error = "cert_fp must be 64-char lowercase SHA-256 hex fingerprint"
			break
		}
		newTok, err := generateRandomToken()
		if err != nil {
			resp.OK = false
			resp.ExitCode = 1
			resp.Status = "ERROR"
			resp.Error = err.Error()
			break
		}
		serial := strings.ToLower(strings.TrimSpace(req.SerialHex))
		if serial == "" && targetFP == peer.CertFP {
			serial = peer.SerialHex
		}
		newSess := s.registerSession(newTok, targetFP, serial, defaultTokenTTL)
		resp.OK = true
		resp.Status = "ISSUED"
		resp.Data = map[string]any{
			"auth_token":       newTok,
			"cert_fp":          targetFP,
			"issued_at":        newSess.IssuedAt.Format(time.RFC3339),
			"token_expires_at": newSess.ExpiresAt.Format(time.RFC3339),
		}
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] session_issue cert_fp=%s", ts, targetFP[:12])}

	case "token_rotate":
		newTok, err := generateRandomToken()
		if err != nil {
			resp.OK = false
			resp.ExitCode = 1
			resp.Status = "ERROR"
			resp.Error = err.Error()
			break
		}
		newSess := s.registerSession(newTok, peer.CertFP, peer.SerialHex, defaultTokenTTL)
		// Shrink previous matching token(s) for this certFP to 30s grace overlap.
		graceDeadline := time.Now().UTC().Add(tokenRotateGrace)
		newHash := newSess.TokenHash
		updateTokenFile := s.tokenFile != "" && (peer.CertFP == "" || peer.CertFP == s.bootstrapCertFP)
		s.sessMu.Lock()
		for _, existing := range s.sessions {
			if existing != nil && existing.TokenHash != newHash && existing.CertFP == newSess.CertFP {
				if existing.ExpiresAt.After(graceDeadline) {
					existing.ExpiresAt = graceDeadline
				}
			}
		}
		if updateTokenFile {
			s.authToken = newTok
		}
		s.sessMu.Unlock()
		if updateTokenFile {
			_ = writeFileNoFollow(s.tokenFile, []byte(newTok+"\n"), 0o600)
		}
		resp.OK = true
		resp.Status = "ROTATED"
		resp.Data = map[string]any{
			"auth_token":       newTok,
			"issued_at":        newSess.IssuedAt.Format(time.RFC3339),
			"token_expires_at": newSess.ExpiresAt.Format(time.RFC3339),
			"grace_sec":        int(tokenRotateGrace.Seconds()),
		}
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] token_rotate grace=%ds", ts, int(tokenRotateGrace.Seconds()))}

	case "cert_revoke":
		if strings.TrimSpace(req.SerialHex) == "" {
			resp.OK = false
			resp.ExitCode = 1
			resp.Status = "USAGE_ERROR"
			resp.Error = "serial_hex is required"
			break
		}
		_, canon, err := mtls.ParseSerial(req.SerialHex)
		if err != nil {
			mapStorageErr(&resp, err, "USAGE_ERROR")
			break
		}
		reasonStr := strings.TrimSpace(req.Reason)
		if reasonStr == "" {
			reasonStr = "key_compromise"
		}
		reasonCode, err := mtls.ParseReason(reasonStr)
		if err != nil {
			mapStorageErr(&resp, err, "USAGE_ERROR")
			break
		}
		if s.storage != nil && s.storage.DBConnected() {
			if s.storage.VaultLocked() {
				mapStorageErr(&resp, vault.ErrVaultLocked, "VAULT_LOCKED")
				break
			}
			cn := peer.CommonName
			if canon != peer.SerialHex || cn == "" {
				cn = "revoked-peer"
			}
			if err := s.storage.RecordRevokedCert(context.Background(), canon, cn, reasonStr); err != nil {
				mapStorageErr(&resp, err, "ERROR")
				break
			}
		}
		if s.crlMgr != nil {
			if _, err := s.crlMgr.Revoke(canon, reasonCode); err != nil {
				resp.OK = false
				resp.ExitCode = 2
				resp.Status = "POLICY_STOP"
				resp.Error = err.Error()
				break
			}
		}
		s.revokedMu.Lock()
		if s.revokedSerials == nil {
			s.revokedSerials = make(map[string]bool)
		}
		s.revokedSerials[canon] = true
		s.revokedMu.Unlock()

		invalidated := s.revokeSessionsForSerial(canon)
		closedOthers := s.closeConnsForSerial(canon, currentConn)
		if strings.EqualFold(canon, peer.SerialHex) && currentConn != nil {
			closeAfterWrite = true
			closedOthers++
		}
		resp.OK = true
		resp.Status = "REVOKED"
		resp.Data = map[string]any{
			"serial_hex":           canon,
			"reason":               reasonStr,
			"sessions_invalidated": invalidated,
			"connections_closed":   closedOthers,
		}
		resp.Logs = []string{fmt.Sprintf("%s [SHUNA:PKI] cert_revoke serial=%s closed_conns=%d", ts, canon, closedOthers)}

	case "backup_export":
		if s.storage == nil {
			mapStorageErr(&resp, ErrDBNotConnected, "MISSING")
			break
		}
		outPath, pathErr := s.resolveBackupOutPath(req.OutPath)
		if pathErr != nil {
			mapStorageErr(&resp, pathErr, "USAGE_ERROR")
			resp.Logs = []string{fmt.Sprintf("%s [SHUNA] backup_export rejected: %s", ts, resp.Error)}
			break
		}
		nBytes, err := s.storage.ExportBackup(context.Background(), outPath)
		if err != nil {
			mapStorageErr(&resp, err, "ERROR")
			resp.Logs = []string{fmt.Sprintf("%s [SHUNA] backup_export failed: %s", ts, resp.Error)}
			break
		}
		_ = s.storage.RecordAuditEvent(context.Background(), req.HostID, "OP-06", "OK", 0, "BACKUP_EXPORTED", AuditDetail{
			Action:  "backup_export",
			Summary: fmt.Sprintf("SHUNAS1 backup (%d bytes)", nBytes),
			Body:    outPath,
		})
		resp.OK = true
		resp.Status = "EXPORTED"
		resp.Data = map[string]any{
			"out_path":          outPath,
			"bytes_written":     nBytes,
			"format":            "SHUNAS1",
			"shuna_key_version": s.storage.ActiveKeyVersion(),
		}
		resp.Logs = []string{fmt.Sprintf("%s [SHUNA] backup_export path=%s bytes=%d", ts, outPath, nBytes)}

	default:
		resp.OK = false
		resp.ExitCode = 1
		resp.Status = "UNKNOWN_ACTION"
		resp.Error = "Unknown action: " + req.Action
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] unknown action=%s", ts, req.Action)}
	}
	return resp, closeAfterWrite
}

func (s *Server) serveConn(conn net.Conn) {
	defer conn.Close()
	if s.connSem != nil {
		defer func() { <-s.connSem }()
	}

	var peer peerConnInfo
	if tc, ok := conn.(*tls.Conn); ok {
		ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
		err := tc.HandshakeContext(ctx)
		cancel()
		if err != nil {
			return
		}
		st := tc.ConnectionState()
		if st.Version != tls.VersionTLS13 || len(st.PeerCertificates) == 0 {
			return
		}
		leaf := st.PeerCertificates[0]
		pqcGroup := "X25519MLKEM768"
		if s.allowClassicalFallback {
			pqcGroup = "X25519MLKEM768:X25519"
		}
		peer = peerConnInfo{
			CertFP:     mtls.CertFingerprintSHA256(leaf),
			SerialHex:  mtls.FormatSerial(leaf.SerialNumber),
			CommonName: leaf.Subject.CommonName,
			PQCGroup:   pqcGroup,
		}
		if s.isSerialRevoked(peer.SerialHex) {
			return
		}
		s.connsMu.Lock()
		if s.activeConns == nil {
			s.activeConns = make(map[net.Conn]peerConnInfo)
		}
		s.activeConns[conn] = peer
		s.connsMu.Unlock()
		defer func() {
			s.connsMu.Lock()
			delete(s.activeConns, conn)
			s.connsMu.Unlock()
		}()
	}

	scanner := bufio.NewScanner(conn)
	scanner.Buffer(make([]byte, 64*1024), 1024*1024)
	for {
		_ = conn.SetReadDeadline(time.Now().Add(30 * time.Second))
		if !scanner.Scan() {
			break
		}
		line := bytes.TrimSpace(scanner.Bytes())
		if len(line) == 0 {
			continue
		}
		var req Request
		var resp Response
		var closeAfter bool
		if err := json.Unmarshal(line, &req); err != nil {
			resp = Response{
				OK:        false,
				ExitCode:  1,
				Status:    "BAD_JSON",
				Timestamp: time.Now().Format("15:04:05"),
				Error:     err.Error(),
			}
		} else {
			resp, closeAfter = s.handleRequestWithPeer(req, peer, conn)
		}
		encoded, err := json.Marshal(resp)
		if err != nil {
			continue
		}
		encoded = append(encoded, '\n')
		_ = conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
		if _, wErr := conn.Write(encoded); wErr != nil {
			break
		}
		if closeAfter {
			break
		}
	}
}

func findTanukiBin(selfExe, tempestDir string) string {
	if envBin := os.Getenv("TANUKI_BIN"); envBin != "" {
		if _, err := os.Stat(envBin); err == nil {
			return envBin
		}
	}
	exeDir := filepath.Dir(selfExe)
	candidates := []string{
		filepath.Join(exeDir, "tanuki"),
		filepath.Join(tempestDir, "tanuki"),
		filepath.Clean(filepath.Join(exeDir, "..", "bin", "tanuki")),
		"bin/tanuki",
	}
	if home, err := os.UserHomeDir(); err == nil {
		candidates = append(candidates, filepath.Join(home, ".local", "bin", "tanuki"))
	}
	for _, c := range candidates {
		if _, err := os.Stat(c); err == nil {
			return c
		}
	}
	if p, err := exec.LookPath("tanuki"); err == nil {
		return p
	}
	return ""
}

func defaultWorkDir() string {
	if xdg := strings.TrimSpace(os.Getenv("XDG_RUNTIME_DIR")); xdg != "" && filepath.IsAbs(xdg) {
		return filepath.Join(xdg, "ciel_workspace")
	}
	if cacheDir, err := os.UserCacheDir(); err == nil && cacheDir != "" {
		return filepath.Join(cacheDir, "ciel_workspace")
	}
	return filepath.Join(os.TempDir(), fmt.Sprintf("ciel_workspace_%d", os.Getuid()))
}

func generateRandomToken() (string, error) {
	buf := make([]byte, 32)
	if _, err := rand.Read(buf); err != nil {
		return "", err
	}
	return hex.EncodeToString(buf), nil
}

func (s *Server) initPKI(pkiDir string) error {
	if err := validateSecureDir(pkiDir); err != nil {
		return err
	}
	s.pkiDir = pkiDir
	if s.revokedSerials == nil {
		s.revokedSerials = make(map[string]bool)
	}

	caPath := filepath.Join(pkiDir, "ca.crt")
	srvPath := filepath.Join(pkiDir, "server.crt")
	srvKeyPath := filepath.Join(pkiDir, "server.key")
	crlPath := filepath.Join(pkiDir, "ca.crl")

	if _, err := os.Stat(caPath); os.IsNotExist(err) {
		if _, err := mtls.InitPKI(pkiDir, []string{"127.0.0.1", "localhost"}, []string{"postgres", "127.0.0.1", "localhost"}, 0); err != nil {
			return fmt.Errorf("auto-init pki: %w", err)
		}
	}

	caCert, caKey, caErr := mtls.LoadCA(pkiDir)
	if caErr != nil {
		// Allow running without ca.key if ca.crt + ca.crl + server.crt + server.key exist.
		caCert, err := mtls.LoadCertificate(caPath)
		if err != nil {
			return err
		}
		crl, err := mtls.LoadCRL(crlPath, caCert)
		if err != nil {
			return err
		}
		s.crlMgr = crl
	} else {
		now := time.Now().UTC()
		if cert, err := mtls.LoadCertificate(srvPath); err != nil || mtls.NeedsRenewal(cert, now) {
			if _, ierr := mtls.IssueCert(pkiDir, "server", mtls.RoleServer, []string{"127.0.0.1", "localhost"}, mtls.DefaultServerTTL); ierr != nil {
				return fmt.Errorf("issue/renew server cert: %w", ierr)
			}
		}
		if _, err := os.Stat(srvKeyPath); err != nil {
			return fmt.Errorf("missing server.key: %w", err)
		}
		crl, err := mtls.OpenCRLManager(caCert, caKey, crlPath)
		if err != nil {
			return err
		}
		s.crlMgr = crl

		clientCertPath := filepath.Join(pkiDir, "client.crt")
		clientKeyPath := filepath.Join(pkiDir, "client.key")
		_, cerr := os.Stat(clientCertPath)
		_, kerr := os.Stat(clientKeyPath)
		if cerr != nil || kerr != nil {
			if _, ierr := mtls.IssueCert(pkiDir, "client", mtls.RoleClient, nil, mtls.DefaultClientTTL); ierr != nil {
				return fmt.Errorf("issue default client cert: %w", ierr)
			}
		} else if cCert, lerr := mtls.LoadCertificate(clientCertPath); lerr == nil && mtls.NeedsRenewal(cCert, now) {
			if _, ierr := mtls.IssueCert(pkiDir, "client", mtls.RoleClient, nil, mtls.DefaultClientTTL); ierr != nil {
				return fmt.Errorf("renew default client cert: %w", ierr)
			}
		}
	}

	if clientCert, err := mtls.LoadCertificate(filepath.Join(pkiDir, "client.crt")); err == nil {
		s.bootstrapCertFP = mtls.CertFingerprintSHA256(clientCert)
		s.bootstrapSerial = mtls.FormatSerial(clientCert.SerialNumber)
	}
	return nil
}

func (s *Server) initAuthToken(explicitTokenFile string) error {
	if err := validateSecureDir(s.workDir); err != nil {
		return err
	}
	envToken := strings.TrimSpace(os.Getenv("CIEL_AUTH_TOKEN"))
	if envToken != "" {
		s.sessMu.Lock()
		s.authToken = envToken
		s.sessMu.Unlock()
		if explicitTokenFile != "" {
			if err := writeFileNoFollow(explicitTokenFile, []byte(envToken+"\n"), 0o600); err != nil {
				return err
			}
			s.tokenFile = explicitTokenFile
		}
		s.registerSession(envToken, s.bootstrapCertFP, s.bootstrapSerial, defaultTokenTTL)
		return nil
	}
	tok, err := generateRandomToken()
	if err != nil {
		return err
	}
	s.sessMu.Lock()
	s.authToken = tok
	s.sessMu.Unlock()
	tf := strings.TrimSpace(explicitTokenFile)
	if tf == "" {
		tf = filepath.Join(s.workDir, "teamserver.token")
	}
	if err := writeFileNoFollow(tf, []byte(tok+"\n"), 0o600); err != nil {
		return err
	}
	s.tokenFile = tf
	s.registerSession(tok, s.bootstrapCertFP, s.bootstrapSerial, defaultTokenTTL)
	return nil
}

func (s *Server) tlsConfig() (*tls.Config, error) {
	return mtls.ServerTLSConfig(
		filepath.Join(s.pkiDir, "server.crt"),
		filepath.Join(s.pkiDir, "server.key"),
		filepath.Join(s.pkiDir, "ca.crt"),
		s.crlMgr,
		mtls.TLSOptions{
			AllowClassicalFallback: s.allowClassicalFallback,
			ExtraRevokedCheck:      s.isSerialRevoked,
		},
	)
}

func main() {
	defaultDir := os.Getenv("TEMPEST_BIN_DIR")
	if defaultDir == "" {
		defaultDir = defaultTempestDir
	}
	if absDir, err := filepath.Abs(defaultDir); err == nil {
		defaultDir = absDir
	}

	addr := flag.String("addr", "127.0.0.1:40056", "TLS listen address for Qt desktop clients")
	tempestDir := flag.String("tempest-dir", defaultDir, "Directory containing built Tempest binaries")
	workDir := flag.String("work-dir", defaultWorkDir(), "Workspace directory for runtime state and KCM output")
	kcmSock := flag.String("kcm-socket", "", "Explicit KCM UNIX socket path to monitor (default: auto-discover system KCM socket)")
	tokenFile := flag.String("token-file", "", "Path to write 0600 RPC auth token when CIEL_AUTH_TOKEN is unset")
	pkiDirFlag := flag.String("pki-dir", "", "PKI directory containing ca.crt, server.crt, server.key, ca.crl (default: <work-dir>/pki)")
	vaultDirFlag := flag.String("vault-dir", "", "Shuna vault directory (default: <work-dir>/vault)")
	pgDSNFlag := flag.String("pg-dsn", "", "PostgreSQL 16 DSN (overrides CIEL_PG_DSN and ~/.config/ciel/pg.dsn)")
	pgInsecureLocal := flag.Bool("pg-insecure-local", false, "Allow non-verify-full PostgreSQL TLS on loopback only (dev override)")
	vaultPassEnv := flag.String("vault-passphrase-env", "", "Environment variable holding Shuna vault passphrase")
	strictMlock := flag.Bool("strict-mlock", false, "Fail closed if mlock fails for Shuna DEKs")
	strictLockoutWipe := flag.Bool("strict-lockout-wipe", false, "Zeroize DEKs and lock vault until admin unlock-reset on 5th failed unlock")
	allowClassical := flag.Bool("allow-classical-tls-fallback", false, "Allow X25519 TLS 1.3 fallback alongside X25519MLKEM768")
	showVersion := flag.Bool("version", false, "Print version and exit")
	flag.BoolVar(showVersion, "V", false, "Print version and exit")
	flag.Parse()

	if *showVersion {
		fmt.Println("ciel-teamserver 1.0.0")
		return
	}

	selfExe, err := os.Executable()
	if err != nil {
		selfExe = os.Args[0]
	}

	resolvedTempestDir := *tempestDir
	if os.Getenv("TEMPEST_BIN_DIR") == "" && resolvedTempestDir == defaultDir {
		exeDir := filepath.Dir(selfExe)
		if _, exeErr := os.Stat(filepath.Join(exeDir, "souei")); exeErr == nil {
			resolvedTempestDir = exeDir
		} else if absTempest, absErr := filepath.Abs(resolvedTempestDir); absErr == nil {
			resolvedTempestDir = absTempest
		}
		if _, statErr := os.Stat(resolvedTempestDir); os.IsNotExist(statErr) {
			exeSibling := filepath.Clean(filepath.Join(exeDir, "..", "..", "tempest", "bin"))
			if _, sibErr := os.Stat(exeSibling); sibErr == nil {
				resolvedTempestDir = exeSibling
			}
		}
	} else if absTempest, absErr := filepath.Abs(resolvedTempestDir); absErr == nil {
		resolvedTempestDir = absTempest
	}

	resolvedWorkDir := *workDir
	if absWork, absErr := filepath.Abs(resolvedWorkDir); absErr == nil {
		resolvedWorkDir = absWork
	}

	resolvedPKIDir := strings.TrimSpace(*pkiDirFlag)
	if resolvedPKIDir == "" {
		resolvedPKIDir = filepath.Join(resolvedWorkDir, "pki")
	} else if absPKI, absErr := filepath.Abs(resolvedPKIDir); absErr == nil {
		resolvedPKIDir = absPKI
	}

	resolvedVaultDir := strings.TrimSpace(*vaultDirFlag)
	if resolvedVaultDir == "" {
		resolvedVaultDir = filepath.Join(resolvedWorkDir, "vault")
	} else if absVault, absErr := filepath.Abs(resolvedVaultDir); absErr == nil {
		resolvedVaultDir = absVault
	}

	srv := &Server{
		tempestDir:             resolvedTempestDir,
		tanukiBin:              findTanukiBin(selfExe, resolvedTempestDir),
		workDir:                resolvedWorkDir,
		selfExe:                selfExe,
		kcmSockPath:            strings.TrimSpace(*kcmSock),
		requireTLSBinding:      true,
		allowClassicalFallback: *allowClassical,
		connSem:                make(chan struct{}, 32),
		revokedSerials:         make(map[string]bool),
		activeConns:            make(map[net.Conn]peerConnInfo),
	}

	if err := validateSecureDir(srv.workDir); err != nil {
		fmt.Fprintf(os.Stderr, "ciel-teamserver: workspace init failed: %v\n", err)
		os.Exit(1)
	}
	if err := srv.initPKI(resolvedPKIDir); err != nil {
		fmt.Fprintf(os.Stderr, "ciel-teamserver: PKI init failed: %v\n", err)
		os.Exit(1)
	}

	pgDSN, pgSource := ResolvePGDSN(*pgDSNFlag)
	passEnvName := strings.TrimSpace(*vaultPassEnv)
	if passEnvName == "" {
		passEnvName = "CIEL_VAULT_PASSPHRASE"
	}
	initialPass := []byte(os.Getenv(passEnvName))
	defer kdf.WipeBytes(initialPass)

	storage, stErr := NewStorage(context.Background(), pgDSN, resolvedPKIDir, resolvedVaultDir, initialPass, *pgInsecureLocal, *strictMlock, *strictLockoutWipe)
	if stErr != nil {
		fmt.Fprintf(os.Stderr, "ciel-teamserver: database/vault init failed (source=%s): %v\n", pgSource, stErr)
		os.Exit(3)
	}
	srv.storage = storage
	defer storage.Close()

	if storage.DBConnected() {
		if revs, rerr := storage.LoadRevokedSerials(context.Background()); rerr == nil {
			srv.revokedMu.Lock()
			for k, v := range revs {
				srv.revokedSerials[k] = v
			}
			srv.revokedMu.Unlock()
		}
	}

	if err := srv.initAuthToken(*tokenFile); err != nil {
		fmt.Fprintf(os.Stderr, "ciel-teamserver: auth/token init failed: %v\n", err)
		os.Exit(1)
	}

	tlsCfg, err := srv.tlsConfig()
	if err != nil {
		fmt.Fprintf(os.Stderr, "ciel-teamserver: TLS config failed: %v\n", err)
		os.Exit(1)
	}
	tcpLn, err := net.Listen("tcp", *addr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "ciel-teamserver: listen %s failed: %v\n", *addr, err)
		os.Exit(1)
	}
	ln := tls.NewListener(tcpLn, tlsCfg)
	defer ln.Close()

	fmt.Printf("[ciel-teamserver] mTLS 1.3 (X25519MLKEM768) listening on %s (db_connected=%v, ssl_mode=%s, workspace=%s, pki=%s, token_file=%s)\n",
		ln.Addr().String(), storage.DBConnected(), storage.SSLMode(), srv.workDir, srv.pkiDir, srv.tokenFile)

	var connWg sync.WaitGroup
	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		<-sigCh
		_ = ln.Close()
		srv.connsMu.Lock()
		for c := range srv.activeConns {
			_ = c.Close()
		}
		srv.connsMu.Unlock()
	}()

	for {
		conn, err := ln.Accept()
		if err != nil {
			if errors.Is(err, net.ErrClosed) {
				break
			}
			continue
		}
		select {
		case srv.connSem <- struct{}{}:
			connWg.Add(1)
			go func(c net.Conn) {
				defer connWg.Done()
				srv.serveConn(c)
			}(conn)
		default:
			_ = conn.Close()
		}
	}

	// Drain active connections and any background key rotation before closing storage.
	drainDone := make(chan struct{})
	go func() {
		connWg.Wait()
		for storage.IsRotating() {
			time.Sleep(25 * time.Millisecond)
		}
		close(drainDone)
	}()
	select {
	case <-drainDone:
	case <-time.After(5 * time.Second):
	}
}
