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
#include "autohint.h"
#include "cvimages.h"
#include "cvundoes.h"
#include "fontforge.h"
#include "namelist.h"
#include "splineoverlap.h"
#include <math.h>
#include <locale.h>
#include <ustring.h>
#include <utype.h>
#include "psfont.h"
#include "sd.h"
#include "views.h"		/* For CharViewBase */
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

typedef struct _io {
    const char *macro;
    char *start;
    FILE *ps, *fog;
    char fogbuf[60];
    int backedup, cnt, isloop, isstopped, fogns;
    struct _io *prev;
} _IO;

typedef struct io {
    struct _io *top;
    int endedstopped;
    int advance_width;		/* Can be set from a PS comment by MF2PT1 */
} IO;

#define GARBAGE_MAX	64
struct garbage {
    int cnt;
    struct garbage *next;
    struct pskeyval *entries[GARBAGE_MAX];
    int16 cnts[GARBAGE_MAX];
};

static void AddTok(GrowBuf *gb,char *buf,int islit) {

    if ( islit )
	GrowBufferAdd(gb,'/');
    GrowBufferAddStr(gb,buf);
    GrowBufferAdd(gb,' ');
}

static struct pskeyval *lookup(struct pskeydict *dict,char *tokbuf) {
    int i;

    for ( i=0; i<dict->cnt; ++i )
	if ( strcmp(dict->entries[i].key,tokbuf)==0 )
return( &dict->entries[i] );

return( NULL );
}

static void dictfree(struct pskeydict *dict) {
    int i;

    for ( i=0; i<dict->cnt; ++i ) {
	if ( dict->entries[i].type==ps_string || dict->entries[i].type==ps_instr ||
		dict->entries[i].type==ps_lit )
	    free(dict->entries[i].u.str);
	else if ( dict->entries[i].type==ps_array || dict->entries[i].type==ps_dict )
	    dictfree(&dict->entries[i].u.dict);
    }
}

static void garbagefree(struct garbage *all) {
    struct garbage *junk, *next;
    int i,j;

    for ( junk = all; junk!=NULL; junk = next ) {
	next = junk->next;
	for ( j=0; j<junk->cnt; ++j ) {
	    for ( i=0; i<junk->cnts[j]; ++i ) {
		if ( junk->entries[j][i].type==ps_string || junk->entries[j][i].type==ps_instr ||
			junk->entries[j][i].type==ps_lit )
		    free(junk->entries[j][i].u.str);
	    }
	    free(junk->entries[j]);
	}
	if ( junk!=all )
	    chunkfree(junk,sizeof(struct garbage));
    }
}
/**************************** PostScript Importer *****************************/
/* It's really dumb. It ignores almost everything except linetos and curvetos */
/*  anything else, function calls, ... is thrown out, if this breaks a lineto */
/*  or curveto or moveto (if there aren't enough things on the stack) then we */
/*  ignore that too */

enum pstoks { pt_eof=-1, pt_moveto, pt_rmoveto, pt_curveto, pt_rcurveto,
    pt_lineto, pt_rlineto, pt_arc, pt_arcn, pt_arct, pt_arcto,
    pt_newpath, pt_closepath, pt_dup, pt_pop, pt_index,
    pt_exch, pt_roll, pt_clear, pt_copy, pt_count,
    pt_setcachedevice, pt_setcharwidth,
    pt_translate, pt_scale, pt_rotate, pt_concat, pt_end, pt_exec,
    pt_add, pt_sub, pt_mul, pt_div, pt_idiv, pt_mod, pt_neg,
    pt_abs, pt_round, pt_ceiling, pt_floor, pt_truncate, pt_max, pt_min,
    pt_ne, pt_eq, pt_gt, pt_ge, pt_lt, pt_le, pt_and, pt_or, pt_xor, pt_not,
    pt_exp, pt_sqrt, pt_ln, pt_log, pt_atan, pt_sin, pt_cos,
    pt_true, pt_false,
    pt_if, pt_ifelse, pt_for, pt_loop, pt_repeat, pt_exit,
    pt_stopped, pt_stop,
    pt_def, pt_bind, pt_load,
    pt_setlinecap, pt_setlinejoin, pt_setlinewidth, pt_setdash,
    pt_currentlinecap, pt_currentlinejoin, pt_currentlinewidth, pt_currentdash,
    pt_setgray, pt_currentgray, pt_sethsbcolor, pt_currenthsbcolor,
    pt_setrgbcolor, pt_currentrgbcolor, pt_setcmykcolor, pt_currentcmykcolor,
    pt_currentpoint,
    pt_fill, pt_stroke, pt_clip,

    pt_imagemask,

    pt_transform, pt_itransform, pt_dtransform, pt_idtransform,

    /* things we sort of pretend to do, but actually do something wrong */
    pt_gsave, pt_grestore, pt_save, pt_restore, pt_currentmatrix, pt_setmatrix,
    pt_null,

    pt_currentflat, pt_setflat,
    pt_currentglobal, pt_setglobal,
    pt_currentmiterlimit, pt_setmiterlimit,
    pt_currentobjectformat, pt_setobjectformat,
    pt_currentoverprint, pt_setoverprint,
    pt_currentpacking, pt_setpacking,
    pt_currentshared,
    pt_currentsmoothness, pt_setsmoothness,
    pt_currentstrokeadjust, pt_setstrokeadjust,

    pt_mark, pt_counttomark, pt_cleartomark, pt_array, pt_aload, pt_astore,
    pt_print, pt_cvi, pt_cvlit, pt_cvn, pt_cvr, pt_cvrs, pt_cvs, pt_cvx, pt_stringop,

    pt_opencurly, pt_closecurly, pt_openarray, pt_closearray, pt_string,
    pt_number, pt_unknown, pt_namelit, pt_output, pt_outputd,

    pt_showpage };

static const char (*toknames[]) = { "moveto", "rmoveto", "curveto", "rcurveto",
	"lineto", "rlineto", "arc", "arcn", "arct", "arcto",
	"newpath", "closepath", "dup", "pop", "index",
	"exch", "roll", "clear", "copy", "count",
	"setcachedevice", "setcharwidth",
	"translate", "scale", "rotate", "concat", "end", "exec",
	"add", "sub", "mul", "div", "idiv", "mod", "neg",
	"abs", "round", "ceiling", "floor", "truncate", "max", "min",
	"ne", "eq", "gt", "ge", "lt", "le", "and", "or", "xor", "not",
	"exp", "sqrt", "ln", "log", "atan", "sin", "cos",
	"true", "false",
	"if", "ifelse", "for", "loop", "repeat", "exit",
	"stopped", "stop",
	"def", "bind", "load",
	"setlinecap", "setlinejoin", "setlinewidth", "setdash",
	"currentlinecap", "currentlinejoin", "currentlinewidth", "currentdash",
	"setgray", "currentgray", "sethsbcolor", "currenthsbcolor",
	"setrgbcolor", "currentrgbcolor", "setcmykcolor", "currentcmykcolor",
	"currentpoint",
	"fill", "stroke", "clip",

	"imagemask",

	"transform", "itransform", "dtransform", "idtransform",

	"gsave", "grestore", "save", "restore", "currentmatrix", "setmatrix",
	"null",

	"currentflat", "setflat",
	"currentglobal", "setglobal",
	"currentmiterlimit", "setmiterlimit",
	"currentobjectformat", "setobjectformat",
	"currentoverprint", "setoverprint",
	"currentpacking", "setpacking",
	"currentshared",
	"currentsmoothness", "setsmoothness",
	"currentstrokeadjust", "setstrokeadjust",

	"mark", "counttomark", "cleartomark", "array", "aload", "astore",
	"print", "cvi", "cvlit", "cvn", "cvr", "cvrs", "cvs", "cvx", "string",

	"opencurly", "closecurly", "openarray", "closearray", "string",
	"number", "unknown", "namelit", "=", "==",

	"showpage",

	NULL };

/* length (of string)
    fill eofill stroke
    gsave grestore
*/

static int getfoghex(_IO *io) {
    int ch,val;

    while ( isspace( ch = getc(io->fog)));
    if ( isdigit(ch))
	val = ch-'0';
    else if ( ch >= 'A' && ch <= 'F' )
	val = ch-'A'+10;
    else if ( ch >= 'a' && ch <= 'f' )
	val = ch-'a'+10;
    else
return(EOF);

    val <<= 4;
    while ( isspace( ch = getc(io->fog)));
    if ( isdigit(ch))
	val |= ch-'0';
    else if ( ch >= 'A' && ch <= 'F' )
	val |= ch-'A'+10;
    else if ( ch >= 'a' && ch <= 'f' )
	val |= ch-'a'+10;
    else
return(EOF);

return( val );
}

static int nextch(IO *wrapper) {
    int ch;
    _IO *io = wrapper->top;
/* This works for fog 4.1. Fonts generated by 2.4 seem to use a different */
/*  vector, and a different number parsing scheme */
    static const char *foguvec[]= { "moveto ", "rlineto ", "rrcurveto ", " ", " ",
	"Cache ", "10 div setlinewidth ", "ShowInt ", " ", " ", " ", " ",
	"FillStroke ", " ", " ", "SetWid ", "100 mul add ", "togNS_ ",
	" ", "closepath ", " ", "SG " };

    while ( io!=NULL ) {
	if ( io->backedup!=EOF ) {
	    ch = io->backedup;
	    io->backedup = EOF;
return( ch );
	} else if ( io->ps!=NULL ) {
	    if ( (ch = getc(io->ps))!=EOF )
return( ch );
	} else if ( io->fog!=NULL ) {
	    if ( io->macro!=NULL && *io->macro!='\0' )
return( *(io->macro++) );
	    ch = getfoghex(io);
	    if ( ch>=233 ) {
		io->macro = foguvec[ch-233];
return( *(io->macro++) );
	    } else if ( ch!=EOF && ch<200 ) {
		sprintf( io->fogbuf, "%d ", ch-100);
		io->macro=io->fogbuf;
return( *(io->macro++) );
	    } else if (ch!=EOF) {
		sprintf( io->fogbuf, "%d %s ", ch-233+17, io->fogns
			? "2 exch exp 3 1 roll 100 mul add mul"
			: "100 mul add" );
		io->macro=io->fogbuf;
return( *(io->macro++) );
	    }
	} else {
	    if ( (ch = *(io->macro++))!='\0' )
return( ch );
	    if ( --io->cnt>0 ) {
		io->macro = io->start;
return( nextch(wrapper));
	    }
	}
	wrapper->top = io->prev;
	if ( io->isstopped )
	    wrapper->endedstopped = true;
	if (io->start != NULL) free(io->start); io->start = NULL;
	free(io);
	io = wrapper->top;
    }
return( EOF );
}

static void unnextch(int ch,IO *wrapper) {
    if ( ch==EOF )
return;
    if ( wrapper->top==NULL )
	LogError( _("Can't back up with nothing on stack\n") );
    else if ( wrapper->top->backedup!=EOF )
	LogError( _("Attempt to back up twice\n") );
    else if ( wrapper->top->ps!=NULL )
	ungetc(ch,wrapper->top->ps);
    else
	wrapper->top->backedup = ch;
}

static void pushio(IO *wrapper, FILE *ps, char *macro, int cnt) {
    _IO *io = calloc(1,sizeof(_IO));

    io->prev = wrapper->top;
    io->ps = ps;
    io->macro = io->start = copy(macro);
    io->backedup = EOF;
    if ( cnt==-1 ) {
	io->cnt = 1;
	io->isstopped = true;
    } else if ( cnt==0 ) {
	io->cnt = 1;
	io->isloop = false;
    } else {
	io->cnt = cnt;
	io->isloop = true;
    }
    wrapper->top = io;
}

static void pushfogio(IO *wrapper, FILE *fog) {
    _IO *io = calloc(1,sizeof(_IO));

    io->prev = wrapper->top;
    io->fog = fog;
    io->backedup = EOF;
    io->cnt = 1;
    io->isloop = false;
    wrapper->top = io;
}

static void ioescapeloop(IO *wrapper) {
    _IO *io = wrapper->top, *iop;
    int wasloop;

    while ( io->prev!=NULL && !io->isstopped ) {
	iop = io->prev;
	wasloop = io->isloop;
	if (io->start != NULL) free(io->start); io->start = NULL;
	free(io);
	if ( wasloop ) {
	    wrapper->top = iop;
return;
	}
	io = iop;
    }

/* GT: This is part of the PostScript language. "exit" should not be translated */
/* GT: as it is a PostScript keyword. (FF contains a small PostScript interpreter */
/* GT: so it can understand some PostScript fonts, and can generate errors when */
/* GT: handed bad PostScript). */
    LogError( _("Use of \"exit\" when not in a loop\n") );
    wrapper->top = io;
}

static int ioescapestopped(IO *wrapper, struct psstack *stack, int sp, const size_t bsize) {
    _IO *io = wrapper->top, *iop;
    int wasstopped;

    while ( io->prev!=NULL ) {
	iop = io->prev;
	wasstopped = io->isstopped;
	if (io->start != NULL) free(io->start); io->start = NULL;
	free(io);
	if ( wasstopped ) {
	    wrapper->top = iop;
	    if ( sp<(int)bsize ) {
		stack[sp].type = ps_bool;
		stack[sp++].u.tf = true;
	    }
return(sp);
	}
	io = iop;
    }

/* GT: This is part of the PostScript language. Neither "stop" nor "stopped" */
/* GT: should be translated as both are PostScript keywords. */
    LogError( _("Use of \"stop\" when not in a stopped\n") );
    wrapper->top = io;
return( sp );
}

static int endedstopped(IO *wrapper) {
    if ( wrapper->endedstopped ) {
	wrapper->endedstopped = false;
return( true );
    }
return( false );
}

static int CheckCodePointsComment(IO *wrapper) {
    /* Check to see if this encoding includes the special comment we use */
    /*  to indicate that the encoding is based on unicode code points rather */
    /*  than glyph names */
    char commentbuffer[128], *pt;
    int ch;

    /* Eat whitespace and comments. Comments last to eol (or formfeed) */
    while ( isspace(ch = nextch(wrapper)) );
    if ( ch!='%' ) {
	unnextch(ch,wrapper);
return( false );
    }

    pt = commentbuffer;
    while ( (ch=nextch(wrapper))!=EOF && ch!='\r' && ch!='\n' && ch!='\f' ) {
	if ( pt-commentbuffer < (ptrdiff_t)sizeof(commentbuffer)-1 )
	    *pt++ = ch;
    }
    *pt = '\0';
    if ( strcmp(commentbuffer," Use codepoints.")== 0 )
return( true );

return( false );
}

static int nextpstoken(IO *wrapper, real *val, char *tokbuf, int tbsize) {
    int ch, r, i;
    char *pt, *end;
    float mf2pt_advance_width;

    pt = tokbuf;
    end = pt+tbsize-1;

    /* Eat whitespace and comments. Comments last to eol (or formfeed) */
    while ( 1 ) {
	while ( isspace(ch = nextch(wrapper)) );
	if ( ch!='%' )
    break;
	while ( (ch=nextch(wrapper))!=EOF && ch!='\r' && ch!='\n' && ch!='\f' )
	    if ( pt<end )
		*pt++ = ch;
	*pt='\0';
	/* Some comments have meanings (that we care about) */
	if ( sscanf( tokbuf, " MF2PT1: bbox %*g %*g %g %*g", &mf2pt_advance_width )==1 )
	    wrapper->advance_width = mf2pt_advance_width;
	else if ( sscanf( tokbuf, " MF2PT1: glyph_dimensions %*g %*g %g %*g", &mf2pt_advance_width )==1 )
	    wrapper->advance_width = mf2pt_advance_width;
	pt = tokbuf;
    }

    if ( ch==EOF )
return( pt_eof );

    pt = tokbuf;
    end = pt+tbsize-1;
    *pt++ = ch; *pt='\0';

    if ( ch=='(' ) {
	int nest=1, quote=0;
	while ( (ch=nextch(wrapper))!=EOF ) {
	    if ( pt<end ) *pt++ = ch;
	    if ( quote )
		quote=0;
	    else if ( ch=='(' )
		++nest;
	    else if ( ch==')' ) {
		if ( --nest==0 )
	break;
	    } else if ( ch=='\\' )
		quote = 1;
	}
	*pt='\0';
return( pt_string );
    } else if ( ch=='<' ) {
	ch = nextch(wrapper);
	if ( pt<end ) *pt++ = ch;
	if ( ch=='>' )
	    /* Done */;
	else if ( ch!='~' ) {
	    while ( (ch=nextch(wrapper))!=EOF && ch!='>' )
		if ( pt<end ) *pt++ = ch;
	} else {
	    int twiddle=0;
	    while ( (ch=nextch(wrapper))!=EOF ) {
		if ( pt<end ) *pt++ = ch;
		if ( ch=='~' ) twiddle = 1;
		else if ( twiddle && ch=='>' )
	    break;
		else twiddle = 0;
	    }
	}
	*pt='\0';
return( pt_string );
    } else if ( ch==')' || ch=='>' || ch=='[' || ch==']' || ch=='{' || ch=='}' ) {
	if ( ch=='{' )
return( pt_opencurly );
	else if ( ch=='}' )
return( pt_closecurly );
	if ( ch=='[' )
return( pt_openarray );
	else if ( ch==']' )
return( pt_closearray );

return( pt_unknown );	/* single character token */
    } else if ( ch=='/' ) {
	pt = tokbuf;
	while ( (ch=nextch(wrapper))!=EOF && !isspace(ch) && ch!='%' &&
		ch!='(' && ch!=')' && ch!='<' && ch!='>' && ch!='[' && ch!=']' &&
		ch!='{' && ch!='}' && ch!='/' )
	    if ( pt<tokbuf+tbsize-2 )
		*pt++ = ch;
	*pt = '\0';
	unnextch(ch,wrapper);
return( pt_namelit );	/* name literal */
    } else {
	while ( (ch=nextch(wrapper))!=EOF && !isspace(ch) && ch!='%' &&
		ch!='(' && ch!=')' && ch!='<' && ch!='>' && ch!='[' && ch!=']' &&
		ch!='{' && ch!='}' && ch!='/' ) {
	    if ( pt<tokbuf+tbsize-2 )
		*pt++ = ch;
	}
	*pt = '\0';
	unnextch(ch,wrapper);
	r = strtol(tokbuf,&end,10);
	pt = end;
	if ( *pt=='\0' ) {		/* It's a normal integer */
	    *val = r;
return( pt_number );
	} else if ( *pt=='#' ) {
	    r = strtol(pt+1,&end,r);
	    if ( *end=='\0' ) {		/* It's a radix integer */
		*val = r;
return( pt_number );
	    }
	} else {
	    *val = strtod(tokbuf,&end);
	    if ( !isfinite(*val) ) {
/* GT: NaN is a concept in IEEE floating point which means "Not a Number" */
/* GT: it is used to represent errors like 0/0 or sqrt(-1). */
		LogError( _("Bad number, infinity or nan: %s\n"), tokbuf );
		*val = 0;
	    }
	    if ( *end=='\0' )		/* It's a real */
return( pt_number );
	}
	/* It's not a number */
	for ( i=0; toknames[i]!=NULL; ++i )
	    if ( strcmp(tokbuf,toknames[i])==0 )
return( i );

return( pt_unknown );
    }
}

static void Transform(BasePoint *to, DBasePoint *from, real trans[6]) {
    to->x = trans[0]*from->x+trans[2]*from->y+trans[4];
    to->y = trans[1]*from->x+trans[3]*from->y+trans[5];
}

void MatMultiply(real m1[6], real m2[6], real to[6]) {
    real trans[6];

    trans[0] = m1[0]*m2[0] +
		m1[1]*m2[2];
    trans[1] = m1[0]*m2[1] +
		m1[1]*m2[3];
    trans[2] = m1[2]*m2[0] +
		m1[3]*m2[2];
    trans[3] = m1[2]*m2[1] +
		m1[3]*m2[3];
    trans[4] = m1[4]*m2[0] +
		m1[5]*m2[2] +
		m2[4];
    trans[5] = m1[4]*m2[1] +
		m1[5]*m2[3] +
		m2[5];
    memcpy(to,trans,sizeof(trans));
}

void MatInverse(real into[6], real orig[6]) {
    real det = orig[0]*orig[3] - orig[1]*orig[2];

    if ( det==0 ) {
	LogError( _("Attempt to invert a singular matrix\n") );
	memset(into,0,sizeof(*into));
    } else {
	into[0] =  orig[3]/det;
	into[1] = -orig[1]/det;
	into[2] = -orig[2]/det;
	into[3] =  orig[0]/det;
	into[4] = -orig[4]*into[0] - orig[5]*into[2];
	into[5] = -orig[4]*into[1] - orig[5]*into[3];
    }
}

int MatIsIdentity(real transform[6]) {
return( transform[0]==1 && transform[3]==1 && transform[1]==0 && transform[2]==0 &&
	transform[4]==0 && transform[5]==0 );
}

static void ECCategorizePoints( EntityChar *ec ) {
    Entity *ent;

    for ( ent=ec->splines; ent!=NULL; ent=ent->next ) if ( ent->type == et_splines ) {
	SPLCategorizePoints( ent->u.splines.splines );
	SPLCategorizePoints( ent->clippath );
    }
}

static int AddEntry(struct pskeydict *dict,struct psstack *stack, int sp) {
    int i;

    if ( dict->cnt>=dict->max ) {
	if ( dict->cnt==0 ) {
	    dict->max = 30;
	    dict->entries = malloc(dict->max*sizeof(struct pskeyval));
	} else {
	    dict->max += 30;
	    dict->entries = realloc(dict->entries,dict->max*sizeof(struct pskeyval));
	}
    }
    if ( sp<2 )
return(sp);
    if ( stack[sp-2].type!=ps_string && stack[sp-2].type!=ps_lit ) {
/* GT: Here "def" is a PostScript keyword, (meaning define). */
/* GT: This "def" should not be translated as it is part of the PostScript language. */
	LogError( _("Key for a def must be a string or name literal\n") );
return(sp-2);
    }
    for ( i=0; i<dict->cnt; ++i )
	if ( strcmp(dict->entries[i].key,stack[sp-2].u.str)==0 )
    break;
    if ( i!=dict->cnt ) {
	free(stack[sp-2].u.str);
	if ( dict->entries[i].type==ps_string || dict->entries[i].type==ps_instr ||
		dict->entries[i].type==ps_lit )
	    free(dict->entries[i].u.str);
    } else {
	memset(&dict->entries[i],'\0',sizeof(struct pskeyval));
	dict->entries[i].key = stack[sp-2].u.str;
	++dict->cnt;
    }
    dict->entries[i].type = stack[sp-1].type;
    dict->entries[i].u = stack[sp-1].u;
return(sp-2);
}

