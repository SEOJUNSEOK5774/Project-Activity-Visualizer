import json
import os
from datetime import datetime, timezone
from pathlib import Path
from tempfile import NamedTemporaryFile

import unreal


def collect_reference_paths(registry, package_names):
    options = unreal.AssetRegistryDependencyOptions()
    options.include_hard_package_references = True
    options.include_soft_package_references = True
    options.include_game_package_references = True
    options.include_editor_only_package_references = True
    packages = {name.casefold(): name for name in package_names}
    referencer_paths = {}
    dependency_paths = {}
    for package_key, package in sorted(packages.items()):
        referencers = registry.get_referencers(package, options)
        dependencies = registry.get_dependencies(package, options)
        if referencers is not None:
            referencer_paths[package] = project_reference_paths(package_key, referencers)
        if dependencies is not None:
            dependency_paths[package] = project_reference_paths(package_key, dependencies)

    return referencer_paths, dependency_paths


def project_reference_paths(package_key, references):
    paths = {}
    for reference in references:
        path = str(reference)
        key = path.casefold()
        if key.startswith("/game/") and key != package_key:
            paths.setdefault(key, path)
    return sorted(paths.values(), key=str.casefold)


def activity_directory():
    saved_directory = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
    return saved_directory / "ProjectActivity"


def write_reference_snapshot(destination, referencer_paths, dependency_paths, automatic=False):
    destination.parent.mkdir(parents=True, exist_ok=True)
    snapshot = {
        "schemaVersion": 1,
        "capturedAtUtc": datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z"),
        "referencerCounts": {package: len(paths) for package, paths in referencer_paths.items()},
        "referencerPaths": referencer_paths,
        "dependencyPaths": dependency_paths,
    }
    if automatic:
        snapshot["automatic"] = True
    temporary_path = None
    try:
        with NamedTemporaryFile(mode="w", encoding="utf-8", dir=destination.parent, delete=False) as stream:
            temporary_path = Path(stream.name)
            json.dump(snapshot, stream, ensure_ascii=False)
        os.replace(temporary_path, destination)
    finally:
        if temporary_path is not None:
            temporary_path.unlink(missing_ok=True)


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(["/Game"], False)
    registry.wait_for_completion()
    if registry.is_loading_assets():
        raise RuntimeError("Asset discovery is still running. Try exporting again shortly.")

    assets = registry.get_assets_by_path("/Game", True, True) or []
    packages = [str(asset.package_name) for asset in assets]
    referencers, dependencies = collect_reference_paths(registry, packages)
    destination = activity_directory() / "asset_references.json"
    write_reference_snapshot(destination, referencers, dependencies)
    unreal.log(f"Project Activity: exported references for {len(referencers.keys() | dependencies.keys())} packages to {destination}")


if __name__ == "__main__":
    main()
