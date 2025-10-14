/* Copyright 2023 Joey Sabey <github.com/Omnikron13>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "language_list.hpp"

#include "utils.hpp"
#include "intl.h"

namespace ff::dlg {

LanguageListDlg::LanguageListDlg(GWindow parent,
                                 const ff::dlg::LanguageRecords& lang_recs,
                                 const std::vector<int>& initial_selection)
    : Dialog(parent), list_(1, false, Gtk::SELECTION_MULTIPLE) {
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

    // Show initially roughtly 8 rows.
    property_default_height() = 40 * ui_font_eX_size();

    scrolled_window->add(list_);

    get_content_area()->pack_start(*search_entry, Gtk::PACK_SHRINK);
    get_content_area()->pack_start(*scrolled_window, Gtk::PACK_EXPAND_WIDGET);

    // Connect this signal before the normal Ok/Cancel processing slot, to
    // prevent dialog closing if we wish.
    signal_response().connect(
        [this](int response_id) {
            if (response_id == Gtk::RESPONSE_OK) {
                if (list_.get_selected().empty()) {
                    gtk_post_error(
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
