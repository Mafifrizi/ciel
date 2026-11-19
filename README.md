<h1 align="center">
<br>
<img src="assets/ciel_logo.jpg" width="160">
<br>
</h1>

<p align="center">
  <i>Ciel is a native desktop command and control console for the Tanuki, Tempest, and Shuna identity runtime ecosystem.</i>
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

Ciel pairs a compiled C++20 Qt6 desktop client with a Go teamserver over post-quantum hybrid mTLS 1.3 (X25519MLKEM768) to orchestrate [Tempest](https://github.com/Mafifrizi/tempest) kernel operatives and [Tanuki](https://github.com/Mafifrizi/tanuki) identity diagnostics. Optional PostgreSQL 16 persistence uses [Shuna](https://github.com/Mafifrizi/shuna) envelope encryption so sensitive hostnames, principals, paths, and audit logs never hit disk or WAL in plaintext.

## Features

- Single-command install (install.sh) and single-command launch that auto-provisions workspace PKI, starts an mTLS-authenticated teamserver, and orchestrates every Tempest and Tanuki module
- Post-quantum hybrid mTLS 1.3 (X25519MLKEM768) between the Qt6 client and Go teamserver with live CRL/DB certificate revocation and cert-bound 256-bit auth tokens
- Encrypted PostgreSQL 16 fleet persistence over verify-full TLS with row-bound SHUNA1 AEAD, HMAC blind-indexed host IDs, persisted vault lockout protection, online key rotation, and SHUNAS1 streaming backups
- Ephemeral standalone mode when no PostgreSQL DSN is configured, or fail-closed startup when a configured database is unreachable
- Native C++20 Qt6 desktop interface with an interactive Session Graph, 10-column Sessions matrix, diff-only Event Viewer, Raw JSON inspector, and Operative Console

## Documentation

- [Architecture, Cryptography, and PostgreSQL Schema](docs/ARCHITECTURE.md)
- [Building, PostgreSQL TLS Provisioning, and Operator Workflow](docs/USAGE.md)
- [Operative Console Commands](docs/COMMANDS.md)
