package main

import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"slices"
	"strings"
	"testing"
	"time"
)

func buildMockBinary(t *testing.T, dir string) string {
	t.Helper()
	src := `package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

func main() {
	name := strings.TrimSuffix(filepath.Base(os.Args[0]), ".exe")
	args := os.Args[1:]
	if logPath := os.Getenv("CIEL_MOCK_ARGS_LOG"); logPath != "" {
		_ = os.WriteFile(logPath, []byte(name+" "+strings.Join(args, " ")), 0o600)
	}
	if os.Getenv("CIEL_MOCK_MODE") != "" {
		name = os.Getenv("CIEL_MOCK_MODE")
	}
	switch name {
	case "souei":
		hasKcm := false
		for _, a := range args {
			if a == "--kcm-socket" {
				hasKcm = true
			}
		}
		if hasKcm {
			fmt.Println(` + "`" + `{"agent":"souei","status":"OK","exit_code":0,"uid":1000,"session_keyring_id":12345,"session_perm_hex":"3f000000","isolated_session_keyring":true,"keyring_entries_count":2,"kcm_socket":{"checked":true,"connected":true,"peer_pid":999}}` + "`" + `)
		} else {
			fmt.Println(` + "`" + `{"agent":"souei","status":"OK","exit_code":0,"uid":1000,"session_keyring_id":12345,"session_perm_hex":"3f000000","isolated_session_keyring":true,"keyring_entries_count":2,"kcm_socket":{"checked":false,"connected":false,"peer_pid":-1}}` + "`" + `)
		}
	case "chronoa":
		if os.Getenv("CIEL_MOCK_CHRONOA_MISSING") == "1" {
			fmt.Println(` + "`" + `{"agent":"chronoa","status":"MISSING","exit_code":3,"default_principal":"","ttl_sec":0,"skew_sec":0,"primary_ticket":null,"error":"CCACHE file not found"}` + "`" + `)
			os.Exit(3)
		}
		if os.Getenv("CIEL_MOCK_CHRONOA_SKEW") == "1" {
			fmt.Println(` + "`" + `{"agent":"chronoa","status":"CLOCK_SKEW_VIOLATION","exit_code":2,"default_principal":"svc@CORP.LOCAL","ttl_sec":3600,"skew_sec":350,"primary_ticket":{"enctype_name":"aes256-cts-hmac-sha1-96"}}` + "`" + `)
			os.Exit(2)
		}
		fmt.Println(` + "`" + `{"agent":"chronoa","status":"HEALTHY","exit_code":0,"default_principal":"svc@CORP.LOCAL","ttl_sec":3600,"skew_sec":0,"primary_ticket":{"enctype_name":"aes256-cts-hmac-sha1-96"}}` + "`" + `)
	case "ranga":
		for i, a := range args {
			if a == "--purge" && i+1 < len(args) {
				fmt.Printf(` + "`" + `{"agent":"ranga","mode":"purge","exit_code":0,"path":%q,"wiped_bytes":64,"passes":3,"removed":true,"session_revoked":false}` + "`" + `, args[i+1])
				return
			}
		}
		fmt.Println(` + "`" + `{"agent":"ranga","session_keyring_id":54321,"perm_hex":"3f000000","possessor_only":true,"description":"_ses","child_key_count":1}` + "`" + `)
	case "tanuki":
		sub := ""
		if len(args) > 0 {
			sub = args[0]
		}
		switch sub {
		case "keytab":
			fmt.Println(` + "`" + `[{"principal":"svc@CORP.LOCAL","vno":1},{"principal":"host/app@CORP.LOCAL","vno":1}]` + "`" + `)
		case "ladder":
			fmt.Println(` + "`" + `[{"rung":1},{"rung":2},{"rung":3},{"rung":4},{"rung":5}]` + "`" + `)
		case "kcm":
			fmt.Println(` + "`" + `[{"offset":0},{"offset":64},{"offset":128}]` + "`" + `)
		case "blast":
			if os.Getenv("CIEL_MOCK_BLAST_TIER") == "0" {
				fmt.Println(` + "`" + `{"status":"SUCCESS","tier":0,"tier_label":"Tier 0 (Control Plane)","blast_score":95,"severity":"CRITICAL"}` + "`" + `)
			} else {
				fmt.Println(` + "`" + `{"status":"SUCCESS","tier":2,"tier_label":"Tier 2 (User Endpoint)","blast_score":20,"severity":"LOW"}` + "`" + `)
			}
		case "token":
			if os.Getenv("CIEL_MOCK_TOKEN_ERR") == "1" {
				fmt.Println(` + "`" + `{"status":"ERROR","reason_code":"CORRUPT_DATA","exit_code":4}` + "`" + `)
				os.Exit(4)
			}
			fmt.Println(` + "`" + `{"valid":false,"identity_type":"SPIFFE SVID","security_evaluation":{"risk_level":"HIGH"}}` + "`" + `)
		case "purge":
			fmt.Println(` + "`" + `{"status":"SUCCESS","shredded_count":1,"memory_zeroed":true}` + "`" + `)
		case "nhi":
			if os.Getenv("CIEL_MOCK_NHI_MODE") == "error" {
				fmt.Println(` + "`" + `{"status":"ERROR","reason_code":"MISSING_ARGUMENT","exit_code":1,"message":"bad nhi arg"}` + "`" + `)
				os.Exit(1)
			}
			if len(args) > 1 && args[1] == "mesh" {
				fmt.Println(` + "`" + `{"status":"NONE_DISCOVERED","discovered_identities":[],"count":0}` + "`" + `)
			} else if os.Getenv("CIEL_MOCK_NHI_FOUND") == "1" {
				fmt.Println(` + "`" + `{"discovered_tokens":["/var/run/secrets/kubernetes.io/serviceaccount/token"]}` + "`" + `)
			} else {
				fmt.Println(` + "`" + `{"discovered_tokens":[]}` + "`" + `)
			}
		case "doctor":
			fmt.Println(` + "`" + `{"status":"PASS","summary":{"passed":5,"warnings":0,"failures":0}}` + "`" + `)
		default:
			fmt.Println(` + "`" + `{"status":"OK"}` + "`" + `)
		}
	}
}
`
	srcPath := filepath.Join(dir, "mockbin.go")
	if err := os.WriteFile(srcPath, []byte(src), 0o600); err != nil {
		t.Fatalf("write mockbin.go: %v", err)
	}
	ext := ""
	if runtime.GOOS == "windows" {
		ext = ".exe"
	}
	baseBin := filepath.Join(dir, "tanuki"+ext)
	cmd := exec.Command("go", "build", "-o", baseBin, srcPath)
	if out, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("build mock binary failed: %v\n%s", err, string(out))
	}
	raw, err := os.ReadFile(baseBin)
	if err != nil {
		t.Fatalf("read built mock binary: %v", err)
	}
	for _, tool := range []string{"souei", "chronoa", "ranga"} {
		dst := filepath.Join(dir, tool+ext)
		if err := os.WriteFile(dst, raw, 0o755); err != nil {
			t.Fatalf("copy mock %s: %v", tool, err)
		}
	}
	return baseBin
}

