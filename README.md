# Project Activity Visualizer

[English](README.md) | [日本語](README.ja.md) | [한국어](README.ko.md)

An Unreal Editor plugin for asset save activity, contributor aliases, current package references, Hotspot review candidates, and Git history.

**Version:** 0.1.0  
**Tested package environment:** Windows 64-bit / Unreal Engine 5.8.4

## Installation

- **Prebuilt package:** see [GitHub Releases](https://github.com/SEOJUNSEOK5774/Project-Activity-Visualizer/releases). The validated 0.1.0 ZIP is prepared for upload. Extract its ProjectActivityVisualizer folder into your project's Plugins folder.
- **Source:** copy this repository's [ProjectActivityVisualizer](ProjectActivityVisualizer) folder into your project's Plugins folder. Build with a compatible Unreal Engine C++ toolchain when required. GitHub's automatic source ZIP does not contain prebuilt binaries.
- Enable the plugin under Edit > Plugins, then open Window > ProjectActivityVisualizer.

## Documentation

- [Installation, setup, metrics, and package validation](ProjectActivityVisualizer/README.md)
- [Example walkthrough](ProjectActivityVisualizer/EXAMPLE.md)
- [0.1.0 changelog](ProjectActivityVisualizer/CHANGELOG.md)
- [Prepared release notes and ZIP checksum](docs/RELEASE_NOTES_0.1.0.md)
- [Original project plan](docs/PROJECT_PLAN.md) — archived design proposal; use the current documentation for implemented behavior.

## Current scope

The plugin records save events in local SQLite storage, groups repeated saves into fixed five-minute UTC activity buckets, supports JSON exchange, and displays Git history with a persistent cache. Hotspot candidates show the conditions they match; activity counts do not measure working hours or productivity.

The listed package checks passed in a separate Blueprint project. Git may emit an Invalid path '/Temp' error that also reproduces with this plugin disabled; its specific cause and a fix remain unconfirmed. See the full documentation for test scope and known issues.

The next planned work is a real-time team web dashboard, followed by editing detection. Neither feature is included in 0.1.0. License selection remains pending as recorded in the original plan.
