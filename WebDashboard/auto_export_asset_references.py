import runpy
import sqlite3
from pathlib import Path

import unreal


REFRESH_SECONDS = 15.0
TICKER_ATTRIBUTE = "_project_activity_reference_ticker"


def start_auto_export():
    script_directory = Path(__file__).resolve().parent
    exporter = runpy.run_path(str(script_directory / "export_asset_references.py"))
    api = runpy.run_path(str(script_directory / "activity_api.py"))
    storage = exporter["activity_directory"]()
    database = storage / "activity.db"
    destination = storage / api["REFERENCE_SNAPSHOT_NAME"]
    read_targets = api["read_reference_targets"]
    collect_paths = exporter["collect_reference_paths"]
    write_snapshot = exporter["write_reference_snapshot"]
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    bridge = runpy.run_path(str(script_directory / "asset_open_bridge.py"))
    bridge["start_asset_open_listener"](unreal, registry, storage)
    has_exported = False
    last_error = None

    def update_references(_delta_seconds):
        nonlocal has_exported, last_error
        if registry.is_loading_assets() or not database.is_file():
            return True

        try:
            packages = read_targets(database)
            referencers, dependencies = collect_paths(registry, packages)
            write_snapshot(destination, referencers, dependencies, automatic=True)
        except (OSError, sqlite3.Error, RuntimeError) as error:
            message = str(error)
            if message != last_error:
                unreal.log_warning(f"Project Activity: reference export will retry: {message}")
                last_error = message
            return True

        last_error = None
        if not has_exported:
            unreal.log(f"Project Activity: automatic reference export is updating {destination}")
            has_exported = True
        return True

    previous_ticker = getattr(unreal, TICKER_ATTRIBUTE, None)
    if previous_ticker is not None:
        unreal.unregister_ticker_callback(previous_ticker)
    ticker = unreal.register_ticker_callback(update_references, REFRESH_SECONDS)
    setattr(unreal, TICKER_ATTRIBUTE, ticker)
    unreal.log(f"Project Activity: automatic reference export scheduled every {REFRESH_SECONDS:g} seconds")


if __name__ == "__main__":
    start_auto_export()
