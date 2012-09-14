/* Copyright (C) 2012 by Ben Martin */
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

#ifndef _HOTKEYS_H
#define _HOTKEYS_H

#include "basics.h"
#include "gwidget.h"
#include "dlist.h"

#define HOTKEY_ACTION_MAX_SIZE 200
#define HOTKEY_TEXT_MAX_SIZE   100

/**
 * A hotkey binds some keyboard combination to an abstract "action"
 *
 * A major use of the action is to be able to pick off menu items from
 * various windows as an action path. For example,
 * CharView.Menu.File.Open is an action for the file/open menu item in
 * the charview/glyph editing window.
 * 
 */
typedef struct hotkey {
    /**
     * Hotkeys are stored in a doubly linked list. Having this entry
     * first makes a pointer to a hotkey able to e treated as a list
     * node.
     */
    struct dlistnode listnode;

    /**
     * The name of the action this hotkey is to perform. For example,
     * CharView.Menu.File.Open
     */
    char   action[HOTKEY_ACTION_MAX_SIZE+1];

    /**
     * A directly machine usable represetation of the modifiers that
     * must be in use for this hotkey to be fired. For example, shift,
     * control etc.
     */
    uint16 state;

    /**
     * A directly machine usable represetation of the key that is to
     * be pressed fo r this hotkey. This would be a number for a key
     * so that 'k' might be 642
     */
    uint16 keysym;

    /**
     * If this hotkey is user defined this is true. If this is true
     * then the hotkey should be saved back to the user
     * ~/.FontForge/hotkeys file instead of any system file.
     */
    int    isUserDefined;

    /**
     * The plain text representation for the key combination that is
     * to be pressed for this hotkey. For example, "Ctrl+k". Note that
     * the modifiers are not localized in this string, it is as it
     * would appear in the hotkeys file and system hotkeys
     * definitations.
     */
    char   text[HOTKEY_TEXT_MAX_SIZE+1];
} Hotkey;

/**
 * Load the list of hotkey from both system and user catalogs. These
 * include the locale versions like en_GB which provide the shipped
 * defaults and the user overrides in ~/.FontForge/hotkeys.
 */
extern void    hotkeysLoad();

/**
 * Save all user defined hotkeys back to ~/.FontForge/hotkeys.
 */
extern void    hotkeysSave();

/**
 * Return the non localized string definition of the keys that must
 * be pressed for this action. The return value might be Shift+Ctl+8
 */
extern char*   hotkeysGetKeyDescriptionFromAction( char* action );

/**
 * Find the hotkey that matches the given event for the given window.
 * The window is needed because hotkeys can bind to specific windows
 * like the fontview, metricsview or charview.
 */
extern Hotkey* hotkeyFindByEvent( GWindow w, GEvent *event );

/**
 * Strip off any modifier definitation from the given fill hotkey string
 * definition. Given a string like Shift+Ctl+8 returns 8.
 * No memory allocations are performed, the return value is null or a pointer
 * into the hktext string.
 */
extern char*   hotkeyTextWithoutModifiers( char* hktext );



#if 0

/**
 * A function that is invoked when the user presses a collection of
 * keys that they have defined should perform an action
 */
typedef void (*CVHotkeyFunc)(CharView *cv,GEvent *event);

/**
 * A cvhotkey holds the text descrition of a hotkey in keydesc, a
 * preparsed version of that hotkey definition in Hotkey, and a
 * callback function that should be called when that hotkey is
 * pressed. Note that as the Hotkey element is the last in the struct
 * is can be left out of a definition when creating an array of
 * cvhotkey elements. The Hotkey struct can later be initialized when
 * the resources are read from the user's theme file.
 */
typedef struct cvhotkey {
    CVHotkeyFunc func;
    void*        udata;
    Hotkey       hk;
} CVHotkey;

#endif


#endif // _HOTKEYS_H