static int forgetstack(struct psstack *stack, int forgets, int sp) {
    /* forget the bottom most "forgets" entries on the stack */
    /* we presume they are garbage that has accumulated because we */
    /*  don't understand all of PS */
    int i;
    for ( i=0; i<forgets; ++i ) {
	if ( stack[i].type==ps_string || stack[i].type==ps_instr ||
		stack[i].type==ps_lit )
	    free(stack[i].u.str);
	else if ( stack[i].type==ps_array || stack[i].type==ps_dict )
	    dictfree(&stack[i].u.dict);
    }
    for ( i=forgets; i<sp; ++i )
	stack[i-forgets] = stack[i];
return( sp-forgets );
}

static int rollstack(struct psstack *stack, int sp) {
    int n,j,i;
    struct psstack *temp;

    if ( sp>1 ) {
	n = stack[sp-2].u.val;
	j = stack[sp-1].u.val;
	sp-=2;
	if ( sp>=n && n>0 ) {
	    j %= n;
	    if ( j<0 ) j += n;
	    temp = malloc(n*sizeof(struct psstack));
	    for ( i=0; i<n; ++i )
		temp[i] = stack[sp-n+i];
	    for ( i=0; i<n; ++i )
		stack[sp-n+(i+j)%n] = temp[i];
	    free(temp);
	}
    }
return( sp );
}

static void CheckMakeB(BasePoint *test, BasePoint *good) {
    if ( !isfinite(test->x) || test->x>100000 || test->x<-100000 ) {
	LogError( _("Value out of bounds in spline.\n") );
	if ( good!=NULL )
	    test->x = good->x;
	else
	    test->x = 0;
    }
    if ( !isfinite(test->y) || test->y>100000 || test->y<-100000 ) {
	LogError( _("Value out of bounds in spline.\n") );
	if ( good!=NULL )
	    test->y = good->y;
	else
	    test->y = 0;
    }
}

static void CheckMake(SplinePoint *from, SplinePoint *to) {
    CheckMakeB(&from->me,NULL);
    CheckMakeB(&from->nextcp,&from->me);
    CheckMakeB(&to->prevcp,&from->nextcp);
    CheckMakeB(&to->me,&to->prevcp);
}

static void circlearcto(real a1, real a2, real cx, real cy, real r,
	SplineSet *cur, real *transform ) {
    SplinePoint *pt;
    DBasePoint temp, base, cp;
    real cplen;
    int sign=1;
    real s1, s2, c1, c2;

    if ( a1==a2 )
return;

    cplen = (a2-a1)/90 * r * .552;
    a1 *= 3.1415926535897932/180; a2 *= 3.1415926535897932/180;
    s1 = sin(a1); s2 = sin(a2); c1 = cos(a1); c2 = cos(a2);
    temp.x = cx+r*c2; temp.y = cy+r*s2;
    base.x = cx+r*c1; base.y = cy+r*s1;
    pt = chunkalloc(sizeof(SplinePoint));
    Transform(&pt->me,&temp,transform);
    cp.x = temp.x-cplen*s2; cp.y = temp.y + cplen*c2;
    if ( (cp.x-base.x)*(cp.x-base.x)+(cp.y-base.y)*(cp.y-base.y) >
	     (temp.x-base.x)*(temp.x-base.x)+(temp.y-base.y)*(temp.y-base.y) ) {
	sign = -1;
	cp.x = temp.x+cplen*s2; cp.y = temp.y - cplen*c2;
    }
    Transform(&pt->prevcp,&cp,transform);
    pt->nonextcp = true;
    cp.x = base.x + sign*cplen*s1; cp.y = base.y - sign*cplen*c1;
    Transform(&cur->last->nextcp,&cp,transform);
    cur->last->nonextcp = false;
    CheckMake(cur->last,pt);
    SplineMake3(cur->last,pt);
    cur->last = pt;
}

static void circlearcsto(real a1, real a2, real cx, real cy, real r,
	SplineSet *cur, real *transform, int clockwise ) {
    int a;
    real last;

    while ( a1<0 ) { a1 += 360; a2 +=360;} while ( a2-a1<=-360 ) a2 += 360;
    while ( a1>360 ) { a1 -= 360; a2 -= 360; } while ( a2-a1>360 ) a2 -= 360;
    if ( !clockwise ) {
	if ( a1>a2 )
	    a2 += 360;
	last = a1;
	for ( a=((int) (a1+90)/90)*90; a<a2; a += 90 ) {
	    circlearcto(last,a,cx,cy,r,cur,transform);
	    last = a;
	}
	circlearcto(last,a2,cx,cy,r,cur,transform);
    } else {
	if ( a2>a1 )
	    a1 += 360;
	last = a1;
	for ( a=((int) (a1-90)/90)*90+90; a>a2; a -= 90 ) {
	    circlearcto(last,a,cx,cy,r,cur,transform);
	    last = a;
	}
	circlearcto(last,a2,cx,cy,r,cur,transform);
    }
}

static void collectgarbage(struct garbage *tofrees,struct pskeydict *to) {
    struct garbage *into;

    /* Garbage collection pointers */
    into = tofrees;
    if ( tofrees->cnt>=GARBAGE_MAX && tofrees->next!=NULL )
	into = tofrees->next;
    if ( into->cnt>=GARBAGE_MAX ) {
	into = chunkalloc(sizeof(struct garbage));
	into->next = tofrees->next;
	tofrees->next = into;
    }
    into->cnts[    into->cnt   ] = to->cnt;
    into->entries[ into->cnt++ ] = to->entries;
}

static void copyarray(struct pskeydict *to,struct pskeydict *from, struct garbage *tofrees) {
    int i;
    struct pskeyval *oldent = from->entries;

    *to = *from;
    to->entries = calloc(to->cnt,sizeof(struct pskeyval));
    for ( i=0; i<to->cnt; ++i ) {
	to->entries[i] = oldent[i];
	if ( to->entries[i].type==ps_string || to->entries[i].type==ps_instr ||
		to->entries[i].type==ps_lit )
	    to->entries[i].u.str = copy(to->entries[i].u.str);
	else if ( to->entries[i].type==ps_array || to->entries[i].type==ps_dict )
	    copyarray(&to->entries[i].u.dict,&oldent[i].u.dict,tofrees);
    }
    collectgarbage(tofrees,to);
}

static int aload(unsigned sp, struct psstack *stack,size_t stacktop, struct garbage *tofrees) {
    int i;

    if ( sp>=1 && stack[sp-1].type==ps_array ) {
	struct pskeydict dict;
	--sp;
	dict = stack[sp].u.dict;
	for ( i=0; i<dict.cnt; ++i ) {
	    if ( sp<stacktop ) {
		stack[sp].type = dict.entries[i].type;
		stack[sp].u = dict.entries[i].u;
		if ( stack[sp].type==ps_string || stack[sp].type==ps_instr ||
			stack[sp].type==ps_lit )
		    stack[sp].u.str = copy(stack[sp].u.str);
/* The following is incorrect behavior, but as I don't do garbage collection */
/*  and I'm not going to implement reference counts, this will work in most cases */
		else if ( stack[sp].type==ps_array )
		    copyarray(&stack[sp].u.dict,&stack[sp].u.dict,tofrees);
		++sp;
	    }
	}
    }
return( sp );
}

static void printarray(struct pskeydict *dict) {
    int i;

    printf("[" );
    for ( i=0; i<dict->cnt; ++i ) {
	switch ( dict->entries[i].type ) {
	  case ps_num:
	    printf( "%g", (double) dict->entries[i].u.val );
	  break;
	  case ps_bool:
	    printf( "%s", dict->entries[i].u.tf ? "true" : "false" );
	  break;
	  case ps_string: case ps_instr: case ps_lit:
	    printf( dict->entries[i].type==ps_lit ? "/" :
		    dict->entries[i].type==ps_string ? "(" : "{" );
	    printf( "%s", dict->entries[i].u.str );
	    printf( dict->entries[i].type==ps_lit ? "" :
		    dict->entries[i].type==ps_string ? ")" : "}" );
	  break;
	  case ps_array:
	    printarray(&dict->entries[i].u.dict);
	  break;
	  case ps_void:
	    printf( "-- void --" );
	  break;
	  default:
	    printf( "-- nostringval --" );
	  break;
	}
	printf(" ");
    }
    printf( "]" );
}

static void freestuff(struct psstack *stack, int sp, struct pskeydict *dict,
	GrowBuf *gb, struct garbage *tofrees) {
    int i;

    free(gb->base);
    for ( i=0; i<dict->cnt; ++i ) {
	if ( dict->entries[i].type==ps_string || dict->entries[i].type==ps_instr ||
		dict->entries[i].type==ps_lit )
	    free(dict->entries[i].u.str);
	free(dict->entries[i].key);
    }
    free( dict->entries );
    for ( i=0; i<sp; ++i ) {
	if ( stack[i].type==ps_string || stack[i].type==ps_instr ||
		stack[i].type==ps_lit )
	    free(stack[i].u.str);
    }
    garbagefree(tofrees);
}

static void DoMatTransform(int tok,int sp,struct psstack *stack) {
    real invt[6], t[6];

    if ( stack[sp-1].u.dict.cnt==6 && stack[sp-1].u.dict.entries[0].type==ps_num ) {
	double x = stack[sp-3].u.val, y = stack[sp-2].u.val;
	--sp;
	t[5] = stack[sp].u.dict.entries[5].u.val;
	t[4] = stack[sp].u.dict.entries[4].u.val;
	t[3] = stack[sp].u.dict.entries[3].u.val;
	t[2] = stack[sp].u.dict.entries[2].u.val;
	t[1] = stack[sp].u.dict.entries[1].u.val;
	t[0] = stack[sp].u.dict.entries[0].u.val;
	dictfree(&stack[sp].u.dict);
	if ( tok==pt_itransform || tok==pt_idtransform ) {
	    MatInverse(invt,t);
	    memcpy(t,invt,sizeof(t));
	}
	stack[sp-2].u.val = t[0]*x + t[1]*y;
	stack[sp-1].u.val = t[2]*x + t[3]*y;
	if ( tok==pt_transform || tok==pt_itransform ) {
	    stack[sp-2].u.val += t[4];
	    stack[sp-1].u.val += t[5];
	}
    }
}

static int DoMatOp(int tok,int sp,struct psstack *stack) {
    real temp[6], t[6];
    int nsp=sp;

    if ( stack[sp-1].u.dict.cnt==6 && stack[sp-1].u.dict.entries[0].type==ps_num ) {
	t[5] = stack[sp-1].u.dict.entries[5].u.val;
	t[4] = stack[sp-1].u.dict.entries[4].u.val;
	t[3] = stack[sp-1].u.dict.entries[3].u.val;
	t[2] = stack[sp-1].u.dict.entries[2].u.val;
	t[1] = stack[sp-1].u.dict.entries[1].u.val;
	t[0] = stack[sp-1].u.dict.entries[0].u.val;
	switch ( tok ) {
	  case pt_translate:
	    if ( sp>=3 ) {
		stack[sp-1].u.dict.entries[5].u.val += stack[sp-3].u.val*t[0]+stack[sp-2].u.val*t[2];
		stack[sp-1].u.dict.entries[4].u.val += stack[sp-3].u.val*t[1]+stack[sp-2].u.val*t[3];
		nsp = sp-2;
	    }
	  break;
	  case pt_scale:
	    if ( sp>=2 ) {
		stack[sp-1].u.dict.entries[0].u.val *= stack[sp-3].u.val;
		stack[sp-1].u.dict.entries[1].u.val *= stack[sp-3].u.val;
		stack[sp-1].u.dict.entries[2].u.val *= stack[sp-2].u.val;
		stack[sp-1].u.dict.entries[3].u.val *= stack[sp-2].u.val;
		/* transform[4,5] are unchanged */
		nsp = sp-2;
	    }
	  break;
	  case pt_rotate:
	    if ( sp>=1 ) {
		--sp;
		temp[0] = temp[3] = cos(stack[sp].u.val);
		temp[1] = sin(stack[sp].u.val);
		temp[2] = -temp[1];
		temp[4] = temp[5] = 0;
		MatMultiply(temp,t,t);
		stack[sp-1].u.dict.entries[5].u.val = t[5];
		stack[sp-1].u.dict.entries[4].u.val = t[4];
		stack[sp-1].u.dict.entries[3].u.val = t[3];
		stack[sp-1].u.dict.entries[2].u.val = t[2];
		stack[sp-1].u.dict.entries[1].u.val = t[1];
		stack[sp-1].u.dict.entries[0].u.val = t[0];
		nsp = sp-1;
	    }
	  break;
	  default:
	  break;
	}
	stack[nsp-1] = stack[sp-1];
    }
return(nsp);
}

static Entity *EntityCreate(SplinePointList *head,int linecap,int linejoin,
	real linewidth, real *transform, SplineSet *clippath) {
    Entity *ent = calloc(1,sizeof(Entity));
    ent->type = et_splines;
    ent->u.splines.splines = head;
    ent->u.splines.cap = linecap;
    ent->u.splines.join = linejoin;
    ent->u.splines.stroke_width = linewidth;
    ent->u.splines.fill.col = 0xffffffff;
    ent->u.splines.stroke.col = 0xffffffff;
    ent->u.splines.fill.opacity = 1.0;
    ent->u.splines.stroke.opacity = 1.0;
    ent->clippath = SplinePointListCopy(clippath);
    memcpy(ent->u.splines.transform,transform,6*sizeof(real));
return( ent );
}

static uint8 *StringToBytes(struct psstack *stackel,int *len) {
    char *pt;
    uint8 *upt, *base, *ret;
    int half, sofar, val, nesting;
    int i,j;

    pt = stackel->u.str;
    if ( stackel->type==ps_instr ) {
	/* imagemask operators take strings or procedures or files */
	/* we support strings, or procedures containing strings */
	while ( isspace(*pt)) ++pt;
	if ( *pt=='{' || *pt=='[' ) ++pt;
	while ( isspace(*pt)) ++pt;
    } else if ( stackel->type!=ps_string )
return( NULL );

    upt = base = malloc(65536+1);	/* Maximum size of ps string */

    if ( *pt=='(' ) {
	/* A conventional string */
	++pt;
	nesting = 0;
	while ( *pt!='\0' && (nesting!=0 || *pt!=')') ) {
	    if ( *pt=='(' ) {
		++nesting;
		*upt++ = *pt++;
	    } else if ( *pt==')' ) {
		--nesting;
		*upt++ = *pt++;
	    } else if ( *pt=='\r' || *pt=='\n' ) {
		/* any of lf, cr, crlf gets translated to \n */
		if ( *pt=='\r' && pt[1]=='\n' ) ++pt;
		*upt++ = '\n';
		++pt;
	    } else if ( *pt!='\\' ) {
		*upt++ = *pt++;
	    } else {
		++pt;
		if ( *pt=='\r' || *pt=='\n' ) {
		    /* any of \lf, \cr, \crlf gets ignored */
		    if ( *pt=='\r' && pt[1]=='\n' ) ++pt;
		    ++pt;
		} else if ( *pt=='n' ) {
		    *upt++ = '\n';
		    ++pt;
		} else if ( *pt=='r' ) {
		    *upt++ = '\r';
		    ++pt;
		} else if ( *pt=='t' ) {
		    *upt++ = '\t';
		    ++pt;
		} else if ( *pt=='b' ) {
		    *upt++ = '\b';
		    ++pt;
		} else if ( *pt=='f' ) {
		    *upt++ = '\f';
		    ++pt;
		} else if ( *pt>='0' && *pt<='7' ) {
		    if ( pt[1]<'0' || pt[1]>'7' )	/* This isn't really legal postscript */
			*upt++ = *pt++ - '0';
		    else if ( pt[2]<'0' || pt[2]>'7' ) {/* 3 octal digits are required */
			*upt++ = ((*pt - '0')<<3) + (pt[1]-'0');
			pt += 2;
		    } else {
			*upt++ = ((*pt - '0')<<6) + ((pt[1]-'0')<<3) + (pt[2]-'0');
			pt += 3;
		    }
		} else if ( *pt=='(' || *pt==')' || *pt=='\\' )
		    *upt++ = *pt++;
		else {
		    LogError( _("Unknown character after backslash in literal string.\n"));
		    *upt++ = *pt++;
		}
	    }
	}
    } else if ( *pt!='<' ) {
	LogError( _("Unknown string type\n" ));
	free(base);
return( NULL );
    } else if ( pt[1]!='~' ) {
	/* A hex string. Ignore any characters which aren't hex */
	half = sofar = 0;
	++pt;
	while ( *pt!='>' && *pt!='\0' ) {
	    if ( *pt>='a' && *pt<='f' )
		val = *pt++-'a'+10;
	    else if ( *pt>='A' && *pt<='F' )
		val = *pt++-'A'+10;
	    else if ( isdigit(*pt))
		val = *pt++-'0';
	    else {
		++pt;		/* Not hex */
	continue;
	    }
	    if ( !half ) {
		half = true;
		sofar = val<<4;
	    } else {
		*upt++ = sofar|val;
		half = false;
	    }
	}
	if ( half )
	    *upt++ = sofar;
    } else {
	/* An ASCII-85 string */
	/* c1 c2 c3 c4 c5 (c>='!' && c<='u') => ((c1-'!')*85+(c2-'!'))*85... */
	/* z => 32bits of 0 */
	pt += 2;
	while ( *pt!='\0' && *pt!='~' ) {
	    if ( upt-base+4 > 65536 )
	break;
	    if ( *pt=='z' ) {
		*upt++ = 0;
		*upt++ = 0;
		*upt++ = 0;
		*upt++ = 0;
		++pt;
	    } else if ( *pt>='!' && *pt<='u' ) {
		val = 0;
		for ( i=0; i<5 && *pt>='!' && *pt<='u'; ++i )
		    val = (val*85) + *pt++ - '!';
		for ( j=i; j<5; ++j )
		    val *= 85;
		*upt++ =  val>>24      ;
		if ( i>2 )
		    *upt++ = (val>>16)&0xff;
		if ( i>3 )
		    *upt++ = (val>>8 )&0xff;
		if ( i>4 )
		    *upt++ =  val     &0xff;
		if ( i<5 )
	break;
	    } else if ( isspace( *pt ) ) {
		++pt;
	    } else
	break;
	}
    }
    *len = upt-base;
    ret = malloc(upt-base);
    memcpy(ret,base,upt-base);
    free(base);
return(ret);
}

static int PSAddImagemask(EntityChar *ec,struct psstack *stack,int sp,
	real transform[6],Color fillcol) {
    uint8 *data;
    int datalen, width, height, polarity;
    real trans[6];
    struct _GImage *base;
    GImage *gi;
    Entity *ent;
    int i,j;

    if ( sp<5 || (stack[sp-1].type!=ps_instr && stack[sp-1].type!=ps_string)) {
	LogError( _("FontForge does not support dictionary based imagemask operators.\n" ));
return( sp-1 );
    }

    if ( stack[sp-2].type!=ps_array || stack[sp-2].u.dict.cnt!=6 ) {
	LogError( _("Fourth argument of imagemask must be a 6-element transformation matrix.\n" ));
return( sp-5 );
    }

    if ( stack[sp-3].type!=ps_bool ) {
	LogError( _("Third argument of imagemask must be a boolean.\n" ));
return( sp-5 );
    }
    polarity = stack[sp-3].u.tf;

    if ( stack[sp-4].type!=ps_num || stack[sp-5].type!=ps_num ) {
	LogError( _("First and second arguments of imagemask must be integers.\n" ));
return( sp-5 );
    }
    height = stack[sp-4].u.val;
    width = stack[sp-5].u.val;

    data = StringToBytes(&stack[sp-1],&datalen);

    if ( width<=0 || height<=0 || ((width+7)/8)*height>datalen ) {
	LogError( _("Width or height arguments to imagemask contain invalid values\n(either negative or they require more data than provided).\n" ));
	free(data);
return( sp-5 );
    }
    trans[0] = stack[sp-2].u.dict.entries[0].u.val;
    trans[1] = stack[sp-2].u.dict.entries[1].u.val;
    trans[2] = stack[sp-2].u.dict.entries[2].u.val;
    trans[3] = stack[sp-2].u.dict.entries[3].u.val;
    trans[4] = stack[sp-2].u.dict.entries[4].u.val;
    trans[5] = stack[sp-2].u.dict.entries[5].u.val;

    gi = GImageCreate(it_mono,width,height);
    base = gi->u.image;
    base->trans = 1;
    if ( polarity ) {
	for ( i=0; i<datalen; ++i )
	    data[i] ^= 0xff;
    }
    if ( trans[0]>0 && trans[3]<0 )
	memcpy(base->data,data,datalen);
    else if ( trans[0]>0 && trans[3]>0 ) {
	for ( i=0; i<height; ++i )
	    memcpy(base->data+i*base->bytes_per_line,data+(height-i)*base->bytes_per_line,
		    base->bytes_per_line);
    } else if ( trans[0]<0 && trans[3]<0 ) {
	for ( i=0; i<height; ++i ) for ( j=0; j<width; ++j ) {
	    if ( data[i*base->bytes_per_line+ (j>>3)]&(0x80>>(j&7)) )
		base->data[i*base->bytes_per_line + ((width-j-1)>>3)] |=
			(0x80>>((width-j-1)&7));
	}
    } else {
	for ( i=0; i<height; ++i ) for ( j=0; j<width; ++j ) {
	    if ( data[i*base->bytes_per_line+ (j>>3)]&(0x80>>(j&7)) )
		base->data[(height-i-1)*base->bytes_per_line + ((width-j-1)>>3)] |=
			(0x80>>((width-j-1)&7));
	}
    }
    free(data);

    ent = calloc(1,sizeof(Entity));
    ent->type = et_image;
    ent->u.image.image = gi;
    memcpy(ent->u.image.transform,transform,sizeof(real[6]));
    ent->u.image.transform[0] /= width;
    ent->u.image.transform[3] /= height;
    ent->u.image.transform[5] += height;
    ent->u.image.col = fillcol;

    ent->next = ec->splines;
    ec->splines = ent;
return( sp-5 );
}

