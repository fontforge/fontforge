/* Copyright (C) 2025 by Maxim Iorsh <iorsh@users@sourceforge.net>
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

#include "find_problems.hpp"

#include "intl.h"

namespace ff::dlg {

FindProblemsDlg::FindProblemsDlg(GWindow parent) : Dialog(parent) {
    set_title(_("Find Problems"));
    set_resizable(false);
    // set_help_context("ui/dialogs/problems.html", nullptr);

    auto tabs = Gtk::make_managed<Gtk::Notebook>();
    tabs->append_page(*Gtk::make_managed<Gtk::Label>(_("Dummy")), "Dummy");
    get_content_area()->pack_start(*tabs);

    auto button_box = Gtk::make_managed<Gtk::HBox>();
    auto clear_all_button = Gtk::make_managed<Gtk::Button>(_("Clear All"));
    auto set_all_button = Gtk::make_managed<Gtk::Button>(_("Set All"));
    button_box->pack_start(*clear_all_button, Gtk::PACK_SHRINK);
    button_box->pack_start(*set_all_button, Gtk::PACK_SHRINK);
    get_content_area()->pack_start(*button_box);

    get_content_area()->pack_start(*Gtk::make_managed<Gtk::HSeparator>(),
                                   Gtk::PACK_SHRINK, 5);

    auto near_value_box = Gtk::make_managed<Gtk::HBox>();
    auto near_value_entry = Gtk::make_managed<Gtk::Entry>();
    near_value_box->pack_start(
        *Gtk::make_managed<Gtk::Label>(U_("¹ \"Near\" means within")),
        Gtk::PACK_SHRINK);
    near_value_box->pack_start(*near_value_entry, Gtk::PACK_SHRINK);
    near_value_box->pack_start(*Gtk::make_managed<Gtk::Label>(_("em-units")),
                               Gtk::PACK_SHRINK);
    get_content_area()->pack_start(*near_value_box);

    show_all();
}

Gtk::ResponseType FindProblemsDlg::show(GWindow parent) {
    FindProblemsDlg dialog(parent);

    Gtk::ResponseType result = dialog.run();

    return result;
}

}  // namespace ff::dlg
