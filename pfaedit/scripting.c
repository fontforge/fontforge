/* Copyright (C) 2002 by George Williams */
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
/*			   Yet another interpreter			      */

#include "pfaeditui.h"
#include <gfile.h>
#include <gresource.h>
#include <utype.h>
#include <ustring.h>
#include <gkeysym.h>
#include <chardata.h>
#include <unistd.h>
#include <math.h>
#include <setjmp.h>

typedef struct val {
    enum val_type { v_int, v_str, v_unicode, v_lval, v_arr, v_arrfree, v_void } type;
    union {
	int ival;
	char *sval;
	struct val *lval;
	struct array *aval;
    } u;
} Val;

struct dictentry {
    char *name;
    Val val;
};

typedef struct array {
    int argc;
    Val *vals;
} Array;

#define TOK_MAX	100
enum token_type { tt_name, tt_string, tt_number, tt_unicode,
	tt_lparen, tt_rparen, tt_comma, tt_eos,		/* eos is end of statement, semicolon, newline */
	tt_lbracket, tt_rbracket,
	tt_minus, tt_plus, tt_not, tt_bitnot, tt_colon,
	tt_mul, tt_div, tt_mod, tt_and, tt_or, tt_bitand, tt_bitor, tt_xor,
	tt_eq, tt_ne, tt_gt, tt_lt, tt_ge, tt_le,
	tt_assign, tt_pluseq, tt_minuseq, tt_muleq, tt_diveq, tt_modeq,
	tt_incr, tt_decr,

	tt_if, tt_else, tt_elseif, tt_endif, tt_while, tt_endloop,
	tt_shift, tt_return,

	tt_eof,

	tt_error = -1
};

typedef struct context {
    struct context *caller;
    Array a;		/* args */
    Array **dontfree;
    struct dictentry *locals;
    int lc, lmax;
    FILE *script;
    unsigned int backedup: 1;
    unsigned int donteval: 1;
    unsigned int returned: 1;
    char tok_text[TOK_MAX+1];
    enum token_type tok;
    Val tok_val;
    Val return_val;
    Val trace;
    Val argsval;
    char *filename;
    int lineno;
    FontView *curfv;
    jmp_buf *err_env;
} Context;

struct keywords { enum token_type tok; char *name; } keywords[] = {
    { tt_if, "if" },
    { tt_else, "else" },
    { tt_elseif, "elseif" },
    { tt_endif, "endif" },
    { tt_while, "while" },
    { tt_endloop, "endloop" },
    { tt_shift, "shift" },
    { tt_return, "return" },
    { 0, NULL }
};

static const char *toknames[] = {
    "name", "string", "number", "unicode id", 
    "lparen", "rparen", "comma", "end of statement",
    "lbracket", "rbracket",
    "minus", "plus", "logical not", "bitwise not", "colon",
    "multiply", "divide", "mod", "logical and", "logical or", "bitwise and", "bitwise or", "bitwise xor",
    "equal to", "not equal to", "greater than", "less than", "greater than or equal to", "less than or equal to",
    "assignment", "plus equals", "minus equals", "mul equals", "div equals", "mod equals",
    "increment", "decrement",
    "if", "else", "elseif", "endif", "while", "endloop",
    "shift", "return",
    "End Of File",
    NULL };

static void arrayfree(Array *a) {
    int i;

    for ( i=0; i<a->argc; ++i ) {
	if ( a->vals[i].type==v_str )
	    free(a->vals[i].u.sval);
	else if ( a->vals[i].type==v_arr )
	    arrayfree(a->vals[i].u.aval);
    }
    free(a->vals);
    free(a);
}

static Array *arraycopy(Array *a) {
    int i;
    Array *c;

    c = galloc(sizeof(Array));
    c->argc = a->argc;
    c->vals = galloc(c->argc*sizeof(Val));
    memcpy(c->vals,a->vals,c->argc*sizeof(Val));
    for ( i=0; i<a->argc; ++i ) {
	if ( a->vals[i].type==v_str )
	    c->vals[i].u.sval = copy(a->vals[i].u.sval);
	else if ( a->vals[i].type==v_arr )
	    c->vals[i].u.aval = arraycopy(a->vals[i].u.aval);
    }
return( c );
}

static void calldatafree(Context *c) {
    int i;

    for ( i=1; i<c->a.argc; ++i ) {	/* child may have freed some args itself by shifting, but argc will reflect the proper values none the less */
	if ( c->a.vals[i].type == v_str )
	    free( c->a.vals[i].u.sval );
	if ( c->a.vals[i].type == v_arrfree || (c->a.vals[i].type == v_arr && c->dontfree[i]!=c->a.vals[i].u.aval ))
	    arrayfree( c->a.vals[i].u.aval );
    }
    for ( i=0; i<c->lc; ++i ) {
	free(c->locals[i].name );
	if ( c->locals[i].val.type == v_str )
	    free( c->locals[i].val.u.sval );
	if ( c->locals[i].val.type == v_arr )
	    arrayfree( c->locals[i].val.u.aval );
    }
    free( c->locals );

    if ( c->script!=NULL )
	fclose(c->script);
}

static void traceback(Context *c) {
    int cnt = 0;
    while ( c!=NULL ) {
	if ( cnt==1 ) printf( "Called from...\n" );
	if ( cnt>0 ) fprintf( stderr, " %s:%d\n", c->filename, c->lineno );
	calldatafree(c);
	if ( c->err_env!=NULL )
	    longjmp(*c->err_env,1);
	c = c->caller;
	++cnt;
    }
    exit(1);
}

static void showtoken(Context *c,enum token_type got) {
    if ( got==tt_name || got==tt_string )
	fprintf( stderr, " \"%s\"\n", c->tok_text );
    else if ( got==tt_number )
	fprintf( stderr, " %d (0x%x)\n", c->tok_val.u.ival, c->tok_val.u.ival );
    else if ( got==tt_unicode )
	fprintf( stderr, " 0u%x\n", c->tok_val.u.ival );
    else
	fprintf( stderr, "\n" );
    traceback(c);
}

static void expect(Context *c,enum token_type expected, enum token_type got) {
    if ( got!=expected ) {
	fprintf( stderr, "%s:%d Expected %s, got %s",
		c->filename, c->lineno, toknames[expected], toknames[got] );
    if ( screen_display!=NULL ) {
	static unichar_t umsg[] = { '%','h','s',':','%','d',' ','E','x','p','e','c','t','e','d',' ','%','h','s',',',' ','g','o','t',' ','%','h','s',  0 };
	GWidgetError(NULL,umsg,c->filename, c->lineno, toknames[expected], toknames[got] );
    }
	showtoken(c,got);
    }
}

static void unexpected(Context *c,enum token_type got) {
    fprintf( stderr, "%s:%d Unexpected %s found",
	    c->filename, c->lineno, toknames[got] );
    if ( screen_display!=NULL ) {
	static unichar_t umsg[] = { '%','h','s',':','%','d',' ','U','n','e','x','p','e','c','t','e','d',' ','%','h','s',  0 };
	GWidgetError(NULL,umsg,c->filename, c->lineno, toknames[got] );
    }
    showtoken(c,got);
}

static void error( Context *c, char *msg ) {
    fprintf( stderr, "%s:%d %s\n", c->filename, c->lineno, msg );
    if ( screen_display!=NULL ) {
	static unichar_t umsg[] = { '%','h','s',':','%','d',' ','%','h','s',  0 };
	GWidgetError(NULL,umsg,c->filename, c->lineno, msg );
    }
    traceback(c);
}

static void errors( Context *c, char *msg, char *name) {
    fprintf( stderr, "%s:%d %s: %s\n", c->filename, c->lineno, msg, name );
    if ( screen_display!=NULL ) {
	static unichar_t umsg[] = { '%','h','s',':','%','d',' ','%','h','s',':',' ','%','h','s',  0 };
	GWidgetError(NULL,umsg,c->filename, c->lineno, msg, name );
    }
    traceback(c);
}

static void dereflvalif(Val *val) {
    if ( val->type == v_lval ) {
	*val = *val->u.lval;
	if ( val->type==v_str )
	    val->u.sval = copy(val->u.sval);
    }
}

/* *************************** Built in Functions *************************** */

static void PrintVal(Val *val) {
    int j;

    if ( val->type==v_str )
	printf( "%s", val->u.sval );
    else if ( val->type==v_arr ) {
	putchar( '[' );
	if ( val->u.aval->argc>0 ) {
	    PrintVal(&val->u.aval->vals[0]);
	    for ( j=1; j<val->u.aval->argc; ++j ) {
		putchar(',');
		PrintVal(&val->u.aval->vals[j]);
	    }
	}
	putchar( ']' );
    } else if ( val->type==v_int )
	printf( "%d", val->u.ival );
    else if ( val->type==v_unicode )
	printf( "0u%x", val->u.ival );
    else if ( val->type==v_void )
	printf( "<void>");
    else
	printf( "<???>");
}

static void bPrint(Context *c) {
    int i;

    for ( i=1; i<c->a.argc; ++i )
	PrintVal(&c->a.vals[i] );
    printf( "\n" );
}

static void bArray(Context *c) {
    int i;

    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments" );
    else if ( c->a.vals[1].type!=v_int )
	error( c, "Expected integer argument" );
    else if ( c->a.vals[1].u.ival<=0 )
	error( c, "Argument must be positive" );
    c->return_val.type = v_arrfree;
    c->return_val.u.aval = galloc(sizeof(Array));
    c->return_val.u.aval->argc = c->a.vals[1].u.ival;
    c->return_val.u.aval->vals = galloc(c->a.vals[1].u.ival*sizeof(Val));
    for ( i=0; i<c->a.vals[1].u.ival; ++i )
	c->return_val.u.aval->vals[i].type = v_void;
}

static void bStrlen(Context *c) {

    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments" );
    else if ( c->a.vals[1].type!=v_str )
	error( c, "Bad type for argument" );

    c->return_val.type = v_int;
    c->return_val.u.ival = strlen( c->a.vals[1].u.sval );
}

static void bStrstr(Context *c) {
    char *pt;

    if ( c->a.argc!=3 )
	error( c, "Wrong number of arguments" );
    else if ( c->a.vals[1].type!=v_str || c->a.vals[2].type!=v_str )
	error( c, "Bad type for argument" );

    c->return_val.type = v_int;
    pt = strstr(c->a.vals[1].u.sval,c->a.vals[2].u.sval);
    c->return_val.u.ival = pt==NULL ? -1 : pt-c->a.vals[1].u.sval;
}

