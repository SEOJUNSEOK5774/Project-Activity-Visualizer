# Project Activity Visualizer web dashboard

The dashboard runs locally on Windows with Python 3.9 or newer. It uses the standard library.

## Run the demo

1. Extract the ZIP and double-click `Start-Demo.cmd`. Leave the command window open.
2. Open the [sample dashboard](http://127.0.0.1:8766/). **Sample data** appears in the header.
3. In **Overview**, hover over the daily chart or tap a date to see its count. With keyboard focus on the chart, use the left and right arrow keys. **View data** opens the table.
4. In **Assets**, expand a folder to see its assets, ranked by activity buckets. Each folder displays 20 assets at a time. Select **Show more** for the next group.
5. In **Collaboration**, select `BP_DemoDoor`. The dependency graph points from `BP_DemoDoor` to `M_DemoMetal`. Change the header range from **7 days** to **30 days** to see the save count increase from 4 to 5.
6. Open **Save History** for the ten most recent saves. This page shows the latest saves across all dates.
7. Open **Edit History** to see when an asset first gained unsaved changes. The demo includes a draft asset with an edit and no save.
8. Select an asset in **Edit History** to see its **Edit and save timeline**. The timeline shows up to ten edits and ten saves, newest first. **Edit detected** marks when Unreal first observed unsaved changes. **Saved** marks when Unreal saved the package.

The demo creates a temporary database based on today's UTC date. Press `Ctrl+C` in the command window to stop it.

## Connect an Unreal project

Drag your project's `.uproject` file onto `Start-Local.cmd`. The launcher finds `Saved\ProjectActivity\activity.db` inside that project. You can also drag the database file directly.

Keep the command window open while using the dashboard. To switch projects, press `Ctrl+C` in that window before running the launcher again.

Save records from older databases without contributor aliases appear as **Unspecified**.

1. Enable the **Project Activity Visualizer** plugin in Unreal and save an asset. The plugin records activity in `Saved/ProjectActivity/activity.db` under the project folder.
2. Open PowerShell in the extracted folder and run the following command with the full path to your database.

   ```powershell
   py -3 -B .\activity_api.py --db "E:\MyUnrealProject\Saved\ProjectActivity\activity.db"
   ```

3. Open the [local dashboard](http://127.0.0.1:8765/). New saves normally appear within a few seconds. If Windows has `python` but no `py` launcher, replace `py -3` with `python`.
4. Open **Edit History** and modify a saved asset in Unreal. With a plugin build that records edits, an entry appears within a few seconds of the asset first gaining unsaved changes. **Edit History** and **Save History** each show the latest ten events across all dates. Edit history uses the local project database.
5. Select an asset in **Edit History** or **Save History** to see its timeline across all dates. Asset details opened from **Overview**, **Assets**, or **Collaboration** follow the selected 7-day or 30-day range.

The header shows the project folder and the status of the Unreal reference script. **Connected** means the editor is ready to receive asset-open requests. **Offline** appears within about 10 seconds of closing the editor. **Live** indicates the dashboard server connection.

## Refresh asset references automatically

1. Enable **Python Editor Script Plugin** under **Edit → Plugins**. Restart Unreal, then open **Edit → Project Settings → Plugins → Python → Startup Scripts**.
2. Add the full path to `auto_export_asset_references.py` from the extracted folder. Replace the old path if you registered an earlier version.
3. Restart Unreal. The editor writes reference information to `Saved/ProjectActivity/asset_references.json` about every 15 seconds.
4. Check the reference refresh time under **Collaboration → Hotspot candidates**. Selecting an asset opens its direct `/Game/` dependencies from the saved package data.

For an immediate refresh, switch the Unreal **Output Log** input to **Cmd** and run `py "C:/path/to/export_asset_references.py"` with the script's full path from this folder. To start automatic updates in the current editor session, run `auto_export_asset_references.py` the same way. Automatic export covers the last 30 days of save and edit activity and the assets visible in **Save History** and **Edit History**, even when those entries are older.

## Open an asset from the reference graph

1. Start the local dashboard and open the same project in Unreal Editor on the same PC.
2. Run the updated `auto_export_asset_references.py` once in the current editor session, or restart Unreal if the updated script is already registered in **Startup Scripts**. The script starts both reference updates and asset opening.
3. In **Overview**, click **View references** on the latest save, or select an asset in **Save History**. Click either end of an arrow in the graph to open that asset in Unreal. The dashboard shows the editor's response beneath the graph.

## Share team activity

1. Create a folder teammates can access. In Unreal on each PC, set its absolute path under **Editor Preferences → Plugins → Project Activity → Team Exchange Folder**.
2. Open PowerShell on the dashboard PC and start the team server. Set `--project` to the name of your `.uproject` file without the extension.

   ```powershell
   py -3 -B .\activity_api.py --team-folder "E:\ActivityTeamExchange" --project "AVPackageTest" --team-db ".\AVPackageTest-team.db" --port 8767
   ```

3. Each teammate sets a **Contributor Alias** in Unreal and uses **Export to team folder** after saving assets. The dashboard checks the folder about every five seconds. Open the [team dashboard](http://127.0.0.1:8767/) on the server PC.
4. Export twice without saving a second time. The second import should report `0 imported`, and the dashboard count should stay the same.

The team dashboard also imports earlier JSON exports from the folder. Team hotspot criteria use activity buckets and named aliases. Follow the [team HTTPS guide](TEAM-HTTPS.md) to give teammates browser access from their own PCs.

## Troubleshooting

- Open the address printed on the command window's `Dashboard:` line in Chrome, Edge, or your default Windows browser. Keep the command window open.
- If an older demo is still using port `8766`, stop it with `Ctrl+C` before starting the new one. To use another port, run `py -3 -B .\activity_api.py --demo --port 8767`.
- If the dashboard is empty, check the database path and selected UTC date range.
