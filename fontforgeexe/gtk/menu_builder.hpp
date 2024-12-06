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

#include <gtkmm.h>

#include "utils.hpp"

typedef struct fontview_context FVContext;

namespace ff::views {

class LabelDecoration {
 public:
    LabelDecoration(const char* image_file = "") : image_file_(image_file) {}

    std::string image_file() const { return image_file_; }

 private:
    std::string image_file_;
};

struct LabelInfo {
    L10nText text;
    LabelDecoration decoration;
    Glib::ustring
        accelerator;  // See the Gtk::AccelKey constructor for the format
};

using ActivateCB = std::function<void(const FVContext&)>;

static const ActivateCB NoAction = [](const FVContext&) {
};  // NOOP callable action

struct MenuInfo {
    LabelInfo label;
    std::vector<MenuInfo> sub_menu;

    ActivateCB handler; /* called on mouse release */

    int mid;
};

static const MenuInfo kMenuSeparator = {{""}};

Gtk::Menu* build_menu(const std::vector<MenuInfo>& info,
                      const FVContext& context);

}  // namespace ff::views