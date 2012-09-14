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

typedef struct hotkey {
    struct dlistnode listnode;
    char   action[HOTKEY_ACTION_MAX_SIZE+1];
    uint16 state;
    uint16 keysym;
    int    isUserDefined;
    char   text[HOTKEY_TEXT_MAX_SIZE+1];
} Hotkey;

extern void    hotkeysLoad();
extern void    hotkeysSave();
extern char*   hotkeysGetKeyDescriptionFromAction( char* action );
extern Hotkey* hotkeyFindByEvent( GWindow w, GEvent *event );
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
