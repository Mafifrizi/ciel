# Architecture

Ciel separates desktop operator presentation from low-level Linux kernel and Kerberos sensing across three decoupled layers.

## Stack Overview

1. **Desktop Client (`client/src/main.cpp`)**: Compiled C++20 Qt6 Widgets application providing the 10-column `Sessions` matrix, interactive zoomable `Session Graph`, `Event Viewer`, `Raw JSON` inspector, `Teamserver` and `OP-01`..`OP-06` detail panes, `Operative Console`, and `Listeners` target configuration & dispatch panel.
2. **Teamserver (`teamserver/main.go`)**: Zero-dependency Go service listening on local TCP JSON (default `127.0.0.1:40056`) with per-session 256-bit `auth_token` enforcement, real host CCACHE/keytab/krb5.conf/KCM socket resolution, direct `kinit -R` renewal without shell interpolation, path-safety guards for `purge`, and full RPC routing for both Tempest and Tanuki.
3. **Runtime Operatives ([Tempest](https://github.com/Mafifrizi/tempest) + [Tanuki](https://github.com/Mafifrizi/tanuki))**: Native C11/C++20 binaries (`souei`, `chronoa`, `ranga`, and `libranga.so`) paired with all 18 Tanuki NHI/Kerberos/AD modules orchestrated inside Ciel.

## UI Components and Scroll/Refresh Behavior

- **Session Graph (`Session Graph` tab)**: Interactive 2-row vector topology linking the root `ciel-teamserver` hub to live operatives (`OP-01` Souei, `OP-02` Chronoa, `OP-03` Ranga baseline, `OP-04` Ranga `0x3f000000` isolation, `OP-05` Ranga NIST purge when executed, and `OP-06` Tanuki). Supports mouse-wheel zoom, `-` / `100%` / `+` / `Fit` toolbar controls, node dragging, and click-to-inspect JSON synchronization.
- **Sessions Matrix (`Sessions` tab)**: Borderless 10-column view (`ID`, `Host`, `Telemetry`, `Principal`, `Scope`, `Module`, `Operative`, `Target`, `Perm / TTL`, `Health`) sorted by operative ID, with hover tooltips, click-to-inspect JSON, double-click to open the operative's bottom context tab, and a right-click action menu.
- **Telemetry Panels (top-right pane)**: `Event Viewer` logs state transitions and manual actions (smart-scrolling only when already at the bottom), while `Raw JSON` renders indented JSON for the selected row or explicit action while preserving scroll position and active text selections across 3-second background polls.
- **Bottom Workspace**: Holds `Teamserver`, `OP-01 Souei` through `OP-06 Tanuki` context tabs (diff-checked so background polls never reset scrollbars), `Operative Console`, and `Listeners`.

## Semantic Exit Contract

Tempest operatives, Tanuki modules, and Ciel RPC responses share a five-state exit code contract:

- **Exit 0** (`OK` / `HEALTHY` / `ISOLATED` / `RENEWED` / `PURGED` / `READY`): Verification or operation passed cleanly.
- **Exit 1** (`USAGE_ERROR` / `BAD_JSON`): Invalid flag, missing required target, protected purge path rejection, or malformed RPC payload.
- **Exit 2** (`POLICY_STOP`): Clock skew beyond RFC 4120 limit (300s), weak cipher (RC4/DES), expired ticket, insecure ACL, or symlink purge target.
- **Exit 3** (`MISSING`): Target CCACHE file, KCM socket, keytab, or artifact does not exist.
- **Exit 4** (`PARSE_ERROR`): Corrupt or unsupported binary format.
