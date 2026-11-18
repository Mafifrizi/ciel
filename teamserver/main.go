package main

import (
	"bufio"
	"bytes"
	"context"
	"crypto/rand"
	"encoding/binary"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"sync"
	"syscall"
	"time"
)

const defaultTempestDir = "../tempest/bin"

type Request struct {
	Action        string `json:"action"`
	CcachePath    string `json:"ccache_path,omitempty"`
	SkewSec       int64  `json:"skew_sec,omitempty"`
	WarnWindowSec int64  `json:"warn_window_sec,omitempty"`
	Renew         bool   `json:"renew,omitempty"`
	WeakEnctype   bool   `json:"weak_enctype,omitempty"`
	IsolateMode   string `json:"isolate_mode,omitempty"`
	PurgePath     string `json:"purge_path,omitempty"`
	RevokeSession bool   `json:"revoke_session,omitempty"`
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

type Server struct {
	tempestDir  string
	workDir     string
	selfExe     string
	kcmSockPath string
	mu          sync.Mutex
}

func packPrincipal(comps []string, realm string, nameType uint32) []byte {
	var b bytes.Buffer
	rb := []byte(realm)
	_ = binary.Write(&b, binary.BigEndian, nameType)
	_ = binary.Write(&b, binary.BigEndian, uint32(len(comps)))
	_ = binary.Write(&b, binary.BigEndian, uint32(len(rb)))
	b.Write(rb)
	for _, c := range comps {
		cb := []byte(c)
		_ = binary.Write(&b, binary.BigEndian, uint32(len(cb)))
		b.Write(cb)
	}
	return b.Bytes()
}

func writeDemoCCache(path string, ttlSec int64, enctype uint16) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o700); err != nil {
		return err
	}
	now := time.Now().Unix()
	auth := uint32(now - 120)
	start := uint32(now - 120)
	end := uint32(now + ttlSec)
	renewTill := uint32(now + 86400)

	var out bytes.Buffer
	_ = binary.Write(&out, binary.BigEndian, uint16(0x0504))
	_ = binary.Write(&out, binary.BigEndian, uint16(0))

	clientP := packPrincipal([]string{"ciel"}, "TEMPEST.LOCAL", 1)
	serverP := packPrincipal([]string{"krbtgt", "TEMPEST.LOCAL"}, "TEMPEST.LOCAL", 2)
	out.Write(clientP)
	out.Write(clientP)
	out.Write(serverP)

	key := bytes.Repeat([]byte{0xAA}, 16)
	_ = binary.Write(&out, binary.BigEndian, enctype)
	_ = binary.Write(&out, binary.BigEndian, uint32(len(key)))
	out.Write(key)

	_ = binary.Write(&out, binary.BigEndian, auth)
	_ = binary.Write(&out, binary.BigEndian, start)
	_ = binary.Write(&out, binary.BigEndian, end)
	_ = binary.Write(&out, binary.BigEndian, renewTill)

	out.WriteByte(0)
	_ = binary.Write(&out, binary.BigEndian, uint32(0x40E10000))
	_ = binary.Write(&out, binary.BigEndian, uint32(0))
	_ = binary.Write(&out, binary.BigEndian, uint32(0))

	tkt := bytes.Repeat([]byte{0x55}, 32)
	_ = binary.Write(&out, binary.BigEndian, uint32(len(tkt)))
	out.Write(tkt)
	_ = binary.Write(&out, binary.BigEndian, uint32(0))

	return os.WriteFile(path, out.Bytes(), 0o600)
}

func writeDemoSecretFile(path string) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o700); err != nil {
		return err
	}
	buf := make([]byte, 4096)
	_, _ = rand.Read(buf)
	return os.WriteFile(path, buf, 0o600)
}

func (s *Server) defaultCCachePath() string {
	return filepath.Join(s.workDir, "krb5cc_ciel")
}

func (s *Server) defaultPurgePath() string {
	return filepath.Join(s.workDir, "PurgeTarget.keytab")
}

func (s *Server) ensureWorkspaceArtifacts() error {
	s.mu.Lock()
	defer s.mu.Unlock()
	if err := os.MkdirAll(s.workDir, 0o700); err != nil {
		return err
	}
	cc := s.defaultCCachePath()
	if _, err := os.Stat(cc); os.IsNotExist(err) {
		if err := writeDemoCCache(cc, 3600, 18); err != nil {
			return err
		}
	}
	pt := s.defaultPurgePath()
	if _, err := os.Stat(pt); os.IsNotExist(err) {
		if err := writeDemoSecretFile(pt); err != nil {
			return err
		}
	}
	return nil
}

