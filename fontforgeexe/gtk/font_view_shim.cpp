/* Copyright 2023 Maxim Iorsh <iorsh@users.sourceforge.net>
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
#include "font_view_shim.hpp"
#include "utils.hpp"

#include <gtkmm.h>

void* create_font_view(int width, int height) {
    static auto app = Gtk::Application::create("org.fontforge");
    Gtk::Window* font_view_window = new Gtk::Window();
    font_view_window->set_default_size(width, height);

    Gtk::DrawingArea* drawing_area = new Gtk::DrawingArea();
    drawing_area->set_name("CharGrid");
    font_view_window->add(*drawing_area);

    font_view_window->show_all();

    return font_view_window;
}

void gtk_set_title(void* window, char* window_title, char* taskbar_title) {
    Gtk::Window* gtk_window = static_cast<Gtk::Window*>(window);

    if (gtk_window != nullptr) {
        gtk_window->set_title(window_title);
    }
}

GtkWidget* get_drawing_widget_c(void* window) {
    Gtk::Window* font_view_window = static_cast<Gtk::Window*>(window);
    Gtk::Widget* drawing_area = gtk_find_child(font_view_window, "CharGrid");

    return (GtkWidget*)drawing_area->gobj();
}
