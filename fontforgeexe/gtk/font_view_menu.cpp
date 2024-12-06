/* Copyright 2024 Maxim Iorsh <iorsh@users.sourceforge.net>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

The name of the author may not be used to endorse or promote
products derived from this software without specific prior written
permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <glib/gi18n.h>
#include <gtkmm.h>

#include "font_view.hpp"
#include "menu_builder.hpp"
#include "menu_ids.h"

namespace ff::views {

std::vector<MenuInfo> view_menu_bitmaps(const UiContext& ui_context) {
    const FontViewUiContext& fv_ui_context =
        static_cast<const FontViewUiContext&>(ui_context);
    auto fv_context = fv_ui_context.legacy();

    std::vector<BitmapMenuData> bitmap_data_array =
        VectorWrapper(fv_context->fv, fv_context->collect_bitmap_data);

    std::vector<MenuInfo> info_arr;

    for (const BitmapMenuData& bitmap_data : bitmap_data_array) {
        char buffer[50];

        if (bitmap_data.depth == 1)
            sprintf(buffer, _("%d pixel bitmap"), bitmap_data.pixelsize);
        else
            sprintf(buffer, _("%d@%d pixel bitmap"), bitmap_data.pixelsize,
                    bitmap_data.depth);

        ActivateCB action =
            [cb = fv_context->change_display_bitmap, fv = fv_context->fv,
             bdf = bitmap_data.bdf](const UiContext&) { cb(fv, bdf); };
        CheckedCB checker =
            [cb = fv_context->current_display_bitmap, fv = fv_context->fv,
             bdf = bitmap_data.bdf](const UiContext&) { return cb(fv, bdf); };
        MenuInfo info{{buffer, CellPixelView, ""},
                      {},
                      {action, AlwaysEnabled, checker},
                      0};
        info_arr.push_back(info);
    }

    return info_arr;
}

std::vector<MenuInfo> view_menu_layers(const UiContext& ui_context) {
    const FontViewUiContext& fv_ui_context =
        static_cast<const FontViewUiContext&>(ui_context);
    auto fv_context = fv_ui_context.legacy();

    std::vector<LayerMenuData> layer_data_array =
        VectorWrapper(fv_context->fv, fv_context->collect_layer_data);
    std::vector<MenuInfo> info_arr;

    for (const LayerMenuData& layer_data : layer_data_array) {
        ActivateCB action = [fv_context,
                             ly = layer_data.index](const UiContext&) {
            fv_context->change_display_layer(fv_context->fv, ly);
        };
        CheckedCB checker = [fv_context,
                             ly = layer_data.index](const UiContext&) {
            return fv_context->current_display_layer(fv_context->fv, ly);
        };
        MenuInfo info{{layer_data.label, ActiveLayer, ""},
                      {},
                      {action, AlwaysEnabled, checker},
                      0};
        info_arr.push_back(info);
    }
    return info_arr;
}

// clang-format off
std::vector<MenuInfo> popup_menu = {
    { { N_("New O_utline Window"), NoDecoration, "" }, {}, LegacyCallbacks, MID_OpenOutline },
    kMenuSeparator,
    { { N_("Cu_t"), "editcut", "" }, {}, LegacyCallbacks, MID_Cut },
    { { N_("_Copy"), "editcopy", "" }, {}, LegacyCallbacks, MID_Copy },
    { { N_("C_opy Reference"), "editcopyref", "" }, {}, LegacyCallbacks, MID_CopyRef },
    { { N_("Copy _Width"), "editcopywidth", "" }, {}, LegacyCallbacks, MID_CopyWidth },
    { { N_("_Paste"), "editpaste", "" }, {}, LegacyCallbacks, MID_Paste },
    { { N_("C_lear"), "editclear", "" }, {}, LegacyCallbacks, MID_Clear },
    { { N_("Copy _Fg To Bg"), "editcopyfg2bg", "" }, {}, LegacyCallbacks, MID_CopyFgToBg },
    { { N_("U_nlink Reference"), "editunlink", "" }, {}, LegacyCallbacks, MID_UnlinkRef },
    kMenuSeparator,
    { { N_("Glyph _Info..."), "elementglyphinfo", "" }, {}, LegacyCallbacks, MID_CharInfo },
    { { N_("_Transform..."), "elementtransform", "" }, {}, LegacyCallbacks, MID_Transform },
    { { N_("_Expand Stroke..."), "elementexpandstroke", "" }, {}, LegacyCallbacks, MID_Stroke },
    { { N_("To _Int"), "elementround", "" }, {}, LegacyCallbacks, MID_Round },
    { { N_("_Correct Direction"), "elementcorrectdir", "" }, {}, LegacyCallbacks, MID_Correct },
    kMenuSeparator,
    { { N_("Auto_Hint"), "hintsautohint", "" }, {}, LegacyCallbacks, MID_AutoHint },
    kMenuSeparator,
    { { N_("_Center in Width"), "metricscenter", "" }, {}, LegacyCallbacks, MID_Center },
    { { N_("Set _Width..."), "metricssetwidth", "" }, {}, LegacyCallbacks, MID_SetWidth },
    { { N_("Set _Vertical Advance..."), "metricssetvwidth", "" }, {}, LegacyCallbacks, MID_SetVWidth },
};

std::vector<MenuInfo> histograms_menu = {
    { { N_("_HStem"), NoDecoration, "" }, {}, LegacyCallbacks, MID_HStemHist },
    { { N_("_VStem"), NoDecoration, "" }, {}, LegacyCallbacks, MID_VStemHist },
    { { N_("BlueValues"), NoDecoration, "" }, {}, LegacyCallbacks, MID_BlueValuesHist },
};

std::vector<MenuInfo> hints_menu = {
    { { N_("Auto_Hint"), "hintsautohint", "<control><shift>H" }, {}, LegacyCallbacks, MID_AutoHint },
    { { N_("Hint _Substitution Pts"), NoDecoration, "" }, {}, LegacyCallbacks, MID_HintSubsPt },
    { { N_("Auto _Counter Hint"), NoDecoration, "" }, {}, LegacyCallbacks, MID_AutoCounter },
    { { N_("_Don't AutoHint"), "hintsdontautohint", "" }, {}, LegacyCallbacks, MID_DontAutoHint },
    kMenuSeparator,
    { { N_("Auto_Instr"), NoDecoration, "<control>T" }, {}, LegacyCallbacks, MID_AutoInstr },
    { { N_("_Edit Instructions..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_EditInstructions },
    { { N_("Edit 'fpgm'..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_Editfpgm },
    { { N_("Edit 'prep'..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_Editprep },
    { { N_("Edit 'maxp'..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_Editmaxp },
    { { N_("Edit 'cvt '..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_Editcvt },
    { { N_("Remove Instr Tables"), NoDecoration, "" }, {}, LegacyCallbacks, MID_RmInstrTables },
    { { N_("S_uggest Deltas..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_Deltas },
    kMenuSeparator,
    { { N_("_Clear Hints"), "hintsclearvstems", "" }, {}, LegacyCallbacks, MID_ClearHints },
    { { N_("Clear Instructions"), NoDecoration, "" }, {}, LegacyCallbacks, MID_ClearInstrs },
    kMenuSeparator,
    { { N_("Histograms"), NoDecoration, "" }, histograms_menu, SubMenuCallbacks, 0 },
};

std::vector<MenuInfo> layers_menu = {
    MenuInfo::CustomBlock(view_menu_layers),
};

std::vector<MenuInfo> view_menu = {
    { { N_("_Next Glyph"), "viewnext", "<control>bracketright" }, {}, LegacyCallbacks, MID_Next },
    { { N_("_Prev Glyph"), "viewprev", "<control>bracketleft" }, {}, LegacyCallbacks, MID_Prev },
    { { N_("Next _Defined Glyph"), "viewnextdef", "<alt><control>bracketright" }, {}, LegacyCallbacks, MID_NextDef },
    { { N_("Prev Defined Gl_yph"), "viewprevdef", "<alt><control>bracketleft" }, {}, LegacyCallbacks, MID_PrevDef },
    { { N_("_Goto"), "viewgoto", "<control><shift>greater" }, {}, LegacyCallbacks, MID_GotoChar },
    kMenuSeparator,
    { { N_("_Layers"), "viewlayers", "" }, layers_menu, SubMenuCallbacks, 0 },
    kMenuSeparator,
    { { N_("_Show ATT"), NoDecoration, "" }, {}, LegacyCallbacks, MID_Show_ATT },
    { { N_("Display S_ubstitutions..."), Checkable, "" }, {}, LegacyCallbacks, MID_DisplaySubs },
    { { N_("Com_binations"), NoDecoration, "" }, {}, SubMenuCallbacks, 0 },
    kMenuSeparator,
    { { N_("Label Gl_yph By"), NoDecoration, "" }, {}, SubMenuCallbacks, 0 },
    kMenuSeparator,
    { { N_("S_how H. Metrics..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_ShowHMetrics },
    { { N_("Show _V. Metrics..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_ShowVMetrics },
    kMenuSeparator,
    { { N_("32x8 cell window"), CellWindowSize, "<control><shift>percent" }, {}, LegacyCallbacks, MID_32x8 },
    { { N_("_16x4 cell window"), CellWindowSize, "<control><shift>asciicircum" }, {}, LegacyCallbacks, MID_16x4 },
    { { N_("_8x2  cell window"), CellWindowSize, "<control><shift>asterisk" }, {}, LegacyCallbacks, MID_8x2 },
    kMenuSeparator,
    { { N_("_24 pixel outline"), CellPixelView, "<control>2" }, {}, LegacyCallbacks, MID_24 },
    { { N_("_36 pixel outline"), CellPixelView, "<control>3" }, {}, LegacyCallbacks, MID_36 },
    { { N_("_48 pixel outline"), CellPixelView, "<control>4" }, {}, LegacyCallbacks, MID_48 },
    { { N_("_72 pixel outline"), CellPixelView, "<control>7" }, {}, LegacyCallbacks, MID_72 },
    { { N_("_96 pixel outline"), CellPixelView, "<control>9" }, {}, LegacyCallbacks, MID_96 },
    { { N_("_128 pixel outline"), CellPixelView, "<control>1" }, {}, LegacyCallbacks, MID_128 },
    { { N_("_Anti Alias"), Checkable, "<control>5" }, {}, LegacyCallbacks, MID_AntiAlias },
    { { N_("_Fit to font bounding box"), Checkable, "<control>6" }, {}, LegacyCallbacks, MID_FitToBbox },
    kMenuSeparator,
    { { N_("Bitmap _Magnification..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_BitmapMag },
    MenuInfo::CustomBlock(view_menu_bitmaps),
};

std::vector<MenuInfo> top_menu = {
    { { N_("_File") }, {}, SubMenuCallbacks, -1 },
    { { N_("_Edit") }, {}, SubMenuCallbacks, -1 },
    { { N_("E_lement") }, {}, SubMenuCallbacks, -1 },
#ifndef _NO_PYTHON
    { { N_("_Tools") }, {}, SubMenuCallbacks, -1 },
#endif
    { { N_("H_ints") }, hints_menu, SubMenuCallbacks, -1 },
    { { N_("E_ncoding") }, {}, SubMenuCallbacks, -1 },
    { { N_("_View") }, view_menu, SubMenuCallbacks, -1 },
    { { N_("_Metrics") }, {}, SubMenuCallbacks, -1 },
    { { N_("_CID") }, {}, SubMenuCallbacks, -1 },
/* GT: Here (and following) MM means "MultiMaster" */
    { { N_("MM") }, {}, SubMenuCallbacks, -1 },
    { { N_("_Window") }, {}, SubMenuCallbacks, -1 },
    { { N_("_Help") }, {}, SubMenuCallbacks, -1 },
};
// clang-format on

}  // namespace ff::views
