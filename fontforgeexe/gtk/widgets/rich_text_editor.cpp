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

extern "C" {
#include "ustring.h"
}

#include "../utils.hpp"
#include <iostream>

namespace ff::widget {

void dump_character(std::vector<gunichar>& unicode_buffer,
                    const gunichar& character) {
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
        unicode_buffer.push_back(character);
    } else {
        for (const gunichar& c : seq) {
            unicode_buffer.push_back(c);
        }
    }
}

void dump_tag(std::vector<gunichar>& unicode_buffer,
              const Glib::ustring& tag_name, bool opening) {
    unicode_buffer.push_back('<');
    if (!opening) {
        unicode_buffer.push_back('/');
    }
    for (const gunichar& c : tag_name) {
        unicode_buffer.push_back(c);
    }
    unicode_buffer.push_back('>');
}

guint8* ff_xml_serialize(const Glib::RefPtr<Gtk::TextBuffer>& content_buffer,
                         const Gtk::TextBuffer::iterator& start,
                         const Gtk::TextBuffer::iterator& end, gsize& length) {
    std::vector<gunichar> unicode_buffer;

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
            auto it = std::find_if(
                closing_tags.begin(), closing_tags.end(),
                [last_open_tag](Glib::RefPtr<const Gtk::TextTag> closing_tag) {
                    return closing_tag->property_name() == last_open_tag;
                });
            if (it == closing_tags.end()) {
                // Closing tag is conflicting with the open tags stack
                temporarily_closed_tags.push(last_open_tag);
                open_tags.pop();
            } else {
                // Closing tag correctly corresponds to the latest open tag
                open_tags.pop();
                closing_tags.erase(it);
            }
            dump_tag(unicode_buffer, last_open_tag, false);
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
    unicode_buffer.push_back('\0');

    char* utf8_buffer = u2utf8_copy(unicode_buffer.data());
    length = strlen(utf8_buffer);

    return (guint8*)utf8_buffer;
}

const std::string RichTechEditor::rich_text_mime_type =
    "application/vnd.fontforge.rich-text+xml";

RichTechEditor::RichTechEditor() {
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

    toolbar_.append(*bold_button);
    toolbar_.append(*italic_button);

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

}  // namespace ff::widget
