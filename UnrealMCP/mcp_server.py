"""
Unreal MCP Server
-----------------
Claude Desktop과 연결되는 MCP 서버.
자연어 명령을 Unreal Bridge HTTP API로 전달.

실행 방법:
  pip install mcp requests
  python mcp_server.py
"""

import requests
import json
from mcp.server.fastmcp import FastMCP

BRIDGE_URL = "http://127.0.0.1:9877"

mcp = FastMCP("UnrealMCP")


# ──────────────────────────────────────────────
# Helper
# ──────────────────────────────────────────────

def send(action: str, **kwargs) -> str:
    payload = {"action": action, **kwargs}
    try:
        resp = requests.post(f"{BRIDGE_URL}/command", json=payload, timeout=15)
        data = resp.json()
        return data.get("message") or data.get("error") or json.dumps(data, ensure_ascii=False)
    except requests.ConnectionError:
        return "오류: 언리얼 브릿지에 연결할 수 없습니다. 언리얼 에디터에서 unreal_bridge.py가 실행 중인지 확인하세요."
    except Exception as e:
        return f"오류: {e}"


# ──────────────────────────────────────────────
# Tools
# ──────────────────────────────────────────────

@mcp.tool()
def spawn_box(
    label: str = "BoxActor",
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    scale_x: float = 1.0,
    scale_y: float = 1.0,
    scale_z: float = 1.0,
) -> str:
    """
    언리얼 레벨에 박스(큐브) 스태틱 메시 액터를 생성합니다.

    Args:
        label: 액터 이름 (기본값: BoxActor)
        x, y, z: 월드 위치 (언리얼 단위 cm, 기본값: 0, 0, 0)
        scale_x, scale_y, scale_z: 스케일 (기본값: 1.0)
    """
    return send("spawn_box", label=label, location=[x, y, z], scale=[scale_x, scale_y, scale_z])


@mcp.tool()
def spawn_sphere(
    label: str = "SphereActor",
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    scale: float = 1.0,
) -> str:
    """
    언리얼 레벨에 구(Sphere) 스태틱 메시 액터를 생성합니다.

    Args:
        label: 액터 이름
        x, y, z: 월드 위치 (cm)
        scale: 균일 스케일
    """
    return send("spawn_sphere", label=label, location=[x, y, z], scale=[scale, scale, scale])


@mcp.tool()
def spawn_cylinder(
    label: str = "CylinderActor",
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    scale: float = 1.0,
) -> str:
    """
    언리얼 레벨에 원기둥(Cylinder) 스태틱 메시 액터를 생성합니다.

    Args:
        label: 액터 이름
        x, y, z: 월드 위치 (cm)
        scale: 균일 스케일
    """
    return send("spawn_cylinder", label=label, location=[x, y, z], scale=[scale, scale, scale])


@mcp.tool()
def list_actors(filter_class: str = "") -> str:
    """
    현재 언리얼 레벨의 모든 액터 목록을 반환합니다.

    Args:
        filter_class: 특정 클래스만 필터링 (예: "StaticMeshActor"). 비워두면 전체 목록.
    """
    return send("list_actors", filter_class=filter_class)


@mcp.tool()
def delete_actor(label: str) -> str:
    """
    지정한 이름의 액터를 레벨에서 삭제합니다.

    Args:
        label: 삭제할 액터의 이름
    """
    return send("delete_actor", label=label)


@mcp.tool()
def move_actor(label: str, x: float, y: float, z: float) -> str:
    """
    액터를 지정한 월드 위치로 이동합니다.

    Args:
        label: 이동할 액터 이름
        x, y, z: 목표 위치 (cm)
    """
    return send("move_actor", label=label, location=[x, y, z])


@mcp.tool()
def set_actor_scale(label: str, scale_x: float, scale_y: float, scale_z: float) -> str:
    """
    액터의 스케일을 변경합니다.

    Args:
        label: 대상 액터 이름
        scale_x, scale_y, scale_z: 각 축의 스케일 값
    """
    return send("set_scale", label=label, scale=[scale_x, scale_y, scale_z])


@mcp.tool()
def get_actor_info(label: str) -> str:
    """
    액터의 위치, 회전, 스케일 정보를 반환합니다.

    Args:
        label: 조회할 액터 이름
    """
    return send("get_actor_info", label=label)


@mcp.tool()
def ping_bridge() -> str:
    """언리얼 브릿지 서버 연결 상태를 확인합니다."""
    try:
        resp = requests.get(f"{BRIDGE_URL}/ping", timeout=3)
        return resp.json().get("message", "연결됨")
    except Exception:
        return "연결 실패: 언리얼 에디터에서 unreal_bridge.py를 먼저 실행하세요."


@mcp.tool()
def create_animation_blueprint(
    skeletal_mesh_path: str,
    asset_name: str,
    package_path: str = "/Game/Animations",
) -> str:
    """
    스켈레탈 메시를 기반으로 애니메이션 블루프린트(ABP)를 생성합니다.

    Args:
        skeletal_mesh_path: 스켈레탈 메시의 언리얼 경로 (예: /Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple)
        asset_name: 생성할 ABP 에셋 이름 (예: ABP_Quinn)
        package_path: 저장할 콘텐츠 경로 (기본값: /Game/Animations)
    """
    return send(
        "create_animation_blueprint",
        skeletal_mesh_path=skeletal_mesh_path,
        asset_name=asset_name,
        package_path=package_path,
    )


if __name__ == "__main__":
    mcp.run()
