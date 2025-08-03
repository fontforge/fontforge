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

// Widget names to be referred by stack children
static const std::string FULL_DISPLAY("full_display");
static const std::string GLYPH_PAGES("glyph_pages");
static const std::string SAMPLE_TEXT("sample_text");

// Margin around the page preview area in pixels. Must be lerge enough to
// accomodate the CSS box-shadow.
static const int wrapper_margin = 20;

bool PrintPreviewWidget::draw_preview_area(
    const Cairo::RefPtr<Cairo::Context>& cr) {
    // Number of preview area pixels in a paper millimeter
    double scale = preview_area.get_allocated_width() /
                   current_setup_->get_paper_width(Gtk::UNIT_MM);
    Cairo::Rectangle printable_area =
        calculate_printable_area(scale, current_setup_, Gtk::UNIT_MM);

    draw_page(cr, printable_area, 0);

    return true;
}

PrintPreviewWidget::PrintPreviewWidget(
    Cairo::RefPtr<Cairo::FtFontFace> cairo_face)
    : fixed_wrapper(0.5, 0.5, 0.5),
      current_setup_(default_setup_),
      cairo_face_(cairo_face) {
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
    Gtk::VBox* controls = Gtk::make_managed<Gtk::VBox>();

    radio_full_display_ =
        Gtk::make_managed<Gtk::RadioButton>(_("_Full Font Display"), true);
    Gtk::RadioButton::Group group = radio_full_display_->get_group();
    radio_glyph_pages_ = Gtk::make_managed<Gtk::RadioButton>(
        group, _("Full Pa_ge Glyphs"), true);
    radio_sample_text_ =
        Gtk::make_managed<Gtk::RadioButton>(group, _("Sample Text"), true);

    radio_full_display_->set_name(FULL_DISPLAY);
    radio_glyph_pages_->set_name(GLYPH_PAGES);
    radio_sample_text_->set_name(SAMPLE_TEXT);

    radio_full_display_->signal_toggled().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::on_display_toggled));
    radio_glyph_pages_->signal_toggled().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::on_display_toggled));
    radio_sample_text_->signal_toggled().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::on_display_toggled));

    Gtk::HBox* size = Gtk::make_managed<Gtk::HBox>();
    Gtk::SpinButton* size_entry = Gtk::make_managed<Gtk::SpinButton>();
    size->pack_start(*Gtk::make_managed<Gtk::Label>(_("Size:")));
    size->pack_start(*size_entry);
    size->pack_start(*Gtk::make_managed<Gtk::Label>(_("points")));

    size_entry->set_width_chars(3);
    size_entry->set_numeric(true);
    size_entry->set_adjustment(Gtk::Adjustment::create(12, 1, 120, 1, 3, 0));
    size->set_halign(Gtk::ALIGN_START);

    Gtk::Entry* sample_text_1line = Gtk::make_managed<Gtk::Entry>();
    sample_text_1line->set_text("Dummy text");

    stack_ = Gtk::make_managed<Gtk::Stack>();
    stack_->add(*size, FULL_DISPLAY);
    stack_->add(*Gtk::make_managed<Gtk::Label>(), GLYPH_PAGES);
    stack_->add(*sample_text_1line, SAMPLE_TEXT);

    controls->pack_start(*radio_full_display_);
    controls->pack_start(*radio_glyph_pages_);
    controls->pack_start(*radio_sample_text_);
    controls->pack_start(*stack_);
    controls->set_valign(Gtk::ALIGN_START);

    attach(fixed_wrapper, 0, 0);
    attach(*controls, 1, 0);
    show_all();
}

Glib::ustring PrintPreviewWidget::label() { return _("Preview"); }

void PrintPreviewWidget::draw_page_cb(
    const Glib::RefPtr<Gtk::PrintContext>& context, int page_nr) {
    Cairo::RefPtr<Cairo::Context> cr = context->get_cairo_context();
    Glib::RefPtr<Gtk::PageSetup> setup = context->get_page_setup();

    Cairo::Rectangle printable_area =
        calculate_printable_area(1.0, setup, Gtk::UNIT_POINTS);

    draw_page(cr, printable_area, page_nr);
}

