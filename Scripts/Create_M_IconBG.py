"""
M_IconBG 머티리얼(세로 그라데이션, UI 도메인)을 만든다.
이미 있으면 노드를 전부 지우고 다시 만든다. (/Game/LB/UI/M_IconBG)

실행: 에디터 상단 File > Execute Python Script... 로 이 파일 선택
      (또는 Output Log 창에 py "D:/Unreal/LB/Scripts/Create_M_IconBG.py")
필요: Edit > Plugins > "Python Editor Script Plugin" 활성화
"""
import unreal

PACKAGE_PATH = "/Game/LB/UI"
ASSET_NAME = "M_IconBG"

# 위/아래 기본 색 (머티리얼 인스턴스나 에디터에서 ColorTop/ColorBottom 값으로 바꿀 수 있음)
COLOR_TOP = unreal.LinearColor(0.12, 0.30, 0.34, 1.0)
COLOR_BOTTOM = unreal.LinearColor(0.02, 0.05, 0.08, 1.0)

lib = unreal.MaterialEditingLibrary

# 1) 머티리얼 에셋 준비 (있으면 재사용, 없으면 생성)
mat = unreal.EditorAssetLibrary.load_asset(f"{PACKAGE_PATH}/{ASSET_NAME}")
if mat:
    lib.delete_all_material_expressions(mat)
else:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(ASSET_NAME, PACKAGE_PATH, unreal.Material, unreal.MaterialFactoryNew())

mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

# 2) 노드 생성
uv = lib.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 0)

mask = lib.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -650, 0)
mask.set_editor_property("r", False)
mask.set_editor_property("g", True)   # 세로(V) 값만 사용
mask.set_editor_property("b", False)
mask.set_editor_property("a", False)

top = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -650, -250)
top.set_editor_property("parameter_name", "ColorTop")
top.set_editor_property("default_value", COLOR_TOP)

bottom = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -650, 250)
bottom.set_editor_property("parameter_name", "ColorBottom")
bottom.set_editor_property("default_value", COLOR_BOTTOM)

lerp = lib.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -350, 0)

# 3) 연결: UV -> (G만) -> Alpha / ColorTop -> A / ColorBottom -> B / 결과 -> Final Color
lib.connect_material_expressions(uv, "", mask, "")
lib.connect_material_expressions(mask, "", lerp, "Alpha")
lib.connect_material_expressions(top, "", lerp, "A")
lib.connect_material_expressions(bottom, "", lerp, "B")
# UI 도메인에서 Emissive Color 핀의 이름이 "Final Color"
lib.connect_material_property(lerp, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# 4) 컴파일 + 저장
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log(f"[LB] {PACKAGE_PATH}/{ASSET_NAME} 생성 완료")
