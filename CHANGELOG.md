# Changelog

## v1.0.0 (2026-11-22)

- Overhauled the C++20 Qt6 desktop client UI against the Infinity Curve dark-slate design system: crisp 2x HiDPI vector icons across the slim left workspace rail, top KPI/action bar, view tabs, inspector header, Vault/PKI cards, and target browsers; restrained control button palette; sans-serif UI typography with tabular monospace strictly scoped to logs, telemetry JSON, dot-leader summaries, and the operative console.
- Replaced bare QInputDialog prompts with styled dialogs (including mandatory certificate serial tracking in session issuance), added a Vault & PKI menu and 2-column scrollable 4-cycle Modules view, added explicit PostgreSQL/Vault empty-state guidance, and enforced -Wall -Wextra -Werror in client/CMakeLists.txt.
- Hardened Go teamserver execution and persistence: transactional PostgreSQL vault lockout state with strict wiped_locked enforcement, bounded-memory process timeouts, operator command redaction and OP-06 Tanuki audit recording, canonical certificate serial enforcement on secondary session_issue, fail-closed ordered token_rotate file writes, and /etc/ purge guardrails.
- Expanded teamserver, PostgreSQL 16 TLS, Qt6 offscreen rendering, and 4-cycle ecosystem end-to-end tests across tests/test_ciel.py and tests/test_ecosystem_e2e.py, and regenerated showcase assets.
