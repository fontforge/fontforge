/* Copyright (C) 2000-2004 by George Williams */
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
#include "pfaedit.h"
#include <stdio.h>
#include <math.h>
#include "splinefont.h"
#include "views.h"
#include <utype.h>
#include <chardata.h>
#include "edgelist.h"

/* to create a type 1 font we must come up with the following entries for the
  private dictionary:
    BlueValues	-- an array of 2n entries where Blue[2i]<Blue[2i+1] max n=7, Blue[i]>0
    OtherBlues	-- (optional) OtherBlue[i]<0
    	(blue zones should be at least 3 units appart)
    StdHW	-- (o) array with one entry, standard hstem height
    StdVW	-- (o) ditto vstem width
    StemSnapH	-- (o) up to 12 numbers containing various hstem heights (includes StdHW), small widths first
    StemSnapV	-- (o) ditto, vstem
This file has routines to figure out at least some of these

Also other routines to guess at good per-character hints
*/

static void AddBlue(real val, real array[5], int force) {
    val = rint(val);
    if ( !force && (val<array[0]-array[1] || val >array[0]+array[1] ))
return;		/* Outside of one sd */
    if ( array[3]==0 && array[4]==0 )
	array[3] = array[4] = val;
    else if ( val>array[4] )
	array[4] = val;
    else if ( val<array[3] )
	array[3] = val;
}

static void MergeZones(real zone1[5], real zone2[5]) {
    if ( zone1[2]!=0 && zone2[2]!=0 &&
	    ((zone1[4]+3>zone2[3] && zone1[3]<=zone2[3]) ||
	     (zone2[4]+3>zone1[3] && zone2[3]<=zone1[3]) )) {
	if (( zone2[0]<zone1[0]-zone1[1] || zone2[0] >zone1[0]+zone1[1] ) &&
		( zone1[0]<zone2[0]-zone2[1] || zone1[0] >zone2[0]+zone2[1] ))
	    /* the means of the zones are too far appart, don't merge em */;
	else {
	    if ( zone1[0]<zone2[0] ) zone2[0] = zone1[0];
	    if ( zone1[1]>zone2[1] ) zone2[1] = zone1[1];
	}
	zone1[2] = 0;
    }
}

/* I can deal with latin, greek and cyrillic because the they all come from */
/*  the same set of letter shapes and have all evolved together and have */
/*  various common features (ascenders, descenders, lower case, etc.). Other */
/*  scripts don't fit */
void FindBlues( SplineFont *sf, real blues[14], real otherblues[10]) {
    real caph[5], xh[5], ascenth[5], digith[5], descenth[5], base[5];
    real otherdigits[5];
    int i, any = 0, j, k;
    DBounds b;

    /* Go through once to get some idea of the average value so we can weed */
    /*  out undesireables */
    caph[0] = caph[1] = caph[2] = xh[0] = xh[1] = xh[2] = 0;
    ascenth[0] = ascenth[1] = ascenth[2] = digith[0] = digith[1] = digith[2] = 0;
    descenth[0] = descenth[1] = descenth[2] = base[0] = base[1] = base[2] = 0;
    otherdigits[0] = otherdigits[1] = otherdigits[2] = 0;
    for ( i=0; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]!=NULL && sf->chars[i]->layers[ly_fore].splines!=NULL ) {
	    int enc = sf->chars[i]->unicodeenc;
	    const unichar_t *upt;
	    if ( isalnum(enc) &&
		    ((enc>=32 && enc<128 ) || enc == 0xfe || enc==0xf0 || enc==0xdf ||
		      enc==0x131 ||
		     (enc>=0x391 && enc<=0x3f3 ) ||
		     (enc>=0x400 && enc<=0x4e9 ) )) {
		/* no accented characters (or ligatures) */
		if ( unicode_alternates[enc>>8]!=NULL &&
			(upt =unicode_alternates[enc>>8][enc&0xff])!=NULL &&
			upt[1]!='\0' )
    continue;
		any = true;
		SplineCharFindBounds(sf->chars[i],&b);
		if ( b.miny==0 && b.maxy==0 )
    continue;
		if ( enc=='g' || enc=='j' || enc=='p' || enc=='q' || enc=='y' ||
			enc==0xfe || 
			enc==0x3c1 /* rho */ ||
			enc==0x3c6 /* phi */ || 
			enc==0x3c7 /* chi */ || 
			enc==0x3c8 /* psi */ ||
			enc==0x440 /* cyr er */ || 
			enc==0x443 /* cyr u */ || 
			enc==0x444 /* cyr ef */) {
		    descenth[0] += b.miny;
		    descenth[1] += b.miny*b.miny;
		    ++descenth[2];
		} else if ( enc=='x' || enc=='r' || enc=='o' || enc=='e' || 
		               enc=='s' || enc=='c' || enc=='h' || enc=='k' || 
			       enc=='l' || enc=='m' || enc=='n' || 
			       enc==0x3b5 /* epsilon */ || 
			       enc==0x3b9 /* iota */ ||
			       enc==0x3ba /* kappa */ ||
			       enc==0x3bf /* omicron */ ||
			       enc==0x3c3 /* sigma */ ||
			       enc==0x3c5 /* upsilon */ ||
			       enc==0x430 /* cyr a */ ||
			       enc==0x432 /* cyr ve */ ||
			       enc==0x433 /* cyr ge */ ||
			       enc==0x435 /* cyr e */ ||
			       enc==0x436 /* cyr zhe */ ||
			       enc==0x438 /* cyr i */ ||
			       enc==0x43a /* cyr ka */ ||
			       enc==0x43d /* cyr en */ ||
			       enc==0x43e /* cyr o */ ||
			       enc==0x441 /* cyr es */ ||
			       enc==0x445 /* cyr ha */ ||
			       enc==0x447 /* cyr che */ ||
			       enc==0x448 /* cyr sha */ ||
			       enc==0x44f /* cyr ya */ ){
		    base[0] += b.miny;
		    base[1] += b.miny*b.miny;
		    ++base[2];
		}
		/* careful of lowercase digits, 6 and 8 should be ascenders */
		if ( enc=='6' || enc=='8' ) {
		    digith[0] += b.maxy;
		    digith[1] += b.maxy*b.maxy;
		    ++digith[2];
		} else if ( isdigit(enc) ) {
		    otherdigits[0] += b.maxy;
		    otherdigits[1] += b.maxy*b.maxy;
		    ++otherdigits[2];
		} else if ( isupper(enc)) {
		    caph[0] += b.maxy;
		    caph[1] += b.maxy*b.maxy;
		    ++caph[2];
		} else if ( enc=='b' || enc=='d' || enc=='f' || enc=='h' || enc=='k' ||
			enc == 'l' || enc==0xf0 || enc==0xfe || enc == 0xdf ||
			enc == 0x3b2 || enc==0x3b6 || enc==0x3b8 || enc==0x3bb ||
			enc == 0x3be ||
			enc == 0x431 /* cyr be */ /* || enc == 0x444 - ef may have varible height */) {
		    ascenth[0] += b.maxy;
		    ascenth[1] += b.maxy*b.maxy;
		    ++ascenth[2];
		} else if ( enc=='c' || enc=='e' || enc=='o' || enc=='s' || enc=='u' || 
		            enc=='u' || enc=='v' || enc=='w' || enc=='x' || enc=='y' || 
			    enc=='z' || 
			    enc==0x3b5 /* epsilon */ ||
			    enc==0x3b9 /* iota */ ||
			    enc==0x3ba /* kappa */ ||
			    enc==0x3bc /* mu */ ||
			    enc==0x3bd /* nu */ ||
			    enc==0x3bf /* omicron */ ||
			    enc==0x3c0 /* pi */ ||
			    enc==0x3c1 /* rho */ ||
			    enc==0x3c5 /* upsilon */ ||
			    enc==0x433 /* cyr ge */ ||
			    enc==0x435 /* cyr e */ ||
			    enc==0x436 /* cyr zhe */ ||
			    enc==0x438 /* cyr i */ ||
			    enc==0x43b /* cyr el */ ||
			    enc==0x43d /* cyr en */ ||
			    enc==0x43e /* cyr o */ ||
			    enc==0x43f /* cyr pe */ ||
			    enc==0x440 /* cyr er */ ||
			    enc==0x441 /* cyr es */ ||
			    enc==0x442 /* cyr te */ ||
			    enc==0x443 /* cyr u */ ||
			    enc==0x445 /* cyr ha */ ||
			    enc==0x446 /* cyr tse */ ||
			    enc==0x447 /* cyr che */ ||
			    enc==0x448 /* cyr sha */ ||
			    enc==0x449 /* cyr shcha */ ||
			    enc==0x44a /* cyr hard sign */ ||
			    enc==0x44b /* cyr yery */ ||
			    enc==0x44c /* cyr soft sign */ ||
			    enc==0x44d /* cyr reversed e */ ||
			    enc==0x44f /* cyr ya */ ) {
		    xh[0] += b.maxy;
		    xh[1] += b.maxy*b.maxy;
		    ++xh[2];
		}
	    }
	}
	if ( !GProgressNext())
    break;
    }
    if ( otherdigits[2]>0 && digith[2]>0 ) {
	if ( otherdigits[0]/otherdigits[2] >= .95*digith[0]/digith[2] ) {
	    /* all digits are about the same height, not lowercase */
	    digith[0] += otherdigits[0];
	    digith[1] += otherdigits[1];
	    digith[2] += otherdigits[2];
	}
    }

    if ( xh[2]>1 ) {
	xh[1] = sqrt((xh[1]-xh[0]*xh[0]/xh[2])/(xh[2]-1));
	xh[0] /= xh[2];
    }
    if ( ascenth[2]>1 ) {
	ascenth[1] = sqrt((ascenth[1]-ascenth[0]*ascenth[0]/ascenth[2])/(ascenth[2]-1));
	ascenth[0] /= ascenth[2];
    }
    if ( caph[2]>1 ) {
	caph[1] = sqrt((caph[1]-caph[0]*caph[0]/caph[2])/(caph[2]-1));
	caph[0] /= caph[2];
    }
    if ( digith[2]>1 ) {
	digith[1] = sqrt((digith[1]-digith[0]*digith[0]/digith[2])/(digith[2]-1));
	digith[0] /= digith[2];
    }
    if ( base[2]>1 ) {
	base[1] = sqrt((base[1]-base[0]*base[0]/base[2])/(base[2]-1));
	base[0] /= base[2];
    }
    if ( descenth[2]>1 ) {
	descenth[1] = sqrt((descenth[1]-descenth[0]*descenth[0]/descenth[2])/(descenth[2]-1));
	descenth[0] /= descenth[2];
    }

    /* we'll accept values between +/- 1sd of the mean */
    /* array[0] == mean, array[1] == sd, array[2] == cnt, array[3]=min, array[4]==max */
    caph[3] = caph[4] = 0;
    xh[3] = xh[4] = 0;
    ascenth[3] = ascenth[4] = 0;
    digith[3] = digith[4] = 0;
    descenth[3] = descenth[4] = 0;
    base[3] = base[4] = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	int enc = sf->chars[i]->unicodeenc;
	const unichar_t *upt;
	if ( isalnum(enc) &&
		((enc>=32 && enc<128 ) || enc == 0xfe || enc==0xf0 || enc==0xdf ||
		 (enc>=0x391 && enc<=0x3f3 ) ||
		 (enc>=0x400 && enc<=0x4e9 ) )) {
	    /* no accented characters (or ligatures) */
	    if ( unicode_alternates[enc>>8]!=NULL &&
		    (upt =unicode_alternates[enc>>8][enc&0xff])!=NULL &&
		    upt[1]!='\0' )
    continue;
	    any = true;
	    SplineCharFindBounds(sf->chars[i],&b);
	    if ( b.miny==0 && b.maxy==0 )
    continue;
	    if ( enc=='g' || enc=='j' || enc=='p' || enc=='q' || enc=='y' ||
		    enc==0xfe || enc == 0x3c6 || enc==0x3c8 ||
		    enc==0x440 || enc==0x443 || enc==0x444) {
		AddBlue(b.miny,descenth,false);
	    } else {
		/* O and o get forced into the baseline blue value even if they*/
		/*  are beyond 1 sd */
		AddBlue(b.miny,base,enc=='O' || enc=='o');
	    }
	    if ( isdigit(enc)) {
		AddBlue(b.maxy,digith,false);
	    } else if ( isupper(enc)) {
		AddBlue(b.maxy,caph,enc=='O');
	    } else if ( enc=='b' || enc=='d' || enc=='f' || enc=='h' || enc=='k' ||
		    enc == 'l' || enc=='t' || enc==0xf0 || enc==0xfe || enc == 0xdf ||
		    enc == 0x3b2 || enc==0x3b6 || enc==0x3b8 || enc==0x3bb ||
		    enc == 0x3be ||
		    enc == 0x431 ) {
		AddBlue(b.maxy,ascenth,false);
	    } else if ( enc=='c' || enc=='e' || enc=='o' || enc=='s' || enc=='u' || 
			enc=='u' || enc=='v' || enc=='w' || enc=='x' || enc=='y' || 
			enc=='z' || 
			enc==0x3b5 /* epsilon */ ||
			enc==0x3b9 /* iota */ ||
			enc==0x3ba /* kappa */ ||
			enc==0x3bc /* mu */ ||
			enc==0x3bd /* nu */ ||
			enc==0x3bf /* omicron */ ||
			enc==0x3c0 /* pi */ ||
			enc==0x3c1 /* rho */ ||
			enc==0x3c5 /* upsilon */ ||
			enc==0x433 /* cyr ge */ ||
			enc==0x435 /* cyr e */ ||
			enc==0x436 /* cyr zhe */ ||
			enc==0x438 /* cyr i */ ||
			enc==0x43b /* cyr el */ ||
			enc==0x43d /* cyr en */ ||
			enc==0x43e /* cyr o */ ||
			enc==0x43f /* cyr pe */ ||
			enc==0x440 /* cyr er */ ||
			enc==0x441 /* cyr es */ ||
			enc==0x442 /* cyr te */ ||
			enc==0x443 /* cyr u */ ||
			enc==0x445 /* cyr ha */ ||
			enc==0x446 /* cyr tse */ ||
			enc==0x447 /* cyr che */ ||
			enc==0x448 /* cyr sha */ ||
			enc==0x449 /* cyr shcha */ ||
			enc==0x44a /* cyr hard sign */ ||
			enc==0x44b /* cyr yery */ ||
			enc==0x44c /* cyr soft sign */ ||
			enc==0x44d /* cyr reversed e */ ||
			enc==0x44f /* cyr ya */ ) {
		AddBlue(b.maxy,xh,enc=='o' || enc=='x');
	    }
	}
    }

    /* the descent blue zone merges into the base zone */
    MergeZones(descenth,base);
    MergeZones(xh,base);
    MergeZones(ascenth,caph);
    MergeZones(digith,caph);
    MergeZones(xh,caph);
    MergeZones(ascenth,digith);
    MergeZones(xh,digith);

    if ( otherblues!=NULL )
	for ( i=0; i<10; ++i )
	    otherblues[i] = 0;
    for ( i=0; i<14; ++i )
	blues[i] = 0;

    if ( otherblues!=NULL && descenth[2]!=0 ) {
	otherblues[0] = descenth[3];
	otherblues[1] = descenth[4];
    }
    i = 0;
    if ( base[2]==0 && (xh[2]!=0 || ascenth[2]!=0 || caph[2]!=0 || digith[2]!=0 )) {
	/* base line blue value must be present if any other value is */
	/*  make one up if we don't have one */
	blues[0] = -20;
	blues[1] = 0;
	i = 2;
    } else if ( base[2]!=0 ) {
	blues[0] = base[3];
	blues[1] = base[4];
	i = 2;
    }
    if ( xh[2]!=0 ) {
	blues[i++] = xh[3];
	blues[i++] = xh[4];
    }
    if ( caph[2]!=0 ) {
	blues[i++] = caph[3];
	blues[i++] = caph[4];
    }
    if ( digith[2]!=0 ) {
	blues[i++] = digith[3];
	blues[i++] = digith[4];
    }
    if ( ascenth[2]!=0 ) {
	blues[i++] = ascenth[3];
	blues[i++] = ascenth[4];
    }

    for ( j=0; j<i; j+=2 ) for ( k=j+2; k<i; k+=2 ) {
	if ( blues[j]>blues[k] ) {
	    real temp = blues[j];
	    blues[j]=blues[k];
	    blues[k] = temp;
	    temp = blues[j+1];
	    blues[j+1] = blues[k+1];
	    blues[k+1] = temp;
	}
    }
}

static int PVAddBlues(BlueData *bd,int bcnt,char *pt) {
    char *end;
    real val1, val2;
    int i,j;

    if ( pt==NULL )
return( bcnt );

    while ( isspace(*pt) || *pt=='[' ) ++pt;
    while ( *pt!=']' && *pt!='\0' ) {
	val1 = strtod(pt,&end);
	if ( *end=='\0' || end==pt )
    break;
	for ( pt=end; isspace(*pt) ; ++pt );
	val2 = strtod(pt,&end);
	if ( end==pt )
    break;
	if ( bcnt==0 || val1>bd->blues[bcnt-1][0] )
	    i = bcnt;
	else {
	    for ( i=0; i<bcnt && val1>bd->blues[i][0]; ++i );
	    for ( j=bcnt; j>i; --j ) {
		bd->blues[j][0] = bd->blues[j-1][0];
		bd->blues[j][1] = bd->blues[j-1][1];
	    }
	}
	bd->blues[i][0] = val1;
	bd->blues[i][1] = val2;
	++bcnt;
	if ( bcnt>=sizeof(bd->blues)/sizeof(bd->blues[0]))
    break;
	for ( pt=end; isspace(*pt) ; ++pt );
    }
return( bcnt );
}

/* Quick and dirty (and sometimes wrong) approach to figure out the common */
/* alignment zones in latin (greek, cyrillic) alphabets */
void QuickBlues(SplineFont *_sf, BlueData *bd) {
    real xheight = -1e10, caph = -1e10, ascent = -1e10, descent = 1e10, max, min;
    real xheighttop = -1e10, caphtop = -1e10;
    real numh = -1e10, numhtop = -1e10;
    real base = -1e10, basebelow = 1e10;
    SplineFont *sf;
    SplinePoint *sp;
    SplineSet *spl;
    int i,j, bcnt;
    SplineChar *t;
    char *pt;

    /* Get the alignment zones we care most about */

    /* be careful of cid fonts */
    if ( _sf->cidmaster!=NULL )
	_sf = _sf->cidmaster;

    j=0;
    do {
	sf = ( _sf->subfontcnt==0 )? _sf : _sf->subfonts[j];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    int enc = sf->chars[i]->unicodeenc;
	    if ( enc=='I' || enc=='O' || enc=='x' || enc=='o' || enc=='p' || enc=='l' ||
/* Jean-Christophe Dubacq points out that x-height should be calculated from */
/*  various characters and not just x and o. Italic "x"s often have strange */
/*  shapes */
		    enc=='A' || enc==0x391 || enc==0x410 ||
		    enc=='V' ||
		    enc=='u' || enc=='v' || enc=='w' || enc=='y' || enc=='z' ||
		    enc=='7' || enc=='8' ||	/* numbers with ascenders */
		    enc==0x399 || enc==0x39f || enc==0x3ba || enc==0x3bf || enc==0x3c1 || enc==0x3be || enc==0x3c7 ||
		    enc==0x41f || enc==0x41e || enc==0x43e || enc==0x43f || enc==0x440 || enc==0x452 || enc==0x445 ) {
		t = sf->chars[i];
		while ( t->layers[ly_fore].splines==NULL && t->layers[ly_fore].refs!=NULL )
		    t = t->layers[ly_fore].refs->sc;
		max = -1e10; min = 1e10;
		if ( t->layers[ly_fore].splines!=NULL ) {
		    for ( spl=t->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
			for ( sp=spl->first; ; ) {
			    if ( sp->me.y > max ) max = sp->me.y;
			    if ( sp->me.y < min ) min = sp->me.y;
			    if ( sp->next==NULL )
			break;
			    sp = sp->next->to;
			    if ( sp==spl->first )
			break;
			}
		    }
		    if ( enc>0x400 ) {
			/* Only use ascent and descent here if we don't have anything better */
			if ( enc==0x41f ) { caph = max; base = min; }
			else if ( enc==0x41e ) { if ( max>caphtop ) caphtop = max; basebelow = min; }
			else if ( enc==0x410 ) { if ( max>caphtop ) caphtop = max; }
			else if ( enc==0x43f && xheight<0 ) xheight = max;
			else if ( enc==0x445 && xheight<0 ) xheight = max;
			else if ( enc==0x43e ) xheighttop = max;
			else if ( enc==0x452 && ascent<0 ) ascent = max;
			else if ( enc==0x440 && descent>0 ) descent = min;
		    } else if ( enc>0x300 ) {
			if ( enc==0x399 ) { caph = max; base = min; }
			else if ( enc==0x391 ) { if ( max>caphtop ) caphtop = max; }
			else if ( enc==0x39f ) { if ( max>caphtop ) caphtop = max; basebelow = min; }
			else if ( enc==0x3ba && xheight<0 ) xheight = max;
			else if ( enc==0x3c7 && xheight<0 ) xheight = max;
			else if ( enc==0x3bf ) xheighttop = max;
			else if ( enc==0x3be && ascent<0 ) ascent = max;
			else if ( enc==0x3c1 && descent>0 ) descent = min;
		    } else {
			if ( enc=='I' ) { caph = max; base = min; }
			else if ( enc=='O' ) { if ( max>caphtop ) caphtop = max; if ( basebelow<min ) basebelow = min; }
			else if ( enc=='V' ) { if ( basebelow<min ) basebelow = min; }
			else if ( enc=='A' ) { if ( max>caphtop ) caphtop = max; }
			else if ( enc=='7' ) numh = max;
			else if ( enc=='0' ) numhtop = max;
			else if ( enc=='x' || enc=='o' || enc=='u' || enc=='v' ||
				enc =='w' || enc=='y' || enc=='z' ) {
			    if ( xheighttop==-1e10 ) xheighttop = max;
			    if ( xheight==-1e10 ) xheight = max;
			    if ( max > xheighttop ) xheighttop = max;
			    else if ( max<xheight && max>20 ) xheight = max;
			    if ( enc=='y' && descent==1e10 ) descent = min;
			} else if ( enc=='l' ) ascent = max;
			else descent = min;
		    }
		}
	    }
	}
	++j;
    } while ( j<_sf->subfontcnt );

    if ( basebelow==1e10 ) basebelow=-1e10;

    if ( caphtop<caph ) caphtop = caph; else if ( caph==-1e10 ) caph=caphtop;
    if ( basebelow>base ) basebelow = base; else if ( base==-1e10 ) base=basebelow;
    if ( base==-1e10 ) { base=basebelow = 0; }
    if ( xheighttop<xheight ) xheighttop = xheight; else if ( xheight==-1e10 ) xheight=xheighttop;
    bd->xheight = xheight; bd->xheighttop = xheighttop;
    bd->caph = caph; bd->caphtop = caphtop;
    bd->numh = numh; bd->numhtop = numhtop;
    bd->ascent = ascent; bd->descent = descent;
    bd->base = base; bd->basebelow = basebelow;

    bcnt = 0;
    if ( (pt=PSDictHasEntry(sf->private,"BlueValues"))!=NULL )
	bcnt = PVAddBlues(bd,bcnt,pt);
    if ( (pt=PSDictHasEntry(sf->private,"OtherBlues"))!=NULL )
	bcnt = PVAddBlues(bd,bcnt,pt);
    if ( bcnt==0 ) {
	if ( basebelow==-1e10 ) basebelow = base;
	if ( base==-1e10 ) base = basebelow;
	if ( xheight==-1e10 ) xheight = xheighttop;
	if ( xheighttop==-1e10 ) xheighttop = xheight;
	if ( caph==-1e10 ) caph = caphtop;
	if ( caphtop==-1e10 ) caphtop = caph;
	if ( numh==-1e10 ) numh = numhtop;
	if ( numhtop==-1e10 ) numhtop = numh;
	if ( numh!=-1e10 && (numhtop>caph-2 && numh<caphtop+2)) {
	    if ( numh<caph ) caph=numh;
	    if ( numhtop>caphtop ) caphtop = numhtop;
	    numh = numhtop = -1e10;
	}
	if ( ascent!=-1e10 && (ascent>caph-2 && ascent<caphtop+2)) {
	    if ( ascent<caph ) caph=ascent;
	    if ( ascent>caphtop ) caphtop = ascent;
	    ascent = -1e10;
	}
	if ( ascent!=-1e10 && (ascent>numh-2 && ascent<numhtop+2)) {
	    if ( ascent<numh ) numh=ascent;
	    if ( ascent>numhtop ) numhtop = ascent;
	    ascent = -1e10;
	    if ( numhtop>caph-2 && numh<caphtop+2 ) {
		if ( numh<caph ) caph=numh;
		if ( numhtop>caphtop ) caphtop = numhtop;
		numh = numhtop = -1e10;
	    }
	}

	if ( descent!=1e10 ) {
	    bd->blues[0][0] = bd->blues[0][1] = descent;
	    ++bcnt;
	}
	if ( basebelow!=-1e10 ) {
	    bd->blues[bcnt][0] = basebelow;
	    bd->blues[bcnt][1] = base;
	    ++bcnt;
	}
	if ( xheight!=-1e10 ) {
	    bd->blues[bcnt][0] = xheight;
	    bd->blues[bcnt][1] = xheighttop;
	    ++bcnt;
	}
	if ( numh!=-1e10 ) {
	    bd->blues[bcnt][0] = numh;
	    bd->blues[bcnt][1] = numhtop;
	    ++bcnt;
	}
	if ( caph!=-1e10 ) {
	    bd->blues[bcnt][0] = caph;
	    bd->blues[bcnt][1] = caphtop;
	    ++bcnt;
	}
	if ( ascent!=-1e10 ) {
	    bd->blues[bcnt][0] = bd->blues[bcnt][1] = ascent;
	    ++bcnt;
	}
    }
    bd->bluecnt = bcnt;
}

