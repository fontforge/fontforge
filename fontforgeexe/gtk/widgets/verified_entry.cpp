/* Copyright 2025 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#include "verified_entry.hpp"

namespace ff::widgets {

VerifiedEntry::VerifiedEntry() {
    error_css_provider_ = Gtk::CssProvider::create();
    error_css_provider_->load_from_data(
        "entry { color: @error_color; border-color: @error_color}");

    // Remove the error highlight when the user focuses on the widget, to
    // prevent visual annoyance during the edit.
    signal_focus_in_event().connect(
        sigc::mem_fun(*this, &VerifiedEntry::focus_in_event_slot));
}

bool VerifiedEntry::verify() {
    int start_pos = 0, end_pos = 0;
    Glib::ustring text = get_text();
    bool contents_valid = verifier_(text, start_pos, end_pos);
    if (!contents_valid) {
        // Special CSS has higher priority than the legacy GDraw compatibility
        // CSS, but lower than the GTK inspector CSS.
        get_style_context()->add_provider(error_css_provider_,
                                          GTK_STYLE_PROVIDER_PRIORITY_USER - 1);
        select_region(start_pos, end_pos);

        // This widget is designed to clear highlight when the user focuses on
        // it to fix the value. The focus is not always lost (e.g. when pressing
        // Enter to close the dialog), so we force it to later trigger the
        // focus-in.
        Gtk::Window* top = dynamic_cast<Gtk::Window*>(get_toplevel());
        if (top) top->unset_focus();
    }
    return contents_valid;
}

bool VerifiedEntry::focus_in_event_slot(GdkEventFocus*) {
    get_style_context()->remove_provider(error_css_provider_);
    return false;
}

}  // namespace ff::widgets
