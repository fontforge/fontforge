/* -*- coding: utf-8 -*- */
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

#include "lookups.h"

#include "fontforgevw.h"
#include "fvfonts.h"
#include "macenc.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "tottfgpos.h"
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ttf.h"
#include "xvasprintf.h"

struct opentype_feature_friendlynames friendlies[] = {
    { CHR('a','a','l','t'),	"aalt", N_("Access All Alternates"),	gsub_single_mask|gsub_alternate_mask },
    { CHR('a','b','v','f'),	"abvf", N_("Above Base Forms"),		gsub_single_mask },
    { CHR('a','b','v','m'),	"abvm", N_("Above Base Mark"),		gpos_mark2base_mask|gpos_mark2ligature_mask },
    { CHR('a','b','v','s'),	"abvs", N_("Above Base Substitutions"),	gsub_ligature_mask },
    { CHR('a','f','r','c'),	"afrc", N_("Vertical Fractions"),	gsub_ligature_mask },
    { CHR('a','k','h','n'),	"akhn", N_("Akhand"),			gsub_ligature_mask },
    { CHR('a','l','i','g'),	"alig", N_("Ancient Ligatures"),	gsub_ligature_mask },
    { CHR('b','l','w','f'),	"blwf", N_("Below Base Forms"),		gsub_ligature_mask },
    { CHR('b','l','w','m'),	"blwm", N_("Below Base Mark"),		gpos_mark2base_mask|gpos_mark2ligature_mask },
    { CHR('b','l','w','s'),	"blws", N_("Below Base Substitutions"),	gsub_ligature_mask },
    { CHR('c','2','p','c'),	"c2pc", N_("Capitals to Petite Capitals"),	gsub_single_mask },
    { CHR('c','2','s','c'),	"c2sc", N_("Capitals to Small Capitals"),	gsub_single_mask },
    { CHR('c','a','l','t'),	"calt", N_("Contextual Alternates"),	gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('c','a','s','e'),	"case", N_("Case-Sensitive Forms"),	gsub_single_mask|gpos_single_mask },
    { CHR('c','c','m','p'),	"ccmp", N_("Glyph Composition/Decomposition"),	gsub_multiple_mask|gsub_ligature_mask },
    { CHR('c','f','a','r'),	"cfar", N_("Conjunct Form After Ro"),	gsub_single_mask },
    { CHR('c','j','c','t'),	"cjct", N_("Conjunct Forms"),		gsub_ligature_mask },
    { CHR('c','l','i','g'),	"clig", N_("Contextual Ligatures"),	gsub_reversecchain_mask },
    { CHR('c','p','c','t'),	"cpct", N_("Centered CJK Punctuation"),	gpos_single_mask },
    { CHR('c','p','s','p'),	"cpsp", N_("Capital Spacing"),		gpos_single_mask },
    { CHR('c','s','w','h'),	"cswh", N_("Contextual Swash"),		gsub_reversecchain_mask },
    { CHR('c','u','r','s'),	"curs", N_("Cursive Attachment"),	gpos_cursive_mask },
    { CHR('c','v','0','0'),	"cv00", N_("Character Variants 00"),	gsub_single_mask },
    { CHR('c','v','0','1'),	"cv01", N_("Character Variants 01"),	gsub_single_mask },
    { CHR('c','v','0','2'),	"cv02", N_("Character Variants 02"),	gsub_single_mask },
    { CHR('c','v','0','3'),	"cv03", N_("Character Variants 03"),	gsub_single_mask },
    { CHR('c','v','0','4'),	"cv04", N_("Character Variants 04"),	gsub_single_mask },
    { CHR('c','v','0','5'),	"cv05", N_("Character Variants 05"),	gsub_single_mask },
    { CHR('c','v','0','6'),	"cv06", N_("Character Variants 06"),	gsub_single_mask },
    { CHR('c','v','0','7'),	"cv07", N_("Character Variants 07"),	gsub_single_mask },
    { CHR('c','v','0','8'),	"cv08", N_("Character Variants 08"),	gsub_single_mask },
    { CHR('c','v','0','9'),	"cv09", N_("Character Variants 09"),	gsub_single_mask },
    { CHR('c','v','1','0'),	"cv10", N_("Character Variants 10"),	gsub_single_mask },
    { CHR('c','v','9','9'),	"cv99", N_("Character Variants 99"),	gsub_single_mask },
    { CHR('d','c','a','p'),	"dcap", N_("Drop Caps"),		gsub_single_mask },
    { CHR('d','i','s','t'),	"dist", N_("Distance"),			gpos_pair_mask },
    { CHR('d','l','i','g'),	"dlig", N_("Discretionary Ligatures"),	gsub_ligature_mask },
    { CHR('d','n','o','m'),	"dnom", N_("Denominators"),		gsub_single_mask },
    { CHR('d','p','n','g'),	"dpng", N_("Dipthongs (Obsolete)"),	gsub_ligature_mask },
    { CHR('d','t','l','s'),	"dtls", N_("Dotless Forms"),		gsub_single_mask },
    { CHR('e','x','p','t'),	"expt", N_("Expert Forms"),		gsub_single_mask },
    { CHR('f','a','l','t'),	"falt", N_("Final Glyph On Line"),	gsub_alternate_mask },
    { CHR('f','i','n','2'),	"fin2", N_("Terminal Forms #2"),	gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('f','i','n','3'),	"fin3", N_("Terminal Forms #3"),	gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('f','i','n','a'),	"fina", N_("Terminal Forms"),		gsub_single_mask },
    { CHR('f','l','a','c'),	"flac", N_("Flattened Accents over Capitals"),	gsub_single_mask|gsub_ligature_mask },
    { CHR('f','r','a','c'),	"frac", N_("Diagonal Fractions"),	gsub_single_mask|gsub_ligature_mask },
    { CHR('f','w','i','d'),	"fwid", N_("Full Widths"),		gsub_single_mask|gpos_single_mask },
    { CHR('h','a','l','f'),	"half", N_("Half Forms"),		gsub_ligature_mask },
    { CHR('h','a','l','n'),	"haln", N_("Halant Forms"),		gsub_ligature_mask },
    { CHR('h','a','l','t'),	"halt", N_("Alternative Half Widths"),	gpos_single_mask },
    { CHR('h','i','s','t'),	"hist", N_("Historical Forms"),		gsub_single_mask },
    { CHR('h','k','n','a'),	"hkna", N_("Horizontal Kana Alternatives"),	gsub_single_mask },
    { CHR('h','l','i','g'),	"hlig", N_("Historic Ligatures"),	gsub_ligature_mask },
    { CHR('h','n','g','l'),	"hngl", N_("Hanja to Hangul"),		gsub_single_mask|gsub_alternate_mask },
    { CHR('h','o','j','o'),	"hojo", N_("Hojo (JIS X 0212-1990) Kanji Forms"),	gsub_single_mask },
    { CHR('h','w','i','d'),	"hwid", N_("Half Widths"),		gsub_single_mask|gpos_single_mask },
    { CHR('i','n','i','t'),	"init", N_("Initial Forms"),		gsub_single_mask },
    { CHR('i','s','o','l'),	"isol", N_("Isolated Forms"),		gsub_single_mask },
    { CHR('i','t','a','l'),	"ital", N_("Italics"),			gsub_single_mask },
    { CHR('j','a','l','t'),	"jalt", N_("Justification Alternatives"),	gsub_alternate_mask },
    { CHR('j','a','j','p'),	"jajp", N_("Japanese Forms (Obsolete)"),	gsub_single_mask|gsub_alternate_mask },
    { CHR('j','p','0','4'),	"jp04", N_("JIS2004 Forms"),		gsub_single_mask },
    { CHR('j','p','7','8'),	"jp78", N_("JIS78 Forms"),		gsub_single_mask|gsub_alternate_mask },
    { CHR('j','p','8','3'),	"jp83", N_("JIS83 Forms"),		gsub_single_mask },
    { CHR('j','p','9','0'),	"jp90", N_("JIS90 Forms"),		gsub_single_mask },
    { CHR('k','e','r','n'),	"kern", N_("Horizontal Kerning"),	gpos_pair_mask|gpos_context_mask|gpos_contextchain_mask|kern_statemachine_mask },
    { CHR('l','f','b','d'),	"lfbd", N_("Left Bounds"),		gpos_single_mask },
    { CHR('l','i','g','a'),	"liga", N_("Standard Ligatures"),	gsub_ligature_mask },
    { CHR('l','j','m','o'),	"ljmo", N_("Leading Jamo Forms"),	gsub_ligature_mask },
    { CHR('l','n','u','m'),	"lnum", N_("Lining Figures"),		gsub_single_mask },
    { CHR('l','o','c','l'),	"locl", N_("Localized Forms"),		gsub_single_mask },
    { CHR('m','a','r','k'),	"mark", N_("Mark Positioning"),		gpos_mark2base_mask|gpos_mark2ligature_mask },
    { CHR('m','e','d','2'),	"med2", N_("Medial Forms 2"),		gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('m','e','d','i'),	"medi", N_("Medial Forms"),		gsub_single_mask },
    { CHR('m','g','r','k'),	"mgrk", N_("Mathematical Greek"),	gsub_single_mask },
    { CHR('m','k','m','k'),	"mkmk", N_("Mark to Mark"),		gpos_mark2mark_mask },
    { CHR('m','s','e','t'),	"mset", N_("Mark Positioning via Substitution"),	gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('n','a','l','t'),	"nalt", N_("Alternate Annotation Forms"),	gsub_single_mask|gsub_alternate_mask },
    { CHR('n','l','c','k'),	"nlck", N_("NLC Kanji Forms"),		gsub_single_mask },
    { CHR('n','u','k','t'),	"nukt", N_("Nukta Forms"),		gsub_ligature_mask },
    { CHR('n','u','m','r'),	"numr", N_("Numerators"),		gsub_single_mask },
    { CHR('o','n','u','m'),	"onum", N_("Oldstyle Figures"),		gsub_single_mask },
    { CHR('o','p','b','d'),	"opbd", N_("Optical Bounds"),		gpos_single_mask },
    { CHR('o','r','d','n'),	"ordn", N_("Ordinals"),			gsub_ligature_mask|gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('o','r','n','m'),	"ornm", N_("Ornaments"),		gsub_single_mask|gsub_alternate_mask },
    { CHR('p','a','l','t'),	"palt", N_("Proportional Alternate Metrics"),	gpos_single_mask },
    { CHR('p','c','a','p'),	"pcap", N_("Lowercase to Petite Capitals"),	gsub_single_mask },
    { CHR('p','k','n','a'),	"pkna", N_("Proportional Kana"),	gpos_single_mask },
    { CHR('p','n','u','m'),	"pnum", N_("Proportional Numbers"),	gsub_single_mask },
    { CHR('p','r','e','f'),	"pref", N_("Pre Base Forms"),		gsub_ligature_mask },
    { CHR('p','r','e','s'),	"pres", N_("Pre Base Substitutions"),	gsub_ligature_mask|gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('p','s','t','f'),	"pstf", N_("Post Base Forms"),		gsub_ligature_mask },
    { CHR('p','s','t','s'),	"psts", N_("Post Base Substitutions"),	gsub_ligature_mask },
    { CHR('p','w','i','d'),	"pwid", N_("Proportional Width"),	gsub_single_mask },
    { CHR('q','w','i','d'),	"qwid", N_("Quarter Widths"),		gsub_single_mask|gpos_single_mask },
    { CHR('r','a','n','d'),	"rand", N_("Randomize"),		gsub_alternate_mask },
    { CHR('r','k','r','f'),	"rkrf", N_("Rakar Forms"),		gsub_ligature_mask },
    { CHR('r','l','i','g'),	"rlig", N_("Required Ligatures"),	gsub_ligature_mask },
    { CHR('r','p','h','f'),	"rphf", N_("Reph Form"),		gsub_ligature_mask },
    { CHR('r','t','b','d'),	"rtbd", N_("Right Bounds"),		gpos_single_mask },
    { CHR('r','t','l','a'),	"rtla", N_("Right to Left Alternates"),	gsub_single_mask },
    { CHR('r','t','l','m'),	"rtlm", N_("Right to Left mirrored forms"),	gsub_single_mask },
    { CHR('r','u','b','y'),	"ruby", N_("Ruby Notational Forms"),	gsub_single_mask },
    { CHR('s','a','l','t'),	"salt", N_("Stylistic Alternatives"),	gsub_single_mask|gsub_alternate_mask },
    { CHR('s','i','n','f'),	"sinf", N_("Scientific Inferiors"),	gsub_single_mask },
    { CHR('s','m','c','p'),	"smcp", N_("Lowercase to Small Capitals"),	gsub_single_mask },
    { CHR('s','m','p','l'),	"smpl", N_("Simplified Forms"),		gsub_single_mask },
    { CHR('s','s','0','1'),	"ss01", N_("Style Set 1"),		gsub_single_mask },
    { CHR('s','s','0','2'),	"ss02", N_("Style Set 2"),		gsub_single_mask },
    { CHR('s','s','0','3'),	"ss03", N_("Style Set 3"),		gsub_single_mask },
    { CHR('s','s','0','4'),	"ss04", N_("Style Set 4"),		gsub_single_mask },
    { CHR('s','s','0','5'),	"ss05", N_("Style Set 5"),		gsub_single_mask },
    { CHR('s','s','0','6'),	"ss06", N_("Style Set 6"),		gsub_single_mask },
    { CHR('s','s','0','7'),	"ss07", N_("Style Set 7"),		gsub_single_mask },
    { CHR('s','s','0','8'),	"ss08", N_("Style Set 8"),		gsub_single_mask },
    { CHR('s','s','0','9'),	"ss09", N_("Style Set 9"),		gsub_single_mask },
    { CHR('s','s','1','0'),	"ss10", N_("Style Set 10"),		gsub_single_mask },
    { CHR('s','s','1','1'),	"ss11", N_("Style Set 11"),		gsub_single_mask },
    { CHR('s','s','1','2'),	"ss12", N_("Style Set 12"),		gsub_single_mask },
    { CHR('s','s','1','3'),	"ss13", N_("Style Set 13"),		gsub_single_mask },
    { CHR('s','s','1','4'),	"ss14", N_("Style Set 14"),		gsub_single_mask },
    { CHR('s','s','1','5'),	"ss15", N_("Style Set 15"),		gsub_single_mask },
    { CHR('s','s','1','6'),	"ss16", N_("Style Set 16"),		gsub_single_mask },
    { CHR('s','s','1','7'),	"ss17", N_("Style Set 17"),		gsub_single_mask },
    { CHR('s','s','1','8'),	"ss18", N_("Style Set 18"),		gsub_single_mask },
    { CHR('s','s','1','9'),	"ss19", N_("Style Set 19"),		gsub_single_mask },
    { CHR('s','s','2','0'),	"ss20", N_("Style Set 20"),		gsub_single_mask },
    { CHR('s','s','t','y'),	"ssty", N_("Script Style"),		gsub_single_mask|gsub_alternate_mask },
    { CHR('s','u','b','s'),	"subs", N_("Subscript"),		gsub_single_mask },
    { CHR('s','u','p','s'),	"sups", N_("Superscript"),		gsub_single_mask },
    { CHR('s','w','s','h'),	"swsh", N_("Swash"),			gsub_single_mask|gsub_alternate_mask },
    { CHR('t','i','t','l'),	"titl", N_("Titling"),			gsub_single_mask },
    { CHR('t','j','m','o'),	"tjmo", N_("Trailing Jamo Forms"),	gsub_ligature_mask },
    { CHR('t','n','a','m'),	"tnam", N_("Traditional Name Forms"),	gsub_single_mask },
    { CHR('t','n','u','m'),	"tnum", N_("Tabular Numbers"),		gsub_single_mask },
    { CHR('t','r','a','d'),	"trad", N_("Traditional Forms"),	gsub_single_mask|gsub_alternate_mask },
    { CHR('t','w','i','d'),	"twid", N_("Third Widths"),		gsub_single_mask|gpos_single_mask },
    { CHR('u','n','i','c'),	"unic", N_("Unicase"),			gsub_single_mask },
    { CHR('v','a','l','t'),	"valt", N_("Alternate Vertical Metrics"),	gpos_single_mask },
    { CHR('v','a','t','u'),	"vatu", N_("Vattu Variants"),		gsub_ligature_mask },
    { CHR('v','e','r','t'),	"vert", N_("Vertical Alternates (obs)"),	gsub_single_mask },
    { CHR('v','h','a','l'),	"vhal", N_("Alternate Vertical Half Metrics"),	gpos_single_mask },
    { CHR('v','j','m','o'),	"vjmo", N_("Vowel Jamo Forms"),		gsub_ligature_mask },
    { CHR('v','k','n','a'),	"vkna", N_("Vertical Kana Alternates"),	gsub_single_mask },
    { CHR('v','k','r','n'),	"vkrn", N_("Vertical Kerning"),		gpos_pair_mask|gpos_context_mask|gpos_contextchain_mask|kern_statemachine_mask },
    { CHR('v','p','a','l'),	"vpal", N_("Proportional Alternate Vertical Metrics"),	gpos_single_mask },
    { CHR('v','r','t','2'),	"vrt2", N_("Vertical Rotation & Alternates"),	gsub_single_mask },
    { CHR('z','e','r','o'),	"zero", N_("Slashed Zero"),		gsub_single_mask },
/* This is my hack for setting the "Required feature" field of a script */
    { CHR(' ','R','Q','D'),	" RQD", N_("Required feature"),		gsub_single_mask|gsub_multiple_mask|gsub_alternate_mask|gsub_ligature_mask|gsub_context_mask|gsub_contextchain_mask|gsub_reversecchain_mask|morx_context_mask|gpos_single_mask|gpos_pair_mask|gpos_cursive_mask|gpos_mark2base_mask|gpos_mark2ligature_mask|gpos_mark2mark_mask|gpos_context_mask|gpos_contextchain_mask },
    OPENTYPE_FEATURE_FRIENDLYNAMES_EMPTY
};

static int uint32_cmp(const void *_ui1, const void *_ui2) {
    if ( *(uint32 *) _ui1 > *(uint32 *)_ui2 )
return( 1 );
    if ( *(uint32 *) _ui1 < *(uint32 *)_ui2 )
return( -1 );

return( 0 );
}

static int lang_cmp(const void *_ui1, const void *_ui2) {
    /* The default language is magic, and should come first in the list even */
    /*  if that is not true alphabetical order */
    if ( *(uint32 *) _ui1 == DEFAULT_LANG )
return( -1 );
    if ( *(uint32 *) _ui2 == DEFAULT_LANG )
return( 1 );

    if ( *(uint32 *) _ui1 > *(uint32 *)_ui2 )
return( 1 );
    if ( *(uint32 *) _ui1 < *(uint32 *)_ui2 )
return( -1 );

return( 0 );
}

FeatureScriptLangList *FindFeatureTagInFeatureScriptList(uint32 tag, FeatureScriptLangList *fl) {

    while ( fl!=NULL ) {
	if ( fl->featuretag==tag )
return( fl );
	fl = fl->next;
    }
return( NULL );
}

int FeatureTagInFeatureScriptList(uint32 tag, FeatureScriptLangList *fl) {

    while ( fl!=NULL ) {
	if ( fl->featuretag==tag )
return( true );
	fl = fl->next;
    }
return( false );
}

int ScriptInFeatureScriptList(uint32 script, FeatureScriptLangList *fl) {
    struct scriptlanglist *sl;

    if ( fl==NULL )		/* No features bound to lookup? (nested?) don't restrict by script */
return( true );

    while ( fl!=NULL ) {
	for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
	    if ( sl->script == script )
return( true );
	}
	fl = fl->next;
    }
return( false );
}

int FeatureScriptTagInFeatureScriptList(uint32 feature, uint32 script, FeatureScriptLangList *fl) {
    struct scriptlanglist *sl;

    while ( fl!=NULL ) {
	if ( fl->featuretag == feature ) {
	    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
		if ( sl->script == script )
return( true );
	    }
	}
	fl = fl->next;
    }
return( false );
}

int DefaultLangTagInOneScriptList(struct scriptlanglist *sl) {
    int l;

    for ( l=0; l<sl->lang_cnt; ++l ) {
	uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
	if ( lang==DEFAULT_LANG )
return( true );
    }
return( false );
}

struct scriptlanglist *DefaultLangTagInScriptList(struct scriptlanglist *sl, int DFLT_ok) {

    while ( sl!=NULL ) {
	if ( DFLT_ok || sl->script!=DEFAULT_SCRIPT ) {
	    if ( DefaultLangTagInOneScriptList(sl))
return( sl );
	}
	sl = sl->next;
    }
return( NULL );
}

uint32 *SFScriptsInLookups(SplineFont *sf,int gpos) {
    /* Presumes that either SFFindUnusedLookups or SFFindClearUnusedLookupBits */
    /*  has been called first */
    /* Since MS will sometimes ignore a script if it isn't found in both */
    /*  GPOS and GSUB we want to return the same script list no matter */
    /*  what the setting of gpos ... so we totally ignore that argument */
    /*  and always look at both sets of lookups */

/* Sergey Malkin from MicroSoft tells me:
    Each shaping engine in Uniscribe can decide on its requirements for
    layout tables - some of them require both GSUB and GPOS, in some cases
    any table present is enough, or it can work without any table.

    Sometimes, purpose of the check is to determine if font is supporting
    particular script - if required tables are not there font is just
    rejected by this shaping engine. Sometimes, shaping engine can not just
    reject the font because there are fonts using older shaping technologies
    we still have to support, so it uses some logic when to fallback to
    legacy layout code.

    In your case this is Hebrew, where both tables are required to use
    OpenType processing. Arabic requires both tables too, Latin requires
    GSUB to execute GPOS. But in general, if you have both tables you should
    be safe with any script to get fully featured OpenType shaping.

In other words, if we have a Hebrew font with just GPOS features they won't work,
and MS will not use the font at all. We must add a GSUB table. In the unlikely
event that we had a hebrew font with only GSUB it would not work either.

So if we want our lookups to have a chance of executing under Uniscribe we
better make sure that both tables have the same script set.

(Sergey says we could optimize a little: A Latin GSUB table will run without
a GPOS, but he says the GPOS won't work without a GSUB.)
*/
    int cnt=0, tot=0, i;
    uint32 *scripts = NULL;
    OTLookup *test;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;

    /* So here always give scripts for both (see comment above) no */
    /*  matter what they asked for */
    for ( gpos=0; gpos<2; ++gpos ) {
	for ( test = gpos ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	    if ( test->unused )
	continue;
	    for ( fl=test->features; fl!=NULL; fl=fl->next ) {
		if ( fl->ismac )
	    continue;
		for ( sl=fl->scripts ; sl!=NULL; sl=sl->next ) {
		    for ( i=0; i<cnt; ++i ) {
			if ( sl->script==scripts[i] )
		    break;
		    }
		    if ( i==cnt ) {
			if ( cnt>=tot )
			    scripts = realloc(scripts,(tot+=10)*sizeof(uint32));
			scripts[cnt++] = sl->script;
		    }
		}
	    }
	}
    }

    if ( cnt==0 )
return( NULL );

    /* We want our scripts in alphabetic order */
    qsort(scripts,cnt,sizeof(uint32),uint32_cmp);
    /* add a 0 entry to mark the end of the list */
    if ( cnt>=tot )
	scripts = realloc(scripts,(tot+1)*sizeof(uint32));
    scripts[cnt] = 0;
return( scripts );
}

uint32 *SFLangsInScript(SplineFont *sf,int gpos,uint32 script) {
    /* However, the language lists (I think) are distinct */
    /* But giving a value of -1 for gpos will give us the set of languages in */
    /*  both tables (for this script) */
    int cnt=0, tot=0, i, g, l;
    uint32 *langs = NULL;
    OTLookup *test;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;

    for ( g=0; g<2; ++g ) {
	if (( gpos==0 && g==1 ) || ( gpos==1 && g==0 ))
    continue;
	for ( test = g ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	    if ( test->unused )
	continue;
	    for ( fl=test->features; fl!=NULL; fl=fl->next ) {
		for ( sl=fl->scripts ; sl!=NULL; sl=sl->next ) {
		    if ( sl->script==script ) {
			for ( l=0; l<sl->lang_cnt; ++l ) {
			    uint32 lang;
			    if ( l<MAX_LANG )
				lang = sl->langs[l];
			    else
				lang = sl->morelangs[l-MAX_LANG];
			    for ( i=0; i<cnt; ++i ) {
				if ( lang==langs[i] )
			    break;
			    }
			    if ( i==cnt ) {
				if ( cnt>=tot )
				    langs = realloc(langs,(tot+=10)*sizeof(uint32));
				langs[cnt++] = lang;
			    }
			}
		    }
		}
	    }
	}
    }

    if ( cnt==0 ) {
	/* We add dummy script entries. Because Uniscribe will refuse to */
	/*  process some scripts if they don't have an entry in both GPOS */
	/*  an GSUB. So if a script appears in either table, force it to */
	/*  appear in both. That means we can get scripts with no lookups */
	/*  and hence no languages. It seems that Uniscribe doesn't like */
	/*  that either. So give each such script a dummy default language */
	/*  entry. This is what VOLT does */
	langs = calloc(2,sizeof(uint32));
	langs[0] = DEFAULT_LANG;
return( langs );
    }

    /* We want our languages in alphabetic order */
    qsort(langs,cnt,sizeof(uint32),lang_cmp);
    /* add a 0 entry to mark the end of the list */
    if ( cnt>=tot )
	langs = realloc(langs,(tot+1)*sizeof(uint32));
    langs[cnt] = 0;
return( langs );
}

