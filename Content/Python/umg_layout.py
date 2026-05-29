"""
umg_layout.py — UE5 UMG 풀스크린 패널 UI 레이아웃 빌더 (규칙 캡슐화)

rules/ue5-umg-layout.md 의 범용 규칙을 함수로 구현.
인벤/상점/강화/거래 등 풀스크린 좌우 분할 패널 UI를 동일 패턴으로 구성.

[에디터 Python 콘솔/MCP 사용법]
    import importlib, umg_layout; importlib.reload(umg_layout)
    umg_layout.fix_item_slot()                         # 슬롯 위젯 1회 보정
    umg_layout.build_split_screen("/Game/UI/Enhance/WBP_EnhanceScreenWidget",
                                  left="EnhancePanel", right="InvPanel", title_text="강화")
"""
import unreal

# ── 디자인 팔레트 (RPG_UI_Design.md) ────────────────────────────
COL_BG     = unreal.LinearColor(0.102, 0.102, 0.180, 1.0)  # #1A1A2E 배경
COL_PANEL  = unreal.LinearColor(0.086, 0.129, 0.243, 1.0)  # #16213E 패널
COL_BORDER = unreal.LinearColor(0.059, 0.204, 0.376, 1.0)  # #0F3460 테두리
COL_ACCENT = unreal.LinearColor(0.914, 0.271, 0.376, 1.0)  # #E94560 강조
COL_TEXT   = unreal.LinearColor(0.918, 0.918, 0.918, 1.0)  # #EAEAEA 텍스트
COL_SLOT   = unreal.LinearColor(0.130, 0.180, 0.300, 1.0)  # 빈 슬롯 타일(패널보다 밝게)
WHITE_TEX  = "/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"

ITEM_SLOT_WBP = "/Game/UI/Common/WBP_ItemSlotWidget"
SLOT_SIZE = 90  # 기본 슬롯 한 변(px)


# ── 내부 헬퍼 ───────────────────────────────────────────────────
def _w(wbp_path, name):
    short = wbp_path.split("/")[-1]
    obj = unreal.find_object(None, "{0}.{1}:WidgetTree.{2}".format(wbp_path, short, name))
    return unreal.Widget.cast(obj) if obj else None


def compile_save(wbp_path):
    bp = unreal.EditorAssetLibrary.load_asset(wbp_path)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_asset(wbp_path)


# ── 슬롯 레이아웃 (규칙 0: 실측/명시) ───────────────────────────
def set_canvas_slot(wbp_path, name, amin, amax, off, align, autosize=False, z=0):
    """CanvasPanelSlot 전체 설정. off=(Left,Top,Right=W,Bottom=H), align=앵커점 기준점."""
    w = _w(wbp_path, name)
    s = unreal.CanvasPanelSlot.cast(w.slot)
    lt = s.get_editor_property("layout_data"); an = lt.anchors
    an.minimum = unreal.Vector2D(amin[0], amin[1]); an.maximum = unreal.Vector2D(amax[0], amax[1])
    lt.anchors = an
    lt.offsets = unreal.Margin(off[0], off[1], off[2], off[3])
    lt.alignment = unreal.Vector2D(align[0], align[1])
    s.set_editor_property("layout_data", lt)
    s.set_editor_property("auto_size", autosize)
    s.set_editor_property("z_order", z)


def set_hbox_fill(wbp_path, name, fill=1.0):
    """HorizontalBox 자식을 Fill 비율로 (규칙 3)."""
    s = unreal.HorizontalBoxSlot.cast(_w(wbp_path, name).slot)
    sz = s.get_editor_property("size"); sz.size_rule = unreal.SlateSizeRule.FILL; sz.value = fill
    s.set_editor_property("size", sz)
    s.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
    s.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)


# ── 브러시/텍스트 (규칙 4,5) ────────────────────────────────────
def set_image(wbp_path, name, color, tex=WHITE_TEX):
    """Image는 텍스처가 있어야 TintColor 렌더됨(규칙 4)."""
    im = unreal.Image.cast(_w(wbp_path, name))
    br = im.get_editor_property("brush")
    if tex:
        br.set_editor_property("resource_object", unreal.load_object(None, tex))
        br.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    sc = br.tint_color; sc.specified_color = color; br.tint_color = sc
    im.set_editor_property("brush", br)


