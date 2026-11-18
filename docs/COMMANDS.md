# Operative Console Commands

Run these commands directly at the prompt inside the active operator tab (Tab autocomplete enabled), or trigger them from the Operatives top menu, Listeners bar, or Sessions right-click menu.

## Verification and Dispatch

- sync: Run a full_sync sweep across Souei, Chronoa, Ranga baseline, and Ranga isolated.
- audit: Print an aligned Kerberos CCACHE and Linux Kernel Keyring verification summary inside the console.
- souei: Execute a passive /proc/keys ACL audit paired with a live AF_UNIX SO_PEERCRED probe against kcm.sock.
- chronoa: Parse the active MIT CCACHE v4 file and validate expiration, encryption type, and drift.
- skew N: Simulate KDC clock drift of N seconds (values above 300 trigger RFC 4120 CLOCK_SKEW_VIOLATION with exit 2).
- rc4: Generate a temporary rc4-hmac (enctype 23) ticket to test WEAK_ENCTYPE policy enforcement (exit 2).
- renew: Invoke the Chronoa renewal hook to refresh the workspace CCACHE to +7200s using AES256.
- isolate: Run a workload wrapped with LD_PRELOAD=libranga.so and verify possessor-only session keyring permissions (0x3f000000).
- purge: Execute a 3-pass NIST SP 800-88 file wipe (0x00, 0xFF, random), explicit_bzero, and unlink on the target secret artifact.

## Viewport Navigation

- graph: Switch the top-left viewport to the interactive 4-column Session Graph.
- table: Switch the top-left viewport back to the 10-column Sessions table.
- clear: Clear console output and print a fresh operator session banner.
- help: Display the inline command table.
