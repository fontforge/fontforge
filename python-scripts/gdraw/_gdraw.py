from ctypes import *

_libraries = {}
_libraries['libgdraw.so.5'] = CDLL('libgdraw.so.5')
STRING = c_char_p


class gwindow(Structure):
    pass
GWindow = POINTER(gwindow)
class gdisplay(Structure):
    pass
GDisplay = gdisplay
class grect(Structure):
    pass
GRect = grect
class gevent(Structure):
    pass
GEvent = gevent
class gwindow_attrs(Structure):
    pass
GWindowAttrs = gwindow_attrs
GDrawCreateTopWindow = _libraries['libgdraw.so.5'].GDrawCreateTopWindow
GDrawCreateTopWindow.restype = GWindow
GDrawCreateTopWindow.argtypes = [POINTER(GDisplay), POINTER(GRect), CFUNCTYPE(c_int, GWindow, POINTER(GEvent)), c_void_p, POINTER(GWindowAttrs)]
GDrawDestroyWindow = _libraries['libgdraw.so.5'].GDrawDestroyWindow
GDrawDestroyWindow.restype = None
GDrawDestroyWindow.argtypes = [GWindow]
GDrawSetVisible = _libraries['libgdraw.so.5'].GDrawSetVisible
GDrawSetVisible.restype = None
GDrawSetVisible.argtypes = [GWindow, c_int]
class gtimer(Structure):
    pass
GTimer = gtimer
int32_t = c_int32
int32 = int32_t
GDrawRequestTimer = _libraries['libgdraw.so.5'].GDrawRequestTimer
GDrawRequestTimer.restype = POINTER(GTimer)
GDrawRequestTimer.argtypes = [GWindow, int32, int32, c_void_p]
GDrawCancelTimer = _libraries['libgdraw.so.5'].GDrawCancelTimer
GDrawCancelTimer.restype = None
GDrawCancelTimer.argtypes = [POINTER(GTimer)]
class ggc(Structure):
    pass
GGC = ggc
grect._fields_ = [
    ('x', int32),
    ('y', int32),
    ('width', int32),
    ('height', int32),
]
class gwidgetdata(Structure):
    pass
gwindow._fields_ = [
    ('ggc', POINTER(GGC)),
    ('display', POINTER(GDisplay)),
    ('eh', CFUNCTYPE(c_int, GWindow, POINTER(GEvent))),
    ('pos', GRect),
    ('parent', GWindow),
    ('user_data', c_void_p),
    ('widget_data', POINTER(gwidgetdata)),
    ('native_window', c_void_p),
    ('is_visible', c_uint, 1),
    ('is_pixmap', c_uint, 1),
    ('is_toplevel', c_uint, 1),
    ('visible_request', c_uint, 1),
    ('is_dying', c_uint, 1),
    ('is_popup', c_uint, 1),
    ('disable_expose_requests', c_uint, 1),
    ('usecairo', c_uint, 1),
    ('usepango', c_uint, 1),
]
gtimer._fields_ = [
    ('time_sec', c_long),
    ('time_usec', c_long),
    ('repeat_time', int32),
    ('owner', GWindow),
    ('userdata', c_void_p),
    ('next', POINTER(gtimer)),
    ('active', c_uint, 1),
]
class displayfuncs(Structure):
    pass
class font_state(Structure):
    pass
int16_t = c_int16
int16 = int16_t
uint32_t = c_uint32
uint32 = uint32_t
Color = uint32
uint16_t = c_uint16
uint16 = uint16_t
gdisplay._fields_ = [
    ('funcs', POINTER(displayfuncs)),
    ('semaphore', c_void_p),
    ('fontstate', POINTER(font_state)),
    ('res', int16),
    ('scale_screen_by', int16),
    ('groot', GWindow),
    ('def_background', Color),
    ('def_foreground', Color),
    ('mykey_state', uint16),
    ('mykey_keysym', uint16),
    ('mykey_mask', uint16),
    ('mykeybuild', c_uint, 1),
]

# values for enumeration 'event_type'
et_noevent = -1
et_char = 0
et_charup = 1
et_mousemove = 2
et_mousedown = 3
et_mouseup = 4
et_crossing = 5
et_focus = 6
et_expose = 7
et_visibility = 8
et_resize = 9
et_timer = 10
et_close = 11
et_create = 12
et_map = 13
et_destroy = 14
et_selclear = 15
et_drag = 16
et_dragout = 17
et_drop = 18
et_lastnativeevent = 18
et_controlevent = 19
et_user = 20
event_type = c_int # enum
class N6gevent4DOT_34E(Union):
    pass
class N6gevent4DOT_344DOT_35E(Structure):
    pass
