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

/* There are two configurable option here that the configure script can't
    figure out:

	_WACOM_DRV_BROKEN
on my system the XFree driver for the WACOM tablet sends no events. I don't
understand the driver, so I'm not able to attempt fixing it. However thanks
to John E. Joganic wacdump program I do know what the event stream on
	/dev/input/event0
looks like, and I shall attempt to simulate driver behavior from that

So set macro this in your makefile if you have problems too, and then
change the protection on /dev/input/event0 so that it is world readable

(there is now a working XFree driver for wacom, but you have to get it from
John, it's not part of standard XFree yet).

	_COMPOSITE_BROKEN
on XFree with the composite extension active, scrolling windows with docket
palletes may result in parts of the palettes duplicated outside their dock
rectangles.

Defining this macro forces redisplay of the entire window area, which is
slightly slower, but should make no significant difference on a machine
capable of using composite.
*/

#ifndef FONTFORGE_XDRAW_H
#define FONTFORGE_XDRAW_H

#include <fontforge-config.h>


#endif /* FONTFORGE_XDRAW_H */
