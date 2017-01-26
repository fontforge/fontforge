/* -*- coding: utf-8 -*- */
/* Copyright (C) 2003-2012 by George Williams */
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
#include "fontforgeui.h"
#include "macenc.h"
#include "splineutil.h"
#include <gkeysym.h>
#include <ustring.h>
#include "ttf.h"

extern MacFeat *default_mac_feature_map,
	*builtin_mac_feature_map,
	*user_mac_feature_map;

static struct macsetting *MacSettingCopy(struct macsetting *ms) {
    struct macsetting *head=NULL, *last, *cur;

    while ( ms!=NULL ) {
	cur = chunkalloc(sizeof(struct macsetting));
	cur->setting = ms->setting;
	cur->setname = MacNameCopy(ms->setname);
	cur->initially_enabled = ms->initially_enabled;
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	ms = ms->next;
    }
return( head );
}

static MacFeat *MacFeatCopy(MacFeat *mf) {
    MacFeat *head=NULL, *last, *cur;

    while ( mf!=NULL ) {
	cur = chunkalloc(sizeof(MacFeat));
	cur->feature = mf->feature;
	cur->featname = MacNameCopy(mf->featname);
	cur->settings = MacSettingCopy(mf->settings);
	cur->ismutex = mf->ismutex;
	cur->default_setting = mf->default_setting;
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	mf = mf->next;
    }
return( head );
}

/* ************************************************************************** */
/* Sigh. This list is duplicated in macenc.c */
static GTextInfo maclanguages[] = {
    { (unichar_t *) N_("English"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("French"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("German"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Italian"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Dutch"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Swedish"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Spanish"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Danish"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Portuguese"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Norwegian"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Hebrew"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Japanese"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Arabic"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Finnish"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Greek"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Icelandic"), NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Maltese"), NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Turkish"), NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Croatian"), NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Traditional Chinese"), NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Urdu"), NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Hindi"), NULL, 0, 0, (void *) 21, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Thai"), NULL, 0, 0, (void *) 22, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Korean"), NULL, 0, 0, (void *) 23, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lithuanian"), NULL, 0, 0, (void *) 24, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Polish"), NULL, 0, 0, (void *) 25, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Hungarian"), NULL, 0, 0, (void *) 26, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Estonian"), NULL, 0, 0, (void *) 27, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Latvian"), NULL, 0, 0, (void *) 28, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Sami (Lappish)"), NULL, 0, 0, (void *) 29, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Faroese (Icelandic)"), NULL, 0, 0, (void *) 30, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Farsi/Persian", ignore "Lang|" */
    { (unichar_t *) N_("Lang|Farsi/Persian"), NULL, 0, 0, (void *) 31, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Russian"), NULL, 0, 0, (void *) 32, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Simplified Chinese"), NULL, 0, 0, (void *) 33, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Flemish"), NULL, 0, 0, (void *) 34, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Irish Gaelic"), NULL, 0, 0, (void *) 35, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Albanian"), NULL, 0, 0, (void *) 36, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Romanian"), NULL, 0, 0, (void *) 37, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Czech"), NULL, 0, 0, (void *) 38, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Slovak"), NULL, 0, 0, (void *) 39, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Slovenian"), NULL, 0, 0, (void *) 40, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Yiddish"), NULL, 0, 0, (void *) 41, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Serbian"), NULL, 0, 0, (void *) 42, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Macedonian"), NULL, 0, 0, (void *) 43, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Bulgarian"), NULL, 0, 0, (void *) 44, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Ukrainian"), NULL, 0, 0, (void *) 45, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Byelorussian"), NULL, 0, 0, (void *) 46, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Uzbek"), NULL, 0, 0, (void *) 47, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Kazakh"), NULL, 0, 0, (void *) 48, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Axerbaijani (Cyrillic)"), NULL, 0, 0, (void *) 49, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Axerbaijani (Arabic)"), NULL, 0, 0, (void *) 50, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Armenian"), NULL, 0, 0, (void *) 51, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Georgian"), NULL, 0, 0, (void *) 52, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Moldavian"), NULL, 0, 0, (void *) 53, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Kirghiz"), NULL, 0, 0, (void *) 54, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Tajiki"), NULL, 0, 0, (void *) 55, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Turkmen"), NULL, 0, 0, (void *) 56, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Mongolian (Mongolian)"), NULL, 0, 0, (void *) 57, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Mongolian (cyrillic)"), NULL, 0, 0, (void *) 58, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Pashto"), NULL, 0, 0, (void *) 59, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Kurdish"), NULL, 0, 0, (void *) 60, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Kashmiri"), NULL, 0, 0, (void *) 61, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Sindhi"), NULL, 0, 0, (void *) 62, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Tibetan"), NULL, 0, 0, (void *) 63, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Nepali"), NULL, 0, 0, (void *) 64, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Sanskrit"), NULL, 0, 0, (void *) 65, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Marathi"), NULL, 0, 0, (void *) 66, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Bengali"), NULL, 0, 0, (void *) 67, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Assamese"), NULL, 0, 0, (void *) 68, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Gujarati"), NULL, 0, 0, (void *) 69, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Punjabi"), NULL, 0, 0, (void *) 70, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Oriya"), NULL, 0, 0, (void *) 71, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Malayalam"), NULL, 0, 0, (void *) 72, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Kannada"), NULL, 0, 0, (void *) 73, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Tamil"), NULL, 0, 0, (void *) 74, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Telugu"), NULL, 0, 0, (void *) 75, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Sinhalese"), NULL, 0, 0, (void *) 76, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Burmese"), NULL, 0, 0, (void *) 77, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Khmer"), NULL, 0, 0, (void *) 78, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Lao"), NULL, 0, 0, (void *) 79, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Vietnamese"), NULL, 0, 0, (void *) 80, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Indonesian"), NULL, 0, 0, (void *) 81, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Tagalog"), NULL, 0, 0, (void *) 82, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Malay (roman)"), NULL, 0, 0, (void *) 83, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Malay (arabic)"), NULL, 0, 0, (void *) 84, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Amharic"), NULL, 0, 0, (void *) 85, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Tigrinya"), NULL, 0, 0, (void *) 86, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Galla"), NULL, 0, 0, (void *) 87, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Somali"), NULL, 0, 0, (void *) 88, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Swahili"), NULL, 0, 0, (void *) 89, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Kinyarwanda/Ruanda"), NULL, 0, 0, (void *) 90, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Rundi"), NULL, 0, 0, (void *) 91, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Nyanja/Chewa"), NULL, 0, 0, (void *) 92, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Malagasy"), NULL, 0, 0, (void *) 93, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Esperanto"), NULL, 0, 0, (void *) 94, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Welsh"), NULL, 0, 0, (void *) 128, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Basque"), NULL, 0, 0, (void *) 129, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Catalan"), NULL, 0, 0, (void *) 130, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Latin"), NULL, 0, 0, (void *) 131, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Quechua"), NULL, 0, 0, (void *) 132, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Guarani"), NULL, 0, 0, (void *) 133, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Aymara"), NULL, 0, 0, (void *) 134, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Tatar"), NULL, 0, 0, (void *) 135, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Lang|Uighur"), NULL, 0, 0, (void *) 136, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Dzongkha"), NULL, 0, 0, (void *) 137, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Javanese (roman)"), NULL, 0, 0, (void *) 138, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Sundanese (roman)"), NULL, 0, 0, (void *) 139, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Galician"), NULL, 0, 0, (void *) 140, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Afrikaans"), NULL, 0, 0, (void *) 141, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Breton"), NULL, 0, 0, (void *) 142, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Inuktitut"), NULL, 0, 0, (void *) 143, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Scottish Gaelic"), NULL, 0, 0, (void *) 144, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Manx Gaelic"), NULL, 0, 0, (void *) 145, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Irish Gaelic (with dot)"), NULL, 0, 0, (void *) 146, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Tongan"), NULL, 0, 0, (void *) 147, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Greek (polytonic)"), NULL, 0, 0, (void *) 148, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Greenlandic"), NULL, 0, 0, (void *) 149, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Azebaijani (roman)"), NULL, 0, 0, (void *) 150, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static void initmaclangs(void) {
    static int inited = false;
    int i;

    if ( !inited ) {
	inited = true;
	for ( i=0; maclanguages[i].text!=NULL; ++i )
	    maclanguages[i].text = (unichar_t *) S_( (char *) maclanguages[i].text);
    }
}

    /* These first three must match the values in prefs.c */
