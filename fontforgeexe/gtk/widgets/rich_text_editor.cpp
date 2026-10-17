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

#include "rich_text_editor.hpp"

#include <iomanip>
#include <iostream>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>

#include "intl.h"
#include "../utils.hpp"

namespace ff::widget {

void dump_character(Glib::ustring& unicode_buffer, const gunichar& character) {
    Glib::ustring seq;
    switch (character) {
        case '<':
            seq = "&lt;";
            break;
        case '\"':
            seq = "&quot;";
            break;
        case '\'':
            seq = "&apos;";
            break;
        case '>':
            seq = "&gt;";
            break;
        case '&':
            seq = "&amp;";
            break;
    }

    if (seq.empty()) {
        unicode_buffer += character;
    } else {
        unicode_buffer += seq;
    }
}

// Dumps the content of a Gtk::TextBuffer to a string, with all the TextBuffer
// tags serialized as XML tags. The result is not a proper XML, since its tags
// can be interleaved and not properly nested.
std::string dump_text_buffer(
    const Glib::RefPtr<Gtk::TextBuffer>& content_buffer,
    const Gtk::TextBuffer::iterator& start,
    const Gtk::TextBuffer::iterator& end) {
    Glib::ustring unicode_buffer;
    for (auto it = start; it != end; ++it) {
        // Retrieve closing and opening tags
        for (bool opening : {false, true}) {
            std::vector<Glib::RefPtr<Gtk::TextTag>> tags =
                it.get_toggled_tags(opening);
            for (auto tag : tags) {
                Glib::ustring tag_name = tag->property_name();
                // Dump the tag in raw format, without normalizing.
                unicode_buffer += (opening ? "<" : "</") + tag_name + ">";
            }
        }

        dump_character(unicode_buffer, *it);
    }

    return unicode_buffer;
}

RichTextEditor::RichTextEditor(const std::vector<double>& pointsizes,
                               const RichTextFontList& font_list) {
    scale_css_provider_ = Gtk::CssProvider::create();
    text_view_.get_style_context()->add_provider(
        scale_css_provider_, GTK_STYLE_PROVIDER_PRIORITY_USER - 1);
    refresh_scale_css();

    auto size_combo = build_size_combo(pointsizes);
    size_combo->set_tooltip_text(_("Font Size"));

    std::string default_font_id =
        font_list.empty() ? "" : "font|" + font_list[0].first;
    ClearFormattingButton* clear_button =
        Gtk::make_managed<ClearFormattingButton>(text_view_.get_buffer(),
                                                 default_font_id);
    clear_button->set_icon_name("edit-clear-all");
    clear_button->set_tooltip_text(_("Clear Formatting"));
    Gtk::ToolButton* hamburger_button = build_tools_menu();

    toolbar_ = build_toolbar(font_list);
    toolbar_->append(*size_combo);
    toolbar_->append(*clear_button);
    toolbar_->append(*hamburger_button);

    toolbar_->set_hexpand();

    text_view_.set_wrap_mode(Gtk::WRAP_WORD);
    text_view_.set_hexpand();
    text_view_.set_vexpand();
    text_view_.add_events(Gdk::SCROLL_MASK);
    text_view_.signal_scroll_event().connect(
        sigc::mem_fun(*this, &RichTextEditor::on_text_view_scroll_event));
    g_signal_connect(text_view_.gobj(), "paste-clipboard",
                     G_CALLBACK(&RichTextEditor::on_text_view_paste_clipboard),
                     this);

    scrolled_.add(text_view_);
    attach(*toolbar_, 0, 0);
    attach(scrolled_, 0, 1);
}

void RichTextEditor::load_buffer(const Glib::RefPtr<Gtk::TextBuffer>& buffer,
                                 const Gtk::TextBuffer::iterator& insert_it,
                                 const layout::ParsedRichText& parsed_text) {
    auto cursor_mark = buffer->create_mark(insert_it, false);
    auto tag_table = buffer->get_tag_table();

    for (const auto& [raw_tags, text] : parsed_text) {
        if (text.empty()) {
            continue;
        }

        auto start_mark = buffer->create_mark(cursor_mark->get_iter());
        buffer->insert(cursor_mark->get_iter(), text);
        // This is a bit of a hack, because property widgets apply their own
        // tags in their signal_insert() handlers, and we don't have a good way
        // to know whether the insertion is due to typing or due to loading text
        // from deserializer.
        buffer->remove_all_tags(start_mark->get_iter(),
                                cursor_mark->get_iter());

        for (const std::string& tag_name : raw_tags) {
            if (tag_name == "ff-sample") {
                continue;
            }

            auto tag = tag_table ? tag_table->lookup(tag_name)
                                 : Glib::RefPtr<Gtk::TextTag>();
            if (tag) {
                buffer->apply_tag(tag, start_mark->get_iter(),
                                  cursor_mark->get_iter());
            }
        }

        buffer->delete_mark(start_mark);
    }
}

void RichTextEditor::on_text_view_paste_clipboard(GtkTextView* text_view,
                                                  gpointer user_data) {
    RichTextEditor* self = static_cast<RichTextEditor*>(user_data);
    if (self == nullptr) return;

    if (self->request_clipboard_rich_text()) {
        g_signal_stop_emission_by_name(text_view, "paste-clipboard");
    }
}

bool RichTextEditor::on_text_view_scroll_event(GdkEventScroll* event) {
    static const double zoom_sensitivity = 0.1;
    if (!event || (event->state & GDK_CONTROL_MASK) == 0) {
        return false;
    }

    double updated_scale = global_scale_;
    if (event->direction == GDK_SCROLL_UP) {
        updated_scale += zoom_sensitivity;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        updated_scale -= zoom_sensitivity;
    } else if (event->direction == GDK_SCROLL_SMOOTH) {
        updated_scale -= event->delta_y * zoom_sensitivity;
    } else {
        return false;
    }

    updated_scale = std::clamp(updated_scale, 0.3, 3.0);

    if (std::abs(updated_scale - global_scale_) < 0.000001) {
        return true;
    }

    global_scale_ = updated_scale;
    refresh_scale_css();

    return true;
}

void RichTextEditor::refresh_scale_css() {
    int relative_percent = static_cast<int>(global_scale_ * 100.0);
    scale_css_provider_->load_from_data(
        "textview {font-size: " + std::to_string(relative_percent) + "%;}");
}

bool RichTextEditor::request_clipboard_rich_text() {
    Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
    if (!clipboard) {
        return false;
    }

    if (clipboard->wait_is_rich_text_available(text_view_.get_buffer())) {
        clipboard->request_rich_text(
            text_view_.get_buffer(),
            sigc::mem_fun(*this,
                          &RichTextEditor::on_clipboard_rich_text_received));
        return true;
    } else
        return false;
}

void RichTextEditor::on_clipboard_rich_text_received(
    const Glib::ustring& format, const std::string& text) {
    Glib::RefPtr<Gtk::TextBuffer> buffer = text_view_.get_buffer();
    if (!buffer || text.empty() || format.empty()) return;

    Gtk::TextBuffer::iterator start, end;
    if (buffer->get_selection_bounds(start, end)) {
        buffer->erase(start, end);
    }

    Gtk::TextBuffer::iterator insert_it =
        buffer->get_iter_at_mark(buffer->get_insert());

    try {
        buffer->deserialize(buffer, format, insert_it,
                            reinterpret_cast<const guint8*>(text.data()),
                            text.size());
    } catch (const Glib::Error&) {
    }
}

RichTextEditor::SlantComboBox* RichTextEditor::build_slanted_combo() {
    std::map<Pango::Style, std::string /*label*/> labels{
        {Pango::STYLE_NORMAL, _("Normal")},
        {Pango::STYLE_OBLIQUE, _("Oblique")},
        {Pango::STYLE_ITALIC, _("Italic")},
    };

    return Gtk::manage(new RichTextEditor::SlantComboBox(
        text_view_.get_buffer(), &Gtk::TextTag::property_style, labels));
}

RichTextEditor::StretchComboBox* RichTextEditor::build_stretch_combo() {
    std::map<Pango::Stretch, std::string /*label*/> labels{
        {Pango::STRETCH_ULTRA_CONDENSED, _("Ultra-Condensed (50%)")},
        {Pango::STRETCH_EXTRA_CONDENSED, _("Extra-Condensed (62.5%)")},
        {Pango::STRETCH_CONDENSED, _("Condensed (75%)")},
        {Pango::STRETCH_SEMI_CONDENSED, _("Semi-Condensed (87.5%)")},
        {Pango::STRETCH_NORMAL, _("Medium (100%)")},
        {Pango::STRETCH_SEMI_EXPANDED, _("Semi-Expanded (112.5%)")},
        {Pango::STRETCH_EXPANDED, _("Expanded (125%)")},
        {Pango::STRETCH_EXTRA_EXPANDED, _("Extra-Expanded (150%)")},
        {Pango::STRETCH_ULTRA_EXPANDED, _("Ultra-Expanded (200%)")},
    };

    return Gtk::manage(new RichTextEditor::StretchComboBox(
        text_view_.get_buffer(), &Gtk::TextTag::property_stretch, labels));
}

RichTextEditor::TagComboBox* RichTextEditor::build_size_combo(
    const std::vector<double>& pointsizes) {
    double default_size = 36.0;
    std::string default_id = "size|36";

    std::vector<double> sorted_pointsizes = pointsizes;
    std::sort(sorted_pointsizes.begin(), sorted_pointsizes.end());

    // By convention, TextBuffer::Tag with name e.g. "size|24" will
    // be exported to XML tag as <size value="24">. Unlike in XML,
    // TextBuffer tags must have unique names.
    std::map<std::string /*id*/, Glib::RefPtr<Gtk::TextTag>> tag_map;
    std::vector<std::pair<std::string /*id*/, std::string /*label*/>> labels;

    for (double size_pt : sorted_pointsizes) {
        // Convert double value to string without trailing zeros or period.
        std::ostringstream oss;
        oss << std::setprecision(8) << std::noshowpoint << size_pt;
        std::string num_str = oss.str();

        std::string tag_id = "size|" + num_str;
        char buffer[321];
        sprintf(buffer, _("%s pt"), num_str.c_str());
        Glib::ustring label(buffer);

        // Create and register tag. The display size is emulated by scaling
        // relatively to the default size.
        if (tag_id != default_id) {
            auto tag = text_view_.get_buffer()->create_tag(tag_id);
            tag->property_scale() = size_pt / default_size;
            tag_map[tag_id] = tag;
        }

        labels.emplace_back(tag_id, label);
    }

    return Gtk::make_managed<TagComboBox>(text_view_.get_buffer(), default_id,
                                          tag_map, labels);
}

RichTextEditor::WeightComboBox* RichTextEditor::build_weight_combo() {
    std::map<int /*weight*/, std::string /*label*/> labels{
        {Pango::WEIGHT_THIN, _("100 Thin")},
        {Pango::WEIGHT_ULTRALIGHT, _("200 Extra-Light")},
        {Pango::WEIGHT_LIGHT, _("300 Light")},
        {Pango::WEIGHT_NORMAL, _("400 Regular")},
        {Pango::WEIGHT_MEDIUM, _("500 Medium")},
        {Pango::WEIGHT_SEMIBOLD, _("600 Semi-Bold")},
        {Pango::WEIGHT_BOLD, _("700 Bold")},
        {Pango::WEIGHT_ULTRABOLD, _("800 Extra-Bold")},
        {Pango::WEIGHT_HEAVY, _("900 Black")},
    };

    return Gtk::manage(new RichTextEditor::WeightComboBox(
        text_view_.get_buffer(), &Gtk::TextTag::property_weight, labels));
}

RichTextEditor::TagComboBox* RichTextEditor::build_fonts_combo(
    const RichTextFontList& font_list) {
    // UI fonts normally don't have a multitude of weights, so we emulate
    // weights by color instead. The user shall check actual rendering in the
    // preview panel.
    static const std::map<Pango::Weight, std::string> weight_color_map{
        {Pango::WEIGHT_THIN, "gray"},
        {Pango::WEIGHT_ULTRALIGHT, "dimgray"},
        {Pango::WEIGHT_LIGHT, "darkslategray"},
        {Pango::WEIGHT_NORMAL, "black"},
        {Pango::WEIGHT_MEDIUM, "dimgray"},
        {Pango::WEIGHT_SEMIBOLD, "darkslategray"},
        {Pango::WEIGHT_BOLD, "black"},
        {Pango::WEIGHT_ULTRABOLD, "blue"},
        {Pango::WEIGHT_HEAVY, "navy"},
    };

    // By convention, TextBuffer::Tag with name e.g. "font|New Century
    // Schoolbook" will be exported to XML tag as <font value="New Century
    // Schoolbook">. Unlike in XML, TextBuffer tags must have unique names.
    std::map<std::string /*id*/, Glib::RefPtr<Gtk::TextTag>> tag_map;
    std::vector<std::pair<std::string /*id*/, std::string /*label*/>> labels;

    for (const auto& [font_name, properties] : font_list) {
        std::string tag_id = "font|" + font_name;
        auto tag = text_view_.get_buffer()->create_tag(tag_id);

        tag->property_weight() = properties.weight;
        tag->property_style() = properties.style;
        tag->property_stretch() = properties.stretch;
        tag->property_underline() = properties.underline;
        if (weight_color_map.count(properties.weight) > 0) {
            tag->property_foreground() = weight_color_map.at(properties.weight);
        }
        tag_map[tag_id] = tag;

        labels.emplace_back(tag_id, font_name);
    }

    return Gtk::make_managed<TagComboBox>(text_view_.get_buffer(), "", tag_map,
                                          labels);
}

Gtk::ToolButton* RichTextEditor::build_tools_menu() {
    Gtk::Menu* hamburger_menu = Gtk::make_managed<Gtk::Menu>();
    Gtk::MenuItem* load_item = Gtk::make_managed<Gtk::MenuItem>(_("Load XML"));
    load_item->signal_activate().connect(
        sigc::mem_fun(*this, &RichTextEditor::on_load_buffer_from_xml));
    Gtk::MenuItem* save_item =
        Gtk::make_managed<Gtk::MenuItem>(_("Save as XML"));
    save_item->signal_activate().connect(
        sigc::mem_fun(*this, &RichTextEditor::on_save_buffer_to_xml));
    hamburger_menu->append(*load_item);
    hamburger_menu->append(*save_item);
    hamburger_menu->show_all();

    Gtk::ToolButton* hamburger_button = Gtk::make_managed<Gtk::ToolButton>();
    hamburger_button->set_icon_name("open-menu-symbolic");
    hamburger_button->set_tooltip_text(_("Import and export tools"));
    hamburger_button->signal_clicked().connect(
        [hamburger_menu, hamburger_button]() {
            hamburger_menu->popup_at_widget(hamburger_button,
                                            Gdk::GRAVITY_SOUTH_WEST,
                                            Gdk::GRAVITY_NORTH_WEST, nullptr);
        });

    return hamburger_button;
}

static std::tuple<std::set<int>, std::set<Pango::Style>,
                  std::set<Pango::Stretch>>
collect_properties(const RichTextFontList& font_list) {
    std::set<int> weights;
    std::set<Pango::Style> styles;
    std::set<Pango::Stretch> stretches;

    for (const auto& [font_name, properties] : font_list) {
        weights.insert(properties.weight);
        styles.insert(properties.style);
        stretches.insert(properties.stretch);
    }

    return {weights, styles, stretches};
}

Gtk::Toolbar* RichTextEditor::build_toolbar(const RichTextFontList& font_list) {
    Gtk::Toolbar* toolbar = Gtk::make_managed<Gtk::Toolbar>();
    std::shared_ptr<std::vector<ToolbarStateSync*>> toolbar_state_widgets =
        std::make_shared<std::vector<ToolbarStateSync*>>();

    auto fonts_combo = build_fonts_combo(font_list);
    fonts_combo->set_tooltip_text(_("Font"));
    toolbar->append(*fonts_combo);
    fonts_combo->add_group(toolbar_state_widgets);

    auto [weights, styles, stretches] = collect_properties(font_list);

    // Build weight UI elements.
    if (weights.size() <= 2) {
        Gtk::ToggleToolButton* bold_button = nullptr;
        if (weights.size() == 1) {
            // Show a disabled "Bold" button just for visualization.
            bold_button = Gtk::make_managed<Gtk::ToggleToolButton>();
            bold_button->set_active(*weights.rbegin() >=
                                    Pango::WEIGHT_SEMIBOLD);
            bold_button->set_sensitive(false);
        } else {
            // Show a "Bold" toggle button with the available weights.
            auto bold_prop_button = Gtk::manage(new TogglePropButton(
                text_view_.get_buffer(), &Gtk::TextTag::property_weight,
                {*weights.begin(), *weights.rbegin()}));
            bold_prop_button->add_group(toolbar_state_widgets);
            bold_button = bold_prop_button;
        }
        bold_button->set_icon_name("format-text-bold");
        bold_button->set_tooltip_text(_("Bold"));
        toolbar->append(*bold_button);
    } else {
        auto weight_combo = build_weight_combo();
        weight_combo->set_tooltip_text(_("Weight Class"));
        weight_combo->set_enabled_items(weights);
        toolbar->append(*weight_combo);
        weight_combo->add_group(toolbar_state_widgets);
    }

    // Build slant UI elements.
    if (styles.size() == 1 ||
        (styles.size() == 2 && styles.count(Pango::STYLE_NORMAL) > 0)) {
        Gtk::ToggleToolButton* italic_button = nullptr;
        if (styles.size() == 1) {
            // Show a disabled "Italic" button just for visualization.
            italic_button = Gtk::make_managed<Gtk::ToggleToolButton>();
            italic_button->set_active(*styles.rbegin() != Pango::STYLE_NORMAL);
            italic_button->set_sensitive(false);
        } else {
            // Show an "Italic" toggle button with the available styles.
            auto italic_prop_button = Gtk::manage(new TogglePropButton(
                text_view_.get_buffer(), &Gtk::TextTag::property_style,
                {*styles.begin(), *styles.rbegin()}));
            italic_prop_button->add_group(toolbar_state_widgets);
            italic_button = italic_prop_button;
        }
        italic_button->set_icon_name("format-text-italic");
        italic_button->set_tooltip_text(_("Italic"));
        toolbar->append(*italic_button);
    } else {
        auto slanted_combo = build_slanted_combo();
        slanted_combo->set_tooltip_text(_("Slant Style"));
        slanted_combo->set_enabled_items(styles);
        toolbar->append(*slanted_combo);
        slanted_combo->add_group(toolbar_state_widgets);
    }

    // Configure availablity of stretched UI element.
    if (stretches.size() > 1) {
        auto stretch_combo = build_stretch_combo();
        stretch_combo->set_tooltip_text(_("Width Class"));
        stretch_combo->set_enabled_items(stretches);
        toolbar->append(*stretch_combo);
        stretch_combo->add_group(toolbar_state_widgets);
    }

    return toolbar;
}

void RichTextEditor::on_load_buffer_from_xml() {
    Gtk::FileChooserDialog dialog(_("Load sample from XML"),
                                  Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*dynamic_cast<Gtk::Window*>(get_toplevel()));

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    auto filter = Gtk::FileFilter::create();
    filter->set_name(_("FontForge sample text (*.ffxml)"));
    filter->add_pattern("*.ffxml");
    dialog.add_filter(filter);
    dialog.set_filter(filter);

    if (dialog.run() != Gtk::RESPONSE_OK) {
        return;
    }

    std::ifstream file(dialog.get_filename(), std::ios::binary);
    if (!file.is_open()) {
        return;
    }

    Glib::RefPtr<Gtk::TextBuffer> buffer = text_view_.get_buffer();
    if (!buffer) {
        return;
    }

    buffer->set_text("");
    auto insert_it = buffer->begin();

    std::string xml(std::istreambuf_iterator<char>(file), {});
    buffer->deserialize(
        buffer, layout::RichTextFontSelector::rich_text_mime_type, insert_it,
        reinterpret_cast<const guint8*>(xml.data()), xml.size());
}

void RichTextEditor::on_save_buffer_to_xml() {
    Gtk::FileChooserDialog dialog(_("Save sample as XML"),
                                  Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*dynamic_cast<Gtk::Window*>(get_toplevel()));

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    auto filter = Gtk::FileFilter::create();
    filter->set_name(_("FontForge sample text (*.ffxml)"));
    filter->add_pattern("*.ffxml");
    dialog.add_filter(filter);
    dialog.set_filter(filter);
    dialog.set_current_name("Untitled.ffxml");

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filepath = dialog.get_filename();
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view_.get_buffer();

        gsize length = 0;
        char* serialized = (char*)buffer->serialize(
            buffer, layout::RichTextFontSelector::rich_text_mime_type,
            buffer->begin(), buffer->end(), length);

        if (serialized != nullptr) {
            std::ofstream file(filepath, std::ios::binary);
            if (file.is_open()) {
                file.write(reinterpret_cast<const char*>(serialized), length);
                file.close();
            }
            delete[] serialized;
        }
    }
}

