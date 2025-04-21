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

#include "utils.hpp"

Glib::RefPtr<Gdk::Window> gtk_get_topmost_window() {
    Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();

    Glib::RefPtr<Gdk::Window> topmost_window = screen->get_active_window();
    if (topmost_window) {
        return topmost_window;
    }

    // Gdk::Screen::get_active_window() is not always reliable, e.g when the
    // active window was just closed, and its parent hasn't been activated back
    // yet.
    std::vector<Glib::RefPtr<Gdk::Window>> stack = screen->get_window_stack();
    for (auto rit = stack.rbegin(); rit != stack.rend(); ++rit) {
        if ((*rit)->get_window_type() == Gdk::WINDOW_TOPLEVEL &&
            (*rit)->is_visible()) {
            topmost_window = *rit;
            break;
        }
    }

    return topmost_window;
}