uint32 *SFFeaturesInScriptLang(SplineFont *sf,int gpos,uint32 script,uint32 lang) {
    int cnt=0, tot=0, i, l, isg;
    uint32 *features = NULL;
    OTLookup *test;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    /* gpos==0 => GSUB, gpos==1 => GPOS, gpos==-1 => both, gpos==-2 => Both & morx & kern */

    if ( sf->cidmaster ) sf=sf->cidmaster;
    for ( isg = 0; isg<2; ++isg ) {
	if ( gpos>=0 && isg!=gpos )
    continue;
	for ( test = isg ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	    if ( test->unused )
	continue;
	    for ( fl=test->features; fl!=NULL; fl=fl->next ) {
		if ( fl->ismac && gpos!=-2 )
	    continue;
		if ( script==0xffffffff ) {
		    for ( i=0; i<cnt; ++i ) {
			if ( fl->featuretag==features[i] )
		    break;
		    }
		    if ( i==cnt ) {
			if ( cnt>=tot )
			    features = realloc(features,(tot+=10)*sizeof(uint32));
			features[cnt++] = fl->featuretag;
		    }
		} else for ( sl=fl->scripts ; sl!=NULL; sl=sl->next ) {
		    if ( sl->script==script ) {
			int matched = false;
			if ( fl->ismac && gpos==-2 )
			    matched = true;
			else for ( l=0; l<sl->lang_cnt; ++l ) {
			    uint32 testlang;
			    if ( l<MAX_LANG )
				testlang = sl->langs[l];
			    else
				testlang = sl->morelangs[l-MAX_LANG];
			    if ( testlang==lang ) {
				matched = true;
			break;
			    }
			}
			if ( matched ) {
			    for ( i=0; i<cnt; ++i ) {
				if ( fl->featuretag==features[i] )
			    break;
			    }
			    if ( i==cnt ) {
				if ( cnt>=tot )
				    features = realloc(features,(tot+=10)*sizeof(uint32));
				features[cnt++] = fl->featuretag;
			    }
			}
		    }
		}
	    }
	}
    }

    if ( sf->design_size!=0 && gpos ) {
	/* The 'size' feature is like no other. It has no lookups and so */
	/*  we will never find it in the normal course of events. If the */
	/*  user has specified a design size, then every script/lang combo */
	/*  gets a 'size' feature which contains no lookups but feature */
	/*  params */
	if ( cnt>=tot )
	    features = realloc(features,(tot+=2)*sizeof(uint32));
	features[cnt++] = CHR('s','i','z','e');
    }

    if ( cnt==0 )
return( calloc(1,sizeof(uint32)) );

    /* We don't care if our features are in alphabetical order here */
    /*  all that matters is whether the complete list of features is */
    /*  ordering here would be irrelevant */
    /* qsort(features,cnt,sizeof(uint32),uint32_cmp); */

    /* add a 0 entry to mark the end of the list */
    if ( cnt>=tot )
	features = realloc(features,(tot+1)*sizeof(uint32));
    features[cnt] = 0;
return( features );
}

OTLookup **SFLookupsInScriptLangFeature(SplineFont *sf,int gpos,uint32 script,uint32 lang, uint32 feature) {
    int cnt=0, tot=0, l;
    OTLookup **lookups = NULL;
    OTLookup *test;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;

    for ( test = gpos ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	if ( test->unused )
    continue;
	for ( fl=test->features; fl!=NULL; fl=fl->next ) {
	    if ( fl->featuretag==feature ) {
		for ( sl=fl->scripts ; sl!=NULL; sl=sl->next ) {
		    if ( sl->script==script ) {
			for ( l=0; l<sl->lang_cnt; ++l ) {
			    uint32 testlang;
			    if ( l<MAX_LANG )
				testlang = sl->langs[l];
			    else
				testlang = sl->morelangs[l-MAX_LANG];
			    if ( testlang==lang ) {
				if ( cnt>=tot )
				    lookups = realloc(lookups,(tot+=10)*sizeof(OTLookup *));
				lookups[cnt++] = test;
	goto found;
			    }
			}
		    }
		}
	    }
	}
	found:;
    }

    if ( cnt==0 )
return( NULL );

    /* lookup order is irrelevant here. might as well leave it in invocation order */
    /* add a 0 entry to mark the end of the list */
    if ( cnt>=tot )
	lookups = realloc(lookups,(tot+1)*sizeof(OTLookup *));
    lookups[cnt] = 0;
return( lookups );
}

static int LigaturesFirstComponentGID(SplineFont *sf,char *components) {
    int gid, ch;
    char *pt;

    for ( pt = components; *pt!='\0' && *pt!=' '; ++pt );
    ch = *pt; *pt = '\0';
    gid = SFFindExistingSlot(sf,-1,components);
    *pt = ch;
return( gid );
}

static int PSTValid(SplineFont *sf,PST *pst) {
    char *start, *pt, ch;
    int ret;

    switch ( pst->type ) {
      case pst_position:
return( true );
      case pst_pair:
return( SCWorthOutputting(SFGetChar(sf,-1,pst->u.pair.paired)) );
      case pst_substitution: case pst_alternate: case pst_multiple:
      case pst_ligature:
	for ( start = pst->u.mult.components; *start ; ) {
	    for ( pt=start; *pt && *pt!=' '; ++pt );
	    ch = *pt; *pt = '\0';
	    ret = SCWorthOutputting(SFGetChar(sf,-1,start));
	    if ( !ret ) {
		LogError(_("Lookup subtable contains unused glyph %s making the whole subtable invalid"), start);
		*pt = ch;
return( false );
	    }
	    *pt = ch;
	    if ( ch==0 )
		start = pt;
	    else
		start = pt+1;
	}
      default:
      break;
    }
return( true );
}

SplineChar **SFGlyphsWithPSTinSubtable(SplineFont *sf,struct lookup_subtable *subtable) {
    uint8 *used = calloc(sf->glyphcnt,sizeof(uint8));
    SplineChar **glyphs, *sc;
    int i, k, gid, cnt;
    KernPair *kp;
    PST *pst;
    int ispair = subtable->lookup->lookup_type == gpos_pair;
    int isliga = subtable->lookup->lookup_type == gsub_ligature;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc = sf->glyphs[i]) ) {
	if ( ispair ) {
	    for ( k=0; k<2; ++k ) {
		for ( kp= k ? sc->kerns : sc->vkerns; kp!=NULL ; kp=kp->next ) {
		    if ( !SCWorthOutputting(kp->sc))
		continue;
		    if ( kp->subtable == subtable ) {
			used[i] = true;
    goto continue_;
		    }
		}
	    }
	}
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable == subtable && PSTValid(sf,pst)) {
		if ( !isliga ) {
		    used[i] = true;
    goto continue_;
		} else {
		    gid = LigaturesFirstComponentGID(sf,pst->u.lig.components);
		    pst->u.lig.lig = sc;
		    if ( gid!=-1 )
			used[gid] = true;
		    /* can't continue here. ffi might be "f+f+i" and "ff+i" */
		    /*  and we need to mark both "f" and "ff" as used */
		}
	    }
	}
    continue_: ;
    }

    for ( i=cnt=0 ; i<sf->glyphcnt; ++i )
	if ( used[i] )
	    ++cnt;

    if ( cnt==0 ) {
	free(used);
return( NULL );
    }
    glyphs = malloc((cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0 ; i<sf->glyphcnt; ++i ) {
	if ( used[i] )
	    glyphs[cnt++] = sf->glyphs[i];
    }
    glyphs[cnt] = NULL;
    free(used);
return( glyphs );
}

SplineChar **SFGlyphsWithLigatureinLookup(SplineFont *sf,struct lookup_subtable *subtable) {
    uint8 *used = calloc(sf->glyphcnt,sizeof(uint8));
    SplineChar **glyphs, *sc;
    int i, cnt;
    PST *pst;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc = sf->glyphs[i]) ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable == subtable ) {
		used[i] = true;
    goto continue_;
	    }
	}
    continue_: ;
    }

    for ( i=cnt=0 ; i<sf->glyphcnt; ++i )
	if ( used[i] )
	    ++cnt;

    if ( cnt==0 ) {
	free(used);
return( NULL );
    }

    glyphs = malloc((cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0 ; i<sf->glyphcnt; ++i ) {
	if ( used[i] )
	    glyphs[cnt++] = sf->glyphs[i];
    }
    glyphs[cnt] = NULL;
    free(used);
return( glyphs );
}

static void TickLookupKids(OTLookup *otl) {
    struct lookup_subtable *sub;
    int i,j;

    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	if ( sub->fpst!=NULL ) {
	    for ( i=0; i<sub->fpst->rule_cnt; ++i ) {
		struct fpst_rule *rule = &sub->fpst->rules[i];
		for ( j=0; j<rule->lookup_cnt; ++j ) {
		    if ( rule->lookups[j].lookup!=NULL )
			rule->lookups[j].lookup->in_gpos = true;
		}
	    }
	}
    }
}

void SFFindUnusedLookups(SplineFont *sf) {
    OTLookup *test;
    struct lookup_subtable *sub;
    int gpos;
    AnchorClass *ac;
    AnchorPoint *ap;
    SplineChar *sc;
    KernPair *kp;
    PST *pst;
    int i,k,gid,isv;
    SplineFont *_sf = sf;
    Justify *jscripts;
    struct jstf_lang *jlangs;

    if ( _sf->cidmaster ) _sf = _sf->cidmaster;

    /* Some things are obvious. If a subtable consists of a kernclass or some */
    /*  such, then obviously it is used. But more distributed info takes more */
    /*  work. So mark anything easy as used, and anything difficult as unused */
    /* We'll work on the difficult things later */
    for ( gpos=0; gpos<2; ++gpos ) {
	for ( test = gpos ? _sf->gpos_lookups : _sf->gsub_lookups; test!=NULL; test = test->next ) {
	    for ( sub = test->subtables; sub!=NULL; sub=sub->next ) {
		if ( sub->kc!=NULL || sub->fpst!=NULL || sub->sm!=NULL ) {
		    sub->unused = false;
	    continue;
		}
		sub->unused = true;
		/* We'll turn the following bit back on if there turns out */
		/*  to be an anchor class attached to it -- that is subtly */
		/*  different than being unused -- unused will be set if all */
		/*  acs are unused, this bit will be on if there are unused */
		/*  classes that still refer to us. */
		sub->anchor_classes = false;
	    }
	}
    }

    /* To be useful an anchor class must have both at least one base and one mark */
    /*  (for cursive anchors that means at least one entry and at least one exit) */
    /* Start by assuming the worst */
    for ( ac = _sf->anchor; ac!=NULL; ac=ac->next )
	ac->has_mark = ac->has_base = false;

    /* Ok, for each glyph, look at all lookups (or anchor classes) it affects */
    /*  and mark the appropriate parts of them as used */
    k = 0;
    do {
	sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( SCWorthOutputting(sc = sf->glyphs[gid]) ) {
	    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
		switch ( ap->type ) {
		  case at_mark: case at_centry:
		    ap->anchor->has_mark = true;
		  break;
		  case at_basechar: case at_baselig: case at_basemark:
		  case at_cexit:
		    ap->anchor->has_base = true;
		  break;
		  default:
		  break;
		}
	    }
	    for ( isv=0; isv<2; ++isv ) {
		for ( kp= isv ? sc->kerns : sc->vkerns ; kp!=NULL; kp=kp->next ) {
		    if ( SCWorthOutputting(kp->sc))
			kp->subtable->unused = false;
		}
	    }
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable==NULL )
	    continue;
		if ( !PSTValid(sf,pst))
	    continue;
		pst->subtable->unused = false;
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );

    /* Finally for any anchor class that has both a mark and a base then it is */
    /*  used, and its lookup is also used */
    /* Also, even if unused, as long as the anchor class exists we must keep */
    /*  the subtable around */
    for ( ac = _sf->anchor; ac!=NULL; ac=ac->next ) {
        if ( ac->subtable==NULL )
    continue;
	ac->subtable->anchor_classes = true;
	if ( ac->has_mark && ac->has_base )
	    ac->subtable->unused = false;
    }

    /* Now for each lookup, a lookup is unused if ALL subtables are unused */
    for ( gpos=0; gpos<2; ++gpos ) {
	for ( test = gpos ? _sf->gpos_lookups : _sf->gsub_lookups; test!=NULL; test = test->next ) {
	    test->unused = test->empty = true;
	    for ( sub=test->subtables; sub!=NULL; sub=sub->next ) {
		if ( !sub->unused )
		    test->unused = false;
		if ( !sub->unused && !sub->anchor_classes ) {
		    test->empty = false;
	    break;
		}
	    }
	}
    }

    /* I store JSTF max lookups in the gpos list because they have the same */
    /*  format. But now I need to tease them out and learn which lookups are */
    /*  used in GPOS and which in JSTF (and conceivably which get duplicated */
    /*  and placed in both) */
    for ( test = sf->gpos_lookups; test!=NULL; test = test->next ) {
	test->only_jstf = test->in_jstf = test->in_gpos = false;
	if ( test->features!=NULL )
	    test->in_gpos = true;
    }
    for ( jscripts = sf->justify; jscripts!=NULL; jscripts=jscripts->next ) {
	for ( jlangs=jscripts->langs; jlangs!=NULL; jlangs=jlangs->next ) {
	    for ( i=0; i<jlangs->cnt; ++i ) {
		struct jstf_prio *prio = &jlangs->prios[i];
		if ( prio->enableShrink!=NULL )
		    for ( k=0; prio->enableShrink[k]!=NULL; ++k )
			prio->enableShrink[k]->in_gpos = true;
		if ( prio->disableShrink!=NULL )
		    for ( k=0; prio->disableShrink[k]!=NULL; ++k )
			prio->disableShrink[k]->in_gpos = true;
		if ( prio->enableExtend!=NULL )
		    for ( k=0; prio->enableExtend[k]!=NULL; ++k )
			prio->enableExtend[k]->in_gpos = true;
		if ( prio->disableExtend!=NULL )
		    for ( k=0; prio->disableExtend[k]!=NULL; ++k )
			prio->disableExtend[k]->in_gpos = true;
		if ( prio->maxShrink!=NULL )
		    for ( k=0; prio->maxShrink[k]!=NULL; ++k )
			prio->maxShrink[k]->in_jstf = true;
		if ( prio->maxExtend!=NULL )
		    for ( k=0; prio->maxExtend[k]!=NULL; ++k )
			prio->maxExtend[k]->in_jstf = true;
	    }
	}
    }
    for ( test = sf->gpos_lookups; test!=NULL; test = test->next ) {
	if ( test->in_gpos && (test->lookup_type==gpos_context || test->lookup_type==gpos_contextchain))
	    TickLookupKids(test);
    }
    for ( test = sf->gpos_lookups; test!=NULL; test = test->next )
	test->only_jstf = test->in_jstf && !test->in_gpos;
}

void SFFindClearUnusedLookupBits(SplineFont *sf) {
    OTLookup *test;
    int gpos;

    for ( gpos=0; gpos<2; ++gpos ) {
	for ( test = gpos ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	    test->unused = false;
	    test->empty = false;
	    test->def_lang_checked = false;
	}
    }
}

static void SFRemoveAnchorPointsOfAC(SplineFont *sf,AnchorClass *ac) {
    int gid;
    SplineChar *sc;
    AnchorPoint *ap, *prev, *next;

    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	for ( prev=NULL, ap=sc->anchor; ap!=NULL; ap=next ) {
	    next = ap->next;
	    if ( ap->anchor!=ac )
		prev = ap;
	    else {
		if ( prev==NULL )
		    sc->anchor = next;
		else
		    prev->next = next;
		ap->next = NULL;
		AnchorPointsFree(ap);
	    }
	}
    }
}

static OTLookup **RemoveFromList(OTLookup **list, OTLookup *dying) {
    int i,j;

    if ( list==NULL )
return( NULL );
    for ( i=0; list[i]!=NULL; ++i ) {
	if ( list[i]==dying ) {
	    for ( j=i+1; ; ++j ) {
		list[j-1] = list[j];
		if ( list[j]==NULL )
	    break;
	    }
	}
    }
    if ( list[0]==NULL ) {
	free(list);
return( NULL );
    }
return( list );
}

static void RemoveJSTFReferences(SplineFont *sf,OTLookup *dying) {
    Justify *jscript;
    struct jstf_lang *jlang;
    int i;

    for ( jscript = sf->justify; jscript!=NULL; jscript=jscript->next ) {
	for ( jlang=jscript->langs; jlang!=NULL; jlang=jlang->next ) {
	    for ( i=0; i<jlang->cnt; ++i ) {
		struct jstf_prio *prio = &jlang->prios[i];
		prio->enableShrink = RemoveFromList(prio->enableShrink,dying);
		prio->disableShrink = RemoveFromList(prio->disableShrink,dying);
		prio->enableExtend = RemoveFromList(prio->enableExtend,dying);
		prio->disableExtend = RemoveFromList(prio->disableExtend,dying);
		prio->maxShrink = RemoveFromList(prio->maxShrink,dying);
		prio->maxExtend = RemoveFromList(prio->maxExtend,dying);
	    }
	}
    }
}

static void RemoveNestedReferences(SplineFont *sf,int isgpos) {
    OTLookup *otl;
    struct lookup_subtable *sub;
    int i,j,k;

    for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl = otl->next ) {
	if ( otl->lookup_type==morx_context ) {
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		ASM *sm = sub->sm;
		if ( sm->type==asm_context ) {
		    for ( i=0; i<sm->state_cnt*sm->class_cnt; ++i ) {
			struct asm_state *state = &sm->state[i];
			if ( state->u.context.mark_lookup == otl )
			    state->u.context.mark_lookup = NULL;
			if ( state->u.context.cur_lookup == otl )
			    state->u.context.cur_lookup = NULL;
		    }
		}
	    }
	/* Reverse chaining tables do not reference lookups. The match pattern*/
	/*  is a (exactly one) coverage table, and each glyph in that table   */
	/*  as an inline replacement. There is no lookup to do the replacement*/
	/* (so we ignore it here) */
	} else if ( otl->lookup_type==gsub_context || otl->lookup_type==gsub_contextchain ||
		otl->lookup_type==gpos_context || otl->lookup_type==gpos_contextchain ) {
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		FPST *fpst = sub->fpst;
		for ( i=0; i<fpst->rule_cnt; ++i ) {
		    for ( j=0; j<fpst->rules[i].lookup_cnt; ++j ) {
			if ( fpst->rules[i].lookups[j].lookup == otl ) {
			    for ( k=j+1; k<fpst->rules[i].lookup_cnt; ++k )
				fpst->rules[i].lookups[k-1] = fpst->rules[i].lookups[k];
			    --fpst->rules[i].lookup_cnt;
			    --j;
			}
		    }
		}
	    }
	}
    }
}

void SFRemoveUnusedLookupSubTables(SplineFont *sf,
	int remove_incomplete_anchorclasses,
	int remove_unused_lookups) {
    int gpos;
    struct lookup_subtable *sub, *subnext, *prev;
    AnchorClass *ac, *acprev, *acnext;
    OTLookup *otl, *otlprev, *otlnext;

    /* Presumes someone has called SFFindUnusedLookups first */

    if ( remove_incomplete_anchorclasses ) {
	for ( acprev=NULL, ac=sf->anchor; ac!=NULL; ac=acnext ) {
	    acnext = ac->next;
	    if ( ac->has_mark && ac->has_base )
		acprev = ac;
	    else {
		SFRemoveAnchorPointsOfAC(sf,ac);
		ac->next = NULL;
		AnchorClassesFree(ac);
		if ( acprev==NULL )
		    sf->anchor = acnext;
		else
		    acprev = acnext;
	    }
	}
    }

    for ( gpos=0; gpos<2; ++gpos ) {
	for ( otl = gpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl = otlnext ) {
	    otlnext = otl->next;
	    if ( remove_unused_lookups && (otl->empty ||
		    (otl->unused && remove_incomplete_anchorclasses)) ) {
		if ( otlprev!=NULL )
		    otlprev->next = otlnext;
		else if ( gpos )
		    sf->gpos_lookups = otlnext;
		else
		    sf->gsub_lookups = otlnext;
		RemoveNestedReferences(sf,gpos);
		OTLookupFree(otl);
	    } else {
		for ( prev=NULL, sub=otl->subtables; sub!=NULL; sub=subnext ) {
		    subnext = sub->next;
		    if ( sub->unused &&
			    (!sub->anchor_classes ||
			      remove_incomplete_anchorclasses )) {
			if ( prev==NULL )
			    otl->subtables = subnext;
			else
			    prev->next = subnext;
			free(sub->subtable_name);
			chunkfree(sub,sizeof(*sub));
		    } else
			prev = sub;
		}
	    }
	}
    }
}

void SFRemoveLookupSubTable(SplineFont *sf,struct lookup_subtable *sub, int remove_acs) {
    OTLookup *otl = sub->lookup;
    struct lookup_subtable *subprev, *subtest;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    if ( sub->sm!=NULL ) {
	ASM *prev = NULL, *test;
	for ( test=sf->sm; test!=NULL && test!=sub->sm; prev=test, test=test->next );
	if ( prev==NULL )
	    sf->sm = sub->sm->next;
	else
	    prev->next = sub->sm->next;
	sub->sm->next = NULL;
	ASMFree(sub->sm);
	sub->sm = NULL;
    } else if ( sub->fpst!=NULL ) {
	FPST *prev = NULL, *test;
	for ( test=sf->possub; test!=NULL && test!=sub->fpst; prev=test, test=test->next );
	if ( prev==NULL )
	    sf->possub = sub->fpst->next;
	else
	    prev->next = sub->fpst->next;
	sub->fpst->next = NULL;
	FPSTFree(sub->fpst);
	sub->fpst = NULL;
    } else if ( sub->kc!=NULL ) {
	KernClass *prev = NULL, *test;
	for ( test=sf->kerns; test!=NULL && test!=sub->kc; prev=test, test=test->next );
	if ( test!=NULL ) {
	    if ( prev==NULL )
		sf->kerns = sub->kc->next;
	    else
		prev->next = sub->kc->next;
	} else {
	    for ( prev=NULL,test=sf->vkerns; test!=NULL && test!=sub->kc; prev=test, test=test->next );
	    if ( prev==NULL )
		sf->vkerns = sub->kc->next;
	    else
		prev->next = sub->kc->next;
	}
	sub->kc->next = NULL;
	KernClassListFree(sub->kc);
	sub->kc = NULL;
    } else if ( otl->lookup_type==gpos_cursive || otl->lookup_type==gpos_mark2base ||
	    otl->lookup_type==gpos_mark2ligature || otl->lookup_type==gpos_mark2mark ) {
	AnchorClass *ac, *acnext;
	for ( ac=sf->anchor; ac!=NULL; ac=acnext ) {
	    acnext = ac->next;
	    if ( ac->subtable==sub ) {
                if ( remove_acs )
		    SFRemoveAnchorClass(sf,ac);
                else
                    ac->subtable = NULL;
            }
	}
    } else {
	int i,k,v;
	SplineChar *sc;
	SplineFont *_sf;
	PST *pst, *prev, *next;
	KernPair *kp, *kpprev, *kpnext;
	k=0;
	do {
	    _sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	    for ( i=0; i<_sf->glyphcnt; ++i ) if ( (sc=_sf->glyphs[i])!=NULL ) {
		for ( pst=sc->possub, prev=NULL ; pst!=NULL; pst=next ) {
		    next = pst->next;
		    if ( pst->subtable==sub ) {
			if ( prev==NULL )
			    sc->possub = next;
			else
			    prev->next = next;
			pst->next = NULL;
			PSTFree(pst);
		    } else
			prev = pst;
		}
		for ( v=0; v<2; ++v ) {
		    for ( kp=v ? sc->vkerns : sc->kerns, kpprev=NULL ; kp!=NULL; kp=kpnext ) {
			kpnext = kp->next;
			if ( kp->subtable==sub ) {
			    if ( kpprev!=NULL )
				kpprev->next = kpnext;
			    else if ( v )
				sc->vkerns = kpnext;
			    else
				sc->kerns = kpnext;
			    kp->next = NULL;
			    KernPairsFree(kp);
			} else
			    kpprev = kp;
		    }
		}
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
    }

    subprev = NULL;
    for ( subtest = otl->subtables; subtest!=NULL && subtest!=sub; subprev = subtest, subtest=subtest->next );
    if ( subprev==NULL )
	otl->subtables = sub->next;
    else
	subprev->next = sub->next;
    free(sub->subtable_name);
    free(sub->suffix);
    chunkfree(sub,sizeof(struct lookup_subtable));
}

void SFRemoveLookup(SplineFont *sf,OTLookup *otl,int remove_acs) {
    OTLookup *test, *prev;
    int isgpos;
    struct lookup_subtable *sub, *subnext;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    for ( sub = otl->subtables; sub!=NULL; sub=subnext ) {
	subnext = sub->next;
	SFRemoveLookupSubTable(sf,sub,remove_acs);
    }

    for ( prev=NULL, test=sf->gpos_lookups; test!=NULL && test!=otl; prev=test, test=test->next );
    if ( test==NULL ) {
	isgpos = false;
	for ( prev=NULL, test=sf->gsub_lookups; test!=NULL && test!=otl; prev=test, test=test->next );
    } else
	isgpos = true;
    if ( prev!=NULL )
	prev->next = otl->next;
    else if ( isgpos )
	sf->gpos_lookups = otl->next;
    else
	sf->gsub_lookups = otl->next;

    RemoveNestedReferences(sf,isgpos);
    RemoveJSTFReferences(sf,otl);

    otl->next = NULL;
    OTLookupFree(otl);
}

struct lookup_subtable *SFFindLookupSubtable(SplineFont *sf,char *name) {
    int isgpos;
    OTLookup *otl;
    struct lookup_subtable *sub;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    if ( name==NULL )
return( NULL );

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups ; otl!=NULL; otl=otl->next ) {
	    for ( sub = otl->subtables; sub!=NULL; sub=sub->next ) {
		if ( strcmp(name,sub->subtable_name)==0 )
return( sub );
	    }
	}
    }
return( NULL );
}

