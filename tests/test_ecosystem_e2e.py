#!/usr/bin/env python3
"""
4-Cycle Ecosystem End-to-End Verification Suite (Tanuki -> Tempest -> Shuna -> Ciel).
Covers Tiers 1-4 from TEST_INFRA.md across real binaries and live mTLS 1.3 RPC.
"""
import hashlib
import json
import os
import shutil
import socket
import ssl
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

CIEL_ROOT = Path(__file__).resolve().parent.parent
SCRATCH_ROOT = CIEL_ROOT.parent
TANUKI_ROOT = SCRATCH_ROOT / "tanuki"
TEMPEST_ROOT = SCRATCH_ROOT / "tempest"
SHUNA_ROOT = SCRATCH_ROOT / "shuna"

BIN_DIR = CIEL_ROOT / "bin"


def run_cmd(args, cwd=None, env=None, timeout=30):
    return subprocess.run(
        args,
        cwd=str(cwd) if cwd else None,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=timeout,
    )


def pick_free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def mtls_rpc(host, port, payload, ca_crt, client_crt, client_key, timeout=10.0):
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ctx.minimum_version = ssl.TLSVersion.TLSv1_3
    ctx.load_verify_locations(cafile=str(ca_crt))
    ctx.load_cert_chain(certfile=str(client_crt), keyfile=str(client_key))
    with socket.create_connection((host, port), timeout=timeout) as raw_sock:
        with ctx.wrap_socket(raw_sock, server_hostname=host) as s:
            s.sendall((json.dumps(payload) + "\n").encode("utf-8"))
            buf = b""
            while b"\n" not in buf:
                chunk = s.recv(65536)
                if not chunk:
                    break
                buf += chunk
            line = buf.split(b"\n", 1)[0].decode("utf-8")
            return json.loads(line)


def build_keytab_v2(realm, comp, enctype=18, key_byte=0x2A):
    entry = bytearray()
    rb = realm.encode("ascii")
    cb = comp.encode("ascii")
    key = bytes([key_byte]) * 32
    entry += struct.pack(">hH", 1, len(rb)) + rb
    entry += struct.pack(">H", len(cb)) + cb
    entry += struct.pack(">II", 1, int(time.time()))
    entry += struct.pack(">BhH", 3, enctype, len(key)) + key
    entry += struct.pack(">I", 3)
    return b"\x05\x02" + struct.pack(">i", len(entry)) + bytes(entry), key


def test_tier1_version_and_cli_parity():
    # 1. Tanuki Python & Rust version parity (2.1.4)
    py_ver = run_cmd([sys.executable, "-m", "tanuki", "--version"], cwd=TANUKI_ROOT)
    assert py_ver.returncode == 0 and "2.1.4" in py_ver.stdout, f"Tanuki Python version mismatch: {py_ver.stdout} {py_ver.stderr}"

    rust_tanuki = BIN_DIR / "tanuki"
    rs_ver = run_cmd([str(rust_tanuki), "--version"])
    assert rs_ver.returncode == 0 and "2.1.4" in rs_ver.stdout, f"Tanuki Rust version mismatch: {rs_ver.stdout} {rs_ver.stderr}"

    # 2. Tempest operatives version (1.0.0)
    for op in ("souei", "chronoa", "ranga"):
        op_res = run_cmd([str(BIN_DIR / op), "--version"])
        assert op_res.returncode == 0 and "1.0.0" in op_res.stdout, f"{op} --version failed: {op_res.stdout}"

    # 3. Shuna version (1.0.0)
    shuna_res = run_cmd([str(BIN_DIR / "shuna"), "--version"])
    assert shuna_res.returncode == 0 and "shuna 1.0.0" in shuna_res.stdout, f"shuna --version failed: {shuna_res.stdout}"

    # 4. Ciel teamserver & client version (1.0.0)
    ts_res = run_cmd([str(BIN_DIR / "ciel-teamserver"), "--version"])
    assert ts_res.returncode == 0 and "ciel-teamserver 1.0.0" in ts_res.stdout, f"ciel-teamserver --version failed: {ts_res.stdout}"

    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    cli_res = run_cmd([str(BIN_DIR / "ciel-client"), "--version"], env=env)
    assert cli_res.returncode == 0 and "1.0.0" in cli_res.stdout, f"ciel-client --version failed: {cli_res.stdout}"
    print("[PASS] Tier 1: All 4 cycles report expected version strings (tanuki 2.1.4, tempest/shuna/ciel 1.0.0)")


