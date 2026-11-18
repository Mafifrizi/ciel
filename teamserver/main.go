package main

import (
	"bufio"
	"bytes"
	"context"
	"crypto/rand"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"strings"
	"sync"
	"syscall"
	"time"
)

const defaultTempestDir = "../tempest/bin"

const (
	demoPacHex    = "02000000000000000a0000001a000000280000000000000001000000f800000048000000000000000000000000a0d90110006300690065006c005f006f007000730000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010001200040002000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000f40100000102000002000000080002000002000000000000000000000000000000000000000000000000000010000200000000000000000000020000100002000000000000000000000000000800000000000000080000006300690065006c005f006f0070007300020000000102000007000000000200000700000004010000000000051500000057040000ae080000050d0000"
	demoShadowHex = "0002000048000345434b3120000000111111111111111111111111111111111111111111111111111111111111111122222222222222222222222222222222222222222222222222222222222222222000011af8f9468674edde04d2975d3cc39345c5c7bf7ee0338e508955b7462d318ae2010004010100050010000611223344556677889900aabbccddeeff020007010108000900206196bdc5db01"
)

type Request struct {
	Action        string   `json:"action"`
	CcachePath    string   `json:"ccache_path,omitempty"`
	SkewSec       int64    `json:"skew_sec,omitempty"`
	WarnWindowSec int64    `json:"warn_window_sec,omitempty"`
	Renew         bool     `json:"renew,omitempty"`
	WeakEnctype   bool     `json:"weak_enctype,omitempty"`
	ExpiredTicket bool     `json:"expired_ticket,omitempty"`
	WatchInotify  bool     `json:"watch_inotify,omitempty"`
	IsolateMode   string   `json:"isolate_mode,omitempty"`
	PurgePath     string   `json:"purge_path,omitempty"`
	RevokeSession bool     `json:"revoke_session,omitempty"`
	Subcommand    string   `json:"subcommand,omitempty"`
	TargetArg     string   `json:"target_arg,omitempty"`
	Args          []string `json:"args,omitempty"`
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
	tanukiBin   string
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

func writeDemoKeytab(path string) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o700); err != nil {
		return err
	}
	var entry bytes.Buffer
	realm := []byte("TEMPEST.LOCAL")
	comp := []byte("ciel")
	key := bytes.Repeat([]byte{0x2A}, 32)
	_ = binary.Write(&entry, binary.BigEndian, int16(1))
	_ = binary.Write(&entry, binary.BigEndian, uint16(len(realm)))
	entry.Write(realm)
	_ = binary.Write(&entry, binary.BigEndian, uint16(len(comp)))
	entry.Write(comp)
	_ = binary.Write(&entry, binary.BigEndian, uint32(1))
	_ = binary.Write(&entry, binary.BigEndian, uint32(time.Now().Unix()))
	entry.WriteByte(3)
	_ = binary.Write(&entry, binary.BigEndian, int16(18))
	_ = binary.Write(&entry, binary.BigEndian, uint16(len(key)))
	entry.Write(key)
	_ = binary.Write(&entry, binary.BigEndian, uint32(3))

	var kt bytes.Buffer
	kt.Write([]byte{0x05, 0x02})
	_ = binary.Write(&kt, binary.BigEndian, int32(entry.Len()))
	kt.Write(entry.Bytes())
	return os.WriteFile(path, kt.Bytes(), 0o600)
}

func writeDemoAdcsTemplates(path string) error {
	payload := `{"templates":[{"name":"CielSubCA-ESC1","msPKI-Certificate-Name-Flag":1,"msPKI-Enrollment-Flag":0,"pKIExtendedKeyUsage":["1.3.6.1.5.5.7.3.2"],"authorized_signatures":0}]}`
	return os.WriteFile(path, []byte(payload), 0o600)
}

func writeDemoJwt(path string) error {
	jwt := "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJzcGlmZmU6Ly90ZW1wZXN0LmxvY2FsL3dvcmtsb2FkL2NpZWwiLCJpc3MiOiJodHRwczovL3N0cy50ZW1wZXN0LmxvY2FsIiwiYXVkIjoiY2llbC10ZWFtc2VydmVyIiwic2NvcGUiOiJ0YW51a2kudG9rZW4gcmVhZCIsImV4cCI6MTk5OTk5OTk5OX0.c2lnbmF0dXJl"
	return os.WriteFile(path, []byte(jwt), 0o600)
}