#define CID_Features	101
#define CID_FeatureDel	103
#define CID_FeatureEdit	105

#define CID_Settings	101
#define CID_SettingDel	103
#define CID_SettingEdit	105

#define CID_NameList	201
#define CID_NameNew	202
#define CID_NameDel	203
#define CID_NameEdit	205

#define CID_Cancel	300
#define CID_OK		301
#define CID_Id		302
#define CID_Name	303
#define CID_Language	304
#define CID_On		305
#define CID_Mutex	306

static char *spacer = " ⇒ ";	/* right double arrow */

static GTextInfo *Pref_MacNamesList(struct macname *all) {
    GTextInfo *ti;
    int i, j;
    struct macname *mn;
    char *temp, *full;

    initmaclangs();

    for ( i=0, mn=all; mn!=NULL; mn=mn->next, ++i );
    ti = calloc(i+1,sizeof( GTextInfo ));

    for ( i=0, mn=all; mn!=NULL; mn=mn->next, ++i ) {
	temp = MacStrToUtf8(mn->name,mn->enc,mn->lang);
	if ( temp==NULL )
    continue;
	for ( j=0 ; maclanguages[j].text!=0; ++j )
	    if ( maclanguages[j].userdata == (void *) (intpt) (mn->lang ))
	break;
	if ( maclanguages[j].text!=0 ) {
	    char *lang = (char *) maclanguages[j].text;
	    full = malloc((strlen(lang)+strlen(temp)+strlen(spacer)+1));
	    strcpy(full,lang);
	} else {
	    char *hunh = "???";
	    full = malloc((strlen(hunh)+strlen(temp)+strlen(spacer)+1));
	    strcpy(full,hunh);
	}
	strcat(full,spacer);
	strcat(full,temp);
	free(temp);
	ti[i].text = (unichar_t *) full;
	ti[i].text_is_1byte = true;
	ti[i].userdata = (void *) mn;
    }
return( ti );
}

static GTextInfo *Pref_SettingsList(struct macsetting *all) {
    GTextInfo *ti;
    int i;
    struct macsetting *ms;
    unichar_t *full; char *temp;
    char buf[20];

    for ( i=0, ms=all; ms!=NULL; ms=ms->next, ++i );
    ti = calloc(i+1,sizeof( GTextInfo ));

    for ( i=0, ms=all; ms!=NULL; ms=ms->next, ++i ) {
	temp = PickNameFromMacName(ms->setname);
	sprintf(buf,"%3d ", ms->setting);
	if ( temp==NULL )
	    full = uc_copy(buf);
	else {
	    full = malloc((strlen(buf)+strlen(temp)+1)*sizeof(unichar_t));
	    uc_strcpy(full,buf);
	    utf82u_strcpy(full+u_strlen(full),temp);
	    free(temp);
	}
	ti[i].text = full;
	ti[i].userdata = ms;
    }
return( ti );
}

