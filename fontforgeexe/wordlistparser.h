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

#ifndef _ALREADY_INCLUDED_FF_WORDLIST_PARSER_H
#define _ALREADY_INCLUDED_FF_WORDLIST_PARSER_H

#include "splinefont.h"
#include "ffglib.h"

const char* Wordlist_getSCName( SplineChar* sc );

typedef int (*WordlistEscapedInputStringToRealString_getFakeUnicodeOfScFunc)( SplineChar *sc, void* udata );
extern int WordlistEscapedInputStringToRealString_getFakeUnicodeAsScUnicodeEnc( SplineChar *sc, void* udata );

extern unichar_t* WordlistEscapedInputStringToRealString(
    SplineFont* sf,
    const unichar_t* input_const,
    GArray** selected_out,
    WordlistEscapedInputStringToRealString_getFakeUnicodeOfScFunc getUnicodeFunc,
    void* udata );

extern unichar_t* WordlistEscapedInputStringToRealStringBasic(
    SplineFont* sf,
    unichar_t* input_const,
    GArray** selected_out );

extern GTextInfo** WordlistLoadFileToGTextInfo( int type, int words_max );
extern GTextInfo** WordlistLoadFileToGTextInfoBasic( int words_max );

extern void Wordlist_MoveByOffset( GGadget* g, int* idx, int offset );
extern void Wordlist_touch( GGadget* g );

extern void WordlistLoadToGTextInfo( GGadget* g, int* idx  );

extern void WordlistTrimTrailingSingleSlash( unichar_t* txt );

extern unichar_t* Wordlist_selectionClear( SplineFont* sf, EncMap *map, unichar_t* txtu );
extern unichar_t* Wordlist_selectionAdd( SplineFont* sf, EncMap *map, unichar_t* txtu, int offset );


extern unichar_t* Wordlist_advanceSelectedCharsBy( SplineFont* sf, EncMap *map, unichar_t* txtu, int offset );

extern bool Wordlist_selectionsEqual( unichar_t* s1, unichar_t* s2 );

#endif