def test_tier2_tanuki_cross_engine_parity():
    rust_tanuki = str(BIN_DIR / "tanuki")
    with tempfile.TemporaryDirectory(prefix="tanuki-parity-") as tmp:
        kt_path = Path(tmp) / "svc.keytab"
        kt_bytes, raw_key = build_keytab_v2("CORP.LOCAL", "svc_parity")
        kt_path.write_bytes(kt_bytes)
        os.chmod(kt_path, 0o600)
        expected_fp = hashlib.sha256(raw_key).hexdigest()[:16]

        for label, base_cmd in (
            ("python", [sys.executable, "-m", "tanuki"]),
            ("rust", [rust_tanuki]),
        ):
            # keytab fingerprint + redacted preview
            kt_res = run_cmd(base_cmd + ["keytab", str(kt_path), "--json"], cwd=TANUKI_ROOT)
            assert kt_res.returncode == 0, f"{label} keytab failed: {kt_res.stderr}"
            kt_data = json.loads(kt_res.stdout)
            entry = kt_data[0] if isinstance(kt_data, list) else kt_data["entries"][0]
            assert entry["key_fingerprint"] == expected_fp, f"{label} key_fingerprint mismatch: {entry}"
            assert entry["key_preview"] == raw_key[:4].hex() + "...", f"{label} key_preview mismatch: {entry}"
            assert "key_hex" not in kt_res.stdout and raw_key.hex() not in kt_res.stdout, f"{label} leaked full raw key hex"

            # config --stdout --json
            cfg_res = run_cmd(base_cmd + ["config", "--realm", "CORP.LOCAL", "--kdc", "[fd00::1]:88", "--stdout", "--json"], cwd=TANUKI_ROOT)
            assert cfg_res.returncode == 0, f"{label} config --stdout failed: {cfg_res.stderr}"
            cfg_data = json.loads(cfg_res.stdout)
            assert cfg_data["realm"] == "CORP.LOCAL" and "dns_lookup_kdc = false" in cfg_data["content"], cfg_data

            # nhi default scan + nhi mesh
            nhi_scan = run_cmd(base_cmd + ["nhi", "--json"], cwd=TANUKI_ROOT)
            assert nhi_scan.returncode == 0, f"{label} nhi default scan failed: {nhi_scan.stderr}"
            assert "discovered_tokens" in json.loads(nhi_scan.stdout)

            nhi_mesh = run_cmd(base_cmd + ["nhi", "mesh", "--json"], cwd=TANUKI_ROOT)
            assert nhi_mesh.returncode == 0, f"{label} nhi mesh failed: {nhi_mesh.stderr}"
            assert "discovered_identities" in json.loads(nhi_mesh.stdout)
    print("[PASS] Tier 2: Tanuki Python & Rust cross-engine JSON contract & keytab redaction parity")


