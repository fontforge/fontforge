/* Copyright (C) 2000-2008 by George Williams */
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
#ifndef _GFILE_H
#define _GFILE_H

extern char *GFileGetAbsoluteName(char *name, char *result, int rsiz);
extern char *GFileMakeAbsoluteName(char *name);
extern char *GFileBuildName(char *dir,char *fname,char *buffer,int size);
extern char *GFileReplaceName(char *oldname,char *fname,char *buffer,int size);
extern char *GFileNameTail(const char *oldname);
extern char *GFileAppendFile(char *dir,char *name,int isdir);
extern int GFileIsAbsolute(const char *file);
extern int GFileIsDir(const char *file);
extern int GFileExists(const char *file);
extern int GFileModifyable(const char *file);
extern int GFileModifyableDir(const char *file);
extern int GFileReadable(char *file);
extern int GFileMkDir(char *name);
extern int GFileRmDir(char *name);
extern int GFileUnlink(char *name);
extern char *_GFile_find_program_dir(char *prog);
extern unichar_t *u_GFileGetAbsoluteName(unichar_t *name, unichar_t *result, int rsiz);
extern unichar_t *u_GFileBuildName(unichar_t *dir,unichar_t *fname,unichar_t *buffer,int size);
extern unichar_t *u_GFileReplaceName(unichar_t *oldname,unichar_t *fname,unichar_t *buffer,int size);
extern unichar_t *u_GFileNameTail(const unichar_t *oldname);
extern unichar_t *u_GFileNormalize(unichar_t *name);
extern unichar_t *u_GFileAppendFile(unichar_t *dir,unichar_t *name,int isdir);
extern int u_GFileIsAbsolute(const unichar_t *file);
extern int u_GFileIsDir(const unichar_t *file);
extern int u_GFileExists(const unichar_t *file);
extern int u_GFileModifyable(const unichar_t *file);
extern int u_GFileModifyableDir(const unichar_t *file);
extern int u_GFileReadable(unichar_t *file);
extern int u_GFileMkDir(unichar_t *name);
extern int u_GFileRmDir(unichar_t *name);
extern int u_GFileUnlink(unichar_t *name);

#endif
