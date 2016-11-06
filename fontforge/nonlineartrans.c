/* Copyright (C) 2003-2012 by George Williams */
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
#include "cvundoes.h"
#include "fontforgevw.h"
#include <utype.h>
#include <ustring.h>
#include <math.h>
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif
#include "nonlineartrans.h"

static struct builtins { const char *name; enum operator op; } builtins[] = {
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
    { NULL, 0 }
};

void nlt_exprfree(struct expr *e) {
    if ( e==NULL )
return;
    nlt_exprfree(e->op1);
    nlt_exprfree(e->op2);
    nlt_exprfree(e->op3);
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
	ff_post_error(_("Bad Token"), _("Bad token \"%.30s\"\nnear ...%40s"), buffer, c->cur );
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
	ff_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\" got \"%.10s\"\nnear ...%40s"), "==", "=" , c->cur );
	c->had_error = true;
return( op_eq );
      case '|':
	if ( *c->cur=='|' ) {
	    ++c->cur;
return( op_or );
	}
	ff_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\" got \"%.10s\"\nnear ...%40s"), "||", "|" , c->cur );
	c->had_error = true;
return( op_or );
      case '&':
	if ( *c->cur=='&' ) {
	    ++c->cur;
return( op_and );
	}
	ff_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\" got \"%.10s\"\nnear ...%40s"), "&&", "&" , c->cur );
	c->had_error = true;
return( op_and );
      case '?':
return( op_if );
      case '(': case ')': case ':': case ',':
return( ch );
      default:
	ff_post_error(_("Bad Token"), _("Bad token. got \"%1$c\"\nnear ...%2$40s"), ch , c->cur );
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

    switch ( (int)op ) { /* Cast avoids a warning that '(' in not in enum operator */
      case op_value: case op_x: case op_y:
	ret = calloc(1,sizeof(struct expr));
	ret->operator = op;
	ret->value = val;
return( ret );
      case '(':
	ret = getexpr(c);
	op = gettoken(c,&val);
	if ( op!=')' ) {
	    ff_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), ")" , c->cur );
	    c->had_error = true;
	}
return(ret );
      case op_log: case op_exp: case op_sqrt:
      case op_sin: case op_cos: case op_tan:
      case op_atan2:
      case op_abs:
      case op_rint: case op_floor: case op_ceil:
	ret = calloc(1,sizeof(struct expr));
	ret->operator = op;
	op = gettoken(c,&val);
	if ( op!='(' ) {
	    ff_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), "(" , c->cur );
	    c->had_error = true;
	}
	ret->op1 = getexpr(c);
	op = gettoken(c,&val);
	if ( ret->operator==op_atan2 ) {
	    if ( op!=',' ) {
		ff_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), "," , c->cur );
	    }
	    ret->op2 = getexpr(c);
	    op = gettoken(c,&val);
	}
	if ( op!=')' ) {
	    ff_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), ")" , c->cur );
	    c->had_error = true;
	}
return( ret );
      case op_add:
	/* Just ignore a unary plus */;
return( gete0(c));
      case op_sub: case op_not:
	ret = calloc(1,sizeof(struct expr));
	ret->operator = op;
	ret->op1 = gete0(c);
return( ret );
      default:
	ff_post_error(_("Bad Token"), _("Unexpected token.\nbefore ...%40s") , c->cur );
	c->had_error = true;
	ret = calloc(1,sizeof(struct expr));
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
	ret = calloc(1,sizeof(struct expr));
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
	ret = calloc(1,sizeof(struct expr));
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
	ret = calloc(1,sizeof(struct expr));
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
	ret = calloc(1,sizeof(struct expr));
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
	ret = calloc(1,sizeof(struct expr));
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
	ret = calloc(1,sizeof(struct expr));
	ret->op1 = op1;
	ret->operator = op;
	ret->op2 = getexpr(c);
	op = gettoken(c,&val);
	if ( op!=':' ) {
	    ff_post_error(_("Bad Token"), _("Bad token. Expected \"%.10s\"\nnear ...%40s"), ":" , c->cur );
	    c->had_error = true;
	}
	ret->op3 = getexpr(c);
return( ret );
    } else {
	backup(c,op,val);
return( op1 );
    }
}

