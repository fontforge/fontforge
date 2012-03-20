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
#include "fontP.h"
#include "utype.h"
#include "ustring.h"

static enum font_style default_type=ft_serif;

struct fontabbrev _gdraw_fontabbrev[] = {
    { "times", ft_serif },
    { "Helvetica", ft_sans },
    { "Courier", ft_mono },

    { "sans-serif", ft_sans, false, false, true },
    { "serif", ft_serif, false, false, true },
    { "cursive", ft_cursive, false, false, true },
    { "monospace", ft_mono, false, false, true },
    { "fantasy", ft_sans, false, false, true },

    { "Albertus", ft_serif },
    { "Arial", ft_sans },
    { "Avant Garde", ft_sans },
    { "Bembo", ft_serif },
    { "Bodoni", ft_serif },
    { "Book Antiqua", ft_serif },
    { "Book Rounded", ft_serif },
    { "Bookman", ft_serif },
    { "Caslon", ft_serif },
    { "Century Sch", ft_serif },	/* New century Schoolbook */
    { "charter", ft_serif },
    { "Charcoal", ft_mono, false, true },
    { "Cheltenham", ft_serif },
    { "ChelthmITC", ft_serif },
    { "Chicago", ft_mono, false, true },
    { "Clarendon", ft_mono },
    { "Coronel", ft_cursive },
    { "Eurostyle", ft_sans },
    { "Fixed", ft_mono },
    { "Footlight", ft_serif },
    { "Futura", ft_sans },
    { "Galliard", ft_serif },
    { "Garamond", ft_serif },
    { "Geneva", ft_sans },
    { "Gill Sans", ft_sans },
    { "Gothic", ft_sans },
    { "Goudy", ft_serif },
    { "Grotesque", ft_serif },
    { "Impact Bold", ft_sans, false, true },
    { "lucidatypewriter", ft_mono },
    { "lucidabright", ft_serif },
    { "lucida", ft_sans },
    { "Marigold", ft_cursive },
    { "Melior", ft_serif },
    { "Monaco", ft_mono },
    { "MS Linedraw", ft_serif },
    { "MS Sans Serif", ft_sans },
    { "MS Serif", ft_serif },
    { "New York", ft_serif },
    { "Omega", ft_sans },
    { "Optima", ft_sans },
    { "Palatino", ft_serif },
    { "Roman", ft_serif },
    { "script", ft_cursive },
    { "terminal", ft_mono },
    { "Univers", ft_sans },
    { "Wide Latin", ft_serif },
    { "Zapf Chancery", ft_cursive },
    
    { NULL }
};

int _GDraw_ClassifyFontName(unichar_t *fontname, int *italic, int *bold) {
    int i;

    *italic = *bold = 0;

    for ( i=0; _gdraw_fontabbrev[i].abbrev!=NULL; ++i )
    if ( uc_strstrmatch(fontname,_gdraw_fontabbrev[i].abbrev)!=NULL ) {
	*italic = _gdraw_fontabbrev[i].italic;
	*bold = _gdraw_fontabbrev[i].bold;
return( _gdraw_fontabbrev[i].ft);
    }
return( ft_unknown );
}

static int IsUserMap(unichar_t *setname) {
    extern unichar_t **usercharset_names;
    int i;

    if ( usercharset_names!=NULL ) {
	for ( i=0; usercharset_names[i]!=NULL; ++i )
	    if ( u_strstrmatch(setname,usercharset_names[i])!=NULL )
return( true );
    }
return( false );
}

enum charset _GDraw_ParseMapping(unichar_t *setname) {
    unichar_t *pt;
    int val;

    if ( uc_strstrmatch(setname,"iso")!=NULL && uc_strstrmatch(setname,"10646")!=NULL )
return( em_unicode );
    else if ( uc_strstrmatch(setname,"UnicodePlane")!=NULL ) {
	pt = u_strchr(setname,'-');
	if ( pt==NULL )
return( em_uplane0+1 );
return( em_uplane0+u_strtol(pt+1,NULL,10) );
    } else if ( uc_strstrmatch(setname,"unicode")!=NULL )
return( em_unicode );

#if 0
    if ( uc_strstrmatch(setname,"ascii")!=NULL ||
	    ( uc_strstrmatch(setname,"iso")!=NULL && uc_strstrmatch(setname,"646")!=NULL )) {
	char *lang = getenv( "LANG" );
	if ( lang==NULL || *lang=='\0' || (*lang=='e' && *lang=='n' ))
return( em_iso8859_1 );		/* ascii can masquarade as iso8859-1 for english speakers (no accents needed) */
    }
#endif

