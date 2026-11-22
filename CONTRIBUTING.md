# Contributing to Ciel

Thank you for your interest in contributing to Ciel. Ciel is a sovereign operations teamserver and native desktop operator console designed for unprivileged Linux hosts in enterprise Active Directory environments.

We welcome contributions ranging from teamserver RPC protocols, Qt6 UI enhancements, and Shuna cryptographic integrations to documentation, test suites, and deployment recipes.

---

## Architectural Invariants

Before submitting code, please review these core architectural invariants. Every pull request is validated against them:

1. **Zero Mock Data in Production Paths**:
   Production runtime paths (teamserver, client) must never generate synthetic telemetry, fake node events, or artificial payloads. All graphs, telemetry streams, and inspector views must reflect real host, socket, and kernel state.

2. **Native Dual-Core Architecture (Go + C++20 Qt6)**:
   - **Teamserver**: Written in Go 1.24+ with strict mTLS 1.3 certificate validation and zero unvetted dependencies.
   - **Operator Console**: Compiled C++20 with Qt6. Zero webview, zero Electron, and strictly scoped stylesheets (`QWidget#ObjectName`).

3. **Strict Cryptographic Envelopes (Shuna)**:
   All PostgreSQL fleet persistence and credential transmission must utilize Shuna AEAD envelopes (SHUNA1 / SHUNAS1). Sensitive hostnames, paths, credentials, and tokens must never hit database storage or WAL in plaintext.

4. **Unprivileged Execution by Default**:
   Ciel and its companion modules (Tanuki, Tempest) are designed for standard Linux user accounts (UID 1000). Root privileges must never be required for normal operation.

---

## Where to Start: Good First Issues

Looking for areas to contribute? Here are curated high-value opportunities:

### 1. Qt6 UI / UX Refinements
- HiDPI vector icons and custom SVG node renderers.
- Keyboard navigation shortcuts across inspector tabs and operative matrices.
- Theme calibration following Swiss dark-slate typography standards.

### 2. Teamserver RPC & Telemetry Hooks
- Additional event stream filters (by host, UID, or module type).
- Custom exporter integrations (e.g., SIEM syslog, JSON-lines audit trail, or OpenTelemetry metrics).
- PostgreSQL migration scripts and fail-closed storage hardening.

### 3. Module Connectors
- Expanded CLI argument parsing and schema validation for Tanuki, Tempest, and Shuna subcommands.
- Real-time inotify watchers for credential cache file rotation.

### 4. Integration & E2E Testing
- Expanding automated pytest scenarios in `tests/test_ciel.py`.
- Mock-free local test harnesses testing teamserver startup, mTLS handshake, and PostgreSQL failover.

---

## Local Development Setup

### Prerequisites
- **Go**: 1.24 or newer
- **C++ Compiler**: GCC 11+ or Clang 14+ with C++20 support
- **Qt6**: Qt6 Base, Gui, Widgets (`libqt6core6`, `libqt6gui6`, `libqt6widgets6`, `qt6-base-dev`)
- **CMake**: 3.20 or newer
- **Make**: GNU Make 4.0+
- **PostgreSQL**: 16+ (optional, for persistent teamserver mode)

### Cloning and Building

```bash
# Clone the repository
git clone https://github.com/Mafifrizi/ciel.git
cd ciel

# Fetch companion dependencies (tanuki, tempest, shuna)
make deps

# Build the complete stack (teamserver + Qt6 client)
make build
```

The compiled binaries will be placed in `bin/`:
- `bin/ciel-teamserver` (Go teamserver backend)
- `bin/ciel-client` (Qt6 desktop GUI client)

### Running Self-Test

Verify your local build using the built-in self-test:

```bash
./bin/ciel-client --self-test
```

---

## Running Automated Tests

Before submitting a pull request, verify that all test suites pass locally:

```bash
# Run the complete test suite
make test
```

This executes:
1. `go vet ./teamserver/...`
2. `go test -v -race ./teamserver/...`
3. Python E2E integration test suite (`tests/test_ciel.py`)
4. C++ client verification and build integrity check

---

## Pull Request Guidelines

1. **Keep Commits Clean & Focused**:
   - Write clear, descriptive commit messages.
   - Maintain the standard git author identity.
   - Avoid unicode em dashes in documentation and commit messages.

2. **Verify Local CI Parity**:
   - Run `make test` locally to ensure zero warnings and passing tests.
   - GitHub Actions CI will validate all push and pull request events.

3. **Update Documentation**:
   - If adding a new feature, CLI command, or configuration option, update the corresponding files in `docs/` (`ARCHITECTURE.md`, `USAGE.md`, `COMMANDS.md`).

---

## Code of Conduct

We are committed to providing a welcoming, inclusive, and harassment-free environment for all contributors. Please be respectful, constructive, and collaborative in all interactions.
