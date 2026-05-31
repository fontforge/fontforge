========================================================

##### Description of GlyphAlignment.py

========================================================

Made by: asdasdsasa

Version: 20260531

========================================================

###### Hotkeys e.g:

✓ Ctrl+ﾵ(Numpad5): Glyph alignment in FontView and CharView.

\- ﾵ(Numpad5): Align selected elements in CharView. (no plan)

\- Ctrl+Alt+ﾵ(Numpad5): Align selected element 1 to selected element 2 in CharView. (no plan)

========================================================

###### Recommended Workflow

1\. Copy selected path(s) from Inkscape by Ctrl+C.

2\. Paste into the selected glyph in FontView or CharView of FontForge by Ctrl+V.

3\. Align the glyph(s) by hotkey.

========================================================

###### Script Path

Windows: C:\\Users\\YourUsername\\AppData\\Roaming\\FontForge\\python\\

Linux\\Mac: \~/.config/fontforge/python/

========================================================

###### Tools -> Glyph Alignment -> Select 1 of 15 below:

Ctrl+Numpad. : Center in Width

Ctrl+Numpad0: Center in Height

Ctrl+Numpad5: Center in Height \& Width

Ctrl+Numpad8: Center Top

Ctrl+Numpad2: Center Bottom

Ctrl+Numpad4: Center Left

Ctrl+Numpad6: Center Right

Ctrl+Numpad7: Top Left

Ctrl+Numpad9: Top Right

Ctrl+Numpad1: Bottom Left

Ctrl+Numpad3: Bottom Right

Ctrl+Numpad/: Left

Ctrl+Numpad\*: Right

Ctrl+Numpad-: Top

Ctrl+Numpad+: Bottom

========================================================

###### Tools -> Glyph Alignment -> Select Transformation Command:

✗Ctrl+Numpad Enter: Transformation Command （Unsupported)

✓Numpad0              : Transformation Command   (Interimistical)

&#x20; Transformation Command (Space-separated, supports expressions and %):

&#x20; ━━━━━━━━━━━━━━━━━━━━━━

&#x20; Local  : x, y     | Both: xy, yx

&#x20; Global: z, t     |  Both: zt, tz

&#x20; Scale  : h, w    | Both: hw, wh

&#x20; Rotate: l, r      | Flip  : fh, fv

&#x20; ━━━━━━━━━━━━━━━━━━━━━━

&#x20; Enter commands e.g: xy-100 hw120% r30

========================================================

##### 说明GlyphAlignment.py

========================================================

制作: 留幺

版本: 20260531

========================================================

###### 键位用途

✓ Ctrl+ﾵ(小键盘5): 字形置位用于字体本体窗口、字形编辑窗口。

\- ﾵ(小键盘5): 所选置位用于字形编辑窗口 （无）

\- Ctrl+Alt+ﾵ(小键盘5): 所选其一置位其二，用于字形编辑窗口（无）

========================================================

###### 推荐做法

1\. Inkscape复制所选物体路径。

2\. FontForge字体窗口，字形窗口皆可黏贴所选字形。

3\. 使用快捷按键，各个自动置位。

========================================================

###### 脚本路径

Windows: C:\\Users\\用户名\\AppData\\Roaming\\FontForge\\python\\

Linux\\Mac: \~/.config/fontforge/python/

========================================================

###### 工具 -> 字形置位 -> 十五居一：

Ctrl+小键盘. : 居中横

Ctrl+小键盘0: 居中从

Ctrl+小键盘5: 居中正

Ctrl+小键盘8: 居中上

Ctrl+小键盘2: 居中下

Ctrl+小键盘4: 居中左

Ctrl+小键盘6: 居中右

Ctrl+小键盘7: 居左上

Ctrl+小键盘9: 居右上

Ctrl+小键盘1: 居左下

Ctrl+小键盘3: 居右下

Ctrl+小键盘/: 居左

Ctrl+小键盘\*: 居右

Ctrl+小键盘-: 居上

Ctrl+小键盘+: 居下

========================================================

###### 工具 -> 字形置位 -> 变换指令：

✗Ctrl+Numpad Enter: 变换指令（其无此能）

✓Numpad0              : 变换指令（暂代按键）

&#x20; 变换指令 (空格分隔，支持算式与%):

&#x20; ━━━━━━━━━━━━━━━━━━━━━━

&#x20; 移位: x, y  | 并行: xy, yx

&#x20; 定位: z, t  | 并行: zt, tz

&#x20; 缩放: h, w| 并行: hw,wh

&#x20; 顺逆: r, l   | 翻转: fh, fv

&#x20; ━━━━━━━━━━━━━━━━━━━━━━

&#x20; 指令案例：xy-100 hw120% r30

