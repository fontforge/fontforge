/* Copyright (C) 2000,2001 by George Williams */
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
#include <stdlib.h>
#include "gdraw.h"
#include "ggadgetP.h"
#include "utype.h"
#include "ustring.h"

int GTextInfoGetWidth(GWindow base,GTextInfo *ti,FontInstance *font) {
    int width=0;
    int iwidth=0;
    int skip = 0;

    if ( ti->text!=NULL ) {
	if ( ti->font!=NULL )
	    font = ti->font;

	GDrawSetFont(base,font);
	width = GDrawGetTextWidth(base,ti->text, -1, NULL);
    }
    if ( ti->image!=NULL ) {
	iwidth = GImageGetScaledWidth(base,ti->image);
	if ( ti->text!=NULL )
	    skip = GDrawPointsToPixels(base,6);
    }
return( width+iwidth+skip );
}

int GTextInfoGetMaxWidth(GWindow base,GTextInfo **ti,FontInstance *font) {
    int width=0, temp;
    int i;

    for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL; ++i ) {
	if (( temp= GTextInfoGetWidth(base,ti[i],font))>width )
	    width = temp;
    }
return( width );
}

int GTextInfoGetHeight(GWindow base,GTextInfo *ti,FontInstance *font) {
    int fh=0, as=0, ds=0, ld;
    int iheight=0;
    int height;
    GTextBounds bounds;

    if ( ti->font!=NULL )
	font = ti->font;
    GDrawFontMetrics(font,&as, &ds, &ld);
    if ( ti->text!=NULL ) {
	GDrawSetFont(base,font);
	GDrawGetTextBounds(base,ti->text, -1, NULL, &bounds);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;
    if ( ti->image!=NULL ) {
	iheight = GImageGetScaledHeight(base,ti->image);
    }
    if ( (height = fh)<iheight ) height = iheight;
return( height );
}

int GTextInfoGetMaxHeight(GWindow base,GTextInfo **ti,FontInstance *font,int *allsame) {
    int height=0, temp, same=1;
    int i;

    for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL; ++i ) {
	temp= GTextInfoGetHeight(base,ti[i],font);
	if ( height!=0 && height!=temp )
	    same = 0;
	if ( height<temp )
	    height = temp;
    }
    *allsame = same;
return( height );
}

int GTextInfoGetAs(GWindow base,GTextInfo *ti, FontInstance *font) {
    int fh=0, as=0, ds=0, ld;
    int iheight=0;
    int height;
    GTextBounds bounds;

    GDrawFontMetrics(font,&as, &ds, &ld);
    if ( ti->text!=NULL ) {
	GDrawSetFont(base,font);
	GDrawGetTextBounds(base,ti->text, -1, NULL, &bounds);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;
    if ( ti->image!=NULL ) {
	iheight = GImageGetScaledHeight(base,ti->image);
    }
    if ( (height = fh)<iheight ) height = iheight;

    if ( ti->text!=NULL )
return( as+(height>fh?(height-fh)/2:0) );

return( iheight );
}

int GTextInfoDraw(GWindow base,int x,int y,GTextInfo *ti,
	FontInstance *font,Color fg, Color sel) {
    int fh=0, as=0, ds=0, ld;
    int iwidth=0, iheight=0;
    int height, skip = 0;
    GTextBounds bounds;
    GRect r, old;

    GDrawFontMetrics(font,&as, &ds, &ld);
    if ( ti->text!=NULL ) {
	if ( ti->font!=NULL )
	    font = ti->font;
	if ( ti->fg!=COLOR_DEFAULT && ti->fg!=COLOR_UNKNOWN )
	    fg = ti->fg;

	GDrawSetFont(base,font);
	GDrawGetTextBounds(base,ti->text, -1, NULL, &bounds);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;
    if ( fg == COLOR_DEFAULT )
	fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(base));
    if ( ti->image!=NULL ) {
	iwidth = GImageGetScaledWidth(base,ti->image);
	iheight = GImageGetScaledHeight(base,ti->image);
	if ( ti->text!=NULL )
	    skip = GDrawPointsToPixels(base,6);
    }
    if ( (height = fh)<iheight ) height = iheight;

    if (( ti->selected && sel!=COLOR_DEFAULT ) || ( ti->bg!=COLOR_DEFAULT && ti->bg!=COLOR_UNKNOWN )) {
	Color bg = ti->bg;
	if ( ti->selected ) {
	    if ( sel==COLOR_DEFAULT )
		sel = fg;
	    bg = sel;
	    if ( sel==fg ) {
		fg = ti->bg;
		if ( fg==COLOR_DEFAULT || fg==COLOR_UNKNOWN )
		    fg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(base));
	    }
	}
	r.y = y; r.height = height;
	r.x = 0; r.width = 10000;
	GDrawFillRect(base,&r,bg);
    }

    if ( ti->line ) {
	GDrawGetClip(base,&r);
	r.x += GDrawPointsToPixels(base,2); r.width -= 2*GDrawPointsToPixels(base,2);
	GDrawPushClip(base,&r,&old);
	r.y = y+2*as/3; r.height = height;
	r.x = x; r.width = 10000;
	GBoxDrawHLine(base,&r,&_GGroup_LineBox);
	GDrawPopClip(base,&old);
    } else {
	if ( ti->image!=NULL && ti->image_precedes ) {
	    GDrawDrawScaledImage(base,ti->image,x,y + (iheight>as?0:as-iheight));
	    x += iwidth + skip;
	}
	if ( ti->text!=NULL ) {
	    int ypos = y+as+(height>fh?(height-fh)/2:0);
	    int width = GDrawDrawBiText(base,x,ypos,ti->text,-1,NULL,fg);
	    _ggadget_underlineMnemonic(base,x,ypos,ti->text,ti->mnemonic,fg);
	    x += width + skip;
	}
	if ( ti->image!=NULL && !ti->image_precedes )
	    GDrawDrawScaledImage(base,ti->image,x,y + (iheight>as?0:as-iheight));
    }

return( height );
}

