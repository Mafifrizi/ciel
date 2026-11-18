#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

NEED_APT=0
for tool in git gcc g++ make go cmake qmake6 python3 pg_dump; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        NEED_APT=1
    fi
done

if [ "$NEED_APT" -eq 1 ] && command -v apt-get >/dev/null 2>&1; then
    echo "[install] Installing system toolchain dependencies (Git, GCC, Go, Qt6, CMake, OpenSSL, PostgreSQL client, Pillow)..."
    if [ "$(id -u)" -eq 0 ]; then
        DEBIAN_FRONTEND=noninteractive apt-get update
        DEBIAN_FRONTEND=noninteractive apt-get install -y git build-essential golang-go qt6-base-dev cmake pkg-config libssl-dev postgresql-client fonts-dejavu-core python3 python3-pil
    else
        sudo apt-get update
        sudo DEBIAN_FRONTEND=noninteractive apt-get install -y git build-essential golang-go qt6-base-dev cmake pkg-config libssl-dev postgresql-client fonts-dejavu-core python3 python3-pil
    fi
fi

echo "[install] Bootstrapping Shuna + Tempest + Tanuki and building Ciel..."
make all
make install

PREFIX_BIN="${PREFIX:-$HOME/.local}/bin"
echo "[install] Done. Add $PREFIX_BIN to PATH if not already present:"
echo "  export PATH=\"$PREFIX_BIN:\$PATH\""