///////////////////////////////////////////////////////////////////////
///                        Templated helpers                        ///
///////////////////////////////////////////////////////////////////////

template <typename PROPERTY_PROXY>
void apply_property_tag(Glib::RefPtr<Gtk::TextBuffer> text_buffer,
                        const Gtk::TextBuffer::iterator& start,
                        const Gtk::TextBuffer::iterator& end,
                        typename PROPERTY_PROXY::PropertyType target_value,
                        PROPERTY_PROXY (Gtk::TextTag::*property)()) {
    // Collect "font" tags.
    auto tag_table = text_buffer->get_tag_table();
    std::vector<Glib::RefPtr<Gtk::TextTag>> font_tags;
    tag_table->foreach ([&font_tags](const Glib::RefPtr<Gtk::TextTag>& tag) {
        Glib::ustring tag_name = tag->property_name();
        if (tag_name.find("font|") == 0) {
            font_tags.push_back(tag);
        }
    });

    // For each "font" tag traverse all ranges tagged with it and apply the new
    // weight to them.
    for (const auto& tag : font_tags) {
        // Define distance between tags. Same family and the target weight are
        // enforced on other_tag.
        auto distance = [tag, property](
                            const Glib::RefPtr<Gtk::TextTag>& other_tag,
                            typename PROPERTY_PROXY::PropertyType target_prop) {
            if (other_tag->property_family() != tag->property_family())
                return std::numeric_limits<int>::max();
            if ((other_tag.get()->*property)() != target_prop)
                return std::numeric_limits<int>::max();

            int dist =
                (tag->property_style() != other_tag->property_style() ? 1 : 0) +
                (tag->property_stretch() != other_tag->property_stretch() ? 1
                                                                          : 0) +
                (tag->property_weight() != other_tag->property_weight() ? 1
                                                                        : 0);

            return dist;
        };

        // Find the closest "font" tag which would replace the current one and
        // has the desired weight.
        auto closest_font_tag = *std::min_element(
            font_tags.begin(), font_tags.end(),
            [&target_value, distance](const auto& a, const auto& b) {
                return distance(a, target_value) < distance(b, target_value);
            });
        if (closest_font_tag == tag) continue;

        // Locate the first range tagged with this font tag.
        Gtk::TextBuffer::iterator tag_start = start;
        if (!start.has_tag(tag)) {
            tag_start.forward_to_tag_toggle(tag);
        }
        Gtk::TextBuffer::iterator tag_end = tag_start;
        tag_end.forward_to_tag_toggle(tag);

        while (tag_start < end) {
            // Apply the new weight to the range tagged with this font tag.
            if (tag_end > end) {
                tag_end = end;
            }
            text_buffer->remove_tag(tag, tag_start, tag_end);
            text_buffer->apply_tag(closest_font_tag, tag_start, tag_end);

            // Advance to the next range tagged with this font tag.
            tag_start = tag_end;
            tag_start.forward_to_tag_toggle(tag);
            tag_end = tag_start;
            tag_end.forward_to_tag_toggle(tag);
        }
    }

    // TODO(iorsh): Determine the new state of all relevant widgets based on the
    // current selection.
}

