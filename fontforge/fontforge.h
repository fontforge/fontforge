/* Copyright (C) 2000-2012 by George Williams */
/*
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
#ifndef _PFAEDIT_H_
#define _PFAEDIT_H_

#include <fontforge-config.h>
#include "configure-fontforge.h"
#include <basics.h>
#include <stdio.h>
#include <string.h>
#include <intl.h>
#include "splinefont.h"
#include "uiinterface.h"


static const int unicode4_size = 17*65536;
    /* Unicode goes up to 0x10ffff */


extern void ProcessNativeScript(int argc, char *argv[], FILE *script);
extern void CheckIsScript(int argc, char *argv[]);

extern char *AdobeStandardEncoding[256];
extern int32 unicode_from_adobestd[256];

/* unicode_nameannot - Deprecated, but kept for older programs to access. */
#if _NO_LIBUNINAMESLIST
struct unicode_nameannot {
    const char *name, *annot;
};
#endif
extern const struct unicode_nameannot * const *const *_UnicodeNameAnnot;

extern int default_fv_font_size;
extern int default_fv_antialias;
extern int default_fv_bbsized;
extern Encoding *default_encoding, custom;
extern int adjustwidth;
extern int adjustlbearing;
extern int autohint_before_generate;
extern int seperate_hint_controls;
extern int no_windowing_ui;
extern uint32 default_background;
extern int use_utf8_in_script;

extern int new_em_size;
extern int new_fonts_are_order2;
extern int loaded_fonts_same_as_new;

extern char *BDFFoundry, *TTFFoundry;
extern char *xuid;

extern int pagewidth, pageheight, printtype;	/* Printer defaults */
extern char *printcommand, *printlazyprinter;

extern Encoding *enclist;


#define SCRIPT_MENU_MAX	10


extern MacFeat *default_mac_feature_map;

typedef struct library_version_configuration {
    uint16 major, minor;
    long library_source_modtime;
    char *library_source_modtime_string;
    int  library_source_versiondate;
    uint16 sizeof_me;
    uint16 sizeof_splinefont;
    uint16 sizeof_splinechar;
    uint16 sizeof_fvbase;
    uint16 sizeof_cvbase;
    uint16 sizeof_cvcontainer;
    uint8  config_had_devicetables;
    uint8  config_had_multilayer;
    uint8  config_had_python;
    uint8  mba1;		/* Must be all ones (0xff), config values are 0,1 need to distinquish from both */
} Library_Version_Configuration;
extern Library_Version_Configuration library_version_configuration;

extern int check_library_version(Library_Version_Configuration *exe_lib_version, int fatal, int quiet);

extern int fontforge_main(int, char **);
#endif