struct lookup_subtable *SFFindLookupSubtableAndFreeName(SplineFont *sf,char *name) {
    struct lookup_subtable *sub = SFFindLookupSubtable(sf,name);
    free(name);
return( sub );
}

OTLookup *SFFindLookup(SplineFont *sf,char *name) {
    int isgpos;
    OTLookup *otl;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    if ( name==NULL )
return( NULL );

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups ; otl!=NULL; otl=otl->next ) {
	    if ( strcmp(name,otl->lookup_name)==0 )
return( otl );
	}
    }
return( NULL );
}

void FListAppendScriptLang(FeatureScriptLangList *fl,uint32 script_tag,uint32 lang_tag) {
    struct scriptlanglist *sl;
    int l;

    for ( sl = fl->scripts; sl!=NULL && sl->script!=script_tag; sl=sl->next );
    if ( sl==NULL ) {
	sl = chunkalloc(sizeof(struct scriptlanglist));
	sl->script = script_tag;
	sl->next = fl->scripts;
	fl->scripts = sl;
    }
    for ( l=0; l<MAX_LANG && l<sl->lang_cnt && sl->langs[l]!=lang_tag; ++l );
    if ( l>=MAX_LANG && l<sl->lang_cnt ) {
	while ( l<sl->lang_cnt && sl->morelangs[l-MAX_LANG]!=lang_tag )
	    ++l;
    }
    if ( l>=sl->lang_cnt ) {
	if ( l<MAX_LANG )
	    sl->langs[l] = lang_tag;
	else {
	    if ( l%MAX_LANG == 0 )
		sl->morelangs = realloc(sl->morelangs,l*sizeof(uint32));
		/* We've just allocated MAX_LANG-1 more than we need */
		/*  so we don't do quite some many allocations */
	    sl->morelangs[l-MAX_LANG] = lang_tag;
	}
	++sl->lang_cnt;
    }
}

void FListsAppendScriptLang(FeatureScriptLangList *fl,uint32 script_tag,uint32 lang_tag) {
    while ( fl!=NULL ) {
	FListAppendScriptLang(fl,script_tag,lang_tag);
	fl=fl->next;
    }
}

char *SuffixFromTags(FeatureScriptLangList *fl) {
    static struct { uint32 tag; const char *suffix; } tags2suffix[] = {
        { CHR('v','r','t','2'), "vert" },	/* Will check for vrt2 later */
        { CHR('o','n','u','m'), "oldstyle" },
        { CHR('s','u','p','s'), "superior" },
        { CHR('s','u','b','s'), "inferior" },
        { CHR('s','w','s','h'), "swash" },
        { CHR('f','w','i','d'), "full" },
        { CHR('h','w','i','d'), "hw" },
        { 0, NULL }
    };
    int i;

    while ( fl!=NULL ) {
	for ( i=0; tags2suffix[i].tag!=0; ++i )
	    if ( tags2suffix[i].tag==fl->featuretag )
return( copy( tags2suffix[i].suffix ));
	fl = fl->next;
    }
return( NULL );
}

const char *lookup_type_names[2][10] = {
    {   N_("Undefined substitution"), N_("Single Substitution"), N_("Multiple Substitution"),
        N_("Alternate Substitution"), N_("Ligature Substitution"), N_("Contextual Substitution"),
        N_("Contextual Chaining Substitution"), N_("Extension"),
        N_("Reverse Contextual Chaining Substitution")
    },
    {   N_("Undefined positioning"), N_("Single Positioning"), N_("Pairwise Positioning (kerning)"),
        N_("Cursive attachment"), N_("Mark to base attachment"),
        N_("Mark to Ligature attachment"), N_("Mark to Mark attachment"),
        N_("Contextual Positioning"), N_("Contextual Chaining Positioning"),
        N_("Extension")
    }
};

/* This is a non-ui based copy of a similar list in lookupui.c */
static struct {
    const char *text;
    uint32 tag;
} localscripts[] = {
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Arabic", ignore "Script|" */
    { N_("Script|Arabic"), CHR('a','r','a','b') },
    { N_("Script|Aramaic"), CHR('a','r','a','m') },
    { N_("Script|Armenian"), CHR('a','r','m','n') },
    { N_("Script|Avestan"), CHR('a','v','e','s') },
    { N_("Script|Balinese"), CHR('b','a','l','i') },
    { N_("Script|Batak"), CHR('b','a','t','k') },
    { N_("Script|Bengali"), CHR('b','e','n','g') },
    { N_("Script|Bengali2"), CHR('b','n','g','2') },
    { N_("Bliss Symbolics"), CHR('b','l','i','s') },
    { N_("Bopomofo"), CHR('b','o','p','o') },
    { NU_("Brāhmī"), CHR('b','r','a','h') },
    { N_("Braille"), CHR('b','r','a','i') },
    { N_("Script|Buginese"), CHR('b','u','g','i') },
    { N_("Script|Buhid"), CHR('b','u','h','d') },
    { N_("Byzantine Music"), CHR('b','y','z','m') },
    { N_("Canadian Syllabics"), CHR('c','a','n','s') },
    { N_("Carian"), CHR('c','a','r','i') },
    { N_("Cherokee"), CHR('c','h','a','m') },
    { N_("Script|Cham"), CHR('c','h','a','m') },
    { N_("Script|Cherokee"), CHR('c','h','e','r') },
    { N_("Cirth"), CHR('c','i','r','t') },
    { N_("CJK Ideographic"), CHR('h','a','n','i') },
    { N_("Script|Coptic"), CHR('c','o','p','t') },
    { N_("Cypro-Minoan"), CHR('c','p','r','t') },
    { N_("Cypriot syllabary"), CHR('c','p','m','n') },
    { N_("Cyrillic"), CHR('c','y','r','l') },
    { N_("Script|Default"), CHR('D','F','L','T') },
    { N_("Deseret (Mormon)"), CHR('d','s','r','t') },
    { N_("Devanagari"), CHR('d','e','v','a') },
    { N_("Devanagari2"), CHR('d','e','v','2') },
/*  { N_("Egyptian demotic"), CHR('e','g','y','d') }, */
/*  { N_("Egyptian hieratic"), CHR('e','g','y','h') }, */
/* GT: Someone asked if FontForge actually was prepared generate hieroglyph output */
/* GT: because of this string. No. But OpenType and Unicode have placeholders for */
/* GT: dealing with these scripts against the day someone wants to use them. So */
/* GT: FontForge must be prepared to deal with those placeholders if nothing else. */
/*  { N_("Egyptian hieroglyphs"), CHR('e','g','y','p') }, */
    { N_("Script|Ethiopic"), CHR('e','t','h','i') },
    { N_("Script|Georgian"), CHR('g','e','o','r') },
    { N_("Glagolitic"), CHR('g','l','a','g') },
    { N_("Gothic"), CHR('g','o','t','h') },
    { N_("Script|Greek"), CHR('g','r','e','k') },
    { N_("Script|Gujarati"), CHR('g','u','j','r') },
    { N_("Script|Gujarati2"), CHR('g','j','r','2') },
    { N_("Gurmukhi"), CHR('g','u','r','u') },
    { N_("Gurmukhi2"), CHR('g','u','r','2') },
    { N_("Hangul Jamo"), CHR('j','a','m','o') },
    { N_("Hangul"), CHR('h','a','n','g') },
    { NU_("Script|Hanunóo"), CHR('h','a','n','o') },
    { N_("Script|Hebrew"), CHR('h','e','b','r') },
/*  { N_("Pahawh Hmong"), CHR('h','m','n','g') },*/
/*  { N_("Indus (Harappan)"), CHR('i','n','d','s') },*/
    { N_("Script|Javanese"), CHR('j','a','v','a') },
    { N_("Kayah Li"), CHR('k','a','l','i') },
    { N_("Hiragana & Katakana"), CHR('k','a','n','a') },
    { NU_("Kharoṣṭhī"), CHR('k','h','a','r') },
    { N_("Script|Kannada"), CHR('k','n','d','a') },
    { N_("Script|Kannada2"), CHR('k','n','d','2') },
    { N_("Script|Khmer"), CHR('k','h','m','r') },
    { N_("Script|Kharosthi"), CHR('k','h','a','r') },
    { N_("Script|Lao") , CHR('l','a','o',' ') },
    { N_("Script|Latin"), CHR('l','a','t','n') },
    { NU_("Lepcha (Róng)"), CHR('l','e','p','c') },
    { N_("Script|Limbu"), CHR('l','i','m','b') },	/* Not in ISO 15924 !!!!!, just guessing */
    { N_("Linear A"), CHR('l','i','n','a') },
    { N_("Linear B"), CHR('l','i','n','b') },
    { N_("Lycian"), CHR('l','y','c','i') },
    { N_("Lydian"), CHR('l','y','d','i') },
    { N_("Script|Mandaean"), CHR('m','a','n','d') },
/*  { N_("Mayan hieroglyphs"), CHR('m','a','y','a') },*/
    { NU_("Script|Malayālam"), CHR('m','l','y','m') },
    { NU_("Script|Malayālam2"), CHR('m','l','m','2') },
    { NU_("Mathematical Alphanumeric Symbols"), CHR('m','a','t','h') },
    { N_("Script|Mongolian"), CHR('m','o','n','g') },
    { N_("Musical"), CHR('m','u','s','c') },
    { N_("Script|Myanmar"), CHR('m','y','m','r') },
    { N_("New Tai Lue"), CHR('t','a','l','u') },
    { N_("N'Ko"), CHR('n','k','o',' ') },
    { N_("Ogham"), CHR('o','g','a','m') },
    { N_("Ol Chiki"), CHR('o','l','c','k') },
    { N_("Old Italic (Etruscan, Oscan, etc.)"), CHR('i','t','a','l') },
    { N_("Script|Old Permic"), CHR('p','e','r','m') },
    { N_("Old Persian cuneiform"), CHR('x','p','e','o') },
    { N_("Script|Oriya"), CHR('o','r','y','a') },
    { N_("Script|Oriya2"), CHR('o','r','y','2') },
    { N_("Osmanya"), CHR('o','s','m','a') },
    { N_("Script|Pahlavi"), CHR('p','a','l','v') },
    { N_("Script|Phags-pa"), CHR('p','h','a','g') },
    { N_("Script|Phoenician"), CHR('p','h','n','x') },
    { N_("Phaistos"), CHR('p','h','s','t') },
    { N_("Pollard Phonetic"), CHR('p','l','r','d') },
    { N_("Rejang"), CHR('r','j','n','g') },
    { N_("Rongorongo"), CHR('r','o','r','o') },
    { N_("Runic"), CHR('r','u','n','r') },
    { N_("Saurashtra"), CHR('s','a','u','r') },
    { N_("Shavian"), CHR('s','h','a','w') },
    { N_("Script|Sinhala"), CHR('s','i','n','h') },
    { N_("Script|Sumero-Akkadian Cuneiform"), CHR('x','s','u','x') },
    { N_("Script|Sundanese"), CHR('s','u','n','d') },
    { N_("Script|Syloti Nagri"), CHR('s','y','l','o') },
    { N_("Script|Syriac"), CHR('s','y','r','c') },
    { N_("Script|Tagalog"), CHR('t','g','l','g') },
    { N_("Script|Tagbanwa"), CHR('t','a','g','b') },
    { N_("Tai Le"), CHR('t','a','l','e') },	/* Not in ISO 15924 !!!!!, just guessing */
    { N_("Tai Lu"), CHR('t','a','l','a') },	/* Not in ISO 15924 !!!!!, just guessing */
    { N_("Script|Tamil"), CHR('t','a','m','l') },
    { N_("Script|Tamil2"), CHR('t','m','l','2') },
    { N_("Script|Telugu"), CHR('t','e','l','u') },
    { N_("Script|Telugu2"), CHR('t','e','l','2') },
    { N_("Tengwar"), CHR('t','e','n','g') },
    { N_("Thaana"), CHR('t','h','a','a') },
    { N_("Script|Thai"), CHR('t','h','a','i') },
    { N_("Script|Tibetan"), CHR('t','i','b','t') },
    { N_("Tifinagh (Berber)"), CHR('t','f','n','g') },
    { N_("Script|Ugaritic"), CHR('u','g','r','t') },	/* Not in ISO 15924 !!!!!, just guessing */
    { N_("Script|Vai"), CHR('v','a','i',' ') },
/*  { N_("Visible Speech"), CHR('v','i','s','p') },*/
    { N_("Cuneiform, Ugaritic"), CHR('x','u','g','a') },
    { N_("Script|Yi")  , CHR('y','i',' ',' ') },
/*  { N_("Private Use Script 1"), CHR('q','a','a','a') },*/
/*  { N_("Private Use Script 2"), CHR('q','a','a','b') },*/
/*  { N_("Undetermined Script"), CHR('z','y','y','y') },*/
/*  { N_("Uncoded Script"), CHR('z','z','z','z') },*/
    { NULL, 0 }
};

void LookupInit(void) {
    static int done = false;
    int i, j;

    if ( done )
return;
    done = true;
    for ( j=0; j<2; ++j ) {
	for ( i=0; i<10; ++i )
	    if ( lookup_type_names[j][i]!=NULL )
		lookup_type_names[j][i] = S_((char *) lookup_type_names[j][i]);
    }
    for ( i=0; localscripts[i].text!=NULL; ++i )
	localscripts[i].text = S_(localscripts[i].text);
    for ( i=0; friendlies[i].friendlyname!=NULL; ++i )
	friendlies[i].friendlyname = S_(friendlies[i].friendlyname);
}

char *TagFullName(SplineFont *sf,uint32 tag, int ismac, int onlyifknown) {
    char ubuf[200], *end = ubuf+sizeof(ubuf), *setname;
    int k;

    if ( ismac==-1 )
	/* Guess */
	ismac = (tag>>24)<' ' || (tag>>24)>0x7e;

    if ( ismac ) {
	sprintf( ubuf, "<%d,%d> ", (int) (tag>>16),(int) (tag&0xffff) );
	if ( (setname = PickNameFromMacName(FindMacSettingName(sf,tag>>16,tag&0xffff)))!=NULL ) {
	    strcat( ubuf, setname );
	    free( setname );
	}
    } else {
	uint32 stag = tag;
	if ( tag==CHR('n','u','t','f') )	/* early name that was standardize later as... */
	    stag = CHR('a','f','r','c');	/*  Stood for nut fractions. "nut" meaning "fits in an en" in old typography-speak => vertical fractions rather than diagonal ones */
	if ( tag==REQUIRED_FEATURE ) {
	    strcpy(ubuf,_("Required Feature"));
	} else {
	    LookupInit();
	    for ( k=0; friendlies[k].tag!=0; ++k ) {
		if ( friendlies[k].tag == stag )
	    break;
	    }
	    ubuf[0] = '\'';
	    ubuf[1] = tag>>24;
	    ubuf[2] = (tag>>16)&0xff;
	    ubuf[3] = (tag>>8)&0xff;
	    ubuf[4] = tag&0xff;
	    ubuf[5] = '\'';
	    ubuf[6] = ' ';
	    if ( friendlies[k].tag!=0 )
		strncpy(ubuf+7, (char *) friendlies[k].friendlyname,end-ubuf-7);
	    else if ( onlyifknown )
return( NULL );
	    else
		ubuf[7]='\0';
	}
    }
return( copy( ubuf ));
}


void NameOTLookup(OTLookup *otl,SplineFont *sf) {
    char *userfriendly = NULL, *script;
    FeatureScriptLangList *fl;
    char *lookuptype;
    const char *format;
    struct lookup_subtable *subtable;
    int k;

    LookupInit();

    if ( otl->lookup_name==NULL ) {
	for ( k=0; k<2; ++k ) {
	    for ( fl=otl->features; fl!=NULL ; fl=fl->next ) {
		/* look first for a feature attached to a default language */
		if ( k==1 || DefaultLangTagInScriptList(fl->scripts,false)!=NULL ) {
		    userfriendly = TagFullName(sf,fl->featuretag, fl->ismac, true);
		    if ( userfriendly!=NULL )
	    break;
		}
	    }
	    if ( userfriendly!=NULL )
	break;
	}
	if ( userfriendly==NULL ) {
	    if ( (otl->lookup_type&0xff)>= 0xf0 )
		lookuptype = _("State Machine");
	    else if ( (otl->lookup_type>>8)<2 && (otl->lookup_type&0xff)<10 )
		lookuptype = _(lookup_type_names[otl->lookup_type>>8][otl->lookup_type&0xff]);
	    else
		lookuptype = S_("LookupType|Unknown");
	    for ( fl=otl->features; fl!=NULL && !fl->ismac; fl=fl->next );
	    if ( fl==NULL )
		userfriendly = copy(lookuptype);
	    else {
		userfriendly = malloc( strlen(lookuptype) + 10);
		sprintf( userfriendly, "%s '%c%c%c%c'", lookuptype,
		    fl->featuretag>>24,
		    fl->featuretag>>16,
		    fl->featuretag>>8 ,
		    fl->featuretag );
	    }
	}
	script = NULL;
	if ( fl==NULL ) fl = otl->features;
	if ( fl!=NULL && fl->scripts!=NULL ) {
	    char buf[8];
	    int j;
	    struct scriptlanglist *sl, *found, *found2;
	    uint32 script_tag = fl->scripts->script;
	    found = found2 = NULL;
	    for ( sl = fl->scripts; sl!=NULL; sl=sl->next ) {
		if ( sl->script == DEFAULT_SCRIPT )
		    /* Ignore it */;
		else if ( DefaultLangTagInOneScriptList(sl)) {
		    if ( found==NULL )
			found = sl;
		    else {
			found = found2 = NULL;
	    break;
		    }
		} else if ( found2 == NULL )
		    found2 = sl;
		else
		    found2 = (struct scriptlanglist *) -1;
	    }
	    if ( found==NULL && found2!=NULL && found2 != (struct scriptlanglist *) -1 )
		found = found2;
	    if ( found!=NULL ) {
		script_tag = found->script;
		for ( j=0; localscripts[j].text!=NULL && script_tag!=localscripts[j].tag; ++j );
		if ( localscripts[j].text!=NULL )
		    script = copy( S_((char *) localscripts[j].text) );
		else {
		    buf[0] = '\'';
		    buf[1] = fl->scripts->script>>24;
		    buf[2] = (fl->scripts->script>>16)&0xff;
		    buf[3] = (fl->scripts->script>>8)&0xff;
		    buf[4] = fl->scripts->script&0xff;
		    buf[5] = '\'';
		    buf[6] = 0;
		    script = copy(buf);
		}
	    }
	}
	if ( script!=NULL ) {
/* GT: This string is used to generate a name for each OpenType lookup. */
/* GT: The %s will be filled with the user friendly name of the feature used to invoke the lookup */
/* GT: The second %s (if present) is the script */
/* GT: While the %d is the index into the lookup list and is used to disambiguate it */
/* GT: In case that is needed */
	    format = _("%s in %s lookup %d");
	    otl->lookup_name = malloc( strlen(userfriendly)+strlen(format)+strlen(script)+10 );
	    sprintf( otl->lookup_name, format, userfriendly, script, otl->lookup_index );
	} else {
	    format = _("%s lookup %d");
	    otl->lookup_name = malloc( strlen(userfriendly)+strlen(format)+10 );
	    sprintf( otl->lookup_name, format, userfriendly, otl->lookup_index );
	}
	free(script);
	free(userfriendly);
    }

    if ( otl->subtables==NULL )
	/* IError( _("Lookup with no subtables"))*/;
    else {
	int cnt = 0;
	for ( subtable = otl->subtables; subtable!=NULL; subtable=subtable->next, ++cnt )
		if ( subtable->subtable_name==NULL ) {
	    if ( subtable==otl->subtables && subtable->next==NULL )
/* GT: This string is used to generate a name for an OpenType lookup subtable. */
/* GT:  %s is the lookup name */
		format = _("%s subtable");
	    else if ( subtable->per_glyph_pst_or_kern )
/* GT: This string is used to generate a name for an OpenType lookup subtable. */
/* GT:  %s is the lookup name, %d is the index of the subtable in the lookup */
		format = _("%s per glyph data %d");
	    else if ( subtable->kc!=NULL )
		format = _("%s kerning class %d");
	    else if ( subtable->fpst!=NULL )
		format = _("%s contextual %d");
	    else if ( subtable->anchor_classes )
		format = _("%s anchor %d");
	    else {
		IError("Subtable status not filled in for %dth subtable of %s", cnt, otl->lookup_name );
		format = "%s !!!!!!!! %d";
	    }
	    subtable->subtable_name = malloc( strlen(otl->lookup_name)+strlen(format)+10 );
	    sprintf( subtable->subtable_name, format, otl->lookup_name, cnt );
	}
    }
    if ( otl->lookup_type==gsub_ligature ) {
	for ( fl=otl->features; fl!=NULL; fl=fl->next )
	    if ( fl->featuretag==CHR('l','i','g','a') || fl->featuretag==CHR('r','l','i','g'))
		otl->store_in_afm = true;
    }

    if ( otl->lookup_type==gsub_single )
	for ( subtable = otl->subtables; subtable!=NULL; subtable=subtable->next )
	    subtable->suffix = SuffixFromTags(otl->features);
}

static void LangOrder(struct scriptlanglist *sl) {
    int i,j;
    uint32 lang, lang2;

    for ( i=0; i<sl->lang_cnt; ++i ) {
	lang = i<MAX_LANG ? sl->langs[i] : sl->morelangs[i-MAX_LANG];
	for ( j=i+1; j<sl->lang_cnt; ++j ) {
	    lang2 = j<MAX_LANG ? sl->langs[j] : sl->morelangs[j-MAX_LANG];
	    if ( lang>lang2 ) {
		if ( i<MAX_LANG )
		    sl->langs[i] = lang2;
		else
		    sl->morelangs[i-MAX_LANG] = lang2;
		if ( j<MAX_LANG )
		    sl->langs[j] = lang;
		else
		    sl->morelangs[j-MAX_LANG] = lang;
		lang = lang2;
	    }
	}
    }
}

static struct scriptlanglist *SLOrder(struct scriptlanglist *sl) {
    int i,j, cnt;
    struct scriptlanglist *sl2, *space[30], **allocked=NULL, **test = space;

    for ( sl2=sl, cnt=0; sl2!=NULL; sl2=sl2->next, ++cnt )
	LangOrder(sl2);
    if ( cnt<=1 )
return( sl );
    if ( cnt>30 )
	test = allocked = malloc(cnt*sizeof(struct scriptlanglist *));
    for ( sl2=sl, cnt=0; sl2!=NULL; sl2=sl2->next, ++cnt )
	test[cnt] = sl2;
    for ( i=0; i<cnt; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( test[i]->script > test[j]->script ) {
	    struct scriptlanglist *temp;
	    temp = test[i];
	    test[i] = test[j];
	    test[j] = temp;
	}
    }
    sl = test[0];
    for ( i=1; i<cnt; ++i )
	test[i-1]->next = test[i];
    test[i-1]->next = NULL;
    free( allocked );
return( sl );
}

FeatureScriptLangList *FLOrder(FeatureScriptLangList *fl) {
    int i,j, cnt;
    FeatureScriptLangList *fl2, *space[30], **allocked=NULL, **test = space;

    for ( fl2=fl, cnt=0; fl2!=NULL; fl2=fl2->next, ++cnt )
	fl2->scripts = SLOrder(fl2->scripts);
    if ( cnt<=1 )
return( fl );
    if ( cnt>30 )
	test = allocked = malloc(cnt*sizeof(FeatureScriptLangList *));
    for ( fl2=fl, cnt=0; fl2!=NULL; fl2=fl2->next, ++cnt )
	test[cnt] = fl2;
    for ( i=0; i<cnt; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( test[i]->featuretag > test[j]->featuretag ) {
	    FeatureScriptLangList *temp;
	    temp = test[i];
	    test[i] = test[j];
	    test[j] = temp;
	}
    }
    fl = test[0];
    for ( i=1; i<cnt; ++i )
	test[i-1]->next = test[i];
    test[i-1]->next = NULL;
    free( allocked );
return( fl );
}

struct scriptlanglist *SLCopy(struct scriptlanglist *sl) {
    struct scriptlanglist *newsl;

    newsl = chunkalloc(sizeof(struct scriptlanglist));
    *newsl = *sl;
    newsl->next = NULL;

    if ( sl->lang_cnt>MAX_LANG ) {
	newsl->morelangs = malloc((newsl->lang_cnt-MAX_LANG)*sizeof(uint32));
	memcpy(newsl->morelangs,sl->morelangs,(newsl->lang_cnt-MAX_LANG)*sizeof(uint32));
    }
return( newsl );
}

struct scriptlanglist *SListCopy(struct scriptlanglist *sl) {
    struct scriptlanglist *head=NULL, *last=NULL, *cur;