template <typename PROPERTY_PROXY>
std::set<typename PROPERTY_PROXY::PropertyType> collect_property_values(
    const std::vector<Glib::RefPtr<Gtk::TextTag>>& tags,
    PROPERTY_PROXY (Gtk::TextTag::*property)()) {
    std::set<typename PROPERTY_PROXY::PropertyType> result;
    for (const auto& tag : tags) {
        Glib::ustring tag_name = tag->property_name();
        if (tag_name.find("font|") == 0)
            result.insert((tag.get()->*property)().get_value());
    }
    return result;
}

// Check if the selected range has consistent property values for all "font"
// tags. If the selection is empty, check the cursor position instead. Return
// std::nullopt if the selection contains more than one value for the property,
// otherwise return the consistent value.
template <typename PROPERTY_PROXY>
std::optional<typename PROPERTY_PROXY::PropertyType> is_consistent_selection(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer,
    PROPERTY_PROXY (Gtk::TextTag::*property)()) {
    // To determine the state of the button, we check if every "font" tag has
    // the style equal to styles_[1].
    Gtk::TextBuffer::iterator start, end;
    std::optional<typename PROPERTY_PROXY::PropertyType> value;

    if (!text_buffer->get_selection_bounds(start, end)) {
        start--;
    }

    // Check tags which are active at the start of the selection.
    auto start_tags = start.get_tags();
    auto property_values = collect_property_values(start_tags, property);

    while (property_values.size() < 2 && start < end) {
        start.forward_to_tag_toggle(Glib::RefPtr<Gtk::TextTag>());
        if (start >= end) break;

        auto tags_on = start.get_toggled_tags(true);
        property_values.merge(collect_property_values(tags_on, property));
    }

    if (property_values.size() == 1) value = *property_values.begin();
    return value;
}

