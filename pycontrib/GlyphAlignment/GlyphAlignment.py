import fontforge, locale, re, math
# ========================================================
# 变换指令处理函数
# ========================================================
def multiply_matrices(m1, m2):
    """ FontForge2D仿射变换，二个矩阵乘法 """
    a1, b1, c1, d1, x1, y1 = m1
    a2, b2, c2, d2, x2, y2 = m2
    return (
        a1 * a2 + b1 * c2, a1 * b2 + b1 * d2,
        c1 * a2 + d1 * c2, c1 * b2 + d1 * d2,
        x1 * a2 + y1 * c2 + x2, x1 * b2 + y1 * d2 + y2
    )

def parse_and_apply_transform(glyph, command_str):
    try:
        # 0.立定四极
        bbox_xmin, bbox_ymin, bbox_xmax, bbox_ymax = glyph.boundingBox()
        if bbox_xmin == bbox_ymin == bbox_xmax == bbox_ymax == 0 and not glyph.layers: return False
        
        orig_w, orig_h = glyph.width, glyph.vwidth

        # 1. 文本规范
        cmd_clean = re.sub(r'\s*([\+\-\*\/])\s*', r'\1', command_str.lower())

        # 2. 指令拆分
        steps = []
        for cmd, val_str in re.findall(r'([a-z]+)\s*([0-9\+\-\*\/\.\%(\)]*)', cmd_clean):
            # 安全算式求解 (自动支持 % 百分比换算)
            v_clean = re.sub(r'[^0-9\+\-\*\/\.\(\)]', '', val_str.replace('%', '/100.0')) if val_str else '0.0'
            val = float(eval(v_clean)) if v_clean else 0.0
            
            # 并项等值
            axis_map = {
                'xy': [('x', val), ('y', val)], 'yx': [('x', val), ('y', val)],
                'zt': [('z', val), ('t', val)], 'tz': [('z', val), ('t', val)],
                'hw': [('h', val), ('w', val)], 'wh': [('w', val), ('h', val)]
            }
            steps.extend(axis_map.get(cmd, [(cmd, val)]))

        # 3. 变换
        final_matrix = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
        c_xmin, c_ymin, c_xmax, c_ymax = bbox_xmin, bbox_ymin, bbox_xmax, bbox_ymax

        for cmd, val in steps:
            cx, cy = (c_xmin + c_xmax) / 2.0, (c_ymin + c_ymax) / 2.0
            
            # 3.1. 映射计算矩阵
            rad = math.radians(-val if cmd == 'r' else val)
            cos_r, sin_r = math.cos(rad), math.sin(rad)
            
            matrix_map = {
                'x':  (1.0, 0.0, 0.0, 1.0, val, 0.0),
                'y':  (1.0, 0.0, 0.0, 1.0, 0.0, val),
                'z':  (1.0, 0.0, 0.0, 1.0, (c_xmin + val) - cx, 0.0),
                't':  (1.0, 0.0, 0.0, 1.0, 0.0, (c_ymin + val) - cy),
                'r':  (cos_r, sin_r, -sin_r, cos_r, 0.0, 0.0), 
                'l':  (cos_r, sin_r, -sin_r, cos_r, 0.0, 0.0),
                'w':  (val, 0.0, 0.0, 1.0, 0.0, 0.0),         
                'h':  (1.0, 0.0, 0.0, val, 0.0, 0.0),         
                'fh': (-1.0, 0.0, 0.0, 1.0, 0.0, 0.0),        
                'fv': (1.0, 0.0, 0.0, -1.0, 0.0, 0.0)
            }
            if cmd not in matrix_map: continue
            m_raw = matrix_map[cmd]

            # 3.2. 位移功能可以直接结果，其他功能需在元位变换还元
            m_step = m_raw if cmd in ['x', 'y', 'z', 't'] else multiply_matrices(multiply_matrices((1.0, 0.0, 0.0, 1.0, -cx, -cy), m_raw), (1.0, 0.0, 0.0, 1.0, cx, cy))
            final_matrix = multiply_matrices(final_matrix, m_step)

            # 3.3. 追踪当前边界
            dx, dy = m_step[4], m_step[5]
            sw = abs(m_raw[0]) if cmd in ['w', 'fh'] else 1.0
            sh = abs(m_raw[3]) if cmd in ['h', 'fv'] else 1.0

            # 宽半高半，当前四极
            w_h, h_h = (c_xmax - c_xmin) * sw / 2.0, (c_ymax - c_ymin) * sh / 2.0
            c_xmin, c_xmax = (cx + dx) - w_h, (cx + dx) + w_h
            c_ymin, c_ymax = (cy + dy) - h_h, (cy + dy) + h_h

        # 4. 还原宽高
        glyph.transform(final_matrix)
        glyph.width, glyph.vwidth = orig_w, orig_h
        glyph.changed()
        return True
    except:
        return False