func stageTestKeytab(t *testing.T, path string) {
	t.Helper()
	var entry bytes.Buffer
	realm := []byte("CORP.LOCAL")
	comp := []byte("svc")
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
	if err := os.WriteFile(path, kt.Bytes(), 0o600); err != nil {
		t.Fatalf("stage keytab: %v", err)
	}
}

func TestIntFieldConcreteTypes(t *testing.T) {
	m := map[string]any{
		"f64":    float64(42),
		"i":      int(7),
		"i64":    int64(9001),
		"i32":    int32(13),
		"u":      uint(5),
		"u64":    uint64(19),
		"jint":   json.Number("123"),
		"jfloat": json.Number("88.0"),
		"str":    "999",
	}
	if got := intField(m, "f64"); got != 42 {
		t.Fatalf("float64: got %d want 42", got)
	}
	if got := intField(m, "i"); got != 7 {
		t.Fatalf("int: got %d want 7", got)
	}
	if got := intField(m, "i64"); got != 9001 {
		t.Fatalf("int64: got %d want 9001", got)
	}
	if got := intField(m, "i32"); got != 13 {
		t.Fatalf("int32: got %d want 13", got)
	}
	if got := intField(m, "u"); got != 5 {
		t.Fatalf("uint: got %d want 5", got)
	}
	if got := intField(m, "u64"); got != 19 {
		t.Fatalf("uint64: got %d want 19", got)
	}
	if got := intField(m, "jint"); got != 123 {
		t.Fatalf("json.Number int: got %d want 123", got)
	}
	if got := intField(m, "jfloat"); got != 88 {
		t.Fatalf("json.Number float: got %d want 88", got)
	}
	if got := intField(m, "missing"); got != 0 {
		t.Fatalf("missing: got %d want 0", got)
	}
	if got := intField(m, "str"); got != 0 {
		t.Fatalf("string: got %d want 0", got)
	}
	if got := intField(nil, "f64"); got != 0 {
		t.Fatalf("nil map: got %d want 0", got)
	}
}

