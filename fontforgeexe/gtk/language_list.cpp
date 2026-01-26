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

#include "language_list.hpp"

#include "utils.hpp"
#include "intl.h"

namespace ff::dlg {

LanguageListDlg::LanguageListDlg(GWindow parent,
                                 const ff::dlg::LanguageRecords& lang_recs,
                                 const std::vector<int>& initial_selection)
    : DialogBase(parent), list_(1, false, Gtk::SELECTION_MULTIPLE) {
    set_title(_("Language List"));
    set_help_context("ui/dialogs/lookups.html", "#lookups-scripts-dlg");

    auto scrolled_window = Gtk::make_managed<Gtk::ScrolledWindow>();
    scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    auto search_entry = Gtk::make_managed<Gtk::SearchEntry>();
    // The default behavior resets the current selection and selects the
    // matching row. Since we have a multiple-selection list, we want to just
    // scroll to the matching row.
    search_entry->signal_search_changed().connect([search_entry, this]() {
        Glib::ustring search_text = search_entry->get_text();
        // Look for matching row, assuming the rows are sorted
        // alphabetically.
        int row_idx = 0;
        for (; row_idx < list_.size(); ++row_idx) {
            if (search_text.lowercase() < list_.get_text(row_idx).lowercase())
                break;
        }
        list_.scroll_to_row(Gtk::TreePath(std::to_string(row_idx)), 0.0);
    });

    for (const auto& [name, tag] : lang_recs) {
        list_.append(name);
    }
    list_.set_headers_visible(false);
    list_.set_enable_search(false);
    list_.set_tooltip_text(
        _("Select as many languages as needed\n"
          "Hold down the control key when clicking\n"
          "to make disjoint selections."));

    // Set initial selection
    for (int idx : initial_selection) {
        list_.get_selection()->select(Gtk::TreePath(std::to_string(idx)));
    }

    // Show initially roughly 8 rows.
    property_default_height() = 25 * ui_utils::ui_font_eX_size();

    scrolled_window->add(list_);

    get_content_area()->pack_start(*search_entry, Gtk::PACK_SHRINK);
    get_content_area()->pack_start(*scrolled_window, Gtk::PACK_EXPAND_WIDGET);

    // Connect this signal before the normal Ok/Cancel processing slot, to
    // prevent dialog closing if we wish.
    signal_response().connect(
        [this](int response_id) {
            if (response_id == Gtk::RESPONSE_OK) {
                if (list_.get_selected().empty()) {
                    ui_utils::post_error(
                        _("Language Missing"),
                        _("You must select at least one language.\nUse the "
                          "\"Default\" language if nothing else fits."));
                    signal_response().emission_stop();
                }
            }
        },
        false);

    signal_realize().connect([this]() { list_.grab_focus(); });

    show_all();
}

std::vector<int> LanguageListDlg::show(
    GWindow parent, const ff::dlg::LanguageRecords& lang_recs,
    const std::vector<int>& initial_selection) {
    LanguageListDlg dialog(parent, lang_recs, initial_selection);

    Gtk::ResponseType result = dialog.run();
    std::vector<int> selection;

    if (result == Gtk::RESPONSE_OK) {
        selection = dialog.get_selection();
    }
    return selection;
}

}  // namespace ff::dlg
