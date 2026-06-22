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

#include "win32_utils.hpp"

#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

extern "C" {
#include "ustring.h"
}

bool is_win32_display() {
    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    if (display) {
        return (strcmp(G_OBJECT_TYPE_NAME(display->gobj()),
                       "GdkWin32Display") == 0);
    }
    return false;
}

Gdk::Rectangle get_win32_print_preview_size() {
    Gdk::Rectangle preview_rectangle;

    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    if (display) {
        Glib::RefPtr<const Gdk::Monitor> monitor =
            display->get_primary_monitor();
        if (monitor) {
            Gdk::Rectangle workarea;
            monitor->get_workarea(workarea);

            // Calculate a rather big roughly centered area of 2/3 screen.
            preview_rectangle = Gdk::Rectangle(
                workarea.get_width() / 6, workarea.get_height() / 6,
                2 * workarea.get_width() / 3, 2 * workarea.get_height() / 3);
            return preview_rectangle;
        }
    }

    // Falback to something reasonable
    preview_rectangle = Gdk::Rectangle(100, 100, 640, 480);
    return preview_rectangle;
}

bool ensure_win32_legacy_print_dialog(GWindow parent) {
#ifdef _WIN32
    static constexpr const wchar_t* kRegPath =
        L"Software\\Microsoft\\Print\\UnifiedPrintDialog";
    static constexpr const wchar_t* kRegValue = L"PreferLegacyPrintDialog";

    // Check current registry value.
    DWORD data = 0;
    DWORD data_size = sizeof(data);
    LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, kRegPath, kRegValue,
                                  RRF_RT_REG_DWORD, nullptr, &data, &data_size);

    // Already enabled — nothing to do.
    if (status == ERROR_SUCCESS && data == 1) {
        return true;
    }

    // Convert wide-char registry constants to UTF-8 for display.
    std::string reg_path_utf8 = std::filesystem::path(kRegPath).string();
    std::string reg_value_utf8 = std::filesystem::path(kRegValue).string();
    std::replace(reg_path_utf8.begin(), reg_path_utf8.end(), '\\', '/');
    std::string reg_key_utf8 = "<tt>HKEY_CURRENT_USER/" + reg_path_utf8 + "/" +
                               reg_value_utf8 + "</tt>";

    char* message = smprintf(
        _("Windows 11 has replaced the legacy print dialog with a modern one "
          "that lacks print preview and some advanced options. Would you like "
          "to restore the legacy print dialog? This change will enable "
          "printing in FontForge and also affect legacy Win32 "
          "applications.\n"
          "This change will set the registry key %s to 1"),
        reg_key_utf8.c_str());

    int response = show_yesno_message_dialog(
        parent, _("Restore Legacy Print Dialog"), message);
    free(message);
    if (response != Gtk::RESPONSE_YES) {
        return false;
    }

    // Write the registry key.
    HKEY hkey;
    status = RegCreateKeyExW(HKEY_CURRENT_USER, kRegPath, 0, nullptr,
                             REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr,
                             &hkey, nullptr);
    if (status != ERROR_SUCCESS) {
        return false;
    }

    DWORD value = 1;
    RegSetValueExW(hkey, kRegValue, 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(hkey);
#endif
    return true;
}
