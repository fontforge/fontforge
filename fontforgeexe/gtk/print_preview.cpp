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
#include "l10n_text.hpp"
#include "utils.hpp"
#include "win32_utils.hpp"

extern "C" {
extern uint64_t* SFScriptsLangs(SplineFont* sf);
extern uint32_t* SFFeaturesInScriptLang(SplineFont* sf, int gposgsub,
                                        uint32_t script, uint32_t lang);
}

std::map<ff::Tag, L10nText> ScriptLabelsMap();
std::map<ff::Tag, L10nText> LanguageLabelsMap();

namespace ff::dlg {

static const std::string preview_area_css(
    "box { box-shadow: 3pt 3pt 3pt black;}");

// Widget names to be referred by stack children
static const std::string FULL_DISPLAY("full_display");
static const std::string GLYPH_PAGES("glyph_pages");
static const std::string MULTI_SIZE("multi_size");
static const std::string SAMPLE_TEXT("sample_text");

static const std::vector<double> kMultiPointsizes{
    72, 48, 36, 24,  20, 18,  16, 15,  14, 13,  12,  11,
    10, 9,  8,  7.5, 7,  6.5, 6,  5.5, 5,  4.5, 4.2, 4};

// Margin around the page preview area in pixels. Must be lerge enough to
// accomodate the CSS box-shadow.
static const int wrapper_margin = 20;

// Preserve the dialog state between invocations
static struct {
    std::string radio_selection = FULL_DISPLAY;
    size_t full_display_size = 20;
    std::string full_page_scaling = utils::FullGlyphPrinter::kScaleToPage;
    std::string sample_text = "";
} persistent;

static std::pair<ff::Tag, ff::Tag> extract_script_lang_tags(
    const Glib::ustring& entry_text) {
    ff::Tag script("DFLT");
    ff::Tag lang("dflt");

    if (entry_text.size() >= 10) {
        const char* tag_id = entry_text.c_str();
        script = ff::Tag(tag_id);
        lang = ff::Tag(tag_id + 5);
    }

    return {script, lang};
}

PrintPreviewWidget::PrintPreviewWidget(utils::CairoPainter&& cairo_painter,
                                       const std::string& sample_text)
    : aspect_wrapper(0.5, 0.5, 0.5),
      current_setup_(default_setup_),
      cairo_painter_(std::move(cairo_painter)) {
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
    radio_full_display_->set_active(persistent.radio_selection == FULL_DISPLAY);
    radio_glyph_pages_->set_name(GLYPH_PAGES);
    radio_glyph_pages_->set_active(persistent.radio_selection == GLYPH_PAGES);
    radio_multi_size_->set_name(MULTI_SIZE);
    radio_multi_size_->set_active(persistent.radio_selection == MULTI_SIZE);
    radio_sample_text_->set_name(SAMPLE_TEXT);
    radio_sample_text_->set_active(persistent.radio_selection == SAMPLE_TEXT);

    radio_full_display_->set_tooltip_text(
        _("Displays all the glyphs in the font on a rectangular grid at the "
          "given point size"));
    radio_glyph_pages_->set_tooltip_text(
        _("Displays all the selected characters, each on its own page, at an "
          "extremely large point size"));
    radio_multi_size_->set_tooltip_text(
        _("Displays all the selected characters, at several different point "
          "sizes"));
    radio_sample_text_->set_tooltip_text(
        _("Displays a text sample using the current font and, if available, "
          "other fonts from the same family."));

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
    size_entry_->set_adjustment(
        Gtk::Adjustment::create(persistent.full_display_size, 1, 120, 1, 3, 0));
    size->set_halign(Gtk::ALIGN_START);
    size->set_valign(Gtk::ALIGN_START);
    size_entry_->signal_value_changed().connect(
        [this] { preview_area.queue_draw(); });

    scaling_option_ = Gtk::make_managed<Gtk::ComboBoxText>();
    scaling_option_->append(utils::FullGlyphPrinter::kScaleToPage,
                            "Scale each glyph to page size");
    scaling_option_->append(utils::FullGlyphPrinter::kScaleEmSize,
                            "Scale glyphs to em size");
    scaling_option_->append(utils::FullGlyphPrinter::kScaleMaxHeight,
                            "Scale glyphs to maximum height");
    scaling_option_->set_active_id(persistent.full_page_scaling);
    scaling_option_->set_valign(Gtk::ALIGN_START);

    Gtk::VBox* sample_text_controls = build_sample_text_controls();
    if (persistent.sample_text.empty())
        sample_text_->get_buffer()->set_text(sample_text);
    else {
        std::istringstream istream(persistent.sample_text);
        sample_text_->load_buffer(istream);
    }

    stack_ = Gtk::make_managed<Gtk::Stack>();
    stack_->set_vhomogeneous(false);
    stack_->add(*size, FULL_DISPLAY);
    stack_->add(*scaling_option_, GLYPH_PAGES);
    stack_->add(*Gtk::make_managed<Gtk::Label>(), MULTI_SIZE);
    stack_->add(*sample_text_controls, SAMPLE_TEXT);

    controls->pack_start(*radio_full_display_, Gtk::PACK_SHRINK);
    controls->pack_start(*radio_glyph_pages_, Gtk::PACK_SHRINK);
    controls->pack_start(*radio_multi_size_, Gtk::PACK_SHRINK);
    controls->pack_start(*radio_sample_text_, Gtk::PACK_SHRINK);
    controls->pack_start(*stack_, Gtk::PACK_EXPAND_WIDGET);
    controls->set_vexpand(true);

    attach(aspect_wrapper, 0, 0);
    attach(*controls, 1, 0);
    show_all();
}

void PrintPreviewWidget::populate_script_lang_combo() {
    // Show the id (tag code) in the text entry, not the display label.
    script_lang_combo_->signal_changed().connect([this] {
        Glib::ustring active_id = script_lang_combo_->get_active_id();
        if (!active_id.empty())
            script_lang_combo_->get_entry()->set_text(active_id);
    });
    // The default language must always be available, to provide automatic
    // shaping regardless of the presence of OpenType features.
    script_lang_combo_->append("DFLT{dflt}", _("Automatic shaping"));
    script_lang_combo_->set_active(0);
    script_lang_combo_->set_tooltip_text(
        "Select script and language for OpenType features.");

    SplineFont* sf = cairo_painter_.default_rec().sf;

    auto script_labels = ScriptLabelsMap();
    auto lang_labels = LanguageLabelsMap();

    uint64_t* scriptlangs = SFScriptsLangs(sf);
    if (scriptlangs == nullptr) return;

    for (int i = 0; scriptlangs[i] != 0; ++i) {
        ff::Tag script_tag((uint32_t)(scriptlangs[i] >> 32));
        ff::Tag lang_tag((uint32_t)(scriptlangs[i] & 0xFFFFFFFF));
        if (script_tag == "DFLT" && lang_tag == "dflt") continue;

        auto sit = script_labels.find(script_tag);
        auto lit = lang_labels.find(lang_tag);

        Glib::ustring script_name = (sit != script_labels.end())
                                        ? (Glib::ustring)sit->second
                                        : (const char*)script_tag;
        Glib::ustring lang_name = (lit != lang_labels.end())
                                      ? (Glib::ustring)lit->second
                                      : (const char*)lang_tag;

        std::string id = std::string((const char*)script_tag) + "{" +
                         std::string((const char*)lang_tag) + "}";
        Glib::ustring title = script_name + "{" + lang_name + "}";

        script_lang_combo_->append(id, title);
    }

    free(scriptlangs);
}

void PrintPreviewWidget::refresh_feature_tags_list() {
    if (feature_tags_list_ == nullptr) return;

    feature_tags_list_->clear_items();

    const utils::CairoFontRec& font_rec = cairo_painter_.default_rec();
    Glib::ustring entry_text = script_lang_combo_->get_entry()->get_text();
    auto [script, lang] = extract_script_lang_tags(entry_text);

    uint32_t* features = SFFeaturesInScriptLang(font_rec.sf, -2, script, lang);
    std::set<Tag> default_features =
        font_rec.shaper->default_features(script, lang, false);
    if (features != nullptr) {
        for (int i = 0; features[i] != 0; ++i) {
            ff::Tag feat_tag(features[i]);
            feature_tags_list_->append((const char*)feat_tag);
            if (default_features.count(feat_tag) != 0) {
                feature_tags_list_->get_selection()->select(
                    Gtk::TreePath(std::to_string(i)));
            }
        }
        free(features);
    }
}

Gtk::Widget* PrintPreviewWidget::build_opentype_controls() {
    script_lang_combo_ = Gtk::make_managed<Gtk::ComboBoxText>(true);
    populate_script_lang_combo();

    feature_tags_list_ =
        Gtk::make_managed<Gtk::ListViewText>(1, false, Gtk::SELECTION_MULTIPLE);
    feature_tags_list_->set_headers_visible(false);
    feature_tags_list_->set_enable_search(false);
    feature_tags_list_->set_tooltip_text("Select active OpenType features.");
    refresh_feature_tags_list();

    script_lang_combo_->signal_changed().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::refresh_feature_tags_list));

    feature_tags_list_->get_selection()->signal_changed().connect(
        sigc::mem_fun(preview_area, &Gtk::DrawingArea::queue_draw));

    // Wrap feature list in a scrolled window
    Gtk::ScrolledWindow* feature_tags_scroll =
        Gtk::make_managed<Gtk::ScrolledWindow>();
    feature_tags_scroll->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    // TODO(iorsh): make expandable feature tag list.
    feature_tags_scroll->set_size_request(-1, 120);
    feature_tags_scroll->add(*feature_tags_list_);

    Gtk::VBox* opentype_controls = Gtk::make_managed<Gtk::VBox>();
    opentype_controls->pack_start(*script_lang_combo_, false, false);
    opentype_controls->pack_start(*feature_tags_scroll, true, true);
    opentype_controls->set_border_width(0.5 * ui_utils::ui_font_em_size());

    Gtk::Frame* opentype_frame =
        Gtk::make_managed<Gtk::Frame>(_("OpenType features"));
    opentype_frame->add(*opentype_controls);
    opentype_frame->set_border_width(0.5 * ui_utils::ui_font_em_size());
    opentype_frame->set_label_align(0.2);

    return opentype_frame;
}