func TestAuthTokenEnforcementAndSecurityGuards(t *testing.T) {
	binDir := t.TempDir()
	tanukiBin := buildMockBinary(t, binDir)
	workDir := t.TempDir()

	srv := &Server{
		tempestDir: binDir,
		tanukiBin:  tanukiBin,
		workDir:    workDir,
		selfExe:    tanukiBin,
	}
	if err := srv.initAuthToken(""); err != nil {
		t.Fatalf("initAuthToken: %v", err)
	}
	if srv.authToken == "" || len(srv.authToken) != 64 {
		t.Fatalf("expected 64-char hex auth token, got %q", srv.authToken)
	}
	// Ensure no demo artifacts were created in workDir
	entries, err := os.ReadDir(workDir)
	if err != nil {
		t.Fatalf("ReadDir workDir: %v", err)
	}
	if len(entries) != 1 || entries[0].Name() != "teamserver.token" {
		t.Fatalf("expected only teamserver.token in clean workDir, got %v", entries)
	}

	// Missing or invalid auth_token must be rejected
	unauth := srv.handleRequest(Request{Action: "ping"})
	if unauth.OK || unauth.Status != "UNAUTHORIZED" {
		t.Fatalf("expected UNAUTHORIZED without token, got %+v", unauth)
	}
	wrong := srv.handleRequest(Request{AuthToken: "deadbeef", Action: "full_sync"})
	if wrong.OK || wrong.Status != "UNAUTHORIZED" {
		t.Fatalf("expected UNAUTHORIZED with wrong token, got %+v", wrong)
	}

	// Disallowed tanuki subcommand must be rejected
	badSub := srv.handleRequest(Request{
		AuthToken:  srv.authToken,
		Action:     "tanuki_exec",
		Subcommand: "rm_rf",
	})
	if badSub.Status != "USAGE_ERROR" || badSub.ExitCode != 1 {
		t.Fatalf("expected disallowed tanuki subcommand to fail with USAGE_ERROR, got %+v", badSub)
	}

	// Interactive -i / --interactive flag must be blocked on both Args and TargetArg
	badInteractive := srv.handleRequest(Request{
		AuthToken:  srv.authToken,
		Action:     "tanuki_exec",
		Subcommand: "doctor",
		Args:       []string{"--interactive"},
	})
	if badInteractive.Status != "USAGE_ERROR" || badInteractive.ExitCode != 1 {
		t.Fatalf("expected --interactive to be blocked, got %+v", badInteractive)
	}
	badTargetInteractive := srv.handleRequest(Request{
		AuthToken:  srv.authToken,
		Action:     "tanuki_exec",
		Subcommand: "triage",
		TargetArg:  "--interactive",
	})
	if badTargetInteractive.Status != "USAGE_ERROR" || badTargetInteractive.ExitCode != 1 {
		t.Fatalf("expected target_arg --interactive to be blocked, got %+v", badTargetInteractive)
	}
	badAdcsDash := srv.handleRequest(Request{
		AuthToken:  srv.authToken,
		Action:     "tanuki_exec",
		Subcommand: "adcs",
		TargetArg:  "--interactive",
	})
	if badAdcsDash.Status != "USAGE_ERROR" || badAdcsDash.ExitCode != 1 {
		t.Fatalf("expected adcs dash target_arg to be blocked, got %+v", badAdcsDash)
	}

	// Purge without path or targeting protected system paths/prefixes must be rejected
	emptyPurge := srv.handleRequest(Request{AuthToken: srv.authToken, Action: "ranga_purge"})
	if emptyPurge.Status != "USAGE_ERROR" || emptyPurge.ExitCode != 1 {
		t.Fatalf("expected empty purge_path to be rejected, got %+v", emptyPurge)
	}
	etcPurge := srv.handleRequest(Request{AuthToken: srv.authToken, Action: "ranga_purge", PurgePath: "/etc/passwd"})
	if etcPurge.Status != "USAGE_ERROR" || etcPurge.ExitCode != 1 {
		t.Fatalf("expected /etc/passwd purge to be rejected, got %+v", etcPurge)
	}
	usrPurge := srv.handleRequest(Request{AuthToken: srv.authToken, Action: "ranga_purge", PurgePath: "/usr/bin/ls"})
	if usrPurge.Status != "USAGE_ERROR" || usrPurge.ExitCode != 1 {
		t.Fatalf("expected /usr/bin/ls purge to be rejected, got %+v", usrPurge)
	}

	// Child env must strip CIEL_AUTH_TOKEN
	t.Setenv("CIEL_AUTH_TOKEN", "super-secret-token")
	childEnv := sanitizedChildEnv("FOO=bar")
	for _, kv := range childEnv {
		if strings.HasPrefix(kv, "CIEL_AUTH_TOKEN=") {
			t.Fatalf("CIEL_AUTH_TOKEN leaked into sanitizedChildEnv: %s", kv)
		}
	}
}