static void HandleType3Reference(IO *wrapper,EntityChar *ec,real transform[6],
	char *tokbuf, int toksize) {
    int tok;
    real dval;
    char *glyphname;
    RefChar *ref;

   tok = nextpstoken(wrapper,&dval,tokbuf,toksize);
   if ( strcmp(tokbuf,"get")!=0 )
return;		/* Hunh. I don't understand it. I give up */
   tok = nextpstoken(wrapper,&dval,tokbuf,toksize);
   if ( tok!=pt_namelit )
return;		/* Hunh. I don't understand it. I give up */
    glyphname = copy(tokbuf);
   tok = nextpstoken(wrapper,&dval,tokbuf,toksize);
   if ( strcmp(tokbuf,"get")!=0 ) {
	free(glyphname);
	return;	/* Hunh. I don't understand it. I give up */
   }
   tok = nextpstoken(wrapper,&dval,tokbuf,toksize);
   if ( strcmp(tokbuf,"exec")!=0 ) {
	free(glyphname);
	return;	/* Hunh. I don't understand it. I give up */
    }

    /* Ok, it looks very much like a reference to glyphname */
    ref = RefCharCreate();
    memcpy(ref->transform,transform,sizeof(ref->transform));
    ref->sc = (SplineChar *) glyphname;
    ref->next = ec->refs;
    ec->refs = ref;
}

