/* Copyright (C) 2008-2012 by George Williams */
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

#include "gimage.h"

#include <math.h>
#include <string.h>

void gRGB2HSL(struct hslrgb *col) {
    double mx, mn;

    /* Algorithm from http://en.wikipedia.org/wiki/HSL_color_space */
    if ( col->r>col->g ) {
	mx = ( col->r>col->b ) ? col->r : col->b;
	mn = ( col->g<col->b ) ? col->g : col->b;
    } else {
	mx = ( col->g>col->b ) ? col->g : col->b;
	mn = ( col->r<col->b ) ? col->r : col->b;
    }
    if ( mx==mn )
	col->h = 0;
    else if ( mx==col->r ) {
	col->h = fmod(60*(col->g-col->b)/(mx-mn),360);
    } else if ( mx==col->g ) {
	col->h = 60*(col->b-col->r)/(mx-mn) + 120;
    } else {
	col->h = 60*(col->r-col->g)/(mx-mn) + 240;
    }
    col->l = (mx+mn)/2;
    if ( mx==mn )
	col->s = 0;
    else if ( col->l<=.5 )
	col->s = (mx-mn)/(mx+mn);
    else
	col->s = (mx-mn)/(2-(mx+mn));
    col->hsl = true; col->hsv = false;
}

void gHSL2RGB(struct hslrgb *col) {
    double q,p,hk, ts[3], cs[3];
    int i;

    /* Algorithm from http://en.wikipedia.org/wiki/HSL_color_space */
    if ( col->l<.5 )
	q = col->l*(1 + col->s);
    else
	q = col->l+col->s - (col->l*col->s);
    p = 2*col->l - q;
    hk = fmod(col->h,360)/360;
    if ( hk<0 ) hk += 1.0;
    ts[0] = hk + 1./3.;
    ts[1] = hk;
    ts[2] = hk - 1./3.;

    for ( i=0; i<3; ++i ) {
	if ( ts[i]<0 ) ts[i] += 1.0;
	else if ( ts[i]>1 ) ts[i] -= 1.0;
	if ( ts[i]<1./6. )
	    cs[i] = p + ((q-p)*6*ts[i]);
	else if ( ts[i]<.5 )
	    cs[i] = q;
	else if ( ts[i]<2./3. )
	    cs[i] = p + ((q-p)*6*(2./3.-ts[i]));
	else
	    cs[i] = p;
    }
    col->r = cs[0];
    col->g = cs[1];
    col->b = cs[2];
    col->rgb = true;
}

void gRGB2HSV(struct hslrgb *col) {
    double mx, mn;

    /* Algorithm from http://en.wikipedia.org/wiki/HSL_color_space */
    if ( col->r>col->g ) {
	mx = ( col->r>col->b ) ? col->r : col->b;
	mn = ( col->g<col->b ) ? col->g : col->b;
    } else {
	mx = ( col->g>col->b ) ? col->g : col->b;
	mn = ( col->r<col->b ) ? col->r : col->b;
    }
    if ( mx==mn )
	col->h = 0;
    else if ( mx==col->r ) {
	col->h = fmod(60*(col->g-col->b)/(mx-mn),360);
    } else if ( mx==col->g ) {
	col->h = 60*(col->b-col->r)/(mx-mn) + 120;
    } else {
	col->h = 60*(col->r-col->g)/(mx-mn) + 240;
    }
    col->v = mx;
    if ( mx==0 )
	col->s = 0;
    else 
	col->s = (mx-mn)/mx;
    col->hsv = true; col->hsl = false;
}

void gHSV2RGB(struct hslrgb *col) {
    double q,p,t,f;
    int h;

    /* Algorithm from http://en.wikipedia.org/wiki/HSL_color_space */
    h = ((int) floor(col->h/60)) % 6;
    if ( h<0 ) h+=6;
    f = col->h/60 - floor(col->h/60);

    p = col->v*(1-col->s);
    q = col->v*(1-f*col->s);
    t = col->v*(1-(1-f)*col->s);

    if ( h==0 ) {
	col->r = col->v; col->g = t; col->b = p;
    } else if ( h==1 ) {
	col->r = q; col->g = col->v; col->b = p;
    } else if ( h==2 ) {
	col->r = p; col->g = col->v; col->b = t;
    } else if ( h==3 ) {
	col->r = p; col->g = q; col->b = col->v;
    } else if ( h==4 ) {
	col->r = t; col->g = p; col->b = col->v;
    } else if ( h==5 ) {
	col->r = col->v; col->g = p; col->b = q;
    }
    col->rgb = true;
}

Color gHslrgb2Color(struct hslrgb *col) {
    if ( !col->rgb ) {
	if ( col->hsv )
	    gHSV2RGB(col);
	else if ( col->hsl )
	    gHSL2RGB(col);
	else
	    return( COLOR_UNKNOWN );
    }
    return( (((int) rint(255.*col->r))<<16 ) | \
	    (((int) rint(255.*col->g))<<8 )  | \
	    (((int) rint(255.*col->b)) ) );
}

Color gHslrgba2Color(struct hslrgba *col) {
    if ( !col->rgb ) {
	if ( col->hsv )
	    gHSV2RGB((struct hslrgb *) col);
	else if ( col->hsl )
	    gHSL2RGB((struct hslrgb *) col);
	else
	    return( COLOR_UNKNOWN );
    }
    if ( !col->has_alpha || col->alpha==1.0 )
	return( (((int) rint(255.*col->r))<<16 ) | \
		(((int) rint(255.*col->g))<<8 )  | \
		(((int) rint(255.*col->b)) ) );
    else if ( col->alpha==0.0 )
	return( COLOR_TRANSPARENT );
    else
	return( (((int) rint(255.*col->alpha))<<24 ) | \
		(((int) rint(255.*col->r))<<16) | \
		(((int) rint(255.*col->g))<<8 ) | \
		(((int) rint(255.*col->b)) ) );
}

void gColor2Hslrgb(struct hslrgb *col,Color from) {
    col->rgb = true;
    col->r = ((from>>16)&0xff)/255.0;
    col->g = ((from>>8 )&0xff)/255.0;
    col->b = ((from    )&0xff)/255.0;
    col->hsl = col->hsv = false;
}

void gColor2Hslrgba(struct hslrgba *col,Color from) {
    if ( from == COLOR_TRANSPARENT ) {
	memset(col,0,sizeof(*col));
	col->has_alpha = 1;
    } else {
	col->alpha = ((from>>24)&0xff)/255.0;
	col->r = ((from>>16)&0xff)/255.0;
	col->g = ((from>>8 )&0xff)/255.0;
	col->b = ((from    )&0xff)/255.0;
	col->hsl = col->hsv = false;
	col->has_alpha = col->alpha!=0;
	if ( !col->has_alpha )
	    col->alpha = 1.0;
    }
    col->rgb = true;
}
