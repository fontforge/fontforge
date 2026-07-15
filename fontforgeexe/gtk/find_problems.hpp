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
#pragma once

#include "dialog_base.hpp"
#include "find_problems_shim.hpp"

#include <variant>

#include "widgets/num_entry.hpp"

namespace ff::dlg {

class FindProblemsDlg final : public DialogBase {
 private:
    FindProblemsDlg(GWindow parent, const std::vector<ProblemTab>& pr_tabs,
                    double near);

    Gtk::HBox* build_record_box(const ProblemRecord& record,
                                Glib::RefPtr<Gtk::SizeGroup> size_group);
    Gtk::Notebook* build_notebook(const std::vector<ProblemTab>& pr_tabs);

    void set_all_checkboxes(bool state);

    using WidgetMap =
        std::map<short /*cid*/,
                 std::pair<Gtk::CheckButton, widgets::NumericalEntry*>>;
    WidgetMap widget_map_;
    widgets::DoubleEntry near_value_entry_;

 public:
    // Return only problem records ticked by the user, with their respective
    // values.
    static ProblemRecordsOut show(GWindow parent,
                                  const std::vector<ProblemTab>& pr_tabs,
                                  double& near);
};

}  // namespace ff::dlg
