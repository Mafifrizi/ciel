<h1 align="center">
<br>
<img src="assets/ciel_logo.jpg" width="160">
<br>
</h1>

<p align="center">
  <i>Ciel is a native desktop command and control console for the Tanuki and Tempest identity runtime ecosystem.</i>
</p>

<p align="center">
  <a href="docs/ARCHITECTURE.md">Architecture</a> |
  <a href="docs/USAGE.md">Usage</a> |
  <a href="docs/COMMANDS.md">Commands</a>
</p>

<br>

<p align="center">
  <img src="assets/FullSessionGraph.png">
</p>

<br>

<p align="center">
  <img src="assets/MultiSessionControl.png">
</p>

## About

Ciel pairs a compiled C++20 Qt6 desktop client with a zero-dependency Go teamserver to orchestrate [Tempest](https://github.com/Mafifrizi/tempest) runtime operatives for the [Tanuki](https://github.com/Mafifrizi/tanuki) identity ecosystem. It inspects active Kerberos caches, kernel session keyrings, UNIX socket peers, and credential teardown without browser runtimes, web toolkits, or synthetic demo data.

## Features

- Single-command install (`install.sh`) and single-command launch (`ciel` auto-starts an authenticated teamserver and orchestrates every Tempest and Tanuki module directly inside Ciel)
- Native C++20 Qt6 desktop interface with zero browser wrappers
- Left-to-right interactive identity topology graph with draggable operative nodes, live status pills, and zoom controls
- Ten-column session matrix with diff-only Event Viewer, Raw JSON inspector, and Field Target Paths overrides
- Zero external Go dependencies in the local TCP JSON teamserver with per-session 256-bit `auth_token` enforcement
- Live verification for RFC 4120 clock skew, weak ciphers, `kinit -R` ticket renewal, and possessor-only `0x3f000000` keyring isolation

## Documentation

- [Architecture and Ecosystem Integration](docs/ARCHITECTURE.md)
- [Building, Operator Workflow, and UI Snapshots](docs/USAGE.md)
- [Operative Console Commands](docs/COMMANDS.md)
