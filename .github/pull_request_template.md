## Description

<!-- Provide a concise summary of the change, fix, or new capability. -->

## Type of Change

- [ ] Bug fix (fix:)
- [ ] New feature (feat:)
- [ ] Documentation update (docs:)
- [ ] Refactoring (refactor:)
- [ ] Test or CI enhancement (test: / ci:)

## Quality & Security Checklist

- [ ] Qt6 C++20 client builds cleanly with -Wall -Wextra -Werror.
- [ ] Go teamserver passes go vet and go test -race -count=1 ./... without failures.
- [ ] Post-quantum mTLS 1.3 (X25519MLKEM768), certificate revocation, and auth token checks remain fail-closed.
- [ ] PostgreSQL storage enforces sslmode=verify-full and encrypts sensitive metadata with row-bound SHUNA1 AEAD.
- [ ] No mock data, hardcoded credentials, or unnecessary wrapper scripts were introduced.
- [ ] Documentation stays concise without inline backtick clutter or unicode em dashes.
