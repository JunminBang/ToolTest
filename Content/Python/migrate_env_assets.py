"""
외주사 에셋 마이그레이션 스크립트
/Game/Developers/qkdwnsals22 → /Game/Env/{Mesh, Material, Texture}

실행 방법:
    MCP execute_python 으로 실행하거나
    언리얼 에디터 Python 콘솔에서 직접 실행

동작:
    1. 소스 경로 에셋 스캔 및 타입 분류
    2. 타입별 대상 경로로 복사 (원본 보존)
    3. 복사된 MaterialInstance의 Parent → 우리 마스터 머티리얼로 교체
    4. 파라미터 값 이전 (외주사 파라미터명 → 우리 파라미터명)
    5. 메시 머티리얼 슬롯 → 새 경로의 머티리얼로 재연결
    6. 전체 저장
"""

import unreal
import json
import datetime
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent if "__file__" in dir() else Path("C:/Unreal/ToolTest/Content/Python")

# ── 설정 ──────────────────────────────────────────────────────────────────────

SOURCE_PATH     = "/Game/Developers/qkdwnsals22"
TARGET_MESH     = "/Game/Env/Mesh"
TARGET_MATERIAL = "/Game/Env/Material"
TARGET_TEXTURE  = "/Game/Env/Texture"
MASTER_MATERIAL = "/Game/Material/M_Env_Base"

# 외주사 파라미터명 → 우리 파라미터명 (param_map.json에서 로드)
_param_map_path = SCRIPT_DIR / "param_map.json"
with open(_param_map_path, "r", encoding="utf-8") as _f:
    PARAM_MAP = {k: v for k, v in json.load(_f).items() if not k.startswith("_")}

# ── 라이브러리 ────────────────────────────────────────────────────────────────

asset_lib = unreal.EditorAssetLibrary
mat_lib   = unreal.MaterialEditingLibrary


# ── 유틸 함수 ─────────────────────────────────────────────────────────────────

def classify_asset(class_path: str) -> str:
    """에셋 클래스 경로로 타입 분류. 'mesh' | 'material_instance' | 'material' | 'texture' | 'unknown'"""
    c = class_path.lower()
    if "staticmesh" in c:
        return "mesh"
    if "materialinstanceconstant" in c or "materialinstancedynamic" in c:
        return "material_instance"
    if "material" in c:
        return "material"
    if "texture" in c:
        return "texture"
    return "unknown"


def get_target_path(source_path: str, asset_type: str) -> str:
    asset_name = source_path.rstrip("/").split("/")[-1]
    targets = {
        "mesh":              TARGET_MESH,
        "material_instance": TARGET_MATERIAL,
        "material":          TARGET_MATERIAL,
        "texture":           TARGET_TEXTURE,
    }
    folder = targets.get(asset_type, TARGET_MATERIAL)
    return f"{folder}/{asset_name}"


def copy_asset(source_path: str, target_path: str):
    """에셋을 대상 경로로 복사. 이미 존재하면 스킵. 성공 시 로드된 에셋 반환."""
    if asset_lib.does_asset_exist(target_path):
        unreal.log(f"  [SKIP] 이미 존재: {target_path}")
        return unreal.load_asset(target_path)

    if not asset_lib.duplicate_asset(source_path, target_path):
        unreal.log_warning(f"  [FAIL] 복사 실패: {source_path}")
        return None

    unreal.log(f"  [COPY] {source_path.split('/')[-1]} → {target_path}")
    return unreal.load_asset(target_path)


# ── 소스 파라미터 덤프 ────────────────────────────────────────────────────────

def dump_source_params(inst_pairs: list, out_dir: Path):
    """소스 MI 파라미터 전체를 JSON으로 저장."""
    data = {}
    for src_inst, _ in inst_pairs:
        entry = {"texture": {}, "vector": {}, "scalar": {}}

        for name in mat_lib.get_texture_parameter_names(src_inst):
            val = mat_lib.get_material_instance_texture_parameter_value(src_inst, str(name))
            entry["texture"][str(name)] = val.get_path_name() if val else None

        for name in mat_lib.get_vector_parameter_names(src_inst):
            val = mat_lib.get_material_instance_vector_parameter_value(src_inst, str(name))
            entry["vector"][str(name)] = {"r": val.r, "g": val.g, "b": val.b, "a": val.a}

        for name in mat_lib.get_scalar_parameter_names(src_inst):
            val = mat_lib.get_material_instance_scalar_parameter_value(src_inst, str(name))
            entry["scalar"][str(name)] = val

        data[src_inst.get_name()] = entry

    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    out_path = out_dir / f"source_params_{timestamp}.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)

    unreal.log(f"  [DUMP] 소스 파라미터 저장: {out_path}")
    return out_path


