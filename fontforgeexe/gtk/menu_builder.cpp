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

static const char kSignalActivateConn[] = "signal_activate_conn";

Gtk::RadioButtonGroup& get_grouper(RadioGroup g) {
    static std::map<RadioGroup, Gtk::RadioButtonGroup> grouper_map;
    static std::map<RadioGroup, Gtk::RadioMenuItem> dummy_item_map;

    auto& grouper = grouper_map[g];

    // Sometimes none of the radio group items should be checked. GTK doesn't
    // have this capability, so we create a predefined dummy item which absorbs
    // the checked state when no other real item wants to be checked.
    if (!dummy_item_map.count(g)) {
        dummy_item_map[g] = Gtk::RadioMenuItem(grouper, "dummy", false);
    }

    return grouper;
}

void check_menuitem_set_visual_state(Gtk::CheckMenuItem* check_menu_item,
                                     ActivateCB handler, CheckedCB checked_cb,
                                     const UiContext& context) {
    // Disconnect the activation signal before setting visual state
    sigc::connection* conn =
        (sigc::connection*)check_menu_item->get_data(kSignalActivateConn);
    if (conn) {
        conn->disconnect();
        delete conn;
        conn = NULL;
    }

    // Change the state of the CheckMenuItem
    bool is_checked = checked_cb(context);
    check_menu_item->set_active(is_checked);

    // Reconnect the activation signal
    conn = new sigc::connection(check_menu_item->signal_activate().connect(
        [handler, &context]() { handler(context); }));
    check_menu_item->set_data(kSignalActivateConn, conn);
}

Gtk::Menu* build_menu(const std::vector<MenuInfo>& info,
                      const UiContext& context) {
    Gtk::Menu* menu = new Gtk::Menu();
    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    int icon_height = std::max(16, (int)(2 * ui_font_eX_size()));

    // GTK doesn't have any signal that would be fired before the specific
    // subitem is shown. We collect enabled state checks for all subitems and
    // call them one by one from menu's show event.
    std::vector<std::function<void(void)>> enablers;
    std::vector<std::function<void(void)>> checkers;

    for (const auto& item : info) {
        Gtk::MenuItem* menu_item = nullptr;
        if (item.is_separator()) {
            menu_item = Gtk::make_managed<Gtk::SeparatorMenuItem>();
        } else if (item.label.decoration.empty()) {
            menu_item = Gtk::make_managed<Gtk::MenuItem>(item.label.text, true);
        } else if (item.label.decoration.has_group()) {
            RadioGroup group = item.label.decoration.group();
            Gtk::RadioButtonGroup& grouper = get_grouper(group);
            menu_item = Gtk::make_managed<Gtk::RadioMenuItem>(
                grouper, item.label.text, true);
        } else if (item.label.decoration.checkable()) {
            menu_item =
                Gtk::make_managed<Gtk::CheckMenuItem>(item.label.text, true);
        } else {
            Glib::RefPtr<Gdk::Pixbuf> pixbuf =
                load_icon(item.label.decoration.image_file(), icon_height);
            Gtk::Image* img = Gtk::make_managed<Gtk::Image>(pixbuf);
            menu_item = Gtk::make_managed<Gtk::ImageMenuItem>(
                *img, item.label.text, true);
        }

        if (!item.sub_menu.empty()) {
            Gtk::Menu* sub_menu =
                Gtk::manage(build_menu(item.sub_menu, context));
            menu_item->set_submenu(*sub_menu);
        }

        ActivateCB action = item.callbacks.handler
                                ? item.callbacks.handler
                                : context.get_activate_cb(item.mid);

        EnabledCB enabled_check = item.callbacks.enabled
                                      ? item.callbacks.enabled
                                      : context.get_enabled_cb(item.mid);

        // Wrap the check into an action which will be called when menuitem
        // becomes visible as a part of its containing menu.
        auto enabler = [menu_item, enabled_check, &context]() {
            menu_item->set_sensitive(enabled_check(context));
        };
        enablers.push_back(enabler);

        Gtk::CheckMenuItem* check_menu_item =
            dynamic_cast<Gtk::CheckMenuItem*>(menu_item);
        if (check_menu_item) {
            CheckedCB checked_cb = item.callbacks.checked
                                       ? item.callbacks.checked
                                       : context.get_checked_cb(item.mid);

            // Set the state of checkable menu item. This action will be called
            // when the menu item becomes visible as a part of its containing
            // menu.
            auto checker = [check_menu_item, action, checked_cb, &context]() {
                check_menuitem_set_visual_state(check_menu_item, action,
                                                checked_cb, context);
            };
            checkers.push_back(checker);
        } else {
            menu_item->signal_activate().connect(
                [action, &context]() { action(context); });
        }

        menu->append(*menu_item);
    }

    // Just call all the collected menuitem enablers and checkers
    auto on_menu_show = [enablers, checkers]() {
        for (auto e : enablers) {
            e();
        }
        for (auto c : checkers) {
            c();
        }
    };
    menu->signal_show().connect(on_menu_show);

    return menu;
}

Gtk::MenuBar build_menu_bar(const std::vector<MenuInfo>& info,
                            const UiContext& context) {
    Gtk::MenuBar menu_bar;

    for (const auto& item : info) {
        Gtk::MenuItem* menu_item =
            Gtk::make_managed<Gtk::MenuItem>(item.label.text, true);
        menu_bar.append(*menu_item);

        if (!item.sub_menu.empty()) {
            Gtk::Menu* sub_menu =
                Gtk::manage(build_menu(item.sub_menu, context));
            menu_item->set_submenu(*sub_menu);
        }
    }

    return menu_bar;
}

}  // namespace ff::views
