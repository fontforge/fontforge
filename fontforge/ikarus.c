/* Copyright (C) 2000-2012 by George Williams */
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

#include "ikarus.h"

#include "fontforge.h"
#include "namelist.h"
#include "mem.h"
#include "splineorder2.h"
#include "splineutil2.h"
#include <utype.h>
#include <string.h>
#include <ustring.h>

/* Ikarus formats defined in Peter Karow's book "Digital formats for typefaces" */
/*  URW Verlag, Hamburg, 1987. The IK format defined in Appendices G&I */
/* Possibly available from URW++ http://www.urwpp.de/english/home.html */

static int urwtable[2000];
static int urw_inited;

static void InitURWTable(void) {
    int i;

    if ( urw_inited )
return;
    memset(urwtable,-1,sizeof(urwtable));
    for ( i=0; i<26; ++i ) {
	urwtable[i+101] = 'A'+i;
	urwtable[i+301] = 'a'+i;
    }
    urwtable[127] = 0xc6;
    urwtable[128] = 0x152;
    urwtable[129] = 0xd8;
    urwtable[200] = 0x132;
    urwtable[201] = 0xc4;
    urwtable[202] = 0xc1;
    urwtable[203] = 0xc0;
    urwtable[204] = 0xc2;
    urwtable[205] = 0x1cd;
    urwtable[206] = 0x102;
    urwtable[207] = 0xc3;
    urwtable[208] = 0xc5;
    urwtable[209] = 0x104;
    urwtable[210] = 0xc7;
    urwtable[211] = 0x106;
    urwtable[212] = 0x10c;
    urwtable[213] = 0x10a;
    urwtable[214] = 0x10e;
    urwtable[215] = 0x110;
    urwtable[216] = 0xcb;
    urwtable[217] = 0xc9;
    urwtable[218] = 0xc8;
    urwtable[219] = 0xca;
    urwtable[220] = 0x11a;
    urwtable[221] = 0x116;
    urwtable[222] = 0x118;
    urwtable[223] = 0x1e6;
    urwtable[224] = 0x11e;
    urwtable[225] = 0x120;
    urwtable[226] = 0xcf;
    urwtable[227] = 0xcd;
    urwtable[228] = 0xcc;
    urwtable[229] = 0xce;
    urwtable[230] = 0x130;
    urwtable[231] = 0x139;
    urwtable[233] = 0x141;
    urwtable[234] = 0x143;
    urwtable[235] = 0x147;
    urwtable[236] = 0xd1;
    urwtable[237] = 0xd6;
    urwtable[238] = 0xd3;
    urwtable[239] = 0xd2;
    urwtable[240] = 0xd4;
    urwtable[241] = 0xd5;
    urwtable[242] = 0x150;
    urwtable[243] = 0x154;
    urwtable[244] = 0x15a;
    urwtable[246] = 0x160;
    urwtable[248] = 0x15e;
    urwtable[249] = 0x164;
    urwtable[251] = 0xdc;
    urwtable[252] = 0xda;
    urwtable[253] = 0xd9;
    urwtable[254] = 0xdb;
    urwtable[255] = 0xdc;
    urwtable[256] = 0x170;
    urwtable[257] = 0xdd;
    urwtable[258] = 0x179;
    urwtable[259] = 0x17d;
    urwtable[260] = 0x17b;
    urwtable[261] = 0xde;
    urwtable[262] = 0x100;
    urwtable[264] = 0x108;
    urwtable[265] = 0x174;
    urwtable[266] = 0x1e84;
    urwtable[267] = 0x176;
    urwtable[268] = 0x178;
    urwtable[269] = 0x126;
    urwtable[272] = 0x112;
    urwtable[273] = 0x11c;
    urwtable[274] = 0x124;
    urwtable[276] = 0x12e;
    urwtable[283] = 0x14c;
    urwtable[286] = 0x15c;
    urwtable[289] = 0x1d3;
    urwtable[290] = 0x16a;
    urwtable[291] = 0x172;
    urwtable[296] = 0x114;
    urwtable[297] = 0x1e1e;
    urwtable[298] = 0x1f4;
    urwtable[327] = 0xe6;
    urwtable[328] = 0x153;
    urwtable[329] = 0xf8;
    urwtable[330] = 0xdf;
    urwtable[331] = 0x131;
    urwtable[332] = 0x237 /*0xf6be*/;
    urwtable[333] = 0x133;
    urwtable[336] = 0xfb00;
    urwtable[337] = 0xfb01;
    urwtable[338] = 0xfb02;
	/* 339 is ft lig */
	/* 340 is fff lig */
    urwtable[341] = 0xfb03;
    urwtable[342] = 0xfb04;
	/* 343 is fft lig */
    urwtable[401] = 0xe4;
    urwtable[402] = 0xe1;
    urwtable[402] = 0xe0;
    urwtable[404] = 0xe2;
    urwtable[405] = 0x1ce;
    urwtable[406] = 0x103;
    urwtable[407] = 0xe3;
    urwtable[408] = 0xe5;
    urwtable[409] = 0x105;
    urwtable[410] = 0x107;
    urwtable[411] = 0x10d;
    urwtable[412] = 0x10b;
    urwtable[413] = 0xe7;
    urwtable[415] = 0x111;
    urwtable[416] = 0xeb;
    urwtable[417] = 0xe9;
    urwtable[418] = 0xe8;
    urwtable[419] = 0xea;
    urwtable[420] = 0x119;
    urwtable[421] = 0x117;
    urwtable[423] = 0x1e7;
    urwtable[424] = 0x11f;
    urwtable[425] = 0x121;
    urwtable[426] = 0xef;
    urwtable[427] = 0xed;
    urwtable[428] = 0xec;
    urwtable[429] = 0xee;
    urwtable[432] = 0x142;
    urwtable[433] = 0x144;
    urwtable[434] = 0x148;
    urwtable[435] = 0xf1;
    urwtable[436] = 0xf6;
    urwtable[437] = 0xf3;
    urwtable[438] = 0xf2;
    urwtable[439] = 0xf4;
    urwtable[440] = 0xf5;
    urwtable[441] = 0x151;
    urwtable[444] = 0x15b;
    urwtable[445] = 0x15d;
    urwtable[446] = 0x161;
    urwtable[449] = 0xfc;
    urwtable[450] = 0xfa;
    urwtable[451] = 0xf9;
    urwtable[452] = 0xfb;
    urwtable[453] = 0x16f;
    urwtable[454] = 0x171;
    urwtable[455] = 0xfd;
    urwtable[456] = 0x17a;
    urwtable[457] = 0x17e;
    urwtable[458] = 0x17c;
    urwtable[459] = 0xf0;
    urwtable[460] = 0xfe;
    urwtable[461] = 0xff;
    urwtable[465] = 0x1e85;
    urwtable[462] = 0x127;
    urwtable[464] = 0x175;
    urwtable[466] = 0x101;
    urwtable[467] = 0x177;
    urwtable[469] = 0x109;
    urwtable[472] = 0x113;
    urwtable[473] = 0x11d;
    urwtable[480] = 0x1e3f;
    urwtable[482] = 0x14d;
    urwtable[486] = 0x16b;
    urwtable[487] = 0x173;
    urwtable[489] = 0x15f;
    for ( i=1; i<=9; ++i )
	urwtable[500+i] = '0'+i;
    urwtable[510] = '0';
    urwtable[511] = 0xa3;
    urwtable[512] = '$';
    urwtable[513] = 0xa2;
    urwtable[514] = 0x192;
    urwtable[516] = 0xa5;
    urwtable[519] = 0x20a4;
    urwtable[523] = 0x20a7;
    urwtable[524] = 0x20a3;
    urwtable[575] = 0xb9;
    urwtable[576] = 0xb2;
    urwtable[577] = 0xb3;
    for ( i=4; i<=9; ++i )
	urwtable[574+i] = 0x2070+i;
    urwtable[584] = 0x2070;
    for ( i=1; i<=9; ++i )
	urwtable[586+i] = 0x2080+i;
    urwtable[596] = 0x2080;
    
    urwtable[601] = '.';
    urwtable[602] = ':';
    urwtable[606] = 0x2026;
    urwtable[607] = ',';
    urwtable[608] = ';';
    urwtable[609] = 0x2019;
    urwtable[610] = 0x2018;
    urwtable[611] = 0x201d;
    urwtable[612] = 0x201c;
    urwtable[613] = 0x201e;
    urwtable[614] = '!';
    urwtable[615] = 0xa1;
    urwtable[616] = '?';
    urwtable[617] = 0xbf;
    urwtable[618] = 0xbb;
    urwtable[619] = 0xab;
    urwtable[620] = 0x203a;
    urwtable[621] = 0x203a;
    urwtable[622] = '/';
    urwtable[623] = 0x2010;
    urwtable[624] = 0x2013;
    urwtable[625] = 0x2014;
    urwtable[626] = '(';
    urwtable[627] = ')';
    urwtable[628] = '[';
    urwtable[629] = ']';
    urwtable[630] = '&';
    urwtable[631] = 0xa7;
    urwtable[632] = 0x2020;
    urwtable[633] = 0x2021;
    urwtable[634] = '*';
    urwtable[635] = '\'';
    urwtable[636] = '"';
    urwtable[637] = '@';
    urwtable[638] = '#';
    urwtable[639] = 0xb0;
    urwtable[640] = '+';
    urwtable[641] = '-';
    urwtable[642] = 0xd7;
    urwtable[643] = 0xf7;
    urwtable[644] = '=';
    urwtable[647] = ':';
    urwtable[648] = 0xa9;
    urwtable[649] = 0xae;
    urwtable[650] = 0x2122;
    urwtable[651] = 0xb6;
    urwtable[652] = 0xa4;
    urwtable[655] = '{';
    urwtable[656] = '}';
    urwtable[657] = 0xaa;
    urwtable[658] = 0xba;
    urwtable[659] = 0xb1;
    urwtable[662] = 0xbd;
    urwtable[663] = 0x2153;
    urwtable[664] = 0x2154;
    urwtable[665] = 0xbc;
    urwtable[666] = 0xbe;
    urwtable[667] = 0x2155;
    for ( i=0; i<=0x215e -0x2155; ++i )
	urwtable[668+i] = 0x215e +i;
    urwtable[677] = 0x2044;
    urwtable[678] = '%';
    urwtable[679] = 0x2030;
	/* 680 and 681 are some sort of varient on % and per mill */
    urwtable[700] = '\\';
    urwtable[701] = 0xa8;
    urwtable[702] = 0x2d9;
    urwtable[703] = 0x2da;
    urwtable[704] = 0x2ca;
    urwtable[705] = 0x2cb;
    urwtable[706] = 0x2c6;
    urwtable[707] = 0x2c7;
    urwtable[708] = 0x2d8;
    urwtable[709] = 0x2dc;
    urwtable[710] = 0x2dd;
    urwtable[711] = 0xb8;
    urwtable[712] = 0x2db;
    urwtable[713] = 0xaf;
    urwtable[714] = 0xaf;
	/* 751-764 seem to be varients on 701-714 */
    urwtable[765] = 0x323;
    urwtable[766] = 0x320;
    urwtable[768] = 0x326;
    urwtable[769] = 0x313;    
	/* 795 is a big copyright */
	/* 796 is a big registered */
    urwtable[854] = 0x1ebd3;
    urwtable[857] = 0x1f5;
    urwtable[863] = 0x12f;
    urwtable[866] = 0x140;
    urwtable[870] = 0x1e45;
    urwtable[873] = 0x1d2;
    urwtable[886] = 0x16b;
    urwtable[892] = 0x1e8f;
    urwtable[897] = 0x1d4;
    urwtable[903] = 0x13f;
    urwtable[905] = 0x1e44;
    urwtable[908] = 0x14e;
    urwtable[916] = 0x16c;
    urwtable[933] = 0x145;
    urwtable[934] = 0x166;
    urwtable[935] = 0x146;
    urwtable[936] = 0x147;
    urwtable[937] = 0x138;
    urwtable[939] = 0x168;
    urwtable[1003] = 0x2665;
    urwtable[1004] = 0x2666;
    urwtable[1005] = 0x2663;
    urwtable[1006] = 0x2660;
    urwtable[1011] = 0x2642;
    urwtable[1012] = 0x2640;
    urwtable[1016] = 0x2022;
    urwtable[1036] = 0x2192;
    urwtable[1037] = 0x2190;
    urwtable[1038] = 0x2194;
    urwtable[1039] = 0x2191;
    urwtable[1040] = 0x2193;
    urwtable[1041] = 0x2195;
    urwtable[1049] = 0x25ca;
	/* 1059 is an apple character (which has no unicode equiv) */
    urwtable[1085] = 0x2206;
    urwtable[1101] = 0x2260;
    urwtable[1103] = 0x2261;
    urwtable[1104] = 0x221a;
    urwtable[1105] = 0x222b;
    urwtable[1108] = 0x223c;
    urwtable[1109] = 0x2248;
    urwtable[1111] = '<';
    urwtable[1112] = '>';
    urwtable[1113] = 0x2264;
    urwtable[1114] = 0x2265;
    urwtable[1117] = 0xac;
    urwtable[1124] = 0x221e;
    urwtable[1133] = 0x220f;
    urwtable[1134] = 0x2211;
    urwtable[1137] = 0x2202;
    urwtable[1151] = '^';
    urwtable[1152] = '_';
    urwtable[1199] = 0x3c0;
    urwtable[1337] = '~';
    urwtable[1342] = 0xb5;
    urwtable[1376] = '|';
	/* 1405-1408 is another set of +,-,=,divide */
    urwtable[1412] = 0x2126;

    urw_inited = true;
}