///////////////////////////////////////////////////////////////////////
///                 RichTextEditor::TogglePropButton                ///
///////////////////////////////////////////////////////////////////////

template <typename PROPERTY_PROXY>
RichTextEditor::TogglePropButton<PROPERTY_PROXY>::TogglePropButton(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer, PROPERTY_GETTER proxy_caller,
    std::array<STYLE, 2> styles)
    : ToolbarStateSync(text_buffer),
      proxy_caller_(proxy_caller),
      styles_(styles) {
    // Called whenever the selection or the cursor position is changed.
    // Sets the correct visual state of the widget
    text_buffer_->signal_mark_set().connect(
        sigc::mem_fun(*this, &TogglePropButton::on_buffer_cursor_changed));
}

template <typename PROPERTY_PROXY>
void RichTextEditor::TogglePropButton<PROPERTY_PROXY>::on_button_toggled() {
    Gtk::TextBuffer::iterator start, end;
    if (text_buffer_->get_selection_bounds(start, end)) {
        apply_property_tag(text_buffer_, start, end,
                           styles_[get_active() ? 1 : 0], proxy_caller_);
    }
    refresh_toolbar_state();
}

template <typename PROPERTY_PROXY>
void RichTextEditor::TogglePropButton<PROPERTY_PROXY>::on_buffer_cursor_changed(
    const Gtk::TextBuffer::iterator&,
    const Glib::RefPtr<Gtk::TextBuffer::Mark>& mark) {
    if (mark->get_name() != "insert") {
        return;
    }

    auto selected_value = is_consistent_selection(text_buffer_, proxy_caller_);
    bool button_active =
        selected_value.has_value() && *selected_value == styles_[1];

    ui_utils::gtk_set_widget_state_without_event(
        (Gtk::ToggleToolButton*)this, &TogglePropButton::signal_toggled,
        sigc::mem_fun(*this, &TogglePropButton::on_button_toggled),
        [this, button_active]() { set_active(button_active); });
}