static void bStrcasestr(Context *c) {
    char *pt;

    if ( c->a.argc!=3 )
	error( c, "Wrong number of arguments" );
    else if ( c->a.vals[1].type!=v_str || c->a.vals[2].type!=v_str )
	error( c, "Bad type for argument" );

    c->return_val.type = v_int;
    pt = strstrmatch(c->a.vals[1].u.sval,c->a.vals[2].u.sval);
    c->return_val.u.ival = pt==NULL ? -1 : pt-c->a.vals[1].u.sval;
}

static void bStrrstr(Context *c) {
    char *pt;
    char *haystack, *needle;
    int nlen;

    if ( c->a.argc!=3 )
	error( c, "Wrong number of arguments" );
    else if ( c->a.vals[1].type!=v_str || c->a.vals[2].type!=v_str )
	error( c, "Bad type for argument" );

    c->return_val.type = v_int;
    haystack = c->a.vals[1].u.sval; needle = c->a.vals[2].u.sval;
    nlen = strlen( needle );
    for ( pt=haystack+strlen(haystack)-nlen; pt>=haystack; --pt )
	if ( strncmp(pt,needle,nlen)==0 )
    break;
    c->return_val.u.ival = pt-haystack;
}

static void bStrsub(Context *c) {
    int start, end;
    char *str;

    if ( c->a.argc!=3 && c->a.argc!=4 )
	error( c, "Wrong number of arguments" );
    else if ( c->a.vals[1].type!=v_str || c->a.vals[2].type!=v_int ||
	    (c->a.argc==4 && c->a.vals[3].type!=v_int))
	error( c, "Bad type for argument" );

    str = c->a.vals[1].u.sval;
    start = c->a.vals[2].u.ival;
    end = c->a.argc==4? c->a.vals[2].u.ival : strlen(str);
    if ( start<0 || start>strlen(str) || end<start || end>strlen(str) )
	error( c, "Arguments out of bounds" );
    c->return_val.type = v_str;
    c->return_val.u.sval = copyn(str+start,end-start);
}

static void bStrcasecmp(Context *c) {

    if ( c->a.argc!=3 )
	error( c, "Wrong number of arguments" );
    else if ( c->a.vals[1].type!=v_str || c->a.vals[2].type!=v_str )
	error( c, "Bad type for argument" );

    c->return_val.type = v_int;
    c->return_val.u.ival = strmatch(c->a.vals[1].u.sval,c->a.vals[2].u.sval);
}

/* **** File menu **** */

static void bQuit(Context *c) {
    if ( c->a.argc==1 )
exit(0);
    if ( c->a.argc>2 )
	error( c, "Too many arguments" );
    else if ( c->a.vals[1].type!=v_int )
	error( c, "Expected integer argument" );
    else
exit(c->a.vals[1].u.ival );
exit(1);
}

static FontView *FVAppend(FontView *fv) {
    /* Normally fontviews get added to the fv list when their windows are */
    /*  created. but we don't create any windows here, so... */
    FontView *test;

    if ( fv_list==NULL ) fv_list = fv;
    else {
	for ( test = fv_list; test->next!=NULL; test=test->next );
	test->next = fv;
    }
return( fv );
}

static void bOpen(Context *c) {
    SplineFont *sf;

    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments to Open");
    else if ( c->a.vals[1].type!=v_str )
	error( c, "Open expects a filename" );
    sf = LoadSplineFont(c->a.vals[1].u.sval);
    if ( sf==NULL )
	errors(c, "Failed to open", c->a.vals[1].u.sval);
    if ( sf->fv==NULL )
	FVAppend(_FontViewCreate(sf));
    c->curfv = sf->fv;
}

static void bNew(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to New");
    c->curfv = FVAppend(_FontViewCreate(SplineFontNew()));
}

static void bClose(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to Close");
    if ( fv_list==c->curfv )
	fv_list = c->curfv->next;
    else {
	FontView *n;
	for ( n=fv_list; n->next!=c->curfv; n=n->next );
	n->next = c->curfv->next;
    }
    FontViewFree(c->curfv);
    c->curfv = NULL;
}

static void bSave(Context *c) {
    SplineFont *sf = c->curfv->sf;

    if ( c->a.argc>2 )
	error( c, "Wrong number of arguments to Save");
    if ( c->a.argc==2 ) {
	if ( c->a.vals[1].type!=v_str )
	    error(c,"If an argument is given to Save it must be a filename");
	if ( !SFDWrite(c->a.vals[1].u.sval,sf))
	    error(c,"Save As failed" );
    } else {
	if ( sf->filename==NULL )
	    error(c,"This font has no associated sfd file yet, you must specify a filename" );
	if ( !SFDWriteBak(sf) )
	    error(c,"Save failed" );
    }
}

static void bGenerate(Context *c) {
    SplineFont *sf = c->curfv->sf;
    char *bitmaptype = "";

    if ( c->a.argc!=2 && c->a.argc!=3 )
	error( c, "Wrong number of arguments to Generate");
    if ( c->a.vals[1].type!=v_str || (c->a.argc==3 && c->a.vals[2].type!=v_str ))
	error( c, "Bad type of arguments to Generate");
    if ( c->a.argc==3 )
	bitmaptype = c->a.vals[2].u.sval;
    if ( !GenerateScript(sf,c->a.vals[1].u.sval,bitmaptype) )
	error(c,"Save failed");
}

static void bImport(Context *c) {
    char *ext;
    int format, back, ok;

    if ( c->a.argc!=2 && c->a.argc!=3 )
	error( c, "Wrong number of arguments to Import");
    if ( c->a.vals[1].type!=v_str || (c->a.argc==3 && c->a.vals[2].type!=v_int ))
	error( c, "Bad type of arguments to Import");
    ext = strrchr(c->a.vals[1].u.sval,'.');
    if ( ext==NULL ) {
	int len = strlen(c->a.vals[1].u.sval);
	ext = c->a.vals[1].u.sval+len-2;
	if ( ext[0]!='p' || ext[1]!='k' )
	    errors( c, "No extension in", c->a.vals[1].u.sval);
    }
    back = 0;
    if ( strmatch(ext,".bdf")==0 )
	format = 0;
    else if ( strmatch(ext,".ttf")==0 )
	format = 1;
    else if ( strmatch(ext,"pk")==0 || strmatch(ext,".pk")==0 ) {
	format = 2;
	back = 1;
    } else if ( strchr(c->a.vals[1].u.sval,'*')==NULL )
	format = 3;
    else
	format = 4;
    if ( c->a.argc==3 )
	back = c->a.vals[2].u.ival;
    if ( format==0 )
	ok = FVImportBDF(c->curfv,c->a.vals[1].u.sval,false, back);
    else if ( format==1 )
	ok = FVImportTTF(c->curfv,c->a.vals[1].u.sval, back);
    else if ( format==2 )
	ok = FVImportBDF(c->curfv,c->a.vals[1].u.sval,true, back);
    else if ( format==3 )
	ok = FVImportImages(c->curfv,c->a.vals[1].u.sval);
    else
	ok = FVImportImageTemplate(c->curfv,c->a.vals[1].u.sval);
    if ( !ok )
	error(c,"Import failed" );
}

/* **** Edit menu **** */
static void doEdit(Context *c, int cmd) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to edit command");
    FVFakeMenus(c->curfv,cmd);
}

static void bCut(Context *c) {
    doEdit(c,0);
}

static void bCopy(Context *c) {
    doEdit(c,1);
}

static void bCopyReference(Context *c) {
    doEdit(c,2);
}

static void bCopyWidth(Context *c) {
    doEdit(c,3);
}

static void bPaste(Context *c) {
    doEdit(c,4);
}

static void bPasteInto(Context *c) {
    doEdit(c,9);
}

static void bClear(Context *c) {
    doEdit(c,5);
}

static void bClearBackground(Context *c) {
    doEdit(c,6);
}

static void bCopyFgToBg(Context *c) {
    doEdit(c,7);
}

static void bUnlinkReference(Context *c) {
    doEdit(c,8);
}

static void bSelectAll(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to SelectAll");
    memset(c->curfv->selected,1,c->curfv->sf->charcnt);
}

static void bSelectNone(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to SelectNone");
    memset(c->curfv->selected,0,c->curfv->sf->charcnt);
}

static int ParseCharIdent(Context *c, Val *val) {
    SplineFont *sf = c->curfv->sf;
    int bottom = -1;

    if ( val->type==v_int )
	bottom = val->u.ival;
    else if ( val->type==v_unicode || val->type==v_str ) {
	int uni = -1;
	char *str = NULL;
	if ( val->type==v_unicode )
	    uni = val->u.ival;
	else {
	    str = val->u.sval;
	    if ( sscanf(str,"uni%x", &uni)==0 )
		sscanf( str, "U+%x", &uni);
	}
	bottom = SFFindChar(sf,uni,str);
    } else
	error( c, "Bad type for argument");
    if ( bottom==-1 )
	error( c, "Character not found" );
    else if ( bottom<0 || bottom>=sf->charcnt )
	error( c, "Character is not in font" );
return( bottom );
}

static void bDoSelect(Context *c) {
    int top, bottom, i,j;

    for ( i=1; i<c->a.argc; i+=2 ) {
	bottom = ParseCharIdent(c,&c->a.vals[i]);
	if ( i+1==c->a.argc )
	    top = bottom;
	else {
	    top = ParseCharIdent(c,&c->a.vals[i+1]);
	}
	if ( top<bottom ) { j=top; top=bottom; bottom = j; }
	for ( j=bottom; j<=top; ++j )
	    c->curfv->selected[j] = true;
    }
}

static void bSelectMore(Context *c) {
    if ( c->a.argc==1 )
	error( c, "SelectMore needs at least one argument");
    bDoSelect(c);
}

static void bSelect(Context *c) {
    memset(c->curfv->selected,0,c->curfv->sf->charcnt);
    bDoSelect(c);
}

