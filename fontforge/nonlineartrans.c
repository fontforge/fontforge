/* Copyright (C) 2003,2004 by George Williams */
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
#include "pfaeditui.h"
#include <utype.h>
#include <ustring.h>
#include <math.h>

#ifdef FONTFORGE_CONFIG_NONLINEAR
enum operator {
    op_base = 0x100,			/* Bigger than any character */

    op_x, op_y,				/* Returns current x & y values, no operands */
    op_value,				/* Returns a constant value */
    op_negate, op_not,			/* Unary operators: op1 */
    op_log, op_exp, op_sqrt, op_sin, op_cos, op_tan, op_abs, op_rint, op_floor, op_ceil,
    op_pow,				/* Binary operators: op1, op2 */
    op_times, op_div, op_mod,
    op_add, op_sub,
    op_eq, op_ne, op_le, op_lt, op_gt, op_ge,
    op_and, op_or,
    op_if				/* Trinary operator: op1 ? op2 : op3 */
};

struct expr {
    enum operator operator;
    struct expr *op1, *op2, *op3;
    real value;
};

struct context {
    char *start, *cur;
    unsigned int had_error: 1;
    enum operator backed_token;
    real backed_val;

    real x, y;
    struct expr *x_expr, *y_expr;
    SplineChar *sc;
};

static struct builtins { char *name; enum operator op; } builtins[] = {
    { "x", op_x },
    { "y", op_y },
    { "log", op_log },
    { "exp", op_exp },
    { "sqrt", op_sqrt },
    { "sin", op_sin },
    { "cos", op_cos },
    { "tan", op_tan },
    { "abs", op_abs },
    { "rint", op_rint },
    { "floor", op_floor },
    { "ceil", op_ceil },
    { NULL }
};

static void exprfree(struct expr *e) {
    if ( e==NULL )
return;
    exprfree(e->op1);
    exprfree(e->op2);
    exprfree(e->op3);
    chunkfree(e,sizeof(*e));
}

static int gettoken(struct context *c, real *val) {
    int ch, i;
    char *end, *pt;
    char buffer[40];

    if ( c->backed_token!=op_base ) {
	ch = c->backed_token;
	if ( ch==op_value )
	    *val = c->backed_val;
	c->backed_token = op_base;
return( ch );
    }

    while (( ch = *(c->cur++))==' ' );

    if ( isdigit(ch) || ch=='.' ) {
	--(c->cur);
	*val = strtod(c->cur,&end);
	c->cur = end;
return( op_value );
    } else if ( isalpha(ch)) {
	pt = buffer; *pt++=ch;
	while ( isalpha(c->cur[0])) {
	    if ( pt<buffer+sizeof(buffer)-1)
		*pt++ = c->cur[0];
	    ++c->cur;
	}
	*pt = '\0';
	for ( i=0; builtins[i].name!=NULL; ++i ) {
	    if ( strcmp(buffer,builtins[i].name)==0 )
return( builtins[i].op );
	}
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Token"), _("Bad token \"%.30s\"\nnear ...%40s"), buffer, c->cur );
#else
	GWidgetErrorR(_STR_BadToken, _STR_BadNameToken, buffer, c->cur );
#endif
	c->had_error = true;
	while (( ch = *(c->cur++))==' ' );
	if ( ch=='(' )
return( op_abs );
	*val = 0;
return( op_value );
    } else switch ( ch ) {
      case '\0':
	--(c->cur);
return( 0 );
      case '!':
	if ( *c->cur=='=' ) {
	    ++c->cur;
return( op_ne );
	}
return( op_not );
      case '-':
return( op_sub );
      case '+':
return( op_add );
      case '*':
return( op_times );
      case '/':
return( op_div );
      case '%':
return( op_mod );
      case '^':
return( op_pow );
      case '>':
	if ( *c->cur=='=' ) {
	    ++c->cur;
return( op_ge );
	}
return( op_gt );
      case '<':
	if ( *c->cur=='=' ) {
	    ++c->cur;
return( op_le );
	}
return( op_lt );
      case '=':
	if ( *c->cur=='=' ) {
	    ++c->cur;
return( op_eq );
	}
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\" got \"%.10s\"\nnear ...%40s"), "==", "=" , c->cur );
#else
	GWidgetErrorR(_STR_BadToken, _STR_BadTokenExpected, "==", "=" , c->cur );
