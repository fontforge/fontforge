/* Copyright (C) 2024 by FontForge Authors */
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

#ifndef FONTFORGE_FFDIR_H
#define FONTFORGE_FFDIR_H

/*
 * Cross-platform directory iteration API.
 * This replaces direct use of POSIX dirent.h which is not available on MSVC.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque directory handle */
typedef struct FF_Dir FF_Dir;

/* Directory entry - contains the filename */
typedef struct {
    char name[260]; /* MAX_PATH on Windows, enough for most Unix systems */
} FF_DirEntry;

/* Open a directory for iteration. Returns NULL on error. */
FF_Dir* ff_opendir(const char* path);

/* Read the next directory entry. Returns NULL when no more entries. */
FF_DirEntry* ff_readdir(FF_Dir* dir);

/* Close the directory handle. */
void ff_closedir(FF_Dir* dir);

/* Rewind directory to the beginning. */
void ff_rewinddir(FF_Dir* dir);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_FFDIR_H */
