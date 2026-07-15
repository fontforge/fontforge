/* Copyright 2025 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#include <cstring>

namespace ff::widgets {

NumericalEntry::NumericalEntry() {
    signal_insert_text().connect(
        sigc::mem_fun(*this, &NumericalEntry::validate_text_cb), false);
}

void NumericalEntry::validate_text_cb(const Glib::ustring& text,
                                      int* position) {
    // Predict what would be the Gtk::Entry contents after the
    // insertion.
    Glib::ustring current_text = get_text();
    Glib::ustring new_text = current_text;
    new_text.replace(*position, 0, text);

    if (!validate_text(new_text)) {
        signal_insert_text().emission_stop();
    }
}

void IntegerEntry::set_value(int val) { set_text(std::to_string(val)); }

int IntegerEntry::get_value() const {
    Glib::ustring text_val = get_text();
    int val = 0;

    try {
        val = std::stoi(text_val);
    } catch (...) {
        // The user can steer the logic here by typing "-" and then leaving
        // the widget
        val = 0;
    }

    return val;
}

// See also source code of gtk_spin_button_insert_text() in GTK repository.
bool IntegerEntry::validate_text(const Glib::ustring& text) const {
    // Special cases which arise when the user started typing a not yet
    // recognizable number
    if (text == "-") {
        return true;
    }

    // Attempt numeric conversion
    char* end{};
    errno = 0;
    std::strtol(text.c_str(), &end, 10);

    // If the new Gtk::Entry contents can't be converted to a number,
    // the change should be discarded.
    if (end != (text.c_str() + std::strlen(text.c_str())) || errno != 0) {
        errno = 0;
        return false;
    }
    return true;
}

DoubleEntry::DoubleEntry() {
    decimal_point_ = std::string(1, *std::localeconv()->decimal_point);
}

void DoubleEntry::set_value(double val) {
    std::string text_val = std::to_string(val);

    // to_string() formats the number with excessive trailing zeros. We need to
    // strip them.
    size_t last_non_zero = text_val.find_last_not_of('0');

    if (last_non_zero == std::string::npos) {
        // All zeros, which is probably not really possible.
        text_val = "0.0";
    } else {
        // If the non-zero character is period, keep one zero after it
        if (text_val[last_non_zero] == decimal_point_[0]) {
            last_non_zero = std::min(last_non_zero + 1, text_val.size() - 1);
        }
        text_val.erase(last_non_zero + 1);
    }

    set_text(text_val);
}

double DoubleEntry::get_value() const {
    Glib::ustring text_val = get_text();
    double val = 0.0;

    try {
        val = std::stod(text_val);
    } catch (...) {
        // The user can steer the logic here by typing "-" and then leaving
        // the widget
        val = 0.0;
    }

    return val;
}

// See also source code of gtk_spin_button_insert_text() in GTK repository.
bool DoubleEntry::validate_text(const Glib::ustring& text) const {
    // Special cases which arise when the user started typing a not yet
    // recognizable number
    if (text == "-" || text == decimal_point_ || text == "-" + decimal_point_) {
        return true;
    }

    // Attempt numeric conversion
    char* end{};
    errno = 0;
    std::strtod(text.c_str(), &end);

    // If the new Gtk::Entry contents can't be converted to a number,
    // the change should be discarded.
    if (end != (text.c_str() + std::strlen(text.c_str())) || errno != 0) {
        errno = 0;
        return false;
    }
    return true;
}

}  // namespace ff::widgets