# ========================================================
# 居中居周处理函数
# ========================================================
def process_glyph_alignment(font_obj, glyph_name, align_type="center_both"):
    try:
        glyph, ascent = font_obj[glyph_name], font_obj.ascent
        g_width = glyph.width
        g_height = glyph.vwidth if glyph.vwidth > 0 else (ascent + font_obj.descent)
        
        # 1. 立定四极
        xmin, ymin, xmax, ymax = glyph.boundingBox()
        if xmin == ymin == xmax == ymax == 0 and not glyph.layers: 
            return False

        # 2. 横向先于中央，中央先于无方
        x_left   = "left" in align_type
        x_right  = "right" in align_type
        x_center = "center" in align_type or "both" in align_type or "horizontal" in align_type

        # 2.1 横向将位，横向儴位
        tgt_x, cur_x = (0.0, 0.0) # 默认不位移
        if x_left:    tgt_x, cur_x = (0.0,           xmin)
        elif x_right: tgt_x, cur_x = (g_width,       xmax)
        elif x_center: tgt_x, cur_x = (g_width / 2.0, (xmin + xmax) / 2.0)

        # 3. 从向先于中央，中央先于无方
        y_top    = "top" in align_type
        y_bottom = "bottom" in align_type
        y_center = "center" in align_type or "both" in align_type or "vertical" in align_type

        # 3.1 从向将位，从向儴位
        tgt_y, cur_y = (0.0, 0.0) # 默认不位移
        if y_top:      tgt_y, cur_y = (ascent,                  ymax)
        elif y_bottom: tgt_y, cur_y = (ascent - g_height,       ymin)
        elif y_center: tgt_y, cur_y = (ascent - g_height / 2.0, (ymin + ymax) / 2.0)

        # 4. 字形变换
        glyph.transform((1.0, 0.0, 0.0, 1.0, tgt_x - cur_x, tgt_y - cur_y))
        glyph.width, glyph.vwidth = g_width, g_height
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

    # 3.  若为指令弹窗
    if align_type == "custom_transform":
        # 字数有限
        prompt_zh = (
            "【横从|纬经|宽高|顺逆|反倒】:\n"
            "  x,y(xy) | z,t(zt) | w,h(wh) | r,l | fh,fv"
        )
        # 英文字数限制，极短缩写（Loc/Glo/Scl/Rot/Flp）
        prompt_en = (
            "【Loc|Glo|Scl|Rot|Flp】:\n"
            "  x,y(xy) | z,t(zt) | w,h(wh) | r,l | fh,fv"
        )
        # 弹出原生输入文框
        prompt = prompt_zh if is_zh else prompt_en
        title = "自由变换" if is_zh else "Custom Transform"
        user_input = fontforge.askString(title, prompt, "")
        
        if user_input:
            for name in glyph_names:
                parse_and_apply_transform(font_obj[name], user_input)
    else:
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
# 额外注册新增的【自定义命令输入】菜单项
custom_zh, custom_en = "变换指令", "Transformation Command"
fontforge.registerMenuItem(lambda m, p: run_alignment(p, "custom_transform"), None, None, "Font", None, menu_tools, custom_zh if is_zh else custom_en)
fontforge.registerMenuItem(lambda m, p: run_alignment(p, "custom_transform"), None, None, "Char", None, menu_tools, custom_zh if is_zh else custom_en)
# ========================================================
# 【追加键位】
# ========================================================
import os
import sys

# 1. 识别运行环境
is_win = sys.platform.startswith("win")

# 1.1 当前脚本，绝对路径
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)

# 1.2 统一路径格式，用于特征匹配
norm_dir = script_dir.replace("\\", "/").lower()

# 1.3 免安装版，路径所在
is_shared_env = "share/fontforge/python" in norm_dir or "share/fontforge" in norm_dir