struct expr *nlt_parseexpr(struct context *c,char *str) {
    struct expr *ret;

    c->backed_token = op_base;
    c->start = c->cur = str;
    ret = getexpr(c);
    if ( *c->cur!='\0' ) {
	c->had_error = true;
	ff_post_error(_("Bad Token"), _("Unexpected token after expression end.\nbefore ...%40s") , c->cur );
    }
    if ( c->had_error ) {
	nlt_exprfree(ret);
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
		ff_post_error(_("Bad Value"),_("Attempt to take logarithm of %1$g in %2$.30s"), val1, c->sc->name );
		c->had_error = true;
return( 0 );
	    }
return( log(val1));
	  case op_sqrt:
	    if ( val1<0 ) {
		ff_post_error(_("Bad Value"),_("Attempt to take the square root of %1$g in %2$.30s"), val1, c->sc->name );
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
	  default:
	  break;
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
	    ff_post_error(_("Bad Value"),_("Attempt to divide by 0 in %.30s"), c->sc->name );
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
    BasePoint old, off, delta;
    int fixup = true;

    old = sp->me;

    if ( c->pov_func!=NULL ) {
	(c->pov_func)(&sp->me,c->pov);
	if (( sp->next!=NULL && sp->next->order2 ) || (sp->prev!=NULL && sp->prev->order2 )) {
	    (c->pov_func)(&sp->prevcp,c->pov);
	    (c->pov_func)(&sp->nextcp,c->pov);
	    fixup = false;
	} else {
	    off.x = old.x+1; off.y = old.y+1;
	    (c->pov_func)(&off,c->pov);
	    delta.x = off.x - sp->me.x;
	    delta.y = off.y - sp->me.y;
	}
    } else {
	c->x = sp->me.x; c->y = sp->me.y;
	sp->me.x = NL_expr(c,c->x_expr);
	sp->me.y = NL_expr(c,c->y_expr);
	if (( sp->next!=NULL && sp->next->order2 ) || (sp->prev!=NULL && sp->prev->order2 )) {
	    c->x = sp->prevcp.x; c->y = sp->prevcp.y;
	    sp->prevcp.x = NL_expr(c,c->x_expr);
	    sp->prevcp.y = NL_expr(c,c->y_expr);
	    c->x = sp->nextcp.x; c->y = sp->nextcp.y;
	    sp->nextcp.x = NL_expr(c,c->x_expr);
	    sp->nextcp.y = NL_expr(c,c->y_expr);
	    fixup = false;
	} else {
	    /* The slope is important, the control points are a way of expressing */
	    /*  the slope. With a linear transform, transforming the cp would */
	    /*  give us the correct transformation of the slope. Not so here */
	    /*  Instead we want to figure out the transform around sp->me, and */
	    /*  apply that to the slope. Pretend it is linear */
	    ++c->x; ++c->y;
	    delta.x = NL_expr(c,c->x_expr) - sp->me.x;
	    delta.y = NL_expr(c,c->y_expr) - sp->me.y;
	}
    }
    if ( fixup ) {
	/* A one unit change in x is transformed into delta.x */
	sp->prevcp.x = (sp->prevcp.x-old.x)*delta.x + sp->me.x;
	sp->prevcp.y = (sp->prevcp.y-old.y)*delta.y + sp->me.y;
	sp->nextcp.x = (sp->nextcp.x-old.x)*delta.x + sp->me.x;
	sp->nextcp.y = (sp->nextcp.y-old.y)*delta.y + sp->me.y;
    }
}

static void SplineSetNLTrans(SplineSet *ss,struct context *c,
	int everything) {
    SplinePoint *first, *last, *next;
    SplinePoint *sp;
    TPoint mids[20];
    bigreal t;
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
	    if ( everything || next->selected )
		NLTransPoint(next,c);
	    next->next = next->prev = NULL;
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
		if ( sp->prev->order2 )	/* Can't be order2 */
		    ApproximateSplineFromPoints(last,next,mids,20,true);
		else
		    /* We transformed the slopes carefully, and I hope correctly */
		    /* This should give smoother joins that the above function */
		    ApproximateSplineFromPointsSlopes(last,next,mids,20,false);
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

static void _SCNLTrans(SplineChar *sc,struct context *c,int layer) {
    SplineSet *ss;
    RefChar *ref;
    int i, last, first;

    if ( sc->layer_cnt==ly_fore+1 &&
	    sc->layers[ly_fore].splines==NULL && sc->layers[ly_fore].refs==NULL )
return;

    c->sc = sc;
    if ( sc->parent->multilayer ) {
	first = ly_fore;
	last = sc->layer_cnt-1;
	SCPreserveState(sc,false);
    } else {
	first = last = layer;
	SCPreserveLayer(sc,layer,false);
    }
    for ( i=first; i<=last; ++i ) {
	for ( ss=sc->layers[i].splines; ss!=NULL; ss=ss->next )
	    SplineSetNLTrans(ss,c,true);
	for ( ref=sc->layers[i].refs; ref!=NULL; ref=ref->next ) {
	    c->x = ref->transform[4]; c->y = ref->transform[5];
	    ref->transform[4] = NL_expr(c,c->x_expr);
	    ref->transform[5] = NL_expr(c,c->y_expr);
	    /* we'll fix up the splines after all characters have been transformed*/
	}
    }
}

void _SFNLTrans(FontViewBase *fv,struct context *c) {
    SplineChar *sc;
    RefChar *ref;
    int i, gid;
    int layer = fv->active_layer;

    SFUntickAll(fv->sf);

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		(sc = fv->sf->glyphs[gid])!=NULL && !sc->ticked ) {
	    _SCNLTrans(sc,c,fv->active_layer);
	    sc->ticked = true;
	}
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		(sc=fv->sf->glyphs[gid])!=NULL &&
		(sc->layers[layer].splines!=NULL || sc->layers[layer].refs!=NULL)) {
	    /* A reference doesn't really work after a non-linear transform */
	    /*  but let's do the obvious thing */
	    for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
		SCReinstanciateRefChar(sc,ref,layer);
	    SCCharChangedUpdate(sc,fv->active_layer);
	}
}

