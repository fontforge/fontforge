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
#include "chardata.h"
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

/* FontForge has a built-in internal table of ligatures. This function returns c */
/* for ligature[n], so for example if ligature[n] is 'ae' then c is 2 {a=0,e=1}, */
/* or if ligature[n] is 'ffi' then c is 3 {f=0,f=1,i=2}. Allowed range of n is 0 */
/* to n < LigatureCount() (the size of the internal table), otherwise return -1. */
int Ligature_alt_getC(int n) {
    int i,j;

    if ( n < 0 || n >= FF_ligatureTOTAL )
	return( -1 );
    if ( (i=ligatureAltI[n])&0x80 ) {
	j = i & 0x7f;
	for ( i=0; j; j=j>>1 )
	    if ( j&1 ) ++i;
	return( i );
    } else if ( i < FF_ligatureTIS ) {
	return( ligatureAltIs[i+1] - ligatureAltIs[i] );
    } else {
	i -= FF_ligatureTIS;
	return( ligatureAltIl[i+1] - ligatureAltIl[i] );
    }
}

/* This function returns the unicode value for ligature[n][a]. Errors return -1. */
/* allowed ranges: {0 <= n < LigatureCount()} & {0 <= a < Ligature_alt_getC(n)}. */
/* For example, if ligature[n]='ae', then LigatureCount(n)=2 and the results are */
/* alternate values of 'a'=Ligature_alt_getV(n,0) and 'e'=Ligature_alt_getV(n,1) */
int32 Ligature_alt_getV(int n,int a) {
    int i,j;
    const unichar_t *upt;
    int32 u;

    if ( n < 0 || n >= FF_ligatureTOTAL || a<0 || a>=Ligature_alt_getC(n) )
	return( -1 );
    if ( (i=ligatureAltI[n])&0x80 ) {
	j = i & 0x7f;
	for ( i=0; a; ++i )
	    if ( j&(1<<i) ) --a;
	u = Ligature_get_U(n);
	upt = unicode_alternates[u>>8][u&0xff];
	return( (int32)(upt[i]) );
    } else if ( i < FF_ligatureTIS ) {
	i = ligatureAltIs[i];
	u = ligatureAlt16[i+a];
	return( u );
    } else {
	i -= FF_ligatureTIS;
	i = ligatureAltIl[i];
	u = ligatureAlt32[i+a];
	return( u );
    }
}

/* This returns similar results like Ligature_alt_getC(n) but uses unicode input */
/* uCode instead of ligature[n]. This isn't as speedy as c=Ligature_alt_getC(n). */
int LigatureU_alt_getC(uint32 uCode) {
    return( Ligature_alt_getC(Ligature_find_N(uCode)) );
}

int32 LigatureU_alt_getV(uint32 uCode,int a) {
    int32 n;

    if ( (n=Ligature_find_N(uCode))<0 )
	return( -1 );
    return( Ligature_alt_getV(n,a) );
}

int VulgFrac_alt_getC(int n) {
    int i,j;

    if ( n < 0 || n >= FF_vulgfracTOTAL )
	return( -1 );
    if ( (i=vulgfracAltI[n])&0x80 ) {
	j = i & 0x7f;
	for ( i=0; j; j=j>>1 )
	    if ( j&1 ) ++i;
	return( i );
    } else if ( i < FF_vulgfracTIS ) {
	return( vulgfracAltIs[i+1] - vulgfracAltIs[i] );
    } else {
	i -= FF_vulgfracTIS;
	return( vulgfracAltIl[i+1] - vulgfracAltIl[i] );
    }
}

int32 VulgFrac_alt_getV(int n,int a) {
    int i,j;
    const unichar_t *upt;
    int32 u;

    if ( n < 0 || n >= FF_vulgfracTOTAL || a<0 || a>=VulgFrac_alt_getC(n) )
	return( -1 );
    if ( (i=vulgfracAltI[n])&0x80 ) {
	j = i & 0x7f;
	for ( i=0; a; ++i )
	    if ( j&(1<<i) ) --a;
	u = VulgFrac_get_U(n);
	upt = unicode_alternates[u>>8][u&0xff];
	return( (int32)(upt[i]) );
    } else if ( i < FF_vulgfracTIS ) {
	i = vulgfracAltIs[i];
	u = vulgfracAlt16[i+a];
	return( u );
    } else {
	i -= FF_vulgfracTIS;
	i = vulgfracAltIl[i];
	u = vulgfracAlt32[i+a];
	return( u );
    }
}

int VulgFracU_alt_getC(uint32 uCode) {
    return( VulgFrac_alt_getC(VulgFrac_find_N(uCode)) );
}

int32 VulgFracU_alt_getV(uint32 uCode,int a) {
    int32 n;

    if ( (n=VulgFrac_find_N(uCode))< 0 )
	return( -1 );
    return( VulgFrac_alt_getV(n,a) );
}

int Fraction_alt_getC(int n) {
    int i,j;

    if ( n < 0 || n >= FF_fractionTOTAL )
	return( -1 );
    if ( (i=fractionAltI[n])&0x80 ) {
	j = i & 0x7f;
	for ( i=0; j; j=j>>1 )
	    if ( j&1 ) ++i;
	return( i );
    } else if ( i < FF_fractionTIS ) {
	return( fractionAltIs[i+1] - fractionAltIs[i] );
    } else {
	i -= FF_fractionTIS;
	return( fractionAltIl[i+1] - fractionAltIl[i] );
    }
}

int32 Fraction_alt_getV(int n,int a) {
    int i,j;
    const unichar_t *upt;
    int32 u;

    if ( n < 0 || n >= FF_fractionTOTAL || a<0 || a>=Fraction_alt_getC(n) )
	return( -1 );
    if ( (i=fractionAltI[n])&0x80 ) {
	j = i & 0x7f;
	for ( i=0; a; ++i )
	    if ( j&(1<<i) ) --a;
	u = Fraction_get_U(n);
	upt = unicode_alternates[u>>8][u&0xff];
	return( (int32)(upt[i]) );
    } else if ( i < FF_fractionTIS ) {
	i = fractionAltIs[i];
	u = fractionAlt16[i+a];
	return( u );
    } else {
	i -= FF_fractionTIS;
	i = fractionAltIl[i];
	u = fractionAlt16[i+a];
	return( u );
    }
}

int FractionU_alt_getC(uint32 uCode) {
    return( Fraction_alt_getC(Fraction_find_N(uCode)) );
}

int32 FractionU_alt_getV(uint32 uCode,int a) {
    int32 n;

    if ( (n=Fraction_find_N(uCode))< 0 )
	return( -1 );
    return( Fraction_alt_getV(n,a) );
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

