CREATE TABLE IF NOT EXISTS schema_migrations (
    version     INTEGER PRIMARY KEY,
    applied_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Master key wrapping metadata (plaintext DEKs are NEVER stored in PostgreSQL)
CREATE TABLE IF NOT EXISTS crypto_keys (
    key_version        INTEGER PRIMARY KEY,
    key_id             TEXT NOT NULL UNIQUE,
    algorithm          TEXT NOT NULL,
    wrapped_dek        BYTEA NOT NULL,
    kem_ciphertext     BYTEA,
    active             BOOLEAN NOT NULL DEFAULT TRUE,
    decrypt_only       BOOLEAN NOT NULL DEFAULT FALSE,
    created_at         TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    rotated_at         TIMESTAMPTZ,
    retired_at         TIMESTAMPTZ
);

-- Active CRL / revoked client & server certificates (opaque 128-bit serial; CN & reason encrypted)
CREATE TABLE IF NOT EXISTS revoked_certs (
    serial_hex         TEXT PRIMARY KEY,
    subject_enc        BYTEA NOT NULL,
    key_version        INTEGER NOT NULL REFERENCES crypto_keys(key_version),
    revoked_at         TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_revoked_certs_kv ON revoked_certs (key_version);

-- Persistent brute-force unlock state (prevents counter reset via teamserver restart)
CREATE TABLE IF NOT EXISTS vault_lockout (
    id                 INTEGER PRIMARY KEY CHECK (id = 1),
    failed_attempts    INTEGER NOT NULL DEFAULT 0,
    locked_until       TIMESTAMPTZ,
    last_failure_at    TIMESTAMPTZ,
    wiped_locked       BOOLEAN NOT NULL DEFAULT FALSE
);

-- Multi-host fleet registry (host_id is pseudonymous HMAC blind-index; host details encrypted in host_meta_enc)
CREATE TABLE IF NOT EXISTS hosts (
    host_id            TEXT PRIMARY KEY,
    status             TEXT NOT NULL DEFAULT 'ONLINE',
    host_meta_enc      BYTEA NOT NULL,
    key_version        INTEGER NOT NULL REFERENCES crypto_keys(key_version),
    first_seen_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_seen_at       TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_hosts_last_seen ON hosts (last_seen_at DESC);
CREATE INDEX IF NOT EXISTS idx_hosts_kv ON hosts (key_version);

-- Persistent operator target overrides per host (encrypted with AAD='target_profiles|<host_id>|1|<kv>')
CREATE TABLE IF NOT EXISTS target_profiles (
    host_id            TEXT PRIMARY KEY REFERENCES hosts(host_id) ON DELETE CASCADE,
    ccache_enc         BYTEA,
    keytab_enc         BYTEA,
    krb5_conf_enc      BYTEA,
    kcm_socket_enc     BYTEA,
    key_version        INTEGER NOT NULL REFERENCES crypto_keys(key_version),
    updated_at         TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_profiles_kv ON target_profiles (key_version);

-- Latest & historical operative snapshots per host
-- target_summary, perm_or_ttl, skew_or_aux, and full raw_json reside strictly inside payload_enc
CREATE TABLE IF NOT EXISTS telemetry_snapshots (
    snapshot_id        BIGSERIAL PRIMARY KEY,
    host_id            TEXT NOT NULL REFERENCES hosts(host_id) ON DELETE CASCADE,
    operative_id       TEXT NOT NULL,
    exit_code          INTEGER NOT NULL,
    status             TEXT NOT NULL,
    payload_enc        BYTEA NOT NULL,
    key_version        INTEGER NOT NULL REFERENCES crypto_keys(key_version),
    observed_at        TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_snapshots_host_op_time
    ON telemetry_snapshots (host_id, operative_id, observed_at DESC);
CREATE INDEX IF NOT EXISTS idx_snapshots_kv
    ON telemetry_snapshots (key_version, snapshot_id);

-- Append-only audit event ledger (summary, action, and full details reside strictly inside detail_enc)
CREATE TABLE IF NOT EXISTS audit_events (
    event_id           BIGSERIAL PRIMARY KEY,
    host_id            TEXT NOT NULL REFERENCES hosts(host_id) ON DELETE CASCADE,
    operative_id       TEXT NOT NULL,
    severity           TEXT NOT NULL,
    exit_code          INTEGER NOT NULL,
    status             TEXT NOT NULL,
    detail_enc         BYTEA NOT NULL,
    key_version        INTEGER NOT NULL REFERENCES crypto_keys(key_version),
    created_at         TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_audit_events_host_time
    ON audit_events (host_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_audit_events_kv
    ON audit_events (key_version, event_id);

-- Operator console command ledger (operator_name and command text reside strictly inside command_enc)
CREATE TABLE IF NOT EXISTS operator_commands (
    cmd_id             BIGSERIAL PRIMARY KEY,
    host_id            TEXT NOT NULL REFERENCES hosts(host_id) ON DELETE CASCADE,
    exit_code          INTEGER NOT NULL,
    status             TEXT NOT NULL,
    command_enc        BYTEA NOT NULL,
    key_version        INTEGER NOT NULL REFERENCES crypto_keys(key_version),
    executed_at        TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_operator_commands_host_time
    ON operator_commands (host_id, executed_at DESC);
CREATE INDEX IF NOT EXISTS idx_operator_commands_kv
    ON operator_commands (key_version, cmd_id);
