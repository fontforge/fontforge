/* Copyright (C) 2003 by George Williams */
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
#include <math.h>

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
	fprintf( stderr, "Bad token \"%s\"\nnear ...%s\n", buffer, c->cur );
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
	fprintf( stderr, "Bad token. Expected == got =\nnear ...%s\n", c->cur );
	c->had_error = true;
return( op_eq );
      case '|':
	if ( *c->cur=='|' ) {
	    ++c->cur;
return( op_or );
	}
	fprintf( stderr, "Bad token. Expected || got |\nnear ...%s\n", c->cur );
	c->had_error = true;
return( op_or );
      case '&':
	if ( *c->cur=='&' ) {
	    ++c->cur;
return( op_and );
	}
	fprintf( stderr, "Bad token. Expected && got &\nnear ...%s\n", c->cur );
	c->had_error = true;
return( op_and );
      case '?':
return( op_if );
      case '(': case ')': case ':':
return( ch );
      default:
	fprintf( stderr, "Bad token. %c\nnear ...%s\n", ch, c->cur );
	c->had_error = true;
	*val = 0;
return( op_value );
    }
}

static void backup(struct context *c,enum operator op, real val ) {
    if ( c->backed_token!=op_base ) {
	fprintf( stderr, "Attempt to back up twice.\nnear ...%s\n", c->cur );
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
	    fprintf( stderr, "Expected ')'.\nnear ...%s\n", c->cur );
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
	    fprintf( stderr, "Expected '(' after function.\nnear ...%s\n", c->cur );
	    c->had_error = true;
	}
	ret->op1 = getexpr(c);
	op = gettoken(c,&val);
	if ( op!=')' ) {
	    fprintf( stderr, "Expected ')' after function call.\nnear ...%s\n", c->cur );
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
	fprintf( stderr, "Unexpected token\nnear ...%s\n", c->cur );
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
	    fprintf( stderr, "Expected ':' after ? operator.\nnear ...%s\n", c->cur );
	    c->had_error = true;
	}
	ret->op3 = getexpr(c);
return( ret );
    } else {
	backup(c,op,val);
return( op1 );
    }
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
		fprintf(stderr, "Attempt to take the logarithem of %g in %s\n", val1, c->sc->name );
		c->had_error = true;
return( 0 );
	    }
return( log(val1));
	  case op_sqrt:
	    if ( val1<0 ) {
		fprintf(stderr, "Attempt to take the square root of %g in %s\n", val1, c->sc->name );
		c->had_error = true;
return( 0 );
	    }
return( log(val1));
	  case op_exp:
return( exp(val1));
	  case op_sin:
return( sin(val1));
	  case op_cos:
return( cos(val1));
	  case op_tan:
return( tan(val1));
	  case op_abs:
return( abs(val1));
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
	    fprintf(stderr, "Division by 0 in %s\n", c->sc->name );
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
	fprintf( stderr, "Bad operator %d in %s\n", e->operator, c->sc->name );
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

static void SPSmoothJoint(SplinePoint *sp) {
    BasePoint unitn, unitp;
    double len, dot, dotn, dotp;
    if ( sp->prev==NULL || sp->next==NULL || sp->pointtype==pt_corner )
return;

    if ( sp->pointtype==pt_curve && !sp->nonextcp && !sp->noprevcp ) {
	unitn.x = sp->nextcp.x-sp->me.x;
	unitn.y = sp->nextcp.y-sp->me.y;
	len = sqrt(unitn.x*unitn.x + unitn.y*unitn.y);
	if ( len==0 )
return;
	unitn.x /= len; unitn.y /= len;
	unitp.x = sp->me.x - sp->prevcp.x;
	unitp.y = sp->me.y - sp->prevcp.y;
	len = sqrt(unitp.x*unitp.x + unitp.y*unitp.y);
	if ( len==0 )
return;
	unitp.x /= len; unitp.y /= len;
	dotn = unitp.y*(sp->nextcp.x-sp->me.x) - unitp.x*(sp->nextcp.y-sp->me.y);
	dotp = unitn.y*(sp->me.x - sp->prevcp.x) - unitn.x*(sp->me.y - sp->prevcp.y);
	sp->nextcp.x -= dotn*unitp.y/2;
	sp->nextcp.y -= -dotn*unitp.x/2;
	sp->prevcp.x += dotp*unitn.y/2;
	sp->prevcp.y += -dotp*unitn.x/2;
	SplineRefigure(sp->prev); SplineRefigure(sp->next);
    }
    if ( sp->pointtype==pt_tangent && !sp->nonextcp ) {
	unitp.x = sp->me.x - sp->prev->from->me.x;
	unitp.y = sp->me.y - sp->prev->from->me.y;
	len = sqrt(unitp.x*unitp.x + unitp.y*unitp.y);
	if ( len!=0 ) {
	    unitp.x /= len; unitp.y /= len;
	    dot = unitp.y*(sp->nextcp.x-sp->me.x) - unitp.x*(sp->nextcp.y-sp->me.y);
	    sp->nextcp.x -= dot*unitp.y;
	    sp->nextcp.y -= -dot*unitp.x;
	    SplineRefigure(sp->next);
	}
    }
    if ( sp->pointtype==pt_tangent && !sp->noprevcp ) {
	unitn.x = sp->nextcp.x-sp->me.x;
	unitn.y = sp->nextcp.y-sp->me.y;
	len = sqrt(unitn.x*unitn.x + unitn.y*unitn.y);
	if ( len!=0 ) {
	    unitn.x /= len; unitn.y /= len;
	    dot = unitn.y*(sp->me.x-sp->prevcp.x) - unitn.x*(sp->me.y-sp->prevcp.y);
	    sp->prevcp.x += dot*unitn.y;
	    sp->prevcp.y += -dot*unitn.x;
	    SplineRefigure(sp->prev);
	}
    }
}
    
