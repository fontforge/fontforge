#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utype.h"

static char used[0x110000];
static int cid_2_unicode[0x100000];
static char *nonuni_names[0x100000];
static int cid_2_rotunicode[0x100000];
#define MULT_MAX	6
static int cid_2_unicodemult[0x100000][MULT_MAX];

static int hwtable390[] = {
    646, 1004, 1005, 1002, 1009, 1010, 1008, 936, 938, 940, 942, 944, 946, 948,
    950, 952, 954, 956, 958, 961, 963, 965, 972, 973, 975, 976, 978, 979, 981,
    982, 984, 985 };
static int hwtable501[] = { 645, 647, 672 };
static int hwtable516[] = {
    923, 842, 844, 846, 848, 850, 908, 910, 912, 876, 843, 845, 847, 849, 851,
    852, 854, 856, 858, 860, 862, 864, 866, 868, 870, 872, 874, 877, 879, 881,
    883, 884, 885, 886, 887, 888, 891, 894, 897, 900, 903, 904, 905, 906, 907,
    909, 911, 913, 914, 915, 916, 917, 918, 920, 924, 921, 922, 919, 853, 855,
    857, 859, 861, 863, 865, 867, 869, 871, 873, 875, 878, 880, 882, 889, 890,
    892, 893, 895, 896, 898, 899, 901, 902 };

#define VERTMARK 0x1000000

static int ucs2_score(int val) {		/* Supplied by KANOU Hiroki */
    if ( val>=0x2e80 && val<=0x2fff )
return( 1 );				/* New CJK Radicals are least important */
    else if ( val>=VERTMARK )
return( 2 );				/* Then vertical guys */
    else if ( val>=0xf000 && val<=0xffff )
return( 3 );
/*    else if (( val>=0x3400 && val<0x3dff ) || (val>=0x4000 && val<=0x4dff))*/
    else if ( val>=0x3400 && val<=0x4dff )
return( 4 );
    else
return( 5 );
}

static int getnth(char *buffer, int col, int *mults) {
    int i,j=0, val=0, best;
    char *end;
    int vals[10];

    if ( col==1 ) {
	/* first column is decimal, others are hex */
	if ( !isdigit(*buffer))
return( -1 );
	while ( isdigit(*buffer))
	    val = 10*val + *buffer++-'0';
return( val );
    }
    for ( i=1; i<col; ++buffer ) {
	if ( *buffer=='\t' )
	    ++i;
	else if ( *buffer=='\0' )
return( -1 );
    }
    val = strtol(buffer,&end,16);
    if ( end==buffer )
return( -1 );
    if ( *end=='v' ) {
	val += VERTMARK;
	++end;
    }
    if ( *end==',' ) {
	/* Multiple guess... now we've got to pick one */
	vals[0] = val;
	i = 1;
	while ( *end==',' && i<9 ) {
	    buffer = end+1;
	    vals[i] = strtol(buffer,&end,16);
	    if ( *end=='v' ) {
		vals[i] += VERTMARK;
		++end;
	    }
	    ++i;
	}
	vals[i] = 0;
	best = 0; val = -1;
	for ( i=0; vals[i]!=0; ++i ) {
	    int score=ucs2_score(vals[i]);
	    if ( score>best || (score==best && vals[i]<val) ) {
		val = vals[i];
		best = score;
	    }
	}
	if ( mults!=NULL ) for ( i=j=0; vals[i]!=0; ++i ) {
	    if ( vals[i]!=val && !(vals[i]&VERTMARK))
		mults[j++] = vals[i];
	}
	if ( j>MULT_MAX ) {
	    fprintf( stderr, "Too many multiple values for %04x, need %d slots\n", (unsigned int)(val), j );
exit(1);
	}
    }
return( val );
}

