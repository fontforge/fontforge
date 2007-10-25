/* Copyright (C) 2003-2007 by George Williams */
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
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

enum operator {
    op_base = 0x100,			/* Bigger than any character */

    op_x, op_y,				/* Returns current x & y values, no operands */
    op_value,				/* Returns a constant value */
    op_negate, op_not,			/* Unary operators: op1 */
    op_log, op_exp, op_sqrt, op_sin, op_cos, op_tan,
    op_abs, op_rint, op_floor, op_ceil,
    op_pow,				/* Binary operators: op1, op2 */
    op_atan2,
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
    void *pov;
    void (*pov_func)(BasePoint *me,void *);
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
    { "atan2", op_atan2 },
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
	while ( isalnum(c->cur[0])) {
	    if ( pt<buffer+sizeof(buffer)-1)
		*pt++ = c->cur[0];
	    ++c->cur;
	}
	*pt = '\0';
	for ( i=0; builtins[i].name!=NULL; ++i ) {
	    if ( strcmp(buffer,builtins[i].name)==0 )
return( builtins[i].op );
	}
	gwwv_post_error(_("Bad Token"), _("Bad token \"%.30s\"\nnear ...%40s"), buffer, c->cur );
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
	gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\" got \"%.10s\"\nnear ...%40s"), "==", "=" , c->cur );
	c->had_error = true;
return( op_eq );
      case '|':
	if ( *c->cur=='|' ) {
	    ++c->cur;
return( op_or );
	}
	gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\" got \"%.10s\"\nnear ...%40s"), "||", "|" , c->cur );
	c->had_error = true;
return( op_or );
      case '&':
	if ( *c->cur=='&' ) {
	    ++c->cur;
return( op_and );
	}
	gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\" got \"%.10s\"\nnear ...%40s"), "&&", "&" , c->cur );
	c->had_error = true;
return( op_and );
      case '?':
return( op_if );
      case '(': case ')': case ':': case ',':
return( ch );
      default:
	gwwv_post_error(_("Bad Token"), _("Bad token. got \"%1$c\"\nnear ...%2$40s"), ch , c->cur );
	c->had_error = true;
	*val = 0;
return( op_value );
    }
}

static void backup(struct context *c,enum operator op, real val ) {
    if ( c->backed_token!=op_base ) {
	IError( "Attempt to back up twice.\nnear ...%s\n", c->cur );
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
	    gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), ")" , c->cur );
	    c->had_error = true;
	}
return(ret );
      case op_log: case op_exp: case op_sqrt:
      case op_sin: case op_cos: case op_tan:
      case op_atan2:
      case op_abs:
      case op_rint: case op_floor: case op_ceil:
	ret = gcalloc(1,sizeof(struct expr));
	ret->operator = op;
	op = gettoken(c,&val);
	if ( op!='(' ) {
	    gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), "(" , c->cur );
	    c->had_error = true;
	}
	ret->op1 = getexpr(c);
	op = gettoken(c,&val);
	if ( ret->operator==op_atan2 ) {
	    if ( op!=',' ) {
		gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), "," , c->cur );
	    }
	    ret->op2 = getexpr(c);
	    op = gettoken(c,&val);
	}
	if ( op!=')' ) {
	    gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), ")" , c->cur );
	    c->had_error = true;
	}
return( ret );
      case op_add:
	/* Just ignore a unary plus */;
return( gete0(c));
      case op_sub: case op_not:
	ret = gcalloc(1,sizeof(struct expr));
	ret->operator = op;
	ret->op1 = gete0(c);
return( ret );
      default:
	gwwv_post_error(_("Bad Token"), _("Unexpected token.\nbefore ...%40s") , c->cur );
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
	    gwwv_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), ":" , c->cur );
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
	gwwv_post_error(_("Bad Token"), _("Unexpected token after expression end.\nbefore ...%40s") , c->cur );
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
		gwwv_post_error(_("Bad Value"),_("Attempt to take logarithm of %1$g in %2$.30s"), val1, c->sc->name );
		c->had_error = true;
return( 0 );
	    }
