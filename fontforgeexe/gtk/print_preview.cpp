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
static const std::string MULTI_SIZE("multi_size");
static const std::string SAMPLE_TEXT("sample_text");

// Margin around the page preview area in pixels. Must be lerge enough to
// accomodate the CSS box-shadow.
static const int wrapper_margin = 20;

PrintPreviewWidget::PrintPreviewWidget(const utils::CairoPainter& cairo_painter)
    : aspect_wrapper(0.5, 0.5, 0.5),
      current_setup_(default_setup_),
      cairo_painter_(cairo_painter) {
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
    radio_multi_size_ = Gtk::make_managed<Gtk::RadioButton>(
        group, _("_Multi Size Glyphs"), true);
    radio_sample_text_ =
        Gtk::make_managed<Gtk::RadioButton>(group, _("Sample Text"), true);

    radio_full_display_->set_name(FULL_DISPLAY);
    radio_glyph_pages_->set_name(GLYPH_PAGES);
    radio_multi_size_->set_name(MULTI_SIZE);
    radio_sample_text_->set_name(SAMPLE_TEXT);

    radio_full_display_->signal_toggled().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::on_display_toggled));
    radio_glyph_pages_->signal_toggled().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::on_display_toggled));
    radio_multi_size_->signal_toggled().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::on_display_toggled));
    radio_sample_text_->signal_toggled().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::on_display_toggled));

    // Size of full-display glyphs
    Gtk::HBox* size = Gtk::make_managed<Gtk::HBox>();
    size_entry_ = Gtk::make_managed<Gtk::SpinButton>();
    size->pack_start(*Gtk::make_managed<Gtk::Label>(_("Size:")));
    size->pack_start(*size_entry_);
    size->pack_start(*Gtk::make_managed<Gtk::Label>(_("points")));

    size_entry_->set_width_chars(3);
    size_entry_->set_numeric(true);
    size_entry_->set_adjustment(Gtk::Adjustment::create(20, 1, 120, 1, 3, 0));
    size->set_halign(Gtk::ALIGN_START);
    size_entry_->signal_value_changed().connect(
        [this] { preview_area.queue_draw(); });

    scaling_option_ = Gtk::make_managed<Gtk::ComboBoxText>();
    scaling_option_->append(utils::CairoPainter::kScaleToPage,
                            "Scale glyphs to page size");
    scaling_option_->append(utils::CairoPainter::kScaleEmSize,
                            "Scale glyphs to em size");
    scaling_option_->append(utils::CairoPainter::kScaleToPage,
                            "Scale glyphs to maximum height");
    scaling_option_->set_active_id(utils::CairoPainter::kScaleToPage);

    // One-liner preview of the sample text popup contents
    sample_text_oneliner_ = Gtk::make_managed<Gtk::Entry>();
    sample_text_oneliner_->set_editable(false);
    sample_text_oneliner_->set_can_focus(false);

    // Make the one-liner preview sensitive to mouse click, which would bring up
    // the sample text popup.
    Gtk::EventBox* oneliner_event_box = Gtk::make_managed<Gtk::EventBox>();
    oneliner_event_box->add(*sample_text_oneliner_);
    oneliner_event_box->set_above_child(true);

    build_sample_text_popover(oneliner_event_box);

    stack_ = Gtk::make_managed<Gtk::Stack>();
    stack_->add(*size, FULL_DISPLAY);
    stack_->add(*scaling_option_, GLYPH_PAGES);
    stack_->add(*Gtk::make_managed<Gtk::Label>(), MULTI_SIZE);
    stack_->add(*oneliner_event_box, SAMPLE_TEXT);

    controls->pack_start(*radio_full_display_);
    controls->pack_start(*radio_glyph_pages_);
    controls->pack_start(*radio_multi_size_);
    controls->pack_start(*radio_sample_text_);
    controls->pack_start(*stack_);
    controls->set_valign(Gtk::ALIGN_START);

    attach(aspect_wrapper, 0, 0);
    attach(*controls, 1, 0);
    show_all();
}

Glib::ustring PrintPreviewWidget::label() { return _("Preview"); }

void PrintPreviewWidget::draw_page_cb(
    const Glib::RefPtr<Gtk::PrintContext>& context, int page_nr) {
    Cairo::RefPtr<Cairo::Context> cr = context->get_cairo_context();
    Glib::RefPtr<Gtk::PageSetup> setup = context->get_page_setup();

    // The physical paper page is measured in points (1/72 in). The Cairo
    // context provided by Gtk::PrintOperation already accounts for the physical
    // size, and we don't need to scale it any further. Accordingly, all
    // dimensions should be passed to Cairo::Context in points.
    Cairo::Rectangle printable_area =
        calculate_printable_area(setup, Gtk::UNIT_POINTS);

    draw_page(cr, printable_area, page_nr);
}