/* **** Element Menu **** */
static void bReencode(Context *c) {
    Encoding *item=NULL;
    int i;
    enum charset new_map = em_none;
    static struct encdata {
	int val;
	char *name;
    } encdata[] = {
	{ em_iso8859_1, "iso8859-1" },
	{ em_iso8859_1, "isolatin1" },
	{ em_iso8859_1, "latin1" },
	{ em_iso8859_2, "iso8859-2" },
	{ em_iso8859_2, "latin2" },
	{ em_iso8859_3, "iso8859-3" },
	{ em_iso8859_3, "latin3" },
	{ em_iso8859_4, "iso8859-4" },
	{ em_iso8859_4, "latin4" },
	{ em_iso8859_5, "iso8859-5" },
	{ em_iso8859_5, "isocyrillic" },
	{ em_iso8859_6, "iso8859-6" },
	{ em_iso8859_6, "isoarabic" },
	{ em_iso8859_7, "iso8859-7" },
	{ em_iso8859_7, "isogreek" },
	{ em_iso8859_8, "iso8859-8" },
	{ em_iso8859_8, "isohebrew" },
	{ em_iso8859_9, "iso8859-9" },
	{ em_iso8859_9, "latin5" },
	{ em_iso8859_10, "iso8859-10" },
	{ em_iso8859_10, "latin6" },
	{ em_iso8859_13, "iso8859-13" },
	{ em_iso8859_13, "latin7" },
	{ em_iso8859_14, "iso8859-14" },
	{ em_iso8859_14, "latin8" },
	{ em_iso8859_15, "iso8859-15" },
	{ em_iso8859_15, "latin0" },
	{ em_iso8859_15, "latin9" },
	{ em_koi8_r, "koi8-r" },
	{ em_koi8_r, "koi8r" },
	{ em_jis201, "jis201" },
	{ em_adobestandard, "AdobeStandardEncoding" },
	{ em_adobestandard, "Adobe" },
	{ em_win, "win" },
	{ em_mac, "mac" },
	{ em_ksc5601, "ksc5601" },
	{ em_ksc5601, "wansung" },
	{ em_big5, "big5" },
	{ em_johab, "johab" },
	{ em_jis208, "jis208" },
	{ em_unicode, "unicode" },
	{ em_unicode, "iso10646" },
	{ em_unicode, "iso10646-1" },
	{ 0, NULL}};
    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments to Reencode");
    else if ( c->a.vals[1].type!=v_str )
	error(c,"Bad argument type in Reencode");
    for ( i=0; encdata[i].name!=NULL; ++i )
	if ( strmatch(encdata[i].name,c->a.vals[1].u.sval)==0 ) {
	    new_map = encdata[i].val;
    break;
	}
    if ( new_map == em_none ) {
	for ( item=enclist; item!=NULL ; item=item->next )
	    if ( strmatch(c->a.vals[1].u.sval,item->enc_name )==0 ) {
		new_map = item->enc_num;
	break;
	    }
    }
    if ( new_map==em_none )
	errors(c,"Unknown encoding", c->a.vals[1].u.sval);
    SFReencodeFont(c->curfv->sf,new_map);
    free( c->curfv->selected );
    c->curfv->selected = gcalloc(c->curfv->sf->charcnt,1);
}

static void bSetCharCnt(Context *c) {
    int oldcnt = c->curfv->sf->charcnt, i;

    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments");
    else if ( c->a.vals[1].type!=v_int )
	error(c,"Bad argument type");
    else if ( c->a.vals[1].u.ival<=0 && c->a.vals[1].u.ival>10*65536 )
	error(c,"Argument out of bounds");

    if ( c->curfv->sf->charcnt==c->a.vals[1].u.ival )
return;

    SFAddDelChars(c->curfv->sf,c->a.vals[1].u.ival);
    c->curfv->selected = grealloc(c->curfv->selected,c->a.vals[1].u.ival);
    for ( i=oldcnt; i<c->a.vals[1].u.ival; ++i )
	c->curfv->selected[i] = false;
}

static SplineChar *GetOneSelChar(Context *c) {
    SplineFont *sf = c->curfv->sf;
    int i, found = -1;

    for ( i=0; i<sf->charcnt; ++i ) if ( c->curfv->selected[i] ) {
	if ( found==-1 )
	    found = i;
	else
	    error(c,"More than one character selected" );
    }
    if ( found==-1 )
	error(c,"No characters selected" );
return( SFMakeChar(sf,found));
}

static void bSetCharName(Context *c) {
    SplineChar *sc;
    char *ligature, *name, *end;
    int uni;

    if ( c->a.argc!=2 && c->a.argc!=3 )
	error( c, "Wrong number of arguments");
    else if ( c->a.vals[1].type!=v_str || (c->a.argc==3 && c->a.vals[1].type!=v_int ))
	error(c,"Bad argument type");
    sc = GetOneSelChar(c);
    uni = sc->unicodeenc;
    name = c->a.vals[1].u.sval;
    ligature = sc->lig==NULL?NULL : copy(sc->lig->components);

    if ( c->a.vals[2].u.ival ) {
	if ( name[0]=='u' && name[1]=='n' && name[2]=='i' && strlen(name)==7 &&
		(uni = strtol(name+3,&end,16), *end=='\0'))
	    /* Good */;
	else {
	    for ( uni=psunicodenames_cnt-1; uni>=0; --uni )
		if ( psunicodenames[uni]!=NULL && strcmp(name,psunicodenames[uni])==0 )
	    break;
	}
	free( ligature );
	ligature = LigDefaultStr(uni,name);
    }
    SCSetMetaData(sc,name,uni,ligature);
}

static void bSetUnicodeValue(Context *c) {
    SplineChar *sc;
    char *ligature, *name;
    int uni;

    if ( c->a.argc!=2 && c->a.argc!=3 )
	error( c, "Wrong number of arguments");
    else if ( (c->a.vals[1].type!=v_int && c->a.vals[1].type!=v_unicode) ||
	    (c->a.argc==3 && c->a.vals[1].type!=v_int ))
	error(c,"Bad argument type");
    sc = GetOneSelChar(c);
    uni = c->a.vals[1].u.ival;
    name = copy(name);
    ligature = sc->lig==NULL?NULL : copy(sc->lig->components);

    if ( c->a.vals[2].u.ival ) {
	free(name);
	if ( uni>=0 && uni<psunicodenames_cnt && psunicodenames[uni]!=NULL )
	    name = copy(psunicodenames[uni]);
	else if (( uni>=32 && uni<0x7f ) || uni>=0xa1 ) {
	    char buf[12];
	    sprintf( buf,"uni%04X", uni );
	    name = copy(buf);
	} else
	    name = copy(".notdef");
	free( ligature );
	ligature = LigDefaultStr(uni,name);
    }
    SCSetMetaData(sc,name,uni,ligature);
}

static void bTransform(Context *c) {
    real trans[6];
    BVTFunc bvts[1];

    if ( c->a.argc!=7 )
	error( c, "Wrong number of arguments to Transform");
    else if ( c->a.vals[1].type!=v_int || c->a.vals[2].type!=v_int ||
	      c->a.vals[3].type!=v_int || c->a.vals[4].type!=v_int ||
	      c->a.vals[5].type!=v_int || c->a.vals[6].type!=v_int )
	error(c,"Bad argument type in Transform");
    trans[0] = c->a.vals[1].u.ival/100.;
    trans[1] = c->a.vals[2].u.ival/100.;
    trans[2] = c->a.vals[3].u.ival/100.;
    trans[3] = c->a.vals[4].u.ival/100.;
    trans[4] = c->a.vals[5].u.ival/100.;
    trans[5] = c->a.vals[6].u.ival/100.;
    bvts[0].func = bvt_none;
    FVTransFunc(c->curfv,trans,0,bvts,true);
}

static void bHFlip(Context *c) {
    real trans[6];
    int otype = 1;
    BVTFunc bvts[2];

    trans[0] = -1; trans[3] = 1;
    trans[1] = trans[2] = trans[4] = trans[5] = 0;
    if ( c->a.argc==1 )
	/* default to center of char for origin */;
    else if ( c->a.argc==2 || c->a.argc==3 ) {
	if ( c->a.vals[1].type!=v_int || ( c->a.argc==3 && c->a.vals[2].type!=v_int ))
	    error(c,"Bad argument type in HFlip");
	trans[4] = 2*c->a.vals[1].u.ival;
	otype = 0;
    } else
	error( c, "Wrong number of arguments to HFlip");
    bvts[0].func = bvt_fliph;
    bvts[1].func = bvt_none;
    FVTransFunc(c->curfv,trans,otype,bvts,true);
}

static void bVFlip(Context *c) {
    real trans[6];
    int otype = 1;
    BVTFunc bvts[2];

    trans[0] = 1; trans[3] = -1;
    trans[1] = trans[2] = trans[4] = trans[5] = 0;
    if ( c->a.argc==1 )
	/* default to center of char for origin */;
    else if ( c->a.argc==2 || c->a.argc==3 ) {
	if ( c->a.vals[1].type!=v_int || ( c->a.argc==3 && c->a.vals[2].type!=v_int ))
	    error(c,"Bad argument type in VFlip");
	if ( c->a.argc==2 )
	    trans[5] = 2*c->a.vals[1].u.ival;
	else
	    trans[5] = 2*c->a.vals[2].u.ival;
	otype = 0;
    } else
	error( c, "Wrong number of arguments to VFlip");
    bvts[0].func = bvt_flipv;
    bvts[1].func = bvt_none;
    FVTransFunc(c->curfv,trans,otype,bvts,true);
}

static void bRotate(Context *c) {
    real trans[6];
    int otype = 1;
    BVTFunc bvts[2];
    double a;

    if ( c->a.argc==1 || c->a.argc==3 || c->a.argc>4 )
	error( c, "Wrong number of arguments to Rotate");
    if ( c->a.vals[1].type!=v_int || (c->a.argc==4 &&
	    (c->a.vals[2].type!=v_int || c->a.vals[3].type!=v_int )))
	error(c,"Bad argument type in Rotate");
    if ( (c->a.vals[1].u.ival %= 360)<0 ) c->a.vals[1].u.ival += 360;
    a = c->a.vals[1].u.ival *3.1415926535897932/180.;
    trans[0] = trans[3] = cos(a);
    trans[2] = -(trans[1] = sin(a));
    trans[4] = trans[5] = 0;
    if ( c->a.argc==4 ) {
	trans[4] = c->a.vals[2].u.ival-(trans[0]*c->a.vals[2].u.ival+trans[2]*c->a.vals[3].u.ival);
	trans[5] = c->a.vals[3].u.ival-(trans[1]*c->a.vals[2].u.ival+trans[3]*c->a.vals[3].u.ival);
	otype = 0;
    }
    bvts[0].func = bvt_none;
    if ( c->a.vals[1].u.ival==90 )
	bvts[0].func = bvt_rotate90ccw;
    else if ( c->a.vals[1].u.ival==180 )
	bvts[0].func = bvt_rotate180;
    else if ( c->a.vals[1].u.ival==270 )
	bvts[0].func = bvt_rotate90cw;
    bvts[1].func = bvt_none;
    FVTransFunc(c->curfv,trans,otype,bvts,true);
}

