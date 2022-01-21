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

#ifndef FONTFORGE_UCHAR_H
#define FONTFORGE_UCHAR_H

#include <fontforge-config.h>

#include "basics.h"

#include <memory.h>
#include <stdarg.h>
#include <string.h>

#pragma push_macro("PRINTF_FORMAT_ATTRIBUTE")
#ifdef __GNUC__
#  if defined(__USE_MINGW_ANSI_STDIO) && __USE_MINGW_ANSI_STDIO != 0
#    define PRINTF_FORMAT_ATTRIBUTE(x, y) __attribute__((format(gnu_printf, x, y)))
#  else
#    define PRINTF_FORMAT_ATTRIBUTE(x, y) __attribute__((format(printf, x, y)))
#  endif
#else
#  define PRINTF_FORMAT_ATTRIBUTE(x, y)
#endif

extern bool SetupUCharMap(const char* unichar_name, const char* local_name, bool is_local_utf8);

extern char *copy(const char *);
extern char *copyn(const char *,long);
extern unichar_t *u_copy(const unichar_t*);
extern unichar_t *u_copyn(const unichar_t*, long);
extern unichar_t *u_copynallocm(const unichar_t *pt, long n, long m);
extern unichar_t *uc_copyn(const char *, int);
extern unichar_t *uc_copy(const char*);
extern unichar_t *u_concat(const unichar_t*,const unichar_t*);
extern char      *cu_copyn(const unichar_t *pt,int len);
extern char      *cu_copy(const unichar_t*);

extern char *vsmprintf(const char *fmt, va_list args);
extern char *smprintf(const char *fmt, ...) PRINTF_FORMAT_ATTRIBUTE(1, 2);

extern long uc_strcmp(const unichar_t *,const char *);
extern long u_strcmp(const unichar_t *, const unichar_t *);
extern long uc_strncmp(const unichar_t *,const char *,int);
extern long u_strncmp(const unichar_t *, const unichar_t *,int);
extern long uc_strmatch(const unichar_t *,const char *);
extern long uc_strnmatch(const unichar_t *,const char *,int);
extern long u_strnmatch(const unichar_t *str1, const unichar_t *str2, int len);
extern long u_strmatch(const unichar_t *, const unichar_t *);
extern int    strmatch(const char *,const char *);
extern int    strnmatch(const char *str1, const char *str2, int n);
extern void uc_strcpy(unichar_t *, const char *);
extern void cu_strcpy(char *, const unichar_t *);
extern void u_strcpy(unichar_t *, const unichar_t *);
extern void u_strncpy(unichar_t *, const unichar_t *,int);
extern void cu_strncpy(char *to, const unichar_t *from, int len);
extern void uc_strncpy(unichar_t *to, const char *from, int len);
/**
 * Like strncpy but passing a null 'from' will simply null terminate
 * to[0] to give a blank result rather than a crash.
 */
extern char *cc_strncpy(char *to, const char *from, int len);
extern void uc_strcat(unichar_t *, const char *);
extern void uc_strncat(unichar_t *, const char *,int len);
extern void cu_strcat(char *, const unichar_t *);
extern void cu_strncat(char *, const unichar_t *,int len);
extern void u_strcat(unichar_t *, const unichar_t *);
extern void u_strncat(unichar_t *, const unichar_t *, int len);
extern int  u_strlen(const unichar_t *);
/**
 * Like strlen() but passing a null pointer gets a 0 length
 */
extern int  c_strlen(const char *);
extern unichar_t *u_strchr(const unichar_t *,unichar_t);
extern unichar_t *u_strrchr(const unichar_t *,unichar_t);
extern unichar_t *uc_strstr(const unichar_t *,const char *);
extern unichar_t *u_strstr(const unichar_t *,const unichar_t *);
extern unichar_t *uc_strstrmatch(const unichar_t *,const char *);
extern unichar_t *u_strstrmatch(const unichar_t *,const unichar_t *);
extern char      *  strstrmatch(const char *,const char *);

extern char *u_to_c(const unichar_t *);
extern unichar_t *c_to_u(const char *);

extern unsigned long u_strtoul(const unichar_t *,unichar_t **,int);
extern long   u_strtol(const unichar_t *,unichar_t **,int);
extern double u_strtod(const unichar_t *,unichar_t **);

/*
 * Convert the integer 'v' to a string and return it.
 * You do not own the return value, it is an internal buffer
 * so you should copy it before using the function again
 */
extern char*  c_itostr( int v );

extern char *strstart(const char *initial,const char *full);
extern char *strstartmatch(const char *initial,const char *full);
extern unichar_t *u_strstartmatch(const unichar_t *initial, const unichar_t *full);
extern unichar_t *cu_strstartmatch(const char *initial, const unichar_t *full);