void PrintPreviewWidget::update(
    const Glib::RefPtr<Gtk::PageSetup>& setup,
    const Glib::RefPtr<Gtk::PrintSettings>& settings) {
    default_setup_ = PrintPreviewWidget::create_default_setup();
    current_setup_ = setup ? setup : default_setup_;
    resize_preview_area(fixed_wrapper.get_allocation());
}

Glib::RefPtr<Gtk::PageSetup> PrintPreviewWidget::create_default_setup() {
    Glib::RefPtr<Gtk::PageSetup> A4_setup = Gtk::PageSetup::create();

    static const double A4_width = 210.0;   // mm
    static const double A4_height = 297.0;  // mm
    static const double A4_margin = 6.0;    // mm
    Gtk::PaperSize A4_size("A4", "A4", A4_width, A4_height, Gtk::UNIT_MM);

    A4_setup->set_paper_size(A4_size);
    A4_setup->set_top_margin(A4_margin, Gtk::UNIT_MM);
    A4_setup->set_bottom_margin(A4_margin, Gtk::UNIT_MM);
    A4_setup->set_left_margin(A4_margin, Gtk::UNIT_MM);
    A4_setup->set_right_margin(A4_margin, Gtk::UNIT_MM);

    return A4_setup;
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
    preview_area.signal_draw().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::draw_preview_area));
    // Localize the page-sized preview area inside the allowed space.
    fixed_wrapper.signal_size_allocate().connect(
        [this](Gtk::Allocation& a) { resize_preview_area(a); });

    fixed_wrapper.put(box_wrapper, 0, 0);
    box_wrapper.pack_start(preview_area, true, true);
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
    const Gtk::Allocation& wrapper_size) {
    double page_ratio = page_ratio =
        current_setup_->get_paper_height(Gtk::UNIT_MM) /
        current_setup_->get_paper_width(Gtk::UNIT_MM);

    Gtk::Allocation page_rectangle =
        calculate_preview_allocation(page_ratio, wrapper_size);

    // Check if the new size is actually needed. Without this check there would
    // be an inifinite cascade of size requests, as preview_area resizing
    // triggers fixed_wrapper resizing, and vice versa.
    if (page_rectangle == last_preview_allocation_) {
        return;
    }

    last_preview_allocation_ = page_rectangle;
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

Cairo::Rectangle PrintPreviewWidget::calculate_printable_area(
    double scale, const Glib::RefPtr<Gtk::PageSetup>& setup, Gtk::Unit unit) {
    Cairo::Rectangle printable_area;

    printable_area.x = scale * setup->get_left_margin(unit);
    printable_area.y = scale * setup->get_top_margin(unit);
    printable_area.width =
        scale * (setup->get_paper_width(unit) - setup->get_left_margin(unit) -
                 setup->get_right_margin(unit));
    printable_area.height =
        scale * (setup->get_paper_height(unit) - setup->get_top_margin(unit) -
                 setup->get_bottom_margin(unit));

    return printable_area;
}

void PrintPreviewWidget::draw_page(const Cairo::RefPtr<Cairo::Context>& cr,
                                   const Cairo::Rectangle& printable_area,
                                   int page_nr) {
    cr->translate(printable_area.x, printable_area.y);
    cr->scale(printable_area.width / 100, printable_area.width / 100);

    // White background
    cr->set_source_rgb(1, 1, 1);
    cr->paint();

    // Print sample text in black
    cr->set_font_face(cairo_face_);
    cr->set_font_size(24.0);
    cr->move_to(50.0, 50.0);
    cr->set_source_rgb(0, 0, 0);
    cr->show_text("Hello World");

    // Print horizontal reference mark in yellow
    cr->set_source_rgb(1.0, 1.0, 0.0);
    cr->rectangle(0, 0, 100, 10);
    cr->fill();
    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->rectangle(99, 0, 1, 10);
    cr->fill();

    // Print vertical reference mark in red
    cr->set_source_rgb(1.0, 0.0, 0.0);
    cr->rectangle(0, 0, 10, 100 * printable_area.height / printable_area.width);
    cr->fill();
    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->rectangle(0, 100 * printable_area.height / printable_area.width - 1, 10,
                  1);
    cr->fill();
}

void PrintPreviewWidget::on_display_toggled() {
    for (auto r :
         {radio_full_display_, radio_glyph_pages_, radio_sample_text_}) {
        if (r->get_active()) {
            stack_->set_visible_child(r->get_name());
        }
    }
}

}  // namespace ff::dlg
