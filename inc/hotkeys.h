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

#ifndef FONTFORGE_HOTKEYS_H
#define FONTFORGE_HOTKEYS_H

#include "basics.h"
#include "dlist.h"
#include "gdraw.h"

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
 * This namespace convention uses dot separated strings. The window
 * type as the first string, followed by "Menu", followed by each menu
 * item on the path down to the menu that should be invoked when the
 * hotkey is pressed.
 *
 * The same system can be extended to allow python code to be
 * executed. The current plan is to use Python instead of Menu. There
 * is no reason that the syntax can not be varied slightly for Python,
 * for example to allow something like
 *
 * CharView.Python.MyFooFunction( 7, "fdsfsd" ): Alt+t
 *
 * Where everything after the "Python." is to be the name of a valid
 * python function to execute, in this case a call to MyFooFunction.
 * While we could allow raw python code inthere too, it's probably
 * simpler for the fontforge code to rely on calling a function
 * instead. There is no reason that arguments can not be supplied as
 * above.
 *
 * The hotkeys are actually made effective (ie execute something), in
 * the GMenuBarCheckKey() function in gmenu.c. That code would need
 * some extending to allow the execution of a python function instead
 * of the current code which only executes a menu item which is named
 * in the action.
 *
 * I am tempted there to move the execution of an action hack into
 * hotkeys.c with GMenuBarCheckKey() passing the window and menubar to
 * the new hotkeyExecuteAction() function which itself could handle
 * working out if it is a CharView.Python prefix action and
 * dispatching accordingly. The gain to that is that other code might
 * also like to execute a hotkey independent of the existing code. For
 * example, python code might execute an "action" through that
 * function.
 *
 * The part I haven't personally investigated is the code to dispatch
 * a python function from C. I've done similar with other codebases
 * but haven't seen how it's done in ff. Others might like to put some
 * hints here for whoever (maybe me!) writes the code to allow python
 * dispatch from hotkeys.
 *
 * In a similar way, I was thinking of just having a top level
 * directory of Action.Foo to allow generic, but maybe too low level
 * for a menuitem things to be made available to the hotkey system.
 * Perhaps an Action.ActionsList which just enumerates these actions
 * to the console would be handy to allow folks to see what the code
 * current version of ff offers in terms of actions.
 * 
 */

enum hk_source { hk_ff=1, hk_user=2, hk_python=3 };

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
     * A directly machine usable representation of the modifiers that
     * must be in use for this hotkey to be fired. For example, shift,
     * control etc.
     */
    uint16_t state;

    /**
     * A directly machine usable representation of the key that is to
     * be pressed fo r this hotkey. This would be a number for a key
     * so that 'k' might be 642
     */
    uint16_t keysym;

    /**
     * If this hotkey is user defined this is true. If this is true
     * then the hotkey should be saved back to the user
     * ~/.FontForge/hotkeys file instead of any system file.
     */
    enum hk_source source;

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
extern void    hotkeysLoad(void);

/**
 * Save all user defined hotkeys back to ~/.FontForge/hotkeys.
 */
extern void    hotkeysSave(void);

/**
 * Return the non localized string definition of the keys that must
 * be pressed for this action. The return value might be Shift+Ctl+8
 */
extern char*   hotkeysGetKeyDescriptionFromAction( char* action );

/**
 * Find the hotkey that matches the given event for the given window.
 * The window is needed because hotkeys can bind to specific windows
 * like the fontview, metricsview or charview.
 *
 * Do not free the return value, it's not yours! 
 */
extern Hotkey* hotkeyFindByEvent( GWindow w, GEvent *event );

/**
 * Like hotkeyFindByEvent but this gives you access to all the hotkeys
 * that are to be triggered for the given event on the given window.
 *
 * You should call dlist_free_external() on the return value. The
 * hotkeys returned are not yours to free, but the list nodes that
 * point to the hotkeys *ARE* yours to free.
 */ 
extern struct dlistnodeExternal* hotkeyFindAllByEvent( GWindow w, GEvent *event );

/**
 * Strip off any modifier definitation from the given fill hotkey string
 * definition. Given a string like Shift+Ctl+8 returns 8.
 * No memory allocations are performed, the return value is null or a pointer
 * into the hktext string.
 */
extern char*   hotkeyTextWithoutModifiers( char* hktext );

/**
 * Convert text like Control to the command key unicode value.
 * Caller must free the returned value
 */
extern char* hotkeyTextToMacModifiers( char* keydesc );

/**
 * Given a menu path like File/Open find the hotkey which will trigger
 * that menu item. The window is needed because there might be a
 * menuitem with the same text path in fontview and charview which the
 * user has decided should have different hotkeys
 *
 * Do not free the return value, it's not yours! 
 */
extern Hotkey* hotkeyFindByMenuPath( GWindow w, char* path );
extern Hotkey* hotkeyFindByMenuPathInSubMenu( GWindow w, char* subMenuName, char* path );

/**
 * Immediate keys are hotkeys like the ` key to turn on preview mode in charview.
 * They are perhaps toggle keys, or just keys which code wants to respond to a keypress
 * but also allow the user to configure what that key is using their hotkeys file.
 * Instead of doing event->u.chr.keysym == '`' code can pass the event and a text name
 * like "TogglePreview" and a window (to determine the prefix like CharView) and this
 * function will tell you if that event matches the key that the user has defined to
 * trigger your event.
 */
extern Hotkey* isImmediateKey( GWindow w, char* path, GEvent *event );



/**
 * Set a hotkey to trigger the given action. If append is not true
 * then any other hotkey bindings using keydefinition for the window
 * type specified in the action you passed are first removed.
 *
 * The new hotkey is returned.
 */
extern Hotkey* hotkeySetFull( const char* action, const char* keydefinition, int append, enum hk_source source);

extern int HotkeyParse( Hotkey* hk, const char *shortcut );

/**
 * Set to true if the hotkey system can use the Command key for its
 * own actions.
 */
extern void hotkeySystemSetCanUseMacCommand( int v );
extern int hotkeySystemGetCanUseMacCommand(void);

#endif /* FONTFORGE_HOTKEYS_H */
