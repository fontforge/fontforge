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

#include "simple_dialogs.hpp"

#include <numeric>
#include <string>
#include <gtkmm.h>

#include "intl.h"
#include "application.hpp"
#include "dialog.hpp"
#include "language_list.hpp"
#include "utils.hpp"

namespace ff::dlg {

// A simple dialog to query the user for a number of new encoding slots to add.
class NumericalInputDialog final : public Dialog {
 private:
    Gtk::SpinButton* input;

    NumericalInputDialog(GWindow parent, const std::string& title,
                         const std::string& label)
        : Dialog(parent) {
        set_title(title);
        set_resizable(false);

        auto label_widget = Gtk::make_managed<Gtk::Label>(label);
        get_content_area()->pack_start(*label_widget);

        input = Gtk::make_managed<Gtk::SpinButton>();
        input->set_numeric(true);
        input->set_adjustment(Gtk::Adjustment::create(1, 1, 65535, 1, 5, 0));
        input->set_activates_default();
        get_content_area()->pack_start(*input);

        show_all();
    }

    int get_value() const { return input->get_value_as_int(); }

 public:
    // Show the dialog and return either the entered integer, 0 if the dialog
    // was cancelled/closed, or -1 if something strange happened
    static int show(GWindow parent, const std::string& title,
                    const std::string& label) {
        NumericalInputDialog dialog(parent, title, label);
        int i = -1;
        dialog.signal_response().connect([&](int response_id) {
            // Ok/Add returns the entered integer
            if (response_id == Gtk::RESPONSE_OK) {
                i = dialog.get_value();
            }
            // Cancel/close returns 0, which is semantically equivalent to the
            // user actually entering 0
            else if (response_id == Gtk::RESPONSE_CANCEL ||
                     response_id == Gtk::RESPONSE_DELETE_EVENT ||
                     response_id == Gtk::RESPONSE_CLOSE) {
                i = 0;
            }
            // Any other response, which shouldn't really be possible, returns
            // -1 to indicate an error
        });
        dialog.run();
        return i;
    }
};

}  // namespace ff::dlg

// Shim for the C code to call the dialog
int add_encoding_slots_dialog(GWindow parent, bool cid) {
    // To avoid instability, the GTK application is lazily initialized only when
    // a GTK window is invoked.
    ff::app::GtkApp();

    return ff::dlg::NumericalInputDialog::show(
        parent, _("Add Encoding Slots..."),
        cid ? _("How many CID slots do you wish to add?")
            : _("How many unencoded glyph slots do you wish to add?"));
}

char* language_list_dialog(GWindow parent, const LanguageRec* languages,
                           const char* initial_tags) {
    // To avoid instability, the GTK application is lazily initialized only when
    // a GTK window is invoked.
    ff::app::GtkApp();

    ff::dlg::LanguageRecords language_vec;
    for (const LanguageRec* lang = languages; lang->name != nullptr; ++lang) {
        language_vec.emplace_back(lang->name, lang->tag);
    }

    // Compute initial selection
    std::stringstream ss(initial_tags);
    std::vector<int> tag_list;
    std::vector<std::string> unrecognized_tags;
    while (ss.good()) {
        std::string substr;
        std::getline(ss, substr, ',');
        auto it = std::find_if(
            language_vec.begin(), language_vec.end(),
            [&substr](const auto& rec) { return (rec.second == substr); });
        if (it != language_vec.end()) {
            tag_list.push_back(it - language_vec.begin());
        } else {
            unrecognized_tags.push_back(substr);
        }
    }

    if (unrecognized_tags.size() == 1)
        gtk_post_error(_("Unknown Language"),
                       _("The language, '%s', is not in the list of known "
                         "languages and will be omitted"),
                       unrecognized_tags[0].c_str());
    else if (unrecognized_tags.size() > 1)
        gtk_post_error(_("Unknown Language"),
                       _("Several language tags, including '%s', are not in "
                         "the list of known languages and will be omitted"),
                       unrecognized_tags[0].c_str());

    std::vector<int> selection =
        ff::dlg::LanguageListDlg::show(parent, language_vec, tag_list);

    if (selection.empty()) {
        return nullptr;
    } else {
        auto append_tag = [&language_vec](std::string a, int b) {
            return std::move(a) + ',' + (const char*)language_vec[b].second;
        };

        // Comma-seperated tags
        std::string s((const char*)language_vec[selection[0]].second);
        s = std::accumulate(std::next(selection.begin()), selection.end(),
                            s,  // start with first element
                            append_tag);
        return strdup(s.c_str());
    }
}

void update_appearance() {
    ff::app::GtkApp();
    ff::app::load_legacy_style();
}