func TestBuildTanukiArgsRealHostAndExplicitTargets(t *testing.T) {
	workDir := t.TempDir()
	srv := &Server{workDir: workDir}

	// Without configured overrides, doctor uses real host defaults (no fake workspace krb5.conf)
	argsHost, lblHost, err := srv.buildTanukiArgs("doctor", Request{Subcommand: "doctor"}, true)
	if err != nil {
		t.Fatalf("doctor host args error: %v", err)
	}
	if lblHost != "host" || slices.Contains(argsHost, "--krb5-conf") {
		t.Fatalf("host doctor should not inject fake --krb5-conf: %v", argsHost)
	}
	if !slices.Contains(argsHost, "--opsec") {
		t.Fatalf("explicit doctor missing --opsec: %v", argsHost)
	}

	// With explicit overrides, doctor forwards them
	customConf := filepath.Join(workDir, "custom_krb5.conf")
	argsCfg, lblCfg, err := srv.buildTanukiArgs("doctor", Request{
		Subcommand:   "doctor",
		Krb5ConfPath: customConf,
	}, false)
	if err != nil {
		t.Fatalf("doctor configured args error: %v", err)
	}
	if lblCfg != "configured" || !slices.Contains(argsCfg, "--krb5-conf") || !slices.Contains(argsCfg, customConf) {
		t.Fatalf("configured doctor missing --krb5-conf %s: %v", customConf, argsCfg)
	}
	if slices.Contains(argsCfg, "--opsec") {
		t.Fatalf("full_sync doctor should omit --opsec: %v", argsCfg)
	}

	// Configured target paths are forwarded even when req.Args is non-empty
	customKT := filepath.Join(workDir, "svc.keytab")
	customCC := filepath.Join(workDir, "svc.ccache")
	argsVerbose, _, err := srv.buildTanukiArgs("doctor", Request{
		Subcommand:   "doctor",
		KeytabPath:   customKT,
		CcachePath:   customCC,
		Krb5ConfPath: customConf,
		Args:         []string{"--verbose"},
	}, true)
	if err != nil {
		t.Fatalf("doctor verbose args error: %v", err)
	}
	if !slices.Contains(argsVerbose, "--verbose") || !slices.Contains(argsVerbose, "--keytab") || !slices.Contains(argsVerbose, customKT) || !slices.Contains(argsVerbose, "--ccache") || !slices.Contains(argsVerbose, customCC) {
		t.Fatalf("configured paths dropped when Args provided: %v", argsVerbose)
	}

	tgsArgs, _, err := srv.buildTanukiArgs("tgs", Request{
		Subcommand: "tgs",
		CcachePath: customCC,
		Args:       []string{"--spn", "HTTP/dc01@CORP.LOCAL", "--kdc", "10.0.0.1"},
	}, true)
	if err != nil || !slices.Contains(tgsArgs, "--ccache") || !slices.Contains(tgsArgs, customCC) {
		t.Fatalf("tgs with Args did not forward configured ccache: args=%v err=%v", tgsArgs, err)
	}

	// Quoted target_arg preserves spaces inside quotes
	cfgArgs, _, err := srv.buildTanukiArgs("config", Request{
		Subcommand: "config",
		TargetArg:  `--realm CORP.LOCAL --kdc 10.0.0.1 -o "/tmp/my dir/krb5.conf"`,
	}, true)
	if err != nil || !slices.Contains(cfgArgs, "/tmp/my dir/krb5.conf") {
		t.Fatalf("quoted config path broken: args=%v err=%v", cfgArgs, err)
	}

	// config via Args without -o/--out must automatically default to --stdout
	cfgStdoutArgs, _, err := srv.buildTanukiArgs("config", Request{
		Subcommand: "config",
		Args:       []string{"--realm", "CORP.LOCAL", "--kdc", "10.0.0.1"},
	}, true)
	if err != nil || !slices.Contains(cfgStdoutArgs, "--stdout") {
		t.Fatalf("config via Args missing default --stdout: args=%v err=%v", cfgStdoutArgs, err)
	}

	// kcm via Args without -o/--out must default to workDir/kcm_out
	kcmArgs, _, err := srv.buildTanukiArgs("kcm", Request{
		Subcommand: "kcm",
		Args:       []string{"-f", "/tmp/test.kcm"},
	}, true)
	if err != nil || !slices.Contains(kcmArgs, "-o") {
		t.Fatalf("kcm via Args missing default -o: args=%v err=%v", kcmArgs, err)
	}

	// purge via Args must enforce validatePurgeTarget on protected system paths
	if _, _, pErr := srv.buildTanukiArgs("purge", Request{
		Subcommand: "purge",
		Args:       []string{"/etc/passwd", "--json"},
	}, true); pErr == nil {
		t.Fatalf("expected purge via Args on /etc/passwd to be rejected")
	}
	if _, _, pErr := srv.buildTanukiArgs("purge", Request{
		Subcommand: "purge",
		Args:       []string{"--target=/etc/passwd"},
	}, true); pErr == nil {
		t.Fatalf("expected purge via Args --target=/etc/passwd to be rejected")
	}

	// fix defaults to --dry-run, while --apply translates to omitting --dry-run (tanuki has no --apply flag)
	fixDefault, _, err := srv.buildTanukiArgs("fix", Request{
		Subcommand: "fix",
		Args:       []string{"--realm", "CORP.LOCAL"},
	}, true)
	if err != nil || !slices.Contains(fixDefault, "--dry-run") {
		t.Fatalf("fix with Args should default to --dry-run: args=%v err=%v", fixDefault, err)
	}
	fixApply, _, err := srv.buildTanukiArgs("fix", Request{
		Subcommand: "fix",
		Args:       []string{"--apply"},
	}, true)
	if err != nil || slices.Contains(fixApply, "--apply") || slices.Contains(fixApply, "--dry-run") {
		t.Fatalf("fix --apply should strip --apply and omit --dry-run: args=%v err=%v", fixApply, err)
	}

	// Explicit KCM: scheme is parsed cleanly without treating as FILE
	kcmCC, kcmScheme, err := resolveCCachePath("KCM:1000")
	if err != nil || kcmCC != "KCM:1000" || kcmScheme != "KCM" {
		t.Fatalf("explicit KCM scheme mismatch: path=%q scheme=%q err=%v", kcmCC, kcmScheme, err)
	}

	nhiDefault, lbl, err := srv.buildTanukiArgs("nhi", Request{Subcommand: "nhi"}, true)
	if err != nil || lbl != "scan" || len(nhiDefault) < 2 || nhiDefault[1] != "scan" {
		t.Fatalf("default nhi should run scan, got args=%v label=%q err=%v", nhiDefault, lbl, err)
	}

	// Missing required arguments fail cleanly instead of fabricating demo blobs
	for _, sub := range []string{"token", "adcs", "pac", "shadow", "blast", "config", "purge"} {
		if _, _, bErr := srv.buildTanukiArgs(sub, Request{Subcommand: sub}, true); bErr == nil {
			t.Fatalf("expected subcommand %s without target to return usage error", sub)
		}
	}
}

