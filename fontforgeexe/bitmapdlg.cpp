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

extern "C" {
#include "bdffont.h"
#include "bitmapcontrol.h"
#include "splinefont_enums.h"
}
#include "gtk/simple_dialogs.hpp"

void BitmapDlg(FontViewBase *fv, GWindow gw, SplineChar *sc, int isavail) {
    CreateBitmapData bd;
    int i;
    int32_t *sizes;
    BDFFont *bdf;
    bool rasterize = true;
    char* scope = NULL;
    enum bitmaps_dlg_mode dlg_mode =
        (isavail == 1) ? bitmaps_dlg_avail
                       : (isavail == -1) ? bitmaps_dlg_remove
                                         : bitmaps_dlg_regen;

    bd.fv = fv;
    bd.sc = sc;
    bd.layer = fv!=NULL ? fv->active_layer : ly_fore;
    bd.sf = fv->cidmaster ? fv->cidmaster : fv->sf;
    bd.isavail = isavail;
    bd.done = false;

    for ( bdf=SFGetBdfFont(bd.sf), i=0; bdf!=NULL; bdf=bdf->next, ++i );

    sizes = (int32_t*)malloc((i+1)*sizeof(int32_t));
    for ( bdf=SFGetBdfFont(bd.sf), i=0; bdf!=NULL; bdf=bdf->next, ++i )
	sizes[i] = bdf->pixelsize | (BDFDepth(bdf)<<16);
    sizes[i] = 0;

    bool is_ok = bitmap_strikes_dialog(
        gw, dlg_mode, &sizes,
        SFIsBitmap(bd.sf), sc != NULL,
        &rasterize, &scope);

    if (!is_ok) return;

    if (bd.isavail < true)
        bd.which = (strcmp(scope, "all") == 0)         ? bd_all
                      : (strcmp(scope, "selection") == 0) ? bd_selected
                                                          : bd_current;
    if (bd.isavail == 1) bd.rasterize = rasterize;

    BitmapsDoIt(&bd, sizes, true);
    free(sizes);
    free(scope);
    return;
}
