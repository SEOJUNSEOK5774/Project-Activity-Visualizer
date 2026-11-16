import argparse
import json
import sqlite3
import webbrowser
from contextlib import ExitStack, closing
from datetime import datetime, timedelta, timezone
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from tempfile import TemporaryDirectory
from urllib.parse import parse_qs, urlsplit


WEB_ROOT = Path(__file__).absolute().parent / "dist"
PAGE_ROUTES = {"/", "/assets", "/collaboration", "/history", "/edits"}
WEB_FILES = {
    **dict.fromkeys(PAGE_ROUTES, ("index.html", "text/html; charset=utf-8")),
    "/dashboard.css": ("dashboard.css", "text/css; charset=utf-8"),
    "/dashboard.js": ("dashboard.js", "text/javascript; charset=utf-8"),
    "/dashboard-motion.js": ("dashboard-motion.js", "text/javascript; charset=utf-8"),
    "/daily-chart.js": ("daily-chart.js", "text/javascript; charset=utf-8"),
    "/crow.svg": ("crow.svg", "image/svg+xml"),
}

ACTIVITY_COLUMNS_SQL = """
    SELECT EventId AS eventId,
           PackagePath AS packagePath,
           TimestampUtc AS timestampUtc,
           ActivityType AS activityType,
           ContributorAlias AS contributorAlias
    FROM Activity
"""

RECENT_ROWS_SQL = """
    ORDER BY TimestampUtc DESC, EventId DESC
    LIMIT ?
"""

RECENT_ACTIVITY_SQL = ACTIVITY_COLUMNS_SQL + RECENT_ROWS_SQL

EDIT_COLUMNS_SQL = """
    SELECT EventId AS eventId,
           PackagePath AS packagePath,
           TimestampUtc AS timestampUtc,
           ContributorAlias AS contributorAlias
    FROM PackageEdit
"""

TIMESTAMP_FILTER_SQL = """
      AND TimestampUtc >= ?
      AND TimestampUtc <= ?
"""

SAVE_WINDOW_SQL = " WHERE ActivityType = ? " + TIMESTAMP_FILTER_SQL

ASSET_ALIAS_COUNTS_SQL = """
    SELECT COALESCE(TRIM(ContributorAlias), '') AS contributorAlias,
           COUNT(*) AS saveCount
    FROM Activity
    {filters}
    GROUP BY COALESCE(TRIM(ContributorAlias), '')
    ORDER BY saveCount DESC, contributorAlias ASC
"""

ACTIVITY_LIMITS = {"/api/activity/latest": 1, "/api/activity/recent": 10}
DASHBOARD_PATH = "/api/activity/dashboard"
ASSET_HISTORY_PATH = "/api/activity/asset"
OPEN_ASSET_PATH = "/api/editor/open-asset"
DEFAULT_RANGE_DAYS = 7
ALLOWED_RANGE_DAYS = {"7": 7, "30": 30}
PACKAGE_SAVED = 0
TOP_ASSET_LIMIT = 5
MULTI_ALIAS_ASSET_LIMIT = 5
ASSET_HISTORY_LIMIT = 10
EDIT_HISTORY_LIMIT = 10
HOTSPOT_SAMPLE_LIMIT = 10
HOTSPOT_MIN_ACTIVITY_BUCKETS = 3
HOTSPOT_MIN_NAMED_ALIASES = 2
HOTSPOT_MIN_REFERENCERS = 1
REFERENCE_SNAPSHOT_NAME = "asset_references.json"

BUCKET_STATS_SQL = """
    WITH BucketStats AS (
        SELECT substr(TimestampUtc, 1, 10) AS DayUtc,
               PackagePath,
               TRIM(ContributorAlias) AS Alias,
               CAST(strftime('%s', TimestampUtc) AS INTEGER) / 300 AS BucketId,
               COUNT(*) AS RawSaveCount
        FROM Activity
""" + SAVE_WINDOW_SQL + """
        GROUP BY DayUtc, PackagePath, Alias, BucketId
    )
"""

DAILY_ACTIVITY_SQL = BUCKET_STATS_SQL + """
    SELECT DayUtc AS dateUtc,
           SUM(RawSaveCount) AS rawSaveCount,
           COUNT(*) AS activityBucketCount
    FROM BucketStats
    GROUP BY DayUtc
"""

