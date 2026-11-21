# Architecture

Ciel separates desktop operator presentation, post-quantum cryptographic fleet persistence, and low-level Linux kernel / Kerberos sensing across four decoupled layers.

## Stack Overview

1. **Desktop Client (client/src/main.cpp)**: Compiled C++20 Qt6 Widgets application communicating strictly over QSslSocket TLS 1.3 mTLS (X25519MLKEM768). Provides a 4-zone workspace with a Left Mode Rail (OPS, MOD, FLT, VLT, CFG), 6-column Operatives matrix with instant filter, interactive zoomable Session Graph, closable Right Inspector Drawer (dot-leader Summary + Raw JSON), 4-Cycle Module Catalog with multi-parameter QDialog modals, Fleet Hosts & Audit History views, Shuna Vault & mTLS PKI controls, and a 2-tab Bottom Dock (Operative Console + diff-only Event Viewer).
2. **Teamserver (teamserver/main.go, teamserver/storage.go)**: Go 1.24+ RPC service enforcing TLS 1.3 mTLS (X25519MLKEM768), fail-closed live CRL + PostgreSQL revoked_certs checking, mTLS-cert-fingerprint-bound 256-bit auth_token sessions (12h TTL, token_rotate with 30s grace, bootstrap-only session_issue for secondary operator certs), workspace-confined backup_export ({work_dir}/backups), and PostgreSQL 16 fleet persistence (pgx/v5, sslmode=verify-full).
3. **Cryptographic Engine ([Shuna](https://github.com/Mafifrizi/shuna))**: Provides ECDSA P-384 PKI + signed CRL (ca.crl), Argon2id + HKDF-SHA512 KDF, mlock/WipeBytes memory protection, hybrid ML-KEM-1024 + X25519 offline recovery keys, SHUNA1 record AEAD (XChaCha20-Poly1305 / AES-256-GCM), and SHUNAS1 chunked streaming AEAD for pg_dump backups.
4. **Runtime Operatives ([Tempest](https://github.com/Mafifrizi/tempest) + [Tanuki](https://github.com/Mafifrizi/tanuki))**: Native C11/C++20 binaries (souei, chronoa, ranga, and libranga.so) paired with all 18 Tanuki NHI/Kerberos/AD modules orchestrated inside Ciel.

## PostgreSQL Threat Model & Row-Bound AEAD

When PostgreSQL 16 is configured (--pg-dsn, CIEL_PG_DSN, or ~/.config/ciel/pg.dsn), teamserver enforces sslmode=verify-full with CA pinning against ca.crt (only UNIX domain sockets or explicit --pg-insecure-local on loopback relax TLS). If a configured DSN is unreachable, ciel-teamserver fails closed with exit code 3. When no DSN is configured, it runs in Ephemeral Standalone Mode (db_connected: false).

- **Zero Plaintext Sensitive Metadata**: All identifying and sensitive fields are encrypted in Go using SHUNA1 AEAD before reaching PostgreSQL or WAL:
  - hosts.host_meta_enc (AAD = hosts|{host_id}|1|{kv}): hostname, os_name, arch, uid, username, and nullable internal_addr.
  - target_profiles.*_enc (AAD = target_profiles|{host_id}|1|{kv}): ccache_enc, keytab_enc, krb5_conf_enc, kcm_socket_enc.
  - telemetry_snapshots.payload_enc (AAD = telemetry_snapshots|{host_id}|{snapshot_id}|{kv}): operative, module, target, detail, perm_or_ttl, skew_or_aux, raw_json.
  - audit_events.detail_enc (AAD = audit_events|{host_id}|{event_id}|{kv}): action, summary, body.
  - operator_commands.command_enc (AAD = operator_commands|{host_id}|{cmd_id}|{kv}): operator_name, action, command, args.
  - revoked_certs.subject_enc (AAD = revoked_certs|global|{serial_hex}|{kv}): subject_cn, reason.
- **Pre-Allocated BIGSERIAL AAD Binding**: Sequence-keyed tables pre-allocate their primary key via SELECT nextval(...), encrypt the payload with the bound AAD in memory, and insert ciphertext directly.
- **Online Batched Key Rotation (vault_rotate)**: Dual-key rotation inserts the new DEK version (active=TRUE), marks the old version decrypt_only=TRUE, re-encrypts older rows in bounded LIMIT 1000 FOR UPDATE SKIP LOCKED transactions across all 6 encrypted tables without blocking concurrent full_sync, and retires/zeroizes old DEKs once zero old rows remain.
- **Persistent Brute-Force Lockout**: Failed vault_unlock attempts persist in vault_lockout (1s..8s exponential cooldown on attempts 1..4, 15-minute hard lockout or --strict-lockout-wipe on attempt 5).

## Semantic Exit Contract

Tempest operatives, Shuna crypto operations, Tanuki modules, and Ciel RPC responses share a five-state exit code contract:

- **Exit 0** (OK / HEALTHY / ISOLATED / RENEWED / PURGED / READY / UNLOCKED): Verification or operation passed cleanly.
- **Exit 1** (USAGE_ERROR / BAD_JSON): Invalid flag, missing required target, protected purge path rejection, or malformed RPC payload.
- **Exit 2** (POLICY_STOP / VAULT_LOCKED / REVOKED): Clock skew beyond RFC 4120 limit (300s), weak cipher (RC4/DES), expired ticket, vault lockout, revoked certificate, or symlink purge target.
- **Exit 3** (MISSING): Target CCACHE file, KCM socket, keytab, vault metadata, or configured PostgreSQL host is unreachable/missing.
- **Exit 4** (PARSE_ERROR / CORRUPT_CIPHERTEXT): Corrupt binary format, AEAD tag failure, AAD mismatch, or non-zero AES-256-GCM nonce padding.
