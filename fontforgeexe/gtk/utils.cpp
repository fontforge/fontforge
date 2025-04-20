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

#include "utils.hpp"

#include <string>
#include <vector>

Gtk::Widget* gtk_find_child(Gtk::Widget* w, const std::string& name) {
    if (w->get_name() == name) {
        return w;
    }

    Gtk::Widget* res = nullptr;
    Gtk::Container* c = dynamic_cast<Gtk::Container*>(w);

    if (c) {
        std::vector<Gtk::Widget*> children = c->get_children();
        for (size_t i = 0; res == nullptr && i < children.size(); ++i) {
            res = gtk_find_child(children[i], name);
        }
    }
    return res;
}

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

Gtk::Window* gtk_get_window(Gtk::Widget& w) {
    Gtk::Container* top_containter = w.get_toplevel();
    Gtk::Window* window = dynamic_cast<Gtk::Window*>(top_containter);
    return window;
}

int label_offset(Gtk::Widget* w) {
    Gtk::Container* c = dynamic_cast<Gtk::Container*>(w);
    if (c) {
        std::vector<Gtk::Widget*> children = c->get_children();
        if (!children.empty()) {
            Gtk::Widget* label = children[0];
            int x, y;
            if (label->translate_coordinates(*w, 0, 0, x, y)) {
                return x;
            }
        }
    }

    return 0;
}

static Cairo::TextExtents ui_font_extents(const std::string& sample_text) {
    Cairo::RefPtr<Cairo::ImageSurface> srf =
        Cairo::ImageSurface::create(Cairo::Format::FORMAT_RGB24, 100, 100);
    Cairo::RefPtr<Cairo::Context> cairo_context = Cairo::Context::create(srf);
    Glib::RefPtr<Gtk::StyleContext> style_context = Gtk::StyleContext::create();

    Pango::FontDescription font = style_context->get_font();
    cairo_context->select_font_face(font.get_family(),
                                    Cairo::FontSlant::FONT_SLANT_NORMAL,
                                    Cairo::FontWeight::FONT_WEIGHT_NORMAL);
    cairo_context->set_font_size(font.get_size() / PANGO_SCALE *
                                 Gdk::Screen::get_default()->get_resolution() /
                                 96);

    Cairo::TextExtents extents;
    cairo_context->get_text_extents("m", extents);
    return extents;
}

double ui_font_em_size() {
    Cairo::TextExtents extents = ui_font_extents("m");
    return extents.x_advance;
}

double ui_font_eX_size() {
    Cairo::TextExtents extents = ui_font_extents("X");
    return extents.height;
}

Glib::RefPtr<Gdk::Pixbuf> load_icon(const Glib::ustring& icon_name, int size) {
    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();

    // Load icon by name from the theme
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    if (theme->lookup_icon(icon_name, size)) {
        return theme->load_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
    }

    // Use generic sad face for missing icons
    if (theme->lookup_icon("computer-fail-symbolic", size)) {
        return theme->load_icon("computer-fail-symbolic", size,
                                Gtk::ICON_LOOKUP_FORCE_SIZE);
    }

    // Fallback to black square
    static const std::vector<guint8> sq(size * size, 0);
    static Glib::RefPtr<Gdk::Pixbuf> fallback_icon =
        Gdk::Pixbuf::create_from_data(sq.data(), Gdk::COLORSPACE_RGB, false, 8,
                                      size, size, 0);

    return fallback_icon;
}

Gdk::ModifierType gtk_get_keyboard_state() {
    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    GdkKeymap* keymap = display->get_keymap();
    Gdk::ModifierType state =
        (Gdk::ModifierType)gdk_keymap_get_modifier_state(keymap);

    return state;

    // The code below should work in GTK4.
#if 0
   Glib::RefPtr<Gdk::Seat> seat = display->get_default_seat();
   Glib::RefPtr<Gdk::Device> keyboard = seat->get_keyboard();
   return keyboard->get_modifier_state();
#endif
}

Glib::RefPtr<Gdk::Cursor> set_cursor(Gtk::Widget* widget,
                                     const Glib::ustring& name) {
    Glib::RefPtr<Gdk::Window> gdk_window = widget->get_window();
    auto old_cursor = gdk_window->get_cursor();

    Glib::RefPtr<Gdk::Cursor> new_cursor =
        Gdk::Cursor::create(gdk_window->get_display(), name);
    gdk_window->set_cursor(new_cursor);

    return old_cursor;
}

void unset_cursor(Gtk::Widget* widget, Glib::RefPtr<Gdk::Cursor> old_cursor) {
    Glib::RefPtr<Gdk::Window> gdk_window = widget->get_window();
    // old_cursor is allowed to be NULL
    gdk_window->set_cursor(old_cursor);
}

guint32 color_from_gdk_rgba(const Gdk::RGBA& color) {
    auto r = color.get_red_u() / 256;
    auto g = color.get_green_u() / 256;
    auto b = color.get_blue_u() / 256;
    auto a = color.get_alpha_u() / 256;
    return (r << 24) | (g << 16) | (b << 8) | a;
}

Glib::RefPtr<Gdk::Pixbuf> build_color_icon(const Gdk::RGBA& rgba, gint size) {
    Glib::RefPtr<Gdk::Pixbuf> pixbuf =
        Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, size, size);

    // Fill with opaque black color to create the frame
    pixbuf->fill(0x000000ff);

    // Fill the interior with actual color
    guint32 g_color = color_from_gdk_rgba(rgba);
    Gdk::Pixbuf::create_subpixbuf(pixbuf, 1, 1, size - 2, size - 2)
        ->fill(g_color);

    return pixbuf;
}
