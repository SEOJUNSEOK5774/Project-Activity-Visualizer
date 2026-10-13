# Project Activity Visualizer for Unreal Engine

> Unreal Engineのチーム開発において、  
> **「プロジェクトのどこに変更が集中しているのか」**を可視化するEditor Plugin。

**Project Activity Visualizer analyzes the project, not the developer.**

> このPluginが分析するのは「開発者」ではなく「プロジェクト」です。

---

## Overview

**Project Activity Visualizer** は、Unreal Engineでのチーム開発における
アセットの変更傾向を収集・分析し、Editor上で可視化するためのPluginです。

主に以下のような情報を把握することを目的としています。

- どのシステムが頻繁に変更されているのか
- どのアセットに変更が集中しているのか
- 複数のContributorが同じアセットを頻繁に変更していないか
- 現在どの領域の開発が活発なのか
- Merge Conflictが発生しやすい場所はどこか
- リファクタリングや責務分離を検討すべきアセットはどこか

本Pluginの目的は、個人の作業量や生産性を評価することではありません。

アセットの変更履歴をプロジェクト構造と結び付けることで、

**設計・役割分担・リファクタリング・チームコミュニケーションの判断材料を提供すること**

を目的としています。

---

## Motivation

ゲーム開発の規模が大きくなるにつれて、

> 「現在、プロジェクトのどこに変更が集中しているのか」

をチーム全体で把握することが難しくなります。

例えば、以下のような状況があります。

- `BP_Player` を複数人が頻繁に変更している
- 一つのBlueprintに多くの責務が集中している
- 特定のシステムだけ変更回数が極端に多い
- 同じアセットを複数人が並行して編集している
- Source Control上では変更履歴を確認できるが、Unreal Engine内のシステム構造との関係が分かりにくい

Source Controlは、

> **What changed? — 何が変更されたか**

を確認することには優れています。

Project Activity Visualizerでは、そこからさらに、

> **Where are changes concentrating? — どこに変更が集中しているのか**

を確認できるツールを目指します。

---

## Core Concept

基本的な処理フローは以下の通りです。

```text
Unreal Editor
      ↓
Asset Activity Detection
      ↓
Activity Record
      ↓
Local Storage
      ↓
Activity Analysis
      ↓
Editor Dashboard
```

例えば、プロジェクト内で以下のアセットが変更されたとします。

```text
/Game/Characters/BP_Player
/Game/Combat/Abilities/GA_Attack
/Game/UI/WBP_HUD
/Game/Maps/Stage01
```

Pluginは、変更そのものの内容ではなく、以下のMetadataを記録します。

```text
Contributor
Asset Path
Asset Type
Category
Timestamp
Activity Type
```

収集したデータを集計し、例えば以下のように表示します。

```text
Character    ███████████  34%
Combat       █████████    28%
UI           █████        18%
Level        ████         13%
Other        ██            7%
```

これにより、

**現在プロジェクトのどこに開発活動が集中しているのか**

をEditor内から確認できます。

---

# Goals

Project Activity Visualizerでは、以下を主な目標とします。

### Project Activity Visualization

プロジェクト内の変更傾向を可視化する。

### Change Concentration Analysis

変更が集中しているアセット・システムを発見する。

### Conflict Risk Awareness

複数人が頻繁に変更しているアセットを把握する。

### Architecture Analysis

責務集中や高依存度のアセットを設計上の確認候補として提示する。

### Refactoring Decision Support

リファクタリングやシステム分離を検討するための情報を提供する。

---

# Non-Goals

本Pluginは、以下を目的としません。

- Employee Monitoring
- Time Tracking
- Productivity Scoring
- Performance Evaluation
- Work Hour Tracking
- Developer Ranking
- Project Management

以下のような使用方法は想定していません。

```text
誰が一番多く作業したか？
誰が一番少なくAssetを変更したか？
誰が何時間Editorを使用したか？
```

代わりに、

```text
なぜBP_Playerに変更が集中しているのか？
複数人が同じAssetを変更していないか？
責務を分離した方が良いSystemはないか？
```

といった、

**プロジェクト設計とチーム開発の改善につながる問い**

を支援します。

---

# Design Principles

## 1. Project First

分析対象はDeveloperではなくProjectです。

