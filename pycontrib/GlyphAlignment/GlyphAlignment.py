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
    ("center_both",       "居中正", "Center in Height & Width", "Ctrl+ﾵ"),
    ("center_vertical",   "居中从", "Center in Height",           "Ctrl+ﾮ"),
    ("center_horizontal", "居中横", "Center in Width",            "Ctrl+ﾰ"),
    ("center_top",        "居中上", "Center Top",               "Ctrl+ﾸ"),
    ("center_bottom",     "居中下", "Center Bottom",            "Ctrl+ﾲ"),
    ("center_left",       "居中左", "Center Left",              "Ctrl+ﾴ"),
    ("center_right",      "居中右", "Center Right",             "Ctrl+ﾶ"),
    ("top_left",          "居左上", "Top Left",                 "Ctrl+ﾷ"),
    ("bottom_left",       "居左下", "Bottom Left",              "Ctrl+ﾱ"),
    ("top_right",         "居右上", "Top Right",                "Ctrl+ﾹ"),
    ("bottom_right",      "居右下", "Bottom Right",             "Ctrl+ﾳ"),
    ("top",               "居上",   "Top",                      "Ctrl+ﾭ"),
    ("bottom",            "居下",   "Bottom",                   "Ctrl+ﾫ"),
    ("left",              "居左",   "Left",                     "Ctrl+ﾯ"),
    ("right",             "居右",   "Right",                    "Ctrl+ﾪ")
]

# 3. 注册字体窗口、字形窗口
for dtype, zh, en, shortcut in directions:
    menu_label = zh if is_zh else en
    
    fontforge.registerMenuItem(lambda m, p, dt=dtype: run_alignment(p, dt), None, None, "Font", shortcut, menu_tools, menu_label)
    fontforge.registerMenuItem(lambda m, p, dt=dtype: run_alignment(p, dt), None, None, "Char", shortcut, menu_tools, menu_label)

# 4. 注册变换指令
menu_label = "变换指令" if is_zh else "Transformation Command"

fontforge.registerMenuItem(lambda m, p: run_alignment(p, "custom_transform"), None, None, "Font", "ﾰ", menu_tools, menu_label)
fontforge.registerMenuItem(lambda m, p: run_alignment(p, "custom_transform"), None, None, "Char", "ﾰ", menu_tools, menu_label)
# ========================================================
