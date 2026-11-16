import json
import re
import sqlite3
from contextlib import closing, contextmanager
from datetime import datetime
from stat import S_ISREG
from threading import Event, Thread
from uuid import UUID


SCAN_SECONDS = 5
TEAM_TABLES = {"Activity", "TeamProject"}
UTC_TIMESTAMP = re.compile(
    r"([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2})(?:\.([0-9]{1,6}))?Z"
)
INSERT_ACTIVITY_SQL = """
    INSERT INTO Activity (EventId, PackagePath, TimestampUtc, ActivityType, ContributorAlias)
    VALUES (?, ?, ?, 0, ?)
    ON CONFLICT(EventId) DO NOTHING
"""


def initialize_team_database(database_path, project):
    database_path.parent.mkdir(parents=True, exist_ok=True)
    with closing(sqlite3.connect(database_path, timeout=1.0)) as db, db:
        db.execute("BEGIN IMMEDIATE")
        tables = {row[0] for row in db.execute("SELECT name FROM sqlite_master WHERE type = 'table'")}
        if tables:
            if not TEAM_TABLES.issubset(tables):
                raise ValueError("Choose a new team database path. This database was not created for team imports.")
            stored_project = db.execute("SELECT ProjectName FROM TeamProject WHERE Id = 1").fetchone()
            if stored_project != (project,):
                raise ValueError("This team database belongs to a different Unreal project.")
            return

        db.execute("""
            CREATE TABLE Activity (
                EventId TEXT PRIMARY KEY NOT NULL,
                PackagePath TEXT NOT NULL,
                TimestampUtc TEXT NOT NULL,
                ActivityType INTEGER NOT NULL,
                ContributorAlias TEXT NOT NULL DEFAULT ''
            )
        """)
        db.execute("CREATE INDEX ActivityTimestamp ON Activity (TimestampUtc)")
        db.execute("""
            CREATE TABLE TeamProject (
                Id INTEGER PRIMARY KEY CHECK (Id = 1),
                ProjectName TEXT NOT NULL
            )
        """)
        db.execute("INSERT INTO TeamProject VALUES (1, ?)", (project,))


def parse_export(path, project):
    raw = path.read_bytes()
    encoding = "utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    data = json.loads(raw.decode(encoding))
    if not isinstance(data, dict) or data.get("Format") != "ProjectActivityVisualizer":
        raise ValueError("Expected a Project Activity Visualizer export.")
    version = data.get("SchemaVersion")
    if type(version) is not int or version != 1:
        raise ValueError("Unsupported export version.")
    if data.get("ProjectName") != project:
        raise ValueError(f"ProjectName does not match {project}.")
    records = data.get("Records")
    count = data.get("RecordCount")
    if not isinstance(records, list) or type(count) is not int or count != len(records):
        raise ValueError("RecordCount does not match Records.")

    rows = []
    for index, record in enumerate(records, start=1):
        try:
            event_id = record["EventId"]
            package = record["PackagePath"]
            timestamp = record["TimestampUtc"]
            alias = record["ContributorAlias"]
            activity_type = record["ActivityType"]
            if not all(isinstance(value, str) and "\0" not in value for value in (event_id, package, timestamp, alias)):
                raise ValueError
            if type(activity_type) is not int or activity_type != 0 or not package.startswith("/Game/") or len(package) <= 6:
                raise ValueError
            identifier = UUID(event_id)
            if not identifier.int or str(identifier) != event_id.lower():
                raise ValueError
            match = UTC_TIMESTAMP.fullmatch(timestamp)
            if match is None:
                raise ValueError
            seconds, fraction = match.groups()
            microseconds = (fraction or "").ljust(6, "0")
            captured_at = datetime.fromisoformat(f"{seconds}.{microseconds}+00:00")
            normalized_time = captured_at.isoformat(timespec="milliseconds").replace("+00:00", "Z")
        except (KeyError, TypeError, ValueError):
            raise ValueError(f"Invalid or unsupported activity at record {index}.") from None
        rows.append((str(identifier).upper(), package, normalized_time, alias))
    return rows


def import_export(database_path, path, project):
    rows = parse_export(path, project)
    with closing(sqlite3.connect(database_path, timeout=1.0)) as db, db:
        db.executemany(INSERT_ACTIVITY_SQL, rows)
        inserted = db.total_changes
    return inserted, len(rows) - inserted


def watch_exports(stop, folder, database_path, project):
    processed = {}
    last_folder_error = None
    while not stop.is_set():
        try:
            paths = sorted(path for path in folder.iterdir() if path.suffix.lower() == ".json")
        except OSError as error:
            message = str(error)
            if message != last_folder_error:
                print(f"Team folder unavailable: {message}", flush=True)
            last_folder_error = message
        else:
            last_folder_error = None
            current_paths = set(paths)
            processed = {path: signature for path, signature in processed.items() if path in current_paths}
            for path in paths:
                if stop.is_set():
                    return
                try:
                    info = path.stat()
                    if not S_ISREG(info.st_mode):
                        continue
                    signature = (info.st_mtime_ns, info.st_size, info.st_ino)
                    if processed.get(path) == signature:
                        continue
                    inserted, duplicates = import_export(database_path, path, project)
                except (ValueError, RecursionError) as error:
                    print(f"Skipped {path.name}: {error}", flush=True)
                except (OSError, sqlite3.Error) as error:
                    print(f"Retrying {path.name}: {error}", flush=True)
                    continue
                else:
                    print(f"{path.name}: {inserted} imported, {duplicates} duplicates", flush=True)
                processed[path] = signature
        stop.wait(SCAN_SECONDS)


@contextmanager
def team_importer(folder, database_path, project):
    stop = Event()
    worker = Thread(target=watch_exports, args=(stop, folder, database_path, project), daemon=True)
    worker.start()
    try:
        yield
    finally:
        stop.set()
        worker.join()
