# Changelog

## 0.1.0 — release preparation, 2026-11-10

### Added

- Editor-only asset save collection with project collection rules and local SQLite storage.
- Activity, Collaboration / Hotspot, and Commit history dashboard pages.
- Raw saves, fixed five-minute UTC activity buckets, daily and category summaries, and four time presets.
- Contributor alias summaries, alias details, current direct /Game/ references, and configurable Hotspot review candidates.
- Activity JSON export/import, event-ID duplicate suppression, project-name validation, and optional team-folder exchange.
- Asynchronous Git status/history lookup and persistent per-package snapshot caching with fetch time, repository, and branch context.
- English, Japanese, and Korean installation documentation and example walkthroughs.

### Package checks

- Win64 Editor BuildPlugin completed successfully for the installed Unreal Engine 5.8.4 environment.
- The packaged plugin loaded in a separate Blueprint project and collected test asset saves.
- Activity JSON restoration, duplicate suppression, and mismatched-project rejection passed.
- A two-commit asset history, cache restoration after restart, disabled-provider fallback, and reconnect refresh passed.
- See README.md for the validation scope. These checks are not full regression coverage or validation of other engine versions or operating systems.

### Known issue

- The Git source-control log may report `fatal: Invalid path '/Temp': No such file or directory`. It reproduced in the test project with Project Activity Visualizer disabled and the Git Provider enabled. The precise origin and a fix have not been confirmed. The history and cache checks above passed in the same environment.

### Planned follow-up

- Real-time team web dashboard.
- Editing detection after the web dashboard work.

This entry records preparation and testing of 0.1.0; it does not indicate that a public release has been published.
