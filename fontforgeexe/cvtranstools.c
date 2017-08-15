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
#include "cvundoes.h"
#include "fontforgeui.h"
#include "nonlineartrans.h"

#include <math.h>

void CVMouseDownTransform(CharView *cv) {
    CVPreserveTState(cv);
}

void CVMouseMoveTransform(CharView *cv) {
    real transform[6];

    CVRestoreTOriginalState(cv);
    if ( cv->info.x != cv->p.cx || cv->info.y != cv->p.cy ) {
	transform[0] = transform[3] = 1;
	transform[1] = transform[2] = 0;
	switch ( cv->active_tool ) {
	  case cvt_rotate: {
	    real angle = atan2(cv->info.y-cv->p.cy,cv->info.x-cv->p.cx);
	    transform[0] = transform[3] = cos(angle);
	    transform[2] = -(transform[1] = sin(angle));
	  } break;
	  case cvt_flip: {
	    real dx,dy;
	    if (( dx = cv->info.x-cv->p.cx)<0 ) dx=-dx;
	    if (( dy = cv->info.y-cv->p.cy)<0 ) dy=-dy;
	    if ( dy>2*dx )
		transform[0] = -1;
	    else if ( dx>2*dy )
		transform[3] = -1;
	    else if ( (cv->info.x-cv->p.cx)*(cv->info.y-cv->p.cy)>0 ) {
		transform[0] = transform[3] = 0;
		transform[1] = transform[2] = -1;
	    } else {
		transform[0] = transform[3] = 0;
		transform[1] = transform[2] = 1;
	    }
	  } break;
	  case cvt_scale: {
	      transform[0] = 1.0+(cv->info.x-cv->p.cx)/(400*cv->scale);
	      transform[3] = 1.0+(cv->info.y-cv->p.cy)/(400*cv->scale);
	  } break;
	  case cvt_skew: {
	    real angle = atan2(cv->info.y-cv->p.cy,cv->info.x-cv->p.cx);
	    transform[2] = sin(angle);
	  } break;
	  case cvt_3d_rotate: {
	    real angle = atan2(cv->info.y-cv->p.cy,cv->info.x-cv->p.cx);
/* Allow one pixel per degree */
	    real zangle = sqrt( (cv->info.x-cv->p.cx)*(cv->info.x-cv->p.cx) +
		    (cv->info.y-cv->p.cy)*(cv->info.y-cv->p.cy) ) * cv->scale *
		    3.1415926535897932/180;
	    real s = sin(angle), c = cos(angle);
	    real cz = cos(zangle);
	    transform[0] = c*c + s*s*cz;
	    transform[3] = s*s + c*c*cz;
	    transform[2] = transform[1] = c*s * (cz-1);
	  } break;
/* Perspective takes three points: origin, start point, cur point */
/*  first rotate so that orig/start are the x axis	*/
/*  then define perspective so that:			*/
/*      y' = y						*/
/*	x' = cur.x + (cur.y - y)/cur.y * (x - cur.x)	*/
/*  then rotate back					*/
	  case cvt_perspective: {
	    real angle = atan2(cv->p.cy,cv->p.cx);
	    real s = sin(angle), c = cos(angle);
	    transform[0] = transform[3] = c;
	    transform[2] = -(transform[1] = -s);
	    transform[4] = transform[5] = 0;
	    CVTransFunc(cv,transform,false);
	    CVYPerspective((CharViewBase *) cv,
			     c*cv->info.x + s*cv->info.y,
			    -s*cv->info.x + c*cv->info.y);
	    transform[2] = -(transform[1] = s);
	  } break;
	  default:
	  break;
	}
	    /* Make the pressed point be the center of the transformation */
	if ( cv->active_tool!=cvt_perspective ) {
	    transform[4] = -cv->p.cx*transform[0] -
			    cv->p.cy*transform[2] +
			    cv->p.cx;
	    transform[5] = -cv->p.cy*transform[3] -
			    cv->p.cx*transform[1] +
			    cv->p.cy;
	}
	CVSetCharChanged(cv,true);
	CVTransFunc(cv,transform,false);
    }
    SCUpdateAll(cv->b.sc);
    CVGridHandlePossibleFitChar(cv);
}

void CVMouseUpTransform(CharView *cv) {
    if ( cv->info.x == cv->p.cx && cv->info.y == cv->p.cy )
    {
	/* Nothing happened */
	cv->needsrasterize = cv->recentchange = false;
	CVRemoveTopUndo(&cv->b);
	SCUpdateAll(cv->b.sc);
	CVGridHandlePossibleFitChar(cv);
    }
    else
    {
	CVUndoCleanup(cv);
    }
}
