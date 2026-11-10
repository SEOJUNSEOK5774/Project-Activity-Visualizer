# Project Activity Visualizer：使用例

[English](EXAMPLE.md) | [日本語](EXAMPLE.ja.md) | [한국어](EXAMPLE.ko.md)

[インストールと指標の説明](README.ja.md) | [0.1.0 の変更履歴](CHANGELOG.md)

このガイドでは、自分のプロジェクト内に小さな使用例を作成します。サンプルの .uproject、.uasset、活動 DB、Git リポジトリを同梱したものではありません。

パッケージの基本動作を確認した環境は Windows 64 ビット / Unreal Engine 5.8.4 です。既存の活動記録や独自の収集規則がある場合は、別のテストプロジェクトで試すと結果を確認しやすくなります。

## 1. プロジェクトの準備

1. README.ja.md の手順に従ってプラグインをインストールし、Edit > Plugins で有効にします。
2. Editor Preferences > Plugins > Project Activity で Contributor Alias を demo_a に設定します。この例では Enable Automatic Import を無効のままにします。
3. Project Settings > Plugins > Project Activity で活動の収集を有効にします。Tracked Paths に /Game/ActivityExample/ を含め、Ignored Paths がこのフォルダーを除外していないことを確認します。
4. Hotspot の例も試す場合は、既定のしきい値を使用します。Minimum Activity Buckets = 3、Minimum Named Aliases = 2、Minimum Referencers = 1 です。

## 2. テストアセットの作成と保存

1. コンテンツブラウザーで、Content の直下に ActivityExample フォルダーを作成します。
2. その中に Actor ブループリントを作り、BP_ActivityExample と名付けます。
3. Integer 変数 TestValue を追加します。
4. Compile 後に TestValue の既定値を 1 に設定し、再度 Compile して保存します。
5. Window > ProjectActivityVisualizer を開き、All time を選んで Refresh をクリックします。

Activity ページで /Game/ActivityExample/BP_ActivityExample を確認します。このアセットについて Raw saves と活動区間がそれぞれ 1 件以上記録されます。作成中の保存回数により初期値は異なる場合があります。

## 3. 保存回数と活動区間の比較

TestValue を 2 に変更して Compile・保存し、続いて 3 に変更して再度 Compile・保存します。ダッシュボードで Refresh をクリックします。

収集処理が保存イベントを記録すると Raw saves が増えます。同じパッケージ・同じ別名の保存は、固定された UTC の 5 分間隔内では一つの活動区間にまとまります。区間の境界を越えると、前回の保存から 5 分未満でも新しい活動区間になります。未変更のアセットでは新しいイベントが発生しない場合があるため、毎回実際の値を変更してから保存してください。

日別グラフは UTC 日付を使用します。カテゴリー規則がない新規プロジェクトでは、通常 Other に集計されます。

## 4. 別名と Hotspot 候補の確認

1. Contributor Alias を demo_b に変更します。TestValue を 4 に変更して Compile・保存し、ダッシュボードを更新します。
2. Collaboration / Hotspot を開きます。複数の別名が記録されたパッケージ一覧でアセットを探し、Show Aliases で demo_a と demo_b を確認します。
3. Contributor Alias を demo_c に変更します。TestValue を 5 に変更して Compile・保存し、もう一度更新します。

活動区間の識別には別名も含まれるため、活動区間は最低 3 件、名前付き別名は 3 種類になります。既定のしきい値では、活動区間の上位 10 パッケージに含まれれば複数別名の Hotspot 条件を満たします。新規の例示用プロジェクトでは見つけやすいはずです。

これは一人のテスターが別名を変えて作成した記録です。実際の参加者 3 人や同時編集を示しません。現在の別名を変更しても過去の記録は書き換わりません。終了後は使用する別名に戻してください。

## 5. JSON 交換の確認

1. Export all JSON をクリックし、プロジェクトの Saved/ProjectActivity/Exports フォルダーで出力ファイルを確認します。
2. 同じプロジェクトで Import JSON を使い、そのファイルを取り込みます。
3. イベント ID は既に存在するためスキップされ、記録の総数は増えません。

活動を交換するときは、同じプロジェクト名のデータを使用してください。活動 JSON に保存記録は含まれますが、Git 履歴のキャッシュは含まれません。実行中のエディター間で使用中の DB を直接コピーしないでください。

## 6. 任意：Git 履歴とキャッシュ

Git のインストール、プロジェクトを含むローカルリポジトリ、Unreal Editor での Git Provider 接続、および BP_ActivityExample.uasset を含むコミットが 1 件以上必要です。Git の設定は活動の収集とは別に準備します。

1. Activity ページで BP_ActivityExample の横にある Show History をクリックします。
2. Commit history でパッケージの状態、コミット作成者、メッセージを確認します。取得に成功すると Local copy updated が表示されます。
3. エディターを再起動し、Refresh や Show History をクリックする前に Commit history を開きます。元の取得時刻を保持した CACHED RESULT が表示されます。
4. キャッシュへの切り替えを試す場合は、Revision Control Provider を一時的に None に設定してプラグインの Refresh をクリックします。利用不可の案内と保存済みの結果が表示されます。
5. Git に再接続して更新します。取得に成功するとローカルコピーと取得時刻が更新されます。

Git の作成者と Contributor Alias は別の情報です。既知の Invalid path '/Temp' ソースコントロールログ問題は README.ja.md に記載しており、このプラグインを無効にしても再現します。

## この例で確認する内容

- 保存イベント、固定された 5 分活動区間、ダッシュボードの更新
- 記録された別名と、設定可能な Hotspot 条件の一つ
- JSON 取り込み時の重複防止
- 任意で Git 履歴、再起動後のキャッシュ復元、Provider 無効時のキャッシュ表示

実作業時間の計測や未保存の編集検知は行いません。リアルタイム Web ダッシュボードは提供しません。