return( log(val1));
	  case op_sqrt:
	    if ( val1<0 ) {
		gwwv_post_error(_("Bad Value"),_("Attempt to take the square root of %1$g in %2$.30s"), val1, c->sc->name );
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
      case op_atan2:
return( atan2(evaluate_expr(c,e->op1),evaluate_expr(c,e->op2)) );
      case op_pow:
return( pow(evaluate_expr(c,e->op1),evaluate_expr(c,e->op2)) );
      case op_times:
return( evaluate_expr(c,e->op1) * evaluate_expr(c,e->op2) );
      case op_div: case op_mod:
	val2 = evaluate_expr(c,e->op2);
	if ( val2==0 ) {
	    gwwv_post_error(_("Bad Value"),_("Attempt to divide by 0 in %.30s"), c->sc->name );
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
	IError( "Bad operator %d in %s\n", e->operator, c->sc->name );
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

    if ( c->pov_func!=NULL ) {
	(c->pov_func)(&sp->me,c->pov);
	(c->pov_func)(&sp->prevcp,c->pov);
	(c->pov_func)(&sp->nextcp,c->pov);
    } else {
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
		    if ( c->pov_func==NULL ) {
			mids[i].x = NL_expr(c,c->x_expr);
			mids[i].y = NL_expr(c,c->y_expr);
		    } else {
			BasePoint temp;
			temp.x = c->x;
			temp.y = c->y;
			(c->pov_func)(&temp,c->pov);
			mids[i].x = temp.x; mids[i].y = temp.y;
		    }
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
    SplineSetBeziersClear(ss);
    SplineSetSpirosClear(ss);
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
    int i, gid;

    SFUntickAll(fv->sf);

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		(sc = fv->sf->glyphs[gid])!=NULL && !sc->ticked ) {
	    SCNLTrans(sc,c);
	    sc->ticked = true;
	}
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		(sc=fv->sf->glyphs[gid])!=NULL &&
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
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Non Linear Transform...");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,97);
    d.gw = GDrawCreateTopWindow(NULL,&pos,nld_e_h,&d,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

/* GT: an expression describing the transformation applied to the X coordinate */
    label[0].text = (unichar_t *) _("X Expr:");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 8;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[0].gd.popup_msg = (unichar_t *) _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
    gcd[0].creator = GLabelCreate;

    if ( lastx!=NULL )
	label[1].text = lastx;
    else {
	label[1].text = (unichar_t *) "x";
	label[1].text_is_1byte = true;
    }
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 55; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 135;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[1].gd.popup_msg = (unichar_t *) _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
    gcd[1].creator = GTextFieldCreate;

/* GT: an expression describing the transformation applied to the Y coordinate */
    label[2].text = (unichar_t *) _("Y Expr:");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 10; gcd[2].gd.pos.y = gcd[0].gd.pos.y+26;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[2].gd.popup_msg = (unichar_t *) _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
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
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[3].gd.popup_msg = (unichar_t *) _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
    gcd[3].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = 30-3; gcd[4].gd.pos.y = gcd[3].gd.pos.y+30;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _("_OK");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.cid = true;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = -30; gcd[5].gd.pos.y = gcd[4].gd.pos.y+3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _("_Cancel");
    label[5].text_is_1byte = true;
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
	    c.had_error = false;
	    if ( (c.x_expr = parseexpr(&c,expstr))==NULL )
		d.done = d.ok = false;
	    else {
		free(expstr);
		c.had_error = false;
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

static GTextInfo originx[] = {
    { (unichar_t *) N_("Glyph Origin"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Center of Selection"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
/* GT: The (x,y) position on the window where the user last pressed a mouse button */
    { (unichar_t *) N_("Last Press"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Value"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }};
static GTextInfo originy[] = {
    { (unichar_t *) N_("Glyph Origin"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Center of Selection"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
/* GT: The (x,y) position on the window where the user last pressed a mouse button */
    { (unichar_t *) N_("Last Press"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Value"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }};
#define CID_XType	1001
#define CID_YType	1002
#define CID_XValue	1003
#define CID_YValue	1004
#define CID_ZValue	1005
#define CID_DValue	1006
#define CID_Tilt	1007
#define CID_GazeDirection 1008
#define CID_Vanish	1009

static real GetQuietReal(GWindow gw,int cid,int *err) {
    const unichar_t *txt; unichar_t *end;
    real val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtod(txt,&end);
    if ( *end!='\0' )
	*err = true;
return( val );
}

static void PoV_DoVanish(struct nldlg *d) {
    double x,y,dv,tilt,dir;
    double t;
    int err = false;
    double vp;
    char buf[80];
    unichar_t ubuf[80];
extern char *coord_sep;

    x = GetQuietReal(d->gw,CID_XValue,&err);
    y = GetQuietReal(d->gw,CID_YValue,&err);
    /*z = GetQuietReal(d->gw,CID_ZValue,&err);*/
    dv = GetQuietReal(d->gw,CID_DValue,&err);
    tilt = GetQuietReal(d->gw,CID_Tilt,&err)*3.1415926535897932/180;
    dir = GetQuietReal(d->gw,CID_GazeDirection,&err)*3.1415926535897932/180;
    if ( err )
return;
    if ( GGadgetGetFirstListSelectedItem( GWidgetGetControl(d->gw,CID_XType))!=3 )
	x = 0;
    if ( GGadgetGetFirstListSelectedItem( GWidgetGetControl(d->gw,CID_YType))!=3 )
	y = 0;
    t = tan(tilt);
    if ( t<.000001 && t>-.000001 )
	sprintf(buf,"inf%sinf", coord_sep);
    else {
	vp = dv/t;
	x -= sin(dir)*vp;
	y += cos(dir)*vp;
	sprintf(buf,"%g%s%g", x, coord_sep, y );
    }
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle( GWidgetGetControl(d->gw,CID_Vanish), ubuf );
}

static int PoV_Vanish(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct nldlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	PoV_DoVanish(d);
    }
return( true );
}

int PointOfViewDlg(struct pov_data *pov, SplineFont *sf, int flags) {
    static struct pov_data def = { or_center, or_value, 0, 0, .1,
	    0, 3.1415926535897932/16, .2 };
    double emsize = (sf->ascent + sf->descent);
    struct nldlg d;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[24];
    GTextInfo label[24];
    int i,k;
    char xval[40], yval[40], zval[40], dval[40], tval[40], dirval[40];
    double x,y,z,dv,tilt,dir;
    int err;
    static int done = false;

    if ( !done ) {
	done = true;
	for ( i=0; originx[i].text!=NULL; ++i )
	    originx[i].text = (unichar_t *) _((char *) originx[i].text);
	for ( i=0; originy[i].text!=NULL; ++i )
	    originy[i].text = (unichar_t *) _((char *) originy[i].text);
    }

    *pov = def;
    pov->x *= emsize; pov->y *= emsize; pov->z *= emsize; pov->d *= emsize;
    if ( !(flags&1) ) {
	if ( pov->xorigin == or_lastpress ) pov->xorigin = or_center;
	if ( pov->yorigin == or_lastpress ) pov->yorigin = or_center;
    }

    memset(&d,'\0',sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Point of View Projection...");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,240));
    pos.height = GDrawPointsToPixels(NULL,216);
    d.gw = GDrawCreateTopWindow(NULL,&pos,nld_e_h,&d,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    k=0;
    label[k].text = (unichar_t *) _("View Point");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 8;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("_X");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y + 16;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    for ( i=or_zero; i<or_undefined; ++i ) originx[i].selected = false;
    originx[pov->xorigin].selected = true;
    originx[or_lastpress].disabled = !(flags&1);
    gcd[k].gd.pos.x = 23; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.label = &originx[pov->xorigin];
    gcd[k].gd.u.list = originx;
    gcd[k].gd.cid = CID_XType;
    gcd[k++].creator = GListButtonCreate;

    sprintf( xval, "%g", rint(pov->x));
    label[k].text = (unichar_t *) xval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;  gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_XValue;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("_Y");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y + 28;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    for ( i=or_zero; i<or_undefined; ++i ) originy[i].selected = false;
    originy[pov->yorigin].selected = true;
    originy[or_lastpress].disabled = !(flags&1);
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.label = &originy[pov->yorigin];
    gcd[k].gd.u.list = originy;
    gcd[k].gd.cid = CID_YType;
    gcd[k++].creator = GListButtonCreate;

    sprintf( yval, "%g", rint(pov->y));
    label[k].text = (unichar_t *) yval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;  gcd[k].gd.pos.width = gcd[k-3].gd.pos.width;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_YValue;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("Distance to drawing plane:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y + 28;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    sprintf( zval, "%g", rint(pov->z));
    label[k].text = (unichar_t *) zval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_ZValue;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("Distance to projection plane:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y + 24;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    sprintf( dval, "%g", rint(pov->d));
    label[k].text = (unichar_t *) dval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_DValue;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("Drawing plane tilt:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y + 24;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    sprintf( tval, "%g", rint(pov->tilt*180/3.1415926535897932));
    label[k].text = (unichar_t *) tval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_Tilt;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) U_("°");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+gcd[k-1].gd.pos.width+3; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("Direction of gaze:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-3].gd.pos.y + 24;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    sprintf( dirval, "%g", rint(pov->direction*180/3.1415926535897932));
    label[k].text = (unichar_t *) dirval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_GazeDirection;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) U_("°");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+gcd[k-1].gd.pos.width+3; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("Vanishing Point:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+18;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("This is the approximate location of the vanishing point.\nIt does not include the offset induced by \"Center of selection\"\nnor \"Last Press\".");
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) "123456.,123456.";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("This is the approximate location of the vanishing point.\nIt does not include the offset induced by \"Center of selection\"\nnor \"Last Press\".");
    gcd[k].gd.cid = CID_Vanish;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+18;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = true;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = false;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[k].creator = GGroupCreate;

    GGadgetsCreate(d.gw,gcd);
    PoV_DoVanish(&d);
    GDrawSetVisible(d.gw,true);
    while ( !d.done ) {
	GDrawProcessOneEvent(NULL);
	if ( d.done ) {
	    if ( !d.ok ) {
		GDrawDestroyWindow(d.gw);
return( -1 );
	    }
	    err = false;
	    x = GetReal8(d.gw,CID_XValue,_("_X"),&err);
	    y = GetReal8(d.gw,CID_YValue,_("_Y"),&err);
	    z = GetReal8(d.gw,CID_ZValue,_("Distance to drawing plane:"),&err);
	    dv = GetReal8(d.gw,CID_DValue,_("Distance to projection plane:"),&err);
	    tilt = GetReal8(d.gw,CID_Tilt,_("Drawing plane tilt:"),&err);
	    dir = GetReal8(d.gw,CID_GazeDirection,_("Direction of gaze:"),&err);
	    if ( err ) {
		d.done = d.ok = false;
    continue;
	    }
	    pov->x = x; pov->y = y; pov->z = z; pov->d = dv;
	    pov->tilt = tilt*3.1415926535897932/180;
	    pov->direction = dir*3.1415926535897932/180;
	    pov->xorigin = GGadgetGetFirstListSelectedItem( GWidgetGetControl(d.gw,CID_XType));
	    pov->yorigin = GGadgetGetFirstListSelectedItem( GWidgetGetControl(d.gw,CID_YType));
	}
    }

    GDrawDestroyWindow(d.gw);
    def = *pov;
    def.x /= emsize; def.y /= emsize; def.z /= emsize; def.d /= emsize;
return( 0 );		/* -1 => Canceled */
}

static void SPLPoV(SplineSet *spl,struct pov_data *pov, int only_selected);

void CVPointOfView(CharView *cv,struct pov_data *pov) {
    int anysel = CVAnySel(cv,NULL,NULL,NULL,NULL);
    BasePoint origin;

    CVPreserveState(cv);

    origin.x = origin.y = 0;
    if ( pov->xorigin==or_center || pov->yorigin==or_center )
	CVFindCenter(cv,&origin,!anysel);
    if ( pov->xorigin==or_lastpress )
	origin.x = cv->p.cx;
    if ( pov->yorigin==or_lastpress )
	origin.y = cv->p.cy;
    if ( pov->xorigin!=or_value )
	pov->x = origin.x;
    if ( pov->yorigin!=or_value )
	pov->y = origin.y;

    MinimumDistancesFree(cv->sc->md); cv->sc->md = NULL;
    SPLPoV(cv->layerheads[cv->drawmode]->splines,pov,anysel);
    CVCharChangedUpdate(cv);
}
# endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static void BpPoV(BasePoint *me,void *_pov) {
    struct pov_data *pov = _pov;
    double z, div;

    z = pov->z + me->y*pov->sintilt;
    div = z/pov->d;
    if ( z< .000001 && z> -.000001 ) {
	me->x = (me->x<0) ? 32768 : 32767;
	me->y = (me->y<0) ? 32768 : 32767;
    } else {
	me->x /= div;
	me->y /= div;
	if ( me->x>32767 ) me->x = 32767;
	else if ( me->x<-32768 ) me->x = -32768;
	if ( me->y>32767 ) me->y = 32767;
	else if ( me->y<-32768 ) me->y = -32768;
    }
}

static void SPLPoV(SplineSet *base,struct pov_data *pov, int only_selected) {
    SplineSet *spl;
    real transform[6];
    double si = sin( pov->direction ), co = cos( pov->direction );
    struct context c;

    if ( pov->z==0 )
return;

    transform[0] = transform[3] = co;
    transform[2] = -(transform[1] = si);
    transform[4] = -pov->x;
    transform[5] = -pov->y;
    SplinePointListTransform(base,transform,!only_selected);

    if ( pov->d==0 || pov->tilt==0 ) {
	transform[0] = transform[3] = pov->d/pov->z;
	transform[1] = transform[2] = transform[4] = transform[5] = 0;
	SplinePointListTransform(base,transform,!only_selected);
return;
    }

    memset(&c,0,sizeof(c)); c.pov = pov; c.pov_func = BpPoV;
    pov->sintilt = sin(pov->tilt);
    for ( spl = base; spl!=NULL; spl = spl->next ) {
	SplineSetNLTrans(spl,&c,!only_selected);
    }
    SPLAverageCps(base);

    transform[0] = transform[3] = co;
    transform[1] = -(transform[2] = si);
    transform[4] = pov->x;
    transform[5] = pov->y;
    SplinePointListTransform(base,transform,!only_selected);
}

static void SCFindCenter(SplineChar *sc,BasePoint *center) {
    DBounds db;
    SplineCharFindBounds(sc,&db);
    center->x = (db.minx+db.maxx)/2;
    center->y = (db.miny+db.maxy)/2;
}

void FVPointOfView(FontView *fv,struct pov_data *pov) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    int i, cnt=0, layer, gid;
    BasePoint origin;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( (gid=fv->map->map[i])!=-1 && fv->sf->glyphs[gid]!=NULL &&
		fv->selected[i] )
	++cnt;
# ifdef FONTFORGE_CONFIG_GDRAW
    gwwv_progress_start_indicator(10,_("Projecting..."),_("Projecting..."),0,cnt,1);
# elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_start_indicator(10,_("Projecting..."),_("Projecting..."),0,cnt,1);
# endif
#else
    int i, layer;
    BasePoint origin;
#endif	/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( (gid = fv->map->map[i])!=-1 && fv->selected[i] &&
		(sc = fv->sf->glyphs[gid])!=NULL && !sc->ticked ) {
	    sc->ticked = true;
	    SCPreserveState(sc,false);

	    origin.x = origin.y = 0;
	    if ( pov->xorigin==or_center || pov->yorigin==or_center )
		SCFindCenter(sc,&origin);
	    if ( pov->xorigin!=or_value )
		pov->x = origin.x;
	    if ( pov->yorigin!=or_value )
		pov->y = origin.y;

	    MinimumDistancesFree(sc->md); sc->md = NULL;
	    for ( layer = ly_fore; layer<sc->layer_cnt; ++layer )
		SPLPoV(sc->layers[layer].splines,pov,false);
	    SCCharChangedUpdate(sc);
	}
    }
}

struct vanishing_point {
    double x_vanish;
    double y_vanish;
};

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void VanishingTrans(BasePoint *me,void *_vanish) {
    struct vanishing_point *vanish = _vanish;

    me->x = vanish->x_vanish + (vanish->y_vanish - me->y)/vanish->y_vanish*
		( me->x-vanish->x_vanish );
}

void CVYPerspective(CharView *cv,double x_vanish, double y_vanish) {
    SplineSet *spl;
    struct context c;
    struct vanishing_point vanish;

    if ( y_vanish==0 )		/* Leave things unchanged */
return;

    vanish.x_vanish = x_vanish;
    vanish.y_vanish = y_vanish;
    memset(&c,0,sizeof(c)); c.pov = &vanish; c.pov_func = VanishingTrans;
    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	SplineSetNLTrans(spl,&c,false);
    }
}
#endif	/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