void PrintPreviewWidget::update_page_setup(
    const Glib::RefPtr<Gtk::PageSetup>& setup,
    const Glib::RefPtr<Gtk::PrintSettings>& settings) {
    default_setup_ = PrintPreviewWidget::create_default_setup();
    current_setup_ = setup ? setup : default_setup_;

    double page_ratio = page_ratio =
        current_setup_->get_paper_width(Gtk::UNIT_MM) /
        current_setup_->get_paper_height(Gtk::UNIT_MM);

    aspect_wrapper.set(Gtk::ALIGN_CENTER, Gtk::ALIGN_CENTER, page_ratio, false);
    preview_area.queue_draw();
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
    // Gtk::AspectFrame container, which locks the aspect ratio of the preview
    // widget. To support CSS styling, the preview widget is wrapped with
    // Gtk::Box, which provides a CSS node.
    Gtk::Box* box_wrapper = Gtk::make_managed<Gtk::Box>();
    Gtk::Overlay* overlay = Gtk::make_managed<Gtk::Overlay>();
    ui_utils::apply_css(*box_wrapper, preview_area_css);
    box_wrapper->set_margin_start(wrapper_margin);
    box_wrapper->set_margin_end(wrapper_margin);
    box_wrapper->set_margin_top(0);  // Gtk::Frame already boosts the top margin
    box_wrapper->set_margin_bottom(wrapper_margin);

    page_counter_ = Gtk::Scale(Gtk::ORIENTATION_HORIZONTAL);
    page_counter_.set_valign(Gtk::ALIGN_END);
    page_counter_.set_round_digits(0);
    page_counter_.signal_format_value().connect([this](double page_num) {
        char buffer[32];
        int total_pages = (int)page_counter_.get_adjustment()->get_upper() - 1;
        sprintf(buffer, "Page %d of %d", (int)page_num, total_pages);
        return Glib::ustring(buffer);
    });

    aspect_wrapper.set_hexpand(true);
    aspect_wrapper.set_vexpand(true);
    aspect_wrapper.set_shadow_type(Gtk::SHADOW_NONE);

    preview_area.signal_draw().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::draw_preview_area));

    overlay->add(preview_area);
    overlay->add_overlay(page_counter_);
    box_wrapper->pack_start(*overlay, true, true);
    aspect_wrapper.add(*box_wrapper);
}

Gdk::Point PrintPreviewWidget::calculate_text_popover_size() {
    // Maximum width: controls_column + half of preview area.
    int width = sample_text_oneliner_->get_allocated_width() +
                aspect_wrapper.get_allocated_width() / 2;

    // Maximum height: the gap between the parent widget and the bottom of the
    // entire preview widget.
    int x, y;
    sample_text_oneliner_->translate_coordinates(
        *this, 0, sample_text_oneliner_->get_allocated_height(), x, y);
    int height = aspect_wrapper.get_allocated_height() - y;

    return Gdk::Point(width, height);
}

void PrintPreviewWidget::reconfigure_text_popover(Gtk::Popover* text_popover) {
    Gtk::Widget* parent_widget = text_popover->get_relative_to();

    // The scroller limits the text popover window. Without limitations the
    // TextView widget can expand until it covers the entire print dialog and
    // get unsigtly cut-off at the dialog window borders.
    widget::RichTechEditor* rich_text =
        dynamic_cast<widget::RichTechEditor*>(text_popover->get_child());
    Gtk::ScrolledWindow& scrolled = rich_text->get_scrolled();

    Gdk::Point size = calculate_text_popover_size();
    text_popover->set_size_request(size.get_x(), size.get_y());
    text_popover->queue_resize();

    text_popover->set_pointing_to({0, 0, parent_widget->get_allocated_width(),
                                   parent_widget->get_allocated_height()});

    text_popover->show_all();
    text_popover->popup();
}

void PrintPreviewWidget::build_sample_text_popover(Gtk::Widget* parent_widget) {
    Gtk::Popover* text_popover = Gtk::make_managed<Gtk::Popover>();
    text_popover->set_relative_to(*parent_widget);
    text_popover->set_position(Gtk::POS_BOTTOM);
    text_popover->set_modal(true);
    text_popover->set_constrain_to(Gtk::POPOVER_CONSTRAINT_WINDOW);

    sample_text_ = Gtk::make_managed<widget::RichTechEditor>();
    sample_text_->set_hexpand();
    sample_text_->set_vexpand();
    sample_text_->get_buffer()->set_text("Sample text\nSecond sample line.");
    sample_text_->get_buffer()->signal_changed().connect([this] {
        preview_area.queue_draw();
        sample_text_oneliner_->set_text(sample_text_->get_buffer()->get_text());
    });
    sample_text_->get_buffer()->signal_apply_tag().connect(
        [this](const Glib::RefPtr<Gtk::TextBuffer::Tag>&,
               const Gtk::TextBuffer::iterator&,
               const Gtk::TextBuffer::iterator&) {
            preview_area.queue_draw();
        });
    sample_text_->get_buffer()->signal_remove_tag().connect(
        [this](const Glib::RefPtr<Gtk::TextBuffer::Tag>&,
               const Gtk::TextBuffer::iterator&,
               const Gtk::TextBuffer::iterator&) {
            preview_area.queue_draw();
        });

    sample_text_oneliner_->set_text(sample_text_->get_buffer()->get_text());

    parent_widget->signal_button_press_event().connect(
        [text_popover, this](GdkEventButton*) {
            reconfigure_text_popover(text_popover);
            return true;
        });

    // Hide the popover when the window is resized. This would allow the popover
    // to recumpute its optimal size when it is shown again.
    signal_size_allocate().connect(
        [text_popover](Gtk::Allocation&) { text_popover->popdown(); });

    // Set up the popover structure
    text_popover->add(*sample_text_);
}