変更回数そのものを個人評価に利用するのではなく、

**プロジェクト構造上の傾向**

として扱います。

---

## 2. Local-First

初期バージョンでは、Activity Dataをすべてローカルに保存します。

```text
<Project>/
└── Saved/
    └── ProjectActivity/
        └── activity.db
```

外部Serverは必要ありません。

また、ユーザーの許可なくデータを外部へ送信することはありません。

将来的にRemote Syncを実装する場合も、

**Default OFF / Opt-In**

を基本方針とします。

---

## 3. Minimum Data Collection

収集する情報は、

**プロジェクト変更傾向の分析に必要な最小限のMetadata**

に限定します。

収集例：

```text
Contributor: Junseok
Asset: /Game/Combat/GA_Attack
Asset Type: Blueprint
Category: Combat
Timestamp: 2026-10-11T06:30:20Z
Event: AssetSaved
```

収集しない情報：

- Source Codeの内容
- Blueprint Graphの内容
- Keyboard Input
- Mouse Activity
- Screenshot
- Chat Log
- 個人ファイル
- Editor使用時間
- 作業時間
- 画面操作履歴

---

## 4. Project Independent

特定のGame Project専用の実装にはしません。

例えば以下のような情報をPlugin内部へHard Codingしません。

```text
BP_Player
GA_Attack
THE_EXPLORER
/Game/CHAR/
```

プロジェクトごとのFolder構成やCategoryは、

**Project Settingsから設定可能**

な構造とします。

---

# Main Features

## Asset Activity Tracking

初期バージョンでは、

```text
Asset Saved
```

を主なTracking Eventとします。

将来的には以下への対応を検討します。

```text
Asset Created
Asset Renamed
Asset Deleted
Source Control Commit
Source Control Changelist
```

基本Tracking対象：

```text
/Game/
```

基本除外対象：

```text
/Engine/
/Script/
/Temp/
/Transient/
```

---

## Project Category

Unreal Engine ProjectごとにFolder構成は異なるため、
Categoryは固定しません。

例えば以下のように設定できます。

```text
/Game/Characters/    → Character
/Game/Combat/        → Combat
/Game/UI/            → UI
/Game/Maps/          → Level
/Game/Animation/     → Animation
```

別のProjectでは、

```text
/Game/Player/        → Gameplay
/Game/Data/          → Data
/Game/Levels/        → Level
```

のように自由に設定できます。

どのRuleにも該当しない場合は、

```text
Other
```

として分類します。

---

## Contributor

各ユーザーは自分のContributor Aliasを設定します。

例：

```text
Project Settings
    ↓
Project Activity
    ↓
Contributor Name
    ↓
Junseok
```

将来的には、

```text
Real Name
Alias
Anonymous ID
```

などのモードを検討します。

Anonymous Modeの例：

```text
Contributor_01
Contributor_02
Contributor_03
```

---

# Dashboard

Pluginを導入すると、Unreal Editor内にDashboardを追加します。

```text
Window
    ↓
Project Activity
```

Dashboardの初期イメージ：

```text
┌──────────────────────────────────────────┐
│ PROJECT ACTIVITY                         │
│                                          │
│ Time Range: Last 7 Days                  │
│                                          │
│ Most Active Systems                      │
│                                          │
│ Character      ███████████      34%      │
│ Combat         █████████        28%      │
│ UI             █████            18%      │
│ Level          ████             13%      │
│                                          │
│ Most Modified Assets                     │
│                                          │
│ BP_Player                    22 changes   │
│ GA_Attack                    14 changes   │
│ WBP_HUD                       9 changes   │
│                                          │
│ Contributor Focus                        │
│                                          │
│ Junseok        Character / Combat        │
│ User B         Level / UI                │
└──────────────────────────────────────────┘
```

---

## Time Range

以下の期間でActivityを分析できるようにします。

```text
Today
Last 7 Days
Last 30 Days
Custom Range
```

これにより、

短期間の開発集中領域と長期間の傾向を比較できます。

---

# Hot Assets

変更頻度が高いアセットを、

**Hot Asset**

として表示します。

例：

```text
BP_Player

Changes         38
Contributors     4
Recent Changes  21
```

Dashboard：

```text
HIGH ACTIVITY
```

ただし、

