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

#include "dialog.hpp"

#include <variant>

namespace ff::dlg {

using ProblemRecordValue = std::variant<std::monostate, int, double>;

struct ProblemRecord {
    std::string label;
    std::string tooltip;
    bool active;
    ProblemRecordValue value;
};
struct ProblemTab {
    std::string label;
    std::vector<ProblemRecord> records;
};

class FindProblemsDlg final : public Dialog {
 private:
    FindProblemsDlg(GWindow parent, const std::vector<ProblemTab>& pr_tabs);

    Gtk::Notebook* build_notebook(const std::vector<ProblemTab>& pr_tabs) const;

 public:
    static Gtk::ResponseType show(GWindow parent,
                                  const std::vector<ProblemTab>& pr_tabs);
};

}  // namespace ff::dlg
