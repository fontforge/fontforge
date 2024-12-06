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
#pragma once

#include <gtkmm.h>

#include "c_context.h"
#include "char_grid.hpp"
#include "dialog.hpp"
#include "i_char_grid_containter.hpp"
#include "widgets/num_entry.hpp"

namespace ff::dlg {

class KerningFormat : public Dialog, public views::ICharGridContainter {
 public:
    KerningFormat(std::shared_ptr<FVContext> context, int width, int height);

    views::CharGrid& get_char_grid(bool second = false) override {
        return second ? char_grid2 : char_grid1;
    }

 private:
    std::shared_ptr<FVContext> fv_context;

    Gtk::RadioButton individual_pairs;

    Gtk::RadioButton matrix_of_classes;
    Gtk::Grid class_options_grid;
    Gtk::CheckButton guess_classes;
    widgets::NumericalEntry intra_class_dist_entry;

    Gtk::Grid general_options_grid;
    widgets::NumericalEntry default_separation;
    widgets::NumericalEntry min_kern;
    Gtk::CheckButton touching;
    Gtk::CheckButton kern_closer;
    Gtk::CheckButton autokern_new;

    Gtk::Frame frame1, frame2;
    views::CharGrid char_grid1;
    views::CharGrid char_grid2;
};

}  // namespace ff::dlg