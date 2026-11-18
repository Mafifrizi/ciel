<h1 align="center">
<br>
<img src="assets/ciel_logo.jpg" width="160">
<br>
Ciel
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

Ciel pairs a compiled C++20 Qt6 desktop client with a zero-dependency Go teamserver to orchestrate [Tempest](https://github.com/Mafifrizi/tempest) runtime operatives for the [Tanuki](https://github.com/Mafifrizi/tanuki) identity ecosystem. It inspects active Kerberos caches, kernel session keyrings, UNIX socket peers, and credential teardown without browser runtimes or web toolkits.

## Features

- Single-command bootstrap (install.sh or make install) that pulls and installs Tempest and Tanuki alongside Ciel
- Native C++20 Qt6 desktop interface with zero browser wrappers
- Multi-column identity topology graph with draggable operative and revoked nodes
- Ten-column session matrix with live Event Viewer and raw telemetry inspection
- Zero external Go dependencies in the local TCP JSON teamserver
- Pre-wired policy verification for RFC 4120 clock skew, RC4 weak ciphers, and possessor-only keyring isolation

## Documentation

- [Architecture and Ecosystem Integration](docs/ARCHITECTURE.md)
- [Building and Running](docs/USAGE.md)
- [Operative Console Commands](docs/COMMANDS.md)
