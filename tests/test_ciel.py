#!/usr/bin/env python3
import json
import os
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "bin"
ASSETS = ROOT / "assets"
TEAMSERVER = str(BIN / "ciel-teamserver")
CLIENT = str(BIN / "ciel-client")
CIEL = str(BIN / "ciel")
TEMPEST_BIN = Path(os.environ.get("TEMPEST_BIN_DIR", str(BIN)))

TEST_PAC_HEX = (
    "02000000000000000a0000001a000000280000000000000001000000f8000000"
    "48000000000000000000000000a0d90110006300690065006c005f006f007000"
    "7300000000000000000000000000000000000000000000000000000000000000"
    "0000000000000000000000000000000000000000000000000000000010001200"
    "0400020000000000000000000000000000000000000000000000000000000000"
    "00000000000000000000000000000000f4010000010200000200000008000200"
    "0002000000000000000000000000000000000000000000000000000010000200"
    "0000000000000000000200001000020000000000000000000000000008000000"
    "00000000080000006300690065006c005f006f00700073000200000001020000"
    "07000000000200000700000004010000000000051500000057040000ae080000"
    "050d0000"
)
TEST_SHADOW_HEX = (
    "0002000048000345434b3120000000"
    + ("11" * 32)
    + ("22" * 32)
    + "2000011af8f9468674edde04d2975d3cc39345c5c7bf7ee0338e508955b7462d318ae2"
    + "010004010100050010000611223344556677889900aabbccddeeff020007010108000900206196bdc5db01"
)


def pack_principal(components, realm, name_type=1):
    r_bytes = realm.encode("ascii")
    out = bytearray()
    out += struct.pack(">III", name_type, len(components), len(r_bytes))
    out += r_bytes
    for comp in components:
        cb = comp.encode("ascii")
        out += struct.pack(">I", len(cb)) + cb
    return bytes(out)


def build_ccache_v4(principal_comps, realm, server_comps, enctype, authtime, starttime, endtime, renew_till):
    data = bytearray()
    data += struct.pack(">HH", 0x0504, 0)
    client_p = pack_principal(principal_comps, realm, name_type=1)
    server_p = pack_principal(server_comps, realm, name_type=2)
    data += client_p
    data += client_p
    data += server_p
    key = b"\xaa" * 16
    data += struct.pack(">HI", enctype, len(key)) + key
    data += struct.pack(">IIII", authtime, starttime, endtime, renew_till)
    data += struct.pack(">BI", 0, 0x40E10000)
    data += struct.pack(">II", 0, 0)
    tkt = b"\x55" * 32
    data += struct.pack(">I", len(tkt)) + tkt + struct.pack(">I", 0)
    return bytes(data)


def build_keytab_v2(realm, comp, enctype=18):
    entry = bytearray()
    rb = realm.encode("ascii")
    cb = comp.encode("ascii")
    key = b"\x2a" * 32
    entry += struct.pack(">hH", 1, len(rb)) + rb
    entry += struct.pack(">H", len(cb)) + cb
    entry += struct.pack(">II", 1, int(time.time()))
    entry += struct.pack(">BhH", 3, enctype, len(key)) + key
    entry += struct.pack(">I", 3)
    return b"\x05\x02" + struct.pack(">i", len(entry)) + bytes(entry)


def load_ui_font(size=14):
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    ]
    for c in candidates:
        if Path(c).exists():
            return ImageFont.truetype(c, size=size)
    return ImageFont.load_default()