void ElFreeEI(EIList *el) {
    EI *e, *next;

    for ( e = el->edges; e!=NULL; e = next ) {
	next = e->next;
	free(e);
    }
}

static int EIAddEdge(Spline *spline, real tmin, real tmax, EIList *el) {
    EI *new = gcalloc(1,sizeof(EI));
    real min, max, temp;
    Spline1D *s;
    real dxdtmin, dxdtmax, dydtmin, dydtmax;

    new->spline = spline;
    new->tmin = tmin;
    new->tmax = tmax;

    s = &spline->splines[1];
    if (( dydtmin = (3*s->a*tmin + 2*s->b)*tmin + s->c )<0 ) dydtmin = -dydtmin;
    if (( dydtmax = (3*s->a*tmax + 2*s->b)*tmax + s->c )<0 ) dydtmax = -dydtmax;
    s = &spline->splines[0];
    if (( dxdtmin = (3*s->a*tmin + 2*s->b)*tmin + s->c )<0 ) dxdtmin = -dxdtmin;
    if (( dxdtmax = (3*s->a*tmax + 2*s->b)*tmax + s->c )<0 ) dxdtmax = -dxdtmax;
    
    /*s = &spline->splines[0];*/
    min = ((s->a * tmin + s->b)* tmin + s->c)* tmin + s->d;
    max = ((s->a * tmax + s->b)* tmax + s->c)* tmax + s->d;
    if ( tmax==1 ) max = spline->to->me.x;	/* beware rounding errors */
    if ( !el->leavetiny && floor(min)==floor(max) ) {	/* If it doesn't cross a pixel boundary then it might as well be vertical */
	if ( tmin==0 ) max = min;
	else if ( tmax==1 ) min = max;
	else max = min;
    }
    if ( min==max )
	new->vert = true;
    else if ( min<max )
	new->hup = true;
    else {
	temp = min; min = max; max=temp;
    }
    if ( !el->leavetiny && min+1>max ) new->almostvert = true;
    if ( 40*dxdtmin<dydtmin ) new->vertattmin = true;
    if ( 40*dxdtmax<dydtmax ) new->vertattmax = true;
    /*if ( new->vertattmin && new->vertattmax && s->a==0 && s->b==0 ) new->almostvert = true;*/
    new->coordmin[0] = min; new->coordmax[0] = max;
    if ( el->coordmin[0]>min )
	el->coordmin[0] = min;
    if ( el->coordmax[0]<max )
	el->coordmax[0] = max;

    s = &spline->splines[1];
    min = ((s->a * tmin + s->b)* tmin + s->c)* tmin + s->d;
    max = ((s->a * tmax + s->b)* tmax + s->c)* tmax + s->d;
    if ( tmax==1 ) max = spline->to->me.y;
    if ( !el->leavetiny && floor(min)==floor(max) ) {	/* If it doesn't cross a pixel boundary then it might as well be horizontal */
	if ( tmin==0 ) max = min;
	else if ( tmax==1 ) min = max;
	else max = min;
    }
    if ( min==max )
	new->hor = true;
    else if ( min<max )
	new->vup = true;
    else {
	temp = min; min = max; max=temp;
    }
    if ( !el->leavetiny && min+1>max ) new->almosthor = true;
    if ( 40*dydtmin<dxdtmin ) new->horattmin = true;
    if ( 40*dydtmax<dxdtmax ) new->horattmax = true;
    /*if ( new->horattmin && new->horattmax && s->a==0 && s->b==0 ) new->almosthor = true;*/
    new->coordmin[1] = min; new->coordmax[1] = max;
    if ( el->coordmin[1]>min )
	el->coordmin[1] = min;
    if ( el->coordmax[1]<max )
	el->coordmax[1] = max;

    if ( new->hor && new->vert ) {
	/* This spline is too small for us to notice */
	free(new);
return( false );
    } else {
	new->next = el->edges;
	el->edges = new;

	if ( el->splinelast!=NULL )
	    el->splinelast->splinenext = new;
	el->splinelast = new;
	if ( el->splinefirst==NULL )
	    el->splinefirst = new;

return( true );
    }
}

static void EIAddSpline(Spline *spline, EIList *el) {
    double ts[6], temp;
    int i, j, base, last;

    ts[0] = 0; ts[5] = 1.0;
    SplineFindExtrema(&spline->splines[0],&ts[1],&ts[2]);
    SplineFindExtrema(&spline->splines[1],&ts[3],&ts[4]);
    /* avoid teeny tiny segments, they just confuse us */
    SplineRemoveInflectionsTooClose(&spline->splines[0],&ts[1],&ts[2]);
    SplineRemoveInflectionsTooClose(&spline->splines[1],&ts[3],&ts[4]);
    for ( i=0; i<4; ++i ) for ( j=i+1; j<5; ++j ) {
	if ( ts[i]>ts[j] ) {
	    temp = ts[i];
	    ts[i] = ts[j];
	    ts[j] = temp;
	}
    }
    for ( base=0; ts[base]==-1; ++base);
    for ( i=5; i>base ; --i ) {
	if ( ts[i]==ts[i-1] ) {
	    for ( j=i-1; j>base; --j )
		ts[j] = ts[j-1];
	    ts[j] = -1;
	    ++base;
	}
    }
    last = base;
    for ( i=base; i<5 ; ++i )
	if ( EIAddEdge(spline,ts[last],ts[i+1],el) )
	    last = i+1;
}

void ELFindEdges(SplineChar *sc, EIList *el) {
    Spline *spline, *first;
    SplineSet *spl;

    el->sc = sc;
    if ( sc->layers[ly_fore].splines==NULL )
return;

    el->coordmin[0] = el->coordmax[0] = sc->layers[ly_fore].splines->first->me.x;
    el->coordmin[1] = el->coordmax[1] = sc->layers[ly_fore].splines->first->me.y;

    for ( spl = sc->layers[ly_fore].splines; spl!=NULL; spl = spl->next ) if ( spl->first->prev!=NULL && spl->first->prev->from!=spl->first ) {
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    EIAddSpline(spline,el);
	    if ( first==NULL ) first = spline;
	}
	if ( el->splinefirst!=NULL && spl->first->prev!=NULL )
	    el->splinelast->splinenext = el->splinefirst;
	el->splinelast = NULL; el->splinefirst = NULL;
    }
}

static int IsBiggerSlope(EI *test, EI *base, int major) {
    int other = !major;
    real tdo, tdm, bdo, bdm, t;

    if (( major && test->vup ) || (!major && test->hup))
	t = test->tmin;
    else
	t = test->tmax;
    tdm = (3*test->spline->splines[major].a*t + 2*test->spline->splines[major].b)*t + test->spline->splines[major].c;
    tdo = (3*test->spline->splines[other].a*t + 2*test->spline->splines[other].b)*t + test->spline->splines[other].c;

    if (( major && base->vup ) || (!major && base->hup))
	t = base->tmin;
    else
	t = base->tmax;
    bdm = (3*base->spline->splines[major].a*t + 2*base->spline->splines[major].b)*t + base->spline->splines[major].c;
    bdo = (3*base->spline->splines[other].a*t + 2*base->spline->splines[other].b)*t + base->spline->splines[other].c;

    if ( tdm==0 && bdm==0 )
return( tdo > bdo );
    if ( tdo==0 )
return( tdo>0 );
    else if ( bdo==0 )
return( bdo>0 );

return( tdo/tdm > bdo/bdm );
}

void ELOrder(EIList *el, int major ) {
    int other = !major;
    int pos;
    EI *ei, *prev, *test;
    BasePoint *me;

    el->low = floor(el->coordmin[major]); el->high = ceil(el->coordmax[major]);
    el->cnt = el->high-el->low+1;
    el->ordered = gcalloc(el->cnt,sizeof(EI *));
    el->ends = gcalloc(el->cnt,1);

    for ( ei = el->edges; ei!=NULL ; ei=ei->next ) {
	me = &ei->spline->from->me;
	pos = ceil(ei->coordmax[major])-el->low;
	el->ends[pos] = true;
	pos = floor(ei->coordmin[major])-el->low;
	ei->ocur = (ei->hup == ei->vup)?ei->coordmin[other]:ei->coordmax[other];
	ei->tcur = ((major && ei->vup) || (!major && ei->hup)) ?
			ei->tmin: ei->tmax;
	if ( major ) {
	    ei->up = ei->vup;
	    ei->hv = (ei->vert || ei->almostvert);
	    ei->hvbottom = ei->vup ? ei->vertattmin : ei->vertattmax;
	    ei->hvtop    =!ei->vup ? ei->vertattmin : ei->vertattmax;
	    if ( ei->hor || ei->almosthor)
    continue;
	} else {
	    ei->up = ei->hup;
	    ei->hv = (ei->hor || ei->almosthor);
	    ei->hvbottom = ei->hup ? ei->horattmin : ei->horattmax;
	    ei->hvtop    =!ei->hup ? ei->horattmin : ei->horattmax;
	    if ( ei->vert || ei->almostvert)
    continue;
	}
	if ( el->ordered[pos]==NULL || ei->ocur<el->ordered[pos]->ocur ) {
	    ei->ordered = el->ordered[pos];
	    el->ordered[pos] = ei;
	} else {
	    for ( prev=el->ordered[pos], test = prev->ordered; test!=NULL;
		    prev = test, test = test->ordered ) {
		if ( test->ocur>ei->ocur ||
			(test->ocur==ei->ocur && IsBiggerSlope(test,ei,major)))
	    break;
	    }
	    ei->ordered = test;
	    prev->ordered = ei;
	}
    }
}

/* If the stem is vertical here, return an approximate height over which it is */
/* if major is 1 then we check for vertical lines, if 0 then horizontal */
static real IsEdgeHorVertHere(EI *e,int major, int i, real *up, real *down) {
    /* It's vertical if dx/dt == 0 or if (dy/dt)/(dx/dt) is infinite */
    Spline *spline = e->spline;
    Spline1D *msp = &spline->splines[major], *osp = &spline->splines[!major];
    real dodt = (3*osp->a*e->tcur+2*osp->b)*e->tcur + osp->c;
    real dmdt = (3*msp->a*e->tcur+2*msp->b)*e->tcur + msp->c;
    real tmax, tmin, t, o, ocur;
    real dup, ddown;

    if ( dodt<0 ) dodt = -dodt;
    if ( dmdt<0 ) dmdt = -dmdt;
    if ( dodt>.02 && dmdt/dodt<40 )		/* very rough defn of 0 and infinite */
return( 0.0 );

    if ( osp->a==0 && osp->b==0 && osp->c==0 ) {
	*up = e->coordmax[major]-i;
	if ( *up<0 && *up>-1 )
	    *up = 0;
	*down = e->coordmin[major]-i;
	if ( *down>0 && *down<1 )
	    *down = 0;
return( e->coordmax[major]-e->coordmin[major] );
    }

    dup = ddown = 0;
    tmax = (e->tmax+1)/2;
    tmin = e->tcur;
    ocur = ((osp->a*tmin+osp->b)*tmin+osp->c)*tmin + osp->d;
    while ( tmax-tmin>.0005 ) {
	t = (tmin+tmax)/2;
	o = ((osp->a*t+osp->b)*t+osp->c)*t + osp->d;
	if ( o-ocur<1 && o-ocur>-1 ) {
	    dup = ((msp->a*t+msp->b)*t+msp->c)*t+msp->d-i;
	    if ( tmax-tmin<.001 )
    break;
	    tmin = t;
	} else
	    tmax = t;
    }
    tmin = (e->tmin+0)/2;
    tmax = e->tcur;
    while ( tmax-tmin>.0005 ) {
	t = (tmin+tmax)/2;
	o = ((osp->a*t+osp->b)*t+osp->c)*t + osp->d;
	if ( o-ocur<1 && o-ocur>-1 ) {
	    ddown = ((msp->a*t+msp->b)*t+msp->c)*t+msp->d-i;
	    if ( tmax-tmin<.001 )
    break;
	    tmax = t;
	} else
	    tmin = t;
    }
    if ( dup<0 && ddown<0 ) {
	if ( ddown>-1 && ddown>dup ) ddown = 0;
	if ( dup>-1 && dup>ddown ) dup =0;
    } else if ( dup>0 && ddown>0 ) {
	if ( ddown<1 && ddown<dup ) ddown = 0;
	if ( dup<1 && dup<ddown ) dup =0;
    }
#if 0
    if ( (dup<0 && ddown<0) || ( dup>0 && ddown>0 ))
	fprintf( stderr, "Bad values in IsEdgeHorVertHere dup=%g, ddown=%g\n", dup, ddown );
#endif
    if ( dup<0 || ddown>0 ) {	/* one might be 0, so test both */
	*down = dup;
	*up = ddown;
    } else {
	*down = ddown;
	*up = dup;
    }
#if 0
    if ( *up<0 || *down>0 )
	fprintf( stderr, "Bad values in IsEdgeHorVertHere dup=%g, ddown=%g\n", dup, ddown );
#endif
return( dup+ddown );
}

static int IsNearHV(EI *hv,EI *test,int i,real *up, real *down, int major) {
    real u1, u2, d1, d2;
    if ( IsEdgeHorVertHere(test,major,i,&u2,&d2)==0 )
return( false );
    if ( IsEdgeHorVertHere(hv,major,i,&u1,&d1)==0 )
return( false );
    if ( u1<u2 ) u2=u1;
    if ( d1>d2 ) d2=d1;
    if ( u2==0 && d2==0 )
return( false );	/* A set of parallel segments one starts where the other ends. No real stem */
    *up = u2;
    *down = d2;
return( true );
}

static real EIFigurePos(EI *e,int other, int *hadpt ) {
    real val = e->ocur;
    int oup = other==0?e->hup:e->vup;
    int major = !other;
    Spline1D *sp = &e->spline->splines[major];
    real mcur = ((sp->a*e->tcur+sp->b)*e->tcur+sp->c)*e->tcur+sp->d;

    *hadpt = false;
    if ( e->spline->from->nonextcp && e->spline->to->noprevcp &&
	    e->spline->splines[other].c!=0 ) {
	/* Pretend that a slightly sloped line is horizontal/vertical */
	/*  and the location is that of the lower end point */
	/*  (I had to pick somewhere, that seems as good as any) */
	/*  I supose I should check to make sure the lower end point is on */
	/*  the stem in question (don't know which it is yet though), and if */
	/*  not pick the upper end point... But that's too hard */
	val = e->coordmin[other];
	if ( (e->up && e->coordmin[major]+2>mcur) ||
		(!e->up && e->coordmax[major]-2<mcur) )
	    *hadpt = true;
    } else if ( e->tmin==0 && 
			((e->up && e->coordmin[major]+2>mcur) ||
			 (!e->up && e->coordmax[major]-2<mcur)) &&
			((oup && e->coordmin[other]+1>val) ||
			 (!oup && e->coordmax[other]-1<val)) ) {
	*hadpt = true;
	val = oup ? e->coordmin[other] : e->coordmax[other];
    } else if ( e->tmax==1 && 
			((!e->up && e->coordmin[major]+2>mcur) ||
			 (e->up && e->coordmax[major]-2<mcur)) &&
			((!oup && e->coordmin[other]+1>val) ||
			 (oup && e->coordmax[other]-1<val)) ) {
	*hadpt = true;
	val = !oup ? e->coordmin[other] : e->coordmax[other];
    }
return( val );
}

static HintInstance *HIMerge(HintInstance *into, HintInstance *hi) {
    HintInstance *n, *first = NULL, *last;

    while ( into!=NULL && hi!=NULL ) {
	if ( into->begin<hi->begin ) {
	    n = into;
	    into = into->next;
	} else {
	    n = hi;
	    hi = hi->next;
	}
	if ( first==NULL )
	    first = n;
	else
	    last->next = n;
	last = n;
    }
    if ( into!=NULL ) {
	if ( first==NULL )
	    first = into;
	else
	    last->next = into;
    } else if ( hi!=NULL ) {
	if ( first==NULL )
	    first = hi;
	else
	    last->next = hi;
    }
return( first );
}

StemInfo *HintCleanup(StemInfo *stem,int dosort,int instance_count) {
    StemInfo *s, *p=NULL, *t, *pt, *sn;
    int swap;

    for ( s=stem; s!=NULL; p=s, s=s->next ) {
	if ( s->backwards ) {
	    s->start += s->width;
	    s->width = -s->width;
	    s->backwards = false;
	}
	s->reordered = false;
	if ( p!=NULL && p->start> s->start )
	    dosort = true;
    }
    if ( dosort ) {
	for ( p=NULL, s=stem; s!=NULL; p=s, s = sn ) {
	    sn = s->next;
	    for ( pt=s, t=sn; t!=NULL; pt=t, t=t->next ) {
		if ( instance_count>1 &&
			t->u.unblended!=NULL && s->u.unblended!=NULL ) {
		    int temp = UnblendedCompare((*t->u.unblended)[0],(*s->u.unblended)[0],instance_count);
		    if ( temp==0 )
			swap = UnblendedCompare((*t->u.unblended)[1],(*s->u.unblended)[1],instance_count);
		    else
			swap = temp<0;
		} else if ( t->start<s->start )
		    swap=true;
		else if ( t->start>s->start )
		    swap = false;
		else
		    swap = (t->width<s->width);
		if ( swap ) {
		    s->next = t->next;
		    if ( pt==s ) {
			t->next = s;
			sn = s;
		    } else {
			t->next = sn;
			pt->next = s;
		    }
		    if ( p==NULL )
			stem = t;
		    else
			p->next = t;
		    pt = s;	/* swap s and t */
		    s = t;
		    t = pt;
		}
	    }
	}
	for ( s=stem; s!=NULL; p=s, s=s->next ) {
	    if ( s->width<0 ) {
		s->start += s->width;
		s->width = -s->width;
		s->backwards = true;
	    }
	}
	/* Remove duplicates */
	if ( stem!=NULL ) for ( p=stem, s=stem->next; s!=NULL; s = sn ) {
	    sn = s->next;
	    if ( p->start==s->start && p->width==s->width && p->hintnumber==s->hintnumber ) {
		p->where = HIMerge(p->where,s->where);
		s->where = NULL;
		p->next = sn;
		StemInfoFree(s);
	    } else
		p = s;
	}
    }
return( stem );
}

static StemInfo *StemInsert(StemInfo *stems,StemInfo *new) {
    StemInfo *prev, *test;

    if ( stems==NULL || new->start<stems->start ||
	    (new->start==stems->start && new->width<stems->width)) {
	new->next = stems;
return( new );
    }
    prev = stems;
    for ( test=stems->next; test!=NULL; prev=test, test=test->next )
	if ( new->start<test->start || (new->start==test->start && new->width<test->width))
    break;
    prev->next = new;
    new->next = test;
return(stems);
}