# ── 파라미터 이전 ─────────────────────────────────────────────────────────────

def build_param_map(master_mat) -> dict:
    """마스터 머티리얼 파라미터명을 기준으로 매핑 딕셔너리를 자동 생성.

    1. 소스와 마스터의 파라미터명이 같으면 → 그대로 매핑 (자동)
    2. 이름이 다르면 → PARAM_MAP fallback (수동 정의)
    반환: {소스_파라미터명: (마스터_파라미터명, 타입)}
    """
    master_tex = {str(n) for n in mat_lib.get_texture_parameter_names(master_mat)}
    master_vec = {str(n) for n in mat_lib.get_vector_parameter_names(master_mat)}
    master_sc  = {str(n) for n in mat_lib.get_scalar_parameter_names(master_mat)}

    result = {}

    # 자동: 마스터 파라미터명과 동일한 것
    for name in master_tex:
        result[name] = (name, "tex")
    for name in master_vec:
        result[name] = (name, "vec")
    for name in master_sc:
        result[name] = (name, "sc")

    # fallback: PARAM_MAP에 정의된 이름 변환 (자동 매핑보다 우선)
    for old_name, new_name in PARAM_MAP.items():
        if new_name in master_tex:
            result[old_name] = (new_name, "tex")
        elif new_name in master_vec:
            result[old_name] = (new_name, "vec")
        elif new_name in master_sc:
            result[old_name] = (new_name, "sc")

    return result


def transfer_parameters(src_inst, dst_inst, param_map: dict):
    """param_map 기준으로 소스 파라미터 값을 읽어 대상 파라미터명에 기록.

    param_map: build_param_map() 반환값 {소스명: (대상명, 타입)}
    """
    src_tex = {str(n) for n in mat_lib.get_texture_parameter_names(src_inst)}
    src_vec = {str(n) for n in mat_lib.get_vector_parameter_names(src_inst)}
    src_sc  = {str(n) for n in mat_lib.get_scalar_parameter_names(src_inst)}

    for old_name, (new_name, ptype) in param_map.items():
        if ptype == "tex" and old_name in src_tex:
            val = mat_lib.get_material_instance_texture_parameter_value(src_inst, old_name)
            if val:
                mat_lib.set_material_instance_texture_parameter_value(dst_inst, new_name, val)
                unreal.log(f"    [TEX ] {old_name} -> {new_name} = {val.get_name()}")
        elif ptype == "vec" and old_name in src_vec:
            val = mat_lib.get_material_instance_vector_parameter_value(src_inst, old_name)
            mat_lib.set_material_instance_vector_parameter_value(dst_inst, new_name, val)
            unreal.log(f"    [VEC ] {old_name} -> {new_name} = {val}")
        elif ptype == "sc" and old_name in src_sc:
            val = mat_lib.get_material_instance_scalar_parameter_value(src_inst, old_name)
            mat_lib.set_material_instance_scalar_parameter_value(dst_inst, new_name, val)
            unreal.log(f"    [SCL ] {old_name} -> {new_name} = {val:.4f}")


# ── 메시 머티리얼 슬롯 재연결 ─────────────────────────────────────────────────

def remap_mesh_materials(mesh, mat_copy_map: dict):
    """메시의 머티리얼 슬롯이 외주사 머티리얼을 참조하면 새 경로 머티리얼로 교체."""
    if not isinstance(mesh, unreal.StaticMesh):
        return

    slots = list(mesh.static_materials)
    changed = False

    for i, slot in enumerate(slots):
        old_mat = slot.material_interface
        if not old_mat:
            continue

        old_path = old_mat.get_path_name()
        new_path = mat_copy_map.get(old_path)
        if not new_path:
            continue

        new_mat = unreal.load_asset(new_path)
        if not new_mat:
            unreal.log_warning(f"    [WARN] 새 머티리얼 로드 실패: {new_path}")
            continue

        slots[i] = unreal.StaticMaterial(
            material_interface=new_mat,
            material_slot_name=slot.material_slot_name,
            uv_channel_data=slot.uv_channel_data,
        )
        changed = True
        unreal.log(f"    [SLOT {i}] {old_path.split('/')[-1]} → {new_path.split('/')[-1]}")

    if changed:
        mesh.set_editor_property("static_materials", slots)


# ── 메인 ──────────────────────────────────────────────────────────────────────

