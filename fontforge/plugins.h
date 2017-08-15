/* Copyright (C) 2005-2012 by George Williams */
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

#ifndef FONTFORGE_PLUGINS_H
#define FONTFORGE_PLUGINS_H

/* If a user wants to write a fontforge plugin s/he should include this file */

/*
    I envision that there will eventually be three types of plug-ins. At the
    moment I am only supporting two of them.

    * A plug in which adds a new encoding to the encoding/force encoding
    	menus.
    * A plug in which adds a new scripting command
    * A plug in which adds a new menu item.
	(I haven't figure out how I want to do this last. So I'm not currently
	 supporting it.)

    A plug-in should be a dynamic library.
    Plug-ins will be loaded at start up if they are in the default plugin
    	directory, or a script may explicitly invoke a LoadPlugin() call
	and pass a filename.
    Each plug-in should contain an entry-point:
    	void FontForgeInit(void);
    When FF loads a plug-in it will call this entry point.
    I expect that this routine in term will call one (or more) of the install
	routines (though it can do whatever it likes):
	* AddEncoding(name,enc-to-unicode-func,unicode-to-enc-func)
	* AddScriptingCommand(name,func,needs-font)

    Once loaded, there is no way to remove a plug in -- but you can map a
    plug in's name to do something else.

    I am presuming that plugins will be linked against libfontforge and
    that it will have access to all routines declared in fontforge's
    header files. I do not expect to turn this into a real library with
    a true API. It's just a catch all bag of routines I have needed.
    It's not documented either.
*/

 /* Entry point all plugins must contain */
extern int FontForgeInit(void);
    /* If the load fails, then this routine should return 0, else 1 */
    /*  if it returns 0, fontforge will dlclose the shared lib	    */
    /*  FontForge will not complain itself. FontForgeInit should    */
    /*  call LogError (or gwwv_post_error or whatever) if it wants  */
    /*  to report failure */

 /* AddScriptingCommand is documented within */
#include "scripting.h"

/* AddEncoding is documented within */
#include "encoding.h"

 /* Internal routines. Plugins shouldn't need these */
extern int LoadPlugin(const char *dynamic_lib_name);
    /* Loads a single plugin file */
extern void LoadPluginDir(const char *search_path);
    /* Loads any dynamic libs from this search path. if search_path is
     * NULL loads from the path returned by lt_dlgetsearchpath() */

#endif /* FONTFORGE_PLUGINS_H */
