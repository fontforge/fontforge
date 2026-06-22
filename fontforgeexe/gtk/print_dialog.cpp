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

#include "print_dialog.hpp"

#include <gtkmm.h>

#include "application.hpp"
#include "dialog_base.hpp"
#include "layout/cairo_painter.hpp"
#include "print_preview.hpp"
#include "win32_utils.hpp"

using ff::utils::PrintGlyphMap;

void print_dialog(GWindow gw, SplineFont* sf, FontViewBase* fv) {
    // To avoid instability, the GTK application is lazily initialized only when
    // a GTK window is invoked.
    ff::app::GtkApp();

    // On Windows 11 22H2+, offer to restore the legacy print dialog if needed.
    if (!ensure_win32_legacy_print_dialog(gw)) {
        return;
    }

    Glib::RefPtr<Gtk::PrintOperation> print_operation =
        Gtk::PrintOperation::create();

    // The preview widget is also responsible for actual printing, which happens
    // after the print dialog has been closed. We should manage its lifecycle
    // independently.
    ff::utils::CairoPainter cairo_painter(sf, fv);
    ff::dlg::PrintPreviewWidget ff_preview_widget(std::move(cairo_painter));

    // The user should be able to select page size and orientation. This is
    // particularly important for printing to PDF.
    print_operation->set_embed_page_setup(true);

    print_operation->set_use_full_page(true);

    print_operation->set_n_pages(1);
    print_operation->signal_draw_page().connect(sigc::mem_fun(
        ff_preview_widget, &ff::dlg::PrintPreviewWidget::draw_page_cb));
    print_operation->set_custom_tab_label(ff_preview_widget.label());
    print_operation->signal_create_custom_widget().connect(
        [&ff_preview_widget]() { return &ff_preview_widget; });

    ff_preview_widget.update_page_setup(
        print_operation->get_default_page_setup(),
        print_operation->get_print_settings());
    print_operation->signal_update_custom_widget().connect(
        [](Gtk::Widget* widget, const Glib::RefPtr<Gtk::PageSetup>& setup,
           const Glib::RefPtr<Gtk::PrintSettings>& settings) {
            ff::dlg::PrintPreviewWidget* preview =
                dynamic_cast<ff::dlg::PrintPreviewWidget*>(widget);
            if (preview) {
                preview->update_page_setup(setup, settings);
            };
        });
    print_operation->signal_begin_print().connect(
        [print_operation,
         &ff_preview_widget](const Glib::RefPtr<Gtk::PrintContext>& context) {
            // On Win32 backend, signal_update_custom_widget is not emitted
            // while changing printer in the native dialog, so refresh from
            // final settings before pagination/printing.
            ff_preview_widget.update_page_setup(
                context->get_page_setup(),
                print_operation->get_print_settings());

            size_t num_pages = ff_preview_widget.begin_print(context);
            print_operation->set_n_pages((int)num_pages);
        });

    if (!is_win32_display()) {
        // This is a terrible hack to make the print dialog modal to the legacy
        // GDraw window. We create a temporary modal dialog, and run the print
        // operation from it. The print operation will then use this dialog as
        // the transient parent for the native print dialog, making it modal to
        // the GDraw window. After the print operation is done, we exit the
        // temporary dialog. It also doesn't work on Windows.
        //
        // TODO(iorsh): remove this hack after the transition to GTK is
        // complete.
        ff::dlg::DialogBase parent_window(gw);
        parent_window.set_modal();
        parent_window.show_all();
        Glib::signal_idle().connect_once([print_operation, &parent_window]() {
            print_operation->run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                 parent_window);

            // Invoke response and effectively exit the temporary dialog.
            parent_window.response(0);
        });
        parent_window.run();
    } else {
        print_operation->run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG);
    }
}