func (s *Server) defaultCCachePath() string {
	return filepath.Join(s.workDir, "krb5cc_ciel")
}

func (s *Server) defaultPurgePath() string {
	return filepath.Join(s.workDir, "PurgeTarget.keytab")
}

func (s *Server) defaultKeytabPath() string {
	return filepath.Join(s.workDir, "service.keytab")
}

func (s *Server) defaultAdcsPath() string {
	return filepath.Join(s.workDir, "adcs_templates.json")
}

func (s *Server) defaultPacPath() string {
	return filepath.Join(s.workDir, "demo.pac")
}

func (s *Server) defaultJwtPath() string {
	return filepath.Join(s.workDir, "workload_token.jwt")
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
	kt := s.defaultKeytabPath()
	if _, err := os.Stat(kt); os.IsNotExist(err) {
		if err := writeDemoKeytab(kt); err != nil {
			return err
		}
	}
	adcs := s.defaultAdcsPath()
	if _, err := os.Stat(adcs); os.IsNotExist(err) {
		if err := writeDemoAdcsTemplates(adcs); err != nil {
			return err
		}
	}
	jwt := s.defaultJwtPath()
	if _, err := os.Stat(jwt); os.IsNotExist(err) {
		if err := writeDemoJwt(jwt); err != nil {
			return err
		}
	}
	pac := s.defaultPacPath()
	if _, err := os.Stat(pac); os.IsNotExist(err) {
		rawPac, err := hex.DecodeString(demoPacHex)
		if err != nil {
			return err
		}
		if err := os.WriteFile(pac, rawPac, 0o600); err != nil {
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

func (s *Server) runJSONProcess(binPath string, extraEnv []string, args ...string) (map[string]any, int, string, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 15*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, binPath, args...)
	if s.workDir != "" {
		cmd.Dir = s.workDir
	}
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

func (s *Server) execSoueiProbe(watchInotify bool) (Row, map[string]any, int, string) {
	args := []string{"--probe"}
	if s.kcmSockPath != "" {
		args = append(args, "--kcm-socket", s.kcmSockPath)
	}
	var doneTrigger chan struct{}
	if watchInotify {
		cc := s.defaultCCachePath()
		s.mu.Lock()
		if _, err := os.Stat(cc); os.IsNotExist(err) {
			_ = writeDemoCCache(cc, 3600, 18)
		}
		s.mu.Unlock()
		args = append(args, "--watch-file", cc, "--timeout-ms", "300")
		doneTrigger = make(chan struct{})
		go func() {
			defer close(doneTrigger)
			time.Sleep(60 * time.Millisecond)
			_, _ = os.ReadFile(cc)
		}()
	}
	args = append(args, "--json")
	data, exitCode, stderrStr, err := s.runBinaryJSON("souei", nil, args...)
	if doneTrigger != nil {
		<-doneTrigger
	}
	modName := "keyring-probe"
	if watchInotify {
		modName = "inotify-watch"
	}
	if err != nil {
		return errRow("OP-01", "Souei", modName, "/proc/keys", exitCode, err),
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
	inoCount := 0
	if ino, ok := data["inotify"].(map[string]any); ok && ino != nil {
		if evs, ok := ino["events"].([]any); ok {
			inoCount = len(evs)
		}
	}

	detail := fmt.Sprintf("keyring=%d isolated=%v kcm=%v", sessID, iso, kcmConn)
	aux := fmt.Sprintf("keys=%d kcm_pid=%d", entries, peerPid)
	if watchInotify {
		detail = fmt.Sprintf("inotify_events=%d kcm=%v", inoCount, kcmConn)
		aux = fmt.Sprintf("ino=%d kcm_pid=%d", inoCount, peerPid)
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
	logLine := fmt.Sprintf("[SOUEI] status=%s exit=%d session_keyring=%d perm=0x%s entries=%d kcm_connected=%v peer_pid=%d inotify_events=%d",
		status, exitCode, sessID, permHex, entries, kcmConn, peerPid, inoCount)
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
	} else if req.ExpiredTicket {
		expPath := filepath.Join(s.workDir, "krb5cc_expired")
		s.mu.Lock()
		_ = writeDemoCCache(expPath, -60, 18)
		s.mu.Unlock()
		ccPath = expPath
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

func (s *Server) buildTanukiArgs(subcmd string, req Request) ([]string, string) {
	arg := strings.TrimSpace(req.TargetArg)
	orDefault := func(fallback string) string {
		if arg != "" {
			return arg
		}
		return fallback
	}
	kt := s.defaultKeytabPath()
	cc := s.defaultCCachePath()

	if len(req.Args) > 0 {
		out := []string{subcmd}
		hasJson := false
		for _, a := range req.Args {
			if a == "--json" || a == "-j" {
				hasJson = true
			}
			out = append(out, a)
		}
		if !hasJson {
			out = append(out, "--json")
		}
		return out, strings.Join(req.Args, " ")
	}

	switch subcmd {
	case "doctor":
		return []string{"doctor", "--keytab", kt, "--ccache", cc, "--opsec", "--json"}, "keytab+ccache"
	case "fix":
		return []string{"fix", "--keytab", kt, "--ccache", cc, "--realm", "TEMPEST.LOCAL", "--kdc", "127.0.0.1", "--dry-run", "--json"}, "dry-run"
	case "keytab":
		t := orDefault(kt)
		return []string{"keytab", t, "--json"}, filepath.Base(t)
	case "config":
		r := orDefault("TEMPEST.LOCAL")
		return []string{"config", "--realm", r, "--kdc", "127.0.0.1", "--enforce-aes", "--fast", "--stdout", "--json"}, r
	case "triage":
		if arg != "" {
			return []string{"triage", arg, "--json"}, arg
		}
		return []string{"triage", "SKEW", "--json"}, "SKEW"
	case "ladder":
		return []string{"ladder", "--json"}, "5-rungs"
	case "nhi":
		if arg != "" {
			parts := strings.Fields(arg)
			return append(append([]string{"nhi"}, parts...), "--json"), arg
		}
		return []string{"nhi", "mesh", "--json"}, "mesh"
	case "token":
		t := orDefault(s.defaultJwtPath())
		return []string{"token", t, "--json"}, filepath.Base(t)
	case "blast":
		if arg != "" {
			return []string{"blast", arg, "--json"}, arg
		}
		return []string{"blast", "--keytab", kt, "--ccache", cc, "--json"}, "keytab+ccache"
	case "adcs":
		t := orDefault(s.defaultAdcsPath())
		return []string{"adcs", "-f", t, "--json"}, filepath.Base(t)
	case "pac":
		t := orDefault(s.defaultPacPath())
		return []string{"pac", t, "--json"}, filepath.Base(t)
	case "shadow":
		t := orDefault(demoShadowHex)
		return []string{"shadow", t, "--json"}, "keyCredLink"
	case "kcm":
		t := orDefault(cc)
		outDir := filepath.Join(s.workDir, "kcm_out")
		return []string{"kcm", "-f", t, "-o", outDir, "--json"}, filepath.Base(t)
	case "purge":
		t := orDefault(s.defaultPurgePath())
		if t == s.defaultPurgePath() {
			s.mu.Lock()
			if _, err := os.Stat(t); os.IsNotExist(err) {
				_ = writeDemoSecretFile(t)
			}
			s.mu.Unlock()
		}
		return []string{"purge", t, "--json"}, filepath.Base(t)
	case "skill":
		return []string{"skill", "--json"}, "manifest"
	default:
		out := []string{subcmd}
		if arg != "" {
			out = append(out, strings.Fields(arg)...)
		}
		out = append(out, "--json")
		return out, orDefault(subcmd)
	}
}

func (s *Server) execTanuki(req Request) (Row, map[string]any, int, string) {
	subcmd := strings.ToLower(strings.TrimSpace(req.Subcommand))
	if subcmd == "" {
		subcmd = "doctor"
	}
	if s.tanukiBin == "" {
		err := errors.New("tanuki binary not found")
		return errRow("OP-06", "Tanuki", subcmd, "-", 3, err),
			nil, 3, "[TANUKI:" + subcmd + "] error: tanuki binary not found"
	}

	args, targetLabel := s.buildTanukiArgs(subcmd, req)
	data, exitCode, stderrStr, err := s.runJSONProcess(s.tanukiBin, nil, args...)
	if err != nil {
		return errRow("OP-06", "Tanuki", subcmd, targetLabel, exitCode, err),
			nil, exitCode, fmt.Sprintf("[TANUKI:%s] error: %v (%s)", subcmd, err, stderrStr)
	}

	status := strings.ToUpper(strField(data, "status"))
	if status == "" {
		switch subcmd {
		case "keytab", "triage", "ladder", "kcm":
			status = "PARSED"
		case "skill":
			status = "READY"
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
		aux = fmt.Sprintf("entries=%d", count)
		perm = "rfc4120"
	case "ladder":
		aux = fmt.Sprintf("rungs=%d", count)
		perm = "decision"
	case "adcs":
		if findings, ok := data["findings"].([]any); ok {
			aux = fmt.Sprintf("findings=%d", len(findings))
			if len(findings) > 0 {
				status = "FINDINGS"
			} else {
				status = "CLEAN"
			}
		}
		perm = "esc1-11"
	case "triage":
		if code := strField(data, "code"); code != "" {
			aux = code
			if status == "" || status == "PARSED" {
				status = "MATCHED"
			}
		} else {
			aux = fmt.Sprintf("codes=%d", count)
		}
		perm = "protocol"
	case "pac":
		if bc := intField(data, "buffer_count"); bc > 0 {
			aux = fmt.Sprintf("buffers=%d", bc)
		}
		perm = "ms-pac"
	case "shadow":
		if ver := strField(data, "version_str"); ver != "" {
			aux = "v" + ver
		}
		perm = "ms-adts"
	case "blast":
		if rt := strField(data, "risk_tier"); rt != "" {
			aux = "tier=" + rt
			if status == "OK" {
				status = rt
			}
		}
		perm = "ad-tier"
	case "token":
		aux = strField(data, "risk_level")
		if aux == "" {
			aux = "jwt"
		}
		perm = "rfc8693"
	case "nhi":
		if cnt := intField(data, "count"); cnt >= 0 {
			aux = fmt.Sprintf("identities=%d", cnt)
		}
		perm = "mesh"
	case "config":
		aux = strField(data, "realm")
		perm = "zero-dns"
	case "fix":
		aux = "dry-run"
		perm = "self-heal"
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

func (s *Server) handleRequest(req Request) Response {
	ts := time.Now().Format("15:04:05")
	resp := Response{
		Action:    req.Action,
		Timestamp: ts,
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
		resp.Data = map[string]any{
			"server":           "ciel-teamserver",
			"tempest_dir":      s.tempestDir,
			"tanuki_bin":       s.tanukiBin,
			"work_dir":         s.workDir,
			"default_ccache":   s.defaultCCachePath(),
			"default_keytab":   s.defaultKeytabPath(),
			"default_purge":    s.defaultPurgePath(),
			"binaries_present": present,
			"pid":              os.Getpid(),
		}
		resp.Logs = []string{fmt.Sprintf("%s [TEAMSERVER] ping status=%s tempest_dir=%s tanuki=%s", ts, resp.Status, s.tempestDir, s.tanukiBin)}

	case "souei_probe":
		row, data, rc, logLine := s.execSoueiProbe(req.WatchInotify)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}

	case "souei_watch":
		row, data, rc, logLine := s.execSoueiProbe(true)
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

	case "tanuki_exec":
		row, data, rc, logLine := s.execTanuki(req)
		resp.OK = true
		resp.ExitCode = rc
		resp.Status = row.Status
		resp.Data = data
		resp.Rows = []Row{row}
		resp.Logs = []string{ts + " " + logLine}

	case "full_sync":
		r1, d1, rc1, l1 := s.execSoueiProbe(false)
		r2, d2, rc2, l2 := s.execChronoaCheck(req)
		r3, d3, rc3, l3 := s.execRangaStatus(false, "")
		r4, d4, rc4, l4 := s.execRangaStatus(true, req.IsolateMode)
		r5, d5, rc5, l5 := s.execTanuki(Request{Subcommand: "doctor"})
		resp.OK = true
		resp.Status = "SYNCED"
		if rc1 != 0 || rc2 != 0 || rc3 != 0 || rc4 != 0 || rc5 != 0 {
			resp.Status = "ATTENTION"
		}
		resp.Rows = []Row{r1, r2, r3, r4, r5}
		resp.Data = map[string]any{
			"souei":            d1,
			"chronoa":          d2,
			"ranga_baseline":   d3,
			"ranga_isolated":   d4,
			"tanuki_doctor":    d5,
			"default_ccache":   s.defaultCCachePath(),
			"default_keytab":   s.defaultKeytabPath(),
			"default_purge":    s.defaultPurgePath(),
		}
		resp.Logs = []string{ts + " " + l1, ts + " " + l2, ts + " " + l3, ts + " " + l4, ts + " " + l5}

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

	srv := &Server{
		tempestDir: resolvedTempestDir,
		tanukiBin:  findTanukiBin(selfExe, resolvedTempestDir),
		workDir:    resolvedWorkDir,
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