static void IkarusNameFromURWNumber(SplineChar *sc,int number) {
    char buf[200];

    if ( !urw_inited )
	InitURWTable();

    free(sc->name); sc->name = NULL;
    if ( number<sizeof(urwtable)/sizeof(urwtable[0]) ) {
	sc->unicodeenc = urwtable[number];
	if ( sc->unicodeenc!=-1 ) {
	    sc->name = copy(StdGlyphName(buf,sc->unicodeenc,ui_none,NULL));
return;
	}
    }
    if ( number==1059 )
	sc->name = copy("apple");
    else if ( number==795 )
	sc->name = copy("copyright.big");
    else if ( number==796 )
	sc->name = copy("registered.big");
    else {
	sprintf(buf, "urw%d", number );
	sc->name = copy(buf);
    }
}

static void IkarusAddContour(SplineChar *sc,int npts,BasePoint *bps,
	uint8 *ptype, int nesting) {
    SplinePointList *spl;
    SplinePoint *last, *next;
    int i, cw;

    spl = chunkalloc(sizeof(SplinePointList));
    spl->next = sc->layers[ly_fore].splines;
    sc->layers[ly_fore].splines = spl;
    spl->first = spl->last = last = SplinePointCreate(bps[0].x,bps[0].y);
    last->pointtype = ptype[npts-1];
    last->nextcpdef = last->prevcpdef = true;
    for ( i=1; i<npts-1; ++i ) {
	next = SplinePointCreate(bps[i].x,bps[i].y);
	next->nextcpdef = next->prevcpdef = true;
	next->pointtype = ptype[i];
	SplineMake3(last,next);
	last = next;
    }
    SplineMake3(last,spl->last);
    last = spl->first;
    for ( i=1; i<npts; ++i ) {
	SplineCharDefaultPrevCP(last);
	SplineCharDefaultNextCP(last);
	last=last->next->to;
    }

    cw = SplinePointListIsClockwise(spl)==1;
    if ( ((nesting&1) && cw) || (!(nesting&1) && !cw) )
	SplineSetReverse(spl);
}

