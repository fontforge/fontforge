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

#include "fontP.h"
#include "ustring.h"
#include "utype.h"

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