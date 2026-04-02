"""
Unreal MCP Bridge
-----------------
언리얼 에디터 내부에서 실행되는 HTTP 서버.
MCP 서버의 명령을 받아 Unreal Python API로 실행.

설정 방법:
  Edit > Project Settings > Python > Additional Paths 에 이 파일 경로 추가
  Edit > Project Settings > Python > Startup Scripts 에 "unreal_bridge" 추가
  (또는 Unreal Python 콘솔에서 직접 exec(open('경로/unreal_bridge.py').read()) 실행)
"""

import unreal
import json
import threading
import queue
import uuid
import time
from http.server import HTTPServer, BaseHTTPRequestHandler

PORT = 9877

pending_commands: queue.Queue = queue.Queue()
completed_results: dict = {}
results_lock = threading.Lock()
_tick_handle = None


# ──────────────────────────────────────────────
# HTTP Handler
# ──────────────────────────────────────────────

class BridgeHandler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass  # 언리얼 출력창 오염 방지

    def do_GET(self):
        if self.path == "/ping":
            self._respond(200, {"status": "ok", "message": "Unreal MCP Bridge is running"})
        else:
            self._respond(404, {"error": "Not found"})

    def do_POST(self):
        try:
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length)
            cmd = json.loads(body)
        except Exception as e:
            self._respond(400, {"error": f"Invalid JSON: {e}"})
            return

        cmd_id = str(uuid.uuid4())
        cmd["_id"] = cmd_id
        pending_commands.put(cmd)

        # 최대 10초 대기
        deadline = time.time() + 10
        while time.time() < deadline:
            with results_lock:
                if cmd_id in completed_results:
                    result = completed_results.pop(cmd_id)
                    self._respond(200, result)
                    return
            time.sleep(0.05)

        self._respond(408, {"success": False, "error": "Timeout: Unreal did not respond in time"})

    def _respond(self, code: int, data: dict):
        body = json.dumps(data, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


# ──────────────────────────────────────────────
# Unreal Commands (메인 스레드에서 실행)
# ──────────────────────────────────────────────

def execute_command(cmd: dict) -> dict:
    action = cmd.get("action", "")
    try:
        if action == "spawn_box":
            return cmd_spawn_box(cmd)
        elif action == "spawn_sphere":
            return cmd_spawn_mesh(cmd, "/Engine/BasicShapes/Sphere", "Sphere")
        elif action == "spawn_cylinder":
            return cmd_spawn_mesh(cmd, "/Engine/BasicShapes/Cylinder", "Cylinder")
        elif action == "list_actors":
            return cmd_list_actors(cmd)
        elif action == "delete_actor":
            return cmd_delete_actor(cmd)
        elif action == "move_actor":
            return cmd_move_actor(cmd)
        elif action == "set_scale":
            return cmd_set_scale(cmd)
        elif action == "get_actor_info":
            return cmd_get_actor_info(cmd)
        elif action == "set_folder":
            return cmd_set_folder(cmd)
        elif action == "delete_folder":
            return cmd_delete_folder(cmd)
        elif action == "delete_actor_at":
            return cmd_delete_actor_at(cmd)
        elif action == "create_animation_blueprint":
            return cmd_create_animation_blueprint(cmd)
        elif action == "run_python":
            return cmd_run_python(cmd)
        else:
            return {"success": False, "error": f"알 수 없는 액션: {action}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def cmd_spawn_box(cmd: dict) -> dict:
    return cmd_spawn_mesh(cmd, "/Engine/BasicShapes/Cube", "Box")


def cmd_spawn_mesh(cmd: dict, mesh_path: str, default_prefix: str) -> dict:
    loc = cmd.get("location", [0.0, 0.0, 0.0])
    rot = cmd.get("rotation", [0.0, 0.0, 0.0])  # pitch, yaw, roll
    scale = cmd.get("scale", [1.0, 1.0, 1.0])
    label = cmd.get("label", f"{default_prefix}Actor")

    location = unreal.Vector(float(loc[0]), float(loc[1]), float(loc[2]))
    rotation = unreal.Rotator(float(rot[0]), float(rot[1]), float(rot[2]))

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, location, rotation
    )
    if not actor:
        return {"success": False, "error": "액터 스폰 실패"}

    mesh = unreal.load_asset(mesh_path)
    if mesh:
        actor.static_mesh_component.set_static_mesh(mesh)

    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(float(scale[0]), float(scale[1]), float(scale[2])))

    return {
        "success": True,
        "message": f"'{label}' 액터 생성 완료 (위치: {loc})",
        "label": label
    }


def cmd_list_actors(cmd: dict) -> dict:
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    actor_list = []
    for actor in actors:
        try:
            loc = actor.get_actor_location()
            actor_list.append({
                "label": actor.get_actor_label(),
                "class": actor.get_class().get_name(),
                "location": [round(loc.x, 1), round(loc.y, 1), round(loc.z, 1)]
            })
        except Exception:
            pass

    filter_class = cmd.get("filter_class", "")
    if filter_class:
        actor_list = [a for a in actor_list if filter_class.lower() in a["class"].lower()]

    lines = [f"- [{a['class']}] {a['label']}  @ {a['location']}" for a in actor_list]
    return {
        "success": True,
        "message": f"총 {len(actor_list)}개 액터:\n" + "\n".join(lines),
        "actors": actor_list
    }


def cmd_delete_actor(cmd: dict) -> dict:
    label = cmd.get("label", "")
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in actors:
        if actor.get_actor_label() == label:
            actor.destroy_actor()
            return {"success": True, "message": f"'{label}' 액터 삭제 완료"}
    return {"success": False, "error": f"'{label}' 액터를 찾을 수 없습니다"}


