# Building, Installing, and Operator Workflow

This guide explains what each component in Ciel does, how to install it, and how to walk through a complete operator session from first launch to artifact cleanup.

## Prerequisites

- Linux host (tested on Kali Linux / Debian / Ubuntu; WSL2 supported with WSLg or X11)
- Build tools (auto-installed by `install.sh` on apt-based systems): `git`, `gcc`, `g++`, `make`, `golang-go`, `cmake`, `qt6-base-dev`, `python3`, `python3-pil`, `fonts-dejavu-core`
- Optional runtime tool: `krb5-user` (`kinit`) if you want `Chronoa` to renew live TGTs via `kinit -R`

## 1. Automatic All-in-One Install (Ciel + Tempest + Tanuki)

```bash
git clone https://github.com/Mafifrizi/ciel.git
cd ciel
chmod +x install.sh
./install.sh
```

If `~/.local/bin` is not on your `PATH`, add it:

```bash
export PATH="$HOME/.local/bin:$PATH"
```

## 2. Manual Makefile Targets

- Build Ciel and bundle Tempest (`souei`, `chronoa`, `ranga`, `libranga.so`) + `tanuki` into `bin/`:
  ```bash
  make clean && make
  ```
- Install all binaries into `PREFIX/bin` (default `~/.local/bin`):
  ```bash
  make install
  ```
- Point to custom sibling clones of Tempest or Tanuki:
  ```bash
  make TEMPEST_DIR=/custom/tempest TANUKI_DIR=/custom/tanuki install
  ```
- Run the full RPC, security guard, Tempest, Tanuki, auto-spawn, and Qt6 desktop self-test suite:
  ```bash
  make test
  ```

## 3. Launching Ciel

Launch directly from the repo or `PATH`:

```bash
./bin/ciel
```

What happens automatically on launch:
1. Ciel generates a 256-bit random `CIEL_AUTH_TOKEN` and starts `ciel-teamserver` on `127.0.0.1:40056`.
2. Ciel executes an initial `full_sync` sweep across `OP-01` (Souei), `OP-02` (Chronoa), `OP-03` (Ranga baseline), `OP-04` (Ranga 0x3f000000 isolated), and `OP-06` (Tanuki doctor).
3. Ciel prints the aligned `audit` matrix in `Operative Console` and enables the 3-second diff-only background monitor (`Ciel -> Background Monitor (3s)`).
4. Closing the window cleanly shuts down the auto-started `ciel-teamserver`.

To connect to an already-running or remote teamserver instead:

```bash
./bin/ciel --addr 127.0.0.1:40056 --token-file ~/.cache/ciel_workspace/teamserver.token
```

## 4. Understanding the Workspace Layout

- **Top-Left (`Sessions` / `Session Graph`)**:
  - `Sessions`: 10-column live status matrix (`OP-01`..`OP-06`). Click any row to load its parsed JSON in `Raw JSON`; double-click a row to jump to its dedicated bottom context tab (`OP-01 Souei`..`OP-06 Tanuki`); right-click for quick operative actions.
  - `Session Graph`: Interactive vector topology showing `ciel-teamserver` linked to each live operative and secondary isolation/purge nodes. Drag nodes to rearrange, scroll the mouse wheel or use `-` / `100%` / `+` / `Fit` to zoom, and click any node to inspect its JSON.
- **Top-Right (`Event Viewer` / `Raw JSON`)**:
  - `Event Viewer`: Logs initial sync, manual actions, and state transitions during background polling.
  - `Raw JSON`: Displays formatted JSON output for the selected operative row or last executed command while preserving your scroll position.
- **Bottom Workspace (`Teamserver`, `OP-01`..`OP-06`, `Operative Console`, `Listeners`)**:
  - `Teamserver` & `OP-01`..`OP-06`: Detailed per-operative state and live JSON breakdown.
  - `Operative Console`: Interactive CLI prompt with Tab autocomplete (`help`, `sync`, `set`, `audit`, Tempest commands, and all 18 Tanuki subcommands).
  - `Listeners`: Sensor health table, **Field Target Paths** override bar, and organized **Tempest Runtime** and **Tanuki Identity Engine** dispatch buttons.

## 5. Step-by-Step Operator Workflow (Start to Finish)

