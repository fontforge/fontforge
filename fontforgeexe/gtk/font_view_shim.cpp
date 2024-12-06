/* Copyright 2023 Maxim Iorsh <iorsh@users.sourceforge.net>
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