**変更回数が多い = 設計が悪い**

とは判断しません。

変更が多い理由には、

- プロジェクトの中心アセットである
- 現在積極的に開発されている
- 多数のシステムから参照されている
- 複数Contributorが担当している
- 責務が集中している

など、さまざまな可能性があります。

Pluginは答えを出すのではなく、

**確認する価値のあるアセットを提示します。**

---

# Hotspot Analysis

将来的には複数の指標を組み合わせ、

**Hotspot Score**

を計算する予定です。

Concept：

```text
Modification Frequency
        +
Contributor Count
        +
Recent Activity
        +
Dependency Count
        ↓
Hotspot Score
```

例：

```text
BP_Player

Modification Frequency    HIGH
Contributors               4
Dependencies              17
Recent Activity           HIGH

Hotspot Score
82 / 100
```

Scoreは、

> このAssetは設計上確認する価値が高い可能性がある

という判断材料として使用します。

具体的な計算式は、実装・検証を通して調整します。

---

# Conflict Risk

複数のContributorが同じAssetを頻繁に変更している場合、
開発上のConflict Riskが高まる可能性があります。

例：

```text
BP_Player

Junseok       12 changes
User B         9 changes
User C         6 changes

Contributors       3
Recent Changes    27
```

Dashboard：

```text
MULTI-CONTRIBUTOR HOTSPOT

Multiple contributors frequently modify this asset.

Consider reviewing:
- Responsibility
- Ownership
- System Separation
```

目的は、

Merge Conflictが発生してから対応するのではなく、

**設計・役割分担・コミュニケーションを事前に確認するきっかけを作ること**

です。

---

# Source Control Integration

Editor上のSave回数は、
必ずしも「意味のある変更回数」と一致しません。

例えば、

```text
Ctrl + S
Ctrl + S
Ctrl + S
```

と保存しただけでも、複数のEventとして記録される可能性があります。

そのため将来的には、

```text
Editor Activity
        +
Source Control History
        ↓
Project Activity Analysis
```

という構成を予定しています。

役割：

```text
Editor Activity
→ リアルタイムの開発傾向

Source Control
→ 確定した変更履歴
```

特定のSource Controlだけに依存せず、
可能な限りUnreal EngineのSource Control Interfaceを利用します。

想定Provider：

```text
Perforce
Git
SVN
Diversion
Other Unreal-compatible Providers
```

---

# Asset Dependency Analysis

将来的にはAsset Registryを使用して、
Asset Dependencyも分析対象にします。

例：

```text
BP_Player
    ↓
GA_Attack
    ↓
GE_Damage
    ↓
AttributeSet
```

以下の条件を持つAssetは、
Project Architecture上の重要なHotspotである可能性があります。

```text
High Modification Frequency
        +
High Dependency Count
        +
Multiple Contributors
```

例：

```text
BP_Player

Changes       HIGH
Dependencies  HIGH
Contributors  HIGH

→ Critical Project Hotspot
```

Dependency情報を含めることで、

単純な変更回数だけでは分からない、
プロジェクト構造上の重要度も考慮できるようにします。

---

# System Architecture

Plugin内部では、

**Collection / Storage / Analysis / Visualization**

を分離します。

```text
┌──────────────────────────┐
│ Activity Collector       │
│                          │
│ Asset Events             │
│ Source Control           │
└────────────┬─────────────┘
             │
             ▼
┌──────────────────────────┐
│ Activity Storage         │
│                          │
│ SQLite                   │
└────────────┬─────────────┘
             │
             ▼
┌──────────────────────────┐
│ Activity Analyzer        │
│                          │
│ Category Statistics      │
│ Asset Statistics         │
│ Contributor Statistics   │
│ Hotspot Analysis         │
└────────────┬─────────────┘
             │
             ▼
┌──────────────────────────┐
│ Editor Dashboard         │
│                          │
│ Slate UI                 │
│ Charts / Tables          │
└──────────────────────────┘
```

各層を分離することで、

- Storageの変更
- Analyzerの追加
- UIの変更
- Source Control統合
- Remote Server対応

を行いやすい構造にします。

---

# Storage

初期バージョンではSQLiteを使用します。

Database：

```text
<Project>/
└── Saved/
    └── ProjectActivity/
        └── activity.db
```

