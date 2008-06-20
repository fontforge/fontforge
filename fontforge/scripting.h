/* Copyright (C) 2005-2008 by George Williams */
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

#ifndef _SCRIPTING_H
#define _SCRIPTING_H

#include "fontforgevw.h"
#include <setjmp.h>
#include <stdarg.h>

/* If users want to write user defined scripting built-in functions they will */
/*  need this file. The most relevant structure is the Context */

struct dictentry {
    char *name;
    Val val;
};

struct dictionary {
    struct dictentry *entries;
    int cnt, max;
};

typedef struct array {
    int argc;
    Val *vals;
} Array;

#define TOK_MAX	256
enum token_type { tt_name, tt_string, tt_number, tt_unicode, tt_real,
	tt_lparen, tt_rparen, tt_comma, tt_eos,		/* eos is end of statement, semicolon, newline */
	tt_lbracket, tt_rbracket,
	tt_minus, tt_plus, tt_not, tt_bitnot, tt_colon,
	tt_mul, tt_div, tt_mod, tt_and, tt_or, tt_bitand, tt_bitor, tt_xor,
	tt_eq, tt_ne, tt_gt, tt_lt, tt_ge, tt_le,
	tt_assign, tt_pluseq, tt_minuseq, tt_muleq, tt_diveq, tt_modeq,
	tt_incr, tt_decr,

	tt_if, tt_else, tt_elseif, tt_endif, tt_while, tt_foreach, tt_endloop,
	tt_shift, tt_return, tt_break,

	tt_eof,

	tt_error = -1
};

typedef struct context {
    struct context *caller;		/* The context of the script that called us */
    Array a;				/* The argument array */
    Array **dontfree;			/* Irrelevant for user defined funcs */
    struct dictionary locals;		/* Irrelevant for user defined funcs */
    FILE *script;			/* Irrelevant for user defined funcs */
    unsigned int backedup: 1;		/* Irrelevant for user defined funcs */
    unsigned int donteval: 1;		/* Irrelevant for user defined funcs */
    unsigned int returned: 1;		/* Irrelevant for user defined funcs */
    unsigned int broken: 1;		/* Irrelevant for user defined funcs */
    char tok_text[TOK_MAX+1];		/* Irrelevant for user defined funcs */
    enum token_type tok;		/* Irrelevant for user defined funcs */
    Val tok_val;			/* Irrelevant for user defined funcs */
    Val return_val;			/* Initialized to void. If user wants */
    					/*  return something set the return */
			                /*  value here */
    Val trace;				/* Irrelevant for user defined funcs */
    Val argsval;			/* Irrelevant for user defined funcs */
    char *filename;			/* Irrelevant for user defined funcs */
    int lineno;				/* Irrelevant for user defined funcs */
    int ungotch;			/* Irrelevant for user defined funcs */
    FontViewBase *curfv;		/* Current fontview */
    jmp_buf *err_env;			/* place to longjump to on an error */
} Context;

void arrayfree(Array *);

void FontImage(SplineFont *sf,char *filename,Array *arr,int width,int height);

 /* Adds a user defined scripting function to the interpretter */
 /* (you can't override a built-in name) */
 /* (you can replace a previous user defined function */
 /* Most functions will require a font to be loaded, but a few do not */
 /*  Open(), Exit(), Sin() don't.  ff uses the needs_font flag to perform */
 /*  this check for you */
 /* Returns 1 if the addition was successful, 2 if it replaced a previous func */
 /* Returns 0 on failure (ie. if it attempts to replace a builtin function) */
typedef void (*UserDefScriptFunc)(Context *);
extern int AddScriptingCommand(char *name,UserDefScriptFunc func,int needs_font);

 /* Returns whether a user defined scripting command already exists with the */
 /*  given name */
extern UserDefScriptFunc HasUserScriptingCommand(char *name);

 /* Scripts used to be in latin1, and we still support that if the user sets */
 /*  an environment variable. Now scripts are by default utf8. These two funcs */
 /*  will interconvert between latin1 & utf8 if appropriate, or just make a */
 /*  utf8 copy if not. They always make a copy. */
extern char *utf82script_copy(const char *ustr);
extern char *script2utf8_copy(const char *str);

 /* Various error routines. */
void ScriptError( Context *c, const char *msg );
	/* Prints an error message and exits. msg is in the script's encoding */
void ScriptErrorString( Context *c, const char *msg, const char *name);
	/* Prints an error message followed by a string and exits. */
	/*  both strings are in the script's encoding */
void ScriptErrorF( Context *c, const char *fmt, ... );
	/* Standard printf-style spec. All string arguments assumed to be in */
	/* utf8 */

extern int running_script;

/* Hooks so a scripting dlg can execute fontforge's legacy scripting language */
extern void ff_VerboseCheck(void);
extern enum token_type ff_NextToken(Context *c);
extern void ff_backuptok(Context *c);
extern void ff_statement(Context*);

#endif	/* _SCRIPTING_H */