static void IkarusReadChar(SplineChar *sc,FILE *file) {
    int n, i, j, number, following, units, ncontours, ptmax;
    DBounds bb;
    int32 base;
    struct contour {
	int32 offset;
	int dir, nest, col, npts;
    } *contours;
    BasePoint *bps;
    uint8 *ptype;
    int x,y;

    /* record len of char = */ getushort(file);
    /* word len of char = */ getushort(file);

    n = getushort(file);
    number = getushort(file);
    IkarusNameFromURWNumber(sc,number);
    following = getushort(file);
    if ( following!=0 )
	LogError( _("This character (gid=%d) has a following part (%d). I'm not sure what that means, please send me (gww@silcom.com) a copy of this font so I can test with it.\n"),
		sc->orig_pos, following );
    for ( i=3; i<n ; ++i )
	getushort(file);	/* Just in case the name section is bigger now */

    n = getushort(file);
    /* sort (leter,logo,line graphic,frame etc.) = */ getushort(file);
    /* num points = */ getushort(file);
    sc->width = getushort(file);
    /* lsb = */ getushort(file);
    /* xmax-xmin = */ getushort(file);
    /* rsb = */ getushort(file);
    bb.minx = (short) getushort(file);
    bb.maxx = (short) getushort(file);
    bb.miny = (short) getushort(file);
    bb.maxy = (short) getushort(file);
    for ( i=12 ; i<n; ++i )
	getushort(file);

    units = getushort(file);
    if ( units!=1 )
	sc->width *= units;

    n = getushort(file)*2048;
    n += getushort(file);		/* Length of contour section in words */
    ncontours = (n-2)/6;
    contours = malloc(ncontours*sizeof(struct contour));
    ptmax = 0;
    for ( i=0; i<ncontours; ++i ) {
	contours[i].offset = getushort(file)*4096;
	contours[i].offset += 2*getushort(file)-2;
	contours[i].dir = getushort(file);
	contours[i].nest = getushort(file);
	contours[i].col = getushort(file);
	contours[i].npts = getushort(file);
	if ( contours[i].npts > ptmax )
	    ptmax = contours[i].npts;
    }
    bps = malloc(ptmax*sizeof(BasePoint));
    ptype = malloc(ptmax*sizeof(uint8));

    base = ftell(file);
    /* 2 words here giving length (in records/words) of image data */

    for ( i=0; i<ncontours; ++i ) {
	fseek(file,base+contours[i].offset,SEEK_SET);
	for ( j=0; j<contours[i].npts; ++j ) {
	    x = (short) getushort(file);
	    y = (short) getushort(file);
	    if ( x<0 && y>0 )
		ptype[j] = -1;		/* Start point */
	    else if ( x<0 /* && y<0 */)
		ptype[j] = pt_corner;
	    else if ( /* x>0 && */ y>0 )
		ptype[j] = pt_curve;
	    else /* if ( x>0 && y<0 ) */
		ptype[j] = pt_tangent;
	    if ( x<0 ) x=-x;
	    if ( y<0 ) y=-y;
	    x += bb.minx-1;
	    y += bb.miny-1;
	    bps[j].x = units*x; bps[j].y = units*y;
	}
	IkarusAddContour(sc,contours[i].npts,bps,ptype,contours[i].nest);
    }

    free(contours);
    free(bps);
    free(ptype);
}

