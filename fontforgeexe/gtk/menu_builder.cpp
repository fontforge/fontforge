/* Copyright 2024 Maxim Iorsh <iorsh@users.sourceforge.net>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

The name of the author may not be used to endorse or promote
products derived from this software without specific prior written
permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include "menu_builder.hpp"

namespace ff::views {

Gtk::Menu* build_menu(const std::vector<MenuInfo>& info,
                      const FVContext& context) {
    Gtk::Menu* menu = new Gtk::Menu();
    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    int icon_height = std::max(16, (int)(2 * ui_font_eX_size()));

    for (const auto& item : info) {
        Gtk::MenuItem* menu_item = nullptr;
        if (item.is_separator()) {
            menu_item = Gtk::make_managed<Gtk::SeparatorMenuItem>();
        } else if (item.label.decoration.image_file().empty()) {
            menu_item = Gtk::make_managed<Gtk::MenuItem>(item.label.text, true);
        } else {
            Glib::RefPtr<Gdk::Pixbuf> pixbuf =
                load_icon(item.label.decoration.image_file(), icon_height);
            Gtk::Image* img = new Gtk::Image(pixbuf);
            menu_item = Gtk::make_managed<Gtk::ImageMenuItem>(
                *img, item.label.text, true);
        }
        menu->append(*menu_item);
    }

    return menu;
}

}  // namespace ff::views