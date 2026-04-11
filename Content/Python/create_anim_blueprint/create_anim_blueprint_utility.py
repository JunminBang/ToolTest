"""
create_anim_blueprint_utility.py

Editor Utility Blueprint의 Execute Python Script 노드에서 호출:
    import importlib, create_anim_blueprint_utility
    importlib.reload(create_anim_blueprint_utility)
    create_anim_blueprint_utility.run()

동작:
  - 선택한 Skeletal Mesh에서 skirt_1 ~ skirt_6 본 중 가장 높은 번호를 찾아
    대응하는 ABP_Test_Template_N 을 스켈레탈 메시 폴더에 ABP_<메시이름> 으로 복제.
  - 복제된 ABP에 스켈레톤 할당, 템플릿 플래그 해제, 프리뷰 메시 지정 후 저장.
"""

import unreal

TEMPLATE_FOLDER = "/Game/Characters/Mannequins"
SKIRT_BONE_COUNT = 6  # skirt_1 ~ skirt_6


def _get_skirt_level(skeletal_mesh) -> int:
    """skirt_1 ~ skirt_6 중 존재하는 가장 높은 번호를 반환. 없으면 0."""
    for i in range(SKIRT_BONE_COUNT, 0, -1):
        if unreal.AnimToolsBlueprintLibrary.has_bone(skeletal_mesh, f"skirt_{i}"):
            return i
    return 0


def run():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    selected_assets = unreal.EditorUtilityLibrary.get_selected_assets()

    if not selected_assets:
        unreal.log_warning("[ABP 생성] 선택된 에셋이 없습니다.")
        return

    success_count = 0
    skip_count = 0

    for asset in selected_assets:
        if not isinstance(asset, unreal.SkeletalMesh):
            unreal.log_warning(f"[ABP 생성] '{asset.get_name()}'은(는) Skeletal Mesh가 아닙니다. 건너뜁니다.")
            skip_count += 1
            continue

        mesh_name = asset.get_name()
        skeleton = asset.skeleton

        if skeleton is None:
            unreal.log_warning(f"[ABP 생성] '{mesh_name}'에 Skeleton이 없습니다. 건너뜁니다.")
            skip_count += 1
            continue

        # skirt 본 레벨 확인
        skirt_level = _get_skirt_level(asset)
        if skirt_level == 0:
            unreal.log_warning(f"[ABP 생성] '{mesh_name}'에 skirt 본(skirt_1~skirt_{SKIRT_BONE_COUNT})이 없습니다. 건너뜁니다.")
            skip_count += 1
            continue

        # 대응하는 템플릿 로드
        template_path = f"{TEMPLATE_FOLDER}/ABP_Test_Template_{skirt_level}"
        source_abp = unreal.load_asset(template_path)
        if source_abp is None:
            unreal.log_error(f"[ABP 생성] 템플릿을 찾을 수 없습니다: {template_path}")
            skip_count += 1
            continue

        # 스켈레탈 메시 폴더에 복제
        mesh_folder = "/".join(asset.get_path_name().split("/")[:-1])
        new_abp_name = f"ABP_{mesh_name}"
        full_dest_path = f"{mesh_folder}/{new_abp_name}"

        if unreal.EditorAssetLibrary.does_asset_exist(full_dest_path):
            unreal.log_warning(f"[ABP 생성] '{full_dest_path}' 이미 존재합니다. 건너뜁니다.")
            skip_count += 1
            continue

        duplicated = asset_tools.duplicate_asset(new_abp_name, mesh_folder, source_abp)
        if duplicated is None:
            unreal.log_error(f"[ABP 생성] '{new_abp_name}' 복제 실패.")
            skip_count += 1
            continue

        try:
            duplicated.set_editor_property("target_skeleton", skeleton)
            unreal.AnimToolsBlueprintLibrary.setup_duplicated_anim_blueprint(duplicated, asset)
            unreal.EditorAssetLibrary.save_asset(duplicated.get_path_name(), only_if_is_dirty=False)
        except Exception as e:
            unreal.log_error(f"[ABP 생성] 설정 중 오류 발생, 생성된 에셋을 삭제합니다: {e}")
            unreal.EditorAssetLibrary.delete_asset(full_dest_path)
            skip_count += 1
            continue

        unreal.log(f"[ABP 생성] 완료: {full_dest_path}  (템플릿: ABP_Test_Template_{skirt_level}, Skeleton: {skeleton.get_name()})")
        success_count += 1

    unreal.log(f"[ABP 생성] 처리 완료 — 성공: {success_count}, 건너뜀: {skip_count}")