    if ( uc_strstrmatch(setname,"iso")!=NULL && uc_strstrmatch(setname,"8859")!=NULL ) {
	pt = uc_strstrmatch(setname,"8859");
	pt += 4;
	if ( *pt=='-' ) ++pt;
	if ( !isdigit(*pt) )
	    /* Bad */;
	else if ( !isdigit(pt[1]) )
return( em_iso8859_1+*pt-'1' );
	else {
	    val = (pt[0]-'0')*10 + pt[1]-'0';
	    switch ( val ) {
	      case 10: case 11:
return( em_iso8859_10+val-10 );
	      case 13: case 14: case 15:
return( em_iso8859_13+val-13 );
	    }
	}
    }

    if ( uc_strstrmatch(setname,"latin1")!=NULL )
return( em_iso8859_1 );
    else if ( uc_strstrmatch(setname,"latin2")!=NULL )
return( em_iso8859_2 );
    else if ( uc_strstrmatch(setname,"latin3")!=NULL )
return( em_iso8859_3 );
    else if ( uc_strstrmatch(setname,"latin4")!=NULL )
return( em_iso8859_4 );
    else if ( uc_strstrmatch(setname,"latin5")!=NULL )
return( em_iso8859_9 );
    else if ( uc_strstrmatch(setname,"latin6")!=NULL )
return( em_iso8859_10 );
    else if ( uc_strstrmatch(setname,"latin7")!=NULL )
return( em_iso8859_13 );
    else if ( uc_strstrmatch(setname,"latin8")!=NULL )
return( em_iso8859_14 );
    else if ( uc_strstrmatch(setname,"latin0")!=NULL || uc_strstrmatch(setname,"latin9")!=NULL )
return( em_iso8859_15 );

    if ( uc_strstrmatch(setname,"koi8")!=NULL )
return( em_koi8_r );

    if ( uc_strstrmatch(setname,"cyrillic")!=NULL )
return( em_iso8859_5 );		/* This is grasping at straws */
    else if ( uc_strstrmatch(setname,"greek")!=NULL )
return( em_iso8859_7 );		/* This is grasping at straws */
    else if ( uc_strstrmatch(setname,"arabic")!=NULL )
return( em_iso8859_6 );		/* This is grasping at straws */
    else if ( uc_strstrmatch(setname,"hebrew")!=NULL )
return( em_iso8859_8 );		/* This is grasping at straws */
    else if ( uc_strstrmatch(setname,"thai")!=NULL || uc_strstrmatch(setname,"tis")!=NULL )
return( em_iso8859_11 );

    if ( uc_strstrmatch(setname,"jis")!=NULL ) {
	if ( uc_strstrmatch(setname,"201")!=NULL )
return( em_jis201 );
	if ( uc_strstrmatch(setname,"208")!=NULL )
return( em_jis208 );
	if ( uc_strstrmatch(setname,"212")!=NULL )
return( em_jis212 );
	if ( uc_strstrmatch(setname,"213")!=NULL )	/* I don't support 213 */
return( em_none );

return( em_jis208 );
    }

    if ( uc_strstrmatch(setname,"ksc")!=NULL && uc_strstrmatch(setname,"5601")!=NULL )
return( em_ksc5601 );	/* Seem to be several versions of 5601, we want 94x94 */

    if ( uc_strstrmatch(setname,"gb")!=NULL && uc_strstrmatch(setname,"2312")!=NULL )
return( em_gb2312 );
    if ( uc_strstrmatch(setname,"big5")!=NULL )
return( em_big5 );

    if ( uc_strstrmatch(setname,"mac")!=NULL )
return( em_mac );
    if ( uc_strstrmatch(setname,"win")!=NULL )
return( em_win );

