import fontforge
import locale

# ========================================================
# 居中居周处理函数（字形极点居中，字形格局居周）
# ========================================================
def process_glyph_alignment(font_obj, glyph_name, align_type="center_both"):
    try:
        glyph = font_obj[glyph_name]
        
        #1. 存下字形原本宽高，防止意外变形
        glyph_width = glyph.width
        glyph_height = glyph.vwidth
        if glyph_height <= 0:
            glyph_height = font_obj.ascent + font_obj.descent
            
        #2. 提取两种界限，居中别于居周
        #【居中方法】：字形极点
        extrema_xmin = None
        extrema_xmax = None
        extrema_ymin = None
        extrema_ymax = None
        
        for layer_idx in range(len(glyph.layers)):
            try:
                contours = glyph.layers[layer_idx]
                for contour in contours:
                    for point in contour:
                        if point.on_curve:  #立定四极
                            px = point.x
                            py = point.y
                            if extrema_xmin is None or px < extrema_xmin: extrema_xmin = px
                            if extrema_xmax is None or px > extrema_xmax: extrema_xmax = px
                            if extrema_ymin is None or py < extrema_ymin: extrema_ymin = py
                            if extrema_ymax is None or py > extrema_ymax: extrema_ymax = py
            except:
                continue

        #【居周方法】：字形格局
        bbox = glyph.boundingBox()
        bbox_xmin, bbox_ymin, bbox_xmax, bbox_ymax = bbox
        
        #若为空白字形，则不处理
        if bbox_xmin == 0 and bbox_xmax == 0 and bbox_ymin == 0 and bbox_ymax == 0 and glyph.layers == ():
            return False
            
        #若未取得字形极点，退回字形格局
        if extrema_xmin is None:
            extrema_xmin, extrema_ymin, extrema_xmax, extrema_ymax = bbox_xmin, bbox_ymin, bbox_xmax, bbox_ymax

        #3. 计算所需从横目标位置
        GLYPH_CENTER_Y = font_obj.ascent - (glyph_height / 2.0)
        GLYPH_TOP_Y = font_obj.ascent
        GLYPH_BOTTOM_Y = font_obj.ascent - glyph_height
        
        x_offset = 0.0
        y_offset = 0.0
        
        # --- 从向计算 (Y轴) ---
        if align_type in ["center_both", "center_vertical", "center_left", "center_right"]:
            # 居中故需极点相间，以处当中
            current_center_y = (extrema_ymin + extrema_ymax) / 2.0
            y_offset = GLYPH_CENTER_Y - current_center_y
        elif align_type in ["top", "top_left", "top_right", "center_top"]:
            # 居周故需字形格局，以不出局
            y_offset = GLYPH_TOP_Y - bbox_ymax
        elif align_type in ["bottom", "bottom_left", "bottom_right", "center_bottom"]:
            # 居周故需字形格局，以不出局
            y_offset = GLYPH_BOTTOM_Y - bbox_ymin
            
        # --- 横向计算 (X轴) ---
        if align_type in ["center_both", "center_horizontal", "center_top", "center_bottom"]:
            # 居中故需极点，防止左多右少
            outline_width = extrema_xmax - extrema_xmin
            perfect_h_margin = (glyph_width - outline_width) / 2.0
            x_offset = perfect_h_margin - extrema_xmin
        elif align_type in ["left", "top_left", "bottom_left", "center_left"]:
            # 居左故需字形格局
            x_offset = 0.0 - bbox_xmin
        elif align_type in ["right", "top_right", "bottom_right", "center_right"]:
            # 居右故需字形格局
            x_offset = glyph_width - bbox_xmax

        # 4. 执行物理平移
        glyph.transform((1.0, 0.0, 0.0, 1.0, x_offset, y_offset))
        
        # 5. 还原原本文字宽高，防止字形意外缩水
        glyph.width = glyph_width
        glyph.vwidth = glyph_height
        
        glyph.changed()
        return True
    except:
        return False