static GTextInfo *Pref_FeaturesList(MacFeat *all) {
    GTextInfo *ti;
    int i;
    MacFeat *mf;
    char *temp;
    unichar_t *full;
    char buf[20];

    for ( i=0, mf=all; mf!=NULL; mf=mf->next, ++i );
    ti = calloc(i+1,sizeof( GTextInfo ));

    for ( i=0, mf=all; mf!=NULL; mf=mf->next, ++i ) {
	temp = PickNameFromMacName(mf->featname);
	sprintf(buf,"%3d ", mf->feature);
	if ( temp==NULL )
	    full = uc_copy(buf);
	else {
	    full = malloc((strlen(buf)+strlen(temp)+1)*sizeof(unichar_t));
	    uc_strcpy(full,buf);
	    utf82u_strcpy(full+u_strlen(full),temp);
	    free(temp);
	}
	ti[i].text = full;
	ti[i].userdata = mf;
    }
return( ti );
}

struct namedata {
    GWindow gw;
    int index;
    int done;
    struct macname *all, *changing;
    GGadget *namelist;		/* Not in this dlg, in the dlg which created us */
};

static int name_e_h(GWindow gw, GEvent *event) {
    struct namedata *nd = GDrawGetUserData(gw);
    int i;
    int32 len;
    GTextInfo **ti, *sel;
    char *ret1, *temp; unichar_t *full;
    int val1, val2;
    struct macname *mn;
    int language;

    if ( event->type==et_close ) {
	nd->done = true;
	if ( nd->index==-1 )
	    MacNameListFree(nd->changing);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("prefs.html#Features");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	if ( GGadgetGetCid(event->u.control.g) == CID_Cancel ) {
	    nd->done = true;
	    if ( nd->index==-1 )
		MacNameListFree(nd->changing);
	} else if ( GGadgetGetCid(event->u.control.g) == CID_OK ) {
	    sel = GGadgetGetListItemSelected(GWidgetGetControl(nd->gw,CID_Language));
	    language = nd->changing->lang;
	    if ( sel!=NULL )
		language = (intpt) sel->userdata;
	    else if ( nd->index==-1 ) {
		ff_post_error(_("Bad Language"),_("Bad Language"));
return( true );
	    }	/* Otherwise use the original language, it might not be one we recognize */
	    if ( language != nd->changing->lang )
		nd->changing->enc = MacEncFromMacLang(language);
	    nd->changing->lang = language;
	    val1 = (nd->changing->enc<<16) | nd->changing->lang;
	    ret1 = GGadgetGetTitle8(GWidgetGetControl(nd->gw,CID_Name));
	    free(nd->changing->name);
	    nd->changing->name = Utf8ToMacStr(ret1,nd->changing->enc,nd->changing->lang);
	    free(ret1);

	    ti = GGadgetGetList(nd->namelist,&len);
	    for ( i=0; i<len; ++i ) if ( i!=nd->index ) {
		val2 = (((struct macname *) (ti[i]->userdata))->enc<<16) |
			(((struct macname *) (ti[i]->userdata))->lang);
		if ( val2==val1 ) {
		    ff_post_error(_("This feature code is already used"),_("This feature code is already used"));
return( true );
		}
	    }

	    temp = MacStrToUtf8(nd->changing->name,nd->changing->enc,nd->changing->lang);
	    if ( sel!=NULL ) {
		const unichar_t *lang = sel->text;
		full = malloc((u_strlen(lang)+strlen(temp)+6)*sizeof(unichar_t));
		u_strcpy(full,lang);
	    } else {
		char *hunh = "???";
		full = malloc((strlen(hunh)+strlen(temp)+6)*sizeof(unichar_t));
		uc_strcpy(full,hunh);
	    }
	    uc_strcat(full,spacer);
	    utf82u_strcpy(full+u_strlen(full),temp);

	    if ( nd->index==-1 )
		GListAddStr(nd->namelist,full,nd->changing);
	    else {
		GListReplaceStr(nd->namelist,nd->index,full,nd->changing);
		if ( nd->all==nd->changing )
		    nd->all = nd->changing->next;
		else {
		    for ( mn=nd->all ; mn!=NULL && mn->next!=nd->changing; mn=mn->next );
		    if ( mn!=NULL ) mn->next = nd->changing->next;
		}
	    }
	    nd->changing->next = NULL;
	    if ( nd->all==NULL || val1< ((nd->all->enc<<16)|nd->all->lang) ) {
		nd->changing->next = nd->all;
		nd->all = nd->changing;
	    } else {
		for ( mn=nd->all; mn->next!=NULL && ((mn->next->enc<<16)|mn->next->lang)<val1; mn=mn->next );
		nd->changing->next = mn->next;
		mn->next = nd->changing;
	    }
	    GGadgetSetUserData(nd->namelist,nd->all);
	    nd->done = true;
	}
    }
return( true );
}

static char *AskName(struct macname *changing,struct macname *all,GGadget *list, int index) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct namedata nd;
    int i;

    initmaclangs();

    memset(&nd,0,sizeof(nd));
    nd.namelist = list;
    nd.index = index;
    nd.changing = changing;
    nd.all = all;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Setting");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,270));
    pos.height = GDrawPointsToPixels(NULL,98);
    gw = GDrawCreateTopWindow(NULL,&pos,name_e_h,&nd,&wattrs);
    nd.gw = gw;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    label[0].text = (unichar_t *) _("_Language:");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+4;
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 60; gcd[1].gd.pos.y = 5;
    gcd[1].gd.pos.width = 200;
    gcd[1].gd.flags = gg_enabled|gg_visible | gg_list_alphabetic;
    gcd[1].gd.u.list = maclanguages;
    gcd[1].gd.cid = CID_Language;
    gcd[1].creator = GListButtonCreate;

    for ( i=0; maclanguages[i].text!=NULL; ++i ) {
	if ( maclanguages[i].userdata == (void *) (intpt) (changing->lang) )
	    maclanguages[i].selected = true;
	else
	    maclanguages[i].selected = false;
	if ( changing->lang==65535 )
	    maclanguages[0].selected = true;
    }

    label[2].text = (unichar_t *) _("_Name:");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[0].gd.pos.y+28;
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    label[3].text = (unichar_t *) MacStrToUtf8(changing->name,changing->enc,changing->lang);
    label[3].text_is_1byte = true;
    gcd[3].gd.label = changing->name==NULL ? NULL : &label[3];
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[2].gd.pos.y-4;
    gcd[3].gd.pos.width = 200;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Name;
    gcd[3].creator = GTextFieldCreate;

    i = 4;

    gcd[i].gd.pos.x = 13-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+30;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_OK;
    /*gcd[i].gd.handle_controlevent = Prefs_Ok;*/
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -13; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_Cancel;
    gcd[i].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    GDrawSetVisible(gw,true);
    GWidgetIndicateFocusGadget(gcd[1].ret);
    while ( !nd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return( false );
}

