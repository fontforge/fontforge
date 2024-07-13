/* Copyright 2023 Maxim Iorsh <iorsh@users.sourceforge.net>
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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// forward declare PyObject
// per http://mail.python.org/pipermail/python-dev/2003-August/037601.html
#ifndef PyObject_HEAD
typedef struct _object PyObject;
#endif

// C structures and callbacks for interacting with legacy code
typedef struct gwindow* GWindow;
typedef struct fontview FontView;
typedef struct fontviewbase FontViewBase;
typedef struct bdffont BDFFont;
typedef struct anchorclass AnchorClass;
typedef struct splinefont SplineFont;

enum glyphlabel { gl_glyph, gl_name, gl_unicode, gl_encoding };

typedef uint32_t Color;

enum merge_type {
    mt_set = 0,
    mt_merge = 4,
    mt_or = mt_merge,
    mt_restrict = 8,
    mt_and = 12
};

typedef struct fv_menu_action {
    int mid;
    bool (*is_disabled)(FontView* fv, int mid); /* called before showing */
    bool (*is_checked)(FontView* fv, int mid);  /* called before showing */
    void (*action)(FontView* fv, int mid);      /* called on mouse release */
} FVMenuAction;

#define MENUACTION_LAST \
    { 0, NULL, NULL, NULL }

typedef struct fv_select_menu_action {
    int mid;
    void (*action)(FontView* fv,
                   enum merge_type merge); /* called on mouse release */
} FVSelectMenuAction;

#define MENU_SELACTION_LAST \
    { 0, NULL }

typedef struct bitmap_menu_data {
    BDFFont* bdf;
    int16_t pixelsize;
    int depth;
    bool current;
} BitmapMenuData;

typedef struct layer_menu_data {
    char* label;
    int index;
} LayerMenuData;

typedef struct anchor_menu_data {
    char* label;
    AnchorClass* ac;
} AnchorMenuData;

typedef struct encoding_menu_data {
    char* label;
    char* enc_name;
} EncodingMenuData;

enum py_menu_flag { pmf_font = 1, pmf_char = 2 };

struct py_menu_text {
    const char* localized;
    const char* untranslated;
    const char* identifier;
};

typedef struct py_menu_spec {
    int depth, divider;
    struct py_menu_text* levels;
    const char* shortcut_str;
    PyObject *func, *check, *data;
} PyMenuSpec;

typedef struct top_level_window {
    bool is_gtk;
    // Either CharGrid* or GWindow, depending on the value of is_gtk
    void* window;
} TopLevelWindow;

typedef struct mm_instance {
    SplineFont* sub;
    char* fontname;
} SubInstance;