void PrintPreviewWidget::build_sample_text_editor() {
    sample_text_ = Gtk::make_managed<widget::RichTechEditor>(kMultiPointsizes);
    sample_text_->set_hexpand();
    sample_text_->set_vexpand();
    sample_text_->get_buffer()->signal_changed().connect([this] {
        preview_area.queue_draw();
        if (sample_text_oneliner_ != nullptr)
            sample_text_oneliner_->set_text(
                sample_text_->get_buffer()->get_text());
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

    bool enable_italic =
        cairo_painter_.family_has_multiple(&SplineFontProperties::italic);
    bool enable_stretch =
        cairo_painter_.family_has_multiple(&SplineFontProperties::os2_width);
    bool enable_weight =
        cairo_painter_.family_has_multiple(&SplineFontProperties::os2_weight);
    sample_text_->configure(enable_weight, enable_italic, enable_stretch,
                            enable_weight);
}

Gtk::VBox* PrintPreviewWidget::build_sample_text_controls() {
    Gtk::Widget* sample_text_holder = nullptr;

    // In Windows the GTK preview tab is embedded in the native Print Dialog.
    // Advanced UI controls such as popovers or even descendant modal windows
    // don't work well when the parent has no top-lever Gtk::Window object. Thus
    // we simplify the UI and put the sample text editor directly in the Preview
    // tab.
    // TODO: Revise this approach when upgrading to GTK4.
    if (is_win32_display()) {
        build_sample_text_editor();
        sample_text_holder = sample_text_;
        sample_text_->set_hexpand(false);
    } else {
        // One-liner preview of the sample text popup contents
        sample_text_oneliner_ = Gtk::make_managed<Gtk::Entry>();
        sample_text_oneliner_->set_editable(false);
        sample_text_oneliner_->set_can_focus(false);

        // Make the one-liner preview sensitive to mouse click, which would
        // bring up the sample text popup.
        Gtk::EventBox* oneliner_event_box = Gtk::make_managed<Gtk::EventBox>();
        oneliner_event_box->add(*sample_text_oneliner_);
        oneliner_event_box->set_above_child(true);
        build_sample_text_popover(oneliner_event_box);
        sample_text_holder = oneliner_event_box;
    }

    Gtk::Widget* opentype_frame = build_opentype_controls();

    Gtk::VBox* sample_text_box = Gtk::make_managed<Gtk::VBox>();
    sample_text_box->pack_start(*sample_text_holder, false, true);
    sample_text_box->pack_start(*opentype_frame, true, true);

    return sample_text_box;
}

Glib::ustring PrintPreviewWidget::label() { return _("Preview"); }

void PrintPreviewWidget::draw_page_cb(
    const Glib::RefPtr<Gtk::PrintContext>& context, int page_nr) {
    Cairo::RefPtr<Cairo::Context> cr = context->get_cairo_context();
    Glib::RefPtr<Gtk::PageSetup> setup = context->get_page_setup();

    // We need to align resolution between the physical paper, Cairo context and
    // our conventions. The physical paper page is normally measured in points
    // (1/72 in). The Cairo context provided by Gtk::PrintOperation carries its
    // own resolution, and must be scaled to points. After the scaling all
    // dimensions should be passed to Cairo::Context in points.
    Cairo::Rectangle printable_area =
        calculate_printable_area(setup, Gtk::UNIT_POINTS);

    cr->scale(context->get_dpi_x() / 72.0, context->get_dpi_y() / 72.0);

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

size_t PrintPreviewWidget::begin_print(
    const Glib::RefPtr<Gtk::PrintContext>& context) {
    Cairo::Rectangle printable_area =
        calculate_printable_area(context->get_page_setup(), Gtk::UNIT_POINTS);
    activate_cairo_printer(context->get_cairo_context(), printable_area);

    return paginate();
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
    page_counter_.set_draw_value(true);
    page_counter_.signal_format_value().connect([this](double page_num) {
        char buffer[32];
        int total_pages = (int)page_counter_.get_adjustment()->get_upper() - 1;
        sprintf(buffer, _("Page %d of %d"), (int)page_num, total_pages);
        return Glib::ustring(buffer);
    });
    // Modify scale label layout to prevent line breaks, they don't look good.
    page_counter_.signal_realize().connect(
        [this]() { page_counter_.get_layout()->set_width(-1); });
    // Initialize with some sane values to avoid unsightly GTK warnings.
    page_counter_.get_adjustment()->configure(1, 1, 2, 1, 1, 1);
    page_counter_.signal_value_changed().connect(
        [this] { preview_area.queue_draw(); });

    aspect_wrapper.set_hexpand(true);
    aspect_wrapper.set_vexpand(true);
    aspect_wrapper.set_shadow_type(Gtk::SHADOW_NONE);

    preview_area.signal_draw().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::draw_preview_area));
    preview_area.add_events(Gdk::SCROLL_MASK);
    preview_area.signal_scroll_event().connect(
        sigc::mem_fun(*this, &PrintPreviewWidget::on_preview_area_scroll));

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

    build_sample_text_editor();
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

void PrintPreviewWidget::activate_cairo_printer(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const Cairo::Rectangle& printable_area) {
    if (radio_full_display_->get_active()) {
        double font_size = size_entry_->get_value();
        persistent.full_display_size = (size_t)font_size;
        cairo_painter_.activate_full_display_printer(printable_area, font_size);
    } else if (radio_glyph_pages_->get_active()) {
        persistent.full_page_scaling = scaling_option_->get_active_id();
        cairo_painter_.activate_full_glyph_printer(
            persistent.full_page_scaling);
    } else if (radio_sample_text_->get_active()) {
        gsize length = 0;
        Glib::RefPtr<Gtk::TextBuffer> buffer = sample_text_->get_buffer();
        char* out_buffer = (char*)buffer->serialize(
            buffer, widget::RichTechEditor::rich_text_mime_type,
            buffer->begin(), buffer->end(), length);
        persistent.sample_text = out_buffer;

        Glib::ustring entry_text = script_lang_combo_->get_entry()->get_text();
        auto [script, lang] = extract_script_lang_tags(entry_text);

        std::map<Tag, bool> features;
        std::vector<int> selected_rows = feature_tags_list_->get_selected();
        std::set<int> selected_rows_set(selected_rows.begin(),
                                        selected_rows.end());
        for (int row_idx = 0; row_idx < feature_tags_list_->size(); ++row_idx) {
            Tag feature(feature_tags_list_->get_text(row_idx).c_str());
            features[feature] = selected_rows_set.count(row_idx) != 0;
        }

        cairo_painter_.activate_sample_text_printer(
            cr, printable_area, persistent.sample_text, script, lang, features);
    } else {
        cairo_painter_.activate_multisize_printer(printable_area,
                                                  kMultiPointsizes);
    }
}

void PrintPreviewWidget::draw_page(const Cairo::RefPtr<Cairo::Context>& cr,
                                   const Cairo::Rectangle& printable_area,
                                   int page_nr) {
    ff::utils::CairoContext context(cr, printable_area);
    cairo_painter_.add_page(page_nr, &context);

    paginate();
}

size_t PrintPreviewWidget::paginate() {
    size_t num_pages = cairo_painter_.page_count();

    // Changes to scale lead to focus changes which may inadvertently close the
    // sample text popover. When the popover is visible, we shall avoid touching
    // the scale. It would be updated automatically after the popover closes and
    // the preview area refreshes.
    if (!sample_text_oneliner_ || !sample_text_->is_visible()) {
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

    activate_cairo_printer(cr, printable_area);
    draw_page(cr, printable_area, page_nr);

    return true;
}

bool PrintPreviewWidget::on_preview_area_scroll(GdkEventScroll* event) {
    double delta = 0;
    if (event->direction == GDK_SCROLL_UP) {
        delta = -1;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        delta = 1;
    } else if (event->direction == GDK_SCROLL_SMOOTH) {
        delta = event->delta_y;
    } else {
        return false;
    }

    if (delta == 0) {
        return false;
    }

    double min_page = page_counter_.get_adjustment()->get_lower();
    double max_page = page_counter_.get_adjustment()->get_upper() - 1;
    double step = delta > 0 ? 1 : -1;
    double new_value =
        std::clamp(page_counter_.get_value() + step, min_page, max_page);

    if (new_value != page_counter_.get_value()) {
        page_counter_.set_value(new_value);
    }
    return true;
}

void PrintPreviewWidget::on_display_toggled() {
    for (auto r : {radio_full_display_, radio_glyph_pages_, radio_multi_size_,
                   radio_sample_text_}) {
        if (r->get_active()) {
            persistent.radio_selection = r->get_name();
            stack_->set_visible_child(persistent.radio_selection);
        }
    }
    preview_area.queue_draw();
}

}  // namespace ff::dlg