///////////////////////////////////////////////////////////////////////
///                   RichTextEditor::PropComboBox                  ///
///////////////////////////////////////////////////////////////////////

template <typename PROPERTY_PROXY>
RichTextEditor::PropComboBox<PROPERTY_PROXY>::PropComboBox(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer, PROPERTY_GETTER proxy_caller,
    const std::map<STYLE, std::string /*label*/>& labels)
    : ToolbarStateSync(text_buffer),
      proxy_caller_(proxy_caller),
      labels_(labels) {
    // Add entries to combo box
    for (const auto& [prop_value, label] : labels) {
        combo_box_.append(label, label);
    }

    combo_box_.set_focus_on_click(false);
    add(combo_box_);

    // Called whenever the selection or the cursor position is changed. Sets the
    // correct visual state of the widget
    text_buffer_->signal_mark_set().connect(
        sigc::mem_fun(*this, &PropComboBox::on_buffer_cursor_changed));
}

template <typename PROPERTY_PROXY>
void RichTextEditor::PropComboBox<PROPERTY_PROXY>::set_enabled_items(
    const std::set<STYLE>& enabled_values) {
    for (const auto& [prop_value, label] : labels_) {
        bool is_enabled = enabled_values.count(prop_value) > 0;
        combo_box_.set_item_sensitive(label, is_enabled);
    }
}

