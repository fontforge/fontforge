/* Copyright 2025 Maxim Iorsh <iorsh@users.sourceforge.net>
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

namespace ff::widget {

class RichTechEditor : public Gtk::Grid {
 public:
    RichTechEditor();

    static const std::string rich_text_mime_type;

    // TextView accessors
    Glib::RefPtr<Gtk::TextBuffer> get_buffer() {
        return text_view_.get_buffer();
    }
    Gtk::ScrolledWindow& get_scrolled() { return scrolled_; }

    // Special widget class for toggling tags on TextBuffer contents. This
    // widget is responsible for applying classes, such as "bold", "italic" etc.
    // to the TextView buffer.
    //
    // The desired behavior is as follows (using Bold style as example):
    //
    // When a text range is selected, the button in "on" if the entire selected
    // range is Bold. Otherwise the button is "off". When there is no selection,
    // the button state is according the character right before the cursor. So
    // if the character before the cursor is Bold, the button will be "on", and
    // newly typed characters will be Bold too.
    //
    // When the user clicks the button, its state changes. Accordingly, if there
    // was a selection, the selection style changes accordingly. If there was no
    // selection, the newly typed characters would have the style according to
    // the button state.
    class ToggleTagButton : public Gtk::ToggleToolButton {
     public:
        ToggleTagButton(Glib::RefPtr<Gtk::TextBuffer> text_buffer,
                        Glib::RefPtr<Gtk::TextTag> tag);

        void toggle_tag(const Gtk::TextBuffer::iterator& start,
                        const Gtk::TextBuffer::iterator& end);

        // Toggle the current selection, if there is any. We don't want to
        // override Gtk::ToggleToolButton::on_toggled(), we want to be able to
        // disconnect it.
        void on_button_toggled();

        // Set the button state when the buffer cursor or selection changes.
        void on_buffer_cursor_changed(
            const Gtk::TextBuffer::iterator&,
            const Glib::RefPtr<Gtk::TextBuffer::Mark>& mark);

     protected:
        Glib::RefPtr<Gtk::TextBuffer> text_buffer_;
        Glib::RefPtr<Gtk::TextTag> tag_;
    };

 protected:
    Gtk::Toolbar toolbar_;
    Gtk::ScrolledWindow scrolled_;
    Gtk::TextView text_view_;
};

}  // namespace ff::widget