    if ( IsUserMap(setname))
return( em_user );

/* !!! Encodings used for postscript japanese fonts, which I don't understand */
#if 0
    if ( uc_strstrmatch(setname,"RJSJ")!=NULL )
return( em_sjis );
    if ( uc_strstrmatch(setname,"EUC")!=NULL )
return( em_euc );
#endif

return( em_none );
}

struct keyword { char *name; int val; };
static struct keyword weights[] = {
    { "extra_light", 100 },
    { "extra-light", 100 },
    { "extralight", 100 },
    { "light", 200 },
    { "demi_light", 300 },
    { "demi-light", 300 },
    { "demilight", 300 },
    { "book", 400 },
    { "regular", 400 },
    { "roman", 400 },
    { "normal", 400 },
    { "medium", 500 },
    { "demi_bold", 600 },
    { "demi-bold", 600 },
    { "demibold", 600 },
    { "demi", 600 },
    { "bold", 700 },
    { "demi_black", 800 },
    { "demi-black", 800 },
    { "demiblack", 800 },
    { "extra_bold", 800 },
    { "extra-bold", 800 },
    { "extrabold", 800 },
    { "heavy", 800 },
    { "black", 900 },
    { "extrablack", 950 },
    { "extra-black", 950 },
    { "extra_black", 950 },
    { "nord", 950 },
    { NULL }};

static struct keyword *KeyFind(unichar_t *name,struct keyword *words) {
    while ( words->name!=NULL ) {
	if ( uc_strmatch(name,words->name)==0 )
return( words );
	++words;
    }
return( NULL );
}

static struct keyword *KeyInside(unichar_t *name,struct keyword *words) {
    while ( words->name!=NULL ) {
	if ( uc_strstrmatch(name,words->name)!=NULL )
return( words );
	++words;
    }
return( NULL );
}

int _GDraw_FontFigureWeights(unichar_t *weight_str) {
    struct keyword *val;

    if (( val=KeyFind(weight_str,weights))!=NULL )
return( val->val );
    else if (( val=KeyInside(weight_str,weights))!=NULL )
return( val->val );
    else
return( 400 );		/* Assume normal */
}

struct font_name *_GDraw_HashFontFamily(FState *fonts,unichar_t *name, int prop) {
    struct font_name *fn;
    int ch;
    int b,i;

    ch = *name;
    if ( isupper(ch)) ch = tolower(ch);
    if ( ch<'a' ) ch = 'q'; else if ( ch>'z' ) ch='z';
    ch -= 'a';

    fn = fonts->font_names[ch];
    while ( fn!=NULL ) {
	if ( u_strmatch(name,fn->family_name)==0 )
    break;
	fn = fn->next;
    }
    if ( fn==NULL ) {
	fn = gcalloc(1,sizeof(struct font_name));
	fn->family_name = u_copy(name);
	fn->ft = _GDraw_ClassifyFontName(fn->family_name,&b,&i);
	if ( !prop && fn->ft==ft_unknown )
	    fn->ft = ft_mono;
	fn->next = fonts->font_names[ch];
	fonts->font_names[ch] = fn;
    }
return( fn );
}

void _GDraw_FreeFD(struct font_data *fd) {
    gfree(fd->charmap_name);
    gfree(fd->localname);
    gfree(fd->fontfile);
    gfree(fd->metricsfile);
    gfree(fd);
}

static void RemoveDuplicateFDs(FState *fonts, struct font_name *fn) {
    struct font_data *fd, *prev, *fd2, *prev2, *next, *next2;
    int i;

    if ( uc_strstr(fn->family_name,"courier")!=NULL )
	i = 0;
    prev = NULL;
    for ( i=0; i<=em_max; ++i )
    for ( fd = fn->data[i]; fd!=NULL; prev=fd, fd = next ) {
	next = fd->next;
	for (prev2=fd, fd2=fd->next; fd2!=NULL; fd2 = next2 ) {
	    next2 = fd2->next;
	    if ( fd2->point_size==fd->point_size && fd2->map==fd->map &&
		    fd2->weight==fd->weight && fd2->style==fd->style ) {
		if ( fd->localname!=NULL && strstr(fd->localname,"bitstream")!=NULL ) {
		    /* adobe's bitmaps are nicer than bitstream's */
		    struct font_data temp;
		    temp = *fd;
		    *fd = *fd2;
		    *fd2 = temp;
		    fd2->next = fd->next; fd->next = temp.next;
		}
		/* Remove fd2 */
		prev2->next = next2;
		_GDraw_FreeFD(fd2);
		if ( next==fd2 ) next = next2;
	    } else
		prev2 = fd2;
	}
    }
}
		
