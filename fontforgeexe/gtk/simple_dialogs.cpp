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

#include <string>
#include <gtkmm.h>

#include "intl.h"
#include "application.hpp"
#include "css_builder.hpp"
#include "dialog.hpp"

namespace ff::dlg {

// A simple dialog to query the user for a number of new encoding slots to add.
class NumericalInputDialog final : public ff::dlg::Dialog {
 private:
    Gtk::SpinButton* input;

    NumericalInputDialog(const std::string& title, const std::string& label) {
        set_title(title);

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
    static int show(const std::string& title, const std::string& label) {
        NumericalInputDialog dialog(title, label);
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

void add_css(const std::string& style) {
    Glib::RefPtr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();

    // Load CSS styles
    css_provider->load_from_data(style);

    // Add CSS provider to the screen, so that it applies to all windows.
    auto screen = Gdk::Screen::get_default();

    // User-defined CSS should usually go with USER priority, but we reduce it
    // by 1 so that GTK inspector which also applies USER, would get priority
    // over it.
    Gtk::StyleContext::add_provider_for_screen(
        screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER - 1);
}

}  // namespace ff::dlg

// Shim for the C code to call the dialog
int add_encoding_slots_dialog(bool cid, GResInfo* ri) {
    // To avoid instability, the GTK application is lazily initialized only when
    // a GTK window is invoked.
    ff::app::GtkApp();

    std::string styles = build_styles(ri);
    ff::dlg::add_css(styles);

    return ff::dlg::NumericalInputDialog::show(
        _("Add Encoding Slots..."),
        cid ? _("How many CID slots do you wish to add?")
            : _("How many unencoded glyph slots do you wish to add?"));
}