GTextInfo *GTextInfoCopy(GTextInfo *ti) {
    GTextInfo *copy;

    copy = galloc(sizeof(GTextInfo));
    *copy = *ti;
    copy->text_is_1byte = false;
    if ( copy->fg == 0 && copy->bg == 0 ) {
	copy->fg = copy->bg = COLOR_UNKNOWN;
    }
    if ( ti->text!=NULL ) {
	if ( ti->text_is_1byte )
	    copy->text = def2u_copy((char *) copy->text);
	else
	    copy->text = u_copy(copy->text);
    }
return( copy);
}

GTextInfo **GTextInfoArrayFromList(GTextInfo *ti, uint16 *cnt) {
    int i;
    GTextInfo **arr;

    i = 0;
    if ( ti!=NULL )
	for ( ; ti[i].text!=NULL || ti[i].image!=NULL || ti[i].line; ++i );
    if ( i==0 ) {
	arr = galloc(sizeof(GTextInfo *));
	i =0;
    } else {
	arr = galloc((i+1)*sizeof(GTextInfo *));
	for ( i=0; ti[i].text!=NULL || ti[i].image!=NULL || ti[i].line; ++i )
	    arr[i] = GTextInfoCopy(&ti[i]);
    }
    arr[i] = gcalloc(1,sizeof(GTextInfo));
    if ( cnt!=NULL ) *cnt = i;
return( arr );
}

GTextInfo **GTextInfoArrayCopy(GTextInfo **ti) {
    int i;
    GTextInfo **arr;

    if ( ti==NULL || (ti[0]->image==NULL && ti[0]->text==NULL && !ti[0]->line) ) {
	arr = galloc(sizeof(GTextInfo *));
	i =0;
    } else {
	for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL || ti[i]->line; ++i );
	arr = galloc((i+1)*sizeof(GTextInfo *));
	for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL || ti[i]->line; ++i )
	    arr[i] = GTextInfoCopy(ti[i]);
    }
    arr[i] = gcalloc(1,sizeof(GTextInfo));
return( arr );
}

int GTextInfoArrayCount(GTextInfo **ti) {
    int i;

    for ( i=0; ti[i]->text || ti[i]->image || ti[i]->line; ++i );
return( i );
}

void GTextInfoFree(GTextInfo *ti) {
    gfree(ti->text);
    gfree(ti);
}

void GTextInfoListFree(GTextInfo *ti) {
    int i;

    for ( i=0; ti[i].text!=NULL || ti[i].image!=NULL || ti[i].line; ++i )
	gfree(ti[i].text);
    gfree(ti);
}

void GTextInfoArrayFree(GTextInfo **ti) {
    int i;

    if ( ti == NULL )
return;
    for ( i=0; ti[i]->text || ti[i]->image || ti[i]->line; ++i )
	GTextInfoFree(ti[i]);
    gfree(ti);
}

int GTextInfoCompare(GTextInfo *ti1, GTextInfo *ti2) {
    if ( ti1->text == NULL && ti2->text==NULL )
return( 0 );
    else if ( ti1->text==NULL )
return( -1 );
    else if ( ti2->text==NULL )
return( 1 );
    else
return( u_strmatch(ti1->text,ti2->text) );
}

GTextInfo **GTextInfoFromChars(char **array, int len) {
    int i;
    GTextInfo **ti;

    if ( array==NULL || len==0 )
return( NULL );
    if ( len==-1 ) {
	for ( len=0; array[len]!=NULL; ++len );
    } else {
	for ( i=0; i<len && array[i]!=NULL; ++i );
	len = i;
    }
    ti = galloc((i+1)*sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->text = uc_copy(array[i]);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

void GMenuItemArrayFree(GMenuItem *mi) {
    int i;

    if ( mi == NULL )
return;
    for ( i=0; mi[i].ti.text || mi[i].ti.image || mi[i].ti.line; ++i ) {
	GMenuItemArrayFree(mi[i].sub);
	free(mi[i].ti.text);
    }
    gfree(mi);
}

GMenuItem *GMenuItemArrayCopy(GMenuItem *mi, uint16 *cnt) {
    int i;
    GMenuItem *arr;

    if ( mi==NULL )
return( NULL );
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i );
    if ( i==0 )
return( NULL );
    arr = galloc((i+1)*sizeof(GMenuItem));
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	arr[i] = mi[i];
	if ( mi[i].ti.text!=NULL ) {
	    if ( mi[i].ti.text_is_1byte )
		arr[i].ti.text = def2u_copy((char *) mi[i].ti.text);
	    else
		arr[i].ti.text = u_copy(mi[i].ti.text);
	}
	if ( islower(arr[i].ti.mnemonic))
	    arr[i].ti.mnemonic = toupper(arr[i].ti.mnemonic);
	if ( islower(arr[i].shortcut))
	    arr[i].shortcut = toupper(arr[i].shortcut);
	if ( mi[i].sub!=NULL )
	    arr[i].sub = GMenuItemArrayCopy(mi[i].sub,NULL);
    }
    memset(&arr[i],'\0',sizeof(GMenuItem));
    if ( cnt!=NULL ) *cnt = i;
return( arr );
}
