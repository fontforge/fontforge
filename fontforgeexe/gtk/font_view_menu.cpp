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

#include "common_menus.hpp"
#include "font_view.hpp"
#include "menu_builder.hpp"
#include "menu_ids.h"

namespace ff::views {

std::vector<MenuInfo> encodings(std::shared_ptr<FVContext> fv_context,
                                void (*encoding_action)(::FontView*,
                                                        const char*),
                                RadioGroup group) {
    std::vector<EncodingMenuData> encoding_data_vec =
        VectorWrapper(fv_context->fv, fv_context->collect_encoding_data);
    std::vector<MenuInfo> info_arr;

    for (const EncodingMenuData& encoding_data : encoding_data_vec) {
        if (encoding_data.enc_name == nullptr) {
            info_arr.push_back(kMenuSeparator);
            continue;
        }

        ActivateCB action = [encoding_action, fv = fv_context->fv,
                             enc_name =
                                 encoding_data.enc_name](const UiContext&) {
            encoding_action(fv, enc_name);
        };
        CheckedCB checker = [cb = fv_context->current_encoding,
                             fv = fv_context->fv,
                             enc_name = encoding_data.enc_name](
                                const UiContext&) { return cb(fv, enc_name); };
        MenuInfo info{{encoding_data.label, group, ""},
                      {},
                      {action, AlwaysEnabled, checker},
                      0};
        info_arr.push_back(info);
    }
    return info_arr;
}

std::vector<MenuInfo> encoding_reencode(const UiContext& ui_context) {
    const FontViewUiContext& fv_ui_context =
        static_cast<const FontViewUiContext&>(ui_context);
    auto fv_context = fv_ui_context.legacy();

    return encodings(fv_context, fv_context->change_encoding, Encoding);
}

std::vector<MenuInfo> encoding_force_encoding(const UiContext& ui_context) {
    const FontViewUiContext& fv_ui_context =
        static_cast<const FontViewUiContext&>(ui_context);
    auto fv_context = fv_ui_context.legacy();

    return encodings(fv_context, fv_context->force_encoding, ForcedEncoding);
}

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

std::vector<MenuInfo> view_menu_anchors(const UiContext& ui_context) {
    const FontViewUiContext& fv_ui_context =
        static_cast<const FontViewUiContext&>(ui_context);
    auto fv_context = fv_ui_context.legacy();

    std::vector<AnchorMenuData> anchor_data_array =
        VectorWrapper(fv_context->fv, fv_context->collect_anchor_data);

    std::vector<MenuInfo> info_arr;

    // Special item for all anchors
    ActivateCB action_all = [cb = fv_context->show_anchor_pair,
                             fv = fv_context->fv](const UiContext&) {
        cb(fv, (AnchorClass*)-1);
    };
    info_arr.push_back({{N_("All"), NoDecoration, ""},
                        {},
                        {action_all, AlwaysEnabled, NotCheckable},
                        0});
    info_arr.push_back(kMenuSeparator);

    for (const AnchorMenuData& anchor_data : anchor_data_array) {
        ActivateCB action =
            [cb = fv_context->show_anchor_pair, fv = fv_context->fv,
             ac = anchor_data.ac](const UiContext&) { cb(fv, ac); };
        MenuInfo info{{anchor_data.label, NoDecoration, ""},
                      {},
                      {action, AlwaysEnabled, NotCheckable},
                      0};
        info_arr.push_back(info);
    }
    return info_arr;
}

void run_autotrace(const UiContext& ui_context) {
    const FontViewUiContext& fv_ui_context =
        static_cast<const FontViewUiContext&>(ui_context);
    auto fv_context = fv_ui_context.legacy();
    Gtk::Widget* drawing_area = gtk_find_child(&ui_context.window_, "CharGrid");

    auto old_cursor = set_cursor(&fv_ui_context.window_, "wait");
    auto old_cursor_da = set_cursor(drawing_area, "wait");

    bool shift_pressed =
        gtk_get_keyboard_state() & Gdk::ModifierType::SHIFT_MASK;
    fv_context->run_autotrace(fv_context->fv, shift_pressed);

    unset_cursor(&fv_ui_context.window_, old_cursor);
    unset_cursor(drawing_area, old_cursor_da);
}

static const Color COLOR_DEFAULT = 0xfffffffe;
static const Color COLOR_CHOOSE = (Color)-10;

template <Color C>
void set_color(const UiContext& ui_context) {
    const FontViewUiContext& fv_ui_context =
        static_cast<const FontViewUiContext&>(ui_context);
    auto fv_context = fv_ui_context.legacy();

    fv_context->set_color(fv_context->fv, C);
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

//////////////////////////////// ELEMENT MENUS ////////////////////////////////////////

std::vector<MenuInfo> show_dependent_menu = {
    { { N_("_References..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_ShowDependentRefs },
    { { N_("_Substitutions..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_ShowDependentSubs },
};

std::vector<MenuInfo> set_color_menu = {
    { { N_("Color|Choose..."), "colorwheel", "" }, {}, { set_color<COLOR_CHOOSE> }, 0 },
    { { N_("Color|Default"), Gdk::RGBA("00000000"), "" }, {}, { set_color<COLOR_DEFAULT> }, 0 },
    { { "White", Gdk::RGBA("white"), "" }, {}, { set_color<0xffffff> }, 0 },
    { { "Red", Gdk::RGBA("red"), "" }, {}, { set_color<0xff0000> }, 0 },
    { { "Green", Gdk::RGBA("green"), "" }, {}, { set_color<0x00ff00> }, 0 },
    { { "Blue", Gdk::RGBA("blue"), "" }, {}, { set_color<0x0000ff> }, 0 },
    { { "Yellow", Gdk::RGBA("yellow"), "" }, {}, { set_color<0xffff00> }, 0 },
    { { "Cyan", Gdk::RGBA("cyan"), "" }, {}, { set_color<0x00ffff> }, 0 },
    { { "Magenta", Gdk::RGBA("magenta"), "" }, {}, { set_color<0xff00ff> }, 0 },
};

std::vector<MenuInfo> other_info_menu = {
    { { N_("_MATH Info..."), "elementmathinfo", "" }, {}, LegacyCallbacks, MID_MathInfo },
    { { N_("_BDF Info..."), "elementbdfinfo", "" }, {}, LegacyCallbacks, MID_StrikeInfo },
    { { N_("_Horizontal Baselines..."), "elementhbaselines", "" }, {}, LegacyCallbacks, MID_HorBaselines },
    { { N_("_Vertical Baselines..."), "elementvbaselines", "" }, {}, LegacyCallbacks, MID_VertBaselines },
    { { N_("_Justification..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_Justification },
    { { N_("Show _Dependent"), "elementshowdep", "" }, show_dependent_menu, SubMenuCallbacks, 0 },
    { { N_("Mass Glyph _Rename..."), "elementrenameglyph", "" }, {}, LegacyCallbacks, MID_MassRename },
    { { N_("Set _Color"), NoDecoration, "" }, set_color_menu, LegacySubMenuCallbacks, MID_SetColor },
};

std::vector<MenuInfo> validation_menu = {
    { { N_("Find Pr_oblems..."), "elementfindprobs", "<control>E" }, {}, LegacyCallbacks, MID_FindProblems },
    { { N_("_Validate..."), "elementvalidate", "" }, {}, LegacyCallbacks, MID_Validate },
    kMenuSeparator,
    { { N_("Set E_xtremum Bound..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_SetExtremumBound },
};

std::vector<MenuInfo> style_menu = {
    { { N_("Change _Weight..."), "styleschangeweight", "<control><shift>exclam" }, {}, LegacyCallbacks, MID_Embolden },
    { { N_("_Italic..."), "stylesitalic", "" }, {}, LegacyCallbacks, MID_Italic },
    { { N_("Obli_que..."), "stylesoblique", "" }, {}, LegacyCallbacks, MID_Oblique },
    { { N_("_Condense/Extend..."), "stylesextendcondense", "" }, {}, LegacyCallbacks, MID_Condense },
    { { N_("Change _X-Height..."), "styleschangexheight", "" }, {}, LegacyCallbacks, MID_ChangeXHeight },
    { { N_("Change _Glyph..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_ChangeGlyph },
    kMenuSeparator,
    { { N_("Add _Small Capitals..."), "stylessmallcaps", "" }, {}, LegacyCallbacks, MID_SmallCaps },
    { { N_("Add Subscripts/Superscripts..."), "stylessubsuper", "" }, {}, LegacyCallbacks, MID_SubSup },
    kMenuSeparator,
    { { N_("In_line..."), "stylesinline", "" }, {}, LegacyCallbacks, MID_Inline },
    { { N_("_Outline..."), "stylesoutline", "" }, {}, LegacyCallbacks, MID_Outline },
    { { N_("S_hadow..."), "stylesshadow", "" }, {}, LegacyCallbacks, MID_Shadow },
    { { N_("_Wireframe..."), "styleswireframe", "" }, {}, LegacyCallbacks, MID_Wireframe },
};

std::vector<MenuInfo> transformations_menu = {
    { { N_("_Transform..."), "elementtransform", "<control>backslash" }, {}, LegacyCallbacks, MID_Transform },
    { { N_("_Point of View Projection..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_POV },
    { { N_("_Non Linear Transform..."), NoDecoration, "<control><shift>colon" }, {}, LegacyCallbacks, MID_NLTransform },
};

std::vector<MenuInfo> overlap_menu = {
    { { N_("_Remove Overlap"), "overlaprm", "<control><shift>O" }, {}, LegacyCallbacks, MID_RmOverlap },
    { { N_("_Intersect"), "overlapintersection", "" }, {}, LegacyCallbacks, MID_Intersection },
    { { N_("_Find Intersections"), "overlapfindinter", "" }, {}, LegacyCallbacks, MID_FindInter },
};

std::vector<MenuInfo> simplify_menu = {
    { { N_("_Simplify"), "elementsimplify", "<control><shift>M" }, {}, LegacyCallbacks, MID_Simplify },
    { { N_("Simplify More..."), NoDecoration, "<alt><control><shift>M" }, {}, LegacyCallbacks, MID_SimplifyMore },
    { { N_("Clea_nup Glyph"), NoDecoration, "" }, {}, LegacyCallbacks, MID_CleanupGlyph },
    { { N_("Canonical Start _Point"), NoDecoration, "" }, {}, LegacyCallbacks, MID_CanonicalStart },
    { { N_("Canonical _Contours"), NoDecoration, "" }, {}, LegacyCallbacks, MID_CanonicalContours },
};

std::vector<MenuInfo> round_menu = {
    { { N_("To _Int"), "elementround", "<control><shift>underscore" }, {}, LegacyCallbacks, MID_Round },
    { { N_("To _Hundredths"), NoDecoration, "" }, {}, LegacyCallbacks, MID_Hundredths },
    { { N_("_Cluster"), NoDecoration, "" }, {}, LegacyCallbacks, MID_Cluster },
};

std::vector<MenuInfo> elem_build_menu = {
    { { N_("_Build Accented Glyph"), "elementbuildaccent", "<control><shift>A" }, {}, LegacyCallbacks, MID_BuildAccent },
    { { N_("Build _Composite Glyph"), "elementbuildcomposite", "" }, {}, LegacyCallbacks, MID_BuildComposite },
    { { N_("Buil_d Duplicate Glyph"), NoDecoration, "" }, {}, LegacyCallbacks, MID_BuildDuplicates },
};

std::vector<MenuInfo> element_menu = {
    { { N_("_Font Info..."), "elementfontinfo", "<control><shift>F" }, {}, LegacyCallbacks, MID_FontInfo },
    { { N_("Glyph _Info..."), "elementglyphinfo", "<control>i" }, {}, LegacyCallbacks, MID_CharInfo },
    { { N_("Other Info"), "elementotherinfo", "" }, other_info_menu, SubMenuCallbacks, 0 },
    { { N_("_Validation"), "elementvalidate", "" }, validation_menu, SubMenuCallbacks, 0 },
    kMenuSeparator,
    { { N_("Bitm_ap Strikes Available..."), "elementbitmapsavail", "<control><shift>B" }, {}, LegacyCallbacks, MID_AvailBitmaps },
    { { N_("Regenerate _Bitmap Glyphs..."), "elementregenbitmaps", "<control>B" }, {}, LegacyCallbacks, MID_RegenBitmaps },
    { { N_("Remove Bitmap Glyphs..."), "elementremovebitmaps", "" }, {}, LegacyCallbacks, MID_RemoveBitmaps },
    kMenuSeparator,
    { { N_("St_yle"), "elementstyles", "" }, style_menu, LegacySubMenuCallbacks, MID_Styles },
    { { N_("_Transformations"), "elementtransform", "" }, transformations_menu, LegacySubMenuCallbacks, MID_Transform },
    { { N_("_Expand Stroke..."), "elementexpandstroke", "<control><shift>E" }, {}, LegacyCallbacks, MID_Stroke },
#ifdef FONTFORGE_CONFIG_TILEPATH
    { { N_("Tile _Path..."), "elementtilepath", "" }, {}, LegacyCallbacks, MID_TilePath },
    { { N_("Tile Pattern..."), "elementtilepattern", "" }, {}, LegacyCallbacks, MID_TilePattern },
#endif
    { { N_("O_verlap"), "overlaprm", "" }, overlap_menu, LegacySubMenuCallbacks, MID_RmOverlap },
    { { N_("_Simplify"), "elementsimplify", "" }, simplify_menu, LegacySubMenuCallbacks, MID_Simplify },
    { { N_("Add E_xtrema"), "elementaddextrema", "<control><shift>X" }, {}, LegacyCallbacks, MID_AddExtrema },
    { { N_("Add Points Of I_nflection"), "elementaddinflections", "<control><shift>Y" }, {}, LegacyCallbacks, MID_AddInflections },
    { { N_("_Balance"), "elementbalance", "<control><shift>P" }, {}, LegacyCallbacks, MID_Balance },
    { { N_("Harmoni_ze"), "elementharmonize", "<control><shift>Z" }, {}, LegacyCallbacks, MID_Harmonize },
    { { N_("Roun_d"), "elementround", "" }, round_menu, LegacySubMenuCallbacks, MID_Round },
    { { N_("Autot_race"), "elementautotrace", "<control><shift>T" }, {}, { run_autotrace, LegacyEnabled, NotCheckable }, MID_Autotrace },
    kMenuSeparator,
    { { N_("_Correct Direction"), "elementcorrectdir", "<control><shift>D" }, {}, LegacyCallbacks, MID_Correct },
    kMenuSeparator,
    { { N_("B_uild"), "elementbuildaccent", "" }, elem_build_menu, SubMenuCallbacks, 0 },
    kMenuSeparator,
    { { N_("_Merge Fonts..."), "elementmergefonts", "" }, {}, LegacyCallbacks, MID_MergeFonts },
    { { N_("Interpo_late Fonts..."), "elementinterpolatefonts", "" }, {}, LegacyCallbacks, MID_InterpolateFonts },
    { { N_("Compare Fonts..."), "elementcomparefonts", "" }, {}, LegacyCallbacks, MID_FontCompare },
    { { N_("Compare Layers..."), "elementcomparelayers", "" }, {}, LegacyCallbacks, MID_LayersCompare },
};

/////////////////////////////////// TOOLS MENU ////////////////////////////////////////

std::vector<MenuInfo> tools_menu = {
    MenuInfo::CustomBlock(python_tools),
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

std::vector<MenuInfo> reencode_menu = {
    MenuInfo::CustomBlock(encoding_reencode),
};

std::vector<MenuInfo> force_encoding_menu = {
    MenuInfo::CustomBlock(encoding_force_encoding),
};

std::vector<MenuInfo> encoding_menu = {
    { { N_("_Reencode"), NoDecoration, "" }, reencode_menu, LegacyCallbacks, MID_Reencode },
    { { N_("_Compact (hide unused glyphs)"), Checkable, "" }, {}, LegacyCallbacks, MID_Compact },
    { { N_("_Force Encoding"), NoDecoration, "" }, force_encoding_menu, LegacyCallbacks, MID_ForceReencode },
    kMenuSeparator,
    { { N_("_Add Encoding Slots..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_AddUnencoded },
    { { N_("Remove _Unused Slots"), NoDecoration, "" }, {}, LegacyCallbacks, MID_RemoveUnused },
    { { N_("_Detach Glyphs"), NoDecoration, "" }, {}, LegacyCallbacks, MID_DetachGlyphs },
    { { N_("Detach & Remo_ve Glyphs..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_DetachAndRemoveGlyphs },
    kMenuSeparator,
    { { N_("Add E_ncoding Name..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_AddEncoding },
    { { N_("_Load Encoding..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_LoadEncoding },
    { { N_("Ma_ke From Font..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_MakeFromFont },
    { { N_("Remove En_coding..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_RemoveEncoding },
    kMenuSeparator,
    { { N_("Display By _Groups..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_DisplayByGroups },
    { { N_("D_efine Groups..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_DefineGroups },
    kMenuSeparator,
    { { N_("_Save Namelist of Font..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_SaveNamelist },
    { { N_("L_oad Namelist..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_LoadNameList },
    { { N_("Rename Gl_yphs..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_RenameGlyphs },
    { { N_("Cre_ate Named Glyphs..."), NoDecoration, "" }, {}, LegacyCallbacks, MID_NameGlyphs },
};

std::vector<MenuInfo> layers_menu = {
    MenuInfo::CustomBlock(view_menu_layers),
};

std::vector<MenuInfo> anchor_pairs_menu = {
    MenuInfo::CustomBlock(view_menu_anchors),
};

std::vector<MenuInfo> combinations_menu = {
    { { N_("_Kern Pairs"), NoDecoration, "" }, {}, LegacyCallbacks, MID_KernPairs },
    { { N_("_Anchored Pairs"), NoDecoration, "" }, anchor_pairs_menu, SubMenuCallbacks, MID_AnchorPairs },
    { { N_("_Ligatures"), NoDecoration, "" }, {}, LegacyCallbacks, MID_Ligatures },
};

std::vector<MenuInfo> label_glyph_menu = {
    { { N_("_Glyph Image"), GlyphLabel, "" }, {}, LegacyCallbacks, MIDSERIES_LabelGlyph + gl_glyph },
    { { N_("_Name"), GlyphLabel, "" }, {}, LegacyCallbacks, MIDSERIES_LabelGlyph + gl_name },
    { { N_("_Unicode"), GlyphLabel, "" }, {}, LegacyCallbacks, MIDSERIES_LabelGlyph + gl_unicode },
    { { N_("_Encoding Hex"), GlyphLabel, "" }, {}, LegacyCallbacks, MIDSERIES_LabelGlyph + gl_encoding },
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
    { { N_("Com_binations"), NoDecoration, "" }, combinations_menu, SubMenuCallbacks, 0 },
    kMenuSeparator,
    { { N_("Label Gl_yph By"), NoDecoration, "" }, label_glyph_menu, SubMenuCallbacks, 0 },
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
    { { N_("E_lement") }, element_menu, SubMenuCallbacks, -1 },
#ifndef _NO_PYTHON
    { { N_("_Tools") }, tools_menu, SubMenuCallbacks, -1 },
#endif
    { { N_("H_ints") }, hints_menu, SubMenuCallbacks, -1 },
    { { N_("E_ncoding") }, encoding_menu, SubMenuCallbacks, -1 },
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
