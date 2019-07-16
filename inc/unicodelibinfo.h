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

#ifndef FONTFORGE_GUTILS_UNICODELIBINFO_H
#define FONTFORGE_GUTILS_UNICODELIBINFO_H

#include <fontforge-config.h>

#include "basics.h"

/* These functions are used with uninameslist or unicodenames library, if */
/* available (oldest function listed first, latest function listed last). */
/* These functions simplify the interface between FontForge and libraries */
/* so that either library can be used or none at all with common results. */
extern void inituninameannot(void);
extern char *unicode_name(int32 unienc);
extern char *unicode_annot(int32 unienc);
extern int32 unicode_block_start(int32 block_i);
extern int32 unicode_block_end(int32 block_i);
extern char *unicode_block_name(int32 block_i);
extern char *unicode_library_version(void);
extern int32 unicode_block_count(void);
extern int32 unicode_names2cnt(void);
extern int32 unicode_names2valFrmTab(int32 n);
extern int32 unicode_names2getUtabLoc(int32 unienc);
extern char *unicode_name2FrmTab(int32 n);
extern char *unicode_name2(int32 unienc);

#endif /* FONTFORGE_GUTILS_UNICODELIBINFO_H */
