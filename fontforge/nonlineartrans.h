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

#ifndef FONTFORGE_NONLINEARTRANS_H
#define FONTFORGE_NONLINEARTRANS_H

#include "baseviews.h"
#include "splinefont.h"

#include "ustring.h"
#include "utype.h"

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

struct expr_context {
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

extern void _SFNLTrans(FontViewBase *fv, struct expr_context *c);
extern struct expr *nlt_parseexpr(struct expr_context *c, char *str);
extern void nlt_exprfree(struct expr *e);
extern void CVNLTrans(CharViewBase *cv,struct expr_context *c);
extern void SPLPoV(SplineSet *spl,struct pov_data *pov, int only_selected);

extern int SCNLTrans(SplineChar *sc, int layer, char *x_expr, char *y_expr);
extern int SSNLTrans(SplineSet *ss, char *x_expr, char *y_expr);
extern void CVYPerspective(CharViewBase *cv, bigreal x_vanish, bigreal y_vanish);
extern void FVPointOfView(FontViewBase *fv, struct pov_data *pov);

#endif /* FONTFORGE_NONLINEARTRANS_H */
