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

#include "menu_builder.hpp"
#include "menu_ids.h"

namespace ff::views {

// clang-format off
std::vector<MenuInfo> popup_menu = {
    { { N_("New O_utline Window"), "", "" }, {}, LegacyCallbacks, MID_OpenOutline },
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
    { { N_("_HStem"), "", "" }, {}, LegacyCallbacks, MID_HStemHist },
    { { N_("_VStem"), "", "" }, {}, LegacyCallbacks, MID_VStemHist },
    { { N_("BlueValues"), "", "" }, {}, LegacyCallbacks, MID_BlueValuesHist },
};

std::vector<MenuInfo> hints_menu = {
    { { N_("Auto_Hint"), "hintsautohint", "<control><shift>H" }, {}, LegacyCallbacks, MID_AutoHint },
    { { N_("Hint _Substitution Pts"), "", "" }, {}, LegacyCallbacks, MID_HintSubsPt },
    { { N_("Auto _Counter Hint"), "", "" }, {}, LegacyCallbacks, MID_AutoCounter },
    { { N_("_Don't AutoHint"), "hintsdontautohint", "" }, {}, LegacyCallbacks, MID_DontAutoHint },
    kMenuSeparator,
    { { N_("Auto_Instr"), "", "<control>T" }, {}, LegacyCallbacks, MID_AutoInstr },
    { { N_("_Edit Instructions..."), "", "" }, {}, LegacyCallbacks, MID_EditInstructions },
    { { N_("Edit 'fpgm'..."), "", "" }, {}, LegacyCallbacks, MID_Editfpgm },
    { { N_("Edit 'prep'..."), "", "" }, {}, LegacyCallbacks, MID_Editprep },
    { { N_("Edit 'maxp'..."), "", "" }, {}, LegacyCallbacks, MID_Editmaxp },
    { { N_("Edit 'cvt '..."), "", "" }, {}, LegacyCallbacks, MID_Editcvt },
    { { N_("Remove Instr Tables"), "", "" }, {}, LegacyCallbacks, MID_RmInstrTables },
    { { N_("S_uggest Deltas..."), "", "" }, {}, LegacyCallbacks, MID_Deltas },
    kMenuSeparator,
    { { N_("_Clear Hints"), "hintsclearvstems", "" }, {}, LegacyCallbacks, MID_ClearHints },
    { { N_("Clear Instructions"), "", "" }, {}, LegacyCallbacks, MID_ClearInstrs },
    kMenuSeparator,
    { { N_("Histograms"), "", "" }, histograms_menu, SubMenuCallbacks, 0 },
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
    { { N_("_View") }, {}, SubMenuCallbacks, -1 },
    { { N_("_Metrics") }, {}, SubMenuCallbacks, -1 },
    { { N_("_CID") }, {}, SubMenuCallbacks, -1 },
/* GT: Here (and following) MM means "MultiMaster" */
    { { N_("MM") }, {}, SubMenuCallbacks, -1 },
    { { N_("_Window") }, {}, SubMenuCallbacks, -1 },
    { { N_("_Help") }, {}, SubMenuCallbacks, -1 },
};
// clang-format on

}  // namespace ff::views
