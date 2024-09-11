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

#include <string>
#include <utility>
#include <vector>

#include "c_context.h"
#include "menu_builder.hpp"

namespace ff::views {

struct PythonMenuText {
    std::string localized;
    std::string untranslated;
    std::string identifier;
};

struct PythonMenuItem {
    int flags = 0;
    bool divider = false;
    std::vector<PythonMenuText> levels;
    std::string accelerator;
    PyObject *func, *check, *data;
};

// Global storage for menu actions registered with API method
// registerMenuItem(), in the order of registration.
extern std::vector<PythonMenuItem> python_menu_items;

void register_py_menu_item(const PyMenuSpec* spec, const std::string& accel,
                           int flags);

std::vector<MenuInfo> python_tools(const UiContext& ui_context);
bool python_tools_enabled(const UiContext& ui_context);

std::vector<MenuInfo> recent_files(const UiContext& ui_context);

std::vector<MenuInfo> legacy_scripts(const UiContext& ui_context);

std::vector<MenuInfo> top_windows_list(const UiContext& ui_context);

}  // namespace ff::views
