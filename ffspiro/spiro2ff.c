/*
ppedit - A pattern plate editor for Spiro splines.
Copyright (C) 2007 Raph Levien

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

*/
/* This file written by George Williams to provide a gateway to fontforge */


#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "bezctx_intf.h"
#include "bezctx_ff.h"
#include "spiro.h"

struct splinepointlist *Spiro2SplineSet(spiro_cp *src,int n) {
    spiro_seg *s = run_spiro(src,n);
    bezctx *bc = new_bezctx_ff();

    spiro_to_bpath(s,n,bc);
    free_spiro(s);
return( bezctx_ff_close(bc));
}
