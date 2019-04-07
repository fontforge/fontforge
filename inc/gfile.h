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
#ifndef _GFILE_H
#define _GFILE_H

#include "basics.h"

/* For mode_t */
#include <sys/types.h>

/* home directories for fontforge */
enum { Cache, Config, Data };

int mkdir_p(const char *path, mode_t mode);

extern char *GFileNormalizePath(char *path);
extern unichar_t *u_GFileNormalizePath(unichar_t *path);

extern char* GFileGetHomeDir(void);
extern unichar_t* u_GFileGetHomeDir(void);

extern char *GFileRemoveExtension(char *path);
extern char *GFileGetAbsoluteName(const char *name, char *result, size_t rsiz);
extern char *GFileMakeAbsoluteName(char *name);
extern char *GFileBuildName(char *dir,char *fname,char *buffer,size_t size);
extern char *GFileReplaceName(char *oldname,char *fname,char *buffer,size_t size);
/**
 * Return the file name for the full path 'path'. This is like the
 * shell "basename" command, for example:
 * GFileBaseName("/a/b/c/foo.sfd") returns "foo.sfd".
 *
 * You might be looking for GFileBaseName(), this function does basename from the shell.
 *
 * The return value is a pointer either being the same as path or a
 * pointer into the string that path points to. So no memory is
 * allocated by this function and the return value is dependant on the
 * 'path' string you passed in.
 */ 
extern char *GFileNameTail(const char *oldname);
extern char *GFileAppendFile(char *dir,char *name,int isdir);
extern int GFileIsAbsolute(const char *file);
extern int GFileIsDir(const char *file);
/**
 * Returns true if the file exists
 */
extern int GFileExists(const char *file);
extern int GFileModifyable(const char *file);
extern int GFileModifyableDir(const char *file);
extern int GFileReadable(const char *file);
extern FILE* GFileTmpfile();
extern int GFileRemove(const char *path, int recursive);
extern int GFileMkDir(const char *name, int mode);
extern int GFileRmDir(const char *name);
extern int GFileUnlink(const char *name);
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
extern off_t GFileGetSize(char *name);
extern char *GFileReadAll(char *name);
extern int   GFileWriteAll(char *filepath, char *data);
extern char* getGResourceProgramDir(void);
extern void  FindProgDir(char *prog);
extern char *getShareDir(void);
extern char *getLocaleDir(void);
extern char *getPixmapDir(void);
extern char *getHelpDir(void);
extern char *getUserHomeDir(void);
extern char *getFontForgeUserDir(int dir);
extern const char *getTempDir(void);

/**
 * This is the full path of ~ on OSX and Linux
 * and something like c:\Users\foo\Documents on windows
 */
extern char *GFileGetHomeDocumentsDir(void);
extern unichar_t* u_GFileGetHomeDocumentsDir(void); 

/**
 * Return the directory name for the full path 'path'.
 * This is like the shell "dirname" command, for example:
 * GFileDirName("/a/b/c/foo.sfd") returns "/a/b/c/".
 * This will also handle mingw paths as expected.
 * A trailing slash is always appended.
 *
 * The return value must be freed.
 */
extern char *GFileDirName(const char *path);

/**
 * Exactly like GFileDirName, but optionally treats the path as if
 * it were a file. This is needed for cases of treating UFO and sfdir folders as
 * 'files'.
 */
 extern char *GFileDirNameEx(const char *path, int treat_as_file);

/**
 * Get the $libexec directory for the installation. As this is only
 * used at the moment on non windows machines and has not been tested
 * on a mingw build, I have added the nonwindows postfix. When the
 * function is needed on Windows then the postfix should be dropped.
 *
 * The return value must NOT be freed.
 **/
extern char* getLibexecDir_NonWindows(void);





#endif
