/* Copyright (C) 2000-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fontforge-config.h>

#include "gdraw.h"
#include "gkeysym.h"

struct keysymentry {
    const char *name;
    int keysym;
};

static struct keysymentry keysym_names[] = {
    {"Alt_L", GK_Alt_L},
    {"Alt_R", GK_Alt_R},
    {"BackSpace", GK_BackSpace},
    {"BackTab", GK_BackTab},
    {"Begin", GK_Begin},
    {"Caps_Lock", GK_Caps_Lock},
    {"Clear", GK_Clear},
    {"Control_L", GK_Control_L},
    {"Control_R", GK_Control_R},
    {"Delete", GK_Delete},
    {"Down", GK_Down},
    {"End", GK_End},
    {"Escape", GK_Escape},
    {"F1", GK_F1},
    {"F10", GK_F10},
    {"F11", GK_F11},
    {"F12", GK_F12},
    {"F13", GK_F13},
    {"F14", GK_F14},
    {"F15", GK_F15},
    {"F16", GK_F16},
    {"F17", GK_F17},
    {"F18", GK_F18},
    {"F19", GK_F19},
    {"F2", GK_F2},
    {"F20", GK_F20},
    {"F21", GK_F21},
    {"F22", GK_F22},
    {"F23", GK_F23},
    {"F24", GK_F24},
    {"F25", GK_F25},
    {"F26", GK_F26},
    {"F27", GK_F27},
    {"F28", GK_F28},
    {"F29", GK_F29},
    {"F3", GK_F3},
    {"F30", GK_F30},
    {"F31", GK_F31},
    {"F32", GK_F32},
    {"F33", GK_F33},
    {"F34", GK_F34},
    {"F35", GK_F35},
    {"F4", GK_F4},
    {"F5", GK_F5},
    {"F6", GK_F6},
    {"F7", GK_F7},
    {"F8", GK_F8},
    {"F9", GK_F9},
    {"Help", GK_Help},
    {"Home", GK_Home},
    {"Hyper_L", GK_Hyper_L},
    {"Hyper_R", GK_Hyper_R},
    {"KP_Begin", GK_KP_Begin},
    {"KP_Down", GK_KP_Down},
    {"KP_End", GK_KP_End},
    {"KP_Enter", GK_KP_Enter},
    {"KP_Home", GK_KP_Home},
    {"KP_Left", GK_KP_Left},
    {"KP_Page_Down", GK_KP_Page_Down},
    {"KP_Page_Up", GK_KP_Page_Up},
    {"KP_Right", GK_KP_Right},
    {"KP_Up", GK_KP_Up},
    {"Left", GK_Left},
    {"Linefeed", GK_Linefeed},
    {"Menu", GK_Menu},
    {"Meta_L", GK_Meta_L},
    {"Meta_R", GK_Meta_R},
    {"Next", GK_Next},
    {"Page_Down", GK_Page_Down},
    {"Page_Up", GK_Page_Up},
    {"Pause", GK_Pause},
    {"Prior", GK_Prior},
    {"Return", GK_Return},
    {"Right", GK_Right},
    {"Scroll_Lock", GK_Scroll_Lock},
    {"Shift_L", GK_Shift_L},
    {"Shift_Lock", GK_Shift_Lock},
    {"Shift_R", GK_Shift_R},
    {"Super_L", GK_Super_L},
    {"Super_R", GK_Super_R},
    {"Sys_Req", GK_Sys_Req},
    {"Tab", GK_Tab},
    {"Up", GK_Up},
};

static int keysym_comp(const void *k, const void *v) {
    return strcmp((const char*)k, ((const struct keysymentry*)v)->name);
}

int GKeysymFromName(const char *name) {
    const struct keysymentry* elem = bsearch(name, keysym_names,
        sizeof(keysym_names)/sizeof(keysym_names[0]), sizeof(keysym_names[0]),
        keysym_comp);
    if (elem) {
        return elem->keysym;
    }
    return 0;
}

int GKeysymIsModifier(uint32_t keysym) {
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
