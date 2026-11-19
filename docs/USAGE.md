# Building, Installing, and Operator Workflow

This guide explains how to build Ciel (with Shuna, Tempest, and Tanuki), optionally provision PostgreSQL 16 with mandatory TLS (sslmode=verify-full), and walk through an operator session.

## Prerequisites

- Linux host (tested on Kali Linux / Debian / Ubuntu; WSL2 supported with WSLg or X11)
- Minimum toolchain: Go 1.24+ (tested go1.27.3), OpenSSL 3.5+ (tested 3.6.5 with native X25519MLKEM768), Qt 6.5+ (tested 6.11.2), CMake, GCC/G++
- Build tools (auto-installed by install.sh on apt-based systems): git, build-essential, golang-go, qt6-base-dev, cmake, pkg-config, libssl-dev, postgresql-client, python3, python3-pil, fonts-dejavu-core
- Optional runtime tools: krb5-user (kinit) for live TGT renewal, and postgresql-16 / Docker for persistent fleet mode

## 1. Automatic All-in-One Install (Ciel + Shuna + Tempest + Tanuki)

```bash
git clone https://github.com/Mafifrizi/ciel.git
cd ciel
chmod +x install.sh
./install.sh
```

If ~/.local/bin is not on your PATH, add it:

```bash
export PATH="$HOME/.local/bin:$PATH"
```

## 2. Manual Makefile Targets

Build Ciel and bundle shuna, Tempest (souei, chronoa, ranga, libranga.so), and tanuki into bin/:

```bash
make clean && make
```

Install all binaries into PREFIX/bin (default ~/.local/bin):

```bash
make install
```

Point to custom sibling clones of Shuna, Tempest, or Tanuki:

```bash
make SHUNA_DIR=/custom/shuna TEMPEST_DIR=/custom/tempest TANUKI_DIR=/custom/tanuki install
```

Run the full Go race suite (teamserver) and Python/Qt6/PostgreSQL integration suite:

```bash
make test
```

## 3. Standalone Mode vs. PostgreSQL 16 Fleet Mode

### Ephemeral Standalone Mode (Zero Setup)

Launch directly with no PostgreSQL DSN configured:

```bash
./bin/ciel
```

What happens automatically on launch:
1. Ciel auto-provisions an ECDSA P-384 CA, CRL (ca.crl), server cert, and 24h client mTLS cert under {work_dir}/pki if missing.
2. Ciel generates a 256-bit random CIEL_AUTH_TOKEN and starts ciel-teamserver on 127.0.0.1:40056 enforcing TLS 1.3 mTLS (X25519MLKEM768).
3. Ciel connects via QSslSocket, executes an initial full_sync across OP-01..OP-04 and OP-06, and enables the 3-second diff-only background monitor.
4. Closing the window cleanly shuts down the auto-started ciel-teamserver.

### PostgreSQL 16 Fleet Mode (sslmode=verify-full)

1. Provision Shuna PKI and Vault (only needed once before first Docker startup if ciel has not run yet):
```bash
export CIEL_WORK_DIR="${XDG_RUNTIME_DIR:-$HOME/.cache}/ciel_workspace"
export CIEL_VAULT_PASSPHRASE="your-strong-vault-passphrase"
./bin/shuna init-pki --out "$CIEL_WORK_DIR/pki" --hosts 127.0.0.1,localhost --pg-hosts postgres,127.0.0.1,localhost
./bin/shuna keygen --out "$CIEL_WORK_DIR/vault" --passphrase-env CIEL_VAULT_PASSPHRASE
```

2. Start PostgreSQL 16 with ssl=on, scram-sha-256, and hostssl enforcement:
```bash
export POSTGRES_PASSWORD="your-strong-pg-password"
CIEL_PKI_DIR="${XDG_RUNTIME_DIR:-$HOME/.cache}/ciel_workspace/pki" \
  docker compose -f deploy/docker-compose.yml up -d
```

3. Start ciel-teamserver pointed at PostgreSQL (sslmode=verify-full + CA pin):
```bash
export CIEL_PG_DSN="postgres://ciel:${POSTGRES_PASSWORD}@127.0.0.1:5432/ciel?sslmode=verify-full&sslrootcert=${XDG_RUNTIME_DIR:-$HOME/.cache}/ciel_workspace/pki/ca.crt"
./bin/ciel-teamserver --addr 127.0.0.1:40056
```
If CIEL_VAULT_PASSPHRASE is omitted at teamserver startup, the teamserver starts in VAULT_LOCKED state and accepts unlock [passphrase] from the Ciel console (rate-limited Argon2id with persisted vault_lockout).

4. Connect the desktop client:
```bash
./bin/ciel --addr 127.0.0.1:40056 --token-file "${XDG_RUNTIME_DIR:-$HOME/.cache}/ciel_workspace/teamserver.token"
```

## 4. Operator Workflow Highlights

- **Configure & Persist Target Paths**: Set CCACHE, Keytab, krb5.conf, or KCM Socket in Listeners -> Field Target Paths (or set [key] [path] in Operative Console). When connected to PostgreSQL, paths auto-save encrypted via profile_save and auto-reload on future sessions via profile_get.
- **Inspect Fleet History & Hosts**: Run hosts and history [OP-01..OP-06] [limit] in Operative Console to decrypt fleet metadata and audit history while verifying row-bound AAD.
- **Online Key Rotation & Encrypted Backups**:

```text
rotate
backup /tmp/ciel-fleet.shunas1
```

Verify or restore an encrypted backup offline with shuna:

```bash
./bin/shuna unseal --stream --key "${XDG_RUNTIME_DIR:-$HOME/.cache}/ciel_workspace/vault" --aad "ciel:pg_dump:v1" --in /tmp/ciel-fleet.shunas1 --out /tmp/ciel-fleet.sql
```

## Headless Self-Test Mode

Run automated end-to-end UI and mTLS verification without an active display server:

```bash
QT_QPA_PLATFORM=offscreen ./bin/ciel --addr 127.0.0.1:40056 --self-test --screenshot table.png --graph-screenshot graph.png
```

## Interface Snapshots

- [Sessions Matrix, Event Viewer & Context Tabs](../assets/SessionsOverview.png)
- [Interactive Session Graph](../assets/FullSessionGraph.png)
- [Listeners, Field Target Paths & Raw JSON](../assets/ListenersAndTelemetry.png)
- [Operative Console & Audit Matrix](../assets/OperativeConsole.png)
- [Multi-Window Control Showcase](../assets/MultiSessionControl.png)