func TestExecTanukiSchemaMappings(t *testing.T) {
	binDir := t.TempDir()
	tanukiBin := buildMockBinary(t, binDir)
	workDir := t.TempDir()
	argsLog := filepath.Join(workDir, "args.log")

	keytabPath := filepath.Join(workDir, "test.keytab")
	stageTestKeytab(t, keytabPath)
	jwtPath := filepath.Join(workDir, "test.jwt")
	_ = os.WriteFile(jwtPath, []byte("header.payload.sig"), 0o600)
	purgePath := filepath.Join(workDir, "secret.bin")
	_ = os.WriteFile(purgePath, []byte("secret-bytes"), 0o600)

	srv := &Server{
		tempestDir: binDir,
		tanukiBin:  tanukiBin,
		workDir:    workDir,
		selfExe:    tanukiBin,
		authToken:  hex.EncodeToString([]byte("0123456789abcdef0123456789abcdef")),
	}

	row, _, rc, _ := srv.execTanuki(Request{Subcommand: "keytab", TargetArg: keytabPath})
	if rc != 0 || row.Status != "PARSED" || row.SkewOrAux != "entries=2" {
		t.Fatalf("keytab mapping failed: rc=%d row=%+v", rc, row)
	}

	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "ladder"})
	if rc != 0 || row.Status != "PARSED" || row.SkewOrAux != "rungs=5" {
		t.Fatalf("ladder mapping failed: rc=%d row=%+v", rc, row)
	}

	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "kcm"})
	if rc != 0 || row.Status != "PARSED" || row.SkewOrAux != "entries=3" {
		t.Fatalf("kcm mapping failed: rc=%d row=%+v", rc, row)
	}

	t.Setenv("CIEL_MOCK_BLAST_TIER", "0")
	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "blast", KeytabPath: keytabPath})
	if rc != 0 || row.Status != "CRITICAL" || row.SkewOrAux != "tier=0 score=95" {
		t.Fatalf("blast tier 0 mapping failed: rc=%d row=%+v", rc, row)
	}

	t.Setenv("CIEL_MOCK_BLAST_TIER", "2")
	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "blast", KeytabPath: keytabPath})
	if rc != 0 || row.Status != "LOW" || row.SkewOrAux != "tier=2 score=20" {
		t.Fatalf("blast tier 2 mapping failed: rc=%d row=%+v", rc, row)
	}

	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "token", TargetArg: jwtPath})
	if rc != 0 || row.Status != "HIGH" || row.SkewOrAux != "risk=HIGH" {
		t.Fatalf("token mapping failed: rc=%d row=%+v", rc, row)
	}
	t.Setenv("CIEL_MOCK_TOKEN_ERR", "1")
	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "token", TargetArg: "bad.jwt"})
	if rc != 4 || row.Status != "ERROR" || row.SkewOrAux != "CORRUPT_DATA" {
		t.Fatalf("token error mapping failed: rc=%d row=%+v", rc, row)
	}
	t.Setenv("CIEL_MOCK_TOKEN_ERR", "")

	t.Setenv("CIEL_MOCK_NHI_MODE", "error")
	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "nhi", TargetArg: "exchange"})
	if rc != 1 || row.Status != "ERROR" || row.SkewOrAux != "MISSING_ARGUMENT" || row.PermOrTTL != "nhi" {
		t.Fatalf("nhi error mapping failed: rc=%d row=%+v", rc, row)
	}
	t.Setenv("CIEL_MOCK_NHI_MODE", "")

	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "nhi", TargetArg: "mesh"})
	if rc != 0 || row.Status != "NONE_DISCOVERED" || row.SkewOrAux != "identities=0" || row.PermOrTTL != "mesh" {
		t.Fatalf("nhi mesh mapping failed: rc=%d row=%+v", rc, row)
	}

	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "purge", TargetArg: purgePath})
	if rc != 0 || row.Status != "SUCCESS" || row.SkewOrAux != "shredded=1" || row.PermOrTTL != "nist-800-88" {
		t.Fatalf("purge mapping failed: rc=%d row=%+v", rc, row)
	}

	t.Setenv("CIEL_MOCK_NHI_FOUND", "")
	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "nhi"})
	if rc != 0 || row.Status != "NONE_DISCOVERED" || row.SkewOrAux != "tokens=0" {
		t.Fatalf("nhi scan empty mapping failed: rc=%d row=%+v", rc, row)
	}
	t.Setenv("CIEL_MOCK_NHI_FOUND", "1")
	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "nhi"})
	if rc != 0 || row.Status != "SUCCESS" || row.SkewOrAux != "tokens=1" {
		t.Fatalf("nhi scan found mapping failed: rc=%d row=%+v", rc, row)
	}

	t.Setenv("CIEL_MOCK_ARGS_LOG", argsLog)
	customConf := filepath.Join(workDir, "krb5.conf")
	row, _, rc, _ = srv.execTanuki(Request{Subcommand: "doctor", Krb5ConfPath: customConf})
	if rc != 0 || row.Status != "PASS" {
		t.Fatalf("doctor explicit failed: rc=%d row=%+v", rc, row)
	}
	loggedBytes, err := os.ReadFile(argsLog)
	if err != nil {
		t.Fatalf("read args log: %v", err)
	}
	logged := string(loggedBytes)
	if !strings.Contains(logged, "--krb5-conf "+customConf) || !strings.Contains(logged, "--opsec") {
		t.Fatalf("explicit doctor missing expected flags: %q", logged)
	}
}