RANKED_ASSETS_SQL = BUCKET_STATS_SQL + """
    SELECT PackagePath AS packagePath,
           SUM(RawSaveCount) AS rawSaveCount,
           COUNT(*) AS activityBucketCount,
           COUNT(DISTINCT NULLIF(Alias, '')) AS aliasCount
    FROM BucketStats
    GROUP BY PackagePath
    ORDER BY activityBucketCount DESC, rawSaveCount DESC, PackagePath ASC
    LIMIT ?
"""

MULTI_ALIAS_ASSETS_SQL = """
    SELECT PackagePath AS packagePath,
           COUNT(DISTINCT NULLIF(TRIM(ContributorAlias), '')) AS aliasCount,
           COUNT(*) AS rawSaveCount
    FROM Activity
""" + SAVE_WINDOW_SQL + """
    GROUP BY PackagePath
    HAVING aliasCount >= 2
    ORDER BY aliasCount DESC, rawSaveCount DESC, PackagePath ASC
    LIMIT ?
"""


def open_activity_database(database_path, timeout=1.0):
    database_uri = database_path.as_uri() + "?mode=ro"
    db = sqlite3.connect(database_uri, uri=True, timeout=timeout)
    db.row_factory = sqlite3.Row
    try:
        with closing(db.execute("PRAGMA main.table_info(Activity)")) as cursor:
            columns = {row["name"].casefold() for row in cursor}
        if columns and "contributoralias" not in columns:
            # Keep legacy save records readable through the same queries as current databases.
            db.execute("""
                CREATE TEMP VIEW Activity AS
                SELECT EventId, PackagePath, TimestampUtc, ActivityType, '' AS ContributorAlias
                FROM main.Activity
            """).close()
    except sqlite3.Error:
        db.close()
        raise
    return db


def query_recent_activity(db, limit):
    with closing(db.execute(RECENT_ACTIVITY_SQL, (limit,))) as cursor:
        return [dict(row) for row in cursor]


def read_recent_activity(database_path, limit):
    with closing(open_activity_database(database_path)) as db:
        return query_recent_activity(db, limit)


def has_package_edit_table(db):
    with closing(db.execute(
        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'PackageEdit'"
    )) as cursor:
        return cursor.fetchone() is not None


def query_edit_history(db, team_project, *, package_path=None, time_bounds=()):
    if team_project is not None:
        return {"status": "team", "edits": []}

    if not has_package_edit_table(db):
        return {"status": "unavailable", "edits": []}

    query = EDIT_COLUMNS_SQL
    parameters = ()
    if package_path is not None:
        query += " WHERE PackagePath = ?"
        parameters = (package_path,)
        if time_bounds:
            query += TIMESTAMP_FILTER_SQL
            parameters += time_bounds

    with closing(db.execute(query + RECENT_ROWS_SQL, (*parameters, EDIT_HISTORY_LIMIT))) as cursor:
        return {"status": "ready", "edits": [dict(row) for row in cursor]}


def query_daily_activity(db, first_day, window_parameters, range_days):
    with closing(db.execute(DAILY_ACTIVITY_SQL, window_parameters)) as cursor:
        counts_by_day = {row["dateUtc"]: dict(row) for row in cursor}

    daily = []
    for offset in range(range_days):
        date_utc = (first_day + timedelta(days=offset)).date().isoformat()
        daily.append(counts_by_day.get(date_utc, {
            "dateUtc": date_utc,
            "rawSaveCount": 0,
            "activityBucketCount": 0,
        }))
    return daily


def activity_window(range_days, now_utc=None):
    if now_utc is None:
        now_utc = datetime.now(timezone.utc)
    today = now_utc.replace(hour=0, minute=0, second=0, microsecond=0)
    first_day = today - timedelta(days=range_days - 1)
    return first_day, (
        PACKAGE_SAVED,
        first_day.isoformat(timespec="milliseconds").replace("+00:00", "Z"),
        now_utc.isoformat(timespec="milliseconds").replace("+00:00", "Z"),
    )


def query_ranked_assets(db, window_parameters, limit=HOTSPOT_SAMPLE_LIMIT):
    with closing(db.execute(RANKED_ASSETS_SQL, (*window_parameters, limit))) as cursor:
        return [dict(row) for row in cursor]


