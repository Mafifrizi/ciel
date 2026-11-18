#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

WORK_DIR="${CIEL_WORK_DIR:-${XDG_RUNTIME_DIR:-$HOME/.cache}/ciel_workspace}"
PKI_DIR="${CIEL_PKI_DIR:-$WORK_DIR/pki}"
VAULT_DIR="${CIEL_VAULT_DIR:-$WORK_DIR/vault}"
PG_HOSTS="${CIEL_PG_HOSTS:-postgres,127.0.0.1,localhost}"
RPC_HOSTS="${CIEL_RPC_HOSTS:-127.0.0.1,localhost}"
PG_USER="${POSTGRES_USER:-ciel}"
PG_DB="${POSTGRES_DB:-ciel}"
PG_PORT="${PG_PORT:-5432}"

SHUNA_BIN="${SHUNA_BIN:-$REPO_ROOT/bin/shuna}"
if [ ! -x "$SHUNA_BIN" ]; then
    SHUNA_BIN="$(command -v shuna || true)"
fi
if [ -z "$SHUNA_BIN" ] || [ ! -x "$SHUNA_BIN" ]; then
    echo "[init_pg] ERROR: shuna binary not found. Run 'make all' in $REPO_ROOT first." >&2
    exit 3
fi

mkdir -p "$WORK_DIR" "$PKI_DIR" "$VAULT_DIR"
chmod 700 "$WORK_DIR" "$VAULT_DIR"

if [ ! -f "$PKI_DIR/ca.crt" ] || [ ! -f "$PKI_DIR/pg-server.crt" ] || [ ! -f "$PKI_DIR/server.crt" ]; then
    echo "[init_pg] Provisioning Shuna P-384 CA, CRL, server cert, and PostgreSQL server cert in $PKI_DIR..."
    "$SHUNA_BIN" init-pki --out "$PKI_DIR" --hosts "$RPC_HOSTS" --pg-hosts "$PG_HOSTS"
fi

if [ ! -f "$PKI_DIR/client.crt" ] || [ ! -f "$PKI_DIR/client.key" ]; then
    echo "[init_pg] Issuing 24h client mTLS certificate in $PKI_DIR..."
    "$SHUNA_BIN" issue-cert --pki-dir "$PKI_DIR" --cn "client" --role client --ttl 24h
fi

chmod 644 "$PKI_DIR/ca.crt" "$PKI_DIR/server.crt" "$PKI_DIR/pg-server.crt" "$PKI_DIR/client.crt"
chmod 600 "$PKI_DIR/ca.key" "$PKI_DIR/server.key" "$PKI_DIR/pg-server.key" "$PKI_DIR/client.key"

if [ ! -f "$VAULT_DIR/vault.json" ]; then
    if [ -z "${CIEL_VAULT_PASSPHRASE:-}" ]; then
        echo "[init_pg] Generating vault requires CIEL_VAULT_PASSPHRASE env var when $VAULT_DIR/vault.json is absent." >&2
        exit 1
    fi
    echo "[init_pg] Initializing Shuna Argon2id + ML-KEM-1024+X25519 vault in $VAULT_DIR..."
    "$SHUNA_BIN" keygen --out "$VAULT_DIR" --passphrase-env CIEL_VAULT_PASSPHRASE
fi

DSN="postgres://${PG_USER}:<POSTGRES_PASSWORD>@127.0.0.1:${PG_PORT}/${PG_DB}?sslmode=verify-full&sslrootcert=${PKI_DIR}/ca.crt"
echo "[init_pg] PKI and Vault ready:"
echo "  PKI Dir   : $PKI_DIR"
echo "  Vault Dir : $VAULT_DIR"
echo "  PG DSN    : $DSN"
echo ""
echo "[init_pg] Start PostgreSQL 16 TLS container:"
echo "  CIEL_PKI_DIR=\"$PKI_DIR\" POSTGRES_PASSWORD=\"<password>\" docker compose -f \"$REPO_ROOT/deploy/docker-compose.yml\" up -d"