static void ChangeName(GGadget *list,int index) {
    struct macname *mn = GGadgetGetListItemSelected(list)->userdata,
		    *all = GGadgetGetUserData(list);

    AskName(mn,all,list,index);
}

static int Pref_NewName(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_NameList);
	struct macname *new, *all;

	all = GGadgetGetUserData(list);
	new = chunkalloc(sizeof(struct macname));
	new->lang = -1;
	AskName(new,all,list,-1);
    }
return( true );
}

static int Pref_DelName(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct macname *mn, *p, *all, *next;
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_NameList);
	int32 len;
	GTextInfo **ti = GGadgetGetList(list,&len);
	int i;

	all = GGadgetGetUserData(list);
	for ( mn = all, p=NULL; mn!=NULL; mn = next ) {
	    next = mn->next;
	    for ( i=len-1; i>=0; --i ) {
		if ( ti[i]->selected && ti[i]->userdata==mn )
	    break;
	    }
	    if ( i>=0 ) {
		if ( p==NULL )
		    all = next;
		else
		    p->next = next;
		mn->next = NULL;
		MacNameListFree(mn);
	    } else
		p = mn;
	}
	GGadgetSetUserData(list,all);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameDel),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameEdit),false);
    }
return( true );
}

static int Pref_EditName(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GGadget *list = GWidgetGetControl(GGadgetGetWindow(g),CID_NameList);
	ChangeName(list,GGadgetGetFirstListSelectedItem(list));
    }
return( true );
}

static int Pref_NameSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	GWindow gw = GGadgetGetWindow(g);
	int i, sel_cnt=0;
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected ) ++sel_cnt;
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameDel),sel_cnt!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameEdit),sel_cnt==1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	ChangeName(g,e->u.control.u.list.changed_index!=-1?e->u.control.u.list.changed_index:
		GGadgetGetFirstListSelectedItem(g));
    }
return( true );
}

struct macname *NameGadgetsGetNames( GWindow gw ) {
return( GGadgetGetUserData(GWidgetGetControl(gw,CID_NameList)) );
}

void NameGadgetsSetEnabled( GWindow gw, int enable ) {

    GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameList),enable);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameNew),enable);
    if ( !enable ) {
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameDel),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameEdit),false);
    } else {
	int32 len;
	GGadget *list = GWidgetGetControl(gw,CID_NameList);
	GTextInfo **ti = GGadgetGetList(list,&len);
	int i, sel_cnt=0;
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected ) ++sel_cnt;
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameDel),sel_cnt>0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NameEdit),sel_cnt=1);
    }
}

int GCDBuildNames(GGadgetCreateData *gcd,GTextInfo *label,int pos,struct macname *names) {

    gcd[pos].gd.pos.x = 6; gcd[pos].gd.pos.y = pos==0 ? 6 :
	    gcd[pos-1].creator==GTextFieldCreate ? gcd[pos-1].gd.pos.y+30 :
						    gcd[pos-1].gd.pos.y+14;
    gcd[pos].gd.pos.width = 250; gcd[pos].gd.pos.height = 5*12+10;
    gcd[pos].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic | gg_list_multiplesel;
    gcd[pos].gd.cid = CID_NameList;
    gcd[pos].data = names = MacNameCopy(names);
    gcd[pos].gd.u.list = Pref_MacNamesList(names);
    gcd[pos].gd.handle_controlevent = Pref_NameSel;
    gcd[pos++].creator = GListCreate;

    gcd[pos].gd.pos.x = 6; gcd[pos].gd.pos.y = gcd[pos-1].gd.pos.y+gcd[pos-1].gd.pos.height+10;
    gcd[pos].gd.flags = gg_visible | gg_enabled;
    label[pos].text = (unichar_t *) S_("MacName|_New...");
    label[pos].text_is_1byte = true;
    label[pos].text_in_resource = true;
    gcd[pos].gd.label = &label[pos];
    gcd[pos].gd.handle_controlevent = Pref_NewName;
    gcd[pos].gd.cid = CID_NameNew;
    gcd[pos++].creator = GButtonCreate;

    gcd[pos].gd.pos.x = gcd[pos-1].gd.pos.x+20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    gcd[pos].gd.pos.y = gcd[pos-1].gd.pos.y;
    gcd[pos].gd.flags = gg_visible ;
    label[pos].text = (unichar_t *) _("_Delete");
    label[pos].text_is_1byte = true;
    label[pos].text_in_resource = true;
    gcd[pos].gd.label = &label[pos];
    gcd[pos].gd.cid = CID_NameDel;
    gcd[pos].gd.handle_controlevent = Pref_DelName;
    gcd[pos++].creator = GButtonCreate;

    gcd[pos].gd.pos.x = gcd[pos-1].gd.pos.x+20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    gcd[pos].gd.pos.y = gcd[pos-1].gd.pos.y;
    gcd[pos].gd.flags = gg_visible ;
    label[pos].text = (unichar_t *) _("_Edit...");
    label[pos].text_is_1byte = true;
    label[pos].text_in_resource = true;
    gcd[pos].gd.label = &label[pos];
    gcd[pos].gd.cid = CID_NameEdit;
    gcd[pos].gd.handle_controlevent = Pref_EditName;
    gcd[pos++].creator = GButtonCreate;

return( pos );
}