unichar_t = uint32
N6gevent4DOT_344DOT_35E._fields_ = [
    ('device', STRING),
    ('time', uint32),
    ('state', uint16),
    ('x', int16),
    ('y', int16),
    ('keysym', uint16),
    ('chars', unichar_t * 10),
]
class N6gevent4DOT_344DOT_36E(Structure):
    pass
N6gevent4DOT_344DOT_36E._fields_ = [
    ('device', STRING),
    ('time', uint32),
    ('state', int16),
    ('x', int16),
    ('y', int16),
    ('button', int16),
    ('clicks', int16),
    ('pressure', int32),
    ('xtilt', int32),
    ('ytilt', int32),
    ('separation', int32),
]
class N6gevent4DOT_344DOT_37E(Structure):
    pass
N6gevent4DOT_344DOT_37E._fields_ = [
    ('rect', GRect),
]
class N6gevent4DOT_344DOT_38E(Structure):
    pass

# values for enumeration 'visibility_state'
vs_unobscured = 0
vs_partially = 1
vs_obscured = 2
visibility_state = c_int # enum
N6gevent4DOT_344DOT_38E._fields_ = [
    ('state', visibility_state),
]
class N6gevent4DOT_344DOT_39E(Structure):
    pass
N6gevent4DOT_344DOT_39E._fields_ = [
    ('size', GRect),
    ('dx', int16),
    ('dy', int16),
    ('dwidth', int16),
    ('dheight', int16),
    ('moved', c_uint, 1),
    ('sized', c_uint, 1),
]
class N6gevent4DOT_344DOT_40E(Structure):
    pass
N6gevent4DOT_344DOT_40E._fields_ = [
    ('device', STRING),
    ('time', uint32),
    ('state', int16),
    ('x', int16),
    ('y', int16),
    ('entered', c_uint, 1),
]
class N6gevent4DOT_344DOT_41E(Structure):
    pass
N6gevent4DOT_344DOT_41E._fields_ = [
    ('gained_focus', c_uint, 1),
    ('mnemonic_focus', c_uint, 2),
]
class N6gevent4DOT_344DOT_42E(Structure):
    pass
N6gevent4DOT_344DOT_42E._fields_ = [
    ('is_visible', c_uint, 1),
]
class N6gevent4DOT_344DOT_43E(Structure):
    pass

# values for enumeration 'selnames'
sn_primary = 0
sn_clipboard = 1
sn_drag_and_drop = 2
sn_user1 = 3
sn_user2 = 4
sn_max = 5
selnames = c_int # enum
N6gevent4DOT_344DOT_43E._fields_ = [
    ('sel', selnames),
]
class N6gevent4DOT_344DOT_44E(Structure):
    pass
N6gevent4DOT_344DOT_44E._fields_ = [
    ('x', int32),
    ('y', int32),
]
class N6gevent4DOT_344DOT_45E(Structure):
    pass
N6gevent4DOT_344DOT_45E._fields_ = [
    ('timer', POINTER(GTimer)),
    ('userdata', c_void_p),
]
class N6gevent4DOT_344DOT_46E(Structure):
    pass

# values for unnamed enumeration
et_buttonpress = 0
et_buttonactivate = 1
et_radiochanged = 2
et_listselected = 3
et_listdoubleclick = 4
et_scrollbarchange = 5
et_textchanged = 6
et_textfocuschanged = 7
et_lastsubtype = 8
class ggadget(Structure):
    pass
class N6gevent4DOT_344DOT_464DOT_48E(Union):
    pass
class sbevent(Structure):
    pass

# values for enumeration 'sb'
et_sb_top = 0
et_sb_uppage = 1
et_sb_up = 2
et_sb_left = 2
et_sb_down = 3
et_sb_right = 3
et_sb_downpage = 4
et_sb_bottom = 5
et_sb_thumb = 6
et_sb_thumbrelease = 7
sb = c_int # enum
sbevent._fields_ = [
    ('type', sb),
    ('pos', int32),
]
class N6gevent4DOT_344DOT_464DOT_484DOT_49E(Structure):
    pass
N6gevent4DOT_344DOT_464DOT_484DOT_49E._fields_ = [
    ('gained_focus', c_int),
]
class N6gevent4DOT_344DOT_464DOT_484DOT_50E(Structure):
    pass
N6gevent4DOT_344DOT_464DOT_484DOT_50E._fields_ = [
    ('from_pulldown', c_int),
]
class N6gevent4DOT_344DOT_464DOT_484DOT_51E(Structure):
    pass
N6gevent4DOT_344DOT_464DOT_484DOT_51E._fields_ = [
    ('clicks', c_int),
    ('button', int16),
    ('state', int16),
]
class N6gevent4DOT_344DOT_464DOT_484DOT_52E(Structure):
    pass
