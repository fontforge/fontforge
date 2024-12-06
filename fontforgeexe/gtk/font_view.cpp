/* Copyright 2024 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#include "font_view.hpp"
#include "utils.hpp"

namespace ff::views {

bool on_drawing_area_event(GdkEvent* event);

FontView::FontView(int width, int height) {
    static auto app = Gtk::Application::create("org.fontforge");

    // Fontforge drawing area processes events in the legacy code
    // expose, keypresses, mouse etc.
    drawing_area.signal_event().connect(&on_drawing_area_event);
    drawing_area.set_events(Gdk::ALL_EVENTS_MASK);
    window.add(drawing_area);

    window.show_all();
    window.resize(width, height);

    // TODO(iorsh): review this very stragne hack.
    // For reasons beyond my comprehension, DrawingArea fails to catch events
    // before this function is called. A mere traverasal of widget tree should
    // theoretically have no side efects, but it does.
    gtk_find_child(&window, "");
}

GtkWidget* FontView::get_drawing_widget_c() {
    return (GtkWidget*)drawing_area.gobj();
}

/////////////////  EVENTS  ////////////////////

bool on_drawing_area_event(GdkEvent* event) {
    // Normally events automatically get to the main loop and picked from there
    // by the legacy GDraw handler. The DrawingArea::resize signal doesn't go
    // there for some reason. My best guess is that this signal is not emitted,
    // but the framework just invokes the handler directly. Here we catch it and
    // place into the main loop, so that it can reach the GDraw handler.
    if (event->type == GDK_CONFIGURE) {
        gdk_event_put(event);
        return true;
    }

    // Return false to allow further propagation of unhandled events
    return false;
}

}  // namespace ff::views