初期Schema案：

```sql
CREATE TABLE Activity
(
    Id INTEGER PRIMARY KEY AUTOINCREMENT,
    EventId TEXT,
    UserAlias TEXT,
    AssetPath TEXT,
    AssetClass TEXT,
    Category TEXT,
    TimestampUtc TEXT,
    ActivityType INTEGER
);
```

将来的には以下の情報を追加する可能性があります。

```text
Source Control Revision
Commit / Changelist
Dependency Count
Session ID
Project ID
```

---

# Storage Abstraction

Storageは他のSystemから分離します。

```text
IProjectActivityStorage
        │
        ├── SQLiteActivityStorage
        ├── JsonActivityStorage
        └── HttpActivityStorage
```

初期実装：

```text
SQLiteActivityStorage
```

将来的には、

```text
JSON Export
Remote Database
REST API
Team Dashboard Server
```

などを追加できる設計を目指します。

---

# Planned Plugin Structure

```text
ProjectActivityVisualizer/
│
├── ProjectActivityVisualizer.uplugin
│
├── README.md
│
├── Resources/
│
└── Source/
    └── ProjectActivityVisualizer/
        │
        ├── ProjectActivityVisualizer.Build.cs
        │
        ├── Public/
        │   ├── ProjectActivitySubsystem.h
        │   ├── ProjectActivitySettings.h
        │   ├── ProjectActivityTypes.h
        │   ├── ProjectActivityStorage.h
        │   └── ProjectActivityAnalyzer.h
        │
        └── Private/
            ├── ProjectActivityVisualizer.cpp
            ├── ProjectActivitySubsystem.cpp
            ├── ProjectActivityStorage.cpp
            ├── ProjectActivityAnalyzer.cpp
            └── SProjectActivityDashboard.cpp
```

---

# Settings

設定イメージ：

```text
Project Activity

[✓] Enable Activity Collection

Contributor Name
[ Junseok ]

Tracked Paths
[ /Game/ ]

Ignored Paths
[ /Game/Developers/ ]
[ /Game/Test/ ]

Category Rules

/Game/Characters/ → Character
/Game/Combat/     → Combat
/Game/UI/         → UI
/Game/Maps/       → Level
```

将来的な設定：

```text
Enable Source Control Analysis
Enable Dependency Analysis
Enable Hotspot Warnings
Data Retention Period
Anonymous Contributor Mode
```

---

# Data Export

将来的には、

```text
JSON
CSV
```

形式でActivity DataをExportできるようにします。

JSON例：

```json
{
    "user": "Junseok",
    "asset": "/Game/Combat/GA_Attack",
    "assetType": "Blueprint",
    "category": "Combat",
    "timestamp": "2026-10-11T06:30:20Z",
    "activityType": "AssetSaved"
}
```

Export Dataは、

- External Visualization
- Team Report
- Data Analysis
- Research
- Remote Dashboard

などへの利用を想定しています。

---

# Team Data Synchronization

初期バージョンでは、

**Local-Only**

とします。

各Contributorは自分のLocal Databaseを保持します。

将来的には以下の共有方式を検討します。

### Source Control

確定した変更履歴を利用する方式。

将来的な主要な分析Sourceとして検討します。

### JSON / CSV Import & Export

MVPでも実装しやすい簡易的なTeam Data共有方式。

### Remote Server

将来的には以下のArchitectureも検討します。

```text
Unreal Plugin
      ↓
REST API
      ↓
Project Activity Server
      ↓
Shared Database
      ↓
Team Dashboard
```

Remote Syncは常にOptionalとします。

---

# Privacy

Project Activity Visualizerでは、

> **プロジェクト分析に必要な最小限の情報だけを収集する**

という方針を採用します。

基本方針：

- Data Collectionを無効化できる
- Contributor Aliasを使用できる
- 保存Dataをユーザーが確認できる
- Remote UploadはDefault OFF
- Keyboard Inputを収集しない
- Mouse Activityを収集しない
- Editor使用時間を収集しない
- Productivity Rankingを行わない
- Source Code本文を収集しない
- Blueprint Graph本文を収集しない

---

# Distribution

本Pluginは、

**特定のProjectに依存しない再利用可能なEditor Plugin**

として配布することを目標とします。

導入例：