#endif
	c->had_error = true;
return( op_eq );
      case '|':
	if ( *c->cur=='|' ) {
	    ++c->cur;
return( op_or );
	}
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\" got \"%.10s\"\nnear ...%40s"), "||", "|" , c->cur );
#else
	GWidgetErrorR(_STR_BadToken, _STR_BadTokenExpected, "||", "|" , c->cur );
#endif
	c->had_error = true;
return( op_or );
      case '&':
	if ( *c->cur=='&' ) {
	    ++c->cur;
return( op_and );
	}
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\" got \"%.10s\"\nnear ...%40s"), "&&", "&" , c->cur );
#else
	GWidgetErrorR(_STR_BadToken, _STR_BadTokenExpected, "&&", "&" , c->cur );
#endif
	c->had_error = true;
return( op_and );
      case '?':
return( op_if );
      case '(': case ')': case ':':
return( ch );
      default:
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Token"), _("Bad token. got \"%c\"\nnear ...%40s"), ch , c->cur );
#else
	GWidgetErrorR(_STR_BadToken, _STR_BadTokenChar, ch , c->cur );
#endif
	c->had_error = true;
	*val = 0;
return( op_value );
    }
}

static void backup(struct context *c,enum operator op, real val ) {
    if ( c->backed_token!=op_base ) {
	GDrawIError( "Attempt to back up twice.\nnear ...%s\n", c->cur );
	c->had_error = true;
    }
    c->backed_token = op;
    if ( op==op_value )
	c->backed_val = val;
}

static struct expr *getexpr(struct context *c);

static struct expr *gete0(struct context *c) {
    real val = 0;
    enum operator op = gettoken(c,&val);
    struct expr *ret;

    switch ( op ) {
      case op_value: case op_x: case op_y:
	ret = gcalloc(1,sizeof(struct expr));
	ret->operator = op;
	ret->value = val;
return( ret );
      case '(':
	ret = getexpr(c);
	op = gettoken(c,&val);
	if ( op!=')' ) {
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), ")" , c->cur );
#else
	    GWidgetErrorR(_STR_BadToken, _STR_BadTokenExpectedChar, ")" , c->cur );
#endif
	    c->had_error = true;
	}
return(ret );
      case op_log: case op_exp: case op_sqrt:
      case op_sin: case op_cos: case op_tan:
      case op_abs:
      case op_rint: case op_floor: case op_ceil:
	ret = gcalloc(1,sizeof(struct expr));
	ret->operator = op;
	op = gettoken(c,&val);
	if ( op!='(' ) {
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), "(" , c->cur );
#else
	    GWidgetErrorR(_STR_BadToken, _STR_BadTokenExpectedChar, "(" , c->cur );
#endif
	    c->had_error = true;
	}
	ret->op1 = getexpr(c);
	op = gettoken(c,&val);
	if ( op!=')' ) {
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), ")" , c->cur );
#else
	    GWidgetErrorR(_STR_BadToken, _STR_BadTokenExpectedChar, ")" , c->cur );
#endif
	    c->had_error = true;
	}
return( ret );
      case op_add:
	/* Just ignore a unary plus */;
return( gete0(c));
      case op_sub: case op_not:
	ret->operator = op;
	ret->op1 = gete0(c);
return( ret );
      default:
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Token"), _("Unexpected token.\nbefore ...%40s") , c->cur );
#else
	GWidgetErrorR(_STR_BadToken, _STR_UnexpectedToken , c->cur );
#endif
	c->had_error = true;
	ret = gcalloc(1,sizeof(struct expr));
	ret->operator = op_value;
	ret->value = val;