N6gevent4DOT_344DOT_464DOT_484DOT_52E._fields_ = [
    ('from_mouse', c_int),
    ('changed_index', c_int),
]
N6gevent4DOT_344DOT_464DOT_48E._fields_ = [
    ('sb', sbevent),
    ('tf_focus', N6gevent4DOT_344DOT_464DOT_484DOT_49E),
    ('tf_changed', N6gevent4DOT_344DOT_464DOT_484DOT_50E),
    ('button', N6gevent4DOT_344DOT_464DOT_484DOT_51E),
    ('list', N6gevent4DOT_344DOT_464DOT_484DOT_52E),
]
N6gevent4DOT_344DOT_46E._fields_ = [
    ('subtype', c_int),
    ('g', POINTER(ggadget)),
    ('u', N6gevent4DOT_344DOT_464DOT_48E),
]
class N6gevent4DOT_344DOT_53E(Structure):
    pass
N6gevent4DOT_344DOT_53E._fields_ = [
    ('subtype', c_long),
    ('userdata', c_void_p),
]
N6gevent4DOT_34E._fields_ = [
    ('chr', N6gevent4DOT_344DOT_35E),
    ('mouse', N6gevent4DOT_344DOT_36E),
    ('expose', N6gevent4DOT_344DOT_37E),
    ('visibility', N6gevent4DOT_344DOT_38E),
    ('resize', N6gevent4DOT_344DOT_39E),
    ('crossing', N6gevent4DOT_344DOT_40E),
    ('focus', N6gevent4DOT_344DOT_41E),
    ('map', N6gevent4DOT_344DOT_42E),
    ('selclear', N6gevent4DOT_344DOT_43E),
    ('drag_drop', N6gevent4DOT_344DOT_44E),
    ('timer', N6gevent4DOT_344DOT_45E),
    ('control', N6gevent4DOT_344DOT_46E),
    ('user', N6gevent4DOT_344DOT_53E),
]
gevent._fields_ = [
    ('type', event_type),
    ('w', GWindow),
    ('u', N6gevent4DOT_34E),
    ('native_window', c_void_p),
]

# values for enumeration 'window_attr_mask'
wam_events = 2
wam_bordwidth = 4
wam_bordcol = 8
wam_backcol = 16
wam_cursor = 32
wam_wtitle = 64
wam_ititle = 128
wam_icon = 256
wam_nodecor = 512
wam_positioned = 1024
wam_centered = 2048
wam_undercursor = 4096
wam_noresize = 8192
wam_restrict = 16384
wam_redirect = 32768
wam_isdlg = 65536
wam_notrestricted = 131072
wam_transient = 262144
wam_utf8_wtitle = 524288
wam_utf8_ititle = 1048576
wam_cairo = 2097152
wam_verytransient = 4194304
window_attr_mask = c_int # enum

# values for enumeration 'cursor_types'
ct_default = 0
ct_pointer = 1
ct_backpointer = 2
ct_hand = 3
ct_question = 4
ct_cross = 5
ct_4way = 6
ct_text = 7
ct_watch = 8
ct_draganddrop = 9
ct_invisible = 10
ct_user = 11
ct_user2 = 12
cursor_types = c_int # enum
GCursor = cursor_types
gwindow_attrs._fields_ = [
    ('mask', window_attr_mask),
    ('event_masks', uint32),
    ('border_width', int16),
    ('border_color', Color),
    ('background_color', Color),
    ('cursor', GCursor),
    ('window_title', POINTER(unichar_t)),
    ('icon_title', POINTER(unichar_t)),
    ('icon', POINTER(gwindow)),
    ('nodecoration', c_uint, 1),
    ('positioned', c_uint, 1),
    ('centered', c_uint, 2),
    ('undercursor', c_uint, 1),
    ('noresize', c_uint, 1),
    ('restrict_input_to_me', c_uint, 1),
    ('redirect_chars_to_me', c_uint, 1),
    ('is_dlg', c_uint, 1),
    ('not_restricted', c_uint, 1),
    ('redirect_from', GWindow),
    ('transient', GWindow),
    ('utf8_window_title', STRING),
    ('utf8_icon_title', STRING),
]
gwidgetdata._fields_ = [
]
font_state._fields_ = [
]
uint8_t = c_uint8
uint8 = uint8_t

# values for enumeration 'gzoom_flags'
gzf_pos = 1
gzf_size = 2
gzoom_flags = c_int # enum
class gpoint(Structure):
    pass
GPoint = gpoint
class gimage(Structure):
    pass
GImage = gimage
class font_data(Structure):
    pass
class FontRequest(Structure):
    pass
class FontMods(Structure):
    pass
class GChar2b(Structure):
    pass
class ginput_context(Structure):
    pass
GIC = ginput_context

# values for enumeration 'gic_style'
gic_overspot = 2
gic_root = 1
gic_hidden = 0
gic_orlesser = 4
gic_type = 3
gic_style = c_int # enum
class gdeveventmask(Structure):
    pass
