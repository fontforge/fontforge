/* Copyright (C) 2009-2012 by George Williams */
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
#include "fontforge.h"
#include "baseviews.h"
#include "libffstamp.h"			/* FontForge version# date time stamps */

struct library_version_configuration exe_library_version_configuration = {
    FONTFORGE_LIBFFE_VERSION_MAJOR,
    FONTFORGE_LIBFFE_VERSION_MINOR,
    LibFF_ModTime,			/* Seconds since 1970 (standard unix time) */
    LibFF_ModTime_Str,			/* Version date (in char string format)    */
    LibFF_VersionDate,			/* Version as long value, Year, month, day */
    sizeof(struct library_version_configuration),
    sizeof(struct splinefont),
    sizeof(struct splinechar),
    sizeof(struct fontviewbase),
    sizeof(struct charviewbase),
    sizeof(struct cvcontainer),
    1,
    1,

#ifdef _NO_PYTHON
    0,
#else
    1,
#endif
    0xff		/* Not currently defined */
};