#define utf82u_strncpy utf82U_strncpy
extern int32_t utf8_ildb(const char **utf8_text);
#define UTF8IDPB_NOZERO 1	/* Allow for 0 encoded as a non-zero utf8 0xc0:0x80 char */
#define UTF8IDPB_OLDLIMIT 2	/* Today's utf8 is agreed to be limited to {0..0x10FFFF} */
#define UTF8IDPB_UCS2 8		/* Encode {0...0xffff} as 16bit ucs2 type values */
#define UTF8IDPB_UTF16 16	/* Encode {0...0x10ffff} as 16bit utf16 type values */
#define UTF8IDPB_UTF32 32	/* Encode {0...0x10ffff} as 32bit utf32 type values */
extern char *utf8_idpb(char *utf8_text,uint32_t ch,int flags);
extern char *utf8_db(char *utf8_text);
extern char *utf8_ib(char *utf8_text);
extern int utf8_valid(const char *str);
extern void utf8_truncatevalid(char *str);
extern char *latin1_2_utf8_strcpy(char *utf8buf,const char *lbuf);
extern char *latin1_2_utf8_copy(const char *lbuf);
extern char *utf8_2_latin1_copy(const char *utf8buf);
extern long utf8_strlen(const char *utf8_str);	 /* Count how many characters in the string NOT bytes */
extern long utf82u_strlen(const char *utf8_str); /* Count how many shorts needed to represent in UCS2 */
extern void utf8_strncpy(register char *to, const char *from, int len); /* copy n characters NOT bytes */
extern char *def2utf8_copy(const char *from);
extern char *utf82def_copy(const char *ufrom);
extern char *utf8_strchr(const char *utf8_str, int search_char);

extern unichar_t *utf82u_strncpy(unichar_t *ubuf,const char *utf8buf,int len);
extern unichar_t *utf82u_strcpy(unichar_t *ubuf,const char *utf8buf);
extern void       utf82u_strcat(unichar_t *ubuf,const char *utf8buf);
extern unichar_t *utf82u_copyn(const char *utf8buf,int len);
extern unichar_t *utf82u_copy(const char *utf8buf);
extern char *u2utf8_strcpy(char *utf8buf,const unichar_t *ubuf);
extern char *u2utf8_strncpy(char *utf8buf,const unichar_t *ubuf,int len);
extern char *u2utf8_copy(const unichar_t *ubuf);
extern char *u2utf8_copyn(const unichar_t *ubuf,int len);
extern unichar_t *def2u_strncpy(unichar_t *uto, const char *from, size_t n);
extern char *u2def_strncpy(char *to, const unichar_t *ufrom, size_t n);
extern unichar_t *def2u_copy(const char *from);
extern char *u2def_copy(const unichar_t *ufrom);

extern int uAllAscii(const unichar_t *str);
extern int AllAscii(const char *);
extern char *StripToASCII(const char *utf8_str);

extern char *copytolower(const char *);
extern int endswith(const char *haystack,const char *needle);
extern int endswithi(const char *haystack,const char *needle);
extern int endswithi_partialExtension( const char *haystack,const char *needle);

/**
 * Remove trailing \n or \r from the given string. No memory
 * allocations are performed, null is injected over these terminators
 * to trim the string.
 *
 * This function is designed to be impotent if called with a string
 * that does not end with \n or \r. ie, you don't need to redundantly
 * check if there is a newline at the end of string and not call here
 * if there is no newline. You can just call here with any string and
 * be assured that afterwards there will be no trailing newline or
 * carrage return character found at the end of the string pointed to
 * by 'p'.
 */
extern char* chomp( char* p );

/**
 * Return true if the haystack plain string ends with the string
 * needle. Return 0 otherwise.
 *
 * Needles which are larger than the haystack are handled.
 *
 * No new strings are allocated, freed, or returned.
 */
int endswith(const char *haystack,const char *needle);

/**
 * Return true if the haystack unicode string ends with the string needle.
 * Return 0 otherwise.
 *
 * Needles which are larger than the haystack are handled.
 *
 * No new strings are allocated, freed, or returned.
 */
extern int u_endswith(const unichar_t *haystack,const unichar_t *needle);

extern int u_startswith(const unichar_t *haystack,const unichar_t *needle);
extern int uc_startswith(const unichar_t *haystack,const char* needle);

/**
 * In the string 's' replace all occurrences of 'orig' with 'replacement'.
 * If you set free_s to true then the string 's' will be freed by this function.
 * Normally you want to set free_s to 0 to avoid that. The case you will want to
 * use free_s to 1 is chaining many calls like:
 *
 * char* s = copy( input );
 * s = str_replace_all( s, "foo", "bar", 1 );
 * s = str_replace_all( s, "baz", "gah", 1 );
 * // use s
 * free(s);
 * // no leaks in the above.
 *
 * Note that 's' is first copied before the first call to replace_all in the above
 * so it can be freed without concern. This also allows the ordering of replace_all
 * in the above to be changed without having to worry about the free_s flag.
 */
extern char* str_replace_all( char* s, char* orig, char* replacement, int free_s );


int toint( char* v );
char* tostr( int v );

#pragma pop_macro("PRINTF_FORMAT_ATTRIBUTE")

#endif /* FONTFORGE_UCHAR_H */
