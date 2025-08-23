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

#include <clocale>
#include <string>
#include <gtkmm.h>

namespace ff::ui_utils {

double ui_font_em_size();
double ui_font_eX_size();

double get_current_ppi(Gtk::Widget* w);

inline char get_decimal_point() { return *std::localeconv()->decimal_point; }

// Under some locales (e.g. French) the double values may contain commas, and in
// that case we separate the list entries by semicolons.
// See https://en.wikipedia.org/wiki/Semicolon#Data.
inline char get_list_separator() {
    return get_decimal_point() == ',' ? ';' : ',';
}

// TODO(iorsh): Integrate this function into the global log collection
void post_error(const char* title, const char* statement, ...);

Glib::RefPtr<Gdk::Cursor> set_cursor(Gtk::Widget* widget,
                                     const Glib::ustring& name);

void unset_cursor(Gtk::Widget* widget, Glib::RefPtr<Gdk::Cursor> old_cursor);

void apply_css(Gtk::Widget& w, const std::string& style);

// Change widget's visual state without triggering the related signal. This
// function is useful when the widget performs some action on activation and
// changes its apperance (e.g. toggle button). Sometimes we just want to set its
// apperance without triggereing the action.
//
// Arguments:
//  * w - GTK widget. Note that it might need to be cast to the type which
//    actually defines the signal, to avoid a conflict during template type
//    deduction.
//  * signal - the signal which should be disconnected and reconnected when
//    changing the visual state.
//  * signal_handler - the function which should be reconnected to the signal
//    after the visual state was changed. Generally, this is just the normal
//    handler for that signal.
//  * state_changer - the function which actually changes the visual state while
//    the signal is disconnected.
template <typename WIDGET, typename SIGNAL_PROXY, typename SIGNAL_HANDLER>
void gtk_set_widget_state_without_event(WIDGET* w,
                                        SIGNAL_PROXY (WIDGET::*signal)(),
                                        SIGNAL_HANDLER signal_handler,
                                        std::function<void()> state_changer) {
    // Currently we use a predefined string to store custom data in the widget's
    // Glib::Object. This should work as long as we are not trying to make two
    // different calls of such kind to any given widget.
    static const char kSignalActivateConn[] = "signal_activate_conn";

    // Disconnect the activation signal before setting visual state
    sigc::connection* conn =
        (sigc::connection*)w->get_data(kSignalActivateConn);
    if (conn) {
        conn->disconnect();
        delete conn;
        conn = NULL;
    }

    // Call the action which sets the new visual state
    state_changer();

    // Reconnect the activation signal
    conn = new sigc::connection((w->*signal)().connect(signal_handler));
    w->set_data(kSignalActivateConn, conn);
}

}  // namespace ff::ui_utils