return( ret );
    }
}

static struct expr *gete1(struct context *c) {
    real val = 0;
    enum operator op;
    struct expr *ret, *op1;

    op1 = gete0(c);
    op = gettoken(c,&val);
    while ( op==op_pow ) {
	ret = gcalloc(1,sizeof(struct expr));
	ret->op1 = op1;
	ret->operator = op;
	ret->op2 = gete0(c);
	op1 = ret;
	op = gettoken(c,&val);
    }
    backup(c,op,val);
return( op1 );
}

static struct expr *gete2(struct context *c) {
    real val = 0;
    enum operator op;
    struct expr *ret, *op1;

    op1 = gete1(c);
    op = gettoken(c,&val);
    while ( op==op_times || op==op_div || op==op_mod ) {
	ret = gcalloc(1,sizeof(struct expr));
	ret->op1 = op1;
	ret->operator = op;
	ret->op2 = gete1(c);
	op1 = ret;
	op = gettoken(c,&val);
    }
    backup(c,op,val);
return( op1 );
}

static struct expr *gete3(struct context *c) {
    real val = 0;
    enum operator op;
    struct expr *ret, *op1;

    op1 = gete2(c);
    op = gettoken(c,&val);
    while ( op==op_add || op==op_sub ) {
	ret = gcalloc(1,sizeof(struct expr));
	ret->op1 = op1;
	ret->operator = op;
	ret->op2 = gete2(c);
	op1 = ret;
	op = gettoken(c,&val);
    }
    backup(c,op,val);
return( op1 );
}

static struct expr *gete4(struct context *c) {
    real val = 0;
    enum operator op;
    struct expr *ret, *op1;

    op1 = gete3(c);
    op = gettoken(c,&val);
    while ( op==op_eq || op==op_ne || op==op_lt || op==op_le || op==op_gt || op==op_ge ) {
	ret = gcalloc(1,sizeof(struct expr));
	ret->op1 = op1;
	ret->operator = op;
	ret->op2 = gete3(c);
	op1 = ret;
	op = gettoken(c,&val);
    }
    backup(c,op,val);
return( op1 );
}

static struct expr *gete5(struct context *c) {
    real val = 0;
    enum operator op;
    struct expr *ret, *op1;

    op1 = gete4(c);
    op = gettoken(c,&val);
    while ( op==op_and || op==op_or ) {
	ret = gcalloc(1,sizeof(struct expr));
	ret->op1 = op1;
	ret->operator = op;
	ret->op2 = gete4(c);
	op1 = ret;
	op = gettoken(c,&val);
    }
    backup(c,op,val);
return( op1 );
}

static struct expr *getexpr(struct context *c) {
    real val = 0;
    enum operator op;
    struct expr *ret, *op1;

    op1 = gete5(c);
    op = gettoken(c,&val);
    if ( op==op_if ) {
	ret = gcalloc(1,sizeof(struct expr));
	ret->op1 = op1;
	ret->operator = op;
	ret->op2 = getexpr(c);
	op = gettoken(c,&val);
	if ( op!=':' ) {
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), ":" , c->cur );
#else
	    GWidgetErrorR(_STR_BadToken, _STR_BadTokenExpectedChar, ":" , c->cur );
#endif
	    c->had_error = true;
	}
	ret->op3 = getexpr(c);
return( ret );
    } else {
	backup(c,op,val);
return( op1 );
    }
}

static struct expr *parseexpr(struct context *c,char *str) {
    struct expr *ret;

    c->backed_token = op_base;
    c->start = c->cur = str;
    ret = getexpr(c);
    if ( *c->cur!='\0' ) {
	c->had_error = true;
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Token"), _("Unexpected token after expression end.\nbefore ...%40s") , c->cur );
#else
	GWidgetErrorR(_STR_BadToken, _STR_UnexpectedTokenAtEnd , c->cur );
#endif
    }
    if ( c->had_error ) {
	exprfree(ret);
return( NULL );
    }
return( ret );
}