static StemInfo *_StemsFind(StemInfo *stems,real start, real end,
	int shadpt, int ehadpt, int lin) {
    StemInfo *test;

    /* I'm giving up on non integral hints. In post cases it means the user */
    /*  didn't design his/her font properly */
    if ( start!=floor(start) || end!=floor(end))
return( NULL );

    for ( test=stems; test!=NULL; test=test->next ) {
	if ( start-3<test->start && start+3>test->start &&
		end-3<test->start+test->width && end+3>test->start+test->width ) {
	    if ( start!=test->start && shadpt && !test->haspointleft ) {
		test->width = test->start+test->width - start;
		test->start = start;
		test->haspointleft = true;
		test->reordered = true;
	    } else if ( shadpt )
		test->haspointleft = true;
	    if ( end!=test->start+test->width && ehadpt && !test->haspointright ) {
		test->width = end-test->start;
		test->haspointright = true;
	    } else if ( ehadpt )
		test->haspointright = true;
return( test );
	}
    }

    test = chunkalloc(sizeof(StemInfo));
    test->start = start;
    test->width = end-start;
    test->haspointleft = shadpt;
    test->haspointright = ehadpt;
    test->linearedges = lin;
return( test );
}

static StemInfo *StemsFind(StemInfo *stems,EI *apt,EI *e, int major) {
    int other = !major;
    int shadpt, ehadpt;
    real start = EIFigurePos(apt,other,&shadpt);
    real end = EIFigurePos(e,other,&ehadpt);
    int lin = apt->spline->knownlinear && e->spline->knownlinear;

return( _StemsFind(stems,start,end,shadpt,ehadpt,lin));
}

static void _StemAddBrief(StemInfo *new, real mstart, real mend ) {
    HintInstance *hi, *thi;

    if ( mend<=mstart ) {
	real temp;
#if 0
	fprintf( stderr, "Bad values in StemAddBrief, mstart=%g mend=%g\n", mstart, mend);
#endif
	/* Have to add some HI, else we might crash later */
	temp = mstart;
	mstart = mend;
	mend = temp;
    }
    thi=new->where;
    if ( thi!=NULL && !(mend<thi->begin || mstart>thi->end) ) {
	if ( mstart<thi->begin ) thi->begin = mstart;
	if ( mend>thi->end ) thi->end = mend;
return;
    }
    hi = chunkalloc(sizeof(HintInstance));
    hi->begin = mstart;
    hi->end = mend;
    hi->next = new->where;
    hi->closed = true;
    new->where = hi;
return;
}

struct pendinglist {
    struct pendinglist *next;
    EI *apt, *e;
    int checka;
    real otherpos, mpos;
};

static StemInfo *StemAddBrief(StemInfo *stems,EI *apt,EI *e,
	real mstart, real mend, int major) {
    StemInfo *new;

    new = StemsFind(stems,apt,e,major);
    if ( new==NULL ) {
return( stems );
    } else if ( new->where==NULL ) {
	stems = StemInsert(stems,new);
    } else if ( new->reordered )
	stems = HintCleanup(stems,false,1);
    _StemAddBrief(new,mstart,mend);
return( stems );
}

static StemInfo *StemAddUpdate(StemInfo *stems,EI *apt,EI *e, int i, int major,
	struct pendinglist *pendings) {
    StemInfo *new;
    HintInstance *hi;
    real up, down;
    struct pendinglist *p;
    int begun = i, ap=i, ep=i;
    double atcur = apt->tcur, aocur = apt->ocur, etcur = e->tcur, eocur = e->ocur;
    Spline1D *sp;

    /* This nasty bit of code is to get the hints at the top and bottom of "a" in n021004l.pfb */
    if (( major && (apt->vertattmin || apt->vertattmax)) ||
	    ( !major && (apt->horattmin || apt->horattmax))) {
	if ( major )
	    apt->tcur = apt->vertattmin ? apt->tmin : apt->tmax;
	else
	    apt->tcur = apt->horattmin ? apt->tmin : apt->tmax;
	sp = &apt->spline->splines[!major];
	apt->ocur = ((sp->a*apt->tcur+sp->b)*apt->tcur+sp->c)*apt->tcur+sp->d;
	sp = &apt->spline->splines[major];
	ap = ((sp->a*apt->tcur+sp->b)*apt->tcur+sp->c)*apt->tcur+sp->d;
    }
    if (( major && (e->vertattmin || e->vertattmax)) ||
	    ( !major && (e->horattmin || e->horattmax))) {
	if ( major )
	    e->tcur = e->vertattmin ? e->tmin : e->tmax;
	else
	    e->tcur = e->horattmin ? e->tmin : e->tmax;
	sp = &e->spline->splines[!major];
	e->ocur = ((sp->a*e->tcur+sp->b)*e->tcur+sp->c)*e->tcur+sp->d;
	sp = &e->spline->splines[major];
	ep = ((sp->a*e->tcur+sp->b)*e->tcur+sp->c)*e->tcur+sp->d;
    }
    new = StemsFind(stems,apt,e,major);
    apt->tcur = atcur; apt->ocur = aocur; e->tcur=etcur; e->ocur = eocur;

    if ( new==NULL )
return( stems );

    if ( new->where==NULL ) {
	if ( !new->haspointleft || !new->haspointright ) {
	    for ( p=pendings; p!=NULL; p=p->next )
		if ( p->apt==apt && p->e==e ) {
		    if ( !new->haspointleft && p->checka && p->otherpos<e->ocur ) {
			new->width += new->start-p->otherpos;
			new->start = p->otherpos;
			new->haspointleft = true;
			begun = p->mpos;
		    } else if ( !new->haspointright && !p->checka && p->otherpos>apt->ocur ) {
			new->width = p->otherpos-new->start;
			new->haspointright = true;
			begun = p->mpos;
		    }
		}
	    /*new->haspoints = new->haspointleft && new->haspointright;*/
	}
	stems = StemInsert(stems,new);
    } else {
	if ( new->reordered )
	    stems = HintCleanup(stems,false,1);
	if ( new->where->end>=i ) {
	    new->where->closed = false;
return(stems);
	}
    }
    if ( new->where==NULL || (new->where->closed && new->where->end<i-1) ) {
	if ( ep!=ap ) {
	    if ( ep<ap ) { double temp = ep; ep=ap; ap=temp; }
	    if ( begun-1<ap ) ap = begun-1;	/* My algorithem misses the end points */
	    else if ( i+1>ep ) ep = i+1;	/*  so add/subtract 1 to make up for that */
	}
	if ( (!apt->spline->from->nonextcp || !apt->spline->to->noprevcp ||
		!e->spline->from->nonextcp || !e->spline->to->noprevcp ) &&
		IsNearHV(apt,e,i,&up,&down,major)) {
	    if ( down<0 && i+down>ap ) down = ap-i;
	    else if ( up<0 && i+up>ep ) up = ep-i;
	    _StemAddBrief(new,i+down,i+up);
return( stems );
	}
	hi = chunkalloc(sizeof(HintInstance));
	hi->begin = ap;
	hi->end = ep;
	hi->next = new->where;
	new->where = hi;
    } else {
	if ( new->where->end <= i )
	    new->where->end = i+1;	/* see comment above */
	new->where->closed = false;
    }
return( stems );
}

static StemInfo *StemsOffsetHack(StemInfo *stems,EI *apt,EI *e, int major) {
    double at, et, start, end;
    StemInfo *new;

    /* This nasty bit of code is to get the hints at the top and bottom of "a" in n021004l.pfb */
    if ( major &&
	    ((apt->vertattmin && apt->tmin==0) || (apt->vertattmax && apt->tmax==1.0)) &&
	    ((e->vertattmin && e->tmin==0) || (e->vertattmax && e->tmax==1.0))) {
	at = et = 0;
	if ( apt->vertattmin && apt->vertattmax && apt->tmin==0 && apt->tmax==1.0 ) {
	    if ( apt->tcur>.5 )
		at = 1;
	} else if ( apt->vertattmax && apt->tmax==1.0 )
	    at = 1;
	if ( e->vertattmin && e->vertattmax && e->tmin==0 && e->tmax==1.0 ) {
	    if ( e->tcur>.5 )
		et = 1;
	} else if ( e->vertattmax && e->tmax==1.0 )
	    et = 1;
	start = (at==1)?apt->spline->to->me.x: apt->spline->from->me.x;
	end = (et==1)?e->spline->to->me.x: e->spline->from->me.x;
	if ( start>end ) { double temp = start; start = end; end = temp; }
	new = _StemsFind(stems,start,end,true,true,false);
	if ( new==NULL || new->where!=NULL )
return( stems );
	stems = StemInsert(stems,new);
	start = (at==1)?apt->spline->to->me.y: apt->spline->from->me.y;
	end = (et==1)?e->spline->to->me.y: e->spline->from->me.y;
    } else if ( !major &&
	    ((apt->horattmin && apt->tmin==0) || (apt->horattmax && apt->tmax==1.0)) &&
	    ((e->horattmin && e->tmin==0) || (e->horattmax && e->tmax==1.0))) {
	at = et = 0;
	if ( apt->horattmin && apt->horattmax && apt->tmin==0 && apt->tmax==1.0 ) {
	    if ( apt->tcur>.5 )
		at = 1;
	} else if ( apt->horattmax && apt->tmax==1.0 )
	    at = 1;
	if ( e->horattmin && e->horattmax && e->tmin==0 && e->tmax==1.0 ) {
	    if ( e->tcur>.5 )
		et = 1;
	} else if ( e->horattmax && e->tmax==1.0 )
	    et = 1;
	start = (at==1)?apt->spline->to->me.y: apt->spline->from->me.y;
	end = (et==1)?e->spline->to->me.y: e->spline->from->me.y;
	if ( start>end ) { double temp = start; start = end; end = temp; }
	new = _StemsFind(stems,start,end,true,true,false);
	if ( new==NULL || new->where!=NULL )
return( stems );
	stems = StemInsert(stems,new);
	start = (at==1)?apt->spline->to->me.x: apt->spline->from->me.x;
	end = (et==1)?e->spline->to->me.x: e->spline->from->me.x;
    } else
return( stems );

    if ( start>end ) { double temp = start; start = end; end = temp; }
    _StemAddBrief(new,start,end);
return( stems );
}

static void PendingListFree( struct pendinglist *pl ) {
    struct pendinglist *n;

    while ( pl!=NULL ) {
	n = pl->next;
	free(pl);
	pl = n;
    }
}

static struct pendinglist *StemPending(struct pendinglist *pendings,
	EI *apt,EI *e, int checka, int major, StemInfo **stems) {
    EI *checkme = checka ? apt : e;
    int other = !major;
    Spline1D *sp = &checkme->spline->splines[major];
    Spline1D *osp = &checkme->spline->splines[other];
    real mcur = ((sp->a*checkme->tcur+sp->b)*checkme->tcur+sp->c)*checkme->tcur+sp->d;
    real ocur = checkme->ocur;
    int oup = other==0?checkme->hup:checkme->vup;
    int atmin;
    struct pendinglist *t, *p;
    real wide, mdiff;
    StemInfo *new;
	
    if ( checkme->tmin==0 && 
	    ((checkme->up && checkme->coordmin[major]+2>mcur) ||
	     (!checkme->up && checkme->coordmax[major]-2<mcur)) &&
	    ((oup && checkme->coordmin[other]+1>ocur) ||
	     (!oup && checkme->coordmax[other]-1<ocur)) )
	atmin = true;
    else if ( checkme->tmax==1 && 
	    ((!checkme->up && checkme->coordmin[major]+2>mcur) ||
	     (checkme->up && checkme->coordmax[major]-2<mcur)) &&
	    ((!oup && checkme->coordmin[other]+1>ocur) ||
	     (oup && checkme->coordmax[other]-1<ocur)) )
	atmin = false;
    else
return( pendings );

    /* Ok, we're near an end point */ /* Get its true position */
    if ( atmin ) {
	ocur = osp->d;
	mcur = sp->d;
    } else {
	ocur = osp->a+osp->b+osp->c+osp->d;
	mcur = sp->a+sp->b+sp->c+sp->d;
    }

    for ( p=NULL, t=pendings; t!=NULL && (t->apt!=apt || t->e!=e); p=t, t=t->next );
    if ( t==NULL ) {
	/* This stem isn't on the list, add it */
	t = gcalloc(1,sizeof(struct pendinglist));
	t->next = pendings;
	/*pendings = t;*/
	t->apt = apt; t->e = e; t->checka = checka;
	t->otherpos = ocur; t->mpos = mcur;
return( t );
    } else if ( t->checka==checka )
return( pendings );
    else {
	/* We've got the other endpoint for the stem */
	/* Make sure the stem is wider than the difference between the points */
	/*  in the other dimension */
	if ( (wide = ocur-t->otherpos)<0 ) wide = -wide;
	if ( (mdiff = mcur-t->mpos)<0 ) mdiff = -mdiff;
	if ( wide<mdiff )
return( pendings );
	new = _StemsFind(*stems,ocur<t->otherpos?ocur:t->otherpos,
		    ocur<t->otherpos?t->otherpos:ocur,
		    true,true,false);	/* pending hints only happen at endpoints */
	if ( new==NULL )
return( pendings );

	if ( new->where==NULL ) {
	    new->pendingpt = true;
	    *stems = StemInsert(*stems,new);
	}
	_StemAddBrief(new,mcur<t->mpos ? mcur : t->mpos,
			  mcur<t->mpos ? t->mpos : mcur);
	if ( p==NULL )
	    pendings = t->next;
	else
	    p->next = t->next;
	free(t);
return( pendings );
    }
}

static void StemCloseUntouched(StemInfo *stems,int i ) {
    StemInfo *test;

    for ( test=stems; test!=NULL; test=test->next )
	if ( test->where->end<i )
	    test->where->closed = true;
}


real EITOfNextMajor(EI *e, EIList *el, real sought_m ) {
    /* We want to find t so that Mspline(t) = sought_m */
    /*  the curve is monotonic */
    Spline1D *msp = &e->spline->splines[el->major];
    real new_t;
    real found_m;
    real t_mmax, t_mmin;
    real mmax, mmin;

    if ( msp->a==0 && msp->b==0 ) {
	if ( msp->c == 0 ) {
	    GDrawIError("Hor/Vert line when not expected");
return( 0 );
	}
	new_t = (sought_m-msp->d)/(msp->c);
return( new_t );
    }

    mmax = e->coordmax[el->major];
    t_mmax = e->up?e->tmax:e->tmin;
    mmin = e->coordmin[el->major];
    t_mmin = e->up?e->tmin:e->tmax;
    /* sought_m += el->low; */

    while ( 1 ) {
	new_t = (t_mmin+t_mmax)/2;
	found_m = ( ((msp->a*new_t+msp->b)*new_t+msp->c)*new_t + msp->d );
	if ( found_m>sought_m-.001 && found_m<sought_m+.001 )
return( new_t );
	if ( found_m > sought_m ) {
	    mmax = found_m;
	    t_mmax = new_t;
	} else {
	    mmin = found_m;
	    t_mmin = new_t;
	}
	if ( t_mmax==t_mmin ) {
	    GDrawIError("EITOfNextMajor failed! on %s", el->sc!=NULL?el->sc->name:"Unknown" );
return( new_t );
	}
    }
}

EI *EIActiveListReorder(EI *active,int *change) {
    int any;
    EI *pr, *apt;

    *change = false;
    if ( active!=NULL ) {
	any = true;
	while ( any ) {
	    any = false;
	    for ( pr=NULL, apt=active; apt->aenext!=NULL; ) {
		if ( apt->ocur <= apt->aenext->ocur ) {
		    /* still ordered */;
		    pr = apt;
		    apt = apt->aenext;
		} else if ( pr==NULL ) {
		    active = apt->aenext;
		    apt->aenext = apt->aenext->aenext;
		    active->aenext = apt;
		    *change = true;
		    /* don't need to set any, since this reorder can't disorder the list */
		    pr = active;
		} else {
		    pr->aenext = apt->aenext;
		    apt->aenext = apt->aenext->aenext;
		    pr->aenext->aenext = apt;
		    any = *change = true;
		    pr = pr->aenext;
		}
	    }
	}
    }
return( active );
}

EI *EIActiveEdgesRefigure(EIList *el, EI *active,real i,int major, int *_change) {
    EI *apt, *pr, *npt;
    int change = false, subchange;
    int other = !major;

    /* first remove any entry which doesn't intersect the new scan line */
    /*  (ie. stopped on last line) */
    for ( pr=NULL, apt=active; apt!=NULL; apt = apt->aenext ) {
	if ( apt->coordmax[major]<i+el->low ) {
	    if ( pr==NULL )
		active = apt->aenext;
	    else
		pr->aenext = apt->aenext;
	    change = true;
	} else
	    pr = apt;
    }
    /* then move the active list to the next line */
    for ( apt=active; apt!=NULL; apt = apt->aenext ) {
	Spline1D *osp = &apt->spline->splines[other];
	apt->tcur = EITOfNextMajor(apt,el,i+el->low);
	apt->ocur = ( ((osp->a*apt->tcur+osp->b)*apt->tcur+osp->c)*apt->tcur + osp->d );
    }
    /* reorder list */
    active = EIActiveListReorder(active,&subchange);
    if ( subchange ) change = true;

    /* Insert new nodes */
    if ( el->ordered[(int) i]!=NULL ) change = true;
    for ( pr=NULL, apt=active, npt=el->ordered[(int) i]; apt!=NULL && npt!=NULL; ) {
	if ( npt->ocur<apt->ocur ) {
	    npt->aenext = apt;
	    if ( pr==NULL )
		active = npt;
	    else
		pr->aenext = npt;
	    pr = npt;
	    npt = npt->ordered;
	} else {
	    pr = apt;
	    apt = apt->aenext;
	}
    }
    while ( npt!=NULL ) {
	npt->aenext = NULL;
	if ( pr==NULL )
	    active = npt;
	else
	    pr->aenext = npt;
	pr = npt;
	npt = npt->ordered;
    }
    *_change = change;
return( active );
}

/* Should I consider e and n to be a continuation of the same spline? */
/*  If we are at an intersection (and it's the same intersection on both) */
/*  and they go in vaguely the same direction then we should */
/* Ah, but also if they are at different intersections and are connected */
/*  by a series of horizontal/vertical lines (whichever are invisible to major)*/
/*  then we still should. */
int EISameLine(EI *e, EI *n, real i, int major) {
    EI *t;

    if ( n!=NULL && /*n->up==e->up &&*/
	    (ceil(e->coordmin[major])==i || floor(e->coordmin[major])==i || floor(e->coordmax[major])==i || ceil(e->coordmax[major])==i) &&
	    (ceil(n->coordmin[major])==i || floor(n->coordmin[major])==i || floor(n->coordmax[major])==i || ceil(n->coordmax[major])==i) ) {
	if (
	     (n==e->splinenext && n->tmin==e->tmax &&
		    n->tcur<n->tmin+.2 && e->tcur>e->tmax-.2 ) ||
	     (n->splinenext==e && n->tmax==e->tmin &&
		    n->tcur>n->tmax-.2 && e->tcur<e->tmin+.2 ) )
return( true );
	/* can be seperated by a horizontal/vertical line in the other direction */
	if ( n->tmax==1 && e->tmin==0 && n->tcur>.8 && e->tcur<.2) {
	    t = n;
	    while ( (t = t->splinenext)!=e ) {
		if ( t==NULL || t==n ||
			(major && !t->hor) || ( !major && !t->vert ))
return( false );
	    }
return( n->up==e->up );
	} else if ( n->tmin==0 && e->tmax==1 && n->tcur<.2 && e->tcur>.8) {
	    t = e;
	    while ( (t = t->splinenext)!=n ) {
		if ( t==NULL || t==e ||
			(major && !t->hor) || ( !major && !t->vert ))
return( false );
	    }
return( n->up==e->up );
	}
    }
return( false );
}

#if 1
int EISkipExtremum(EI *e, real i, int major) {
    EI *n = e->aenext, *t;

    if ( n==NULL )
return( false );
    if ( 
	    (ceil(e->coordmin[major])==i || floor(e->coordmin[major])==i || floor(e->coordmax[major])==i || ceil(e->coordmax[major])==i) &&
	    (ceil(n->coordmin[major])==i || floor(n->coordmin[major])==i || floor(n->coordmax[major])==i || ceil(n->coordmax[major])==i) ) {
	if (
	     (n==e->splinenext && n->tmin==e->tmax &&
		    n->tcur<n->tmin+.2 && e->tcur>e->tmax-.2 ) ||
	     (n->splinenext==e && n->tmax==e->tmin &&
		    n->tcur>n->tmax-.2 && e->tcur<e->tmin+.2 ) )
return( n->up!=e->up );
	/* can be seperated by a horizontal/vertical line in the other direction */
	if ( n->tmax==1 && e->tmin==0 && n->tcur>.8 && e->tcur<.2) {
	    t = n;
	    while ( (t = t->splinenext)!=e ) {
		if ( t==NULL || t==n ||
			(major && !t->hor) || ( !major && !t->vert ))
return( false );
	    }
return( n->up!=e->up );
	} else if ( n->tmin==0 && e->tmax==1 && n->tcur<.2 && e->tcur>.8) {
	    t = e;
	    while ( (t = t->splinenext)!=n ) {
		if ( t==NULL || t==e ||
			(major && !t->hor) || ( !major && !t->vert ))
return( false );
	    }
return( n->up!=e->up );
	}
    }
return( false );
}
#else
int EISkipExtremum(EI *e, real pos, int major) {
    Spline1D *s;
    real slopem, slopeo;

    s = &e->spline->splines[major];
    slopem = (3*s->a*e->tcur+2*s->b)*e->tcur+s->c;
    s = &e->spline->splines[!major];
    slopeo = (3*s->a*e->tcur+2*s->b)*e->tcur+s->c;
    if ( !RealNear(slopeo,0)) {
	slopem/=slopeo;
	if ( slopem>-.15 && slopem<.15 )
return( true );
    }
return( false );
}
#endif

EI *EIActiveEdgesFindStem(EI *apt, real i, int major) {
    int cnt=apt->up?1:-1;
    EI *pr, *e, *p;

	/* If we're at an intersection point and the next spline continues */
	/*  in about the same direction then this doesn't count as two lines */
	/*  but as one */
    if ( EISameLine(apt,apt->aenext,i,major))
	apt = apt->aenext;

    pr=apt; e=apt->aenext;
    if ( e==NULL )
return( NULL );

    for ( ; e!=NULL && cnt!=0; pr=e, e=e->aenext ) {
	p = e;
	if ( EISkipExtremum(e,i,major)) {
	    e = e->aenext;
	    if ( e==NULL )
    break;
    continue;
	}
	if ( EISameLine(e,e->aenext,i,major))
	    e = e->aenext;
	cnt += (e->up?1:-1);
    }
return( p );
}