class gprinter_attrs(Structure):
    pass
GPrinterAttrs = gprinter_attrs
class font_instance(Structure):
    pass
GFont = font_instance

# values for enumeration 'gcairo_flags'
gc_buildpath = 1
gc_alpha = 2
gc_xor = 4
gc_pango = 8
gc_all = 3
gcairo_flags = c_int # enum
displayfuncs._fields_ = [
    ('init', CFUNCTYPE(None, POINTER(GDisplay))),
    ('term', CFUNCTYPE(None, POINTER(GDisplay))),
    ('nativeDisplay', CFUNCTYPE(c_void_p, POINTER(GDisplay))),
    ('setDefaultIcon', CFUNCTYPE(None, GWindow)),
    ('createTopWindow', CFUNCTYPE(GWindow, POINTER(GDisplay), POINTER(GRect), CFUNCTYPE(c_int, GWindow, POINTER(GEvent)), c_void_p, POINTER(GWindowAttrs))),
    ('createSubWindow', CFUNCTYPE(GWindow, GWindow, POINTER(GRect), CFUNCTYPE(c_int, GWindow, POINTER(GEvent)), c_void_p, POINTER(GWindowAttrs))),
    ('createPixmap', CFUNCTYPE(GWindow, POINTER(GDisplay), uint16, uint16)),
    ('createBitmap', CFUNCTYPE(GWindow, POINTER(GDisplay), uint16, uint16, POINTER(uint8))),
    ('createCursor', CFUNCTYPE(GCursor, GWindow, GWindow, Color, Color, int16, int16)),
    ('destroyWindow', CFUNCTYPE(None, GWindow)),
    ('destroyCursor', CFUNCTYPE(None, POINTER(GDisplay), GCursor)),
    ('nativeWindowExists', CFUNCTYPE(c_int, POINTER(GDisplay), c_void_p)),
    ('setZoom', CFUNCTYPE(None, GWindow, POINTER(GRect), gzoom_flags)),
    ('setWindowBorder', CFUNCTYPE(None, GWindow, c_int, Color)),
    ('setDither', CFUNCTYPE(c_int, POINTER(GDisplay), c_int)),
    ('reparentWindow', CFUNCTYPE(None, GWindow, GWindow, c_int, c_int)),
    ('setVisible', CFUNCTYPE(None, GWindow, c_int)),
    ('move', CFUNCTYPE(None, GWindow, int32, int32)),
    ('trueMove', CFUNCTYPE(None, GWindow, int32, int32)),
    ('resize', CFUNCTYPE(None, GWindow, int32, int32)),
    ('moveResize', CFUNCTYPE(None, GWindow, int32, int32, int32, int32)),
    ('raise', CFUNCTYPE(None, GWindow)),
    ('raiseAbove', CFUNCTYPE(None, GWindow, GWindow)),
    ('isAbove', CFUNCTYPE(c_int, GWindow, GWindow)),
    ('lower', CFUNCTYPE(None, GWindow)),
    ('setWindowTitles', CFUNCTYPE(None, GWindow, POINTER(unichar_t), POINTER(unichar_t))),
    ('setWindowTitles8', CFUNCTYPE(None, GWindow, STRING, STRING)),
    ('getWindowTitle', CFUNCTYPE(POINTER(unichar_t), GWindow)),
    ('getWindowTitle8', CFUNCTYPE(STRING, GWindow)),
    ('setTransientFor', CFUNCTYPE(None, GWindow, GWindow)),
    ('getPointerPos', CFUNCTYPE(None, GWindow, POINTER(GEvent))),
    ('getPointerWindow', CFUNCTYPE(GWindow, GWindow)),
    ('setCursor', CFUNCTYPE(None, GWindow, GCursor)),
    ('getCursor', CFUNCTYPE(GCursor, GWindow)),
    ('getRedirectWindow', CFUNCTYPE(GWindow, POINTER(GDisplay))),
    ('translateCoordinates', CFUNCTYPE(None, GWindow, GWindow, POINTER(GPoint))),
    ('beep', CFUNCTYPE(None, POINTER(GDisplay))),
    ('flush', CFUNCTYPE(None, POINTER(GDisplay))),
    ('pushClip', CFUNCTYPE(None, GWindow, POINTER(GRect), POINTER(GRect))),
    ('popClip', CFUNCTYPE(None, GWindow, POINTER(GRect))),
    ('clear', CFUNCTYPE(None, GWindow, POINTER(GRect))),
    ('drawLine', CFUNCTYPE(None, GWindow, int32, int32, int32, int32, Color)),
    ('drawArrow', CFUNCTYPE(None, GWindow, int32, int32, int32, int32, int16, Color)),
    ('drawRect', CFUNCTYPE(None, GWindow, POINTER(GRect), Color)),
    ('fillRect', CFUNCTYPE(None, GWindow, POINTER(GRect), Color)),
    ('drawElipse', CFUNCTYPE(None, GWindow, POINTER(GRect), Color)),
    ('fillElipse', CFUNCTYPE(None, GWindow, POINTER(GRect), Color)),
    ('drawArc', CFUNCTYPE(None, GWindow, POINTER(GRect), int32, int32, Color)),
    ('drawPoly', CFUNCTYPE(None, GWindow, POINTER(GPoint), int16, Color)),
    ('fillPoly', CFUNCTYPE(None, GWindow, POINTER(GPoint), int16, Color)),
    ('scroll', CFUNCTYPE(None, GWindow, POINTER(GRect), int32, int32)),
    ('drawImage', CFUNCTYPE(None, GWindow, POINTER(GImage), POINTER(GRect), int32, int32)),
    ('tileImage', CFUNCTYPE(None, GWindow, POINTER(GImage), POINTER(GRect), int32, int32)),
    ('drawGlyph', CFUNCTYPE(None, GWindow, POINTER(GImage), POINTER(GRect), int32, int32)),
    ('drawImageMag', CFUNCTYPE(None, GWindow, POINTER(GImage), POINTER(GRect), int32, int32, int32, int32)),
    ('copyScreenToImage', CFUNCTYPE(POINTER(GImage), GWindow, POINTER(GRect))),
    ('drawPixmap', CFUNCTYPE(None, GWindow, GWindow, POINTER(GRect), int32, int32)),
    ('tilePixmap', CFUNCTYPE(None, GWindow, GWindow, POINTER(GRect), int32, int32)),
    ('scaleFont', CFUNCTYPE(POINTER(font_data), POINTER(GDisplay), POINTER(font_data), POINTER(FontRequest))),
    ('stylizeFont', CFUNCTYPE(POINTER(font_data), POINTER(GDisplay), POINTER(font_data), POINTER(FontRequest))),
    ('loadFontMetrics', CFUNCTYPE(c_void_p, POINTER(GDisplay), POINTER(font_data))),
    ('drawText1', CFUNCTYPE(None, GWindow, POINTER(font_data), int32, int32, STRING, int32, POINTER(FontMods), Color)),
    ('drawText2', CFUNCTYPE(None, GWindow, POINTER(font_data), int32, int32, POINTER(GChar2b), int32, POINTER(FontMods), Color)),
    ('createInputContext', CFUNCTYPE(POINTER(GIC), GWindow, gic_style)),
    ('setGIC', CFUNCTYPE(None, GWindow, POINTER(GIC), c_int, c_int)),
    ('grabSelection', CFUNCTYPE(None, GWindow, selnames)),
    ('addSelectionType', CFUNCTYPE(None, GWindow, selnames, STRING, c_void_p, int32, int32, CFUNCTYPE(c_void_p, c_void_p, POINTER(int32)), CFUNCTYPE(None, c_void_p))),
    ('requestSelection', CFUNCTYPE(c_void_p, GWindow, selnames, STRING, POINTER(int32))),
    ('selectionHasType', CFUNCTYPE(c_int, GWindow, selnames, STRING)),
    ('bindSelection', CFUNCTYPE(None, POINTER(GDisplay), selnames, STRING)),
    ('selectionHasOwner', CFUNCTYPE(c_int, POINTER(GDisplay), selnames)),
    ('pointerUngrab', CFUNCTYPE(None, POINTER(GDisplay))),
    ('pointerGrab', CFUNCTYPE(None, GWindow)),
    ('requestExpose', CFUNCTYPE(None, GWindow, POINTER(GRect), c_int)),
    ('forceUpdate', CFUNCTYPE(None, GWindow)),
    ('sync', CFUNCTYPE(None, POINTER(GDisplay))),
    ('skipMouseMoveEvents', CFUNCTYPE(None, GWindow, POINTER(GEvent))),
    ('processPendingEvents', CFUNCTYPE(None, POINTER(GDisplay))),
    ('processWindowEvents', CFUNCTYPE(None, GWindow)),
    ('processOneEvent', CFUNCTYPE(None, POINTER(GDisplay))),
    ('eventLoop', CFUNCTYPE(None, POINTER(GDisplay))),
    ('postEvent', CFUNCTYPE(None, POINTER(GEvent))),
    ('postDragEvent', CFUNCTYPE(None, GWindow, POINTER(GEvent), event_type)),
    ('requestDeviceEvents', CFUNCTYPE(c_int, GWindow, c_int, POINTER(gdeveventmask))),
    ('requestTimer', CFUNCTYPE(POINTER(GTimer), GWindow, int32, int32, c_void_p)),
    ('cancelTimer', CFUNCTYPE(None, POINTER(GTimer))),
    ('syncThread', CFUNCTYPE(None, POINTER(GDisplay), CFUNCTYPE(None, c_void_p), c_void_p)),
    ('startJob', CFUNCTYPE(GWindow, POINTER(GDisplay), c_void_p, POINTER(GPrinterAttrs))),
    ('nextPage', CFUNCTYPE(None, GWindow)),
    ('endJob', CFUNCTYPE(c_int, GWindow, c_int)),
    ('getFontMetrics', CFUNCTYPE(None, GWindow, POINTER(GFont), POINTER(c_int), POINTER(c_int), POINTER(c_int))),
    ('hasCairo', CFUNCTYPE(gcairo_flags, GWindow)),
    ('startNewPath', CFUNCTYPE(None, GWindow)),
    ('closePath', CFUNCTYPE(None, GWindow)),
    ('moveto', CFUNCTYPE(None, GWindow, c_double, c_double)),
    ('lineto', CFUNCTYPE(None, GWindow, c_double, c_double)),
    ('curveto', CFUNCTYPE(None, GWindow, c_double, c_double, c_double, c_double, c_double, c_double)),
    ('stroke', CFUNCTYPE(None, GWindow, Color)),
    ('fill', CFUNCTYPE(None, GWindow, Color)),
    ('fillAndStroke', CFUNCTYPE(None, GWindow, Color, Color)),
    ('layoutInit', CFUNCTYPE(None, GWindow, STRING, c_int, POINTER(GFont))),
    ('layoutDraw', CFUNCTYPE(None, GWindow, int32, int32, Color)),
    ('layoutIndexToPos', CFUNCTYPE(None, GWindow, c_int, POINTER(GRect))),
    ('layoutXYToIndex', CFUNCTYPE(c_int, GWindow, c_int, c_int)),
    ('layoutExtents', CFUNCTYPE(None, GWindow, POINTER(GRect))),
    ('layoutSetWidth', CFUNCTYPE(None, GWindow, c_int)),
    ('layoutLineCount', CFUNCTYPE(c_int, GWindow)),
    ('layoutLineStart', CFUNCTYPE(c_int, GWindow, c_int)),
]

