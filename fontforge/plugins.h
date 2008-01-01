/* Copyright (C) 2005-2008 by George Williams */
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

/* If a user wants to write a fontforge plugin s/he should include this file */

/*
    I envision that there will eventually be three types of plug-ins. At the
    moment I am only supporing two of them.

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

 /* AddEncoding is documented here */
typedef int (*EncFunc)(int);
extern int AddEncoding(char *name,EncFunc enc_to_uni,EncFunc uni_to_enc,int max);
	/* The "Encoding" here is a little different from what you normally see*/
	/*  It isn't a mapping from a byte stream to unicode, but from an int */
	/*  to unicode. If we have an 8/16 encoding (EUC or SJIS) then the */
	/*  single byte entries will be numbers less than <256 and the */
	/*  multibyte entries will be numbers >=256. So an encoding might be */
	/*  valid for the domain [0x20..0x7f] [0xa1a1..0xfefe] */
	/* In other words, we're interested in the ordering displayed in the */
	/*  fontview. Nothing else */
	/* The max value need not be exact (though it should be at least as big)*/
	/*  if you create a new font with the given encoding, then the font will */
	/*  have max slots in it by default */
	/* A return value of -1 (from an EncFunc) indicates no mapping */
	/* AddEncoding returns 1 if the encoding was added, 2 if it replaced */
	/*  an existing encoding, 0 if you attempt to replace a builtin */
	/*  encoding */


 /* Internal routines. Plugins shouldn't need these */
extern void LoadPlugin(char *dynamic_lib_name);
    /* Loads a single plugin file */
extern void LoadPluginDir(char *dir);
    /* Loads any dynamic libs from this directory. if dir is NULL loads from */
    /*  default directory */