def run_migration():
    SEP = "─" * 60
    unreal.log(SEP)
    unreal.log("[MIGRATION] 외주사 에셋 마이그레이션 시작")
    unreal.log(f"  소스: {SOURCE_PATH}")
    unreal.log(f"  대상: {TARGET_MESH} / {TARGET_MATERIAL} / {TARGET_TEXTURE}")
    unreal.log(SEP)

    # ── 소스 에셋 전체 조회 ──────────────────────────────────
    all_assets = asset_lib.list_assets(SOURCE_PATH, recursive=True, include_folder=False)
    if not all_assets:
        unreal.log_warning("[MIGRATION] 소스 경로에 에셋이 없습니다.")
        return

    unreal.log(f"발견된 에셋: {len(all_assets)}개\n")

    # ── 1단계: 분류 및 복사 ──────────────────────────────────
    unreal.log("[1단계] 에셋 복사")

    mat_copy_map   = {}  # {source_path: target_path} — 메시 슬롯 재연결용
    inst_pairs     = []  # [(src_instance, dst_instance), ...]
    copied_meshes  = []  # [dst_mesh, ...]

    for source_path in all_assets:
        asset_data = asset_lib.find_asset_data(source_path)
        asset_type = classify_asset(str(asset_data.asset_class_path))

        if asset_type == "unknown":
            unreal.log(f"  [SKIP-TYPE] {source_path}")
            continue

        target_path = get_target_path(source_path, asset_type)
        dst_asset   = copy_asset(source_path, target_path)

        if not dst_asset:
            continue

        if asset_type == "mesh":
            copied_meshes.append(dst_asset)

        elif asset_type == "material_instance":
            src_asset = unreal.load_asset(source_path)
            # get_path_name() 형태(패키지명.오브젝트명)와 매칭되도록 full path 키로 저장
            src_full = unreal.load_asset(source_path).get_path_name()
            mat_copy_map[src_full] = target_path
            inst_pairs.append((src_asset, dst_asset))

        elif asset_type == "material":
            src_full = unreal.load_asset(source_path).get_path_name()
            mat_copy_map[src_full] = target_path

    # ── 1.5단계: 소스 파라미터 덤프 ─────────────────────────
    if inst_pairs:
        unreal.log("\n[1.5단계] 소스 파라미터 덤프")
        dump_source_params(inst_pairs, SCRIPT_DIR)

    # ── 2단계: 마스터 머티리얼 교체 + 파라미터 이전 ─────────
    unreal.log(f"\n[2단계] 마스터 머티리얼 교체 + 파라미터 이전 ({len(inst_pairs)}개)")

    master_mat = unreal.load_asset(MASTER_MATERIAL)
    if not master_mat:
        unreal.log_warning(f"[MIGRATION] 마스터 머티리얼을 찾을 수 없습니다: {MASTER_MATERIAL}")
        unreal.log_warning("  2단계를 건너뜁니다. 마스터 머티리얼 경로를 확인하세요.")
    else:
        param_map = build_param_map(master_mat)
        unreal.log(f"  자동 파라미터 매핑: {list(param_map.keys())}")
        for src_inst, dst_inst in inst_pairs:
            if not isinstance(dst_inst, unreal.MaterialInstanceConstant):
                continue
            unreal.log(f"  처리: {dst_inst.get_name()}")
            mat_lib.set_material_instance_parent(dst_inst, master_mat)
            transfer_parameters(src_inst, dst_inst, param_map)
            mat_lib.update_material_instance(dst_inst)

    # ── 3단계: 메시 머티리얼 슬롯 재연결 ────────────────────
    unreal.log(f"\n[3단계] 메시 머티리얼 슬롯 재연결 ({len(copied_meshes)}개)")
    for mesh in copied_meshes:
        unreal.log(f"  메시: {mesh.get_name()}")
        remap_mesh_materials(mesh, mat_copy_map)

    # ── 4단계: 저장 ──────────────────────────────────────────
    unreal.log("\n[4단계] 저장 중...")
    for target_dir in (TARGET_MESH, TARGET_MATERIAL, TARGET_TEXTURE):
        if asset_lib.does_directory_exist(target_dir):
            asset_lib.save_directory(target_dir)

    # ── 결과 요약 ─────────────────────────────────────────────
    unreal.log(f"\n{SEP}")
    unreal.log("[MIGRATION] 완료!")
    unreal.log(f"  메시        {len(copied_meshes)}개 → {TARGET_MESH}")
    unreal.log(f"  머티리얼    {len(inst_pairs)}개 → {TARGET_MATERIAL}")
    unreal.log(f"  파라미터 매핑: {list(PARAM_MAP.keys())}")
    unreal.log(SEP)


run_migration()