func (s *Server) startKcmSocket() (net.Listener, error) {
	sockPath := filepath.Join(s.workDir, "kcm.sock")
	_ = os.Remove(sockPath)
	ln, err := net.Listen("unix", sockPath)
	if err != nil {
		return nil, err
	}
	if err := os.Chmod(sockPath, 0o600); err != nil {
		_ = ln.Close()
		return nil, err
	}
	s.kcmSockPath = sockPath
	go func() {
		for {
			c, err := ln.Accept()
			if err != nil {
				return
			}
			go func(conn net.Conn) {
				time.Sleep(30 * time.Millisecond)
				_ = conn.Close()
			}(c)
		}
	}()
	return ln, nil
}

func (s *Server) runBinaryJSON(binName string, extraEnv []string, args ...string) (map[string]any, int, string, error) {
	binPath := filepath.Join(s.tempestDir, binName)
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, binPath, args...)
	if len(extraEnv) > 0 {
		cmd.Env = append(os.Environ(), extraEnv...)
	}
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
	if len(bytes.TrimSpace(stdout.Bytes())) > 0 {
		if jErr := json.Unmarshal(bytes.TrimSpace(stdout.Bytes()), &parsed); jErr != nil {
			return nil, exitCode, stdout.String() + " " + stderr.String(), jErr
		}
	}
	return parsed, exitCode, stderr.String(), nil
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
	if v, ok := m[key].(float64); ok {
		return int64(v)
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

func (s *Server) execSoueiProbe() (Row, map[string]any, int, string) {
	args := []string{"--probe"}
	if s.kcmSockPath != "" {
		args = append(args, "--kcm-socket", s.kcmSockPath)
	}
	args = append(args, "--json")
	data, exitCode, stderrStr, err := s.runBinaryJSON("souei", nil, args...)
	if err != nil {
		return errRow("OP-01", "Souei", "keyring-probe", "/proc/keys", exitCode, err),
			nil, exitCode, fmt.Sprintf("[SOUEI] error executing souei: %v (%s)", err, stderrStr)
	}
	sessID := intField(data, "session_keyring_id")
	permHex := strField(data, "session_perm_hex")
	iso := boolField(data, "isolated_session_keyring")
	entries := intField(data, "keyring_entries_count")
	status := strField(data, "status")

	kcmConn := false
	var peerPid int64
	if kcm, ok := data["kcm_socket"].(map[string]any); ok && kcm != nil {
		kcmConn = boolField(kcm, "connected")
		peerPid = intField(kcm, "peer_pid")
	}

	row := Row{
		ID:        "OP-01",
		Operative: "Souei",
		Module:    "keyring-probe",
		Target:    fmt.Sprintf("uid=%d", intField(data, "uid")),
		Detail:    fmt.Sprintf("keyring=%d isolated=%v kcm=%v", sessID, iso, kcmConn),
		PermOrTTL: fmt.Sprintf("perm=0x%s", permHex),
		SkewOrAux: fmt.Sprintf("keys=%d kcm_pid=%d", entries, peerPid),
		ExitCode:  exitCode,
		Status:    status,
		RawJSON:   data,
	}
	logLine := fmt.Sprintf("[SOUEI] status=%s exit=%d session_keyring=%d perm=0x%s entries=%d kcm_connected=%v peer_pid=%d",
		status, exitCode, sessID, permHex, entries, kcmConn, peerPid)
	return row, data, exitCode, logLine
}

func (s *Server) execChronoaCheck(req Request) (Row, map[string]any, int, string) {
	ccPath := req.CcachePath
	if ccPath == "" {
		ccPath = s.defaultCCachePath()
	}
	if req.WeakEnctype {
		rc4Path := filepath.Join(s.workDir, "krb5cc_rc4")
		s.mu.Lock()
		_ = writeDemoCCache(rc4Path, 3600, 23)
		s.mu.Unlock()
		ccPath = rc4Path
	} else if ccPath == s.defaultCCachePath() {
		s.mu.Lock()
		if _, err := os.Stat(ccPath); os.IsNotExist(err) {
			_ = writeDemoCCache(ccPath, 3600, 18)
		}
		s.mu.Unlock()
	}

	nowEpoch := time.Now().Unix()
	warnWin := req.WarnWindowSec
	if warnWin <= 0 {
		warnWin = 300
	}
	if req.Renew && warnWin < 86400 {
		warnWin = 86400
	}

	args := []string{
		"--ccache", ccPath,
		"--now", fmt.Sprintf("%d", nowEpoch),
		"--kdc-time", fmt.Sprintf("%d", nowEpoch-req.SkewSec),
		"--warn-window-sec", fmt.Sprintf("%d", warnWin),
		"--once",
		"--json",
	}
	if req.Renew {
		renewCmd := fmt.Sprintf("%q -refresh-ccache %q", s.selfExe, ccPath)
		args = append(args, "--renew-cmd", renewCmd)
	}

	data, exitCode, stderrStr, err := s.runBinaryJSON("chronoa", nil, args...)
	if err != nil {
		return errRow("OP-02", "Chronoa", "ccache-watchdog", ccPath, exitCode, err),
			nil, exitCode, fmt.Sprintf("[CHRONOA] error executing chronoa: %v (%s)", err, stderrStr)
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
	row := Row{
		ID:        "OP-02",
		Operative: "Chronoa",
		Module:    "ccache-watchdog",
		Target:    princ,
		Detail:    encName,
		PermOrTTL: fmt.Sprintf("ttl=%ds", ttl),
		SkewOrAux: fmt.Sprintf("skew=%ds", skew),
		ExitCode:  exitCode,
		Status:    status,
		RawJSON:   data,
	}
	logLine := fmt.Sprintf("[CHRONOA] status=%s exit=%d principal=%s enctype=%s ttl=%ds skew=%ds renewed=%v",
		status, exitCode, princ, encName, ttl, skew, boolField(data, "renewed"))
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
	target := req.PurgePath
	if target == "" {
		target = s.defaultPurgePath()
		s.mu.Lock()
		if _, err := os.Stat(target); os.IsNotExist(err) {
			_ = writeDemoSecretFile(target)
		}
		s.mu.Unlock()
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

func (s *Server) handleRequest(req Request) Response {
	ts := time.Now().Format("15:04:05")
	resp := Response{
		Action:    req.Action,
		Timestamp: ts,
	}

	switch req.Action {
	case "ping":
		bins := []string{"souei", "chronoa", "ranga", "libranga.so"}
		present := make(map[string]bool, len(bins))
		allReady := true
		for _, b := range bins {
			_, err := os.Stat(filepath.Join(s.tempestDir, b))
			ok := err == nil
			present[b] = ok
			if !ok {
				allReady = false
			}
		}
		resp.OK = allReady
		resp.Status = "READY"
		if !allReady {
			resp.Status = "MISSING_BINARIES"
			resp.ExitCode = 3
		}
		resp.Data = map[string]any{
			"server":           "ciel-teamserver",
			"tempest_dir":      s.tempestDir,
			"work_dir":         s.workDir,
			"default_ccache":   s.defaultCCachePath(),
			"default_purge":    s.defaultPurgePath(),
			"binaries_present": present,
			"pid":              os.Getpid(),
		}
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] ping status=%s tempest_dir=%s", ts, resp.Status, s.tempestDir)}

	case "souei_probe":
		row, data, rc, logLine := s.execSoueiProbe()
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}

	case "chronoa_check":
		row, data, rc, logLine := s.execChronoaCheck(req)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}

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

	case "ranga_purge":
		row, data, rc, logLine := s.execRangaPurge(req)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}

	case "full_sync":
		r1, d1, rc1, l1 := s.execSoueiProbe()
		r2, d2, rc2, l2 := s.execChronoaCheck(req)
		r3, d3, rc3, l3 := s.execRangaStatus(false, "")
		r4, d4, rc4, l4 := s.execRangaStatus(true, req.IsolateMode)
		resp.OK = true
		resp.Status = "SYNCED"
		if rc1 != 0 || rc2 != 0 || rc3 != 0 || rc4 != 0 {
			resp.Status = "ATTENTION"
		}
		resp.Rows = []Row{r1, r2, r3, r4}
		resp.Data = map[string]any{
			"souei":            d1,
			"chronoa":          d2,
			"ranga_baseline":   d3,
			"ranga_isolated":   d4,
			"default_ccache":   s.defaultCCachePath(),
			"default_purge":    s.defaultPurgePath(),
		}
		resp.Logs = []string{ts + " " + l1, ts + " " + l2, ts + " " + l3, ts + " " + l4}

	default:
		resp.OK = false
		resp.ExitCode = 1
		resp.Status = "UNKNOWN_ACTION"
		resp.Error = "Unknown action: " + req.Action
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] unknown action=%s", ts, req.Action)}
	}
	return resp
}