static real evaluate_expr(struct context *c,struct expr *e) {
    real val1, val2;

    switch ( e->operator ) {
      case op_value:
return( e->value );
      case op_x:
return( c->x );
      case op_y:
return( c->y );
      case op_negate:
return( -evaluate_expr(c,e->op1) );
      case op_not:
return( !evaluate_expr(c,e->op1) );
      case op_log: case op_exp: case op_sqrt:
      case op_sin: case op_cos: case op_tan:
      case op_abs:
      case op_rint: case op_floor: case op_ceil:
	val1 = evaluate_expr(c,e->op1);
	switch ( e->operator ) {
	  case op_log:
	    if ( val1<=0 ) {
#if defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Bad Value"),_("Attempt to take logarithem of %g in %.30s"), val1, c->sc->name );
#else
		GWidgetErrorR(_STR_BadValue,_STR_BadLogarithem, val1, c->sc->name );
#endif
		c->had_error = true;
return( 0 );
	    }
return( log(val1));
	  case op_sqrt:
	    if ( val1<0 ) {
#if defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Bad Value"),_("Attempt to take the square root of %g in %.30s"), val1, c->sc->name );
#else
		GWidgetErrorR(_STR_BadValue,_STR_BadSqrt, val1, c->sc->name );
#endif
		c->had_error = true;
return( 0 );
	    }
return( sqrt(val1));
	  case op_exp:
return( exp(val1));
	  case op_sin:
return( sin(val1));
	  case op_cos:
return( cos(val1));
	  case op_tan:
return( tan(val1));
	  case op_abs:
return( val1<0?-val1:val1 );
	  case op_rint:
return( rint(val1));
	  case op_floor:
return( floor(val1));
	  case op_ceil:
return( ceil(val1));
	}
      case op_pow:
return( pow(evaluate_expr(c,e->op1),evaluate_expr(c,e->op2)) );
      case op_times:
return( evaluate_expr(c,e->op1) * evaluate_expr(c,e->op2) );
      case op_div: case op_mod:
	val2 = evaluate_expr(c,e->op2);
	if ( val2==0 ) {
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Bad Value"),_("Attempt divide by 0 in %.30s"), c->sc->name );
#else
	    GWidgetErrorR(_STR_BadValue,_STR_DivideByZero, c->sc->name );
#endif
	    c->had_error = true;
return( 0 );
	}
	if ( e->operator==op_div )
return( evaluate_expr(c,e->op1)/val2 );
return( fmod(evaluate_expr(c,e->op1),val2) );
      case op_add:
return( evaluate_expr(c,e->op1) + evaluate_expr(c,e->op2) );
      case op_sub:
return( evaluate_expr(c,e->op1) - evaluate_expr(c,e->op2) );
      case op_eq:
return( evaluate_expr(c,e->op1) == evaluate_expr(c,e->op2) );
      case op_ne:
return( evaluate_expr(c,e->op1) != evaluate_expr(c,e->op2) );
      case op_le:
return( evaluate_expr(c,e->op1) <= evaluate_expr(c,e->op2) );
      case op_lt:
return( evaluate_expr(c,e->op1) < evaluate_expr(c,e->op2) );
      case op_ge:
return( evaluate_expr(c,e->op1) >= evaluate_expr(c,e->op2) );
      case op_gt:
return( evaluate_expr(c,e->op1) > evaluate_expr(c,e->op2) );
      case op_and:
	val1 = evaluate_expr(c,e->op1);
	if ( val1==0 )
return( 0 );
return( evaluate_expr(c,e->op1)!=0 );
      case op_or:
	val1 = evaluate_expr(c,e->op1);
	if ( val1!=0 )
return( 1 );
return( evaluate_expr(c,e->op1)!=0 );
      case op_if:
	val1 = evaluate_expr(c,e->op1);
	if ( val1!=0 )
return( evaluate_expr(c,e->op2) );
	else
return( evaluate_expr(c,e->op3) );
      default:
	GDrawIError( "Bad operator %d in %s\n", e->operator, c->sc->name );
	c->had_error = true;
return( 0 );
    }
}

