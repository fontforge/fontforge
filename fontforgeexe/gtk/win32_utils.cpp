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

#include "win32_utils.hpp"

bool is_win32_display() {
    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    if (display) {
        return (strcmp(G_OBJECT_TYPE_NAME(display->gobj()), "GdkWin32Display") == 0);
    }
    return false;
}

Gdk::Rectangle get_win32_print_preview_size() {
    Gdk::Rectangle preview_rectangle;

    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    if (display) {
        Glib::RefPtr<const Gdk::Monitor> monitor = display->get_primary_monitor();
        if (monitor) {
            Gdk::Rectangle workarea;
            monitor->get_workarea(workarea);

            // Calculate a rather big roughly centered area of 2/3 screen.
            preview_rectangle = Gdk::Rectangle(workarea.get_width() / 6, workarea.get_height() / 6,
             2 * workarea.get_width() / 3, 2 * workarea.get_height() / 3);
            return preview_rectangle;
        }
    }

    // Falback to something reasonable
    preview_rectangle = Gdk::Rectangle(100, 100, 640, 480);
    return preview_rectangle;
}