static void _InterpretPS(IO *wrapper, EntityChar *ec, RetStack *rs) {
    SplinePointList *cur=NULL, *head=NULL;
    DBasePoint current, temp;
    int tok, i, j;
    struct psstack stack[100];
    real dval;
    unsigned sp=0;
    SplinePoint *pt;
    RefChar *ref, *lastref=NULL;
    real transform[6], t[6];
    struct graphicsstate {
	real transform[6];
	DBasePoint current;
	real linewidth;
	int linecap, linejoin;
	Color fore;
	DashType dashes[DASH_MAX];
	SplineSet *clippath;
    } gsaves[30];
    int gsp = 0;
    int ccnt=0;
    GrowBuf gb;
    struct pskeydict dict;
    struct pskeyval *kv;
    Color fore=COLOR_INHERITED;
    int linecap=lc_inherited, linejoin=lj_inherited; real linewidth=WIDTH_INHERITED;
    DashType dashes[DASH_MAX];
    int dash_offset = 0;
    Entity *ent;
    int warned = 0;
    struct garbage tofrees;
    SplineSet *clippath = NULL;
    char *tokbuf;
    const int tokbufsize = 2*65536+10;

    tokbuf = malloc(tokbufsize);

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.

    memset(&gb,'\0',sizeof(GrowBuf));
    memset(&dict,'\0',sizeof(dict));
    tofrees.cnt = 0; tofrees.next = NULL;

    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = transform[4] = transform[5] = 0;
    current.x = current.y = 0;
    dashes[0] = 0; dashes[1] = DASH_INHERITED;

    if ( ec->fromtype3 ) {
	/* My type3 fonts have two things pushed on the stack when they */
	/*  start. One is a dictionary, the other a flag (number). If the */
	/*  flag is non-zero then we are a nested call (a reference char) */
	/*  if 0, we're normal. We don't want to do setcachedevice for */
	/*  reference chars.  We can't represent a dictionary on the stack */
	/*  so just push two 0s */
	stack[0].type = stack[1].type = ps_num;
	stack[0].u.val = stack[1].u.val = 0;
	sp = 2;
    }

    while ( (tok = nextpstoken(wrapper,&dval,tokbuf,tokbufsize))!=pt_eof ) {
	if ( endedstopped(wrapper)) {
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_bool;
		stack[sp++].u.tf = false;
	    }
	}
	if ( sp>sizeof(stack)/sizeof(stack[0])*4/5 ) {
	    /* We don't interpret all of postscript */
	    /* Sometimes we leave garbage on the stack that a real PS interp */
	    /*  would have handled. If the stack gets too deep, clean out the */
	    /*  oldest entries */
	    sp = forgetstack(stack,sizeof(stack)/sizeof(stack[0])/3,sp );
	}
	if ( ccnt>0 ) {
	    if ( tok==pt_closecurly )
		--ccnt;
	    else if ( tok==pt_opencurly )
		++ccnt;
	    if ( ccnt>0 )
		AddTok(&gb,tokbuf,tok==pt_namelit);
	    else {
		if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		    stack[sp].type = ps_instr;
		    if ( gb.pt==NULL )
			stack[sp++].u.str = copy("");
		    else {
			*gb.pt = '\0'; gb.pt = gb.base;
			stack[sp++].u.str = copy((char *)gb.base);
		    }
		}
	    }
	} else if ( tok==pt_unknown && (kv=lookup(&dict,tokbuf))!=NULL ) {
	    if ( kv->type == ps_instr )
		pushio(wrapper,NULL,copy(kv->u.str),0);
	    else if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = kv->type;
		stack[sp++].u = kv->u;
		if ( kv->type==ps_instr || kv->type==ps_lit || kv->type==ps_string )
		    stack[sp-1].u.str = copy(stack[sp-1].u.str);
		else if ( kv->type==ps_array || kv->type==ps_dict ) {
		    copyarray(&stack[sp-1].u.dict,&stack[sp-1].u.dict,&tofrees);
		    if ( stack[sp-1].u.dict.is_executable )
			sp = aload(sp,stack,sizeof(stack)/sizeof(stack[0]),&tofrees);
		}
	    }
	} else {
	if ( tok==pt_unknown ) {
	    if ( strcmp(tokbuf,"Cache")==0 )	/* Fontographer type3s */
		tok = pt_setcachedevice;
	    else if ( strcmp(tokbuf,"SetWid")==0 ) {
		tok = pt_setcharwidth;
		if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		    stack[sp].type = ps_num;
		    stack[sp++].u.val = 0;
		}
	    } else if ( strcmp(tokbuf,"rrcurveto")==0 ) {
		if ( sp>=6 ) {
		    stack[sp-4].u.val += stack[sp-6].u.val;
		    stack[sp-3].u.val += stack[sp-5].u.val;
		    stack[sp-2].u.val += stack[sp-4].u.val;
		    stack[sp-1].u.val += stack[sp-3].u.val;
		    tok = pt_rcurveto;
		}
	    } else if ( strcmp(tokbuf,"FillStroke")==0 ) {
		if ( sp>0 )
		    --sp;
		tok = linewidth!=WIDTH_INHERITED ? pt_stroke : pt_fill;
		if ( wrapper->top!=NULL && wrapper->top->ps!=NULL &&
			linewidth!=WIDTH_INHERITED )
		    linewidth /= 10.0;	/* bug in Fontographer's unencrypted type3 fonts */
	    } else if ( strcmp(tokbuf,"SG")==0 ) {
		if ( linewidth!=WIDTH_INHERITED && sp>1 )
		    stack[sp-2].u.val = stack[sp-1].u.val;
		if ( sp>0 )
		    --sp;
		if ( sp>0 )
		    stack[sp-1].u.val = (stack[sp-1].u.val+99)/198.0;
		tok = pt_setgray;
	    } else if ( strcmp(tokbuf,"ShowInt")==0 ) {
	    	/* Fontographer reference */
		if ( (!wrapper->top->fogns && sp>0 && stack[sp-1].type == ps_num &&
			 stack[sp-1].u.val>=0 && stack[sp-1].u.val<=255 ) ||
			(wrapper->top->fogns && sp>6 && stack[sp-7].type == ps_num &&
			 stack[sp-7].u.val>=0 && stack[sp-7].u.val<=255 )) {
		    ref = RefCharCreate();
		    memcpy(ref->transform,transform,sizeof(ref->transform));
		    if ( wrapper->top->fogns ) {
			sp -= 6;
			t[0] = stack[sp+0].u.val;
			t[1] = stack[sp+1].u.val;
			t[2] = stack[sp+2].u.val;
			t[3] = stack[sp+3].u.val;
			t[4] = stack[sp+4].u.val;
			t[5] = stack[sp+5].u.val;
			MatMultiply(t,ref->transform,ref->transform);
			wrapper->top->fogns = false;
		    }
		    ref->orig_pos = stack[--sp].u.val;
		    ref->next = ec->refs;
		    ec->refs = ref;
    continue;
		}
	    } else if ( strcmp(tokbuf,"togNS_")==0 ) {
		wrapper->top->fogns = !wrapper->top->fogns;
    continue;
	    }
	}
	switch ( tok ) {
	  case pt_number:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_num;
		stack[sp++].u.val = dval;
	    }
	  break;
	  case pt_string:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_string;
		stack[sp++].u.str = copyn(tokbuf+1,strlen(tokbuf)-2);
	    }
	  break;
	  case pt_true: case pt_false:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_bool;
		stack[sp++].u.tf = tok==pt_true;
	    }
	  break;
	  case pt_opencurly:
	    ++ccnt;
	  break;
	  case pt_closecurly:
	    --ccnt;
	    if ( ccnt<0 ) {
 goto done;
	    }
	  break;
	  case pt_count:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_num;
		stack[sp].u.val = sp;
		++sp;
	    }
	  break;
	  case pt_pop:
	    if ( sp>0 ) {
		--sp;
		if ( stack[sp].type==ps_string || stack[sp].type==ps_instr ||
			stack[sp].type==ps_lit )
		    free(stack[sp].u.str);
		else if ( stack[sp].type==ps_array || stack[sp].type==ps_dict )
		    dictfree(&stack[sp].u.dict);
	    }
	  break;
	  case pt_clear:
	    while ( sp>0 ) {
		--sp;
		if ( stack[sp].type==ps_string || stack[sp].type==ps_instr ||
			stack[sp].type==ps_lit )
		    free(stack[sp].u.str);
		else if ( stack[sp].type==ps_array || stack[sp].type==ps_dict )
		    dictfree(&stack[sp].u.dict);
	    }
	  break;
	  case pt_dup:
	    if ( sp>0 && sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp] = stack[sp-1];
		if ( stack[sp].type==ps_string || stack[sp].type==ps_instr ||
			stack[sp].type==ps_lit )
		    stack[sp].u.str = copy(stack[sp].u.str);
    /* The following is incorrect behavior, but as I don't do garbage collection */
    /*  and I'm not going to implement reference counts, this will work in most cases */
		else if ( stack[sp].type==ps_array )
		    copyarray(&stack[sp].u.dict,&stack[sp].u.dict,&tofrees);
		++sp;
	    }
	  break;
	  case pt_copy:
	    if ( sp>0 ) {
		int n = stack[--sp].u.val;
		if ( n+sp<sizeof(stack)/sizeof(stack[0]) ) {
		    int i;
		    for ( i=0; i<n; ++i ) {
			stack[sp] = stack[sp-n];
			if ( stack[sp].type==ps_string || stack[sp].type==ps_instr ||
				stack[sp].type==ps_lit )
			    stack[sp].u.str = copy(stack[sp].u.str);
    /* The following is incorrect behavior, but as I don't do garbage collection */
    /*  and I'm not going to implement reference counts, this will work in most cases */
			else if ( stack[sp].type==ps_array )
			    copyarray(&stack[sp].u.dict,&stack[sp].u.dict,&tofrees);
			++sp;
		    }
		}
	    }
	  break;
	  case pt_exch:
	    if ( sp>1 ) {
		struct psstack temp;
		temp = stack[sp-1];
		stack[sp-1] = stack[sp-2];
		stack[sp-2] = temp;
	    }
	  break;
	  case pt_roll:
	    sp = rollstack(stack,sp);
	  break;
	  case pt_index:
	    if ( sp>0 ) {
		i = stack[--sp].u.val;
		if ( i>=0 && sp>(unsigned)i ) {
		    stack[sp] = stack[sp-i-1];
		    if ( stack[sp].type==ps_string || stack[sp].type==ps_instr ||
			    stack[sp].type==ps_lit )
			stack[sp].u.str = copy(stack[sp].u.str);
    /* The following is incorrect behavior, but as I don't do garbage collection */
    /*  and I'm not going to implement reference counts, this will work in most cases */
		    else if ( stack[sp].type==ps_array )
			copyarray(&stack[sp].u.dict,&stack[sp].u.dict,&tofrees);
		    ++sp;
		}
	    }
	  break;
	  case pt_add:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		stack[sp-2].u.val += stack[sp-1].u.val;
		--sp;
	    }
	  break;
	  case pt_sub:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		stack[sp-2].u.val -= stack[sp-1].u.val;
		--sp;
	    }
	  break;
	  case pt_mul:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		stack[sp-2].u.val *= stack[sp-1].u.val;
		--sp;
	    }
	  break;
	  case pt_div:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		if ( stack[sp-1].u.val == 0 )
		    LogError( _("Divide by zero in postscript code.\n" ));
		else
		    stack[sp-2].u.val /= stack[sp-1].u.val;
		--sp;
	    }
	  break;
	  case pt_idiv:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		if ( stack[sp-1].u.val == 0 )
		    LogError( _("Divide by zero in postscript code.\n" ));
		else
		    stack[sp-2].u.val = ((int) stack[sp-2].u.val) / ((int) stack[sp-1].u.val);
		--sp;
	    }
	  break;
	  case pt_mod:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		if ( stack[sp-1].u.val == 0 )
		    LogError( _("Divide by zero in postscript code.\n" ));
		else
		    stack[sp-2].u.val = ((int) stack[sp-2].u.val) % ((int) stack[sp-1].u.val);
		--sp;
	    }
	  break;
	  case pt_max:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		if ( stack[sp-2].u.val < stack[sp-1].u.val )
		    stack[sp-2].u.val = stack[sp-1].u.val;
		--sp;
	    }
	  break;
	  case pt_min:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		if ( stack[sp-2].u.val > stack[sp-1].u.val )
		    stack[sp-2].u.val = stack[sp-1].u.val;
		--sp;
	    }
	  break;
	  case pt_neg:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type == ps_num )
		    stack[sp-1].u.val = -stack[sp-1].u.val;
	    }
	  break;
	  case pt_abs:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type == ps_num )
		    if ( stack[sp-1].u.val < 0 )
			stack[sp-1].u.val = -stack[sp-1].u.val;
	    }
	  break;
	  case pt_round:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type == ps_num )
		    stack[sp-1].u.val = rint(stack[sp-1].u.val);
		    /* rint isn't quite right, round will take 6.5 to 7, 5.5 to 6, etc. while rint() will take both to 6 */
	    }
	  break;
	  case pt_floor:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type == ps_num )
		    stack[sp-1].u.val = floor(stack[sp-1].u.val);
	    }
	  break;
	  case pt_ceiling:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type == ps_num )
		    stack[sp-1].u.val = ceil(stack[sp-1].u.val);
	    }
	  break;
	  case pt_truncate:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type == ps_num ) {
		    if ( stack[sp-1].u.val<0 )
			stack[sp-1].u.val = ceil(stack[sp-1].u.val);
		    else
			stack[sp-1].u.val = floor(stack[sp-1].u.val);
		}
	    }
	  break;
	  case pt_ne: case pt_eq:
	    if ( sp>=2 ) {
		if ( stack[sp-2].type!=stack[sp-1].type )
		    stack[sp-2].u.tf = false;
		else if ( stack[sp-2].type==ps_num )
		    stack[sp-2].u.tf = (stack[sp-2].u.val == stack[sp-1].u.val);
		else if ( stack[sp-2].type==ps_bool )
		    stack[sp-2].u.tf = (stack[sp-2].u.tf == stack[sp-1].u.tf);
		else
		    stack[sp-2].u.tf = strcmp(stack[sp-2].u.str,stack[sp-1].u.str)==0 ;
		stack[sp-2].type = ps_bool;
		if ( tok==pt_ne ) stack[sp-2].u.tf = !stack[sp-2].u.tf;
		--sp;
	    }
	  break;
	  case pt_gt: case pt_le: case pt_lt: case pt_ge:
	    if ( sp>=2 ) {
		if ( stack[sp-2].type!=stack[sp-1].type )
		    stack[sp-2].u.tf = false;
		else if ( stack[sp-2].type==ps_array )
		    LogError( _("Can't compare arrays\n" ));
		else {
		    int cmp;
		    if ( stack[sp-2].type==ps_num )
			cmp = (stack[sp-2].u.val > stack[sp-1].u.val)?1:
				(stack[sp-2].u.val == stack[sp-1].u.val)?0:-1;
		    else if ( stack[sp-2].type==ps_bool )
			cmp = (stack[sp-2].u.tf - stack[sp-1].u.tf);
		    else
			cmp = strcmp(stack[sp-2].u.str,stack[sp-1].u.str);
		    if ( tok==pt_gt )
			stack[sp-2].u.tf = cmp>0;
		    else if ( tok==pt_lt )
			stack[sp-2].u.tf = cmp<0;
		    else if ( tok==pt_le )
			stack[sp-2].u.tf = cmp<=0;
		    else
			stack[sp-2].u.tf = cmp>=0;
		}
		stack[sp-2].type = ps_bool;
		--sp;
	    }
	  break;
	  case pt_not:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type == ps_bool )
		    stack[sp-1].u.tf = !stack[sp-1].u.tf;
	    }
	  break;
	  case pt_and:
	    if ( sp>=2 ) {
		if ( stack[sp-2].type == ps_num )
		    stack[sp-2].u.val = ((int) stack[sp-2].u.val) & (int) stack[sp-1].u.val;
		else if ( stack[sp-2].type == ps_bool )
		    stack[sp-2].u.tf &= stack[sp-1].u.tf;
		--sp;
	    }
	  break;
	  case pt_or:
	    if ( sp>=2 ) {
		if ( stack[sp-2].type == ps_num )
		    stack[sp-2].u.val = ((int) stack[sp-2].u.val) | (int) stack[sp-1].u.val;
		else if ( stack[sp-2].type == ps_bool )
		    stack[sp-2].u.tf |= stack[sp-1].u.tf;
		--sp;
	    }
	  break;
	  case pt_xor:
	    if ( sp>=2 ) {
		if ( stack[sp-2].type == ps_num )
		    stack[sp-2].u.val = ((int) stack[sp-2].u.val) ^ (int) stack[sp-1].u.val;
		else if ( stack[sp-2].type == ps_bool )
		    stack[sp-2].u.tf ^= stack[sp-1].u.tf;
		--sp;
	    }
	  break;
	  case pt_exp:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		stack[sp-2].u.val = pow(stack[sp-2].u.val,stack[sp-1].u.val);
		--sp;
	    }
	  break;
	  case pt_sqrt:
	    if ( sp>=1 && stack[sp-1].type==ps_num ) {
		stack[sp-1].u.val = sqrt(stack[sp-1].u.val);
	    }
	  break;
	  case pt_ln:
	    if ( sp>=1 && stack[sp-1].type==ps_num ) {
		stack[sp-1].u.val = log(stack[sp-1].u.val);
	    }
	  break;
	  case pt_log:
	    if ( sp>=1 && stack[sp-1].type==ps_num ) {
		stack[sp-1].u.val = log10(stack[sp-1].u.val);
	    }
	  break;
	  case pt_atan:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_num ) {
		stack[sp-2].u.val = atan2(stack[sp-2].u.val,stack[sp-1].u.val)*
			180/3.1415926535897932;
		--sp;
	    }
	  break;
	  case pt_sin:
	    if ( sp>=1 && stack[sp-1].type==ps_num ) {
		stack[sp-1].u.val = sin(stack[sp-1].u.val*3.1415926535897932/180);
	    }
	  break;
	  case pt_cos:
	    if ( sp>=1 && stack[sp-1].type==ps_num ) {
		stack[sp-1].u.val = cos(stack[sp-1].u.val*3.1415926535897932/180);
	    }
	  break;
	  case pt_if:
	    if ( sp>=2 ) {
		if ( ((stack[sp-2].type == ps_bool && stack[sp-2].u.tf) ||
			(stack[sp-2].type == ps_num && strstr(stack[sp-1].u.str,"setcachedevice")!=NULL)) &&
			stack[sp-1].type==ps_instr )
		    pushio(wrapper,NULL,stack[sp-1].u.str,0);
		if ( stack[sp-1].type==ps_string || stack[sp-1].type==ps_instr || stack[sp-1].type==ps_lit )
		    free(stack[sp-1].u.str);
		sp -= 2;
	    } else if ( sp==1 && stack[sp-1].type==ps_instr ) {
		/*This can happen when reading our type3 fonts, we get passed */
		/* values on the stack which the interpreter knows nothing */
		/* about, but the interp needs to learn the width of the char */
		if ( strstr(stack[sp-1].u.str,"setcachedevice")!=NULL ||
			strstr(stack[sp-1].u.str,"setcharwidth")!=NULL )
		    pushio(wrapper,NULL,stack[sp-1].u.str,0);
		free(stack[sp-1].u.str);
		sp = 0;
	    }
	  break;
	  case pt_ifelse:
	    if ( sp>=3 ) {
		if ( stack[sp-3].type == ps_bool && stack[sp-3].u.tf ) {
		    if ( stack[sp-2].type==ps_instr )
			pushio(wrapper,NULL,stack[sp-2].u.str,0);
		} else {
		    if ( stack[sp-1].type==ps_instr )
			pushio(wrapper,NULL,stack[sp-1].u.str,0);
		}
		if ( stack[sp-1].type==ps_string || stack[sp-1].type==ps_instr || stack[sp-1].type==ps_lit )
		    free(stack[sp-1].u.str);
		if ( stack[sp-2].type==ps_string || stack[sp-2].type==ps_instr || stack[sp-2].type==ps_lit )
		    free(stack[sp-2].u.str);
		sp -= 3;
	    }
	  break;
	  case pt_for:
	    if ( sp>=4 ) {
		real init, incr, limit;
		char *func;
		int cnt;

		if ( stack[sp-4].type == ps_num && stack[sp-3].type==ps_num &&
			stack[sp-2].type==ps_num && stack[sp-1].type==ps_instr ) {
		    init = stack[sp-4].u.val;
		    incr = stack[sp-3].u.val;
		    limit = stack[sp-2].u.val;
		    func = stack[sp-1].u.str;
		    sp -= 4;
		    cnt = 0;
		    if ( incr>0 ) {
			while ( init<=limit ) { ++cnt; init += incr; }
		    } else if ( incr<0 ) {
			while ( init>=limit ) { ++cnt; init += incr; }
		    }
		    pushio(wrapper,NULL,func,cnt);
		    free(func);
		}
	    }
	  break;
	  case pt_loop:
	    if ( sp>=1 ) {
		char *func;
		int cnt;

		if ( stack[sp-1].type==ps_instr ) {
		    cnt = 0x7fffffff;		/* Loop for ever */
		    func = stack[sp-1].u.str;
		    --sp;
		    pushio(wrapper,NULL,func,cnt);
		    free(func);
		}
	    }
	  break;
	  case pt_repeat:
	    if ( sp>=2 ) {
		char *func;
		int cnt;

		if ( stack[sp-2].type==ps_num && stack[sp-1].type==ps_instr ) {
		    cnt = stack[sp-2].u.val;
		    func = stack[sp-1].u.str;
		    sp -= 2;
		    pushio(wrapper,NULL,func,cnt);
		    free(func);
		}
	    }
	  break;
	  case pt_exit:
	    ioescapeloop(wrapper);
	  break;
	  case pt_stopped:
	    if ( sp>=1 ) {
		char *func;

		if ( stack[sp-1].type==ps_instr ) {
		    func = stack[sp-1].u.str;
		    --sp;
		    pushio(wrapper,NULL,func,-1);
		    free(func);
		}
	    }
	  break;
	  case pt_stop:
	    sp = ioescapestopped(wrapper,stack,sp,sizeof(stack)/sizeof(stack[0]));
	  break;
	  case pt_load:
	    if ( sp>=1 && stack[sp-1].type==ps_lit ) {
		kv = lookup(&dict,stack[sp-1].u.str);
		if ( kv!=NULL ) {
		    free( stack[sp-1].u.str );
		    stack[sp-1].type = kv->type;
		    stack[sp-1].u = kv->u;
		    if ( kv->type==ps_instr || kv->type==ps_lit )
			stack[sp-1].u.str = copy(stack[sp-1].u.str);
		} else
		    stack[sp-1].type = ps_instr;
	    }
	  break;
	  case pt_def:
	    sp = AddEntry(&dict,stack,sp);
	  break;
	  case pt_bind:
	    /* a noop in this context */
	  break;
	  case pt_setcachedevice:
	    if ( sp>=6 ) {
		ec->width = stack[sp-6].u.val;
		ec->vwidth = stack[sp-5].u.val;
		/* I don't care about the bounding box */
		sp-=6;
	    }
	  break;
	  case pt_setcharwidth:
	    if ( sp>=2 )
		ec->width = stack[sp-=2].u.val;
	  break;
	  case pt_translate:
	    if ( sp>=1 && stack[sp-1].type==ps_array )
		sp = DoMatOp(tok,sp,stack);
	    else if ( sp>=2 ) {
		transform[4] += stack[sp-2].u.val*transform[0]+stack[sp-1].u.val*transform[2];
		transform[5] += stack[sp-2].u.val*transform[1]+stack[sp-1].u.val*transform[3];
		sp -= 2;
	    }
	  break;
	  case pt_scale:
	    if ( sp>=1 && stack[sp-1].type==ps_array )
		sp = DoMatOp(tok,sp,stack);
	    else if ( sp>=2 ) {
		transform[0] *= stack[sp-2].u.val;
		transform[1] *= stack[sp-2].u.val;
		transform[2] *= stack[sp-1].u.val;
		transform[3] *= stack[sp-1].u.val;
		/* transform[4,5] are unchanged */
		sp -= 2;
	    }
	  break;
	  case pt_rotate:
	    if ( sp>=1 && stack[sp-1].type==ps_array )
		sp = DoMatOp(tok,sp,stack);
	    else if ( sp>=1 ) {
		--sp;
		t[0] = t[3] = cos(stack[sp].u.val);
		t[1] = sin(stack[sp].u.val);
		t[2] = -t[1];
		t[4] = t[5] = 0;
		MatMultiply(t,transform,transform);
	    }
	  break;
	  case pt_concat:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type==ps_array ) {
		    if ( stack[sp-1].u.dict.cnt==6 && stack[sp-1].u.dict.entries[0].type==ps_num ) {
			--sp;
			t[5] = stack[sp].u.dict.entries[5].u.val;
			t[4] = stack[sp].u.dict.entries[4].u.val;
			t[3] = stack[sp].u.dict.entries[3].u.val;
			t[2] = stack[sp].u.dict.entries[2].u.val;
			t[1] = stack[sp].u.dict.entries[1].u.val;
			t[0] = stack[sp].u.dict.entries[0].u.val;
			dictfree(&stack[sp].u.dict);
			MatMultiply(t,transform,transform);
		    }
		}
	    }
	  break;
	  case pt_transform:
	    if ( sp>=1 && stack[sp-1].type==ps_array ) {
		if ( sp>=3 ) {
		    DoMatTransform(tok,sp,stack);
		    --sp;
		}
	    } else if ( sp>=2 ) {
		double x = stack[sp-2].u.val, y = stack[sp-1].u.val;
		stack[sp-2].u.val = transform[0]*x + transform[1]*y + transform[4];
		stack[sp-1].u.val = transform[2]*x + transform[3]*y + transform[5];
	    }
	  break;
	  case pt_itransform:
	    if ( sp>=1 && stack[sp-1].type==ps_array ) {
		if ( sp>=3 ) {
		    DoMatTransform(tok,sp,stack);
		    --sp;
		}
	    } else if ( sp>=2 ) {
		double x = stack[sp-2].u.val, y = stack[sp-1].u.val;
		MatInverse(t,transform);
		stack[sp-2].u.val = t[0]*x + t[1]*y + t[4];
		stack[sp-1].u.val = t[2]*x + t[3]*y + t[5];
	    }
	  break;
	  case pt_dtransform:
	    if ( sp>=1 && stack[sp-1].type==ps_array ) {
		if ( sp>=3 ) {
		    DoMatTransform(tok,sp,stack);
		    --sp;
		}
	    } else if ( sp>=2 ) {
		double x = stack[sp-2].u.val, y = stack[sp-1].u.val;
		stack[sp-2].u.val = transform[0]*x + transform[1]*y;
		stack[sp-1].u.val = transform[2]*x + transform[3]*y;
	    }
	  break;
	  case pt_idtransform:
	    if ( sp>=1 && stack[sp-1].type==ps_array ) {
		if ( sp>=3 ) {
		    DoMatTransform(tok,sp,stack);
		    --sp;
		}
	    } else if ( sp>=2 ) {
		double x = stack[sp-2].u.val, y = stack[sp-1].u.val;
		MatInverse(t,transform);
		stack[sp-2].u.val = t[0]*x + t[1]*y;
		stack[sp-1].u.val = t[2]*x + t[3]*y;
	    }
	  break;
	  case pt_namelit:
	    if ( strcmp(tokbuf,"CharProcs")==0 && ec!=NULL ) {
		HandleType3Reference(wrapper,ec,transform,tokbuf,tokbufsize);
	    } else if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_lit;
		stack[sp++].u.str = copy(tokbuf);
	    }
	  break;
	  case pt_exec:
	    if ( sp>0 && stack[sp-1].type == ps_lit ) {
		ref = RefCharCreate();
		ref->sc = (SplineChar *) stack[--sp].u.str;
		memcpy(ref->transform,transform,sizeof(transform));
		if ( ec->refs==NULL )
		    ec->refs = ref;
		else
		    lastref->next = ref;
		lastref = ref;
	    }
	  break;
	  case pt_newpath:
	    SplinePointListsFree(head);
	    head = NULL;
	    cur = NULL;
	  break;
	  case pt_lineto: case pt_rlineto:
	  case pt_moveto: case pt_rmoveto:
	    if ( sp>=2 || tok==pt_newpath ) {
		if ( tok==pt_rlineto || tok==pt_rmoveto ) {
		    current.x += stack[sp-2].u.val;
		    current.y += stack[sp-1].u.val;
		    sp -= 2;
		} else if ( tok==pt_lineto || tok == pt_moveto ) {
		    current.x = stack[sp-2].u.val;
		    current.y = stack[sp-1].u.val;
		    sp -= 2;
		}
		pt = chunkalloc(sizeof(SplinePoint));
		Transform(&pt->me,&current,transform);
		pt->noprevcp = true; pt->nonextcp = true;
		if ( tok==pt_moveto || tok==pt_rmoveto ) {
		    SplinePointList *spl = chunkalloc(sizeof(SplinePointList));
		    spl->first = spl->last = pt;
		    if ( cur!=NULL )
			cur->next = spl;
		    else
			head = spl;
		    cur = spl;
		} else {
		    if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
			CheckMake(cur->last,pt);
			SplineMake3(cur->last,pt);
			cur->last = pt;
		    }
		}
	    } else
		sp = 0;
	  break;
	  case pt_curveto: case pt_rcurveto:
	    if ( sp>=6 ) {
		if ( tok==pt_rcurveto ) {
		    stack[sp-1].u.val += current.y;
		    stack[sp-3].u.val += current.y;
		    stack[sp-5].u.val += current.y;
		    stack[sp-2].u.val += current.x;
		    stack[sp-4].u.val += current.x;
		    stack[sp-6].u.val += current.x;
		}
		current.x = stack[sp-2].u.val;
		current.y = stack[sp-1].u.val;
		if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
		    temp.x = stack[sp-6].u.val; temp.y = stack[sp-5].u.val;
		    Transform(&cur->last->nextcp,&temp,transform);
		    cur->last->nonextcp = false;
		    pt = chunkalloc(sizeof(SplinePoint));
		    temp.x = stack[sp-4].u.val; temp.y = stack[sp-3].u.val;
		    Transform(&pt->prevcp,&temp,transform);
		    Transform(&pt->me,&current,transform);
		    pt->nonextcp = true;
		    CheckMake(cur->last,pt);
		    SplineMake3(cur->last,pt);
		    cur->last = pt;
		}
		sp -= 6;
	    } else
		sp = 0;
	  break;
	  case pt_arc: case pt_arcn:
	    if ( sp>=5 ) {
		real cx, cy, r, a1, a2;
		cx = stack[sp-5].u.val;
		cy = stack[sp-4].u.val;
		r = stack[sp-3].u.val;
		a1 = stack[sp-2].u.val;
		a2 = stack[sp-1].u.val;
		sp -= 5;
		temp.x = cx+r*cos(a1/180 * 3.1415926535897932);
		temp.y = cy+r*sin(a1/180 * 3.1415926535897932);
		if ( temp.x!=current.x || temp.y!=current.y ||
			!( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) )) {
		    pt = chunkalloc(sizeof(SplinePoint));
		    Transform(&pt->me,&temp,transform);
		    pt->noprevcp = true; pt->nonextcp = true;
		    if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
			CheckMake(cur->last,pt);
			SplineMake3(cur->last,pt);
			cur->last = pt;
		    } else {	/* if no current point, then start here */
			SplinePointList *spl = chunkalloc(sizeof(SplinePointList));
			spl->first = spl->last = pt;
			if ( cur!=NULL )
			    cur->next = spl;
			else
			    head = spl;
			cur = spl;
		    }
		}
		circlearcsto(a1,a2,cx,cy,r,cur,transform,tok==pt_arcn);
		current.x = cx+r*cos(a2/180 * 3.1415926535897932);
		current.y = cy+r*sin(a2/180 * 3.1415926535897932);
	    } else
		sp = 0;
	  break;
	  case pt_arct: case pt_arcto:
	    if ( sp>=5 ) {
		real x1, y1, x2, y2, r;
		real xt1, xt2, yt1, yt2;
		x1 = stack[sp-5].u.val;
		y1 = stack[sp-4].u.val;
		x2 = stack[sp-3].u.val;
		y2 = stack[sp-2].u.val;
		r = stack[sp-1].u.val;
		sp -= 5;

		xt1 = xt2 = x1; yt1 = yt2 = y1;
		if ( cur==NULL || cur->first==NULL || (cur->first==cur->last && cur->first->next!=NULL) )
		    /* Error */;
		else if ( current.x==x1 && current.y==y1 )
		    /* Error */;
		else if (( x1==x2 && y1==y2 ) ||
			(current.x-x1)*(y2-y1) == (x2-x1)*(current.y-y1) ) {
		    /* Degenerate case */
		    current.x = x1; current.y = y1;
		    pt = chunkalloc(sizeof(SplinePoint));
		    Transform(&pt->me,&current,transform);
		    pt->noprevcp = true; pt->nonextcp = true;
		    CheckMake(cur->last,pt);
		    SplineMake3(cur->last,pt);
		    cur->last = pt;
		} else {
		    real l1 = sqrt((current.x-x1)*(current.x-x1)+(current.y-y1)*(current.y-y1));
		    real l2 = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
		    real dx = ((current.x-x1)/l1 + (x2-x1)/l2);
		    real dy = ((current.y-y1)/l1 + (y2-y1)/l2);
		    /* the line from (x1,y1) to (x1+dx,y1+dy) contains the center*/
		    real l3 = sqrt(dx*dx+dy*dy);
		    real cx, cy, t, tmid;
		    real a1, amid, a2;
		    int clockwise = true;
		    dx /= l3; dy /= l3;
		    a1 = atan2(current.y-y1,current.x-x1);
		    a2 = atan2(y2-y1,x2-x1);
		    amid = atan2(dy,dx) - a1;
		    tmid = r/sin(amid);
		    t = r/tan(amid);
		    if ( t<0 ) {
			clockwise = false;
			t = -t;
			tmid = -tmid;
		    }
		    cx = x1+ tmid*dx; cy = y1 + tmid*dy;
		    xt1 = x1 + t*(current.x-x1)/l1; yt1 = y1 + t*(current.y-y1)/l1;
		    xt2 = x1 + t*(x2-x1)/l2; yt2 = y1 + t*(y2-y1)/l2;
		    if ( xt1!=current.x || yt1!=current.y ) {
			DBasePoint temp;
			temp.x = xt1; temp.y = yt1;
			pt = chunkalloc(sizeof(SplinePoint));
			Transform(&pt->me,&temp,transform);
			pt->noprevcp = true; pt->nonextcp = true;
			CheckMake(cur->last,pt);
			SplineMake3(cur->last,pt);
			cur->last = pt;
		    }
		    a1 = 3*3.1415926535897932/2+a1;
		    a2 = 3.1415926535897932/2+a2;
		    if ( !clockwise ) {
			a1 += 3.1415926535897932;
			a2 += 3.1415926535897932;
		    }
		    circlearcsto(a1*180/3.1415926535897932,a2*180/3.1415926535897932,
			    cx,cy,r,cur,transform,clockwise);
		}
		if ( tok==pt_arcto ) {
		    stack[sp].type = stack[sp+1].type = stack[sp+2].type = stack[sp+3].type = ps_num;
		    stack[sp++].u.val = xt1;
		    stack[sp++].u.val = yt1;
		    stack[sp++].u.val = xt2;
		    stack[sp++].u.val = yt2;
		}
		current.x = xt2; current.y = yt2;
	    }
	  break;
	  case pt_closepath:
	    if ( cur!=NULL && cur->first!=NULL && cur->first!=cur->last ) {
		if ( RealNear(cur->first->me.x,cur->last->me.x) && RealNear(cur->first->me.y,cur->last->me.y) ) {
		    SplinePoint *oldlast = cur->last;
		    cur->first->prevcp = oldlast->prevcp;
		    cur->first->prevcp.x += (cur->first->me.x-oldlast->me.x);
		    cur->first->prevcp.y += (cur->first->me.y-oldlast->me.y);
		    cur->first->noprevcp = oldlast->noprevcp;
		    oldlast->prev->from->next = NULL;
		    cur->last = oldlast->prev->from;
		    SplineFree(oldlast->prev);
		    SplinePointFree(oldlast);
		}
		CheckMake(cur->last,cur->first);
		SplineMake3(cur->last,cur->first);
		cur->last = cur->first;
	    }
	  break;
	  case pt_setlinecap:
	    if ( sp>=1 )
		linecap = stack[--sp].u.val;
	  break;
	  case pt_setlinejoin:
	    if ( sp>=1 )
		linejoin = stack[--sp].u.val;
	  break;
	  case pt_setlinewidth:
	    if ( sp>=1 )
		linewidth = stack[--sp].u.val;
	  break;
	  case pt_setdash:
	    if ( sp>=2 && stack[sp-1].type==ps_num && stack[sp-2].type==ps_array ) {
		sp -= 2;
		dash_offset = stack[sp+1].u.val;
		for ( i=0; i<DASH_MAX && i<stack[sp].u.dict.cnt; ++i )
		    dashes[i] = stack[sp].u.dict.entries[i].u.val;
		dictfree(&stack[sp].u.dict);
	    }
	  break;
	  case pt_currentlinecap: case pt_currentlinejoin:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_num;
		stack[sp++].u.val = tok==pt_currentlinecap?linecap:linejoin;
	    }
	  break;
	  case pt_currentlinewidth:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_num;
		stack[sp++].u.val = linewidth;
	    }
	  break;
	  case pt_currentdash:
	    if ( sp+1<sizeof(stack)/sizeof(stack[0]) ) {
		struct pskeydict dict;
		for ( i=0; i<DASH_MAX && dashes[i]!=0; ++i );
		dict.cnt = dict.max = i;
		dict.entries = calloc(i,sizeof(struct pskeyval));
                dict.is_executable = false;
		for ( j=0; j<i; ++j ) {
		    dict.entries[j].type = ps_num;
		    dict.entries[j].u.val = dashes[j];
		}
		stack[sp].type = ps_array;
		stack[sp++].u.dict = dict;
		stack[sp].type = ps_num;
		stack[sp++].u.val = dash_offset;
	    }
	  break;
	  case pt_currentgray:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_num;
		stack[sp++].u.val = (3*((fore>>16)&0xff) + 6*((fore>>8)&0xff) + (fore&0xff))/2550.;
	    }
	  break;
	  case pt_setgray:
	    if ( sp>=1 ) {
		fore = stack[--sp].u.val*255;
		fore *= 0x010101;
	    }
	  break;
	  case pt_setrgbcolor:
	    if ( sp>=3 ) {
		fore = (((int) (stack[sp-3].u.val*255))<<16) +
			(((int) (stack[sp-2].u.val*255))<<8) +
			(int) (stack[sp-1].u.val*255);
		sp -= 3;
	    }
	  break;
	  case pt_currenthsbcolor: case pt_currentrgbcolor:
	    if ( sp+2<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = stack[sp+1].type = stack[sp+2].type = ps_num;
		if ( tok==pt_currentrgbcolor ) {
		    stack[sp++].u.val = ((fore>>16)&0xff)/255.;
		    stack[sp++].u.val = ((fore>>8)&0xff)/255.;
		    stack[sp++].u.val = (fore&0xff)/255.;
		} else {
		    int r=fore>>16, g=(fore>>8)&0xff, bl=fore&0xff;
		    int mx, mn;
		    real h, s, b;
		    mx = mn = r;
		    if ( mx>g ) mn=g; else mx=g;
		    if ( mx<bl ) mx = bl; if ( mn>bl ) mn = bl;
		    b = mx/255.;
		    s = h = 0;
		    if ( mx>0 )
			s = ((real) (mx-mn))/mx;
		    if ( s!=0 ) {
			real rdiff = ((real) (mx-r))/(mx-mn);
			real gdiff = ((real) (mx-g))/(mx-mn);
			real bdiff = ((real) (mx-bl))/(mx-mn);
			if ( rdiff==0 )
			    h = bdiff-gdiff;
			else if ( gdiff==0 )
			    h = 2 + rdiff-bdiff;
			else
			    h = 4 + gdiff-rdiff;
			h /= 6;
			if ( h<0 ) h += 1;
		    }
		    stack[sp++].u.val = h;
		    stack[sp++].u.val = s;
		    stack[sp++].u.val = b;
		}
	    }
	  break;
	  case pt_sethsbcolor:
	    if ( sp>=3 ) {
		real h = stack[sp-3].u.val, s = stack[sp-2].u.val, b = stack[sp-1].u.val;
		int r,g,bl;
		if ( s==0 )	/* it's grey */
		    fore = ((int) (b*255)) * 0x010101;
		else {
		    real sextant = (h-floor(h))*6;
		    real mod = sextant-floor(sextant);
		    real p = b*(1-s), q = b*(1-s*mod), t = b*(1-s*(1-mod));
		    switch( (int) sextant) {
		      case 0:
			r = b*255.; g = t*255.; bl = p*255.;
		      break;
		      case 1:
			r = q*255.; g = b*255.; bl = p*255.;
		      break;
		      case 2:
			r = p*255.; g = b*255.; bl = t*255.;
		      break;
		      case 3:
			r = p*255.; g = q*255.; bl = b*255.;
		      break;
		      case 4:
			r = t*255.; g = p*255.; bl = b*255.;
		      break;
		      case 5:
			r = b*255.; g = p*255.; bl = q*255.;
		      break;
		      default:
		      break;
		    }
		    fore = COLOR_CREATE(r,g,bl);
		}
		sp -= 3;
	    }
	  break;
	  case pt_currentcmykcolor:
	    if ( sp+3<sizeof(stack)/sizeof(stack[0]) ) {
		real c,m,y,k;
		stack[sp].type = stack[sp+1].type = stack[sp+2].type = stack[sp+3].type = ps_num;
		y = 1.-(fore&0xff)/255.;
		m = 1.-((fore>>8)&0xff)/255.;
		c = 1.-((fore>>16)&0xff)/255.;
		k = y; if ( k>m ) k=m; if ( k>c ) k=c;
		if ( k!=1 ) {
		    y = (y-k)/(1-k);
		    m = (m-k)/(1-k);
		    c = (c-k)/(1-k);
		} else
		    y = m = c = 0;
		stack[sp++].u.val = c;
		stack[sp++].u.val = m;
		stack[sp++].u.val = y;
		stack[sp++].u.val = k;
	    }
	  break;
	  case pt_setcmykcolor:
	    if ( sp>=4 ) {
		real c=stack[sp-4].u.val,m=stack[sp-3].u.val,y=stack[sp-2].u.val,k=stack[sp-1].u.val;
		sp -= 4;
		if ( k==1 )
		    fore = 0x000000;
		else {
		    if (( y = (1-k)*y+k )<0 ) y=0; else if ( y>1 ) y=1;
		    if (( m = (1-k)*m+k )<0 ) m=0; else if ( m>1 ) m=1;
		    if (( c = (1-k)*c+k )<0 ) c=0; else if ( c>1 ) c=1;
		    fore = ((int) ((1-c)*255.)<<16) |
			    ((int) ((1-m)*255.)<<8) |
			    ((int) ((1-y)*255.));
		}
	    }
	  break;
	  case pt_currentpoint:
	    if ( sp+1<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_num;
		stack[sp++].u.val = current.x;
		stack[sp].type = ps_num;
		stack[sp++].u.val = current.y;
	    }
	  break;
	  case pt_fill: case pt_stroke:
	    if ( head==NULL && ec->splines!=NULL ) {
		/* assume they did a "gsave fill grestore stroke" (or reverse)*/
		ent = ec->splines;
		if ( tok==pt_stroke ) {
		    ent->u.splines.cap = linecap; ent->u.splines.join = linejoin;
		    ent->u.splines.stroke_width = linewidth;
		    memcpy(ent->u.splines.transform,transform,sizeof(transform));
		}
	    } else {
		ent = EntityCreate(head,linecap,linejoin,linewidth,transform,clippath);
		ent->next = ec->splines;
		ec->splines = ent;
	    }
	    if ( tok==pt_fill )
		ent->u.splines.fill.col = fore;
	    else
		ent->u.splines.stroke.col = fore;
	    head = NULL; cur = NULL;
	  break;
	  case pt_clip:
	    /* I really should intersect the old clip path with the new, but */
	    /*  I don't trust my intersect routine, crashes too often */
	    SplinePointListsFree(clippath);
	    clippath = SplinePointListCopy(head);
	    if ( clippath!=NULL && clippath->first!=clippath->last ) {
		SplineMake3(clippath->last,clippath->first);
		clippath->last = clippath->first;
	    }
	  break;
	  case pt_imagemask:
	    i = PSAddImagemask(ec,stack,sp,transform,fore);
	    while ( sp>i ) {
		--sp;
		if ( stack[sp].type==ps_string || stack[sp].type==ps_instr ||
			stack[sp].type==ps_lit )
		    free(stack[sp].u.str);
		else if ( stack[sp].type==ps_array || stack[sp].type==ps_dict )
		    dictfree(&stack[sp].u.dict);
	    }
	  break;

	  /* We don't do these right, but at least we'll avoid some errors with this hack */
	  case pt_save: case pt_currentmatrix:
	    /* push some junk on the stack */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_num;
		stack[sp++].u.val = 0;
	    }
	  /* Fall through into gsave */;
	  case pt_gsave:
	    if ( gsp<30 ) {
		memcpy(gsaves[gsp].transform,transform,sizeof(transform));
		gsaves[gsp].current = current;
		gsaves[gsp].linewidth = linewidth;
		gsaves[gsp].linecap = linecap;
		gsaves[gsp].linejoin = linejoin;
		gsaves[gsp].fore = fore;
		gsaves[gsp].clippath = SplinePointListCopy(clippath);
		++gsp;
		/* I should be saving the "current path" too, but that's too hard */
	    }
	  break;
	  case pt_restore: case pt_setmatrix:
	    /* pop some junk off the stack */
	    if ( sp>=1 )
		--sp;
	  /* Fall through into grestore */;
	  case pt_grestore:
	    if ( gsp>0 ) {
		--gsp;
		memcpy(transform,gsaves[gsp].transform,sizeof(transform));
		current = gsaves[gsp].current;
		linewidth = gsaves[gsp].linewidth;
		linecap = gsaves[gsp].linecap;
		linejoin = gsaves[gsp].linejoin;
		fore = gsaves[gsp].fore;
		SplinePointListsFree(clippath);
		clippath = gsaves[gsp].clippath;
	    }
	  break;
	  case pt_null:
	    /* push a 0. I don't handle pointers properly */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].u.val = 0;
		stack[sp++].type = ps_num;
	    }
	  break;
	  case pt_currentoverprint:
	    /* push false. I don't handle this properly */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].u.val = 0;
		stack[sp++].type = ps_bool;
	    }
	  break;
	  case pt_setoverprint:
	    /* pop one item on stack */
	    if ( sp>=1 )
		--sp;
	  break;
	  case pt_currentflat:
	    /* push 1.0 (default value). I don't handle this properly */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].u.val = 1.0;
		stack[sp++].type = ps_num;
	    }
	  break;
	  case pt_setflat:
	    /* pop one item on stack */
	    if ( sp>=1 )
		--sp;
	  break;
	  case pt_currentmiterlimit:
	    /* push 10.0 (default value). I don't handle this properly */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].u.val = 10.0;
		stack[sp++].type = ps_num;
	    }
	  break;
	  case pt_setmiterlimit:
	    /* pop one item off stack */
	    if ( sp>=1 )
		--sp;
	  break;
	  case pt_currentpacking:
	    /* push false (default value). I don't handle this properly */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].u.val = 0;
		stack[sp++].type = ps_bool;
	    }
	  break;
	  case pt_setpacking:
	    /* pop one item on stack */
	    if ( sp>=1 )
		--sp;
	  break;
	  case pt_currentstrokeadjust:
	    /* push false (default value). I don't handle this properly */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].u.val = 0;
		stack[sp++].type = ps_bool;
	    }
	  break;
	  case pt_setstrokeadjust:
	    /* pop one item on stack */
	    if ( sp>=1 )
		--sp;
	  break;
	  case pt_currentsmoothness:
	    /* default value is installation dependant. I don't handle this properly */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].u.val = 1.0;
		stack[sp++].type = ps_num;
	    }
	  break;
	  case pt_setsmoothness:
	    /* pop one item on stack */
	    if ( sp>=1 )
		--sp;
	  break;
	  case pt_currentobjectformat:
	    /* default value is installation dependant. I don't handle this properly */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].u.val = 0.0;
		stack[sp++].type = ps_num;
	    }
	  break;
	  case pt_setobjectformat:
	    /* pop one item on stack */
	    if ( sp>=1 )
		--sp;
	  break;
	  case pt_currentglobal: case pt_currentshared:
	    /* push false (default value). I don't handle this properly */
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].u.val = 0;
		stack[sp++].type = ps_bool;
	    }
	  break;
	  case pt_setglobal:
	    /* pop one item on stack */
	    if ( sp>=1 )
		--sp;
	  break;

	  case pt_openarray: case pt_mark:
	    if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp++].type = ps_mark;
	    }
	  break;
	  case pt_counttomark:
	    for ( i=0; (unsigned)i<sp; ++i )
		if ( stack[sp-1-i].type==ps_mark )
	    break;
	    if ( (unsigned)i==sp )
		LogError( _("No mark in counttomark\n") );
	    else if ( sp<sizeof(stack)/sizeof(stack[0]) ) {
		stack[sp].type = ps_num;
		stack[sp++].u.val = i;
	    }
	  break;
	  case pt_cleartomark:
	    for ( i=0; (unsigned)i<sp; ++i )
		if ( stack[sp-1-i].type==ps_mark )
	    break;
	    if ( (unsigned)i==sp )
		LogError( _("No mark in cleartomark\n") );
	    else {
		while ( sp>=i ) {
		    --sp;
		    if ( stack[sp].type==ps_string || stack[sp].type==ps_instr ||
			    stack[sp].type==ps_lit )
			free(stack[sp].u.str);
		    else if ( stack[sp].type==ps_array || stack[sp].type==ps_dict )
			dictfree(&stack[sp].u.dict);
		}
	    }
	  break;
	  case pt_closearray:
	    for ( i=0; (unsigned)i<sp; ++i )
		if ( stack[sp-1-i].type==ps_mark )
	    break;
	    if ( (unsigned)i==sp )
		LogError( _("No mark in ] (close array)\n") );
	    else {
		struct pskeydict dict;
		dict.cnt = dict.max = i;
		dict.entries = calloc(i,sizeof(struct pskeyval));
                dict.is_executable = false;
		for ( j=0; j<i; ++j ) {
		    dict.entries[j].type = stack[sp-i+j].type;
		    dict.entries[j].u = stack[sp-i+j].u;
		    /* don't need to copy because the things on the stack */
		    /*  are being popped (don't need to free either) */
		}
		collectgarbage(&tofrees,&dict);
		sp = sp-i;
		stack[sp-1].type = ps_array;
		stack[sp-1].u.dict = dict;
	    }
	  break;
	  case pt_array:
	    if ( sp>=1 && stack[sp-1].type==ps_num ) {
		struct pskeydict dict;
		dict.cnt = dict.max = stack[sp-1].u.val;
		dict.entries = calloc(dict.cnt,sizeof(struct pskeyval));
                dict.is_executable = false;
		/* all entries are inited to void */
		stack[sp-1].type = ps_array;
		stack[sp-1].u.dict = dict;
	    }
	  break;
	  case pt_aload:
	    sp = aload(sp,stack,sizeof(stack)/sizeof(stack[0]),&tofrees);
	  break;
	  case pt_astore:
	    if ( sp>=1 && stack[sp-1].type==ps_array ) {
		struct pskeydict dict;
		--sp;
		dict = stack[sp].u.dict;
		if ( sp>=dict.cnt ) {
		    for ( i=dict.cnt-1; i>=0 ; --i ) {
			--sp;
			dict.entries[i].type = stack[sp].type;
			dict.entries[i].u = stack[sp].u;
		    }
		}
		stack[sp].type = ps_array;
		stack[sp].u.dict = dict;
		++sp;
	    }
	  break;

	  case pt_output: case pt_outputd: case pt_print:
	    if ( sp>=1 ) {
		--sp;
		switch ( stack[sp].type ) {
		  case ps_num:
		    printf( "%g", (double) stack[sp].u.val );
		  break;
		  case ps_bool:
		    printf( "%s", stack[sp].u.tf ? "true" : "false" );
		  break;
		  case ps_string: case ps_instr: case ps_lit:
		    if ( tok==pt_outputd )
			printf( stack[sp].type==ps_lit ? "/" :
				stack[sp].type==ps_string ? "(" : "{" );
		    printf( "%s", stack[sp].u.str );
		    if ( tok==pt_outputd )
			printf( stack[sp].type==ps_lit ? "" :
				stack[sp].type==ps_string ? ")" : "}" );
		    free(stack[sp].u.str);
		  break;
		  case ps_void:
		    printf( "-- void --" );
		  break;
		  case ps_array:
		    if ( tok==pt_outputd ) {
			printarray(&stack[sp].u.dict);
			dictfree(&stack[sp].u.dict);
		  break;
		    } /* else fall through */
		    dictfree(&stack[sp].u.dict);
		  default:
		    printf( "-- nostringval --" );
		  break;
		}
		if ( tok==pt_output || tok==pt_outputd )
		    printf( "\n" );
	    } else
		LogError( _("Nothing on stack to print\n") );
	  break;

	  case pt_cvi: case pt_cvr:
	    /* I shan't distinguish between integers and reals */
	    if ( sp>=1 && stack[sp-1].type==ps_string ) {
		double val = strtod(stack[sp-1].u.str,NULL);
		free(stack[sp-1].u.str);
		stack[sp-1].u.val = val;
		stack[sp-1].type = ps_num;
	    }
	  break;
	  case pt_cvlit:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type==ps_array )
		    stack[sp-1].u.dict.is_executable = false;
	    }
	  case pt_cvn:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type==ps_string )
		    stack[sp-1].type = ps_lit;
	    }
	  case pt_cvx:
	    if ( sp>=1 ) {
		if ( stack[sp-1].type==ps_array )
		    stack[sp-1].u.dict.is_executable = true;
	    }
	  break;
	  case pt_cvrs:
	    if ( sp>=3 && stack[sp-1].type==ps_string &&
		    stack[sp-2].type==ps_num &&
		    stack[sp-3].type==ps_num ) {
		if ( stack[sp-2].u.val==8 )
		    sprintf( stack[sp-1].u.str, "%o", (int) stack[sp-3].u.val );
		else if ( stack[sp-2].u.val==16 )
		    sprintf( stack[sp-1].u.str, "%X", (int) stack[sp-3].u.val );
		else /* default to radix 10 no matter what they asked for */
		    sprintf( stack[sp-1].u.str, "%g", (double) stack[sp-3].u.val );
		stack[sp-3] = stack[sp-1];
		sp-=2;
	    }
	  break;
	  case pt_cvs:
	    if ( sp>=2 && stack[sp-1].type==ps_string ) {
		switch ( stack[sp].type ) {
		  case ps_num:
		    sprintf( stack[sp-1].u.str, "%g", (double) stack[sp-2].u.val );
		  break;
		  case ps_bool:
		    sprintf( stack[sp-1].u.str, "%s", stack[sp-2].u.tf ? "true" : "false" );
		  break;
		  case ps_string: case ps_instr: case ps_lit:
		    sprintf( stack[sp-1].u.str, "%s", stack[sp-2].u.str );
		    free(stack[sp].u.str);
		  break;
		  case ps_void:
		    printf( "-- void --" );
		  break;
		  case ps_array:
		    dictfree(&stack[sp].u.dict);
		  default:
		    sprintf( stack[sp-1].u.str, "-- nostringval --" );
		  break;
		}
		stack[sp-2] = stack[sp-1];
		--sp;
	    }
	  break;
	  case pt_stringop:	/* the string keyword, not the () thingy */
	    if ( sp>=1 && stack[sp-1].type==ps_num ) {
		stack[sp-1].type = ps_string;
		stack[sp-1].u.str = calloc(stack[sp-1].u.val+1,1);
	    }
	  break;

	  case pt_unknown:
	    if ( !warned ) {
		LogError( _("Warning: Unable to parse token %s, some features may be lost\n"), tokbuf );
		warned = true;
	    }
	  break;

	  default:
	  break;
	}}
    }
 done:
    if ( rs!=NULL ) {
	int i, cnt, j;
	for ( i=sp-1; i>=0; --i )
	    if ( stack[i].type!=ps_num )
	break;
	cnt = sp-1-i;
	if ( cnt>rs->max ) cnt = rs->max;
	rs->cnt = cnt;
	for ( j=i+1; (unsigned)j<sp; ++j )
	    rs->stack[j-i-1] = stack[j].u.val;
    }
    freestuff(stack,sp,&dict,&gb,&tofrees);
    if ( head!=NULL ) {
	ent = EntityCreate(head,linecap,linejoin,linewidth,transform,clippath);
	ent->next = ec->splines;
	ec->splines = ent;
    }
    while ( gsp>0 ) {
	--gsp;
	SplinePointListsFree(gsaves[gsp].clippath);
    }
    SplinePointListsFree(clippath);
    ECCategorizePoints(ec);
    if ( ec->width == UNDEFINED_WIDTH )
	ec->width = wrapper->advance_width;
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    free(tokbuf);
}

