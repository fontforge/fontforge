/*
Copyright: 2012 Barry Schwartz
Copyright: 2016 Joe Da Silva
License: BSD-3-clause
Contributions:
*/

#include "is_Ligature_data.h"

#include <basics.h> /* for non-standard uint{16,32} typedefs */

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

#define ELEMENTS_IN_ARRAY(x) (sizeof(x) / sizeof(x[0]))

int LigatureCount(void) {
	return ELEMENTS_IN_ARRAY(ligature16) + ELEMENTS_IN_ARRAY(ligature32);
}

int VulgarFractionCount(void) {
	return ELEMENTS_IN_ARRAY(vulgfrac16) + ELEMENTS_IN_ARRAY(vulgfrac32);
}

int OtherFractionCount(void) {
	return ELEMENTS_IN_ARRAY(fraction16) + ELEMENTS_IN_ARRAY(fraction32);
}

int FractionCount(void) {
	return VulgarFractionCount() + OtherFractionCount();
}

int32 Ligature_get_U(int n) {
	size_t num_ligature = ELEMENTS_IN_ARRAY(ligature16) + ELEMENTS_IN_ARRAY(ligature32);
	size_t num_ligature16 = ELEMENTS_IN_ARRAY(ligature16);

	if ((n < 0) || (n >= num_ligature))
		return -1;

	if (n < num_ligature16)
		return (int32)(ligature16[n]);
	else
		return (int32)(ligature32[n-num_ligature16]);
}

int32 VulgFrac_get_U(int n) {
	size_t num_vulgfrac = ELEMENTS_IN_ARRAY(vulgfrac16) + ELEMENTS_IN_ARRAY(vulgfrac32);
	size_t num_vulgfrac16 = ELEMENTS_IN_ARRAY(vulgfrac16);

	if ((n < 0) || (n >= num_vulgfrac))
		return -1;

	if (n < num_vulgfrac16)
		return (int32)(vulgfrac16[n]);
	else
		return (int32)(vulgfrac32[n-num_vulgfrac16]);
}

int32 Fraction_get_U(int n) {
	size_t num_fraction = ELEMENTS_IN_ARRAY(fraction16) + ELEMENTS_IN_ARRAY(fraction32);
	size_t num_fraction16 = ELEMENTS_IN_ARRAY(fraction16);

	if ((n < 0) || (n >= num_fraction))
		return -1;

	if (n < num_fraction16)
		return (int32)(fraction16[n]);
	else
		return (int32)(fraction32[n-num_fraction16]);
}

int Ligature_find_N(uint32 uCode) {
	uint16 uCode16;
	int n=-1;

	uint32 ligature16_first = ligature16[0];
	uint32 ligature16_last = ligature16[ELEMENTS_IN_ARRAY(ligature16)-1];

	if ((uCode < ligature16_first) || (uCode > ligature16_last) ||
	    ((uCode < FF_UNICODE_COMPACT_TABLE_SIZE_MAX) && (isligorfrac(uCode)==0)))
		return -1;

	if (uCode < ligature16_last) {
		uCode16 = uCode;
		uint16* p16 = (uint16 *)(bsearch(&uCode16,
		                                 ligature16,
		                                 ELEMENTS_IN_ARRAY(ligature16),
		                                 sizeof(uint16),
		                                 compare_codepoints16));
		if (p16)
			n = p16 - ligature16;
	} else {
		uint32* p32 = (uint32 *)(bsearch(&uCode,
		                                 ligature32,
		                                 ELEMENTS_IN_ARRAY(ligature32),
		                                 sizeof(uint32),
		                                 compare_codepoints32));
		if (p32)
			n = p32 - ligature32 + ELEMENTS_IN_ARRAY(ligature32);
	}

	return n;
}

int VulgFrac_find_N(uint32 uCode) {
	uint16 uCode16;
	int n=-1;

	uint32 vulgfrac16_first = vulgfrac16[0];
	uint32 vulgfrac16_last = vulgfrac16[ELEMENTS_IN_ARRAY(vulgfrac16)-1];

	if ((uCode < vulgfrac16_first) || (uCode > vulgfrac16_last) ||
	    ((uCode < FF_UNICODE_COMPACT_TABLE_SIZE_MAX) && (isligorfrac(uCode)==0)))
		return -1;

	if (uCode < vulgfrac16_last) {
		uCode16 = uCode;
		uint16* p16 = (uint16 *)(bsearch(&uCode16,
		                                 vulgfrac16,
		                                 ELEMENTS_IN_ARRAY(vulgfrac16),
		                                 sizeof(uint16),
		                                 compare_codepoints16));
		if (p16)
			n = p16 - vulgfrac16;
	} else {
		uint32* p32 = (uint32 *)(bsearch(&uCode,
		                                 vulgfrac32,
		                                 ELEMENTS_IN_ARRAY(vulgfrac32),
		                                 sizeof(uint32),
		                                 compare_codepoints32));
		if (p32)
			n = p32 - vulgfrac32 + ELEMENTS_IN_ARRAY(vulgfrac32);
	}

	return n;
}

int Fraction_find_N(uint32 uCode) {
	uint16 uCode16;
	int n=-1;

	uint32 fraction16_first = fraction16[0];
	uint32 fraction16_last = fraction16[ELEMENTS_IN_ARRAY(fraction16)-1];

	if ((uCode < fraction16_first) || (uCode > fraction16_last) ||
	    ((uCode < FF_UNICODE_COMPACT_TABLE_SIZE_MAX) && (isligorfrac(uCode)==0)))
		return -1;

	if (uCode < fraction16_last) {
		uCode16 = uCode;
		uint16* p16 = (uint16 *)(bsearch(&uCode16,
		                                 fraction16,
		                                 ELEMENTS_IN_ARRAY(fraction16),
		                                 sizeof(uint16),
		                                 compare_codepoints16));
		if (p16)
			n = p16 - fraction16;
	} else {
		uint32* p32 = (uint32 *)(bsearch(&uCode,
		                                 fraction32,
		                                 ELEMENTS_IN_ARRAY(fraction32),
		                                 sizeof(uint32),
		                                 compare_codepoints32));
		if (p32)
			n = p32 - fraction32 + ELEMENTS_IN_ARRAY(fraction32);
	}

	return n;
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

