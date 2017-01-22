#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

typedef unsigned int uint32;
typedef int int32;
typedef short int16;
typedef signed char int8;
typedef unsigned short uint16;
typedef unsigned char uint8;
#define true	1
#define false	0
#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

/* This little program looks through all its arguments trying to find a file */
/*  which contains a specific sfnt table */

static int getushort(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

static int32 getlong(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    int ch4 = getc(ttf);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

static int readttfheader(FILE *ttf, int search_tag) {
    int i, numtables;
    int tag, checksum, offset, length, sr, es, rs; uint32 v;
    int e_sr, e_es, e_rs;

    v = getlong(ttf);
    if ( v==CHR('t','t','c','f')) {
	int ttccnt, *ttcoffsets;
	v = getlong(ttf);
	ttccnt = getlong(ttf);
	ttcoffsets = calloc(ttccnt,sizeof(int));
	for ( i = 0; i<ttccnt; ++i ) {
	    ttcoffsets[i] = getlong(ttf);
	}
	for ( i=0; i<ttccnt; ++i ) {
	    fseek(ttf,ttcoffsets[i],SEEK_SET);
	    if ( readttfheader(ttf,search_tag)) {
		free(ttcoffsets);
return(true);
	    }
	}
	free(ttcoffsets);
return( false );
    }
	
    numtables = getushort(ttf);
    sr = getushort(ttf),
    es = getushort(ttf),
    rs = getushort(ttf);
    if ( v==CHR('O','T','T','O'))
	;
    else if ( v==CHR('t','r','u','e'))
	;
    else if ( v==CHR('S','p','l','i'))
return( false );
    else if ( v==CHR('%','!','P','S'))
return( false );
    else if ( v>=0x80000000 && numtables==0 )
return( false );
    else if ( (v>>24)==1 && ((v>>16)&0xff)==0 && ((v>>8)&0xff)==4 )
return( false );
    else if ( v==CHR('t','y','p','1')) {
    } else
	;
    e_sr = (numtables<8?4:numtables<16?8:numtables<32?16:numtables<64?32:64)*16;
    e_es = (numtables<8?2:numtables<16?3:numtables<32?4:numtables<64?5:6);
    e_rs = numtables*16-e_sr;
    

    for ( i=0; i<numtables; ++i ) {
	tag = getlong(ttf);
	checksum = getlong(ttf);
	offset = getlong(ttf);
	length = getlong(ttf);
	if ( tag == search_tag )
return( true );
    }
return( false );
}

int main(int argc, char **argv) {
    int search_tag, i, any;
    char stag[4];

    if ( argc<=1 ) {
	fprintf( stderr, "Usage: tag files\n" );
return( 2 );
    }
    if ( strlen(argv[1])>4 || argv[1][0]=='\0' ) {
	fprintf( stderr, "Bad tag, must contain at least one and at most 4 characters.\n" );
return( 2 );
    }
    memset(stag,' ',sizeof(stag));
    stag[0] = argv[1][0];
    if ( argv[1][1]!='\0' ) {
	stag[1] = argv[1][1];
	if ( argv[1][2]!='\0' ) {
	    stag[2] = argv[1][2];
	    if ( argv[1][3]!='\0' )
		stag[3] = argv[1][3];
	}
    }
    search_tag = CHR(stag[0],stag[1],stag[2],stag[3]);

    any = false;
    for ( i=2; i<argc; ++i ) {
	FILE *ttf = fopen( argv[i],"r");
	if ( ttf==NULL )
	    fprintf( stderr, "Failed to open %s\n", argv[i]);
	else {
	    if ( readttfheader(ttf,search_tag)) {
		printf ( "%s contains table %s\n", argv[i], argv[1]);
		any = true;
	    }
	    fclose(ttf);
	}
    }
return( !any );
}
	
