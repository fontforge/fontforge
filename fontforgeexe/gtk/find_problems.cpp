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

FindProblemsDlg::FindProblemsDlg(GWindow parent,
                                 const std::vector<ProblemTab>& pr_tabs)
    : Dialog(parent) {
    set_title(_("Find Problems"));
    set_resizable(false);
    // set_help_context("ui/dialogs/problems.html", nullptr);

    auto tabs = build_notebook(pr_tabs);
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
    near_value_entry->set_width_chars(6);
    near_value_box->pack_start(
        *Gtk::make_managed<Gtk::Label>(U_("¹ \"Near\" means within")),
        Gtk::PACK_SHRINK);
    near_value_box->pack_start(*near_value_entry, Gtk::PACK_SHRINK);
    near_value_box->pack_start(*Gtk::make_managed<Gtk::Label>(_("em-units")),
                               Gtk::PACK_SHRINK);
    get_content_area()->pack_start(*near_value_box);

    show_all();
}

Gtk::Notebook* FindProblemsDlg::build_notebook(
    const std::vector<ProblemTab>& pr_tabs) const {
    auto tabs = Gtk::make_managed<Gtk::Notebook>();

    for (const ProblemTab& tab : pr_tabs) {
        auto record_page = Gtk::make_managed<Gtk::VBox>();
        for (const ProblemRecord& record : tab.records) {
            auto record_box = Gtk::make_managed<Gtk::HBox>();

            auto record_check =
                Gtk::make_managed<Gtk::CheckButton>(record.label, true);
            record_check->set_tooltip_text(record.tooltip);
            record_check->set_active(record.active);
            record_box->pack_start(*record_check, Gtk::PACK_SHRINK);

            if (!std::holds_alternative<std::monostate>(record.value)) {
                auto record_entry = Gtk::make_managed<Gtk::Entry>();
                record_entry->set_width_chars(6);
                if (std::holds_alternative<int>(record.value)) {
                    record_entry->set_text(
                        std::to_string(std::get<int>(record.value)));
                } else {
                    record_entry->set_text(
                        std::to_string(std::get<double>(record.value)));
                }
                record_box->pack_start(*record_entry, Gtk::PACK_SHRINK);
            }

            record_page->pack_start(*record_box, Gtk::PACK_SHRINK);
        }
        tabs->append_page(*record_page, tab.label);
    }
    return tabs;
}

Gtk::ResponseType FindProblemsDlg::show(
    GWindow parent, const std::vector<ProblemTab>& pr_tabs) {
    FindProblemsDlg dialog(parent, pr_tabs);

    Gtk::ResponseType result = dialog.run();

    return result;
}

}  // namespace ff::dlg