struct setdata {
    GWindow gw;
    int index;
    int done;
    struct macsetting *all, *changing;
    GGadget *settinglist;		/* Not in this dlg, in the dlg which created us */
};

static int set_e_h(GWindow gw, GEvent *event) {
    struct setdata *sd = GDrawGetUserData(gw);
    int i;
    int32 len;
    GTextInfo **ti;
    const unichar_t *ret1; unichar_t *end, *res; char *temp;
    int val1, val2;
    char buf[20];
    struct macsetting *ms;

    if ( event->type==et_close ) {
	sd->done = true;
	MacNameListFree(GGadgetGetUserData(GWidgetGetControl(sd->gw,CID_NameList)));
	if ( sd->index==-1 )
	    MacSettingListFree(sd->changing);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("prefs.html#Settings");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	if ( GGadgetGetCid(event->u.control.g) == CID_Cancel ) {
	    sd->done = true;
	    MacNameListFree(GGadgetGetUserData(GWidgetGetControl(sd->gw,CID_NameList)));
	    if ( sd->index==-1 )
		MacSettingListFree(sd->changing);
	} else if ( GGadgetGetCid(event->u.control.g) == CID_OK ) {
	    ret1 = _GGadgetGetTitle(GWidgetGetControl(sd->gw,CID_Id));
	    val1 = u_strtol(ret1,&end,10);
	    if ( *end!='\0' ) {
		ff_post_error(_("Bad Number"),_("Bad Number"));
return( true );
	    }
	    ti = GGadgetGetList(sd->settinglist,&len);
	    for ( i=0; i<len; ++i ) if ( i!=sd->index ) {
		val2 = ((struct macsetting *) (ti[i]->userdata))->setting;
		if ( val2==val1 ) {
		    ff_post_error(_("This setting is already used"),_("This setting is already used"));
return( true );
		}
	    }
	    MacNameListFree(sd->changing->setname);
	    sd->changing->setname = GGadgetGetUserData(GWidgetGetControl(sd->gw,CID_NameList));
	    sd->changing->setting = val1;
	    sd->changing->initially_enabled = GGadgetIsChecked(GWidgetGetControl(sd->gw,CID_On));
	    if ( sd->changing->initially_enabled &&
		    GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(sd->settinglist),CID_Mutex)) ) {
		/* If the mutually exclusive bit were set in the feature then */
		/*  turning this guy on, means we must turn others off */
		struct macsetting *test;
		for ( test = sd->all; test!=NULL; test = test->next )
		    if ( test!=sd->changing )
			test->initially_enabled = false;
	    }

	    sprintf(buf,"%3d ", val1);
	    temp = PickNameFromMacName(sd->changing->setname);
	    len = strlen(temp);
	    res = malloc( (strlen(buf)+len+3)*sizeof(unichar_t) );
	    uc_strcpy(res,buf);
	    utf82u_strcpy(res+u_strlen(res),temp);
	    free(temp);

	    if ( sd->index==-1 )
		GListAddStr(sd->settinglist,res,sd->changing);
	    else {
		GListReplaceStr(sd->settinglist,sd->index,res,sd->changing);
		if ( sd->all==sd->changing )
		    sd->all = sd->changing->next;
		else {
		    for ( ms=sd->all ; ms!=NULL && ms->next!=sd->changing; ms=ms->next );
		    if ( ms!=NULL ) ms->next = sd->changing->next;
		}
	    }
	    sd->changing->next = NULL;
	    if ( sd->all==NULL || sd->changing->setting<sd->all->setting ) {
		sd->changing->next = sd->all;
		sd->all = sd->changing;
	    } else {
		for ( ms=sd->all; ms->next!=NULL && ms->next->setting<sd->changing->setting; ms=ms->next );
		sd->changing->next = ms->next;
		ms->next = sd->changing;
	    }
	    GGadgetSetUserData(sd->settinglist,sd->all);
	    sd->done = true;
	}
    }
return( true );
}

static char *AskSetting(struct macsetting *changing,struct macsetting *all,
	GGadget *list, int index) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12];
    GTextInfo label[12];
    struct setdata sd;
    char buf[20];
    int i;

    memset(&sd,0,sizeof(sd));
    sd.settinglist = list;
    sd.index = index;
    sd.changing = changing;
    sd.all = all;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Setting");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,270));
    pos.height = GDrawPointsToPixels(NULL,193);
    gw = GDrawCreateTopWindow(NULL,&pos,set_e_h,&sd,&wattrs);
    sd.gw = gw;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    label[0].text = (unichar_t *) _("Setting Id:");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+4;
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( buf, "%d", changing->setting );
    label[1].text = (unichar_t *) buf;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 60; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 40;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Id;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _("_Enabled");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 110; gcd[2].gd.pos.y = 5;
    gcd[2].gd.flags = gg_enabled|gg_visible | (changing->initially_enabled?gg_cb_on:0);
    gcd[2].gd.cid = CID_On;
    gcd[2].creator = GCheckBoxCreate;

    label[3].text = (unichar_t *) _("_Name:");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 5+24;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].creator = GLabelCreate;


    i = GCDBuildNames(gcd,label,4,changing->setname);

    gcd[i].gd.pos.x = 13-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+35;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_OK;
    /*gcd[i].gd.handle_controlevent = Prefs_Ok;*/
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -13; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_Cancel;
    gcd[i].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    GTextInfoListFree(gcd[4].gd.u.list);

    GDrawSetVisible(gw,true);
    GWidgetIndicateFocusGadget(gcd[1].ret);
    while ( !sd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

return( false );
}

