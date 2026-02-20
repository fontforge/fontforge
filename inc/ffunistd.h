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

#ifndef FONTFORGE_FFUNISTD_H
#define FONTFORGE_FFUNISTD_H

/*
 * Cross-platform unistd.h compatibility.
 * MSVC doesn't have unistd.h, but provides equivalent functions with
 * different names in io.h, process.h, and direct.h.
 */

#ifdef _MSC_VER

#include <io.h>
#include <process.h>
#include <direct.h>
#include <stdlib.h>

/* File access mode constants */
#ifndef F_OK
#define F_OK 0
#endif
#ifndef X_OK
#define X_OK 1
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef R_OK
#define R_OK 4
#endif

/* PATH_MAX - use MAX_PATH on Windows */
#ifndef PATH_MAX
#define PATH_MAX 260
#endif

/* Map POSIX names to MSVC equivalents */
#define access _access
#define unlink _unlink
#define getpid _getpid
#define getcwd _getcwd
#define chdir _chdir
#define rmdir _rmdir
#define dup _dup
#define dup2 _dup2
#define close _close
#define read _read
#define write _write
#define lseek _lseek
#define isatty _isatty

/* P_tmpdir - POSIX temp directory path */
static inline char* _ff_get_tmpdir(void) {
    char* dir = getenv("TEMP");
    if (dir == NULL) dir = getenv("TMP");
    if (dir == NULL) dir = (char*)"C:\\Temp";
    return dir;
}
#define P_tmpdir (_ff_get_tmpdir())

/* MSVC doesn't have fork/exec - these would need separate handling */
/* Files using these will need #ifndef _MSC_VER guards */

#else /* !_MSC_VER */

#include <unistd.h>

#endif /* _MSC_VER */

#endif /* FONTFORGE_FFUNISTD_H */