static void bScale(Context *c) {
    real trans[6];
    int otype = 1;
    BVTFunc bvts[2];
    double xfact, yfact;
    int i;
    /* Arguments:
	1 => same scale factor both directions, origin at center
	2 => different scale factors for each direction, origin at center
	    (scale factors are per cents)
	3 => same scale factor both directions, origin last two args
	4 => different scale factors for each direction, origin last two args
    */

    if ( c->a.argc==1 || c->a.argc>5 )
	error( c, "Wrong number of arguments to Scale");
    for ( i=1; i<c->a.argc; ++i )
	if ( c->a.vals[1].type!=v_int )
	    error(c,"Bad argument type in Scale");
    i = 1;
    if ( c->a.argc&1 ) {
	xfact = c->a.vals[1].u.ival/100.;
	yfact = c->a.vals[2].u.ival/100.;
	i = 3;
    } else {
	xfact = yfact = c->a.vals[1].u.ival/100.;
	i = 2;
    }
    trans[0] = xfact; trans[3] = yfact;
    trans[2] = trans[1] = 0;
    trans[4] = trans[5] = 0;
    if ( c->a.argc>i ) {
	trans[4] = c->a.vals[i].u.ival-(trans[0]*c->a.vals[i].u.ival);
	trans[5] = c->a.vals[i+1].u.ival-(trans[3]*c->a.vals[i+1].u.ival);
	otype = 0;
    }
    bvts[0].func = bvt_none;
    FVTransFunc(c->curfv,trans,otype,bvts,true);
}

static void bSkew(Context *c) {
    real trans[6];
    int otype = 1;
    BVTFunc bvts[2];
    double a;

    if ( c->a.argc==1 || c->a.argc==3 || c->a.argc>4 )
	error( c, "Wrong number of arguments to Skew");
    if ( c->a.vals[1].type!=v_int || (c->a.argc==4 &&
	    (c->a.vals[2].type!=v_int || c->a.vals[3].type!=v_int )))
	error(c,"Bad argument type in Skew");
    if ( (c->a.vals[1].u.ival %= 360)<0 ) c->a.vals[1].u.ival += 360;
    a = c->a.vals[1].u.ival *3.1415926535897932/180.;
    trans[0] = trans[3] = 1;
    trans[1] = 0; trans[2] = tan(a);
    trans[4] = trans[5] = 0;
    if ( c->a.argc==4 ) {
	trans[4] = c->a.vals[2].u.ival-(trans[0]*c->a.vals[2].u.ival+trans[2]*c->a.vals[3].u.ival);
	trans[5] = c->a.vals[3].u.ival-(trans[1]*c->a.vals[2].u.ival+trans[3]*c->a.vals[3].u.ival);
	otype = 0;
    }
    skewselect(&bvts[0],trans[2]);
    bvts[1].func = bvt_none;
    FVTransFunc(c->curfv,trans,otype,bvts,true);
}

static void bMove(Context *c) {
    real trans[6];
    int otype = 1;
    BVTFunc bvts[2];

    if ( c->a.argc!=3 )
	error( c, "Wrong number of arguments to Move");
    if ( c->a.vals[1].type!=v_int || c->a.vals[2].type!=v_int )
	error(c,"Bad argument type in Move");
    trans[0] = trans[3] = 1;
    trans[1] = trans[2] = 0;
    trans[4] = c->a.vals[1].u.ival; trans[5] = c->a.vals[2].u.ival;
    bvts[0].func = bvt_transmove;
    bvts[0].x = trans[4]; bvts[0].y = trans[5];
    bvts[1].func = bvt_none;
    FVTransFunc(c->curfv,trans,otype,bvts,true);
}

static void bExpandStroke(Context *c) {
    StrokeInfo si;
    /* Arguments:
	1 => stroke width (implied butt, round)
	2 => stroke width, caligraphic angle
	3 => stroke width, line cap, line join
    */

    if ( c->a.argc==1 || c->a.argc>4 )
	error( c, "Wrong number of arguments to ExpandStroke");
    if ( c->a.vals[1].type!=v_int ||
	    (c->a.argc>=3 && c->a.vals[2].type!=v_int ) ||
	    (c->a.argc==4 && c->a.vals[3].type!=v_int ))
	error(c,"Bad argument type in ExpandStroke");
    memset(&si,0,sizeof(si));
    si.radius = c->a.vals[1].u.ival/2.;
    if ( c->a.argc==2 ) {
	si.join = lj_round;
	si.cap = lc_butt;
    } else if ( c->a.argc==4 ) {
	si.cap = c->a.vals[2].u.ival;
	si.join = c->a.vals[3].u.ival;
    } else {
	si.caligraphic = true;
	si.penangle = c->a.vals[2].u.ival;
    }
    FVStrokeItScript(c->curfv, &si);
}

static void bRemoveOverlap(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to RemoveOverlap");
    FVFakeMenus(c->curfv,100);
}

static void bSimplify(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to Simplify");
    FVFakeMenus(c->curfv,101);
}

static void bAddExtrema(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to AddExtrema");
    FVFakeMenus(c->curfv,102);
}

static void bRoundToInt(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to RoundToInt");
    FVFakeMenus(c->curfv,103);
}

static void bAutotrace(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to Autotrace");
    FVAutoTrace(c->curfv,false);
}

static void bCorrectDirection(Context *c) {
    int i;
    SplineFont *sf = c->curfv->sf;
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to CorrectDirection");
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && c->curfv->selected[i] ) {
	SplineChar *sc = sf->chars[i];
	sc->splines = SplineSetsCorrect(sc->splines);
    }
}

static void bBuildComposit(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to BuildComposit");
    FVBuildAccent(c->curfv,false);
}

static void bBuildAccented(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to BuildAccented");
    FVBuildAccent(c->curfv,true);
}

static void bMergeFonts(Context *c) {
    SplineFont *sf;

    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments to MergeFonts");
    if ( c->a.vals[1].type!=v_str )
	error(c,"Bad argument type in MergeFonts");
    sf = LoadSplineFont(c->a.vals[1].u.sval);
    if ( sf==NULL )
	errors(c,"Can't find font", c->a.vals[1].u.sval);
    MergeFont(c->curfv,sf);
    if ( sf->fv==NULL )
	SplineFontFree(sf);
}

static void bAutoHint(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to AutoHint");
    FVFakeMenus(c->curfv,200);
}

static void bSetWidth(Context *c) {
    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments to SetWidth");
    if ( c->a.vals[1].type!=v_int )
	error(c,"Bad argument type in SetWidth");
    FVSetWidthScript(c->curfv,wt_width,c->a.vals[1].u.ival);
}

static void bSetVWidth(Context *c) {
    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments to SetVWidth");
    if ( c->a.vals[1].type!=v_int )
	error(c,"Bad argument type in SetVWidth");
    FVSetWidthScript(c->curfv,wt_vwidth,c->a.vals[1].u.ival);
}

static void bSetLBearing(Context *c) {
    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments to SetLBearing");
    if ( c->a.vals[1].type!=v_int )
	error(c,"Bad argument type in SetLBearing");
    FVSetWidthScript(c->curfv,wt_lbearing,c->a.vals[1].u.ival);
}

static void bSetRBearing(Context *c) {
    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments to SetRBearing");
    if ( c->a.vals[1].type!=v_int )
	error(c,"Bad argument type in SetRBearing");
    FVSetWidthScript(c->curfv,wt_rbearing,c->a.vals[1].u.ival);
}

static void bSetKern(Context *c) {
    SplineFont *sf = c->curfv->sf;
    SplineChar *sc1, *sc2;
    int i, kern, ch2;
    KernPair *kp;

    if ( c->a.argc!=3 )
	error( c, "Wrong number of arguments" );
    ch2 = ParseCharIdent(c,&c->a.vals[1]);
    if ( c->a.vals[2].type!=v_int )
	error(c,"Bad argument type");
    kern = c->a.vals[2].u.ival;
    if ( kern!=0 )
	sc2 = SFMakeChar(sf,ch2);
    else {
	sc2 = sf->chars[ch2];
	if ( sc2==NULL )
return;		/* It already has a kern==0 with everything */
    }

    for ( i=0; i<sf->charcnt; ++i ) if ( c->curfv->selected[i] ) {
	if ( kern!=0 )
	    sc1 = SFMakeChar(sf,i);
	else {
	    if ( (sc1 = sf->chars[i])==NULL )
    continue;
	}
	for ( kp = sc1->kerns; kp!=NULL && kp->sc!=sc2; kp = kp->next );
	if ( kp==NULL && kern==0 )
    continue;
	else if ( kp!=NULL )
	    kp->off = kern;
	else {
	    kp = gcalloc(1,sizeof(KernPair));
	    kp->next = sc1->kerns;
	    sc1->kerns = kp;
	    kp->sc = sc2;
	    kp->off = kern;
	}
    }
}

/* **** Info routines **** */

static void bCharCnt(Context *c) {
    if ( c->a.argc!=1 )
	error( c, "Wrong number of arguments to CharCnt");
    c->return_val.type = v_int;
    c->return_val.u.ival = c->curfv->sf->charcnt;
}

static void bInFont(Context *c) {
    SplineFont *sf = c->curfv->sf;
    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments to InFont");
    c->return_val.type = v_int;
    if ( c->a.vals[1].type==v_int )
	c->return_val.u.ival = c->a.vals[1].u.ival>=0 && c->a.vals[1].u.ival<sf->charcnt;
    else if ( c->a.vals[1].type==v_unicode || c->a.vals[1].type==v_str ) {
	int enc, uni = -1;
	char *str = NULL;
	if ( c->a.vals[1].type==v_unicode )
	    uni = c->a.vals[1].u.ival;
	else {
	    str = c->a.vals[1].u.sval;
	    if ( sscanf(str,"uni%x", &uni)==0 )
		sscanf( str, "U+%x", &uni);
	}
	enc = SFFindChar(sf,uni,str);
	c->return_val.u.ival = (enc!=-1);
    } else
	error( c, "Bad type of argument to InFont");
}

static void bWorthOutputting(Context *c) {
    SplineFont *sf = c->curfv->sf;
    if ( c->a.argc!=2 )
	error( c, "Wrong number of arguments to InFont");
    c->return_val.type = v_int;
    if ( c->a.vals[1].type==v_int )
	c->return_val.u.ival = c->a.vals[1].u.ival>=0 &&
		c->a.vals[1].u.ival<sf->charcnt &&
		SCWorthOutputting(sf->chars[c->a.vals[1].u.ival]);
    else if ( c->a.vals[1].type==v_unicode || c->a.vals[1].type==v_str ) {
	int enc, uni = -1;
	char *str = NULL;
	if ( c->a.vals[1].type==v_unicode )
	    uni = c->a.vals[1].u.ival;
	else {
	    str = c->a.vals[1].u.sval;
	    if ( sscanf(str,"uni%x", &uni)==0 )
		sscanf( str, "U+%x", &uni);
	}
	enc = SFFindChar(sf,uni,str);
	c->return_val.u.ival = enc!=-1 && SCWorthOutputting(sf->chars[enc]);
    } else
	error( c, "Bad type of argument to InFont");
}

