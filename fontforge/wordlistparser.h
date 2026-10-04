/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2013 Ben Martin
    
    This file is part of FontForge.

    FontForge is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation,either version 3 of the License, or
    (at your option) any later version.

    FontForge is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FontForge.  If not, see <http://www.gnu.org/licenses/>.

    For more details see the COPYING.gplv3 file in the root directory of this
    distribution.

*******************************************************************************
*******************************************************************************
******************************************************************************/
#pragma once

#include "splinefont.h"

const char* Wordlist_getSCName( SplineChar* sc );

typedef int (*WordlistEscapedInputStringToRealString_getFakeUnicodeOfScFunc)( SplineChar *sc, void* udata );
extern int WordlistEscapedInputStringToRealString_getFakeUnicodeAsScUnicodeEnc( SplineChar *sc, void* udata );

extern void WordlistTrimTrailingSingleSlash( unichar_t* txt );

extern unichar_t* Wordlist_selectionClear( SplineFont* sf, EncMap *map, unichar_t* txtu );
extern unichar_t* Wordlist_selectionAdd( SplineFont* sf, EncMap *map, unichar_t* txtu, int offset );


extern unichar_t* Wordlist_advanceSelectedCharsBy( SplineFont* sf, EncMap *map, unichar_t* txtu, int offset );


/**
 * This is the array of characters that are referenced from a
 * wordlistline. I'd felt that such a setup was much better than the
 * previous string wrangling, and with fonts that have splinechars
 * that have no unicode value, it makes more sense to explicitly
 * address the string this way.
 * 
 * for example,
 * ab/comma/slash will have 4 elements in the array and a null sc as element this[4].
 * this[0].sc = 'a'
 * this[1].sc = 'b'
 * this[2].sc = ','
 * this[3].sc = '/'
 * this[4].sc = \0
 *
 * currentGlyphIndex is a hang over from old code, still might be handy if you have a pointer
 * to a single WordListChar element and you want to know what splinechar it is in the string.
 * Selections are now handled using isSelected in each element.
 */
typedef struct wordlistchar {
    SplineChar* sc;
    int isSelected;
    int currentGlyphIndex;
    int n;
} WordListChar;

typedef WordListChar* WordListLine;

extern int WordlistSCFakeUnicode(SplineChar *sc);
extern int WordlistSCFakeUnicode_cb( SplineChar *sc, void* );
extern bool Wordlist_selectionsEqual( unichar_t* s1, unichar_t* s2 );
extern WordListLine WordListLine_end( WordListLine wll );
extern int WordListLine_size( WordListLine wll );
extern unichar_t* WordListLine_toustr( WordListLine wll );

/**
 * Parse the string input_const into a WordListLine of WordListChar
 * structures for each character in input_const. A structure such as
 * WordListChar is used to describe each character so that you can
 * quickly tell if it should be "selected" in the user interface and
 * what the character index is.
 *
 * The input_const string can be complex, including selectors for
 * glyphs so that many unichar_t are consumed to select a single
 * splinechar. So the currentGlyphIndex allows you to see which
 * SplineChar from 0 upwards the current WordListChar is.
 *
 * NOTE that this function (and
 * WordlistEscapedInputStringToParsedDataComplex) may create
 * splinechar instances in the splinefont that you pass in. So the
 * function may not be free of side effects.
 */
extern WordListLine WordlistEscapedInputStringToParsedData( SplineFont* sf,
							    unichar_t* input_const );
extern WordListLine WordlistEscapedInputStringToParsedDataComplex(
    SplineFont* sf,
    const unichar_t* input_const,
    WordlistEscapedInputStringToRealString_getFakeUnicodeOfScFunc getUnicodeFunc,
    void* udata );

/* The unput may contain unencoded glyphs references as \<glyph_name>. These
 * glyphs are recoded to fake values above the Unicode range. */
extern char* WordlistEscapedInputStringToUTF8(SplineFont* sf, const char* input);
