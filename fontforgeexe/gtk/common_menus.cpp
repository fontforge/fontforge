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

#include "common_menus.hpp"

namespace ff::views {

std::vector<PythonMenuItem> python_menu_items;

void register_py_menu_item(const PyMenuSpec* spec, int flags) {
    PythonMenuItem py_menu_item;

    py_menu_item.flags = flags;
    py_menu_item.divider = spec->divider;
    py_menu_item.shortcut = spec->shortcut_str ? spec->shortcut_str : "";
    py_menu_item.func = spec->func;
    py_menu_item.check = spec->check;
    py_menu_item.data = spec->data;

    py_menu_item.levels.resize(spec->depth);
    for (size_t i = 0; i < spec->depth; ++i) {
        const auto& level = spec->levels[i];
        py_menu_item.levels[i].localized =
            level.localized ? level.localized : "";
        py_menu_item.levels[i].untranslated =
            level.untranslated ? level.untranslated : "";
        py_menu_item.levels[i].identifier =
            level.identifier ? level.identifier : "";
    }

    python_menu_items.push_back(std::move(py_menu_item));
}

}  // namespace ff::views