static void bCharInfo(Context *c) {
    SplineFont *sf = c->curfv->sf;
    SplineChar *sc;
    DBounds b;

    if ( c->a.argc!=2 && c->a.argc!=3 )
	error( c, "Wrong number of arguments");
    else if ( c->a.vals[1].type!=v_str )
	error( c, "Bad type for argument");

    sc = GetOneSelChar(c);

    c->return_val.type = v_int;
    if ( c->a.argc==3 ) {
	int ch2 = ParseCharIdent(c,&c->a.vals[2]);
	if ( strmatch( c->a.vals[1].u.sval,"Kern")==0 ) {
	    c->return_val.u.ival = 0;
	    if ( sf->chars[ch2]!=NULL ) { KernPair *kp;
		for ( kp = sc->kerns; kp!=NULL && kp->sc!=sf->chars[ch2]; kp=kp->next );
		if ( kp!=NULL )
		    c->return_val.u.ival = kp->off;
	    }
	} else
	    errors(c,"Unknown tag", c->a.vals[1].u.sval);
    } else {
	if ( strmatch( c->a.vals[1].u.sval,"Name")==0 ) {
	    c->return_val.type = v_str;
	    c->return_val.u.sval = copy(sc->name);
	} else if ( strmatch( c->a.vals[1].u.sval,"Unicode")==0 )
	    c->return_val.u.ival = sc->unicodeenc;
	else if ( strmatch( c->a.vals[1].u.sval,"Encoding")==0 )
	    c->return_val.u.ival = sc->enc;
	else if ( strmatch( c->a.vals[1].u.sval,"Width")==0 )
	    c->return_val.u.ival = sc->width;
	else if ( strmatch( c->a.vals[1].u.sval,"VWidth")==0 )
	    c->return_val.u.ival = sc->vwidth;
	else {
	    SplineCharFindBounds(sc,&b);
	    if ( strmatch( c->a.vals[1].u.sval,"LBearing")==0 )
		c->return_val.u.ival = b.minx;
	    else if ( strmatch( c->a.vals[1].u.sval,"RBearing")==0 )
		c->return_val.u.ival = sc->width-b.maxx;
	    else
		errors(c,"Unknown tag", c->a.vals[1].u.sval);
	}
    }
}
    
struct builtins { char *name; void (*func)(Context *); int nofontok; } builtins[] = {
/* Generic utilities */
    { "Print", bPrint, 1 },
    { "Array", bArray, 1 },
    { "Strsub", bStrsub, 1 },
    { "Strlen", bStrlen, 1 },
    { "Strstr", bStrstr, 1 },
    { "Strrstr", bStrrstr, 1 },
    { "Strcasestr", bStrcasestr, 1 },
    { "Strcasecmp", bStrcasecmp, 1 },
/* File menu */
    { "Quit", bQuit, 1 },
    { "Open", bOpen, 1 },
    { "New", bNew, 1 },
    { "Close", bClose },
    { "Save", bSave },
    { "Generate", bGenerate },
    { "Import", bImport },
/* Edit Menu */
    { "Cut", bCut },
    { "Copy", bCopy },
    { "CopyReference", bCopyReference },
    { "CopyWidth", bCopyWidth },
    { "Paste", bPaste },
    { "PasteInto", bPasteInto },
    { "Clear", bClear },
    { "ClearBackground", bClearBackground },
    { "CopyFgToBg", bCopyFgToBg },
    { "UnlinkReference", bUnlinkReference },
    { "SelectAll", bSelectAll },
    { "SelectNone", bSelectNone },
    { "SelectMore", bSelectMore },
    { "Select", bSelect },
/* Element Menu */
    { "Reencode", bReencode },
    { "SetCharCnt", bSetCharCnt },
    { "SetCharName", bSetCharName },
    { "SetUnicodeValue", bSetUnicodeValue },
    { "Transform", bTransform },
    { "HFlip", bHFlip },
    { "VFlip", bVFlip },
    { "Rotate", bRotate },
    { "Scale", bScale },
    { "Skew", bSkew },
    { "Move", bMove },
    { "ExpandStroke", bExpandStroke },
    { "RemoveOverlap", bRemoveOverlap },
    { "Simplify", bSimplify },
    { "AddExtrema", bAddExtrema },
    { "RoundToInt", bRoundToInt },
    { "Autotrace", bAutotrace },
    { "CorrectDirection", bCorrectDirection },
    { "BuildComposit", bBuildComposit },
    { "BuildAccented", bBuildAccented },
    { "MergeFonts", bMergeFonts },
/*  Menu */
    { "AutoHint", bAutoHint },
    { "SetWidth", bSetWidth },
    { "SetVWidth", bSetVWidth },
    { "SetLBearing", bSetLBearing },
    { "SetRBearing", bSetRBearing },
    { "SetKern", bSetKern },
/* ***** */
    { "CharCnt", bCharCnt },
    { "InFont", bInFont },
    { "WorthOutputting", bWorthOutputting },
    { "CharInfo", bCharInfo },
    { NULL }
};

/* ******************************* Interpreter ****************************** */

static void expr(Context*,Val *val);
static void statement(Context*);

static int cgetc(Context *c) {
    int ch = getc(c->script);
    if ( ch=='\r' ) {
	int nch = getc(c->script);
	if ( nch!='\n' )
	    ungetc(nch,c->script);
	++c->lineno;
    } else if ( ch=='\n' )
	++c->lineno;
return( ch );
}

static enum token_type NextToken(Context *c) {
    int ch;
    enum token_type tok = tt_error;

    if ( c->backedup ) {
	c->backedup = false;
return( c->tok );
    }
    do {
	ch = cgetc(c);
	if ( isalpha(ch) || ch=='$' || ch=='_' ) {
	    char *pt = c->tok_text, *end = c->tok_text+TOK_MAX;
	    int toolong = false;
	    while ( (isalnum(ch) || ch=='$' || ch=='_' ) && pt<end ) {
		*pt++ = ch;
		ch = getc(c->script);
	    }
	    *pt = '\0';
	    while ( isalnum(ch) || ch=='$' || ch=='_' ) {
		ch = getc(c->script);
		toolong = true;
	    }
	    ungetc(ch,c->script);
	    tok = tt_name;
	    if ( toolong )
		error( c, "Name too long" );
	    else {
		int i;
		for ( i=0; keywords[i].name!=NULL; ++i )
		    if ( strcmp(keywords[i].name,c->tok_text)==0 ) {
			tok = keywords[i].tok;
		break;
		    }
	    }
	} else if ( isdigit(ch) ) {
	    int val=0;
	    tok = tt_number;
	    if ( ch!='0' ) {
		while ( isdigit(ch)) {
		    val = 10*val+(ch-'0');
		    ch = getc(c->script);
		}
	    } else if ( isdigit(ch=getc(c->script)) ) {
		while ( isdigit(ch) && ch<'8' ) {
		    val = 8*val+(ch-'0');
		    ch = getc(c->script);
		}
	    } else if ( ch=='X' || ch=='x' || ch=='u' || ch=='U' ) {
		if ( ch=='u' || ch=='U' ) tok = tt_unicode;
		ch = getc(c->script);
		while ( isdigit(ch) || (ch>='a' && ch<='f') || (ch>='A'&&ch<='F')) {
		    if ( isdigit(ch))
			ch -= '0';
		    else if ( ch>='a' && ch<='f' )
			ch += 10-'a';
		    else
			ch += 10-'A';
		    val = 16*val+ch;
		    ch = getc(c->script);
		}
	    }
	    ungetc(ch,c->script);
	    c->tok_val.u.ival = val;
	    c->tok_val.type = tok==tt_number ? v_int : v_unicode;
	} else if ( ch=='\'' || ch=='"' ) {
	    int quote = ch;
	    char *pt = c->tok_text, *end = c->tok_text+TOK_MAX;
	    int toolong = false;
	    ch = getc(c->script);
	    while ( ch!=EOF && ch!='\r' && ch!='\n' && ch!=quote ) {
		if ( ch=='\\' ) {
		    ch=getc(c->script);
		    if ( ch=='\n' || ch=='\r' ) {
			ungetc(ch,c->script);
			ch = '\\';
		    } else if ( ch==EOF )
			ch = '\\';
		}
		if ( pt<end )
		    *pt++ = ch;
		else
		    toolong = true;
		ch = getc(c->script);
	    }
	    *pt = '\0';
	    if ( ch=='\n' || ch=='\r' )
		ungetc(ch,c->script);
	    tok = tt_string;
	    if ( toolong )
		error( c, "String too long" );
	} else switch( ch ) {
	  case EOF:
	    tok = tt_eof;
	  break;
	  case ' ': case '\t':
	    /* Ignore spaces */
	  break;
	  case '#':
	    /* Ignore comments */
	    while ( (ch=getc(c->script))!=EOF && ch!='\r' && ch!='\n' );
	    if ( ch=='\r' || ch=='\n' )
		ungetc(ch,c->script);
	  break;
	  case '(':
	    tok = tt_lparen;
	  break;
	  case ')':
	    tok = tt_rparen;
	  break;
	  case '[':
	    tok = tt_lbracket;
	  break;
	  case ']':
	    tok = tt_rbracket;
	  break;
	  case ',':
	    tok = tt_comma;
	  break;
	  case ':':
	    tok = tt_colon;
	  break;
	  case ';': case '\n': case '\r':
	    tok = tt_eos;
	  break;
	  case '-':
	    tok = tt_minus;
	    ch=getc(c->script);
	    if ( ch=='=' )
		tok = tt_minuseq;
	    else if ( ch=='-' )
		tok = tt_decr;
	    else
		ungetc(ch,c->script);
	  break;
	  case '+':
	    tok = tt_plus;
	    ch=getc(c->script);
	    if ( ch=='=' )
		tok = tt_pluseq;
	    else if ( ch=='+' )
		tok = tt_incr;
	    else
		ungetc(ch,c->script);
	  break;
	  case '!':
	    tok = tt_not;
	    ch=getc(c->script);
	    if ( ch=='=' )
		tok = tt_ne;
	    else
		ungetc(ch,c->script);
	  break;
	  case '~':
	    tok = tt_bitnot;
	  break;
	  case '*':
	    tok = tt_mul;
	    ch=getc(c->script);
	    if ( ch=='=' )
		tok = tt_muleq;
	    else
		ungetc(ch,c->script);
	  break;
	  case '%':
	    tok = tt_mod;
	    ch=getc(c->script);
	    if ( ch=='=' )
		tok = tt_modeq;
	    else
		ungetc(ch,c->script);
	  break;
	  case '/':
	    ch=getc(c->script);
	    if ( ch=='/' ) {
		/* another comment to eol */;
		while ( (ch=getc(c->script))!=EOF && ch!='\r' && ch!='\n' );
		if ( ch=='\r' || ch=='\n' )
		    ungetc(ch,c->script);
	    } else if ( ch=='*' ) {
		int found=false;
		ch = cgetc(c);
		while ( !found || ch!='/' ) {
		    if ( ch==EOF )
		break;
		    if ( ch=='*' ) found = true;
		    else found = false;
		    ch = cgetc(c);
		}
	    } else if ( ch=='=' ) {
		tok = tt_diveq;
	    } else {
		tok = tt_div;
		ungetc(ch,c->script);
	    }
	  break;
	  case '&':
	    tok = tt_bitand;
	    ch=getc(c->script);
	    if ( ch=='&' )
		tok = tt_and;
	    else
		ungetc(ch,c->script);
	  break;
	  case '|':
	    tok = tt_bitor;
	    ch=getc(c->script);
	    if ( ch=='|' )
		tok = tt_or;
	    else
		ungetc(ch,c->script);
	  break;
	  case '^':
	    tok = tt_xor;
	  break;
	  case '=':
	    tok = tt_assign;
	    ch=getc(c->script);
	    if ( ch=='=' )
		tok = tt_eq;
	    else
		ungetc(ch,c->script);
	  break;
	  case '>':
	    tok = tt_gt;
	    ch=getc(c->script);
	    if ( ch=='=' )
		tok = tt_ge;
	    else
		ungetc(ch,c->script);
	  break;
	  case '<':
	    tok = tt_lt;
	    ch=getc(c->script);
	    if ( ch=='=' )
		tok = tt_le;
	    else
		ungetc(ch,c->script);
	  break;
	  default:
	    fprintf( stderr, "%s:%d Unexpected character %c (%d)\n",
		    c->filename, c->lineno, ch, ch);
	    traceback(c);
	}
    } while ( tok==tt_error );

