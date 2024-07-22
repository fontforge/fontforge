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
static const char kDynamicMenuItemData[] = "dynamic_menu_item";

Gtk::MenuItem* menu_item_factory(const MenuInfo& item, const UiContext& context,
                                 int icon_height);

// TODO: get grouper by group id and window id. Current implementation
// might break if there is more than one window.
Gtk::RadioButtonGroup& get_grouper(RadioGroup g) {
    static std::map<RadioGroup, Gtk::RadioButtonGroup> grouper_map;

    return grouper_map[g];
}

// Sometimes none of the radio group items should be checked. GTK doesn't
// have this capability, so we create a predefined dummy item which absorbs
// the checked state when no other real item wants to be checked.
Gtk::RadioMenuItem& get_dummy_radio_item(RadioGroup g) {
    static std::map<RadioGroup, Gtk::RadioMenuItem> dummy_item_map;

    if (!dummy_item_map.count(g)) {
        Gtk::RadioButtonGroup& grouper = get_grouper(g);
        dummy_item_map[g] = Gtk::RadioMenuItem(grouper, "dummy", false);
    }

    return dummy_item_map[g];
}

std::function<void(void)> build_action(Gtk::MenuItem* menu_item,
                                       ActivateCB handler,
                                       const UiContext& context) {
    Gtk::RadioMenuItem* radio_menu_item =
        dynamic_cast<Gtk::RadioMenuItem*>(menu_item);
    std::function<void(void)> action;
    if (radio_menu_item) {
        // All check items are activated when they become either checked or
        // unchecked. Radio items should be activated only when they become
        // selected, not when they lose selection.
        action = [radio_menu_item, handler, &context]() {
            if (radio_menu_item->get_active()) {
                handler(context);
            }
        };
    } else {
        action = [handler, &context]() { handler(context); };
    }

    return action;
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
    std::function<void(void)> action =
        build_action(check_menu_item, handler, context);
    conn = new sigc::connection(
        check_menu_item->signal_activate().connect(action));
    check_menu_item->set_data(kSignalActivateConn, conn);
}

void clean_dynamic_items(Gtk::Menu* menu) {
    // Clear existing dynamic menu items
    menu->foreach ([menu](Gtk::Widget& w) {
        if (w.get_data(kDynamicMenuItemData)) {
            menu->remove(w);
        }
    });
}

class BlockBuilder {
 public:
    BlockBuilder(Gtk::Menu* menu, int stable_position,
                 MenuBlockCB block_provider, const UiContext& context,
                 int icon_height)
        : menu_(menu),
          position_(stable_position),
          block_provider_(block_provider),
          context_(context),
          icon_height_(icon_height) {}

    int get_real_position() const {
        auto widgets = menu_->get_children();
        int i = 0, stable_count = 0;
        for (; i < widgets.size(); ++i) {
            if (stable_count == position_) {
                break;
            }
            if (!widgets[i]->get_data(kDynamicMenuItemData)) {
                ++stable_count;
            }
        }
        return i;
    }

    void operator()() {
        std::vector<MenuInfo> info = block_provider_(context_);
        int position = get_real_position();

        for (const auto& item : info) {
            Gtk::MenuItem* menu_item =
                menu_item_factory(item, context_, icon_height_);

            ActivateCB handler = item.callbacks.handler
                                     ? item.callbacks.handler
                                     : context_.get_activate_cb(item.mid);
            std::function<void(void)> action =
                build_action(menu_item, handler, context_);
            menu_item->signal_activate().connect(action);

            EnabledCB enabled_check = item.callbacks.enabled
                                          ? item.callbacks.enabled
                                          : context_.get_enabled_cb(item.mid);
            menu_item->set_sensitive(enabled_check(context_));

            Gtk::CheckMenuItem* check_menu_item =
                dynamic_cast<Gtk::CheckMenuItem*>(menu_item);
            if (check_menu_item) {
                CheckedCB checked_cb = item.callbacks.checked
                                           ? item.callbacks.checked
                                           : context_.get_checked_cb(item.mid);

                // Gtk::Widget::set_state_flags() allows us to set visual
                // item state without triggering its action.
                check_menu_item->set_state_flags(checked_cb(context_)
                                                     ? Gtk::STATE_FLAG_CHECKED
                                                     : Gtk::STATE_FLAG_NORMAL);
            }

            menu_item->set_data(kDynamicMenuItemData, (void*)1);
            menu_item->show_all();

            menu_->insert(*menu_item, position++);
        }
    }