    for ( ; sl!=NULL; sl=sl->next ) {
	cur = SLCopy(sl);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( head );
}

FeatureScriptLangList *FeatureListCopy(FeatureScriptLangList *fl) {
    FeatureScriptLangList *newfl;

    if ( fl==NULL )
return( NULL );

    newfl = chunkalloc(sizeof(FeatureScriptLangList));
    *newfl = *fl;
    newfl->next = NULL;

    newfl->scripts = SListCopy(fl->scripts);
return( newfl );
}

static void LangMerge(struct scriptlanglist *into, struct scriptlanglist *from) {
    int i,j;
    uint32 flang, tlang;

    for ( i=0 ; i<from->lang_cnt; ++i ) {
	flang = i<MAX_LANG ? from->langs[i] : from->morelangs[i-MAX_LANG];
	for ( j=0; j<into->lang_cnt; ++j ) {
	    tlang = j<MAX_LANG ? into->langs[j] : into->morelangs[j-MAX_LANG];
	    if ( tlang==flang )
	break;
	}
	if ( j==into->lang_cnt ) {
	    if ( into->lang_cnt<MAX_LANG )
		into->langs[into->lang_cnt++] = flang;
	    else {
		into->morelangs = realloc(into->morelangs,(into->lang_cnt+1-MAX_LANG)*sizeof(uint32));
		into->morelangs[into->lang_cnt++-MAX_LANG] = flang;
	    }
	}
    }
}

void SLMerge(FeatureScriptLangList *into, struct scriptlanglist *fsl) {
    struct scriptlanglist *isl;

    for ( ; fsl!=NULL; fsl = fsl->next ) {
	for ( isl=into->scripts; isl!=NULL; isl=isl->next ) {
	    if ( fsl->script==isl->script )
	break;
	}
	if ( isl!=NULL )
	    LangMerge(isl,fsl);
	else {
	    isl = SLCopy(fsl);
	    isl->next = into->scripts;
	    into->scripts = isl;
	}
    }
}

void FLMerge(OTLookup *into, OTLookup *from) {
    /* Merge the feature list from "from" into "into" */
    FeatureScriptLangList *ifl, *ffl;

    /* first check for common featuretags and merge the scripts of each */
    for ( ffl = from->features; ffl!=NULL; ffl = ffl->next ) {
	for ( ifl=into->features; ifl!=NULL; ifl=ifl->next ) {
	    if ( ffl->featuretag==ifl->featuretag )
	break;
	}
	if ( ifl!=NULL )
	    SLMerge(ffl,ifl->scripts);
	else {
	    ifl = FeatureListCopy(ffl);
	    ifl->next = into->features;
	    into->features = ifl;
	}
    }
    into->features = FLOrder(into->features);
}

void SFSubTablesMerge(SplineFont *_sf,struct lookup_subtable *subfirst,
	struct lookup_subtable *subsecond) {
    uint16 lookup_type = subfirst->lookup->lookup_type;
    int gid,k,isv;
    SplineChar *sc;
    SplineFont *sf = _sf;
    PST *pst, *fpst, *spst, *pstprev, *pstnext;
    KernPair *fkp, *skp, *kpprev, *kpnext;
    AnchorClass *ac;

    if ( lookup_type != subsecond->lookup->lookup_type ) {
	IError("Attempt to merge lookup subtables with mismatch types");
return;
    }
    if ( lookup_type != gsub_single &&
	    lookup_type != gsub_multiple &&
	    lookup_type != gsub_alternate &&
	    lookup_type != gsub_ligature &&
	    lookup_type != gpos_single &&
	    lookup_type != gpos_pair &&
	    lookup_type != gpos_cursive &&
	    lookup_type != gpos_mark2base &&
	    lookup_type != gpos_mark2ligature &&
	    lookup_type != gpos_mark2mark ) {
	IError("Attempt to merge lookup subtables with bad types");
return;
    } else if ( subfirst->kc!=NULL || subsecond->kc != NULL ) {
	IError("Attempt to merge lookup subtables with kerning classes");
return;
    }

    if ( lookup_type==gpos_cursive || lookup_type==gpos_mark2base ||
	    lookup_type==gpos_mark2ligature || lookup_type==gpos_mark2mark ) {
	for ( ac = sf->anchor; ac!=NULL ; ac=ac->next )
	    if ( ac->subtable == subsecond )
		ac->subtable = subfirst;
    } else {
	k=0;
	do {
	    sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
		if ( lookup_type==gsub_single || lookup_type==gsub_multiple ||
			lookup_type==gsub_alternate || lookup_type==gpos_single ) {
		    fpst = spst = NULL;
		    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
			if ( pst->subtable == subfirst ) {
			    fpst = pst;
			    if ( spst!=NULL )
		    break;
			} else if ( pst->subtable == subsecond ) {
			    spst = pst;
			    if ( fpst!=NULL )
		    break;
			}
		    }
		    if ( fpst==NULL && spst!=NULL )
			spst->subtable = subfirst;
		    else if ( spst!=NULL ) {
			LogError(_("The glyph, %s, contains a %s from %s and one from %s.\nThe one from %s will be removed.\n"),
				sc->name,
			        lookup_type==gpos_single ? _("positioning") : _("substitution"),
			        subfirst->subtable_name, subsecond->subtable_name,
			        subsecond->subtable_name );
			pstprev = NULL;
			for ( pst=sc->possub; pst!=NULL && pst!=spst; pstprev=pst, pst=pst->next );
			if ( pstprev==NULL )
			    sc->possub = spst->next;
			else
			    pstprev = spst->next;
			spst->next = NULL;
			PSTFree(spst);
		    }
		} else if ( lookup_type==gsub_ligature || lookup_type==gpos_pair ) {
		    pstprev = NULL;
		    for ( spst=sc->possub; spst!=NULL ; spst = pstnext ) {
			pstnext = spst->next;
			if ( spst->subtable == subsecond ) {
			    for ( fpst=sc->possub; fpst!=NULL; fpst=fpst->next ) {
				if ( fpst->subtable == subfirst &&
					strcmp(fpst->u.lig.components,spst->u.lig.components)==0 )
			    break;
			    }
			    if ( fpst==NULL )
				spst->subtable = subfirst;
			    else {
				LogError(_("The glyph, %s, contains the same %s from %s and from %s.\nThe one from %s will be removed.\n"),
					sc->name,
			                lookup_type==gsub_ligature ? _("ligature") : _("kern pair"),
					subfirst->subtable_name, subsecond->subtable_name,
					subsecond->subtable_name );
				if ( pstprev==NULL )
				    sc->possub = pstnext;
				else
				    pstprev->next = pstnext;
				spst->next = NULL;
			        PSTFree(spst);
			        spst = pstprev;
			    }
			}
			pstprev = spst;
		    }
		    for ( isv=0; isv<2; ++isv ) {
			kpprev = NULL;
			for ( skp=isv ? sc->vkerns : sc->kerns; skp!=NULL ; skp = kpnext ) {
			    kpnext = skp->next;
			    if ( skp->subtable == subsecond ) {
				for ( fkp=isv ? sc->vkerns : sc->kerns; fkp!=NULL; fkp=fkp->next ) {
				    if ( fkp->subtable == subfirst && fkp->sc==skp->sc )
				break;
				}
				if ( fkp==NULL )
				    skp->subtable = subfirst;
				else {
				    LogError(_("The glyph, %s, contains the same kern pair from %s and from %s.\nThe one from %s will be removed.\n"),
					    sc->name,
					    subfirst->subtable_name, subsecond->subtable_name,
					    subsecond->subtable_name );
				    if ( kpprev!=NULL )
					kpprev->next = kpnext;
				    else if ( isv )
					sc->vkerns = kpnext;
				    else
					sc->kerns = kpnext;
				    skp->next = NULL;
				    KernPairsFree(skp);
				    skp = kpprev;
				}
			    }
			    kpprev = skp;
			}
		    }
		}
	    }
	    ++k;
	} while ( k<_sf->subfontcnt );
    }
}

/* ************************************************************************** */
/* ******************************* copy lookup ****************************** */
/* ************************************************************************** */

static char **ClassCopy(int class_cnt,char **classes) {
    char **newclasses;
    int i;

    if ( classes==NULL || class_cnt==0 )
return( NULL );
    newclasses = malloc(class_cnt*sizeof(char *));
    for ( i=0; i<class_cnt; ++i )
	newclasses[i] = copy(classes[i]);
return( newclasses );
}

static OTLookup *_OTLookupCopyInto(struct sfmergecontext *mc,
	OTLookup *from_otl, OTLookup *before, int do_contents);
static OTLookup *OTLookupCopyNested(struct sfmergecontext *mc,
	OTLookup *from_otl) {
    char *newname;
    OTLookup *to_nested_otl;
    int l;

    if ( from_otl==NULL )
return( NULL );

    for ( l=0; l<mc->lcnt; ++l ) {
	if ( mc->lks[l].from == from_otl )
return( mc->lks[l].to );
    }

    newname = strconcat(mc->prefix,from_otl->lookup_name);
    to_nested_otl = SFFindLookup(mc->sf_to,newname);
    free(newname);
    if ( to_nested_otl==NULL )
	to_nested_otl = _OTLookupCopyInto(mc, from_otl, (OTLookup *) -1, true );
return( to_nested_otl );
}

static KernClass *SF_AddKernClass(struct sfmergecontext *mc,KernClass *kc,
	struct lookup_subtable *sub ) {
    KernClass *newkc;

    newkc = chunkalloc(sizeof(KernClass));
    *newkc = *kc;
    newkc->subtable = sub;
    if ( sub->vertical_kerning ) {
	newkc->next = mc->sf_to->vkerns;
	mc->sf_to->vkerns = newkc;
    } else {
	newkc->next = mc->sf_to->kerns;
	mc->sf_to->kerns = newkc;
    }

    newkc->firsts = ClassCopy(newkc->first_cnt,newkc->firsts);
    newkc->seconds = ClassCopy(newkc->second_cnt,newkc->seconds);
    newkc->offsets = malloc(newkc->first_cnt*newkc->second_cnt*sizeof(int16));
    memcpy(newkc->offsets,kc->offsets,newkc->first_cnt*newkc->second_cnt*sizeof(int16));
    // We support group kerning as well.
    if (newkc->firsts_names) newkc->firsts_names = ClassCopy(newkc->first_cnt,newkc->firsts_names);
    if (newkc->seconds_names) newkc->seconds_names = ClassCopy(newkc->second_cnt,newkc->seconds_names);
    if (newkc->firsts_flags) {
      newkc->firsts_flags = malloc(newkc->first_cnt*sizeof(int));
      memcpy(newkc->firsts_flags,kc->firsts_flags,newkc->first_cnt*sizeof(int));
    }
    if (newkc->seconds_flags) {
      newkc->seconds_flags = malloc(newkc->second_cnt*sizeof(int));
      memcpy(newkc->seconds_flags,kc->seconds_flags,newkc->second_cnt*sizeof(int));
    }
    if (newkc->offsets_flags) {
      newkc->offsets_flags = malloc(newkc->first_cnt*newkc->second_cnt*sizeof(int));
      memcpy(newkc->offsets_flags,kc->offsets_flags,newkc->first_cnt*newkc->second_cnt*sizeof(int));
    }
return( newkc );
}

static FPST *SF_AddFPST(struct sfmergecontext *mc,FPST *fpst,
	struct lookup_subtable *sub ) {
    FPST *newfpst;
    int i, k;

    newfpst = chunkalloc(sizeof(FPST));
    *newfpst = *fpst;
    newfpst->subtable = sub;
    newfpst->next = mc->sf_to->possub;
    mc->sf_to->possub = newfpst;

    newfpst->nclass = ClassCopy(newfpst->nccnt,newfpst->nclass);
    newfpst->bclass = ClassCopy(newfpst->bccnt,newfpst->bclass);
    newfpst->fclass = ClassCopy(newfpst->fccnt,newfpst->fclass);

    newfpst->nclassnames = ClassCopy(newfpst->nccnt,newfpst->nclassnames);
    newfpst->bclassnames = ClassCopy(newfpst->bccnt,newfpst->bclassnames);
    newfpst->fclassnames = ClassCopy(newfpst->fccnt,newfpst->fclassnames);

    newfpst->rules = malloc(newfpst->rule_cnt*sizeof(struct fpst_rule));
    memcpy(newfpst->rules,fpst->rules,newfpst->rule_cnt*sizeof(struct fpst_rule));

    for ( i=0; i<newfpst->rule_cnt; ++i ) {
	struct fpst_rule *r = &newfpst->rules[i], *oldr = &fpst->rules[i];

	r->lookups = malloc(r->lookup_cnt*sizeof(struct seqlookup));
	memcpy(r->lookups,oldr->lookups,r->lookup_cnt*sizeof(struct seqlookup));
	for ( k=0; k<r->lookup_cnt; ++k ) {
	    r->lookups[k].lookup = OTLookupCopyNested(mc,
		r->lookups[k].lookup);
	}

	switch ( newfpst->format ) {
	  case pst_glyphs:
	    r->u.glyph.names = copy( r->u.glyph.names );
	    r->u.glyph.back = copy( r->u.glyph.back );
	    r->u.glyph.fore = copy( r->u.glyph.fore );
	  break;
	  case pst_class:
	    r->u.class.nclasses = malloc( r->u.class.ncnt*sizeof(uint16));
	    memcpy(r->u.class.nclasses,oldr->u.class.nclasses, r->u.class.ncnt*sizeof(uint16));
	    r->u.class.bclasses = malloc( r->u.class.bcnt*sizeof(uint16));
	    memcpy(r->u.class.bclasses,oldr->u.class.bclasses, r->u.class.bcnt*sizeof(uint16));
	    r->u.class.fclasses = malloc( r->u.class.fcnt*sizeof(uint16));
	    memcpy(r->u.class.fclasses,oldr->u.class.fclasses, r->u.class.fcnt*sizeof(uint16));
	  break;
	  case pst_coverage:
	    r->u.coverage.ncovers = ClassCopy( r->u.coverage.ncnt, r->u.coverage.ncovers );
	    r->u.coverage.bcovers = ClassCopy( r->u.coverage.bcnt, r->u.coverage.bcovers );
	    r->u.coverage.fcovers = ClassCopy( r->u.coverage.fcnt, r->u.coverage.fcovers );
	  break;
	  case pst_reversecoverage:
	    r->u.rcoverage.ncovers = ClassCopy( r->u.rcoverage.always1, r->u.rcoverage.ncovers );
	    r->u.rcoverage.bcovers = ClassCopy( r->u.rcoverage.bcnt, r->u.rcoverage.bcovers );
	    r->u.rcoverage.fcovers = ClassCopy( r->u.rcoverage.fcnt, r->u.rcoverage.fcovers );
	    r->u.rcoverage.replacements = copy( r->u.rcoverage.replacements );
	  break;
	  default:
	  break;
	}
    }
return( newfpst );
}

static ASM *SF_AddASM(struct sfmergecontext *mc,ASM *sm, struct lookup_subtable *sub ) {
    ASM *newsm;
    int i;

    newsm = chunkalloc(sizeof(ASM));
    *newsm = *sm;
    newsm->subtable = sub;
    newsm->next = mc->sf_to->sm;
    mc->sf_to->sm = newsm;
    mc->sf_to->changed = true;
    newsm->classes = ClassCopy(newsm->class_cnt, newsm->classes);
    newsm->state = malloc(newsm->class_cnt*newsm->state_cnt*sizeof(struct asm_state));
    memcpy(newsm->state,sm->state,
	    newsm->class_cnt*newsm->state_cnt*sizeof(struct asm_state));
    if ( newsm->type == asm_kern ) {
	for ( i=newsm->class_cnt*newsm->state_cnt-1; i>=0; --i ) {
	    newsm->state[i].u.kern.kerns = malloc(newsm->state[i].u.kern.kcnt*sizeof(int16));
	    memcpy(newsm->state[i].u.kern.kerns,sm->state[i].u.kern.kerns,newsm->state[i].u.kern.kcnt*sizeof(int16));
	}
    } else if ( newsm->type == asm_insert ) {
	for ( i=0; i<newsm->class_cnt*newsm->state_cnt; ++i ) {
	    struct asm_state *this = &newsm->state[i];
	    this->u.insert.mark_ins = copy(this->u.insert.mark_ins);
	    this->u.insert.cur_ins = copy(this->u.insert.cur_ins);
	}
    } else if ( newsm->type == asm_context ) {
	for ( i=0; i<newsm->class_cnt*newsm->state_cnt; ++i ) {
	    newsm->state[i].u.context.mark_lookup = OTLookupCopyNested(mc,
		    newsm->state[i].u.context.mark_lookup);
	    newsm->state[i].u.context.cur_lookup = OTLookupCopyNested(mc,
		    newsm->state[i].u.context.cur_lookup);
	}
    }
return( newsm );
}

static SplineChar *SCFindOrMake(SplineFont *into,SplineChar *fromsc) {
    int to_index;

    if ( into->cidmaster==NULL && into->fv!=NULL ) {
	to_index = SFFindSlot(into,into->fv->map,fromsc->unicodeenc,fromsc->name);
	if ( to_index==-1 )
return( NULL );
return( SFMakeChar(into,into->fv->map,to_index));
    }
return( SFGetChar(into,fromsc->unicodeenc,fromsc->name));
}

static void SF_SCAddAP(SplineChar *tosc,AnchorPoint *ap, AnchorClass *newac) {
    AnchorPoint *newap;

    newap = chunkalloc(sizeof(AnchorPoint));
    *newap = *ap;
    newap->anchor = newac;
    newap->next = tosc->anchor;
    tosc->anchor = newap;
}

static void SF_AddAnchorClasses(struct sfmergecontext *mc,
	struct lookup_subtable *from_sub, struct lookup_subtable *sub ) {
    AnchorClass *ac, *nac;
    int k, gid;
    SplineFont *fsf;
    AnchorPoint *ap;
    SplineChar *fsc, *tsc;

    for ( ac=mc->sf_from->anchor; ac!=NULL; ac=ac->next ) if ( ac->subtable==from_sub ) {
	nac = chunkalloc(sizeof(AnchorClass));
	*nac = *ac;
	nac->subtable = sub;
	nac->name = strconcat(mc->prefix,nac->name);
	nac->next = mc->sf_to->anchor;
	mc->sf_to->anchor = nac;

	k=0;
	do {
	    fsf = mc->sf_from->subfontcnt==0 ? mc->sf_from : mc->sf_from->subfonts[k];
	    for ( gid = 0; gid<fsf->glyphcnt; ++gid ) if ( (fsc = fsf->glyphs[gid])!=NULL ) {
		for ( ap=fsc->anchor; ap!=NULL; ap=ap->next ) {
		    if ( ap->anchor==ac ) {
			tsc = SCFindOrMake(mc->sf_to,fsc);
			if ( tsc==NULL )
		break;
			SF_SCAddAP(tsc,ap,nac);
		    }
		}
	    }
	    ++k;
	} while ( k<mc->sf_from->subfontcnt );
    }
}

static int SF_SCAddPST(SplineChar *tosc,PST *pst,struct lookup_subtable *sub) {
    PST *newpst;

    newpst = chunkalloc(sizeof(PST));
    *newpst = *pst;
    newpst->subtable = sub;
    newpst->next = tosc->possub;
    tosc->possub = newpst;

    switch( newpst->type ) {
      case pst_pair:
	newpst->u.pair.paired = copy(pst->u.pair.paired);
	newpst->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
	memcpy(newpst->u.pair.vr,pst->u.pair.vr,sizeof(struct vr [2]));
      break;
      case pst_ligature:
	newpst->u.lig.lig = tosc;
	/* Fall through */
      case pst_substitution:
      case pst_alternate:
      case pst_multiple:
	newpst->u.subs.variant = copy(pst->u.subs.variant);
      break;
      default:
      break;
    }
return( true );
}

static int SF_SCAddKP(SplineChar *tosc,KernPair *kp,struct lookup_subtable *sub,
	int isvkern, SplineFont *to_sf ) {
    SplineChar *tosecond;
    KernPair *newkp;

    tosecond = SFGetChar(to_sf,kp->sc->unicodeenc,kp->sc->name);
    if ( tosecond==NULL )
return( false );

    newkp = chunkalloc(sizeof(KernPair));
    *newkp = *kp;
    newkp->subtable = sub;
    newkp->sc = tosecond;
    if ( isvkern ) {
	newkp->next = tosc->vkerns;
	tosc->vkerns = newkp;
    } else {
	newkp->next = tosc->kerns;
	tosc->kerns = newkp;
    }
return(true);
}

static void SF_AddPSTKern(struct sfmergecontext *mc,struct lookup_subtable *from_sub, struct lookup_subtable *sub) {
    int k, gid, isv;
    SplineFont *fsf;
    SplineChar *fsc, *tsc;
    PST *pst;
    KernPair *kp;
    int iskern = sub->lookup->lookup_type==gpos_pair;

    k=0;
    do {
	fsf = mc->sf_from->subfontcnt==0 ? mc->sf_from : mc->sf_from->subfonts[k];
	for ( gid = 0; gid<fsf->glyphcnt; ++gid ) if ( (fsc = fsf->glyphs[gid])!=NULL ) {
	    tsc = (SplineChar *) -1;
	    for ( pst = fsc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable==from_sub ) {
		    if ( tsc==(SplineChar *) -1 ) {
			tsc = SCFindOrMake(mc->sf_to,fsc);
			if ( tsc==NULL )
	    break;
		    }
		    SF_SCAddPST(tsc,pst,sub);
		}
	    }
	    if ( tsc!=NULL && iskern ) {
		for ( isv=0; isv<2 && tsc!=NULL; ++isv ) {
		    for ( kp= isv ? fsc->vkerns : fsc->kerns; kp!=NULL; kp=kp->next ) {
			if ( kp->subtable==sub ) {
			    /* Kerning data tend to be individualistic. Only copy if */
			    /*  glyphs exist */
			    if ( tsc==(SplineChar *) -1 ) {
				tsc = SFGetChar(mc->sf_to,fsc->unicodeenc,fsc->name);
				if ( tsc==NULL )
		    break;
			    }
			    SF_SCAddKP(tsc,kp,sub,isv,mc->sf_to);
			}
		    }
		}
	    }
	}
	++k;
    } while ( k<mc->sf_from->subfontcnt );
}

int _FeatureOrderId( int isgpos,uint32 tag ) {
    /* This is the order in which features should be executed */

    if ( !isgpos ) switch ( tag ) {
/* GSUB ordering */
      case CHR('c','c','m','p'):	/* Must be first? */
return( -2 );
      case CHR('l','o','c','l'):	/* Language dependent letter forms (serbian uses some different glyphs than russian) */
return( -1 );
      case CHR('i','s','o','l'):
return( 0 );
      case CHR('j','a','l','t'):		/* must come after 'isol' */
return( 1 );
      case CHR('f','i','n','a'):
return( 2 );
      case CHR('f','i','n','2'):
      case CHR('f','a','l','t'):		/* must come after 'fina' */
return( 3 );
      case CHR('f','i','n','3'):
return( 4 );
      case CHR('m','e','d','i'):
return( 5 );
      case CHR('m','e','d','2'):
return( 6 );
      case CHR('i','n','i','t'):
return( 7 );

      case CHR('r','t','l','a'):
return( 100 );
      case CHR('s','m','c','p'): case CHR('c','2','s','c'):
return( 200 );

      case CHR('r','l','i','g'):
return( 300 );
      case CHR('c','a','l','t'):
return( 301 );
      case CHR('l','i','g','a'):
return( 302 );
      case CHR('d','l','i','g'): case CHR('h','l','i','g'):
return( 303 );
      case CHR('c','s','w','h'):
return( 304 );
      case CHR('m','s','e','t'):
return( 305 );

      case CHR('f','r','a','c'):
return( 306 );

/* Indic processing */
      case CHR('n','u','k','t'):
      case CHR('p','r','e','f'):
return( 301 );
      case CHR('a','k','h','n'):
return( 302 );
      case CHR('r','p','h','f'):
return( 303 );
      case CHR('b','l','w','f'):
return( 304 );
      case CHR('h','a','l','f'):
      case CHR('a','b','v','f'):
return( 305 );
      case CHR('p','s','t','f'):
return( 306 );
      case CHR('v','a','t','u'):
return( 307 );

      case CHR('p','r','e','s'):
return( 310 );
      case CHR('b','l','w','s'):
return( 311 );
      case CHR('a','b','v','s'):
return( 312 );
      case CHR('p','s','t','s'):
return( 313 );
      case CHR('c','l','i','g'):
return( 314 );

      case CHR('h','a','l','n'):
return( 320 );
/* end indic ordering */

      case CHR('a','f','r','c'):
      case CHR('l','j','m','o'):
      case CHR('v','j','m','o'):
return( 350 );
      case CHR('v','r','t','2'): case CHR('v','e','r','t'):
return( 1010 );		/* Documented to come last */

/* Unknown things come after everything but vert/vrt2 */
      default:
return( 1000 );

    } else switch ( tag ) {
/* GPOS ordering */
      case CHR('c','u','r','s'):
return( 0 );
      case CHR('d','i','s','t'):
return( 100 );
      case CHR('b','l','w','m'):
return( 201 );
      case CHR('a','b','v','m'):
return( 202 );
      case CHR('k','e','r','n'):
return( 300 );
      case CHR('m','a','r','k'):
return( 400 );
      case CHR('m','k','m','k'):
return( 500 );
/* Unknown things come after everything  */
      default:
return( 1000 );
    }
}

int FeatureOrderId( int isgpos,FeatureScriptLangList *fl ) {
    int pos = 9999, temp;

    if ( fl==NULL )
return( 0 );

    while ( fl!=NULL ) {
	temp = _FeatureOrderId(isgpos,fl->featuretag );
	if ( temp<pos ) pos = temp;
	fl = fl->next;
    }
return( pos );
}