typedef struct fontview_context {
    FontView* fv;

    // Set character grid to the desired position according to the scrollbar
    void (*scroll_fontview_to_position_cb)(FontView* fv, int32_t position);

    // Tooltip message to display for particular character
    char* (*tooltip_message_cb)(FontView* fv, int x, int y);

    // Get pixmap resource directory
    const char* (*get_pixmap_dir)();

    // Set view to bitmap font
    void (*change_display_bitmap)(FontView* fv, BDFFont* bdf);

    // Check if the current view is set to the bitmap font
    bool (*current_display_bitmap)(FontView* fv, BDFFont* bdf);

    // Collect bitmap fonts data for menu display
    unsigned int (*collect_bitmap_data)(FontView* fv,
                                        BitmapMenuData** bitmap_data_array);

    // Set view to layer id
    void (*change_display_layer)(FontView* fv, int ly);

    // Check if the current view is set to the layer id
    bool (*current_display_layer)(FontView* fv, int ly);

    // Collect layers data for menu display
    unsigned int (*collect_layer_data)(FontView* fv,
                                       LayerMenuData** layer_data_array);

    // Open anchor pair dialog
    void (*show_anchor_pair)(FontView* fv, AnchorClass* ac);

    // Collect layers data for menu display
    unsigned int (*collect_anchor_data)(FontView* fv,
                                        AnchorMenuData** anchor_data_array);

    // Reencode to new encoding
    void (*change_encoding)(FontView* fv, const char* enc_name);

    // Force new encoding
    void (*force_encoding)(FontView* fv, const char* enc_name);

    // Check if "enc" is the current encoding
    bool (*current_encoding)(FontView* fv, const char* enc_name);

    // Collect standard and user encodings. NULL entries may exist to designate
    // separators.
    unsigned int (*collect_encoding_data)(
        FontView* fv, EncodingMenuData** encoding_data_array);

    // Python callbacks for menu activation or checking if disabled
    void (*py_activate)(FontView* fv, PyObject* func, PyObject* data);
    bool (*py_check)(FontView* fv, const char* label, PyObject* check,
                     PyObject* data);

    // Invoke external autotrace / potrace command
    void (*run_autotrace)(FontView* fv, bool ask_user_for_arguments);

    // Set glyph color (legacy format 0xaarrggbb or -10/0xfffffff6 for color
    // chooser)
    void (*set_color)(FontView* fv, Color legacy_color);

    // Select glyph by color (legacy format 0xaarrggbb or -10/0xfffffff6 for
    // color chooser)
    void (*select_color)(FontView* fv, Color legacy_color,
                         enum merge_type merge);

    // Collect recent files
    unsigned int (*collect_recent_files)(char*** recent_files_array);

    // Show font
    FontViewBase* (*show_font)(const char* filename, int openflags);

    // Collect legacy PE script names
    unsigned int (*collect_script_names)(char*** collect_script_names);

    // Collect top-level windows
    unsigned int (*collect_windows)(void*, TopLevelWindow** windows_array);

    // Get legacy window title
    char* (*get_window_title)(GWindow window);

    // Raise legacy window to front
    void (*raise_window)(GWindow window);

    // Collect Multiple Master instances
    unsigned int (*collect_mm_instances)(FontView* fv,
                                         SubInstance** instance_array);

    // Show Multiple Master instance
    void (*show_mm_instance)(FontView* fv, SplineFont* sub);

    // Check if the Multiple Master instance is currently selected
    bool (*mm_selected)(FontView* fv, SplineFont* sub);

    // Collect CID instances
    unsigned int (*collect_cid_instances)(FontView* fv,
                                          SubInstance** instance_array);

    // Show CID instance
    void (*show_cid_instance)(FontView* fv, SplineFont* sub);

    // Check if the CID instance is currently selected
    bool (*cid_selected)(FontView* fv, SplineFont* sub);

    // Launch documentation
    void (*help)(const char* file, const char* section);

    // Menu actions per menu ID
    FVMenuAction* actions;
    FVSelectMenuAction* select_actions;
} FVContext;

typedef struct kerning_format_data {
    bool use_individual_pairs;
    bool guess_kerning_classes;
    double intra_class_dist;
    int default_separation;
    int min_kern;
    bool touching;
    bool kern_closer;
    bool autokern_new;
} KFDlgData;

#ifdef __cplusplus
}

#include <string>
#include <vector>

// A wrapper converts C-style callback which returns C-style buffer to C++
// function with same argumants which internally reallocates buffer to C++
// std::string.
//
// Usage for original callback with return value type char*:
//	std::string s = StringWrapper(c_callback)(c_callback_arguments);
template <typename... ARGS>
auto StringWrapper(char(*f(ARGS... args))) {
    return [f](ARGS... args) -> std::string {
        char* c_str = f(args...);
        // Check NULL pointer before initializing std::string.
        std::string str(c_str ? c_str : "");
        free(c_str);
        return str;
    };
}

// A wrapper executes C-style callback which returns C-style dynamically
// allocated array and converts its output to C++ std::vector.
//
// Sample usage: for C-style callback
//	unsigned int get_data(FontView* fv, Data** array)
// the wrapper should used as follows:
// 	std::vector<Data> data_vector = VectorWrapper(fv, get_data);
template <typename ELEM, typename C_OBJ>
std::vector<ELEM> VectorWrapper(C_OBJ* obj, unsigned int(f(C_OBJ*, ELEM**))) {
    ELEM* data_array = nullptr;
    unsigned int n_elems = f(obj, &data_array);

    // Create a std::vector from the existing array
    std::vector<ELEM> data_vec(data_array, data_array + n_elems);
    free(data_array);

    return data_vec;
}

// A wrapper executes C-style callback which returns C-style dynamically
// allocated array of null-terminated strings and converts its output to C++
// std::vector<std::string>.
//
// Sample usage: for C-style callback
//	unsigned int get_strings(char*** array)
// the wrapper should used as follows:
// 	std::vector<std::string> string_vector = StringsWrapper(get_strings);
inline std::vector<std::string> StringsWrapper(unsigned int(f(char***)),
                                               bool release_strings = false) {
    char** string_array = nullptr;
    unsigned int n_elems = f(&string_array);

    // Create a std::vector from the existing array
    std::vector<std::string> string_vec(string_array, string_array + n_elems);
    if (release_strings) {
        for (unsigned int i = 0; i < n_elems; ++i) {
            free(string_array[i]);
        }
    }
    free(string_array);

    return string_vec;
}

#endif
