/* Copyright (C) 2016-2022 by Jeremy Tan */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

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

#ifndef FONTFORGE_GDRAWLOGGER_H
#define FONTFORGE_GDRAWLOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <fontforge-config.h>

#pragma push_macro("PRINTF_FORMAT_ATTRIBUTE")
#ifdef __GNUC__
#if defined(__USE_MINGW_ANSI_STDIO) && __USE_MINGW_ANSI_STDIO != 0
#define PRINTF_FORMAT_ATTRIBUTE(x, y) __attribute__((format(gnu_printf, x, y)))
#else
#define PRINTF_FORMAT_ATTRIBUTE(x, y) __attribute__((format(printf, x, y)))
#endif
#else
#define PRINTF_FORMAT_ATTRIBUTE(x, y)
#endif

//To get around a 'pedantic' C99 rule that you must have at least 1 variadic arg, combine fmt into that.
#define Log(level, ...) LogEx(level, __func__, __FILE__, __LINE__, __VA_ARGS__)

/** An enum to make the severity of log messages human readable in code **/
enum {LOGNONE = 0, LOGERR = 1, LOGWARN = 2, LOGINFO = 3, LOGDEBUG = 4};

extern void LogInit(void);
extern void LogEx(int level, const char *funct, const char *file, int line, const char *fmt,  ...) PRINTF_FORMAT_ATTRIBUTE(5, 6);   // General function for printing log messages to stderr

#ifdef FONTFORGE_CAN_USE_GDK
extern const char *GdkEventName(int code);
#endif

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_GDRAWLOGGER_H */