int SFNLTrans(FontViewBase *fv,char *x_expr,char *y_expr) {
    struct context c;

    memset(&c,0,sizeof(c));
    if ( (c.x_expr = nlt_parseexpr(&c,x_expr))==NULL )
return( false );
    if ( (c.y_expr = nlt_parseexpr(&c,y_expr))==NULL ) {
	nlt_exprfree(c.x_expr);
return( false );
    }

    _SFNLTrans(fv,&c);

    nlt_exprfree(c.x_expr);
    nlt_exprfree(c.y_expr);
return( true );
}

int SSNLTrans(SplineSet *ss,char *x_expr,char *y_expr) {
    struct context c;

    memset(&c,0,sizeof(c));
    if ( (c.x_expr = nlt_parseexpr(&c,x_expr))==NULL )
return( false );
    if ( (c.y_expr = nlt_parseexpr(&c,y_expr))==NULL ) {
	nlt_exprfree(c.x_expr);
return( false );
    }

    while ( ss!=NULL ) {
	SplineSetNLTrans(ss,&c,false);
	ss = ss->next;
    }

    nlt_exprfree(c.x_expr);
    nlt_exprfree(c.y_expr);
return( true );
}

int SCNLTrans(SplineChar *sc, int layer,char *x_expr,char *y_expr) {
    struct context c;

    memset(&c,0,sizeof(c));
    if ( (c.x_expr = nlt_parseexpr(&c,x_expr))==NULL )
return( false );
    if ( (c.y_expr = nlt_parseexpr(&c,y_expr))==NULL ) {
	nlt_exprfree(c.x_expr);
return( false );
    }

    _SCNLTrans(sc,&c,layer);

    nlt_exprfree(c.x_expr);
    nlt_exprfree(c.y_expr);
return( true );
}

