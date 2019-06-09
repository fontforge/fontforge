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

#ifndef FONTFORGE_GIO_H
#define FONTFORGE_GIO_H

#include "basics.h"

#include <time.h>

enum giofuncs { gf_dir, gf_statfile, gf_getfile, gf_putfile,
	gf_mkdir, gf_delfile, gf_deldir, gf_renamefile,
	/*gf_lockfile, gf_unlockfile,*/
	gf_max };

typedef struct giocontrol {
    unichar_t *path;
    unichar_t *origpath;		/* what the user asked for (before any redirects), NULL if path doesn't change */
    unichar_t *topath;			/* for renames and copies */
    void *userdata;
    void *iodata;
    void (*receivedata)(struct giocontrol *);
    void (*receiveintermediate)(struct giocontrol *);
    void (*receiveerror)(struct giocontrol *);
    unsigned int done: 1;
    unsigned int direntrydata: 1;
    unsigned int abort: 1;
    enum giofuncs gf;
    int protocol_index;
    struct giocontrol *next;
    int return_code;
    unichar_t *error;
    unichar_t status[80];
} GIOControl;

typedef struct gdirentry {
    unichar_t *name;
    unichar_t *mimetype;
    unsigned int isdir: 1;
    unsigned int isexe: 1;
    unsigned int islnk: 1;
    unsigned int hasdir: 1;
    unsigned int hasexe: 1;
    unsigned int haslnk: 1;
    unsigned int hasmode: 1;
    unsigned int hassize: 1;
    unsigned int hastime: 1;
    unsigned int timezoneknown: 1;	/* We got a time, but we don't know the timezone. might be off by 24 hours either way */
    unsigned int fcdata: 2;
    short mode;
    uint32 size;
    time_t modtime;
    struct gdirentry *next;
} GDirEntry;

extern void GIOdir(GIOControl *gc);
extern void GIOstatFile(GIOControl *gc);
extern void GIOfileExists(GIOControl *gc);
extern void GIOmkDir(GIOControl *gc);
extern void GIOdelFile(GIOControl *gc);
extern void GIOdelDir(GIOControl *gc);
extern void GIOrenameFile(GIOControl *gc);
extern GDirEntry *GIOgetDirData(GIOControl *gc);
extern int32 GIOread(GIOControl *gc,void *buffer,int32 len);
extern int32 GIOwrite(GIOControl *gc,void *buffer,int32 len);
extern void GIOFreeDirEntries(GDirEntry *lst);
extern void GIOcancel(GIOControl *gc);
extern void GIOclose(GIOControl *gc);
extern GIOControl *GIOCreate(unichar_t *path,void *userdata,
	void (*receivedata)(struct giocontrol *),
	void (*receiveerror)(struct giocontrol *));


extern char *GIOguessMimeType(const char *path);
extern char *GIOGetMimeType(const char *path);

#endif /* FONTFORGE_GIO_H */
