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
    { { N_("New O_utline Window"), "", "" }, {}, LegacyAction, MID_OpenOutline },
    kMenuSeparator,
    { { N_("Cu_t"), "editcut", "" }, {}, LegacyAction, MID_Cut },
    { { N_("_Copy"), "editcopy", "" }, {}, LegacyAction, MID_Copy },
    { { N_("C_opy Reference"), "editcopyref", "" }, {}, LegacyAction, MID_CopyRef },
    { { N_("Copy _Width"), "editcopywidth", "" }, {}, LegacyAction, MID_CopyWidth },
    { { N_("_Paste"), "editpaste", "" }, {}, LegacyAction, MID_Paste },
    { { N_("C_lear"), "editclear", "" }, {}, LegacyAction, MID_Clear },
    { { N_("Copy _Fg To Bg"), "editcopyfg2bg", "" }, {}, LegacyAction, MID_CopyFgToBg },
    { { N_("U_nlink Reference"), "editunlink", "" }, {}, LegacyAction, MID_UnlinkRef },
    kMenuSeparator,
    { { N_("Glyph _Info..."), "elementglyphinfo", "" }, {}, LegacyAction, MID_CharInfo },
    { { N_("_Transform..."), "elementtransform", "" }, {}, LegacyAction, MID_Transform },
    { { N_("_Expand Stroke..."), "elementexpandstroke", "" }, {}, LegacyAction, MID_Stroke },
    { { N_("To _Int"), "elementround", "" }, {}, LegacyAction, MID_Round },
    { { N_("_Correct Direction"), "elementcorrectdir", "" }, {}, LegacyAction, MID_Correct },
    kMenuSeparator,
    { { N_("Auto_Hint"), "hintsautohint", "" }, {}, LegacyAction, MID_AutoHint },
    kMenuSeparator,
    { { N_("_Center in Width"), "metricscenter", "" }, {}, LegacyAction, MID_Center },
    { { N_("Set _Width..."), "metricssetwidth", "" }, {}, LegacyAction, MID_SetWidth },
    { { N_("Set _Vertical Advance..."), "metricssetvwidth", "" }, {}, LegacyAction, MID_SetVWidth },
};

std::vector<MenuInfo> top_menu = {
    { { N_("_File") }, {}, NoAction, -1 },
    { { N_("_Edit") }, {}, NoAction, -1 },
    { { N_("E_lement") }, {}, NoAction, -1 },
#ifndef _NO_PYTHON
    { { N_("_Tools") }, {}, NoAction, -1 },
#endif
    { { N_("H_ints") }, {}, NoAction, -1 },
    { { N_("E_ncoding") }, {}, NoAction, -1 },
    { { N_("_View") }, {}, NoAction, -1 },
    { { N_("_Metrics") }, {}, NoAction, -1 },
    { { N_("_CID") }, {}, NoAction, -1 },
/* GT: Here (and following) MM means "MultiMaster" */
    { { N_("MM") }, {}, NoAction, -1 },
    { { N_("_Window") }, {}, NoAction, -1 },
    { { N_("_Help") }, {}, NoAction, -1 },
};
// clang-format on

}  // namespace ff::views