void SortInsertLookup(SplineFont *sf, OTLookup *newotl) {
    int isgpos = newotl->lookup_type>=gpos_start;
    int pos;
    OTLookup *prev, *otl;

    pos = FeatureOrderId(isgpos,newotl->features);
    for ( prev=NULL, otl= isgpos ? sf->gpos_lookups : sf->gsub_lookups ;
	    otl!=NULL && FeatureOrderId(isgpos,newotl->features)<pos;
	    prev = otl, otl=otl->next );
    newotl->next = otl;
    if ( prev!=NULL )
	prev->next = newotl;
    else if ( isgpos )
	sf->gpos_lookups = newotl;
    else
	sf->gsub_lookups = newotl;
}

/* Before may be:
    * A lookup in into_sf, in which case insert new lookup before it
    * NULL               , in which case insert new lookup at end
    * -1                 , in which case insert new lookup at start
    * -2                 , try to guess a good position
*/
static void OrderNewLookup(SplineFont *into_sf,OTLookup *otl,OTLookup *before) {
    int isgpos = otl->lookup_type>=gpos_start;
    OTLookup **head = isgpos ? &into_sf->gpos_lookups : &into_sf->gsub_lookups;
    OTLookup *prev;

    if ( before == (OTLookup *) -2 )
	SortInsertLookup(into_sf,otl);
    else if ( before == (OTLookup *) -1 || *head==NULL || *head==before ) {
	otl->next = *head;
	*head = otl;
    } else {
	for ( prev= *head; prev->next!=NULL && prev->next!=before ; prev=prev->next );
	otl->next = prev->next;
	prev->next = otl;
    }
}

static OTLookup *_OTLookupCopyInto(struct sfmergecontext *mc,
	OTLookup *from_otl, OTLookup *before, int do_contents) {
    OTLookup *otl;
    struct lookup_subtable *sub, *last, *from_sub;
    int scnt, l;

    for ( l=0; l<mc->lcnt; ++l ) {
	if ( mc->lks[l].from == from_otl ) {
	    if ( mc->lks[l].old )
return( mc->lks[l].to );
	    else
    break;
	}
    }

    if ( l>=mc->lmax )
	mc->lks = realloc(mc->lks,(mc->lmax += 20)*sizeof(struct lookup_cvt));
    mc->sf_to->changed = true;

    if ( l>=mc->lcnt ) {
	otl = chunkalloc(sizeof(OTLookup));
	*otl = *from_otl;
	memset(&mc->lks[l],0,sizeof(mc->lks[l]));
	mc->lks[l].from = from_otl; mc->lks[l].to = otl; ++mc->lcnt;
	otl->lookup_name = strconcat(mc->prefix,from_otl->lookup_name);
	otl->features = FeatureListCopy(from_otl->features);
	otl->next = NULL; otl->subtables = NULL;
	OrderNewLookup(mc->sf_to,otl,before);
	if ( !do_contents )
	    FIOTLookupCopyInto(mc->sf_to,mc->sf_from, from_otl, otl, 0, before);
    } else
	otl = mc->lks[l].to;
    if ( !do_contents )
return( otl );

    last = NULL;
    scnt = 0;
    for ( from_sub = from_otl->subtables; from_sub!=NULL; from_sub=from_sub->next ) {
	sub = chunkalloc(sizeof(struct lookup_subtable));
	*sub = *from_sub;
	sub->lookup = otl;
	sub->subtable_name = strconcat(mc->prefix,from_sub->subtable_name);
	sub->suffix = copy(sub->suffix);
	if ( last==NULL )
	    otl->subtables = sub;
	else
	    last->next = sub;
	last = sub;
	if ( from_sub->kc!=NULL )
	    sub->kc = SF_AddKernClass(mc, from_sub->kc, sub);
	else if ( from_sub->fpst!=NULL )
	    sub->fpst = SF_AddFPST(mc, from_sub->fpst, sub);
	else if ( from_sub->sm!=NULL )
	    sub->sm = SF_AddASM(mc, from_sub->sm, sub);
	else if ( from_sub->anchor_classes )
	    SF_AddAnchorClasses(mc, from_sub, sub);
	else
	    SF_AddPSTKern(mc, from_sub, sub);
	++scnt;
    }
    FIOTLookupCopyInto(mc->sf_to,mc->sf_from, from_otl, otl, scnt, before);
return( otl );
}

static int NeedsPrefix(SplineFont *into_sf,SplineFont *from_sf, OTLookup **list) {
    struct lookup_subtable *from_sub;
    int i,j,k;
    OTLookup *sublist[2];

    sublist[1] = NULL;

    if ( list==NULL || list[0]==NULL )
return( false );
    for ( k=0; list[k]!=NULL; ++k ) {
	OTLookup *from_otl = list[k];
	if ( SFFindLookup(into_sf,from_otl->lookup_name)!=NULL )
return( true );
	for ( from_sub = from_otl->subtables; from_sub!=NULL; from_sub=from_sub->next ) {
	    if ( from_sub->fpst!=NULL ) {
		for ( i=0; i<from_sub->fpst->rule_cnt; ++i ) {
		    struct fpst_rule *r = &from_sub->fpst->rules[i];
		    for ( j=0; j<r->lookup_cnt; ++j ) {
			sublist[0] = r->lookups[j].lookup;
			if ( NeedsPrefix(into_sf,from_sf, sublist))
return( true );
		    }
		}
	    } else if ( from_sub->sm!=NULL && from_sub->sm->type==asm_context ) {
		for ( i=0; i<from_sub->sm->class_cnt*from_sub->sm->state_cnt; ++i ) {
		    sublist[0] = from_sub->sm->state[i].u.context.mark_lookup;
		    if ( NeedsPrefix(into_sf,from_sf,sublist))
return( true );
		    sublist[0] = from_sub->sm->state[i].u.context.cur_lookup;
		    if ( NeedsPrefix(into_sf,from_sf,sublist))
return( true );
		}
	    }
	}
    }
return( false );
}

OTLookup *OTLookupCopyInto(SplineFont *into_sf,SplineFont *from_sf, OTLookup *from_otl) {
    OTLookup *newotl, *list[2];
    struct sfmergecontext mc;

    memset(&mc,0,sizeof(mc));
    mc.sf_from = from_sf; mc.sf_to = into_sf;

    list[0] = from_otl; list[1] = NULL;
    mc.prefix = NeedsPrefix(into_sf,from_sf,list)
	    ? strconcat(from_sf->fontname,"-") : copy("");
    newotl = _OTLookupCopyInto(&mc,from_otl,(OTLookup *) -2,true);
    free(mc.lks);
    free(mc.prefix);
return( newotl );
}

void OTLookupsCopyInto(SplineFont *into_sf,SplineFont *from_sf,
	OTLookup **list, OTLookup *before) {
    int i, do_contents;
    struct sfmergecontext mc;

    memset(&mc,0,sizeof(mc));
    mc.sf_from = from_sf; mc.sf_to = into_sf;

    mc.prefix = NeedsPrefix(into_sf,from_sf,list)
	    ? strconcat(from_sf->fontname,"-") : copy("");
    for ( i=0; list[i]!=NULL; ++i );
    mc.lks = malloc((mc.lmax=i+5)*sizeof(struct lookup_cvt));
    /* First create all the lookups and position them in the right order */
    /*  then create subtables (which may in turn create some new lookups */
    /*  for contextual lookups which invoke other lookups, don't care how */
    /*  those nested lookups are ordered) */
    for ( do_contents=0; do_contents<2; ++do_contents )
	for ( i=0; list[i]!=NULL; ++i )
	    (void) _OTLookupCopyInto(&mc,list[i],before,do_contents);
    free(mc.lks);
    free(mc.prefix);
}

/* ************************************************************************** */
/* ****************************** Apply lookups ***************************** */
/* ************************************************************************** */

struct lookup_data {
    struct opentype_str *str;
    int cnt, max;

    uint32 script;
    SplineFont *sf;

    struct lookup_subtable *lig_owner;
    int lcnt, lmax;
    SplineChar ***ligs;		/* For each ligature we have an array of SplineChars that are its components preceded by the ligature glyph itself */
				/*  NULL terminated */
    int pixelsize;
    double scale;
};

static int ApplyLookupAtPos(uint32 tag, OTLookup *otl,struct lookup_data *data,int pos);

static int GlyphNameInClass(const char *name,const char *class) {
    const char *pt;
    int len = strlen(name);

    for (pt = class; pt && (pt=strstr(pt,name))!=NULL; pt += len) {
	if ( (pt==class || pt[-1]==' ') && (pt[len]=='\0' || pt[len]==' '))
return( true );
    }

return( false );
}

/* ************************************************************************** */
/* ************************ Apply Apple State Machines ********************** */
/* ************************************************************************** */

static void ApplyMacIndicRearrangement(struct lookup_data *data,int verb,
	int first_pos,int last_pos) {
    struct opentype_str temp, temp2, temp3, temp4;
    int i;

    if ( first_pos==-1 || last_pos==-1 || last_pos <= first_pos )
return;
    switch ( verb ) {
      case 1: /* Ax => xA */
	temp = data->str[first_pos];
	for ( i= first_pos; i<last_pos; ++i )
	    data->str[i] = data->str[i+1];
	data->str[last_pos] = temp;
      break;
      case 2: /* xD => Dx */
	temp = data->str[last_pos];
	for ( i= last_pos; i>first_pos; --i )
	    data->str[i] = data->str[i-1];
	data->str[first_pos] = temp;
      break;
      case 3: /* AxD => DxA */
	temp = data->str[last_pos];
	data->str[last_pos] = data->str[first_pos];
	data->str[first_pos] = temp;
      break;
      case 4: /* ABx => xAB */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	for ( i= first_pos; i<last_pos-1; ++i )
	    data->str[i] = data->str[i+21];
	data->str[last_pos-1] = temp;
	data->str[last_pos] = temp2;
      break;
      case 5: /* ABx => xBA */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	for ( i= first_pos; i<last_pos-1; ++i )
	    data->str[i] = data->str[i+21];
	data->str[last_pos-1] = temp2;
	data->str[last_pos] = temp;
      break;
      case 6: /* xCD => CDx */
	temp = data->str[last_pos];
	temp2 = data->str[last_pos-1];
	for ( i= last_pos; i>first_pos+1; --i )
	    data->str[i] = data->str[i-2];
	data->str[first_pos+1] = temp;
	data->str[first_pos] = temp2;
      break;
      case 7: /* xCD => DCx */
	temp = data->str[last_pos];
	temp2 = data->str[last_pos-1];
	for ( i= last_pos; i>first_pos+1; --i )
	    data->str[i] = data->str[i-2];
	data->str[first_pos+1] = temp2;
	data->str[first_pos] = temp;
      break;
      case 8: /* AxCD => CDxA */
	temp = data->str[first_pos];
	temp2 = data->str[last_pos-1];
	temp3 = data->str[last_pos];
	for ( i= last_pos-1; i>first_pos; --i )
	    data->str[i] = data->str[i-1];
	data->str[first_pos+1] = temp2;
	data->str[first_pos] = temp3;
	data->str[last_pos] = temp;
      break;
      case 9: /* AxCD => DCxA */
	temp = data->str[first_pos];
	temp2 = data->str[last_pos-1];
	temp3 = data->str[last_pos];
	for ( i= last_pos-1; i>first_pos; --i )
	    data->str[i] = data->str[i-1];
	data->str[first_pos+1] = temp3;
	data->str[first_pos] = temp2;
	data->str[last_pos] = temp;
      break;
      case 10: /* ABxD => DxAB */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos];
	for ( i= first_pos+1; i<last_pos; ++i )
	    data->str[i] = data->str[i+1];
	data->str[first_pos] = temp3;
	data->str[last_pos-1] = temp;
	data->str[last_pos] = temp2;
      break;
      case 11: /* ABxD => DxBA */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos];
	for ( i= first_pos+1; i<last_pos; ++i )
	    data->str[i] = data->str[i+1];
	data->str[first_pos] = temp3;
	data->str[last_pos-1] = temp2;
	data->str[last_pos] = temp;
      break;
      case 12: /* ABxCD => CDxAB */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos-1];
	temp4 = data->str[last_pos];
	data->str[last_pos] = temp2;
	data->str[last_pos-1] = temp;
	data->str[first_pos+1] = temp4;
	data->str[first_pos] = temp3;
      break;
      case 13: /* ABxCD => CDxBA */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos-1];
	temp4 = data->str[last_pos];
	data->str[last_pos] = temp;
	data->str[last_pos-1] = temp2;
	data->str[first_pos+1] = temp4;
	data->str[first_pos] = temp3;
      break;
      case 14: /* ABxCD => DCxAB */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos-1];
	temp4 = data->str[last_pos];
	data->str[last_pos] = temp2;
	data->str[last_pos-1] = temp;
	data->str[first_pos+1] = temp3;
	data->str[first_pos] = temp4;
      break;
      case 15: /* ABxCD => DCxBA */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos-1];
	temp4 = data->str[last_pos];
	data->str[last_pos] = temp;
	data->str[last_pos-1] = temp2;
	data->str[first_pos+1] = temp3;
	data->str[first_pos] = temp4;
      break;
      default:
      break;
    }
}

static int ApplyMacInsert(struct lookup_data *data,int ipos,int cnt,
	char *glyphnames, int orig_index) {
    SplineChar *inserts[32];
    char *start, *pt;
    int i, ch;

    if ( cnt==0 || glyphnames==NULL || ipos == -1 )
return( 0 );

    for ( i=0, start = glyphnames; i<cnt; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt = start; *pt!=' ' && *pt!='\0'; ++pt );
	ch = *pt; *pt = '\0';
	inserts[i] = SFGetChar(data->sf,-1,start);
	*pt = ch;
	if ( inserts[i]!=NULL )
	    ++i;
    }
    cnt = i;
    if ( i==0 )
return( 0 );
    for ( i= data->cnt; i>=ipos; --i )
	data->str[i+cnt] = data->str[i];
    memset(data->str+ipos,0,cnt*sizeof(struct opentype_str));
    for ( i=0; i<cnt; ++i ) {
	data->str[ipos+i].sc = inserts[i];
	data->str[ipos+i].orig_index = orig_index;
    }
return( cnt );
}

static void ApplyAppleStateMachine(OTLookup *otl,struct lookup_data *data) {
    struct lookup_subtable *sub;
    int state, class, pos, mark_pos, markend_pos, i;
    ASM *sm;
    int cnt_cur, cnt_mark;
    struct asm_state *entry;
    int kern_stack[8], kcnt;		/* Kerning state machines handle at most 8 glyphs */
    /* Flaws: Line processing has not been done yet, so we are never in the */
    /*  start of line state and we never get an end of line token. We never */
    /*  get deleted tokens either, those glyphs are just gone */
    /* Class 0: End of text */
    /* Class 1: Glyph not in any classes */
    /* Class 2: Deleted (we never see) */
    /* Class 3: End of line (we never see) */

    /* Mac doesn't have the concept of subtables, but a user could create one */
    /*  it will get flattened out into its own "lookup" when written to a file*/
    /*  So if there are multiple subtables, just process them all */
    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	sm = sub->sm;

	state = 0;
	mark_pos = markend_pos = -1;
	for ( pos = 0; pos<=data->cnt; ) {
	    if ( pos==data->cnt )
		class = 0;
	    else {
		for ( class = sm->class_cnt-1; class>3; --class )
		    if ( GlyphNameInClass(data->str[i].sc->name,sm->classes[class]) )
		break;
		if ( class==3 )
		    class = 1;		/* Glyph not in any class */;
	    }
	    entry = &sm->state[state*sm->class_cnt+class];
	    switch ( otl->lookup_type ) {
	      case morx_context:
		if ( entry->u.context.cur_lookup!=NULL )
		    ApplyLookupAtPos(0,entry->u.context.cur_lookup,data,pos);
		if ( entry->u.context.mark_lookup!=NULL && mark_pos!=-1 ) {
		    ApplyLookupAtPos(0,entry->u.context.mark_lookup,data,mark_pos);
		    mark_pos = -1;
		}
	      break;
	      case morx_indic:
		if ( entry->flags & 0x2000 )
		    markend_pos = pos;
		if ( (entry->flags&0xf)!=0 ) {
		    ApplyMacIndicRearrangement(data,entry->flags&0xf,mark_pos,markend_pos);
		    mark_pos = markend_pos = -1;
		}
	      break;
	      case morx_insert:
		/* I live in total ignorance of what I should do if the glyph */
		/*  "is Kashida like"... so I ignore those flags. */
		cnt_cur = (entry->flags>>5)&0x1f;
		cnt_mark = (entry->flags&0x1f);
		if ( data->cnt + cnt_cur + cnt_mark >= data->max )
		    data->str = realloc(data->str,(data->max = data->cnt + cnt_cur + cnt_mark +20)*sizeof(struct opentype_str));
		if ( cnt_cur!=0 )
		    cnt_cur = ApplyMacInsert(data,(entry->flags& 0x0800)? pos : pos+1,
			    cnt_cur,entry->u.insert.cur_ins,data->str[pos].orig_index);
		if ( cnt_mark!=0 && mark_pos!=-1 ) {
		    cnt_mark = ApplyMacInsert(data,(entry->flags& 0x0800)? mark_pos : mark_pos+1,
			    cnt_mark,entry->u.insert.mark_ins,data->str[mark_pos].orig_index);
		    mark_pos = -1;
		} else
		    cnt_mark = 0;
		pos += cnt_cur+cnt_mark;
	      break;
	      case kern_statemachine:
		if ( entry->u.kern.kcnt!=0 ) {
		    for ( i=0; i<kcnt && i<entry->u.kern.kcnt; ++i )
			data->str[kern_stack[i]].vr.h_adv_off +=
				entry->u.kern.kerns[i];
		    kcnt = 0;
		}
		if ( entry->flags & 0x8000 ) {
		    for ( i=6; i>=0; --i )
			kern_stack[i+1] = kern_stack[i];
		    kern_stack[0] = pos;
		    if ( ++kcnt>8 ) kcnt = 8;
		}
	      break;
	      default:
	      break;
	    }
	    if ( entry->flags & 0x8000 )
		mark_pos = pos;		/* The docs do not state whether this happens before or after substitutions are applied at the mark */
					/* after is more useful. So assume that */
	    if ( !(entry->flags & 0x4000) )
		++pos;
	    state = entry->next_state;
	}
    }
}

/* ************************************************************************** */
/* ************************* Apply OpenType Lookups ************************* */
/* ************************************************************************** */

static void LigatureFree(struct lookup_data *data) {
    int i;

    if ( data->ligs==NULL )
return;
    for ( i=0; data->ligs[i]!=NULL; ++i )
	free(data->ligs[i]);
}

static void LigatureSearch(struct lookup_subtable *sub, struct lookup_data *data) {
    SplineFont *sf = data->sf;
    int gid, ccnt, cnt, ch, err;
    SplineChar *sc;
    PST *pst;
    char *pt, *start;

    LigatureFree(data);
    cnt = 0;
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->subtable==sub ) {
	    for ( pt = pst->u.lig.components, ccnt=0; *pt; ++pt )
		if ( *pt==' ' )
		    ++ccnt;
	    if ( cnt>=data->lmax )
		data->ligs = realloc(data->ligs,(data->lmax+=100)*sizeof(SplineChar **));
	    data->ligs[cnt] = malloc((ccnt+3)*sizeof(SplineChar *));
	    data->ligs[cnt][0] = sc;
	    ccnt = 1;
	    err = 0;
	    for ( pt = pst->u.lig.components; *pt; ) {
		while ( *pt==' ' ) ++pt;
		if ( *pt=='\0' )
	    break;
		for ( start=pt; *pt!='\0' && *pt!=' '; ++pt );
		ch = *pt; *pt = '\0';
		data->ligs[cnt][ccnt++] = SFGetChar(sf,-1,start);
		*pt = ch;
		if ( data->ligs[cnt][ccnt-1]==NULL )
		    err = 1;
	    }
	    if ( !err )
		data->ligs[cnt++][ccnt] = NULL;
	}
    }
    if ( cnt>=data->lmax )
	data->ligs = realloc(data->ligs,(data->lmax+=1)*sizeof(SplineChar **));
    data->ligs[cnt] = NULL;
    data->lcnt = cnt;
}

static int skipglyphs(int lookup_flags, struct lookup_data *data, int pos) {
    int mc, glyph_class, ms;
    /* The lookup flags tell us what glyphs to ignore. Skip over anything we */
    /*  should ignore */

    if ( lookup_flags==0 )
return( pos );
    mc = (lookup_flags>>8);
    if ( mc<0 || mc>=data->sf->mark_class_cnt )
	mc = 0;
    ms = lookup_flags>>16;
    if ( !(lookup_flags&pst_usemarkfilteringset) || ms>=data->sf->mark_set_cnt )
	ms = -1;
    while ( pos<data->cnt ) {
	glyph_class = gdefclass(data->str[pos].sc);
	/* 1=>base, 2=>ligature, 3=>mark, 4=>component?, 0=>.notdef */
	if ( (glyph_class==1 && (lookup_flags&pst_ignorebaseglyphs)) ||
		(glyph_class==2 && (lookup_flags&pst_ignoreligatures)) ||
		(glyph_class==3 && (lookup_flags&pst_ignorecombiningmarks)) ||
		(glyph_class==3 && mc!=0 &&
			!GlyphNameInClass(data->str[pos].sc->name,data->sf->mark_classes[mc])) ||
		(ms>=0 &&
			!GlyphNameInClass(data->str[pos].sc->name,data->sf->mark_sets[ms])) ) {
	    ++pos;
	} else
    break;
    }
return( pos );
}

static int bskipmarkglyphs(int lookup_flags, struct lookup_data *data, int pos) {
    int mc, glyph_class, ms;
    /* The lookup flags tell us what glyphs to ignore. Skip over anything we */
    /*  should ignore. Here we skip backward */

    mc = (lookup_flags>>8);
    if ( mc<0 || mc>=data->sf->mark_class_cnt )
	mc = 0;
    ms = lookup_flags>>16;
    if ( !(lookup_flags&pst_usemarkfilteringset) || ms>=data->sf->mark_set_cnt )
	ms = -1;
    while ( pos>=0 ) {
	glyph_class = gdefclass(data->str[pos].sc);
	/* 1=>base, 2=>ligature, 3=>mark, 4=>component?, 0=>.notdef */
	if ( glyph_class==3 )
	    --pos;
	else if ( (glyph_class==1 && (lookup_flags&pst_ignorebaseglyphs)) ||
		(glyph_class==2 && (lookup_flags&pst_ignoreligatures)) ||
		(glyph_class==3 && (lookup_flags&pst_ignorecombiningmarks)) ||
		(glyph_class==3 && mc!=0 &&
			!GlyphNameInClass(data->str[pos].sc->name,data->sf->mark_classes[mc])) ||
		(ms>=0 &&
			!GlyphNameInClass(data->str[pos].sc->name,data->sf->mark_sets[ms])) ) {
	    --pos;
	} else
    break;
    }
return( pos );
}

static int bskipglyphs(int lookup_flags, struct lookup_data *data, int pos) {
    int mc, glyph_class, ms;
    /* The lookup flags tell us what glyphs to ignore. Skip over anything we */
    /*  should ignore. Here we skip backward */

    if ( lookup_flags==0 )
return( pos );
    mc = (lookup_flags>>8);
    if ( mc<0 || mc>=data->sf->mark_class_cnt )
	mc = 0;
    ms = lookup_flags>>16;
    if ( !(lookup_flags&pst_usemarkfilteringset) || ms>=data->sf->mark_set_cnt )
	ms = -1;
    while ( pos>=0 ) {
	glyph_class = gdefclass(data->str[pos].sc);
	/* 1=>base, 2=>ligature, 3=>mark, 4=>component?, 0=>.notdef */
	if ( (glyph_class==1 && (lookup_flags&pst_ignorebaseglyphs)) ||
		(glyph_class==2 && (lookup_flags&pst_ignoreligatures)) ||
		(glyph_class==3 && (lookup_flags&pst_ignorecombiningmarks)) ||
		(glyph_class==3 && mc!=0 &&
			!GlyphNameInClass(data->str[pos].sc->name,data->sf->mark_classes[mc])) ||
		(ms>=0 &&
			!GlyphNameInClass(data->str[pos].sc->name,data->sf->mark_sets[ms])) ) {
	    --pos;
	} else
    break;
    }
return( pos );
}

static int ContextualMatch(struct lookup_subtable *sub,struct lookup_data *data,
	int pos, struct fpst_rule **_rule) {
    int i, cpos, retpos, r;
    FPST *fpst = sub->fpst;
    int lookup_flags = sub->lookup->lookup_flags;
    const char *pt;

    /* If we should skip the current glyph then don't try for a match here */
    cpos = skipglyphs(lookup_flags,data,pos);
    if ( cpos!=pos )
return( 0 );

    for ( r=0; r<fpst->rule_cnt; ++r ) {
	struct fpst_rule *rule = &fpst->rules[r];
	for ( i=pos; i<data->cnt; ++i )
	    data->str[i].context_pos = -1;

/* Handle backtrack (backtrace in the rule is stored in reverse textual order) */
	if ( fpst->type == pst_chainpos || fpst->type == pst_chainsub ) {
	    if ( fpst->format==pst_glyphs ) {
		pt = rule->u.glyph.back;
		for ( i=bskipglyphs(lookup_flags,data,pos-1), cpos=0; i>=0; i = bskipglyphs(lookup_flags,data,i-1)) {
		    const char *name = data->str[i].sc->name;
		    int len = strlen( name );
		    if ( strncmp(name,pt,len)!=0 || (pt[len]!='\0' && pt[len]!=' '))
		break;
		    pt += len;
		    while ( *pt==' ' ) ++pt;
		}
		if ( *pt!='\0' )
    continue;		/* didn't match */
	    } else if ( fpst->format==pst_class ) {
		for ( i=bskipglyphs(lookup_flags,data,pos-1), cpos=0; i>=0 && cpos<rule->u.class.bcnt; i = bskipglyphs(lookup_flags,data,i-1)) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,fpst->bclass[rule->u.class.bclasses[cpos]]) )
		break;
		    ++cpos;
		}
		if ( cpos!=rule->u.class.bcnt )
    continue;		/* didn't match */
	    } else if ( fpst->format==pst_coverage ) {
		for ( i=bskipglyphs(lookup_flags,data,pos-1), cpos=0; i>=0 && cpos<rule->u.coverage.bcnt; i = bskipglyphs(lookup_flags,data,i-1)) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,rule->u.coverage.bcovers[cpos]) )
		break;
		    ++cpos;
		}
		if ( cpos<rule->u.coverage.bcnt )
    continue;		/* didn't match */
	    }
	}