template <typename PROPERTY_PROXY>
void RichTextEditor::PropComboBox<PROPERTY_PROXY>::on_box_changed() {
    // Locate the property value corresponding to the active label and apply it
    // to the selection.
    for (const auto& [prop_value, label] : labels_) {
        if (combo_box_.get_active_id() == label) {
            Gtk::TextBuffer::iterator start, end;
            if (text_buffer_->get_selection_bounds(start, end)) {
                apply_property_tag(text_buffer_, start, end, prop_value,
                                   proxy_caller_);
            }
            refresh_toolbar_state();
            break;
        }
    }
}

template <typename PROPERTY_PROXY>
void RichTextEditor::PropComboBox<PROPERTY_PROXY>::on_buffer_cursor_changed(
    const Gtk::TextBuffer::iterator&,
    const Glib::RefPtr<Gtk::TextBuffer::Mark>& mark) {
    if (mark->get_name() != "insert") {
        return;
    }

    auto selected_value = is_consistent_selection(text_buffer_, proxy_caller_);
    std::string active_label = "";
    if (selected_value.has_value() && labels_.count(*selected_value) > 0)
        active_label = labels_.at(*selected_value);

    ui_utils::gtk_set_widget_state_without_event(
        (Gtk::ComboBox*)&combo_box_, &Gtk::ComboBox::signal_changed,
        sigc::mem_fun(*this, &PropComboBox::on_box_changed),
        [this, active_label]() {
            if (active_label.empty()) {
                // Gtk::ComboBox continues to show the last active item even
                // after it was unset. The following hack addresses it.
                combo_box_.insert(0, "empty", "");
                combo_box_.set_active_id("empty");
                combo_box_.remove_text(0);
            } else {
                combo_box_.set_active_id(active_label);
            }
        });
}

