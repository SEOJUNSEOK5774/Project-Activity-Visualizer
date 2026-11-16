import json
import sqlite3
from contextlib import closing
from datetime import datetime, timedelta, timezone


def create_demo_database(directory):
    now = datetime.now(timezone.utc)
    midnight = now.replace(hour=0, minute=0, second=0, microsecond=0)
    door = "/Game/Demo/BP_DemoDoor"
    controller = "/Game/Demo/BP_DemoLevelController"
    material = "/Game/Demo/Materials/M_DemoMetal"
    sample_saves = [
        (door, "Alex", 1, 540),
        (door, "Alex", 1, 541),
        (door, "Alex", 1, 550),
        (door, "Sam", 1, 560),
        (controller, "Sam", 2, 570),
        (material, "Alex", 3, 620),
        (material, "Alex", 3, 630),
        (material, "Alex", 3, 640),
        (door, "Sam", 10, 550),
    ]
    sample_edits = [
        (door, "Alex", midnight - timedelta(hours=32)),
        ("/Game/Demo/BP_DemoDraft", "Sam", midnight - timedelta(hours=8)),
    ]
    database_path = directory / "activity.db"
    with closing(sqlite3.connect(database_path)) as db:
        with db:
            db.execute("""
                CREATE TABLE Activity (
                    EventId TEXT PRIMARY KEY,
                    PackagePath TEXT NOT NULL,
                    TimestampUtc TEXT NOT NULL,
                    ActivityType INTEGER NOT NULL,
                    ContributorAlias TEXT NOT NULL DEFAULT ''
                )
            """)
            db.executemany(
                "INSERT INTO Activity VALUES (?, ?, ?, 0, ?)",
                [
                    (
                        f"{index:032x}",
                        package,
                        (midnight + timedelta(days=-days, minutes=minute))
                        .isoformat(timespec="milliseconds").replace("+00:00", "Z"),
                        alias,
                    )
                    for index, (package, alias, days, minute) in enumerate(sample_saves, start=1)
                ],
            )
            db.execute("""
                CREATE TABLE PackageEdit (
                    EventId TEXT PRIMARY KEY NOT NULL,
                    PackagePath TEXT NOT NULL,
                    TimestampUtc TEXT NOT NULL,
                    ContributorAlias TEXT NOT NULL DEFAULT ''
                )
            """)
            db.executemany(
                "INSERT INTO PackageEdit VALUES (?, ?, ?, ?)",
                [
                    (f"{index:032x}", package,
                     edited_at.isoformat(timespec="milliseconds").replace("+00:00", "Z"), alias)
                    for index, (package, alias, edited_at) in enumerate(sample_edits, start=1)
                ],
            )

    referencers = {door: [controller], controller: [], material: [door]}
    dependencies = {door: [material], controller: [door], material: []}
    snapshot = {
        "schemaVersion": 1,
        "capturedAtUtc": now.isoformat(timespec="seconds").replace("+00:00", "Z"),
        "referencerCounts": {package: len(paths) for package, paths in referencers.items()},
        "referencerPaths": referencers,
        "dependencyPaths": dependencies,
    }
    (directory / "asset_references.json").write_text(json.dumps(snapshot), encoding="utf-8")
    return database_path
