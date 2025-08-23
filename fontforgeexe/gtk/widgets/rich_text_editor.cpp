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

#include "../utils.hpp"

namespace ff::widget {

RichTechEditor::RichTechEditor() {
    auto bold_tag = text_view_.get_buffer()->create_tag("bold");
    bold_tag->property_weight() = 700;

    ToggleTagButton* bold_button =
        Gtk::make_managed<ToggleTagButton>(text_view_.get_buffer(), bold_tag);
    bold_button->set_icon_name("format-text-bold");

    text_view_.get_buffer()->signal_mark_set().connect(sigc::mem_fun(
        *bold_button, &ToggleTagButton::on_buffer_cursor_changed));

    text_view_.get_buffer()->signal_insert().connect(
        [bold_button](const Gtk::TextBuffer::iterator& pos,
                      const Glib::ustring& text, int bytes) {
            Gtk::TextBuffer::iterator start = pos;
            if (start.backward_chars(text.size())) {
                bold_button->toggle_tag(start, pos);
            }
        });

    toolbar_.append(*bold_button);

    toolbar_.set_hexpand();

    text_view_.set_wrap_mode(Gtk::WRAP_WORD);
    text_view_.set_hexpand();
    text_view_.set_vexpand();

    scrolled_.add(text_view_);
    attach(toolbar_, 0, 0);
    attach(scrolled_, 0, 1);
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