///////////////////////////////////////////////////////////////////////
///                 RichTextEditor::TagComboBox                     ///
///////////////////////////////////////////////////////////////////////

RichTextEditor::TagComboBox::TagComboBox(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer, const std::string& default_id,
    const std::map<std::string /*id*/, Glib::RefPtr<Gtk::TextTag>>& tag_map,
    const std::vector<std::pair<std::string /*id*/, std::string /*label*/>>&
        labels)
    : ToolbarStateSync(text_buffer),
      default_id_(default_id),
      tag_map_(tag_map) {
    // Add entries to combo box
    for (const auto& [tag_id, label] : labels) {
        combo_box_.append(tag_id, label);
    }

    // If the default id was already specified, it should have no actual tag
    // associated with it.
    if (default_id_.empty()) default_id_ = labels.front().first;

    combo_box_.set_active_id(default_id_);
    combo_box_.set_focus_on_click(false);
    add(combo_box_);

    // Called whenever the selection or the cursor position is changed. Sets the
    // correct visual state of the widget
    text_buffer_->signal_mark_set().connect(
        sigc::mem_fun(*this, &TagComboBox::on_buffer_cursor_changed));

    // Called whenever a character is typed into the buffer. Set the tag on this
    // character according to the widget state.
    text_buffer_->signal_insert().connect(
        [this](const Gtk::TextBuffer::iterator& pos, const Glib::ustring& text,
               int bytes) {
            Gtk::TextBuffer::iterator start = pos;
            if (start.backward_chars(text.size())) {
                apply_tag(start, pos);
            }
        });
}