int main(int argc, char **argv) {
    char buffer[600];
    int cid, uni, max=0, maxcid=0, i,j, fakeuni;

    nonuni_names[0] = ".notdef";
    for ( cid=0; cid<0x10000; ++cid ) cid_2_unicode[cid] = -1;

    while ( fgets(buffer,sizeof(buffer),stdin)!=NULL ) {
	if ( *buffer=='#' )
    continue;
	cid = getnth(buffer,1,NULL);
	if ( cid==-1 )
    continue;
	uni = getnth(buffer,22,cid_2_unicodemult[cid]);
	if ( uni==-1 )
	    uni = getnth(buffer,18,cid_2_unicodemult[cid]);
	if ( uni==-1 )
	    uni = getnth(buffer,19,cid_2_unicodemult[cid]);
	fakeuni = -1;
	if ( uni==-1 ) {
	    /* I'm mis-understanding something. Why aren't these guys encoded? */
	    /* Ah, some are halfwidth rather than full... */
	    if ( cid==124 ) fakeuni = 0x2026;	/* and this is proportional not full */
	    else if ( cid>=425 && cid<=500 ) fakeuni = 0x2500+cid-425;
	    else if ( cid>=504 && cid<=505 ) fakeuni = 0x3014+cid-504;
	    else if ( cid>=506 && cid<=513 ) fakeuni = 0x3008+cid-506;
	    else if ( cid>=516 && cid<=597 ) {
		/* These are hiragana, but so disordered (to my eye) that */
		/*  I can't identify them */
	    } else if ( cid==599 ) fakeuni=0xc4;
	    else if ( cid==600 ) fakeuni=0xf9;
	    else if ( cid==601 ) fakeuni=0xe9;
	    else if ( cid==602 ) fakeuni=0xed;
	    else if ( cid==603 ) fakeuni=0xdf;
	    else if ( cid==604 ) fakeuni=0xe7;
	    else if ( cid==605 ) fakeuni=0xc7;
	    else if ( cid==606 ) fakeuni=0xd1;
	    else if ( cid==607 ) fakeuni=0xf1;
	    else if ( cid==608 ) fakeuni=0xa2;
	    else if ( cid==609 ) fakeuni=0xa3;
	    else if ( cid==610 ) fakeuni=0xf3;
	    else if ( cid==611 ) fakeuni=0xfa;
	    else if ( cid==612 ) fakeuni=0xa1;
	    else if ( cid==613 ) fakeuni=0xbf;
	    else if ( cid==614 ) fakeuni=0xbd;
	    else if ( cid==615 ) fakeuni=0xd6;
	    else if ( cid==616 ) fakeuni=0xdc;
	    else if ( cid==617 ) fakeuni=0xe4;
	    else if ( cid==618 ) fakeuni=0xeb;
	    else if ( cid==619 ) fakeuni=0xef;
	    else if ( cid==620 ) fakeuni=0xf6;
	    else if ( cid==621 ) fakeuni=0xfc;
	    else if ( cid==622 ) fakeuni=0xe2;
	    else if ( cid==623 ) fakeuni=0xea;
	    else if ( cid==624 ) fakeuni=0xee;
	    else if ( cid==625 ) fakeuni=0xf4;
	    else if ( cid==626 ) fakeuni=0xfb;
	    else if ( cid==627 ) fakeuni=0xe0;
	    else if ( cid==628 ) fakeuni=0xe8;
	    else if ( cid==630 ) fakeuni=0xe1;
	    else if ( cid==632 ) fakeuni=0xd8;
	    else if ( cid>=8720 && cid<=8814 ) fakeuni=' '+cid-8720;
	    else if ( cid==9435 ) fakeuni = 0x237;	/* dotlessj, 0xf6be */
	    else if ( cid==9504 ) fakeuni = 0xa5;
	    else if ( cid>=9444 && cid<=9537 ) fakeuni = ' '+cid-9444;
	    else if ( cid==9540 ) fakeuni = '\\';
	    else if ( cid>=9544 && cid<=9546 ) fakeuni = 0xa1+cid-9544;
	    else if ( cid==9549 ) fakeuni = 0xa7;
	    else if ( cid==9550 ) fakeuni = 0xa4;
	    else if ( cid==9551 ) fakeuni = 0x201c;
	    else if ( cid==9552 ) fakeuni = 0xab;
	    else if ( cid==9553 ) fakeuni = 0x2039;
	    else if ( cid==9554 ) fakeuni = 0x203a;
	    else if ( cid==9555 ) fakeuni = 0xfb01;
	    else if ( cid==9556 ) fakeuni = 0xfb02;
	    else if ( cid==9558 ) fakeuni = 0x2020;
	    else if ( cid==9559 ) fakeuni = 0x2021;
	    else if ( cid==9560 ) fakeuni = 0xb7;
	    else if ( cid==9561 ) fakeuni = 0xb6;
	    else if ( cid==9562 ) fakeuni = 0x2022;
	    else if ( cid==9563 ) fakeuni = 0x201a;
	    else if ( cid==9564 ) fakeuni = 0x201e;
	    else if ( cid==9565 ) fakeuni = 0x201d;
	    else if ( cid==9566 ) fakeuni = 0xbb;
	    else if ( cid==9567 ) fakeuni = 0x2026;
	    else if ( cid==9568 ) fakeuni = 0x2030;
	    else if ( cid==9569 ) fakeuni = 0xbf;
	    else if ( cid==9570 ) fakeuni = 0x2ca;
	    else if ( cid==9571 ) fakeuni = 0x2c6;
	    else if ( cid==9572 ) fakeuni = 0xaf;
	    else if ( cid==9573 ) fakeuni = 0x2d8;
	    else if ( cid==9574 ) fakeuni = 0x2d9;
	    else if ( cid==9575 ) fakeuni = 0xa8;
	    else if ( cid==9576 ) fakeuni = 0x2da;
	    else if ( cid==9577 ) fakeuni = 0xb8;
	    else if ( cid==9578 ) fakeuni = 0x2dd;
	    else if ( cid==9579 ) fakeuni = 0x2db;
	    else if ( cid==9580 ) fakeuni = 0x2c7;
	    else if ( cid==9581 ) fakeuni = 0x2014;
	    else if ( cid==9582 ) fakeuni = 0xc6;
	    else if ( cid==9583 ) fakeuni = 0xaa;
	    else if ( cid==9584 ) fakeuni = 0x141;
	    else if ( cid==9585 ) fakeuni = 0xd8;
	    else if ( cid==9586 ) fakeuni = 0x152;
	    else if ( cid==9587 ) fakeuni = 0xba;
	    else if ( cid==9588 ) fakeuni = 0xe6;
	    else if ( cid==9589 ) fakeuni = 0x131;
	    else if ( cid==9590 ) fakeuni = 0x142;
	    else if ( cid==9591 ) fakeuni = 0xf8;
	    else if ( cid==9592 ) fakeuni = 0x153;
	    else if ( cid==9593 ) fakeuni = 0xdf;
	    else if ( cid==9595 ) fakeuni = 0xa9;
	    else if ( cid==9596 ) fakeuni = 0xac;
	    else if ( cid==9597 ) fakeuni = 0xae;
	    else if ( cid==9598 ) fakeuni = 0xb0;
	    else if ( cid>=9599 && cid<=9601 ) fakeuni = 0xb1+cid-9599;
	    else if ( cid==9602 ) fakeuni = 0xb5;
	    else if ( cid==9603 ) fakeuni = 0xb9;
	    else if ( cid>=9604 && cid<=9606 ) fakeuni = 0xbc+cid-9604;
	    else if ( cid>=9607 && cid<=9612 ) fakeuni = 0xc0+cid-9607;
	    else if ( cid>=9613 && cid<=9629 ) fakeuni = 0xc7+cid-9613;
	    else if ( cid>=9630 && cid<=9641 ) fakeuni = 0xd9+cid-9630;
	    else if ( cid>=9642 && cid<=9658 ) fakeuni = 0xe7+cid-9642;
	    else if ( cid>=9659 && cid<=9665 ) fakeuni = 0xf9+cid-9659;
	    else if ( cid==9666 ) fakeuni = 0x160;
	    else if ( cid==9667 ) fakeuni = 0x178;
	    else if ( cid==9668 ) fakeuni = 0x17d;
	    else if ( cid==9670 ) fakeuni = 0x160;
	    else if ( cid==9671 ) fakeuni = 0x2122;
	    else if ( cid==9672 ) fakeuni = 0x17e;
	    else if ( cid==9673 ) fakeuni = 0xd8;
	    else if ( cid==9674 ) fakeuni = 0x20ac;
	    else if ( cid==9675 ) fakeuni = 0x2126;
	    else if ( cid==9676 ) fakeuni = 0x2032;
	    else if ( cid==9677 ) fakeuni = 0x2033;
	    else if ( cid==9678 ) fakeuni = 0xfb00;
	    else if ( cid==9679 ) fakeuni = 0xfb03;
	    else if ( cid==9680 ) fakeuni = 0xfb04;
	    else if ( cid==9681 ) fakeuni = 0x101;
	    else if ( cid==9682 ) fakeuni = 0x12b;
	    else if ( cid==9683 ) fakeuni = 0x16b;
	    else if ( cid==9684 ) fakeuni = 0x113;
	    else if ( cid==9685 ) fakeuni = 0x14d;
	    else if ( cid==9686 ) fakeuni = 0x100;
	    else if ( cid==9687 ) fakeuni = 0x12a;
	    else if ( cid==9688 ) fakeuni = 0x16a;
	    else if ( cid==9689 ) fakeuni = 0x112;
	    else if ( cid==9690 ) fakeuni = 0x14c;
	    else if ( cid==9691 ) fakeuni = 0x2158;
	    else if ( cid==9692 ) fakeuni = 0x2159;
	    else if ( cid==9693 ) fakeuni = 0x215a;
	    else if ( cid==9694 ) fakeuni = 0x215b;
	    else if ( cid==9695 ) fakeuni = 0x2153;
	    else if ( cid==9696 ) fakeuni = 0x2154;
	    else if ( cid==9697 ) fakeuni = 0x2070;
	    else if ( cid>=9698 && cid<=9703 ) fakeuni = 0x2074+(cid-9698);
	    else if ( cid>=9704 && cid<=9713 ) fakeuni = 0x2080+(cid-9704);
	    else if ( cid==9714 ) fakeuni = 0x1cd;
	    else if ( cid==9715 ) fakeuni = 0x11a;
	    else if ( cid==9717 ) fakeuni = 0x1ebc;
	    else if ( cid==9718 ) fakeuni = 0x1cf;
	    else if ( cid==9721 ) fakeuni = 0x1d1;
	    else if ( cid==9723 ) fakeuni = 0x1d3;
	    else if ( cid==9724 ) fakeuni = 0x16e;
	    else if ( cid==9725 ) fakeuni = 0x168;
	    else if ( cid==9726 ) fakeuni = 0x1ce;
	    else if ( cid==9727 ) fakeuni = 0x11b;
	    else if ( cid==9729 ) fakeuni = 0x1ebd;
	    else if ( cid==9730 ) fakeuni = 0x1d0;
	    else if ( cid==9733 ) fakeuni = 0x1d2;
	    else if ( cid==9735 ) fakeuni = 0x1d4;
	    else if ( cid==9736 ) fakeuni = 0x16f;
	    else if ( cid==9737 ) fakeuni = 0x169;
    /* duplicate with U+29fd7. JIS X 0213:2004 declares that U+29fce is authentic */
	    else if ( cid==19071 ) fakeuni = 0x29fce;
	}
	maxcid = cid;
	if (( cid>=7887 && cid<=7916 ) || ( cid>=8720 && cid<=9353) ||
		(cid>=12870 && cid<=13313)) {
	    if ( cid>=7894 && cid<=7898 )
		sprintf( buffer, "Japan1.%d.vert", cid-7894+665 );
	    else if ( cid>=7899 && cid<=7900 )
		sprintf( buffer, "Japan1.%d.vert", cid-7899+674 );
	    else if ( cid>=7901 && cid<=7916 )
		sprintf( buffer, "Japan1.%d.vert", cid-7901+676 );
    /* Rotated Proportional latin, first 95 are ascii, rest are a mix */
	    else if ( cid>=8720 && cid<=8949 )
		sprintf( buffer, "Japan1.%d.vert", cid-8720+1 );
    /* Rotated halfwidth latin, japanese ascii (ie yen for backslash) */
    /*  (don't really need a separate case here, both blocks are consecutive) */
	    else if ( cid>=8950 && cid<=9044 )
		sprintf( buffer, "Japan1.%d.vert", cid-8950+231 );
    /* Rotated halfwidth latin, nonascii */
	    else if ( cid>=9045 && cid<=9078 )
		sprintf( buffer, "Japan1.%d.vert", cid-9045+599 );
	    else if ( cid>=9079 && cid<=9081 )
		sprintf( buffer, "Japan1.%d.vert", cid-9079+630 );
	    else if ( cid==9083 )		/* halfwidth backslash */
		strcpy(buffer, "Japan1.8719.vert" );
    /* Rotated halfwidth katakana */
	    else if ( cid>=9084 && cid<=9147 )
		sprintf( buffer, "Japan1.%d.vert", cid-9084+326 );
    /* cid 390 gets used later */
	    else if ( cid>=9148 && cid<=9178 )
		sprintf( buffer, "Japan1.%d.vert", cid-9148+391 );
    /* Rotated halfwidth hiragana */
	    else if ( cid>=9179 && cid<=9262 )
		sprintf( buffer, "Japan1.%d.vert", cid-9179+515 );
	    else if ( cid>=9263 && cid<=9264 )
		sprintf( buffer, "Japan1.%d.vert", cid-9263+423 );
    /* Rotated halfwidth brackets */
	    else if ( cid>=9265 && cid<=9276 )
		sprintf( buffer, "Japan1.%d.vert", cid-9265+504 );
    /* Rotated halfwidth box builders */
	    else if ( cid>=9277 && cid<=9353 )
		sprintf( buffer, "Japan1.%d.vert", cid-9277+425 );
	    else if ( cid==9353 )
		strcpy( buffer, "Japan1.390.vert" );
    /* More proportional latin */
	    else if ( cid>=12870 && cid<=12959 )
		sprintf( buffer, "Japan1.%d.vert", cid-12870+9354 );
    /* proportional italic latin */
	    else if ( cid>=12960 && cid<=13253 )
		sprintf( buffer, "Japan1.%d.vert", cid-12960+9444 );
    /* quarter width latin */
	    else if ( cid>=13254 && cid<=13294 )
		sprintf( buffer, "Japan1.%d.vert", cid-13254+9738 );
	    else if ( uni!=-1 )
		sprintf( buffer, "uni%04X.vert", (unsigned int)((uni>=VERTMARK?uni-VERTMARK:uni)) );
	    else if ( fakeuni!=-1 )
		sprintf( buffer, "uni%04X.vert", (unsigned int)((fakeuni>=VERTMARK?fakeuni-VERTMARK:fakeuni)) );
	    else
    continue;
		/*sprintf( buffer, "japan1-%d.vert", cid );*/
	    nonuni_names[cid] = strdup(buffer);
	    if ( uni!=-1 && uni<VERTMARK && used[uni] ) ++used[uni];
	} else if ( uni>VERTMARK ) {
	    /* rotated */
	    cid_2_rotunicode[cid] = uni-VERTMARK;
	} else if ( uni==-1 && fakeuni==-1 ) {
	    if ( cid>=390 && cid<421 )
		sprintf( buffer, "Japan1.%d.hw", hwtable390[cid-390] );
	    else if ( cid>=501 && cid<=503 )
		sprintf( buffer, "Japan1.%d.hw", hwtable501[cid-501] );
	    else if ( cid>=516 && cid<=598 )
		sprintf( buffer, "Japan1.%d.hw", hwtable516[cid-516] );
	    else if ( cid>=231 && cid<=632 )
		sprintf( buffer, "Japan1.%d.hw", cid );
	    else
    continue;
		/*sprintf( buffer,"Japan1.%d", cid );*/
	    nonuni_names[cid] = strdup(buffer);
	} else if ((( cid>=231 && cid<=326 ) || ( cid>=390 && cid<=632 ) ||
		cid==8719) && uni!=0x2002 && fakeuni!=0x2002 ) {
	    /* halfwidth characters that are not specifically entered as such in uni */
	    /*if ( psunicodenames[uni]!=NULL )
		sprintf( buffer, "%s.hw", psunicodenames[uni]);
	    else*/
	    if ( uni!=-1 )
		sprintf( buffer, "uni%04X.hw", (unsigned int)(uni) );
	    else
		sprintf( buffer, "uni%04X.hw", (unsigned int)(fakeuni) );
	    nonuni_names[cid] = strdup(buffer);
	    if ( uni!=-1 && used[uni] ) ++used[uni];
	    else if ( fakeuni!=-1 && used[fakeuni] ) ++used[fakeuni];
	} else if ( cid>=9444 && cid<=9737 ) {
	    /* italic */
	    /*if ( psunicodenames[uni]!=NULL )
		sprintf( buffer, "%s.italic", psunicodenames[uni]);
	    else*/
		sprintf( buffer, "uni%04X.italic", (unsigned int)(fakeuni) );
	    nonuni_names[cid] = strdup(buffer);
	    if ( used[uni] ) ++used[uni];
	} else if ( uni==-1 ) {
	    if ( fakeuni!=-1 ) {
		sprintf( buffer, "uni%04X.dup%d", (unsigned int)(fakeuni), ++used[fakeuni] );
		nonuni_names[cid] = strdup(buffer);
	    }
    continue;
	} else if ( !used[uni] ) {
	    used[uni] = 1;
	    cid_2_unicode[cid] = uni;
	} else {
	    sprintf( buffer, "uni%04X.dup%d", (unsigned int)(uni), ++used[uni] );
	    nonuni_names[cid] = strdup(buffer);
	}
	max = cid;
    }
    for ( i=0; i<maxcid; ++i ) if ( cid_2_rotunicode[i]!=0 ) {
	for ( j=0; j<maxcid; ++j )
	    if ( cid_2_unicode[j] == cid_2_rotunicode[i] )
	break;
	if ( j==maxcid )
	    sprintf( buffer, "uni%04X.vert", (unsigned int)(cid_2_rotunicode[i]) );
	else
	    sprintf( buffer, "Japan1.%d.vert", j);
	nonuni_names[i] = strdup(buffer);
    }

    printf("%d %d\n",maxcid, max );

    for ( cid=0; cid<=max; ++cid ) {
	if ( cid_2_unicode[cid]!=-1 && cid_2_unicodemult[cid][0]==0 ) {
	    for ( i=1; cid+i<=max && cid_2_unicode[cid+i]==cid_2_unicode[cid]+i && cid_2_unicodemult[cid+i][0]==0; ++i );
	    --i;
	    if ( i!=0 ) {
		printf( "%d..%d %04x\n", cid, cid+i, (unsigned int)(cid_2_unicode[cid]) );
		cid += i;
	    } else
		printf( "%d %04x\n", cid, (unsigned int)(cid_2_unicode[cid]) );
	} else if ( cid_2_unicode[cid]!=-1 ) {
	    printf( "%d %04x", cid, (unsigned int)(cid_2_unicode[cid]) );
	    for ( i=0; cid_2_unicodemult[cid][i]!=0; ++i )
		printf( ",%04x", (unsigned int)(cid_2_unicodemult[cid][i]) );
	    printf( "\n");
	} else if ( nonuni_names[cid]!=NULL )
	    printf( "%d /%s\n", cid, nonuni_names[cid] );
    }
return( 0 );
}
