/* Copyright (C) 2007-2012 by George Williams */
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

#include "fontforgeui.h"
#include "uiinterface.h"

/* I don't care what window "owns" the X (or other UI) clipboard */
/*  as far as I am concerned they are all the same. I have an internal clip */
/*  and I export IT when needed, I don't need to figure out what window */
/*  currently has things. This makes my clipboard interface much simpler */

static void ClipBoard_Grab(void) {
    GDrawGrabSelection(((FontView *) FontViewFirst())->gw,sn_clipboard);
}

static void ClipBoard_AddDataType(const char *mimetype, void *data, int cnt, int size,
	void *(*gendata)(void *,int32_t *len), void (*freedata)(void *)) {
    GDrawAddSelectionType(((FontView *) FontViewFirst())->gw,sn_clipboard,
	    (char *) mimetype, data, cnt, size,
	    gendata,freedata);
}

/* Asks for the clip and waits for the response. */
static void *ClipBoard_Request(const char *mimetype,int *len) {
return( GDrawRequestSelection(((FontView *) FontViewFirst())->gw,sn_clipboard,
	    (char *) mimetype,len));
}

static int ClipBoard_HasType(const char *mimetype) {
return( GDrawSelectionHasType(((FontView *) FontViewFirst())->gw,sn_clipboard,
	    (char *) mimetype));
}

struct clip_interface gdraw_clip_interface = {
    ClipBoard_Grab,
    ClipBoard_AddDataType,
    ClipBoard_HasType,
    ClipBoard_Request
};
