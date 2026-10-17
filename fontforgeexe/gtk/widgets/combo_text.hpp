/* Copyright (C) 2026 by Maxim Iorsh <iorsh@users@sourceforge.net>
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

namespace ff::widgets {

// Gtk::ComboBoxText with support for disabling individual entries.
class ComboText : public Gtk::ComboBoxText {
 public:
    ComboText() { register_item_sensitivity_callback(); };
    // Don't copy - set_cell_data_func()'s callback will not survive.
    ComboText(const ComboText&) = delete;
    ComboText& operator=(const ComboText&) = delete;

    void set_item_sensitive(const Glib::ustring& item_id, bool sensitive) {
        if (sensitive) {
            item_sensitivity_.erase(item_id);
        } else {
            item_sensitivity_.insert(item_id);
        }
        queue_draw();
    }

    bool get_item_sensitive(const Glib::ustring& item_id) const {
        return item_sensitivity_.count(item_id) == 0;
    }

 private:
    // INSENSITIVE (disabled) items are stored here.
    std::set<Glib::ustring> item_sensitivity_;

    void register_item_sensitivity_callback() {
        auto* renderer = get_first_cell();
        if (!renderer) return;

        set_cell_data_func(
            *renderer,
            [this, renderer](const Gtk::TreeModel::const_iterator& it) {
                // In the Gtk::ComboBoxText, the second column of the TreeModel
                // contains the item id as a string. It's a sacred knowledge, we
                // must not question it.
                Glib::ustring item_id;
                it->get_value(1, item_id);
                renderer->set_sensitive(get_item_sensitive(item_id));
            });
    }
};

}  // namespace ff::widgets
