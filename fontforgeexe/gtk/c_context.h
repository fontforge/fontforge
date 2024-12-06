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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fontview FontView;
typedef struct bdffont BDFFont;

enum glyphlabel { gl_glyph, gl_name, gl_unicode, gl_encoding };

typedef struct fv_menu_action {
    int mid;
    bool (*is_disabled)(FontView* fv, int mid); /* called before showing */
    bool (*is_checked)(FontView* fv, int mid);  /* called before showing */
    void (*action)(FontView* fv, int mid);      /* called on mouse release */
} FVMenuAction;

#define MENUACTION_LAST \
    { 0, NULL, NULL, NULL }

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

// C structure and callback for interacting with legacy code
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

    // Menu actions per menu ID
    FVMenuAction* actions;
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

#endif
