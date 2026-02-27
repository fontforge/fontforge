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
#include "splinefill.h"
#include "splinefont.h"
#include "bitmapcontrol.h"
#include "splinefont_enums.h"
}
#include "gtk/application.hpp"
#include "gtk/bitmaps_dlg.hpp"
#include "gtk/simple_dialogs.hpp"

extern "C" void BitmapDlg(FontViewBase* fv, GWindow gw, SplineChar* sc,
                          int isavail) {
    CreateBitmapData bd;
    bool rasterize = true;
    std::string scope;
    ff::dlg::bitmaps_dlg_mode dlg_mode = (ff::dlg::bitmaps_dlg_mode)isavail;

    bd.fv = fv;
    bd.sc = sc;
    bd.layer = fv != NULL ? fv->active_layer : ly_fore;
    bd.sf = fv->cidmaster ? fv->cidmaster : fv->sf;
    bd.isavail = isavail;
    bd.done = false;

    ff::dlg::BitmapSizes sizes;
    for (BDFFont* bdf = bd.sf->bitmaps; bdf != NULL; bdf = bdf->next) {
        sizes.emplace_back(bdf->pixelsize, BDFDepth(bdf));
    }

    // To avoid instability, the GTK application is lazily initialized only when
    // a GTK window is invoked.
    ff::app::GtkApp();

    bool is_bitmap = bd.sf->onlybitmaps && bd.sf->bitmaps != NULL;
    ff::dlg::BitmapsDlg dialog(gw, dlg_mode, sizes, is_bitmap, sc != NULL);
    bool is_ok = dialog.show();
    if (!is_ok) return;

    ff::dlg::BitmapSizes new_sizes = dialog.get_sizes();
    std::vector<int32_t> c_sizes;
    for (const ff::dlg::BitmapSize& sz : new_sizes) {
        c_sizes.push_back(sz.first | (sz.second << 16));
    }
    c_sizes.push_back(0); /* C data is zero-terminated */
    rasterize = dialog.get_rasterize();
    scope = dialog.get_active_scope();

    if (dlg_mode != ff::dlg::bitmaps_dlg_avail)
        bd.which = (scope == "all")         ? bd_all
                   : (scope == "selection") ? bd_selected
                                            : bd_current;
    else
        bd.rasterize = rasterize;

    BitmapsDoIt(&bd, c_sizes.data());
}
