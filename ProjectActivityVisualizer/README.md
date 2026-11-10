# Project Activity Visualizer

[English](README.md) | [日本語](README.ja.md) | [한국어](README.ko.md)

**Target version:** 0.1.0  
**Release status:** Listed package smoke tests passed; release preparation in progress

An Unreal Editor plugin that records asset save events and brings activity statistics, contributor aliases, package references, and Git history into one dashboard.

## Environment

- Package smoke-test environment: Windows 64-bit / Unreal Engine 5.8.4.
- Editor-only C++ plugin.
- Depends on the engine's SQLiteCore plugin, declared in the plugin descriptor.
- Git features require a Git installation, a local repository, and a connected Git Provider in Unreal Editor.
- Other engine versions and operating systems have not been validated.

## Package validation (2026-11-10)

The prebuilt Win64 Editor package was installed in a separate Blueprint project. The following checks passed through manual testing and review of the available build and editor logs.

| Check | Observed result |
| --- | --- |
| Win64 Editor packaging | BuildPlugin completed successfully |
| Installation in a new project | Plugin loaded and created a separate activity database |
| Save collection and statistics | Test asset saves appeared as raw saves and five-minute activity buckets |
| JSON export and import | Exported activity was restored into a fresh database |
| Duplicate JSON import | Previously imported event IDs were skipped without adding duplicate records |
| Project-name validation | JSON with a different project name was rejected without adding records |
| Git history | Two asset commits, author names, and messages were displayed; the asset was tracked and unmodified after committing |
| Cache restoration after restart | The two-entry history returned as CACHED RESULT with the original fetch time |
| Git Provider disable and reconnect | Cached history remained available while disabled; reconnecting and refreshing updated the local copy |

These are targeted smoke checks, not a full regression test of every feature. Automatic team imports and full Hotspot rule coverage were not included in these package checks. The source-control log issue below remains open.

## Installation

1. Close Unreal Editor.
2. Copy the ProjectActivityVisualizer plugin folder into your project's Plugins folder.
3. For a C++ project, regenerate project files if needed and build the Development Editor / Win64 target. Building from source requires a compatible Unreal Engine C++ toolchain.
4. Enable the plugin under Edit > Plugins and restart the editor if prompted.
5. Open Window > ProjectActivityVisualizer.

~~~text
YourProject/
└─ Plugins/
   └─ ProjectActivityVisualizer/
      ├─ ProjectActivityVisualizer.uplugin
      ├─ Source/
      ├─ Resources/
      └─ Config/
~~~

Use an engine-compatible package when installing prebuilt binaries.

## Quick start

1. Set Contributor Alias under Editor Preferences > Plugins > Project Activity.
2. Check the collection toggle, Tracked Paths, Ignored Paths, category rules, and Hotspot thresholds under Project Settings > Plugins > Project Activity. Ignored paths take priority over tracked paths.
3. Modify and save an asset under a tracked /Game/ folder.
4. Open the dashboard and click Refresh.

Changing an alias affects new records; it does not rename existing records. Disabling collection keeps previously stored records available.

## Dashboard

| Tab | Contents |
| --- | --- |
| Activity | Daily activity, categories, and the most active packages |
| Collaboration / Hotspot | Packages with multiple aliases and review candidates with their matching reasons |
| Commit history | Source control state and up to five recent history entries for the selected asset |

Click Show History beside a package on the Activity tab to query it and open the Commit history tab.

### Time ranges

- All time
- Last 24 hours
- Last 7 days: a rolling 168-hour window
- Last 30 days: a rolling 720-hour window

Filtered windows end at the last dashboard refresh and use UTC. They apply to activity counts, category statistics, package rankings, alias statistics, and the activity evidence used by Hotspot rules.

The daily chart always displays the most recent seven UTC dates, including a partial current date. Package references use current saved package data. Selected asset history is independent of the activity time filter.

## Interpreting the metrics

- **Raw saves:** recorded asset save events. Editor autosaves, cooking, and procedural saves are excluded by the collector.
- **Activity buckets:** repeated saves of the same package under the same alias within one fixed five-minute UTC interval count as one bucket. Raw events are retained.
- **Named aliases:** distinct non-empty recorded labels. These do not establish the number of unique people or simultaneous editors. Empty aliases appear as (Unspecified) in alias details.
- **References:** direct package relationships under /Game/, excluding duplicates and self-references. They are not transitive dependency counts.
- **Hotspot candidates:** packages matching configurable review conditions within the ten most active packages in the selected range. The initial rule is at least three activity buckets AND either at least two named aliases OR at least one current /Game/ referencer. These are review indicators, not quality or productivity scores.
- **Unknown:** the relevant evidence could not be retrieved; it does not mean zero. Historical entries for moved or deleted package paths may remain visible.
- **CACHED RESULT:** a previous lookup, labeled with its repository, branch, and fetch time. Its state may differ from the current workspace.

Save counts and activity buckets do not measure actual working time or productivity. Git author names and Contributor Alias labels are separate identities.

## Local data

The plugin creates its SQLite database inside the project:

~~~text
Saved/ProjectActivity/activity.db
~~~

It stores activity events and the latest source control lookup snapshot for each package. A new successful lookup updates that package's snapshot. Reading a cached result does not change its fetch time.

Export all JSON exports all stored activity events regardless of the selected dashboard range. Activity JSON does not include the source control snapshot cache.

## Team JSON exchange

1. Set Team Exchange Folder to an existing absolute path under Editor Preferences > Plugins > Project Activity.
2. Use Export to team folder to write activity JSON to that folder.
3. Use Import JSON, or enable Enable Automatic Import to check the folder every five seconds.

Imported activity must belong to the matching project name. Existing event IDs are skipped to avoid importing the same event again.

Recheck the exchange folder path after moving the project or switching PCs. Automatic import is disabled by default; leave it disabled when unused. Local exchange JSON files should be excluded from the distribution package.

## Known issues

### Git source-control log: Invalid path '/Temp'

The following error was observed after restarting the test project:

~~~text
SourceControl: Error: fatal: Invalid path '/Temp': No such file or directory
~~~

The error also reproduced with Project Activity Visualizer disabled while the Git Provider remained enabled. Reproduction therefore does not require this plugin. The specific cause in the editor, Git Provider, or project setup has not been isolated, and no fix has been confirmed.

The Git history and cache checks listed above passed in the same test environment. This release records the error as a known issue; it does not claim to resolve it.

## Current scope

Activity is recorded when assets are saved. Unsaved editing time and idle editor time are not tracked. The real-time web dashboard and editing detection are planned follow-up features.

