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
#pragma once

#include <variant>
#include <gtkmm.h>

namespace ff::widgets {

using NumericalValue = std::variant<std::monostate, int, double>;

class NumericalEntry : public Gtk::Entry {
 public:
    NumericalEntry();
    virtual NumericalValue get_num_value() const = 0;

 protected:
    virtual bool validate_text(const Glib::ustring& text) const = 0;

 private:
    void validate_text_cb(const Glib::ustring& text, int* position);
};

class IntegerEntry : public NumericalEntry {
 public:
    void set_value(int val);
    int get_value() const;
    NumericalValue get_num_value() const override { return get_value(); };

 private:
    bool validate_text(const Glib::ustring& text) const override;
};

class DoubleEntry : public NumericalEntry {
 public:
    DoubleEntry();

    void set_value(double val);
    double get_value() const;
    NumericalValue get_num_value() const override { return get_value(); };

 private:
    std::string decimal_point_;

    bool validate_text(const Glib::ustring& text) const override;
};

}  // namespace ff::widgets