void CVNLTrans(CharViewBase *cv,struct context *c) {
    SplineSet *ss;
    RefChar *ref;
    int layer = CVLayer(cv);

    if ( cv->layerheads[cv->drawmode]->splines==NULL && (cv->drawmode!=dm_fore || cv->sc->layers[layer].refs==NULL ))
return;

    CVPreserveState(cv);
    c->sc = cv->sc;
    for ( ss=cv->layerheads[cv->drawmode]->splines; ss!=NULL; ss=ss->next )
	SplineSetNLTrans(ss,c,false);
    for ( ref=cv->layerheads[cv->drawmode]->refs; ref!=NULL; ref=ref->next ) {
	c->x = ref->transform[4]; c->y = ref->transform[5];
	ref->transform[4] = NL_expr(c,c->x_expr);
	ref->transform[5] = NL_expr(c,c->y_expr);
	SCReinstanciateRefChar(cv->sc,ref,layer);
    }
    CVCharChangedUpdate(cv);
}

static void BpPoV(BasePoint *me,void *_pov) {
    struct pov_data *pov = _pov;
    bigreal z, div;

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

void SPLPoV(SplineSet *base,struct pov_data *pov, int only_selected) {
    SplineSet *spl;
    real transform[6];
    bigreal si = sin( pov->direction ), co = cos( pov->direction );
    struct context c;

    if ( pov->z==0 )
return;

    transform[0] = transform[3] = co;
    transform[2] = -(transform[1] = si);
    transform[4] = -pov->x;
    transform[5] = -pov->y;
    SplinePointListTransform(base,transform,only_selected?tpt_OnlySelected:tpt_AllPoints);

    if ( pov->d==0 || pov->tilt==0 ) {
	transform[0] = transform[3] = pov->d/pov->z;
	transform[1] = transform[2] = transform[4] = transform[5] = 0;
	SplinePointListTransform(base,transform,only_selected?tpt_OnlySelected:tpt_AllPoints);
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
    SplinePointListTransform(base,transform,only_selected?tpt_OnlySelected:tpt_AllPoints);
}

static void SCFindCenter(SplineChar *sc,BasePoint *center) {
    DBounds db;
    SplineCharFindBounds(sc,&db);
    center->x = (db.minx+db.maxx)/2;
    center->y = (db.miny+db.maxy)/2;
}

void FVPointOfView(FontViewBase *fv,struct pov_data *pov) {
    int i, cnt=0, layer, last, first, gid;
    BasePoint origin;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( (gid=fv->map->map[i])!=-1 && fv->sf->glyphs[gid]!=NULL &&
		fv->selected[i] )
	++cnt;
    ff_progress_start_indicator(10,_("Projecting..."),_("Projecting..."),0,cnt,1);

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( (gid = fv->map->map[i])!=-1 && fv->selected[i] &&
		(sc = fv->sf->glyphs[gid])!=NULL && !sc->ticked ) {
	    sc->ticked = true;
	    SCPreserveLayer(sc,layer,false);

	    origin.x = origin.y = 0;
	    if ( pov->xorigin==or_center || pov->yorigin==or_center )
		SCFindCenter(sc,&origin);
	    if ( pov->xorigin!=or_value )
		pov->x = origin.x;
	    if ( pov->yorigin!=or_value )
		pov->y = origin.y;

	    MinimumDistancesFree(sc->md); sc->md = NULL;
	    if ( sc->parent->multilayer ) {
		first = ly_fore;
		last = sc->layer_cnt-1;
	    } else
		first = last = fv->active_layer;
	    for ( layer = first; layer<=last; ++layer )
		SPLPoV(sc->layers[layer].splines,pov,false);
	    SCCharChangedUpdate(sc,layer);
	}
    }
}

struct vanishing_point {
    bigreal x_vanish;
    bigreal y_vanish;
};

static void VanishingTrans(BasePoint *me,void *_vanish) {
    struct vanishing_point *vanish = _vanish;

    me->x = vanish->x_vanish + (vanish->y_vanish - me->y)/vanish->y_vanish*
		( me->x-vanish->x_vanish );
}

void CVYPerspective(CharViewBase *cv,bigreal x_vanish, bigreal y_vanish) {
    SplineSet *spl;
    struct context c;
    struct vanishing_point vanish;

    if ( y_vanish==0 )		/* Leave things unchanged */
return;

    vanish.x_vanish = x_vanish;
    vanish.y_vanish = y_vanish;
    memset(&c,0,sizeof(c));
    c.pov = &vanish; c.pov_func = VanishingTrans;
    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	SplineSetNLTrans(spl,&c,false);
    }
}
