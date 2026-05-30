import fontforge, locale

# ========================================================
# 居中居周处理函数
# ========================================================
def process_glyph_alignment(font_obj, glyph_name, align_type="center_both"):
    try:
        glyph, ascent = font_obj[glyph_name], font_obj.ascent
        glyph_width, glyph_height = glyph.width, (glyph.vwidth if glyph.vwidth > 0 else ascent + font_obj.descent)
        
        # 1. 极点坐标列表 by point in contour(s)
        x_list = [p.x for i in range(len(glyph.layers)) for c in glyph.layers[i] for p in c if p.on_curve]
        y_list = [p.y for i in range(len(glyph.layers)) for c in glyph.layers[i] for p in c if p.on_curve]
        
        # 2. 试探格局，拦截空白
        bbox_xmin, bbox_ymin, bbox_xmax, bbox_ymax = glyph.boundingBox()
        if bbox_xmin == bbox_ymin == bbox_xmax == bbox_ymax == 0 and not glyph.layers: return False
        
        # 3. 无四极点，便同格局
        extrema_xmin, extrema_xmax = (min(x_list), max(x_list)) if x_list else (bbox_xmin, bbox_xmax)
        extrema_ymin, extrema_ymax = (min(y_list), max(y_list)) if y_list else (bbox_ymin, bbox_ymax)

        # 4. 从横位移核心计算（目标坐标 - 当前坐标）
        # 从向计算 (Y轴)
        is_center_v = "center" in align_type and align_type not in ["center_top", "center_bottom"]
        target_y    = (ascent - glyph_height / 2.0) if is_center_v else (ascent if "top" in align_type else ascent - glyph_height)
        current_y   = (extrema_ymin + extrema_ymax) / 2.0 if is_center_v else (bbox_ymax if "top" in align_type else bbox_ymin)
        y_offset    = target_y - current_y

        # 横向计算 (X轴)
        is_center_h = align_type in ["center_both", "center_horizontal", "center_top", "center_bottom"]
        target_x    = (glyph_width - (extrema_xmax - extrema_xmin)) / 2.0 if is_center_h else (0.0 if "left" in align_type or align_type == "left" else glyph_width)
        current_x   = extrema_xmin if is_center_h else (bbox_xmin if "left" in align_type or align_type == "left" else bbox_xmax)
        x_offset    = target_x - current_x

        # 5. 变换还原宽高
        glyph.transform((1.0, 0.0, 0.0, 1.0, x_offset, y_offset))
        glyph.width, glyph.vwidth = glyph_width, glyph_height
        glyph.changed()
        return True
    except:
        return False
# ========================================================
# 识别字体字形，两者窗口所传
# ========================================================
def run_alignment(passed_obj, align_type):
    # 1. 取得字体对象
    font_obj = passed_obj.font if hasattr(passed_obj, 'font') else fontforge.activeFont()
    if not font_obj: return

    # 2. 所选字形列表
    glyph_names = [passed_obj.glyphname] if hasattr(passed_obj, 'glyphname') else [g.glyphname for g in font_obj.selection.byGlyphs]

    # 3. 遍历令归置位
    for name in glyph_names:
        process_glyph_alignment(font_obj, name, align_type)
# ========================================================
# 【菜单注册】
# ========================================================
# 1. 识别系统语言
try:
    lang_code, _ = locale.getdefaultlocale()
    is_zh = bool(lang_code and lang_code.startswith("zh"))
except:
    is_zh = False

menu_tools = "字形置位" if is_zh else "Glyph Alignment"

# 2. 十五方向
directions = [
    ("center_both",       "居中正", "Center Both"),       ("center_vertical",   "居中从", "Center Vertical"),
    ("center_horizontal", "居中横", "Center Horizontal"), ("center_top",        "居中上", "Center Top"),
    ("center_bottom",     "居中下", "Center Bottom"),     ("center_left",       "居中左", "Center Left"),
    ("center_right",      "居中右", "Center Right"),      ("top_left",          "居左上", "Top Left"),
    ("bottom_left",       "居左下", "Bottom Left"),       ("top_right",         "居右上", "Top Right"),
    ("bottom_right",      "居右下", "Bottom Right"),      ("top",               "居上",   "Top"),
    ("bottom",            "居下",   "Bottom"),            ("left",              "居左",   "Left"),
    ("right",             "居右",   "Right")
]

# 3. 注册字体窗口、字形窗口
for dtype, zh, en in directions:
     fontforge.registerMenuItem(lambda m, p, dt=dtype: run_alignment(p, dt), None, None, "Font", None, menu_tools, zh if is_zh else en)
     fontforge.registerMenuItem(lambda m, p, dt=dtype: run_alignment(p, dt), None, None, "Char", None, menu_tools, zh if is_zh else en)
# ========================================================
import os, sys

# 1. 自动根据操作系统，匹配路径和文件名
is_win = sys.platform.startswith("win")
appdata = os.environ.get("APPDATA") if is_win else os.path.expanduser("~")
fontforge_dir = os.path.join(appdata, "FontForge" if is_win else ".FontForge")
hotkeys_path = os.path.join(fontforge_dir, "hotkeys" if is_win else "shortcuts")
os.makedirs(fontforge_dir, exist_ok=True)

