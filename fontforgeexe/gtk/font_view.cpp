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

#include "application.hpp"
#include "utils.hpp"

namespace ff::views {

bool on_button_press_event(GdkEventButton* event, Gtk::Menu& pop_up);

FontView::FontView(std::shared_ptr<FVContext> fv_context, int width, int height)
    : context(fv_context), char_grid(fv_context) {
    ff::app::add_top_view(window);

    window.signal_delete_event().connect([this](GdkEventAny*) {
        ff::app::remove_top_view(window);
        return false;
    });

    // dialog.resize() doesn't work until after the realization, i.e. after
    // dialog.show_all(). Use the realize event to ensure reliable resizing.
    //
    // Also, the signal itself must be connected before dialog.show_all(),
    // otherwise it wouldn't work for some reason...
    window.signal_realize().connect([this, width, height]() {
        char_grid.resize_drawing_area(width, height);
    });

    window.add(char_grid.get_top_widget());
    window.show_all();

    Gtk::MenuItem* menu1 = Gtk::make_managed<Gtk::MenuItem>("Menu 1");
    Gtk::MenuItem* menu2 = Gtk::make_managed<Gtk::MenuItem>("Menu 2");
    pop_up.append(*menu1);
    pop_up.append(*menu2);
    char_grid.get_top_widget().signal_button_press_event().connect(
        [this](GdkEventButton* event) {
            return on_button_press_event(event, pop_up);
        });

    // TODO(iorsh): review this very stragne hack.
    // For reasons beyond my comprehension, DrawingArea fails to catch events
    // before this function is called. A mere traverasal of widget tree should
    // theoretically have no side efects, but it does.
    gtk_find_child(&window, "");
}

/////////////////  EVENTS  ////////////////////

bool on_button_press_event(GdkEventButton* event, Gtk::Menu& pop_up) {
    if (event->button == GDK_BUTTON_SECONDARY) {
        pop_up.show_all();
        pop_up.popup(event->button, event->time);
        return true;
    }
    return false;
}

}  // namespace ff::views