static void SplineSetNLTrans(SplineSet *ss,struct context *c) {
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
    first->next = first->prev = NULL;
    c->x = first->me.x; c->y = first->me.y;
    first->me.x = NL_expr(c,c->x_expr);
    first->me.y = NL_expr(c,c->y_expr);

    if ( ss->first->next!=NULL ) {
	for ( sp=ss->first->next->to; sp!=NULL; ) {
	    next = chunkalloc(sizeof(SplinePoint));
	    *next = *sp;
	    next->next = next->prev = NULL;
	    c->x = next->me.x; c->y = next->me.y;
	    next->me.x = NL_expr(c,c->x_expr);
	    next->me.y = NL_expr(c,c->y_expr);
	    xsp = &sp->prev->splines[0]; ysp = &sp->prev->splines[1];
	    for ( i=0; i<20; ++i ) {
		t = (i+1)/21.0;
		c->x = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
		c->y = ((ysp->a*t+ysp->b)*t+ysp->c)*t + ysp->d;
		mids[i].t = t;
		mids[i].x = NL_expr(c,c->x_expr);
		mids[i].y = NL_expr(c,c->y_expr);
	    }
	    ApproximateSplineFromPoints(last,next,mids,20);
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

    if ( sc->splines==NULL && sc->refs==NULL )
return;

    SCPreserveState(sc,false);
    c->sc = sc;
    for ( ss=sc->splines; ss!=NULL; ss=ss->next )
	SplineSetNLTrans(ss,c);
    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) {
	c->x = ref->transform[4]; c->y = ref->transform[5];
	ref->transform[4] = NL_expr(c,c->x_expr);
	ref->transform[5] = NL_expr(c,c->y_expr);
	/* we'll fix up the splines after all characters have been transformed*/
    }
}

int SFNLTrans(FontView *fv,char *x_expr,char *y_expr) {
    struct context c;
    SplineChar *sc;
    RefChar *ref;
    int i;

    memset(&c,0,sizeof(c));
    c.backed_token = op_base;
    c.start = c.cur = x_expr;
    c.x_expr = getexpr(&c);
    if ( *c.cur!='\0' ) {
	c.had_error = true;
	fprintf( stderr, "Unexpected token after expression end\nnear ...%s\n", c.cur );
    }
    if ( c.had_error ) {
	exprfree(c.x_expr);
return( false );
    }

    c.backed_token = op_base;
    c.start = c.cur = y_expr;
    c.y_expr = getexpr(&c);
    if ( *c.cur!='\0' ) {
	c.had_error = true;
	fprintf( stderr, "Unexpected token after expression end\nnear ...%s\n", c.cur );
    }
    if ( c.had_error ) {
	exprfree(c.x_expr);
	exprfree(c.y_expr);
return( false );
    }

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] && fv->sf->chars[i]!=NULL )
	SCNLTrans(fv->sf->chars[i],&c);
    for ( i=0; i<fv->sf->charcnt; ++i )
	if ( fv->selected[i] && (sc=fv->sf->chars[i])!=NULL &&
		(sc->splines!=NULL || sc->refs!=NULL)) {
	    /* A reference doesn't really work after a non-linear transform */
	    /*  but let's do the obvious thing */
	    for ( ref = sc->refs; ref!=NULL; ref=ref->next )
		SCReinstanciateRefChar(sc,ref);
	    SCCharChangedUpdate(sc);
	}

    exprfree(c.x_expr);
    exprfree(c.y_expr);
return( true );
}
