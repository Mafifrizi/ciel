#!/usr/bin/env python3
import json
import os
import socket
import subprocess
import sys
import tempfile
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


def compose_showcase_images(table_shot, graph_shot, alt_shot):
    ASSETS.mkdir(parents=True, exist_ok=True)
    icon_path = ASSETS / "ciel_icon.png"

    im_graph = Image.open(graph_shot)
    framed_graph = frame_desktop_window(im_graph, "Ciel", icon_path)
    out_graph = ASSETS / "FullSessionGraph.png"
    framed_graph.convert("RGB").save(out_graph, format="PNG")

    im_back = frame_desktop_window(Image.open(alt_shot), "Ciel | Telemetry & Listeners", icon_path)
    im_front = frame_desktop_window(Image.open(table_shot), "Ciel | Session Control", icon_path)

    win_w, win_h = 1220, int(1220 * im_front.height / im_front.width)
    back_res = im_back.resize((win_w, win_h), Image.Resampling.LANCZOS)
    front_res = im_front.resize((win_w, win_h), Image.Resampling.LANCZOS)

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

    out_multi = ASSETS / "MultiSessionControl.png"
    canvas.convert("RGB").save(out_multi, format="PNG")
    return out_graph, out_multi


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
    shot_path = Path("/tmp/ciel_preview.png")
    graph_shot_path = Path("/tmp/ciel_graph_preview.png")
    alt_shot_path = Path("/tmp/ciel_alt_preview.png")
    for p in (shot_path, graph_shot_path, alt_shot_path):
        if p.exists():
            p.unlink()

    with tempfile.TemporaryDirectory() as work_dir:
        srv = subprocess.Popen(
            [TEAMSERVER, "--addr", addr, "--work-dir", work_dir],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        try:
            wait_for_server("127.0.0.1", port, srv)

            r_ping = rpc_call("127.0.0.1", port, {"action": "ping"})
            assert r_ping["ok"] is True and r_ping["status"] == "READY", r_ping
            assert r_ping["data"]["binaries_present"]["tanuki"] is True, r_ping
            print("[PASS] teamserver ping (bundled Tempest + Tanuki ready)")

            r_sync = rpc_call("127.0.0.1", port, {"action": "full_sync"})
            assert r_sync["ok"] is True and len(r_sync["rows"]) == 4, r_sync
            assert len(r_sync["logs"]) == 4
            print("[PASS] teamserver full_sync (4 operative rows)")

            r_souei = rpc_call("127.0.0.1", port, {"action": "souei_probe"})
            assert r_souei["exit_code"] in (0, 2)
            assert r_souei["data"]["session_keyring_id"] > 0
            assert r_souei["data"]["kcm_socket"]["connected"] is True, r_souei
            assert r_souei["data"]["kcm_socket"]["peer_pid"] > 0, r_souei
            print("[PASS] teamserver souei_probe (keyring + AF_UNIX SO_PEERCRED)")

            r_ok = rpc_call("127.0.0.1", port, {"action": "chronoa_check", "skew_sec": 0})
            assert r_ok["exit_code"] == 0 and r_ok["status"] == "HEALTHY", r_ok
            assert r_ok["data"]["primary_ticket"]["enctype"] == 18

            r_skew = rpc_call("127.0.0.1", port, {"action": "chronoa_check", "skew_sec": 350})
            assert r_skew["exit_code"] == 2 and r_skew["status"] == "CLOCK_SKEW_VIOLATION", r_skew

            r_rc4 = rpc_call("127.0.0.1", port, {"action": "chronoa_check", "weak_enctype": True})
            assert r_rc4["exit_code"] == 2 and r_rc4["status"] == "WEAK_ENCTYPE", r_rc4

            r_ren = rpc_call("127.0.0.1", port, {"action": "chronoa_check", "skew_sec": 0, "renew": True})
            assert r_ren["exit_code"] == 0 and r_ren["status"] == "RENEWED", r_ren
            assert r_ren["data"]["renewed"] is True and r_ren["data"]["ttl_sec"] > 6000
            print("[PASS] teamserver chronoa_check (healthy, skew 350s, rc4 weak cipher, renew)")

            r_iso = rpc_call("127.0.0.1", port, {"action": "ranga_isolate"})
            assert r_iso["exit_code"] == 0 and r_iso["data"]["possessor_only"] is True
            assert r_iso["data"]["perm_hex"] == "3f000000", r_iso
            r_wrap = rpc_call("127.0.0.1", port, {"action": "ranga_isolate", "isolate_mode": "wrapper"})
            assert r_wrap["exit_code"] == 0 and r_wrap["data"]["possessor_only"] is True
            assert r_wrap["data"]["perm_hex"] == "3f000000", r_wrap
            print("[PASS] teamserver ranga_isolate (LD_PRELOAD + --isolate wrapper 0x3f000000)")

            r_prg = rpc_call("127.0.0.1", port, {"action": "ranga_purge"})
            assert r_prg["exit_code"] == 0 and r_prg["data"]["removed"] is True
            assert r_prg["data"]["passes"] == 3 and r_prg["data"]["wiped_bytes"] == 4096, r_prg
            print("[PASS] teamserver ranga_purge (3-pass NIST wipe)")

            env = os.environ.copy()
            env["QT_QPA_PLATFORM"] = "offscreen"
            c_res = subprocess.run(
                [
                    CLIENT, "--addr", addr, "--self-test",
                    "--screenshot", str(shot_path),
                    "--graph-screenshot", str(graph_shot_path),
                    "--alt-screenshot", str(alt_shot_path),
                ],
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=30,
            )
            assert c_res.returncode == 0, f"ciel-client --self-test failed ({c_res.returncode}): {c_res.stdout}\n{c_res.stderr}"
            for src, label in (
                (shot_path, "table screenshot"),
                (graph_shot_path, "graph screenshot"),
                (alt_shot_path, "alt telemetry screenshot"),
            ):
                assert src.exists() and src.stat().st_size > 10000, f"Missing or tiny {label}"
                raw = src.read_bytes()
                assert raw[:8] == b"\x89PNG\r\n\x1a\n", f"{label} is not a valid PNG"
                print(f"[PASS] native Qt6 ciel-client --self-test {label} ({len(raw)} bytes)")

            out_graph, out_multi = compose_showcase_images(shot_path, graph_shot_path, alt_shot_path)
            print(f"[PASS] showcase FullSessionGraph.png ({out_graph.stat().st_size} bytes)")
            print(f"[PASS] showcase MultiSessionControl.png ({out_multi.stat().st_size} bytes)")
        finally:
            srv.terminate()
            try:
                srv.wait(timeout=5)
            except subprocess.TimeoutExpired:
                srv.kill()

    # Verify standalone zero-setup launch: ./bin/ciel auto-spawns ciel-teamserver, runs full checks, and stops it on exit
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
    try:
        with socket.create_connection(("127.0.0.1", auto_port), timeout=0.4):
            raise AssertionError("Auto-spawned ciel-teamserver did not terminate cleanly on exit")
    except OSError:
        pass
    print("[PASS] standalone ./bin/ciel auto-spawn & clean shutdown")

    print("\nALL CIEL VERIFICATION CHECKS PASSED")


if __name__ == "__main__":
    main()
