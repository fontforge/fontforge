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

#pragma once

#include <variant>

#include <gtkmm.h>

#include "ui_context.hpp"
#include "utils.hpp"

namespace ff::views {

enum DecorType {
    NoDecoration,
    Checkable,
};

enum RadioGroup {
    NoGroup,
    CellWindowSize,
    CellPixelView,
};

// Lazily initialized collection of GTK groupers for radio buttons
Gtk::RadioButtonGroup& get_grouper(RadioGroup g);

class LabelDecoration {
 public:
    LabelDecoration(DecorType s = NoDecoration) : d_(s) {}
    LabelDecoration(const char* image_file) : d_(image_file) {}
    LabelDecoration(RadioGroup g) : d_(g) {}

    bool empty() const {
        return std::holds_alternative<DecorType>(d_) &&
               std::get<DecorType>(d_) == NoDecoration;
    }

    bool checkable() const {
        return std::holds_alternative<DecorType>(d_) &&
               std::get<DecorType>(d_) == Checkable;
    }

    bool named_icon() const { return std::holds_alternative<std::string>(d_); }
    std::string image_file() const {
        return std::holds_alternative<std::string>(d_)
                   ? std::get<std::string>(d_)
                   : "";
    }

    bool has_group() const { return std::holds_alternative<RadioGroup>(d_); }
    RadioGroup group() const { return std::get<RadioGroup>(d_); }

 private:
    std::variant<DecorType, std::string, RadioGroup> d_;
};

struct LabelInfo {
    L10nText text;
    LabelDecoration decoration;
    Glib::ustring
        accelerator;  // See the Gtk::AccelKey constructor for the format
};

static const ActivateCB LegacyAction;
// NOOP callable action
static const ActivateCB NoAction = [](const UiContext&) {};
static const EnabledCB LegacyEnabled;
static const EnabledCB AlwaysEnabled = [](const UiContext&) { return true; };
static const EnabledCB NeverEnabled = [](const UiContext&) { return false; };
static const CheckedCB LegacyChecked;
static const CheckedCB NotCheckable = [](const UiContext&) { return true; };

struct MenuCallbacks {
    ActivateCB handler; /* called on mouse release */
    EnabledCB enabled = AlwaysEnabled;
    CheckedCB checked = NotCheckable;

    // Callback for custom block of menu items
    MenuBlockCB custom_block;
};

static const MenuCallbacks LegacyCallbacks = {LegacyAction, LegacyEnabled,
                                              LegacyChecked};
static const MenuCallbacks SubMenuCallbacks = {NoAction};

struct MenuInfo {
    LabelInfo label;
    std::vector<MenuInfo> sub_menu;

    MenuCallbacks callbacks;

    int mid;

    bool is_separator() const { return label.text == Glib::ustring(); }
    bool is_custom_block() const { return (bool)callbacks.custom_block; }

    static MenuInfo CustomBlock(MenuBlockCB cb) {
        return MenuInfo{
            .label = {""},
            .callbacks = {NoAction, LegacyEnabled, LegacyChecked, cb}};
    }
};

static const MenuInfo kMenuSeparator = {{""}};

Gtk::Menu* build_menu(const std::vector<MenuInfo>& info,
                      const UiContext& context);

Gtk::MenuBar build_menu_bar(const std::vector<MenuInfo>& info,
                            const UiContext& context);

}  // namespace ff::views
