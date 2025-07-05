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

#include "print_preview.hpp"

#include "intl.h"
#include "utils.hpp"
#include "win32_utils.hpp"

namespace ff::dlg {

static const std::string preview_area_css(
    "box { box-shadow: 3pt 3pt 3pt black;}");

// Margin around the page preview area in pixels. Must be lerge enough to
// accomodate the CSS box-shadow.
static const int wrapper_margin = 20;

static bool draw_preview_area(const Cairo::RefPtr<Cairo::Context>& cr) {
    // Dummy red coloring
    cr->set_source_rgb(1.0, 0.0, 0.0);
    cr->paint();

    return true;
}

PrintPreviewWidget::PrintPreviewWidget() : fixed_wrapper(0.5, 0.5, 0.5) {
    if (is_win32_display()) {
        // In Windows the GTK preview tab is embedded in the native Print
        // Dialog. The native dialog is not resizable, and its size can't be
        // queried. We must decide on a reasonable size for print preview
        // widget, and request the allocation in advance.
        Gdk::Rectangle preview_rectangle = get_win32_print_preview_size();
        set_size_request(preview_rectangle.get_width(),
                         preview_rectangle.get_height());
    }

    build_compound_preview_area();
    dummy_label = Gtk::Label("Dummy label");

    attach(fixed_wrapper, 0, 0);
    attach(dummy_label, 1, 0);
    show_all();
}

Glib::ustring PrintPreviewWidget::label() { return _("Preview"); }

void PrintPreviewWidget::draw_page_cb(
    const Glib::RefPtr<Gtk::PrintContext>& context, int page_nr) {
    Cairo::RefPtr<Cairo::Context> cr = context->get_cairo_context();

    // White background
    cr->set_source_rgb(1, 1, 1);
    cr->paint();

    // Print sample text in black
    cr->select_font_face("Sans", Cairo::FontSlant::FONT_SLANT_NORMAL,
                         Cairo::FontWeight::FONT_WEIGHT_NORMAL);
    cr->set_font_size(24.0);
    cr->move_to(100.0, 100.0);
    cr->set_source_rgb(0, 0, 0);
    cr->show_text("Hello World");
}

void PrintPreviewWidget::update(
    const Glib::RefPtr<Gtk::PageSetup>& setup,
    const Glib::RefPtr<Gtk::PrintSettings>& settings) {
    current_setup_ = setup;
    resize_preview_area(setup, fixed_wrapper.get_allocation());
}

void PrintPreviewWidget::build_compound_preview_area() {
    // The preview area contains a page preview with a 3D shadow on a grey
    // background, in a Firefox style. Unfortunately, 3D shadow is difficult to
    // draw manually in Cairo, so we implement it using CSS. This requires a
    // Gtk::Fixed container, on which the preview widget can be placed in a free
    // manner. To support CSS styling, the preview widget is wrapped with
    // Gtk::Box, which provides a CSS node.
    ui_utils::apply_css(box_wrapper, preview_area_css);

    fixed_wrapper.set_hexpand(true);
    fixed_wrapper.set_vexpand(true);
    preview_area.signal_draw().connect(&draw_preview_area);

    // Localize the page-sized preview area inside the allowed space.
    fixed_wrapper.signal_size_allocate().connect(
        [this](Gtk::Allocation& a) { resize_preview_area(current_setup_, a); });

    fixed_wrapper.put(box_wrapper, 0, 0);
    box_wrapper.pack_start(preview_area, true, true);
}

double PrintPreviewWidget::calculate_page_ratio(
    const Glib::RefPtr<Gtk::PageSetup>& setup) {
    // See gtkprintunixdialog.c:draw_page() for default behaviour
    double page_ratio = G_SQRT2;

    if (setup) {
        page_ratio = setup->get_paper_height(Gtk::UNIT_MM) /
                     setup->get_paper_width(Gtk::UNIT_MM);
    }

    return page_ratio;
}

Gtk::Allocation PrintPreviewWidget::calculate_preview_allocation(
    double page_ratio, const Gtk::Allocation& wrapper_size) {
    int available_width = wrapper_size.get_width() - 2 * wrapper_margin;
    int available_height = wrapper_size.get_height() - 2 * wrapper_margin;
    Gtk::Allocation page_rectangle;

    if (available_width < 0 || available_height < 0) {
        return page_rectangle;
    }

    double wrapper_ratio = available_height / (double)available_width;
    if (wrapper_ratio > page_ratio) {
        // total area too high, leaving space above and below the page preview
        // area
        int space_above = (available_height - available_width * page_ratio) / 2;
        page_rectangle =
            Gtk::Allocation(wrapper_margin, wrapper_margin + space_above,
                            available_width, available_width * page_ratio);
    } else {
        // total area too wide, leaving space at the left and at the right of
        // the page preview area
        int space_left = (available_width - available_height / page_ratio) / 2;
        page_rectangle =
            Gtk::Allocation(wrapper_margin + space_left, wrapper_margin,
                            available_height / page_ratio, available_height);
    }

    return page_rectangle;
}

void PrintPreviewWidget::resize_preview_area(
    const Glib::RefPtr<Gtk::PageSetup>& setup,
    const Gtk::Allocation& wrapper_size) {
    double page_ratio = calculate_page_ratio(setup);
    Gtk::Allocation page_rectangle =
        calculate_preview_allocation(page_ratio, wrapper_size);

    // Check if the new size is actually needed. Without this check there would
    // be an inifinite cascade of size requests, as preview_area resizing
    // triggers fixed_wrapper resizing, and vice versa.
    int old_width, old_height;
    preview_area.get_size_request(old_width, old_height);
    if (page_rectangle.get_width() == old_width &&
        page_rectangle.get_height() == old_height) {
        return;
    }

    preview_area.set_size_request(page_rectangle.get_width(),
                                  page_rectangle.get_height());
    fixed_wrapper.move(box_wrapper, page_rectangle.get_x(),
                       page_rectangle.get_y());

    // queue_resize() will not work right inside the slot. We need to defer
    // it as a separate subsequent event.
    Glib::signal_idle().connect_once([this]() {
        preview_area.queue_resize();
        queue_draw();
    });
}

}  // namespace ff::dlg