# values for enumeration 'draw_func'
df_copy = 0
df_xor = 1
draw_func = c_int # enum
ggc._fields_ = [
    ('w', POINTER(gwindow)),
    ('xor_base', int32),
    ('fg', Color),
    ('bg', Color),
    ('clip', GRect),
    ('func', draw_func),
    ('copy_through_sub_windows', c_uint, 1),
    ('bitmap_col', c_uint, 1),
    ('skip_len', int16),
    ('dash_len', int16),
    ('line_width', int16),
    ('ts', int16),
    ('ts_xoff', int32),
    ('ts_yoff', int32),
    ('dash_offset', c_int),
    ('fi', POINTER(GFont)),
]
ggadget._fields_ = [
]
GChar2b._fields_ = [
    ('byte1', c_ubyte),
    ('byte2', c_ubyte),
]
ginput_context._fields_ = [
    ('w', GWindow),
    ('style', gic_style),
    ('ic', c_void_p),
    ('next', POINTER(ginput_context)),
]
font_data._fields_ = [
]

# values for enumeration 'font_style'
fs_none = 0
fs_italic = 1
fs_smallcaps = 2
fs_condensed = 4
fs_extended = 8
font_style = c_int # enum
FontRequest._fields_ = [
    ('family_name', POINTER(unichar_t)),
    ('point_size', int16),
    ('weight', int16),
    ('style', font_style),
    ('utf8_family_name', STRING),
]