/* Handle Match */
	if ( fpst->format==pst_glyphs ) {
	    pt = rule->u.glyph.names;
	    for ( i=pos, cpos=0; i<data->cnt && *pt!='\0'; i = skipglyphs(lookup_flags,data,i+1)) {
		const char *name = data->str[i].sc->name;
		int len = strlen( name );
		if ( strncmp(name,pt,len)!=0 || (pt[len]!='\0' && pt[len]!=' '))
	    break;
		data->str[i].context_pos = cpos++;
		pt += len;
		while ( *pt==' ' ) ++pt;
	    }
	    if ( *pt!='\0' )
    continue;		/* didn't match */
	} else if ( fpst->format==pst_class ) {
	    for ( i=pos, cpos=0; i<data->cnt && cpos<rule->u.class.ncnt; i = skipglyphs(lookup_flags,data,i+1)) {
		int class = rule->u.class.nclasses[cpos];
		if ( class!=0 ) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,fpst->nclass[class]) )
	    break;
		} else {
		    int c;
		    /* Ok, to match class 0 we must fail to match all other classes */
		    for ( c=1; c<fpst->nccnt; ++c )
			if ( !GlyphNameInClass(data->str[i].sc->name,fpst->nclass[c]) )
		    break;
		    if ( c!=fpst->nccnt )
	    break;		/* It matched another class => not in class 0 */
		}
		data->str[i].context_pos = cpos++;
	    }
	    if ( cpos<rule->u.class.ncnt )
    continue;		/* didn't match */
	} else if ( fpst->format==pst_coverage ) {
	    for ( i=pos, cpos=0; i<data->cnt && cpos<rule->u.coverage.ncnt; i = skipglyphs(lookup_flags,data,i+1)) {
		if ( !GlyphNameInClass(data->str[i].sc->name,rule->u.coverage.ncovers[cpos]) )
	    break;
		data->str[i].context_pos = cpos++;
	    }
	    if ( cpos<rule->u.coverage.ncnt )
    continue;		/* didn't match */
	} else
return( 0 );		/* Not ready to deal with reverse chainging */

	retpos = i;
/* Handle lookahead */
	if ( fpst->type == pst_chainpos || fpst->type == pst_chainsub ) {
	    if ( fpst->format==pst_glyphs ) {
		pt = rule->u.glyph.fore;
		for ( i=retpos; i<data->cnt && *pt!='\0'; i = skipglyphs(lookup_flags,data,i+1)) {
		    const char *name = data->str[i].sc->name;
		    int len = strlen( name );
		    if ( strncmp(name,pt,len)!=0 || (pt[len]!='\0' && pt[len]!=' '))
		break;
		    pt += len;
		    while ( *pt==' ' ) ++pt;
		}
		if ( *pt!='\0' )
    continue;		/* didn't match */
	    } else if ( fpst->format==pst_class ) {
		for ( i=retpos, cpos=0; i<data->cnt && cpos<rule->u.class.fcnt; i = skipglyphs(lookup_flags,data,i+1)) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,fpst->fclass[rule->u.class.fclasses[cpos]]) )
		break;
		    cpos++;
		}
		if ( cpos<rule->u.class.fcnt )
    continue;		/* didn't match */
	    } else if ( fpst->format==pst_coverage ) {
		for ( i=retpos, cpos=0; i<data->cnt && cpos<rule->u.coverage.fcnt; i = skipglyphs(lookup_flags,data,i+1)) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,rule->u.coverage.fcovers[cpos]) )
		break;
		    cpos++;
		}
		if ( cpos<rule->u.coverage.fcnt )
    continue;		/* didn't match */
	    }
	}
	*_rule = rule;
return( retpos );
    }
return( 0 );
}

static int ApplySingleSubsAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    PST *pst;
    SplineChar *sc;

    for ( pst=data->str[pos].sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
    if ( pst==NULL )
return( 0 );

    sc = SFGetChar(data->sf,-1,pst->u.subs.variant);
    if ( sc!=NULL ) {
	data->str[pos].sc = sc;
return( pos+1 );
    } else if ( strcmp(pst->u.subs.variant,MAC_DELETED_GLYPH_NAME)==0 ) {
	/* Under AAT we delete the glyph. But OpenType doesn't have that concept */
	int i;
	for ( i=pos+1; i<data->cnt; ++i )
	    data->str[pos-1] = data->str[pos];
	--data->cnt;
return( pos );
    } else {
return( 0 );
    }
}

static int ApplyMultSubsAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    PST *pst;
    SplineChar *sc;
    char *start, *pt;
    int mcnt, ch, i;
    SplineChar *mults[20];

    for ( pst=data->str[pos].sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
    if ( pst==NULL )
return( 0 );

    mcnt = 0;
    for ( start = pst->u.alt.components; *start==' '; ++start);
    for ( ; *start; ) {
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	sc = SFGetChar(data->sf,-1,start);
	*pt = ch;
	if ( sc==NULL )
return( 0 );
	if ( mcnt<20 ) mults[mcnt++] = sc;
	while ( *pt==' ' ) ++pt;
	start = pt;
    }

    if ( mcnt==0 ) {
	/* Is this legal? that is can we remove a glyph with an empty multiple? */
	for ( i=pos+1; i<data->cnt; ++i )
	    data->str[i-1] = data->str[i];
	--data->cnt;
return( pos );
    } else if ( mcnt==1 ) {
	data->str[pos].sc = mults[0];
return( pos+1 );
    } else {
	if ( data->cnt+mcnt-1 >= data->max )
	    data->str = realloc(data->str,(data->max+=mcnt) * sizeof( struct opentype_str ));
	for ( i=data->cnt-1; i>pos; --i )
	    data->str[i+mcnt-1] = data->str[i];
	memset(data->str+pos,0,mcnt*sizeof(struct opentype_str));
	for ( i=0; i<mcnt; ++i ) {
	    data->str[pos+i].sc = mults[i];
	    data->str[pos+i].orig_index = data->str[pos].orig_index;
	}
	data->cnt += (mcnt-1);
return( pos+mcnt );
    }
}

static int ApplyAltSubsAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    PST *pst;
    SplineChar *sc;
    char *start, *pt, ch;

    for ( pst=data->str[pos].sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
    if ( pst==NULL )
return( 0 );

    for ( start = pst->u.alt.components; *start==' '; ++start);
    for ( ; *start; ) {
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	sc = SFGetChar(data->sf,-1,start);
	*pt = ch;
	if ( sc!=NULL ) {
	    data->str[pos].sc = sc;
return( pos+1 );
	}
	while ( *pt==' ' ) ++pt;
	start = pt;
    }
return( 0 );
}

static int ApplyLigatureSubsAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    int i,k, lpos, npos;
    int lookup_flags = sub->lookup->lookup_flags;
    int match_found = -1, match_len=0;

    if ( data->lig_owner!=sub )
	LigatureSearch(sub,data);
    for ( i=0; i<data->lcnt; ++i ) {
	if ( data->ligs[i][1]==data->str[pos].sc ) {
	    lpos = 0;
	    npos = pos+1;
	    for ( k=2; data->ligs[i][k]!=NULL; ++k ) {
		npos = skipglyphs(lookup_flags,data,npos);
		if ( npos>=data->cnt || data->str[npos].sc != data->ligs[i][k] )
	    break;
		++npos;
	    }
	    if ( data->ligs[i][k]==NULL ) {
		if ( match_found==-1 || k>match_len ) {
		    match_found = i;
		    match_len = k;
		}
	    }
	}
    }
    if ( match_found!=-1 ) {
	/* Matched. Remove the component glyphs, and note which component */
	/*  any intervening marks should be attached to */
	data->str[pos].sc = data->ligs[match_found][0];
	npos = pos+1;
	for ( k=2; data->ligs[match_found][k]!=NULL; ++k ) {
	    lpos = skipglyphs(lookup_flags,data,npos);
	    for ( ; npos<lpos; ++npos )
		data->str[npos].lig_pos = k-2;
	    /* Remove this glyph (copy the final NUL too) */
	    for ( ++lpos; lpos<=data->cnt; ++lpos )
		data->str[lpos-1] = data->str[lpos];
	    --data->cnt;
	}
	/* Any marks after the last component (which should be attached */
	/*  to it) will not have been tagged, so do that now */
	lpos = skipglyphs(lookup_flags,data,npos);
	for ( ; npos<lpos; ++npos )
	    data->str[npos].lig_pos = k-2;
return( pos+1 );
    }

return( 0 );
}

static int ApplyContextual(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    /* On this level there is no difference between GPOS/GSUB contextuals */
    /*  If the contextual matches, then we apply the lookups, otherwise we */
    /*  don't. Now the lookups will be different, but we don't care here */
    struct fpst_rule *rule;
    int retpos, i,j;

    retpos = ContextualMatch(sub,data,pos,&rule);
    if ( retpos==0 )
return( 0 );
    for ( i=0; i<rule->lookup_cnt; ++i ) {
	for ( j=pos; j<data->cnt; ++j ) {
	    if ( data->str[j].context_pos == rule->lookups[i].seq ) {
		ApplyLookupAtPos(0,rule->lookups[i].lookup,data,j);
	break;
	    }
	}
    }
return( retpos );
}

static int FigureDeviceTable(DeviceTable *dt,int pixelsize) {

    if ( dt==NULL || dt->corrections==NULL || pixelsize<dt->first_pixel_size ||
	    pixelsize>dt->last_pixel_size )
return( 0 );

return( dt->corrections[pixelsize - dt->last_pixel_size] );
}

static int ApplySinglePosAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    PST *pst;

    for ( pst=data->str[pos].sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
    if ( pst==NULL )
return( 0 );

    data->str[pos].vr.xoff += rint( pst->u.pos.xoff * data->scale );
    data->str[pos].vr.yoff += rint( pst->u.pos.yoff * data->scale );
    data->str[pos].vr.h_adv_off += rint( pst->u.pos.h_adv_off * data->scale );
    data->str[pos].vr.v_adv_off += rint( pst->u.pos.v_adv_off * data->scale );
    if ( pst->u.pos.adjust!=NULL ) {
	data->str[pos].vr.xoff += FigureDeviceTable(&pst->u.pos.adjust->xadjust,data->pixelsize);
	data->str[pos].vr.yoff += FigureDeviceTable(&pst->u.pos.adjust->yadjust,data->pixelsize);
	data->str[pos].vr.h_adv_off += FigureDeviceTable(&pst->u.pos.adjust->xadv,data->pixelsize);
	data->str[pos].vr.v_adv_off += FigureDeviceTable(&pst->u.pos.adjust->yadv,data->pixelsize);
    }
return( pos+1 );
}

static int ApplyPairPosAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos,int allow_class0) {
    PST *pst;
    int npos, isv, within, f, l, kcspecd;
    KernPair *kp;

    npos = skipglyphs(sub->lookup->lookup_flags,data,pos+1);
    if ( npos>=data->cnt )
return( 0 );
    if ( sub->kc!=NULL ) {
	kcspecd = sub->kc->firsts[0] != NULL;
	f = KCFindName(data->str[pos].sc->name ,sub->kc->firsts ,sub->kc->first_cnt ,allow_class0);
	l = KCFindName(data->str[npos].sc->name,sub->kc->seconds,sub->kc->second_cnt,allow_class0);
	if ( f==-1 || l==-1 || ( !kcspecd && f==0 && l==0 ) )
return( 0 );
	data->str[pos].kc_index = within = f*sub->kc->second_cnt+l;
	data->str[pos].kc = sub->kc;
	data->str[pos].prev_kc0 = ( !kcspecd && f==0 );
	data->str[pos].next_kc0 = ( l==0 );
	if ( sub->vertical_kerning ) {
	    data->str[pos].vr.v_adv_off += rint( sub->kc->offsets[within] * data->scale );
	    data->str[pos].vr.v_adv_off += FigureDeviceTable(&sub->kc->adjusts[within],data->pixelsize);
	} else if ( sub->lookup->lookup_flags & pst_r2l ) {
	    data->str[npos].vr.h_adv_off += rint( sub->kc->offsets[within] * data->scale );
	    data->str[npos].vr.h_adv_off += FigureDeviceTable(&sub->kc->adjusts[within],data->pixelsize);
	} else {
	    data->str[pos].vr.h_adv_off += rint( sub->kc->offsets[within] * data->scale );
	    data->str[pos].vr.h_adv_off += FigureDeviceTable(&sub->kc->adjusts[within],data->pixelsize);
	}
	/* Return 1 if the result we have got comes from a combination with an {Everything Else} class */
	/* This is acceptable, but we should continue looking for a better match */
return( pos+1 );
    } else {
	for ( pst=data->str[pos].sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable==sub && strcmp(pst->u.pair.paired,data->str[npos].sc->name)==0 ) {
		data->str[pos].vr.xoff += rint( pst->u.pair.vr[0].xoff * data->scale);
		data->str[pos].vr.yoff += rint( pst->u.pair.vr[0].yoff * data->scale);
		data->str[pos].vr.h_adv_off += rint( pst->u.pair.vr[0].h_adv_off * data->scale);
		data->str[pos].vr.v_adv_off += rint( pst->u.pair.vr[0].v_adv_off * data->scale);
		data->str[npos].vr.xoff += rint( pst->u.pair.vr[1].xoff * data->scale);
		data->str[npos].vr.yoff += rint( pst->u.pair.vr[1].yoff * data->scale);
		data->str[npos].vr.h_adv_off += rint( pst->u.pair.vr[1].h_adv_off * data->scale);
		data->str[npos].vr.v_adv_off += rint( pst->u.pair.vr[1].v_adv_off * data->scale);
		/* I got bored. I should do all of them */
		if ( pst->u.pair.vr[0].adjust!=NULL ) {
		    data->str[pos].vr.h_adv_off += FigureDeviceTable(&pst->u.pair.vr[0].adjust->xadv,data->pixelsize);
		}
return( pos+1 );	/* We do NOT want to return npos+1 */
	    }
	}
	for ( isv = 0; isv<2; ++isv ) {
	    for ( kp = isv ? data->str[pos].sc->vkerns : data->str[pos].sc->kerns; kp!=NULL; kp=kp->next ) {
		if ( kp->subtable == sub && kp->sc == data->str[npos].sc ) {
		    data->str[pos].kp = kp;
		    if ( isv ) {
			data->str[pos].vr.v_adv_off += rint( kp->off * data->scale);
			data->str[pos].vr.v_adv_off += FigureDeviceTable(kp->adjust,data->pixelsize);
		    } else if ( sub->lookup->lookup_flags & pst_r2l ) {
			data->str[npos].vr.h_adv_off += rint( kp->off * data->scale);
			data->str[npos].vr.h_adv_off += FigureDeviceTable(kp->adjust,data->pixelsize);
		    } else {
			data->str[pos].vr.h_adv_off += rint( kp->off * data->scale);
			data->str[pos].vr.h_adv_off += FigureDeviceTable(kp->adjust,data->pixelsize);
		    }
return( pos+1 );
		}
	    }
	}
    }

return( 0 );
}

static int ApplyAnchorPosAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    AnchorPoint *ap1, *ap2;
    int bpos;

    /* Anchors do not position the base glyph, but the mark (or second glyph */
    /*  of a cursive attachment). This means we don't apply the attachment when*/
    /*  we meet the first glyph, but wait until we meet the second, and then */
    /*  walk backwards */
    /* The backwards walk is different depending on the lookup type (I think) */
    /*  mark to base and mark to ligature lookups will skip all marks even if */
    /*  lookup flags don't specify that */
    /* mark to mark, and cursive attachment only skip what the lookup flags */
    /*  tell them to skip. */
    for ( ap2=data->str[pos].sc->anchor; ap2!=NULL ; ap2=ap2->next ) {
	if ( ap2->anchor->subtable==sub && (ap2->type == at_mark || ap2->type == at_centry))
    break;
    }
    if ( ap2==NULL ) {
	/* This subtable is not used by this glyph ... at least this glyph is */
	/*  neither a mark nor an entry point for this subtable */
return( 0 );
    }

    /* There's only going to be one mark anchor on a glyph in a given subtable*/
    /* And cursive attachments only allow one anchor class per subtable */
    /* in either case we have already found the only attachment site possible */
    /*  in the current glyph */

    if ( sub->lookup->lookup_type == gpos_mark2base ||
	    sub->lookup->lookup_type == gpos_mark2ligature )
	bpos = bskipmarkglyphs(sub->lookup->lookup_flags,data,pos-1);
    else
	bpos = bskipglyphs(sub->lookup->lookup_flags,data,pos-1);
    if ( bpos==-1 )
return( 0 );		/* No match */

    if ( sub->lookup->lookup_type == gpos_cursive ) {
	for ( ap1=data->str[bpos].sc->anchor; ap1!=NULL ; ap1=ap1->next ) {
	    if ( ap1->anchor==ap2->anchor && ap1->type==at_cexit )
	break;
	}
    } else if ( sub->lookup->lookup_type == gpos_mark2ligature ) {
	for ( ap1=data->str[bpos].sc->anchor; ap1!=NULL ; ap1=ap1->next ) {
	    if ( ap1->anchor==ap2->anchor && ap1->type==at_baselig &&
		    ap1->lig_index == data->str[pos].lig_pos )
	break;
	}
    } else {
	for ( ap1=data->str[bpos].sc->anchor; ap1!=NULL ; ap1=ap1->next ) {
	    if ( ap1->anchor==ap2->anchor &&
		    (ap1->type==at_basechar || ap1->type==at_basemark) )
	break;
	}
    }
    if ( ap1==NULL )
return( 0 );		/* No match */

/* This probably doesn't work for vertical text */
    data->str[pos].vr.yoff = data->str[bpos].vr.yoff +
	    rint((ap1->me.y - ap2->me.y) * data->scale);
    data->str[pos].vr.yoff += FigureDeviceTable(&ap1->yadjust,data->pixelsize)-
		FigureDeviceTable(&ap2->yadjust,data->pixelsize);
    if ( sub->lookup->lookup_flags&pst_r2l ) {
	data->str[pos].vr.xoff = data->str[bpos].vr.xoff +
		rint( -(ap1->me.x - ap2->me.x)*data->scale );
	data->str[pos].vr.xoff -= FigureDeviceTable(&ap1->xadjust,data->pixelsize)-
		    FigureDeviceTable(&ap2->xadjust,data->pixelsize);
    } else {
	data->str[pos].vr.xoff = data->str[bpos].vr.xoff +
		rint( (ap1->me.x - ap2->me.x - data->str[bpos].sc->width)*data->scale -
		data->str[bpos].vr.h_adv_off);
	data->str[pos].vr.xoff += FigureDeviceTable(&ap1->xadjust,data->pixelsize)-
		    FigureDeviceTable(&ap2->xadjust,data->pixelsize);
    }

return( pos+1 );
}

static int ConditionalTagOk(uint32 tag, OTLookup *otl,struct lookup_data *data,int pos) {
    int npos, bpos;
    uint32 script;
    int before_in_script, after_in_script;

    if ( tag==CHR('i','n','i','t') || tag==CHR('i','s','o','l') ||
	    tag==CHR('f','i','n','a') || tag==CHR('m','e','d','i') ) {
	npos = skipglyphs(otl->lookup_flags,data,pos+1);
	bpos = bskipglyphs(otl->lookup_flags,data,pos-1);
	script = SCScriptFromUnicode(data->str[pos].sc);
	before_in_script = (bpos>=0 && SCScriptFromUnicode(data->str[bpos].sc)==script);
	after_in_script = (npos<data->cnt && SCScriptFromUnicode(data->str[npos].sc)==script);
	if ( tag==CHR('i','n','i','t') )
return( !before_in_script && after_in_script );
	else if ( tag==CHR('i','s','o','l') )
return( !before_in_script && !after_in_script );
	else if ( tag==CHR('f','i','n','a') )
return( before_in_script && !after_in_script );
	else
return( before_in_script && after_in_script );
    }

return( true );
}

static int ApplyLookupAtPos(uint32 tag, OTLookup *otl,struct lookup_data *data,int pos) {
    struct lookup_subtable *sub;
    int newpos;
    /* Need two passes for pair kerning lookups. At the second pass we accept */
    /* also combinations with the {Everything Else} class */
    int i, pcnt = otl->lookup_type==gpos_pair ? 2 : 1;

    /* Some tags imply a conditional check. Do that now */
    if ( !ConditionalTagOk(tag,otl,data,pos))
return( 0 );

    for ( i=0; i<pcnt; i++ ) {
	for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	    switch ( otl->lookup_type ) {
	      case gsub_single:
		newpos = ApplySingleSubsAtPos(sub,data,pos);
	      break;
	      case gsub_multiple:
		newpos = ApplyMultSubsAtPos(sub,data,pos);
	      break;
	      case gsub_alternate:
		newpos = ApplyAltSubsAtPos(sub,data,pos);
	      break;
	      case gsub_ligature:
		newpos = ApplyLigatureSubsAtPos(sub,data,pos);
	      break;
	      case gsub_context:
		newpos = ApplyContextual(sub,data,pos);
	      break;
	      case gsub_contextchain:
		newpos = ApplyContextual(sub,data,pos);
	      break;
	      case gsub_reversecchain:
		newpos = ApplySingleSubsAtPos(sub,data,pos);
	      break;

	      case gpos_single:
		newpos = ApplySinglePosAtPos(sub,data,pos);
	      break;
	      case gpos_pair:
		newpos = ApplyPairPosAtPos(sub,data,pos,i>0);
	      break;
	      case gpos_cursive:
		newpos = ApplyAnchorPosAtPos(sub,data,pos);
	      break;
	      case gpos_mark2base:
		newpos = ApplyAnchorPosAtPos(sub,data,pos);
	      break;
	      case gpos_mark2ligature:
		newpos = ApplyAnchorPosAtPos(sub,data,pos);
	      break;
	      case gpos_mark2mark:
		newpos = ApplyAnchorPosAtPos(sub,data,pos);
	      break;
	      case gpos_context:
		newpos = ApplyContextual(sub,data,pos);
	      break;
	      case gpos_contextchain:
		newpos = ApplyContextual(sub,data,pos);
	      break;
	      default:
		/* apple state machines */
		newpos = 0;
	      break;
	    }
	    /* if a subtable worked, we don't try to apply the next one */
	    if ( newpos!=0 )
    return( newpos );
	}
    }
return( 0 );
}

static void ApplyLookup(uint32 tag, OTLookup *otl,struct lookup_data *data) {
    int pos, npos;
    int lt = otl->lookup_type;

    if ( lt == morx_indic || lt == morx_context || lt == morx_insert ||
	    lt == kern_statemachine )
	ApplyAppleStateMachine(otl,data);
    else {
	/* OpenType */
	for ( pos = 0; pos<data->cnt; ) {
	    npos = ApplyLookupAtPos(tag,otl,data,pos);
	    if ( npos<=pos)		/* !!!!! */
		npos = pos+1;
	    pos = npos;
	}
    }
}

static uint32 FSLLMatches(FeatureScriptLangList *fl,uint32 *flist,uint32 script,uint32 lang) {
    int i,l;
    struct scriptlanglist *sl;

    if ( flist==NULL )
return( 0 );

    while ( fl!=NULL ) {
	for ( i=0; flist[i]!=0; ++i ) {
	    if ( fl->featuretag==flist[i] )
	break;
	}
	if ( flist[i]!=0 ) {
	    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
		if ( sl->script == script ) {
		    if ( fl->ismac )	/* Language irrelevant on macs (scripts too, but we pretend they matter) */
return( fl->featuretag );
		    for ( l=0; l<sl->lang_cnt; ++l )
			if ( (l<MAX_LANG && sl->langs[l]==lang) ||
				(l>=MAX_LANG && sl->morelangs[l-MAX_LANG]==lang))
return( fl->featuretag );
		}
	    }
	}
	fl = fl->next;
    }
return( 0 );
}

/* This routine takes a string of glyphs and applies the opentype transformations */
/*  indicated by the features (and script and language) we are passed, it returns */
/*  a transformed string with substitutions applied and containing positioning */
/*  info */
struct opentype_str *ApplyTickedFeatures(SplineFont *sf,uint32 *flist, uint32 script, uint32 lang,
	int pixelsize, SplineChar **glyphs) {
    int isgpos, cnt;
    OTLookup *otl;
    struct lookup_data data;
    uint32 *langs, templang;
    int i;

    memset(&data,0,sizeof(data));
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt );
    data.str = calloc(cnt+1,sizeof(struct opentype_str));
    data.cnt = data.max = cnt;
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt ) {
	data.str[cnt].sc = glyphs[cnt];
	data.str[cnt].orig_index = cnt;
	data.str[cnt].lig_pos = data.str[cnt].context_pos = -1;
    }
    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    data.sf = sf;
    data.pixelsize = pixelsize;
    data.scale = pixelsize/(double) (sf->ascent+sf->descent);

    /* Indic glyph reordering???? */
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	/* Check that this table has an entry for this language */
	/*  if it doesn't use the default language */
	/* GPOS/GSUB may have different language sets, so we must be prepared */
	templang = lang;
	langs = SFLangsInScript(sf,isgpos,script);
	for ( i=0; langs[i]!=0 && langs[i]!=lang; ++i );
	if ( langs[i]==0 )
	    templang = DEFAULT_LANG;
	free(langs);

	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL ; otl = otl->next ) {
	    uint32 tag;
	    if ( (tag=FSLLMatches(otl->features,flist,script,templang))!=0 )
		ApplyLookup(tag,otl,&data);
	}
    }
    LigatureFree(&data);
    free(data.ligs);

    data.str = realloc(data.str,(data.cnt+1)*sizeof(struct opentype_str));
    memset(&data.str[data.cnt],0,sizeof(struct opentype_str));
