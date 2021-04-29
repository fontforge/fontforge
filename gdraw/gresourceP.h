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

#ifndef FONTFORGE_GRESOURCE_P_H
#define FONTFORGE_GRESOURCE_P_H

#include "ggadget.h"

int _GResource_FindResName(const char *name, int do_restrict);
extern struct _GResource_Res { char *res, *val; unsigned int generic: 1; unsigned int new: 1; } *_GResource_Res;

typedef struct gimage_cache_bucket {
    struct gimage_cache_bucket *next;
    char *filename, *absname;
    GImage *image;
} GImageCacheBucket;
extern char *_GGadget_ImagePath;
GImageCacheBucket *_GGadgetImageCache(const char *filename, int keep_empty);

extern void GResourceFind( GResStruct *info, const char *prefix);
extern char *GResourceFindString(const char *name);
extern int GResourceFindBool(const char *name, int def);
extern long GResourceFindInt(const char *name, long def);
extern Color GResourceFindColor(const char *name, Color def);
extern void GResourceFindFont(const char *resourcename, const char *elemname, GResFont *font);

#endif /* FONTFORGE_GRESOURCE_P_H */
