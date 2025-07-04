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

#include "dialogs.hpp"

#include <gtkmm.h>

#include "application.hpp"
#include "print_preview.hpp"

void print_dialog() {
    // To avoid instability, the GTK application is lazily initialized only when
    // a GTK window is invoked.
    ff::app::GtkApp();

    Glib::RefPtr<Gtk::PrintOperation> print_operation =
        Gtk::PrintOperation::create();

    ff::dlg::PrintPreviewWidget* ff_preview_widget =
        Gtk::make_managed<ff::dlg::PrintPreviewWidget>();

    // The user should be able to select page size and orientation. This is
    // particularly important for printing to PDF.
    print_operation->set_embed_page_setup(true);

    print_operation->set_n_pages(1);
    print_operation->signal_draw_page().connect(
        &ff_preview_widget->draw_page_cb);
    print_operation->set_custom_tab_label(ff_preview_widget->label());
    print_operation->signal_create_custom_widget().connect(
        [ff_preview_widget]() { return ff_preview_widget; });

    ff_preview_widget->update(print_operation->get_default_page_setup(),
                              print_operation->get_print_settings());
    print_operation->signal_update_custom_widget().connect(
        [](Gtk::Widget* widget, const Glib::RefPtr<Gtk::PageSetup>& setup,
           const Glib::RefPtr<Gtk::PrintSettings>& settings) {
            ff::dlg::PrintPreviewWidget* preview =
                dynamic_cast<ff::dlg::PrintPreviewWidget*>(widget);
            if (preview) {
                preview->update(setup, settings);
            };
        });

    print_operation->run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG);
}
