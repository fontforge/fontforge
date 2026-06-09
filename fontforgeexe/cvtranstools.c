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

#include "cvundoes.h"
#include "fontforgeui.h"
#include "nonlineartrans.h"
#include <math.h>

/* Cache the fixed point and original drag vector for the scale gesture. */
static void ScaleSetOppositeOrigin(CharView *cv) {
    BasePoint center;

    CVFindCenter(cv, &center, !CVAnySel(cv,NULL,NULL,NULL,NULL));
    cv->expandorigin.x = 2*center.x - cv->p.cx;
    cv->expandorigin.y = 2*center.y - cv->p.cy;
    cv->expandwidth    = cv->p.cx - cv->expandorigin.x;
    cv->expandheight   = cv->p.cy - cv->expandorigin.y;
}

void CVMouseDownTransform(CharView *cv) {
    CVPreserveTState(cv);
    if ( cv->active_tool==cvt_scale )
        ScaleSetOppositeOrigin(cv);
}

void CVMouseMoveTransform(CharView *cv, uint32_t input_state) {
    CharViewTab* tab = CVGetActiveTab(cv);
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
              /* Scale from cached opposite side/corner so dragged point tracks the mouse. */
	      transform[0] = cv->expandwidth==0 ? 1 : (cv->info.x-cv->expandorigin.x)/cv->expandwidth;
	      transform[3] = cv->expandheight==0 ? 1 : (cv->info.y-cv->expandorigin.y)/cv->expandheight;

	      /* Shift key constrains interactive scaling to single dimension */
              if ( input_state&ksm_shift ) {
                  real dx = fabs(cv->info.x-cv->p.cx);
                  real dy = fabs(cv->info.y-cv->p.cy);

                  if ( cv->expandwidth==0 )       transform[0] = 1; // press centered horizontally, ver motion only
                  else if ( cv->expandheight==0 ) transform[3] = 1; // press centered vertically, hor motion only
                  else if ( dx>=dy )              transform[3] = 1; // hor drag dominant, lock ver scale
                  else                            transform[0] = 1; // ver drag dominant, lock hor scale
              }
	  } break;
	  case cvt_skew: {
	    real angle = atan2(cv->info.y-cv->p.cy,cv->info.x-cv->p.cx);
	    transform[2] = sin(angle);
	  } break;
	  case cvt_3d_rotate: {
	    real angle = atan2(cv->info.y-cv->p.cy,cv->info.x-cv->p.cx);
/* Allow one pixel per degree */
	    real zangle = sqrt( (cv->info.x-cv->p.cx)*(cv->info.x-cv->p.cx) +
		    (cv->info.y-cv->p.cy)*(cv->info.y-cv->p.cy) ) * tab->scale *
		    FF_PI/180;
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
	if ( cv->active_tool==cvt_scale ) {
	    /* Translate scaled outline back so cached origin remains fixed */
	    transform[4] = (1-transform[0])*cv->expandorigin.x;
	    transform[5] = (1-transform[3])*cv->expandorigin.y;
	} else if ( cv->active_tool!=cvt_perspective ) {
	    /* Other transform tools: pressed point is center of the transformation */
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
