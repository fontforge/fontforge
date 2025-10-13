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

    auto scrolled_window = Gtk::make_managed<Gtk::ScrolledWindow>();
    scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    auto search_entry = Gtk::make_managed<Gtk::SearchEntry>();

    for (const auto& [name, tag] : lang_recs) {
        list_.append(name);
    }
    list_.set_headers_visible(false);
    list_.set_search_entry(*search_entry);
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