static void InterpretPS(FILE *ps, char *psstr, EntityChar *ec, RetStack *rs) {
    IO wrapper;

    memset(&wrapper,0,sizeof(wrapper));
    wrapper.advance_width = UNDEFINED_WIDTH;
    pushio(&wrapper,ps,psstr,0);
    _InterpretPS(&wrapper,ec,rs);
}

static SplinePointList *EraseStroke(SplineChar *sc,SplinePointList *head,SplinePointList *erase) {
    SplineSet *spl, *last;
    SplinePoint *sp;

    if ( head==NULL ) {
	/* Pointless, but legal */
	SplinePointListsFree(erase);
return( NULL );
    }

    last = NULL;
    for ( spl=head; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; sp!=NULL; ) {
	    sp->selected = false;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	last = spl;
    }
    for ( spl=erase; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; sp!=NULL; ) {
	    sp->selected = true;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
    last->next = erase;
return( SplineSetRemoveOverlap(sc,head,over_exclude) );
}

static Entity *EntityReverse(Entity *ent) {
    Entity *next, *last = NULL;

    while ( ent!=NULL ) {
	next = ent->next;
	ent->next = last;
	last = ent;
	ent = next;
    }
return( last );
}

static SplinePointList *SplinesFromLayers(SplineChar *sc,int *flags, int tostroke) {
    int layer;
    SplinePointList *head=NULL, *last, *new, *nlast, *temp, *each, *transed;
    StrokeInfo si;
    /*SplineSet *spl;*/
    int handle_eraser;
    real inversetrans[6], transform[6];
    int changed;

    if ( tostroke ) {
	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	    if ( sc->layers[layer].splines==NULL )
	continue;
	    else if ( head==NULL )
		head = sc->layers[layer].splines;
	    else
		last->next = sc->layers[layer].splines;
	    for ( last = sc->layers[layer].splines; last->next!=NULL; last=last->next );
	    sc->layers[layer].splines = NULL;
	}
return( head );
    }

    if ( *flags==-1 )
	*flags = PsStrokeFlagsDlg();

    if ( *flags & sf_correctdir ) {
	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) if ( sc->layers[layer].dofill )
	    SplineSetsCorrect(sc->layers[layer].splines,&changed);
    }

    handle_eraser = *flags & sf_handle_eraser;

    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	if ( sc->layers[layer].dostroke ) {
	    memset(&si,'\0',sizeof(si));
	    si.join = sc->layers[layer].stroke_pen.linejoin;
	    si.cap = sc->layers[layer].stroke_pen.linecap;
	    si.radius = sc->layers[layer].stroke_pen.width/2.0f;
	    if ( sc->layers[layer].stroke_pen.width==WIDTH_INHERITED )
		si.radius = .5;
	    if ( si.cap == lc_inherited ) si.cap = lc_butt;
	    if ( si.join == lj_inherited ) si.join = lj_miter;
	    new = NULL;
	    memcpy(transform,sc->layers[layer].stroke_pen.trans,4*sizeof(real));
	    transform[4] = transform[5] = 0;
	    MatInverse(inversetrans,transform);
	    transed = SplinePointListTransform(SplinePointListCopy(
		    sc->layers[layer].splines),inversetrans,tpt_AllPoints);
	    for ( each = transed; each!=NULL; each=each->next ) {
		temp = SplineSetStroke(each,&si,sc->layers[layer].order2);
		if ( new==NULL )
		    new=temp;
		else
		    nlast->next = temp;
		if ( temp!=NULL )
		    for ( nlast=temp; nlast->next!=NULL; nlast=nlast->next );
	    }
	    new = SplinePointListTransform(new,transform,tpt_AllPoints);
	    SplinePointListsFree(transed);
	    if ( handle_eraser && sc->layers[layer].stroke_pen.brush.col==0xffffff ) {
		head = EraseStroke(sc,head,new);
		last = head;
		if ( last!=NULL )
		    for ( ; last->next!=NULL; last=last->next );
	    } else {
		if ( head==NULL )
		    head = new;
		else
		    last->next = new;
		if ( new!=NULL )
		    for ( last = new; last->next!=NULL; last=last->next );
	    }
	}
	if ( sc->layers[layer].dofill ) {
	    if ( handle_eraser && sc->layers[layer].fill_brush.col==0xffffff ) {
		head = EraseStroke(sc,head,sc->layers[layer].splines);
		last = head;
		if ( last!=NULL )
		    for ( ; last->next!=NULL; last=last->next );
	    } else {
		new = SplinePointListCopy(sc->layers[layer].splines);
		if ( head==NULL )
		    head = new;
		else
		    last->next = new;
		if ( new!=NULL )
		    for ( last = new; last->next!=NULL; last=last->next );
	    }
	}
    }
return( head );
}

void SFSplinesFromLayers(SplineFont *sf,int tostroke) {
    /* User has turned off multi-layer, flatten the font */
    int i, layer;
    int flags= -1;
    Layer *new;
    CharViewBase *cv;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];
	SplineSet *splines = SplinesFromLayers(sc,&flags,tostroke);
	RefChar *head=NULL, *last=NULL;
	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	    if ( head==NULL )
		head = last = sc->layers[layer].refs;
	    else
		last->next = sc->layers[layer].refs;
	    if ( last!=NULL )
		while ( last->next!=NULL ) last = last->next;
	    sc->layers[layer].refs = NULL;
	}
	new = calloc(2,sizeof(Layer));
	new[ly_back] = sc->layers[ly_back];
	memset(&sc->layers[ly_back],0,sizeof(Layer));
	LayerDefault(&new[ly_fore]);
	new[ly_fore].splines = splines;
	new[ly_fore].refs = head;
	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	    SplinePointListsMDFree(sc,sc->layers[layer].splines);
	    RefCharsFree(sc->layers[layer].refs);
	    ImageListsFree(sc->layers[layer].images);
	}
	free(sc->layers);
	sc->layers = new;
	sc->layer_cnt = 2;
	for ( cv=sc->views; cv!=NULL; cv=cv->next ) {
	    cv->layerheads[dm_back] = &sc->layers[ly_back];
	    cv->layerheads[dm_fore] = &sc->layers[ly_fore];
	}
    }
    SFReinstanciateRefs(sf);
}

void SFSetLayerWidthsStroked(SplineFont *sf, real strokewidth) {
    int i;
    /* We changed from a stroked font to a multilayered font */

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];
	sc->layers[ly_fore].dofill = false;
	sc->layers[ly_fore].dostroke = true;
	sc->layers[ly_fore].stroke_pen.width = strokewidth;
    }
}