static real NL_expr(struct context *c,struct expr *e) {
    real val = evaluate_expr(c,e);
    if ( isnan(val))
return( 0 );
    if ( val>=32768 )
return( 32767 );
    else if ( val<-32768 )
return( -32768 );

return( val );
}

static void NLTransPoint(SplinePoint *sp,struct context *c) {

    c->x = sp->me.x; c->y = sp->me.y;
    sp->me.x = NL_expr(c,c->x_expr);
    sp->me.y = NL_expr(c,c->y_expr);
    c->x = sp->prevcp.x; c->y = sp->prevcp.y;
    sp->prevcp.x = NL_expr(c,c->x_expr);
    sp->prevcp.y = NL_expr(c,c->y_expr);
    c->x = sp->nextcp.x; c->y = sp->nextcp.y;
    sp->nextcp.x = NL_expr(c,c->x_expr);
    sp->nextcp.y = NL_expr(c,c->y_expr);
}

static void SplineSetNLTrans(SplineSet *ss,struct context *c,
	int everything) {
    SplinePoint *first, *last, *next;
    SplinePoint *sp;
    TPoint mids[20];
    double t;
    int i;
    Spline1D *xsp, *ysp;
    /* When doing a linear transform, all we need to do is transform the */
    /*  end and control points and figure the new spline and it works. A */
    /*  non-linear transform is harder, we must transform each point along */
    /*  the spline and then approximate a new spline to match. There is no */
    /*  guarantee that we'll get a good match. Straight lines may become */
    /*  curves, curves may become higher order curves (which we still approx */
    /*  imate with cubics) */

    first = last = chunkalloc(sizeof(SplinePoint));
    *first = *ss->first;
    first->hintmask = NULL;
    first->next = first->prev = NULL;
    if ( everything || first->selected )
	NLTransPoint(first,c);

    if ( ss->first->next!=NULL ) {
	for ( sp=ss->first->next->to; sp!=NULL; ) {
	    next = chunkalloc(sizeof(SplinePoint));
	    *next = *sp;
	    next->hintmask = NULL;
	    next->next = next->prev = NULL;
	    if ( everything || next->selected )
		NLTransPoint(next,c);
	    if ( everything || (next->selected && last->selected) ) {
		xsp = &sp->prev->splines[0]; ysp = &sp->prev->splines[1];
		for ( i=0; i<20; ++i ) {
		    t = (i+1)/21.0;
		    c->x = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
		    c->y = ((ysp->a*t+ysp->b)*t+ysp->c)*t + ysp->d;
		    mids[i].t = t;
		    mids[i].x = NL_expr(c,c->x_expr);
		    mids[i].y = NL_expr(c,c->y_expr);
		}
		ApproximateSplineFromPoints(last,next,mids,20,false);
	    } else
		SplineMake3(last,next);
	    last = next;
	    if ( sp==ss->first )
	break;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	}
	if ( ss->first->prev ) {
	    first->prev = last->prev;
	    first->prevcp = last->prevcp;
	    first->noprevcp = last->noprevcp;
	    first->prevcpdef = false;
	    first->prev->to = first;
	    SplinePointFree(last);
	    last = first;
	}
	for ( next=first ; ; ) {
	    if ( next->next==NULL )
	break;
	    if ( everything || next->selected )
		SPSmoothJoint(next);
	    next = next->next->to;
	    if ( next==first )
	break;
	}
    }
    SplinePointsFree(ss);
    ss->first = first;
    ss->last = last;
}

