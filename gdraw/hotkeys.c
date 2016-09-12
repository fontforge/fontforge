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

#include "gdraw.h"
#include "gfile.h"
#include "hotkeys.h"
#include <locale.h>
#include <string.h>
#include <ustring.h>
#include <errno.h>
#include <unistd.h>
#include "intl.h"

#ifdef __MINGW32__
#define fsync _commit
#endif

static int hotkeySystemCanUseMacCommand = 0;

struct dlistnode* hotkeys = 0;

static char* hotkeyGetWindowTypeString( Hotkey* hk ) 
{
    char* pt = strchr(hk->action,'.');
    if( !pt )
	return 0;
    int len = pt - hk->action;
    static char buffer[HOTKEY_ACTION_MAX_SIZE+1];
    strncpy( buffer, hk->action, len );
    buffer[len] = '\0';
    return buffer;
}

/**
 * Does the hotkey 'hk' have the right window_type to trigger its
 * action on the window 'w'.
 */
static int hotkeyHasMatchingWindowTypeString( char* windowType, Hotkey* hk ) {
    if( !windowType )
	return 0;
    char* pt = strchr(hk->action,'.');
    if( !pt )
	return 0;
    
    int len = pt - hk->action;
    if( strlen(windowType) < len )
	return 0;
    int rc = strncmp( windowType, hk->action, len );
    if( !rc )
	return 1;
    return 0;
}

/**
 * Does the hotkey 'hk' have the right window_type to trigger its
 * action on the window 'w'.
 */
/*
 * Unused
static int hotkeyHasMatchingWindowType( GWindow w, Hotkey* hk ) {
    char* windowType = GDrawGetWindowTypeName( w );
    return hotkeyHasMatchingWindowTypeString( windowType, hk );
}
*/

static struct dlistnodeExternal*
hotkeyFindAllByStateAndKeysym( char* windowType, uint16 state, uint16 keysym ) {

    struct dlistnodeExternal* ret = 0;
    struct dlistnode* node = hotkeys;
    for( ; node; node=node->next ) {
	Hotkey* hk = (Hotkey*)node;
	if( hk->keysym ) {
	    if( keysym == hk->keysym ) {
		if( state == hk->state ) {
		    if( hotkeyHasMatchingWindowTypeString( windowType, hk ) ) {
			dlist_pushfront_external( (struct dlistnode **)&ret, hk );
		    }
		}
	    }
	}
    }
    return ret;
}


static Hotkey* hotkeyFindByStateAndKeysym( char* windowType, uint16 state, uint16 keysym ) {

    struct dlistnode* node = hotkeys;
    for( ; node; node=node->next ) {
	Hotkey* hk = (Hotkey*)node;
	if( hk->keysym ) {
	    if( keysym == hk->keysym ) {
		if( state == hk->state ) {
		    if( hotkeyHasMatchingWindowTypeString( windowType, hk ) ) {
			return hk;
		    }
		}
	    }
	}
    }
    return 0;
}


struct dlistnodeExternal* hotkeyFindAllByEvent( GWindow w, GEvent *event ) {
    if( event->u.chr.autorepeat )
	return 0;
    char* windowType = GDrawGetWindowTypeName( w );
    return hotkeyFindAllByStateAndKeysym( windowType,
					  event->u.chr.state,
					  event->u.chr.keysym );
}


Hotkey* hotkeyFindByEvent( GWindow w, GEvent *event ) {

    if( event->u.chr.autorepeat )
	return 0;

    char* windowType = GDrawGetWindowTypeName( w );
    return hotkeyFindByStateAndKeysym( windowType, event->u.chr.state, event->u.chr.keysym );
}

/**
 * Return the file name of the user defined hotkeys.
 * The return value must be freed by the caller.
 *
 * If extension is not null it will be postpended to the returned path.
 * This way you get to avoid dealing with appending to a string in c.
 */
static char* getHotkeyFilename(char* extension) {
    char *ret = NULL;
    char buffer[1025];
    char *ffdir = getFontForgeUserDir(Config);

    if ( ffdir==NULL ) {
        fprintf(stderr,_("Cannot find your hotkey definition file!\n"));
        return NULL;
    }
    if( !extension )
        extension = "";
    
    sprintf(buffer,"%s/hotkeys%s", ffdir, extension);
    ret = copy(buffer);
    free(ffdir);
    return ret;
}

/**
 * Remove leading and trailing " " characters. N memory allocations
 * are performed, null is injected at the end of string and if there are leading
 * spaces the return value will be past them.
 */
