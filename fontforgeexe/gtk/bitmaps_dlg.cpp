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

static const uint16_t kSizeError = 0xffffu;

static std::string FormatBitmapSize(int px_size) {
    return std::to_string(px_size);
}

static std::string FormatBitmapSize(int px_size, double scale) {
    char decimal_point = ui_utils::get_decimal_point();
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.1f", px_size * scale);
    return buffer;
}

template <typename... ARGS>
static std::string SizeString(const BitmapSizes& sizes, ARGS... args) {
    std::string list_separator =
        ui_utils::get_list_separator() + std::string(" ");
    std::string size_list;
    for (auto [px_size, depth] : sizes) {
        if (!size_list.empty()) size_list += list_separator;
        if (depth == 1) {
            size_list += FormatBitmapSize(px_size, args...);
        } else {
            size_list += FormatBitmapSize(px_size, args...) + "@" +
                         std::to_string(depth);
        }
    }
    return size_list;
}

static BitmapSizes ParseList(const Glib::ustring& str) {
    BitmapSizes sizes;
    uint16_t size, depth;
    char* end = nullptr;

    for (const char* pt = str.c_str(); *pt != '\0';) {
        size = strtoul(pt, &end, 10);
        if (*end == '@')
            depth = strtoul(end + 1, &end, 10);
        else
            depth = 1u;
        if (size > 0) sizes.emplace_back(size, depth);
        if (*end != ' ' && *end != ',' && *end != ';' && *end != '\0') {
            // If the token is bad, we seek to its end and report its position
            // in a slight abuse of the return value.
            while (*end != ' ' && *end != ',' && *end != ';' && *end != '\0')
                ++end;
            return {{kSizeError, kSizeError},
                    {pt - str.c_str(), end - str.c_str()}};
        }
        // We are pedantic with list separators, but the user could be not.
        while (*end == ' ' || *end == ',' || *end == ';') ++end;
        pt = end;
    }

    return sizes;
}

Glib::ustring BitmapsDlg::last_scope_ = "selection";

BitmapsDlg::BitmapsDlg(GWindow parent, BitmapsDlgMode mode,
                       const BitmapSizes& sizes, bool bitmaps_only,
                       bool has_current_char)
    : DialogBase(parent) {
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
    set_hints_horizontal_resize_only();

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

    pixels_entry_.set_text(SizeString(sizes));
    pixels_entry_.set_activates_default();
    pixels_entry_.set_verifier(pixel_size_verifier);
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
    pt_this_frame->signal_realize().connect([pt_this_frame, pt_this_entry,
                                             sizes]() {
        // Set the PPI here, as the monitor is not available before the
        // realization.
        int current_ppi = ui_utils::get_current_ppi(pt_this_frame);
        char* pt_this_label =
            smprintf(_("Point sizes on this monitor (%d PPI)"), current_ppi);
        pt_this_frame->set_label(pt_this_label);
        pt_this_entry->set_text(SizeString(sizes, 72.0 / current_ppi));
        free(pt_this_label);
    });
    get_content_area()->pack_start(*pt_this_frame);

    auto pt_96_frame =
        Gtk::make_managed<Gtk::Frame>(_("Point sizes on a 96 PPI monitor"));
    auto pt_96_entry = Gtk::make_managed<Gtk::Entry>();
    pt_96_entry->set_text(SizeString(sizes, 72.0 / 96.0));
    pt_96_entry->set_editable(false);
    pt_96_entry->set_can_focus(false);
    pt_96_frame->add(*pt_96_entry);
    pt_96_frame->set_shadow_type(Gtk::SHADOW_NONE);
    get_content_area()->pack_start(*pt_96_frame);

    pixels_entry_.signal_changed().connect(
        [this, pt_96_entry, pt_this_entry]() {
            auto sizes = ParseList(pixels_entry_.get_text());
            if (!sizes.empty() && sizes[0].first == kSizeError) {
                return;
            }

            int current_ppi = ui_utils::get_current_ppi(pt_this_entry);
            pt_this_entry->set_text(SizeString(sizes, 72.0 / current_ppi));
            pt_96_entry->set_text(SizeString(sizes, 72.0 / 96.0));
        });

    if (mode == bitmaps_dlg_avail) {
        rasterize_check_.set_label(
            _("Create Rasterized Strikes (Not empty ones)"));
        rasterize_check_.set_halign(Gtk::ALIGN_START);
        rasterize_check_.set_active(true);
        get_content_area()->pack_start(rasterize_check_);
    }

    signal_response().connect(
        [this](int response_id) {
            if (response_id == Gtk::RESPONSE_OK && !pixels_entry_.verify()) {
                signal_response().emission_stop();
            }
        },
        false);

    get_content_area()->set_spacing(0.5 * ui_utils::ui_font_eX_size());
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

    // In Font View the current character is not applicable, and we need to
    // disable its entry in the drop-down list. Conversely, in Char and Metrics
    // Views the current character is applicable, but the selection belongs to
    // an obscured Font View, and acting on it would be counterintuitive.
    Glib::ustring disabled_item = has_current_char ? "selection" : "current";

    Glib::ustring active_item = (last_scope_ == "all") ? last_scope_
                                : has_current_char     ? "current"
                                                       : "selection";
    glyphs_combo.set_active_id(active_item);

    Gtk::CellRenderer* renderer = glyphs_combo.get_first_cell();
    glyphs_combo.set_cell_data_func(
        *renderer,
        [renderer, disabled_item](const Gtk::TreeModel::const_iterator& it) {
            // In the Gtk::ComboBoxText, the second column of the TreeModel
            // contains the item id as a string. It's a sacred knowledge, we
            // must not question it.
            Glib::ustring item_id;
            it->get_value(1, item_id);
            renderer->set_sensitive(item_id != disabled_item);
        });

    return glyphs_combo;
}

bool BitmapsDlg::pixel_size_verifier(const Glib::ustring& text, int& start_pos,
                                     int& end_pos) {
    auto sizes = ParseList(text);
    bool is_valid = true;
    if (sizes.size() == 2 && sizes[0].first == kSizeError) {
        is_valid = false;
        start_pos = sizes[1].first;
        end_pos = sizes[1].second;
    }
    return is_valid;
}

bool BitmapsDlg::show() {
    Gtk::ResponseType result = run();
    if (result == Gtk::RESPONSE_OK) {
        last_scope_ = get_active_scope();
        return true;
    }
    return false;
}

BitmapSizes BitmapsDlg::get_sizes() const {
    return ParseList(pixels_entry_.get_text());
}

bool BitmapsDlg::get_rasterize() const { return rasterize_check_.get_active(); }

Glib::ustring BitmapsDlg::get_active_scope() const {
    return glyphs_combo_.get_active_id();
}

}  // namespace ff::dlg
