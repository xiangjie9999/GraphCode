# Security policy

## Reporting a problem

Please **do not open a public issue** for a security problem. Email **support@graphcode.dev** with
`Security` in the subject, or use GitHub's private vulnerability reporting on this repository.
Include the GraphCode version, what you did, and what happened. We will acknowledge your report
within [3] working days and keep you informed until it is fixed.

Things we especially want to hear about:

- a way to make the editor's tool server accept a request that does not come from the same machine;
- a write that succeeds without a valid licence token;
- an update or Market pack accepted with a bad signature or hash;
- GraphCode sending anything beyond what [PRIVACY.md](PRIVACY.md) lists.

## Supported versions

Security fixes go into the newest release. Older releases stay downloadable for pinning, but are not patched.

## How a release is protected

- Each release manifest (`release.json`) is signed with ES256 (ECDSA P-256 over SHA-256) by one of
  the release keys in [release/release-keys.json](release/release-keys.json). The keys a build
  trusts are compiled into it (0.1.14 trusts `release-tpm-1`, `release-backup-1` and `release-tpm-2`).
- The manifest records the archive's size and SHA-256 and the SHA-256 of each download part.
  `BUNDLE-SHA256.txt` records every installed file.
- The in-editor updater checks the signature, every part, the archive, and the unpacked binaries
  and SDK files against `BUNDLE-SHA256.txt` (documentation, tools, content, config and shader folders
  are edited in place and are not hash-checked after install), and deletes a download that fails. Nothing installs without a person clicking.
- [`tools/Verify-GraphCodeRelease.ps1`](tools/Verify-GraphCodeRelease.ps1) runs the same checks on a
  manual download and sends nothing anywhere.
- The editor's tool server accepts requests from this machine only, and refuses tool requests that
  carry a browser `Origin` header. It trusts the local user account; see the
  [threat model](docs/STUDIOS.md#threat-model-who-can-talk-to-the-editor).