static HintInstance *HIReverse(HintInstance *cur) {
    HintInstance *p, *n;

    p = NULL;
    for ( ; (n=cur->next)!=NULL; cur = n ) {
	cur->next = p;
	p = cur;
    }
    cur->next = p;
return( cur );
}

static void CheckDiagEnd(SplinePoint *nl,SplinePoint *l,SplinePoint *r,
	SplinePoint *nr,int top,MinimumDistance **mds) {
    MinimumDistance *md;

    if ( nl->me.y==nr->me.y && nl->me.x<l->me.x && nr->me.x>r->me.x &&
	    ((top && nl->me.y>l->me.y && nl->me.y>r->me.y ) ||
	     (!top&& nl->me.y<l->me.y && nl->me.y<r->me.y )) ) {
	md = chunkalloc(sizeof(MinimumDistance));
	md->sp1 = nl;
	md->sp2 = nr;
	md->x = true;
	md->next = *mds;
	*mds = md;
    }
}

static void AddDiagMD(DStemInfo *new,EI *apt,EI *e,MinimumDistance **mds) {
    SplinePoint *lt, *lb, *rt, *rb;
    SplinePoint *nlt, *nlb, *nrt, *nrb;

    nlt = nlb = nrt = nrb = NULL;
    if ( apt->spline->to->me.y > apt->spline->from->me.y ) {
	lt = apt->spline->to;
	if ( lt->next!=NULL ) nlt = lt->next->to;
	lb = apt->spline->from;
	if ( lb->prev!=NULL ) nlb = lb->prev->from;
    } else {
	lt = apt->spline->from;
	if ( lt->prev!=NULL ) nlt = lt->prev->from;
	lb = apt->spline->to;
	if ( lb->next!=NULL ) nlb = lb->next->to;
    }
    if ( e->spline->to->me.y > e->spline->from->me.y ) {
	rt = e->spline->to;
	if ( rt->next!=NULL ) nrt = rt->next->to;
	rb = e->spline->from;
	if ( rb->prev!=NULL ) nrb = rb->prev->from;
    } else {
	rt = e->spline->from;
	if ( rt->prev!=NULL ) nrt = rt->prev->from;
	rb = e->spline->to;
	if ( rb->next!=NULL ) nrb = rb->next->to;
    }
    if ( nlt!=NULL && nrt!=NULL )
	CheckDiagEnd(nlt,lt,rt,nrt,1,mds);
    if ( nlb!=NULL && nrb!=NULL )
	CheckDiagEnd(nlb,lb,rb,nrb,0,mds);
}

static int MakeDStem(DStemInfo *d,EI *apt,EI *e) {
    memset(d,'\0',sizeof(DStemInfo));
    if ( apt->spline->to->me.y > apt->spline->from->me.y ) {
	d->leftedgetop = apt->spline->to->me;
	d->leftedgebottom = apt->spline->from->me;
    } else if ( apt->spline->to->me.y == apt->spline->from->me.y ||
		apt->spline->to->me.x == apt->spline->from->me.x )
return(false);			/* If it's horizontal/vertical, I'm not interested */
    else {
	d->leftedgetop = apt->spline->from->me;
	d->leftedgebottom = apt->spline->to->me;
    }

    if ( e->spline->to->me.y > e->spline->from->me.y ) {
	d->rightedgetop = e->spline->to->me;
	d->rightedgebottom = e->spline->from->me;
    } else if ( e->spline->to->me.y == e->spline->from->me.y ||
		e->spline->to->me.x == e->spline->from->me.x )
return(false);			/* If it's horizontal/vertical, I'm not interested */
    else {
	d->rightedgetop = e->spline->from->me;
	d->rightedgebottom = e->spline->to->me;
    }
return( true );
}

static int AreNearlyParallel(EI *apt,EI *e) {
    DStemInfo d;
    real x,y,len, len1,len2, stemwidth;
    BasePoint top, bottom;
    double scale, slope;

    if ( !MakeDStem(&d,apt,e))
return( false );

    scale = (d.rightedgetop.y-d.rightedgebottom.y)/(d.leftedgetop.y-d.leftedgebottom.y);
    slope = (d.leftedgetop.y-d.leftedgebottom.y)/(d.leftedgetop.x-d.leftedgebottom.x);
    if ( !RealWithin(d.rightedgetop.x,d.rightedgebottom.x+(d.leftedgetop.x-d.leftedgebottom.x)*scale,10) &&
	    !RealWithin(d.rightedgetop.y,d.rightedgebottom.y+slope*(d.rightedgetop.x-d.rightedgebottom.x),12) )
return( false );

    /* Now check that the two segments have considerable overlap */
	/* First find the normal vector to either segment (should be approximately the same) */
    x = d.leftedgetop.y-d.leftedgebottom.y; y = -(d.leftedgetop.x-d.leftedgebottom.x);
    len = sqrt(x*x+y*y);
    x/=len; y/=len;
	/* Now find the projection onto this normal of the vector between any */
	/*  point on the first segment to any point on the second (should be the same) */
    stemwidth = x*(d.leftedgetop.x-d.rightedgetop.x) +
	    y*(d.leftedgetop.y-d.rightedgetop.y);
    x *= stemwidth; y *= stemwidth;
	/* Now we should have a vector which will move us orthogonal to the */
	/*  segments from one to the other */

    /* Now figure the overlap... */
    if ( d.leftedgetop.y>d.rightedgetop.y+y ) {
	top = d.rightedgetop;
	top.x += x; top.y += y;
    } else
	top = d.leftedgetop;

    if ( d.leftedgebottom.y<d.rightedgebottom.y+y ) {
	bottom = d.rightedgebottom;
	bottom.x += x; bottom.y += y;
    } else
	bottom = d.leftedgebottom;
    if ( top.y<bottom.y )		/* No overlap */
return( false );
    x = d.leftedgetop.x-d.leftedgebottom.x; y = d.leftedgetop.y-d.leftedgebottom.y;
    len1 = x*x + y*y;
    x = d.rightedgetop.x-d.rightedgebottom.x; y = d.rightedgetop.y-d.rightedgebottom.y;
    len2 = x*x + y*y;
    if ( len1>len2 )
	len1 = len2;
    x = top.x-bottom.x; y = top.y-bottom.y;
    len = x*x + y*y;
    if ( len<len1/9 )			/* Very little overlap (remember the lengths are squared so 9=>3) */
return( false );

    /* Now suppose we have a parallelogram. Then there are two possibilities for */
    /*  dstems, but we only want one of them. We want the dstem where the stem */
    /*  width is less than the stem length */ /* len1 is the smaller stem length */
    if ( stemwidth*stemwidth >= len1 )
return( false );

return( true );
}

static DStemInfo *AddDiagStem(DStemInfo *dstems,EI *apt,EI *e,MinimumDistance **mds) {
    DStemInfo d, *test, *prev, *new;

    if ( !MakeDStem(&d,apt,e))
return( dstems );
    if ( dstems==NULL || d.leftedgetop.x < dstems->leftedgetop.x ) {
	new = chunkalloc(sizeof(DStemInfo));
	*new = d;
	new->next = dstems;
	AddDiagMD(new,apt,e,mds);
return( new );
    }
    for ( prev=NULL, test=dstems; test!=NULL && d.leftedgetop.x>test->leftedgetop.x;
	    prev = test, test = test->next );
    if ( test!=NULL &&
	    d.leftedgetop.x==test->leftedgetop.x && d.leftedgetop.y==test->leftedgetop.y &&
	    d.rightedgetop.x==test->rightedgetop.x && d.rightedgetop.y==test->rightedgetop.y &&
	    d.leftedgebottom.x==test->leftedgebottom.x && d.leftedgebottom.y==test->leftedgebottom.y &&
	    d.rightedgebottom.x==test->rightedgebottom.x && d.rightedgebottom.y==test->rightedgebottom.y )
return( dstems );
    new = chunkalloc(sizeof(DStemInfo));
    *new = d;
    new->next = test;
    AddDiagMD(new,apt,e,mds);
    if ( prev==NULL )
return( new );
    prev->next = new;
return( dstems );
}

static StemInfo *URWSerifChecker(SplineChar *sc,StemInfo *stems) {
    /* Some serifs we just don't notice because they are too far from */
    /*  horizontal/vertical. So do a special check for these cases */
    SplineSet *spl;
    SplinePoint *sp, *n;
    BasePoint *bp, *bn;
    double maxheight = .04*(sc->parent->ascent+sc->parent->descent);
    StemInfo *prev, *test, *cur;

    for ( spl = sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	for ( sp = spl->first; ; sp=n ) {
	    if ( sp->next==NULL )
	break;
	    n = sp->next->to;
	    if ( n->next==NULL )
	break;
	    if ( sp->me.x==n->me.x && sp->prev!=NULL &&
		    n->me.y-sp->me.y>-maxheight && n->me.y-sp->me.y<maxheight &&
		    (sp->prev->from->me.x-sp->me.x<0) == (n->next->to->me.x-sp->me.x<0) ) {
		double slope = 20;
		if ( n->nonextcp ) bn = &n->next->to->me; else bn = &n->nextcp;
		if ( sp->noprevcp ) bp = &sp->prev->from->me; else bp = &sp->prevcp;
		if ( bp->y == sp->me.y && n->me.x!=bn->x )
		    slope = (n->me.y-bn->y)/( n->me.x-bn->x );
		else if ( bn->y == n->me.y && sp->me.x!=bp->x )
		    slope = (sp->me.y-bp->y)/( sp->me.x-bp->x );
		if ( slope<0 ) slope = -slope;
		if ( slope< 1/7. ) {	/* URW NimbusRoman has slope of 6/74 */
		    double start, width;
		    start = sp->me.y; width = n->me.y-sp->me.y;
		    if ( width<0 ) {
			start += width;
			width = -width;
		    }
		    for ( prev=NULL, test=stems; test!=NULL &&
			    (test->start<start || (test->start==start && test->width<width));
			    prev = test, test=test->next );
		    if ( test==NULL || test->start!=start || test->width!=width ) {
			cur = chunkalloc(sizeof(StemInfo));
			cur->start = start;
			cur->width = width;
			cur->haspointleft = cur->haspointright = true;
			cur->next = test;
			if ( prev==NULL ) stems = cur;
			else prev->next = cur;
			test = cur;
		    }
		    if ( test->start==start && test->width==width && slope!=0 ) {
			double b, e;
			HintInstance *p, *t, *hi;
			if ( sp->prev->from->me.x>sp->me.x ) {
			    b = sp->me.x;
			    e = sp->me.x+1/slope;
			} else {
			    b = sp->me.x-1/slope;
			    e = sp->me.x;
			}
			for ( p=NULL, t=test->where; t!=NULL &&
				(t->begin<b || (t->begin==b && t->end<e)); p=t, t=t->next );
			if ( t!=NULL && ((b>=t->begin && b<t->end) || (e>=t->begin && e<t->end)) )
			    /* already there */;
			else {
			    hi = chunkalloc(sizeof(HintInstance));
			    hi->begin = b;
			    hi->end = e;
			    hi->next = t;
			    if ( p==NULL ) test->where = hi;
			    else p->next = hi;
			}
		    }
		}
	    }
	    if ( n==spl->first )
	break;
	}
    }
return( stems );
}

static StemInfo *MergePossible(StemInfo *stems,StemInfo *possible) {
    StemInfo *next, *s, *prev;

    prev = NULL;
    if ( possible!=NULL )
    for ( s=possible; s->next!=NULL; s=next ) {
	next = s->next;
	if ( s->start+s->width>=next->start ) {
	    if ( /*s->width<next->width*/HIlen(s)<HIlen(next) ) {
		s->next = next->next;
		next->next = NULL;
		StemInfoFree(next);
		next = s;
	    } else {
		if ( prev==NULL )
		    possible=next;
		else
		    prev->next = next;
		s->next = NULL;
		StemInfoFree(s);
	    }
	} else
	    prev = s;
    }

    while ( possible!=NULL ) {
	next = possible->next;
	possible->next = NULL;
	for ( s=stems; s!=NULL; s=s->next ) {
	    if ( s->start==possible->start && s->width==possible->width )
	break;
	    if ((( s->start>=possible->start && s->start<=possible->start+possible->width ) ||
		    (s->start+s->width>=possible->start && s->start+s->width<=possible->start+possible->width)) &&
		    (HIlen(s)>s->width && s->linearedges))
	break;
	}
	if ( s==NULL )
	    stems = StemInsert(stems,possible);
	else
	    StemInfoFree(possible);
	possible = next;
    }
return( stems );
}

static StemInfo *ELFindStems(EIList *el, int major, DStemInfo **dstems, MinimumDistance **mds ) {
    EI *active=NULL, *apt, *e, *p;
    int i, change, ahv, ehv, waschange;
    StemInfo *stems=NULL, *s;
    real up, down;
    struct pendinglist *pendings = NULL;
    int other = !major;
    StemInfo *possible = NULL;

    waschange = false;
    for ( i=0; i<el->cnt; ++i ) {
	active = EIActiveEdgesRefigure(el,active,i,major,&change);
	/* Change means something started, ended, crossed */
	/* I'd like to know when a crossing is going to happen the pixel before*/
	/*  it does. but that's too hard to compute */
	/* We also check every 16 pixels, mostly for cosmetic reasons */
	/*  (long almost horizontal/vert regions may appear to end to abruptly) */
	if ( !( waschange || change || el->ends[i] || el->ordered[i]!=NULL ||
		(i!=el->cnt-1 && (el->ends[i+1] || el->ordered[i+1]!=NULL)) ||
		(i&0xf)==0 ))
	    /* It's in the middle of everything. Nothing will have changed */
    continue;
	waschange = change;
	for ( apt=active; apt!=NULL; apt = e->aenext ) {
	    if ( EISkipExtremum(apt,i+el->low,major)) {
		e = apt->aenext;
		if ( e==NULL )
	break;
		else
	continue;
	    }
	    if ( !apt->hv && apt->aenext!=NULL && apt->aenext->hv &&
		    EISameLine(apt,apt->aenext,i+el->low,major))
		apt = apt->aenext;
	    e = p = EIActiveEdgesFindStem(apt, i+el->low, major);
	    if ( e==NULL )
	break;
	    if ( !e->hv && e->aenext!=NULL && e->aenext->hv &&
		    EISameLine(e,e->aenext,i+el->low,major))
		e = e->aenext;
	    ahv = ( apt->hv ||
		    (apt->coordmax[other]-apt->ocur<1 &&
			((apt->hup==apt->vup && apt->hvtop) ||
			 (apt->hup!=apt->vup && apt->hvbottom))) ||
		    (apt->ocur-apt->coordmin[other]<1 && 
			((apt->hup==apt->vup && apt->hvbottom) ||
			 (apt->hup!=apt->vup && apt->hvtop)) ) );
	    ehv = ( e->hv ||
		    (e->coordmax[other]-e->ocur<1 &&
			((e->hup==e->vup && e->hvtop) ||
			 (e->hup!=e->vup && e->hvbottom))) ||
		    (e->ocur-e->coordmin[other]<1 && 
			((e->hup==e->vup && e->hvbottom) ||
			 (e->hup!=e->vup && e->hvtop)) ) );
	    if ( ahv && ehv )
		stems = StemAddUpdate(stems,apt,e,el->low+i,major,pendings);
	    else if ( ahv && IsNearHV(apt,e,i+el->low,&up,&down,major))
		stems = StemAddBrief(stems,apt,e,el->low+i+down,el->low+i+up,major);
	    else if ( ehv && IsNearHV(e,apt,i+el->low,&up,&down,major))
		stems = StemAddBrief(stems,apt,e,el->low+i+down,el->low+i+up,major);
	    else if ( ( ehv && !e->hv ) || ( ahv && !apt->hv ) ) {
		StemInfo *temp = stems;
		possible = StemsOffsetHack(possible,apt,e,major);
		pendings = StemPending(pendings,apt,e,ahv,major,&temp);
		stems = temp;
	    } else if ( dstems!=NULL &&
		    apt->spline->knownlinear && e->spline->knownlinear &&
		    AreNearlyParallel(apt,e)) {
		*dstems = AddDiagStem(*dstems,apt,e,mds);
	    }
		
	    if ( EISameLine(p,p->aenext,i+el->low,major))	/* There's one case where this doesn't happen in FindStem */
		e = p->aenext;		/* If the e is horizontal and e->aenext is not */
	}
	StemCloseUntouched(stems,i+el->low);
    }
    for ( s=stems; s!=NULL; s=s->next )
	s->where = HIReverse(s->where);
    PendingListFree(pendings);
    stems = MergePossible(stems,possible);
    if ( major==0 )
	stems = URWSerifChecker(el->sc,stems);
return( stems );
}

static void HIExtendTo(HintInstance *where, real val ) {
    while ( where!=NULL ) {
	if ( where->begin>val && where->begin<=val+1 ) {
	    where->begin=val;
return;
	} else if ( where->end<val && where->end>=val-1 ) {
	    where->end = val;
return;
	}
	where = where->next;
    }
}

static StemInfo *StemRemoveZeroHIlen(StemInfo *stems) {
    StemInfo *s, *p, *t, *sn;
    HintInstance *hi;
    /* Ok, consider the vstems of a serifed I */
    /* Now at the top of the main stem (at the base of the top serif) we have */
    /*  four points on a horizontal line (left of serif, left of stem, right of stem, right of serif) */
    /*  PfaEdit will pick two of these at random and attempt to draw a stem there */
    /*  If we picked the left edge of the serif and the right edge of the stem */
    /*  we made a mistake. It will show up as a 0 hilen stem. But it will also*/
    /*  keep the serif and main stems from reaching the endpoints, so fix that*/

    for ( p=NULL, s=stems; s!=NULL; s = sn ) {
	sn = s->next;
	if ( HIlen(s)==0 ) {
	    if ( s->where!=NULL ) {
		t = NULL;
		if ( sn!=NULL && sn->start==s->start )
		    t = sn;
		else if ( p!=NULL && p->start==s->start )
		    t = p;
		if ( t!=NULL )
		    for ( hi = s->where; hi!=NULL; hi=hi->next )
			HIExtendTo(t->where,hi->begin);
		for ( t=s->next; t!=NULL && t->start<s->start+s->width; t=t->next )
		    if ( t->start+t->width==s->start+s->width )
		break;
		if ( t!=NULL && t->start+t->width==s->start+s->width )
		    for ( hi = s->where; hi!=NULL; hi=hi->next )
			HIExtendTo(t->where,hi->begin);
	    }
	    if ( p==NULL )
		stems = sn;
	    else
		p->next = sn;
	    StemInfoFree(s);
	} else
	    p = s;
    }
return( stems );
}

static int AnyPointsAt(SplineSet *spl,int major,real coord) {
    SplinePoint *sp;

    while ( spl!=NULL ) {
	sp = spl->first;
	do {
	    if (( major && sp->me.x==coord ) || (!major && sp->me.y==coord))
return( true );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	} while ( sp!=spl->first );
	spl = spl->next;
    }
return( false );
}
		
/* I don't do a good job of figuring points left/right because sometimes */
/*  the point is actually outside the (obvious) range of the hint. So check */
/*  and see if there are any points with our coord value. If there are none */
/*  then it's safe to remove the hint */
static StemInfo *StemRemovePointlessHints(SplineChar *sc,int major,StemInfo *stems) {
    StemInfo *head=stems, *n, *p;
    int right, left;

    p = NULL;
    while ( stems!=NULL ) {
	n = stems->next;
	right = stems->haspointright; left = stems->haspointleft;
	if ( !left )
	    left = AnyPointsAt(sc->layers[ly_fore].splines,major,stems->start);
	if ( !right )
	    right = AnyPointsAt(sc->layers[ly_fore].splines,major,stems->start+stems->width);
	if ( !right || !left ) {
	    StemInfoFree(stems);
	    if ( p==NULL )
		head = n;
	    else
		p->next = n;
	} else
	    p = stems;
	stems = n;
    }
return( head );
}

static StemInfo *StemRemoveWiderThanLong(StemInfo *stems,real big) {
    StemInfo *s, *p, *sn;
    /* Consider hyphen */
    /* It seems to confuse freetype if we give it a vertical stem (even though*/
    /*  it certainly is one). Instead, for rectangular stems remove the orientation */
    /*  which is wider than long */

    for ( p=NULL, s=stems; s!=NULL; s = sn ) {
	sn = s->next;
	if ( (s->linearedges || s->width>big || !(s->haspointleft || s->haspointright)) &&
		s->width>HIlen(s)) {
	    if ( p==NULL )
		stems = sn;
	    else
		p->next = sn;
	    StemInfoFree(s);
	} else
	    p = s;
    }
return( stems );
}

static StemInfo *StemRemoveSerifOverlaps(StemInfo *stems) {
    /* I don't think the rasterizer will be able to do much useful stuff with*/
    /*  with a serif vstem. What we want is to make sure the distance between*/
    /*  the nested (main) stem and the serif is the same on both sides but */
    /*  there is no mechanism for that */
    /* There are also a few hstem serifs (the central stem of "E" or the low */
    /*  stem of "F" for instance) */
    /* So I think they are useless. But they provide overlaps, which means */
    /*  we need to invoke hint substitution for something useless. So let's */
    /*  just get rid of them */

    /* The stems list is ordered by stem start. Look for any overlaps where: */
    /*  the nested stem has about the same distance to the right as to the left */
    /*  the nested stem's height is large compared to that of the serif (containing) */
    /*  the containing stem only happens at the top and bottom of the nested */

    StemInfo *serif, *main, *prev, *next;
    HintInstance *hi;

    prev = NULL;
    for ( serif=stems; serif!=NULL; serif=next ) {
	next = serif->next;
	for ( main=serif->next; main!=NULL && main->start<serif->start+serif->width;
		main = main->next ) {
	    real left, right, sh, top, bottom;
	    left = main->start-serif->start;
	    right = serif->start+serif->width - (main->start+main->width);
	    if ( left-right<-20 || left-right>20 || left==0 || right==0 )
	continue;
	    /* In "H" the main stem is broken in two */
	    bottom = main->where->begin; top = main->where->end;
	    for ( hi = main->where; hi!=NULL; hi=hi->next ) {
		if ( bottom>hi->begin ) bottom = hi->begin;
		if ( top < hi->end ) top = hi->end;
	    }
	    sh = 0;
	    for ( hi = serif->where; hi!=NULL; hi=hi->next ) {
		sh += hi->end-hi->begin;
		if ( hi->end<top && hi->begin>bottom )
	    break;	/* serif in middle => not serif */
	    }
	    if ( hi!=NULL )
	continue;	/* serif in middle => not serif */
	    if ( 2*sh>(top-bottom) )
	continue;
	    if ( serif->where!=NULL && serif->where->next!=NULL && serif->where->next->next!=NULL )
	continue;	/* No more that two serifs, top & bottom */
	    /* If we get here, then we've got a serif and a main stem */
	break;
	}
	if ( main!=NULL && main->start<serif->start+serif->width ) {
	    /* If we get here, then we've got a serif and a main stem */
	    if ( prev==NULL )
		stems = next;
	    else
		prev->next = next;
	    StemInfoFree(serif);
	} else
	    prev = serif;
    }
return( stems );
}

