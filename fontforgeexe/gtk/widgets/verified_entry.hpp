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
#pragma once

#include <gtkmm.h>

namespace ff::widgets {

// A text entry field which can verify its contents and highlight the error.
class VerifiedEntry : public Gtk::Entry {
 public:
    // Verification callback for the entry widget contents.
    // Arguments:
    //  * text [input] - widget contents to be verified.
    //  * start_pos, end_pos [output] - the invalid region to be selected in
    //    case of verification failure.
    //  * return value - true if the input is verified successfully, otherwise
    //    false.
    using Verifier = std::function<bool(const Glib::ustring& text,
                                        int& /*start_pos*/, int& /*end_pos*/)>;

    VerifiedEntry();

    void set_verifier(Verifier verifier) { verifier_ = verifier; }

    // Verify the widget contents using the provided callback, and highlight the
    // widget in case of error.
    bool verify();

 private:
    Glib::RefPtr<Gtk::CssProvider> error_css_provider_;
    Verifier verifier_;

    bool focus_in_event_slot(GdkEventFocus*);
};

}  // namespace ff::widgets