def read_reference_targets(database_path):
    _, parameters = activity_window(max(ALLOWED_RANGE_DAYS.values()))
    query = "SELECT DISTINCT PackagePath FROM Activity" + SAVE_WINDOW_SQL
    with closing(open_activity_database(database_path, timeout=0.0)) as db:
        db.execute("BEGIN")
        has_edits = has_package_edit_table(db)
        if has_edits:
            query += " UNION SELECT PackagePath FROM PackageEdit WHERE TimestampUtc BETWEEN ? AND ?"
            parameters = (*parameters, *parameters[1:])
        with closing(db.execute(query, parameters)) as cursor:
            packages = {row[0] for row in cursor}
        packages.update(save["packagePath"] for save in query_recent_activity(db, ACTIVITY_LIMITS["/api/activity/recent"]))
        if has_edits:
            with closing(db.execute(EDIT_COLUMNS_SQL + RECENT_ROWS_SQL, (EDIT_HISTORY_LIMIT,))) as cursor:
                packages.update(row["packagePath"] for row in cursor)
    return sorted(path for path in packages if path.casefold().startswith("/game/"))


def parse_reference_paths(packages):
    paths_by_package = {}
    for package, paths in packages.items():
        key = package.casefold()
        if not key.startswith("/game/") or key in paths_by_package or not isinstance(paths, list):
            raise ValueError("Invalid package reference list")
        normalized = set()
        for path in paths:
            path_key = path.casefold()
            if not path_key.startswith("/game/") or path_key == key or path_key in normalized:
                raise ValueError("Invalid reference path")
            normalized.add(path_key)
        paths_by_package[key] = paths
    return paths_by_package


def read_reference_snapshot(database_path):
    snapshot_path = database_path.with_name(REFERENCE_SNAPSHOT_NAME)
    try:
        snapshot = json.loads(snapshot_path.read_text(encoding="utf-8"))
        if type(snapshot["schemaVersion"]) is not int or snapshot["schemaVersion"] != 1:
            raise ValueError("Unknown reference snapshot format")
        captured_at = snapshot["capturedAtUtc"]
        if not captured_at.endswith("Z"):
            raise ValueError("Expected a UTC snapshot date")
        captured_time = datetime.fromisoformat(captured_at[:-1] + "+00:00")
        if captured_time.utcoffset() != timedelta(0):
            raise ValueError("Expected a UTC snapshot date")
        counts = {}
        for package, count in snapshot["referencerCounts"].items():
            key = package.casefold()
            if not key.startswith("/game/") or key in counts or type(count) is not int or count < 0:
                raise ValueError("Invalid package referencer count")
            counts[key] = count
        referencer_paths = parse_reference_paths(snapshot.get("referencerPaths", {}))
        if any(counts.get(key) != len(paths) for key, paths in referencer_paths.items()):
            raise ValueError("Invalid package referencer count")
    except FileNotFoundError:
        return {"status": "missing"}, {}, {}, {}
    except (OSError, ValueError, TypeError, KeyError, AttributeError):
        return {"status": "unavailable"}, {}, {}, {}
    try:
        dependency_paths = parse_reference_paths(snapshot.get("dependencyPaths", {}))
    except (ValueError, TypeError, AttributeError):
        dependency_paths = {}
    metadata = {"status": "ready", "capturedAtUtc": captured_at}
    if snapshot.get("automatic") is True:
        metadata["automatic"] = True
        metadata["stale"] = datetime.now(timezone.utc) - captured_time >= timedelta(minutes=1)
    return metadata, counts, referencer_paths, dependency_paths


def group_assets_by_folder(ranked_assets):
    folders = {}
    for asset in ranked_assets:
        folder = asset["packagePath"].rpartition("/")[0] or "/"
        folders.setdefault(folder, []).append({
            "packagePath": asset["packagePath"],
            "activityBucketCount": asset["activityBucketCount"],
            "rawSaveCount": asset["rawSaveCount"],
        })
    return [{"folderPath": folder, "assets": assets} for folder, assets in folders.items()]


