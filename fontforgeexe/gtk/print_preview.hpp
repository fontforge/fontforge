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
#pragma once

#include <gtkmm.h>

#include "cairo_painter.hpp"
#include "widgets/aspect_frame_bg.hpp"

namespace ff::dlg {

class PrintPreviewWidget : public Gtk::Grid {
 public:
    PrintPreviewWidget(const utils::CairoPainter& cairo_painter);

    static Glib::ustring label();

    // This slot is provided to Gtk::PrintOperation
    void draw_page_cb(const Glib::RefPtr<Gtk::PrintContext>& context,
                      int page_nr);

    void update(const Glib::RefPtr<Gtk::PageSetup>& setup,
                const Glib::RefPtr<Gtk::PrintSettings>& settings);

 private:
    // Create the default A4-based page setup
    static Glib::RefPtr<Gtk::PageSetup> create_default_setup();

    // Build and initialize the preview area widgets
    void build_compound_preview_area();

    Gdk::Point calculate_text_popover_size();

    // The optimal popover size and attachment rectangle depend on the size of
    // the print dialog, and they are recalculated every time the popover is
    // shown.
    void reconfigure_text_popover(Gtk::Popover* popover);

    // Build and initialize the sample text popover
    void build_sample_text_popover(Gtk::Widget* parent_widget);

    // Calculate printable area: (x, y) are positive left/top margin offsets
    // from the upper left corner of the actual canvas - either paper or
    // preview_area. The prinable area is located within the page margins.
    static Cairo::Rectangle calculate_printable_area(
        double scale, const Glib::RefPtr<Gtk::PageSetup>& setup,
        Gtk::Unit unit);

    // Draw the entire printable page.
    void draw_page(const Cairo::RefPtr<Cairo::Context>& cr, double scale,
                   const Cairo::Rectangle& printable_area, int page_nr);

    bool draw_preview_area(const Cairo::RefPtr<Cairo::Context>& cr);

    void on_display_toggled();

    // Containers for compound preview area
    widget::AspectFrameWithBackground aspect_wrapper;
    Gtk::DrawingArea preview_area;

    // Preview controls
    Gtk::RadioButton* radio_full_display_;
    Gtk::RadioButton* radio_glyph_pages_;
    Gtk::RadioButton* radio_sample_text_;

    Gtk::SpinButton* size_entry_;
    Gtk::Entry* sample_text_oneliner_;
    Gtk::Stack* stack_;

    Gtk::TextView* sample_text_;

    // The default A4-based setup is used for preview when no printer has been
    // selected yet.
    Glib::RefPtr<Gtk::PageSetup> default_setup_;

    // This setup should never be nullptr, all drawing functions rely on it.
    Glib::RefPtr<Gtk::PageSetup> current_setup_;

    utils::CairoPainter cairo_painter_;
};

}  // namespace ff::dlg