func TestChronoaAndFullSyncRealHostBehavior(t *testing.T) {
	t.Setenv("XDG_RUNTIME_DIR", filepath.Join(t.TempDir(), "xdg_rt"))
	wd := defaultWorkDir()
	if !strings.HasSuffix(wd, filepath.Join("xdg_rt", "ciel_workspace")) {
		t.Fatalf("unexpected defaultWorkDir with XDG_RUNTIME_DIR: %s", wd)
	}

	binDir := t.TempDir()
	tanukiBin := buildMockBinary(t, binDir)
	workDir := t.TempDir()
	argsLog := filepath.Join(workDir, "doctor_sync.log")

	srv := &Server{
		tempestDir: binDir,
		tanukiBin:  tanukiBin,
		workDir:    workDir,
		selfExe:    tanukiBin,
	}
	if err := srv.initAuthToken(""); err != nil {
		t.Fatalf("initAuthToken: %v", err)
	}

	// Reject path starting with '-'
	badRow, _, badRc, _ := srv.execChronoaCheck(Request{
		Action:     "chronoa_check",
		CcachePath: "-evil-flag",
	})
	if badRc != 1 || badRow.Status != "ERROR" {
		t.Fatalf("expected dash-prefixed ccache_path to be rejected, got rc=%d row=%+v", badRc, badRow)
	}

	// Missing CCACHE is valid on a host without tickets; full_sync stays SYNCED when chronoa returns 3 (MISSING)
	t.Setenv("CIEL_MOCK_CHRONOA_MISSING", "1")
	t.Setenv("CIEL_MOCK_ARGS_LOG", argsLog)
	resp := srv.handleRequest(Request{AuthToken: srv.authToken, Action: "full_sync"})
	if !resp.OK || resp.Status != "SYNCED" || len(resp.Rows) != 5 {
		t.Fatalf("full_sync failed on missing ccache: %+v", resp)
	}
	if resp.Rows[1].Status != "MISSING" {
		t.Fatalf("expected OP-02 MISSING, got %+v", resp.Rows[1])
	}
	loggedBytes, err := os.ReadFile(argsLog)
	if err != nil {
		t.Fatalf("read doctor_sync.log: %v", err)
	}
	if strings.Contains(string(loggedBytes), "--opsec") {
		t.Fatalf("full_sync doctor must NOT run --opsec on every poll: %q", string(loggedBytes))
	}

	// Skew violation marks full_sync ATTENTION
	t.Setenv("CIEL_MOCK_CHRONOA_MISSING", "")
	t.Setenv("CIEL_MOCK_CHRONOA_SKEW", "1")
	respAttn := srv.handleRequest(Request{AuthToken: srv.authToken, Action: "full_sync"})
	if respAttn.Status != "ATTENTION" {
		t.Fatalf("expected ATTENTION on skew violation, got %q", respAttn.Status)
	}

	// Verify real UNIX socket probe works when explicitly supplied
	if runtime.GOOS != "windows" {
		sockPath := filepath.Join(workDir, "real_kcm.sock")
		ln, lErr := net.Listen("unix", sockPath)
		if lErr == nil {
			defer ln.Close()
			go func() {
				c, aErr := ln.Accept()
				if aErr == nil {
					_ = c.Close()
				}
			}()
			sRow, sData, sRc, _ := srv.execSoueiProbe(Request{KcmSocket: sockPath}, false)
			if sRc != 0 || sRow.Status != "OK" {
				t.Fatalf("souei probe with unix socket failed: rc=%d row=%+v", sRc, sRow)
			}
			kcmObj, _ := sData["kcm_socket"].(map[string]any)
			if !boolField(kcmObj, "connected") {
				t.Fatalf("expected kcm_socket.connected=true, got %+v", kcmObj)
			}
		}
	}
}
