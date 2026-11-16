# Project Activity Visualizer

[English](README.md) | [日本語](README.ja.md) | [한국어](README.ko.md)

An Unreal Editor plugin for asset saves, edit detection, contributor aliases, current package references, Hotspot review candidates, and Git history. The local web dashboard shows editing and saves in the same timeline.

**Version** 0.2.0

**Tested package environment** Windows x64 / Unreal Engine 5.8.3

## Installation

- [Download the 0.2.0 ZIP for UE 5.8.3 / Win64](https://github.com/SEOJUNSEOK5774/Project-Activity-Visualizer/releases/download/v0.2.0/ProjectActivityVisualizer-0.2.0-UE5.8.3-Win64.zip). Copy the extracted `ProjectActivityVisualizer` folder into your project's `Plugins` folder.
- **Source:** copy this repository's [ProjectActivityVisualizer](ProjectActivityVisualizer) folder into your project's Plugins folder. Build with a compatible Unreal Engine C++ toolchain when required. GitHub's automatic source ZIP does not contain prebuilt binaries.
- Enable the plugin under Edit > Plugins, then open Window > ProjectActivityVisualizer.
- On Windows with Python 3.9 or later, save a tracked asset and drag the project's `.uproject` onto `WebDashboard/Start-Local.cmd` to open the local dashboard.

## Documentation

- [Installation, setup, metrics, and package validation](ProjectActivityVisualizer/README.md)
- [Example walkthrough](ProjectActivityVisualizer/EXAMPLE.md)
- [Web dashboard setup](WebDashboard/README.md)
- [Changelog](ProjectActivityVisualizer/CHANGELOG.md)
- [0.2.0 release notes and ZIP checksum](docs/RELEASE_NOTES_0.2.0.md)
- [Original project plan](docs/PROJECT_PLAN.md) — archived design proposal; use the current documentation for implemented behavior.

## Current scope

The plugin records save events in local SQLite storage, groups repeated saves into fixed five-minute UTC activity buckets, supports JSON exchange, and displays Git history with a persistent cache. Hotspot candidates show the conditions they match; activity counts do not measure working hours or productivity.

The listed package checks passed in a separate Blueprint project. Git may emit an Invalid path '/Temp' error that also reproduces with this plugin disabled; its specific cause and a fix remain unconfirmed. See the full documentation for test scope and known issues.

The dashboard shows the first unsaved edit and subsequent saves with separate timestamps. With the Python startup script configured, it also shows direct dependencies and opens them in Unreal Editor. License selection remains pending as recorded in the original plan.