real HIlen( StemInfo *stems) {
    HintInstance *hi;
    real len = 0;

    for ( hi=stems->where; hi!=NULL; hi = hi->next )
	len += hi->end-hi->begin;
return( len );
}

real HIoverlap( HintInstance *mhi, HintInstance *thi) {
    HintInstance *hi;
    real len = 0;
    real s, e;

    for ( ; mhi!=NULL; mhi = mhi->next ) {
	for ( hi = thi; hi!=NULL && hi->begin<=mhi->end; hi = hi->next ) {
	    if ( hi->end<mhi->begin ) {
		thi = hi;
	continue;
	    }
	    s = hi->begin<mhi->begin?mhi->begin:hi->begin;
	    e = hi->end>mhi->end?mhi->end:hi->end;
	    if ( e<s )
	continue;		/* Shouldn't happen */
	    len += e-s;
	}
    }
return( len );
}

int StemInfoAnyOverlaps(StemInfo *stems) {
    while ( stems!=NULL ) {
	if ( stems->hasconflicts )
return( true );
	stems = stems->next;
    }
return( false );
}

static int StemWouldConflict(StemInfo *stems,double base, double width) {
    int found;

    if ( width<0 ) {
	base += width;
	width = -width;
    }
    while ( stems!=NULL && stems->start<base && stems->start+stems->width<base )
	stems = stems->next;

    found = false;
    while ( stems!=NULL && stems->start<base+width && stems->start+stems->width<base+width ) {
	if (( stems->start==base && stems->width==width ) ||
		(stems->start+stems->width==base && stems->width==-width ))
return( false );		/* If it has already been added, then too late to worry about conflicts */
	if ( stems->width>0 ) {
	    if ( stems->start+stems->width>=base && stems->start<=base+width )
		found = true;
	} else {
	    if ( stems->start>=base && stems->start+stems->width<=base+width )
		found = true;
	}
	stems = stems->next;
    }
return( found );
}

int StemListAnyConflicts(StemInfo *stems) {
    StemInfo *s;
    int any= false;
    double end;

    for ( s=stems; s!=NULL ; s=s->next )
	s->hasconflicts = false;
    while ( stems!=NULL ) {
	end = stems->width<0 ? stems->start : stems->start+stems->width;
	for ( s=stems->next; s!=NULL && (s->width>0 ? s->start : s->start+s->width)<end; s=s->next ) {
	    stems->hasconflicts = true;
	    s->hasconflicts = true;
	    any = true;
	}
	stems = stems->next;
    }
return( any );
}

#if 0
static StemInfo *StemRemoveConflictingHintsWithoutPoints(StemInfo *stems) {
    StemInfo *head=stems, *n, *p;

    p = NULL;
    while ( stems!=NULL ) {
	n = stems->next;
	if ( n==NULL )
    break;
	if ( stems->start==n->start || stems->start+stems->width>=n->start+n->width ) {
	    if ( !stems->haspointright || !stems->haspointleft ) {
		StemInfoFree(stems);
		if ( p==NULL )
		    head = n;
		else
		    p->next = n;
		stems = n;
    continue;
	    } else if ( !n->haspointright || !n->haspointleft ) {
		stems->next = n->next;
		StemInfoFree(n);
    continue;
	    }
	}
	p = stems;
	stems = n;
    }
return( head );
}
#endif

static StemInfo *StemRemoveWideConflictingHintsContainingLittleOnes(StemInfo *stems) {
    /* The crossbar of the H when treated as a vstem is an annoyance */
    /*  There are no points for which the hint applies, but it's existance */
    /*  means that we have conflicts. So let's just get rid of it. It does */
    /*  no good */
    StemInfo *head=stems, *n, *sn, *p;
    int any;

    p = NULL;
    while ( stems!=NULL ) {
	n = stems->next;
	if ( n==NULL )
    break;
	if ( stems->start==n->start && stems->pendingpt && stems->width>n->width ) {
	    StemInfoFree(stems);
	    if ( p==NULL )
		head = n;
	    else
		p->next = n;
	    stems = n;
    continue;
	} else if ( stems->start==n->start && n->pendingpt && n->width>stems->width ) {
	    stems->next = n->next;
	    StemInfoFree(n);
	    p = stems;
	    n = stems->next;
	}
	p = stems;
	stems = n;
    }

    while ( stems!=NULL ) {
	n = stems->next;
	while ( n!=NULL && RealNear(n->start,stems->start) && n->width>4*stems->width &&
		4*HIlen(stems)>HIlen(n) ) {
	    stems->next = n->next;
	    StemInfoFree(n);
	    n = stems->next;
	}
	stems = n;
    }
    for ( p=NULL, stems=head; stems!=NULL; stems=sn ) {
	sn = stems->next;
	any = false;
	for ( n=stems->next; n!=NULL && n->start<stems->start+stems->width; n=n->next ) {
	    if ( (RealNear(n->start+n->width,stems->start+stems->width) &&
		      HIlen(stems)<HIlen(n)) ||
		    (stems->width>4*n->width && 4*HIlen(stems)<HIlen(n)) ) {
		if ( p==NULL )    
		    head = sn;
		else
		    p->next = sn;
		StemInfoFree(stems);
		any = true;
	break;
	    }
	}
	if ( !any )
	    p = stems;
    }
return( head );
}

static StemInfo *StemRemoveConflictingBigHint(StemInfo *stems,real big) {
    /* In "I" we may have a hint that runs from the top of the character to */
    /*  the bottom (especially if the serif is slightly curved), if it's the */
    /*  whole character then it doesn't do much good */
    /* Unless it is needed for blue zones?... */
    /*  It should be a ghost instead */
    StemInfo *p, *head=stems, *n, *biggest, *bp, *s;
    int any=true, conflicts;
    double max;

    while ( any ) {
	any = false;
	stems = head;
	p = NULL;
	while ( stems!=NULL ) {
	    max = stems->width<0 ? stems->start : stems->start+stems->width;
	    biggest = stems;
	    bp = p;
	    conflicts = false;
	    for ( p = stems, s=stems->next;
		    s!=NULL && (s->width<0 ? s->start+s->width : s->start) < max ;
		    p = s, s=s->next ) {
		conflicts = true;
		if ( (s->width<0 ? s->start : s->start+s->width)>max )
		    max = (s->width<0 ? s->start : s->start+s->width);
		if ( fabs(s->width) > fabs(biggest->width) ) {
		    biggest = s;
		    bp = p;
		}
	    }
	    if ( conflicts && biggest->width>big ) {
		any = true;
		n = biggest->next;
		/* Die! */
		if ( bp==NULL )
		    head = n;
		else
		    bp->next = n;
		if ( biggest==p )
		    p = bp;
		StemInfoFree(biggest);
	    }
	    stems = s;
	}
    }
#if 0
    /* if we have a hint which controls no points, conflicts with another hint*/
    /*  and contains the other hint completely then remove it */
    for ( p=NULL, stems=head; stems!=NULL; stems = n ) {
	n = stems->next;
	if ( n==NULL )
    break;
	if ( stems->hasconflicts && n->start==stems->start && n->width>=stems->width &&
		!n->haspointleft && !n->haspointright &&
		(stems->haspointleft || stems->haspointright) ) {
	    stems->next = n->next;
	    StemInfoFree(n);
	    n = stems->next;
	    p = stems;
	} else if ( stems->hasconflicts && stems->start+stems->width>n->start+n->width &&
		(n->haspointleft || n->haspointright) &&
		!stems->haspointleft && !stems->haspointright ) {
	    if ( p==NULL )
		head = n;
	    else
		p->next = n;
	    StemInfoFree(stems);
	} else
	    p = stems;
    }
#endif
return( head );
}

HintInstance *HICopyTrans(HintInstance *hi, real mul, real offset) {
    HintInstance *first=NULL, *last, *cur, *p;

    while ( hi!=NULL ) {
	cur = chunkalloc(sizeof(HintInstance));
	if ( mul>0 ) {
	    cur->begin = hi->begin*mul+offset;
	    cur->end = hi->end*mul+offset;
	    if ( first==NULL )
		first = cur;
	    else
		last->next = cur;
	    last = cur;
	} else {
	    cur->begin = hi->end*mul+offset;
	    cur->end = hi->begin*mul+offset;
	    if ( first==NULL || cur->begin<first->begin ) {
		cur->next = first;
		first = cur;
	    } else {
		for ( p=first, last=p->next; last!=NULL && cur->begin>last->begin; last=last->next );
		p->next = cur;
		cur->next = last;
	    }
	}
	hi = hi->next;
    }
return( first );
}

static int inhints(StemInfo *stems,real base, real width) {

    while ( stems!=NULL ) {
	if ( stems->start==base || stems->start+stems->width==base+width ||
		stems->start+stems->width==base || stems->start==base+width )
return( true );
	stems = stems->next;
    }
return( false );
}

static StemInfo *GhostAdd(StemInfo *ghosts, StemInfo *stems, real base,
	real width, real xstart, real xend ) {
    StemInfo *s, *prev, *test;
    HintInstance *hi;

    if ( base!=rint(base))
return( ghosts );

    if ( xstart>xend ) {
	real temp = xstart;
	xstart = xend;
	xend = temp;
    }
    if ( width==20 ) base -= 20;

    if ( inhints(stems,base,width))
return(ghosts);		/* already recorded */
    if ( StemWouldConflict(stems,base,width))
return(ghosts);		/* Let's not add a conflicting ghost hint */
    if ( StemWouldConflict(ghosts,base,width))
return(ghosts);

    for ( s=ghosts; s!=NULL; s=s->next )
	if ( s->start==base && s->width==width )
    break;
    if ( s==NULL ) {
	s = chunkalloc(sizeof(StemInfo));
	s->start = base;
	s->width = width;
	s->ghost = true;
	s->backwards = true;
	if ( ghosts==NULL || base<ghosts->start ) {
	    s->next = ghosts;
	    ghosts = s;
	} else {
	    for ( prev=ghosts, test=ghosts->next; test!=NULL && base<test->start;
		    prev = test, test = test->next);
	    prev->next = s;
	    s->next = test;
	}
    }
    hi = chunkalloc(sizeof(HintInstance));
    hi->begin = xstart;
    hi->end = xend;
    s->where = HIMerge(s->where,hi);
return( ghosts );
}

static StemInfo *CheckForGhostHints(StemInfo *stems,SplineChar *sc) {
    /* PostScript doesn't allow a hint to stretch from one alignment zone to */
    /*  another. (Alignment zones are the things in bluevalues).  */
    /* Oops, I got this wrong. PS doesn't allow a hint to start in a bottom */
    /*  zone and stretch to a top zone. Everything in OtherBlues is a bottom */
    /*  zone. The baseline entry in BlueValues is also a bottom zone. Every- */
    /*  thing else in BlueValues is a top-zone. */
    /* This means */
    /*  that we can't define a horizontal stem hint which stretches from */
    /*  the baseline to the top of a capital I, or the x-height of lower i */
    /*  If we find any such hints we must remove them, and replace them with */
    /*  ghost hints. The bottom hint has height -21, and the top -20 */
    BlueData bd;
    SplineFont *sf = sc->parent;
    StemInfo *prev, *s, *n, *snext, *ghosts = NULL;
    SplineSet *spl;
    Spline *spline, *first;
    SplinePoint *sp, *sp2;
    real base, width/*, toobig = (sc->parent->ascent+sc->parent->descent)/2*/;
    int i,startfound, widthfound;

    /* Get the alignment zones */
    QuickBlues(sf,&bd);

    /* look for any stems stretching from one zone to another and remove them */
    /*  (I used to turn them into ghost hints here, but that didn't work (for */
    /*  example on "E" where we don't need any ghosts from the big stem because*/
    /*  the narrow stems provide the hints that PS needs */
    /* However, there are counter-examples. in Garamond-Pro the "T" character */
    /*  has a horizontal stem at the top which stretches between two adjacent */
    /*  bluezones. Removing it is wrong. Um... Thanks Adobe */
    /* I misunderstood. Both of these were top-zones */
    for ( prev=NULL, s=stems; s!=NULL; s=snext ) {
	snext = s->next;
	startfound = widthfound = -1;
	for ( i=0; i<bd.bluecnt; ++i ) {
	    if ( s->start>=bd.blues[i][0]-1 && s->start<=bd.blues[i][1]+1 )
		startfound = i;
	    else if ( s->start+s->width>=bd.blues[i][0]-1 && s->start+s->width<=bd.blues[i][1]+1 )
		widthfound = i;
	}
	if ( startfound!=-1 && widthfound!=-1 &&
		( s->start>0 || s->start+s->width<=0 ))
	    startfound = widthfound = -1;
	if ( startfound!=-1 && widthfound!=-1 ) {
	    if ( prev==NULL )
		stems = snext;
	    else
		prev->next = snext;
	    s->next = NULL;
	    StemInfoFree(s);
	} else
	    prev = s;
    }

    /* Now look and see if we can find any edges which lie in */
    /*  these zones.  Edges which are not currently in hints */
    for ( spl = sc->layers[ly_fore].splines; spl!=NULL; spl = spl->next ) if ( spl->first->prev!=NULL ) {
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	    base = spline->from->me.y;
	    if ( spline->knownlinear && base == spline->to->me.y ) {
		for ( i=0; i<bd.bluecnt; ++i ) {
		    if ( base>=bd.blues[i][0]-1 && base<=bd.blues[i][1]+1 )
		break;
		}
		if ( i!=bd.bluecnt ) {
		    for ( sp2= spline->from->prev->from; sp2!=spline->from && sp2->me.y!=spline->from->me.y; sp2=sp2->prev->from );
		    width = (sp2->me.y > spline->from->me.y)?21:20;
		    ghosts = GhostAdd(ghosts,stems, base,width,spline->from->me.x,spline->to->me.x);
		}
	    }
	    if ( first==NULL ) first = spline;
	}
	/* And check for the horizontal top of a curved surface */
	for ( sp = spl->first; ; ) {
	    base = sp->me.y;
	    if ( !sp->nonextcp && !sp->noprevcp && sp->nextcp.y==base &&
		    sp->prevcp.y==base ) {
		for ( i=0; i<bd.bluecnt; ++i ) {
		    if ( base>=bd.blues[i][0]-1 && base<=bd.blues[i][1]+1 )
		break;
		}
		if ( i!=bd.bluecnt ) {
		    for ( sp2= sp->prev->from; sp2!=sp && sp2->me.y!=spline->from->me.y; sp2=sp2->prev->from );
		    width = (sp2->me.y > sp->me.y)?21:20;
		    ghosts = GhostAdd(ghosts,stems, base,width,(sp->me.x+sp->prevcp.x)/2,(sp->me.x+sp->nextcp.x)/2);
		}
	    }
	    sp = sp->next->to;
	    if ( sp == spl->first )
	break;
	}
/* It's just too hard to detect what isn't a dished serifs... */
/* We find so much stuff that is just wrong. */
#if 0
	for ( sp=spl->first; ; ) {
	    if ( sp->next==NULL )
	break;
	    /* Look for dished serifs */
	    if ( !sp->nonextcp && !sp->noprevcp && sp->me.y==sp->nextcp.y &&
		    sp->me.y==sp->prevcp.y &&
		    sp->prev!=NULL && sp->prev->from->prev!=NULL &&
		    /* sp->next!=NULL &&*/ sp->next->to->next!=NULL &&
		    sp->next->to->me.x != sp->me.x &&
		    sp->prev->from->me.x != sp->me.x &&
		    sp->next->to->me.y != sp->me.y &&
		    sp->next->to->me.y == sp->prev->from->me.y &&
		    sp->next->to->next->knownlinear &&
		    sp->prev->from->prev->knownlinear &&
		    (sp->next->to->me.y>sp->next->to->next->to->me.y)==
		     (sp->prev->from->me.y>sp->prev->from->prev->from->me.y)) {
		real xstart = (sp->prev->from->me.x-sp->me.x)/(sp->prev->from->me.y-sp->me.y);
		real xend = (sp->next->to->me.x-sp->me.x)/(sp->next->to->me.y-sp->me.y);
		if (( xstart<0 && sp->prev->from->me.x>sp->me.x) ||
			(xstart>0 && sp->prev->from->me.x<sp->me.x))
		    xstart = -xstart;
		if (( xend<0 && sp->next->to->me.x>sp->me.x) ||
			(xend>0 && sp->next->to->me.x<sp->me.x))
		    xend = -xend;
		width = (sp->next->to->me.y>sp->next->to->next->to->me.y)?21:20;
		ghosts = GhostAdd(ghosts,stems, base,width,sp->me.x+xstart,sp->me.x+xend);
	    }
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
#endif
    }

    /* Finally add any ghosts we've got back into the stem list */
    for ( s=ghosts; s!=NULL; s=snext ) {
	snext = s->next;
	for ( prev=NULL, n=stems; n!=NULL && s->start>n->start; prev=n, n=n->next );
	if ( prev==NULL ) {
	    s->next = stems;
	    stems = s;
	} else {
	    prev->next = s;
	    s->next = n;
	}
    }
return( stems );
}

static int DStemsOverlapBigger(DStemInfo *t1,DStemInfo *t2,int ls /*leftsame*/) {
    real width1, width2;
    real x,y,len, dist;

    x =  (t1->leftedgetop.y-t1->leftedgebottom.y);
    y = -(t1->leftedgetop.x-t1->leftedgebottom.x);
    len = sqrt(x*x+y*y);
    x /= len; y/= len;
    /* We now have a unit vector perpendicular to the stems (all stems should */
    /*  be parallel, so we could have chosen any of them */

    dist = ( (&t1->leftedgetop)[ls].x-(&t2->leftedgetop)[ls].x )*x +
	    ( (&t1->leftedgetop)[ls].y-(&t2->leftedgetop)[ls].y )*y;
    /* This is the distance (along our unit vector) from the edge of t1 that */
    /*  is not an edge of t2 TO the edge of t2 that is not an edge of t1 */
    if ( (&t2->leftedgebottom)[ls].y+dist*y>=(&t1->leftedgetop)[ls].y )
return( 0 );	/* no overlap */
    if ( (&t2->leftedgetop)[ls].y+dist*y<=(&t1->leftedgebottom)[ls].y )
return( 0 );	/* no overlap */

    width1 = (t1->leftedgetop.x-t1->rightedgetop.x)*x +
		(t1->leftedgetop.y-t1->rightedgetop.y)*y;
    width2 = (t2->leftedgetop.x-t2->rightedgetop.x)*x +
		(t2->leftedgetop.y-t2->rightedgetop.y)*y;
    if ( width1*width2<0 )
return( 0 );
    if ( width1<0 ) {
	width1 = -width1;
	width2 = -width2;
    }
    if ( width1>width2 )
return( 1 );			/* t1 has the wider dstem */
    else if ( width1==width2 )
return( -1 );			/* they must be the same, remove either one */

return( -1 );			/* t2 has the wider dstem */
}

static DStemInfo *DStemPrune(DStemInfo *dstems) {
    DStemInfo *test, *prev, *t2, *next, *t2next, *t2prev;
    int which;

    prev = NULL;
    for ( test=dstems; test!=NULL; test=next ) {
	next = test->next;
	t2prev = test;
	which = 0;
	for ( t2 = test->next; t2!=NULL; t2 = t2next ) {
	    t2next = t2->next;
	    which = 0;
	    if ( t2->leftedgetop.x==test->leftedgetop.x &&
		    t2->leftedgetop.y==test->leftedgetop.y &&
		    t2->leftedgebottom.x==test->leftedgebottom.x &&
		    t2->leftedgebottom.y==test->leftedgebottom.y )
		which = DStemsOverlapBigger(test,t2,1);
	    else if ( t2->rightedgetop.x==test->rightedgetop.x &&
		    t2->rightedgetop.y==test->rightedgetop.y &&
		    t2->rightedgebottom.x==test->rightedgebottom.x &&
		    t2->rightedgebottom.y==test->rightedgebottom.y )
		which = DStemsOverlapBigger(test,t2,0);
	    if ( which==1 ) {
		if ( prev==NULL )
		    dstems = next;
		else
		    prev->next = next;
		chunkfree(test,sizeof(DStemInfo));
	break;
	    } else if ( which==-1 ) {
		t2prev->next = t2next;
		if ( t2prev==test )
		    next = t2next;
		chunkfree(t2,sizeof(DStemInfo));
	    } else
		t2prev = t2;
	}
	if ( which!=1 )
	    prev = test;
    }
return( dstems );
}

static StemInfo *SCFindStems(EIList *el, int major, int removeOverlaps,DStemInfo **dstems,MinimumDistance **mds) {
    StemInfo *stems;
    real big = (el->coordmax[1-major]-el->coordmin[1-major])*.40;

    el->major = major;
    ELOrder(el,major);
    stems = ELFindStems(el,major,dstems,mds);
    free(el->ordered);
    free(el->ends);
    stems = StemRemoveZeroHIlen(stems);
    stems = StemRemoveWiderThanLong(stems,big);
    stems = StemRemovePointlessHints(el->sc,major,stems);
    if ( removeOverlaps ) {
	/*if ( major==1 )*/ /* There are a few hstem serifs that should be removed, central stem of "E" */
	    stems = StemRemoveSerifOverlaps(stems);
	stems = StemRemoveWideConflictingHintsContainingLittleOnes(stems);
	if ( major==0 )
	    stems = CheckForGhostHints(stems,el->sc);
		/* Should be done by WiderThanLong now, and done better */
		/* Nope. There are some fonts where these hints are longer than wide but still should go. */
	if ( StemListAnyConflicts(stems) ) {
	    real big = (el->coordmax[1-major]-el->coordmin[1-major])*.40;
	    char *pt;
	    if ( (major==1 && (pt=PSDictHasEntry(el->sc->parent->private,"StdVW"))!=NULL ) ||
		    (major==0 && (pt=PSDictHasEntry(el->sc->parent->private,"StdHW"))!=NULL )) {
		real val;
		while ( isspace(*pt) || *pt=='[' ) ++pt;
		val = strtod(pt,NULL);
		if ( val>big )
		    big = val*1.3;
	    }
	    stems = StemRemoveConflictingBigHint(stems,big);
	    /* Now we need to run AnyConflicts again, but we'll have to do that */
	    /*  anyway after adding hints from References */
	}
	/*stems = StemRemoveConflictingHintsWithoutPoints(stems);*/ /* Too extreme */
    }
    if ( dstems!=NULL )
	*dstems = DStemPrune( *dstems );
return( stems );
}