static void IkarusFontname(SplineFont *sf,char *fullname,char *fnam) {
    char *pt, *tpt;

    for ( pt=fullname; *pt==' '; ++pt );
    if ( *pt=='\0' ) {
	/* Often the fontname is blank */
	pt = strchr(fnam,'.');
	if ( pt!=NULL ) *pt='\0';
	for ( pt=fnam; *pt==' '; ++pt );
	if ( *pt!='\0' )
	    strcpy(fullname,fnam);
	else {
	    pt = strrchr(sf->origname,'/');
	    if ( pt==NULL ) pt = sf->origname-1;
	    strncpy(fullname,pt,80);
	    fullname[80] = '\0';
	    pt = strchr(fullname,'.');
	    if ( pt!=NULL ) *pt = '\0';
	}
    }

    free(sf->fullname);
    sf->fullname=copy(fullname);

    free(sf->fontname);
    sf->fontname=copy(fullname);
    for ( pt=tpt=sf->fontname; *pt; ++pt ) {
	if ( isalnum(*pt) || *pt=='-' || *pt=='_' || *pt=='$' )
	    *tpt++ = *pt;
    }
    *tpt = '\0';

    if ( (pt=strstr(fullname,"Regu"))!=NULL ||
	    (pt=strstr(fullname,"Medi"))!=NULL ||
	    (pt=strstr(fullname,"Bold"))!=NULL ||
	    (pt=strstr(fullname,"Demi"))!=NULL ||
	    (pt=strstr(fullname,"Gras"))!=NULL ||
	    (pt=strstr(fullname,"Fett"))!=NULL ||
	    (pt=strstr(fullname,"Norm"))!=NULL ||
	    (pt=strstr(fullname,"Nord"))!=NULL ||
	    (pt=strstr(fullname,"Heav"))!=NULL ||
	    (pt=strstr(fullname,"Blac"))!=NULL ||
	    (pt=strstr(fullname,"Ligh"))!=NULL ||
	    (pt=strstr(fullname,"Thin"))!=NULL ) {
	free(sf->weight);
	sf->weight = copyn(pt,4);
	*pt='\0';
    }
    while ( (pt=strstr(fullname,"Ital"))!=NULL ||
	    (pt=strstr(fullname,"Slanted"))!=NULL ||
	    (pt=strstr(fullname,"Obli"))!=NULL ||
	    (pt=strstr(fullname,"Roma"))!=NULL ||
	    (pt=strstr(fullname,"Cond"))!=NULL ||
	    (pt=strstr(fullname,"Expa"))!=NULL ) {
	*pt='\0';
    }
    free(sf->familyname);
    sf->familyname = copy(fullname);
}

