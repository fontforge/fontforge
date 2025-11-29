/* Copyright (C) 2025 by Maxim Iorsh <iorsh@users@sourceforge.net>
 *
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

#include "bitmaps_dlg.hpp"

#include <numeric>

#include "utils.hpp"
#include "intl.h"

extern "C" {
#include "ustring.h"
}

namespace ff::dlg {

BitmapsDlg::BitmapsDlg(GWindow parent, BitmapsDlgMode mode, bool bitmaps_only,
                       bool has_current_char)
    : Dialog(parent) {
    set_help_context("ui/menus/elementmenu.html", "#elementmenu-bitmaps");
    Glib::ustring title;
    std::vector<Glib::ustring> headings;

    if (mode == bitmaps_dlg_avail) {
        title = _("Bitmap Strikes Available");
        headings = {_("The list of current pixel bitmap sizes"),
                    _(" Removing a size will delete it."),
                    bitmaps_only
                        ? _(" Adding a size will create it by scaling.")
                        : _(" Adding a size will create it.")};
    } else if (mode == bitmaps_dlg_regen) {
        title = _("Regenerate Bitmap Glyphs");
        headings = {_("Specify bitmap sizes to be regenerated")};
    } else {
        title = _("Remove Bitmap Glyphs");
        headings = {_("Specify bitmap sizes to be removed")};
    }

    set_title(title);

    Glib::ustring heading = std::accumulate(
        headings.begin() + 1, headings.end(), headings[0],
        [](const auto& a, const auto& b) { return a + "\n•  " + b; });
    auto heading_label = Gtk::make_managed<Gtk::Label>(heading);
    heading_label->set_halign(Gtk::ALIGN_START);
    get_content_area()->pack_start(*heading_label);

    if (mode != bitmaps_dlg_avail) {
        glyphs_combo_ = build_glyphs_combo(has_current_char);
        get_content_area()->pack_start(glyphs_combo_);
    }

    auto pixels_frame = Gtk::make_managed<Gtk::Frame>(_("Pixel Sizes:"));
    pixels_frame->add(pixels_entry_);
    pixels_frame->set_shadow_type(Gtk::SHADOW_NONE);
    get_content_area()->pack_start(*pixels_frame);

    auto pt_this_frame = Gtk::make_managed<Gtk::Frame>();
    auto pt_this_entry = Gtk::make_managed<Gtk::Entry>();
    pt_this_entry->set_editable(false);
    pt_this_entry->set_can_focus(false);
    pt_this_frame->add(*pt_this_entry);
    pt_this_frame->set_shadow_type(Gtk::SHADOW_NONE);
    pt_this_frame->signal_realize().connect([pt_this_frame]() {
        // Set the PPI here, as the monitor is not available before the
        // realization.
        int current_ppi = ui_utils::get_current_ppi(pt_this_frame);
        char* pt_this_label =
            smprintf(_("Point sizes on this monitor (%d PPI)"), current_ppi);
        pt_this_frame->set_label(pt_this_label);
        free(pt_this_label);
    });
    get_content_area()->pack_start(*pt_this_frame);

    auto pt_96_frame =
        Gtk::make_managed<Gtk::Frame>(_("Point sizes on a 96 ppi monitor"));
    auto pt_96_entry = Gtk::make_managed<Gtk::Entry>();
    pt_96_entry->set_editable(false);
    pt_96_entry->set_can_focus(false);
    pt_96_frame->add(*pt_96_entry);
    pt_96_frame->set_shadow_type(Gtk::SHADOW_NONE);
    get_content_area()->pack_start(*pt_96_frame);

    if (mode == bitmaps_dlg_avail) {
        rasterize_check_.set_label(
            _("Create Rasterized Strikes (Not empty ones)"));
        rasterize_check_.set_halign(Gtk::ALIGN_START);
        rasterize_check_.set_active(true);
        get_content_area()->pack_start(rasterize_check_);
    }

    get_content_area()->set_spacing(ui_utils::ui_font_eX_size());
    show_all();
}

Gtk::ComboBoxText BitmapsDlg::build_glyphs_combo(bool has_current_char) const {
    static const std::vector<std::pair<Glib::ustring, Glib::ustring>>
        glyphs_combo_items = {
            {"all", _("All Glyphs")},
            {"selection", _("Selected Glyphs")},
            {"current", _("Current Glyph")},
        };

    Gtk::ComboBoxText glyphs_combo;
    for (const auto& [item_id, item_label] : glyphs_combo_items) {
        glyphs_combo.append(item_id, item_label);
    }
    glyphs_combo.set_active_id("selection");

    // When the current character is not applicable, we need to disable its
    // entry in the drop-down list.
    if (!has_current_char) {
        Gtk::CellRenderer* renderer = glyphs_combo.get_first_cell();
        glyphs_combo.set_cell_data_func(
            *renderer, [renderer](const Gtk::TreeModel::const_iterator& it) {
                // In the Gtk::ComboBoxText, the second column of the TreeModel
                // contains the item id as a string. It's a sacred knowledge, we
                // must not question it.
                Glib::ustring item_id;
                it->get_value(1, item_id);
                renderer->set_sensitive(item_id != "current");
            });
    }
    return glyphs_combo;
}

void BitmapsDlg::show(GWindow parent, BitmapsDlgMode mode, bool bitmaps_only,
                      bool has_current_char) {
    BitmapsDlg dialog(parent, mode, bitmaps_only, has_current_char);

    Gtk::ResponseType result = dialog.run();

    if (result == Gtk::RESPONSE_OK) {
        return;
    }
    return;
}

}  // namespace ff::dlg
