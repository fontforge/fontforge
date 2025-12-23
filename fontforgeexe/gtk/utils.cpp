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

#include <glib/gprintf.h>
#include <iostream>

namespace ff::ui_utils {

static Cairo::TextExtents ui_font_extents(const std::string& sample_text) {
    Cairo::RefPtr<Cairo::ImageSurface> srf =
        Cairo::ImageSurface::create(Cairo::Format::FORMAT_RGB24, 100, 100);
    Cairo::RefPtr<Cairo::Context> cairo_context = Cairo::Context::create(srf);
    Glib::RefPtr<Gtk::StyleContext> style_context = Gtk::StyleContext::create();

    Pango::FontDescription font = style_context->get_font();
    Cairo::RefPtr<Cairo::ToyFontFace> toy_face = Cairo::ToyFontFace::create(
        font.get_family(), Cairo::FontSlant::FONT_SLANT_NORMAL,
        Cairo::FontWeight::FONT_WEIGHT_NORMAL);
    cairo_context->set_font_face(toy_face);
    cairo_context->set_font_size(font.get_size() / PANGO_SCALE *
                                 Gdk::Screen::get_default()->get_resolution() /
                                 72);

    Cairo::TextExtents extents;
    cairo_context->get_text_extents(sample_text, extents);
    toy_face->unreference();  // Prevent memory leak
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

double get_current_ppi(Gtk::Widget* w) {
    Glib::RefPtr<const Gdk::Display> display = w->get_display();
    Glib::RefPtr<Gdk::Window> window = w->get_window();
    Glib::RefPtr<const Gdk::Monitor> monitor =
        display->get_monitor_at_window(window);

    Gdk::Rectangle monitor_geom;
    monitor->get_geometry(monitor_geom);
    int monitor_width_mm = monitor->get_width_mm();
    int monitor_width_px = monitor_geom.get_width();

    double ppi = 25.4 * monitor_width_px / monitor_width_mm;
    return ppi;
}

void post_error(const char* title, const char* statement, ...) {
    va_list ap;
    va_start(ap, statement);

    // Format error statement
    gchar* result_string = NULL;
    gint chars_written = g_vasprintf(&result_string, statement, ap);
    if (chars_written >= 0 && result_string != NULL) {
        // Passing use_markup=false causes GTK to embolden the text, and we
        // don't want it.
        Gtk::MessageDialog message_dlg(result_string, true, Gtk::MESSAGE_ERROR,
                                       Gtk::BUTTONS_OK, true);
        message_dlg.set_title(title);
        message_dlg.run();
        g_free(result_string);
    } else {
        std::cerr << "Error formatting statement \"" << statement << "\""
                  << std::endl;
    }

    va_end(ap);
}

}  // namespace ff::ui_utils
