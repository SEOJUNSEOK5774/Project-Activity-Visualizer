# Project Activity Visualizer: example walkthrough

[English](EXAMPLE.md) | [日本語](EXAMPLE.ja.md) | [한국어](EXAMPLE.ko.md)

[Installation and metric definitions](README.md) | [Changes in 0.1.0](CHANGELOG.md)

This walkthrough creates a small example inside your own project. No sample .uproject, .uasset, activity database, or Git repository is bundled with this guide.

The packaged plugin was smoke-tested on Windows 64-bit with Unreal Engine 5.8.4. Use a separate test project for this walkthrough if your existing project already has activity records or customized collection rules.

## 1. Prepare the project

1. Install the plugin as described in README.md and enable it under Edit > Plugins.
2. Open Editor Preferences > Plugins > Project Activity. Set Contributor Alias to demo_a and leave Enable Automatic Import disabled for this example.
3. Open Project Settings > Plugins > Project Activity. Enable activity collection, include /Game/ActivityExample/ in Tracked Paths, and ensure that Ignored Paths does not exclude that folder.
4. For the optional Hotspot example, keep the default thresholds: Minimum Activity Buckets = 3, Minimum Named Aliases = 2, and Minimum Referencers = 1.

## 2. Create and save a test asset

1. In the Content Browser, create the ActivityExample folder directly under Content.
2. In that folder, create an Actor Blueprint named BP_ActivityExample.
3. Add an Integer variable named TestValue.
4. Compile the Blueprint, set TestValue's default value to 1, then compile and save it.
5. Open Window > ProjectActivityVisualizer, select All time, and click Refresh.

Look for /Game/ActivityExample/BP_ActivityExample on the Activity page. At least one raw save and one activity bucket should be recorded for the asset. Initial counts can vary with the number of saves made while creating it.

## 3. Compare raw saves and activity buckets

Change TestValue to 2, compile and save, then change it to 3, compile and save again. Click Refresh in the dashboard.

Raw saves increase when the collector records these saves. Saves of the same package under the same alias in one fixed five-minute UTC interval share an activity bucket. Crossing an interval boundary adds another bucket, even if less than five minutes have elapsed since the previous save. Make a real value change before each save; saving an unchanged asset may not produce a new event.

The daily chart uses UTC dates. A new project with no category rules normally groups this activity under Other.

## 4. Demonstrate aliases and a Hotspot candidate

1. Set Contributor Alias to demo_b. Change TestValue to 4, compile and save, then refresh the dashboard.
2. Open Collaboration / Hotspot. The asset should appear among packages with multiple contributor aliases. Use Show Aliases to inspect demo_a and demo_b.
3. Set Contributor Alias to demo_c. Change TestValue to 5, compile and save, then refresh again.

The asset now has at least three activity buckets because the bucket key includes the alias, and it has three named aliases. With the default thresholds, it qualifies for the multiple-alias Hotspot condition if it is among the ten packages selected by activity buckets. In a fresh example project it should be easy to find there.

These are simulated labels used by one tester. They do not demonstrate three real contributors or simultaneous editing. Changing the current alias does not rename historical events. Restore your preferred alias after the example.

## 5. Try JSON exchange

1. Click Export all JSON and locate the exported file under the project's Saved/ProjectActivity/Exports folder.
2. Import that file into the same project with Import JSON.
3. Because those event IDs already exist, they should be skipped and the record count should not increase.

Use JSON from a project with the same project name when exchanging activity. Activity JSON contains save records, not the Git history cache. Do not copy a working database between running editors.

## 6. Optional: Git history and cached results

This part requires Git, a local repository containing the project, a connected Git Provider in Unreal Editor, and at least one commit containing BP_ActivityExample.uasset. Git setup is separate from activity collection.

1. On the Activity page, click Show History beside BP_ActivityExample.
2. Confirm that Commit history shows the selected package's state, commit author, and message. A successful query displays Local copy updated.
3. Restart the editor and open Commit history before clicking Refresh or Show History. The saved result should appear as CACHED RESULT with its original fetch time.
4. To try the fallback, temporarily set the Revision Control Provider to None and click the plugin's Refresh. The unavailable-provider message should accompany the saved result.
5. Reconnect Git and refresh. A successful lookup updates the local copy and fetch time.

Git authors and Contributor Alias labels are separate fields. The known Invalid path '/Temp' source-control log issue is documented in README.md; it also reproduces with this plugin disabled.

## What this example demonstrates

- Save events, fixed five-minute buckets, and dashboard refresh.
- Historical aliases and one configurable Hotspot condition.
- Duplicate suppression when importing JSON.
- Optional Git history, persistent cache, and provider fallback.

It does not measure working hours, capture unsaved editing, or provide a live web dashboard.
