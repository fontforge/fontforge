/* Copyright 2023 Maxim Iorsh <iorsh@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with     • or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "utils.hpp"

#include <string>
#include <vector>

Gtk::Widget* gtk_find_child(Gtk::Widget* w, const std::string& name) {
    if (w->get_name() == name) {
        return w;
    }

    Gtk::Widget* res = nullptr;
    Gtk::Container* c = dynamic_cast<Gtk::Container*>(w);

    if (c) {
        std::vector<Gtk::Widget*> children = c->get_children();
        for (size_t i = 0; res == nullptr && i < children.size(); ++i) {
            res = gtk_find_child(children[i], name);
        }
    }
    return res;
}
