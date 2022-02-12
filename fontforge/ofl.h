/*
Copyright: 2007 George Williams
License: BSD-3-clause
*/

#ifndef FONTFORGE_OFL_H
#define FONTFORGE_OFL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "splinefont.h"

extern struct str_lang_data {
    enum ttfnames strid;
    int lang;
    char **data;
} ofl_str_lang_data[];

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_OFL_H */
