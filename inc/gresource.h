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

#ifndef FONTFORGE_GRESOURCE_H
#define FONTFORGE_GRESOURCE_H

#include "gdraw.h"
#include "ggadget.h"

#include <iconv.h>

enum res_type { rt_int, rt_double, rt_bool/* int */, rt_color, rt_string, rt_image, rt_font, rt_stringlong, rt_coloralpha };

struct gimage_cache_bucket;

typedef struct gresimage {
    const char *ini_name;
    struct gimage_cache_bucket *bucket;
} GResImage;

typedef struct gresfont {
    GFont *fi;
    char *rstr;
    uint8_t can_free_name;
} GResFont;

#define GRESIMAGE_INIT(defstr) { (defstr), NULL }
#define GRESFONT_INIT(defstr) { NULL, (defstr), false }

GImage *GResImageGetImage(GResImage *);

typedef struct gresstruct {
    const char *resname;
    enum res_type type;
    void *val;
    void *(*cvt)(char *,void *);	/* converts a string into a whatever */
    int found;
} GResStruct;

#define GRESSTRUCT_EMPTY { NULL, 0, NULL, NULL, 0 }


extern char *GResourceProgramName;

int ResStrToFontRequest(const char *resname, FontRequest *rq);

void GResourceSetProg(const char *prog);
void GResourceAddResourceFile(const char *filename,const char *prog,int warn);
void GResourceAddResourceString(const char *string,const char *prog);

#endif /* FONTFORGE_GRESOURCE_H */
