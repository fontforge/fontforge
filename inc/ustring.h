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
#ifndef _UCHAR_H
# define _UCHAR_H
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include "basics.h"
#include "charset.h"

extern char *copy(const char *);
extern char *copyn(const char *,long);
extern unichar_t *u_copy(const unichar_t*);
extern unichar_t *u_copyn(const unichar_t*, long);
extern unichar_t *uc_copyn(const char *, int);
extern unichar_t *uc_copy(const char*);
extern unichar_t *u_concat(const unichar_t*,const unichar_t*);
extern char      *cu_copyn(const unichar_t *pt,int len);
extern char      *cu_copy(const unichar_t*);

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
extern void uc_strcat(unichar_t *, const char *);
extern void uc_strncat(unichar_t *, const char *,int len);
extern void cu_strcat(char *, const unichar_t *);
extern void cu_strncat(char *, const unichar_t *,int len);
extern void u_strcat(unichar_t *, const unichar_t *);
extern void u_strncat(unichar_t *, const unichar_t *, int len);
extern int  u_strlen(const unichar_t *);
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

extern char *strstart(const char *initial,const char *full);
extern char *strstartmatch(const char *initial,const char *full);
extern unichar_t *u_strstartmatch(const unichar_t *initial, const unichar_t *full);
extern unichar_t *cu_strstartmatch(const char *initial, const unichar_t *full);

#ifdef UNICHAR_16
extern uint32 *utf82u32_strncpy(int32 *ubuf,const char *utf8buf,int len);
extern uint32 *utf82u32_copy(const char *utf8buf);
extern char *u322utf8_copy(const uint32 *ubuf);
extern char *u322utf8_strncpy(char *utf8buf, const uint32 *ubuf,int len);
#else
/* Make sure we have different entry points in the library */
#define utf82u_strncpy utf82U_strncpy
#endif
extern int32 utf8_ildb(const char **utf8_text);
extern char *utf8_idpb(char *utf8_text,uint32 ch);
extern char *utf8_db(char *utf8_text);
extern char *utf8_ib(char *utf8_text);
extern int utf8_valid(const char *str);
extern void utf8_truncatevalid(char *str);
extern char *latin1_2_utf8_strcpy(char *utf8buf,const char *lbuf);
extern char *latin1_2_utf8_copy(const char *lbuf);
extern char *utf8_2_latin1_copy(const char *utf8buf);
extern int utf8_strlen(const char *utf8_str); /* how many characters in the string */
extern int utf82u_strlen(const char *utf8_str); /* how many long would this be in shorts (UCS2) */
extern char *def2utf8_copy(const char *from);
extern char *utf82def_copy(const char *ufrom);
extern char *utf8_strchr(const char *utf8_str, int search_char);

extern unichar_t *utf82u_strncpy(unichar_t *ubuf,const char *utf8buf,int len);
extern unichar_t *utf82u_strcpy(unichar_t *ubuf,const char *utf8buf);
extern void       utf82u_strcat(unichar_t *ubuf,const char *utf8buf);
extern unichar_t *utf82u_copyn(const char *utf8buf,int len);
extern unichar_t *utf82u_copy(const char *utf8buf);
extern char *u2utf8_strcpy(char *utf8buf,const unichar_t *ubuf);
extern char *u2utf8_copy(const unichar_t *ubuf);
extern char *u2utf8_copyn(const unichar_t *ubuf,int len);
extern unichar_t *encoding2u_strncpy(unichar_t *uto, const char *from, int n, enum encoding cs);
extern char *u2encoding_strncpy(char *to, const unichar_t *ufrom, int n, enum encoding cs);
extern unichar_t *def2u_strncpy(unichar_t *uto, const char *from, int n);
extern char *u2def_strncpy(char *to, const unichar_t *ufrom, int n);
extern unichar_t *def2u_copy(const char *from);
extern char *u2def_copy(const unichar_t *ufrom);

extern int u_sprintf(unichar_t *str, const unichar_t *format, ... );
extern int u_snprintf(unichar_t *str, int len, const unichar_t *format, ... );
extern int u_vsnprintf(unichar_t *str, int len, const unichar_t *format, va_list ap );

extern int uAllAscii(const unichar_t *str);
extern int AllAscii(const char *);
extern char *StripToASCII(const char *utf8_str);
#endif
