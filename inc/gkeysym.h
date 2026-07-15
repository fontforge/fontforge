#ifndef FONTFORGE_GKEYSYM_H
#define FONTFORGE_GKEYSYM_H

#include <fontforge-config.h>

#include <gdk/gdkkeysyms.h>

#define GK_Special         0xfe00   /* keysyms above this value aren't unicode */
#define GK_TypeAhead       0x0000    /* no single keysym, unicode value in event string */

#define GK_BackSpace       GDK_KEY_BackSpace
#define GK_Tab             GDK_KEY_Tab
#define GK_BackTab         GDK_KEY_ISO_Left_Tab

#define GK_Linefeed        GDK_KEY_Linefeed
#define GK_Clear           GDK_KEY_Clear
#define GK_Return          GDK_KEY_Return
#define GK_Pause           GDK_KEY_Pause
#define GK_Scroll_Lock     GDK_KEY_Scroll_Lock
#define GK_Sys_Req         GDK_KEY_Sys_Req
#define GK_Escape          GDK_KEY_Escape
#define GK_Delete          GDK_KEY_Delete

#define GK_Home            GDK_KEY_Home
#define GK_Left            GDK_KEY_Left
#define GK_Up              GDK_KEY_Up
#define GK_Right           GDK_KEY_Right

#define GK_Down            GDK_KEY_Down
#define GK_Prior           GDK_KEY_Prior
#define GK_Page_Up         GDK_KEY_Page_Up
#define GK_Next            GDK_KEY_Next
#define GK_Page_Down       GDK_KEY_Page_Down
#define GK_End             GDK_KEY_End
#define GK_Begin           GDK_KEY_Begin

#define GK_Menu            GDK_KEY_Menu
#define GK_Help            GDK_KEY_Help

#define GK_KP_Enter        GDK_KEY_KP_Enter
#define GK_KP_Home         GDK_KEY_KP_Home
#define GK_KP_Left         GDK_KEY_KP_Left
#define GK_KP_Up           GDK_KEY_KP_Up
#define GK_KP_Right        GDK_KEY_KP_Right
#define GK_KP_Down         GDK_KEY_KP_Down
#define GK_KP_Prior        GDK_KEY_KP_Prior
#define GK_KP_Page_Up      GDK_KEY_KP_Page_Up
#define GK_KP_Next         GDK_KEY_KP_Next
#define GK_KP_Page_Down    GDK_KEY_KP_Page_Down
#define GK_KP_End          GDK_KEY_KP_End
#define GK_KP_Begin        GDK_KEY_KP_Begin

#define GK_F1              GDK_KEY_F1
#define GK_F2              GDK_KEY_F2
#define GK_F3              GDK_KEY_F3
#define GK_F4              GDK_KEY_F4
#define GK_F5              GDK_KEY_F5
#define GK_F6              GDK_KEY_F6
#define GK_F7              GDK_KEY_F7
#define GK_F8              GDK_KEY_F8
#define GK_F9              GDK_KEY_F9
#define GK_F10             GDK_KEY_F10
#define GK_F11             GDK_KEY_F11
#define GK_L1              GDK_KEY_L1
#define GK_F12             GDK_KEY_F12
#define GK_L2              GDK_KEY_L2
#define GK_F13             GDK_KEY_F13
#define GK_L3              GDK_KEY_L3
#define GK_F14             GDK_KEY_F14
#define GK_L4              GDK_KEY_L4
#define GK_F15             GDK_KEY_F15
#define GK_L5              GDK_KEY_L5
#define GK_F16             GDK_KEY_F16
#define GK_L6              GDK_KEY_L6
#define GK_F17             GDK_KEY_F17
#define GK_L7              GDK_KEY_L7
#define GK_F18             GDK_KEY_F18
#define GK_L8              GDK_KEY_L8
#define GK_F19             GDK_KEY_F19
#define GK_L9              GDK_KEY_L9
#define GK_F20             GDK_KEY_F20
#define GK_L10             GDK_KEY_L10
#define GK_F21             GDK_KEY_F21
#define GK_R1              GDK_KEY_R1
#define GK_F22             GDK_KEY_F22
#define GK_R2              GDK_KEY_R2
#define GK_F23             GDK_KEY_F23
#define GK_R3              GDK_KEY_R3
#define GK_F24             GDK_KEY_F24
#define GK_R4              GDK_KEY_R4
#define GK_F25             GDK_KEY_F25
#define GK_R5              GDK_KEY_R5
#define GK_F26             GDK_KEY_F26
#define GK_R6              GDK_KEY_R6
#define GK_F27             GDK_KEY_F27
#define GK_R7              GDK_KEY_R7
#define GK_F28             GDK_KEY_F28
#define GK_R8              GDK_KEY_R8
#define GK_F29             GDK_KEY_F29
#define GK_R9              GDK_KEY_R9
#define GK_F30             GDK_KEY_F30
#define GK_R10             GDK_KEY_R10
#define GK_F31             GDK_KEY_F31
#define GK_R11             GDK_KEY_R11
#define GK_F32             GDK_KEY_F32
#define GK_R12             GDK_KEY_R12
#define GK_F33             GDK_KEY_F33
#define GK_R13             GDK_KEY_R13
#define GK_F34             GDK_KEY_F34
#define GK_R14             GDK_KEY_R14
#define GK_F35             GDK_KEY_F35
#define GK_R15             GDK_KEY_R15

#define GK_Shift_L         GDK_KEY_Shift_L
#define GK_Shift_R         GDK_KEY_Shift_R
#define GK_Control_L       GDK_KEY_Control_L
#define GK_Control_R       GDK_KEY_Control_R
#define GK_Caps_Lock       GDK_KEY_Caps_Lock
#define GK_Shift_Lock      GDK_KEY_Shift_Lock

#define GK_Meta_L          GDK_KEY_Meta_L
#define GK_Meta_R          GDK_KEY_Meta_R
#define GK_Alt_L           GDK_KEY_Alt_L
#define GK_Alt_R           GDK_KEY_Alt_R
#define GK_Super_L         GDK_KEY_Super_L
#define GK_Super_R         GDK_KEY_Super_R
#define GK_Hyper_L         GDK_KEY_Hyper_L
#define GK_Hyper_R         GDK_KEY_Hyper_R

#define GK_Mode_switch     GDK_KEY_Mode_switch


int GKeysymIsModifier(uint16_t keysym);

#endif /* FONTFORGE_GKEYSYM_H */
