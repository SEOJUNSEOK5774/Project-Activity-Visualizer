import json
import os
import time
from tempfile import NamedTemporaryFile
from uuid import uuid4


QUEUE_NAME = "AssetOpenRequests"
HEARTBEAT_NAME = "editor.ready"
LISTENER_ATTRIBUTE = "_project_activity_asset_open_ticker"
TICK_SECONDS = 0.5
HEARTBEAT_SECONDS = 2
HEARTBEAT_MAX_AGE = 8
REQUEST_WAIT_SECONDS = 8
RESULT_MAX_AGE = 30
RESULT_STATUSES = {"opened", "not_found", "failed"}
# Unreal's INVALID_LONGPACKAGE_CHARACTERS plus control characters.
INVALID_PACKAGE_CHARACTERS = frozenset("\\:*?\"<>|' ,.&!~@#")


def valid_package_path(path):
    return (
        isinstance(path, str)
        and len(path) <= 1024
        and path[:6].casefold() == "/game/"
        and all(path[6:].split("/"))
        and all(char.isprintable() and char not in INVALID_PACKAGE_CHARACTERS for char in path)
    )


def write_json_atomically(destination, payload):
    temporary_path = None
    try:
        with NamedTemporaryFile(mode="w", encoding="utf-8", dir=destination.parent, delete=False) as stream:
            temporary_path = stream.name
            json.dump(payload, stream, ensure_ascii=False)
        os.replace(temporary_path, destination)
    finally:
        if temporary_path is not None:
            try:
                os.unlink(temporary_path)
            except FileNotFoundError:
                pass


def file_age(path):
    try:
        return time.time() - path.stat().st_mtime
    except FileNotFoundError:
        return float("inf")


def editor_is_connected(storage):
    return file_age(storage / QUEUE_NAME / HEARTBEAT_NAME) <= HEARTBEAT_MAX_AGE


def request_asset_open(storage, package_path):
    if not editor_is_connected(storage):
        return "offline"

    queue = storage / QUEUE_NAME
    request_id = uuid4().hex
    request_path = queue / f"{request_id}.request.json"
    result_path = queue / f"{request_id}.result.json"
    expires_at = time.time() + REQUEST_WAIT_SECONDS
    deadline = time.monotonic() + REQUEST_WAIT_SECONDS
    try:
        write_json_atomically(request_path, {"packagePath": package_path, "expiresAt": expires_at})
        while time.monotonic() < deadline:
            try:
                result = json.loads(result_path.read_text(encoding="utf-8"))
            except FileNotFoundError:
                time.sleep(0.1)
                continue
            except ValueError:
                return "failed"
            status = result.get("status") if isinstance(result, dict) else None
            return status if isinstance(status, str) and status in RESULT_STATUSES else "failed"
        return "timeout"
    finally:
        request_path.unlink(missing_ok=True)
        result_path.unlink(missing_ok=True)


def open_editor_asset(unreal, registry, package_path):
    assets = registry.get_assets_by_package_name(package_path) or []
    name = package_path.rsplit("/", 1)[-1].casefold()
    asset_data = next((item for item in assets if str(item.asset_name).casefold() == name), None)
    if asset_data is None and len(assets) == 1:
        asset_data = assets[0]
    asset = asset_data.get_asset() if asset_data is not None else None
    if asset is None:
        return "not_found"
    subsystem = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
    return "opened" if subsystem.open_editor_for_assets([asset]) else "failed"


def start_asset_open_listener(unreal, registry, storage):
    queue = storage / QUEUE_NAME
    next_heartbeat = 0
    last_error = None

    def check_requests(_delta_seconds):
        nonlocal next_heartbeat, last_error
        if registry.is_loading_assets():
            return True

        try:
            now = time.monotonic()
            if now >= next_heartbeat:
                queue.mkdir(parents=True, exist_ok=True)
                (queue / HEARTBEAT_NAME).touch()
                for path in queue.glob("*.result.json"):
                    if file_age(path) > RESULT_MAX_AGE:
                        path.unlink(missing_ok=True)
                next_heartbeat = now + HEARTBEAT_SECONDS

            request_path = next(queue.glob("*.request.json"), None)
            if request_path is not None:
                # Read, then claim by deleting. Only one editor can claim this request.
                try:
                    raw_request = request_path.read_text(encoding="utf-8")
                    request_path.unlink()
                except FileNotFoundError:
                    return True
                request = json.loads(raw_request)
                package_path = request["packagePath"]
                if request["expiresAt"] >= time.time() and valid_package_path(package_path):
                    try:
                        status = open_editor_asset(unreal, registry, package_path)
                    except Exception as error:
                        unreal.log_warning(f"Project Activity: could not open {package_path}: {error}")
                        status = "failed"
                    result_path = request_path.with_suffix("").with_suffix(".result.json")
                    write_json_atomically(result_path, {"status": status})
        except (OSError, ValueError, KeyError, TypeError) as error:
            message = str(error)
            if message != last_error:
                unreal.log_warning(f"Project Activity: asset opening will retry: {message}")
                last_error = message
        else:
            last_error = None
        return True

    previous_ticker = getattr(unreal, LISTENER_ATTRIBUTE, None)
    if previous_ticker is not None:
        unreal.unregister_ticker_callback(previous_ticker)
    setattr(unreal, LISTENER_ATTRIBUTE, unreal.register_ticker_callback(check_requests, TICK_SECONDS))
    unreal.log("Project Activity: listening for local dashboard asset opens")
