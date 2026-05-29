import fontforge
import locale

# ========================================================
# 纵向以及纵横居中插件
# ========================================================
def process_glyph_centering(font_obj, glyph_name, center_horizontal=True):
    try:
        glyph = font_obj[glyph_name]
        
        # 1. 在移动前，存下这个字形原本宽高
        glyph_width = glyph.width
        glyph_height = glyph.vwidth
        
        if glyph_height <= 0:
            glyph_height = font_obj.ascent + font_obj.descent
        
        # 2. 纯骨架顶点提取
        real_xmin = None
        real_xmax = None
        real_ymin = None
        real_ymax = None
        
        for layer_idx in range(len(glyph.layers)):
            try:
                contours = glyph.layers[layer_idx]
                for contour in contours:
                    for point in contour:
                        if point.on_curve:
                            px = point.x
                            py = point.y
                            if real_xmin is None or px < real_xmin: real_xmin = px
                            if real_xmax is None or px > real_xmax: real_xmax = px
                            if real_ymin is None or py < real_ymin: real_ymin = py
                            if real_ymax is None or py > real_ymax: real_ymax = py
            except:
                continue
        
        if real_xmin is None:
            return False
            
        outline_width = real_xmax - real_xmin
        outline_height = real_ymax - real_ymin
        
        # 3. 动态专属垂直中心算法
        GLYPH_CENTER_Y = font_obj.ascent - (glyph_height / 2.0)
        current_glyph_center_y = (real_ymin + real_ymax) / 2.0
        y_offset = GLYPH_CENTER_Y - current_glyph_center_y
        
        # 4. 水平纯骨架居中算法
        x_offset = 0.0
        if center_horizontal:
            perfect_h_margin = (glyph_width - outline_width) / 2.0
            x_offset = perfect_h_margin - real_xmin
            
        # 5. 执行物理平移
        glyph.transform((1.0, 0.0, 0.0, 1.0, x_offset, y_offset))
        
        # 6. 写入原本文字宽高，防止缩水
        glyph.width = glyph_width
        glyph.vwidth = glyph_height
        
        glyph.changed()
        return True
    except:
        return False

# ========================================================
# 功能一：纵横选项绑定的触发函数（应为"Ctrl+Numpad5")
# ========================================================
def do_center_both_directions(menu_obj, font_obj):
    if font_obj is None: return
    for name in font_obj.selection:
        process_glyph_centering(font_obj, name, center_horizontal=True)

# ========================================================
# 功能二：纵向选项绑定的触发函数（应为"Ctrl+Numpad0")
# ========================================================
def do_center_only_vertically(menu_obj, font_obj):
    if font_obj is None: return
    for name in font_obj.selection:
        process_glyph_centering(font_obj, name, center_horizontal=False)

# ========================================================
# 【语言文本】
# ========================================================
try:
    # 自动获取操作系统的语言环境编码 (如 zh_CN、en_US)
    sys_lang, _ = locale.getdefaultlocale()
except:
    sys_lang = "en"

# 根据系统语言环境，命名菜单选项
if sys_lang and sys_lang.startswith("zh"):
    name_both = "纵横居中" #选项名称
    name_vertical = "纵向居中" #选项名称
    menu_tools = "文字居中"  #菜单名称
else:
    name_both = "Center in Height & Width"
    name_vertical = "Center in Height"
    menu_tools = "Center Glyphs"

# ========================================================
# 【二项注册】
# 语言变量
# ========================================================
fontforge.registerMenuItem(do_center_both_directions, None, None, "Font", None, menu_tools, name_both)
fontforge.registerMenuItem(do_center_only_vertically, None, None, "Font", None, menu_tools, name_vertical)
# ========================================================
#制作：留幺