static void ChangeSetting(GGadget *list,int index) {
    struct macsetting *ms = GGadgetGetListItemSelected(list)->userdata,
		    *all = GGadgetGetUserData(list);

    AskSetting(ms,all,list,index);
}

static int Pref_NewSetting(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_Settings);
	struct macsetting *new, *ms, *all;
	int expected=0;

	all = GGadgetGetUserData(list);
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Mutex)) ) {
	    for ( ms=all; ms!=NULL; ms=ms->next ) {
		if ( ms->setting!=expected )
	    break;
		++expected;
	    }
	} else {
	    for ( ms=all; ms!=NULL; ms=ms->next ) {
		if ( ms->setting&1 )	/* Shouldn't be any odd settings for non-mutex */
	    continue;
		if ( ms->setting!=expected )
	    break;
		expected += 2;
	    }
	}
	new = chunkalloc(sizeof(struct macsetting));
	new->setting = expected;
	AskSetting(new,all,list,-1);
    }
return( true );
}

static int Pref_DelSetting(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct macsetting *ms, *p, *all, *next;
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_Settings);
	int32 len;
	GTextInfo **ti = GGadgetGetList(list,&len);
	int i;

	all = GGadgetGetUserData(list);
	for ( ms = all, p=NULL; ms!=NULL; ms = next ) {
	    next = ms->next;
	    for ( i=len-1; i>=0; --i ) {
		if ( ti[i]->selected && ti[i]->userdata==ms )
	    break;
	    }
	    if ( i>=0 ) {
		if ( p==NULL )
		    all = next;
		else
		    p->next = next;
		ms->next = NULL;
		MacSettingListFree(ms);
	    } else
		p = ms;
	}
	GGadgetSetUserData(list,all);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_SettingDel),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_SettingEdit),false);
    }
return( true );
}

static int Pref_EditSetting(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GGadget *list = GWidgetGetControl(GGadgetGetWindow(g),CID_Settings);
	ChangeSetting(list,GGadgetGetFirstListSelectedItem(list));
    }
return( true );
}

static int Pref_SettingSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	GWindow gw = GGadgetGetWindow(g);
	int i, sel_cnt=0;
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected ) ++sel_cnt;
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_SettingDel),sel_cnt!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_SettingEdit),sel_cnt==1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	ChangeSetting(g,e->u.control.u.list.changed_index!=-1?e->u.control.u.list.changed_index:
		GGadgetGetFirstListSelectedItem(g));
    }
return( true );
}

struct featdata {
    GWindow gw;
    int index;
    int done;
    MacFeat *all, *changing;
    GGadget *featurelist;		/* Not in this dlg, in the dlg which created us */
};

static int feat_e_h(GWindow gw, GEvent *event) {
    struct featdata *fd = GDrawGetUserData(gw);
    int i;
    int32 len;
    GTextInfo **ti;
    const unichar_t *ret1; unichar_t *end, *res; char *temp;
    int val1, val2;
    char buf[20];
    MacFeat *mf;

    if ( event->type==et_close ) {
	fd->done = true;
	if ( fd->index==-1 )
	    MacFeatListFree(fd->changing);
	MacSettingListFree(GGadgetGetUserData(GWidgetGetControl(fd->gw,CID_Settings)));
	MacNameListFree(GGadgetGetUserData(GWidgetGetControl(fd->gw,CID_NameList)));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("prefs.html#Features");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	if ( GGadgetGetCid(event->u.control.g) == CID_Cancel ) {
	    fd->done = true;
	    if ( fd->index==-1 )
		MacFeatListFree(fd->changing);
	    MacSettingListFree(GGadgetGetUserData(GWidgetGetControl(fd->gw,CID_Settings)));
	    MacNameListFree(GGadgetGetUserData(GWidgetGetControl(fd->gw,CID_NameList)));
	} else if ( GGadgetGetCid(event->u.control.g) == CID_OK ) {
	    ret1 = _GGadgetGetTitle(GWidgetGetControl(fd->gw,CID_Id));
	    val1 = u_strtol(ret1,&end,10);
	    if ( *end!='\0' ) {
		ff_post_error(_("Bad Number"),_("Bad Number"));
return( true );
	    }
	    ti = GGadgetGetList(fd->featurelist,&len);
	    for ( i=0; i<len; ++i ) if ( i!=fd->index ) {
		val2 = ((MacFeat *) (ti[i]->userdata))->feature;
		if ( val2==val1 ) {
		    ff_post_error(_("This feature code is already used"),_("This feature code is already used"));
return( true );
		}
	    }
	    MacSettingListFree(fd->changing->settings);
	    MacNameListFree(fd->changing->featname);
	    fd->changing->featname = GGadgetGetUserData(GWidgetGetControl(fd->gw,CID_NameList));
	    fd->changing->settings = GGadgetGetUserData(GWidgetGetControl(fd->gw,CID_Settings));
	    fd->changing->ismutex = GGadgetIsChecked(GWidgetGetControl(fd->gw,CID_Mutex));
	    if ( fd->changing->ismutex ) {
		struct macsetting *ms;
		for ( i=0, ms = fd->changing->settings; ms!=NULL && !ms->initially_enabled; ms = ms->next, ++i );
		if ( ms==NULL ) i = 0;
		fd->changing->default_setting = i;
		if ( ms!=NULL ) {
		    for ( ms=ms->next ; ms!=NULL; ms = ms->next )
			ms->initially_enabled = false;
		}
	    }

	    sprintf(buf,"%3d ", val1);
	    temp = PickNameFromMacName(fd->changing->featname);
	    len = strlen(temp);
	    res = malloc( (strlen(buf)+len+3)*sizeof(unichar_t) );
	    uc_strcpy(res,buf);
	    utf82u_strcpy(res+u_strlen(res),temp);
	    free(temp);

	    if ( fd->index==-1 )
		GListAddStr(fd->featurelist,res,fd->changing);
	    else {
		GListReplaceStr(fd->featurelist,fd->index,res,fd->changing);
		if ( fd->all==fd->changing )
		    fd->all = fd->changing->next;
		else {
		    for ( mf=fd->all ; mf!=NULL && mf->next!=fd->changing; mf=mf->next );
		    if ( mf!=NULL ) mf->next = fd->changing->next;
		}
	    }
	    fd->changing->next = NULL;
	    if ( fd->all==NULL || fd->changing->feature<fd->all->feature ) {
		fd->changing->next = fd->all;
		fd->all = fd->changing;
	    } else {
		for ( mf=fd->all; mf->next!=NULL && mf->next->feature<fd->changing->feature; mf=mf->next );
		fd->changing->next = mf->next;
		mf->next = fd->changing;
	    }
	    GGadgetSetUserData(fd->featurelist,fd->all);
	    fd->done = true;
	}
    }
return( true );
}