static char* trimspaces( char* line ) {
   while ( line[strlen(line)-1]==' ' )
	line[strlen(line)-1] = '\0';
   while( *line == ' ' )
	++line;
    return line;
}

static Hotkey* hotkeySetFull( char* action, char* keydefinition, int append, int isUserDefined ) 
{
    Hotkey* hk = calloc(1,sizeof(Hotkey));
    if ( hk==NULL ) return 0;
    strncpy( hk->action, action, HOTKEY_ACTION_MAX_SIZE );
    HotkeyParse( hk, keydefinition );

    // If we didn't get a hotkey (No Shortcut)
    // then we move along
    if( !hk->state && !hk->keysym ) {
	free(hk);
	return 0;
    }
	
    // If we already have a binding for that hotkey combination
    // for this window, forget the old one. One combo = One action.
    if( !append ) {
	Hotkey* oldkey = hotkeyFindByStateAndKeysym( hotkeyGetWindowTypeString(hk),
						     hk->state, hk->keysym );
	if( oldkey ) {
	    dlist_erase( &hotkeys, (struct dlistnode *)oldkey );
	    free(oldkey);
	}
    }
	
    hk->isUserDefined = isUserDefined;
    dlist_pushfront( &hotkeys, (struct dlistnode *)hk );
    return hk;
}
Hotkey* hotkeySet( char* action, char* keydefinition, int append )
{
    int isUserDefined = 1;
    return hotkeySetFull( action, keydefinition, append, isUserDefined );
}



/**
 * Load all the hotkeys from the file at filename, marking them as userdefined
 * if isUserDefined is set.
 */
static void loadHotkeysFromFile( const char* filename, int isUserDefined, int warnIfNotFound ) 
{
    char line[1100];
    FILE* f = fopen(filename,"r");
    if( !f ) {
	if( warnIfNotFound ) {
	    fprintf(stderr,_("Failed to open hotkey definition file: %s\n"), filename );
	}
	return;
    }

    while ( fgets(line,sizeof(line),f)!=NULL ) {
	int append = 0;
	
	if ( *line=='#' )
	    continue;
	char* pt = strchr(line,':');
	if ( pt==NULL )
	    continue;
	*pt = '\0';
	char* keydefinition = pt+1;
	chomp( keydefinition );
	keydefinition = trimspaces( keydefinition );
	char* action = line;
	if( line[0] == '+' ) {
	    append = 1;
	    action++;
	}
	
	hotkeySetFull( action, keydefinition, append, isUserDefined );
    }
    fclose(f);
}

/**
 * Load all the default hotkeys for this locale and then the users
 * ~/.Fontforge/hotkeys.
 */
void hotkeysLoad()
{
    char localefn[PATH_MAX+1];
    char* p = 0;
    char* sharedir = getShareDir();

    snprintf(localefn,PATH_MAX,"%s/hotkeys/default", sharedir );
    loadHotkeysFromFile( localefn, false, true );
    
    // FUTURE: perhaps find out how to convert en_AU.UTF-8 that setlocale()
    //   gives to its fallback of en_GB. There are likely to be a bunch of other
    //   languages which are similar but have specific locales
    char* currentlocale = copy(setlocale(LC_MESSAGES, 0));
    snprintf(localefn,PATH_MAX,"%s/hotkeys/%s", sharedir, currentlocale);
    loadHotkeysFromFile( localefn, false, false );
    while((p = strrchr( currentlocale, '.' ))) {
	*p = '\0';
	snprintf(localefn,PATH_MAX,"%s/hotkeys/%s", sharedir, currentlocale);
	loadHotkeysFromFile( localefn, false, false );
    }
    while((p = strrchr( currentlocale, '_' ))) {
	*p = '\0';
	snprintf(localefn,PATH_MAX,"%s/hotkeys/%s", sharedir, currentlocale);
	loadHotkeysFromFile( localefn, false, false );
    }
    free(currentlocale);
    
    char* fn = getHotkeyFilename(0);
    if( !fn ) {
	return;
    }
    loadHotkeysFromFile( fn, true, false );
    free(fn);
}

static void hotkeysSaveCallback(Hotkey* hk,FILE* f) {
    if( hk->isUserDefined ) {
	fprintf( f, "%s:%s\n", hk->action, hk->text );
    }
}

/**
 * Save all the user defined hotkeys back to the users
 * ~/.Fontforge/hotkeys file.
 */
