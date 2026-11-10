# Project Activity Visualizer 0.1.0

Editor-only plugin package, smoke-tested on Windows 64-bit / Unreal Engine 5.8.4.

## Included

- Local SQLite asset save records and fixed five-minute UTC activity buckets.
- Activity summaries, category statistics, contributor aliases, and current direct package references.
- Configurable Hotspot review candidates with matching reasons.
- JSON activity exchange with duplicate-event suppression and project-name validation.
- Git asset status and history, persistent snapshots, and cached results when the provider is disabled.
- English, Japanese, and Korean documentation and example walkthroughs.

## Installation

1. Download ProjectActivityVisualizer_0.1.0_UE5.8.4_Win64.zip from the release assets.
2. Close Unreal Editor and extract the ZIP.
3. Copy its ProjectActivityVisualizer folder into your project's Plugins folder.
4. Enable the plugin under Edit > Plugins and restart if prompted.
5. Open Window > ProjectActivityVisualizer.

Use the prebuilt package with the tested engine environment. The automatic GitHub source archives contain source code, not this binary package. Other engine versions and operating systems have not been validated.

## Validation

The package was installed in a separate Blueprint project. Save collection, JSON restoration, duplicate suppression, mismatched-project rejection, two-commit Git history, cache restoration after restart, provider-disabled fallback, and reconnect refresh passed. The final ZIP contained 36 files, all verified against the packaged originals using SHA-256.

See the plugin README for the full validation scope. These are targeted checks rather than full regression coverage.

## Known issue

Git may log `fatal: Invalid path '/Temp': No such file or directory`. It also reproduced with this plugin disabled while the Git Provider remained enabled. Its specific cause and a fix remain unconfirmed. Git history and cache tests passed in the same environment.

## ZIP checksum

File: ProjectActivityVisualizer_0.1.0_UE5.8.4_Win64.zip  
Size: 15,717,198 bytes  
SHA-256:

```text
0941167f033fd35c0eff07ab66746a502b4324e38be2b0da5cd0c1eb2d3f6745
```

The real-time web dashboard and editing detection are planned follow-up work and are not included in this version.