# values for enumeration 'text_mods'
tm_none = 0
tm_upper = 1
tm_lower = 2
tm_initialcaps = 4
tm_showsofthyphen = 8
text_mods = c_int # enum

# values for enumeration 'text_lines'
tl_none = 0
tl_under = 1
tl_strike = 2
tl_over = 4
tl_dash = 8
text_lines = c_int # enum

# values for enumeration 'charset'
em_none = -1
em_iso8859_1 = 0
em_iso8859_2 = 1
em_iso8859_3 = 2
em_iso8859_4 = 3
em_iso8859_5 = 4
em_iso8859_6 = 5
em_iso8859_7 = 6
em_iso8859_8 = 7
em_iso8859_9 = 8
em_iso8859_10 = 9
em_iso8859_11 = 10
em_iso8859_13 = 11
em_iso8859_14 = 12
em_iso8859_15 = 13
em_koi8_r = 14
em_jis201 = 15
em_win = 16
em_mac = 17
em_symbol = 18
em_zapfding = 19
em_user = 20
em_adobestandard = 20
em_jis208 = 21
em_jis212 = 22
em_ksc5601 = 23
em_gb2312 = 24
em_big5 = 25
em_big5hkscs = 26
em_johab = 27
em_unicode = 28
em_unicode4 = 29
em_gb18030 = 30
em_max = 31
em_first2byte = 21
em_last94x94 = 24
charset = c_int # enum
FontMods._fields_ = [
    ('letter_spacing', int16),
    ('starts_word', c_uint, 1),
    ('has_charset', c_uint, 1),
    ('mods', text_mods),
    ('lines', text_lines),
    ('charset', charset),
]
font_instance._fields_ = [
]

