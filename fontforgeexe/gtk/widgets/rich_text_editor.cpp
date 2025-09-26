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

const std::string RichTechEditor::rich_text_mime_type =
    "application/vnd.fontforge.rich-text+xml";

RichTechEditor::RichTechEditor(const std::vector<double>& pointsizes) {
    auto bold_tag = text_view_.get_buffer()->create_tag("bold");
    bold_tag->property_weight() = 700;

    ToggleTagButton* bold_button =
        Gtk::make_managed<ToggleTagButton>(text_view_.get_buffer(), bold_tag);
    bold_button->set_icon_name("format-text-bold");

    auto italic_tag = text_view_.get_buffer()->create_tag("italic");
    italic_tag->property_style() = Pango::STYLE_ITALIC;

    ToggleTagButton* italic_button =
        Gtk::make_managed<ToggleTagButton>(text_view_.get_buffer(), italic_tag);
    italic_button->set_icon_name("format-text-italic");

    TagComboBox* stretch_combo = build_stretch_combo(text_view_.get_buffer());
    TagComboBox* size_combo =
        build_size_combo(text_view_.get_buffer(), pointsizes);
    TagComboBox* weight_combo = build_weight_combo(text_view_.get_buffer());

    ClearFormattingButton* clear_button =
        Gtk::make_managed<ClearFormattingButton>(text_view_.get_buffer());
    clear_button->set_icon_name("edit-clear-all");

    toolbar_.append(*bold_button);
    toolbar_.append(*italic_button);
    toolbar_.append(*stretch_combo);
    toolbar_.append(*size_combo);
    toolbar_.append(*weight_combo);
    toolbar_.append(*clear_button);

    toolbar_.set_hexpand();

    text_view_.set_wrap_mode(Gtk::WRAP_WORD);
    text_view_.set_hexpand();
    text_view_.set_vexpand();

    auto result = text_view_.get_buffer()->register_serialize_format(
        rich_text_mime_type, &ff_xml_serialize);

    scrolled_.add(text_view_);
    attach(toolbar_, 0, 0);
    attach(scrolled_, 0, 1);
}

RichTechEditor::TagComboBox* RichTechEditor::build_stretch_combo(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer) {
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
            auto tag = text_buffer->create_tag(tag_id);
            tag->property_stretch() = property;
            tag_map[tag_id] = tag;
        }

        labels.emplace_back(tag_id, label);
    }

    return Gtk::make_managed<TagComboBox>(text_view_.get_buffer(), default_id,
                                          tag_map, labels);
}

RichTechEditor::TagComboBox* RichTechEditor::build_size_combo(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer,
    const std::vector<double>& pointsizes) {
    std::string default_id = "size|12";

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

        // Create and register tag
        if (tag_id != default_id) {
            auto tag = text_buffer->create_tag(tag_id);
            tag->property_size_points() = size_pt;
            tag_map[tag_id] = tag;
        }

        labels.emplace_back(tag_id, label);
    }

    return Gtk::make_managed<TagComboBox>(text_view_.get_buffer(), default_id,
                                          tag_map, labels);
}

RichTechEditor::TagComboBox* RichTechEditor::build_weight_combo(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer) {
    std::string default_id = "weight|regular";

    // By convention, TextBuffer::Tag with name e.g. "weight|light" will
    // be exported to XML tag as <weight value="light">. Unlike in XML,
    // TextBuffer tags must have unique names.
    //
    // UI fonts normally don't have a multitude of weights, so we emulate
    // weights by color instead. The user shall check actual rendering in the
    // preview panel.
    std::vector<std::tuple<std::string /*id*/, std::string /*label*/,
                           Pango::Weight, std::string /*color*/>>
        property_vec{
            {"weight|thin", _("100 Thin"), Pango::WEIGHT_NORMAL, "gray"},
            {"weight|extra-light", _("200 Extra-Light"), Pango::WEIGHT_NORMAL,
             "dimgray"},
            {"weight|light", _("300 Light"), Pango::WEIGHT_NORMAL,
             "darkslategray"},
            {"weight|regular", _("400 Regular"), Pango::WEIGHT_NORMAL, "black"},
            {"weight|medium", _("500 Medium"), Pango::WEIGHT_BOLD, "dimgray"},
            {"weight|semi-bold", _("600 Semi-Bold"), Pango::WEIGHT_BOLD,
             "darkslategray"},
            {"weight|bold", _("700 Bold"), Pango::WEIGHT_BOLD, "black"},
            {"weight|extra-bold", _("800 Extra-Bold"), Pango::WEIGHT_BOLD,
             "blue"},
            {"weight|black", _("900 Black"), Pango::WEIGHT_BOLD, "navy"},
        };

    std::map<std::string /*id*/, Glib::RefPtr<Gtk::TextTag>> tag_map;
    std::vector<std::pair<std::string /*id*/, std::string /*label*/>> labels;

    for (const auto& [tag_id, label, weight, color] : property_vec) {
        // Create and register tag
        if (tag_id != default_id) {
            auto tag = text_buffer->create_tag(tag_id);
            tag->property_weight() = weight;
            tag->property_foreground() = color;
            tag_map[tag_id] = tag;
        }

        labels.emplace_back(tag_id, label);
    }

    return Gtk::make_managed<TagComboBox>(text_view_.get_buffer(), default_id,
                                          tag_map, labels);
}

