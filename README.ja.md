# Project Activity Visualizer

[English](README.md) | [日本語](README.ja.md) | [한국어](README.ko.md)

アセットの保存と編集を検知し、コントリビューターの別名、パッケージの参照関係、Hotspot 確認候補、Git 履歴を表示する Unreal Editor 用プラグインです。ローカル Web ダッシュボードでは編集と保存を一つのタイムラインで確認できます。

**バージョン** 0.2.0

**パッケージの検証環境** Windows x64 / Unreal Engine 5.8.3

## インストール

- [UE 5.8.3 / Win64 用の 0.2.0 ZIP をダウンロード](https://github.com/SEOJUNSEOK5774/Project-Activity-Visualizer/releases/download/v0.2.0/ProjectActivityVisualizer-0.2.0-UE5.8.3-Win64.zip)。展開した `ProjectActivityVisualizer` フォルダーをプロジェクトの `Plugins` フォルダーにコピーします。
- **ソース：** このリポジトリの [ProjectActivityVisualizer](ProjectActivityVisualizer) フォルダーをプロジェクトの Plugins フォルダーにコピーします。必要に応じて、互換性のある Unreal Engine C++ ビルド環境でビルドしてください。GitHub が自動生成するソース ZIP にビルド済みバイナリは含まれません。
- Edit > Plugins で有効にし、Window > ProjectActivityVisualizer を開きます。
- Windows に Python 3.9 以降をインストールし、追跡対象のアセットを保存します。プロジェクトの `.uproject` を `WebDashboard/Start-Local.cmd` にドラッグすると Web ダッシュボードが開きます。

## ドキュメント

- [インストール・設定・指標とパッケージ検証結果](ProjectActivityVisualizer/README.ja.md)
- [使用例](ProjectActivityVisualizer/EXAMPLE.ja.md)
- [Web ダッシュボードの設定](WebDashboard/README.md)
- [変更履歴](ProjectActivityVisualizer/CHANGELOG.md)
- [0.2.0 のリリースノートと ZIP チェックサム](docs/RELEASE_NOTES_0.2.0.md)
- [当初の企画書](docs/PROJECT_PLAN.md) — 設計案として保存しています。実装済みの動作については最新の使用ドキュメントを参照してください。

## 現在の実装範囲

保存イベントをローカルの SQLite に記録し、繰り返しの保存を固定された UTC の 5 分活動区間にまとめます。JSON 交換と、永続キャッシュ付きの Git 履歴取得に対応しています。Hotspot 候補には該当した条件を表示し、活動件数を作業時間や生産性として扱いません。

別のブループリントプロジェクトで、ドキュメントに記載したパッケージ検証項目に合格しました。Git の Invalid path '/Temp' エラーはこのプラグインを無効にしても再現し、具体的な原因と解決方法は未確認です。検証範囲と既知の問題は詳細ドキュメントを参照してください。

Web ダッシュボードはアセットが最初に未保存状態になった時刻と、その後の保存時刻を個別に表示します。Python 起動スクリプトを設定すると、直接参照するアセットをグラフから Unreal Editor で開けます。ライセンスは当初の企画書に記載したとおり未定です。