def read_dashboard(database_path, range_days=DEFAULT_RANGE_DAYS, *, team_project=None, include_folders=False):
    first_day, window_parameters = activity_window(range_days)
    with closing(open_activity_database(database_path)) as db:
        # All dashboard queries observe the same committed records.
        db.execute("BEGIN")
        activities = query_recent_activity(db, ACTIVITY_LIMITS["/api/activity/recent"])
        edit_history = query_edit_history(db, team_project)
        daily = query_daily_activity(db, first_day, window_parameters, range_days)
        # SQLite uses a negative LIMIT to return all matching rows.
        ranked_assets = query_ranked_assets(db, window_parameters, -1 if include_folders else HOTSPOT_SAMPLE_LIMIT)
        with closing(db.execute(MULTI_ALIAS_ASSETS_SQL, (*window_parameters, MULTI_ALIAS_ASSET_LIMIT))) as cursor:
            multi_alias_assets = [dict(row) for row in cursor]

    if team_project is None:
        reference_snapshot, referencer_counts, _, _ = read_reference_snapshot(database_path)
    else:
        reference_snapshot, referencer_counts = {"status": "team"}, {}
    hotspot_candidates = []
    for asset in ranked_assets[:HOTSPOT_SAMPLE_LIMIT]:
        if asset["activityBucketCount"] < HOTSPOT_MIN_ACTIVITY_BUCKETS:
            continue
        referencer_count = referencer_counts.get(asset["packagePath"].casefold())
        has_referencers = referencer_count is not None and referencer_count >= HOTSPOT_MIN_REFERENCERS
        if asset["aliasCount"] >= HOTSPOT_MIN_NAMED_ALIASES or has_referencers:
            hotspot_candidates.append({**asset, "referencerCount": referencer_count})

    payload = {
        "rangeDays": range_days,
        "activities": activities,
        "editHistory": edit_history,
        "daily": daily,
        "topAssets": [
            {
                "packagePath": asset["packagePath"],
                "rawSaveCount": asset["rawSaveCount"],
                "activityBucketCount": asset["activityBucketCount"],
            }
            for asset in ranked_assets[:TOP_ASSET_LIMIT]
        ],
        "multiAliasAssets": multi_alias_assets,
        "hotspots": {
            "sampleLimit": HOTSPOT_SAMPLE_LIMIT,
            "minActivityBuckets": HOTSPOT_MIN_ACTIVITY_BUCKETS,
            "minNamedAliases": HOTSPOT_MIN_NAMED_ALIASES,
            "minReferencers": HOTSPOT_MIN_REFERENCERS,
            "referenceSnapshot": reference_snapshot,
            "candidates": hotspot_candidates,
        },
    }
    if include_folders:
        payload["assetFolders"] = group_assets_by_folder(ranked_assets)
    return payload


def read_asset_detail(database_path, package_path, range_days, *, team_project=None):
    filters = " WHERE ActivityType = ? AND PackagePath = ?"
    time_bounds = ()
    if range_days != "all":
        _, (_, start, end) = activity_window(range_days)
        filters += TIMESTAMP_FILTER_SQL
        time_bounds = (start, end)
    parameters = (PACKAGE_SAVED, package_path, *time_bounds)
    with closing(open_activity_database(database_path)) as db:
        db.execute("BEGIN")
        with closing(db.execute(
            ACTIVITY_COLUMNS_SQL + filters + RECENT_ROWS_SQL, (*parameters, ASSET_HISTORY_LIMIT)
        )) as cursor:
            activities = [dict(row) for row in cursor]
        with closing(db.execute(ASSET_ALIAS_COUNTS_SQL.format(filters=filters), parameters)) as cursor:
            alias_counts = [dict(row) for row in cursor]
        edit_history = query_edit_history(db, team_project, package_path=package_path, time_bounds=time_bounds)

    references = None
    dependencies = None
    reference_snapshot = {"status": "team"}
    if team_project is None:
        reference_snapshot, referencer_counts, referencer_paths, dependency_paths = read_reference_snapshot(database_path)
        package_key = package_path.casefold()
        references = {
            "count": referencer_counts.get(package_key),
            "paths": referencer_paths.get(package_key),
        }
        dependencies = dependency_paths.get(package_key)
    return {
        "packagePath": package_path,
        "rangeDays": range_days,
        "activities": activities,
        "editHistory": edit_history,
        "aliasCounts": alias_counts,
        "references": references,
        "referenceSnapshot": reference_snapshot,
        "dependencies": dependencies,
    }


def parse_range_days(values):
    if len(values) != 1 or values[0] not in ALLOWED_RANGE_DAYS:
        raise ValueError("Invalid dashboard period")
    return ALLOWED_RANGE_DAYS[values[0]]


def parse_activity_filters(query, allowed):
    if not query:
        return {}
    options = parse_qs(query, keep_blank_values=True, strict_parsing=True, max_num_fields=len(allowed))
    if not options.keys() <= allowed:
        raise ValueError("Unexpected query parameter")
    return options


def read_local_project(database_path):
    from asset_open_bridge import editor_is_connected

    storage = database_path.parent
    saved = storage.parent
    is_project_storage = saved.name.casefold() == "saved" and storage.name.casefold() == "projectactivity"
    project_path = str(saved.parent) if is_project_storage else None
    try:
        connected = editor_is_connected(storage)
    except OSError:
        connected = None
    return {
        "projectPath": project_path,
        "databasePath": str(database_path),
        "editorConnected": connected,
    }


