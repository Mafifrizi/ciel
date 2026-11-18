# Operative Console and Menu Commands

Run commands directly inside `Operative Console` (Tab autocomplete enabled), or trigger them from the `Operatives` menu, `Tanuki` menu, `Listeners` dispatch panel, or `Sessions` right-click menu.

## Target Configuration & Workspace Navigation

- `set` : Display active CCACHE, keytab, `krb5.conf`, and KCM socket targets (either custom override or `auto:` host-discovered).
- `set <ccache|keytab|krb5conf|kcm> <path>` : Override a target path (mirrors **Field Target Paths** in `Listeners`).
- `set clear` : Clear all custom overrides and revert to host auto-discovery.
- `sync` : Run an immediate `full_sync` across `OP-01`..`OP-04` and `OP-06` (`doctor`), resetting `OP-06` back to host health view.
- `audit` : Print an aligned summary matrix of all recorded operative snapshots in the console.
- `graph` / `table` : Switch the top-left pane between `Session Graph` and `Sessions`.
- `clear` / `help` : Clear console output or print the inline command summary.

## Tempest Runtime Commands (`OP-01` through `OP-05`)

- `souei` (`OP-01`): Probe `/proc/keys`, session keyring ACLs, and `AF_UNIX` `SO_PEERCRED` on any detected or configured KCM socket.
- `watch [path] [timeout_ms]` (`OP-01`): Attach Souei's Linux `inotify` monitor to a real file path (defaults to active `FILE:` CCACHE if omitted; timeout clamped 100..10000 ms).
- `chronoa [ccache_path]` (`OP-02`): Parse MIT CCACHE v3/v4 (`$KRB5CCNAME` or `/tmp/krb5cc_<uid>`) and validate expiration, encryption type, and RFC 4120 clock skew.
- `renew` (`OP-02`): Execute `kinit -R` against the active CCACHE and re-evaluate with Chronoa.
- `ranga` (`OP-03`): Inspect the current process baseline session keyring.
- `isolate [wrapper]` (`OP-04`): Verify `0x3f000000` possessor-only session keyring isolation via `LD_PRELOAD=libranga.so` (default) or `ranga --isolate` (`wrapper`).
- `purge <path> [--revoke-session]` (`OP-05`): Execute Ranga's 3-pass NIST SP 800-88 file overwrite (`0x00`, `0xFF`, random), `explicit_bzero`, and unlink on a single explicit file (and optionally revoke the session keyring).

## Tanuki NHI & AD Engine Commands (`OP-06`)

Invoke any of the 18 Tanuki modules via `tanuki <subcmd> [args...]` or directly by name (for example `doctor` or `tanuki doctor`). Run `tanuki help` inside the console for a quick list. Note that `purge` without prefix invokes Ranga single-file purge, while `tanuki purge` invokes Tanuki cache sanitization.

- `doctor [flags]`: Pre-flight Kerberos, SSSD, keytab/CCACHE, and OPSEC diagnostics (automatically uses configured target paths if set).
- `fix [--apply | flags]`: Closed-loop self-healing checks (defaults to `--dry-run` unless `--apply` is explicitly passed).
- `keytab [path]`: Parse and inspect an RFC 4120 keytab v2 binary (defaults to configured keytab or `/etc/krb5.keytab`).
- `config --realm <REALM> --kdc <HOST> [-o <path>]`: Generate a hardened zero-DNS `krb5.conf` (outputs via `--stdout` in JSON unless `-o`/`--out` is specified).
- `triage [query]`: Lookup Kerberos and SSSD error codes (omit query to list all codes).
- `ladder`: Display the 5-rung Tactical Decision Ladder.
- `nhi [scan|mesh|...]`: Scan local filesystem for workload tokens (`scan`, default) or probe cloud/SPIFFE federation mesh (`mesh`).
- `token <path_or_jwt> [flags]`: Validate RFC 8693 workload JWT tokens.
- `blast <principal_or_flags>`: Compute Active Directory privilege tiering and blast radius (automatically forwards configured `--keytab` / `--ccache` when omitted).
- `adcs <file>`: Scan AD CS certificate templates (JSON/LDIF) or X.509 certificates for ESC1 through ESC11 misconfigurations.
- `pac <path_or_payload>`: Decode MS-PAC privilege buffers, group RIDs, and UAC flags.
- `shadow <input>`: Decode `[MS-ADTS]` 2.2.20 `msDS-KeyCredentialLink` Shadow Credential blobs.
- `kcm [file]`: Triage local SSSD KCM caches or extract streams from a specified KCM database/CCACHE file (extracted output goes to `<workspace>/kcm_out` unless `-o` is specified).
- `auth --keytab <path> --principal <p> --kdc <IP>`: Acquire a Kerberos TGT via live AS-REQ pre-auth.
- `tgs --spn <svc/host@REALM> [--ccache <path>] --kdc <IP>`: Request a Kerberos TGS service ticket.
- `ldap --host <DC_IP> --realm <REALM> --query <type>`: Query Active Directory over LDAP/LDAPS (`spn`, `asreproast`, `rbcd`, `shadow`, `unconstrained`, `all`).
- `tanuki purge <path|--all>`: Cryptographically shred an explicit credential artifact or sweep `--all` discovered ticket caches and temp configs (GUI prompts for confirmation).
- `skill`: Inspect the Tanuki capability manifest as structured JSON.