 private:
    Gtk::Menu* menu_;
    // Number of *static* menu items before the dynamic block
    int position_;
    MenuBlockCB block_provider_;
    const UiContext& context_;
    int icon_height_;
};

Gtk::MenuItem* menu_item_factory(const MenuInfo& item, const UiContext& context,
                                 int icon_height) {
    Gtk::MenuItem* menu_item = nullptr;
    if (item.is_separator()) {
        menu_item = new Gtk::SeparatorMenuItem();
    } else if (item.label.decoration.empty()) {
        menu_item = new Gtk::MenuItem(item.label.text, true);
    } else if (item.label.decoration.has_group()) {
        RadioGroup group = item.label.decoration.group();
        Gtk::RadioButtonGroup& grouper = get_grouper(group);
        menu_item = new Gtk::RadioMenuItem(grouper, item.label.text, true);
    } else if (item.label.decoration.checkable()) {
        menu_item = new Gtk::CheckMenuItem(item.label.text, true);
    } else {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf =
            load_icon(item.label.decoration.image_file(), icon_height);
        Gtk::Image* img = Gtk::make_managed<Gtk::Image>(pixbuf);
        menu_item = new Gtk::ImageMenuItem(*img, item.label.text, true);
    }

    if (!item.label.accelerator.empty()) {
        Gtk::AccelKey key(item.label.accelerator);
        menu_item->add_accelerator("activate", context.get_accel_group(),
                                   key.get_key(), key.get_mod(),
                                   Gtk::ACCEL_VISIBLE);
    }

    if (!item.sub_menu.empty()) {
        Gtk::Menu* sub_menu = Gtk::manage(build_menu(item.sub_menu, context));
        menu_item->set_submenu(*sub_menu);
    }

    return menu_item;
}

Gtk::Menu* build_menu(const std::vector<MenuInfo>& info,
                      const UiContext& context) {
    Gtk::Menu* menu = new Gtk::Menu();
    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    int icon_height = std::max(16, (int)(2 * ui_font_eX_size()));
    int stable_position = 0;

    // GTK doesn't have any signal that would be fired before the specific
    // subitem is shown. We collect enabled state checks for all subitems and
    // call them one by one from menu's show event.
    std::vector<std::function<void(void)>> enablers;
    std::vector<std::function<void(void)>> checkers;
    std::vector<std::function<void(void)>> block_builders;

    // Collect group identifiers for this submenu, so that we can later access
    // their dummy items.
    std::set<RadioGroup> radio_info;

    for (const auto& item : info) {
        if (item.is_custom_block()) {
            block_builders.push_back(BlockBuilder(menu, stable_position,
                                                  item.callbacks.custom_block,
                                                  context, icon_height));
            continue;
        }

        Gtk::MenuItem* menu_item =
            Gtk::manage(menu_item_factory(item, context, icon_height));

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

            // Initially connect action to checkable state to support keyboard
            // accelerators.
            check_menuitem_set_visual_state(check_menu_item, action, checked_cb,
                                            context);

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

        if (item.label.decoration.has_group()) {
            radio_info.insert(item.label.decoration.group());
        }

        menu->append(*menu_item);
        ++stable_position;
    }

    // Activate all dummy radio items for this submenu first. If an actual radio
    // item wants to be active, it would be activated by one of the checkers.
    // Note that checkers must be called after radio_chekers.
    std::vector<std::function<void(void)>> dummy_radio_checkers;
    for (RadioGroup group : radio_info) {
        dummy_radio_checkers.push_back(
            [group]() { get_dummy_radio_item(group).set_active(); });
    }

    // Just call all the collected menuitem enablers and checkers
    auto on_menu_show = [menu, enablers, dummy_radio_checkers, checkers,
                         block_builders]() {
        for (auto e : enablers) {
            e();
        }
        for (auto r : dummy_radio_checkers) {
            r();
        }
        for (auto c : checkers) {
            c();
        }
        clean_dynamic_items(menu);
        for (auto b : block_builders) {
            b();
        }
    };
    menu->signal_show().connect(on_menu_show);

    // Enable all menu items when the menu is hidden, to ensure that keyboard
    // shortcuts can be always activated.
    auto on_menu_hide = [menu]() {
        menu->foreach ([](Gtk::Widget& w) { w.set_sensitive(true); });
    };
    menu->signal_hide().connect(on_menu_hide);

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
