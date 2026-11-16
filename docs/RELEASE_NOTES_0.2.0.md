# Project Activity Visualizer 0.2.0

This release adds edit detection and the local web dashboard. The ZIP contains the editor plugin for **Unreal Engine 5.8.3 on Windows x64** and the dashboard with its Python server.

## What's new

- **Edit History** records when a tracked asset first gains unsaved changes. **Save History** records the later save as a separate event.
- The web dashboard includes Overview, Assets, Collaboration, and edit and save timelines. Dark mode uses a black background and yellow accents.
- The reference graph shows direct `/Game/` dependencies. With the Python startup script configured, selecting a node opens that asset in the matching Unreal Editor.
- The team dashboard can import save records from a shared folder. The bundle includes a private-network HTTPS configuration example.

## Install

1. Download the Win64 ZIP below, close Unreal Editor, and extract it. Copy `ProjectActivityVisualizer` into your project's `Plugins` folder.
2. Enable **Project Activity Visualizer** in the editor and save a tracked asset.
3. With Python 3.9 or later installed, drag your `.uproject` onto `WebDashboard/Start-Local.cmd`. Follow the dashboard guide to enable reference export and asset opening. [Web dashboard setup](https://github.com/SEOJUNSEOK5774/Project-Activity-Visualizer/blob/v0.2.0/WebDashboard/README.md)

## Verified on UE 5.8.3

A separate Blueprint project loaded the prebuilt plugin and connected through the Python startup script. The test verified an edit before saving, separate edit and save timestamps, the Blueprint dependency graph, and opening its target in the editor. The extracted ZIP was also checked in Chromium with the test project's activity data.

## ZIP checksum

`ProjectActivityVisualizer-0.2.0-UE5.8.3-Win64.zip` · 380,348 bytes

SHA-256

~~~text
9c40f0c63e28cc78ebb90d90a431e6186e30d666385255af9f1cf2694af953b3
~~~