static char *AskFeature(MacFeat *changing,MacFeat *all,GGadget *list, int index) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[16];
    GTextInfo label[16], *freeme;
    struct featdata fd;
    char buf[20];
    int i;

    memset(&fd,0,sizeof(fd));
    fd.featurelist = list;
    fd.index = index;
    fd.changing = changing;
    fd.all = all;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Feature");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,265));
    pos.height = GDrawPointsToPixels(NULL,353);
    gw = GDrawCreateTopWindow(NULL,&pos,feat_e_h,&fd,&wattrs);
    fd.gw = gw;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    label[0].text = (unichar_t *) _("Feature _Id:");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+4;
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( buf, "%d", changing->feature );
    label[1].text = (unichar_t *) buf;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 60; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 40;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Id;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _("Mutually Exclusive");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 105; gcd[2].gd.pos.y = 5+4;
    gcd[2].gd.flags = gg_enabled|gg_visible | (changing->ismutex?gg_cb_on:0);
    gcd[2].gd.cid = CID_Mutex;
    gcd[2].creator = GCheckBoxCreate;

    label[3].text = (unichar_t *) _("_Name:");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 5+24;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].creator = GLabelCreate;

    i = GCDBuildNames(gcd,label,4,changing->featname);

    label[i].text = (unichar_t *) _("Settings");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+35;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = 6; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+14;
    gcd[i].gd.pos.width = 250; gcd[i].gd.pos.height = 8*12+10;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic | gg_list_multiplesel;
    gcd[i].gd.cid = CID_Settings;
    gcd[i].data = MacSettingCopy(changing->settings);
    gcd[i].gd.u.list = freeme = Pref_SettingsList(gcd[i].data);
    gcd[i].gd.handle_controlevent = Pref_SettingSel;
    gcd[i++].creator = GListCreate;

    gcd[i].gd.pos.x = 6; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+gcd[i-1].gd.pos.height+10;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) S_("MacSetting|_New...");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = Pref_NewSetting;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = gcd[i-1].gd.pos.x+20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible ;
    label[i].text = (unichar_t *) _("_Delete");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_SettingDel;
    gcd[i].gd.handle_controlevent = Pref_DelSetting;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = gcd[i-1].gd.pos.x+20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible ;
    label[i].text = (unichar_t *) _("_Edit...");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_SettingEdit;
    gcd[i].gd.handle_controlevent = Pref_EditSetting;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 13-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+30;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_OK;
    /*gcd[i].gd.handle_controlevent = Prefs_Ok;*/
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -13; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_Cancel;
    gcd[i].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    GTextInfoListFree(gcd[4].gd.u.list);
    GTextInfoListFree(freeme);

    GDrawSetVisible(gw,true);
    GWidgetIndicateFocusGadget(gcd[1].ret);
    while ( !fd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

return( false );
}

static void ChangeFeature(GGadget *list,int index) {
    MacFeat *mf = GGadgetGetListItemSelected(list)->userdata,
	    *all = GGadgetGetUserData(list);

    AskFeature(mf,all,list,index);
}

static int Pref_NewFeat(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_Features);
	MacFeat *new, *mf, *all;
	int expected=0;

	all = GGadgetGetUserData(list);
	for ( mf=all; mf!=NULL; mf=mf->next ) {
	    if ( mf->feature!=expected )
	break;
	    ++expected;
	}
	new = chunkalloc(sizeof(MacFeat));
	new->feature = expected;
	AskFeature(new,all,list,-1);
    }
return( true );
}

static int Pref_DelFeat(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MacFeat *mf, *p, *all, *next;
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_Features);
	int32 len;
	GTextInfo **ti = GGadgetGetList(list,&len);
	int i;

	all = GGadgetGetUserData(list);
	for ( mf = all, p=NULL; mf!=NULL; mf = next ) {
	    next = mf->next;
	    for ( i=len-1; i>=0; --i ) {
		if ( ti[i]->selected && ti[i]->userdata==mf )
	    break;
	    }
	    if ( i>=0 ) {
		if ( p==NULL )
		    all = next;
		else
		    p->next = next;
		mf->next = NULL;
		MacFeatListFree(mf);
	    } else
		p = mf;
	}
	GGadgetSetUserData(list,all);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_FeatureDel),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_FeatureEdit),false);
    }
return( true );
}

static int Pref_EditFeat(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GGadget *list = GWidgetGetControl(GGadgetGetWindow(g),CID_Features);
	ChangeFeature(list,GGadgetGetFirstListSelectedItem(list));
    }
return( true );
}

static int Pref_FeatureSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	GWindow gw = GGadgetGetWindow(g);
	int i, sel_cnt=0;
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected ) ++sel_cnt;
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_FeatureDel),sel_cnt!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_FeatureEdit),sel_cnt==1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	ChangeFeature(g,e->u.control.u.list.changed_index!=-1?e->u.control.u.list.changed_index:
		GGadgetGetFirstListSelectedItem(g));
    }