static void SCNLTrans(SplineChar *sc,struct context *c) {
    SplineSet *ss;
    RefChar *ref;
#ifdef FONTFORGE_CONFIG_TYPE3
    int i;

    if ( sc->layer_cnt==ly_fore+1 &&
	    sc->layers[ly_fore].splines==NULL && sc->layers[ly_fore].refs==NULL )
return;

    SCPreserveState(sc,false);
    c->sc = sc;
    for ( i=ly_fore; i<sc->layer_cnt; ++i ) {
	for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next )
	    SplineSetNLTrans(ss,c,true);
	for ( ref=sc->layers[i].refs; ref!=NULL; ref=ref->next ) {
	    c->x = ref->transform[4]; c->y = ref->transform[5];
	    ref->transform[4] = NL_expr(c,c->x_expr);
	    ref->transform[5] = NL_expr(c,c->y_expr);
	    /* we'll fix up the splines after all characters have been transformed*/
	}
    }
#else

    if ( sc->layers[ly_fore].splines==NULL && sc->layers[ly_fore].refs==NULL )
return;

    SCPreserveState(sc,false);
    c->sc = sc;
    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next )
	SplineSetNLTrans(ss,c,true);
    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	c->x = ref->transform[4]; c->y = ref->transform[5];
	ref->transform[4] = NL_expr(c,c->x_expr);
	ref->transform[5] = NL_expr(c,c->y_expr);
	/* we'll fix up the splines after all characters have been transformed*/
    }
#endif
}

static void _SFNLTrans(FontView *fv,struct context *c) {
    SplineChar *sc;
    RefChar *ref;
    int i;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] && fv->sf->chars[i]!=NULL )
	SCNLTrans(fv->sf->chars[i],c);
    for ( i=0; i<fv->sf->charcnt; ++i )
	if ( fv->selected[i] && (sc=fv->sf->chars[i])!=NULL &&
		(sc->layers[ly_fore].splines!=NULL || sc->layers[ly_fore].refs!=NULL)) {
	    /* A reference doesn't really work after a non-linear transform */
	    /*  but let's do the obvious thing */
	    for ( ref = sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
		SCReinstanciateRefChar(sc,ref);
	    SCCharChangedUpdate(sc);
	}
}

int SFNLTrans(FontView *fv,char *x_expr,char *y_expr) {
    struct context c;

    memset(&c,0,sizeof(c));
    if ( (c.x_expr = parseexpr(&c,x_expr))==NULL )
return( false );
    if ( (c.y_expr = parseexpr(&c,y_expr))==NULL ) {
	exprfree(c.x_expr);
return( false );
    }

    _SFNLTrans(fv,&c);

    exprfree(c.x_expr);
    exprfree(c.y_expr);
return( true );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void CVNLTrans(CharView *cv,struct context *c) {
    SplineSet *ss;
    RefChar *ref;

    if ( cv->layerheads[cv->drawmode]->splines==NULL && (cv->drawmode!=dm_fore || cv->sc->layers[ly_fore].refs==NULL ))
return;

    CVPreserveState(cv);
    c->sc = cv->sc;
    for ( ss=cv->layerheads[cv->drawmode]->splines; ss!=NULL; ss=ss->next )
	SplineSetNLTrans(ss,c,false);
    if ( cv->drawmode==dm_fore ) {
	for ( ref=cv->layerheads[cv->drawmode]->refs; ref!=NULL; ref=ref->next ) {
	    c->x = ref->transform[4]; c->y = ref->transform[5];
	    ref->transform[4] = NL_expr(c,c->x_expr);
	    ref->transform[5] = NL_expr(c,c->y_expr);
	    SCReinstanciateRefChar(cv->sc,ref);
	}
    }
    CVCharChangedUpdate(cv);
}

struct nldlg {
    GWindow gw;
    int done, ok;
};

static int nld_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct nldlg *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype==et_buttonactivate ) {
	struct nldlg *d = GDrawGetUserData(gw);
	d->done = true;
	d->ok = GGadgetGetCid(event->u.control.g);
    }
return( true );
}