    c->tok = tok;
return( tok );
}

static void backuptok(Context *c) {
    if ( c->backedup )
	fprintf( stderr, "%s:%d Internal Error: Attempt to back token twice\n",
		c->filename, c->lineno );
    c->backedup = true;
}

#define PE_ARG_MAX	20

static void docall(Context *c,char *name,Val *val) {
    /* Be prepared for c->donteval */
    Val args[PE_ARG_MAX];
    Array *dontfree[PE_ARG_MAX];
    int i;
    enum token_type tok;
    Context sub;

    tok = NextToken(c);
    dontfree[0] = NULL;
    if ( tok==tt_rparen )
	i = 1;
    else {
	backuptok(c);
	for ( i=1; tok!=tt_rparen; ++i ) {
	    if ( i>=PE_ARG_MAX )
		error(c,"Too many arguments");
	    expr(c,&args[i]);
	    tok = NextToken(c);
	    if ( tok!=tt_comma )
		expect(c,tt_rparen,tok);
	    dontfree[i]=NULL;
	}
    }

    if ( !c->donteval ) {
	args[0].type = v_str;
	args[0].u.sval = name;
	memset( &sub,0,sizeof(sub));
	sub.caller = c;
	sub.a.vals = args;
	sub.a.argc = i;
	sub.return_val.type = v_void;
	sub.filename = name;
	sub.curfv = c->curfv;
	sub.trace = c->trace;
	sub.dontfree = dontfree;
	for ( i=0; i<sub.a.argc; ++i ) {
	    dereflvalif(&args[i]);
	    if ( args[i].type == v_arrfree )
		args[i].type = v_arr;
	    else if ( args[i].type == v_arr )
		dontfree[i] = args[i].u.aval;
	}

	if ( c->trace.u.ival ) {
	    printf( "%s:%d Calling %s(", GFileNameTail(c->filename), c->lineno,
		    name );
	    for ( i=1; i<sub.a.argc; ++i ) {
		if ( i!=1 ) putchar(',');
		if ( args[i].type == v_int )
		    printf( "%d", args[i].u.ival );
		else if ( args[i].type == v_unicode )
		    printf( "0u%x", args[i].u.ival );
		else if ( args[i].type == v_str )
		    printf( "\"%s\"", args[i].u.sval );
		else if ( args[i].type == v_void )
		    printf( "<void>");
		else
		    printf( "<???>");
	    }
	    printf(")\n");
	}

	for ( i=0; builtins[i].name!=NULL; ++i )
	    if ( strcmp(builtins[i].name,name)==0 )
	break;
	if ( builtins[i].name!=NULL ) {
	    if ( sub.curfv==NULL && !builtins[i].nofontok )
		error(&sub,"This command requires an active font");
	    (builtins[i].func)(&sub);
	} else {
	    if ( strchr(name,'/')==NULL && strchr(c->filename,'/')!=NULL ) {
		char *pt;
		sub.filename = galloc(strlen(c->filename)+strlen(name)+1);
		strcpy(sub.filename,c->filename);
		pt = strrchr(sub.filename,'/');
		strcpy(pt+1,name);
	    }
	    sub.script = fopen(sub.filename,"r");
	    if ( sub.script==NULL )
		error(&sub, "No such file");
	    else {
		sub.lineno = 1;
		while ( !sub.returned && (tok = NextToken(&sub))!=tt_eof ) {
		    backuptok(&sub);
		    statement(&sub);
		}
		fclose(sub.script); sub.script = NULL;
	    }
	    if ( sub.filename!=name )
		free( sub.filename );
	}
	c->curfv = sub.curfv;
    }
    calldatafree(&sub);
    if ( val->type==v_str )
	free(val->u.sval);
    *val = sub.return_val;
}

static void handlename(Context *c,Val *val) {
    char name[TOK_MAX+1];
    enum token_type tok;
    int temp;
    char *pt;
    SplineFont *sf;

    strcpy(name,c->tok_text);
    val->type = v_void;
    tok = NextToken(c);
    if ( tok==tt_lparen ) {
	docall(c,name,val);
    } else if ( c->donteval ) {
	backuptok(c);
    } else {
	if ( *name=='$' ) {
	    if ( isdigit(name[1])) {
		temp = 0;
		for ( pt = name+1; isdigit(*pt); ++pt )
		    temp = 10*temp+*pt-'0';
		if ( *pt=='\0' && temp<c->a.argc ) {
		    val->type = v_lval;
		    val->u.lval = &c->a.vals[temp];
		}
	    } else if ( strcmp(name,"$argc")==0 || strcmp(name,"$#")==0 ) {
		val->type = v_int;
		val->u.ival = c->a.argc;
	    } else if ( strcmp(name,"$argv")==0 ) {
		c->argsval.type = v_arr;
		c->argsval.u.aval = &c->a;
	    } else if ( strcmp(name,"$curfont")==0 || strcmp(name,"$nextfont")==0 ||
		    strcmp(name,"$firstfont")==0 ) {
		if ( strcmp(name,"$firstfont")==0 ) {
		    if ( fv_list==NULL ) error(c,"No fonts loaded");
		    sf = fv_list->sf;
		} else {
		    if ( c->curfv==NULL ) error(c,"No current font");
		    if ( strcmp(name,"$curfont")==0 ) 
			sf = c->curfv->sf;
		    else {
			if ( c->curfv->next==NULL ) sf = NULL;
			else sf = c->curfv->next->sf;
		    }
		}
		val->type = v_str;
		val->u.sval = copy(sf==NULL?"":
			sf->filename!=NULL?sf->filename:sf->origname);
	    } else if ( strcmp(name,"$fontname")==0 || strcmp(name,"familyname")==0 ||
		    strcmp(name,"$fullname")==0 ) {
		if ( c->curfv==NULL ) error(c,"No current font");
		val->type = v_str;
		val->u.sval = copy(name[2]=='o'?c->curfv->sf->fontname:
			name[2]=='a'?c->curfv->sf->familyname:
				    c->curfv->sf->fullname);
	    } else if ( strcmp(name,"$trace")==0 ) {
		val->type = v_lval;
		val->u.lval = &c->trace;
	    }
	} else if ( c->locals!=NULL ) {
	    for ( temp=0; temp<c->lc; ++temp )
		if ( strcmp(c->locals[temp].name,name)==0 ) {
		    val->type = v_lval;
		    val->u.lval = &c->locals[temp].val;
		}
	}
	if ( tok==tt_assign && val->type==v_void && *name!='$' ) {
	    /* It's ok to create this as a new variable, we're going to assign to it */
	    if ( c->locals==NULL ) {
		c->lmax = 10;
		c->locals = galloc(c->lmax*sizeof(struct dictentry));
	    } else if ( c->lc>=c->lmax ) {
		c->lmax += 10;
		c->locals = grealloc(c->locals,c->lmax*sizeof(struct dictentry));
	    }
	    c->locals[c->lc].name = copy(name);
	    c->locals[c->lc].val.type = v_void;
	    val->type = v_lval;
	    val->u.lval = &c->locals[c->lc].val;
	    ++c->lc;
	}
	if ( val->type==v_void )
	    errors(c, "Undefined variable", name);
	backuptok(c);
    }
}