```text
MyProject/
└── Plugins/
    └── ProjectActivityVisualizer/
```

Plugin Folderをコピー、またはRepositoryからCloneすることで、
別のUnreal Engine Projectでも使用できる構成を目指します。

将来的には、

```text
Source Distribution
Binary Distribution
```

の両方を検討します。

---

# Example Use Case

Dashboardで以下の結果が表示されたとします。

```text
Most Modified Assets

BP_Player        42
BP_EnemyBase     18
GA_Attack        15
WBP_HUD           9
```

さらに`BP_Player`を確認すると、

```text
BP_Player

Contributors      4
Recent Changes   42
Dependencies     23
```

となっていたとします。

Pluginは、

> `BP_Player` は設計上確認する価値があるAssetかもしれない

という情報を提示します。

チームはその情報を元に、

- BP_Playerに責務が集中していないか
- Actor Componentへ分離できないか
- 複数人が頻繁に同じBlueprintを変更していないか
- Ownershipを明確にする必要がないか
- System Separationが必要ではないか

といった議論を行えます。

Pluginが、

> 「この設計は悪い」

と自動的に判断することはありません。

**最終的な設計判断は開発チームが行います。**

---

# Development Roadmap

## v0.1 — Activity Tracking MVP

### Goal

別のUnreal Engine Projectへ導入して使用できる
最小構成を完成させる。

### Features

- Editor Plugin
- Project Settings
- Contributor Alias
- Asset Save Tracking
- SQLite Storage
- Most Modified Assets
- Most Modified Categories
- Last 7 Days Dashboard

---

## v0.2 — Configurable Project Analysis

- Category Rules
- Ignored Paths
- Tracked Paths
- Time Range Filter
- JSON Export
- CSV Export
- Dashboard Improvements

---

## v0.3 — Team Analysis

- Contributor Activity Distribution
- Multi-Contributor Assets
- Hot Asset Detection
- Hotspot Score
- Conflict Risk Indicator

---

## v0.4 — Source Control Integration

- Unreal Source Control Integration
- Commit / Changelist History
- Author Mapping
- Confirmed Change Analysis
- Editor Activity vs Source Control Activity

---

## v0.5 — Dependency Analysis

- Asset Registry Integration
- Asset Dependencies
- Dependency Count
- Critical Asset Detection
- Dependency-Aware Hotspot Score

---

## v1.0 — Public Release

Target：

- Reusable Unreal Editor Plugin
- Polished Slate Dashboard
- Configurable Analytics
- Source Distribution
- Binary Plugin Distribution
- Documentation
- Example Project

---

# MVP Definition

`Project Activity Visualizer v0.1` は、
以下を満たした時点で最初の完成版とします。

- [ ] 別のUnreal Engine Projectに導入できる
- [ ] Activity CollectionをON / OFFできる
- [ ] Contributor Aliasを設定できる
- [ ] Asset保存時にActivity Recordが生成される
- [ ] Activity DataがSQLiteへ保存される
- [ ] Most Modified Assetsを表示できる
- [ ] CategoryごとのActivityを表示できる
- [ ] Last 7 DaysのActivityを確認できる
- [ ] Project固有Codeを必要としない

これ以降の機能はExtensionとして追加します。

---

# Long-Term Vision

Project Activity Visualizerでは、

単純に、

```text
What changed?
```

だけではなく、

```text
Where are changes concentrating?

Which systems are becoming hotspots?

Which assets are modified by multiple contributors?

Which parts may need architectural attention?
```

という問いに答えられるツールを目指します。

最終目的は、

**より多くのDataを収集することではありません。**

プロジェクト構造やチーム開発について、

**より良い設計判断を行うための情報を提供すること**

が最終的な目標です。

---

# Current Status

**Planning / Prototype Stage**

Initial Target：

```text
Unreal Engine 5.8
Windows
Editor-Only Plugin
Source Distribution
```

First Implementation Milestone：

```text
Asset Save Event
        ↓
Activity Record
        ↓
SQLite
        ↓
Project Activity Dashboard
```

---

# License

TBD

Public Release前にLicenseを決定します。

Candidates：

- MIT License
- Apache License 2.0

---

# Author

**SEO JUNSEOK**

Game Developer / Unreal Engine Programmer