def set_text(wbp_path, name, text=None, size=None, color=None, center=False):
    t = unreal.TextBlock.cast(_w(wbp_path, name))
    if text is not None:
        t.set_text(text)
    if size is not None:
        f = t.get_editor_property("font"); f.set_editor_property("size", size)
        t.set_editor_property("font", f)
    if color is not None:
        t.set_color_and_opacity(unreal.SlateColor(color))
    if center:
        t.set_editor_property("justification", unreal.TextJustify.CENTER)


# ── 고수준 빌더 ─────────────────────────────────────────────────
def fullscreen_overlay(wbp_path, name="BackgroundOverlay", color=COL_BG):
    """규칙 4: 불투명 풀스크린 딤."""
    set_canvas_slot(wbp_path, name, (0, 0), (1, 1), (0, 0, 0, 0), (0, 0), False, 0)
    set_image(wbp_path, name, color)


def panel_box(wbp_path, name, w, h, color=COL_PANEL, z=0):
    """규칙 3: 패널 영역 정의 배경 박스(화면 중앙)."""
    set_canvas_slot(wbp_path, name, (0.5, 0.5), (0.5, 0.5), (0, 0, w, h), (0.5, 0.5), False, z)
    set_image(wbp_path, name, color)


def center_widget(wbp_path, name, w, h, dx=0, dy=0, z=1, ax=0.5, ay=0.5):
    """규칙 3: 패널 중앙 기준 상대 배치."""
    set_canvas_slot(wbp_path, name, (ax, ay), (ax, ay), (dx, dy, w, h), (0.5, 0.5), False, z)


def grid_cells(wbp_path, grid_name, size=SLOT_SIZE, pad=2):
    """규칙 1: UniformGridPanel 셀 최소 크기 강제."""
    gp = unreal.UniformGridPanel.cast(_w(wbp_path, grid_name))
    gp.set_editor_property("min_desired_slot_width", float(size))
    gp.set_editor_property("min_desired_slot_height", float(size))
    gp.set_editor_property("slot_padding", unreal.Margin(pad, pad, pad, pad))


def fix_item_slot(wbp_path=ITEM_SLOT_WBP):
    """규칙 1: 슬롯 배경 fill+z최하, 아이콘 위, 수량/강화 텍스트 모서리."""
    set_canvas_slot(wbp_path, "RarityBorder", (0, 0), (1, 1), (0, 0, 0, 0), (0, 0), False, 0)
    set_canvas_slot(wbp_path, "IconWidget",   (0, 0), (1, 1), (4, 4, 4, 4), (0, 0), False, 1)
    set_canvas_slot(wbp_path, "QuantityText", (1, 1), (1, 1), (-2, -2, 36, 16), (1, 1), False, 2)
    set_canvas_slot(wbp_path, "EnhanceText",  (0, 1), (0, 1), (2, -2, 30, 16), (0, 1), False, 2)
    unreal.Border.cast(_w(wbp_path, "RarityBorder")).set_editor_property("brush_color", COL_SLOT)
    compile_save(wbp_path)


def build_split_screen(wbp_path, left, right, title=None, title_text="",
                       overlay="BackgroundOverlay", main="MainPanel",
                       close_btn="CloseBtn", close_text="CloseText",
                       left_fill=1.0, right_fill=1.0):
    """풀스크린 좌우 분할 컨테이너 표준 배치 (인벤/상점/강화/거래 공통)."""
    fullscreen_overlay(wbp_path, overlay)
    set_canvas_slot(wbp_path, main, (0, 0), (1, 1), (40, 70, 40, 40), (0, 0), False, 0)
    set_hbox_fill(wbp_path, left, left_fill)
    set_hbox_fill(wbp_path, right, right_fill)
    if title:
        set_canvas_slot(wbp_path, title, (0.5, 0), (0.5, 0), (0, 18, 500, 40), (0.5, 0), False, 1)
        set_text(wbp_path, title, title_text, size=26, color=COL_TEXT, center=True)
    set_canvas_slot(wbp_path, close_btn, (1, 0), (1, 0), (-44, 8, 40, 40), (1, 0), False, 2)
    if _w(wbp_path, close_text):
        set_text(wbp_path, close_text, "X", size=20, color=COL_TEXT, center=True)
    compile_save(wbp_path)