return( data.str );
}

static void doreplace(char **haystack,char *start,const char *rpl,int slen) {
    int rlen;
    char *pt = start+slen;

    rlen = strlen(rpl);
    if ( slen>=rlen ) {
	memcpy(start,rpl,rlen);
	if ( slen>rlen ) {
	    int diff = slen-rlen;
	    for ( ; *pt ; ++pt )
		pt[-diff] = *pt;
	    pt[-diff] = '\0';
	}
    } else {
	char *base = *haystack;
	char *new = malloc(pt-base+strlen(pt)+rlen-slen+1);
	memcpy(new,base,start-base);
	memcpy(new+(start-base),rpl,rlen);
	strcpy(new+(start-base)+rlen,pt);
	free( base );
	*haystack = new;
    }
}

static int rplstr(char **haystack,const char *search, const char *rpl,int multipleoccurances) {
    char *start, *pt, *base = *haystack;
    int ch, match, slen = strlen(search);
    int any = 0;

    if ( base==NULL )
return( false );

    for ( pt=base ; ; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
return( any );
	start=pt;
	while ( *pt!=' ' && *pt!='\0' ) ++pt;
	if ( pt-start!=slen )
	    match = -1;
	else {
	    ch = *pt; *pt='\0';
	    match = strcmp(start,search);
	    *pt = ch;
	}
	if ( match==0 ) {
	    doreplace(haystack,start,rpl,slen);
	    if ( !multipleoccurances )
return( true );
	    any = true;
	    if ( base!=*haystack ) {
		pt = *haystack + (start-base)+strlen(rpl);
		base = *haystack;
	    } else
		pt = start+strlen(rpl);
	}
    }
}

static int rplglyphname(char **haystack,const char *search, const char *rpl) {
    /* If we change "f" to "uni0066" then we should also change "f.sc" to */
    /*  "uni0066.sc" and "f_f_l" to "uni0066_uni0066_l" */
    char *start, *pt, *base = *haystack;
    int ch, match;
    size_t slen = strlen(search);
    int any = 0;

    if ( slen>=strlen( base ))
return( false );

    for ( pt=base ; ; ) {
	while ( *pt=='_' ) ++pt;
	if ( *pt=='\0' || *pt=='.' )
return( any );
	start=pt;
	while ( *pt!='_' && *pt!='\0' && *pt!='.' ) ++pt;
	if ( *pt=='\0' && start==base )	/* Don't change any unsegmented names */
return( false );			/* In particular don't rename ourselves*/
	if ( pt-start!=(ptrdiff_t)slen )
	    match = -1;
	else {
	    ch = *pt; *pt='\0';
	    match = strcmp(start,search);
	    *pt = ch;
	}
	if ( match==0 ) {
	    doreplace(haystack,start,rpl,slen);
	    any = true;
	    if ( base!=*haystack ) {
		pt = *haystack + (start-base) + strlen(rpl);
		base = *haystack;
	    } else
		pt = start+strlen(rpl);
	}
    }
}

static int glyphnameIsComponent(const char *haystack, const char *search) {
    /* Check for a glyph name in ligature names and dotted names */
    const char *start, *pt;
    size_t slen = strlen(search);

    if ( slen>=strlen( haystack ))
return( false );

    for ( pt=haystack ; ; ) {
	while ( *pt=='_' ) ++pt;
	if ( *pt=='\0' || *pt=='.' )
return( false );
	start=pt;
	while ( *pt!='_' && *pt!='\0' && *pt!='.' ) ++pt;
	if ( *pt=='\0' && start==haystack )/* Don't change any unsegmented names */
return( false );			/* In particular don't rename ourselves*/
	if ( pt-start==(ptrdiff_t)slen && strncmp(start,search,slen)==0 )
return( true );
    }
}

static int gvfixup(struct glyphvariants *gv,const char *old, const char *new) {
    int i;
    int ret=0;

    if ( gv==NULL )
return( false );
    ret = rplstr(&gv->variants,old,new,false);
    for ( i=0; i<gv->part_cnt; ++i ) {
	if ( strcmp(gv->parts[i].component,old)==0 ) {
	    free( gv->parts[i].component);
	    gv->parts[i].component = copy(new);
	    ret = true;
	}
    }
return( ret );
}

void SFGlyphRenameFixup(SplineFont *sf, const char *old, const char *new, int rename_related_glyphs) {
/* NOTE: Existing GUI behaviour renames glyphs, rename_related_glyphs turns */
/* off this behaviour for scripting - see github issue #523 */
    int k, gid, isv;
    int i,r;
    SplineFont *master = sf;
    SplineChar *sc;
    PST *pst;
    FPST *fpst;
    KernClass *kc;
    ASM *sm;

    CVGlyphRenameFixup(sf,old,new);
    if ( sf->cidmaster!=NULL )
	master = sf->cidmaster;

    /* Look through all substitutions (and pairwise psts) stored on the glyphs*/
    /*  and change any occurances of the name */
    /* (KernPairs have a reference to the SC rather than the name, and need no fixup) */
    /* Also if the name is "f" then look for glyph names like "f.sc" or "f_f_l"*/
    /*  and be ready to change them too */
    k = 0;
    do {
	sf = k<master->subfontcnt ? master->subfonts[k] : master;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	    if ( rename_related_glyphs && glyphnameIsComponent(sc->name,old) ) {
		char *newer = copy(sc->name);
		rplglyphname(&newer,old,new);
		SFGlyphRenameFixup(master,sc->name,newer,true);
		free(sc->name);
		sc->name = newer;
		sc->namechanged = sc->changed = true;
	    }
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type==pst_substitution || pst->type==pst_alternate ||
			pst->type==pst_multiple || pst->type==pst_pair ||
			pst->type==pst_ligature ) {
		    if ( rplstr(&pst->u.mult.components,old,new,pst->type==pst_ligature))
			sc->changed = true;
		}
	    }
	    /* For once I don't want a short circuit eval of "or", so I use */
	    /*  bitwise rather than boolean intentionally */
	    if ( gvfixup(sc->vert_variants,old,new) |
		    gvfixup(sc->horiz_variants,old,new))
		sc->changed = true;
	}
	++k;
    } while ( k<master->subfontcnt );

    /* Now look for contextual fpsts which might use the name */
    for ( fpst=master->possub; fpst!=NULL; fpst=fpst->next ) {
	if ( fpst->format==pst_class ) {
	    for ( i=0; i<fpst->nccnt; ++i ) if ( fpst->nclass[i]!=NULL ) {
		if ( rplstr(&fpst->nclass[i],old,new,false))
	    break;
	    }
	    for ( i=0; i<fpst->bccnt; ++i ) if ( fpst->bclass[i]!=NULL ) {
		if ( rplstr(&fpst->bclass[i],old,new,false))
	    break;
	    }
	    for ( i=0; i<fpst->fccnt; ++i ) if ( fpst->fclass[i]!=NULL ) {
		if ( rplstr(&fpst->fclass[i],old,new,false))
	    break;
	    }
	}
	for ( r=0; r<fpst->rule_cnt; ++r ) {
	    struct fpst_rule *rule = &fpst->rules[r];
	    if ( fpst->format==pst_glyphs ) {
		rplstr(&rule->u.glyph.names,old,new,true);
		rplstr(&rule->u.glyph.back,old,new,true);
		rplstr(&rule->u.glyph.fore,old,new,true);
	    } else if ( fpst->format==pst_coverage ||
		    fpst->format==pst_reversecoverage ) {
		for ( i=0; i<rule->u.coverage.ncnt ; ++i )
		    rplstr(&rule->u.coverage.ncovers[i],old,new,false);
		for ( i=0; i<rule->u.coverage.bcnt ; ++i )
		    rplstr(&rule->u.coverage.bcovers[i],old,new,false);
		for ( i=0; i<rule->u.coverage.fcnt ; ++i )
		    rplstr(&rule->u.coverage.fcovers[i],old,new,false);
		if ( fpst->format==pst_reversecoverage )
		    rplstr(&rule->u.rcoverage.replacements,old,new,true);
	    }
	}
    }

    /* Now look for contextual apple state machines which might use the name */
    for ( sm = master->sm; sm!=NULL; sm=sm->next ) {
	for ( i=0; i<sm->class_cnt; ++i ) if ( sm->classes[i]!=NULL ) {
	    if ( rplstr(&sm->classes[i],old,new,false))
	break;
	}
    }

    /* Now look for contextual kerning classes which might use the name */
    for ( isv=0; isv<2; ++isv ) {
	for ( kc=isv ? master->vkerns : master->kerns; kc!=NULL; kc=kc->next ) {
	    for ( i=0; i<kc->first_cnt; ++i ) if ( kc->firsts[i]!=NULL ) {
		if ( rplstr(&kc->firsts[i],old,new,false))
	    break;
	    }
	    for ( i=0; i<kc->second_cnt; ++i ) if ( kc->seconds[i]!=NULL ) {
		if ( rplstr(&kc->seconds[i],old,new,false))
	    break;
	    }
	}
    }
}

struct lookup_subtable *SFSubTableFindOrMake(SplineFont *sf,uint32 tag,uint32 script,
	int lookup_type ) {
    OTLookup **base;
    OTLookup *otl, *found=NULL;
    int isgpos = lookup_type>=gpos_start;
    struct lookup_subtable *sub;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    base = isgpos ? &sf->gpos_lookups : &sf->gsub_lookups;
    for ( otl= *base; otl!=NULL; otl=otl->next ) {
	if ( otl->lookup_type==lookup_type &&
		FeatureScriptTagInFeatureScriptList(tag,script,otl->features) ) {
	    for ( sub = otl->subtables; sub!=NULL; sub=sub->next )
		if ( sub->kc==NULL )
return( sub );
	    found = otl;
	}
    }

    if ( found==NULL ) {
	found = chunkalloc(sizeof(OTLookup));
	found->lookup_type = lookup_type;
	found->features = chunkalloc(sizeof(FeatureScriptLangList));
	found->features->featuretag = tag;
	found->features->scripts = chunkalloc(sizeof(struct scriptlanglist));
	found->features->scripts->script = script;
	found->features->scripts->langs[0] = DEFAULT_LANG;
	found->features->scripts->lang_cnt = 1;

	SortInsertLookup(sf, found);
    }

    sub = chunkalloc(sizeof(struct lookup_subtable));
    sub->next = found->subtables;
    found->subtables = sub;
    sub->lookup = found;
    sub->per_glyph_pst_or_kern = true;

    NameOTLookup(found,sf);
return( sub );
}

struct lookup_subtable *SFSubTableMake(SplineFont *sf,uint32 tag,uint32 script,
	int lookup_type ) {
    OTLookup **base;
    OTLookup *otl, *found=NULL;
    int isgpos = lookup_type>=gpos_start;
    struct lookup_subtable *sub;
    int isnew = false;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    base = isgpos ? &sf->gpos_lookups : &sf->gsub_lookups;
    for ( otl= *base; otl!=NULL; otl=otl->next ) {
	if ( otl->lookup_type==lookup_type &&
		FeatureScriptTagInFeatureScriptList(tag,script,otl->features) ) {
	    found = otl;
	}
    }

    if ( found==NULL ) {
	found = chunkalloc(sizeof(OTLookup));
	found->lookup_type = lookup_type;
	found->features = chunkalloc(sizeof(FeatureScriptLangList));
	found->features->featuretag = tag;
	found->features->scripts = chunkalloc(sizeof(struct scriptlanglist));
	found->features->scripts->script = script;
	found->features->scripts->langs[0] = DEFAULT_LANG;
	found->features->scripts->lang_cnt = 1;

	SortInsertLookup(sf, found);
	isnew = true;
    }

    sub = chunkalloc(sizeof(struct lookup_subtable));
    sub->next = found->subtables;
    found->subtables = sub;
    sub->lookup = found;

    if ( isnew )
	NameOTLookup(found,sf);
return( sub );
}

int LookupUsedNested(SplineFont *sf,OTLookup *checkme) {
    OTLookup *otl;
    struct lookup_subtable *sub;
    int r,c;

    if ( checkme->lookup_type>=gpos_start )
	otl = sf->gpos_lookups;
    else
	otl = sf->gsub_lookups;
    while ( otl!=NULL ) {
	for ( sub = otl->subtables; sub!=NULL; sub=sub->next ) {
	    if ( sub->fpst!=NULL ) {
		for ( r=0; r<sub->fpst->rule_cnt; ++r ) {
		    struct fpst_rule *rule = &sub->fpst->rules[r];
		    for ( c=0; c<rule->lookup_cnt; ++c ) {
			if ( rule->lookups[c].lookup == checkme )
return( true );
		    }
		}
	    } else if ( otl->lookup_type==morx_context ) {
		for ( c = 0; c<sub->sm->class_cnt*sub->sm->state_cnt; ++c ) {
		    struct asm_state *state = &sub->sm->state[c];
		    if ( state->u.context.mark_lookup==checkme )
return( true );
		    if ( state->u.context.cur_lookup==checkme )
return( true );
		}
	    }
	}
    }
return( false );
}

static void AALTRemoveOld(SplineFont *sf) {
    FeatureScriptLangList *fl, *prev;
    OTLookup *otl, *otlnext;

    for ( otl=sf->gsub_lookups; otl!=NULL; otl=otlnext ) {
	otlnext = otl->next;
	prev = NULL;
	for ( fl = otl->features; fl!=NULL; prev=fl, fl=fl->next ) {
	    if ( fl->featuretag==CHR('a','a','l','t') ) {
		if ( fl==otl->features && fl->next==NULL && !LookupUsedNested(sf,otl))
		    SFRemoveLookup(sf,otl,0);
		else {
		    if ( prev==NULL )
			otl->features = fl->next;
		    else
			prev->next = fl->next;
		    fl->next = NULL;
		    FeatureScriptLangListFree(fl);
		}
	break;
	    }
	}
    }
}

void SllkFree(struct sllk *sllk,int sllk_cnt) {
    int i;

    for ( i=0; i<sllk_cnt; ++i ) {
	free( sllk[i].langs );
	free( sllk[i].lookups );
    }
    free(sllk);
}

static void AddOTLToSllk(struct sllk *sllk, OTLookup *otl, struct scriptlanglist *sl) {
    int i,j,k,l;

    if ( otl->lookup_type==gsub_single || otl->lookup_type==gsub_alternate ) {
	for ( i=0; i<sllk->cnt; ++i )
	    if ( sllk->lookups[i]==otl )
	break;
	if ( i==sllk->cnt ) {
	    if ( sllk->cnt>=sllk->max )
		sllk->lookups = realloc(sllk->lookups,(sllk->max+=5)*sizeof(OTLookup *));
	    sllk->lookups[sllk->cnt++] = otl;
	    for ( l=0; l<sl->lang_cnt; ++l ) {
		uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
		for ( j=0; j<sllk->lcnt; ++j )
		    if ( sllk->langs[j]==lang )
		break;
		if ( j==sllk->lcnt ) {
		    if ( sllk->lcnt>=sllk->lmax )
			sllk->langs = realloc(sllk->langs,(sllk->lmax+=sl->lang_cnt+MAX_LANG)*sizeof(uint32));
		    sllk->langs[sllk->lcnt++] = lang;
		}
	    }
	}
    } else if ( otl->lookup_type==gsub_context || otl->lookup_type==gsub_contextchain ) {
	struct lookup_subtable *sub;
	for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	    FPST *fpst = sub->fpst;
	    for ( j=0; j<fpst->rule_cnt; ++j ) {
		struct fpst_rule *r = &fpst->rules[j];
		for ( k=0; k<r->lookup_cnt; ++k )
		    AddOTLToSllk(sllk,r->lookups[k].lookup,sl);
	    }
	}
    }
    /* reverse contextual chaining is weird and I shall ignore it. Adobe does too*/
}

static char *ComponentsFromPSTs(PST **psts,int pcnt) {
    char **names=NULL;
    int ncnt=0, nmax=0;
    int i,j,len;
    char *ret;

    /* First find all the names */
    for ( i=0; i<pcnt; ++i ) {
	char *nlist = psts[i]->u.alt.components;
	char *start, *pt, ch;

	for ( start = nlist; ; ) {
	    while ( *start==' ' )
		++start;
	    if ( *start=='\0' )
	break;
	    for ( pt=start; *pt!=' ' && *pt!='\0'; ++pt );
	    ch = *pt; *pt = '\0';
	    for ( j=0; j<ncnt; ++j )
		if ( strcmp( start,names[j])==0 )
	    break;
	    if ( j==ncnt ) {
		if ( ncnt>=nmax )
		    names = realloc(names,(nmax+=10)*sizeof(char *));
		names[ncnt++] = copy(start);
	    }
	    *pt = ch;
	    start = pt;
	}
    }

    len = 0;
    for ( i=0; i<ncnt; ++i )
	len += strlen(names[i])+1;
    if ( len==0 ) len=1;
    ret = malloc(len);
    len = 0;
    for ( i=0; i<ncnt; ++i ) {
	strcpy(ret+len,names[i]);
	len += strlen(names[i]);
	ret[len++] = ' ';
    }
    if ( len==0 )
	*ret = '\0';
    else
	ret[len-1] = '\0';

    for ( i=0; i<ncnt; ++i )
	free(names[i]);
    free(names);
return( ret );
}

static int SllkMatch(struct sllk *sllk,int s1,int s2) {
    int i;

    if ( sllk[s1].cnt != sllk[s2].cnt )
return( false );

    for ( i=0; i<sllk[s1].cnt; ++i ) {
	if ( sllk[s1].lookups[i] != sllk[s2].lookups[i] )
return( false );
    }

return( true );
}

struct sllk *AddOTLToSllks( OTLookup *otl, struct sllk *sllk,
	int *_sllk_cnt, int *_sllk_max ) {
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int s;

    for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
	for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
	    for ( s=0; s<*_sllk_cnt; ++s )
		if ( sl->script == sllk[s].script )
	    break;
	    if ( s==*_sllk_cnt ) {
		if ( *_sllk_cnt>=*_sllk_max )
		    sllk = realloc(sllk,((*_sllk_max)+=10)*sizeof(struct sllk));
		memset(&sllk[*_sllk_cnt],0,sizeof(struct sllk));
		sllk[(*_sllk_cnt)++].script = sl->script;
	    }
	    AddOTLToSllk(&sllk[s], otl,sl);
	}
    }
return( sllk );
}

OTLookup *NewAALTLookup(SplineFont *sf,struct sllk *sllk, int sllk_cnt, int i) {
    OTLookup *otl;
    struct lookup_subtable *sub;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    PST **psts, *pst;
    int j,k,l;
    int gid,pcnt;
    SplineFont *_sf;
    SplineChar *sc;

    /* Make the new lookup (and all its supporting data structures) */
    otl = chunkalloc(sizeof(OTLookup));
    otl->lookup_type = gsub_alternate;
    otl->lookup_flags = sllk[i].lookups[0]->lookup_flags & pst_r2l;
    otl->features = fl = chunkalloc(sizeof(FeatureScriptLangList));
    fl->featuretag = CHR('a','a','l','t');
    /* Any other scripts with the same lookup set? */
    for ( j=i; j<sllk_cnt; ++j ) {
	if ( i==j || SllkMatch(sllk,i,j)) {
	    sl = chunkalloc(sizeof(struct scriptlanglist));
	    sl->next = fl->scripts;
	    fl->scripts = sl;
	    sl->script = sllk[j].script;
	    sl->lang_cnt = sllk[j].lcnt;
	    if ( sl->lang_cnt>MAX_LANG )
		sl->morelangs = malloc((sl->lang_cnt-MAX_LANG)*sizeof(uint32));
	    for ( l=0; l<sl->lang_cnt; ++l )
		if ( l<MAX_LANG )
		    sl->langs[l] = sllk[j].langs[l];
		else
		    sl->morelangs[l-MAX_LANG] = sllk[j].langs[l];
	    if ( i!=j ) sllk[j].cnt = 0;	/* Mark as processed */
	}
    }
    otl->subtables = sub = chunkalloc(sizeof(struct lookup_subtable));
    sub->lookup = otl;
    sub->per_glyph_pst_or_kern = true;

    /* Add it to the various lists it needs to be in */
    otl->next = sf->gsub_lookups;
    sf->gsub_lookups = otl;

    /* Now look at every glyph in the font, and see if it has any of the */
    /*  lookups we are interested in, and if it does, build a new pst */
    /*  containing all posibilities listed on any of them */
    if ( sf->cidmaster ) sf = sf->cidmaster;
    psts = malloc(sllk[i].cnt*sizeof(PST *));
    k=0;
    do {
	_sf = k<sf->subfontcnt ? sf->subfonts[k] : sf;
	for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc = _sf->glyphs[gid])!=NULL ) {
	    pcnt = 0;
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable==NULL )
	    continue;
		for ( j=0; j<sllk[i].cnt; ++j )
		    if ( pst->subtable->lookup == sllk[i].lookups[j] )
		break;
		if ( j<sllk[i].cnt )
		    psts[pcnt++] = pst;
	    }
	    if ( pcnt==0 )
	continue;
	    pst = chunkalloc(sizeof(PST));
	    pst->subtable = sub;
	    pst->type = pst_alternate;
	    pst->next = sc->possub;
	    sc->possub = pst;
	    pst->u.alt.components = ComponentsFromPSTs(psts,pcnt);
	}
	++k;
    } while ( k<sf->subfontcnt );
    free(psts);
    NameOTLookup(otl,sf);
return( otl );
}

void AddNewAALTFeatures(SplineFont *sf) {
    /* different script/lang combinations may need different 'aalt' lookups */
    /*  well, let's just say different script combinations */
    /* for each script/lang combo find all single/alternate subs for each */
    /*  glyph. Merge those choices and create new lookup with that info */
    struct sllk *sllk = NULL;
    int sllk_cnt=0, sllk_max = 0;
    int i;
    OTLookup *otl;

    AALTRemoveOld(sf);

    /* Find all scripts, and all the single/alternate lookups for each */
    /*  and all the languages used for these in each script */
    for ( otl=sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
	sllk = AddOTLToSllks( otl, sllk, &sllk_cnt, &sllk_max );
    }
    /* Each of these gets its own gsub_alternate lookup which gets inserted */
    /*  at the head of the lookup list. Each lookup has one subtable */
    for ( i=0; i<sllk_cnt; ++i ) {
	if ( sllk[i].cnt==0 )		/* Script used, but provides no alternates */
    continue;
	NewAALTLookup(sf,sllk,sllk_cnt,i);
    }

    SllkFree(sllk,sllk_cnt);
}


int VerticalKernFeature(SplineFont *sf, OTLookup *otl, int ask) {
    FeatureScriptLangList *fl;
    struct lookup_subtable *sub;
    KernClass *kc;
    char *buts[3];

    for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
	if ( fl->featuretag==CHR('k','e','r','n') )
return( false );
	else if ( fl->featuretag==CHR('v','k','r','n') )
return( true );
    }

    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	if ( sub->kc!=NULL ) {
	    for ( kc=sf->kerns; kc!=NULL; kc=kc->next )
		if ( kc==sub->kc )
return( false );
	    for ( kc=sf->vkerns; kc!=NULL; kc=kc->next )
		if ( kc==sub->kc )
return( true );
	}
    }

    if ( !ask )
return( -1 );

    buts[0] = _("_Horizontal"); buts[1] = _("_Vertical"); buts[2] = NULL;
return( ff_ask(_("Kerning direction"),(const char **) buts,0,1,_("Is this horizontal or vertical kerning data?")) );
}

int IsAnchorClassUsed(SplineChar *sc,AnchorClass *an) {
    AnchorPoint *ap;
    int waslig=0, sawentry=0, sawexit=0;

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->anchor==an ) {
	    if ( ap->type==at_centry )
		sawentry = true;
	    else if ( ap->type==at_cexit )
		sawexit = true;
	    else if ( an->type==act_mkmk ) {
		if ( ap->type==at_basemark )
		    sawexit = true;
		else
		    sawentry = true;
            } else if ( an->type==act_unknown ) {
                if ( ap->type==at_basechar )
                    sawexit = true;
                else
                    sawentry = true;
	    } else if ( ap->type!=at_baselig )
return( -1 );
	    else if ( waslig<ap->lig_index+1 )
		waslig = ap->lig_index+1;
	}
    }
    if ( sawentry && sawexit )
return( -1 );
    else if ( sawentry )
return( -2 );
    else if ( sawexit )
return( -3 );
return( waslig );
}

int PSTContains(const char *components,const char *name) {
    const char *pt;
    int len = strlen(name);

    for ( pt = strstr(components,name); pt!=NULL; pt = strstr(pt+len,name)) {
	if (( pt==components || pt[-1]==' ') && (pt[len]==' ' || pt[len]=='\0'))
return( true );
    }
return( false );
}