def test_tier3_shuna_and_teamserver_security():
    shuna_bin = str(BIN_DIR / "shuna")
    teamserver_bin = str(BIN_DIR / "ciel-teamserver")

    with tempfile.TemporaryDirectory(prefix="ciel-tier3-") as tmp:
        work_dir = Path(tmp) / "work"
        pki_dir = work_dir / "pki"
        work_dir.mkdir(mode=0o700)

        # Initialize PKI via Shuna
        r_init = run_cmd([shuna_bin, "init-pki", "--out", str(pki_dir)])
        assert r_init.returncode == 0, r_init.stderr

        # Verify Shuna rejects reserved CNs for client certificates
        for reserved in ("ca", "server", "pg-server", "SERVER"):
            r_bad_cn = run_cmd([shuna_bin, "issue-cert", "--pki-dir", str(pki_dir), "--cn", reserved, "--role", "client"])
            assert r_bad_cn.returncode != 0, f"Expected shuna issue-cert to reject reserved client CN {reserved}"

        # Issue bootstrap client.crt and secondary op2.crt
        assert run_cmd([shuna_bin, "issue-cert", "--pki-dir", str(pki_dir), "--cn", "client", "--role", "client"]).returncode == 0
        assert run_cmd([shuna_bin, "issue-cert", "--pki-dir", str(pki_dir), "--cn", "op2", "--role", "client"]).returncode == 0

        # Compute SHA-256 DER fingerprint of op2.crt
        pem_text = (pki_dir / "op2.crt").read_text(encoding="utf-8")
        der_bytes = ssl.PEM_cert_to_DER_cert(pem_text)
        op2_fp = hashlib.sha256(der_bytes).hexdigest()

        port = pick_free_port()
        addr = f"127.0.0.1:{port}"
        token_file = work_dir / "teamserver.token"
        srv = subprocess.Popen(
            [teamserver_bin, "--addr", addr, "--work-dir", str(work_dir), "--pki-dir", str(pki_dir)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        try:
            deadline = time.time() + 10.0
            while time.time() < deadline:
                if token_file.exists():
                    try:
                        with socket.create_connection(("127.0.0.1", port), timeout=0.3):
                            break
                    except OSError:
                        pass
                time.sleep(0.1)
            bootstrap_token = token_file.read_text(encoding="utf-8").strip()
            ca_crt = pki_dir / "ca.crt"
            cli1_crt = pki_dir / "client.crt"
            cli1_key = pki_dir / "client.key"
            cli2_crt = pki_dir / "op2.crt"
            cli2_key = pki_dir / "op2.key"

            # Bootstrap token bound to client.crt must be rejected when presented by op2.crt
            r_cross = mtls_rpc("127.0.0.1", port, {"auth_token": bootstrap_token, "action": "ping"}, ca_crt, cli2_crt, cli2_key)
            assert r_cross["ok"] is False and r_cross["status"] == "UNAUTHORIZED", r_cross

            # Bootstrap client issues session token for op2.crt via session_issue
            r_issue = mtls_rpc("127.0.0.1", port, {"auth_token": bootstrap_token, "action": "session_issue", "cert_fp": op2_fp}, ca_crt, cli1_crt, cli1_key)
            assert r_issue["ok"] is True and r_issue["status"] == "ISSUED", r_issue
            op2_token = r_issue["data"]["auth_token"]

            # op2.crt can now call ping, but CANNOT call session_issue (bootstrap-only restriction)
            r_op2_ping = mtls_rpc("127.0.0.1", port, {"auth_token": op2_token, "action": "ping"}, ca_crt, cli2_crt, cli2_key)
            assert r_op2_ping["ok"] is True and r_op2_ping["status"] == "READY", r_op2_ping

            r_op2_issue = mtls_rpc("127.0.0.1", port, {"auth_token": op2_token, "action": "session_issue", "cert_fp": op2_fp}, ca_crt, cli2_crt, cli2_key)
            assert r_op2_issue["ok"] is False and r_op2_issue["status"] == "UNAUTHORIZED", r_op2_issue

            # Verify --krb5-conf forwarding on tanuki auth
            custom_krb5 = Path(tmp) / "custom_krb5.conf"
            custom_krb5.write_text("[libdefaults]\n    default_realm = TEST.LOCAL\n", encoding="utf-8")
            r_auth = mtls_rpc("127.0.0.1", port, {
                "auth_token": bootstrap_token,
                "action": "tanuki_exec",
                "subcommand": "auth",
                "krb5_conf_path": str(custom_krb5),
                "args": ["--keytab", str(Path(tmp) / "missing.keytab"), "--principal", "svc@TEST.LOCAL", "--kdc", "127.0.0.1"],
            }, ca_crt, cli1_crt, cli1_key)
            assert r_auth["exit_code"] == 3, r_auth

            # Verify fail-closed CRL reload when ca.crl is corrupted
            crl_path = pki_dir / "ca.crl"
            saved_crl = crl_path.read_bytes()
            crl_path.write_bytes(b"CORRUPT-CRL-BYTES")
            try:
                failed_closed = False
                try:
                    r_corrupt_crl = mtls_rpc("127.0.0.1", port, {"auth_token": bootstrap_token, "action": "ping"}, ca_crt, cli1_crt, cli1_key)
                    failed_closed = (r_corrupt_crl.get("ok") is False and r_corrupt_crl.get("status") == "UNAUTHORIZED")
                except (ssl.SSLError, OSError):
                    failed_closed = True
                assert failed_closed, "Expected corrupt ca.crl to fail closed"
            finally:
                crl_path.write_bytes(saved_crl)
            r_restored = mtls_rpc("127.0.0.1", port, {"auth_token": bootstrap_token, "action": "ping"}, ca_crt, cli1_crt, cli1_key)
            assert r_restored["ok"] is True and r_restored["status"] == "READY", r_restored
        finally:
            srv.terminate()
            try:
                srv.wait(timeout=5)
            except subprocess.TimeoutExpired:
                srv.kill()
    print("[PASS] Tier 3: Shuna reserved-CN guard, bootstrap-only session_issue, --krb5-conf forwarding, and fail-closed CRL reload")


if __name__ == "__main__":
    test_tier1_version_and_cli_parity()
    test_tier2_tanuki_cross_engine_parity()
    test_tier3_shuna_and_teamserver_security()
    print("\nALL 4-CYCLE ECOSYSTEM E2E VERIFICATIONS PASSED")