static HintInstance *SCGuessHintPoints(SplineChar *sc, StemInfo *stem,int major, int off) {
    SplinePoint *starts[20], *ends[20];
    int spt=0, ept=0;
    SplinePointList *spl;
    SplinePoint *sp, *np;
    int sm, wm, i, j, val;
    real coord;
    HintInstance *head, *test, *cur, *prev;

    for ( spl=sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; sp = np ) {
	    coord = (major?sp->me.x:sp->me.y);
	    sm = coord>=stem->start-off && coord<=stem->start+off;
	    wm = coord>=stem->start+stem->width-off && coord<=stem->start+stem->width+off;
	    if ( sm && spt<20 )
		starts[spt++] = sp;
	    if ( wm && ept<20 )
		ends[ept++] = sp;
	    if ( sp->next==NULL )
	break;
	    np = sp->next->to;
	    if ( np==spl->first )
	break;
	}
    }

    head = NULL;
    for ( i=0; i<spt; ++i ) {
	val = 0x80000000;
	for ( j=0; j<ept; ++j ) {
	    if ( major && starts[i]->me.y>=ends[j]->me.y-1 && starts[i]->me.y<=ends[j]->me.y+1 ) {
		val = starts[i]->me.y;
	break;
	    } else if ( !major && starts[i]->me.x>=ends[j]->me.x-1 && starts[i]->me.x<=ends[j]->me.x+1 ) {
		val = starts[i]->me.x;
	break;
	    }
	}
	if ( val!=0x80000000 ) {
	    for ( prev=NULL, test=head; test!=NULL && val>test->begin; prev=test, test=test->next );
	    if ( test==NULL || val!=test->begin ) {
		cur = chunkalloc(sizeof(HintInstance));
		cur->begin = cur->end = val;
		cur->next = test;
		if ( prev==NULL ) head = cur;
		else prev->next = cur;
	    }
	}
    }
return( head );
}

static void SCGuessHintInstances(SplineChar *sc, StemInfo *stem,int major) {
    SplinePointList *spl;
    SplinePoint *sp, *np;
    int sm, wm, off;
    real ob, oe;
    HintInstance *s=NULL, *w=NULL, *cur, *p, *t, *n, *w2;
    /* We've got a hint (from somewhere, old data, reading in a font, user specified etc.) */
    /*  but we don't have HintInstance info. So see if we can find those data */
    /* Will get confused by stems with holes in them (for example if you make */
    /*  a hint from one side of an H to the other, it will get the whole thing */
    /*  not just the cross stem) */

    for ( spl=sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; sp = np ) {
	    sm = (major?sp->me.x:sp->me.y)==stem->start;
	    wm = (major?sp->me.x:sp->me.y)==stem->start+stem->width;
	    if ( sp->next==NULL )
	break;
	    np = sp->next->to;
	    if ( sm || wm ) {
		if ( !major ) {
		    if ( np->me.y==sp->me.y ) {
			ob = sp->me.x; oe = np->me.x;
		    } else if ( sp->nextcp.y==sp->me.y ) {
			ob = sp->me.x; oe = (sp->me.x+sp->nextcp.x)/2;
			if ( sp->prevcp.y==sp->me.y )
			    ob = (sp->prevcp.x+sp->me.x)/2;
		    } else if ( sp->prevcp.y==sp->me.y ) {
			ob = sp->me.x; oe = (sp->prevcp.x+sp->me.x)/2;
		    } else
			sm = wm = false;
		} else {
		    if ( np->me.x==sp->me.x ) {
			ob = sp->me.y; oe = np->me.y;
		    } else if ( sp->nextcp.x==sp->me.x ) {
			ob = sp->me.y; oe = (sp->nextcp.y+sp->me.y)/2;
			if ( sp->prevcp.x==sp->me.x )
			    ob = (sp->prevcp.y+sp->me.y)/2;
		    } else if ( sp->prevcp.x==sp->me.x ) {
			ob = sp->me.y; oe = (sp->prevcp.y+sp->me.y)/2;
		    } else
			sm = wm = false;
		}
	    }
	    if ( sm || wm ) {
		cur = chunkalloc(sizeof(HintInstance));
		if ( ob>oe ) { real temp=ob; ob=oe; oe=temp;}
		cur->begin = ob;
		cur->end = oe;
		if ( sm ) {
		    if ( s==NULL || s->begin>cur->begin ) {
			cur->next = s;
			s = cur;
		    } else {
			p = s;
			for ( t=s->next; t!=NULL && t->begin<cur->begin; p=t, t=t->next );
			p->next = cur; cur->next = t;
		    }
		} else {
		    if ( w==NULL || w->begin>cur->begin ) {
			cur->next = w;
			w = cur;
		    } else {
			p = w;
			for ( t=w->next; t!=NULL && t->begin<cur->begin; p=t, t=t->next );
			p->next = cur; cur->next = t;
		    }
		}
	    }
	    if ( np==spl->first )
	break;
	}
    }

    /* Now we know what the right side of the stem looks like, and we know */
    /*  what the left side looks like. They may not look the same (H for example) */
    /*  Figure out the set where both are active */
    /* Unless it's a ghost hint */
    if ( stem->width==20 && s==NULL && w!=NULL ) {
	s = w;
	w = NULL;
    } else if ( stem->width==21 && s!=NULL && w==NULL) {
	/* Just use s */;
    } else for ( p=NULL, t=s; t!=NULL; t=n ) {
	n = t->next;
	for ( w2=w; w2!=NULL && w2->begin<t->end ; w2=w2->next ) {
	    if ( w2->end<=t->begin )
	continue;
	    if ( w2->begin<=t->begin && w2->end>=t->end ) {
		/* Perfect match */
	break;
	    }
	    if ( w2->begin>=t->begin )
		t->begin = w2->begin;
	    if ( w2->end<=t->end ) {
		cur = chunkalloc(sizeof(HintInstance));
		cur->begin = w2->end;
		cur->end = t->end;
		cur->next = n;
		t->next = cur;
		n = cur;
		t->end = w2->end;
	    }
	break;
	}
	if ( w2==NULL || w2->begin>=t->end ) {
	    /* No match for t (or if there were it wasn't complete) get rid */
	    /*  of what's left of t */
	    chunkfree(t,sizeof(*t));
	    if ( p==NULL )
		s = n;
	    else
		p->next = n;
	} else
	    p = t;
    }
    while ( w!=NULL ) {
	n = w->next;
	chunkfree(w,sizeof(*w));
	w=n;
    }

    /* If we couldn't find anything, then see if there are two points which */
    /*  have the same x or y value and whose other coordinates match those of */
    /*  the hint */
    /* Surprisingly many fonts have hints which don't accurately match the */
    /*  points. Perhaps BlueFuzz (default 1) applies here too */
    for ( off=0; off<1 && s==NULL; ++off )
	s = SCGuessHintPoints(sc,stem,major,off);

    stem->where = s;
}

static StemInfo *StemInfoAdd(StemInfo *list, StemInfo *new) {
    StemInfo *prev, *test;

    for ( prev=NULL, test=list; test!=NULL && new->start>test->start; prev=test, test=test->next );
    if ( test!=NULL && test->start==new->start && test->width==new->width ) {
	/* Replace the old with the new */
	/* can't just free the new, because the Guess routines depend on it */
	/*  being around */
	new->next = test->next;
	StemInfoFree(test);
    } else
	new->next = test;
    if ( prev==NULL )
	list = new;
    else
	prev->next = new;
return( list );
}

#if 0
/* Make sure that the hi list in stem does not contain any regions covered by */
/*  the cantuse list (if it does, remove them) */
static void HIRemoveFrom(StemInfo *stem, HintInstance *cantuse) {
    HintInstance *hi, *p, *t;

    p = NULL;
    for ( hi=stem->where; hi!=NULL && cantuse!=NULL; ) {
	if ( cantuse->end<hi->begin )
	    cantuse = cantuse->next;
	else if ( hi->end<cantuse->begin ) {
	    p = hi;
	    hi = hi->next;
	} else if ( cantuse->begin<=hi->begin && cantuse->end>=hi->end ) {
	    /* Perfect match, remove entirely */
	    t = hi->next;
	    if ( p==NULL )
		stem->where = t;
	    else
		p->next = t;
	    chunkfree(hi,sizeof(HintInstance));
	    hi = t;
	} else if ( cantuse->begin<=hi->begin ) {
	    hi->begin = cantuse->end;
	    cantuse = cantuse->next;
	} else if ( cantuse->end>=hi->end ) {
	    hi->end = cantuse->begin;
	    p = hi;
	    hi = hi->next;
	} else {
	    /* cantuse breaks hi into two bits */
	    t = chunkalloc(sizeof(HintInstance));
	    t->begin = cantuse->end;
	    t->end = hi->end;
	    t->next = hi->next;
	    hi->next = t;
	    hi->end = cantuse->begin;
	    p = hi;
	    hi = t;
	    cantuse = cantuse->next;
	}
    }
}

static void StemInfoReduceOverlap(StemInfo *list, StemInfo *stem) {
    /* Find all stems which conflict with this one */
    /* then for every stem which contains this one, remove this one's hi's from*/
    /*  its where list (so if this one is active, that one can't be) */
    /* Do the reverse if this stem contains others */
    /* And if they just overlap? Neither containing the other? */

    for ( ; list!=NULL && list->start<stem->start+stem->width; list=list->next ) {
	if ( list==stem || stem->start>list->start+list->width )
    continue;
	/* They overlap somehow */
	if ( list->start<=stem->start && list->start+list->width>=stem->start+stem->width )
	    HIRemoveFrom(list,stem->where);	/* Remove stem's hi's from list's */
	else if ( stem->start<=list->start && stem->start+stem->width>=list->start+list->width )
	    HIRemoveFrom(stem,list->where);	/* Remove list's hi's from stem's */
	else
	    /* I should do something here, but I've no idea what. */
	    /*  Remove the intersection from both? */
	    /*  Remove from the stem with greater width? */;
    }
}
#endif

void SCGuessHHintInstancesAndAdd(SplineChar *sc, StemInfo *stem, real guess1, real guess2) {
    SCGuessHintInstances(sc, stem, 0);
    sc->hstem = StemInfoAdd(sc->hstem,stem);
    if ( stem->where==NULL && guess1!=0x80000000 ) {
	if ( guess1>guess2 ) { real temp = guess1; guess1 = guess2; guess2 = temp; }
	stem->where = chunkalloc(sizeof(HintInstance));
	stem->where->begin = guess1;
	stem->where->end = guess2;
    }
    sc->hconflicts = StemListAnyConflicts(sc->hstem);
    if ( stem->hasconflicts ) {
	/*StemInfoReduceOverlap(sc->hstem,stem);*/	/* User asked for it, assume s/he knows what s/he's doing */
	if ( stem->where==NULL )
	    GDrawError("Couldn't figure out where this hint is active");
    }
}

void SCGuessVHintInstancesAndAdd(SplineChar *sc, StemInfo *stem, real guess1, real guess2) {
    SCGuessHintInstances(sc, stem, 1);
    sc->vstem = StemInfoAdd(sc->vstem,stem);
    if ( stem->where==NULL && guess1!=0x80000000 ) {
	if ( guess1>guess2 ) { real temp = guess1; guess1 = guess2; guess2 = temp; }
	stem->where = chunkalloc(sizeof(HintInstance));
	stem->where->begin = guess1;
	stem->where->end = guess2;
    }
    sc->vconflicts = StemListAnyConflicts(sc->vstem);
    if ( stem->hasconflicts ) {
	/*StemInfoReduceOverlap(sc->vstem,stem);*/	/* User asked for it, assume s/he knows what s/he's doing */
	if ( stem->where==NULL )
	    GDrawError("Couldn't figure out where this hint is active");
    }
}

void SCGuessHHintInstancesList(SplineChar *sc) {
    StemInfo *h;
    int any = false;

    for ( h= sc->hstem; h!=NULL; h=h->next )
	if ( h->where==NULL ) {
	    SCGuessHintInstances(sc,h,0);
	    any |= h->where!=NULL;
	}
/*
    if ( any )
	for ( h= sc->hstem; h!=NULL; h=h->next )
	    StemInfoReduceOverlap(h->next,h);
*/
}

void SCGuessVHintInstancesList(SplineChar *sc) {
    StemInfo *h;
    int any = false;

    for ( h= sc->vstem; h!=NULL; h=h->next )
	if ( h->where==NULL ) {
	    SCGuessHintInstances(sc,h,1);
	    any |= h->where!=NULL;
	}
/*
    if ( any )
	for ( h= sc->vstem; h!=NULL; h=h->next )
	    StemInfoReduceOverlap(h->next,h);
*/
}

static StemInfo *RefHintsMerge(StemInfo *into, StemInfo *rh, real mul, real offset,
	real omul, real oofset) {
    StemInfo *prev, *h, *n;
    real start, width;

    for ( ; rh!=NULL; rh=rh->next ) {
	start = rh->start*mul + offset;
	width = rh->width *mul;
	if ( width<0 ) {
	    start += width; width = -width;
	}
	for ( h=into, prev=NULL; h!=NULL && (start>h->start || (start==h->start && width>h->width)); prev=h, h=h->next );
	if ( h==NULL || start!=h->start || width!=h->width ) {
	    n = chunkalloc(sizeof(StemInfo));
	    n->start = start; n->width = width;
	    n->next = h;
	    if ( prev==NULL )
		into = n;
	    else
		prev->next = n;
	    n->where = HICopyTrans(rh->where,omul,oofset);
	} else
	    h->where = HIMerge(h->where,HICopyTrans(rh->where,omul,oofset));
    }
return( into );
}

static DStemInfo *RefDHintsMerge(DStemInfo *into, DStemInfo *rh, real xmul, real xoffset,
	real ymul, real yoffset) {
    DStemInfo *new;
    BasePoint temp;
    DStemInfo *prev, *n;

    for ( ; rh!=NULL; rh=rh->next ) {
	new = chunkalloc(sizeof(DStemInfo));
	*new = *rh;
	new->leftedgetop.x = xmul*new->leftedgetop.x + xoffset;
	new->rightedgetop.x = xmul*new->rightedgetop.x + xoffset;
	new->leftedgebottom.x = xmul*new->leftedgebottom.x + xoffset;
	new->rightedgebottom.x = xmul*new->rightedgebottom.x + xoffset;
	new->leftedgetop.y = ymul*new->leftedgetop.y + yoffset;
	new->rightedgetop.y = ymul*new->rightedgetop.y + yoffset;
	new->leftedgebottom.y = ymul*new->leftedgebottom.y + yoffset;
	new->rightedgebottom.y = ymul*new->rightedgebottom.y + yoffset;
	if ( xmul<0 ) {
	    temp = new->leftedgetop;
	    new->leftedgetop = new->rightedgetop;
	    new->rightedgetop = temp;
	    temp = new->leftedgebottom;
	    new->leftedgebottom = new->rightedgebottom;
	    new->rightedgebottom = temp;
	}
	if ( ymul<0 ) {
	    temp = new->leftedgetop;
	    new->leftedgetop = new->leftedgebottom;
	    new->leftedgebottom = temp;
	    temp = new->rightedgetop;
	    new->rightedgetop = new->rightedgebottom;
	    new->rightedgebottom = temp;
	}
	if ( into==NULL || new->leftedgetop.x<into->leftedgetop.x ) {
	    new->next = into;
	    into = new;
	} else {
	    for ( prev=into, n=prev->next; n!=NULL && new->leftedgetop.x>n->leftedgetop.x;
		    prev = n, n = n->next );
	    new->next = n;
	    prev->next = new;
	}
    }
return( into );
}

static void AutoHintRefs(SplineChar *sc,int removeOverlaps) {
    RefChar *ref;

    /* Add hints for base characters before accent hints => if there are any */
    /*  conflicts, the base characters win */
    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[1]==0 && ref->transform[2]==0 ) {
	    if ( !ref->sc->manualhints && ref->sc->changedsincelasthinted )
		SplineCharAutoHint(ref->sc,removeOverlaps);
	    if ( ref->sc->unicodeenc!=-1 && isalnum(ref->sc->unicodeenc) ) {
		sc->hstem = RefHintsMerge(sc->hstem,ref->sc->hstem,ref->transform[3], ref->transform[5], ref->transform[0], ref->transform[4]);
		sc->vstem = RefHintsMerge(sc->vstem,ref->sc->vstem,ref->transform[0], ref->transform[4], ref->transform[3], ref->transform[5]);
		sc->dstem = RefDHintsMerge(sc->dstem,ref->sc->dstem,ref->transform[0], ref->transform[4], ref->transform[3], ref->transform[5]);
	    }
	}
    }

    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[1]==0 && ref->transform[2]==0 &&
		(ref->sc->unicodeenc==-1 || !isalnum(ref->sc->unicodeenc)) ) {
	    sc->hstem = RefHintsMerge(sc->hstem,ref->sc->hstem,ref->transform[3], ref->transform[5], ref->transform[0], ref->transform[4]);
	    sc->vstem = RefHintsMerge(sc->vstem,ref->sc->vstem,ref->transform[0], ref->transform[4], ref->transform[3], ref->transform[5]);
	    sc->dstem = RefDHintsMerge(sc->dstem,ref->sc->dstem,ref->transform[0], ref->transform[4], ref->transform[3], ref->transform[5]);
	}
    }
}

void MDAdd(SplineChar *sc, int x, SplinePoint *sp1, SplinePoint *sp2) {
    StemInfo *h;
    real low, high, temp;
    MinimumDistance *md;
    int c1, c2, nc1;

    /* Don't add this if there's an md with this info already... */
    /* Also, don't add if there's an md with a stricter requirement (ie which */
    /*  is closer */
    for ( md=sc->md; md!=NULL; md = md->next ) {
	if ( md->x==x && md->sp2==sp2 ) {
	    if ( md->sp1==sp1 )
return;
	    if ( x ) {
		c1 = md->sp1->me.x; nc1 = sp1->me.x;
		c2 = sp2==NULL ? sc->width : sp2->me.x;
	    } else {
		c1 = md->sp1->me.y; nc1 = sp1->me.y;
		c2 = sp2->me.y;
	    }
	    if ( c1>c2 && nc1>c2 ) {
		if ( c1 > nc1 )
		    md->sp1 = sp1;
return;
	    } else if ( c1<c2 && nc1<c2 ) {
		if ( c1 < nc1 )
		    md->sp1 = sp1;
return;
	    }
	}
	if ( md->x==x && md->sp1==sp2 && md->sp2==sp1 )
return;
    }

    /* Don't add this if there's an hstem with this info already... */
    if ( x ) {
	low = sp1->me.x;
	if ( sp2==NULL )
	    high = low;
	else
	    high = sp2->me.x;
	h = sc->vstem;
    } else {
	low = sp1->me.y;
	if ( sp2==NULL )
	    high = low;
	else
	    high = sp2->me.y;
	h = sc->hstem;
    }
    if ( low>high ) {
	temp = low;
	low = high;
	high = temp;
    }
    for ( ; h!=NULL && (h->start<low || (h->start==low && h->start+h->width!=high)); h = h->next );
    if ( h==NULL || h->start!=low || h->start+h->width!=high ) {
	md = chunkalloc(sizeof(MinimumDistance));
	md->sp1 = sp1;
	md->sp2 = sp2;
	md->x = x;
	md->next = sc->md;
	sc->md = md;
    }
}