func (s *Server) serveConn(conn net.Conn) {
	defer conn.Close()
	scanner := bufio.NewScanner(conn)
	scanner.Buffer(make([]byte, 64*1024), 1024*1024)
	for scanner.Scan() {
		line := bytes.TrimSpace(scanner.Bytes())
		if len(line) == 0 {
			continue
		}
		var req Request
		var resp Response
		if err := json.Unmarshal(line, &req); err != nil {
			resp = Response{
				OK:        false,
				ExitCode:  1,
				Status:    "BAD_JSON",
				Timestamp: time.Now().Format("15:04:05"),
				Error:     err.Error(),
			}
		} else {
			resp = s.handleRequest(req)
		}
		encoded, err := json.Marshal(resp)
		if err != nil {
			continue
		}
		encoded = append(encoded, '\n')
		_, _ = conn.Write(encoded)
	}
}

func main() {
	defaultDir := os.Getenv("TEMPEST_BIN_DIR")
	if defaultDir == "" {
		defaultDir = defaultTempestDir
	}
	if absDir, err := filepath.Abs(defaultDir); err == nil {
		defaultDir = absDir
	}

	addr := flag.String("addr", "127.0.0.1:40056", "TCP listen address for Qt desktop clients")
	tempestDir := flag.String("tempest-dir", defaultDir, "Directory containing built Tempest binaries")
	workDir := flag.String("work-dir", "/tmp/ciel_workspace", "Workspace directory for CCACHE and purge demo artifacts")
	refreshPath := flag.String("refresh-ccache", "", "Internal helper: write fresh CCACHE v4 at path and exit")
	flag.Parse()

	if *refreshPath != "" {
		if err := writeDemoCCache(*refreshPath, 7200, 18); err != nil {
			fmt.Fprintf(os.Stderr, "ciel-teamserver: refresh-ccache failed: %v\n", err)
			os.Exit(1)
		}
		os.Exit(0)
	}

	selfExe, err := os.Executable()
	if err != nil {
		selfExe = os.Args[0]
	}

	resolvedTempestDir := *tempestDir
	if absTempest, absErr := filepath.Abs(resolvedTempestDir); absErr == nil {
		resolvedTempestDir = absTempest
	}
	if _, statErr := os.Stat(resolvedTempestDir); os.IsNotExist(statErr) && os.Getenv("TEMPEST_BIN_DIR") == "" {
		exeSibling := filepath.Clean(filepath.Join(filepath.Dir(selfExe), "..", "..", "tempest", "bin"))
		if _, sibErr := os.Stat(exeSibling); sibErr == nil {
			resolvedTempestDir = exeSibling
		}
	}

	srv := &Server{
		tempestDir: resolvedTempestDir,
		workDir:    *workDir,
		selfExe:    selfExe,
	}
	if err := srv.ensureWorkspaceArtifacts(); err != nil {
		fmt.Fprintf(os.Stderr, "ciel-teamserver: workspace init failed: %v\n", err)
		os.Exit(1)
	}
	kcmLn, err := srv.startKcmSocket()
	if err != nil {
		fmt.Fprintf(os.Stderr, "ciel-teamserver: kcm socket init failed: %v\n", err)
		os.Exit(1)
	}
	defer kcmLn.Close()

	ln, err := net.Listen("tcp", *addr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "ciel-teamserver: listen %s failed: %v\n", *addr, err)
		os.Exit(1)
	}
	defer ln.Close()

	fmt.Printf("[ciel-teamserver] listening on %s (tempest=%s, workspace=%s)\n",
		ln.Addr().String(), srv.tempestDir, srv.workDir)

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		<-sigCh
		_ = ln.Close()
		os.Exit(0)
	}()

	for {
		conn, err := ln.Accept()
		if err != nil {
			if errors.Is(err, net.ErrClosed) {
				return
			}
			continue
		}
		go srv.serveConn(conn)
	}
}
