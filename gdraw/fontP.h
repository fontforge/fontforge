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

#ifndef FONTFORGE_FONTP_H
#define FONTFORGE_FONTP_H

#include <fontforge-config.h>

#include "charset.h"
#include "gdrawP.h"
#ifndef FONTFORGE_CAN_USE_GDK
#  include "gxdrawP.h"
#else
#  include "ggdkdrawP.h"
#endif

#define em_uplane0     (em_max+1)

struct font_instance {
    FontRequest rq;		/* identification of this instance */
    PangoFontDescription *pango_fd;
#ifndef _NO_LIBCAIRO
    PangoFontDescription *pangoc_fd;
#endif
};

typedef struct font_state {
    int res;
    struct font_name *font_names[26];
    unsigned int names_loaded: 1;
} FState;

extern enum charset _GDraw_ParseMapping(unichar_t *setname);

#endif /* FONTFORGE_FONTP_H */