void NonLinearDlg(FontView *fv,CharView *cv) {
    static unichar_t *lastx, *lasty;
    struct nldlg d;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct context c;
    char *expstr;

    memset(&d,'\0',sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(_STR_NonLinearTransform,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = _("Non Linear Transform...");
#endif
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,97);
    d.gw = GDrawCreateTopWindow(NULL,&pos,nld_e_h,&d,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    label[0].text = (unichar_t *) _STR_XExpr;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 8;
    gcd[0].gd.flags = gg_visible | gg_enabled;
#if defined(FONTFORGE_CONFIG_GDRAW)
    gcd[0].gd.popup_msg = GStringGetResource(_STR_ExprPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    gcd[0].gd.popup_msg = _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
#endif
    gcd[0].creator = GLabelCreate;

    if ( lastx!=NULL )
	label[1].text = lastx;
    else {
	label[1].text = (unichar_t *) "x";
	label[1].text_is_1byte = true;
    }
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 55; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 135;
    gcd[1].gd.flags = gg_visible | gg_enabled;
#if defined(FONTFORGE_CONFIG_GDRAW)
    gcd[1].gd.popup_msg = GStringGetResource(_STR_ExprPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    gcd[1].gd.popup_msg = _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
#endif
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _STR_YExpr;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 10; gcd[2].gd.pos.y = gcd[0].gd.pos.y+26;
    gcd[2].gd.flags = gg_visible | gg_enabled;
#if defined(FONTFORGE_CONFIG_GDRAW)
    gcd[2].gd.popup_msg = GStringGetResource(_STR_ExprPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    gcd[2].gd.popup_msg = _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
#endif
    gcd[2].creator = GLabelCreate;

    if ( lastx!=NULL )
	label[3].text = lasty;
    else {
	label[3].text = (unichar_t *) "y";
	label[3].text_is_1byte = true;
    }
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[1].gd.pos.y+26;
    gcd[3].gd.pos.width = gcd[1].gd.pos.width;
    gcd[3].gd.flags = gg_visible | gg_enabled;
#if defined(FONTFORGE_CONFIG_GDRAW)
    gcd[3].gd.popup_msg = GStringGetResource(_STR_ExprPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    gcd[3].gd.popup_msg = _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
#endif
    gcd[3].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = 30-3; gcd[4].gd.pos.y = gcd[3].gd.pos.y+30;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _STR_OK;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.cid = true;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = -30; gcd[5].gd.pos.y = gcd[4].gd.pos.y+3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _STR_Cancel;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.cid = false;
    gcd[5].creator = GButtonCreate;

    gcd[6].gd.pos.x = 2; gcd[6].gd.pos.y = 2;
    gcd[6].gd.pos.width = pos.width-4; gcd[6].gd.pos.height = pos.height-4;
    gcd[6].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[6].creator = GGroupCreate;

    GGadgetsCreate(d.gw,gcd);
    GDrawSetVisible(d.gw,true);
    while ( !d.done ) {
	GDrawProcessOneEvent(NULL);
	if ( d.done && d.ok ) {
	    expstr = cu_copy(_GGadgetGetTitle(gcd[1].ret));
	    if ( (c.x_expr = parseexpr(&c,expstr))==NULL )
		d.done = d.ok = false;
	    else {
		free(expstr);
		expstr = cu_copy(_GGadgetGetTitle(gcd[3].ret));
		if ( (c.y_expr = parseexpr(&c,expstr))==NULL ) {
		    d.done = d.ok = false;
		    exprfree(c.x_expr);
		} else {
		    free(expstr);
		    free(lasty); free(lastx);
		    lastx = GGadgetGetTitle(gcd[1].ret);
		    lasty = GGadgetGetTitle(gcd[3].ret);
		}
	    }
	}
    }
    if ( d.ok ) {
	if ( fv!=NULL )
	    _SFNLTrans(fv,&c);
	else
	    CVNLTrans(cv,&c);
	exprfree(c.x_expr);
	exprfree(c.y_expr);
    }
    GDrawDestroyWindow(d.gw);
}
# endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
#endif		/* FONTFORGE_CONFIG_NONLINEAR */