#if 0
static void SCAddWidthMD(SplineChar *sc) {
    StemInfo *h;
    DStemInfo *dh;
    SplineSet *ss;
    SplinePoint *sp;
    int xmax = 0;
    MinimumDistance *md;

    /* find the max of: vertical stems, diagonal stems, md's */

    if ( sc->vstem==NULL && sc->dstem==NULL && sc->md==NULL )
return;

    if ( sc->vstem!=NULL ) {
	for ( h=sc->vstem; h->next!=NULL; h=h->next );
	xmax = h->width>0?h->start+h->width:h->start;
    }
    for ( dh = sc->dstem; dh!=NULL; dh=dh->next ) {
	if ( dh->rightedgetop.x>xmax ) xmax = dh->rightedgetop.x;
	if ( dh->rightedgebottom.x>xmax ) xmax = dh->rightedgebottom.x;
    }
    for ( md=sc->md; md!=NULL; md=md->next ) {
	if ( md->x && md->sp1!=NULL && md->sp1->me.x>xmax )
	    xmax = md->sp1->me.x;
	if ( md->x && md->sp2!=NULL && md->sp2->me.x>xmax )
	    xmax = md->sp2->me.x;
    }
    if ( xmax<sc->width ) {
	for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next) {
	    for ( sp=ss->first ; ; ) {
		if ( sp->me.x>=xmax-1 && sp->me.x<xmax+1 ) {
		    md = chunkalloc(sizeof(MinimumDistance));
		    md->x = true;
		    md->sp1 = sp;
		    md->sp2 = NULL;
		    md->next = sc->md;
		    sc->md = md;
		    sp->dontinterpolate = true;
return;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
    }
    /* Couldn't find a point on the last hint. Oh well, no md */
}

/* Look for simple serifs (a vertical stem to the left or right of a hinted */
/*  stem). Serifs will not be hinted because they overlap the main stem. */
/*  To be a serif it must be a vertical stem that doesn't overlap (in y) the */
/*  hinted stem. It must be to the left of the left edge, or to the right of */
/*  the right edge of the hint. */
/* This is a pretty dumb detector, but it gets simple cases */
/*					 			*/
/*  |		 |		 |		 |		*/
/*  |		 |		 +		 +		*/
/*  |		 |		 |		 |		*/
/*  |		 |		  \		  \		*/
/*  +----+	 +-----+	    -+---+	    -+		*/
/*       |		+		 |	     |		*/
/* +-----+	+------+	 +-------+	+----+		*/
/*					 			*/
/* Simplest case is just a slab serif, all right angles, no extrenious points */
/*  slighlty more complex, we allow one curved point between the two on the */
/*	vertical (now curved) edge (the remaining two cases can also have */
/*	rounded edges on the bottom */
/*  We can also have a curved segment connecting the slab to the stem */
/*  and the curve may be so long that there is no horizontal edge to the slab */
/* (I've only shown one orientation, but obviously these can be flipped) */
static void ptserifcheck(SplineChar *sc,StemInfo *hint, SplinePoint *sp, int xdir) {
    int up, right;
    SplinePoint *serif0, *serif1, *serif2, *serif3;
    int coord= !xdir, other=xdir;

    if ( sp->prev==NULL || sp->next==NULL )
return;

    serif0 = sp;
    if ( (&sp->prev->from->me.x)[coord]==(&sp->me.x)[coord] ) {
	up = ((&sp->prev->from->me.x)[other]<(&sp->me.x)[other]);
	serif1 = sp->next->to;
	if ( serif1->next==NULL )
return;
	serif2 = serif1->next->to;
	if ( (&serif1->me.x)[other]!=(&sp->me.x)[other] ) {
	    if ( up!=((&sp->me.x)[other]<(&serif1->me.x)[other]))
return;
	    /* should be vertical at sp, and horizontal at serif1 */
	    if ( !RealNear(sp->next->splines[coord].c,0) ||
		    !RealNear(3*sp->next->splines[other].a+2*sp->next->splines[other].b+sp->next->splines[other].c,0))
return;
	    serif0 = serif1;
	    serif1 = serif2;
	    if ( (&serif0->me.x)[other]!=(&serif1->me.x)[other] )
		serif1 = serif0;
	    else {
		if ( serif1->next==NULL )
return;
		serif2 = serif1->next->to;
	    }
	}
	if ( serif2->next==NULL )
return;
	serif3 = serif2->next->to;
	if ( (&serif1->me.x)[coord]!=(&serif2->me.x)[coord] && (&serif1->me.x)[coord]==(&serif3->me.x)[coord] ) {
	    /* Allow for one curved point in the serif's "vertical" edge */
	    serif2 = serif3;
	    if ( serif2->next==NULL )
return;
	    serif3 = serif2->next->to;
	}
    } else if ( (&sp->next->to->me.x)[coord]==(&sp->me.x)[coord] ) {
	up = (&sp->next->to->me.x)[other]<(&sp->me.x)[other];
	serif1 = sp->prev->from;
	if ( serif1->prev==NULL )
return;
	serif2 = serif1->prev->from;
	if ( (&serif1->me.x)[other]!=(&sp->me.x)[other] ) {
	    if ( up!=((&sp->me.x)[other]<(&serif1->me.x)[other]))
return;
	    /* should be vertical at sp, and horizontal at serif1 */
	    if ( !RealNear(3*sp->prev->splines[coord].a+2*sp->prev->splines[coord].b+sp->prev->splines[coord].c,0) ||
		    !RealNear(sp->prev->splines[other].c,0))
return;
	    serif0 = serif1;
	    serif1 = serif2;
	    if ( (&serif0->me.x)[other]!=(&serif1->me.x)[other] )
		serif1 = serif0;
	    else {
		if ( serif1->prev==NULL )
return;
		serif2 = serif1->prev->from;
	    }
	}
	if ( serif2->prev==NULL )
return;
	serif3 = serif2->prev->from;
	if ( (&serif1->me.x)[coord]!=(&serif2->me.x)[coord] && (&serif1->me.x)[coord]==(&serif3->me.x)[coord] ) {
	    /* Allow for one curved point in the serif's "vertical" edge */
	    serif2 = serif3;
	    if ( serif2->prev==NULL )
return;
	    serif3 = serif2->prev->from;
	}
    } else		/* no vertical stem here, unlikely to be serifs */
return;

    right = false;
    if ( (&sp->me.x)[coord]>=hint->start-1 && (&sp->me.x)[coord]<=hint->start+1 )
	right = hint->width<0;
    else
	right = hint->width>0;

    /* must go in the right direction... */
    if (( right && (&serif1->me.x)[coord]<=(&sp->me.x)[coord] ) ||
	    ( !right && (&serif1->me.x)[coord]>=(&sp->me.x)[coord] ) ||
	    ( up && (&serif2->me.x)[other]<(&sp->me.x)[other] ) ||
	    ( !up && (&serif2->me.x)[other]>(&sp->me.x)[other] ))
return;
    if ( serif0!=sp ) {
	if (( right && (&serif0->me.x)[coord]<=(&sp->me.x)[coord] ) ||
		( !right && (&serif0->me.x)[coord]>=(&sp->me.x)[coord] ))
return;
    }
    /* Must have a vertical edge between serif1 and serif2 */
    if ( (&serif1->me.x)[coord]!=(&serif2->me.x)[coord] )
return;
    /* Must go in the same vertical direction as the stem's edge */
    if ( serif1->next->to!=serif2 && up != (&serif1->next->to->me.x)[other]<(&serif2->me.x)[other] )
return;
    /* Must return back towards the stem */
    if ( right != ( (&serif2->me.x)[coord]>(&serif3->me.x)[coord] ))
return;

    /* Well, It's probably a serif... */
    /* So create a set of minimum distances for it */
    MDAdd(sc,xdir,sp,serif1);
    MDAdd(sc,xdir,sp,serif2);
    MDAdd(sc,!xdir,serif2,serif0);
}

/* So. We look for all points that lie on the edges of all vertical stem hint */
/*  and see if there are any serifs protruding from them */
static void SCSerifCheck(SplineChar *sc,StemInfo *hint, int xdir) {
    SplineSet *ss;
    SplinePoint *sp;

    for ( ; hint!=NULL ; hint=hint->next ) {
	for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	    for ( sp=ss->first ; ; ) {
		if ( xdir && ((sp->me.x>=hint->start-1 && sp->me.x<=hint->start+1) ||
			(sp->me.x>=hint->start+hint->width-1 && sp->me.x<=hint->start+hint->width+2)) )
		    ptserifcheck(sc,hint,sp,xdir);
		else if (!xdir && ((sp->me.y>=hint->start-1 && sp->me.y<=hint->start+1) ||
			(sp->me.y>=hint->start+hint->width-1 && sp->me.y<=hint->start+hint->width+2)) )
		    ptserifcheck(sc,hint,sp,xdir);
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
    }
}
#endif

static void _SCClearHintMasks(SplineChar *sc,int counterstoo) {
    SplineSet *spl;
    SplinePoint *sp;
    RefChar *ref;

    if ( counterstoo ) {
	free(sc->countermasks);
	sc->countermasks = NULL; sc->countermask_cnt = 0;
    }

    for ( spl = sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	for ( sp = spl->first ; ; ) {
	    chunkfree(sp->hintmask,sizeof(HintMask));
	    sp->hintmask = NULL;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    for ( ref = sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	for ( spl = ref->layers[0].splines; spl!=NULL; spl=spl->next ) {
	    for ( sp = spl->first ; ; ) {
		chunkfree(sp->hintmask,sizeof(HintMask));
		sp->hintmask = NULL;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }
}

static void SCFigureSimpleCounterMasks(SplineChar *sc) {
    SplineChar *scs[MmMax];
    int hadh3, hadv3, i;
    HintMask mask;

    if ( sc->countermask_cnt!=0 )
return;

    scs[0] = sc;
    hadh3 = CvtPsStem3(NULL,scs,1,true,false);
    hadv3 = CvtPsStem3(NULL,scs,1,false,false);
    if ( hadh3 || hadv3 ) {
	memset(mask,0,sizeof(mask));
	if ( hadh3 ) mask[0] = 0x80|0x40|0x20;
	if ( hadv3 ) {
	    for ( i=0; i<3 ; ++i ) {
		int j = i+sc->vstem->hintnumber;
		mask[j>>3] |= (0x80>>(j&7));
	    }
	}
	sc->countermask_cnt = 1;
	sc->countermasks = galloc(sizeof(HintMask));
	memcpy(sc->countermasks[0],mask,sizeof(HintMask));
return;
    }
}

/* find all the other stems (after main) which seem to form a counter group */
/*  with main. That is their stems have a considerable overlap (in the other */
/*  coordinate) with main */
static int stemmatches(StemInfo *main) {
    StemInfo *last=main, *test;
    real mlen, olen;
    int cnt;

    cnt = 1;		/* for the main stem */
    main->tobeused = true;
    mlen = HIlen(main);
    for ( test=main->next; test!=NULL; test=test->next )
	test->tobeused = false;
    for ( test=main->next; test!=NULL; test=test->next ) {
	if ( test->used || last->start+last->width>test->start || test->hintnumber==-1 )
    continue;
	olen = HIoverlap(main->where,test->where);
	if ( olen>mlen/3 && olen>HIlen(test)/3 ) {
	    test->tobeused = true;
	    ++cnt;
	}
    }
return( cnt );
}

static int FigureCounters(StemInfo *stems,HintMask mask ) {
    StemInfo *h, *first;

    while ( stems!=NULL ) {
	for ( first=stems; first!=NULL && first->used; first = first->next );
	if ( first==NULL )
    break;
	if ( first->where==NULL || first->hintnumber==-1 || stemmatches(first)<=2 ) {
	    first->used = true;
	    stems = first->next;
    continue;
	}
	for ( h = first; h!=NULL; h = h->next ) {
	    if ( h->tobeused ) {
		mask[h->hintnumber>>3] |= (0x80>>(h->hintnumber&7));
		h->used = true;
	    }
	}
return( true );
    }
return( false );
}

void SCFigureCounterMasks(SplineChar *sc) {
    HintMask masks[30];
    uint32 script;
    StemInfo *h;
    int mc=0, i;

    /* I'm not supporting counter hints for mm fonts */

    if ( sc==NULL )
return;

    free(sc->countermasks);
    sc->countermask_cnt = 0;
    sc->countermasks = NULL;

    /* Check for h/vstem3 case */
    /* Which is allowed even for non-CJK letters */
    script = SCScriptFromUnicode(sc);
    if ( script==CHR('l','a','t','n') || script==CHR('c','y','r','l') ||
	    script==CHR('g','r','e','k') ) {
	SCFigureSimpleCounterMasks(sc);
return;
    }

    for ( h=sc->hstem; h!=NULL ; h=h->next )
	h->used = false;
    for ( h=sc->vstem; h!=NULL ; h=h->next )
	h->used = false;

    mc = 0;
    
    while ( mc<sizeof(masks)/sizeof(masks[0]) ) {
	memset(masks[mc],'\0',sizeof(HintMask));
	if ( !FigureCounters(sc->hstem,masks[mc]) && !FigureCounters(sc->vstem,masks[mc]))
    break;
	++mc;
    }
    if ( mc!=0 ) {
	sc->countermask_cnt = mc;
	sc->countermasks = galloc(mc*sizeof(HintMask));
	for ( i=0; i<mc ; ++i )
	    memcpy(sc->countermasks[i],masks[i],sizeof(HintMask));
    }
}

void SCClearHintMasks(SplineChar *sc,int counterstoo) {
    MMSet *mm = sc->parent->mm;
    int i;

    if ( mm==NULL )
	_SCClearHintMasks(sc,counterstoo);
    else {
	for ( i=0; i<mm->instance_count; ++i ) {
	    if ( sc->enc<mm->instances[i]->charcnt )
		_SCClearHintMasks(mm->instances[i]->chars[sc->enc],counterstoo);
	}
	if ( sc->enc<mm->normal->charcnt )
	    _SCClearHintMasks(mm->normal->chars[sc->enc],counterstoo);
    }
}

static StemInfo *OnHHint(SplinePoint *sp, StemInfo *s) {
    StemInfo *possible=NULL;
    HintInstance *hi;

    if ( sp==NULL )
return( NULL );

    for ( ; s!=NULL; s=s->next ) {
	if ( sp->me.y<s->start )
return( possible );
	if ( s->start==sp->me.y || s->start+s->width==sp->me.y ) {
	    if ( !s->hasconflicts )
return( s );
	    for ( hi=s->where; hi!=NULL; hi=hi->next ) {
		if ( hi->begin<=sp->me.x && hi->end>=sp->me.x )
return( s );
	    }
	    if ( !s->used )
		possible = s;
	}
    }
return( possible );
}

static StemInfo *OnVHint(SplinePoint *sp, StemInfo *s) {
    StemInfo *possible=NULL;
    HintInstance *hi;

    if ( sp==NULL )
return( NULL );

    for ( ; s!=NULL; s=s->next ) {
	if ( sp->me.x<s->start )
return( possible );
	if ( s->start==sp->me.x || s->start+s->width==sp->me.x ) {
	    if ( !s->hasconflicts )
return( s );
	    for ( hi=s->where; hi!=NULL; hi=hi->next ) {
		if ( hi->begin<=sp->me.y && hi->end>=sp->me.y )
return( s );
	    }
	    if ( !s->used )
		possible = s;
	}
    }
return( possible );
}

/* Does h have a conflict with any of the stems in the list which have bits */
/*  set in the mask */
static int ConflictsWithMask(StemInfo *stems, HintMask mask,StemInfo *h) {
    while ( stems!=NULL && stems->start<h->start+h->width ) {
	if ( stems->start+stems->width>=h->start && stems!=h ) {
	    if ( stems->hintnumber!=-1 &&
		    (mask[stems->hintnumber>>3]&(0x80>>(stems->hintnumber&7))) )
return( true );
	}
	stems = stems->next;
    }
return( false );
}

/* All instances of a MM set must have the same hint mask at all points */
static void FigureHintMask(SplineChar *scs[MmMax], SplinePoint *to[MmMax], int instance_count,
	HintMask mask) {
    StemInfo *s;
    int i;
    SplinePoint *sp;

    memset(mask,'\0',sizeof(HintMask));

    /* Install all hints that are always active */
    i=0; {
	SplineChar *sc = scs[i];

	if ( sc==NULL )
return;

	for ( s=sc->hstem; s!=NULL; s=s->next )
	    if ( s->hintnumber!=-1 && !s->hasconflicts )
		mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));
	for ( s=sc->vstem; s!=NULL; s=s->next )
	    if ( s->hintnumber!=-1 && !s->hasconflicts )
		mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));

	if ( sc->hconflicts ) {
	    for ( sp=to[i]; sp!=NULL; ) {
		s = OnHHint(sp,sc->hstem);
		if ( s!=NULL && s->hintnumber!=-1 ) {
		    if ( ConflictsWithMask(scs[i]->hstem,mask,s))
	    break;
		    mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( to[i]==sp )
	    break;
	    }
	}
	if ( sc->vconflicts ) {
	    for ( sp=to[i]; sp!=NULL; ) {
		s = OnVHint(sp,sc->vstem);
		if ( s!=NULL && s->hintnumber!=-1 ) {
		    if ( ConflictsWithMask(scs[i]->vstem,mask,s))
	    break;
		    mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( to[i]==sp )
	    break;
	    }
	}
    }
    for ( i=0; i<instance_count; ++i ) if ( to[i]!=NULL ) {
	to[i]->hintmask = chunkalloc(sizeof(HintMask));
	memcpy(to[i]->hintmask,mask,sizeof(HintMask));
    }
}

static int TestHintMask(SplineChar *scs[MmMax], SplinePoint *to[MmMax], int instance_count,
	HintMask mask) {
    StemInfo *h=NULL, *v=NULL;
    int i;

    for ( i=0; i<instance_count; ++i ) {
	SplineChar *sc = scs[i];

	if ( sc==NULL || (!sc->hconflicts && !sc->vconflicts ))
    continue;

	/* Does this point lie on any hints? */
	if ( scs[i]->hconflicts )
	    h = OnHHint(to[i],sc->hstem);
	if ( scs[i]->vconflicts )
	    v = OnVHint(to[i],sc->vstem);

	/* Need to set this hint */
	if ( (h!=NULL && h->hintnumber!=-1 && (mask[h->hintnumber>>3]&(0x80>>(h->hintnumber&7)))==0 ) ||
	     (v!=NULL && v->hintnumber!=-1 && (mask[v->hintnumber>>3]&(0x80>>(v->hintnumber&7)))==0 ))
    break;
    }
    if ( i==instance_count )	/* All hint masks were ok */
return( false );

    FigureHintMask(scs,to,instance_count,mask);
return( true );
}

static void UnnumberHints(SplineChar *sc) {
    StemInfo *h;

    for ( h=sc->hstem; h!=NULL; h=h->next )
	h->hintnumber = -1;
    for ( h=sc->vstem; h!=NULL; h=h->next )
	h->hintnumber = -1;
}

static int NumberHints(SplineChar *sc) {
    StemInfo *h;
    int hcnt=0;

    for ( h=sc->hstem; h!=NULL; h=h->next )
	h->hintnumber = hcnt>=HntMax ? -1 : hcnt++;
    for ( h=sc->vstem; h!=NULL; h=h->next )
	h->hintnumber = hcnt>=HntMax ? -1 : hcnt++;
return( hcnt );
}

static void UntickHints(SplineChar *sc) {
    StemInfo *h;

    for ( h=sc->hstem; h!=NULL; h=h->next )
	h->used = false;
    for ( h=sc->vstem; h!=NULL; h=h->next )
	h->used = false;
}

struct coords {
    real coords[MmMax];
    struct coords *next;
};

typedef struct mmh {
    StemInfo *hints[MmMax], *map[MmMax];
    struct coords *where;
    struct mmh *next;
} MMH;

static void AddCoord(MMH *mmh,SplinePoint *sps[MmMax],int instance_count, int ish) {
    struct coords *coords;
    int i;

    coords = chunkalloc(sizeof(struct coords));
    coords->next = mmh->where;
    mmh->where = coords;
    if ( ish )
	for ( i=0; i<instance_count; ++i )
	    coords->coords[i] = sps[i]->me.x;
    else
	for ( i=0; i<instance_count; ++i )
	    coords->coords[i] = sps[i]->me.y;
}

static MMH *AddHintSet(MMH *hints,StemInfo *h[MmMax], int instance_count,
	SplinePoint *sps[MmMax], int ish) {
    int i, cnt, bestc;
    MMH *test, *best;

    for ( i=0; i<instance_count; ++i )
	if ( h[i]==NULL )
return( hints );

    best = NULL; bestc = 0;
    for ( test=hints; test!=NULL; test=test->next ) {
	cnt = 0;
	for ( i=0; i<instance_count; ++i )
	    if ( test->hints[i]==h[i] )
		++cnt;
	if ( cnt==instance_count ) {
	    AddCoord(test,sps,instance_count,ish);
return( hints );
	}
	if ( cnt>bestc ) {
	    bestc = cnt;
	    best = test;
	}
    }
    test = chunkalloc(sizeof(MMH));
    test->next = hints;
    AddCoord(test,sps,instance_count,ish);
    for ( i=0; i<instance_count; ++i )
	test->hints[i]=h[i];
    if ( bestc!=0 ) {
	for ( i=0; i<instance_count; ++i ) {
	    if ( best->hints[i]==h[i] ) {
		h[i]->hasconflicts = true;
		test->map[i] = chunkalloc(sizeof(StemInfo));
		*test->map[i] = *h[i];
		test->map[i]->where = NULL;
		test->map[i]->used = true;
		h[i]->next = test->map[i];
	    } else
		test->map[i] = h[i];
	}
    } else {
	for ( i=0; i<instance_count; ++i )
	    test->map[i]=h[i];
    }
return( test );
}

static int CompareMMH(MMH *mmh1,MMH *mmh2, int instance_count) {
    int i;

    if ( mmh1->map[0]==NULL )
return( 1 );
    if ( mmh2->map[0]==NULL )
return( -1 );

    for ( i=0; i<instance_count; ++i ) {
	if ( mmh1->map[i]->start!=mmh2->map[i]->start ) {
	    if ( mmh1->map[i]->start > mmh2->map[i]->start )
return( 1 );
	    else
return( -1 );
	}
    }
    for ( i=0; i<instance_count; ++i ) {
	if ( mmh1->map[i]->width!=mmh2->map[i]->width ) {
	    if ( mmh1->map[i]->width > mmh2->map[i]->width )
return( 1 );
	    else
return( -1 );
	}
    }
return( 0 );
}
    
static MMH *SortMMH(MMH *head,int instance_count) {
    MMH *mmh, *p, *smallest, *psmallest, *test, *ptest;

    for ( mmh = head, p=NULL; mmh!=NULL ; ) {
	smallest = mmh; psmallest = p;
	ptest = mmh; test = mmh->next;
	while ( test!=NULL ) {
	    if ( CompareMMH(test,smallest,instance_count)<0 ) {
		smallest = test;
		psmallest = ptest;
	    }
	    ptest = test;
	    test = test->next;
	}
	if ( smallest!=mmh ) {
	    if ( p==NULL )
		head = smallest;
	    else
		p->next = smallest;
	    if ( mmh->next==smallest ) {
		mmh->next = smallest->next;
		smallest->next = mmh;
	    } else {
		test = mmh->next;
		mmh->next = smallest->next;
		smallest->next = test;
		psmallest->next = mmh;
	    }
	}
	p = smallest;
	mmh = smallest->next;
    }
return( head );
}

static int NumberMMH(MMH *mmh,int hstart,int instance_count) {
    int i;
    HintInstance *hi, *n;
    struct coords *coords;

    while ( mmh!=NULL ) {
	for ( i=0; i<instance_count; ++i ) {
	    StemInfo *h = mmh->map[i];
	    if ( h==NULL )
	continue;

	    h->hintnumber = hstart;

	    for ( hi=h->where; hi!=NULL; hi=n ) {
		n = hi->next;
		chunkfree(hi,sizeof(HintInstance));
	    }
	    h->where = NULL;
	    for ( coords=mmh->where; coords!=NULL; coords = coords->next ) {
		hi = chunkalloc(sizeof(HintInstance));
		hi->next = h->where;
		h->where = hi;
		hi->begin = coords->coords[i]-1;
		hi->end = coords->coords[i]+1;
	    }
	}
	if ( mmh->map[0]!=NULL ) ++hstart;
	mmh = mmh->next;
    }
return( hstart );
}
	    
static void SortMMH2(SplineChar *scs[MmMax],MMH *mmh,int instance_count,int ish) {
    int i;
    StemInfo *h, *n;
    MMH *m;

    for ( i=0; i<instance_count; ++i ) {
	for ( h= ish ? scs[i]->hstem : scs[i]->vstem; h!=NULL; h=n ) {
	    n = h->next;
	    if ( h->hintnumber==-1 )
		StemInfoFree(h);
	}
	n = NULL;
	for ( m = mmh ; m!=NULL; m=m->next ) {
	    h = m->map[i];
	    if ( n!=NULL )
		n->next = h;
	    else if ( ish )
		scs[i]->hstem = h;
	    else
		scs[i]->vstem = h;
	    n = h;
	}
	if ( n!=NULL )
	    n->next = NULL;
	else if ( ish )
	    scs[i]->hstem = NULL;
	else
	    scs[i]->vstem = NULL;
    }
}

static void MMHFreeList(MMH *mmh) {
    MMH *mn;
    struct coords *c, *n;

    for ( ; mmh!=NULL; mmh = mn ) {
	mn = mmh->next;
	for ( c=mmh->where; c!=NULL; c=n ) {
	    n = c->next;
	    chunkfree(c,sizeof(struct coords));
	}
	chunkfree(mmh,sizeof(struct coords));
    }
}

static void SplResolveSplitHints(SplineChar *scs[MmMax], SplineSet *spl[MmMax],
	int instance_count, MMH **hs, MMH **vs) {
    SplinePoint *to[MmMax];
    StemInfo *h[MmMax], *v[MmMax];
    int i, anymore;

    forever {
	for ( i=0; i<instance_count; ++i ) {
	    if ( spl[i]!=NULL )
		to[i] = spl[i]->first;
	    else
		to[i] = NULL;
	}
	forever {
	    for ( i=0; i<instance_count; ++i ) {
		h[i] = OnHHint(to[i],scs[i]->hstem);
		v[i] = OnVHint(to[i],scs[i]->vstem);
	    }
	    *hs = AddHintSet(*hs,h,instance_count,to,true);
	    *vs = AddHintSet(*vs,v,instance_count,to,false);
	    anymore = false;
	    for ( i=0; i<instance_count; ++i ) if ( to[i]!=NULL ) {
		if ( to[i]->next==NULL ) to[i] = NULL;
		else {
		    to[i] = to[i]->next->to;
		    if ( to[i]==spl[i]->first ) to[i] = NULL;
		}
		if ( to[i]!=NULL ) anymore = true;
	    }
	    if ( !anymore )
	break;
	}
	anymore = false;
	for ( i=0; i<instance_count; ++i ) {
	    if ( spl[i]!=NULL )
		spl[i] = spl[i]->next;
	    if ( spl[i]!=NULL ) anymore = true;
	}
	if ( !anymore )
    break;
    }
}

static void ResolveSplitHints(SplineChar *scs[16],int instance_count) {
    /* It is possible for a single hint in one mm instance to split into two */
    /*  in a different MM set. For example, we have two stems which happen */
    /*  to line up in one instance but which do not in another instance. */
    /* It is even possible that there could be no instance with any conflicts */
    /*  but some of the intermediate forms might conflict. */
    /* We can't deal (nor can postscript) with the case where hints change order*/
    SplinePointList *spl[MmMax];
    RefChar *ref[MmMax];
    int i, hcnt, hmax=0, anymore;
    MMH *hs=NULL, *vs=NULL;

    for ( i=0; i<instance_count; ++i ) {
	hcnt = NumberHints(scs[i]);
	UntickHints(scs[i]);
	if ( hcnt>hmax ) hmax = hcnt;
	spl[i] = scs[i]->layers[ly_fore].splines;
    }
    if ( hmax==0 )
return;

    SplResolveSplitHints(scs,spl,instance_count,&hs,&vs);
    anymore = false;
    for ( i=0; i<instance_count; ++i ) {
	ref[i] = scs[i]->layers[ly_fore].refs;
	if ( ref[i]!=NULL ) anymore = true;
    }
    while ( anymore ) {
	for ( i=0; i<instance_count; ++i )
	    spl[i] = ( ref[i]!=NULL ) ? ref[i]->layers[0].splines : NULL;
	SplResolveSplitHints(scs,spl,instance_count,&hs,&vs);
	anymore = false;
	for ( i=0; i<instance_count; ++i ) {
	    if ( ref[i]!=NULL ) {
		ref[i] = ref[i]->next;
		if ( ref[i]!=NULL ) anymore = true;
	    }
	}
    }

    for ( i=0; i<instance_count; ++i )
	UnnumberHints(scs[i]);
    hs = SortMMH(hs,instance_count);
    vs = SortMMH(vs,instance_count);
    hcnt = NumberMMH(hs,0,instance_count);
    hcnt = NumberMMH(vs,hcnt,instance_count);
    SortMMH2(scs,hs,instance_count,true);
    SortMMH2(scs,vs,instance_count,false);
    MMHFreeList(hs);
    MMHFreeList(vs);
}

static int SplFigureHintMasks(SplineChar *scs[MmMax], SplineSet *spl[MmMax],
	int instance_count, HintMask mask, int inited) {
    SplinePoint *to[MmMax];
    int i, anymore;

    anymore = false;
    for ( i=0; i<instance_count; ++i ) {
	if ( spl[i]!=NULL ) {
	    SplineSetReverse(spl[i]);
	    to[i] = spl[i]->first;
	    anymore = true;
	} else
	    to[i] = NULL;
    }

    /* Assign the initial hint mask */
    if ( anymore && !inited ) {
	FigureHintMask(scs,to,instance_count,mask);
	inited = true;
    }

    forever {
	for ( i=0; i<instance_count; ++i ) {
	    if ( spl[i]!=NULL )
		to[i] = spl[i]->first;
	    else
		to[i] = NULL;
	}
	forever {
	    TestHintMask(scs,to,instance_count,mask);
	    anymore = false;
	    for ( i=0; i<instance_count; ++i ) if ( to[i]!=NULL ) {
		if ( to[i]->next==NULL ) to[i] = NULL;
		else {
		    to[i] = to[i]->next->to;
		    if ( to[i]==spl[i]->first ) to[i] = NULL;
		}
		if ( to[i]!=NULL ) anymore = true;
	    }
	    if ( !anymore )
	break;
	}
	anymore = false;
	for ( i=0; i<instance_count; ++i ) {
	    if ( spl[i]!=NULL ) {
		SplineSetReverse(spl[i]);
		spl[i] = spl[i]->next;
	    }
	    if ( spl[i]!=NULL ) {
		anymore = true;
		SplineSetReverse(spl[i]);
	    }
	}
	if ( !anymore )
    break;
    }
return( inited );
}

void SCFigureHintMasks(SplineChar *sc) {
    SplineChar *scs[MmMax];
    SplinePointList *spl[MmMax];
    RefChar *ref[MmMax];
    MMSet *mm = sc->parent->mm;
    int i, instance_count, conflicts, anymore, inited;
    HintMask mask;

    if ( mm==NULL ) {
	scs[0] = sc;
	instance_count = 1;
	SCClearHintMasks(sc,false);
    } else {
	if ( mm->apple )
return;
	instance_count = mm->instance_count;
	for ( i=0; i<instance_count; ++i )
	    if ( sc->enc < mm->instances[i]->charcnt ) {
		scs[i] = mm->instances[i]->chars[sc->enc];
		SCClearHintMasks(scs[i],false);
	    }
	ResolveSplitHints(scs,instance_count);
    }
    conflicts = false;
    for ( i=0; i<instance_count; ++i ) {
	NumberHints(scs[i]);
	if ( scs[i]->hconflicts || scs[i]->vconflicts )
	    conflicts = true;
    }
    if ( !conflicts && instance_count==1 ) {	/* All hints always active */
	SCFigureSimpleCounterMasks(sc);
return;						/* In an MM font we may still need to resolve things like different numbers of hints */
    }

    for ( i=0; i<instance_count; ++i ) {
	spl[i] = scs[i]->layers[ly_fore].splines;
	ref[i] = scs[i]->layers[ly_fore].refs;
    }
    inited = SplFigureHintMasks(scs,spl,instance_count,mask,false);
    forever {
	for ( i=0; i<instance_count; ++i ) {
	    if ( ref[i]!=NULL )
		spl[i] = ref[i]->layers[0].splines;
	}
	inited = SplFigureHintMasks(scs,spl,instance_count,mask,inited);
	anymore = false;
	for ( i=0; i<instance_count; ++i ) {
	    if ( ref[i]!=NULL ) {
		ref[i] = ref[i]->next;
		if ( ref[i]!=NULL ) anymore = true;
	    }
	}
	if ( !anymore )
    break;
    }
    if ( instance_count==1 )
	SCFigureSimpleCounterMasks(sc);
}

static void _SplineCharAutoHint( SplineChar *sc, int removeOverlaps ) {
    EIList el;

    StemInfosFree(sc->vstem); sc->vstem=NULL;
    StemInfosFree(sc->hstem); sc->hstem=NULL;
    DStemInfosFree(sc->dstem); sc->dstem=NULL;
    MinimumDistancesFree(sc->md); sc->md=NULL;

    free(sc->countermasks);
    sc->countermasks = NULL; sc->countermask_cnt = 0;
    /* We'll free the hintmasks when we call SCFigureHintMasks */

    memset(&el,'\0',sizeof(el));
    ELFindEdges(sc, &el);

    sc->changedsincelasthinted = false;
    sc->manualhints = false;

    sc->vstem = SCFindStems(&el,1,removeOverlaps,&sc->dstem,&sc->md);
    sc->hstem = SCFindStems(&el,0,removeOverlaps,NULL,NULL);
#if 0
    SCSerifCheck(sc,sc->vstem,1);
    SCSerifCheck(sc,sc->hstem,0);
    SCAddWidthMD(sc);
#endif
    AutoHintRefs(sc,removeOverlaps);
    sc->vconflicts = StemListAnyConflicts(sc->vstem);
    sc->hconflicts = StemListAnyConflicts(sc->hstem);

    ElFreeEI(&el);
    SCOutOfDateBackground(sc);
    sc->parent->changed = true;
}

void SplineCharAutoHint( SplineChar *sc, int removeOverlaps ) {
    MMSet *mm = sc->parent->mm;
    int i;

    if ( mm==NULL )
	_SplineCharAutoHint(sc,removeOverlaps);
    else {
	for ( i=0; i<mm->instance_count; ++i )
	    if ( sc->enc < mm->instances[i]->charcnt )
		_SplineCharAutoHint(mm->instances[i]->chars[sc->enc],removeOverlaps);
	if ( sc->enc < mm->normal->charcnt )
	    _SplineCharAutoHint(mm->normal->chars[sc->enc],removeOverlaps);
    }
    SCFigureHintMasks(sc);
    SCUpdateAll(sc);
}

int SFNeedsAutoHint( SplineFont *_sf) {
    int i,k;
    SplineFont *sf;

    k=0;
    do {
	sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    if ( sf->chars[i]->changedsincelasthinted &&
		    !sf->chars[i]->manualhints )
return( true );
	}
	++k;
    } while ( k<_sf->subfontcnt );
return( false );
}
    
void SplineFontAutoHint( SplineFont *_sf) {
    int i,k;
    SplineFont *sf;

    k=0;
    do {
	sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    if ( sf->chars[i]->changedsincelasthinted &&
		    !sf->chars[i]->manualhints )
		SplineCharAutoHint(sf->chars[i],true);
	    if ( !GProgressNext()) {
		k = _sf->subfontcnt+1;
	break;
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
}

static void FigureStems( SplineFont *sf, real snaps[12], real cnts[12],
	int which ) {
    int i, j, k, cnt, smax=0, smin=2000;
    real stemwidths[2000];
    StemInfo *stems, *test;
    int len;
    HintInstance *hi;

    memset(stemwidths,'\0',sizeof(stemwidths));

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	stems = which?sf->chars[i]->hstem:sf->chars[i]->vstem;
	for ( test=stems; test!=NULL; test = test->next ) if ( !test->ghost ) {
	    if ( (j=test->width)<0 ) j= -j;
	    if ( j<2000 ) {
		len = 0;
		for ( hi=test->where; hi!=NULL; hi=hi->next )
		    len += hi->end-hi->begin;
		if ( len==0 ) len = 100;
		stemwidths[j] += len;
		if ( smax<j ) smax=j;
		if ( smin>j ) smin=j;
	    }
	}
    }

    for ( i=smin, cnt=0; i<=smax; ++i ) {
	if ( stemwidths[i]!=0 )
	    ++cnt;
    }

    if ( cnt>12 ) {
	/* Merge width windows */
	int windsize=3, j;
	for ( i=smin; i<=smax; ++i ) if ( stemwidths[i]!=0 ) {
	    if ( (j = i-windsize)<0 ) j=0;
	    for ( ; j<smax && j<=i+windsize; ++j )
		if ( stemwidths[i]<stemwidths[j] )
	    break;
	    if ( j==smax || j>i+windsize ) {
		if ( (j = i-windsize)<0 ) j=0;
		for ( ; j<smax && j<=i+windsize; ++j ) if ( j!=i ) {
		    stemwidths[i] += stemwidths[j];
		    stemwidths[j] = 0;
		}
	    }
	}
	/* Merge adjacent widths */
	for ( i=smin; i<=smax; ++i ) {
	    if ( stemwidths[i]!=0 && i<=smax-1 && stemwidths[i+1]!=0 ) {
		if ( stemwidths[i]>stemwidths[i+1] ) {
		    stemwidths[i] += stemwidths[i+1];
		    stemwidths[i+1] = 0;
		} else {
		    if ( i<=smax-2 && stemwidths[i+2] && stemwidths[i+2]<stemwidths[i+1] ) {
			stemwidths[i+1] += stemwidths[i+2];
			stemwidths[i+2] = 0;
		    }
		    stemwidths[i+1] += stemwidths[i];
		    stemwidths[i] = 0;
		    ++i;
		}
	    }
	}
	for ( i=smin, cnt=0; i<=smax; ++i ) {
	    if ( stemwidths[i]!=0 )
		++cnt;
	}
    }
    if ( cnt<=12 ) {
	for ( i=smin, cnt=0; i<=smax; ++i ) {
	    if ( stemwidths[i]!=0 ) {
		snaps[cnt] = i;
		cnts[cnt++] = stemwidths[i];
	    }
	}
    } else { real firstbiggest=0;
	for ( cnt = 0; cnt<12; ++cnt ) {
	    int biggesti=0;
	    real biggest=0;
	    for ( i=smin; i<=smax; ++i ) {
		if ( stemwidths[i]>biggest ) { biggest = stemwidths[i]; biggesti=i; }
	    }
	    /* array must be sorted */
	    if ( biggest<firstbiggest/6 )
	break;
	    for ( j=0; j<cnt; ++j )
		if ( snaps[j]>biggesti )
	    break;
	    for ( k=cnt-1; k>=j; --k ) {
		snaps[k+1] = snaps[k];
		cnts[k+1]=cnts[k];
	    }
	    snaps[j] = biggesti;
	    cnts[j] = biggest;
	    stemwidths[biggesti] = 0;
	    if ( firstbiggest==0 ) firstbiggest = biggest;
	}
    }
    for ( ; cnt<12; ++cnt ) {
	snaps[cnt] = 0;
	cnts[cnt] = 0;
    }
}

void FindHStems( SplineFont *sf, real snaps[12], real cnt[12]) {
    FigureStems(sf,snaps,cnt,1);
}

void FindVStems( SplineFont *sf, real snaps[12], real cnt[12]) {
    FigureStems(sf,snaps,cnt,0);
}

static int _SplineCharIsFlexible(SplineChar *sc, int blueshift) {
    /* Need two splines
	outer endpoints have same x (or y) values
	inner point must be less than 20 horizontal (v) units from the outer points
	inner point must also be less than BlueShift units (defaults to 7=>6)
	    (can increase BlueShift up to 21)
	the inner point must be a local extremum
		(something incomprehensible is written about the control points)
		perhaps that they should have the same x(y) value as the center
    */
    SplineSet *spl;
    SplinePoint *sp, *np, *pp;
    int max=0, val;

    if ( sc==NULL )
return(false);

    for ( spl = sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	if ( spl->first->prev==NULL ) {
	    /* Mark everything on the open path as inflexible */
	    sp=spl->first;
	    while ( 1 ) {
		sp->flexx = sp->flexy = false;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } 
    continue;				/* Ignore open paths */
	}
	sp=spl->first;
	do {
	    if ( sp->next==NULL || sp->prev==NULL )
	break;
	    np = sp->next->to;
	    pp = sp->prev->from;
	    if ( !pp->flexx && !pp->flexy ) {
		sp->flexy = sp->flexx = 0;
		val = 0;
		if ( RealNear(sp->nextcp.x,sp->me.x) &&
			RealNear(sp->prevcp.x,sp->me.x) &&
			RealNear(np->me.x,pp->me.x) &&
			!RealNear(np->me.x,sp->me.x) &&
			np->me.x-sp->me.x < blueshift &&
			np->me.x-sp->me.x > -blueshift ) {
		    if ( (np->me.x>sp->me.x &&
			    np->prevcp.x<=np->me.x && np->prevcp.x>=sp->me.x &&
			    pp->nextcp.x<=pp->me.x && pp->prevcp.x>=sp->me.x ) ||
			  (np->me.x<sp->me.x &&
			    np->prevcp.x>=np->me.x && np->prevcp.x<=sp->me.x &&
			    pp->nextcp.x>=pp->me.x && pp->prevcp.x<=sp->me.x )) {
			sp->flexx = true;
			val = np->me.x-sp->me.x;
		    }
		}
		if ( RealNear(sp->nextcp.y,sp->me.y) &&
			RealNear(sp->prevcp.y,sp->me.y) &&
			RealNear(np->me.y,pp->me.y) &&
			!RealNear(np->me.y,sp->me.y) &&
			np->me.y-sp->me.y < blueshift &&
			np->me.y-sp->me.y > -blueshift ) {
		    if ( (np->me.y>sp->me.y &&
			    np->prevcp.y<=np->me.y && np->prevcp.y>=sp->me.y &&
			    pp->nextcp.y<=pp->me.y && pp->nextcp.y>=sp->me.y ) ||
			  (np->me.y<sp->me.y &&
			    np->prevcp.y>=np->me.y && np->prevcp.y<=sp->me.y &&
			    pp->nextcp.y>=pp->me.y && pp->nextcp.y<=sp->me.y )) {
			sp->flexy = true;
			val = np->me.y-sp->me.y;
		    }
		}
		if ( val<0 ) val = -val;
		if ( val>max ) max = val;
	    }
	    sp = np;
	} while ( sp!=spl->first );
    }
    sc->anyflexes = max>0;
return( max );
}

static int MatchFlexes(MMSet *mm,int enc) {
    int any=false, i;
    SplineSet *spl[16];
    SplinePoint *sp[16];
    int mismatchx, mismatchy;

    for ( i=0; i<mm->instance_count; ++i )
	if ( enc<mm->instances[i]->charcnt && mm->instances[i]->chars[enc]!=NULL )
	    spl[i] = mm->instances[i]->chars[enc]->layers[ly_fore].splines;
	else
	    spl[i] = NULL;
    while ( spl[0]!=NULL ) {
	for ( i=0; i<mm->instance_count; ++i )
	    if ( spl[i]!=NULL )
		sp[i] = spl[i]->first;
	    else
		sp[i] = NULL;
	while ( sp[0]!=NULL ) {
	    mismatchx = mismatchy = false;
	    for ( i=1 ; i<mm->instance_count; ++i ) {
		if ( sp[i]==NULL )
		    mismatchx = mismatchy = true;
		else {
		    if ( sp[i]->flexx != sp[0]->flexx )
			mismatchx = true;
		    if ( sp[i]->flexy != sp[0]->flexy )
			mismatchy = true;
		}
	    }
	    if ( mismatchx || mismatchy ) {
		for ( i=0 ; i<mm->instance_count; ++i ) if ( sp[i]!=NULL ) {
		    if ( mismatchx ) sp[i]->flexx = false;
		    if ( mismatchy ) sp[i]->flexy = false;
		}
	    }
	    if ( sp[0]->flexx || sp[0]->flexy )
		any = true;
	    for ( i=0 ; i<mm->instance_count; ++i ) if ( sp[i]!=NULL ) {
		if ( sp[i]->next==NULL ) sp[i] = NULL;
		else sp[i] = sp[i]->next->to;
	    }
	    if ( sp[0] == spl[0]->first )
	break;
	}
	for ( i=0; i<mm->instance_count; ++i )
	    if ( spl[i]!=NULL )
		spl[i] = spl[i]->next;
    }
return( any );
}

int SplineCharIsFlexible(SplineChar *sc) {
    char *pt;
    int blueshift;
    int i;
    MMSet *mm;

    pt = PSDictHasEntry(sc->parent->private,"BlueShift");
    blueshift = 7;		/* use default value here */
    if ( pt!=NULL ) {
	blueshift = strtol(pt,NULL,10);
	if ( blueshift>21 ) blueshift = 21;
    } else if ( PSDictHasEntry(sc->parent->private,"BlueValues")!=NULL )
	blueshift = 7;
    if ( sc->parent->mm==NULL )
return( _SplineCharIsFlexible(sc,blueshift));

    mm = sc->parent->mm;
    for ( i = 0; i<mm->instance_count; ++i )
	if ( sc->enc<mm->instances[i]->charcnt && mm->instances[i]->chars[sc->enc]!=NULL )
	    _SplineCharIsFlexible(mm->instances[i]->chars[sc->enc],blueshift);
return( MatchFlexes(mm,sc->enc));	
}

static void SCUnflex(SplineChar *sc) {
    SplineSet *spl;
    SplinePoint *sp;

    for ( spl = sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	/* Mark everything on the path as inflexible */
	sp=spl->first;
	while ( 1 ) {
	    sp->flexx = sp->flexy = false;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	} 
    }
    sc->anyflexes = false;
}

int SplineFontIsFlexible(SplineFont *sf,int flags) {
    int i;
    int max=0, val;
    char *pt;
    int blueshift;
    /* if the return value is bigger than 6 and we don't have a BlueShift */
    /*  then we must set BlueShift to ret+1 before saving private dictionary */
    /* If the first point in a spline set is flexible, then we must rotate */
    /*  the splineset */

    if ( flags&(ps_flag_nohints|ps_flag_noflex)) {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	    SCUnflex(sf->chars[i]);
return( 0 );
    }
	
    pt = PSDictHasEntry(sf->private,"BlueShift");
    blueshift = 21;		/* maximum posible flex, not default */
    if ( pt!=NULL ) {
	blueshift = strtol(pt,NULL,10);
	if ( blueshift>21 ) blueshift = 21;
    } else if ( PSDictHasEntry(sf->private,"BlueValues")!=NULL )
	blueshift = 7;	/* The BlueValues array may depend on BlueShift having its default value */

    for ( i=0; i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL ) {
	    val = _SplineCharIsFlexible(sf->chars[i],blueshift);
	    if ( val>max ) max = val;
	}
return( max );
}