static void term(Context *c,Val *val) {
    enum token_type tok = NextToken(c);
    Val temp;

    if ( tok==tt_lparen ) {
	expr(c,val);
	tok = NextToken(c);
	expect(c,tt_rparen,tok);
    } else if ( tok==tt_number || tok==tt_unicode ) {
	*val = c->tok_val;
    } else if ( tok==tt_string ) {
	val->type = v_str;
	val->u.sval = copy( c->tok_text );
    } else if ( tok==tt_name ) {
	handlename(c,val);
    } else if ( tok==tt_minus || tok==tt_plus || tok==tt_not || tok==tt_bitnot ) {
	term(c,val);
	if ( !c->donteval ) {
	    dereflvalif(val);
	    if ( val->type!=v_int )
		error( c, "Invalid type in integer expression" );
	    if ( tok==tt_minus )
		val->u.ival = -val->u.ival;
	    else if ( tok==tt_not )
		val->u.ival = !val->u.ival;
	    else if ( tok==tt_bitnot )
		val->u.ival = ~val->u.ival;
	}
    } else if ( tok==tt_incr || tok==tt_decr ) {
	term(c,val);
	if ( !c->donteval ) {
	    if ( val->type!=v_lval )
		error( c, "Expected lvalue" );
	    if ( val->u.lval->type!=v_int && val->u.lval->type!=v_unicode )
		error( c, "Invalid type in integer expression" );
	    if ( tok == tt_incr )
		++val->u.lval->u.ival;
	    else
		--val->u.lval->u.ival;
	    dereflvalif(val);
	}
    } else
	expect(c,tt_name,tok);

    tok = NextToken(c);
    while ( tok==tt_incr || tok==tt_decr || tok==tt_colon || tok==tt_lparen || tok==tt_lbracket ) {
	if ( tok==tt_colon ) {
	    if ( c->donteval ) {
		tok = NextToken(c);
		expect(c,tt_name,tok);
	    } else {
		dereflvalif(val);
		if ( val->type!=v_str )
		    error( c, "Invalid type in string expression" );
		else {
		    char *pt, *ept;
		    tok = NextToken(c);
		    expect(c,tt_name,tok);
		    if ( strcmp(c->tok_text,"h")==0 ) {
			pt = strrchr(val->u.sval,'/');
			if ( pt!=NULL ) *pt = '\0';
		    } else if ( strcmp(c->tok_text,"t")==0 ) {
			pt = strrchr(val->u.sval,'/');
			if ( pt!=NULL ) {
			    char *ret = copy(pt+1);
			    free(val->u.sval);
			    val->u.sval = ret;
			}
		    } else if ( strcmp(c->tok_text,"r")==0 ) {
			pt = strrchr(val->u.sval,'/');
			if ( pt==NULL ) pt=val->u.sval;
			ept = strrchr(pt,'.');
			if ( ept!=NULL ) *ept = '\0';
		    } else if ( strcmp(c->tok_text,"e")==0 ) {
			pt = strrchr(val->u.sval,'/');
			if ( pt==NULL ) pt=val->u.sval;
			ept = strrchr(pt,'.');
			if ( ept!=NULL ) {
			    char *ret = copy(ept+1);
			    free(val->u.sval);
			    val->u.sval = ret;
			}
		    } else
			errors(c,"Unknown colon substitution", c->tok_text );
		}
	    }
	} else if ( tok==tt_lparen ) {
	    if ( c->donteval ) {
		docall(c,NULL,val);
	    } else {
		dereflvalif(val);
		if ( val->type!=v_str ) {
		    error(c,"Expected string to hold filename in procedure call");
		} else
		    docall(c,val->u.sval,val);
	    }
	} else if ( tok==tt_lbracket ) {
	    expr(c,&temp);
	    tok = NextToken(c);
	    expect(c,tt_rbracket,tok);
	    if ( !c->donteval ) {
		dereflvalif(&temp);
		if ( val->type==v_lval && (val->u.lval->type==v_arr ||val->u.lval->type==v_arrfree))
		    *val = *val->u.lval;
		if ( val->type!=v_arr && val->type!=v_arrfree )
		    error(c,"Array required");
		if (temp.type!=v_int )
		    error(c,"Integer expression required in array");
		else if ( temp.u.ival<0 || temp.u.ival>=val->u.aval->argc )
		    error(c,"Integer expression out of bounds in array");
		else if ( val->type==v_arrfree ) {
		    temp = val->u.aval->vals[temp.u.ival];
		    arrayfree(val->u.aval);
		    *val = temp;
		} else {
		    val->type = v_lval;
		    val->u.lval = &val->u.aval->vals[temp.u.ival];
		}
	    }
	} else if ( !c->donteval ) {
	    Val temp;
	    if ( val->type!=v_lval )
		error( c, "Expected lvalue" );
	    if ( val->u.lval->type!=v_int && val->u.lval->type!=v_unicode )
		error( c, "Invalid type in integer expression" );
	    temp = *val->u.lval;
	    if ( tok == tt_incr )
		++val->u.lval->u.ival;
	    else
		--val->u.lval->u.ival;
	    *val = temp;
	}
	tok = NextToken(c);
    }
    backuptok(c);
}

static void mul(Context *c,Val *val) {
    Val other;
    enum token_type tok;

    term(c,val);
    tok = NextToken(c);
    while ( tok==tt_mul || tok==tt_div || tok==tt_mod ) {
	other.type = v_void;
	term(c,&other);
	if ( !c->donteval ) {
	    dereflvalif(val);
	    dereflvalif(&other);
	    if ( val->type!=v_int || other.type!=v_int )
		error( c, "Invalid type in integer expression" );
	    else if ( (tok==tt_div || tok==tt_mod ) && other.u.ival==0 )
		error( c, "Division by zero" );
	    else if ( tok==tt_mul )
		val->u.ival *= other.u.ival;
	    else if ( tok==tt_mod )
		val->u.ival %= other.u.ival;
	    else
		val->u.ival /= other.u.ival;
	}
	tok = NextToken(c);
    }
    backuptok(c);
}

static void add(Context *c,Val *val) {
    Val other;
    enum token_type tok;

    mul(c,val);
    tok = NextToken(c);
    while ( tok==tt_plus || tok==tt_minus ) {
	other.type = v_void;
	mul(c,&other);
	if ( !c->donteval ) {
	    dereflvalif(val);
	    dereflvalif(&other);
	    if ( val->type==v_str && (other.type==v_str || other.type==v_int) && tok==tt_plus ) {
		char *ret, *temp;
		char buffer[10];
		if ( other.type == v_int ) {
		    sprintf(buffer,"%d", other.u.ival);
		    temp = buffer;
		} else
		    temp = other.u.sval;
		ret = galloc(strlen(val->u.sval)+strlen(temp)+1);
		strcpy(ret,val->u.sval);
		strcat(ret,temp);
		if ( other.type==v_str ) free(other.u.sval);
		free(val->u.sval);
		val->u.sval = ret;
	    } else if ( (val->type!=v_int && val->type!=v_unicode) || (other.type!=v_int&&other.type!=v_unicode) )
		error( c, "Invalid type in integer expression" );
	    else if ( tok==tt_plus )
		val->u.ival += other.u.ival;
	    else
		val->u.ival -= other.u.ival;
	}
	tok = NextToken(c);
    }
    backuptok(c);
}

static void comp(Context *c,Val *val) {
    Val other;
    int cmp;
    enum token_type tok;

    add(c,val);
    tok = NextToken(c);
    while ( tok==tt_eq || tok==tt_ne || tok==tt_gt || tok==tt_lt || tok==tt_ge || tok==tt_le ) {
	other.type = v_void;
	add(c,&other);
	if ( !c->donteval ) {
	    dereflvalif(val);
	    dereflvalif(&other);
	    if ( val->type==v_str && other.type==v_str ) {
		cmp = strcmp(val->u.sval,other.u.sval);
		free(val->u.sval); free(other.u.sval);
	    } else if (( val->type==v_int || val->type==v_unicode ) &&
		    (other.type==v_int || other.type==v_unicode)) {
		cmp = val->u.ival - other.u.ival;
	    } else 
		error( c, "Invalid type in integer expression" );
	    val->type = v_int;
	    if ( tok==tt_eq ) val->u.ival = (cmp==0);
	    else if ( tok==tt_ne ) val->u.ival = (cmp!=0);
	    else if ( tok==tt_gt ) val->u.ival = (cmp>0);
	    else if ( tok==tt_lt ) val->u.ival = (cmp<0);
	    else if ( tok==tt_ge ) val->u.ival = (cmp>=0);
	    else if ( tok==tt_le ) val->u.ival = (cmp<=0);
	}
	tok = NextToken(c);
    }
    backuptok(c);
}

static void _and(Context *c,Val *val) {
    Val other;
    int old = c->donteval;
    enum token_type tok;

    comp(c,val);
    tok = NextToken(c);
    while ( tok==tt_and || tok==tt_bitand ) {
	other.type = v_void;
	if ( !c->donteval )
	    dereflvalif(val);
	if ( tok==tt_and && val->u.ival==0 )
	    c->donteval = true;
	comp(c,&other);
	c->donteval = old;
	if ( !old ) {
	    dereflvalif(&other);
	    if ( tok==tt_and && val->type==v_int && val->u.ival==0 )
		val->u.ival = 0;
	    else if ( val->type!=v_int || other.type!=v_int )
		error( c, "Invalid type in integer expression" );
	    else if ( tok==tt_and )
		val->u.ival = val->u.ival && other.u.ival;
	    else
		val->u.ival &= other.u.ival;
	}
	tok = NextToken(c);
    }
    backuptok(c);
}

static void _or(Context *c,Val *val) {
    Val other;
    int old = c->donteval;
    enum token_type tok;

    _and(c,val);
    tok = NextToken(c);
    while ( tok==tt_or || tok==tt_bitor || tok==tt_xor ) {
	other.type = v_void;
	if ( !c->donteval )
	    dereflvalif(val);
	if ( tok==tt_or && val->u.ival!=0 )
	    c->donteval = true;
	_and(c,&other);
	c->donteval = old;
	if ( !c->donteval ) {
	    dereflvalif(&other);
	    if ( tok==tt_or && val->type==v_int && val->u.ival!=0 )
		val->u.ival = 1;
	    else if ( val->type!=v_int || other.type!=v_int )
		error( c, "Invalid type in integer expression" );
	    else if ( tok==tt_or )
		val->u.ival = val->u.ival || other.u.ival;
	    else if ( tok==tt_bitor )
		val->u.ival |= other.u.ival;
	    else
		val->u.ival ^= other.u.ival;
	}
	tok = NextToken(c);
    }
    backuptok(c);
}

static void assign(Context *c,Val *val) {
    Val other;
    enum token_type tok;

    _or(c,val);
    tok = NextToken(c);
    if ( tok==tt_assign || tok==tt_pluseq || tok==tt_minuseq || tok==tt_muleq || tok==tt_diveq || tok==tt_modeq ) {
	other.type = v_void;
	assign(c,&other);		/* that's the evaluation order here */
	if ( !c->donteval ) {
	    dereflvalif(&other);
	    if ( val->type!=v_lval )
		error( c, "Expected lvalue" );
	    else if ( other.type == v_void )
		error( c, "Void found on right side of assignment" );
	    else if ( tok==tt_assign ) {
		Val temp;
		int argi;
		temp = *val->u.lval;
		*val->u.lval = other;
		if ( other.type==v_arr )
		    val->u.lval->u.aval = arraycopy(other.u.aval);
		else if ( other.type==v_arrfree )
		    val->u.lval->type = v_arr;
		argi = val->u.lval-c->a.vals;
		/* Have to free things after we copy them */
		if ( argi>=0 && argi<c->a.argc && temp.type==v_arr &&
			temp.u.aval==c->dontfree[argi] )
		    c->dontfree[argi] = NULL;		/* Don't free it */
		else if ( temp.type == v_arr )
		    arrayfree(temp.u.aval);
		else if ( temp.type == v_str )
		    free( temp.u.sval);
	    } else if (( val->u.lval->type==v_int || val->u.lval->type==v_unicode ) && (other.type==v_int || other.type==v_unicode)) {
		if ( tok==tt_pluseq ) val->u.lval->u.ival += other.u.ival;
		else if ( tok==tt_minuseq ) val->u.lval->u.ival -= other.u.ival;
		else if ( tok==tt_muleq ) val->u.lval->u.ival *= other.u.ival;
		else if ( other.u.ival==0 )
		    error(c,"Divide by zero");
		else if ( tok==tt_modeq ) val->u.lval->u.ival %= other.u.ival;
		else val->u.lval->u.ival /= other.u.ival;
	    } else if ( tok==tt_pluseq && val->u.lval->type==v_str &&
		    (other.type==v_str || other.type==v_int)) {
		char *ret, *temp;
		char buffer[10];
		if ( other.type == v_int ) {
		    sprintf(buffer,"%d", other.u.ival);
		    temp = buffer;
		} else
		    temp = other.u.sval;
		ret = galloc(strlen(val->u.lval->u.sval)+strlen(temp)+1);
		strcpy(ret,val->u.lval->u.sval);
		strcat(ret,temp);
		if ( other.type==v_str ) free(other.u.sval);
		free(val->u.lval->u.sval);
		val->u.sval = ret;
	    } else
		error( c, "Invalid types in assignment");
	}
    } else
	backuptok(c);
}

