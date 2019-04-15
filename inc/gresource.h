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
#ifndef _GRESOURCE_H
#define _GRESOURCE_H

#include "gdraw.h"

enum res_type { rt_int, rt_double, rt_bool/* int */, rt_color, rt_string };

typedef struct gresstruct {
    const char *resname;
    enum res_type type;
    void *val;
    void *(*cvt)(char *,void *);	/* converts a string into a whatever */
    int found;
} GResStruct;

#define GRESSTRUCT_EMPTY { NULL, 0, NULL, NULL, 0 }


extern char *GResourceProgramName, *GResourceFullProgram, *GResourceProgramDir;
extern int local_encoding;
#if HAVE_ICONV_H
# include <iconv.h>
extern char *iconv_local_encoding_name;
#endif

void GResourceSetProg(char *prog);
void GResourceAddResourceFile(char *filename,char *prog,int warn);
void GResourceAddResourceString(char *string,char *prog);
void GResourceFind( GResStruct *info, char *prefix);
char *GResourceFindString(char *name);
int GResourceFindBool(char *name, int def);
long GResourceFindInt(char *name, long def);
Color GResourceFindColor(char *name, Color def);
GImage *GResourceFindImage(char *name, GImage *def);
#endif