def frame_desktop_window(shot_img, title_text, icon_path):
    bar_h = 30
    w, h = shot_img.size
    framed = Image.new("RGBA", (w + 2, h + bar_h + 2), (24, 25, 34, 255))
    draw = ImageDraw.Draw(framed)
    draw.rectangle([1, 1, w, bar_h], fill=(32, 34, 44, 255))
    draw.line([1, bar_h, w, bar_h], fill=(68, 71, 90, 255), width=1)

    if icon_path.exists():
        ico = Image.open(icon_path).convert("RGBA").resize((20, 20), Image.Resampling.LANCZOS)
        framed.paste(ico, (10, 6), ico)

    font = load_ui_font(13)
    bbox = draw.textbbox((0, 0), title_text, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    draw.text(((w - tw) // 2, (bar_h - th) // 2 - 1), title_text, fill=(248, 248, 242, 255), font=font)

    dots = [(w - 58, (255, 95, 86, 255)), (w - 38, (255, 189, 46, 255)), (w - 18, (39, 201, 63, 255))]
    for cx, col in dots:
        draw.ellipse([cx - 5, 10, cx + 5, 20], fill=col)

    framed.paste(shot_img.convert("RGBA"), (1, bar_h + 1))
    draw.rectangle([0, 0, w + 1, h + bar_h + 1], outline=(72, 75, 96, 255), width=1)
    return framed


def compose_showcase_images(table_shot, graph_shot, alt_shot, console_shot, out_dir):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    icon_path = ASSETS / "ciel_icon.png"

    im_graph = Image.open(graph_shot)
    framed_graph = frame_desktop_window(im_graph, "Ciel | Session Graph", icon_path)
    out_graph = out_dir / "FullSessionGraph.png"
    framed_graph.convert("RGB").save(out_graph, format="PNG")

    im_table = frame_desktop_window(Image.open(table_shot), "Ciel | Session Control", icon_path)
    out_sessions = out_dir / "SessionsOverview.png"
    im_table.convert("RGB").save(out_sessions, format="PNG")

    im_alt = frame_desktop_window(Image.open(alt_shot), "Ciel | Telemetry & Listeners", icon_path)
    out_listeners = out_dir / "ListenersAndTelemetry.png"
    im_alt.convert("RGB").save(out_listeners, format="PNG")

    im_console = frame_desktop_window(Image.open(console_shot), "Ciel | Operative Console", icon_path)
    out_console = out_dir / "OperativeConsole.png"
    im_console.convert("RGB").save(out_console, format="PNG")

    win_w, win_h = 1220, int(1220 * im_table.height / im_table.width)
    back_res = im_alt.resize((win_w, win_h), Image.Resampling.LANCZOS)
    front_res = im_table.resize((win_w, win_h), Image.Resampling.LANCZOS)

    canvas_w = win_w + 260
    canvas_h = win_h + 90
    canvas = Image.new("RGBA", (canvas_w, canvas_h), (24, 26, 36, 255))
    canvas.paste(back_res, (16, 16), back_res)

    fx, fy = 240, 68
    shadow = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
    sdraw = ImageDraw.Draw(shadow)
    sdraw.rectangle([fx - 4, fy - 4, fx + win_w + 8, fy + win_h + 12], fill=(0, 0, 0, 175))
    shadow = shadow.filter(ImageFilter.GaussianBlur(14))
    canvas = Image.alpha_composite(canvas, shadow)
    canvas.paste(front_res, (fx, fy), front_res)

    out_multi = out_dir / "MultiSessionControl.png"
    canvas.convert("RGB").save(out_multi, format="PNG")
    return out_graph, out_multi, out_sessions, out_listeners, out_console


def pick_free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def rpc_call(host, port, payload, timeout=10.0):
    with socket.create_connection((host, port), timeout=timeout) as s:
        s.sendall((json.dumps(payload) + "\n").encode("utf-8"))
        buf = b""
        while b"\n" not in buf:
            chunk = s.recv(65536)
            if not chunk:
                break
            buf += chunk
        line = buf.split(b"\n", 1)[0].decode("utf-8")
        return json.loads(line)


def wait_for_server(host, port, proc, deadline=8.0):
    end = time.time() + deadline
    while time.time() < end:
        if proc.poll() is not None:
            out, err = proc.communicate()
            raise RuntimeError(f"ciel-teamserver exited early ({proc.returncode}): {out} {err}")
        try:
            with socket.create_connection((host, port), timeout=0.3):
                return
        except OSError:
            time.sleep(0.1)
    raise RuntimeError("Timed out waiting for ciel-teamserver TCP listener")


def main():
    for name in ("souei", "chronoa", "ranga", "libranga.so", "tanuki", "ciel"):
        assert (TEMPEST_BIN / name).exists() or (BIN / name).exists(), f"Missing bundled operative: {name}"
    assert Path(TEAMSERVER).exists(), f"Missing {TEAMSERVER}"
    assert Path(CLIENT).exists(), f"Missing {CLIENT}"
    assert Path(CIEL).exists(), f"Missing {CIEL}"

    tanuki_help = subprocess.run([str(BIN / "tanuki"), "--help"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=10)
    assert tanuki_help.returncode == 0, f"bundled tanuki --help failed: {tanuki_help.stderr}"
    print("[PASS] bundled tanuki CLI (--help)")

    port = pick_free_port()
    addr = f"127.0.0.1:{port}"
    with tempfile.TemporaryDirectory() as work_dir, tempfile.TemporaryDirectory() as fix_dir:
        shot_path = Path(fix_dir) / "ciel_preview.png"
        graph_shot_path = Path(fix_dir) / "ciel_graph_preview.png"
        alt_shot_path = Path(fix_dir) / "ciel_alt_preview.png"
        con_shot_path = Path(fix_dir) / "ciel_console_preview.png"
        token_file = Path(work_dir) / "teamserver.token"
        srv = subprocess.Popen(
            [TEAMSERVER, "--addr", addr, "--work-dir", work_dir],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        try:
            wait_for_server("127.0.0.1", port, srv)
            assert token_file.exists(), "teamserver.token was not created"
            assert (token_file.stat().st_mode & 0o777) == 0o600, "teamserver.token must be 0600"
            # Verify NO fake artifacts were fabricated in work_dir
            wd_files = {p.name for p in Path(work_dir).iterdir()}
            assert wd_files == {"teamserver.token"}, f"Unexpected fabricated artifacts in work_dir: {wd_files}"
            auth_token = token_file.read_text(encoding="utf-8").strip()

            # Auth rejection tests
            r_unauth = rpc_call("127.0.0.1", port, {"action": "ping"})
            assert r_unauth["ok"] is False and r_unauth["status"] == "UNAUTHORIZED", r_unauth
            r_bad_tok = rpc_call("127.0.0.1", port, {"auth_token": "wrong", "action": "full_sync"})
            assert r_bad_tok["ok"] is False and r_bad_tok["status"] == "UNAUTHORIZED", r_bad_tok
            print("[PASS] teamserver auth_token enforcement (unauthorized rejected)")

            r_ping = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "ping"})
            assert r_ping["ok"] is True and r_ping["status"] == "READY", r_ping
            assert r_ping["data"]["binaries_present"]["tanuki"] is True, r_ping
            assert r_ping["data"]["auth_enforced"] is True, r_ping
            print("[PASS] teamserver ping (authenticated, bundled Tempest + Tanuki ready)")

            r_sync = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "full_sync"})
            assert r_sync["ok"] is True and len(r_sync["rows"]) == 5, r_sync
            assert len(r_sync["logs"]) == 5
            assert any(row["operative"] == "Tanuki" for row in r_sync["rows"]), r_sync
            print("[PASS] teamserver full_sync (real host telemetry without fake artifacts)")

            # Real temporary UNIX socket test for souei_probe
            test_sock_path = os.path.join(fix_dir, "test_kcm.sock")
            kcm_srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            kcm_srv.bind(test_sock_path)
            os.chmod(test_sock_path, 0o600)
            kcm_srv.listen(4)
            stop_kcm = threading.Event()

            def kcm_accept():
                kcm_srv.settimeout(0.2)
                while not stop_kcm.is_set():
                    try:
                        conn, _ = kcm_srv.accept()
                        time.sleep(0.03)
                        conn.close()
                    except socket.timeout:
                        continue
                    except OSError:
                        break

            kcm_t = threading.Thread(target=kcm_accept, daemon=True)
            kcm_t.start()
            try:
                r_souei = rpc_call("127.0.0.1", port, {
                    "auth_token": auth_token,
                    "action": "souei_probe",
                    "kcm_socket": test_sock_path,
                })
                assert r_souei["exit_code"] in (0, 2)
                assert r_souei["data"]["session_keyring_id"] > 0
                assert r_souei["data"]["kcm_socket"]["connected"] is True, r_souei
                assert r_souei["data"]["kcm_socket"]["peer_pid"] > 0, r_souei
                print("[PASS] teamserver souei_probe (keyring + AF_UNIX SO_PEERCRED on test KCM socket)")
            finally:
                stop_kcm.set()
                kcm_srv.close()
                kcm_t.join(timeout=2)

            # Live inotify watch triggered by test process (not teamserver)
            watch_file = os.path.join(fix_dir, "watch_target.ccache")
            Path(watch_file).write_bytes(b"watch-probe")
            stop_touch = threading.Event()

            def touch_watch():
                while not stop_touch.is_set():
                    time.sleep(0.1)
                    try:
                        Path(watch_file).read_bytes()
                    except OSError:
                        break

            touch_t = threading.Thread(target=touch_watch, daemon=True)
            touch_t.start()
            try:
                r_watch = rpc_call("127.0.0.1", port, {
                    "auth_token": auth_token,
                    "action": "souei_watch",
                    "watch_path": watch_file,
                    "watch_timeout_ms": 1200,
                })
            finally:
                stop_touch.set()
                touch_t.join(timeout=2)
            assert r_watch["exit_code"] == 0, r_watch
            assert r_watch["data"]["inotify"]["watched"] is True, r_watch
            assert len(r_watch["data"]["inotify"]["events"]) >= 1, r_watch
            print("[PASS] teamserver souei_watch (real external inotify access monitor)")

            # Stage temporary CCACHE v4 test fixtures in fix_dir
            now = int(time.time())
            cc_ok = os.path.join(fix_dir, "cc_ok")
            cc_rc4 = os.path.join(fix_dir, "cc_rc4")
            cc_exp = os.path.join(fix_dir, "cc_exp")
            Path(cc_ok).write_bytes(build_ccache_v4(["svc"], "CORP.LOCAL", ["krbtgt", "CORP.LOCAL"], 18, now - 60, now - 60, now + 3600, now + 86400))
            Path(cc_rc4).write_bytes(build_ccache_v4(["svc"], "CORP.LOCAL", ["krbtgt", "CORP.LOCAL"], 23, now - 60, now - 60, now + 3600, now + 86400))
            Path(cc_exp).write_bytes(build_ccache_v4(["svc"], "CORP.LOCAL", ["krbtgt", "CORP.LOCAL"], 18, now - 7200, now - 7200, now - 60, now + 3600))

            r_ok = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "chronoa_check", "ccache_path": cc_ok})
            assert r_ok["exit_code"] == 0 and r_ok["status"] == "HEALTHY", r_ok
            assert r_ok["data"]["primary_ticket"]["enctype"] == 18

            r_skew = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "chronoa_check", "ccache_path": cc_ok, "kdc_time_epoch": now + 350})
            assert r_skew["exit_code"] == 2 and r_skew["status"] == "CLOCK_SKEW_VIOLATION", r_skew

            r_rc4 = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "chronoa_check", "ccache_path": cc_rc4})
            assert r_rc4["exit_code"] == 2 and r_rc4["status"] == "WEAK_ENCTYPE", r_rc4

            r_exp = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "chronoa_check", "ccache_path": cc_exp})
            assert r_exp["exit_code"] == 2 and r_exp["status"] == "EXPIRED", r_exp

            r_miss = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "chronoa_check", "ccache_path": os.path.join(fix_dir, "missing_cc")})
            assert r_miss["exit_code"] == 3 and r_miss["status"] == "MISSING", r_miss
            print("[PASS] teamserver chronoa_check (healthy, kdc_time skew 350s, rc4, expired, missing)")

            r_iso = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "ranga_isolate"})
            assert r_iso["exit_code"] == 0 and r_iso["data"]["possessor_only"] is True
            assert r_iso["data"]["perm_hex"] == "3f000000", r_iso
            r_wrap = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "ranga_isolate", "isolate_mode": "wrapper"})
            assert r_wrap["exit_code"] == 0 and r_wrap["data"]["possessor_only"] is True
            assert r_wrap["data"]["perm_hex"] == "3f000000", r_wrap
            print("[PASS] teamserver ranga_isolate (LD_PRELOAD + --isolate wrapper 0x3f000000)")

            r_empty_prg = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "ranga_purge"})
            assert r_empty_prg["exit_code"] == 1 and r_empty_prg["status"] == "USAGE_ERROR", r_empty_prg
            purge_file = os.path.join(fix_dir, "purge_secret.bin")
            Path(purge_file).write_bytes(os.urandom(4096))
            r_prg = rpc_call("127.0.0.1", port, {"auth_token": auth_token, "action": "ranga_purge", "purge_path": purge_file})
            assert r_prg["exit_code"] == 0 and r_prg["data"]["removed"] is True
            assert r_prg["data"]["passes"] == 3 and r_prg["data"]["wiped_bytes"] == 4096, r_prg
            print("[PASS] teamserver ranga_purge (explicit target guard + 3-pass NIST wipe)")

            # Stage temporary Tanuki fixtures in fix_dir and verify all 15 modules
            kt_file = os.path.join(fix_dir, "svc.keytab")
            Path(kt_file).write_bytes(build_keytab_v2("CORP.LOCAL", "svc"))
            os.chmod(kt_file, 0o600)
            krb5_file = os.path.join(fix_dir, "krb5.conf")
            Path(krb5_file).write_text("[libdefaults]\n    default_realm = CORP.LOCAL\n", encoding="utf-8")
            adcs_file = os.path.join(fix_dir, "adcs.json")
            Path(adcs_file).write_text(json.dumps({"templates": [{"name": "SubCA-ESC1", "msPKI-Certificate-Name-Flag": 1, "msPKI-Enrollment-Flag": 0, "pKIExtendedKeyUsage": ["1.3.6.1.5.5.7.3.2"], "authorized_signatures": 0}]}), encoding="utf-8")
            jwt_file = os.path.join(fix_dir, "workload.jwt")
            Path(jwt_file).write_text("eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJzcGlmZmU6Ly9jb3JwLmxvY2FsL3dvcmtsb2FkL3N2YyIsImlzcyI6Imh0dHBzOi8vc3RzLmNvcnAubG9jYWwiLCJhdWQiOiJjaWVsLXRlYW1zZXJ2ZXIiLCJleHAiOjE5OTk5OTk5OTl9.c2ln", encoding="utf-8")
            pac_file = os.path.join(fix_dir, "test.pac")
            Path(pac_file).write_bytes(bytes.fromhex(TEST_PAC_HEX))
            tanuki_purge_file = os.path.join(fix_dir, "tanuki_purge.bin")
            Path(tanuki_purge_file).write_bytes(b"SECRET_TO_SHRED" * 32)

            tanuki_cases = [
                {"subcommand": "doctor", "keytab_path": kt_file, "ccache_path": cc_ok, "krb5_conf_path": krb5_file},
                {"subcommand": "fix", "keytab_path": kt_file, "ccache_path": cc_ok, "krb5_conf_path": krb5_file},
                {"subcommand": "keytab", "target_arg": kt_file},
                {"subcommand": "config", "target_arg": "--realm CORP.LOCAL --kdc 127.0.0.1 --stdout"},
                {"subcommand": "triage", "target_arg": "SKEW"},
                {"subcommand": "ladder"},
                {"subcommand": "nhi", "target_arg": "scan"},
                {"subcommand": "token", "target_arg": jwt_file},
                {"subcommand": "blast", "keytab_path": kt_file, "ccache_path": cc_ok},
                {"subcommand": "adcs", "target_arg": adcs_file},
                {"subcommand": "pac", "target_arg": pac_file},
                {"subcommand": "shadow", "target_arg": TEST_SHADOW_HEX},
                {"subcommand": "kcm", "target_arg": cc_ok},
                {"subcommand": "skill"},
                {"subcommand": "purge", "target_arg": tanuki_purge_file},
            ]
            for tc in tanuki_cases:
                payload = {"auth_token": auth_token, "action": "tanuki_exec", **tc}
                r_tan = rpc_call("127.0.0.1", port, payload)
                assert r_tan["ok"] is True, r_tan
                assert r_tan["exit_code"] == 0, f"tanuki_exec {tc['subcommand']} failed: {r_tan}"
                assert r_tan["rows"] and r_tan["rows"][0]["operative"] == "Tanuki", r_tan
            print("[PASS] teamserver tanuki_exec (15 Tanuki modules with explicit test fixtures)")

            # Verify wire/negative Tanuki modules (auth, tgs, ldap) + configured path forwarding + security rejections
            r_auth_miss = rpc_call("127.0.0.1", port, {
                "auth_token": auth_token, "action": "tanuki_exec", "subcommand": "auth",
                "args": ["--keytab", os.path.join(fix_dir, "nope.keytab"), "--principal", "svc@CORP.LOCAL", "--kdc", "127.0.0.1"],
            })
            assert r_auth_miss["exit_code"] == 3 and r_auth_miss["rows"][0]["skew_or_aux"] == "MISSING_KEYTAB", r_auth_miss

            r_tgs_fwd = rpc_call("127.0.0.1", port, {
                "auth_token": auth_token, "action": "tanuki_exec", "subcommand": "tgs",
                "ccache_path": os.path.join(fix_dir, "nope_tgs.ccache"),
                "args": ["--spn", "HTTP/app@CORP.LOCAL", "--kdc", "127.0.0.1"],
            })
            assert r_tgs_fwd["exit_code"] == 3 and r_tgs_fwd["rows"][0]["skew_or_aux"] == "MISSING_CCACHE", r_tgs_fwd

            r_ldap_ref = rpc_call("127.0.0.1", port, {
                "auth_token": auth_token, "action": "tanuki_exec", "subcommand": "ldap",
                "args": ["--host", "127.0.0.1", "--realm", "CORP.LOCAL", "--query", "all"],
            })
            assert r_ldap_ref["exit_code"] == 3 and r_ldap_ref["status"] == "CONNECTION_FAILED", r_ldap_ref

            r_bad_triage = rpc_call("127.0.0.1", port, {
                "auth_token": auth_token, "action": "tanuki_exec", "subcommand": "triage", "target_arg": "--interactive",
            })
            assert r_bad_triage["exit_code"] == 1 and r_bad_triage["status"] == "USAGE_ERROR", r_bad_triage

            r_bad_purge = rpc_call("127.0.0.1", port, {
                "auth_token": auth_token, "action": "ranga_purge", "purge_path": "/usr/bin/ls",
            })
            assert r_bad_purge["exit_code"] == 1 and r_bad_purge["status"] == "USAGE_ERROR", r_bad_purge

            r_bad_tanuki_purge = rpc_call("127.0.0.1", port, {
                "auth_token": auth_token, "action": "tanuki_exec", "subcommand": "purge",
                "args": ["/etc/passwd", "--json"],
            })
            assert r_bad_tanuki_purge["exit_code"] == 1 and r_bad_tanuki_purge["status"] == "USAGE_ERROR", r_bad_tanuki_purge

            r_cfg_args = rpc_call("127.0.0.1", port, {
                "auth_token": auth_token, "action": "tanuki_exec", "subcommand": "config",
                "args": ["--realm", "CORP.LOCAL", "--kdc", "127.0.0.1"],
            })
            assert r_cfg_args["exit_code"] == 0 and r_cfg_args["data"]["realm"] == "CORP.LOCAL" and "content" in r_cfg_args["data"], r_cfg_args
            print("[PASS] teamserver auth/tgs/ldap + target_arg/purge/config security guards")

            # Verify client fails cleanly on wrong auth_token (via --token-file, not CLI --token)
            bad_tok_file = os.path.join(fix_dir, "bad.token")
            Path(bad_tok_file).write_text("0000deadbeef\n", encoding="utf-8")
            os.chmod(bad_tok_file, 0o600)
            bad_env = os.environ.copy()
            bad_env["QT_QPA_PLATFORM"] = "offscreen"
            bad_c = subprocess.run(
                [CLIENT, "--addr", addr, "--token-file", bad_tok_file, "--self-test"],
                env=bad_env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=15,
            )
            assert bad_c.returncode != 0, "ciel-client should fail self-test with invalid auth token"
            print("[PASS] ciel-client rejects invalid teamserver auth_token cleanly")

            env = os.environ.copy()
            env["QT_QPA_PLATFORM"] = "offscreen"
            c_res = subprocess.run(
                [
                    CLIENT, "--addr", addr,
                    "--token-file", str(token_file),
                    "--self-test",
                    "--screenshot", str(shot_path),
                    "--graph-screenshot", str(graph_shot_path),
                    "--alt-screenshot", str(alt_shot_path),
                    "--console-screenshot", str(con_shot_path),
                ],
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=30,
            )
            assert c_res.returncode == 0, f"ciel-client --self-test failed ({c_res.returncode}): {c_res.stdout}\n{c_res.stderr}"
            assert "QIODevice::read" not in c_res.stderr, f"Unexpected QTcpSocket stderr warning: {c_res.stderr}"
            for src, label in (
                (shot_path, "table screenshot"),
                (graph_shot_path, "graph screenshot"),
                (alt_shot_path, "alt telemetry screenshot"),
                (con_shot_path, "console screenshot"),
            ):
                assert src.exists() and src.stat().st_size > 10000, f"Missing or tiny {label}"
                raw = src.read_bytes()
                assert raw[:8] == b"\x89PNG\r\n\x1a\n", f"{label} is not a valid PNG"
                print(f"[PASS] native Qt6 ciel-client --self-test {label} ({len(raw)} bytes)")

            showcase_dir = ASSETS if os.environ.get("CIEL_UPDATE_ASSETS") == "1" else Path(fix_dir) / "showcase"
            out_graph, out_multi, out_sessions, out_listeners, out_console = compose_showcase_images(
                shot_path, graph_shot_path, alt_shot_path, con_shot_path, showcase_dir
            )
            for out_p in (out_graph, out_multi, out_sessions, out_listeners, out_console):
                assert out_p.exists() and out_p.stat().st_size > 10000, f"Missing or tiny showcase {out_p}"
                assert out_p.read_bytes()[:8] == b"\x89PNG\r\n\x1a\n", f"{out_p} is not a valid PNG"
            print(f"[PASS] showcase FullSessionGraph.png ({out_graph.stat().st_size} bytes)")
            print(f"[PASS] showcase MultiSessionControl.png ({out_multi.stat().st_size} bytes)")
            print(f"[PASS] showcase SessionsOverview.png ({out_sessions.stat().st_size} bytes)")
            print(f"[PASS] showcase ListenersAndTelemetry.png ({out_listeners.stat().st_size} bytes)")
            print(f"[PASS] showcase OperativeConsole.png ({out_console.stat().st_size} bytes)")
        finally:
            srv.terminate()
            try:
                srv.wait(timeout=5)
            except subprocess.TimeoutExpired:
                srv.kill()

    # Verify standalone zero-setup launch: ./bin/ciel auto-spawns authenticated ciel-teamserver and stops it on exit
    auto_port = pick_free_port()
    auto_addr = f"127.0.0.1:{auto_port}"
    auto_env = os.environ.copy()
    auto_env["QT_QPA_PLATFORM"] = "offscreen"
    auto_res = subprocess.run(
        [CIEL, "--addr", auto_addr, "--self-test"],
        env=auto_env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=30,
    )
    assert auto_res.returncode == 0, f"ciel auto-spawn --self-test failed ({auto_res.returncode}): {auto_res.stdout}\n{auto_res.stderr}"
    assert "QIODevice::read" not in auto_res.stderr, f"Unexpected QTcpSocket stderr warning: {auto_res.stderr}"
    try:
        with socket.create_connection(("127.0.0.1", auto_port), timeout=0.4):
            raise AssertionError("Auto-spawned ciel-teamserver did not terminate cleanly on exit")
    except OSError:
        pass
    print("[PASS] standalone ./bin/ciel auto-spawn & clean shutdown")

    print("\nALL CIEL VERIFICATION CHECKS PASSED")


if __name__ == "__main__":
    main()
