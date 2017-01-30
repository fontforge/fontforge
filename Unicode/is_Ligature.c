/*
Copyright: 2012 Barry Schwartz
    Create is_Ligature.c (using a python script) to test for Vulgar fractions
Copyright: 2016 Joe Da Silva
    Re-write is_Ligature.c to test for ligatures, Vulgar and Other-fractions.
Copyright: 2016 Gioele Barabucci
    Simplify makeutype.c code. Split is_Ligature.c, create is_Ligature_data.h
License: BSD-3-clause
Contributions:
*/

#include "basics.h"	/* for non-standard uint{16,32} typedefs */
#include "is_Ligature_data.h"
#include "utype.h"
#include <stdlib.h>

static int compare_codepoints16(const void *uCode1, const void *uCode2) {
    const uint16 *cp1 = (const uint16 *)(uCode1);
    const uint16 *cp2 = (const uint16 *)(uCode2);
    return( (*cp1 < *cp2) ? -1 : ((*cp1 == *cp2) ? 0 : 1) );
}

static int compare_codepoints32(const void *uCode1, const void *uCode2) {
    const uint32 *cp1 = (const uint32 *)(uCode1);
    const uint32 *cp2 = (const uint32 *)(uCode2);
    return( (*cp1 < *cp2) ? -1 : ((*cp1 == *cp2) ? 0 : 1) );
}

int LigatureCount(void) {
    return( FF_ligatureTOTAL );
}

int VulgarFractionCount(void) {
    return( FF_vulgfracTOTAL );
}

int OtherFractionCount(void) {
    return( FF_fractionTOTAL );
}

int FractionCount(void) {
    return( FF_vulgfracTOTAL + FF_fractionTOTAL );
}

int32 Ligature_get_U(int n) {
    if ( n < 0 || n >= FF_ligatureTOTAL )
	return( -1 );
    if ( n < FF_ligatureTOTAL16 )
	return( (int32)(ligature16[n]) );
    else
	return( (int32)(ligature32[n-FF_ligatureTOTAL16]) );
}

int32 VulgFrac_get_U(int n) {
    if ( n < 0 || n >= FF_vulgfracTOTAL )
	return( -1 );
    if ( n < FF_vulgfracTOTAL16 )
	return( (int32)(vulgfrac16[n]) );
    else
	return( (int32)(vulgfrac32[n-FF_vulgfracTOTAL16]) );
}

int32 Fraction_get_U(int n) {
    if ( n < 0 || n >= FF_fractionTOTAL )
	return( -1 );
    if ( n < FF_fractionTOTAL16 )
	return( (int32)(fraction16[n]) );
    else
	return( (int32)(fraction32[n-FF_fractionTOTAL16]) );
}

int Ligature_find_N(uint32 uCode) {
    uint16 uCode16, *p16;
    uint32 *p32;
    int n=-1;

    if ( uCode < FF_ligature16FIRST || uCode > FF_ligature32LAST || \
	 ((uCode < FF_UTYPE_MAXC) && (isligorfrac(uCode)==0)) )
	return( -1 );
    if ( uCode <= FF_ligature16LAST ) {
	uCode16 = uCode;
	p16 = (uint16 *)(bsearch(&uCode16, ligature16, FF_ligatureTOTAL16, \
				sizeof(uint16), compare_codepoints16));
	if ( p16 ) n = p16 - ligature16;
    } else if ( uCode >= FF_ligature32FIRST ) {
	p32 = (uint32 *)(bsearch(&uCode, ligature32, FF_ligatureTOTAL32, \
				sizeof(uint32), compare_codepoints32));
	if ( p32 ) n = p32 - ligature32 + FF_ligatureTOTAL16;
    }
    return( n );
}

int VulgFrac_find_N(uint32 uCode) {
    uint16 uCode16, *p16;
    uint32 *p32;
    int n=-1;

    if ( uCode < FF_vulgfrac16FIRST || uCode > FF_vulgfrac32LAST || \
	 ((uCode < FF_UTYPE_MAXC) && (isligorfrac(uCode)==0)) )
	return( -1 );
    if (uCode <= FF_vulgfrac16LAST) {
	uCode16 = uCode;
	p16 = (uint16 *)(bsearch(&uCode16, vulgfrac16, FF_vulgfracTOTAL16, \
				sizeof(uint16), compare_codepoints16));
	if ( p16 ) n = p16 - vulgfrac16;
    } else if ( uCode >= FF_vulgfrac32FIRST ) {
	p32 = (uint32 *)(bsearch(&uCode, vulgfrac32, FF_vulgfracTOTAL32, \
				sizeof(uint32), compare_codepoints32));
	if (p32) n = p32 - vulgfrac32 + FF_vulgfracTOTAL16;
    }
    return( n );
}

int Fraction_find_N(uint32 uCode) {
    uint16 uCode16, *p16;
    uint32 *p32;
    int n=-1;

    if ( uCode < FF_fractionTOTAL16 || uCode > FF_fraction32LAST || \
	 ((uCode < FF_UTYPE_MAXC) && (isligorfrac(uCode)==0)) )
	return( -1 );
    if ( uCode <= FF_fraction16LAST ) {
	uCode16 = uCode;
	p16 = (uint16 *)(bsearch(&uCode16, fraction16, FF_fractionTOTAL16, \
				sizeof(uint16), compare_codepoints16));
	if ( p16 ) n = p16 - fraction16;
    } else if ( uCode >= FF_fraction32FIRST ) {
	p32 = (uint32 *)(bsearch(&uCode, fraction32, FF_fractionTOTAL32, \
				sizeof(uint32), compare_codepoints32));
	if ( p32 ) n = p32 - fraction32 + FF_fractionTOTAL16;
    }
    return( n );
}

/* Boolean-style tests (found==0) to see if your codepoint value is listed */
/* unicode.org codepoints for ligatures, vulgar fractions, other fractions */

int is_LIGATURE(uint32 codepoint) {
    return( Ligature_find_N(codepoint)<0 );
}

int is_VULGAR_FRACTION(uint32 codepoint) {
    return( VulgFrac_find_N(codepoint)<0 );
}

int is_OTHER_FRACTION(uint32 codepoint) {
    return( Fraction_find_N(codepoint)<0 );
}

int is_FRACTION(uint32 codepoint) {
    return( VulgFrac_find_N(codepoint)<0 && Fraction_find_N(codepoint)<0 );
}

int is_LIGATURE_or_VULGAR_FRACTION(uint32 codepoint) {
    return( Ligature_find_N(codepoint)<0 && VulgFrac_find_N(codepoint)<0 );
}

int is_LIGATURE_or_OTHER_FRACTION(uint32 codepoint) {
    return( Ligature_find_N(codepoint)<0 && Fraction_find_N(codepoint)<0 );
}

int is_LIGATURE_or_FRACTION(uint32 codepoint) {
    return( Ligature_find_N(codepoint)<0 && VulgFrac_find_N(codepoint)<0 && Fraction_find_N(codepoint)<0 );
}