# ========================================================
# 识别Font以及Char窗口传参
# ========================================================
def run_alignment(menu_obj, passed_obj, align_type):
    if passed_obj is None: return
    
    # 检测：若传Glyph对象（说明是从Charview编辑窗口点击的菜单）
    if hasattr(passed_obj, 'glyphname'):
        real_glyph = passed_obj
        real_font = real_glyph.font  # 反向获取字形所属的字体对象
        process_glyph_alignment(real_font, real_glyph.glyphname, align_type)
        
    # 检测：若传Font对象（说明是从Fontview字体主列表点击的菜单）
    elif hasattr(passed_obj, 'selection'):
        real_font = passed_obj
        for name in real_font.selection: 
            process_glyph_alignment(real_font, name, align_type)

# 各个方向选项绑定触发函数
def do_center_both(m, p): run_alignment(m, p, "center_both")
def do_center_vertical(m, p): run_alignment(m, p, "center_vertical")
def do_center_horizontal(m, p): run_alignment(m, p, "center_horizontal")
def do_align_center_top(m, p): run_alignment(m, p, "center_top")       
def do_align_center_bottom(m, p): run_alignment(m, p, "center_bottom") 
def do_align_center_left(m, p): run_alignment(m, p, "center_left")
def do_align_center_right(m, p): run_alignment(m, p, "center_right")
def do_align_top_left(m, p): run_alignment(m, p, "top_left")
def do_align_bottom_left(m, p): run_alignment(m, p, "bottom_left")
def do_align_top_right(m, p): run_alignment(m, p, "top_right")
def do_align_bottom_right(m, p): run_alignment(m, p, "bottom_right")
def do_align_top(m, p): run_alignment(m, p, "top")
def do_align_bottom(m, p): run_alignment(m, p, "bottom")
def do_align_left(m, p): run_alignment(m, p, "left")
def do_align_right(m, p): run_alignment(m, p, "right")


# ========================================================
# 【语言文本】
# ========================================================
try:
    sys_lang, _ = locale.getdefaultlocale()
except:
    sys_lang = "en"

# 根据系统语言环境，命名菜单选项
if sys_lang and sys_lang.startswith("zh"):
    menu_tools = "字形置位"  #菜单名称
    
    name_both = "从横居中"  #选项名称
    name_vertical = "从向居中"
    name_horizontal = "横向居中"
    name_center_top = "居中上"    
    name_center_bottom = "居中下" 
    name_center_left = "居中左"
    name_center_right = "居中右"
    name_top_left = "居左上"
    name_bottom_left = "居左下"
    name_top_right = "居右上"
    name_bottom_right = "居右下"
    name_top = "居上"
    name_bottom = "居下"
    name_left = "居左"
    name_right = "居右"
else:
    menu_tools = "Glyph Alignment"
    
    name_both = "Center in Height & Width"
    name_vertical = "Center in Height"
    name_horizontal = "Center in Width"
    name_center_top = "Center Top"
    name_center_bottom = "Center Bottom" 
    name_center_left = "Center Left"
    name_center_right = "Center Right"
    name_top_left = "Top Left"
    name_bottom_left = "Bottom Left"
    name_top_right = "Top Right"
    name_bottom_right = "Bottom Right"
    name_top = "Top"
    name_bottom = "Bottom"
    name_left = "Left"
    name_right = "Right"

# ========================================================
# 【菜单注册】
# ========================================================
menus = [
    (do_center_both, name_both),
    (do_center_vertical, name_vertical),
    (do_center_horizontal, name_horizontal),
    (do_align_center_top, name_center_top),      
    (do_align_center_bottom, name_center_bottom),
    (do_align_center_left, name_center_left),
    (do_align_center_right, name_center_right),
    (do_align_top_left, name_top_left),
    (do_align_bottom_left, name_bottom_left),
    (do_align_top_right, name_top_right),
    (do_align_bottom_right, name_bottom_right),
    (do_align_top, name_top),
    (do_align_bottom, name_bottom),
    (do_align_left, name_left),
    (do_align_right, name_right)
]

