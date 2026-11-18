<h1 align="center">
<br>
<img src="assets/ciel_logo.jpg" width="160">
<br>
</h1>

<p align="center">
  <i>Ciel is a native desktop command and control console for the Tanuki and Tempest identity runtime ecosystem.</i>
</p>

<p align="center">
  <a href="#what-ciel-is-for">Purpose</a> |
  <a href="#quick-start-from-zero-to-first-audit">Quick Start</a> |
  <a href="docs/ARCHITECTURE.md">Architecture</a> |
  <a href="docs/USAGE.md">Usage Guide</a> |
  <a href="docs/COMMANDS.md">Command Reference</a>
</p>

<br>

<p align="center">
  <img src="assets/FullSessionGraph.png" alt="Ciel Interactive Session Graph">
</p>

<br>

<p align="center">
  <img src="assets/MultiSessionControl.png" alt="Ciel Multi-Session Control and Listeners">
</p>

## What Ciel Is For

Ciel is a compiled C++20 Qt6 desktop console backed by a zero-dependency Go teamserver. It provides direct inspection and control over:

1. **Linux Kernel Keyring & Socket Boundaries ([Tempest](https://github.com/Mafifrizi/tempest))**:
   - `OP-01` **Souei**: Audits `/proc/keys` ACLs, active session keyring permissions, `AF_UNIX` `SO_PEERCRED` on SSSD/Heimdal KCM sockets, and real-time `inotify` file access events.
   - `OP-02` **Chronoa**: Parses MIT Kerberos CCACHE v3/v4 binary ticket caches, enforces RFC 4120 clock skew (300s) and AES cipher policy, and runs real `kinit -R` ticket renewal.
   - `OP-03` / `OP-04` / `OP-05` **Ranga**: Inspects baseline session keyrings (`OP-03`), isolates process keyrings to `0x3f000000` possessor-only permissions via `libranga.so` (`OP-04`), and executes 3-pass NIST SP 800-88 file overwrite + `explicit_bzero` teardown (`OP-05`).
2. **Non-Human Identity (NHI), Kerberos & Active Directory Analysis ([Tanuki](https://github.com/Mafifrizi/tanuki))**:
   - `OP-06` **Tanuki**: Orchestrates all 18 Tanuki modules directly inside Ciel (`doctor`, `fix`, `keytab`, `config`, `triage`, `ladder`, `nhi`, `token`, `blast`, `adcs`, `pac`, `shadow`, `kcm`, `auth`, `tgs`, `ldap`, `purge`, `skill`).

All views read directly from live kernel interfaces, credential caches, and Kerberos/LDAP endpoints, with no browser runtime or synthetic demo data.

## Quick Start (From Zero to First Audit)

### 1. Install and Build Everything

```bash
git clone https://github.com/Mafifrizi/ciel.git
cd ciel
chmod +x install.sh
./install.sh
```

`install.sh` checks toolchain dependencies (`git`, `gcc`, `g++`, `make`, `go`, `cmake`, `qmake6`, `python3`), compiles Tempest, links Tanuki, builds `ciel-teamserver` and `ciel-client`, and installs all binaries into `bin/` and `~/.local/bin/`.

### 2. Launch Ciel (Single Command)

```bash
./bin/ciel
```

On startup, Ciel automatically spawns an authenticated `ciel-teamserver` bound to `127.0.0.1:40056` with a per-session 256-bit `auth_token`, runs an initial host sweep (`full_sync`), prints the aligned `audit` matrix in `Operative Console`, and starts the 3-second diff-only background monitor. Closing the Ciel window cleanly terminates the managed teamserver.

### 3. Point Ciel at Real Targets & Run Your Workflow

1. **Inspect baseline host health**: Check `Sessions` (`OP-01`..`OP-06`) or `Session Graph` (use mouse wheel or `+`/`-`/`100%`/`Fit` to zoom, drag nodes, or click a node to inspect its `Raw JSON`). If your host has no active Kerberos ticket yet, `OP-02 Chronoa` honestly reports `MISSING`.
2. **Set real target paths** (optional): Open the `Listeners` tab, fill in `CCACHE`, `Keytab`, `krb5.conf`, or `KCM Socket` under **Field Target Paths** and click **Apply & Sync**, or use the `Operative Console`:
   ```text
   set keytab /path/to/svc.keytab
   set ccache /tmp/krb5cc_1000
   sync
   ```
3. **Run diagnostics and wire operations**: Use the `Operative Console` (with Tab autocomplete), the `Listeners` dispatch buttons, the `Tanuki` / `Operatives` menus, or right-click any row in `Sessions`. For a full walkthrough from initial setup to artifact teardown, see [docs/USAGE.md](docs/USAGE.md).

## Interface Views

| View | Snapshot |
| --- | --- |
| **Sessions, Event Viewer & Context Tabs** | [assets/SessionsOverview.png](assets/SessionsOverview.png) |
| **Interactive Session Graph** | [assets/FullSessionGraph.png](assets/FullSessionGraph.png) |
| **Listeners & Raw JSON Inspector** | [assets/ListenersAndTelemetry.png](assets/ListenersAndTelemetry.png) |
| **Operative Console** | [assets/OperativeConsole.png](assets/OperativeConsole.png) |

## Documentation

- [Building, Installing, and Step-by-Step Operator Workflow](docs/USAGE.md)
- [Architecture and Ecosystem Integration](docs/ARCHITECTURE.md)
- [Operative Console and Menu Command Reference](docs/COMMANDS.md)