return( true );
}

static int Pref_DefaultFeat(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GGadget *list = GWidgetGetControl(GGadgetGetWindow(g),CID_Features);
	int inprefs = (intpt) GGadgetGetUserData(g);
	GTextInfo *ti, **arr;
	uint16 cnt;
	/* In preferences the default is the built in data. */
	/* in a font the default is the preference data (which might be built in or might not) */
	MacFeat *def = inprefs ? builtin_mac_feature_map : default_mac_feature_map;

	def = MacFeatCopy(def);
	MacFeatListFree(GGadgetGetUserData(list));
	GGadgetSetUserData(list,def);
	ti = Pref_FeaturesList(def);
	arr = GTextInfoArrayFromList(ti,&cnt);
	GGadgetSetList(list,arr,false);
	GTextInfoListFree(ti);
    }
return( true );
}

void GCDFillMacFeat(GGadgetCreateData *mfgcd,GTextInfo *mflabels, int width,
	MacFeat *all, int fromprefs, GGadgetCreateData *boxes,
	GGadgetCreateData **array) {
    int sgc;
    GGadgetCreateData **butarray = array+4;

    all = MacFeatCopy(all);

    sgc = 0;

    mfgcd[sgc].gd.pos.x = 6; mfgcd[sgc].gd.pos.y = 6;
    mfgcd[sgc].gd.pos.width = 250; mfgcd[sgc].gd.pos.height = 16*12+10;
    mfgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic | gg_list_multiplesel;
    mfgcd[sgc].gd.cid = CID_Features;
    mfgcd[sgc].gd.u.list = Pref_FeaturesList(all);
    mfgcd[sgc].gd.handle_controlevent = Pref_FeatureSel;
    mfgcd[sgc].data = all;
    mfgcd[sgc++].creator = GListCreate;
    array[0] = &mfgcd[sgc-1];

    mfgcd[sgc].gd.pos.x = 6; mfgcd[sgc].gd.pos.y = mfgcd[sgc-1].gd.pos.y+mfgcd[sgc-1].gd.pos.height+10;
    mfgcd[sgc].gd.flags = gg_visible | gg_enabled;
    mflabels[sgc].text = (unichar_t *) S_("MacFeature|_New...");
    mflabels[sgc].text_is_1byte = true;
    mflabels[sgc].text_in_resource = true;
    mfgcd[sgc].gd.label = &mflabels[sgc];
    /*mfgcd[sgc].gd.cid = CID_AnchorRename;*/
    mfgcd[sgc].gd.handle_controlevent = Pref_NewFeat;
    mfgcd[sgc++].creator = GButtonCreate;
    butarray[0] = GCD_Glue; butarray[1] = &mfgcd[sgc-1];

    mfgcd[sgc].gd.pos.x = mfgcd[sgc-1].gd.pos.x+10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    mfgcd[sgc].gd.pos.y = mfgcd[sgc-1].gd.pos.y;
    mfgcd[sgc].gd.flags = gg_visible ;
    mflabels[sgc].text = (unichar_t *) _("_Delete");
    mflabels[sgc].text_is_1byte = true;
    mflabels[sgc].text_in_resource = true;
    mfgcd[sgc].gd.label = &mflabels[sgc];
    mfgcd[sgc].gd.cid = CID_FeatureDel;
    mfgcd[sgc].gd.handle_controlevent = Pref_DelFeat;
    mfgcd[sgc++].creator = GButtonCreate;
    butarray[2] = GCD_Glue; butarray[3] = &mfgcd[sgc-1];

    mfgcd[sgc].gd.pos.x = mfgcd[sgc-1].gd.pos.x+10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    mfgcd[sgc].gd.pos.y = mfgcd[sgc-1].gd.pos.y;
    mfgcd[sgc].gd.flags = gg_visible ;
    mflabels[sgc].text = (unichar_t *) _("_Edit...");
    mflabels[sgc].text_is_1byte = true;
    mflabels[sgc].text_in_resource = true;
    mfgcd[sgc].gd.label = &mflabels[sgc];
    mfgcd[sgc].gd.cid = CID_FeatureEdit;
    mfgcd[sgc].gd.handle_controlevent = Pref_EditFeat;
    mfgcd[sgc++].creator = GButtonCreate;
    butarray[4] = GCD_Glue; butarray[5] = &mfgcd[sgc-1];

    mfgcd[sgc].gd.pos.x = mfgcd[sgc-1].gd.pos.x+10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    mfgcd[sgc].gd.pos.y = mfgcd[sgc-1].gd.pos.y;
    mfgcd[sgc].gd.flags = gg_visible|gg_enabled ;
    mflabels[sgc].text = (unichar_t *) S_("MacFeature|Default");
    mflabels[sgc].text_is_1byte = true;
    mfgcd[sgc].gd.label = &mflabels[sgc];
    mfgcd[sgc].gd.handle_controlevent = Pref_DefaultFeat;
    mfgcd[sgc].data = (void *) (intpt) fromprefs;
    mfgcd[sgc++].creator = GButtonCreate;
    butarray[6] = GCD_Glue; butarray[7] = &mfgcd[sgc-1];
    butarray[8] = GCD_Glue; butarray[9] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = butarray;
    boxes[2].creator = GHBoxCreate;
    array[1] = GCD_Glue;
    array[2] = &boxes[2];
    array[3] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = array;
    boxes[0].creator = GVBoxCreate;
}

void Prefs_ReplaceMacFeatures(GGadget *list) {
    MacFeatListFree(user_mac_feature_map);
    user_mac_feature_map = GGadgetGetUserData(list);
    default_mac_feature_map = user_mac_feature_map;
}

