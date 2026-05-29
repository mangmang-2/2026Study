"""
icon_scan.py  —  EUW_IconGenerator [스캔] 버튼에서 호출
py "icon_scan.py"
"""
import unreal
import json
import os


def _get_euw_widget():
    subsystem = unreal.get_editor_subsystem(unreal.EditorUtilitySubsystem)
    bp = unreal.load_asset("/Game/Editor/Widgets/EUW_IconGenerator")
    return subsystem.find_utility_widget_from_blueprint(bp)


def run():
    widget = _get_euw_widget()
    if widget is None:
        unreal.log_error("[IconGen] EUW 위젯 인스턴스를 찾을 수 없습니다.")
        return

    # 입력값 읽기
    folder_input = widget.get_widget_from_name("FolderPathInput")
    combo = widget.get_widget_from_name("MeshTypeCombo")
    status = widget.get_widget_from_name("StatusText")
    list_box = widget.get_widget_from_name("MeshListBox")

    folder_path = folder_input.get_text().to_string() if folder_input else "/Game/Meshes"
    mesh_type = combo.get_selected_option() if combo else "Skeletal"

    if not folder_path:
        folder_path = "/Game/Meshes"

    # AssetRegistry 검색
    registry = unreal.AssetRegistryHelpers.get_asset_registry()

    if mesh_type == "Skeletal":
        class_paths = [unreal.TopLevelAssetPath("/Script/Engine", "SkeletalMesh")]
    else:
        class_paths = [unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")]

    ar_filter = unreal.ARFilter(
        class_paths=class_paths,
        package_paths=[folder_path],
        recursive_paths=True
    )
    assets = registry.get_assets(ar_filter)
    paths = [str(a.package_name) for a in assets]

    # 결과 저장 (generate 버튼이 읽어감)
    result_path = unreal.Paths.project_saved_dir() + "IconGen_ScanResult.json"
    with open(result_path, "w", encoding="utf-8") as f:
        json.dump({"paths": paths, "count": len(paths), "mesh_type": mesh_type}, f)

    # MeshListBox 갱신
    if list_box:
        list_box.clear_children()

    # StatusText 갱신
    if status:
        status.set_text(unreal.Text(f"총 {len(paths)}개 메시 발견"))

    unreal.log(f"[IconGen] 스캔 완료: {len(paths)}개 ({folder_path}, {mesh_type})")


run()