if is_shared_env:
    # 1.3.1 内部脚本，是免安装
    idx = norm_dir.find("share/fontforge")
    base_dir = script_dir[:idx] if idx != -1 else os.path.dirname(os.path.dirname(script_dir))
    hotkeys_path = os.path.join(base_dir, "share", "fontforge", "hotkeys", "default")
    os.makedirs(os.path.dirname(hotkeys_path), exist_ok=True)
else:
    # 1.3.2 外部脚本，非免安装
    appdata = os.environ.get("APPDATA") if is_win else os.path.expanduser("~")
    fontforge_dir = os.path.join(appdata, "FontForge" if is_win else ".FontForge")
    hotkeys_path = os.path.join(fontforge_dir, "hotkeys" if is_win else "shortcuts")
    os.makedirs(fontforge_dir, exist_ok=True)

# 2. 识别当前系统语言环境
try:
    sys_lang, _ = locale.getdefaultlocale()
except Exception:
    sys_lang = "en"
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
    succ_msg = lambda a, r: f"完成键位配置！\n\n 修改目标: {hotkeys_path}\n\n 清理外文键位：{r} 项。\n 更新补全键位：{a} 项。\n\n重启软件之后，键位自动激活。"
    
    suffixes = ["居中正: Ctrl+ﾵ", "居中从: Ctrl+ﾮ", "居中横: Ctrl+ﾰ", "居中上: Ctrl+ﾸ", "居中下: Ctrl+ﾲ", "居中左: Ctrl+ﾴ", "居中右: Ctrl+ﾶ", "居左上: Ctrl+ﾷ", "居左下: Ctrl+ﾱ", "居右上: Ctrl+ﾹ", "居右下: Ctrl+ﾳ", "居上: Ctrl+ﾭ", "居下: Ctrl+ﾫ", "居左: Ctrl+ﾯ", "居右: Ctrl+ﾪ", "变换指令:ﾰ"]
else:
    gui_title = "Glyph Alignment Notice"
    err_r_title, err_r_msg = "Failed to Read Hotkeys", "Cannot read the shortcut configuration file. Reason:\n"
    err_w_title, err_w_msg = "Failed to Write Hotkeys", "Cannot write hotkeys to the configuration file. Reason:\n"
    no_change_msg = "Scan completed.\nGlyph Alignment menu options already exist and match current language. No changes were made."
    succ_msg = lambda a, r: f"Hotkey patch completed!\n\n Target: {hotkeys_path}\n\n Successfully removed conflicting language keys: {r}.\n Successfully added/updated: {a} shortcut(s).\n\nPlease restart FontForge to activate the numpad hotkeys!"
    
    suffixes = ["Center in Height & Width: Ctrl+ﾵ", "Center in Height: Ctrl+ﾮ", "Center in Width: Ctrl+ﾰ", "Center Top: Ctrl+ﾸ", "Center Bottom: Ctrl+ﾲ", "Center Left: Ctrl+ﾴ", "Center Right: Ctrl+ﾶ", "Top Left: Ctrl+ﾷ", "Bottom Left: Ctrl+ﾱ", "Top Right: Ctrl+ﾹ", "Bottom Right: Ctrl+ﾳ", "Top: Ctrl+ﾭ", "Bottom: Ctrl+ﾫ", "Left: Ctrl+ﾯ", "Right: Ctrl+ﾪ", "Transformation Command:ﾰ"]

# 组合菜单快捷键
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
    is_valid = line.strip() and not line.strip().startswith("#") and ":" in line
    menu_path = line.split(":")[0].strip() if is_valid else ""
    
    # 剔除冲突旧键
    if menu_path.startswith(opp_prefix): 
        removed_count += 1
        continue  
    # 记录今天已存在的键
    if menu_path.startswith(prefix):     
        existing_paths.add(menu_path)  
        
    clean_file_lines.append(line)

# 7. 行目查漏补缺，合并覆盖写入
lines_to_append = [line for line in raw_hotkeys if line.split(":")[0].strip() not in existing_paths]

# 状态完备，无需修改
if removed_count == 0 and not lines_to_append:
    fontforge.logWarning(no_change_msg)
else:
    # 新旧合并写入文件
    try:
        file_content = "".join(clean_file_lines).rstrip() + ("\n\n" + "\n".join(lines_to_append) + "\n" if lines_to_append else "\n")
        with open(hotkeys_path, "w", encoding="utf-8") as f: 
            f.write(file_content)
        fontforge.postNotice(gui_title, succ_msg(len(lines_to_append), removed_count))
    except Exception as e: 
        fontforge.postError(err_w_title, f"{err_w_msg}{e}")