# values for enumeration 'printer_attr_mask'
pam_pagesize = 1
pam_margins = 2
pam_scale = 4
pam_res = 8
pam_copies = 16
pam_thumbnails = 32
pam_printername = 64
pam_filename = 128
pam_args = 256
pam_color = 512
pam_transparent = 1024
pam_lpr = 2048
pam_queue = 4096
pam_eps = 8192
pam_landscape = 16384
pam_title = 32768
printer_attr_mask = c_int # enum

# values for enumeration 'printer_units'
pu_inches = 0
pu_points = 1
pu_mm = 2
printer_units = c_int # enum
gprinter_attrs._fields_ = [
    ('mask', printer_attr_mask),
    ('width', c_float),
    ('height', c_float),
    ('lmargin', c_float),
    ('rmargin', c_float),
    ('tmargin', c_float),
    ('bmargin', c_float),
    ('scale', c_float),
    ('units', printer_units),
    ('res', int32),
    ('num_copies', int16),
    ('thumbnails', int16),
    ('do_color', c_uint, 1),
    ('do_transparent', c_uint, 1),
    ('use_lpr', c_uint, 1),
    ('donot_queue', c_uint, 1),
    ('landscape', c_uint, 1),
    ('eps', c_uint, 1),
    ('printer_name', STRING),
    ('file_name', STRING),
    ('extra_lpr_args', STRING),
    ('title', POINTER(unichar_t)),
    ('start_page', uint16),
    ('end_page', uint16),
]
gdeveventmask._fields_ = [
    ('event_mask', c_int),
    ('device_name', STRING),
]
class N6gimage4DOT_31E(Union):
    pass
class _GImage(Structure):
    pass
N6gimage4DOT_31E._fields_ = [
    ('image', POINTER(_GImage)),
    ('images', POINTER(POINTER(_GImage))),
]
gimage._fields_ = [
    ('list_len', c_short),
    ('u', N6gimage4DOT_31E),
    ('userdata', c_void_p),
]
gpoint._fields_ = [
    ('x', int16),
    ('y', int16),
]
class clut(Structure):
    pass
