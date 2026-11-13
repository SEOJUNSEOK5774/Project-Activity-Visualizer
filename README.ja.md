# Project Activity Visualizer

[English](README.md) | [日本語](README.ja.md) | [한국어](README.ko.md)

アセットの保存活動、コントリビューターの別名、現在のパッケージ参照、Hotspot 確認候補、Git 履歴を表示する Unreal Editor 用プラグインです。

**バージョン：** 0.1.0  
**パッケージの検証環境：** Windows 64 ビット / Unreal Engine 5.8.4

## インストール

- **ビルド済みパッケージ：** [UE 5.8.4 / Win64 用の 0.1.0 ZIP をダウンロード](https://github.com/SEOJUNSEOK5774/Project-Activity-Visualizer/releases/download/v0.1.0/ProjectActivityVisualizer_0.1.0_UE5.8.4_Win64.zip)。ZIP 内の ProjectActivityVisualizer フォルダーをプロジェクトの Plugins フォルダーにコピーします。
- **ソース：** このリポジトリの [ProjectActivityVisualizer](ProjectActivityVisualizer) フォルダーをプロジェクトの Plugins フォルダーにコピーします。必要に応じて、互換性のある Unreal Engine C++ ビルド環境でビルドしてください。GitHub が自動生成するソース ZIP にビルド済みバイナリは含まれません。
- Edit > Plugins で有効にし、Window > ProjectActivityVisualizer を開きます。

## ドキュメント

- [インストール・設定・指標とパッケージ検証結果](ProjectActivityVisualizer/README.ja.md)
- [使用例](ProjectActivityVisualizer/EXAMPLE.ja.md)
- [0.1.0 の変更履歴](ProjectActivityVisualizer/CHANGELOG.md)
- [リリース説明の原稿と ZIP チェックサム](docs/RELEASE_NOTES_0.1.0.md)
- [当初の企画書](docs/PROJECT_PLAN.md) — 設計案として保存しています。実装済みの動作については最新の使用ドキュメントを参照してください。

## 現在の実装範囲

保存イベントをローカルの SQLite に記録し、繰り返しの保存を固定された UTC の 5 分活動区間にまとめます。JSON 交換と、永続キャッシュ付きの Git 履歴取得に対応しています。Hotspot 候補には該当した条件を表示し、活動件数を作業時間や生産性として扱いません。

別のブループリントプロジェクトで、ドキュメントに記載したパッケージ検証項目に合格しました。Git の Invalid path '/Temp' エラーはこのプラグインを無効にしても再現し、具体的な原因と解決方法は未確認です。検証範囲と既知の問題は詳細ドキュメントを参照してください。

次の開発対象はリアルタイムのチーム Web ダッシュボードで、その後に編集検知を予定しています。どちらも 0.1.0 には含まれません。ライセンスは当初の企画書に記載したとおり未定です。
