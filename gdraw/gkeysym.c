#include <stdio.h>
#include <gdraw.h>
#include <gkeysym.h>

static unichar_t BackSpace[] = { 'B', 'a', 'c', 'k', 'S', 'p', 'a', 'c', 'e', '\0' };
static unichar_t Tab[] = { 'T', 'a', 'b', '\0' };
static unichar_t Linefeed[] = { 'L', 'i', 'n', 'e', 'f', 'e', 'e', 'd', '\0' };
static unichar_t Clear[] = { 'C', 'l', 'e', 'a', 'r', '\0' };
static unichar_t Return[] = { 'R', 'e', 't', 'u', 'r', 'n', '\0' };
static unichar_t Pause[] = { 'P', 'a', 'u', 's', 'e', '\0' };
static unichar_t Scroll_Lock[] = { 'S', 'c', 'r', 'o', 'l', 'l', '_', 'L', 'o', 'c', 'k', '\0' };
static unichar_t Sys_Req[] = { 'S', 'y', 's', '_', 'R', 'e', 'q', '\0' };
static unichar_t Escape[] = { 'E', 's', 'c', 'a', 'p', 'e', '\0' };
static unichar_t Delete[] = { 'D', 'e', 'l', 'e', 't', 'e', '\0' };
static unichar_t Home[] = { 'H', 'o', 'm', 'e', '\0' };
static unichar_t Left[] = { 'L', 'e', 'f', 't', '\0' };
static unichar_t Up[] = { 'U', 'p', '\0' };
static unichar_t Right[] = { 'R', 'i', 'g', 'h', 't', '\0' };
static unichar_t Down[] = { 'D', 'o', 'w', 'n', '\0' };
static unichar_t Page_Up[] = { 'P', 'a', 'g', 'e', 'U', 'p', '\0' };
static unichar_t Page_Down[] = { 'P', 'a', 'g', 'e', 'D', 'o', 'w', 'n', '\0' };
static unichar_t End[] = { 'E', 'n', 'd', '\0' };
static unichar_t Begin[] = { 'B', 'e', 'g', 'i', 'n', '\0' };
static unichar_t KP_Home[] = { 'K', 'P', '_', 'H', 'o', 'm', 'e', '\0' };
static unichar_t KP_Left[] = { 'K', 'P', '_', 'L', 'e', 'f', 't', '\0' };
static unichar_t KP_Up[] = { 'K', 'P', '_', 'U', 'p', '\0' };
static unichar_t KP_Right[] = { 'K', 'P', '_', 'R', 'i', 'g', 'h', 't', '\0' };
static unichar_t KP_Down[] = { 'K', 'P', '_', 'D', 'o', 'w', 'n', '\0' };
static unichar_t KP_Page_Up[] = { 'K', 'P', '_', 'P', 'a', 'g', 'e', 'U', 'p', '\0' };
static unichar_t KP_Page_Down[] = { 'K', 'P', '_', 'P', 'a', 'g', 'e', 'D', 'o', 'w', 'n', '\0' };
static unichar_t KP_End[] = { 'K', 'P', '_', 'E', 'n', 'd', '\0' };
static unichar_t KP_Begin[] = { 'K', 'P', '_', 'B', 'e', 'g', 'i', 'n', '\0' };
static unichar_t F1[] = { 'F', '1', '\0' };
static unichar_t F2[] = { 'F', '2', '\0' };
static unichar_t F3[] = { 'F', '3', '\0' };
static unichar_t F4[] = { 'F', '4', '\0' };
static unichar_t F5[] = { 'F', '5', '\0' };
static unichar_t F6[] = { 'F', '6', '\0' };
static unichar_t F7[] = { 'F', '7', '\0' };
static unichar_t F8[] = { 'F', '8', '\0' };
static unichar_t F9[] = { 'F', '9', '\0' };
static unichar_t F10[] = { 'F', '1', '0', '\0' };
static unichar_t F11[] = { 'F', '1', '1', '\0' };
static unichar_t F12[] = { 'F', '1', '2', '\0' };
static unichar_t F13[] = { 'F', '1', '3', '\0' };
static unichar_t F14[] = { 'F', '1', '4', '\0' };
static unichar_t F15[] = { 'F', '1', '5', '\0' };
static unichar_t F16[] = { 'F', '1', '6', '\0' };
static unichar_t F17[] = { 'F', '1', '7', '\0' };
static unichar_t F18[] = { 'F', '1', '8', '\0' };
static unichar_t F19[] = { 'F', '1', '9', '\0' };
static unichar_t F20[] = { 'F', '2', '0', '\0' };
static unichar_t F21[] = { 'F', '2', '1', '\0' };
static unichar_t F22[] = { 'F', '2', '2', '\0' };
static unichar_t F23[] = { 'F', '2', '3', '\0' };
static unichar_t F24[] = { 'F', '2', '4', '\0' };
static unichar_t F25[] = { 'F', '2', '5', '\0' };
static unichar_t F26[] = { 'F', '2', '6', '\0' };
static unichar_t F27[] = { 'F', '2', '7', '\0' };
static unichar_t F28[] = { 'F', '2', '8', '\0' };
static unichar_t F29[] = { 'F', '2', '9', '\0' };
static unichar_t F30[] = { 'F', '3', '0', '\0' };
static unichar_t F31[] = { 'F', '3', '1', '\0' };
static unichar_t F32[] = { 'F', '3', '2', '\0' };
static unichar_t F33[] = { 'F', '3', '3', '\0' };
static unichar_t F34[] = { 'F', '3', '4', '\0' };
static unichar_t F35[] = { 'F', '3', '5', '\0' };
static unichar_t Shift_L[] = { 'S', 'h', 'i', 'f', 't', '_', 'L', '\0' };
static unichar_t Shift_R[] = { 'S', 'h', 'i', 'f', 't', '_', 'R', '\0' };
static unichar_t Control_L[] = { 'C', 'o', 'n', 't', 'r', 'o', 'l', '_', 'L', '\0' };
static unichar_t Control_R[] = { 'C', 'o', 'n', 't', 'r', 'o', 'l', '_', 'R', '\0' };
static unichar_t Caps_Lock[] = { 'C', 'a', 'p', 's', '_', 'L', 'o', 'c', 'k', '\0' };
static unichar_t Shift_Lock[] = { 'S', 'h', 'i', 'f', 't', '_', 'L', 'o', 'c', 'k', '\0' };
static unichar_t Meta_L[] = { 'M', 'e', 't', 'a', '_', 'L', '\0' };
static unichar_t Meta_R[] = { 'M', 'e', 't', 'a', '_', 'R', '\0' };
static unichar_t Alt_L[] = { 'A', 'l', 't', '_', 'L', '\0' };
static unichar_t Alt_R[] = { 'A', 'l', 't', '_', 'R', '\0' };
static unichar_t Super_L[] = { 'S', 'u', 'p', 'e', 'r', '_', 'L', '\0' };
static unichar_t Super_R[] = { 'S', 'u', 'p', 'e', 'r', '_', 'R', '\0' };
static unichar_t Hyper_L[] = { 'H', 'y', 'p', 'e', 'r', '_', 'L', '\0' };
static unichar_t Hyper_R[] = { 'H', 'y', 'p', 'e', 'r', '_', 'R', '\0' };

