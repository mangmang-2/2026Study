"""
icon_chroma_key.py
  RT_IconCapture(마젠타 배경으로 캡처된 렌더타깃)에서 배경색을 키잉해 투명 PNG로 만들고
  /Game/Textures/Icons/T_Icon_<name> 텍스처로 import 한다.

  - 엔진 알파(PropagateAlpha/SceneColorHDR)는 Substrate에서 커버리지로 안 나오므로
    "특이한 배경색(마젠타) + 색상 거리 키잉" 방식을 쓴다.
  - 배경색은 4모서리 픽셀 평균을 샘플링 → 톤매핑으로 변색돼도 자동 대응.
  - 순수 Python(zlib)으로 RGBA PNG 인코딩 (UE Python에 PIL/numpy 없음).

사용:
  import icon_chroma_key
  icon_chroma_key.key_rt_to_texture("SK_weapon_01", "/Game/Textures/Icons")
"""
import unreal
import zlib
import struct

RT_PATH = "/Game/Editor/RenderTargets/RT_IconCapture"
TOL = 60.0    # 배경색과의 맨해튼 거리 이하 → 완전 투명
RAMP = 70.0   # TOL~TOL+RAMP 사이 → 알파 보간(에지 AA)


def _clamp(v, lo, hi):
    return lo if v < lo else (hi if v > hi else v)


def _write_png(path, w, h, rgba):
    sig = b"\x89PNG\r\n\x1a\n"

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)  # 8bit RGBA
    raw = bytearray()
    for y in range(h):
        raw.append(0)  # filter type 0
        raw.extend(rgba[y * w * 4:(y + 1) * w * 4])
    with open(path, "wb") as f:
        f.write(sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))


def key_rt_to_texture(mesh_name, save_dir="/Game/Textures/Icons"):
    """현재 RT_IconCapture 내용을 키잉 → T_Icon_<mesh_name> 텍스처 생성. 텍스처 경로 반환."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    rt = unreal.load_asset(RT_PATH)
    if rt is None:
        unreal.log_error("[IconKey] RT_IconCapture 없음")
        return None

    w = int(rt.get_editor_property("size_x"))
    h = int(rt.get_editor_property("size_y"))
    px = unreal.RenderingLibrary.read_render_target(world, rt, True)  # BGRA sRGB, per-pixel

    # 배경색 = 4모서리 평균
    cs = [px[0], px[w - 1], px[(h - 1) * w], px[h * w - 1]]
    br = sum(c.r for c in cs) // 4
    bg = sum(c.g for c in cs) // 4
    bb = sum(c.b for c in cs) // 4

    rgba = bytearray(w * h * 4)
    for i, c in enumerate(px):
        r, g, b = c.r, c.g, c.b
        dist = abs(r - br) + abs(g - bg) + abs(b - bb)
        a = int(_clamp((dist - TOL) / RAMP, 0.0, 1.0) * 255)
        # 마젠타(또는 r,b가 g보다 높은) 프린지 디스필 — 에지에서만
        if a < 250 and r > g + 20 and b > g + 20:
            cap = g + 25
            r = _clamp(r, 0, cap)
            b = _clamp(b, 0, cap)
        j = i * 4
        rgba[j] = r
        rgba[j + 1] = g
        rgba[j + 2] = b
        rgba[j + 3] = a

    png = unreal.Paths.project_saved_dir() + "IconPreview/_key_%s.png" % mesh_name
    _write_png(png, w, h, rgba)

    # import as Texture2D — 안전하게: 기존 삭제 안 함, save=False(별도 저장)
    tex_path = save_dir.rstrip("/") + "/T_Icon_" + mesh_name
    task = unreal.AssetImportTask()
    task.filename = png
    task.destination_path = save_dir.rstrip("/")
    task.destination_name = "T_Icon_" + mesh_name
    task.automated = True
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    tex = unreal.load_asset(tex_path)
    if tex:
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    unreal.log("[IconKey] 생성: %s (bg=%d,%d,%d)" % (tex_path, br, bg, bb))
    return tex_path