for func, name in menus:
    # 1. 注册字体本体窗口 (FontView)
    fontforge.registerMenuItem(func, None, None, "Font", None, menu_tools, name)
    # 2. 注册字形编辑窗口 (CharView)
    fontforge.registerMenuItem(func, None, None, "Char", None, menu_tools, name)

# ========================================================
# 制作：留幺。其时：二空二六年五月廿九日。
# Hotkeys e.g:
# ✓ Ctrl+ﾵ(Numpad5): Glyph alignment in FontView and CharView.
# - ﾵ(Numpad5): Align selected elements in CharView. (no plan)
# - Ctrl+Alt+ﾵ(Numpad5): Align selected element 1 to selected element 2 in CharView. (no plan)
# ========================================================
# 【追加键位】
# ========================================================
# Tools -> Glyph Alignment -> Select 1 of 15 below:
# Ctrl+Numpad. : Center in Width
# Ctrl+Numpad0: Center in Height
# Ctrl+Numpad5: Center in Height & Width
# Ctrl+Numpad8: Center Top
# Ctrl+Numpad2: Center Bottom
# Ctrl+Numpad4: Center Left
# Ctrl+Numpad6: Center Right
# Ctrl+Numpad7: Top Left
# Ctrl+Numpad9: Top Right
# Ctrl+Numpad3: Bottom Left
# Ctrl+Numpad1: Bottom Right
# Ctrl+Numpad/: Left
# Ctrl+Numpad*: Right
# Ctrl+Numpad-: Top
# Ctrl+Numpad+: Bottom
# ========================================================
import os
import sys

# 1. 自动根据操作系统，匹配路径和文件名
if sys.platform.startswith("win"):
    appdata_dir = os.environ.get("APPDATA")
    fontforge_dir = os.path.join(appdata_dir, "FontForge") if appdata_dir else os.path.join(os.path.expanduser("~"), "AppData", "Roaming", "FontForge")
    filename = "hotkeys"
else:
    fontforge_dir = os.path.join(os.path.expanduser("~"), ".FontForge")
    filename = "shortcuts"

hotkeys_path = os.path.join(fontforge_dir, filename)
os.makedirs(fontforge_dir, exist_ok=True)

# 2. 识别当前系统语言环境
try:
    sys_lang, _ = locale.getdefaultlocale()
except:
    sys_lang = "en"

