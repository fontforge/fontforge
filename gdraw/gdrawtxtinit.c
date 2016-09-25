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

struct fontabbrev _gdraw_fontabbrev[] = {
    { "times", ft_serif, 0, 0, 0, 0, NULL },
    { "Helvetica", ft_sans, 0, 0, 0, 0, NULL },
    { "Courier", ft_mono, 0, 0, 0, 0, NULL },

    { "sans-serif", ft_sans, 0, 0, true, 0, NULL },
    { "serif", ft_serif, 0, 0, true, 0, NULL },
    { "cursive", ft_cursive, 0, 0, true, 0, NULL },
    { "monospace", ft_mono, 0, 0, true, 0, NULL },
    { "fantasy", ft_sans, 0, 0, true, 0, NULL },

    { "Albertus", ft_serif, 0, 0, 0, 0, NULL },
    { "Arial", ft_sans, 0, 0, 0, 0, NULL },
    { "Avant Garde", ft_sans, 0, 0, 0, 0, NULL },
    { "Bembo", ft_serif, 0, 0, 0, 0, NULL },
    { "Bodoni", ft_serif, 0, 0, 0, 0, NULL },
    { "Book Antiqua", ft_serif, 0, 0, 0, 0, NULL },
    { "Book Rounded", ft_serif, 0, 0, 0, 0, NULL },
    { "Bookman", ft_serif, 0, 0, 0, 0, NULL },
    { "Caslon", ft_serif, 0, 0, 0, 0, NULL },
    { "Century Sch", ft_serif, 0, 0, 0, 0, NULL },	/* New century Schoolbook */
    { "charter", ft_serif, 0, 0, 0, 0, NULL },
    { "Charcoal", ft_mono, 0, true, 0, 0, NULL },
    { "Cheltenham", ft_serif, 0, 0, 0, 0, NULL },
    { "ChelthmITC", ft_serif, 0, 0, 0, 0, NULL },
    { "Chicago", ft_mono, 0, true, 0, 0, NULL },
    { "Clarendon", ft_mono, 0, 0, 0, 0, NULL },
    { "Coronel", ft_cursive, 0, 0, 0, 0, NULL },
    { "Eurostyle", ft_sans, 0, 0, 0, 0, NULL },
    { "Fixed", ft_mono, 0, 0, 0, 0, NULL },
    { "Footlight", ft_serif, 0, 0, 0, 0, NULL },
    { "Futura", ft_sans, 0, 0, 0, 0, NULL },
    { "Galliard", ft_serif, 0, 0, 0, 0, NULL },
    { "Garamond", ft_serif, 0, 0, 0, 0, NULL },
    { "Geneva", ft_sans, 0, 0, 0, 0, NULL },
    { "Gill Sans", ft_sans, 0, 0, 0, 0, NULL },
    { "Gothic", ft_sans, 0, 0, 0, 0, NULL },
    { "Goudy", ft_serif, 0, 0, 0, 0, NULL },
    { "Grotesque", ft_serif, 0, 0, 0, 0, NULL },
    { "Impact Bold", ft_sans, 0, true, 0, 0, NULL },
    { "lucidatypewriter", ft_mono, 0, 0, 0, 0, NULL },
    { "lucidabright", ft_serif, 0, 0, 0, 0, NULL },
    { "lucida", ft_sans, 0, 0, 0, 0, NULL },
    { "Marigold", ft_cursive, 0, 0, 0, 0, NULL },
    { "Melior", ft_serif, 0, 0, 0, 0, NULL },
    { "Monaco", ft_mono, 0, 0, 0, 0, NULL },
    { "MS Linedraw", ft_serif, 0, 0, 0, 0, NULL },
    { "MS Sans Serif", ft_sans, 0, 0, 0, 0, NULL },
    { "MS Serif", ft_serif, 0, 0, 0, 0, NULL },
    { "New York", ft_serif, 0, 0, 0, 0, NULL },
    { "Omega", ft_sans, 0, 0, 0, 0, NULL },
    { "Optima", ft_sans, 0, 0, 0, 0, NULL },
    { "Palatino", ft_serif, 0, 0, 0, 0, NULL },
    { "Roman", ft_serif, 0, 0, 0, 0, NULL },
    { "script", ft_cursive, 0, 0, 0, 0, NULL },
    { "terminal", ft_mono, 0, 0, 0, 0, NULL },
    { "Univers", ft_sans, 0, 0, 0, 0, NULL },
    { "Wide Latin", ft_serif, 0, 0, 0, 0, NULL },
    { "Zapf Chancery", ft_cursive, 0, 0, 0, 0, NULL },

    FONTABBREV_EMPTY
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

    if ( uc_strstrmatch(setname,"latin10")!=NULL )
return( em_iso8859_16 );
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
/*     if ( uc_strstrmatch(setname,"RJSJ")!=NULL ) */
/* return( em_sjis ); */
/*     if ( uc_strstrmatch(setname,"EUC")!=NULL ) */
/* return( em_euc ); */

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
    { NULL, 0 }
};

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
	fn = calloc(1,sizeof(struct font_name));
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
    free(fd->charmap_name);
    free(fd->localname);
    free(fd->fontfile);
    free(fd->metricsfile);
    free(fd);
}

