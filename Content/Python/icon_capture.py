"""
icon_capture.py
  SKnight_modular의 armor/weapon 스켈레탈 메시를 아이콘 PNG로 일괄 캡처.
  - 빈 L_IconCapture 레벨에서 캡처(메인 레벨 금지: PostProcess/대기가 노출 망침).
  - 검정 배경 + 검정 키잉(머티리얼 불필요). 텍스처 강제 로드로 색 정상 출력.
  - 순수 Python zlib PNG 인코딩. PNG는 Saved/IconPreview/Icons/ 에 저장 → 수동 드래그&드롭 import.
사용:
  import icon_capture, importlib; importlib.reload(icon_capture)
  icon_capture.list_targets()                 # (path, name) 목록
  icon_capture.capture_chunk(0, 10)           # 0~9번 캡처
"""
import unreal, math, zlib, struct

LEVEL = "/Game/Editor/Maps/L_IconCapture"
RT_PATH = "/Game/Editor/RenderTargets/RT_IconCapture"
SAVE_DIR = None  # 기본: 프로젝트 Saved/IconPreview/Icons/
W = H = 256


def _save_dir():
    return SAVE_DIR or (unreal.Paths.project_saved_dir() + "IconPreview/Icons/")


def list_targets():
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    f = unreal.ARFilter(class_names=["SkeletalMesh"], package_paths=["/Game/SKnight_modular"], recursive_paths=True)
    out = []
    for a in ar.get_assets(f):
        pn = str(a.package_name)
        low = pn.lower()
        if "_skeleton" in low or "physicsasset" in low:
            continue
        if "/armor/" in low or "/weapon/" in low:
            out.append((pn, pn.split("/")[-1]))
    out.sort(key=lambda x: x[1].lower())
    return out


def _write_png(path, w, h, rgba):
    sig = b"\x89PNG\r\n\x1a\n"
    def ch(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw.extend(rgba[y * w * 4:(y + 1) * w * 4])
    with open(path, "wb") as fp:
        fp.write(sig + ch(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) + ch(b"IDAT", zlib.compress(bytes(raw), 9)) + ch(b"IEND", b""))


def _cl(v, a, b):
    return a if v < a else (b if v > b else v)


def _autocrop(rgba, w, h, margin=0.06):
    """불투명 픽셀 bbox를 찾아 잘라내고, 여백 margin만 남기고 중앙에 꽉 차게 재배치(nearest-neighbor 확대)."""
    minx = w; miny = h; maxx = -1; maxy = -1
    for y in range(h):
        base = y * w * 4
        for x in range(w):
            if rgba[base + x * 4 + 3] > 16:
                if x < minx: minx = x
                if x > maxx: maxx = x
                if y < miny: miny = y
                if y > maxy: maxy = y
    if maxx < minx:
        return rgba  # 빈 이미지
    bw = maxx - minx + 1
    bh = maxy - miny + 1
    target = int(w * (1.0 - 2.0 * margin))
    s = min(target / float(bw), target / float(bh))
    nw = max(1, int(bw * s)); nh = max(1, int(bh * s))
    ox = (w - nw) // 2; oy = (h - nh) // 2
    out = bytearray(w * h * 4)  # 전부 투명
    for y in range(nh):
        sy = miny + int(y / s)
        if sy > maxy: sy = maxy
        drow = (oy + y) * w * 4
        srow = sy * w * 4
        for x in range(nw):
            sx = minx + int(x / s)
            if sx > maxx: sx = maxx
            si = srow + sx * 4
            di = drow + (ox + x) * 4
            out[di] = rgba[si]; out[di + 1] = rgba[si + 1]; out[di + 2] = rgba[si + 2]; out[di + 3] = rgba[si + 3]
    return out


def _ensure_level():
    ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if ues.get_editor_world().get_name() != "L_IconCapture":
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL)
    # 누수 액터 정리
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() in ("SkeletalMeshActor", "SceneCapture2D", "StaticMeshActor"):
            eas.destroy_actor(a)


def capture_chunk(start, count):
    import os
    _ensure_level()
    sd = _save_dir()
    if not os.path.isdir(sd):
        os.makedirs(sd)
    targets = list_targets()[start:start + count]
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "r.Streaming.FullyLoadUsedTextures 1")
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    rt = unreal.load_asset(RT_PATH)
    rt.set_editor_property("clear_color", unreal.LinearColor(0, 0, 0, 1))

    done = []
    for path, name in targets:
        mesh = unreal.load_asset(path)
        if mesh is None:
            continue
        ma = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        smc = ma.skeletal_mesh_component
        smc.set_skeletal_mesh_asset(mesh)
        try:
            smc.set_forced_lod(1)
            smc.prestream_textures(5.0, True, 0)
        except Exception:
            pass
        origin, extent = ma.get_actor_bounds(False)
        radius = math.sqrt(extent.x ** 2 + extent.y ** 2 + extent.z ** 2) or 50.0
        fov = 45.0
        dist = radius * 2.5
        dl = math.sqrt(1 + 1 + 0.16)
        d = unreal.Vector(1 / dl, 1 / dl, 0.4 / dl)
        cam = unreal.Vector(origin.x + d.x * dist, origin.y + d.y * dist, origin.z + d.z * dist)
        look = unreal.MathLibrary.find_look_at_rotation(cam, origin)
        light = eas.spawn_actor_from_class(unreal.DirectionalLight, cam, look)
        light.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(6.0)
        ca = eas.spawn_actor_from_class(unreal.SceneCapture2D, cam, look)
        cc = ca.capture_component2d
        cc.set_editor_property("texture_target", rt)
        cc.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
        cc.set_editor_property("fov_angle", fov)
        cc.show_only_actor_components(ma)
        flags = []
        for nm in ["Atmosphere", "Fog", "VolumetricFog", "Bloom", "LensFlares"]:
            fs = unreal.EngineShowFlagsSetting()
            fs.set_editor_property("show_flag_name", nm)
            fs.set_editor_property("enabled", False)
            flags.append(fs)
        cc.set_editor_property("show_flag_settings", flags)
        pp = unreal.PostProcessSettings()
        for k, v in [("override_auto_exposure_min_brightness", True), ("auto_exposure_min_brightness", 0.3),
                     ("override_auto_exposure_max_brightness", True), ("auto_exposure_max_brightness", 0.3),
                     ("override_auto_exposure_bias", True), ("auto_exposure_bias", 1.0),
                     ("override_vignette_intensity", True), ("vignette_intensity", 0.0)]:
            pp.set_editor_property(k, v)
        cc.set_editor_property("post_process_settings", pp)
        cc.capture_scene()
        cc.capture_scene()  # 텍스처 로드 후 2차
        px = unreal.RenderingLibrary.read_render_target(world, rt, True)
        rgba = bytearray(W * H * 4)
        for i, c in enumerate(px):
            r, g, b = c.r, c.g, c.b
            mx = r if r >= g and r >= b else (g if g >= b else b)
            a = int(_cl(mx * 7, 0, 255)) if mx < 37 else 255
            j = i * 4
            rgba[j] = r; rgba[j + 1] = g; rgba[j + 2] = b; rgba[j + 3] = a
        rgba = _autocrop(rgba, W, H)
        _write_png(sd + "T_Icon_" + name + ".png", W, H, rgba)
        eas.destroy_actor(ma); eas.destroy_actor(ca); eas.destroy_actor(light)
        done.append(name)
    return done
