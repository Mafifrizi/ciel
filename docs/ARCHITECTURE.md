# Architecture

Ciel separates desktop operator presentation from low-level Linux kernel and Kerberos sensing across three decoupled layers.

## Stack Overview

1. Desktop Client (client/src/main.cpp): Compiled C++20 Qt6 Widgets application providing the Sessions matrix, 4-column Session Graph, Event Viewer, Raw JSON inspector, Listeners panel, and interactive console.
2. Teamserver (teamserver/main.go): Zero-dependency Go service listening on local TCP JSON (default port 40056), managing a private AF_UNIX KCM test socket (kcm.sock) and sample Kerberos workspace artifacts under /tmp/ciel_workspace.
3. Runtime Operatives ([Tempest](https://github.com/Mafifrizi/tempest)): Native C11 and C++20 binaries (souei, chronoa, ranga, and libranga.so) executed by the teamserver for direct kernel and MIT CCACHE inspection.

## UI Components

- Session Graph (Session Graph tab): Interactive vector topology linking the root ciel-teamserver firewall hub to primary daemons (OP-01 Souei, OP-02 Chronoa, OP-03 Ranga baseline), derived isolation and wipe nodes (OP-04 0x3f000000, OP-05 NIST purge, OP-06 grayscale revoked ticket), and verified identity leaf targets (OP-07, OP-08).
- Sessions Matrix (Sessions tab): Borderless ten-column view displaying ID, External, Internal, User, Computer, OS, Process, PID, Perm / TTL, and Health, paired with a right-click action menu.
- Telemetry Panels (top-right pane): Event Viewer formats timestamped operator logs, while Raw JSON renders the full JSON payload from Tempest.
- Multi-Tab Workspace (bottom pane): Holds Teamserver Chat, context tabs OP-01 through OP-05, the active operator console, and the Listeners dispatch tab.

## Semantic Exit Contract

Tempest operatives and Ciel RPC responses share a five-state exit code contract designed for post-hackathon integration with [Tanuki](https://github.com/Mafifrizi/tanuki):

- Exit 0 (OK / HEALTHY / ISOLATED / RENEWED / PURGED): Verification passed cleanly.
- Exit 1 (USAGE / BAD_JSON): Invalid flag or malformed RPC payload.
- Exit 2 (POLICY_STOP): Clock skew beyond RFC 4120 limit (300s), weak cipher (RC4/DES), expired ticket, insecure ACL, or symlink purge target.
- Exit 3 (MISSING): Target CCACHE file, socket, or secret file does not exist.
- Exit 4 (PARSE_ERROR): Corrupt or unsupported CCACHE binary format.