SplineFont *SFReadIkarus(char *fontname) {
    SplineFont *sf;
    FILE *file = fopen(fontname,"rb");
    int ch1, ch2, rpos, wpos, i;
    int hlen, ilen, jlen, llen, mlen;
    int numchars, maxnum, opt_pt_size;
    double italic_angle;
    char fnam[13], fullname[81];
    int32 *offsets, *numbers;

    if ( file==NULL )
return( NULL );
    hlen = getushort(file);		/* Length of font header */
    ilen = getushort(file);		/* Length of name section */
    getushort(file);			/* Number on URW list */
    fread(fnam,1,12,file);		/* 6 words of filename */
    fread(fullname,1,80,file);		/* 40 words of fontname (human readable) */
    fnam[12] = fullname[80] = '\0';
    ch1 = getc(file);
    ch2 = getc(file);
    if ( ch1=='i' || ch2=='k' )
	/* Docs don't mention this, but lower case is ok too */;
    else if ( ch1!='I' || ch2!='K' ) {
	if ( (ch1=='D' && ch2=='I') || (ch1=='V' && ch2=='C') ||
		(ch1=='V' && ch2=='S') || (ch1=='V' && ch2=='E') || 
		(ch1=='S' && ch2=='C') || (ch1=='S' && ch2=='N') || 
		(ch1=='B' && ch2=='I') || (ch1=='G' && isdigit(ch2)) ||
		(ch1=='d' && ch2=='i') || (ch1=='v' && ch2=='c') ||
		(ch1=='v' && ch2=='s') || (ch1=='v' && ch2=='e') || 
		(ch1=='s' && ch2=='c') || (ch1=='s' && ch2=='n') || 
		(ch1=='b' && ch2=='i') || (ch1=='g' && isdigit(ch2)))
	    LogError( _("This is probably a valid URW font, but it is in a format (%c%c) which FontForge\ndoes not support. FontForge only supports 'IK' format fonts.\n"), ch1, ch2 );
	else if ( ch1==0 && ch2==0 && ilen==55 )
	    LogError( _("This looks like an ikarus format which I have seen examples of, but for which\nI have no documentation. FontForge does not support it yet.\n") );
	fclose(file);
return( NULL );
    } else if ( ilen<55 || hlen<=ilen ) {
	fclose(file);
return( NULL );
    }
    if ( ilen!=55 )
	LogError( _("Unexpected size for name section of URW font (expected 55, got %d)\n"), ilen );

    fseek(file,2*ilen+2,SEEK_SET);
    jlen = getushort(file);
    if ( jlen<12 || hlen<=jlen+ilen ) {
	fclose(file);
return( NULL );
    }
    if ( jlen!=12 )
	LogError( _("Unexpected size for font info section of URW font (expected 12, got %d)\n"), jlen );
    if ( getushort(file)!=1 ) {		/* 1=> typeface */
	fclose(file);
return( NULL );
    }
    numchars = getushort(file);
    /* cap height = */ getushort(file);
    /* body size = */ getushort(file);
    /* x-height = */ getushort(file);
    /* descender? = */ getushort(file);
    /* line thickness = */ getushort(file);
    /* stroke thickness = */ getushort(file);
    italic_angle = getushort(file)/10.0 * 3.1415926535897932/180.0;
    opt_pt_size = getushort(file);
    /* average char width = */ getushort(file);

    fseek(file,2*ilen+2*jlen+2,SEEK_SET);
    llen = getushort(file);		/* the hierarchy section is unused in font files */
    if ( llen!=1 || hlen<=jlen+ilen+llen ) {
	fclose(file);
return( NULL );
    }

    mlen = getushort(file);
    /* Peter Karow's book documents that hlen==jlen+ilen+llen+mlen+1, but this*/
    /*  does not appear to be the case. */
    if ( hlen<jlen+ilen+llen+mlen+1 || mlen<3*numchars+3 ) {
	fclose(file);
return( NULL );
    }
    /* last record */ getushort(file);
    /* last word of last record */ getushort(file);

    offsets = malloc(numchars*sizeof(int32));
    numbers = malloc(numchars*sizeof(int32));
    maxnum = 0;
    for ( i=0; i<numchars; ++i ) {
	numbers[i] = getushort(file);
	if ( numbers[i]>maxnum ) maxnum = numbers[i];
	rpos = getushort(file);		/* record pos (1 record=2048 words) */
	wpos = getushort(file);		/* word pos in record */
	offsets[i] = (rpos-1)*4096 + 2*(wpos-1);
    }

    sf = SplineFontBlank(numchars/*maxnum+1*/);
    IkarusFontname(sf,fullname,fnam);
    sf->italicangle = italic_angle;
    sf->ascent = 12000; sf->descent = 3000;	/* Ikarus fonts live in a 15,000 em world */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->width = sf->glyphs[i]->vwidth = 15000;
    sf->map = EncMapNew(numchars,numchars,&custom);

    for ( i=0; i<numchars; ++i ) {
	fseek(file,offsets[i],SEEK_SET);
	IkarusReadChar(SFMakeChar(sf,sf->map,i),file);
    }
    if ( loaded_fonts_same_as_new && new_fonts_are_order2 )
	SFConvertToOrder2(sf);

    free(numbers); free(offsets);
    fclose(file);
return( sf );
}