int KernClassFindIndexContaining( char **firsts_or_seconds,
				  int firsts_or_seconds_size,
				  const char *name )
{
    int ret = -1;
    int i = 0 ;

    for ( i=1; i < firsts_or_seconds_size; ++i )
    {
	if ( PSTContains(firsts_or_seconds[i],name) )
	{
	    ret = i;
	    break;
	}
    }

    return ret;
}


int KernClassContains(KernClass *kc, const char *name1, const char *name2, int ordered ) {
    int infirst=0, insecond=0, scpos1, kwpos1, scpos2, kwpos2;
    int i;

    for ( i=1; i<kc->first_cnt; ++i ) {
	if ( PSTContains(kc->firsts[i],name1) ) {
	    scpos1 = i;
	    if ( ++infirst>=3 )		/* The name occurs twice??? */
    break;
	} else if ( PSTContains(kc->firsts[i],name2) ) {
	    kwpos1 = i;
	    if ( (infirst+=2)>=3 )
    break;
	}
    }
    if ( infirst==0 || infirst>3 )
return( 0 );
    for ( i=1; i<kc->second_cnt; ++i ) {
	if ( PSTContains(kc->seconds[i],name1) ) {
	    scpos2 = i;
	    if ( ++insecond>=3 )
    break;
	} else if ( PSTContains(kc->seconds[i],name2) ) {
	    kwpos2 = i;
	    if ( (insecond+=2)>=3 )
    break;
	}
    }
    if ( insecond==0 || insecond>3 )
return( 0 );
    if ( (infirst&1) && (insecond&2) ) {
	if ( kc->offsets[scpos1*kc->second_cnt+kwpos2]!=0 )
return( kc->offsets[scpos1*kc->second_cnt+kwpos2] );
    }
    if ( !ordered ) {
	if ( (infirst&2) && (insecond&1) ) {
	    if ( kc->offsets[kwpos1*kc->second_cnt+scpos2]!=0 )
return( kc->offsets[kwpos1*kc->second_cnt+scpos2] );
	}
    }
return( 0 );
}

int KCFindName(const char *name, char **classnames, int cnt, int allow_class0 ) {
    int i;
    char *pt, *end, ch;

    for ( i=0; i<cnt; ++i ) {
	if ( classnames[i]==NULL )
    continue;
	for ( pt = classnames[i]; *pt; pt=end+1 ) {
	    end = strchr(pt,' ');
	    if ( end==NULL ) end = pt+strlen(pt);
	    ch = *end;
	    *end = '\0';
	    if ( strcmp(pt,name)==0 ) {
		*end = ch;
return( i );
	    }
	    *end = ch;
	    if ( ch=='\0' )
	break;
	}
    }
    /* If class 0 is specified, then we didn't find anything. If class 0 is */
    /*  unspecified then it means "anything" so we found something */
return( classnames[0]!=NULL || !allow_class0 ? -1 : 0 );
}

/* Routines to generate human readable forms of FPST rules */
static void GrowBufferAddLookup(GrowBuf *gb,struct fpst_rule *rule, int seq) {
    int i;

    for ( i=0; i<rule->lookup_cnt; ++i ) {
	if ( seq==rule->lookups[i].seq ) {
	    GrowBufferAddStr(gb,"@<");
	    GrowBufferAddStr(gb,rule->lookups[i].lookup->lookup_name);
	    GrowBufferAddStr(gb,"> ");
	}
    }
}

static void GrowBufferAddClass(GrowBuf *gb,int class_n,char **classnames, int class_cnt) {
    char buffer[20], *str;

    if ( class_n<0 || class_n>=class_cnt ) {
	IError("Bad class in FPST" );
	class_n = 0;
    }
    if ( classnames==NULL || (str=classnames[class_n])==NULL ) {
	sprintf( buffer,"%d", class_n );
	str = buffer;
    }
    GrowBufferAddStr(gb,str);
    GrowBufferAdd(gb,' ');
}

int GlyphNameCnt(const char *pt) {
    int cnt = 0;

    while ( *pt ) {
	while ( isspace( *pt )) ++pt;
	if ( *pt=='\0' )
return( cnt );
	++cnt;
	while ( !isspace(*pt) && *pt!='\0' ) ++pt;
    }
return( cnt );
}

char *reverseGlyphNames(char *str) {
    char *ret;
    char *rpt, *pt, *start, *spt;

    if ( str==NULL )
return( NULL );

    rpt = ret = malloc(strlen(str)+1);
    *ret = '\0';
    for ( pt=str+strlen(str); pt>str; pt=start ) {
	for ( start = pt-1; start>=str && *start!=' '; --start );
	for ( spt=start+1; spt<pt; )
	    *rpt++ = *spt++;
	*rpt++ = ' ';
    }
    if ( rpt>ret )
	rpt[-1] = '\0';
return( ret );
}

/* Note: My string format cannot represent contextual rules were the order */
/*  of application of lookups is not the same as textual order. This rarely */
/*  happens. Adobe's feature files have the same drawback. And there are */
/*  rarely two or more lookups applied by one rule. But it could, so be aware!*/
char *FPSTRule_To_Str(SplineFont *sf,FPST *fpst,struct fpst_rule *rule) {
    int i, max=0;
    char *ret, *npt;
    int seq=0;
    GrowBuf gb;

    /* Note that nothing in the output distinquishes between back, match and forward */
    /*  the thought being that to all intents and purposes, match starts at */
    /*  the first lookup and ends at the last. Anything before is back, */
    /*  anything after is for */ /* Adobe uses this convention in feature files*/
    /* Drat. That doesn't work for classes if the back/fore classes differ */
    /*  from match classes */ /* Adobe doesn't support classes in feature files*/
    /* Nor does it work when the lookup needs more than one input characters */
    /*  ligature lookups */
    memset(&gb,0,sizeof(gb));
    switch ( fpst->format ) {
      case pst_glyphs:
	max = ( rule->u.glyph.names ? strlen(rule->u.glyph.names) : 0 ) +
		( rule->u.glyph.back ? strlen(rule->u.glyph.back) : 0 ) +
		( rule->u.glyph.fore ? strlen(rule->u.glyph.fore) : 0 ) +
		200;
	gb.base = gb.pt = malloc(max+1);
	gb.end = gb.base+max;
        if ( rule->u.glyph.back!=NULL ) {
	    char *temp;
	    GrowBufferAddStr(&gb,(temp = reverseGlyphNames(rule->u.glyph.back)));
	    free(temp);
	    GrowBufferAdd(&gb,' ');
	}
	if ( fpst->type!=pst_contextpos && fpst->type!=pst_contextsub )
	    GrowBufferAddStr(&gb,"| ");
	for ( npt=rule->u.glyph.names; isspace(*npt); ++npt)
	    /* Skip over leading spaces, if any */;
	for ( npt=rule->u.glyph.names, seq=0; *npt; ++seq ) {
	    while ( isspace(*npt))
		++npt;
	    while ( *npt!='\0' && !isspace( *npt ) ) {
		GrowBufferAdd(&gb, *npt++);
	    }
	    GrowBufferAdd(&gb,' ');
	    GrowBufferAddLookup(&gb,rule,seq);
	}
	if ( fpst->type!=pst_contextpos && fpst->type!=pst_contextsub )
	    GrowBufferAddStr(&gb,"| ");
        if ( rule->u.glyph.fore!=NULL ) {
	    GrowBufferAddStr(&gb,rule->u.glyph.fore);
	}
      break;
      case pst_class:
	/* Reverse the backtrack classes */
	for ( i=rule->u.class.bcnt-1; i>=0; --i )
	    GrowBufferAddClass(&gb,rule->u.class.bclasses[i],fpst->bclassnames,fpst->bccnt);
	if ( fpst->type!=pst_contextpos && fpst->type!=pst_contextsub )
	    GrowBufferAddStr(&gb,"| ");
	for ( i=0; i<rule->u.class.ncnt; ++i ) {
	    GrowBufferAddClass(&gb,rule->u.class.nclasses[i],fpst->nclassnames,fpst->nccnt);
	    GrowBufferAddLookup(&gb,rule,i);
	}
	if ( fpst->type!=pst_contextpos && fpst->type!=pst_contextsub )
	    GrowBufferAddStr(&gb,"| ");
	for ( i=0; i<rule->u.class.fcnt; ++i )
	    GrowBufferAddClass(&gb,rule->u.class.fclasses[i],fpst->fclassnames,fpst->fccnt);
      break;
      case pst_coverage:
      case pst_reversecoverage:
        /* Reverse the backtrack tables */
	for ( i=rule->u.coverage.bcnt-1; i>=0; --i ) {
	    GrowBufferAdd(&gb,'[');
	    GrowBufferAddStr(&gb,rule->u.coverage.bcovers[i]);
	    GrowBufferAddStr(&gb,"] ");
	}
	if ( fpst->type!=pst_contextpos && fpst->type!=pst_contextsub )
	    GrowBufferAddStr(&gb,"| ");
	for ( i=0; i<rule->u.coverage.ncnt; ++i ) {
	    GrowBufferAdd(&gb,'[');
	    GrowBufferAddStr(&gb,rule->u.coverage.ncovers[i]);
	    GrowBufferAddStr(&gb,"] ");
	    if ( fpst->format==pst_reversecoverage ) {
		GrowBufferAddStr(&gb,"=> [");
		GrowBufferAddStr(&gb,rule->u.rcoverage.replacements);
		GrowBufferAddStr(&gb,"] ");
	    } else {
		GrowBufferAddLookup(&gb,rule,i);
	    }
	}
	if ( fpst->type!=pst_contextpos && fpst->type!=pst_contextsub )
	    GrowBufferAddStr(&gb,"| ");
	for ( i=0; i<rule->u.coverage.fcnt; ++i ) {
	    GrowBufferAdd(&gb,'[');
	    GrowBufferAddStr(&gb,rule->u.coverage.fcovers[i]);
	    GrowBufferAddStr(&gb,"] ");
	}
      break;
      default:
	IError( "Bad FPST format");
return( NULL );
    }
    if ( gb.pt>gb.base && gb.pt[-1]==' ' )
	gb.pt[-1] = '\0';
    ret = copy(gb.base);
    free(gb.base);
return( ret );
}

typedef struct lookuplist {
    OTLookup *lookup;
    struct lookuplist *next;
} LookupList;

typedef struct matchstr {
    char *entity;
    char *replacements;		/* For reverse contextual chaining */
    LookupList *lookups;
} MatchStr;

/* Returns an error message, a warning, or NULL if parsing were successful */
/*  if successful rule will be filled in. Error msg must be freed */
char *FPSTRule_From_Str(SplineFont *sf,FPST *fpst,struct fpst_rule *rule,
	char *line, int *return_is_warning ) {
    char *lpt, *start, *end;
    int ch;
    int do_replacements=0, anylookup=0;
    int cnt=0, max=0;
    MatchStr *parsed=NULL;
    LookupList *ll, *llp;
    int i,j,first,last;
    char *ret;
    int isgpos = fpst->type==pst_contextpos || fpst->type==pst_chainpos;
    OTLookup *lookup;

    /* Parse the string into meaningful chunks. These could be:
	A coverage table (a list of glyph names enclosed in []
	A glyph name	(I'll accept this as a degenerate coverage table containing one glyph)
	A class name
	A lookup invocation
	A list of replacement glyphs for a reverse contextual chaining lookup
	    (This is a two step process. First parse the replacement marker "=>"
	     then parse the coverage table)
	We can also have a mark that we've switched from the backtracking list
	    to the match list to the forward list. Might be needed for classes
    */

    first = last = -1;
    *return_is_warning = false;
    for ( lpt=line; *lpt; ) {
	while ( isspace(*lpt)) ++lpt;
	if ( *lpt=='\0' )
    break;
	start = lpt;
	if ( *start=='|' ) {
	    if ( fpst->type==pst_contextpos || fpst->type==pst_contextsub )
return( xasprintf( _("Separation marks only meaningful in contextual chaining lookups, starting at: %.20s..."), lpt ));
	    if ( first==-1 )
		first = cnt;
	    else if ( last==-1 )
		last = cnt-1;
	    else
return( xasprintf( _("Too many separation marks, starting at: %.20s..."), lpt ));
	    ++lpt;
    continue;
	} else if ( *start=='[' ) {
	    /* A coverage table */
	    if ( fpst->format!=pst_coverage && fpst->format!=pst_reversecoverage )
return( xasprintf( _("A coverage table was found in a glyph or class based contextual lookup, starting at: %.20s..."), lpt ));
	    ++start;
	    for ( lpt = start; *lpt!='\0' && *lpt!=']'; ++lpt );
	    if ( *lpt!=']' )
return( xasprintf( _("Unterminated coverage table, starting at: %.20s..."), start-1 ));
	    end = lpt++;
	    if ( do_replacements==1 ) {
		int rcnt, ecnt;
		do_replacements = 2;
		if ( cnt==0 )
return( xasprintf( _("Replacements must follow the coverage table to which they apply: %s"), start-4 ));
		ch = *end; *end = '\0';
		parsed[cnt].replacements = copy(start);
		*end = ch;
		rcnt = GlyphNameCnt(parsed[cnt].replacements);
		ecnt = GlyphNameCnt(parsed[cnt].entity);
		if ( ecnt==rcnt )
		    /* Good */;
		else if ( rcnt==1 && ecnt>1 ) {
		    char *newr;
		    newr = malloc(ecnt*(strlen(parsed[cnt].replacements)+1)+1);
		    *newr = '\0';
		    for ( i=0; i<ecnt; ++i ) {
			strcat(newr,parsed[cnt].replacements);
			if ( i!=ecnt-1 )
			    strcat(newr," ");
		    }
		    free(parsed[cnt].replacements);
		    parsed[cnt].replacements = newr;
		} else
return( xasprintf( _("There must be as many replacement glyphs as there are match glyphs: %s => %s"),
		    parsed[cnt].entity, parsed[cnt].replacements));
    continue;
	    }
	} else if ( *start!='@' && *start!='<' && !(*start=='=' && start[1]=='>') ) {
	    /* Just a normal glyph or class name. (If we expect a coverage table we'll treat it as a table with one glyph) */
	    while ( *lpt!='\0' && !isspace(*lpt) && *lpt!='@' && *lpt!='<' && *lpt!='[' )
		++lpt;
	    end = lpt;
	} else if ( *start=='=' && start[1]=='>' ) {
	    /* A reverse contextual chaining */
	    if ( fpst->format!=pst_reversecoverage )
return( xasprintf( _("No replacement lists may be specified in this contextual lookup, use a nested lookup instead, starting at: %.20s..."), lpt ));
	    if ( do_replacements )
return( xasprintf( _("Only one replacement list may be specified in a reverse contextual chaining lookup, starting at: %.20s..."), lpt ));
	    do_replacements = true;
	    lpt += 2;
	} else {
	    /* A lookup invocation */
	    if ( fpst->format==pst_reversecoverage )
return( xasprintf( _("No lookups may be specified in a reverse contextual lookup (use a replacement list instead), starting at: %.20s..."), lpt ));

	    if ( *start=='@' ) {
		for ( lpt=start+1; isspace( *lpt ); ++lpt );
		if ( *lpt!='<' )
return( xasprintf( _("A lookup invocation must be started by the sequence '@<' and ended with '>', starting at: %.20s..." ), start ) );
	    }
	    start= ++lpt;
	    for ( lpt = start; *lpt!='\0' && *lpt!='>'; ++lpt );
	    if ( *lpt!='>' )
return( xasprintf( _("Unterminated lookup invocation, starting at: %.20s..."), start-1 ));
	    *lpt = '\0';
	    lookup = SFFindLookup(sf,start);
	    if ( lookup==NULL ) {
		ret = xasprintf( _("Unknown lookup: %s"), start );
		*lpt = '>';
return( ret );
	    } else if ( (isgpos && lookup->lookup_type<gpos_start) || (!isgpos && lookup->lookup_type>gpos_start)) {
		ret = xasprintf( isgpos ? _("GSUB lookup refered to in this GPOS contextual lookup: %s"):
			    _("GPOS lookup refered to in this GSUB contextual lookup: %s"),
			start );
		*lpt = '>';
return( ret );
	    } else if ( cnt==0 ) {
		ret = xasprintf( _("Lookups must follow the glyph, class or coverage table to which they apply: %s"), start );
		*lpt = '>';
return( ret );
	    }
	    *lpt++ = '>';
	    ll = chunkalloc(sizeof(LookupList));
	    ll->lookup = lookup;
	    /* Lookup order is important */
	    if ( parsed[cnt-1].lookups==NULL )
		parsed[cnt-1].lookups = ll;
	    else {
		for ( llp=parsed[cnt-1].lookups; llp->next!=NULL; llp=llp->next );
		llp->next = ll;
	    }
	    anylookup = true;
	    if ( first==-1 ) first = cnt-1;
    continue;
	}
	/* We get here on glyph/class names and coverage tables */
	/*  not on lookup invocations */
	ch = *end; *end='\0';
	if ( cnt>=max )
	    parsed = realloc(parsed,(max+=200)*sizeof(MatchStr));
	memset(&parsed[cnt],'\0',sizeof(MatchStr));
	parsed[cnt++].entity = copy(start);
	*end = ch;
    }

    if ( cnt==0 )
return( copy( _("Empty rule" )) );

    ret = NULL;

    if ( !do_replacements && !anylookup ) {
	if ( fpst->format==pst_reversecoverage )
return( copy( _("A reverse contextual chaining lookup must have a set of replacement glyphs somewhere" )) );
	else {
	    *return_is_warning = true;
	    /* If there are no lookups then this rule matches and does nothing */
	    /*  which can make it easier to write subsequent rules */
	    ret = copy( _("This contextual rule applies no lookups." ));
	}
    }

    /* Figure out the boundaries between backtrack, match and forward */
    for ( i=0; i<cnt; ++i ) {
	if ( parsed[i].lookups!=NULL ) {
	    if ( i>last ) last = i;
	    if ( first==-1 ) first = i;
	}
	if ( parsed[i].replacements!=NULL ) {
	    if (( first!=-1 && first!=i ) || (last!=-1 && last!=i ))
return( copy( _("A reverse contextual chaining lookup can only match one coverage table directly" )) );
	    first = last = i;
	}
    }
    if ( fpst->type==pst_contextpos || fpst->type==pst_contextsub ) {
	first = 0;
	last = cnt-1;
    }

    switch ( fpst->format ) {
      case pst_glyphs: {
	int blen=0, mlen=0, flen=0;
	for ( i=0; i<cnt; ++i ) {
	    if ( SFGetChar(sf,-1,parsed[i].entity)==NULL ) {
		if ( ret==NULL ) {
		    ret = xasprintf( _("There is no glyph named \"%s\" in the font."), parsed[i].entity );
		    *return_is_warning = true;
		}
	    }
	    if ( i<first )
		blen += strlen(parsed[i].entity)+1;
	    else if ( i<=last )
		mlen += strlen(parsed[i].entity)+1;
	    else
		flen += strlen(parsed[i].entity)+1;
	}
	rule->u.glyph.names = calloc(mlen+1,1);
	if ( blen!=0 )
	    rule->u.glyph.back = calloc(blen+1,1);
	if ( flen!=0 )
	    rule->u.glyph.fore = calloc(flen+1,1);
	for ( i=0; i<cnt; ++i ) {
	    if ( i<first ) {
		strcat(rule->u.glyph.back,parsed[i].entity);
		if ( i!=first-1)
		    strcat(rule->u.glyph.back," ");
	    } else if ( i<=last ) {
		strcat(rule->u.glyph.names,parsed[i].entity);
		if ( i!=last)
		    strcat(rule->u.glyph.names," ");
	    } else {
		strcat(rule->u.glyph.fore,parsed[i].entity);
		if ( i!=cnt-1)
		    strcat(rule->u.glyph.fore," ");
	    }
	}
	if ( blen!=0 ) {
	    char *temp = reverseGlyphNames(rule->u.glyph.back);
	    free(rule->u.glyph.back);
	    rule->u.glyph.back = temp;
	}
      } break;
      case pst_class:
        rule->u.class.ncnt = last+1-first;
	rule->u.class.nclasses = malloc(rule->u.class.ncnt*sizeof(uint16));
	rule->u.class.bcnt = first;
	if ( first!=0 )
	    rule->u.class.bclasses = malloc(first*sizeof(uint16));
	rule->u.class.fcnt = cnt==last?0:cnt-last-1;
	if ( rule->u.class.fcnt!=0 )
	    rule->u.class.fclasses = malloc(rule->u.class.fcnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i ) {
	    char **classnames, *pend;
	    int class_cnt, val;
	    if ( i<first ) {
		classnames = fpst->bclassnames;
		class_cnt = fpst->bccnt;
	    } else if ( i<=last ) {
		classnames = fpst->nclassnames;
		class_cnt = fpst->nccnt;
	    } else {
		classnames = fpst->fclassnames;
		class_cnt = fpst->fccnt;
	    }
	    val = strtol(parsed[i].entity,&pend,10);
	    if ( *pend!='\0' )
		val = -1;
	    for ( j=0; j<class_cnt; ++j ) {
		if ( classnames[j]!=NULL ) {
		    if ( strcmp(parsed[i].entity,classnames[j])==0 )
	    break;
		} else {
		    if ( val==j )
	    break;
		}
	    }
	    if ( j==class_cnt ) {
		free( rule->u.class.nclasses ); rule->u.class.nclasses = NULL;
		free( rule->u.class.bclasses ); rule->u.class.bclasses = NULL;
		free( rule->u.class.fclasses ); rule->u.class.fclasses = NULL;
		rule->u.class.bcnt = rule->u.class.fcnt = rule->u.class.ncnt = 0;
		if ( i<first )
return( xasprintf( _("%s is not a class name for the backtracking classes." ), parsed[i].entity ) );
		else if ( i<=last )
return( xasprintf( _("%s is not a class name for the matching classes." ), parsed[i].entity ) );
		else
return( xasprintf( _("%s is not a class name for the forward classes." ), parsed[i].entity ) );
	    }
	    if ( i<first )
		rule->u.class.bclasses[first-1-i] = j;	/* Reverse the backtrack classes */
	    else if ( i<=last )
		rule->u.class.nclasses[i-first] = j;
	    else
		rule->u.class.fclasses[i-last-1] = j;
	}
      break;
      case pst_coverage:
      case pst_reversecoverage:
	for ( i=0; i<cnt; ++i ) {
	    for ( lpt = parsed[i].entity; *lpt ; ) {
		while ( isspace(*lpt)) ++lpt;
		if ( *lpt=='\0' )
	    break;
		start = lpt;
		while ( !isspace(*lpt) && *lpt!='\0' )
		    ++lpt;
		ch = *lpt; *lpt='\0';
		if ( SFGetChar(sf,-1,start)==NULL ) {
		    if ( ret==NULL ) {
			ret = xasprintf( _("There is no glyph named \"%s\" in the font."), start );
			*return_is_warning = true;
		    }
		}
		*lpt = ch;
	    }
	}
        rule->u.coverage.ncnt = last+1-first;
	rule->u.coverage.ncovers = malloc(rule->u.coverage.ncnt*sizeof(char *));
	rule->u.coverage.bcnt = first;
	if ( first!=0 )
	    rule->u.coverage.bcovers = malloc(first*sizeof(char *));
	rule->u.coverage.fcnt = cnt-last-1;
	if ( rule->u.coverage.fcnt!=0 )
	    rule->u.coverage.fcovers = malloc(rule->u.coverage.fcnt*sizeof(char *));
	for ( i=0; i<cnt; ++i ) {
	    if ( i<first )
		rule->u.coverage.bcovers[first-1-i] = parsed[i].entity;	/* Reverse the order of backtrack coverage tables */
	    else if ( i<=last ) {
		rule->u.coverage.ncovers[i-first] = parsed[i].entity;
		if ( fpst->format==pst_reversecoverage ) {
		    rule->u.rcoverage.replacements = parsed[i].replacements;
		    parsed[i].replacements = NULL;
		}
	    } else
		rule->u.coverage.ncovers[i-last-1] = parsed[i].entity;
	    parsed[i].entity = NULL;
	}
      break;
      default:
return( copy( _("Bad FPST format")) );
    }
    if ( fpst->format!=pst_reversecoverage ) {
	int tot=0;
	for ( i=first; i<=last; ++i ) {
	    for ( ll=parsed[i].lookups; ll!=NULL; ll=ll->next )
		++tot;
	}
	rule->lookups = calloc(tot,sizeof(struct seqlookup));
	rule->lookup_cnt = tot;
	tot = 0;
	for ( i=first; i<=last; ++i ) {
	    for ( ll=parsed[i].lookups; ll!=NULL; ll=llp ) {
		llp = ll->next;
		rule->lookups[tot].seq = i-first;
		rule->lookups[tot].lookup = ll->lookup;
		++tot;
		chunkfree(ll,sizeof(*ll));
	    }
	}
    }
    for ( i=0; i<cnt; ++i )
	free( parsed[i].entity );
    free(parsed);
return( ret );
}

/* User interface functionality when we have no UI */
static void NOFI_SortInsertLookup(SplineFont *sf, OTLookup *newotl) {
}

static void NOFI_OTLookupCopyInto(SplineFont *into_sf,SplineFont *from_sf,
	OTLookup *from_otl, OTLookup *to_otl, int scnt, OTLookup *before ) {
}

static void NOFI_Destroy(SplineFont *sf) {
}

struct fi_interface noui_fi = {
    NOFI_SortInsertLookup,
    NOFI_OTLookupCopyInto,
    NOFI_Destroy
};

struct fi_interface *fi_interface = &noui_fi;

void FF_SetFIInterface(struct fi_interface *fii) {
    fi_interface = fii;
}
