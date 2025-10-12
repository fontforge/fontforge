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
#pragma once

#include "dialog.hpp"
#include "tag.hpp"

namespace ff::dlg {

using LanguageRecords = std::vector<std::pair<std::string /*name*/, ff::Tag>>;

class LanguageListDlg final : public Dialog {
 private:
    Gtk::ListViewText list_;

    LanguageListDlg(GWindow parent, const ff::dlg::LanguageRecords& lang_recs);

    std::vector<int> get_selection() { return list_.get_selected(); }

 public:
    // Show the dialog and return indexes of selected rows (or empty vector if
    // the dialog was cancelled/closed).
    static std::vector<int> show(GWindow parent,
                                 const ff::dlg::LanguageRecords& lang_recs);
};

}  // namespace ff::dlg