# 3. 根据语言环境，分别定制【菜单名称】以及【GUI弹窗文本】
if sys_lang and sys_lang.startswith("zh"):
    # === 中文系统配置 ===
    raw_hotkeys_lines = [
        "FontView.Menu.Tools.字形置位.从横居中: Ctrl+ﾵ",
        "FontView.Menu.Tools.字形置位.从向居中: Ctrl+ﾮ",
        "FontView.Menu.Tools.字形置位.横向居中: Ctrl+ﾰ",
        "FontView.Menu.Tools.字形置位.居中上: Ctrl+ﾸ",
        "FontView.Menu.Tools.字形置位.居中下: Ctrl+ﾲ",
        "FontView.Menu.Tools.字形置位.居中左: Ctrl+ﾴ",
        "FontView.Menu.Tools.字形置位.居中右: Ctrl+ﾶ",
        "FontView.Menu.Tools.字形置位.居左上: Ctrl+ﾷ",
        "FontView.Menu.Tools.字形置位.居左下: Ctrl+ﾱ",
        "FontView.Menu.Tools.字形置位.居右上: Ctrl+ﾹ",
        "FontView.Menu.Tools.字形置位.居右下: Ctrl+ﾳ",
        "FontView.Menu.Tools.字形置位.居上: Ctrl+ﾭ",
        "FontView.Menu.Tools.字形置位.居下: Ctrl+ﾫ",
        "FontView.Menu.Tools.字形置位.居左: Ctrl+ﾯ",
        "FontView.Menu.Tools.字形置位.居右: Ctrl+ﾪ",

        "CharView.Menu.Tools.字形置位.从横居中: Ctrl+ﾵ",
        "CharView.Menu.Tools.字形置位.从向居中: Ctrl+ﾮ",
        "CharView.Menu.Tools.字形置位.横向居中: Ctrl+ﾰ",
        "CharView.Menu.Tools.字形置位.居中上: Ctrl+ﾸ",
        "CharView.Menu.Tools.字形置位.居中下: Ctrl+ﾲ",
        "CharView.Menu.Tools.字形置位.居中左: Ctrl+ﾴ",
        "CharView.Menu.Tools.字形置位.居中右: Ctrl+ﾶ",
        "CharView.Menu.Tools.字形置位.居左上: Ctrl+ﾷ",
        "CharView.Menu.Tools.字形置位.居左下: Ctrl+ﾱ",
        "CharView.Menu.Tools.字形置位.居右上: Ctrl+ﾹ",
        "CharView.Menu.Tools.字形置位.居右下: Ctrl+ﾳ",
        "CharView.Menu.Tools.字形置位.居上: Ctrl+ﾭ",
        "CharView.Menu.Tools.字形置位.居下: Ctrl+ﾫ",
        "CharView.Menu.Tools.字形置位.居左: Ctrl+ﾯ",
        "CharView.Menu.Tools.字形置位.居右: Ctrl+ﾪ",
    ]
    
    # 中文弹窗文本
    gui_title = "字形置位提示"
    err_read_title = "读取快捷键失败"
    err_read_msg = "无法读取原本的配置文件，原因:\n"
    err_write_title = "写入快捷键失败"
    err_write_msg = "无法将快捷键写入配置文件，原因:\n"
    
    def get_success_msg(added, kept):
        return f"完成键位配置！\n\n 成功补全：{added} 项键位。\n 保留原有：{kept} 项不变。\n\n重启软件之后，键位自动激活。"
        
    no_change_msg = "检测完毕。\n所有键位在原文件中都已存在配置，未做任何修改。"