Cairo::Rectangle PrintPreviewWidget::calculate_printable_area(
    const Glib::RefPtr<Gtk::PageSetup>& setup, Gtk::Unit unit) {
    Cairo::Rectangle printable_area;

    printable_area.x = setup->get_left_margin(unit);
    printable_area.y = setup->get_top_margin(unit);
    printable_area.width = setup->get_paper_width(unit) -
                           setup->get_left_margin(unit) -
                           setup->get_right_margin(unit);
    printable_area.height = setup->get_paper_height(unit) -
                            setup->get_top_margin(unit) -
                            setup->get_bottom_margin(unit);

    return printable_area;
}

void PrintPreviewWidget::draw_page(const Cairo::RefPtr<Cairo::Context>& cr,
                                   const Cairo::Rectangle& printable_area,
                                   int page_nr) {
    double font_size = size_entry_->get_value();

    gsize length = 0;
    char* out_buffer = (char*)sample_text_->get_buffer()->serialize(
        sample_text_->get_buffer(), widget::RichTechEditor::rich_text_mime_type,
        sample_text_->get_buffer()->begin(), sample_text_->get_buffer()->end(),
        length);

    if (radio_full_display_->get_active()) {
        cairo_painter_.draw_page_full_display(cr, printable_area, page_nr,
                                              font_size);
    } else if (radio_glyph_pages_->get_active()) {
        Glib::ustring active_option = scaling_option_->get_active_id();
        cairo_painter_.draw_page_full_glyph(cr, printable_area, page_nr,
                                            active_option);
    } else if (radio_sample_text_->get_active()) {
        cairo_painter_.draw_page_sample_text(cr, printable_area, page_nr,
                                             out_buffer);
    } else {
        cairo_painter_.draw_page_multisize(cr, printable_area, page_nr);
    }

    paginate();
}

size_t PrintPreviewWidget::paginate() {
    size_t num_pages = 1;
    if (radio_glyph_pages_->get_active()) {
        num_pages = cairo_painter_.page_count_full_glyph();
    } else if (radio_sample_text_->get_active()) {
        num_pages = cairo_painter_.page_count_sample_text();
    }

    // Changes to scale lead to focus changes which may inadvertently close the
    // sample text popover. When the popover is visible, we shall avoid touching
    // the scale. It would be updated automatically after the popover closes and
    // the preview area refreshes.
    if (!sample_text_->is_visible()) {
        size_t old_num_pages = page_counter_.get_adjustment()->get_upper() - 1;
        size_t new_value =
            std::clamp((size_t)page_counter_.get_value(), (size_t)1, num_pages);

        // We must reconfigure the scale sparingly to avoid infinite loop of
        // redrawing events.
        if (num_pages != old_num_pages) {
            page_counter_.get_adjustment()->configure(new_value, 1,
                                                      num_pages + 1, 1, 1, 1);
        }

        page_counter_.set_visible(num_pages > 1);
    }
    return num_pages;
}

bool PrintPreviewWidget::draw_preview_area(
    const Cairo::RefPtr<Cairo::Context>& cr) {
    Cairo::Rectangle printable_area =
        calculate_printable_area(current_setup_, Gtk::UNIT_POINTS);
    size_t page_nr = (size_t)page_counter_.get_value() - 1;

    // Number of preview area pixels in a paper pt (1 pt = 1/72 in)
    double scale = preview_area.get_allocated_width() /
                   current_setup_->get_paper_width(Gtk::UNIT_POINTS);
    // We are scaling the Cairo::Context according to the preview area pixel
    // size. From this point on, all measurements and font sizes passed to the
    // Cairo::Context correspond to the physical paper size.
    cr->scale(scale, scale);

    draw_page(cr, printable_area, page_nr);

    return true;
}

void PrintPreviewWidget::on_display_toggled() {
    for (auto r : {radio_full_display_, radio_glyph_pages_, radio_multi_size_,
                   radio_sample_text_}) {
        if (r->get_active()) {
            stack_->set_visible_child(r->get_name());
        }
    }
    preview_area.queue_draw();
}

}  // namespace ff::dlg