class ActivityServer(ThreadingHTTPServer):
    def __init__(self, database_path, port, demo=False, team_project=None):
        self.database_path = database_path
        self.demo = demo
        self.team_project = team_project
        super().__init__(("127.0.0.1", port), ActivityHandler)


class ActivityHandler(BaseHTTPRequestHandler):
    server: ActivityServer

    def valid_host(self):
        return self.headers.get("Host") in (
            f"127.0.0.1:{self.server.server_port}",
            f"localhost:{self.server.server_port}",
        )

    def do_POST(self):
        from asset_open_bridge import request_asset_open, valid_package_path

        if self.path != OPEN_ASSET_PATH:
            self.send_json(HTTPStatus.NOT_FOUND, {"error": "Unknown endpoint."})
            return
        if not self.valid_host() or self.headers.get("Origin") != f'http://{self.headers.get("Host")}':
            self.send_json(HTTPStatus.FORBIDDEN, {"status": "unavailable"})
            return
        if self.server.demo or self.server.team_project is not None:
            self.send_json(HTTPStatus.FORBIDDEN, {"status": "unavailable"})
            return
        if self.headers.get_content_type() != "application/json":
            self.send_json(HTTPStatus.UNSUPPORTED_MEDIA_TYPE, {"status": "invalid"})
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 8192:
                raise ValueError("Invalid request length")
            payload = json.loads(self.rfile.read(length).decode("utf-8"))
            package_path = payload.get("packagePath") if isinstance(payload, dict) else None
            if not valid_package_path(package_path):
                raise ValueError("Invalid package path")
        except ValueError:
            self.send_json(HTTPStatus.BAD_REQUEST, {"status": "invalid"})
            return

        try:
            status = request_asset_open(self.server.database_path.parent, package_path)
        except OSError as error:
            self.log_error("Asset open request failed: %s", error)
            status = "failed"
        response_codes = {
            "opened": HTTPStatus.OK,
            "offline": HTTPStatus.SERVICE_UNAVAILABLE,
            "not_found": HTTPStatus.NOT_FOUND,
            "timeout": HTTPStatus.GATEWAY_TIMEOUT,
            "failed": HTTPStatus.INTERNAL_SERVER_ERROR,
        }
        self.send_json(response_codes[status], {"status": status})

    def do_GET(self):
        if not self.valid_host():
            self.send_json(HTTPStatus.BAD_REQUEST, {"error": "Invalid Host header."})
            return

        try:
            target = urlsplit(self.path)
        except ValueError:
            self.send_json(HTTPStatus.BAD_REQUEST, {"error": "Invalid request URL."})
            return

        path = target.path
        if path in WEB_FILES:
            file_name, content_type = WEB_FILES[path]
            try:
                body = (WEB_ROOT / file_name).read_bytes()
            except OSError as error:
                self.log_error("Dashboard file unavailable: %s", error)
                self.send_json(
                    HTTPStatus.INTERNAL_SERVER_ERROR,
                    {"error": "A dashboard file could not be read."},
                )
                return
            self.send_body(HTTPStatus.OK, content_type, body)
            return

        limit = ACTIVITY_LIMITS.get(path)
        if limit is None and path not in (DASHBOARD_PATH, ASSET_HISTORY_PATH):
            self.send_json(HTTPStatus.NOT_FOUND, {"error": "Unknown endpoint."})
            return

        if path in (DASHBOARD_PATH, ASSET_HISTORY_PATH):
            try:
                allowed = {"days", "package"} if path == ASSET_HISTORY_PATH else {"days", "folders"}
                options = parse_activity_filters(target.query, allowed)
                days = options.get("days", [str(DEFAULT_RANGE_DAYS)])
                range_days = "all" if path == ASSET_HISTORY_PATH and days == ["all"] else parse_range_days(days)
                if "folders" in options and options["folders"] != ["1"]:
                    raise ValueError("Invalid folders option")
                if path == ASSET_HISTORY_PATH:
                    packages = options.get("package", [])
                    if len(packages) != 1 or not packages[0].strip() or "\0" in packages[0]:
                        raise ValueError("Invalid package path")
                    package_path = packages[0]
            except ValueError:
                message = (
                    "Supply one package path and use days=7, days=30, or days=all."
                    if path == ASSET_HISTORY_PATH else "Use days=7 or days=30 and optional folders=1."
                )
                self.send_json(HTTPStatus.BAD_REQUEST, {"error": message})
                return

        try:
            if path == DASHBOARD_PATH:
                payload = read_dashboard(
                    self.server.database_path, range_days,
                    team_project=self.server.team_project, include_folders="folders" in options,
                )
                if self.server.demo:
                    payload["demo"] = True
                elif self.server.team_project is not None:
                    payload["teamProject"] = self.server.team_project
                else:
                    payload["localProject"] = read_local_project(self.server.database_path)
            elif path == ASSET_HISTORY_PATH:
                payload = read_asset_detail(
                    self.server.database_path, package_path, range_days, team_project=self.server.team_project
                )
                payload["canOpenInEditor"] = self.server.team_project is None and not self.server.demo
            else:
                activities = read_recent_activity(self.server.database_path, limit)
                if path == "/api/activity/latest":
                    payload = {"activity": activities[0] if activities else None}
                else:
                    payload = {"activities": activities}
        except sqlite3.Error as error:
            self.log_error("Activity query failed: %s", error)
            self.send_json(
                HTTPStatus.SERVICE_UNAVAILABLE,
                {"error": "The activity database could not be read. Try again."},
            )
            return

        self.send_json(HTTPStatus.OK, payload)

    def send_json(self, status, payload):
        body = (json.dumps(payload, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
        self.send_body(status, "application/json; charset=utf-8", body)

    def send_body(self, status, content_type, body):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header(
            "Content-Security-Policy",
            "default-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'",
        )
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def main():
    parser = argparse.ArgumentParser(description="Read recent Unreal Editor activity locally.")
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--db", type=Path, help="Path to activity.db")
    source.add_argument("--demo", action="store_true", help="Use temporary example data")
    source.add_argument("--team-folder", type=Path, help="Watch a folder for Unreal activity JSON exports")
    parser.add_argument("--project", help="Unreal project name for team imports")
    parser.add_argument("--team-db", type=Path, help="Separate database for team imports")
    parser.add_argument("--port", type=int, default=8765, help="Local HTTP port (default: 8765)")
    parser.add_argument("--open-browser", action="store_true", help="Open the dashboard in the default browser")
    args = parser.parse_args()

    if not 1 <= args.port <= 65535:
        parser.error("--port must be between 1 and 65535.")
    if args.team_folder is not None:
        if not args.project or not args.project.strip() or args.team_db is None:
            parser.error("--team-folder requires --project and --team-db.")
        if not args.team_folder.is_dir():
            parser.error(f"Team export folder not found: {args.team_folder}")
    elif args.project is not None or args.team_db is not None:
        parser.error("--project and --team-db require --team-folder.")

    with ExitStack() as resources:
        if args.demo:
            from demo_data import create_demo_database

            demo_directory = Path(resources.enter_context(TemporaryDirectory(prefix="activity-web-demo-")))
            database_path = create_demo_database(demo_directory)
        elif args.team_folder is not None:
            from team_import import initialize_team_database, team_importer

            database_path = args.team_db.absolute()
            try:
                initialize_team_database(database_path, args.project)
            except (OSError, sqlite3.Error, ValueError) as error:
                parser.error(f"Could not prepare the team database: {error}")
        else:
            database_path = args.db.absolute()
            if not database_path.is_file():
                parser.error(f"Activity database not found: {database_path}")

        try:
            read_dashboard(database_path, team_project=args.project)
        except sqlite3.Error as error:
            parser.error(f"Could not read the Activity table: {error}")

        try:
            server = resources.enter_context(
                ActivityServer(database_path, args.port, demo=args.demo, team_project=args.project)
            )
        except OSError as error:
            parser.error(f"Could not listen on 127.0.0.1:{args.port}: {error}")

        if args.team_folder is not None:
            print(f"Team project: {args.project}", flush=True)
            print(f"Watching exports in: {args.team_folder.absolute()}", flush=True)
            resources.enter_context(team_importer(args.team_folder.absolute(), database_path, args.project))

        dashboard_url = f"http://127.0.0.1:{server.server_port}/"
        print(f"Dashboard: {dashboard_url}", flush=True)
        print(f"API: {dashboard_url.rstrip('/')}{DASHBOARD_PATH}", flush=True)
        print("Press Ctrl+C to stop.", flush=True)
        if args.open_browser:
            try:
                opened = webbrowser.open_new_tab(dashboard_url)
            except (OSError, webbrowser.Error):
                opened = False
            if not opened:
                print("Open the Dashboard URL above in your browser.", flush=True)

        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
