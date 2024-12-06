/* Copyright 2024 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#include "num_entry.hpp"
#include "../utils.hpp"

namespace ff::widgets {

NumericalEntry::NumericalEntry(const Glib::ustring& label_, bool mnemonic)
    : label(label_, mnemonic) {
    // 1em offset between label and entry widget
    label.set_margin_end(ui_font_em_size());
    if (mnemonic) {
        label.set_mnemonic_widget(entry);
    }

    entry.set_input_purpose(Gtk::INPUT_PURPOSE_NUMBER);
    entry.set_width_chars(5);

    pack_start(label, Gtk::PACK_SHRINK);
    pack_start(entry);
}

void NumericalEntry::set_value(double value) {
    std::string str = std::to_string(value);
    // Strip trailing zeros and dot
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    str.erase(str.find_last_not_of('.') + 1, std::string::npos);
    entry.set_text(str);
}

double NumericalEntry::get_value() {
    return strtod(entry.get_text().c_str(), nullptr);
}

}  // namespace ff::widgets