def cmd_move_actor(cmd: dict) -> dict:
    label = cmd.get("label", "")
    loc = cmd.get("location", [0.0, 0.0, 0.0])
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in actors:
        if actor.get_actor_label() == label:
            actor.set_actor_location(
                unreal.Vector(float(loc[0]), float(loc[1]), float(loc[2])),
                False, False
            )
            return {"success": True, "message": f"'{label}' → {loc} 이동 완료"}
    return {"success": False, "error": f"'{label}' 액터를 찾을 수 없습니다"}


def cmd_set_scale(cmd: dict) -> dict:
    label = cmd.get("label", "")
    scale = cmd.get("scale", [1.0, 1.0, 1.0])
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in actors:
        if actor.get_actor_label() == label:
            actor.set_actor_scale3d(
                unreal.Vector(float(scale[0]), float(scale[1]), float(scale[2]))
            )
            return {"success": True, "message": f"'{label}' 스케일 → {scale} 설정 완료"}
    return {"success": False, "error": f"'{label}' 액터를 찾을 수 없습니다"}


def cmd_set_folder(cmd: dict) -> dict:
    label = cmd.get("label", "")
    folder = cmd.get("folder", "")
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in actors:
        if actor.get_actor_label() == label:
            actor.set_folder_path(folder)
            return {"success": True, "message": f"'{label}' → 폴더 '{folder}' 설정 완료"}
    return {"success": False, "error": f"'{label}' 액터를 찾을 수 없습니다"}


def cmd_delete_actor_at(cmd: dict) -> dict:
    label = cmd.get("label", "")
    loc = cmd.get("location", [0.0, 0.0, 0.0])
    tolerance = cmd.get("tolerance", 50.0)
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in actors:
        if actor.get_actor_label() == label:
            al = actor.get_actor_location()
            if (abs(al.x - loc[0]) <= tolerance and
                abs(al.y - loc[1]) <= tolerance and
                abs(al.z - loc[2]) <= tolerance):
                actor.destroy_actor()
                return {"success": True, "message": f"'{label}' @ {loc} 삭제 완료"}
    return {"success": False, "error": f"'{label}' @ {loc} 에서 액터를 찾을 수 없습니다"}


def cmd_delete_folder(cmd: dict) -> dict:
    folder = cmd.get("folder", "")
    try:
        subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        subsystem.delete_folder(unreal.Name(folder))
        return {"success": True, "message": f"폴더 '{folder}' 삭제 완료"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def cmd_run_python(cmd: dict) -> dict:
    import io, contextlib
    code = cmd.get("code", "")
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        exec(code, {"unreal": unreal})
    return {"success": True, "message": buf.getvalue()}


def cmd_create_animation_blueprint(cmd: dict) -> dict:
    skeletal_mesh_path = cmd.get("skeletal_mesh_path", "")
    asset_name = cmd.get("asset_name", "")
    package_path = cmd.get("package_path", "")

    skeletal_mesh = unreal.load_asset(skeletal_mesh_path)
    if not skeletal_mesh:
        return {"success": False, "error": f"스켈레탈 메시를 찾을 수 없습니다: {skeletal_mesh_path}"}

    skeleton = skeletal_mesh.skeleton
    if not skeleton:
        return {"success": False, "error": "스켈레탈 메시에서 스켈레톤을 가져올 수 없습니다"}

    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property("target_skeleton", skeleton)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    abp = asset_tools.create_asset(asset_name, package_path, unreal.AnimBlueprint, factory)
    if not abp:
        return {"success": False, "error": f"ABP 생성 실패: {package_path}/{asset_name}"}

    unreal.EditorAssetLibrary.save_asset(abp.get_path_name())
    return {
        "success": True,
        "message": f"애니메이션 블루프린트 '{asset_name}' 생성 완료: {package_path}/{asset_name}",
        "path": f"{package_path}/{asset_name}"
    }


def cmd_get_actor_info(cmd: dict) -> dict:
    label = cmd.get("label", "")
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in actors:
        if actor.get_actor_label() == label:
            loc = actor.get_actor_location()
            rot = actor.get_actor_rotation()
            scale = actor.get_actor_scale3d()
            return {
                "success": True,
                "message": (
                    f"[{label}]\n"
                    f"  위치: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})\n"
                    f"  회전: ({rot.pitch:.1f}, {rot.yaw:.1f}, {rot.roll:.1f})\n"
                    f"  스케일: ({scale.x:.2f}, {scale.y:.2f}, {scale.z:.2f})"
                )
            }
    return {"success": False, "error": f"'{label}' 액터를 찾을 수 없습니다"}


# ──────────────────────────────────────────────
# Tick Callback (메인 스레드에서 큐 처리)
# ──────────────────────────────────────────────

def process_tick(delta_time: float):
    while not pending_commands.empty():
        try:
            cmd = pending_commands.get_nowait()
            cmd_id = cmd.get("_id", "")
            result = execute_command(cmd)
            with results_lock:
                completed_results[cmd_id] = result
        except Exception as e:
            unreal.log_warning(f"[UnrealMCP] Tick error: {e}")


# ──────────────────────────────────────────────
# 시작
# ──────────────────────────────────────────────

def start_bridge():
    global _tick_handle

    # HTTP 서버 백그라운드 스레드
    server = HTTPServer(("127.0.0.1", PORT), BridgeHandler)
    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.start()

    # 메인 스레드 틱 등록
    _tick_handle = unreal.register_slate_post_tick_callback(process_tick)

    unreal.log(f"[UnrealMCP] Bridge started on http://127.0.0.1:{PORT}")
    unreal.log("[UnrealMCP] 지원 액션: spawn_box, spawn_sphere, spawn_cylinder, list_actors, delete_actor, move_actor, set_scale, get_actor_info")


start_bridge()
