| 🇺🇸 English | 🇨🇳 简体中文 |
| :--- | :--- |
| **GlyphAlignment.py** - FontForge Glyph Alignment Script | **GlyphAlignment.py** - FontForge字形置位脚本 |
| **Made by:** unbeebnu | **制作:** unbeebnu |
| **Version:** 20261024 | **版本:** 20261024 |

### 1. Hotkeys / 键位用途
| 🇺🇸 English | 🇨🇳 简体中文 |
| :--- | :--- |
| ✓ **Ctrl+ﾵ (Numpad5):** Glyph alignment in FontView and CharView. | ✓ **Ctrl+ﾵ (小键盘5):** 字体窗口、字形窗口，可以字形置位。 |
| ✗ **ﾵ (Numpad5):** Align selected elements in CharView. | ✗ **ﾵ (小键盘5):** 字形窗口，所选置位。 |
| ✗ **Ctrl+Alt+ﾵ (Numpad5):** Align selected element 1 to 2 in CharView. | ✗ **Ctrl+Alt+ﾵ (小键盘5):** 字形窗口，所选其一置位其二。 |

### 2. Recommended Workflow / 推荐做法
| 🇺🇸 English | 🇨🇳 简体中文 |
| :--- | :--- |
| 1. Copy selected path(s) from Inkscape by `Ctrl+C`. | 1. 在 Inkscape 中复制所选物体路径 (`Ctrl+C`)。 |
| 2. Paste into the selected glyph in FontForge by `Ctrl+V`. | 2. 在 FontForge 字体窗口或字形窗口中粘贴所选字形 (`Ctrl+V`)。 |
| 3. Align the glyph(s) by hotkey. | 3. 使用快捷按键，一键自动置位。 |

### 3. Script Path / 脚本路径
| 🇺🇸 English | 🇨🇳 简体中文 |
| :--- | :--- |
| `C:\Users\YourUsername\AppData\Roaming\FontForge\python\` | `C:\Users\用户名\AppData\Roaming\FontForge\python\` |
| `FontForge\share\fontforge\python\` | `FontForge\share\fontforge\python\` |
| `~/.config/fontforge/python/` | `~/.config/fontforge/python/` |

### 4. Tools -> Glyph Alignment (15 Directions) / 工具 -> 字形置位 (十五居一)
| 🇺🇸 English | 🇨🇳 简体中文 |
| :--- | :--- |
| `Ctrl+Numpad .` : Center in Height | `Ctrl+小键盘.` : 居中从 |
| `Ctrl+Numpad 0` : Center in Width | `Ctrl+小键盘0` : 居中横 |
| `Ctrl+Numpad 5` : Center in Height & Width | `Ctrl+小键盘5` : 居中正 |
| `Ctrl+Numpad 8` : Center Top | `Ctrl+小键盘8` : 居中上 |
| `Ctrl+Numpad 2` : Center Bottom | `Ctrl+小键盘2` : 居中下 |
| `Ctrl+Numpad 4` : Center Left | `Ctrl+小键盘4` : 居中左 |
| `Ctrl+Numpad 6` : Center Right | `Ctrl+小键盘6` : 居中右 |
| `Ctrl+Numpad 7` : Top Left | `Ctrl+小键盘7` : 居左上 |
| `Ctrl+Numpad 9` : Top Right | `Ctrl+小键盘9` : 居右上 |
| `Ctrl+Numpad 1` : Bottom Left | `Ctrl+小键盘1` : 居左下 |
| `Ctrl+Numpad 3` : Bottom Right | `Ctrl+小键盘3` : 居右下 |
| `Ctrl+Numpad /` : Left | `Ctrl+小键盘/` : 居左 |
| `Ctrl+Numpad *` : Right | `Ctrl+小键盘*` : 居右 |
| `Ctrl+Numpad -` : Top | `Ctrl+小键盘-` : 居上 |
| `Ctrl+Numpad +` : Bottom | `Ctrl+小键盘+` : 居下 |

### 5. Transformation Command / 变换指令

| 🇺🇸 English | 🇨🇳 简体中文 |
| :--- | :--- |
| ✗ `Ctrl+Numpad Enter` : Unsupported | ✗ `Ctrl+Numpad Enter` : 暂不支持 |
| ✓ `Numpad 0` : Interimistical | ✓ `小键盘 0` : 暂代按键 |
| **Syntax:** Space-separated, supports expressions and `%`<br><br>**Parameters:**<br>• `Local (X, Y)`: `x`, `y` \| Both: `xy`, `yx`<br>• `Global (Z, T)`: `z`, `t` \| Both: `zt`, `tz`<br>• `Scale (W, H)`: `w`, `h` \| Both: `wh`, `hw`<br>• `Rotate (L, R)`: `l`, `r` \| Flip: `fh`, `fv`<br><br>**Example:** `xy-100 hw120% r30` | **语法说明:** 空格分隔，支持算式与 `%`<br><br>**参数说明:**<br>• `本地相对 (横, 从)`: `x`, `y` \| 并行: `xy`, `yx`<br>• `全局绝对 (纬, 经)`: `z`, `t` \| 并行: `zt`, `tz`<br>• `等比例缩放 (宽, 高)`: `w`, `h` \| 并行: `wh`, `hw`<br>• `旋转方向 (逆, 顺)`: `l`, `r` \| 反倒: `fh`, `fv`<br><br>**指令案例:** `xy-100 hw120% r30` |