static void expr(Context *c,Val *val) {
    val->type = v_void;
    assign(c,val);
}

static void dowhile(Context *c) {
    long here = ftell(c->script);
    int lineno = c->lineno;
    enum token_type tok;
    Val val;
    int nest;

    while ( 1 ) {
	tok=NextToken(c);
	expect(c,tt_lparen,tok);
	val.type = v_void;
	expr(c,&val);
	tok=NextToken(c);
	expect(c,tt_rparen,tok);
	dereflvalif(&val);
	if ( val.type!=v_int )
	    error( c, "Expected integer expression in while condition");
	if ( val.u.ival==0 )
    break;
	while ( (tok=NextToken(c))!=tt_endloop && tok!=tt_eof && !c->returned ) {
	    backuptok(c);
	    statement(c);
	}
	if ( tok==tt_eof )
	    error(c,"End of file found in while loop" );
	fseek(c->script,here,SEEK_SET);
	c->lineno = lineno;
    }

    nest = 0;
    while ( (tok=NextToken(c))!=tt_endloop || nest>0 ) {
	if ( tok==tt_eof )
	    error(c,"End of file found in while loop" );
	else if ( tok==tt_while ) ++nest;
	else if ( tok==tt_endloop ) --nest;
    }
}

static void doif(Context *c) {
    enum token_type tok;
    Val val;
    int nest;

    while ( 1 ) {
	tok=NextToken(c);
	expect(c,tt_lparen,tok);
	val.type = v_void;
	expr(c,&val);
	tok=NextToken(c);
	expect(c,tt_rparen,tok);
	dereflvalif(&val);
	if ( val.type!=v_int )
	    error( c, "Expected integer expression in if condition");
	if ( val.u.ival!=0 ) {
	    while ( (tok=NextToken(c))!=tt_endif && tok!=tt_eof && tok!=tt_else && tok!=tt_elseif && !c->returned ) {
		backuptok(c);
		statement(c);
	    }
	    if ( tok==tt_eof )
		error(c,"End of file found in if statement" );
    break;
	} else {
	    nest = 0;
	    while ( ((tok=NextToken(c))!=tt_endif && tok!=tt_else && tok!=tt_elseif ) || nest>0 ) {
		if ( tok==tt_eof )
		    error(c,"End of file found in if statement" );
		else if ( tok==tt_if ) ++nest;
		else if ( tok==tt_endif ) --nest;
	    }
	    if ( tok==tt_else ) {
		while ( (tok=NextToken(c))!=tt_endif && tok!=tt_eof && !c->returned ) {
		    backuptok(c);
		    statement(c);
		}
    break;
	    } else if ( tok==tt_endif )
    break;
	}
    }
    if ( c->returned )
return;
    if ( tok!=tt_endif && tok!=tt_eof ) {
	nest = 0;
	while ( ((tok=NextToken(c))!=tt_endif && tok!=tt_else && tok!=tt_elseif ) || nest>0 ) {
	    if ( tok==tt_eof )
return;
	    else if ( tok==tt_if ) ++nest;
	    else if ( tok==tt_endif ) --nest;
	}
    }
}

static void doshift(Context *c) {
    int i;

    if ( c->a.argc==1 )
	error(c,"Attempt to shift when there are no arguments left");
    if ( c->a.vals[1].type==v_str )
	free(c->a.vals[1].u.sval );
    if ( c->a.vals[1].type==v_arr && c->a.vals[1].u.aval != c->dontfree[1] )
	arrayfree(c->a.vals[1].u.aval );
    --c->a.argc;
    for ( i=1; i<c->a.argc ; ++i ) {
	c->a.vals[i] = c->a.vals[i+1];
	c->dontfree[i] = c->dontfree[i+1];
    }
}

static void statement(Context *c) {
    enum token_type tok = NextToken(c);
    Val val;

    if ( tok==tt_while )
	dowhile(c);
    else if ( tok==tt_if )
	doif(c);
    else if ( tok==tt_shift )
	doshift(c);
    else if ( tok==tt_else || tok==tt_elseif || tok==tt_endif || tok==tt_endloop ) {
	unexpected(c,tok);
    } else if ( tok==tt_return ) {
	tok = NextToken(c);
	backuptok(c);
	c->returned = true;
	c->return_val.type = v_void;
	if ( tok!=tt_eos ) {
	    expr(c,&c->return_val);
	    dereflvalif(&c->return_val);
	    if ( c->return_val.type==v_arr ) {
		c->return_val.type = v_arrfree;
		c->return_val.u.aval = arraycopy(c->return_val.u.aval);
	    }
	}
    } else if ( tok==tt_eos ) {
	backuptok(c);
    } else {
	backuptok(c);
	expr(c,&val);
	if ( val.type == v_str )
	    free( val.u.sval );
    }
    tok = NextToken(c);
    if ( tok!=tt_eos && tok!=tt_eof && !c->returned )
	error( c, "Unterminated statement" );
}

static void ProcessScript(int argc, char *argv[]) {
    int i,j;
    Context c;
    enum token_type tok;

    i=1;
    if ( strcmp(argv[1],"-script")==0 )
	++i;
    memset( &c,0,sizeof(c));
    c.a.argc = argc-i;
    c.a.vals = galloc(c.a.argc*sizeof(Val));
    c.dontfree = gcalloc(c.a.argc,sizeof(Array*));
    for ( j=i; j<argc; ++j ) {
	c.a.vals[j-i].type = v_str;
	c.a.vals[j-i].u.sval = copy(argv[j]);
    }
    c.filename = argv[i];
    c.return_val.type = v_void;

    c.script = fopen(c.filename,"r");
    if ( c.script==NULL )
	error(&c, "No such file");
    else {
	c.lineno = 1;
	while ( !c.returned && (tok = NextToken(&c))!=tt_eof ) {
	    backuptok(&c);
	    statement(&c);
	}
	fclose(c.script);
    }
    for ( i=0; i<c.a.argc; ++i )
	free(c.a.vals[i].u.sval);
    free(c.a.vals);
    free(c.dontfree);
    exit(0);
}

void CheckIsScript(int argc, char *argv[]) {
    if ( argc==1 )
return;
    if ( strcmp(argv[1],"-script")==0 && argc>2 )
	ProcessScript(argc, argv);
    if ( access(argv[1],X_OK|R_OK)==0 ) {
	FILE *temp = fopen(argv[1],"r");
	char buffer[200];
	if ( temp==NULL )
return;
	buffer[0] = '\0';
	fgets(buffer,sizeof(buffer),temp);
	fclose(temp);
	if ( buffer[0]=='#' && buffer[1]=='!' && strstr(buffer,"pfaedit")!=NULL )
	    ProcessScript(argc, argv);
    }
}

struct sd_data {
    int done;
    FontView *fv;
    GWindow gw;
};

#define SD_Width	250
#define SD_Height	270
#define CID_Script	1001

static int SD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct sd_data *sd = GDrawGetUserData(GGadgetGetWindow(g));
	Context c;
	Val args[1];
	Array *dontfree[1];
	jmp_buf env;
	enum token_type tok;

	memset( &c,0,sizeof(c));
	memset( args,0,sizeof(args));
	memset( dontfree,0,sizeof(dontfree));
	c.a.argc = 1;
	c.a.vals = args;
	c.dontfree = dontfree;
	c.filename = args[0].u.sval = "ScriptDlg";
	args[0].type = v_str;
	c.return_val.type = v_void;
	c.err_env = &env;
	c.curfv = sd->fv;
	if ( setjmp(env)!=0 )
return( true );			/* Error return */

	c.script = tmpfile();
	if ( c.script==NULL )
	    error(&c, "Can't create temporary file");
	else {
	    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(sd->gw,CID_Script));
	    while ( *ret ) {
		putc(*ret,c.script);
		++ret;
	    }
	    rewind(c.script);
	    c.lineno = 1;
	    while ( !c.returned && (tok = NextToken(&c))!=tt_eof ) {
		backuptok(&c);
		statement(&c);
	    }
	    fclose(c.script);
	    sd->done = true;
	}
    }
return( true );
}

static void SD_DoCancel(struct sd_data *sd) {
    sd->done = true;
}

static int SD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SD_DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int sd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	SD_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("scripting.html");
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void ScriptDlg(FontView *fv) {
    GRect pos;
    static GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[10];
    struct sd_data sd;
    FontView *list;

    memset(&sd,0,sizeof(sd));
    sd.fv = fv;

    if ( gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_ExecuteScript,NULL);
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,SD_Width);
	pos.height = GDrawPointsToPixels(NULL,SD_Height);
	gw = GDrawCreateTopWindow(NULL,&pos,sd_e_h,&sd,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 10;
	gcd[0].gd.pos.width = SD_Width-20; gcd[0].gd.pos.height = SD_Height-50;
	gcd[0].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
	gcd[0].gd.cid = CID_Script;
	gcd[0].creator = GTextAreaCreate;

	gcd[1].gd.pos.x = 25-3; gcd[1].gd.pos.y = SD_Height-32-3;
	gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
	gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[1].text = (unichar_t *) _STR_OK;
	label[1].text_in_resource = true;
	gcd[1].gd.mnemonic = 'O';
	gcd[1].gd.label = &label[1];
	gcd[1].gd.handle_controlevent = SD_OK;
	gcd[1].creator = GButtonCreate;

	gcd[2].gd.pos.x = SD_Width-GIntGetResource(_NUM_Buttonsize)-25; gcd[2].gd.pos.y = SD_Height-32;
	gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
	gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[2].text = (unichar_t *) _STR_Cancel;
	label[2].text_in_resource = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.mnemonic = 'C';
	gcd[2].gd.handle_controlevent = SD_Cancel;
	gcd[2].creator = GButtonCreate;

	gcd[3].gd.pos.x = 2; gcd[3].gd.pos.y = 2;
	gcd[3].gd.pos.width = pos.width-4; gcd[3].gd.pos.height = pos.height-4;
	gcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[3].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
    }
    sd.gw = gw;
    GDrawSetUserData(gw,&sd);
    GDrawSetVisible(gw,true);
    while ( !sd.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);

    /* Selection may be out of date, force a refresh */
    for ( list = fv_list; list->next!=NULL; list=list->next )
	GDrawRequestExpose(list->v,NULL,false);
}
