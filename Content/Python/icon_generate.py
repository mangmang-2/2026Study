"""
icon_generate.py  —  EUW_IconGenerator [전체 생성] 버튼에서 호출
py "icon_generate.py"
"""
import unreal
import json
import os


def _get_euw_widget():
    subsystem = unreal.get_editor_subsystem(unreal.EditorUtilitySubsystem)
    bp = unreal.load_asset("/Game/Editor/Widgets/EUW_IconGenerator")
    return subsystem.find_utility_widget_from_blueprint(bp)


def _update_ui(widget, progress: float, text: str):
    if widget is None:
        return
    bar = widget.get_widget_from_name("GenerateProgress")
    status = widget.get_widget_from_name("StatusText")
    if bar:
        bar.set_percent(progress)
    if status:
        status.set_text(unreal.Text(text))


def run():
    widget = _get_euw_widget()

    # 저장 경로 읽기
    save_input = widget.get_widget_from_name("SavePathInput") if widget else None
    save_path = save_input.get_text().to_string() if save_input else "/Game/Textures/Icons/"
    if not save_path:
        save_path = "/Game/Textures/Icons/"

    # 스캔 결과 불러오기
    result_path = unreal.Paths.project_saved_dir() + "IconGen_ScanResult.json"
    if not os.path.exists(result_path):
        unreal.log_error("[IconGen] 먼저 [스캔]을 실행하세요.")
        _update_ui(widget, 0.0, "오류: 먼저 스캔을 실행하세요")
        return

    with open(result_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    paths = data.get("paths", [])
    mesh_type = data.get("mesh_type", "Skeletal")

    if not paths:
        unreal.log_warning("[IconGen] 스캔 결과가 없습니다.")
        _update_ui(widget, 0.0, "스캔 결과 없음 — 경로를 확인하세요")
        return

    # RenderTarget 로드
    rt = unreal.load_asset("/Game/Editor/RenderTargets/RT_IconCapture")
    if rt is None:
        unreal.log_error("[IconGen] RT_IconCapture 없음")
        return

    # BP_IconCaptureRig 액터 찾기
    rig_class = unreal.load_class(None, "/Game/Editor/Blueprints/BP_IconCaptureRig.BP_IconCaptureRig_C")
    editor_world = unreal.UnrealEditorSubsystem().get_editor_world()
    rigs = unreal.GameplayStatics.get_all_actors_of_class(editor_world, rig_class)
    if not rigs:
        unreal.log_error("[IconGen] 레벨에 BP_IconCaptureRig 없음. L_IconCapture 레벨을 열고 배치하세요.")
        _update_ui(widget, 0.0, "오류: CaptureRig 없음 — L_IconCapture 레벨 확인")
        return

    rig = rigs[0]
    skel_comp = rig.get_component_by_class(unreal.SkeletalMeshComponent)
    stat_comp = rig.get_component_by_class(unreal.StaticMeshComponent)
    capture_comp = rig.get_component_by_class(unreal.SceneCaptureComponent2D)

    total = len(paths)
    _update_ui(widget, 0.0, f"생성 시작: 총 {total}개")

    for i, pkg_path in enumerate(paths):
        asset_name = pkg_path.split("/")[-1]
        full_asset_path = pkg_path + "." + asset_name

        # 메시 로드 및 컴포넌트 설정
        mesh = unreal.load_asset(full_asset_path)
        if mesh is None:
            unreal.log_warning(f"[IconGen] 메시 로드 실패: {full_asset_path}")
            continue

        if mesh_type == "Skeletal" and skel_comp:
            skel_comp.set_skeletal_mesh_asset(mesh)
            skel_comp.set_visibility(True)
            if stat_comp:
                stat_comp.set_visibility(False)
        elif stat_comp:
            stat_comp.set_static_mesh(mesh)
            stat_comp.set_visibility(True)
            if skel_comp:
                skel_comp.set_visibility(False)

        # 캡처
        if capture_comp:
            capture_comp.capture_scene()

        # RenderTarget → Texture2D 저장
        texture_save_path = save_path.rstrip("/") + "/T_Icon_" + asset_name
        try:
            unreal.RenderingLibrary.render_target_create_static_texture2d_editor_only(
                rt, texture_save_path
            )
            unreal.EditorAssetLibrary.save_asset(texture_save_path)
        except Exception as e:
            unreal.log_warning(f"[IconGen] 저장 실패 {asset_name}: {e}")

        progress = (i + 1) / total
        _update_ui(widget, progress, f"저장 중: {asset_name} ({i+1}/{total})")
        unreal.log(f"[IconGen] {i+1}/{total} — {asset_name}")

    _update_ui(widget, 1.0, f"완료! 총 {total}개 저장됨 → {save_path}")
    unreal.log(f"[IconGen] 전체 생성 완료: {total}개")


run()