else:
    # === 英文及其他系统配置 ===
    raw_hotkeys_lines = [
        "FontView.Menu.Tools.Glyph Alignment.Center in Height & Width: Ctrl+ﾵ",
        "FontView.Menu.Tools.Glyph Alignment.Center in Height: Ctrl+ﾮ",
        "FontView.Menu.Tools.Glyph Alignment.Center in Width: Ctrl+ﾰ",
        "FontView.Menu.Tools.Glyph Alignment.Center Top: Ctrl+ﾸ",
        "FontView.Menu.Tools.Glyph Alignment.Center Bottom: Ctrl+ﾲ",
        "FontView.Menu.Tools.Glyph Alignment.Center Left: Ctrl+ﾴ",
        "FontView.Menu.Tools.Glyph Alignment.Center Right: Ctrl+ﾶ",
        "FontView.Menu.Tools.Glyph Alignment.Top Left: Ctrl+ﾷ",
        "FontView.Menu.Tools.Glyph Alignment.Bottom Left: Ctrl+ﾱ",
        "FontView.Menu.Tools.Glyph Alignment.Top Right: Ctrl+ﾹ",
        "FontView.Menu.Tools.Glyph Alignment.Bottom Right: Ctrl+ﾳ",
        "FontView.Menu.Tools.Glyph Alignment.Top: Ctrl+ﾭ",
        "FontView.Menu.Tools.Glyph Alignment.Bottom: Ctrl+ﾫ",
        "FontView.Menu.Tools.Glyph Alignment.Left: Ctrl+ﾯ",
        "FontView.Menu.Tools.Glyph Alignment.Right: Ctrl+ﾪ",

        "CharView.Menu.Tools.Glyph Alignment.Center in Height & Width: Ctrl+ﾵ",
        "CharView.Menu.Tools.Glyph Alignment.Center in Height: Ctrl+ﾮ",
        "CharView.Menu.Tools.Glyph Alignment.Center in Width: Ctrl+ﾰ",
        "CharView.Menu.Tools.Glyph Alignment.Center Top: Ctrl+ﾸ",
        "CharView.Menu.Tools.Glyph Alignment.Center Bottom: Ctrl+ﾲ",
        "CharView.Menu.Tools.Glyph Alignment.Center Left: Ctrl+ﾴ",
        "CharView.Menu.Tools.Glyph Alignment.Center Right: Ctrl+ﾶ",
        "CharView.Menu.Tools.Glyph Alignment.Top Left: Ctrl+ﾷ",
        "CharView.Menu.Tools.Glyph Alignment.Bottom Left: Ctrl+ﾱ",
        "CharView.Menu.Tools.Glyph Alignment.Top Right: Ctrl+ﾹ",
        "CharView.Menu.Tools.Glyph Alignment.Bottom Right: Ctrl+ﾳ",
        "CharView.Menu.Tools.Glyph Alignment.Top: Ctrl+ﾭ",
        "CharView.Menu.Tools.Glyph Alignment.Bottom: Ctrl+ﾫ",
        "CharView.Menu.Tools.Glyph Alignment.Left: Ctrl+ﾯ",
        "CharView.Menu.Tools.Glyph Alignment.Right: Ctrl+ﾪ"
    ]
    
    # 英文弹窗文本
    gui_title = "Glyph Alignment Notice"
    err_read_title = "Failed to Read Hotkeys"
    err_read_msg = "Cannot read the shortcut configuration file. Reason:\n"
    err_write_title = "Failed to Write Hotkeys"
    err_write_msg = "Cannot write hotkeys to the configuration file. Reason:\n"
    
    def get_success_msg(added, kept):
        return f"Hotkey patch configuration completed!\n\n Successfully added: {added} new shortcut(s).\n Kept original: {kept} existing shortcut(s) unchanged.\n\nPlease restart FontForge to activate the numpad hotkeys!"
        
    no_change_msg = "Scan completed.\nGlyph Alignment menu including options already exist in the configuration file. No changes were made."

# 4. 扫描读取原本键位文件，提取已被占用路径
existing_paths = set()
if os.path.exists(hotkeys_path):
    try:
        with open(hotkeys_path, "r", encoding="utf-8") as f:
            for line in f:
                line_striped = line.strip()
                if line_striped and not line_striped.startswith("#"):
                    if ":" in line_striped:
                        menu_path = line_striped.split(":")[0].strip()
                        existing_paths.add(menu_path)
    except Exception as e:
        fontforge.postError(err_read_title, f"{err_read_msg}{e}")

# 5. 行级筛选：路径若被占用，该行不做变更
lines_to_append = []
for line in raw_hotkeys_lines:
    menu_path = line.split(":")[0].strip()
    if menu_path not in existing_paths:
        lines_to_append.append(line)

# 6. 执行安全追加，触发原生界面面板提示
if lines_to_append:

    append_content = "\n\n" + "\n" + "\n".join(lines_to_append) + "\n"
    
    try:
        with open(hotkeys_path, "a", encoding="utf-8") as f:
            f.write(append_content)
        
        # 追加成功，原生Notice弹窗
        conflict_count = len(raw_hotkeys_lines) - len(lines_to_append)
        fontforge.postNotice(gui_title, get_success_msg(len(lines_to_append), conflict_count))
        
    except Exception as e:
        fontforge.postError(err_write_title, f"{err_write_msg}{e}")
else:
    # 无需修改，原生Warning/Error弹窗
    fontforge.postError(gui_title, no_change_msg)