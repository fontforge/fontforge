/* Copyright (C) 2007-2012 by George Williams */
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

#ifndef FONTFORGE_LANGFREQ_H
#define FONTFORGE_LANGFREQ_H

#include "splinefont.h"

struct letter_frequencies {
	const char *utf8_letter;
	float frequency[4];
	float *afters;
};

struct lang_frequencies {
	uint32_t script;
	uint32_t lang;
	char *note;
	struct letter_frequencies *cnts;
	float *wordlens;
	char *vowels;
	float *consonant_run;
	float *all_consonants;
	float *vowel_run;
};

extern char *RandomParaFromScriptLang(uint32_t script, uint32_t lang, SplineFont *sf, struct lang_frequencies *freq);
extern char **SFScriptLangs(SplineFont *sf, struct lang_frequencies ***_freq);
extern int SF2Scripts(SplineFont *sf, uint32_t scripts[100]);

#endif /* FONTFORGE_LANGFREQ_H */
