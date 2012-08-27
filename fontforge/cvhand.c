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
#include "fontforgeui.h"
#include <math.h>


void CVMouseDownHand(CharView *cv) {
    cv->handscroll_base.x = cv->p.x;
    cv->handscroll_base.y = cv->p.y;
}

void CVMouseMoveHand(CharView *cv, GEvent *event) {
    cv->xoff += event->u.mouse.x-cv->handscroll_base.x; cv->handscroll_base.x = event->u.mouse.x;
    cv->yoff -= event->u.mouse.y-cv->handscroll_base.y; cv->handscroll_base.y = event->u.mouse.y;
    cv->back_img_out_of_date = true;
    GScrollBarSetPos(cv->hsb,-cv->xoff);
    GScrollBarSetPos(cv->vsb,cv->yoff-cv->height);
    GDrawRequestExpose(cv->v,NULL,false);
    if ( cv->showrulers )
	GDrawRequestExpose(cv->gw,NULL,false);
}

void CVMouseUpHand(CharView *cv) {
}