static void EntityCharCorrectDir(EntityChar *ec) {
    SplineSet *ss;
    Entity *ent;
    int changed;

    for ( ent=ec->splines; ent!=NULL; ent = ent->next ) {
	/* ignore splines which are only stoked, but not filled */
	if ( ent->type == et_splines && ent->u.splines.fill.col!=0xffffffff ) {
	    /* Correct the direction of each stroke or fill with respect to */
	    /*  the splines in it */
	    SplineSetsCorrect(ent->u.splines.splines,&changed);
	    if ( ent->u.splines.fill.col==0xffffff ) {
		/* If they are filling with white, then assume they mean */
		/*  an internal area that should be drawn backwards */
		for ( ss=ent->u.splines.splines; ss!=NULL; ss=ss->next )
		    SplineSetReverse(ss);
	    }
	    SplineSetsCorrect(ent->clippath,&changed);
	}
    }
}

void EntityDefaultStrokeFill(Entity *ent) {
    while ( ent!=NULL ) {
	if ( ent->type == et_splines &&
		ent->u.splines.stroke.col==0xffffffff &&
		ent->u.splines.fill.col==0xffffffff ) {
	    SplineSet *spl;
	    int all=1;
	    for ( spl=ent->u.splines.splines; spl!=NULL; spl=spl->next )
		if ( spl->first->prev!=NULL ) {
		    all = false;
	    break;
		}
	    if ( all && ent->u.splines.splines!=NULL &&
		    (ent->u.splines.stroke_width==0 || ent->u.splines.stroke_width==WIDTH_INHERITED))
		ent->u.splines.stroke_width=40;		/* random guess */
	    if (ent->u.splines.stroke_width==0 || ent->u.splines.stroke_width==WIDTH_INHERITED)
		ent->u.splines.fill.col = COLOR_INHERITED;
	    else
		ent->u.splines.stroke.col = COLOR_INHERITED;
	}
	ent = ent->next;
    }
}

SplinePointList *SplinesFromEntityChar(EntityChar *ec,int *flags,int is_stroked) {
    Entity *ent, *next;
    SplinePointList *head=NULL, *last, *new, *nlast, *temp, *each, *transed;
    StrokeInfo si;
    real inversetrans[6];
    /*SplineSet *spl;*/
    int handle_eraser = false;
    int ask = false;

    EntityDefaultStrokeFill(ec->splines);

    if ( !is_stroked ) {

	if ( *flags==-1 ) {
	    for ( ent=ec->splines; ent!=NULL; ent = ent->next ) {
		if ( ent->type == et_splines &&
			(ent->u.splines.fill.col==0xffffff ||
			 /*ent->u.splines.clippath!=NULL ||*/
			 (ent->u.splines.stroke_width!=0 && ent->u.splines.stroke.col!=0xffffffff))) {
		    ask = true;
	    break;
		}
	    }
	    if ( ask )
		*flags = PsStrokeFlagsDlg();
	}

	if ( *flags & sf_correctdir )		/* Will happen if flags still unset (-1) */
	    EntityCharCorrectDir(ec);

	handle_eraser = *flags!=-1 && (*flags & sf_handle_eraser);
	if ( handle_eraser )
	    ec->splines = EntityReverse(ec->splines);
    }

    for ( ent=ec->splines; ent!=NULL; ent = next ) {
	next = ent->next;
	if ( ent->type == et_splines && is_stroked ) {
	    if ( head==NULL )
		head = ent->u.splines.splines;
	    else
		last->next = ent->u.splines.splines;
	    if ( ent->u.splines.splines!=NULL )
		for ( last = ent->u.splines.splines; last->next!=NULL; last=last->next );
	    ent->u.splines.splines = NULL;
	} else if ( ent->type == et_splines ) {
	    if ( ent->u.splines.stroke.col!=0xffffffff &&
		    (ent->u.splines.fill.col==0xffffffff || ent->u.splines.stroke_width!=0)) {
		/* What does a stroke width of 0 mean? PS Says minimal width line */
		/* How do we implement that? Special case: If filled and stroked 0, then */
		/*  ignore the stroke. This idiom is used by MetaPost sometimes and means */
		/*  no stroke */
		memset(&si,'\0',sizeof(si));
		si.join = ent->u.splines.join;
		si.cap = ent->u.splines.cap;
		si.radius = ent->u.splines.stroke_width/2;
		if ( ent->u.splines.stroke_width==WIDTH_INHERITED )
		    si.radius = .5;
		if ( si.cap == lc_inherited ) si.cap = lc_butt;
		if ( si.join == lj_inherited ) si.join = lj_miter;
		new = NULL;
		MatInverse(inversetrans,ent->u.splines.transform);
		transed = SplinePointListTransform(SplinePointListCopy(
			ent->u.splines.splines),inversetrans,tpt_AllPoints);
		for ( each = transed; each!=NULL; each=each->next ) {
		    temp = SplineSetStroke(each,&si,false);
		    if ( new==NULL )
			new=temp;
		    else
			nlast->next = temp;
		    if ( temp!=NULL )
			for ( nlast=temp; nlast->next!=NULL; nlast=nlast->next );
		}
		new = SplinePointListTransform(new,ent->u.splines.transform,tpt_AllPoints);
		SplinePointListsFree(transed);
		if ( handle_eraser && ent->u.splines.stroke.col==0xffffff ) {
		    head = EraseStroke(ec->sc,head,new);
		    last = head;
		    if ( last!=NULL )
			for ( ; last->next!=NULL; last=last->next );
		} else {
		    if ( head==NULL )
			head = new;
		    else
			last->next = new;
		    if ( new!=NULL )
			for ( last = new; last->next!=NULL; last=last->next );
		}
	    }
	    /* If they have neither a stroke nor a fill, pretend they said fill */
	    if ( ent->u.splines.fill.col==0xffffffff && ent->u.splines.stroke.col!=0xffffffff )
		SplinePointListsFree(ent->u.splines.splines);
	    else if ( handle_eraser && ent->u.splines.fill.col==0xffffff ) {
		head = EraseStroke(ec->sc,head,ent->u.splines.splines);
		last = head;
		if ( last!=NULL )
		    for ( ; last->next!=NULL; last=last->next );
	    } else {
		new = ent->u.splines.splines;
		if ( head==NULL )
		    head = new;
		else
		    last->next = new;
		if ( new!=NULL )
		    for ( last = new; last->next!=NULL; last=last->next );
	    }
	}
	SplinePointListsFree(ent->clippath);
	free(ent);
    }
return( head );
}

SplinePointList *SplinesFromEntities(Entity *ent,int *flags,int is_stroked) {
    EntityChar ec;

    memset(&ec,'\0',sizeof(ec));
    ec.splines = ent;
return( SplinesFromEntityChar(&ec,flags,is_stroked));
}

SplinePointList *SplinePointListInterpretPS(FILE *ps,int flags,int is_stroked, int *width) {
    EntityChar ec;
    SplineChar sc;

    memset(&ec,'\0',sizeof(ec));
    ec.width = ec.vwidth = UNDEFINED_WIDTH;
    memset(&sc,0,sizeof(sc)); sc.name = "<No particular character>";
    ec.sc = &sc;
    InterpretPS(ps,NULL,&ec,NULL);
    if ( width!=NULL )
	*width = ec.width;
return( SplinesFromEntityChar(&ec,&flags,is_stroked));
}

Entity *EntityInterpretPS(FILE *ps,int *width) {
    EntityChar ec;

    memset(&ec,'\0',sizeof(ec));
    ec.width = ec.vwidth = UNDEFINED_WIDTH;
    InterpretPS(ps,NULL,&ec,NULL);
    if ( width!=NULL )
	*width = ec.width;
return( ec.splines );
}

static RefChar *revrefs(RefChar *cur) {
    RefChar *p, *n;

    if ( cur==NULL )
return( NULL );

    p = NULL;
    for ( ; (n=cur->next)!=NULL; cur = n ) {
	cur->next = p;
	p = cur;
    }
    cur->next = p;
return( cur );
}

static void SCInterpretPS(FILE *ps,SplineChar *sc) {
    EntityChar ec;
    real dval;
    char tokbuf[10];
    IO wrapper;
    int ch;

    while ( isspace(ch = getc(ps)) );
    ungetc(ch,ps);

    memset(&wrapper,0,sizeof(wrapper));
    wrapper.advance_width = UNDEFINED_WIDTH;
    if ( ch!='<' ) {
	pushio(&wrapper,ps,NULL,0);

	if ( nextpstoken(&wrapper,&dval,tokbuf,sizeof(tokbuf))!=pt_opencurly )
	    LogError( _("We don't understand this font\n") );
    } else {
	(void) getc(ps);
	pushfogio(&wrapper,ps);
    }
    memset(&ec,'\0',sizeof(ec));
    ec.fromtype3 = true;
    ec.sc = sc;
    _InterpretPS(&wrapper,&ec,NULL);
    sc->width = ec.width;
    sc->layer_cnt = 1;
    SCAppendEntityLayers(sc,ec.splines);
    if ( sc->layer_cnt==1 ) ++sc->layer_cnt;
    sc->layers[ly_fore].refs = revrefs(ec.refs);
    free(wrapper.top);
}

void PSFontInterpretPS(FILE *ps,struct charprocs *cp,char **encoding) {
    char tokbuf[100];
    int tok,i, j;
    real dval;
    SplineChar *sc; EntityChar dummy;
    RefChar *p, *ref, *next;
    IO wrapper;

    wrapper.top = NULL;
    wrapper.advance_width = UNDEFINED_WIDTH;
    pushio(&wrapper,ps,NULL,0);

    while ( (tok = nextpstoken(&wrapper,&dval,tokbuf,sizeof(tokbuf)))!=pt_eof && tok!=pt_end ) {
	if ( tok==pt_namelit ) {
	    if ( cp->next>=cp->cnt ) {
		++cp->cnt;
		cp->keys = realloc(cp->keys,cp->cnt*sizeof(char *));
		cp->values = realloc(cp->values,cp->cnt*sizeof(char *));
	    }
	    if ( cp->next<cp->cnt ) {
		sc = SplineCharCreate(2);
		cp->keys[cp->next] = copy(tokbuf);
		cp->values[cp->next++] = sc;
		sc->name = copy(tokbuf);
		SCInterpretPS(ps,sc);
       		ff_progress_next();
	    } else {
		memset(&dummy,0,sizeof(dummy));
		dummy.fromtype3 = true;
		InterpretPS(ps,NULL,&dummy,NULL);
	    }
	}
    }
    free(wrapper.top);

    /* References were done by name in the postscript. we stored the names in */
    /*  ref->sc (which is a hack). Now look up all those names and replace */
    /*  with the appropriate splinechar. If we can't find anything then throw */
    /*  out the reference */
    /* Further fixups come later, where all ps refs are fixedup */
    for ( i=0; i<cp->next; ++i ) {
	for ( p=NULL, ref=cp->values[i]->layers[ly_fore].refs; ref!=NULL; ref=next ) {
	    char *refname = (char *) (ref->sc);
	    next = ref->next;
	    if ( ref->sc==NULL )
		refname=encoding[ref->orig_pos];
	    for ( j=0; j<cp->next; ++j )
		if ( strcmp(cp->keys[j],refname)==0 )
	    break;
	    free(ref->sc);	/* a string, not a splinechar */
	    if ( j!=cp->next ) {
		ref->sc = cp->values[j];
		SCMakeDependent(cp->values[i],ref->sc);
		ref->adobe_enc = getAdobeEnc(ref->sc->name);
		ref->checked = true;
		p = ref;
	    } else {
		if ( p==NULL )
		    cp->values[i]->layers[ly_fore].refs = next;
		else
		    p->next = next;
		ref->next = NULL;
		RefCharFree(ref);
	    }
	}
    }
}

/* Slurp up an encoding in the form:
 /Enc-name [
    /charname
    ...
 ] def
We're not smart here no: 0 1 255 {1 index exch /.notdef put} for */
Encoding *PSSlurpEncodings(FILE *file) {
    char *names[1024];
    int32 encs[1024];
    Encoding *item, *head = NULL, *last;
    char *encname;
    char tokbuf[200];
    IO wrapper;
    real dval;
    size_t i, any;
    int max, enc, codepointsonly, tok;

    wrapper.top = NULL;
    wrapper.advance_width = UNDEFINED_WIDTH;
    pushio(&wrapper,file,NULL,0);

    while ( (tok = nextpstoken(&wrapper,&dval,tokbuf,sizeof(tokbuf)))!=pt_eof ) {
	encname = NULL;
	if ( tok==pt_namelit ) {
	    encname = copy(tokbuf);
	    tok = nextpstoken(&wrapper,&dval,tokbuf,sizeof(tokbuf));
	}
	if ( tok!=pt_openarray && tok!=pt_opencurly )
return( head );
	for ( i=0; i<sizeof(names)/sizeof(names[0]); ++i ) {
	    encs[i] = -1;
	    names[i]=NULL;
	}
	codepointsonly = CheckCodePointsComment(&wrapper);

	max = -1; any = 0;
	for (i = 0; (tok = nextpstoken(&wrapper,&dval,tokbuf,sizeof(tokbuf)))!=pt_eof &&
                 tok!=pt_closearray && tok!=pt_closecurly;
             i++) {
	    if ( tok==pt_namelit && i<sizeof(names)/sizeof(names[0]) ) {
		max = i;
		if ( strcmp(tokbuf,".notdef")==0 ) {
		    encs[i] = -1;
		} else if ( (enc=UniFromName(tokbuf,ui_none,&custom))!=-1 ) {
		    encs[i] = enc;
		    /* Used not to do this, but there are several legal names */
		    /*  for some slots and people get unhappy (rightly) if we */
		    /*  use the wrong one */
		    names[i] = copy(tokbuf);
		    any = 1;
		} else {
		    names[i] = copy(tokbuf);
		    any = 1;
		}
	    }
	}
	if ( encname!=NULL ) {
	    tok = nextpstoken(&wrapper,&dval,tokbuf,sizeof(tokbuf));
	    if ( tok==pt_def ) {
		/* Good */
	    } else {
        	/* TODO! */
        	/* I guess it's not good... */
	    }
	}
	if ( max!=-1 ) {
	    if ( ++max<256 ) max = 256;
	    item = calloc(1,sizeof(Encoding));
	    item->enc_name = encname;
	    item->char_cnt = max;
	    item->unicode = malloc(max*sizeof(int32));
	    memcpy(item->unicode,encs,max*sizeof(int32));
	    if ( any && !codepointsonly ) {
		item->psnames = calloc(max,sizeof(char *));
		memcpy(item->psnames,names,max*sizeof(char *));
	    } else {
		for ( i=0; i<max; ++i )
		    free(names[i]);
	    }
	    if ( head==NULL )
		head = item;
	    else
		last->next = item;
	    last = item;
	}
    }

return( head );
}

int EvaluatePS(char *str,real *stack,int size) {
    EntityChar ec;
    RetStack rs;

    memset(&ec,'\0',sizeof(ec));
    memset(&rs,'\0',sizeof(rs));
    rs.max = size;
    rs.stack = stack;
    InterpretPS(NULL,str,&ec,&rs);
return( rs.cnt );
}

static void closepath(SplinePointList *cur, int is_type2) {
    if ( cur!=NULL && cur->first==cur->last && cur->first->prev==NULL && is_type2 )
return;		/* The "path" is just a single point created by a moveto */
		/* Probably we're just doing another moveto */
    if ( cur!=NULL && cur->first!=NULL && cur->first!=cur->last ) {
/* I allow for greater errors here than I do in the straight postscript code */
/*  because: 1) the rel-rel operators will accumulate more rounding errors   */
/*  2) I only output 2 decimal digits after the decimal in type1 output */
	if ( RealWithin(cur->first->me.x,cur->last->me.x,.05) && RealWithin(cur->first->me.y,cur->last->me.y,.05) ) {
	    SplinePoint *oldlast = cur->last;
	    cur->first->prevcp = oldlast->prevcp;
	    cur->first->prevcp.x += (cur->first->me.x-oldlast->me.x);
	    cur->first->prevcp.y += (cur->first->me.y-oldlast->me.y);
	    cur->first->noprevcp = oldlast->noprevcp;
	    oldlast->prev->from->next = NULL;
	    cur->last = oldlast->prev->from;
	    chunkfree(oldlast->prev,sizeof(*oldlast));
	    chunkfree(oldlast->hintmask,sizeof(HintMask));
	    chunkfree(oldlast,sizeof(*oldlast));
	}
	CheckMake(cur->last,cur->first);
	SplineMake3(cur->last,cur->first);
	cur->last = cur->first;
    }
}

static void UnblendFree(StemInfo *h ) {
    while ( h!=NULL ) {
	chunkfree(h->u.unblended,sizeof(real [2][MmMax]));
	h->u.unblended = NULL;
	h = h->next;
    }
}

static StemInfo *HintsAppend(StemInfo *to,StemInfo *extra) {
    StemInfo *h;

    if ( to==NULL )
return( extra );
    if ( extra==NULL )
return( to );
    for ( h=to; h->next!=NULL; h=h->next );
    h->next = extra;
return( to );
}

static StemInfo *HintNew(double start,double width) {
    StemInfo *h;

    h = chunkalloc(sizeof(StemInfo));
    h->start = start;
    h->width = width;
return( h );
}

static void RemapHintMask(HintMask *hm,int mapping[96],int max) {
    HintMask rpl;
    int i, mb;

    if ( hm==NULL )
return;

    if ( max>96 ) max = 96;
    mb = (max+7)>>3;

    memset(&rpl,0,mb);
    for ( i=0; i<max; ++i ) if ( (*hm)[i>>3]&(0x80>>(i&0x7)) )
	rpl[mapping[i]>>3] |= (0x80>>(mapping[i]&0x7));
    memcpy(hm,&rpl,mb);
}

