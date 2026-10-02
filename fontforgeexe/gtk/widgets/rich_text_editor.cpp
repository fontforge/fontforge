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
#include <optional>

#include "intl.h"
#include "../utils.hpp"
#include "layout/cairo_painter.hpp"

namespace ff::widget {

static ff::utils::ParsedRichText collapse_html_spaces(
    const ff::utils::ParsedRichText& input) {
    ff::utils::ParsedRichText collapsed;
    bool in_space = false;
    bool prev_in_p = false;
    auto tag_is_p = [](const std::string& tag) {
        return tag == "p" || tag.substr(0, 2) == "p ";
    };

    for (const auto& [tags, text] : input) {
        bool cur_in_p =
            std::find_if(tags.begin(), tags.end(), tag_is_p) != tags.end();
        bool new_paragraph = cur_in_p && !prev_in_p;
        prev_in_p = cur_in_p;

        std::string normalized_text;
        for (char ch : text) {
            if (std::isspace(static_cast<unsigned char>(ch))) {
                if (!in_space) {
                    normalized_text += ' ';
                    in_space = true;
                }
            } else {
                normalized_text += ch;
                in_space = false;
            }
        }

        if (new_paragraph && !collapsed.empty()) {
            normalized_text = '\n' + normalized_text;
        }
        if (!normalized_text.empty()) {
            collapsed.emplace_back(tags, normalized_text);
        }
    }

    return collapsed;
}

static std::string normalize_ff_xml_tag(const std::string& raw_tag) {
    auto [tag_name, tag_value] = ff::utils::parse_tag(raw_tag);
    if (tag_value == "set") {
        return tag_name;
    }

    return tag_name + "|" + tag_value;
}

bool ff_deserialize_html(const Glib::RefPtr<Gtk::TextBuffer>& content_buffer,
                         Gtk::TextBuffer::iterator& iter, const guint8* data,
                         gsize length, bool create_tags) {
    if (data == nullptr || length == 0) return true;
    if (data[length - 1] == '\0') length -= 1;

    std::string html((const char*)data, length);
    std::istringstream html_stream(html);
    ff::utils::ParsedRichText parsed =
        collapse_html_spaces(ff::utils::parse_xml_stream(html_stream));

    Glib::RefPtr<Gtk::TextBuffer> target_buffer = iter.get_buffer();
    auto tag_table = target_buffer->get_tag_table();
    auto bold_tag =
        tag_table ? tag_table->lookup("bold") : Glib::RefPtr<Gtk::TextTag>();
    auto italic_tag =
        tag_table ? tag_table->lookup("italic") : Glib::RefPtr<Gtk::TextTag>();

    Glib::RefPtr<Gtk::TextBuffer::Mark> cursor_mark =
        target_buffer->create_mark(iter, false);

    for (const auto& [current_tags, text] : parsed) {
        if (text.empty()) continue;

        // Skip the <head> tag, which contains styles and no actual text.
        bool has_head = std::find(current_tags.begin(), current_tags.end(),
                                  "head") != current_tags.end();
        if (has_head) continue;

        Glib::RefPtr<Gtk::TextBuffer::Mark> start_mark =
            target_buffer->create_mark(cursor_mark->get_iter());
        target_buffer->insert(cursor_mark->get_iter(), text);

        bool has_b = std::find(current_tags.begin(), current_tags.end(), "b") !=
                     current_tags.end();
        bool has_i = std::find(current_tags.begin(), current_tags.end(), "i") !=
                     current_tags.end();
        if (has_b && bold_tag) {
            target_buffer->apply_tag(bold_tag, start_mark->get_iter(),
                                     cursor_mark->get_iter());
        }
        if (has_i && italic_tag) {
            target_buffer->apply_tag(italic_tag, start_mark->get_iter(),
                                     cursor_mark->get_iter());
        }
        target_buffer->delete_mark(start_mark);
    }

    return true;
}

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

void dump_tag(Glib::ustring& unicode_buffer, const Glib::ustring& tag_name,
              bool opening) {
    // By convention, TextBuffer::Tag name may come in the format
    // "tag_name|tag_value". This format should be dumped as <tag_name
    // value="tag_value">.
    std::string name, value;
    size_t delim = tag_name.find('|');
    if (delim != std::string::npos) {
        name = tag_name.substr(0, delim);
        value = tag_name.substr(delim + 1);
    } else {
        name = tag_name;
    }

    unicode_buffer.push_back('<');
    if (!opening) {
        unicode_buffer.push_back('/');
    }
    unicode_buffer += name;
    if (opening && !value.empty()) {
        unicode_buffer += " value=\"" + value + '\"';
    }
    unicode_buffer.push_back('>');
}

guint8* ff_xml_serialize(const Glib::RefPtr<Gtk::TextBuffer>& content_buffer,
                         const Gtk::TextBuffer::iterator& start,
                         const Gtk::TextBuffer::iterator& end, gsize& length) {
    Glib::ustring unicode_buffer;

    // Gtk::TextBuffer doesn't enforce nested ranges, so the sequence
    // "aa<bold>bc<italic>dd</bold>efg</italic>hi" is perfectly valid. We will
    // use the tag stack to normalize opening and closing tags to follow XML
    // convention.
    std::stack<std::string> open_tags;

    dump_tag(unicode_buffer, "ff_root", true);

    for (auto it = start; it != end; ++it) {
        // Retrieve closing tags
        std::vector<Glib::RefPtr<Gtk::TextTag>> closing_tags =
            it.get_toggled_tags(false);

        // Try to close the tags in the reverse order of opening
        bool closing_tag_found = true;
        std::stack<std::string> temporarily_closed_tags;

        while (!closing_tags.empty() && !open_tags.empty()) {
            const std::string& last_open_tag = open_tags.top();
            auto tag_it = std::find_if(
                closing_tags.begin(), closing_tags.end(),
                [last_open_tag](Glib::RefPtr<const Gtk::TextTag> closing_tag) {
                    return closing_tag->property_name() == last_open_tag;
                });
            if (tag_it == closing_tags.end()) {
                // Closing tag is conflicting with the open tags stack
                temporarily_closed_tags.push(last_open_tag);
            } else {
                // Closing tag correctly corresponds to the latest open tag
                closing_tags.erase(tag_it);
            }
            dump_tag(unicode_buffer, last_open_tag, false);
            open_tags.pop();
        }

        if (!closing_tags.empty()) {
            std::cerr << "TextBuffer corruption: some closing tags haven't "
                         "been opened."
                      << std::endl;
        }

        // Reopen the tags which were temporarily closed to resolve conflicts.
        while (!temporarily_closed_tags.empty()) {
            const std::string& tag_name = temporarily_closed_tags.top();
            open_tags.push(tag_name);
            dump_tag(unicode_buffer, tag_name, true);
            temporarily_closed_tags.pop();
        }

        // Dump opening tags
        std::vector<Glib::RefPtr<Gtk::TextTag>> opening_tags =
            it.get_toggled_tags(true);

        for (auto tag : opening_tags) {
            Glib::ustring tag_name = tag->property_name();
            dump_tag(unicode_buffer, tag_name, true);
            open_tags.push(tag_name);
        }

        dump_character(unicode_buffer, *it);
    }

    // Dump the remaining closing tags
    while (!open_tags.empty()) {
        dump_tag(unicode_buffer, open_tags.top(), false);
        open_tags.pop();
    }

    dump_tag(unicode_buffer, "ff_root", false);

    length = unicode_buffer.bytes();
    char* utf8_buffer = new char[length + 1];
    std::strcpy(utf8_buffer, unicode_buffer.c_str());

    return (guint8*)utf8_buffer;
}

const std::string RichTextEditor::rich_text_mime_type =
    "application/vnd.fontforge.rich-text+xml";

RichTextEditor::RichTextEditor(const std::vector<double>& pointsizes,
                               const RichTextFontList& font_list) {
    scale_css_provider_ = Gtk::CssProvider::create();
    text_view_.get_style_context()->add_provider(
        scale_css_provider_, GTK_STYLE_PROVIDER_PRIORITY_USER - 1);
    refresh_scale_css();

    size_combo_ = build_size_combo(pointsizes);
    size_combo_->set_tooltip_text(_("Font Size"));

    ClearFormattingButton* clear_button =
        Gtk::make_managed<ClearFormattingButton>(text_view_.get_buffer());
    clear_button->set_icon_name("edit-clear-all");
    clear_button->set_tooltip_text(_("Clear Formatting"));
    Gtk::ToolButton* hamburger_button = build_tools_menu();

    toolbar_ = build_toolbar(font_list);
    toolbar_->append(*size_combo_);
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

    text_view_.get_buffer()->register_serialize_format(rich_text_mime_type,
                                                       &ff_xml_serialize);
    text_view_.get_buffer()->register_deserialize_format("text/html",
                                                         &ff_deserialize_html);

    scrolled_.add(text_view_);
    attach(*toolbar_, 0, 0);
    attach(scrolled_, 0, 1);
}

void RichTextEditor::load_buffer(std::istream& istream) {
    ff::utils::ParsedRichText parsed = ff::utils::parse_xml_stream(istream);

    Glib::RefPtr<Gtk::TextBuffer> buffer = text_view_.get_buffer();
    if (!buffer) {
        return;
    }

    buffer->set_text("");
    auto insert_it = buffer->begin();
    auto cursor_mark = buffer->create_mark(insert_it, false);
    auto tag_table = buffer->get_tag_table();

    for (const auto& [raw_tags, text] : parsed) {
        if (text.empty()) {
            continue;
        }

        auto start_mark = buffer->create_mark(cursor_mark->get_iter());
        buffer->insert(cursor_mark->get_iter(), text);

        for (const std::string& raw_tag : raw_tags) {
            std::string tag_name = normalize_ff_xml_tag(raw_tag);
            if (tag_name == "ff_root") {
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

RichTextEditor::TagComboBox* RichTextEditor::build_slanted_combo() {
    std::string default_id = "slant|normal";
    std::vector<
        std::tuple<std::string /*id*/, std::string /*label*/, Pango::Style>>
        property_vec{
            {"slant|normal", _("Normal"), Pango::STYLE_NORMAL},
            {"slant|oblique", _("Oblique"), Pango::STYLE_OBLIQUE},
            {"slant|italic", _("Italic"), Pango::STYLE_ITALIC},
        };

    std::map<std::string /*id*/, Glib::RefPtr<Gtk::TextTag>> tag_map;
    std::vector<std::pair<std::string /*id*/, std::string /*label*/>> labels;

    for (const auto& [tag_id, label, property] : property_vec) {
        // Create and register tag
        if (tag_id != default_id) {
            auto tag = text_view_.get_buffer()->create_tag(tag_id);
            tag->property_style() = property;
            tag_map[tag_id] = tag;
        }

        labels.emplace_back(tag_id, label);
    }

    return Gtk::make_managed<TagComboBox>(text_view_.get_buffer(), default_id,
                                          tag_map, labels);
}

RichTextEditor::TagComboBox* RichTextEditor::build_stretch_combo() {
    std::string default_id = "width|medium";

    // By convention, TextBuffer::Tag with name e.g. "width|condensed" will
    // be exported to XML tag as <width value="condensed">. Unlike in XML,
    // TextBuffer tags must have unique names.
    std::vector<
        std::tuple<std::string /*id*/, std::string /*label*/, Pango::Stretch>>
        property_vec{
            {"width|ultra-condensed", _("Ultra-Condensed (50%)"),
             Pango::STRETCH_ULTRA_CONDENSED},
            {"width|extra-condensed", _("Extra-Condensed (62.5%)"),
             Pango::STRETCH_EXTRA_CONDENSED},
            {"width|condensed", _("Condensed (75%)"), Pango::STRETCH_CONDENSED},
            {"width|semi-condensed", _("Semi-Condensed (87.5%)"),
             Pango::STRETCH_SEMI_CONDENSED},
            {"width|medium", _("Medium (100%)"), Pango::STRETCH_NORMAL},
            {"width|semi-expanded", _("Semi-Expanded (112.5%)"),
             Pango::STRETCH_SEMI_EXPANDED},
            {"width|expanded", _("Expanded (125%)"), Pango::STRETCH_EXPANDED},
            {"width|extra-expanded", _("Extra-Expanded (150%)"),
             Pango::STRETCH_EXTRA_EXPANDED},
            {"width|ultra-expanded", _("Ultra-Expanded (200%)"),
             Pango::STRETCH_ULTRA_EXPANDED},
        };

    std::map<std::string /*id*/, Glib::RefPtr<Gtk::TextTag>> tag_map;
    std::vector<std::pair<std::string /*id*/, std::string /*label*/>> labels;

    for (const auto& [tag_id, label, property] : property_vec) {
        // Create and register tag
        if (tag_id != default_id) {
            auto tag = text_view_.get_buffer()->create_tag(tag_id);
            tag->property_stretch() = property;
            tag_map[tag_id] = tag;
        }

        labels.emplace_back(tag_id, label);
    }

    return Gtk::make_managed<TagComboBox>(text_view_.get_buffer(), default_id,
                                          tag_map, labels);
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
    // TODO(iorsh): Define tag color when building "font|" tags.
    // UI fonts normally don't have a multitude of weights, so we emulate
    // weights by color instead. The user shall check actual rendering in the
    // preview panel.
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
    // By convention, TextBuffer::Tag with name e.g. "font|New Century
    // Schoolbook" will be exported to XML tag as <font value="New Century
    // Schoolbook">. Unlike in XML, TextBuffer tags must have unique names.
    std::map<std::string /*id*/, Glib::RefPtr<Gtk::TextTag>> tag_map;
    std::vector<std::pair<std::string /*id*/, std::string /*label*/>> labels;
    std::string default_id;

    for (const auto& [font_name, properties] : font_list) {
        std::string tag_id = "font|" + font_name;
        if (default_id.empty()) {
            default_id = tag_id;
        }
        auto tag = text_view_.get_buffer()->create_tag(tag_id);
        tag->property_weight() = properties.weight;
        tag->property_style() = properties.style;
        tag->property_stretch() = properties.stretch;
        tag->property_underline() = properties.underline;
        tag_map[tag_id] = tag;

        labels.emplace_back(tag_id, font_name);
    }

    return Gtk::make_managed<TagComboBox>(text_view_.get_buffer(), default_id,
                                          tag_map, labels);
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

Gtk::Toolbar* RichTextEditor::build_toolbar(const RichTextFontList& font_list) {
    fonts_combo_ = build_fonts_combo(font_list);
    fonts_combo_->set_tooltip_text(_("Font"));

    auto bold_tag = text_view_.get_buffer()->create_tag("bold");
    bold_tag->property_weight() = 700;

    // TODO(iorsh): Pass actual available weight values.
    bold_button_ = Gtk::manage(new TogglePropButton(
        text_view_.get_buffer(), &Gtk::TextTag::property_weight,
        {(int)Pango::WEIGHT_LIGHT, (int)Pango::WEIGHT_SEMIBOLD}));
    bold_button_->set_icon_name("format-text-bold");
    bold_button_->set_tooltip_text(_("Bold"));

    auto italic_tag = text_view_.get_buffer()->create_tag("italic");
    italic_tag->property_style() = Pango::STYLE_ITALIC;

    italic_button_ = Gtk::manage(new TogglePropButton(
        text_view_.get_buffer(), &Gtk::TextTag::property_style,
        {Pango::STYLE_NORMAL, Pango::STYLE_ITALIC}));
    italic_button_->set_icon_name("format-text-italic");
    italic_button_->set_tooltip_text(_("Italic"));

    slanted_combo_ = build_slanted_combo();
    slanted_combo_->set_tooltip_text(_("Slant Style"));

    stretch_combo_ = build_stretch_combo();
    stretch_combo_->set_tooltip_text(_("Width Class"));

    weight_combo_ = build_weight_combo();
    weight_combo_->set_tooltip_text(_("Weight Class"));

    Gtk::Toolbar* toolbar = Gtk::make_managed<Gtk::Toolbar>();
    toolbar->append(*fonts_combo_);
    toolbar->append(*bold_button_);
    toolbar->append(*italic_button_);
    toolbar->append(*slanted_combo_);
    toolbar->append(*stretch_combo_);
    toolbar->append(*weight_combo_);
    configure_toolbar(font_list);

    return toolbar;
}

void RichTextEditor::configure_toolbar(const RichTextFontList& font_list) {
    // Configure availablity of weight UI elements.
    std::set<int> unique_weights;
    for (const auto& properties : font_list) {
        unique_weights.insert(properties.second.weight);
    }

    if (unique_weights.size() == 1) {
        bold_button_->set_active(*unique_weights.rbegin() >=
                                 Pango::WEIGHT_SEMIBOLD);
        bold_button_->set_sensitive(false);
        weight_combo_->set_visible_horizontal(false);
    } else if (unique_weights.size() == 2) {
        weight_combo_->set_visible_horizontal(false);
    } else {
        bold_button_->set_visible_horizontal(false);
        weight_combo_->set_enabled_items(unique_weights);
    }

    // Configure availablity of slanted UI elements.
    std::set<Pango::Style> unique_styles;
    for (const auto& properties : font_list) {
        unique_styles.insert(properties.second.style);
    }

    if (unique_styles.size() == 1) {
        italic_button_->set_active(*unique_styles.rbegin() !=
                                   Pango::STYLE_NORMAL);
        italic_button_->set_sensitive(false);
        slanted_combo_->set_visible_horizontal(false);
    } else if (unique_styles.size() == 2 &&
               unique_styles.count(Pango::STYLE_NORMAL) > 0) {
        slanted_combo_->set_visible_horizontal(false);
    } else {
        italic_button_->set_visible_horizontal(false);
        // slanted_combo_->set_enabled_items(unique_styles);
    }

    // Configure availablity of stretched UI element.
    std::set<Pango::Stretch> unique_widths;
    for (const auto& properties : font_list) {
        unique_widths.insert(properties.second.stretch);
    }
    stretch_combo_->set_visible_horizontal(unique_widths.size() > 1);
    // stretch_combo_->set_enabled_items(unique_widths);
}

void RichTextEditor::on_load_buffer_from_xml() {
    Gtk::FileChooserDialog dialog(_("Load sample from XML"),
                                  Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*dynamic_cast<Gtk::Window*>(get_toplevel()));

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    auto filter = Gtk::FileFilter::create();
    filter->set_name(_("FontForge sample text"));
    filter->add_pattern("*.ffxml");
    dialog.add_filter(filter);

    if (dialog.run() != Gtk::RESPONSE_OK) {
        return;
    }

    std::ifstream file(dialog.get_filename(), std::ios::binary);
    if (!file.is_open()) {
        return;
    }

    load_buffer(file);
}

void RichTextEditor::on_save_buffer_to_xml() {
    Gtk::FileChooserDialog dialog(_("Save sample as XML"),
                                  Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*dynamic_cast<Gtk::Window*>(get_toplevel()));

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    auto filter = Gtk::FileFilter::create();
    filter->set_name(_("FontForge sample text"));
    filter->add_pattern("*.ffxml");
    dialog.add_filter(filter);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filepath = dialog.get_filename();
        Glib::RefPtr<Gtk::TextBuffer> buffer = text_view_.get_buffer();

        Gtk::TextBuffer::iterator start = buffer->begin();
        Gtk::TextBuffer::iterator end = buffer->end();

        gsize length = 0;
        guint8* serialized = ff_xml_serialize(buffer, start, end, length);

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
    : text_buffer_(text_buffer), proxy_caller_(proxy_caller), styles_(styles) {
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
    : text_buffer_(text_buffer), proxy_caller_(proxy_caller), labels_(labels) {
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
    : text_buffer_(text_buffer), default_id_(default_id), tag_map_(tag_map) {
    // Add entries to combo box
    for (const auto& [tag_id, label] : labels) {
        combo_box_.append(tag_id, label);
    }

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
    if (property_box()) {
        if (combo_box_.get_active_id() == "") {
            // No tag is selected, nothing to apply.
            return;
        }
        int target_value =
            tag_map_[combo_box_.get_active_id()]->property_weight();

        apply_property_tag(text_buffer_, start, end, target_value,
                           &Gtk::TextTag::property_weight);
    }

    // Remove all other tags from this group, except the new one.
    for (const auto& [tag_id, tag] : tag_map_) {
        if (tag_id != default_id_ && tag_id != combo_box_.get_active_id()) {
            text_buffer_->remove_tag(tag, start, end);
        } else if (tag_id == combo_box_.get_active_id()) {
            text_buffer_->apply_tag(tag, start, end);
        }
    }
}

void RichTextEditor::TagComboBox::on_box_changed() {
    Gtk::TextBuffer::iterator start, end;
    if (text_buffer_->get_selection_bounds(start, end)) {
        apply_tag(start, end);
    }
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
    Glib::RefPtr<Gtk::TextBuffer> text_buffer)
    : text_buffer_(text_buffer) {
    signal_clicked().connect(
        sigc::mem_fun(*this, &ClearFormattingButton::on_button_clicked));
}

void RichTextEditor::ClearFormattingButton::on_button_clicked() {
    Gtk::TextBuffer::iterator start, end;
    if (text_buffer_->get_selection_bounds(start, end)) {
        text_buffer_->remove_all_tags(start, end);
    }
}

}  // namespace ff::widget
