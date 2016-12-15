#ifndef FONTFORGE_LANGFREQ_H
#define FONTFORGE_LANGFREQ_H

#include "splinefont.h"

struct letter_frequencies {
	const char *utf8_letter;
	float frequency[4];
	float *afters;
};

struct lang_frequencies {
	uint32 script;
	uint32 lang;
	char *note;
	struct letter_frequencies *cnts;
	float *wordlens;
	char *vowels;
	float *consonant_run;
	float *all_consonants;
	float *vowel_run;
};

extern char *RandomParaFromScriptLang(uint32 script, uint32 lang, SplineFont *sf, struct lang_frequencies *freq);
extern char **SFScriptLangs(SplineFont *sf, struct lang_frequencies ***_freq);
extern int SF2Scripts(SplineFont *sf, uint32 scripts[100]);

#endif /* FONTFORGE_LANGFREQ_H */