void hotkeysSave() {
    char* fn = getHotkeyFilename(".new");
    if( !fn ) {
	return;
    }
    FILE* f = fopen(fn,"w");
    if( !f ) {
	free(fn);
	fprintf(stderr,_("Failed to open your hotkey definition file for updates.\n"));
	return;
    }

    dlist_foreach_reverse_udata( &hotkeys,
				 (dlist_foreach_udata_func_type)hotkeysSaveCallback, f );
    fsync(fileno(f));
    fclose(f);

    //
    // Atomic rename of new over the old.
    //
    char* newpath = getHotkeyFilename(0);
#ifdef __MINGW32__
    //Atomic rename doesn't exist on Windows.
    unlink(newpath);
#endif
    int rc = rename( fn, newpath );
    int e = errno;
    free(fn);
    free(newpath);
    if( rc == -1 ) {
	fprintf(stderr,_("Failed to rename the new hotkeys file over your old one!\n"));
	fprintf(stderr,_("Reason:%s\n"), strerror(e));
    }
}


char* hotkeysGetKeyDescriptionFromAction( char* action ) {
    struct dlistnode* node = hotkeys;
    for( ; node; node=node->next ) {
	Hotkey* hk = (Hotkey*)node;
	if(!strcmp(hk->action,action)) {
	    return hk->text;
	}
    }
    return 0;
}


/**
 * Find a hotkey by the action. This is useful for menus to find out
 * what hotkey is currently bound to them. So if the user changes
 * file/open to be alt+j then the menu can adjust the hotkey is is
 * displaying to show the user what key they have assigned.
 */
static Hotkey* hotkeyFindByAction( char* action ) {
    struct dlistnode* node = hotkeys;
    for( ; node; node=node->next ) {
	Hotkey* hk = (Hotkey*)node;
	if(!strcmp(hk->action,action)) {
	    return hk;
	}
    }
    return 0;
}

Hotkey* isImmediateKey( GWindow w, char* path, GEvent *event )
{
    char* wt = GDrawGetWindowTypeName(w);
    if(!wt)
	return 0;

    char* subMenuName = "_ImmediateKeys";
    char line[PATH_MAX+1];
    snprintf(line,PATH_MAX,"%s.%s.%s",wt, subMenuName, path );
//    printf("line:%s\n",line);
    Hotkey* hk = hotkeyFindByAction( line );
    if( !hk )
	return 0;
    if( !hk->action )
	return 0;
    
    if( event->u.chr.keysym == hk->keysym )
	return hk;

    return 0;
}

Hotkey* hotkeyFindByMenuPathInSubMenu( GWindow w, char* subMenuName, char* path ) {

    char* wt = GDrawGetWindowTypeName(w);
    if(!wt)
	return 0;

    char line[PATH_MAX+1];
    snprintf(line,PATH_MAX,"%s.%s%s%s",wt, subMenuName, ".Menu.", path );
//    printf("line:%s\n",line);
    return(hotkeyFindByAction(line));
}

Hotkey* hotkeyFindByMenuPath( GWindow w, char* path ) {

    char* wt = GDrawGetWindowTypeName(w);
    if(!wt)
	return 0;

    char line[PATH_MAX+1];
    snprintf(line,PATH_MAX,"%s%s%s",wt, ".Menu.", path );
    return(hotkeyFindByAction(line));
}


char* hotkeyTextToMacModifiers( char* keydesc )
{
    keydesc = copy( keydesc );
    keydesc = str_replace_all( keydesc, "Ctl", "⌘", 1 );
    keydesc = str_replace_all( keydesc, "Command", "⌘", 1 );
    keydesc = str_replace_all( keydesc, "Cmd", "⌘", 1 );
    keydesc = str_replace_all( keydesc, "Shft", "⇧", 1 );
    keydesc = str_replace_all( keydesc, "Alt", "⎇", 1 );
    keydesc = str_replace_all( keydesc, "+", "", 1 );
    return keydesc;
}


char* hotkeyTextWithoutModifiers( char* hktext ) {
    if( !strcmp( hktext, "no shortcut" )
	|| !strcmp( hktext, "No shortcut" )
	|| !strcmp( hktext, "No Shortcut" ))
	return "";
    
    char* p = strrchr( hktext, '+' );
    if( !p )
	return hktext;

    //
    // Handle Control++ by moving back over the last plus
    //
    if( p > hktext )
    {
	char* pp = p - 1;
	if( *pp == '+' )
	    --p;
    }
    return p+1;
}


void hotkeySystemSetCanUseMacCommand( int v )
{
    hotkeySystemCanUseMacCommand = v;
}

int hotkeySystemGetCanUseMacCommand()
{
    return hotkeySystemCanUseMacCommand;
}