GClut = clut
_GImage._fields_ = [
    ('image_type', c_uint, 2),
    ('delay', int16),
    ('width', int32),
    ('height', int32),
    ('bytes_per_line', int32),
    ('data', POINTER(uint8)),
    ('clut', POINTER(GClut)),
    ('trans', Color),
]
clut._fields_ = [
    ('clut_len', int16),
    ('is_grey', c_uint, 1),
    ('trans_index', uint32),
    ('clut', Color * 256),
]
__all__ = ['et_resize', 'em_max', 'gc_xor', 'GClut', 'GImage',
           'N6gevent4DOT_344DOT_45E', 'em_iso8859_9',
           'et_lastnativeevent', 'et_mousemove',
           'N6gevent4DOT_344DOT_36E', 'ct_cross', 'em_iso8859_3',
           'em_iso8859_2', 'text_lines', 'et_drag', 'em_iso8859_7',
           'ct_default', 'fs_smallcaps', 'em_iso8859_4',
           'et_controlevent', 'et_map', 'et_focus', 'gc_alpha',
           'uint8_t', 'em_jis212', 'wam_transient', 'sbevent',
           'GDisplay', 'wam_utf8_wtitle', 'et_lastsubtype', 'uint16',
           'clut', 'displayfuncs', 'et_user', 'et_listdoubleclick',
           'wam_icon', 'int32', 'et_radiochanged', 'pam_queue',
           'wam_bordwidth', 'N6gevent4DOT_344DOT_44E', 'ct_pointer',
           'wam_utf8_ititle', 'em_gb2312', 'pam_printername',
           'GCursor', 'GIC', 'pam_pagesize',
           'N6gevent4DOT_344DOT_37E', 'gic_overspot', 'wam_nodecor',
           'et_char', 'em_johab', 'gdeveventmask', 'fs_condensed',
           'N6gevent4DOT_344DOT_464DOT_484DOT_50E',
           'N6gimage4DOT_31E', 'gwindow', 'vs_partially',
           'et_sb_uppage', 'em_none', 'wam_isdlg', 'em_iso8859_8',
           'et_scrollbarchange', 'gzf_size', 'em_first2byte',
           'em_win', 'wam_ititle', 'GWindowAttrs', 'pam_margins',
           'N6gevent4DOT_344DOT_53E', 'et_crossing', 'pu_points',
           'printer_units', 'et_mousedown', 'em_unicode', 'gevent',
           'int16_t', '_GImage', 'N6gevent4DOT_344DOT_43E',
           'gc_buildpath', 'N6gevent4DOT_34E', 'vs_unobscured',
           'fs_none', 'sn_drag_and_drop', 'gzf_pos', 'GRect',
           'em_mac', 'gwindow_attrs', 'wam_bordcol', 'gpoint',
           'em_iso8859_6', 'selnames', 'gimage', 'pam_thumbnails',
           'wam_events', 'wam_backcol',
           'N6gevent4DOT_344DOT_464DOT_484DOT_49E', 'et_visibility',
           'uint32', 'em_big5', 'FontRequest', 'et_charup',
           'font_data', 'pam_scale', 'gic_type', 'tl_strike', 'ggc',
           'N6gevent4DOT_344DOT_35E', 'N6gevent4DOT_344DOT_42E',
           'window_attr_mask', 'wam_wtitle', 'GDrawRequestTimer',
           'et_sb_thumb', 'et_buttonactivate', 'et_create',
           'pam_transparent', 'et_destroy', 'gwidgetdata', 'GPoint',
           'gdisplay', 'FontMods', 'em_koi8_r', 'et_expose',
           'vs_obscured', 'pam_filename', 'ct_backpointer',
           'ct_question', 'em_ksc5601', 'em_big5hkscs', 'uint16_t',
           'wam_restrict', 'N6gevent4DOT_344DOT_41E',
           'printer_attr_mask', 'sn_user1', 'int32_t', 'sn_user2',
           'em_last94x94', 'draw_func',
           'N6gevent4DOT_344DOT_464DOT_48E', 'GChar2b', 'et_sb_down',
           'fs_italic', 'GDrawCancelTimer', 'wam_redirect',
           'em_iso8859_14', 'ginput_context', 'event_type', 'gtimer',
           'et_selclear', 'et_sb_left', 'pam_landscape',
           'et_sb_thumbrelease', 'unichar_t', 'em_jis208',
           'wam_undercursor', 'ct_draganddrop', 'GDrawDestroyWindow',
           'N6gevent4DOT_344DOT_39E', 'GFont', 'GGC', 'gcairo_flags',
           'gic_hidden', 'pam_color', 'tm_initialcaps', 'pam_args',
           'GDrawSetVisible', 'tm_lower', 'pam_title', 'text_mods',
           'et_textchanged', 'et_sb_top', 'uint8', 'visibility_state',
           'et_timer', 'em_zapfding', 'wam_noresize', 'pu_inches',
           'em_user', 'N6gevent4DOT_344DOT_40E', 'font_state',
           'GTimer', 'gic_root', 'grect', 'df_copy', 'tl_over',
           'gprinter_attrs', 'cursor_types', 'et_dragout',
           'fs_extended', 'ct_invisible', 'GDrawCreateTopWindow',
           'tl_dash', 'wam_positioned', 'et_close', 'em_unicode4',
           'gc_all', 'et_drop', 'et_buttonpress', 'sn_max',
           'et_sb_right', 'tl_under', 'tm_upper', 'em_iso8859_1',
           'gc_pango', 'tm_none', 'font_instance', 'em_iso8859_5',
           'pam_lpr', 'em_adobestandard',
           'N6gevent4DOT_344DOT_464DOT_484DOT_51E', 'font_style',
           'em_gb18030', 'ct_user2', 'charset', 'df_xor', 'em_symbol',
           'wam_cursor', 'sn_clipboard', 'gic_style', 'ct_watch',
           'sn_primary', 'et_sb_downpage', 'pam_eps', 'em_jis201',
           'wam_verytransient', 'et_sb_bottom', 'int16',
           'N6gevent4DOT_344DOT_464DOT_484DOT_52E', 'GPrinterAttrs',
           'N6gevent4DOT_344DOT_46E', 'ggadget', 'pam_res', 'GEvent',
           'ct_hand', 'et_mouseup', 'ct_4way', 'gic_orlesser',
           'ct_user', 'pu_mm', 'gzoom_flags', 'ct_text', 'et_noevent',
           'tl_none', 'wam_cairo', 'Color', 'em_iso8859_13',
           'et_listselected', 'wam_centered', 'GWindow',
           'tm_showsofthyphen', 'et_sb_up', 'wam_notrestricted',
           'uint32_t', 'pam_copies', 'sb', 'em_iso8859_15',
           'N6gevent4DOT_344DOT_38E', 'et_textfocuschanged',
           'em_iso8859_11', 'em_iso8859_10']