unichar_t *GDrawKeysyms[] = { 
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	BackSpace,
	Tab,
	Linefeed,
	Clear,
	NULL,
	Return,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	Pause,
	Scroll_Lock,
	Sys_Req,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	Escape,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	Home,
	Left,
	Up,
	Right,
	Down,
	Page_Up,
	Page_Down,
	End,
	Begin,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	KP_Home,
	KP_Left,
	KP_Up,
	KP_Right,
	KP_Down,
	KP_Page_Up,
	KP_Page_Down,
	KP_End,
	KP_Begin,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	F13,
	F14,
	F15,
	F16,
	F17,
	F18,
	F19,
	F20,
	F21,
	F22,
	F23,
	F24,
	F25,
	F26,
	F27,
	F28,
	F29,
	F30,
	F31,
	F32,
	F33,
	F34,
	F35,
	Shift_L,
	Shift_R,
	Control_L,
	Control_R,
	Caps_Lock,
	Shift_Lock,
	Meta_L,
	Meta_R,
	Alt_L,
	Alt_R,
	Super_L,
	Super_R,
	Hyper_L,
	Hyper_R,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	Delete,
	NULL
};

int GKeysymIsModifier(uint16 keysym) {
    switch(keysym) {
        case GK_Shift_L:
        case GK_Shift_R:
        case GK_Control_L:
        case GK_Control_R:
        case GK_Meta_L:
        case GK_Meta_R:
        case GK_Alt_L:
        case GK_Alt_R:
        case GK_Super_L:
        case GK_Super_R:
        case GK_Hyper_L:
        case GK_Hyper_R:
            return true;
        default:
            return false;
    }
}