### Step 1: Review Baseline Host State
On a fresh Linux host with no Kerberos tickets yet:
- `OP-01 Souei` reports your kernel session keyring ID, `/proc/keys` permissions, and detected KCM socket (if SSSD/Heimdal KCM is active).
- `OP-02 Chronoa` reports `MISSING` (exit 3) because `/tmp/krb5cc_<uid>` does not exist yet.
- `OP-03 Ranga` vs `OP-04 Isolate` shows your default shared session keyring permissions (typically `0x1f3f0000`) compared against Tempest's `0x3f000000` possessor-only isolation.
- `OP-06 Tanuki` runs `tanuki doctor` for pre-flight Kerberos/SSSD/OPSEC health.

### Step 2: Configure Real Target Artifacts
If your keytab, CCACHE, `krb5.conf`, or KCM socket lives in a custom path (for example, an unprivileged lab workspace), open **Listeners -> Field Target Paths** (or type `set` in **Operative Console**):

```text
set keytab /home/user/lab/svc.keytab
set ccache /tmp/krb5cc_lab
set krb5conf /home/user/lab/krb5.conf
sync
```

Clicking **Apply & Sync** (or running `sync`) immediately re-probes all sensors against those paths. Click **Reset Auto** (or run `set clear`) to return to host auto-discovery (`$KRB5CCNAME`, `$KRB5_KTNAME`, `$KRB5_CONFIG`, system KCM socket).

### Step 3: Diagnose, Acquire, and Inspect Kerberos / AD / NHI State
- **Pre-flight & self-healing check**: Run `doctor` (or click **Doctor** in `Listeners` / `Tanuki` menu) for a full OPSEC audit, then `fix` for dry-run remediation guidance.
- **Generate zero-DNS config & acquire tickets**:
  ```text
  config --realm CORP.LOCAL --kdc 192.168.56.106
  auth --keytab /home/user/lab/svc.keytab --principal svc@CORP.LOCAL --kdc 192.168.56.106
  chronoa /tmp/krb5cc_lab
  tgs --spn HTTP/app.corp.local@CORP.LOCAL --ccache /tmp/krb5cc_lab --kdc 192.168.56.106
  ```
- **Renew an existing renewable TGT**: Run `renew` (requires `kinit` on `PATH`).
- **Inspect AD / NHI / Certificates**:
  - `ldap --host 192.168.56.106 --realm CORP.LOCAL --query all`
  - `blast --keytab /home/user/lab/svc.keytab`
  - `adcs /path/to/templates.json`
  - `pac /path/to/pac.bin` or `shadow <hex_or_file>`
  - `nhi scan`, `nhi mesh`, `token /path/to/workload.jwt`, `kcm`

### Step 4: Monitor File Access & Verify Isolation
- Attach Souei's `inotify` watcher to confirm which process reads a sensitive credential file:
  ```text
  watch /tmp/krb5cc_lab 2000
  ```
- Verify `libranga.so` possessor-only keyring isolation (`0x3f000000`) via `isolate` (LD_PRELOAD mode) or `isolate wrapper`.

### Step 5: Sanitize Artifacts on Teardown
- **Single sensitive file + optional session keyring revocation (`OP-05` Ranga)**:
  Use **Ranga Purge File...** in `Listeners` / `Operatives` menu (prompts for confirmation) or run:
  ```text
  purge /tmp/krb5cc_lab
  ```
- **Sweep Kerberos ticket caches & temp configs (`OP-06` Tanuki)**:
  Use **Purge Caches...** in `Listeners` / `Tanuki` menu (prompts for confirmation) or run:
  ```text
  tanuki purge --all
  ```

## Headless Self-Test Mode

Run automated end-to-end UI verification without an active display server:

```bash
QT_QPA_PLATFORM=offscreen ./bin/ciel --addr 127.0.0.1:40056 --self-test --screenshot table.png --graph-screenshot graph.png
```

## Interface Snapshots

- [Sessions Matrix, Event Viewer & Context Tabs](../assets/SessionsOverview.png)
- [Interactive Session Graph](../assets/FullSessionGraph.png)
- [Listeners, Field Target Paths & Raw JSON](../assets/ListenersAndTelemetry.png)
- [Operative Console & Audit Matrix](../assets/OperativeConsole.png)
- [Multi-Window Control Showcase](../assets/MultiSessionControl.png)