///////////////////////////////////////////////////////////////////////
///               RichTechEditor::ToggleTagButton                   ///
///////////////////////////////////////////////////////////////////////

RichTechEditor::ToggleTagButton::ToggleTagButton(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer, Glib::RefPtr<Gtk::TextTag> tag)
    : text_buffer_(text_buffer), tag_(tag) {
    // Called whenever the selection or the cursor position is changed. Sets the
    // correct visual state of the widget
    text_buffer_->signal_mark_set().connect(
        sigc::mem_fun(*this, &ToggleTagButton::on_buffer_cursor_changed));

    // Called whenever a character is typed into the buffer. Set the tag on this
    // character according to the widget state.
    text_buffer_->signal_insert().connect(
        [this](const Gtk::TextBuffer::iterator& pos, const Glib::ustring& text,
               int bytes) {
            Gtk::TextBuffer::iterator start = pos;
            if (start.backward_chars(text.size())) {
                toggle_tag(start, pos);
            }
        });
}

void RichTechEditor::ToggleTagButton::toggle_tag(
    const Gtk::TextBuffer::iterator& start,
    const Gtk::TextBuffer::iterator& end) {
    if (get_active()) {
        text_buffer_->apply_tag(tag_, start, end);
    } else {
        text_buffer_->remove_tag(tag_, start, end);
    }
}

void RichTechEditor::ToggleTagButton::on_button_toggled() {
    Gtk::TextBuffer::iterator start, end;
    if (text_buffer_->get_selection_bounds(start, end)) {
        toggle_tag(start, end);
    }
}

void RichTechEditor::ToggleTagButton::on_buffer_cursor_changed(
    const Gtk::TextBuffer::iterator&,
    const Glib::RefPtr<Gtk::TextBuffer::Mark>& mark) {
    if (mark->get_name() != "insert") {
        return;
    }

    Gtk::TextBuffer::iterator start, end;
    bool button_active = false;

    if (!text_buffer_->get_selection_bounds(start, end)) {
        start--;
    }

    if (start.has_tag(tag_) && start.forward_to_tag_toggle(tag_)) {
        if (start >= end) {
            button_active = true;
        }
    }

    ui_utils::gtk_set_widget_state_without_event(
        (Gtk::ToggleToolButton*)this, &ToggleTagButton::signal_toggled,
        sigc::mem_fun(*this, &ToggleTagButton::on_button_toggled),
        [this, button_active]() { set_active(button_active); });
}

///////////////////////////////////////////////////////////////////////
///                 RichTechEditor::TagComboBox                     ///
///////////////////////////////////////////////////////////////////////

RichTechEditor::TagComboBox::TagComboBox(
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

void RichTechEditor::TagComboBox::apply_tag(
    const Gtk::TextBuffer::iterator& start,
    const Gtk::TextBuffer::iterator& end) {
    // Remove all other tags from this group, except the new one.
    for (const auto& [tag_id, tag] : tag_map_) {
        if (tag_id != default_id_ && tag_id != combo_box_.get_active_id()) {
            text_buffer_->remove_tag(tag, start, end);
        } else if (tag_id == combo_box_.get_active_id()) {
            text_buffer_->apply_tag(tag, start, end);
        }
    }
}

void RichTechEditor::TagComboBox::on_box_changed() {
    Gtk::TextBuffer::iterator start, end;
    if (text_buffer_->get_selection_bounds(start, end)) {
        apply_tag(start, end);
    }
}

std::string RichTechEditor::TagComboBox::get_active_tag(
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

void RichTechEditor::TagComboBox::on_buffer_cursor_changed(
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
///             RichTechEditor::ClearFormattingButton               ///
///////////////////////////////////////////////////////////////////////

RichTechEditor::ClearFormattingButton::ClearFormattingButton(
    Glib::RefPtr<Gtk::TextBuffer> text_buffer)
    : text_buffer_(text_buffer) {
    signal_clicked().connect(
        sigc::mem_fun(*this, &ClearFormattingButton::on_button_clicked));
}

void RichTechEditor::ClearFormattingButton::on_button_clicked() {
    Gtk::TextBuffer::iterator start, end;
    if (text_buffer_->get_selection_bounds(start, end)) {
        text_buffer_->remove_all_tags(start, end);
    }
}

}  // namespace ff::widget