void RichTextEditor::TagComboBox::apply_tag(
    const Gtk::TextBuffer::iterator& start,
    const Gtk::TextBuffer::iterator& end) {
    std::string active_id = combo_box_.get_active_id();
    if (active_id.empty()) {
        // A mixed selection may have no active tag. When typing or pasting new
        // raw text into it, we apply the tag corresponding to the beginning of
        // the selection.
        Gtk::TextBuffer::iterator prev = start;
        prev.backward_char();
        active_id = get_active_tag(prev, start);

        // Fallback to default.
        if (active_id.empty()) active_id = default_id_;
    }

    // Remove all other tags from this group, except the new one.
    for (const auto& [tag_id, tag] : tag_map_) {
        if (tag_id != active_id) {
            text_buffer_->remove_tag(tag, start, end);
        } else {
            text_buffer_->apply_tag(tag, start, end);
        }
    }
}

void RichTextEditor::TagComboBox::on_box_changed() {
    Gtk::TextBuffer::iterator start, end;
    if (text_buffer_->get_selection_bounds(start, end)) {
        apply_tag(start, end);
    }
    refresh_toolbar_state();
}

std::string RichTextEditor::TagComboBox::get_active_tag(
    const Gtk::TextBuffer::iterator& start,
    const Gtk::TextBuffer::iterator& end) {
    // We are only interested in tags controlled by this widget. Check if any
    // controlled tag is active at the start.
    auto active_tags = start.get_tags();
    for (const auto& tag : active_tags) {
        Glib::ustring tag_id = tag->property_name();
        if (tag_map_.count(tag_id) > 0) {
            // We found an active controlled tag in the set. Check if it remains
            // active throughout the selection.
            auto start_copy = start;
            // TODO: Remove this cast in GTKMM4.
            start_copy.forward_to_tag_toggle(
                Glib::RefPtr<Gtk::TextTag>::cast_const(tag));
            if (start_copy >= end) {
                return tag_id;
            }
            return "";  // Nothing is consistently active throughout the
                        // selection
        }
    }

    // No controlled tag is active at the start. Check if any tag become active
    // before the end.
    for (const auto& [id, tag] : tag_map_) {
        auto start_copy = start;
        start_copy.forward_to_tag_toggle(tag);
        if (start_copy < end) {
            // Controlled tag  activated, nothing is consistently active
            // throughout the selection
            return "";
        }
    }

    return default_id_;
}

void RichTextEditor::TagComboBox::on_buffer_cursor_changed(
    const Gtk::TextBuffer::iterator&,
    const Glib::RefPtr<Gtk::TextBuffer::Mark>& mark) {
    if (mark->get_name() != "insert") {
        return;
    }

    Gtk::TextBuffer::iterator start, end;
    if (!text_buffer_->get_selection_bounds(start, end)) {
        start--;
    }

    std::string active_id = get_active_tag(start, end);

    ui_utils::gtk_set_widget_state_without_event(
        (Gtk::ComboBox*)&combo_box_, &Gtk::ComboBox::signal_changed,
        sigc::mem_fun(*this, &TagComboBox::on_box_changed),
        [this, active_id]() {
            if (active_id.empty()) {
                // Gtk::ComboBox continues to show the last active item even
                // after it was unset. The following hack addresses it.
                combo_box_.insert(0, "empty", "");
                combo_box_.set_active_id("empty");
                combo_box_.remove_text(0);
            } else {
                combo_box_.set_active_id(active_id);
            }
        });
}

///////////////////////////////////////////////////////////////////////
///             RichTextEditor::ClearFormattingButton               ///
///////////////////////////////////////////////////////////////////////

RichTextEditor::ClearFormattingButton::ClearFormattingButton(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer,
    const std::string& default_font_id)
    : text_buffer_(text_buffer), default_font_id_(default_font_id) {
    signal_clicked().connect(
        sigc::mem_fun(*this, &ClearFormattingButton::on_button_clicked));
}

void RichTextEditor::ClearFormattingButton::on_button_clicked() {
    Gtk::TextBuffer::iterator start, end;
    if (text_buffer_->get_selection_bounds(start, end)) {
        text_buffer_->remove_all_tags(start, end);
        auto default_font_tag =
            text_buffer_->get_tag_table()->lookup(default_font_id_);
        if (default_font_tag) {
            text_buffer_->apply_tag(default_font_tag, start, end);
        }
    }
}

}  // namespace ff::widget