static void HintsRenumber(SplineChar *sc) {
    /* In a type1 font the hints may get added to our hint list in a semi- */
    /*  random order. In an incorrect type2 font the same thing could happen. */
    /*  Force the order to be correct, and then update all masks */
    int mapping[96];
    int i, max;
    StemInfo *h;
    SplineSet *spl;
    SplinePoint *sp;

    for ( i=0; i<96; ++i ) mapping[i] = i;

    i = 0;
    for ( h=sc->hstem; h!=NULL; h=h->next ) {
	if ( h->hintnumber<96 && i<96 ) {
	    mapping[h->hintnumber] = i;
	    h->hintnumber = i++;
	}
	chunkfree(h->u.unblended,sizeof(real [2][MmMax]));
	h->u.unblended = NULL;
    }
    for ( h=sc->vstem; h!=NULL; h=h->next ) {
	if ( h->hintnumber<96 && i<96 ) {
	    mapping[h->hintnumber] = i;
	    h->hintnumber = i++;
	}
	chunkfree(h->u.unblended,sizeof(real [2][MmMax]));
	h->u.unblended = NULL;
    }
    max = i;
    for ( i=0; i<max; ++i )
	if ( mapping[i]!=i )
    break;
    if ( i==max )
return;				/* Didn't change the order */

    for ( i=0; i<sc->countermask_cnt; ++i )
	RemapHintMask(&sc->countermasks[i],mapping,max);
    for ( spl = sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	for ( sp = spl->first; ; ) {
	    RemapHintMask(sp->hintmask,mapping,max);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
}

int UnblendedCompare(real u1[MmMax], real u2[MmMax], int cnt) {
    int i;

    for ( i=0; i<cnt; ++i ) {
	if ( u1[i]!=u2[i] )
return( u1[i]>u2[i]?1:-1 );
    }
return( 0 );
}

static StemInfo *SameH(StemInfo *old,real start, real width,
	real unblended[2][MmMax], int instance_count) {
    StemInfo *sameh;

    if ( instance_count==0 ) {
	for ( sameh=old; sameh!=NULL; sameh=sameh->next )
	    if ( sameh->start==start && sameh->width==width)
	break;
    } else { int j;
	for ( j=1; j<instance_count; ++j ) {
	    unblended[0][j] += unblended[0][j-1];
	    unblended[1][j] += unblended[1][j-1];
	}
	for ( sameh=old; sameh!=NULL; sameh=sameh->next ) {
	    if ( (*sameh->u.unblended)[0] == NULL || (*sameh->u.unblended)[1]==NULL )
	continue;
	    if ( UnblendedCompare((*sameh->u.unblended)[0],unblended[0],instance_count)==0 &&
		    UnblendedCompare((*sameh->u.unblended)[1],unblended[1],instance_count)==0)
	break;
	}
    }
return( sameh );
}

static real Blend(real u[MmMax],struct pscontext *context) {
    real sum = u[0];
    int i;

    for ( i=1; i<context->instance_count; ++i )
	sum += context->blend_values[i]*u[i];
return( sum );
}

/* this handles either Type1 or Type2 charstrings. Type2 charstrings have */
/*  more operators than Type1s and the old operators have extended meanings */
/*  (ie. the rlineto operator can produce more than one line). But pretty */
/*  much it's a superset and if we parse for type2 (with a few additions) */
/*  we'll get it right */
/* Char width is done differently. Moveto starts a newpath. 0xff starts a 16.16*/
/*  number rather than a 32 bit number */
SplineChar *PSCharStringToSplines(uint8 *type1, int len, struct pscontext *context,
	struct pschars *subrs, struct pschars *gsubrs, const char *name) {
    int is_type2 = context->is_type2;
    real stack[50]; int sp=0, v;		/* Type1 stack is about 25 long, Type2 stack is 48 */
    real transient[32];
    SplineChar *ret = SplineCharCreate(2);
    SplinePointList *cur=NULL, *oldcur=NULL;
    RefChar *r1, *r2, *rlast=NULL;
    DBasePoint current;
    real dx, dy, dx2, dy2, dx3, dy3, dx4, dy4, dx5, dy5, dx6, dy6;
    SplinePoint *pt;
    /* subroutines may be nested to a depth of 10 */
    struct substate { unsigned char *type1; int len; int subnum; } pcstack[11];
    int pcsp=0;
    StemInfo *hint, *hp;
    real pops[30];
    int popsp=0;
    int base, polarity;
    real coord;
    struct pschars *s;
    int hint_cnt = 0;
    StemInfo *activeh=NULL, *activev=NULL, *sameh;
    HintMask *pending_hm = NULL;
    HintMask *counters[96];
    int cp=0;
    real unblended[2][MmMax];
    int last_was_b1=false, old_last_was_b1;

    if ( !is_type2 && context->instance_count>1 )
	memset(unblended,0,sizeof(unblended));

    ret->name = copy( name );
    ret->unicodeenc = -1;
    ret->width = (int16) 0x8000;
    if ( name==NULL ) name = "unnamed";
    ret->manualhints = true;

    current.x = current.y = 0;
    while ( len>0 ) {
	if ( sp>48 ) {
	    LogError( _("Stack got too big in %s\n"), name );
	    sp = 48;
	}
	base = 0;
	--len;
	if ( (v = *type1++)>=32 ) {
	    if ( v<=246) {
		stack[sp++] = v - 139;
	    } else if ( v<=250 ) {
		stack[sp++] = (v-247)*256 + *type1++ + 108;
		--len;
	    } else if ( v<=254 ) {
		stack[sp++] = -(v-251)*256 - *type1++ - 108;
		--len;
	    } else {
		int val = (*type1<<24) | (type1[1]<<16) | (type1[2]<<8) | type1[3];
		stack[sp++] = val;
		type1 += 4;
		len -= 4;
		if ( is_type2 ) {
#ifndef PSFixed_Is_TTF	/* The type2 spec is contradictory. It says this is a */
			/*  two's complement number, but it also says it is a */
			/*  Fixed, which in truetype is not two's complement */
			/*  (mantisa is always unsigned) */
		    stack[sp-1] /= 65536.;
#else
		    int mant = val&0xffff;
		    stack[sp-1] = (val>>16) + mant/65536.;
#endif
		}
	    }
	} else if ( v==28 ) {
	    stack[sp++] = (short) ((type1[0]<<8) | type1[1]);
	    type1 += 2;
	    len -= 2;
	/* In the Dict tables of CFF, a 5byte fixed value is prefixed by a */
	/*  29 code. In Type2 strings the prefix is 255. */
	} else if ( v==12 ) {
	    old_last_was_b1 = last_was_b1; last_was_b1 = false;
	    v = *type1++;
	    --len;
	    switch ( v ) {
	      case 0: /* dotsection */
		if ( is_type2 )
		    LogError( _("%s\'s dotsection operator is deprecated for Type2\n"), name );
		sp = 0;
	      break;
	      case 1: /* vstem3 */	/* specifies three v hints zones at once */
		if ( sp<6 ) LogError( _("Stack underflow on vstem3 in %s\n"), name );
		/* according to the standard, if there is a vstem3 there can't */
		/*  be any vstems, so there can't be any confusion about hint order */
		/*  so we don't need to worry about unblended stuff */
		if ( is_type2 )
		    LogError( _("%s\'s vstem3 operator is not supported for Type2\n"), name );
		sameh = NULL;
		if ( !is_type2 )
		    sameh = SameH(ret->vstem,stack[0] + ret->lsidebearing,stack[1],
				unblended,0);
		hint = HintNew(stack[0] + ret->lsidebearing,stack[1]);
		hint->hintnumber = sameh!=NULL ? sameh->hintnumber : hint_cnt++;
		if ( activev==NULL )
		    activev = hp = hint;
		else {
		    for ( hp=activev; hp->next!=NULL; hp = hp->next );
		    hp->next = hint;
		    hp = hint;
		}
		sameh = NULL;
		if ( !is_type2 )
		    sameh = SameH(ret->vstem,stack[2] + ret->lsidebearing,stack[3],
				unblended,0);
		hp->next = HintNew(stack[2] + ret->lsidebearing,stack[3]);
		hp->next->hintnumber = sameh!=NULL ? sameh->hintnumber : hint_cnt++;
		if ( !is_type2 )
		    sameh = SameH(ret->vstem,stack[4] + ret->lsidebearing,stack[5],
				unblended,0);
		hp->next->next = HintNew(stack[4] + ret->lsidebearing,stack[5]);
		hp->next->next->hintnumber = sameh!=NULL ? sameh->hintnumber : hint_cnt++;
		if ( !is_type2 && hp->next->next->hintnumber<96 ) {
		    if ( pending_hm==NULL )
			pending_hm = chunkalloc(sizeof(HintMask));
		    (*pending_hm)[hint->hintnumber>>3] |= 0x80>>(hint->hintnumber&0x7);
		    (*pending_hm)[hint->next->hintnumber>>3] |= 0x80>>(hint->next->hintnumber&0x7);
		    (*pending_hm)[hint->next->next->hintnumber>>3] |= 0x80>>(hint->next->next->hintnumber&0x7);
		}
		hp = hp->next->next;
		sp = 0;
	      break;
	      case 2: /* hstem3 */	/* specifies three h hints zones at once */
		if ( sp<6 ) LogError( _("Stack underflow on hstem3 in %s\n"), name );
		if ( is_type2 )
		    LogError( _("%s\'s vstem3 operator is not supported for Type2\n"), name );
		sameh = NULL;
		if ( !is_type2 )
		    sameh = SameH(ret->hstem,stack[0],stack[1], unblended,0);
		hint = HintNew(stack[0],stack[1]);
		hint->hintnumber = sameh!=NULL ? sameh->hintnumber : hint_cnt++;
		if ( activeh==NULL )
		    activeh = hp = hint;
		else {
		    for ( hp=activeh; hp->next!=NULL; hp = hp->next );
		    hp->next = hint;
		    hp = hint;
		}
		sameh = NULL;
		if ( !is_type2 )
		    sameh = SameH(ret->hstem,stack[2],stack[3], unblended,0);
		hp->next = HintNew(stack[2],stack[3]);
		hp->next->hintnumber = sameh!=NULL ? sameh->hintnumber : hint_cnt++;
		sameh = NULL;
		if ( !is_type2 )
		    sameh = SameH(ret->hstem,stack[4],stack[5], unblended,0);
		hp->next->next = HintNew(stack[4],stack[5]);
		hp->next->next->hintnumber = sameh!=NULL ? sameh->hintnumber : hint_cnt++;
		if ( !is_type2 && hp->next->next->hintnumber<96 ) {
		    if ( pending_hm==NULL )
			pending_hm = chunkalloc(sizeof(HintMask));
		    (*pending_hm)[hint->hintnumber>>3] |= 0x80>>(hint->hintnumber&0x7);
		    (*pending_hm)[hint->next->hintnumber>>3] |= 0x80>>(hint->next->hintnumber&0x7);
		    (*pending_hm)[hint->next->next->hintnumber>>3] |= 0x80>>(hint->next->next->hintnumber&0x7);
		}
		hp = hp->next->next;
		sp = 0;
	      break;
	      case 6: /* seac */	/* build accented characters */
 seac:
		if ( sp<5 ) LogError( _("Stack underflow on seac in %s\n"), name );
		if ( is_type2 ) {
			if ( v==6 ) LogError( _("%s\'s SEAC operator is invalid for Type2\n"), name );
			else LogError( _("%s\'s SEAC-like endchar operator is deprecated for Type2\n"), name );
		}
		/* stack[0] must be the lsidebearing of the accent. I'm not sure why */
		r1 = RefCharCreate();
		r2 = RefCharCreate();
		r2->transform[0] = 1; r2->transform[3]=1;
		r2->transform[4] = stack[1] - (stack[0]-ret->lsidebearing);
		r2->transform[5] = stack[2];
		/* the translation of the accent here is said to be relative */
		/*  to the origins of the base character. I think they place */
		/*  the origin at the left bearing. And they don't mean the  */
		/*  base char at all, they mean the current char's lbearing  */
		/*  (which is normally the same as the base char's, except   */
		/*  when I has a big accent (like diaerisis) */
		r1->transform[0] = 1; r1->transform[3]=1;
		r1->adobe_enc = stack[3];
		r2->adobe_enc = stack[4];
		if ( stack[3]<0 || stack[3]>=256 || stack[4]<0 || stack[4]>=256 ) {
		    LogError( _("Reference encoding out of bounds in %s\n"), name );
		    r1->adobe_enc = 0;
		    r2->adobe_enc = 0;
		}
		r1->next = r2;
		if ( rlast!=NULL ) rlast->next = r1;
		else ret->layers[ly_fore].refs = r1;
		ret->changedsincelasthinted = true;	/* seac glyphs contain no hints */
		rlast = r2;
		sp = 0;
	      break;
	      case 7: /* sbw */		/* generalized width/sidebearing command */
		if ( sp<4 ) LogError( _("Stack underflow on sbw in %s\n"), name );
		if ( is_type2 )
		    LogError( _("%s\'s sbw operator is not supported for Type2\n"), name );
		ret->lsidebearing = stack[0];
		/* stack[1] is lsidebearing y (only for vertical writing styles, CJK) */
		ret->width = stack[2];
		/* stack[3] is height (for vertical writing styles, CJK) */
		sp = 0;
	      break;
	      case 5: case 9: case 14: case 26:
		if ( sp<1 ) LogError( _("Stack underflow on unary operator in %s\n"), name );
		switch ( v ) {
		  case 5: stack[sp-1] = (stack[sp-1]==0); break;	/* not */
		  case 9: if ( stack[sp-1]<0 ) stack[sp-1]= -stack[sp-1]; break;	/* abs */
		  case 14: stack[sp-1] = -stack[sp-1]; break;		/* neg */
		  case 26: stack[sp-1] = sqrt(stack[sp-1]); break;	/* sqrt */
		  default: break;
		}
	      break;
	      case 3: case 4: case 10: case 11: case 12: case 15: case 24:
		if ( sp<2 ) LogError( _("Stack underflow on binary operator in %s\n"), name );
		else switch ( v ) {
		  case 3: /* and */
		    stack[sp-2] = (stack[sp-1]!=0 && stack[sp-2]!=0);
		  break;
		  case 4: /* and */
		    stack[sp-2] = (stack[sp-1]!=0 || stack[sp-2]!=0);
		  break;
		  case 10: /* add */
		    stack[sp-2] += stack[sp-1];
		  break;
		  case 11: /* sub */
		    stack[sp-2] -= stack[sp-1];
		  break;
		  case 12: /* div */
		    stack[sp-2] /= stack[sp-1];
		  break;
		  case 24: /* mul */
		    stack[sp-2] *= stack[sp-1];
		  break;
		  case 15: /* eq */
		    stack[sp-2] = (stack[sp-1]==stack[sp-2]);
		  break;
		  default:
		  break;
		}
		--sp;
	      break;
	      case 22: /* ifelse */
		if ( sp<4 ) LogError( _("Stack underflow on ifelse in %s\n"), name );
		else {
		    if ( stack[sp-2]>stack[sp-1] )
			stack[sp-4] = stack[sp-3];
		    sp -= 3;
		}
	      break;
	      case 23: /* random */
		/* This function returns something (0,1]. It's not clear to me*/
		/*  if rand includes 0 and RAND_MAX or not, but this approach */
		/*  should work no matter what */
		do {
		    stack[sp] = (rand()/(RAND_MAX-1));
		} while ( stack[sp]==0 || stack[sp]>1 );
		++sp;
	      break;
	      case 16: /* callothersubr */
		/* stack[sp-1] is the number of the thing to call in the othersubr array */
		/* stack[sp-2] is the number of args to grab off our stack and put on the */
		/*  real postscript stack */
		if ( is_type2 )
		    LogError( _("Type2 fonts do not support the Type1 callothersubrs operator") );
		if ( sp<2 || sp < 2+stack[sp-2] ) {
		    LogError( _("Stack underflow on callothersubr in %s\n"), name );
		    sp = 0;
		} else {
		    int tot = stack[sp-2], i, k, j;
		    popsp = 0;
		    for ( k=sp-3; k>=sp-2-tot; --k )
			pops[popsp++] = stack[k];
		    /* othersubrs 0-3 must be interpretted. 0-2 are Flex, 3 is Hint Replacement */
		    /* othersubrs 12,13 are for counter hints. We don't need to */
		    /*  do anything to ignore them */
		    /* Subroutines 14-18 are multiple master blenders. We need */
		    /*  to pay attention to them too */
		    switch ( (int) stack[sp-1] ) {
		      case 3: {
			/* when we weren't capabable of hint replacement we */
			/*  punted by putting 3 on the stack (T1 spec page 70) */
			/*  subroutine 3 is a noop */
			/*pops[popsp-1] = 3;*/
			ret->manualhints = false;
			/* We can manage hint substitution from hintmask though*/
			/*  well enough that we needn't clear the manualhints bit */
			ret->hstem = HintsAppend(ret->hstem,activeh); activeh=NULL;
			ret->vstem = HintsAppend(ret->vstem,activev); activev=NULL;
		      } break;
		      case 1: {
			/* Essentially what we want to do is draw a line from */
			/*  where we are at the beginning to where we are at */
			/*  the end. So we save the beginning here (this starts*/
			/*  the flex sequence), we ignore all calls to othersub*/
			/*  2, and when we get to othersub 0 we put everything*/
			/*  back to where it should be and free up whatever */
			/*  extranious junk we created along the way and draw */
			/*  our line. */
			/* Let's punt a little less, and actually figure out */
			/*  the appropriate rrcurveto commands and put in a */
			/*  dished serif */
			/* We should never get here in a type2 font. But we did*/
			/*  this code won't work if we follow type2 conventions*/
			/*  so turn off type2 until we get 0 callothersubrs */
			/*  which marks the end of the flex sequence */
			is_type2 = false;
			if ( cur!=NULL ) {
			    oldcur = cur;
			    cur->next = NULL;
			} else
			    LogError( _("Bad flex subroutine in %s\n"), name );
		      } break;
		      case 2: {
			/* No op */;
		      } break;
		      case 0: if ( oldcur!=NULL ) {
			SplinePointList *spl = oldcur->next;
			if ( spl!=NULL && spl->next!=NULL &&
				spl->next->next!=NULL &&
				spl->next->next->next!=NULL &&
				spl->next->next->next->next!=NULL &&
				spl->next->next->next->next->next!=NULL &&
				spl->next->next->next->next->next->next!=NULL ) {
			    BasePoint old_nextcp, mid_prevcp, mid, mid_nextcp,
				    end_prevcp, end;
			    old_nextcp	= spl->next->first->me;
			    mid_prevcp	= spl->next->next->first->me;
			    mid		= spl->next->next->next->first->me;
			    mid_nextcp	= spl->next->next->next->next->first->me;
			    end_prevcp	= spl->next->next->next->next->next->first->me;
			    end		= spl->next->next->next->next->next->next->first->me;
			    cur = oldcur;
			    if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
				cur->last->nextcp = old_nextcp;
				cur->last->nonextcp = false;
				pt = chunkalloc(sizeof(SplinePoint));
			        pt->hintmask = pending_hm; pending_hm = NULL;
				pt->prevcp = mid_prevcp;
				pt->me = mid;
				pt->nextcp = mid_nextcp;
				/*pt->flex = pops[2];*/
			        CheckMake(cur->last,pt);
				SplineMake3(cur->last,pt);
				cur->last = pt;
				pt = chunkalloc(sizeof(SplinePoint));
				pt->prevcp = end_prevcp;
				pt->me = end;
				pt->nonextcp = true;
			        CheckMake(cur->last,pt);
				SplineMake3(cur->last,pt);
				cur->last = pt;
			    } else
				LogError( _("No previous point on path in curveto from flex 0 in %s\n"), name );
			} else {
			    /* Um, something's wrong. Let's just draw a line */
			    /* do the simple method, which consists of creating */
			    /*  the appropriate line */
			    pt = chunkalloc(sizeof(SplinePoint));
			    pt->me.x = pops[1]; pt->me.y = pops[0];
			    pt->noprevcp = true; pt->nonextcp = true;
			    SplinePointListFree(oldcur->next); oldcur->next = NULL; spl = NULL;
			    cur = oldcur;
			    if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
				CheckMake(cur->last,pt);
				SplineMake3(cur->last,pt);
				cur->last = pt;
			    } else
				LogError( _("No previous point on path in lineto from flex 0 in %s\n"), name );
			}
			--popsp;
			cur->next = NULL;
			SplinePointListsFree(spl);
			oldcur = NULL;
		      } else
			LogError( _("Bad flex subroutine in %s\n"), name );

			is_type2 = context->is_type2;
			/* If we found a type2 font with a type1 flex sequence */
			/*  (an illegal idea, but never mind, someone gave us one)*/
			/*  then we had to turn off type2 untill the end of the */
			/*  flex sequence. Which is here */
		      break;
		      case 14: 		/* results in 1 blended value */
		      case 15:		/* results in 2 blended values */
		      case 16:		/* results in 3 blended values */
		      case 17:		/* results in 4 blended values */
		      case 18: {	/* results in 6 blended values */
			int cnt = stack[sp-1]-13;
			if ( cnt==5 ) cnt=6;
			if ( context->instance_count==0 )
			    LogError( _("Attempt to use a multiple master subroutine in a non-mm font in %s.\n"), name );
			else if ( tot!=cnt*context->instance_count )
			    LogError( _("Multiple master subroutine called with the wrong number of arguments in %s.\n"), name );
			else {
			    /* Hints need to keep track of the original blends */
			    if ( cnt==1 && !is_type2 ) {
				if ( sp-2-tot>=1 && (!old_last_was_b1 || stack[0]!=Blend(unblended[1],context))) {
				    unblended[0][0] = stack[0];
				    for ( i=1; i<context->instance_count; ++i )
					unblended[0][i] = 0;
			        } else
				    memcpy(unblended,unblended+1,context->instance_count*sizeof(real));
			        for ( j=0; j<context->instance_count; ++j )
				    unblended[1][j] = stack[sp-2-tot+j];
			    } else if ( cnt==2 && !is_type2 ) {
				unblended[0][0] = stack[sp-2-tot];
				unblended[1][0] = stack[sp-2-tot+1];
				for ( i=0; i<2; ++i )
				    for ( j=1; j<context->instance_count; ++j )
					unblended[i][j] = stack[sp-2-tot+2+i*(context->instance_count-1)+(j-1)];
			    }
			    popsp = 0;
			    for ( i=0; i<cnt; ++i ) {
				double sum = stack[sp-2-tot+ i];
				for ( j=1; j<context->instance_count; ++j )
				    sum += context->blend_values[j]*
					    stack[sp-2-tot+ cnt +i*(context->instance_count-1)+ j-1];
				pops[cnt-1-popsp++] = sum;
			    }
			}
		      } break;
		    }
		    sp = k+1;
		}
	      break;
	      case 20: /* put */
		if ( sp<2 ) LogError( _("Too few items on stack for put in %s\n"), name );
		else if ( stack[sp-1]<0 || stack[sp-1]>=32 ) LogError( _("Reference to transient memory out of bounds in put in %s\n"), name );
		else {
		    transient[(int)stack[sp-1]] = stack[sp-2];
		    sp -= 2;
		}
	      break;
	      case 21: /* get */
		if ( sp<1 ) LogError( _("Too few items on stack for get in %s\n"), name );
		else if ( stack[sp-1]<0 || stack[sp-1]>=32 ) LogError( _("Reference to transient memory out of bounds in put in %s\n"), name );
		else
		    stack[sp-1] = transient[(int)stack[sp-1]];
	      break;
	      case 17: /* pop */
		/* pops something from the postscript stack and pushes it on ours */
		/* used to get a return value from an othersubr call */
		/* Bleah. Adobe wants the pops to return the arguments if we */
		/*  don't understand the call. What use is the subroutine then?*/
		if ( popsp<=0 )
		    LogError( _("Pop stack underflow on pop in %s\n"), name );
		else
		    stack[sp++] = pops[--popsp];
	      break;
	      case 18: /* drop */
		if ( sp>0 ) --sp;
	      break;
	      case 27: /* dup */
		if ( sp>=1 ) {
		    stack[sp] = stack[sp-1];
		    ++sp;
		}
	      break;
	      case 28: /* exch */
		if ( sp>=2 ) {
		    real temp = stack[sp-1];
		    stack[sp-1] = stack[sp-2]; stack[sp-2] = temp;
		}
	      break;
	      case 29: /* index */
		if ( sp>=1 ) {
		    int index = stack[--sp];
		    if ( index<0 || sp<index+1 )
			LogError( _("Index out of range in %s\n"), name );
		    else {
			stack[sp] = stack[sp-index-1];
			++sp;
		    }
		}
	      break;
	      case 30: /* roll */
		if ( sp>=2 ) {
		    int j = stack[sp-1], N=stack[sp-2];
		    if ( N>sp || j>=N || j<0 || N<0 )
			LogError( _("roll out of range in %s\n"), name );
		    else if ( j==0 || N==0 )
			/* No op */;
		    else {
			real *temp = malloc(N*sizeof(real));
			int i;
			for ( i=0; i<N; ++i )
			    temp[i] = stack[sp-N+i];
			for ( i=0; i<N; ++i )
			    stack[sp-N+i] = temp[(i+j)%N];
			free(temp);
		    }
		}
	      break;
	      case 33: /* setcurrentpoint */
		if ( is_type2 )
		    LogError( _("Type2 fonts do not support the Type1 setcurrentpoint operator") );
		if ( sp<2 ) LogError( _("Stack underflow on setcurrentpoint in %s\n"), name );
		else {
		    current.x = stack[0];
		    current.y = stack[1];
		}
		sp = 0;
	      break;
	      case 34:	/* hflex */
	      case 35:	/* flex */
	      case 36:	/* hflex1 */
	      case 37:	/* flex1 */
		dy = dy3 = dy4 = dy5 = dy6 = 0;
		dx = stack[base++];
		if ( v!=34 )
		    dy = stack[base++];
		dx2 = stack[base++];
		dy2 = stack[base++];
		dx3 = stack[base++];
		if ( v!=34 && v!=36 )
		    dy3 = stack[base++];
		dx4 = stack[base++];
		if ( v!=34 && v!=36 )
		    dy4 = stack[base++];
		dx5 = stack[base++];
		if ( v==34 )
		    dy5 = -dy2;
		else
		    dy5 = stack[base++];
		switch ( v ) {
		    real xt, yt;
		    case 35:    /* flex */
			dx6 = stack[base++];
			dy6 = stack[base++];
			break;
		    case 34:    /* hflex */
			dx6 = stack[base++];
			break;
		    case 36:    /* hflex1 */
			dx6 = stack[base++];
			dy6 = -dy-dy2-dy5;
			break;
		    case 37:    /* flex1 */
			xt = dx+dx2+dx3+dx4+dx5;
			yt = dy+dy2+dy3+dy4+dy5;
			if ( xt<0 ) xt= -xt;
			if ( yt<0 ) yt= -yt;
			if ( xt>yt ) {
			    dx6 = stack[base++];
			    dy6 = -dy-dy2-dy3-dy4-dy5;
			} else {
			    dy6 = stack[base++];
			    dx6 = -dx-dx2-dx3-dx4-dx5;
			}
			break;
		}
		if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
		    current.x = rint((current.x+dx)*1024)/1024; current.y = rint((current.y+dy)*1024)/1024;
		    cur->last->nextcp.x = current.x; cur->last->nextcp.y = current.y;
		    cur->last->nonextcp = false;
		    current.x = rint((current.x+dx2)*1024)/1024; current.y = rint((current.y+dy2)*1024)/1024;
		    pt = chunkalloc(sizeof(SplinePoint));
		    pt->hintmask = pending_hm; pending_hm = NULL;
		    pt->prevcp.x = current.x; pt->prevcp.y = current.y;
		    current.x = rint((current.x+dx3)*1024)/1024; current.y = rint((current.y+dy3)*1024)/1024;
		    pt->me.x = current.x; pt->me.y = current.y;
		    pt->nonextcp = true;
		    CheckMake(cur->last,pt);
		    SplineMake3(cur->last,pt);
		    cur->last = pt;

		    current.x = rint((current.x+dx4)*1024)/1024; current.y = rint((current.y+dy4)*1024)/1024;
		    cur->last->nextcp.x = current.x; cur->last->nextcp.y = current.y;
		    cur->last->nonextcp = false;
		    current.x = rint((current.x+dx5)*1024)/1024; current.y = rint((current.y+dy5)*1024)/1024;
		    pt = chunkalloc(sizeof(SplinePoint));
		    pt->prevcp.x = current.x; pt->prevcp.y = current.y;
		    current.x = rint((current.x+dx6)*1024)/1024; current.y = rint((current.y+dy6)*1024)/1024;
		    pt->me.x = current.x; pt->me.y = current.y;
		    pt->nonextcp = true;
		    CheckMake(cur->last,pt);
		    SplineMake3(cur->last,pt);
		    cur->last = pt;
		} else
		    LogError( _("No previous point on path in flex operator in %s\n"), name );
		sp = 0;
	      break;
	      default:
		LogError( _("Uninterpreted opcode 12,%d in %s\n"), v, name );
	      break;
	    }
	} else { last_was_b1 = false; switch ( v ) {
	  case 1: /* hstem */
	  case 18: /* hstemhm */
	    base = 0;
	    if ( (sp&1) && ret->width == (int16) 0x8000 )
		ret->width = stack[0];
	    if ( sp&1 )
		base=1;
	    if ( sp-base<2 )
		LogError( _("Stack underflow on hstem in %s\n"), name );
	    /* stack[0] is absolute y for start of horizontal hint */
	    /*	(actually relative to the y specified as lsidebearing y in sbw*/
	    /* stack[1] is relative y for height of hint zone */
	    coord = 0;
	    hp = NULL;
	    if ( activeh!=NULL )
		for ( hp=activeh; hp->next!=NULL; hp = hp->next );
	    while ( sp-base>=2 ) {
		sameh = NULL;
		if ( !is_type2 )
		    sameh = SameH(ret->hstem,stack[base]+coord,stack[base+1],
				unblended,context->instance_count);
		hint = HintNew(stack[base]+coord,stack[base+1]);
		hint->hintnumber = sameh!=NULL ? sameh->hintnumber : hint_cnt++;
		if ( !is_type2 && context->instance_count!=0 ) {
		    hint->u.unblended = chunkalloc(sizeof(real [2][MmMax]));
		    memcpy(hint->u.unblended,unblended,sizeof(real [2][MmMax]));
		}
		if ( activeh==NULL )
		    activeh = hint;
		else
		    hp->next = hint;
		hp = hint;
		if ( !is_type2 && hint->hintnumber<96 ) {
		    if ( pending_hm==NULL )
			pending_hm = chunkalloc(sizeof(HintMask));
		    (*pending_hm)[hint->hintnumber>>3] |= 0x80>>(hint->hintnumber&0x7);
		}
		base+=2;
		coord = hint->start+hint->width;
	    }
	    sp = 0;
	    break;
	  case 19: /* hintmask */
	  case 20: /* cntrmask */
	    /* If there's anything on the stack treat it as a vstem hint */
	  case 3: /* vstem */
	  case 23: /* vstemhm */
	    base = 0;
	    if ( cur==NULL || v==3 || v==23 ) {
		if ( (sp&1) && is_type2 && ret->width == (int16) 0x8000 ) {
		    ret->width = stack[0];
		}
		if ( sp&1 )
		    base=1;
		/* I've seen a vstemhm with no arguments. I've no idea what that */
		/*  means. It came right after a hintmask */
		/* I'm confused about v/hstemhm because the manual says it needs */
		/*  to be used if one uses a hintmask, but that's not what the */
		/*  examples show.  Or I'm not understanding. */
		if ( sp-base<2 && v!=19 && v!=20 )
		    LogError( _("Stack underflow on vstem in %s\n"), name );
		/* stack[0] is absolute x for start of vertical hint */
		/*	(actually relative to the x specified as lsidebearing in h/sbw*/
		/* stack[1] is relative x for height of hint zone */
		coord = ret->lsidebearing;
		hp = NULL;
		if ( activev!=NULL )
		    for ( hp=activev; hp->next!=NULL; hp = hp->next );
		while ( sp-base>=2 ) {
		    sameh = NULL;
		    if ( !is_type2 )
			sameh = SameH(ret->vstem,stack[base]+coord,stack[base+1],
				    unblended,context->instance_count);
		    hint = HintNew(stack[base]+coord,stack[base+1]);
		    hint->hintnumber = sameh!=NULL ? sameh->hintnumber : hint_cnt++;
		    if ( !is_type2 && context->instance_count!=0 ) {
			hint->u.unblended = chunkalloc(sizeof(real [2][MmMax]));
			memcpy(hint->u.unblended,unblended,sizeof(real [2][MmMax]));
		    }
		    if ( !is_type2 && hint->hintnumber<96 ) {
			if ( pending_hm==NULL )
			    pending_hm = chunkalloc(sizeof(HintMask));
			(*pending_hm)[hint->hintnumber>>3] |= 0x80>>(hint->hintnumber&0x7);
		    }
		    if ( activev==NULL )
			activev = hint;
		    else
			hp->next = hint;
		    hp = hint;
		    base+=2;
		    coord = hint->start+hint->width;
		}
		sp = 0;
	    }
	    if ( v==19 || v==20 ) {		/* hintmask, cntrmask */
		int bytes = (hint_cnt+7)/8;
		if ( bytes>sizeof(HintMask) ) bytes = sizeof(HintMask);
		if ( v==19 ) {
		    ret->hstem = HintsAppend(ret->hstem,activeh); activeh=NULL;
		    ret->vstem = HintsAppend(ret->vstem,activev); activev=NULL;
		    if ( pending_hm==NULL )
			pending_hm = chunkalloc(sizeof(HintMask));
		    memcpy(pending_hm,type1,bytes);
		} else if ( cp<sizeof(counters)/sizeof(counters[0]) ) {
		    counters[cp] = chunkalloc(sizeof(HintMask));
		    memcpy(counters[cp],type1,bytes);
		    ++cp;
		}
		if ( bytes!=hint_cnt/8 ) {
		    int mask = 0xff>>(hint_cnt&7);
		    if ( type1[bytes-1]&mask )
			LogError( _("Hint mask (or counter mask) with too many hints in %s\n"), name );
		}
		type1 += bytes;
		len -= bytes;
	    }
	  break;
	  case 14: /* endchar */
	    /* endchar is allowed to terminate processing even within a subroutine */
	    if ( (sp&1) && is_type2 && ret->width == (int16) 0x8000 )
		ret->width = stack[0];
	    if ( context->painttype!=2 )
		closepath(cur,is_type2);
	    pcsp = 0;
	    if ( sp==4 ) {
		/* In Type2 strings endchar has a deprecated function of doing */
		/*  a seac (which doesn't exist at all). Except enchar takes */
		/*  4 args and seac takes 5. Bleah */
		stack[4] = stack[3]; stack[3] = stack[2]; stack[2] = stack[1]; stack[1] = stack[0];
		stack[0] = 0;
		sp = 5;
  goto seac;
	    } else if ( sp==5 ) {
		/* same as above except also specified a width */
		stack[0] = 0;
  goto seac;
	    }
	    /* the docs say that endchar must be the last command in a char */
	    /*  (or the last command in a subroutine which is the last in the */
	    /*  char) So in theory if there's anything left we should complain*/
	    /*  In practice though, the EuroFont has a return statement after */
	    /*  the endchar in a subroutine. So we won't try to catch that err*/
	    /*  and just stop. */
	    /* Adobe says it's not an error, but I can't understand their */
	    /*  logic */
  goto done;
	  break;
	  case 13: /* hsbw (set left sidebearing and width) */
	    if ( sp<2 ) LogError( _("Stack underflow on hsbw in %s\n"), name );
	    ret->lsidebearing = stack[0];
	    current.x = stack[0];		/* sets the current point too */
	    ret->width = stack[1];
	    sp = 0;
	  break;
	  case 9: /* closepath */
	    sp = 0;
	    closepath(cur,is_type2);
	  break;
	  case 21: /* rmoveto */
	  case 22: /* hmoveto */
	  case 4: /* vmoveto */
	    if ( is_type2 ) {
		if (( (v==21 && sp==3) || (v!=21 && sp==2))  && ret->width == (int16) 0x8000 )
		    /* Character's width may be specified on the first moveto */
		    ret->width = stack[0];
		if ( v==21 && sp>2 ) {
		    stack[0] = stack[sp-2]; stack[1] = stack[sp-1];
		    sp = 2;
		} else if ( v!=21 && sp>1 ) {
		    stack[0] = stack[sp-1];
		    sp = 1;
		}
		if ( context->painttype!=2 )
		    closepath(cur,true);
	    }
	  case 5: /* rlineto */
	  case 6: /* hlineto */
	  case 7: /* vlineto */
	    polarity = 0;
	    base = 0;
	    while ( base<sp ) {
		dx = dy = 0;
		if ( v==5 || v==21 ) {
		    if ( sp<base+2 ) {
			LogError( _("Stack underflow on rlineto/rmoveto in %s\n"), name );
	    break;
		    }
		    dx = stack[base++];
		    dy = stack[base++];
		} else if ( (v==6 && !(polarity&1)) || (v==7 && (polarity&1)) || v==22 ) {
		    if ( sp<=base ) {
			LogError( _("Stack underflow on hlineto/hmoveto in %s\n"), name );
	    break;
		    }
		    dx = stack[base++];
		} else /*if ( (v==7 && !(parity&1)) || (v==6 && (parity&1) || v==4 )*/ {
		    if ( sp<=base ) {
			LogError( _("Stack underflow on vlineto/vmoveto in %s\n"), name );
	    break;
		    }
		    dy = stack[base++];
		}
		++polarity;
		current.x = rint((current.x+dx)*1024)/1024; current.y = rint((current.y+dy)*1024)/1024;
		pt = chunkalloc(sizeof(SplinePoint));
		pt->hintmask = pending_hm; pending_hm = NULL;
		pt->me.x = current.x; pt->me.y = current.y;
		pt->noprevcp = true; pt->nonextcp = true;
		if ( v==4 || v==21 || v==22 ) {
		    if ( cur!=NULL && cur->first==cur->last && cur->first->prev==NULL && is_type2 ) {
			/* Two adjacent movetos should not create single point paths */
			cur->first->me.x = current.x; cur->first->me.y = current.y;
			SplinePointFree(pt);
		    } else {
			SplinePointList *spl = chunkalloc(sizeof(SplinePointList));
			spl->first = spl->last = pt;
			if ( cur!=NULL )
			    cur->next = spl;
			else
			    ret->layers[ly_fore].splines = spl;
			cur = spl;
		    }
	    break;
		} else {
		    if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
			CheckMake(cur->last,pt);
			SplineMake3(cur->last,pt);
			cur->last = pt;
		    } else
			LogError( _("No previous point on path in lineto in %s\n"), name );
		    if ( !is_type2 )
	    break;
		}
	    }
	    sp = 0;
	  break;
	  case 25: /* rlinecurve */
	    base = 0;
	    while ( sp>base+6 ) {
		current.x = rint((current.x+stack[base++])*1024)/1024; current.y = rint((current.y+stack[base++])*1024)/1024;
		if ( cur!=NULL ) {
		    pt = chunkalloc(sizeof(SplinePoint));
		    pt->hintmask = pending_hm; pending_hm = NULL;
		    pt->me.x = current.x; pt->me.y = current.y;
		    pt->noprevcp = true; pt->nonextcp = true;
		    CheckMake(cur->last,pt);
		    SplineMake3(cur->last,pt);
		    cur->last = pt;
		}
	    }
	  case 24: /* rcurveline */
	  case 8:  /* rrcurveto */
	  case 31: /* hvcurveto */
	  case 30: /* vhcurveto */
	  case 27: /* hhcurveto */
	  case 26: /* vvcurveto */
	    polarity = 0;
	    while ( sp>base+2 ) {
		dx = dy = dx2 = dy2 = dx3 = dy3 = 0;
		if ( v==8 || v==25 || v==24 ) {
		    if ( sp<6+base ) {
			LogError( _("Stack underflow on rrcurveto in %s\n"), name );
			base = sp;
		    } else {
			dx = stack[base++];
			dy = stack[base++];
			dx2 = stack[base++];
			dy2 = stack[base++];
			dx3 = stack[base++];
			dy3 = stack[base++];
		    }
		} else if ( v==27 ) {		/* hhcurveto */
		    if ( sp<4+base ) {
			LogError( _("Stack underflow on hhcurveto in %s\n"), name );
			base = sp;
		    } else {
			if ( (sp-base)&1 ) dy = stack[base++];
			dx = stack[base++];
			dx2 = stack[base++];
			dy2 = stack[base++];
			dx3 = stack[base++];
		    }
		} else if ( v==26 ) {		/* vvcurveto */
		    if ( sp<4+base ) {
			LogError( _("Stack underflow on hhcurveto in %s\n"), name );
			base = sp;
		    } else {
			if ( (sp-base)&1 ) dx = stack[base++];
			dy = stack[base++];
			dx2 = stack[base++];
			dy2 = stack[base++];
			dy3 = stack[base++];
		    }
		} else if ( (v==31 && !(polarity&1)) || (v==30 && (polarity&1)) ) {
		    if ( sp<4+base ) {
			LogError( _("Stack underflow on hvcurveto in %s\n"), name );
			base = sp;
		    } else {
			dx = stack[base++];
			dx2 = stack[base++];
			dy2 = stack[base++];
			dy3 = stack[base++];
			if ( sp==base+1 )
			    dx3 = stack[base++];
		    }
		} else /*if ( (v==30 && !(polarity&1)) || (v==31 && (polarity&1)) )*/ {
		    if ( sp<4+base ) {
			LogError( _("Stack underflow on vhcurveto in %s\n"), name );
			base = sp;
		    } else {
			dy = stack[base++];
			dx2 = stack[base++];
			dy2 = stack[base++];
			dx3 = stack[base++];
			if ( sp==base+1 )
			    dy3 = stack[base++];
		    }
		}
		++polarity;
		if ( cur!=NULL && cur->first!=NULL && (cur->first!=cur->last || cur->first->next==NULL) ) {
		    current.x = rint((current.x+dx)*1024)/1024; current.y = rint((current.y+dy)*1024)/1024;
		    cur->last->nextcp.x = current.x; cur->last->nextcp.y = current.y;
		    cur->last->nonextcp = false;
		    current.x = rint((current.x+dx2)*1024)/1024; current.y = rint((current.y+dy2)*1024)/1024;
		    pt = chunkalloc(sizeof(SplinePoint));
		    pt->hintmask = pending_hm; pending_hm = NULL;
		    pt->prevcp.x = current.x; pt->prevcp.y = current.y;
		    current.x = rint((current.x+dx3)*1024)/1024; current.y = rint((current.y+dy3)*1024)/1024;
		    pt->me.x = current.x; pt->me.y = current.y;
		    pt->nonextcp = true;
		    CheckMake(cur->last,pt);
		    SplineMake3(cur->last,pt);
		    cur->last = pt;
		} else
		    LogError( _("No previous point on path in curveto in %s\n"), name );
	    }
	    if ( v==24 ) {
		current.x = rint((current.x+stack[base++])*1024)/1024; current.y = rint((current.y+stack[base++])*1024)/1024;
		if ( cur!=NULL ) {	/* In legal code, cur can't be null here, but I got something illegal... */
		    pt = chunkalloc(sizeof(SplinePoint));
		    pt->hintmask = pending_hm; pending_hm = NULL;
		    pt->me.x = current.x; pt->me.y = current.y;
		    pt->noprevcp = true; pt->nonextcp = true;
		    CheckMake(cur->last,pt);
		    SplineMake3(cur->last,pt);
		    cur->last = pt;
		}
	    }
	    sp = 0;
	  break;
	  case 29: /* callgsubr */
	  case 10: /* callsubr */
	    /* stack[sp-1] contains the number of the subroutine to call */
	    if ( sp<1 ) {
		LogError( _("Stack underflow on callsubr in %s\n"), name );
	  break;
	    } else if ( pcsp>10 ) {
		LogError( _("Too many subroutine calls in %s\n"), name );
	  break;
	    }
	    s=subrs; if ( v==29 ) s = gsubrs;
	    if ( s!=NULL ) stack[sp-1] += s->bias;
	    /* Type2 subrs have a bias that must be added to the subr-number */
	    /* Type1 subrs do not. We set the bias on them to 0 */
	    if ( s==NULL || stack[sp-1]>=s->cnt || stack[sp-1]<0 ||
		    s->values[(int) stack[sp-1]]==NULL )
		LogError( _("Subroutine number out of bounds in %s\n"), name );
	    else {
		pcstack[pcsp].type1 = type1;
		pcstack[pcsp].len = len;
		pcstack[pcsp].subnum = stack[sp-1];
		++pcsp;
		type1 = s->values[(int) stack[sp-1]];
		len = s->lens[(int) stack[sp-1]];
	    }
	    if ( --sp<0 ) sp = 0;
	  break;
	  case 11: /* return */
	    /* return from a subr outine */
	    if ( pcsp<1 ) LogError( _("return when not in subroutine in %s\n"), name );
	    else {
		--pcsp;
		type1 = pcstack[pcsp].type1;
		len = pcstack[pcsp].len;
	    }
	  break;
	  case 16: { /* blend -- obsolete type 2 multiple master operator */
	    int cnt,i,j;
	    if ( context->instance_count==0 )
		LogError( _("Attempt to use a multiple master subroutine in a non-mm font.\n") );
	    else if ( sp<1 || sp<context->instance_count*stack[sp-1]+1 )
		LogError( _("Too few items on stack for blend in %s\n"), name );
	    else {
		if ( !context->blend_warn ) {
		    LogError( _("Use of obsolete blend operator.\n") );
		    context->blend_warn = true;
		}
		cnt = stack[sp-1];
		sp -= context->instance_count*stack[sp-1]+1;
		for ( i=0; i<cnt; ++i ) {
		    for ( j=1; j<context->instance_count; ++j )
			stack[sp+i] += context->blend_values[j]*stack[sp+
				cnt+ i*(context->instance_count-1)+ j-1];
		}
		/* there will always be fewer pushes than there were pops */
		/*  so I don't bother to check the stack */
		sp += cnt;
	    }
	  }
	  break;
	  default:
	    LogError( _("Uninterpreted opcode %d in %s\n"), v, name );
	  break;
	}}
    }
  done:
    if ( pcsp!=0 )
	LogError( _("end of subroutine reached with no return in %s\n"), name );
    SCCategorizePoints(ret);

    ret->hstem = HintsAppend(ret->hstem,activeh); activeh=NULL;
    ret->vstem = HintsAppend(ret->vstem,activev); activev=NULL;

    if ( cp!=0 ) { int i;
	ret->countermasks = malloc(cp*sizeof(HintMask));
	ret->countermask_cnt = cp;
	for ( i=0; i<cp; ++i ) {
	    memcpy(&ret->countermasks[i],counters[i],sizeof(HintMask));
	    chunkfree(counters[i],sizeof(HintMask));
	}
    }

    /* Even in type1 fonts all paths should be closed. But if we close them at*/
    /*  the obvious moveto, that breaks flex hints. So we have a hack here at */
    /*  the end which closes any open paths. */
    /* If we do have a PaintType 2 font, then presumably the difference between*/
    /*  open and closed paths matters */
    if ( !is_type2 && !context->painttype )
	for ( cur = ret->layers[ly_fore].splines; cur!=NULL; cur = cur->next ) if ( cur->first->prev==NULL ) {
	    CheckMake(cur->last,cur->first);
	    SplineMake3(cur->last,cur->first);
	    cur->last = cur->first;
	}

    /* Oh, I see. PS and TT disagree on which direction to use, so Fontographer*/
    /*  chose the TT direction and we must reverse postscript */
    for ( cur = ret->layers[ly_fore].splines; cur!=NULL; cur = cur->next )
	SplineSetReverse(cur);
    if ( ret->hstem==NULL && ret->vstem==NULL )
	ret->manualhints = false;
    if ( !is_type2 && context->instance_count!=0 ) {
	UnblendFree(ret->hstem);
	UnblendFree(ret->vstem);
    }
    ret->hstem = HintCleanup(ret->hstem,true,context->instance_count);
    ret->vstem = HintCleanup(ret->vstem,true,context->instance_count);

    SCGuessHHintInstancesList(ret,ly_fore);
    SCGuessVHintInstancesList(ret,ly_fore);

    ret->hconflicts = StemListAnyConflicts(ret->hstem);
    ret->vconflicts = StemListAnyConflicts(ret->vstem);
    if ( context->instance_count==1 && !ret->hconflicts && !ret->vconflicts )
	SCClearHintMasks(ret,ly_fore,false);
    HintsRenumber(ret);
    if ( name!=NULL && strcmp(name,".notdef")!=0 )
	ret->widthset = true;
return( ret );
}