void _GDraw_RemoveDuplicateFonts(FState *fonts) {
    int j;
    struct font_name *fn;

    for ( j=0; j<26; ++j )
	for ( fn = fonts->font_names[j]; fn!=NULL; fn = fn->next )
	    RemoveDuplicateFDs(fonts,fn);
}

static struct font_name *_FindFontName(FState *fonts, char *name) {
    struct font_name *fn;
    int ch;

    ch = *name;
    if ( isupper(ch)) ch = tolower(ch);
    if ( ch<'a' ) ch = 'q'; else if ( ch>'z' ) ch='z';
    ch -= 'a';
    fn = fonts->font_names[ch];
    while ( fn!=NULL ) {
	if ( uc_strmatch(fn->family_name,name)==0 )
return( fn );
	fn = fn->next;
    }
return( NULL );
}

static int fontdatalistlen(struct font_data *list ) {
    int cnt = 0;

    while ( list!=NULL ) {
	++cnt;
	list = list->next;
    }
return( cnt );
}

void _GDraw_FillLastChance(FState *fonts) {
    struct font_name *any, *any2, *fn;
    struct font_name *cour, *hel, *times;
    int j, i, k, best, test;

    fonts->mappings_avail = 0;
    cour = _FindFontName(fonts,"courier");
    hel = _FindFontName(fonts,"helvetica");
    if ( hel==NULL )
	hel = _FindFontName(fonts,"arial");
    times = _FindFontName(fonts,"times");
    for ( i=0; i<em_uplanemax; ++i ) {
	fonts->lastchance[i][ft_serif]= times==NULL || times->data[i]==NULL?NULL: times;
	fonts->lastchance[i][ft_sans]= hel==NULL || hel->data[i]==NULL?NULL: hel;
	fonts->lastchance[i][ft_mono]= cour==NULL || cour->data[i]==NULL?NULL: cour;
	any = any2 = NULL;
	for ( j=ft_unknown; j<ft_max; ++j ) {
	    fonts->lastchance2[i][j] = NULL;
	    best = 0;
	    for ( k=0; k<26; ++k ) for ( fn=fonts->font_names[k]; fn!=NULL; fn=fn->next ) {
		if ( fn->data[i]!=NULL && fn->ft == j ) {
		    test = fontdatalistlen(fn->data[i]);
		    if ( test>best ) {
			fonts->lastchance2[i][j] = fn;
			best = test;
		    }
		}
	    }
	    if ( fonts->lastchance[i][j]==NULL ) {
		fonts->lastchance[i][j] = fonts->lastchance2[i][j];
		fonts->lastchance2[i][j] = NULL;
	    }
	    if ( any==NULL && fonts->lastchance[i][j]!=NULL )
		any = fonts->lastchance[i][j];
	    if ( any2==NULL && fonts->lastchance2[i][j]!=NULL )
		any2 = fonts->lastchance2[i][j];
	}
	fonts->lastchance[i][ft_unknown] = fonts->lastchance[i][default_type];
	if ( fonts->lastchance[i][ft_unknown]==NULL )
	    fonts->lastchance[i][ft_unknown] = any;
	fonts->lastchance2[i][ft_unknown] = fonts->lastchance2[i][default_type];
	if ( fonts->lastchance2[i][ft_unknown]==NULL )
	    fonts->lastchance2[i][ft_unknown] = any2;
	if ( fonts->lastchance[i][ft_unknown]!=NULL || fonts->lastchance2[i][ft_unknown]!=NULL )
	    fonts->mappings_avail |= (1<<i);
    }
}
