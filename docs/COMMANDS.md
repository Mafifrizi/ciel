# Operative Console Commands

Run these commands directly at the prompt inside the active operator tab (Tab autocomplete enabled), or trigger them from the Operatives menu, Tanuki menu, Listeners bar, or Sessions right-click menu.

## Tempest Runtime Commands

- sync: Run a full_sync sweep across Souei, Chronoa, Ranga baseline, Ranga isolated, and Tanuki doctor.
- audit: Print an aligned Kerberos CCACHE, Kernel Keyring, and Tanuki verification summary inside the console.
- souei: Execute a passive /proc/keys ACL audit paired with a live AF_UNIX SO_PEERCRED probe against kcm.sock.
- watch: Attach Souei Linux inotify real-time access watcher to the workspace CCACHE file.
- chronoa: Parse the active MIT CCACHE v4 file and validate expiration, encryption type, and drift.
- skew N: Simulate KDC clock drift of N seconds (values above 300 trigger RFC 4120 CLOCK_SKEW_VIOLATION with exit 2).
- rc4: Generate a temporary rc4-hmac (enctype 23) ticket to test WEAK_ENCTYPE policy enforcement (exit 2).
- expired: Generate a temporary expired CCACHE ticket to test EXPIRED lifecycle handling (exit 2).
- renew: Invoke the Chronoa renewal hook to refresh the workspace CCACHE to +7200s using AES256.
- isolate: Run a workload wrapped with LD_PRELOAD=libranga.so and verify possessor-only session keyring permissions (0x3f000000).
- purge: Execute a 3-pass NIST SP 800-88 file wipe (0x00, 0xFF, random), explicit_bzero, and unlink on the target secret artifact.

## Tanuki NHI & AD Engine Commands

Invoke any Tanuki module either directly by name or prefixed with tanuki (for example, tanuki doctor or doctor):

- doctor: Run pre-flight Kerberos and OPSEC system diagnostics on the workspace keytab and CCACHE.
- fix: Run closed-loop self-healing checks in dry-run mode against the active keytab and CCACHE.
- keytab: Parse and inspect the RFC 4120 Keytab v2 binary (service.keytab or custom path).
- config: Generate a hardened zero-DNS krb5.conf enforcing AES256 and FAST armor.
- triage [query]: Lookup Kerberos and SSSD error codes (defaults to SKEW).
- ladder: Display the 5-rung Tactical Decision Ladder.
- nhi [subcmd]: Scan workload identities and cloud federation mesh (defaults to mesh).
- token [path]: Validate RFC 8693 workload JWT tokens.
- blast [source]: Compute Active Directory privilege tiering and blast radius.
- adcs [file]: Scan AD CS certificate templates and CAs for ESC1 through ESC11 misconfigurations.
- pac [source]: Decode MS-PAC privilege buffers, group RIDs, and UAC flags.
- shadow [input]: Decode [MS-ADTS] 2.2.20 msDS-KeyCredentialLink Shadow Credential blobs.
- kcm [file]: Extract credential cache streams from SSSD KCM databases or CCACHE files.
- auth / tgs / ldap [flags]: Dispatch live AS-REQ pre-auth, TGS service ticket requests, or LDAP SPN/delegation queries.
- skill: Output the Tanuki capability manifest as structured JSON.

## Viewport Navigation

- graph: Switch the top-left viewport to the interactive 4-column Session Graph.
- table: Switch the top-left viewport back to the 10-column Sessions table.
- clear: Clear console output and print a fresh operator session banner.
- help: Display the inline command table.