# 2. 识别当前系统语言环境
try: sys_lang, _ = locale.getdefaultlocale()
except: sys_lang = "en"
is_zh_sys = bool(sys_lang and sys_lang.startswith("zh"))

# 3. 基础菜单路径前缀（固定定义）
zh_pfx = ("FontView.Menu.Tools.字形置位.", "CharView.Menu.Tools.字形置位.")
en_pfx = ("FontView.Menu.Tools.Glyph Alignment.", "CharView.Menu.Tools.Glyph Alignment.")

# 4. 中外语言［当前前缀］以及［冲突前缀］
prefix, opp_prefix = (zh_pfx, en_pfx) if is_zh_sys else (en_pfx, zh_pfx)

# 5. 中外提示、键位文本
if is_zh_sys:
    gui_title = "字形置位提示"
    err_r_title, err_r_msg = "读取键位失败", "读取不了原本配置文件，原因:\n"
    err_w_title, err_w_msg = "写入键位失败", "键位无法写入配置文件，原因:\n"
    no_change_msg = "检测完毕。\n当前系统的所有键位已正确配置，未做任何修改。"
    succ_msg = lambda a, r: f"完成键位配置！\n\n 清理外文键位：{r} 项。\n 更新补全键位：{a} 项。\n\n重启软件之后，键位自动激活。"
    
    suffixes = ["居中正: Ctrl+ﾵ", "居中从: Ctrl+ﾮ", "居中横: Ctrl+ﾰ", "居中上: Ctrl+ﾸ", "居中下: Ctrl+ﾲ", "居中左: Ctrl+ﾴ", "居中右: Ctrl+ﾶ", "居左上: Ctrl+ﾷ", "居左下: Ctrl+ﾱ", "居右上: Ctrl+ﾹ", "居右下: Ctrl+ﾳ", "居上: Ctrl+ﾭ", "居下: Ctrl+ﾫ", "居左: Ctrl+ﾯ", "居右: Ctrl+ﾪ"]
else:
    gui_title = "Glyph Alignment Notice"
    err_r_title, err_r_msg = "Failed to Read Hotkeys", "Cannot read the shortcut configuration file. Reason:\n"
    err_w_title, err_w_msg = "Failed to Write Hotkeys", "Cannot write hotkeys to the configuration file. Reason:\n"
    no_change_msg = "Scan completed.\nGlyph Alignment menu options already exist and match current language. No changes were made."
    succ_msg = lambda a, r: f"Hotkey patch configuration completed!\n\n Successfully removed conflicting language keys: {r}.\n Successfully added/updated: {a} shortcut(s).\n\nPlease restart FontForge to activate the numpad hotkeys!"
    
    suffixes = ["Center in Height & Width: Ctrl+ﾵ", "Center in Height: Ctrl+ﾮ", "Center in Width: Ctrl+ﾰ", "Center Top: Ctrl+ﾸ", "Center Bottom: Ctrl+ﾲ", "Center Left: Ctrl+ﾴ", "Center Right: Ctrl+ﾶ", "Top Left: Ctrl+ﾷ", "Bottom Left: Ctrl+ﾱ", "Top Right: Ctrl+ﾹ", "Bottom Right: Ctrl+ﾳ", "Top: Ctrl+ﾭ", "Bottom: Ctrl+ﾫ", "Left: Ctrl+ﾯ", "Right: Ctrl+ﾪ"]

# 组合卅行键位
raw_hotkeys = [f"{p}{s}" for p in prefix for s in suffixes]

# 6. 读取之前文件，过滤冲突语言键位，保留用户自定义键
clean_file_lines, existing_paths, removed_count = [], set(), 0
lines = []

# 读取键位配置，预防文件尚无
if os.path.exists(hotkeys_path):
    try:
        with open(hotkeys_path, "r", encoding="utf-8") as f:
            lines = f.readlines()
    except Exception as e:
        fontforge.postError(err_r_title, f"{err_r_msg}{e}")

# 逐行过滤冲突语言，保留用户自定义键
for line in lines:
    # 过滤空行註行，筛选有效键位
    is_valid = line.strip() and not line.strip().startswith("#") and ":" in line
    menu_path = line.split(":")[0].strip() if is_valid else ""
    
    if menu_path.startswith(opp_prefix): removed_count += 1; continue  # 剔除冲突旧键
    if menu_path.startswith(prefix):     existing_paths.add(menu_path)  # 记录今已存键
    clean_file_lines.append(line)

# 7. 行目查漏补缺，合并覆盖写入
lines_to_append = [line for line in raw_hotkeys if line.split(":")[0].strip() not in existing_paths]

# 状态完备，无需修改
if removed_count == 0 and not lines_to_append:
    fontforge.postError(gui_title, no_change_msg)
else:
    # 新旧合并写入文件
    try:
        # 去除尾部空白之后换行，若有新增键位，留出空行追加，否则正常结尾
        file_content = "".join(clean_file_lines).rstrip() + ("\n\n" + "\n".join(lines_to_append) + "\n" if lines_to_append else "\n")
        with open(hotkeys_path, "w", encoding="utf-8") as f: 
            f.write(file_content)
        fontforge.postNotice(gui_title, succ_msg(len(lines_to_append), removed_count))
    except Exception as e: 
        fontforge.postError(err_w_title, f"{err_w_msg}{e}")
