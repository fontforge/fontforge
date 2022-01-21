#if 0
#include "basics.h"
#else
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#endif
#include <string.h>

static int verbose = false;
static int max_lig_nest = 10000;
static int just_headers = false;
static int head_check = false;

struct dup {
    int glyph;
    int enc;
    struct dup *prev;
};

struct features {
    int feature;
    int nsettings;
    struct settings { uint16_t setting; int16_t nameid; char *name; } *settings;
    int featureflags;
    char *name;
    int nameIndex;
};

struct ttfinfo {
    int emsize;			/* ascent + descent? from the head table */
    int ascent, descent;	/* from the hhea table */
				/* not the usWinAscent from the OS/2 table */
    int width_cnt;		/* from the hhea table, in the hmtx table */
    int glyph_cnt;		/* from maxp table */
    unsigned int index_to_loc_is_long:1;	/* in head table */
    /* Mac fonts platform=0/1, platform specific enc id, roman=0, english is lang code 0 */
    /* iso platform=2, platform specific enc id, latin1=0/2, no language */
    /* microsoft platform=3, platform specific enc id, 1, english is lang code 0x??09 */
    char *copyright;		/* from the name table, nameid=0 */
    char *familyname;		/* nameid=1 */
    char *fullname;		/* nameid=4 */
    char *version;		/* nameid=5 */
    char *fontname;		/* postscript font name, nameid=6 */
    double italicAngle;		/* from post table */
    int upos, uwidth;		/* underline pos, width from post table */

    int ttccnt;			/* if a ttc file, number of fonts */
    int *ttcoffsets;		/* Offsets to font headers */

    int numtables;
    		/* BASE  */
    int base_start;
    int base_length;
    		/* CFF  */
    int cff_start;
    int cff_length;
    		/* cmap */
    int encoding_start;		/* Offset from sof to start of encoding table */
    		/* cvt  */
    int cvt_start;
    int cvt_length;
		/* glyf */
    int glyph_start;		/* Offset from sof to start of glyph table */
		/* bdat */
		/* EBDT */
    int bitmapdata_start;	/* Offset to start of bitmap data */
		/* bloc */
		/* EBLT */
    int bitmaploc_start;	/* Offset to start of bitmap locator data */
    int bitmaploc_length;
		/* EBSC */
    int bitmapscale_start;	/* Offset to start of bitmap scaling data */
    		/* feat */
    int feat_start;
    		/* fdsc */
    int fdsc_start;
    		/* gasp */
    int gasp_start;
    		/* GDEF */
    int gdef_start;		/* OpenType glyph definitions */
    		/* GPOS */
    int gpos_start;		/* OpenType glyph positioning, kerning etc. */
    		/* GSUB */
    int gsub_start;		/* OpenType glyph substitution, ligatures etc. */
		/* hdmx */
    int hdmx_start;
		/* head */
    int head_start;
		/* hhea */
    int hhea_start;
		/* hmtx */
    int hmetrics_start;
		/* JSFT */
    int JSTF_start;
		/* kern */
    int kern_start;

    int lcar_start;
		/* loca */
    int glyphlocations_start;	/* there are glyph_cnt of these, from maxp tab */
    int loca_length;		/* actually glypn_cnt is wrong. Use the table length (divided by size) instead */
		/* MATH */
    int math_start;
		/* maxp */
    int maxp_start;		/* maximum number of glyphs */
    int maxp_length;
		/* mort */
    int mort_start;		/* Apple metamorph tab, ligs, etc. */
    int mort_length;
		/* morx */
    int morx_start;		/* Apple extended metamorph tab, ligs, etc. */
    int morx_length;
		/* name */
    int copyright_start;	/* copyright and fontname */

    int opbd_start;
		/* post */
    int postscript_start;	/* names for the glyphs, italic angle, etc. */
    int postscript_len;		/* names for the glyphs, italic angle, etc. */
		/* OS/2 */
    int os2_start;
    int os2_len;
		/* fvar */
    int fvar_start;
    int fvar_length;
		/* gvar */
    int gvar_start;
    int gvar_length;

    int fpgm_start;
    int fpgm_length;
    int prep_start;
    int prep_length;
    int prop_start;

		/* head */
    int fftm_start;

    unsigned int *glyph_unicode;
    char **glyph_names;
    struct dup *dups;
    struct features *features;

    uint16_t *morx_classes;
    int fvar_axiscount;
};

static int getushort(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

static int32_t getlong(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    int ch4 = getc(ttf);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

static int32_t get3byte(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    if ( ch3==EOF )
return( EOF );
return( (ch1<<16)|(ch2<<8)|ch3 );
}

static int32_t getoffset(FILE *ttf, int offsize) {
    int ch1, ch2, ch3, ch4;

    ch1 = getc(ttf);
    if ( offsize==1 )
return( ch1 );
    ch2 = getc(ttf);
    if ( offsize==2 ) {
	if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2);
    }
    ch3 = getc(ttf);
    if ( offsize==3 ) {
	if ( ch3==EOF )
return( EOF );
return( (ch1<<16)|(ch2<<8)|ch3 );
    }
    ch4 = getc(ttf);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

static double getfixed(FILE *ttf) {
    int32_t val = getlong(ttf);
    int mant = val&0xffff;
    /* This oddity may be needed to deal with the first 16 bits being signed */
    /*  and the low-order bits unsigned */
return( (double) (val>>16) + (mant/65536.0) );
}

static double long2fixed(int32_t val) {
    int mant = val&0xffff;
    /* This oddity may be needed to deal with the first 16 bits being signed */
    /*  and the low-order bits unsigned */
return( (double) (val>>16) + (mant/65536.0) );
}

static int32_t filecheck(FILE *file, int start, int len) {
    uint32_t sum = 0, chunk;

    fseek(file,start,SEEK_SET);
    if ( len!=-1 ) len=(len+3)>>2;
    while ( len==-1 || --len>=0 ) {
	chunk = getlong(file);
	if ( feof(file))
    break;
	sum += chunk;
    }
return( sum );
}

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

static int readcff(FILE *ttf,FILE *util, struct ttfinfo *info);

static int readttfheader(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int i;
    int tag, checksum, offset, length, sr, es, rs, cs; uint32_t v;
    int e_sr, e_es, e_rs;
    double version;

    v = getlong(ttf);
    if ( v==CHR('t','t','c','f')) {
	v = getlong(ttf);
	info->ttccnt = getlong(ttf);
	printf( "This is a TrueType Font Collection file (version=%x), with %d fonts\n", v, info->ttccnt );
	info->ttcoffsets = calloc(info->ttccnt,sizeof(int));
	for ( i = 0; i<info->ttccnt; ++i ) {
	    info->ttcoffsets[i] = getlong(ttf);
	    printf( " Offset to font %d header: %d\n", i, info->ttcoffsets[i]);
	}
return(true);
    }
	
    version = long2fixed(v);
    info->numtables = getushort(ttf);
    sr = getushort(ttf),
    es = getushort(ttf),
    rs = getushort(ttf);
    if ( v==CHR('O','T','T','O'))
	printf( "version='OTTO', " );
    else if ( v==CHR('t','r','u','e'))
	printf( "version='true', " );
    else if ( v==CHR('S','p','l','i')) {
	fprintf(stderr, "This looks like one of fontforge's spline font databases, and not a truetype font.\n" );
exit ( 1 );
    } else if ( v==CHR('%','!','P','S')) {
	fprintf(stderr, "This looks like a postscript (pfa) file, and not a truetype font.\n" );
exit ( 1 );
    } else if ( v>=0x80000000 && info->numtables==0 ) {
	fprintf(stderr, "This looks like a postscript (pfb) file, and not a truetype font.\n" );
exit ( 1 );
    } else if ( (v>>24)==1 && ((v>>16)&0xff)==0 && ((v>>8)&0xff)==4 ) {
	fprintf( stderr, "This looks like a bare CFF file. Proceeding under that assumption.\n");
	info->cff_start = 0;
	fseek(ttf,0,SEEK_END);
	info->cff_length = ftell(ttf);
	rewind(ttf);
	readcff(ttf,util,info);
exit(0);
    } else if ( v==CHR('t','y','p','1')) {
	printf( "version='typ1', " );
	/* This is apple's embedding of a type1 font in a truetype file. I don't know how to parse it. I'd like a copy to look at if you don't mind sending me one... gww@silcom.com */
    } else
	printf( "version=%g, ", version );
    printf( "numtables=%d, searchRange=%d entrySel=%d rangeshift=%d\n",
	    info->numtables, sr, es, rs);
    e_sr = (info->numtables<8?4:info->numtables<16?8:info->numtables<32?16:info->numtables<64?32:64)*16;
    e_es = (info->numtables<8?2:info->numtables<16?3:info->numtables<32?4:info->numtables<64?5:6);
    e_rs = info->numtables*16-e_sr;
    if ( e_sr!=sr || e_es!=es || e_rs!=rs )
	printf( "!!!! Unexpected values for binsearch header. Based on the number of tables I\n!!!!! expect searchRange=%d (not %d), entrySel=%d (not %d) rangeShift=%d (not %d)\n",
		e_sr, sr, e_es, es, e_rs, rs );

/* The one example I have of a ttc file has the file checksum set to: 0xdcd07d3e */
/*  I don't know if that's magic or not (docs don't say), but it vaguely follows */
/*  the same pattern as 0xb1b0afba so it might be */
/* All the fonts used the same head table so one difficulty was eased */
    printf( "File Checksum =%x (should be 0xb1b0afba), diff=%x\n",
	    (unsigned int)(filecheck(util,0,-1)), (unsigned int)((0xb1b0afba-filecheck(util,0,-1))) );

    for ( i=0; i<info->numtables; ++i ) {
	tag = getlong(ttf);
	checksum = getlong(ttf);
	offset = getlong(ttf);
	length = getlong(ttf);
	cs = filecheck(util,offset,length);
 printf( "%c%c%c%c checksum=%08x actual=%08x diff=%x offset=%d len=%d\n",
	     (char)((tag>>24)&0xff), (char)((tag>>16)&0xff),
	     (char)((tag>>8)&0xff), (char)(tag&0xff),
	     (unsigned int)(checksum), (unsigned int)(cs), (unsigned int)((checksum^cs)),
	     offset, length );
	switch ( tag ) {
	  case CHR('B','A','S','E'):
	    info->base_start = offset;
	    info->base_length = length;
	  break;
	  case CHR('C','F','F',' '):
	    info->cff_start = offset;
	    info->cff_length = length;
	  break;
	  case CHR('c','m','a','p'):
	    info->encoding_start = offset;
	  break;
	  case CHR('c','v','t',' '):
	    info->cvt_start = offset;
	    info->cvt_length = length;
	  break;
	  case CHR('F','F','T','M'):
	    info->fftm_start = offset;
	  break;
	  case CHR('f','p','g','m'):
	    info->fpgm_start = offset;
	    info->fpgm_length = length;
	  break;
	  case CHR('f','v','a','r'):
	    info->fvar_start = offset;
	    info->fvar_length = length;
	  break;
	  case CHR('f','e','a','t'):
	    info->feat_start = offset;
	  break;
	  case CHR('f','d','s','c'):
	    info->fdsc_start = offset;
	  break;
	  case CHR('g','a','s','p'):
	    info->gasp_start = offset;
	  break;
	  case CHR('g','l','y','f'):
	    info->glyph_start = offset;
	  break;
	  case CHR('G','D','E','F'):
	    info->gdef_start = offset;
	  break;
	  case CHR('G','P','O','S'):
	    info->gpos_start = offset;
	  break;
	  case CHR('G','S','U','B'):
	    info->gsub_start = offset;
	  break;
	  case CHR('g','v','a','r'):
	    info->gvar_start = offset;
	    info->gvar_length = length;
	  break;
	  case CHR('m','o','r','t'):
	    info->mort_start = offset;
	    info->mort_length = length;
	  break;
	  case CHR('m','o','r','x'):
	    info->morx_start = offset;
	    info->morx_length = length;
	  break;
	  case CHR('b','d','a','t'):
	  case CHR('E','B','D','T'):
	    info->bitmapdata_start = offset;
	  break;
	  case CHR('b','l','o','c'):
	  case CHR('E','B','L','C'):
	    info->bitmaploc_start = offset;
	    info->bitmaploc_length = length;
	  break;
	  case CHR('E','B','S','C'):	/* Apple uses the same as MS on this one */
	    info->bitmapscale_start = offset;
	  break;
	  case CHR('h','d','m','x'):
	    info->hdmx_start = offset;
	  break;
	  case CHR('b','h','e','d'):	/* Fonts with bitmaps but no outlines get bhea */
	  case CHR('h','e','a','d'):
	    info->head_start = offset;
	    if ( length!=54 ) fprintf( stderr, "> ! Unexpected length for head table, was %d expected 54 ! <\n", length );
	  break;
	  case CHR('h','h','e','a'):
	    info->hhea_start = offset;
	    if ( length!=36 ) fprintf( stderr, "> ! Unexpected length for hhea table, was %d expected 36 ! <\n", length );
	  break;
	  case CHR('h','m','t','x'):
	    info->hmetrics_start = offset;
	  break;
	  case CHR('k','e','r','n'):
	    info->kern_start = offset;
	  break;
	  case CHR('J','S','T','F'):
	    info->JSTF_start = offset;
	  break;
	  case CHR('l','o','c','a'):
	    info->glyphlocations_start = offset;
	    info->loca_length = length;
	    info->glyph_cnt = length/2-1;
	  break;
	  case CHR('M','A','T','H'):
	    info->math_start = offset;
	  break;
	  case CHR('m','a','x','p'):
	    info->maxp_start = offset;
	    if ( length!=32 && length!=6 ) fprintf( stderr, "> ! Unexpected length for maxp table, was %d expected 32 ! <\n", length );
	    info->maxp_length = length;
	  break;
	  case CHR('n','a','m','e'):
	    info->copyright_start = offset;
	  break;
	  case CHR('o','p','b','d'):
	    info->opbd_start = offset;
	  break;
	  case CHR('p','o','s','t'):
	    info->postscript_start = offset;
	    info->postscript_len = length;
	  break;
	  case CHR('O','S','/','2'):
	    info->os2_start = offset;
	    info->os2_len = length;
	    if ( length!=78 && length!=86 && length!=96 )
		fprintf( stderr, "> ! Unexpected length for OS/2 table, was %d expected either 78 or 86 ! <\n", length );
	  break;
	  case CHR('p','r','e','p'):
	    info->prep_start = offset;
	    info->prep_length = length;
	  break;
	  case CHR('p','r','o','p'):
	    info->prop_start = offset;
	  break;
	}
    }
    if ( (info->encoding_start!=0 && info->glyph_start==0 && info->head_start!=0 &&
	    info->hhea_start!=0 && info->hmetrics_start!=0 && info->glyphlocations_start == 0 &&
	    info->maxp_start!=0 && info->copyright_start!=0 && info->postscript_start!=0 &&
	    info->os2_start!=0 ) && info->cff_start!=0 ) {
	fprintf( stderr, "Required tables: glyf and loca have been replaced by CFF => OpenType\n" );
    } else if ( info->encoding_start==0 || info->glyph_start==0 || info->head_start==0 ||
	    info->hhea_start==0 || info->hmetrics_start==0 || info->glyphlocations_start == 0 ||
	    info->maxp_start==0 || info->copyright_start==0 || info->postscript_start==0 ||
	    info->os2_start==0 ) {
	fprintf( stderr, "Required table(s) missing: " );
	if ( info->encoding_start==0 ) fprintf(stderr,"cmap ");
	if ( info->glyph_start==0 ) fprintf(stderr,"glyf ");
	if ( info->head_start==0 ) fprintf(stderr,"head ");
	if ( info->hhea_start==0 ) fprintf(stderr,"hhea ");
	if ( info->hmetrics_start==0 ) fprintf(stderr,"hmtx ");
	if ( info->glyphlocations_start==0 ) fprintf(stderr,"loca ");
	if ( info->maxp_start==0 ) fprintf(stderr,"maxp ");
	if ( info->copyright_start==0 ) fprintf(stderr,"name ");
	if ( info->postscript_start==0 ) fprintf(stderr,"post ");
	if ( info->os2_start==0 ) fprintf(stderr,"OS/2");
	fprintf(stderr,"\n");
    }
return( true );
}

static time_t readdate(FILE *ttf) {
    int date[4], date1970[4], year[2];
    int i;
    /* Dates in sfnt files are seconds since 1904. I adjust to unix time */
    /*  seconds since 1970 by figuring out how many seconds were in between */

    date[3] = getushort(ttf);
    date[2] = getushort(ttf);
    date[1] = getushort(ttf);
    date[0] = getushort(ttf);
    memset(date1970,0,sizeof(date1970));
    year[0] = (60*60*24*365L)&0xffff;
    year[1] = (60*60*24*365L)>>16;
    for ( i=1904; i<1970; ++i ) {
	date1970[0] += year[0];
	date1970[1] += year[1];
	if ( (i&3)==0 && (i%100!=0 || i%400==0))
	    date1970[0] += 24*60*60L;		/* Leap year */
	date1970[1] += (date1970[0]>>16);
	date1970[0] &= 0xffff;
	date1970[2] += date1970[1]>>16;
	date1970[1] &= 0xffff;
	date1970[3] += date1970[2]>>16;
	date1970[2] &= 0xffff;
    }

    for ( i=0; i<3; ++i ) {
	date[i] -= date1970[i];
	date[i+1] += date[i]>>16;
	date[i] &= 0xffff;
    }
    date[3] -= date1970[3];
return( (date[1]<<16) | date[0] );
}

static void readttfFFTM(FILE *ttf, FILE *util, struct ttfinfo *info) {
    time_t unix_date;
    struct tm *tm;
    static const char *months[] = { "Jan", "Feb", "March", "April", "May",
	    "June", "July", "August", "Sept", "Oct", "Nov", "Dec", NULL };

    fseek(ttf,info->fftm_start,SEEK_SET);
    printf( "\n\nCreated by FontForge " );
    if ( getlong(ttf)!=0x00000001 ) {
	fprintf( stderr, "Unknown version for 'FFTM' table\n" );
    } else {
	unix_date = readdate(ttf);
	tm = localtime(&unix_date);
	printf( "%d:%02d %02d-%s-%d\n",
		tm->tm_hour, tm->tm_min, tm->tm_mday, months[tm->tm_mon],
		tm->tm_year+1900 );
	unix_date = readdate(ttf);
	printf( "\tFont created: %s", ctime(&unix_date));
	unix_date = readdate(ttf);
	printf( "\tFont modified: %s", ctime(&unix_date));
    }
    printf( "\n" );
}

static void readttfhead(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int mn, flags, fd;
    time_t unix_date;

    fseek(ttf,info->head_start,SEEK_SET);
    printf( "\nHEAD table (at %d)\n", info->head_start );
    printf( "\tVersion=%g\n", getfixed(ttf));
    printf( "\tfontRevision=%g\n", getfixed(ttf));
    printf( "\tchecksumAdj=%x\n", (unsigned int)(getlong(ttf)) );
    mn = getlong(ttf);
    printf( "\tmagicNumber=%x (0x5f0f3cf5, diff=%x)\n",
	    (unsigned int)(mn), (unsigned int)(mn-0x5f0f3cf5) );
    printf( "\tflags=%x ", (unsigned int)((flags=getushort(ttf))) );
    if ( flags&1 ) printf( "baseline_at_0 " );
    if ( flags&2 ) printf( "lsb_at_0 " );
    if ( flags&4 ) printf( "instrs_depend_on_size " );
    if ( flags&8 ) printf( "ppem_to_int " );
    if ( flags&16 ) printf( "instr_set_width " );
    printf( "\n" );
    printf( "\tunitsPerEm=%d\n", getushort(ttf));
    printf( "\tcreate[0]=%x\n", (unsigned int)(getlong(ttf)) );
    printf( "\t create[1]=%x\n", (unsigned int)(getlong(ttf)) );
    fseek(ttf,-8,SEEK_CUR);
    unix_date = readdate(ttf);
    printf( "\tFile created: %s", ctime(&unix_date));
    printf( "\tmodtime[0]=%x\n", (unsigned int)(getlong(ttf)) );
    printf( "\t modtime[1]=%x\n", (unsigned int)(getlong(ttf)) );
    fseek(ttf,-8,SEEK_CUR);
    unix_date = readdate(ttf);
    printf( "\tFile modified: %s", ctime(&unix_date));
    printf( "\txmin=%d\n", (short) getushort(ttf));
    printf( "\tymin=%d\n", (short) getushort(ttf));
    printf( "\txmax=%d\n", (short) getushort(ttf));
    printf( "\tymax=%d\n", (short) getushort(ttf));
    printf( "\tmacstyle=%x\n", (unsigned int)(getushort(ttf)) );
    printf( "\tlowestppem=%d\n", getushort(ttf));
    printf( "\tfontdirhint=%d ", fd = getushort(ttf));
    switch ( fd ) {
      case 0: printf("mixed directional glyphs\n" ); break;
      case 1: printf("strongly left to right glyphs\n" ); break;
      case 2: printf("left to right and neutrals\n" ); break;
      case -1: printf("strongly right to left glyphs\n" ); break;
      case -2: printf("right to left and neutrals\n" ); break;
      default: printf("???\n" ); break;
    }
    printf( "\tloca_is_32=%d\n", info->index_to_loc_is_long = getushort(ttf));
    if ( info->index_to_loc_is_long )
	info->glyph_cnt = info->loca_length/4-1;
    printf( "\tglyphdataformat=%d\n", getushort(ttf));
}

static void readttfhhead(FILE *ttf, FILE *util, struct ttfinfo *info) {

    fseek(ttf,info->hhea_start,SEEK_SET);
    printf( "\nHHEAD table (at %d)\n", info->hhea_start );
    printf( "\tVersion=%g\n", getfixed(ttf));
    printf( "\tascender=%d\n", (short) getushort(ttf));
    printf( "\tdescender=%d\n", (short) getushort(ttf));
    printf( "\tlinegap=%d\n", (short) getushort(ttf));
    printf( "\tadvanceWidthMax=%d\n", getushort(ttf));
    printf( "\tminlsb=%d\n", (short) getushort(ttf));
    printf( "\tminrsb=%d\n", (short) getushort(ttf));
    printf( "\tmaxextent=%d\n", (short) getushort(ttf));
    printf( "\tcaretsloperise=%d\n", (short) getushort(ttf));
    printf( "\tcaretsloperun=%d\n", (short) getushort(ttf));
    printf( "\tmbz=%d\n", (short) getushort(ttf));
    printf( "\tmbz=%d\n", (short) getushort(ttf));
    printf( "\tmbz=%d\n", (short) getushort(ttf));
    printf( "\tmbz=%d\n", (short) getushort(ttf));
    printf( "\tmbz=%d\n", (short) getushort(ttf));
    printf( "\tmetricdataformat=%d\n", (short) getushort(ttf));
    printf( "\tnumberOfHMetrics=%d\n", getushort(ttf));
}

static char *getttfname(FILE *util, struct ttfinfo *info, int nameid) {
    int nrec, taboff, stroff, strlen, platform;
    int i,j,id;

    fseek(util,info->copyright_start,SEEK_SET);
    /* format */ getushort(util);
    nrec=getushort(util);
    taboff=getushort(util);
    for ( i=0; i<nrec; ++i ) {
	platform = getushort(util);
	/* plat spec encoding */ getushort(util);
	/* language */ getushort(util);
	id=getushort(util);
	strlen=getushort(util);
	stroff=getushort(util);
	if ( platform==1 && id==nameid ) {
	    char *ret = malloc(strlen+2);	/* In case it's unicode */
	    fseek(util,info->copyright_start+taboff+stroff,SEEK_SET);
	    for ( j=0; j<strlen; ++j )
		ret[j] = getc(util);
	    ret[strlen] = '\0';
	    ret[strlen+1] = '\0';
return( ret );
	}
    }
return( NULL );
}

static void readttfname(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int nrec, taboff, stroff, strlen;
    int i,j,id;

    fseek(ttf,info->copyright_start,SEEK_SET);
    printf( "\nNAME table (at %d)\n", info->copyright_start );
    printf( "\tformat=%d\n", getushort(ttf));
    printf( "\tnrecords=%d\n", nrec=getushort(ttf));
    printf( "\ttaboff=%d\n", taboff=getushort(ttf));
    for ( i=0; i<nrec; ++i ) {
	printf( "\t platform=%d", getushort(ttf));
	printf( " plat spec encoding=%d", getushort(ttf));
	printf( " language=%x", (unsigned int)(getushort(ttf)) );
	printf( " name=%d ", id=getushort(ttf));
/* Name IDs defined in original TTF docs */
	printf( id==0?"Copyright\n":id==1?"Family\n":id==2?"Subfamily\n":id==3?
		"UniqueID\n":id==4?"FullName\n":id==5?"Version\n":id==6?"Postscript\n":
		id==7?"Trademark\n":
/* OpenType extensions. documented: http://partners.adobe.com/asn/developer/opentype/recom.html */
		id==8?"Manufacturer\n":
		id==9?"Designer\n":
		id==10?"Descriptor\n":
		id==11?"Vendor URL\n":
		id==12?"Designer URL\n":
/* Guesse  based on ARIAL.TTF usage. Not documented that I've seen */
		id==13?"License\n":
/* Guesses based on OpenType usage. Not documented that I've seen */
		id==14?"License URL\n":
		/* 15 is reserved */
		id==16?"Preferred Family Name\n":
		id==17?"Preferred Subfamily\n":
		id==18?"Compatible full\n":
		id==19?"Sample Text\n":
/* End guesses */
		"???????\n");
	printf( "\t strlen=%d ", strlen=getushort(ttf));
	printf( " stroff=%d\t   ", stroff=getushort(ttf));
	fseek(util,info->copyright_start+taboff+stroff,SEEK_SET);
	for ( j=0; j<strlen; ++j ) {
	    int ch = getc(util);
	    if ( ch<' ' ) {
		putchar('^');
		putchar(ch+'@');
	    } else if ( ch=='\177' ) {
		putchar('^');
		putchar('?');
	    } else if ( ch>=0x80 && ch<0xa0 ) {
		printf("<%2X>", (unsigned int)(ch) );
	    } else
		putchar(ch);
	}
	putchar('\n');
    }
}

static void readttfmaxp(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int ng;

    fseek(ttf,info->maxp_start,SEEK_SET);
    printf( "\nMAXP table (at %d)\n", info->maxp_start );
    printf( "\tVersion=%g\n", getfixed(ttf));
    printf( "\t numGlyphs=%d\n", ng = getushort(ttf));
    info->glyph_cnt = ng;		/* Open type doesn't have a loca table */
    if ( info->maxp_length==6 )		/* Open Type */
return;
    printf( "\t maxPoints=%d\n", getushort(ttf));
    printf( "\t maxContours=%d\n", getushort(ttf));
    printf( "\t maxCompositPoints=%d\n", getushort(ttf));
    printf( "\t maxCompositContours=%d\n", getushort(ttf));
    printf( "\t maxZones=%d\n", getushort(ttf));
    printf( "\t maxTwilightPoints=%d\n", getushort(ttf));
    printf( "\t maxStorage=%d\n", getushort(ttf));
    printf( "\t maxFunctionDefs=%d\n", getushort(ttf));
    printf( "\t maxInstructionDefs=%d\n", getushort(ttf));
    printf( "\t maxStackElements=%d\n", getushort(ttf));
    printf( "\t maxSizeOfInstructions=%d\n", getushort(ttf));
    printf( "\t maxComponentElements=%d\n", getushort(ttf));
    printf( "\t maxComponentDepth=%d\n", getushort(ttf));
}

static void readttfcvt(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int i;

    if ( info->cvt_start==0 )
return;
    fseek(ttf,info->cvt_start,SEEK_SET);
    printf( "\nCVT  table (at %d, len=%d)\n", info->cvt_start, info->cvt_length );
    for ( i=0; i<info->cvt_length/2; ++i )
	printf( " CVT[%d] = %d\n", i, (short) getushort(ttf));
}

static void readttfos2(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int i, val;
    static const char *weights[] = { "Unspecified", "Thin", "Extra-Light", "Light", "Normal", "Medium", "Semi-Bold", "Bold",  "Extra-Bold", "Black", "???" };
    static const char *widths[] = { "Unspecified", "Ultra-Condensed", "Extra-Condensed", "Condensed", "Semi-Condensed", "Medium", "Semi-Expanded", "Expanded", "Extra-Expanded", "Ultra-Expanded", "???" };
    static const char *class[16] = { "No classification", "Old Style Serifs", "Transitional Serifs", "Modern Serifs", "Clarendon Serifs", "Slab Serifs", "???", "Freeform Serifs", "Sans Serif", "Ornamentals", "Scripts", "???", "Symbolic", "???", "???", "???" };
    static const char *subclass0[16] = { "", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "Misc" };
    static const char *subclass1[16] = { "", "ibm rounded", "garalde", "venetian", "modified venetian", "dutch modern", "dutch traditional", "contemporary", "caligraphic", "???", "???", "???", "???", "???", "???", "Misc" };
    static const char *subclass2[16] = { "", "direct line", "script", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "Misc" };
    static const char *subclass3[16] = { "", "italian", "script", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "Misc" };
    static const char *subclass4[16] = { "", "clarendon", "modern", "traditional", "newspaper", "stub", "monotone", "typewriter", "???", "???", "???", "???", "???", "???", "???", "Misc" };
    static const char *subclass5[16] = { "", "monotone", "humanist", "geometric", "swiss", "typewriter", "???", "???", "???", "???", "???", "???", "???", "???", "???", "Misc" };
    static const char *subclass7[16] = { "", "modern", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "Misc" };
    static const char *subclass8[16] = { "", "ibm neogrotesque", "humanist", "low-x rounded", "high-x rounded", "neo-grotesque", "modified neo-grotesque", "???", "???", "typewriter", "matrix", "???", "???", "???", "???", "Misc" };
    static const char *subclass9[16] = { "", "engraver", "black letter", "decorative", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "Misc" };
    static const char *subclass10[16] = { "", "???", "uncial", "brush joined", "formal joined", "monotone joined", "calligraphic", "brush unjoined", "formal unjoined", "monotone unjoined", "???", "???", "???", "???", "???", "Misc" };
    static const char *subclass12[16] = { "", "???", "???", "mixed serif", "???", "???", "old style serif", "neo-grotesque sans", "???", "???", "???", "???", "???", "???", "???", "Misc" };
    static const char **subclasses[16] = { subclass0, subclass1, subclass2, subclass3,
	    subclass4, subclass5, subclass0, subclass7, subclass8, subclass9,
	    subclass10, subclass0, subclass12, subclass0, subclass0, subclass0 };
    static const char *panose0[] = { "Any", "No Fit", "Text & Display", "Script", "Decorative", "Pictoral" };
    static const char *panose1[] = { "Any", "No Fit", "Cove", "Obtuse Cove", "Square Cove", "Obtuse Square Cove", "Square", "Thin", "Bone", "Exaggerated", "Triangle", "Normal Sans", "Obtuse Sans", "Perp Sans", "Flared", "Rounded" };
    static const char *panose2[] = { "Any", "No Fit", "Very Light", "Light", "Thin", "Book", "Medium", "Demi", "Bold", "Heavy", "Black", "Nord" };
    static const char *panose3[] = { "Any", "No Fit", "Old Style", "Modern", "Even Width", "Expanded", "Condensed", "Very Expanded", "Very Condensed", "Monospaced" };
    static const char *panose4[] = { "Any", "No Fit", "None", "Very Low", "Low", "Medium Low", "Medium", "Medium High", "High", "Very High" };
    static const char *panose5[] = { "Any", "No Fit", "No Variation", "Gradual/Diagonal", "Gradual/Transitional", "Gradual/Vertical", "Gradual/Horizontal", "Rapid/Vertical", "Rapid/Horizontal", "Instant/Vertical", "Instant/Horizontal" };
    static const char *panose6[] = { "Any", "No Fit", "Straight Arms/Horizontal", "Straight Arms/Wedge", "Straight Arms/Vertical", "Straight Arms/Single Serif", "Straight Arms/Double Serif", "Non-Straight Arms/Horizontal", "Non-Straight Arms/Wedge", "Non-Straight Arms/Vertical", "Non-Straight Arms/Single Serif", "Non-Straight Arms/Double Serif" };
    static const char *panose7[] = { "Any", "No Fit", "Normal/Contact", "Normal/Weighted", "Normal/Boxed", "Normal/Flattened", "Normal/Rounded", "Normal/Off-Center", "Normal/Square", "Oblique/Contact", "Oblique/Weighted", "Oblique/Boxed", "Oblique/Flattened", "Oblique/Rounded", "Oblique/Off-Center", "Oblique/Square" };
    static const char *panose8[] = { "Any", "No Fit", "Standard/Trimmed", "Standard/Pointed", "Standard/Serifed", "High/Trimmed", "High/Pointed", "High/Serifed", "Constant/Trimmed", "Constant/Pointed", "Constant/Serifed", "Low/Trimmed", "Low/Pointed", "Low/Serifed" };
    static const char *panose9[] = { "Any", "No Fit", "Constant/Small", "Constant/Standard", "Constant/Large", "Ducking/Small", "Ducking/Standard", "Ducking/Large" };
    static struct { const char *name; char const *const *const choices; int cnt; } panose[10] = {
	{ "Family", panose0, sizeof(panose0) },
	{ "Serif Type", panose1, sizeof(panose1) },
	{ "Weight", panose2, sizeof(panose2) },
	{ "Proportion", panose3, sizeof(panose3) },
	{ "Contrast", panose4, sizeof(panose4) },
	{ "Stroke Variation", panose5, sizeof(panose5) },
	{ "Arm Style", panose6, sizeof(panose6) },
	{ "Letterform", panose7, sizeof(panose7) },
	{ "Midline", panose8, sizeof(panose8) },
	{ "X-Height", panose9, sizeof(panose9) } };

    fseek(ttf,info->os2_start,SEEK_SET);
    printf( "\nOS/2 table (at %d for %d bytes)\n", info->os2_start, info->os2_len );
    printf( "\tVersion=%d\n", getushort(ttf));
    printf( "\t avgWidth=%d\n", (short)getushort(ttf));
    printf( "\t weightClass=%d ", val = getushort(ttf));
    if ( val>=0 && val<=999 ) printf( "%s\n", weights[val/100] ); else printf( "???\n");
    printf( "\t widthClass=%d ", val = getushort(ttf));
    if ( val>=0 && val<=9 ) printf( "%s\n", widths[val] ); else printf( "???\n");
    printf( "\t fstype=0x%x ", (unsigned int)((val = (short) getushort(ttf))) );
    if ( val&0x8 ) printf( "Editable embedding\n");
    else if ( val&0x4 ) printf( "Preview & Print embedding\n");
    else if ( val&0x2 ) printf( "Restricted license embedding\n");
    else printf( "\n");
    printf( "\t ySubscript XSize=%d\n", (short) getushort(ttf));
    printf( "\t ySubscript YSize=%d\n", (short) getushort(ttf));
    printf( "\t ySubscript XOffset=%d\n", (short) getushort(ttf));
    printf( "\t ySubscript YOffset=%d\n", (short) getushort(ttf));
    printf( "\t ySupscript XSize=%d\n", (short) getushort(ttf));
    printf( "\t ySupscript YSize=%d\n", (short) getushort(ttf));
    printf( "\t ySupscript XOffset=%d\n", (short) getushort(ttf));
    printf( "\t ySupscript YOffset=%d\n", (short) getushort(ttf));
    printf( "\t yStrikeoutSize=%d\n", (short) getushort(ttf));
    printf( "\t yStrikeoutPos=%d\n", (short) getushort(ttf));
    printf( "\t sFamilyClass=%04x ", (unsigned int)((val = getushort(ttf))) );
    if ( (val>>8)>=0 && (val>>8)<16 ) {
	printf( "%s ", class[val>>8]);
	if ( (val&0xff)<16 )
	    printf( "%s\n", subclasses[val>>8][val&0xff]);
	else
	    printf( "???\n" );
    } else printf ( "??? ???\n" );
    printf( "\t Panose\n" );
    for ( i=0; i<10; ++i ) {
	printf( "\t\t%s: %02x ", panose[i].name, (unsigned int)(val= getc(ttf)) );
	if ( val>0 && val<panose[i].cnt ) printf( "%s\n", panose[i].choices[val]);
	else printf( "???\n" );
    }
    printf( "\n");
    printf( "\t UnicodeRange=%08x ", (unsigned int)(getlong(ttf)) );
    printf( "%08x ", (unsigned int)(getlong(ttf)) );
    printf( "%08x ", (unsigned int)(getlong(ttf)) );
    printf( "%08x\n", (unsigned int)(getlong(ttf)) );
    printf( "\t achVendId " );
    for ( i=0; i<4; ++i )
	printf( "%02x ", (unsigned int)(getc(ttf)) );
    printf( "\n");
    printf( "\t fsSelection=%d\n", getushort(ttf));
    printf( "\t firstcharindex=%d\n", getushort(ttf));
    printf( "\t lastcharindex=%d\n", getushort(ttf));
    printf( "\t stypeascender=%d\n", getushort(ttf));
    printf( "\t stypedescender=%d\n", (short) getushort(ttf));
    printf( "\t stypelinegap=%d\n", getushort(ttf));
    printf( "\t usWinAscent=%d\n", getushort(ttf));
    printf( "\t usWinDescent=%d\n", (short) getushort(ttf));
    if ( info->os2_len==78 ) {
	printf( "\t (no CodePageRange)\n" );
    } else {
	printf( "\t CodePageRange=%08x ", (unsigned int)(getlong(ttf)) );
	printf( "%08x\n", (unsigned int)(getlong(ttf)) );
	if ( info->os2_len==96 ) {
	    /* Open type additions */
	    printf( "\txHeight=%d\n", (short) getushort(ttf));
	    printf( "\tCapHeight=%d\n", (short) getushort(ttf));
	    printf( "\tDefaultChar=%d\n", getushort(ttf));	/* 0, or space I guess */
	    printf( "\tBreakChar=%d\n", getushort(ttf));	/* space */
	    printf( "\tMaxContext=%d\n", getushort(ttf));	/* Maximum look-ahead needed for processing. kerning=>2. 3letter ligature=>3, 4letter=>4 */
	}
    }
}

const unsigned short unicode_from_mac[] = {
  0x0000, 0x00d0, 0x00f0, 0x0141, 0x0142, 0x0160, 0x0161, 0x00dd,
  0x00fd, 0x0009, 0x000a, 0x00de, 0x00fe, 0x000d, 0x017d, 0x017e,
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x00bd, 0x00bc, 0x00b9,
  0x00be, 0x00b3, 0x00b2, 0x00a6, 0x00ad, 0x00d7, 0x001e, 0x001f,
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
  0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
  0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
  0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
  0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
  0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
  0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1,
  0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8,
  0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3,
  0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc,
  0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df,
  0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8,
  0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211,
  0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x2126, 0x00e6, 0x00f8,
  0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab,
  0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153,
  0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca,
  0x00ff, 0x0178, 0x2044, 0x00a4, 0x2039, 0x203a, 0xfb01, 0xfb02,
  0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1,
  0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4,
  0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc,
  0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7
};

static const char *standardnames[258] = {
".notdef",
".null",
"nonmarkingreturn",
"space",
"exclam",
"quotedbl",
"numbersign",
"dollar",
"percent",
"ampersand",
"quotesingle",
"parenleft",
"parenright",
"asterisk",
"plus",
"comma",
"hyphen",
"period",
"slash",
"zero",
"one",
"two",
"three",
"four",
"five",
"six",
"seven",
"eight",
"nine",
"colon",
"semicolon",
"less",
"equal",
"greater",
"question",
"at",
"A",
"B",
"C",
"D",
"E",
"F",
"G",
"H",
"I",
"J",
"K",
"L",
"M",
"N",
"O",
"P",
"Q",
"R",
"S",
"T",
"U",
"V",
"W",
"X",
"Y",
"Z",
"bracketleft",
"backslash",
"bracketright",
"asciicircum",
"underscore",
"grave",
"a",
"b",
"c",
"d",
"e",
"f",
"g",
"h",
"i",
"j",
"k",
"l",
"m",
"n",
"o",
"p",
"q",
"r",
"s",
"t",
"u",
"v",
"w",
"x",
"y",
"z",
"braceleft",
"bar",
"braceright",
"asciitilde",
"Adieresis",
"Aring",
"Ccedilla",
"Eacute",
"Ntilde",
"Odieresis",
"Udieresis",
"aacute",
"agrave",
"acircumflex",
"adieresis",
"atilde",
"aring",
"ccedilla",
"eacute",
"egrave",
"ecircumflex",
"edieresis",
"iacute",
"igrave",
"icircumflex",
"idieresis",
"ntilde",
"oacute",
"ograve",
"ocircumflex",
"odieresis",
"otilde",
"uacute",
"ugrave",
"ucircumflex",
"udieresis",
"dagger",
"degree",
"cent",
"sterling",
"section",
"bullet",
"paragraph",
"germandbls",
"registered",
"copyright",
"trademark",
"acute",
"dieresis",
"notequal",
"AE",
"Oslash",
"infinity",
"plusminus",
"lessequal",
"greaterequal",
"yen",
"mu",
"partialdiff",
"summation",
"product",
"pi",
"integral",
"ordfeminine",
"ordmasculine",
"Omega",
"ae",
"oslash",
"questiondown",
"exclamdown",
"logicalnot",
"radical",
"florin",
"approxequal",
"Delta",
"guillemotleft",
"guillemotright",
"ellipsis",
"nonbreakingspace",
"Agrave",
"Atilde",
"Otilde",
"OE",
"oe",
"endash",
"emdash",
"quotedblleft",
"quotedblright",
"quoteleft",
"quoteright",
"divide",
"lozenge",
"ydieresis",
"Ydieresis",
"fraction",
"currency",
"guilsinglleft",
"guilsinglright",
"fi",
"fl",
"daggerdbl",
"periodcentered",
"quotesinglbase",
"quotedblbase",
"perthousand",
"Acircumflex",
"Ecircumflex",
"Aacute",
"Edieresis",
"Egrave",
"Iacute",
"Icircumflex",
"Idieresis",
"Igrave",
"Oacute",
"Ocircumflex",
"apple",
"Ograve",
"Uacute",
"Ucircumflex",
"Ugrave",
"dotlessi",
"circumflex",
"tilde",
"macron",
"breve",
"dotaccent",
"ring",
"cedilla",
"hungarumlaut",
"ogonek",
"caron",
"Lslash",
"lslash",
"Scaron",
"scaron",
"Zcaron",
"zcaron",
"brokenbar",
"Eth",
"eth",
"Yacute",
"yacute",
"Thorn",
"thorn",
"minus",
"multiply",
"onesuperior",
"twosuperior",
"threesuperior",
"onehalf",
"onequarter",
"threequarters",
"franc",
"Gbreve",
"gbreve",
"Idotaccent",
"Scedilla",
"scedilla",
"Cacute",
"cacute",
"Ccaron",
"ccaron",
"dcroat"
};

/* Standard names for cff */
const char *cffnames[] = {
 ".notdef",
 "space",
 "exclam",
 "quotedbl",
 "numbersign",
 "dollar",
 "percent",
 "ampersand",
 "quoteright",
 "parenleft",
 "parenright",
 "asterisk",
 "plus",
 "comma",
 "hyphen",
 "period",
 "slash",
 "zero",
 "one",
 "two",
 "three",
 "four",
 "five",
 "six",
 "seven",
 "eight",
 "nine",
 "colon",
 "semicolon",
 "less",
 "equal",
 "greater",
 "question",
 "at",
 "A",
 "B",
 "C",
 "D",
 "E",
 "F",
 "G",
 "H",
 "I",
 "J",
 "K",
 "L",
 "M",
 "N",
 "O",
 "P",
 "Q",
 "R",
 "S",
 "T",
 "U",
 "V",
 "W",
 "X",
 "Y",
 "Z",
 "bracketleft",
 "backslash",
 "bracketright",
 "asciicircum",
 "underscore",
 "quoteleft",
 "a",
 "b",
 "c",
 "d",
 "e",
 "f",
 "g",
 "h",
 "i",
 "j",
 "k",
 "l",
 "m",
 "n",
 "o",
 "p",
 "q",
 "r",
 "s",
 "t",
 "u",
 "v",
 "w",
 "x",
 "y",
 "z",
 "braceleft",
 "bar",
 "braceright",
 "asciitilde",
 "exclamdown",
 "cent",
 "sterling",
 "fraction",
 "yen",
 "florin",
 "section",
 "currency",
 "quotesingle",
 "quotedblleft",
 "guillemotleft",
 "guilsinglleft",
 "guilsinglright",
 "fi",
 "fl",
 "endash",
 "dagger",
 "daggerdbl",
 "periodcentered",
 "paragraph",
 "bullet",
 "quotesinglbase",
 "quotedblbase",
 "quotedblright",
 "guillemotright",
 "ellipsis",
 "perthousand",
 "questiondown",
 "grave",
 "acute",
 "circumflex",
 "tilde",
 "macron",
 "breve",
 "dotaccent",
 "dieresis",
 "ring",
 "cedilla",
 "hungarumlaut",
 "ogonek",
 "caron",
 "emdash",
 "AE",
 "ordfeminine",
 "Lslash",
 "Oslash",
 "OE",
 "ordmasculine",
 "ae",
 "dotlessi",
 "lslash",
 "oslash",
 "oe",
 "germandbls",
 "onesuperior",
 "logicalnot",
 "mu",
 "trademark",
 "Eth",
 "onehalf",
 "plusminus",
 "Thorn",
 "onequarter",
 "divide",
 "brokenbar",
 "degree",
 "thorn",
 "threequarters",
 "twosuperior",
 "registered",
 "minus",
 "eth",
 "multiply",
 "threesuperior",
 "copyright",
 "Aacute",
 "Acircumflex",
 "Adieresis",
 "Agrave",
 "Aring",
 "Atilde",
 "Ccedilla",
 "Eacute",
 "Ecircumflex",
 "Edieresis",
 "Egrave",
 "Iacute",
 "Icircumflex",
 "Idieresis",
 "Igrave",
 "Ntilde",
 "Oacute",
 "Ocircumflex",
 "Odieresis",
 "Ograve",
 "Otilde",
 "Scaron",
 "Uacute",
 "Ucircumflex",
 "Udieresis",
 "Ugrave",
 "Yacute",
 "Ydieresis",
 "Zcaron",
 "aacute",
 "acircumflex",
 "adieresis",
 "agrave",
 "aring",
 "atilde",
 "ccedilla",
 "eacute",
 "ecircumflex",
 "edieresis",
 "egrave",
 "iacute",
 "icircumflex",
 "idieresis",
 "igrave",
 "ntilde",
 "oacute",
 "ocircumflex",
 "odieresis",
 "ograve",
 "otilde",
 "scaron",
 "uacute",
 "ucircumflex",
 "udieresis",
 "ugrave",
 "yacute",
 "ydieresis",
 "zcaron",
 "exclamsmall",
 "Hungarumlautsmall",
 "dollaroldstyle",
 "dollarsuperior",
 "ampersandsmall",
 "Acutesmall",
 "parenleftsuperior",
 "parenrightsuperior",
 "twodotenleader",
 "onedotenleader",
 "zerooldstyle",
 "oneoldstyle",
 "twooldstyle",
 "threeoldstyle",
 "fouroldstyle",
 "fiveoldstyle",
 "sixoldstyle",
 "sevenoldstyle",
 "eightoldstyle",
 "nineoldstyle",
 "commasuperior",
 "threequartersemdash",
 "periodsuperior",
 "questionsmall",
 "asuperior",
 "bsuperior",
 "centsuperior",
 "dsuperior",
 "esuperior",
 "isuperior",
 "lsuperior",
 "msuperior",
 "nsuperior",
 "osuperior",
 "rsuperior",
 "ssuperior",
 "tsuperior",
 "ff",
 "ffi",
 "ffl",
 "parenleftinferior",
 "parenrightinferior",
 "Circumflexsmall",
 "hyphensuperior",
 "Gravesmall",
 "Asmall",
 "Bsmall",
 "Csmall",
 "Dsmall",
 "Esmall",
 "Fsmall",
 "Gsmall",
 "Hsmall",
 "Ismall",
 "Jsmall",
 "Ksmall",
 "Lsmall",
 "Msmall",
 "Nsmall",
 "Osmall",
 "Psmall",
 "Qsmall",
 "Rsmall",
 "Ssmall",
 "Tsmall",
 "Usmall",
 "Vsmall",
 "Wsmall",
 "Xsmall",
 "Ysmall",
 "Zsmall",
 "colonmonetary",
 "onefitted",
 "rupiah",
 "Tildesmall",
 "exclamdownsmall",
 "centoldstyle",
 "Lslashsmall",
 "Scaronsmall",
 "Zcaronsmall",
 "Dieresissmall",
 "Brevesmall",
 "Caronsmall",
 "Dotaccentsmall",
 "Macronsmall",
 "figuredash",
 "hypheninferior",
 "Ogoneksmall",
 "Ringsmall",
 "Cedillasmall",
 "questiondownsmall",
 "oneeighth",
 "threeeighths",
 "fiveeighths",
 "seveneighths",
 "onethird",
 "twothirds",
 "zerosuperior",
 "foursuperior",
 "fivesuperior",
 "sixsuperior",
 "sevensuperior",
 "eightsuperior",
 "ninesuperior",
 "zeroinferior",
 "oneinferior",
 "twoinferior",
 "threeinferior",
 "fourinferior",
 "fiveinferior",
 "sixinferior",
 "seveninferior",
 "eightinferior",
 "nineinferior",
 "centinferior",
 "dollarinferior",
 "periodinferior",
 "commainferior",
 "Agravesmall",
 "Aacutesmall",
 "Acircumflexsmall",
 "Atildesmall",
 "Adieresissmall",
 "Aringsmall",
 "AEsmall",
 "Ccedillasmall",
 "Egravesmall",
 "Eacutesmall",
 "Ecircumflexsmall",
 "Edieresissmall",
 "Igravesmall",
 "Iacutesmall",
 "Icircumflexsmall",
 "Idieresissmall",
 "Ethsmall",
 "Ntildesmall",
 "Ogravesmall",
 "Oacutesmall",
 "Ocircumflexsmall",
 "Otildesmall",
 "Odieresissmall",
 "OEsmall",
 "Oslashsmall",
 "Ugravesmall",
 "Uacutesmall",
 "Ucircumflexsmall",
 "Udieresissmall",
 "Yacutesmall",
 "Thornsmall",
 "Ydieresissmall",
 "001.000",
 "001.001",
 "001.002",
 "001.003",
 "Black",
 "Bold",
 "Book",
 "Light",
 "Medium",
 "Regular",
 "Roman",
 "Semibold",
 NULL
};
static const int nStdStrings = sizeof(cffnames)/sizeof(cffnames[0])-1;

static const char *instrs[] = {
    "SVTCA[y-axis]",
    "SVTCA[x-axis]",
    "SPVTCA[y-axis]",
    "SPVTCA[x-axis]",
    "SFVTCA[y-axis]",
    "SFVTCA[x-axis]",
    "SPVTL[parallel]",
    "SPVTL[orthog]",
    "SFVTL[parallel]",
    "SFVTL[orthog]",
    "SPVFS",
    "SFVFS",
    "GPV",
    "GFV",
    "SFVTPV",
    "ISECT",
    "SRP0",
    "SRP1",
    "SRP2",
    "SZP0",
    "SZP1",
    "SZP2",
    "SZPS",
    "SLOOP",
    "RTG",
    "RTHG",
    "SMD",
    "ELSE",
    "JMPR",
    "SCVTCI",
    "SSWCI",
    "SSW",
    "DUP",
    "POP",
    "CLEAR",
    "SWAP",
    "DEPTH",
    "CINDEX",
    "MINDEX",
    "ALIGNPTS",
    "Unknown28",
    "UTP",
    "LOOPCALL",
    "CALL",
    "FDEF",
    "ENDF",
    "MDAP[no round]",
    "MDAP[round]",
    "IUP[y]",
    "IUP[x]",
    "SHP[rp2]",
    "SHP[rp1]",
    "SHC[rp2]",
    "SHC[rp1]",
    "SHZ[rp2]",
    "SHZ[rp1]",
    "SHPIX",
    "IP",
    "MSIRP[no set rp0]",
    "MSIRP[set rp0]",
    "ALIGNRP",
    "RTDG",
    "MIAP[no round]",
    "MIAP[round]",
    "NPUSHB",
    "NPUSHW",
    "WS",
    "RS",
    "WCVTP",
    "RCVT",
    "GC[cur]",
    "GC[orig]",
    "SCFS",
    "MD[grid]",
    "MD[orig]",
    "MPPEM",
    "MPS",
    "FLIPON",
    "FLIPOFF",
    "DEBUG",
    "LT",
    "LTEQ",
    "GT",
    "GTEQ",
    "EQ",
    "NEQ",
    "ODD",
    "EVEN",
    "IF",
    "EIF",
    "AND",
    "OR",
    "NOT",
    "DELTAP1",
    "SDB",
    "SDS",
    "ADD",
    "SUB",
    "DIV",
    "MUL",
    "ABS",
    "NEG",
    "FLOOR",
    "CEILING",
    "ROUND[Grey]",
    "ROUND[Black]",
    "ROUND[White]",
    "ROUND[Undef4]",
    "NROUND[Grey]",
    "NROUND[Black]",
    "NROUND[White]",
    "NROUND[Undef4]",
    "WCVTF",
    "DELTAP2",
    "DELTAP3",
    "DELTAC1",
    "DELTAC2",
    "DELTAC3",
    "SROUND",
    "S45ROUND",
    "JROT",
    "JROF",
    "ROFF",
    "Unknown7B",
    "RUTG",
    "RDTG",
    "SANGW",
    "AA",
    "FLIPPT",
    "FLIPRGON",
    "FLIPRGOFF",
    "Unknown83",
    "Unknown84",
    "SCANCTRL",
    "SDPVTL0",
    "SDPVTL1",
    "GETINFO",
    "IDEF",
    "ROLL",
    "MAX",
    "MIN",
    "SCANTYPE",
    "INSTCTRL",
    "Unknown8F",
    "Unknown90",
    "Unknown91",
    "Unknown92",
    "Unknown93",
    "Unknown94",
    "Unknown95",
    "Unknown96",
    "Unknown97",
    "Unknown98",
    "Unknown99",
    "Unknown9A",
    "Unknown9B",
    "Unknown9C",
    "Unknown9D",
    "Unknown9E",
    "Unknown9F",
    "UnknownA0",
    "UnknownA1",
    "UnknownA2",
    "UnknownA3",
    "UnknownA4",
    "UnknownA5",
    "UnknownA6",
    "UnknownA7",
    "UnknownA8",
    "UnknownA9",
    "UnknownAA",
    "UnknownAB",
    "UnknownAC",
    "UnknownAD",
    "UnknownAE",
    "UnknownAF",
    "PUSHB [1]",
    "PUSHB [2]",
    "PUSHB [3]",
    "PUSHB [4]",
    "PUSHB [5]",
    "PUSHB [6]",
    "PUSHB [7]",
    "PUSHB [8]",
    "PUSHW [1]",
    "PUSHW [2]",
    "PUSHW [3]",
    "PUSHW [4]",
    "PUSHW [5]",
    "PUSHW [6]",
    "PUSHW [7]",
    "PUSHW [8]",
    "MDRP[grey]",
    "MDRP[black]",
    "MDRP[white]",
    "MDRP03",
    "MDRP[round, grey]",
    "MDRP[round, black]",
    "MDRP[round, white]",
    "MDRP07",
    "MDRP[minimum, grey]",
    "MDRP[minimum, black]",
    "MDRP[minimum, white]",
    "MDRP0b",
    "MDRP[minimum, round, grey]",
    "MDRP[minimum, round, black]",
    "MDRP[minimum, round, white]",
    "MDRP0f",
    "MDRP[rp0, grey]",
    "MDRP[rp0, black]",
    "MDRP[rp0, white]",
    "MDRP13",
    "MDRP[rp0, round, grey]",
    "MDRP[rp0, round, black]",
    "MDRP[rp0, round, white]",
    "MDRP17",
    "MDRP[rp0, minimum, grey]",
    "MDRP[rp0, minimum, black]",
    "MDRP[rp0, minimum, white]",
    "MDRP1b",
    "MDRP[rp0, minimum, round, grey]",
    "MDRP[rp0, minimum, round, black]",
    "MDRP[rp0, minimum, round, white]",
    "MDRP1f",
    "MIRP[grey]",
    "MIRP[black]",
    "MIRP[white]",
    "MIRP03",
    "MIRP[round, grey]",
    "MIRP[round, black]",
    "MIRP[round, white]",
    "MIRP07",
    "MIRP[minimum, grey]",
    "MIRP[minimum, black]",
    "MIRP[minimum, white]",
    "MIRP0b",
    "MIRP[minimum, round, grey]",
    "MIRP[minimum, round, black]",
    "MIRP[minimum, round, white]",
    "MIRP0f",
    "MIRP[rp0, grey]",
    "MIRP[rp0, black]",
    "MIRP[rp0, white]",
    "MIRP13",
    "MIRP[rp0, round, grey]",
    "MIRP[rp0, round, black]",
    "MIRP[rp0, round, white]",
    "MIRP17",
    "MIRP[rp0, minimum, grey]",
    "MIRP[rp0, minimum, black]",
    "MIRP[rp0, minimum, white]",
    "MIRP1b",
    "MIRP[rp0, minimum, round, grey]",
    "MIRP[rp0, minimum, round, black]",
    "MIRP[rp0, minimum, round, white]",
    "MIRP1f"
};

static struct dup *makedup(int glyph, int uni, struct dup *prev) {
    struct dup *d = calloc(1,sizeof(struct dup));

    d->glyph = glyph;
    d->enc = uni;
    d->prev = prev;
return( d );
}

static void readttfencodings(FILE *ttf,FILE *util, struct ttfinfo *info) {
    int i,j;
    int nencs, version;
    int enc = 0;
    int platform, specific;
    int offset, encoff;
    int format, len;
    uint16_t table[256];
    int segCount;
    unsigned short *endchars, *startchars, *delta, *rangeOffset, *glyphs;
    int index;
    struct dup *dup;
    int vs_map = -1;

    fseek(ttf,info->encoding_start,SEEK_SET);
    printf( "\nEncoding (cmap) table (at %d)\n", info->encoding_start );
    version = getushort(ttf);
    nencs = getushort(ttf);
    if ( version!=0 && nencs==0 )
	nencs = version;		/* Sometimes they are backwards */
    for ( i=0; i<nencs; ++i ) {
	platform = getushort(ttf);
	specific = getushort(ttf);
	offset = (int) getlong(ttf);
	if ( platform==3 /*&& (specific==1 || specific==5)*/) {
	    enc = 1;
	    encoff = offset;
	} else if ( platform==1 && specific==0 && enc!=1 ) {
	    enc = 2;
	    encoff = offset;
	} else if ( platform==1 && specific==1 ) {
	    enc = 1;
	    encoff = offset;
	} else if ( platform==0 && specific!=5 ) {
	    enc = 1;
	    encoff = offset;
	}
	printf( "platform=%d specific=%d offset=%d ", platform, specific, offset );
	if ( platform==3 ) {
	    if ( specific==0 ) printf( "MS Symbol\n" );
	    else if ( specific==1 ) printf( "MS Unicode\n" );
	    else if ( specific==2 ) printf( "MS Shift JIS\n" );
	    else if ( specific==3 ) printf( "MS PRC\n" );
	    else if ( specific==4 ) printf( "MS Big5\n" );
	    else if ( specific==5 ) printf( "MS Wansung\n" );
	    else if ( specific==6 ) printf( "MS Johab\n" );
	    else if ( specific==10 ) printf( "MS UCS4\n" );
	    else printf( "???\n" );
	} else if ( platform==1 ) {
	    if ( specific==0 ) printf( "Mac Roman\n" );
	    else if ( specific==1 ) printf( "Japanese\n" );
	    else if ( specific==2 ) printf( "Traditional Chinese\n" );
	    else if ( specific==25 ) printf( "Simplified Chinese\n" );
	    else if ( specific==3 ) printf( "Korean\n" );
	} else if ( platform==0 ) {
	    if ( specific==0 ) printf( "Unicode Default\n" );
	    else if ( specific==1 ) printf( "Unicode 1.0\n" );
	    else if ( specific==2 ) printf( "Unicode 1.1\n" );
	    else if ( specific==3 ) printf( "Unicode 2.0+\n" );
	    else if ( specific==4 ) printf( "Unicode UCS4\n" );
	    else if ( specific==5 ) printf( "Unicode Variations\n" );
	    else printf( "???\n" );
	    if ( specific==5 )
		vs_map = offset;
	} else printf( "???\n" );
    }
    if ( enc!=0 ) {
	info->glyph_unicode = calloc(info->glyph_cnt,sizeof(unsigned int));
	fseek(ttf,info->encoding_start+encoff,SEEK_SET);
	format = getushort(ttf);
	if ( format!=12 && format!=10 && format!=8 ) {
	    len = getushort(ttf);
	    printf( " Format=%d len=%d Language=%x\n", format, len, (unsigned int)(getushort(ttf)) );
	} else {
	    /* padding */ getushort(ttf);
	    len = (int) getlong(ttf);
	    printf( " Format=%d len=%d Language=%x\n", format, len, (unsigned int)(getlong(ttf)) );
	}
	if ( format==0 ) {
	    printf( "  Table: " );
	    for ( i=0; i<len-6; ++i )
		printf( "%d ", table[i] = getc(ttf));
	    putchar('\n');
	    for ( i=0; i<256 && i<info->glyph_cnt && i<len-6; ++i )
		info->glyph_unicode[table[i]] = i;
	} else if ( format==4 ) {
	    segCount = getushort(ttf)/2;
	    /* searchRange = */ getushort(ttf);
	    /* entrySelector = */ getushort(ttf);
	    /* rangeShift = */ getushort(ttf);
	    endchars = malloc(segCount*sizeof(unsigned short));
	    for ( i=0; i<segCount; ++i )
		endchars[i] = getushort(ttf);
	    if ( getushort(ttf)!=0 )
		fprintf(stderr,"Expected 0 in true type font");
	    startchars = malloc(segCount*sizeof(unsigned short));
	    for ( i=0; i<segCount; ++i )
		startchars[i] = getushort(ttf);
	    delta = malloc(segCount*sizeof(unsigned short));
	    for ( i=0; i<segCount; ++i )
		delta[i] = getushort(ttf);
	    rangeOffset = malloc(segCount*sizeof(unsigned short));
	    for ( i=0; i<segCount; ++i )
		rangeOffset[i] = getushort(ttf);
	    len -= 8*sizeof(unsigned short) +
		    4*segCount*sizeof(unsigned short);
	    if ( len<0 ) {
		fprintf( stderr, "This font has an illegal format 4 subtable with too little space for all the segments.\n" );
		free(startchars); free(endchars); free(delta); free(rangeOffset);
return;
	    }
	    /* that's the amount of space left in the subtable and it must */
	    /*  be filled with glyphIDs */
	    glyphs = malloc(len);
	    for ( i=0; i<len/2; ++i )
		glyphs[i] = getushort(ttf);

	    printf( "Format 4 (Windows unicode), %d segments\n", segCount );

	    for ( i=0; i<segCount; ++i ) {
		printf( "  Segment=%d unicode-start=%04x end=%04x range-offset=%d delta=%d",
			i, startchars[i], endchars[i], rangeOffset[i], delta[i]);
		if ( rangeOffset[i]==0 )
		    printf( " glyph-start=%d gend=%d\n", startchars[i]+delta[i], endchars[i]+delta[i]);
		else if ( rangeOffset[i]!=0xffff ) {
		    index = glyphs[ (i-segCount+rangeOffset[i]/2)];
		    if ( index!=0 ) index += delta[i];
		    printf( " glyph-start=%d", index );
		    index = glyphs[ (i-segCount+rangeOffset[i]/2) + endchars[i]-startchars[i]];
		    if ( index!=0 ) index += delta[i];
		    printf( " end=%d\n", index);
		} else
		    printf ( " End\n" );

		if ( rangeOffset[i]==0 ) {
		    for ( j=startchars[i]; j<=endchars[i]; ++j ) {
			if ( ((unsigned short) (j+delta[i]))>=info->glyph_cnt )
			    printf( "!!! Glyph index out of bounds! %d\n", (unsigned short) (j+delta[i]) );
			else if ( info->glyph_unicode[(unsigned short) (j+delta[i])] )
			    info->dups = makedup((unsigned short) (j+delta[i]),j,info->dups);
			else
			    info->glyph_unicode[(unsigned short) (j+delta[i])] = j;
		    }
		} else if ( rangeOffset[i]!=0xffff ) {
		    /* It isn't explicitly mentioned by a rangeOffset of 0xffff*/
		    /*  means no glyph */
		    for ( j=startchars[i]; j<=endchars[i]; ++j ) {
			index = glyphs[ (i-segCount+rangeOffset[i]/2) +
					    j-startchars[i] ];
			if ( index!=0 ) {
			    index = (unsigned short) (index+delta[i]);
			    if ( index>=info->glyph_cnt || index<0 )
				printf( "!!! Glyph index out of bounds! %d\n", index );
			    else if ( info->glyph_unicode[index]!=0 )
				info->dups = makedup(index,j,info->dups);
			    else
				info->glyph_unicode[index] = j;
			}
		    }
		}
	    }
	    free(glyphs);
	    free(rangeOffset);
	    free(delta);
	    free(startchars);
	    free(endchars);
	} else if ( format==6 ) {
	    /* Apple's unicode format */
	    /* Well, the docs say it's for 2byte encodings, but Apple actually*/
	    /*  uses it for 1 byte encodings which don't fit into the require-*/
	    /*  ments for a format 0 sub-table. See Zapfino.dfont */
	    int first, count;
	    first = getushort(ttf);
	    count = getushort(ttf);
	    for ( i=0; i<count; ++i )
		info->glyph_unicode[getushort(ttf)] = first+i;
	} else if ( format==2 ) {
	    int max_sub_head_key = 0, cnt, last;
	    struct subhead { uint16_t first, cnt, delta, rangeoff; } *subheads;

	    for ( i=0; i<256; ++i ) {
		table[i] = getushort(ttf)/8;	/* Sub-header keys */
		if ( table[i]>max_sub_head_key )
		    max_sub_head_key = table[i];	/* The entry is a byte pointer, I want a pointer in units of struct subheader */
	    }
	    subheads = malloc((max_sub_head_key+1)*sizeof(struct subhead));
	    for ( i=0; i<=max_sub_head_key; ++i ) {
		subheads[i].first = getushort(ttf);
		subheads[i].cnt = getushort(ttf);
		subheads[i].delta = getushort(ttf);
		subheads[i].rangeoff = (getushort(ttf)-
				(max_sub_head_key-i)*sizeof(struct subhead)-
				sizeof(short))/sizeof(short);
	    }
	    cnt = (len-(ftell(ttf)-(info->encoding_start+encoff)))/sizeof(short);
	    /* The count is the number of glyph indexes to read. it is the */
	    /*  length of the entire subtable minus that bit we've read so far */
	    glyphs = malloc(cnt*sizeof(short));
	    for ( i=0; i<cnt; ++i )
		glyphs[i] = getushort(ttf);
	    last = -1;
	    for ( i=0; i<256; ++i ) {
		if ( table[i]==0 ) {
		    /* Special case, single byte encoding entry, look i up in */
		    /*  subhead */
		    /* In the one example I've got of this encoding (wcl-02.ttf) the chars */
		    /* 0xfd, 0xfe, 0xff are said to exist but there is no mapping */
		    /* for them. */
		    if ( last!=-1 )
			index = 0;	/* the subhead says there are 256 entries, but in fact there are only 193, so attempting to find these guys should give an error */
		    else if ( i<subheads[0].first || i>=subheads[0].first+subheads[0].cnt ||
			    subheads[0].rangeoff+(i-subheads[0].first)>=cnt )
			index = 0;
		    else if ( (index = glyphs[subheads[0].rangeoff+(i-subheads[0].first)])!= 0 )
			index = (uint32_t) (index+subheads[0].delta);
		    /* I assume the single byte codes are just ascii or latin1*/
		    if ( index!=0 && index<info->glyph_cnt ) {
			if ( info->glyph_unicode[index]==0 )
			    info->glyph_unicode[index] = i;
			else
			    info->dups = makedup(index,i,info->dups);
		    }
		} else {
		    int enc0, k = table[i];
		    for ( j=0; j<subheads[k].cnt; ++j ) {
			if ( subheads[k].rangeoff+j>=cnt )
			    index = 0;
			else if ( (index = glyphs[subheads[k].rangeoff+j])!= 0 )
			    index = (uint16_t) (index+subheads[k].delta);
			if ( index!=0 && index<info->glyph_cnt ) {
			    enc0 = (i<<8)|(j+subheads[k].first);
			    if ( info->glyph_unicode[index]==0 )
				info->glyph_unicode[index] = enc0;
			    else
				info->dups = makedup(index,enc0,info->dups);
			}
		    }
		    if ( last==-1 ) last = i;
		}
	    }
	    free(subheads);
	    free(glyphs);
	} else if ( format==12 ) {
	    uint32_t ngroups, start, end, startglyph;
	    ngroups = getlong(ttf);
	    for ( j=0; j<ngroups; ++j ) {
		start = getlong(ttf);
		end = getlong(ttf);
		startglyph = getlong(ttf);
		for ( i=start; i<=end; ++i ) {
		    info->glyph_unicode[startglyph+i-start] = i;
		}
	    }
	} else if ( format==8 ) {
	    fprintf(stderr,"I don't support mixed 16/32 bit characters (no unicode surogates), format=%d", format);
	} else if ( format==10 ) {
	    fprintf(stderr,"I don't support 32 bit characters format=%d", format);
	} else {
	    fprintf(stderr,"Eh? Unknown encoding format=%d", format);
	}
    }

    if ( info->glyph_unicode!=NULL ) {
	for ( i=0; i<info->glyph_cnt && (format!=0 || i<256); ++i )
	    printf( " Glyph %d -> U+%04X\n", i, info->glyph_unicode[i]);
	for ( i=0; i<info->glyph_cnt; ++i ) if ( info->glyph_unicode[i]!=0 ) {
	    for ( j=i+1; j<info->glyph_cnt; ++j )
		if ( info->glyph_unicode[i]==info->glyph_unicode[j] )
		    printf( "!!!Two glyphs %d and %d have the same encoding U+%04X",
			    i, j, info->glyph_unicode[i]);
	}
	if ( info->dups!=NULL ) {
	    printf("Some glyphs have multiple encodings:\n" );
	    for ( dup = info->dups; dup!=NULL ; dup=dup->prev )
		printf( " Glyph %d -> U+%04X (primary=U+%04X)\n",
			dup->glyph,
			(unsigned int)(dup->enc),
			(unsigned int)(info->glyph_unicode[dup->glyph]) );
	}
    } else
	printf( "Could not understand encoding table\n" );
    if ( vs_map!=-1 ) {
	struct vs_data { int vs; uint32_t defoff, nondefoff; } *vs_data;
	int cnt, rcnt;

	fseek(ttf,info->encoding_start+vs_map,SEEK_SET);
	if ( getushort(ttf)!=14 ) {
	    fprintf( stderr, "A unicode variation subtable (platform=0,specific=5) must have format=14\n" );
return;
	}
	len = getlong(ttf);
	cnt = getlong(ttf);
	printf( " Format=14 len=%d\n numVarSelRec=%d\n", len, cnt );
	vs_data = malloc( cnt*sizeof(struct vs_data));
	for ( i=0; i<cnt; ++i ) {
	    vs_data[i].vs = get3byte(ttf);
	    vs_data[i].defoff = getlong(ttf);
	    vs_data[i].nondefoff = getlong(ttf);
	    printf( "  varSelector=%04x, defaultUVSOffset=%d nonDefaultOffset=%d\n",
		    (unsigned int)(vs_data[i].vs),
		    (int)(vs_data[i].defoff),
		    (int)(vs_data[i].nondefoff) );
	}
	for ( i=0; i<cnt; ++i ) {
	    printf( " Variation Selector=%04x\n", (unsigned int)(vs_data[i].vs) );
	    if ( vs_data[i].defoff!=0 ) {
		fseek(ttf,info->encoding_start+vs_map+vs_data[i].defoff,SEEK_SET);
		rcnt = getlong(ttf);
		printf( "  Default glyphs\n  %d ranges\n", rcnt );
		for ( j=0; j<rcnt; ++j ) {
		    int uni = get3byte(ttf);
		    printf( "   U+%04x and %d code points following that\n",
			    (unsigned int)(uni), getc(ttf));
		}
	    }
	    if ( vs_data[i].nondefoff!=0 ) {
		fseek(ttf,info->encoding_start+vs_map+vs_data[i].nondefoff,SEEK_SET);
		rcnt = getlong(ttf);
		printf( "  non Default glyphs\n  %d mappings\n", rcnt );
		for ( j=0; j<rcnt; ++j ) {
		    int uni = get3byte(ttf);
		    printf( "   U+%04x.U+%04x -> GID %d\n",
			    (unsigned int)(uni), (unsigned int)(vs_data[i].vs), getushort(ttf) );
		}
	    }
	}
    }
}

static void readttfpost(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int i,j,ind, extras;
    int format, gc, len;
    uint16_t *indexes, *glyphs;
    char **names, *name;

    fseek(ttf,info->postscript_start,SEEK_SET);
    printf( "\npost table (at %d)\n", info->postscript_start );
    printf( "\t format=%08x\n", (unsigned int)((format = (int) getlong(ttf))) );
    printf( "\t italicAngle=%g\n", getfixed(ttf));
    printf( "\t underlinePos=%d\n", (short) getushort(ttf));
    printf( "\t underlineWidth=%d\n", getushort(ttf));
    printf( "\t fixedpitch=%d\n", (int) getlong(ttf));
    printf( "\t mem1=%d\n", (int) getlong(ttf));
    printf( "\t mem2=%d\n", (int) getlong(ttf));
    printf( "\t mem3=%d\n", (int) getlong(ttf));
    printf( "\t mem4=%d\n", (int) getlong(ttf));

    gc = 0; names = NULL;
    if ( format==0x00030000 ) {
	/* Used in Open Type, seems only to contain the above stuff */
	/* (no names, they're in the cff section) */
    } else if ( format==0x00020000 ) {
	gc = getushort(ttf);
	if ( gc!=info->glyph_cnt )
	    fprintf( stderr, "!!!! Post table glyph count does not match that in 'maxp'\n" );
	indexes = calloc(65536,sizeof(uint16_t));
	glyphs = calloc(gc,sizeof(uint16_t));
	names = calloc(gc<info->glyph_cnt?info->glyph_cnt:gc,sizeof(char *));
	/* the index table is backwards from the way I want to use it */
	extras = 0;
	for ( i=0; i<gc; ++i ) {
	    ind = getushort(ttf);
	    if ( ind>=258 ) ++extras;
	    indexes[ind] = i;
	    glyphs[i] = ind;
	}
	for ( i=258; i<258+extras; ++i ) {
	    if ( indexes[i]==0 )
	break;
	    len = getc(ttf);
	    name = malloc(len+1);
	    for ( j=0; j<len; ++j )
		name[j] = getc(ttf);
	    name[j] = '\0';
	    names[indexes[i]] = name;
	}
	for ( i=0; i<gc; ++i ) {
	    printf( "Glyph %d (name index=%d) -> \"", i, glyphs[i] );
	    if ( names[i]!=NULL )
		printf( "%s", names[i]);
	    else if ( glyphs[i]<258 ) {
		printf( "%s", standardnames[glyphs[i]]);
		names[i] = strdup(standardnames[glyphs[i]]);
	    } else
		printf( "Gleep!!!!! %d", glyphs[i]);
	    if ( info->glyph_unicode!=NULL )
		printf("\"\t  U+%04X\n", info->glyph_unicode[i]);
	    else
		printf( "\"\n" );
	}
	free(indexes);
	free(glyphs);
	info->glyph_names = names;
    }
    if ( info->glyph_unicode!=NULL ) {
	if ( info->glyph_names==NULL )
	    info->glyph_names = calloc(info->glyph_cnt,sizeof(char *));
	for ( i=0; i<info->glyph_cnt ; ++i ) {
	    if ( info->glyph_names[i]!=NULL )
		/* Keep it */;
	    else if ( info->glyph_unicode[i]!=0 && info->glyph_unicode[i]!=0xffff ) {
		char buffer[40];
		sprintf(buffer,">U+%04X<", info->glyph_unicode[i]);
		info->glyph_names[i] = strdup(buffer);
	    }
	}
    } else if ( info->glyph_names!=NULL ) {
	for ( i=0; i<gc; ++i )
	    free(names[i]);
	free(names);
    }
}

static void showlangsys(FILE *ttf,int script_start, uint16_t ls_off, uint32_t ls_name ) {
    int i,cnt;

    if ( ls_name==0 )
	printf( "\t Language System table for default language\n" );
    else
	printf( "\t Language System table for '%c%c%c%c'\n",
		(char)((ls_name>>24)&0xff), (char)((ls_name>>16)&0xff),
		(char)((ls_name>>8)&0xff), (char)(ls_name&0xff) );
    fseek(ttf,script_start+ls_off,SEEK_SET);
    printf( "\t  LookupOrder=%d\n", getushort(ttf));
    printf( "\t  Required Feature Index=%d\n", (short) getushort(ttf));
    printf( "\t  Feature Count=%d\n", cnt = getushort(ttf));
    for ( i=0; i<cnt; ++i )
	printf( "\t   Feature %d Offset=%d\n", i, getushort(ttf));
}

static void showscriptlist(FILE *ttf,int script_start ) {
    int cnt,i, j;
    uint16_t *script_table_offsets;
    uint32_t *script_table_names;
    int dlo, ls_cnt;
    uint32_t *ls_names;
    uint16_t *ls_offsets;

    fseek(ttf,script_start,SEEK_SET);
    printf( "\tScript List\n" );
    printf( "\t script count=%d\n", cnt=getushort(ttf));
    script_table_offsets = malloc(cnt*sizeof(uint16_t));
    script_table_names = malloc(cnt*sizeof(uint32_t));
    for ( i=0; i<cnt; ++i ) {
	script_table_names[i] = getlong(ttf);
	script_table_offsets[i] = getushort(ttf);
	printf( "\t Script[%d] '%c%c%c%c' Offset=%d\n", i,
		(char)((script_table_names[i]>>24)&0xff),
		(char)((script_table_names[i]>>16)&0xff),
		(char)((script_table_names[i]>>8)&0xff),
		(char)(script_table_names[i]&0xff),
		script_table_offsets[i] );
    }
    printf( "\t--\n" );
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,script_start+script_table_offsets[i],SEEK_SET);
	printf( "\t Script table for '%c%c%c%c'\n",
		(char)((script_table_names[i]>>24)&0xff),
		(char)((script_table_names[i]>>16)&0xff),
		(char)((script_table_names[i]>>8)&0xff),
		(char)(script_table_names[i]&0xff) );
	printf( "\t  default language offset=%d\n", dlo=getushort(ttf));
	printf( "\t  language systems count=%d\n", ls_cnt = getushort(ttf));
	ls_names = malloc(ls_cnt*sizeof(uint32_t));
	ls_offsets = malloc(ls_cnt*sizeof(uint16_t));
	for ( j=0; j<ls_cnt; ++j ) {
	    ls_names[j] = getlong(ttf);
	    ls_offsets[j] = getushort(ttf);
	    printf( "\t   Language System '%c%c%c%c' Offset=%d\n",
		    (char)((ls_names[j]>>24)&0xff),
		    (char)((ls_names[j]>>16)&0xff),
		    (char)((ls_names[j]>>8)&0xff),
		    (char)(ls_names[j]&0xff),
		    ls_offsets[j] );
	}
	if ( dlo!=0 )
	    showlangsys(ttf,script_start+script_table_offsets[i],dlo,0);
	for ( j=0; j<ls_cnt; ++j )
	    showlangsys(ttf,script_start+script_table_offsets[i],ls_offsets[j],ls_names[j]);
	free(ls_names); free(ls_offsets);
    }
    free(script_table_offsets);
    free(script_table_names);
}

static void showfeaturelist(FILE *ttf,int feature_start ) {
    int cnt,i, j, lu_cnt;
    uint32_t *feature_record_names;
    uint16_t *feature_record_offsets;
    uint16_t *lu_offsets;

    fseek(ttf,feature_start,SEEK_SET);
    printf( "\tFeature List\n" );
    printf( "\t feature count=%d\n", cnt=getushort(ttf));
    feature_record_offsets = malloc(cnt*sizeof(uint16_t));
    feature_record_names = malloc(cnt*sizeof(uint32_t));
    for ( i=0; i<cnt; ++i ) {
	feature_record_names[i] = getlong(ttf);
	feature_record_offsets[i] = getushort(ttf);
	printf( "\t Feature[%d] '%c%c%c%c' Offset=%d\n", i,
		(char)((feature_record_names[i]>>24)&0xff),
		(char)((feature_record_names[i]>>16)&0xff),
		(char)((feature_record_names[i]>>8)&0xff),
		(char)(feature_record_names[i]&0xff),
		feature_record_offsets[i] );
    }
    printf( "\t--\n" );
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,feature_start+feature_record_offsets[i],SEEK_SET);
	printf( "\t Feature Table[%d] '%c%c%c%c'\n", i,
		(char)((feature_record_names[i]>>24)&0xff),
		(char)((feature_record_names[i]>>16)&0xff),
		(char)((feature_record_names[i]>>8)&0xff),
		(char)(feature_record_names[i]&0xff) );
	printf( "\t  Feature Parameters Offset=%d\n", getushort(ttf));
	printf( "\t  Lookup Count = %d\n", lu_cnt = getushort(ttf));
	if ( i+1<cnt && feature_record_offsets[i]<feature_record_offsets[i+1] &&
		feature_record_offsets[i]+4+2*lu_cnt>feature_record_offsets[i+1] )
	    printf( "!!!! Bad lookup count. More lookups than there is space for!!!!\n" );
	lu_offsets = malloc(lu_cnt*sizeof(uint16_t));
	for ( j=0; j<lu_cnt; ++j ) {
	    printf( "\t   Lookup List Offset[%d] = %d\n", j,
		    lu_offsets[j] = getushort(ttf));
	}
	free(lu_offsets);
    }
    free( feature_record_offsets );
    free( feature_record_names );
}

static uint16_t *showCoverageTable(FILE *ttf, int coverage_offset, int specified_cnt) {
    int format, cnt, i,j, rcnt;
    uint16_t *glyphs=NULL;
    int start, end, ind, max;

    fseek(ttf,coverage_offset,SEEK_SET);
    printf( "\t   Coverage Table\n" );
    printf( "\t    Format=%d\n", format = getushort(ttf));
    if ( format==1 ) {
	printf("\t    Glyph Count=%d\n\t     ", cnt = getushort(ttf));
	glyphs = malloc((cnt+1)*sizeof(uint16_t));
	for ( i=0; i<cnt; ++i )
	    printf( "%d ", glyphs[i] = getushort(ttf));
	glyphs[i] = 0xffff;
	putchar('\n');
    } else if ( format==2 ) {
	glyphs = malloc((max=256)*sizeof(uint16_t));
	printf("\t    Range Count=%d\n\t     ", rcnt = getushort(ttf));
	cnt = 0;
	for ( i=0; i<rcnt; ++i ) {
	    printf( "\t     Range [%d] Start=%d ", i, start = getushort(ttf));
	    printf( "End=%d ", end = getushort(ttf));
	    printf( "Index=%d\n", ind = getushort(ttf));
	    if ( ind+end-start+2 >= max ) {
		max = ind+end-start+2;
		glyphs = realloc(glyphs,max*sizeof(uint16_t));
	    }
	    for ( j=start; j<=end; ++j )
		glyphs[j-start+ind] = j;
	    if ( end-start+1+ind>cnt ) {
		glyphs[end-start+1+ind] = 0xffff;
		cnt = end-start+1+ind;
	    }
	}
    }
    if ( specified_cnt>=0 && cnt!=specified_cnt )
	fprintf(stderr, "! > Bad coverage table: Calculated Count(%d) does not match specified (%d)\n",
		cnt, specified_cnt );
return( glyphs );
}

static uint16_t *getClassDefTable(FILE *ttf, int classdef_offset, int cnt) {
    int format, i, j;
    uint16_t start, glyphcnt, rangecnt, end, class;
    uint16_t *glist=NULL;

    fseek(ttf, classdef_offset, SEEK_SET);
    glist = malloc(cnt*sizeof(uint16_t));
    for ( i=0; i<cnt; ++i )
	glist[i] = 0;	/* Class 0 is default */
    format = getushort(ttf);
    if ( format==1 ) {
	start = getushort(ttf);
	glyphcnt = getushort(ttf);
	if ( start+glyphcnt>cnt ) {
	    fprintf( stderr, "Bad class def table. start=%d cnt=%d, max glyph=%d\n", start, glyphcnt, cnt );
	    glyphcnt = cnt-start;
	}
	for ( i=0; i<glyphcnt; ++i )
	    glist[start+i] = getushort(ttf);
    } else if ( format==2 ) {
	rangecnt = getushort(ttf);
	for ( i=0; i<rangecnt; ++i ) {
	    start = getushort(ttf);
	    end = getushort(ttf);
	    if ( start>end || end>=cnt )
		fprintf( stderr, "Bad class def table. Glyph range %d-%d out of range [0,%d)\n", start, end, cnt );
	    class = getushort(ttf);
	    for ( j=start; j<=end; ++j )
		glist[j] = class;
	}
    }
return glist;
}

static void readvaluerecord(int vf,FILE *ttf,const char *label) {
    printf( "\t\t %s: ", label );
    if ( vf&1 )
	printf( "XPlacement: %d  ", (short) getushort(ttf));
    if ( vf&2 )
	printf( "YPlacement: %d  ", (short) getushort(ttf));
    if ( vf&4 )
	printf( "XAdvance: %d  ", (short) getushort(ttf));
    if ( vf&8 )
	printf( "YAdvance: %d  ", (short) getushort(ttf));
    if ( vf&0x10 )
	printf( "XPlacementDevOff: %d  ", getushort(ttf));
    if ( vf&0x20 )
	printf( "YPlacementDevOff: %d  ", getushort(ttf));
    if ( vf&0x40 )
	printf( "XAdvanceDevOff: %d  ", getushort(ttf));
    if ( vf&0x80 )
	printf( "YAdvanceDevOff: %d  ", getushort(ttf));
    printf( "\n" );
}

static void PrintGlyphs(uint16_t *glyphs, struct ttfinfo *info) {
    if ( glyphs==NULL )
	printf( "<Empty>\n" );
    else if ( info->glyph_names!=NULL ) {
	int i;
	printf( "      " );
	for ( i=0; glyphs[i]!=0xffff; ++i ) {
	    printf( "%s ", info->glyph_names[glyphs[i]]);
	}
	printf("\n" );
    }
}

static void gposPairSubTable(FILE *ttf, int which, int stoffset, struct ttfinfo *info) {
    int coverage, cnt, i, subformat, vf1, vf2, j, pair_cnt;
    uint16_t *glyphs = NULL;
    uint16_t *ps_offsets;

    printf( "\t  Pair Sub Table[%d]\n", which );
    printf( "\t   SubFormat=%d\n", subformat = getushort(ttf));
    printf( "\t   Coverage Offset=%d\n", coverage = getushort(ttf));
    printf( "\t   ValueFormat1=0x%x ", (unsigned int)((vf1 = getushort(ttf))) );
    printf( "%s%s%s%s%s%s%s%s\n", 
	    (vf1&1) ? "XPlacement|":"",
	    (vf1&2) ? "YPlacement|":"",
	    (vf1&4) ? "XAdvance|":"",
	    (vf1&8) ? "YAdvance|":"",
	    (vf1&0x10) ? "XDevPlacement|":"",
	    (vf1&0x20) ? "YDevPlacement|":"",
	    (vf1&0x40) ? "XDevAdvance|":"",
	    (vf1&0x80) ? "YDevAdvance|":"" );
    printf( "\t   ValueFormat2=0x%x ", (unsigned int)(vf2 = getushort(ttf)) );
    printf( "%s%s%s%s%s%s%s%s\n",
	    (vf2&1) ? "XPlacement|":"",
	    (vf2&2) ? "YPlacement|":"",
	    (vf2&4) ? "XAdvance|":"",
	    (vf2&8) ? "YAdvance|":"",
	    (vf2&0x10) ? "XDevPlacement|":"",
	    (vf2&0x20) ? "YDevPlacement|":"",
	    (vf2&0x40) ? "XDevAdvance|":"",
	    (vf2&0x80) ? "YDevAdvance|":"" );
    if ( subformat==1 ) {
	printf( "\t   PairSetCnt=%d\n", cnt = getushort(ttf));
	ps_offsets = calloc(cnt,sizeof(short));
	for ( i=0; i<cnt; ++i )
	    ps_offsets[i] = getushort(ttf);
	glyphs = showCoverageTable(ttf,stoffset+coverage, cnt);
	for ( i=0; i<cnt; ++i ) {
	    fseek(ttf,stoffset+ps_offsets[i],SEEK_SET);
	    printf( "\t    PairCnt=%d\n", pair_cnt = getushort(ttf));
	    for ( j=0; j<pair_cnt; ++j ) {
		int glyph2 = getushort(ttf);
		printf( "\t\tGlyph %d (%s) -> %d (%s)\n", glyphs[i],
			glyphs[i]>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names==NULL? "" : info->glyph_names[glyphs[i]],
			glyph2,
			glyph2>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[glyph2]);
		readvaluerecord(vf1,ttf,"First");
		readvaluerecord(vf2,ttf,"Second");
	    }
	}
	free(ps_offsets);
    } else if ( subformat==2 ) {
	printf( "\t   Class based kerning (not displayed)\n" );
	glyphs = showCoverageTable(ttf,stoffset+coverage,-1);
	PrintGlyphs(glyphs,info);
    } else {
	printf( "\t   !!! unknown sub-table format !!!!\n" );
    }
    free(glyphs);
}

static void ShowAttach(FILE *ttf) {
    int format = getushort(ttf);
    int x = getushort(ttf);
    int y = getushort(ttf);

    if ( format==1 )
	printf( "Attach at (%d,%d)\n", x, y );
    else if ( format==2 )
	printf( "Attach at (%d,%d pt=%d)\n", x, y, getushort(ttf) );
    else if ( format==3 ) {
	printf( "Attach at (%d,%d XDeviceOff=%d", x, y, getushort(ttf) );
	printf( " YDeviceOff=%d)\n", getushort(ttf) );
    } else
	printf( "Unknown attachment format %d\n", format );
}

static void gposMarkToBaseSubTable(FILE *ttf, int which, int stoffset, struct ttfinfo *info, int m2b) {
    int mcoverage, bcoverage, classcnt, markoff, baseoff;
    uint16_t *mglyphs, *bglyphs;
    int i, j;
    uint16_t *offsets;
    uint32_t pos;

    printf( m2b ? "\t  Mark To Base Sub Table[%d]\n" : "\t  Mark To Mark Sub Table[%d]\n", which );
    printf( "\t   SubFormat=%d\n", getushort(ttf));
    printf( "\t   Mark Coverage Offset=%d\n", mcoverage = getushort(ttf));
    printf( "\t   Base Coverage Offset=%d\n", bcoverage = getushort(ttf));
    printf( "\t   Class Count=%d\n", classcnt = getushort(ttf));
    printf( "\t   Mark Offset=%d\n", markoff = getushort(ttf));
    printf( "\t   Base Offset=%d\n", baseoff = getushort(ttf));
    printf( "\t   Mark Glyphs\n" );
    mglyphs = showCoverageTable(ttf,stoffset+mcoverage, -1); /* Class cnt is not the count of marks */
    printf( "\t   Base Glyphs\n" );
    bglyphs = showCoverageTable(ttf,stoffset+bcoverage, -1);
    fseek(ttf,stoffset+baseoff,SEEK_SET);
    printf( "\t    Base Glyph Count=%d\n", getushort(ttf));
    offsets = malloc(classcnt*sizeof(uint16_t));
    for ( i=0; bglyphs[i]!=0xffff; ++i ) {
	printf( "\t\tBase Glyph %d (%s)\n", bglyphs[i],
		bglyphs[i]>=info->glyph_cnt ? "!!! Bad glyph !!!" :
		info->glyph_names == NULL ? "" : info->glyph_names[bglyphs[i]]);
	for ( j=0; j<classcnt; ++j )
	    offsets[j] = getushort(ttf);
	pos = ftell(ttf);
	for ( j=0; j<classcnt; ++j ) {
	    if ( offsets[j]!=0 ) {
		printf( "\t\t\tClass=%d  Offset=%d  ", j, offsets[j]);
		fseek(ttf,stoffset+baseoff+offsets[j],SEEK_SET);
		ShowAttach(ttf);
	    }
	}
	fseek(ttf,pos,SEEK_SET);
    }
    fseek(ttf,stoffset+markoff,SEEK_SET);
    printf( "\t    Mark Glyph Count=%d\n", getushort(ttf));
    for ( i=0; mglyphs[i]!=0xffff; ++i ) {
	printf( "\t\tMark Glyph %d (%s)\n", mglyphs[i],
		mglyphs[i]>=info->glyph_cnt ? "!!! Bad glyph !!!" :
		info->glyph_names == NULL ? "" : info->glyph_names[mglyphs[i]]);
	printf( "\t\t\tClass=%d  ", getushort(ttf));
	offsets[0] = getushort(ttf);
	pos = ftell(ttf);
	if ( offsets[0]!=0 ) {
	    printf( "Offset=%d  ", offsets[0]);
	    fseek(ttf,stoffset+markoff+offsets[0],SEEK_SET);
	    ShowAttach(ttf);
	}
	fseek(ttf,pos,SEEK_SET);
    }
    free(offsets);
    free(mglyphs);
    free(bglyphs);
}

static void gsubSingleSubTable(FILE *ttf, int which, int stoffset, struct ttfinfo *info) {
    int coverage, cnt, i, type;
    uint16_t *glyphs = NULL;

    printf( "\t  Single Sub Table[%d] (variant forms)\n", which );
    printf( "\t   Type=%d\n", type = getushort(ttf));
    printf( "\t   Coverage Offset=%d\n", coverage = getushort(ttf));
    if ( type==1 ) {
	uint16_t delta = getushort(ttf);
	printf( "\t   Delta=%d\n", delta);
	glyphs = showCoverageTable(ttf,stoffset+coverage, -1);
	printf( "\t   Which means ...\n" );
	for ( i=0; glyphs[i]!=0xffff; ++i )
	    printf( "\t\tGlyph %d (%s) -> %d (%s)\n", glyphs[i],
		    glyphs[i]>=info->glyph_cnt ? "!!! Bad glyph !!!" : info->glyph_names == NULL ? "" : info->glyph_names[glyphs[i]],
		    (uint16_t) (glyphs[i]+delta),
		    (uint16_t) (glyphs[i]+delta)>=info->glyph_cnt ? "!!! Bad glyph !!!" : info->glyph_names == NULL ? "" : info->glyph_names[(uint16_t) (glyphs[i]+delta)]);
    } else {
	int here;
	printf( "\t   Count=%d\n", cnt = getushort(ttf));
	here = ftell(ttf);
	glyphs = showCoverageTable(ttf,stoffset+coverage, cnt);
	fseek(ttf,here,SEEK_SET);
	for ( i=0; i<cnt; ++i ) {
	    int val = getushort(ttf);
	    printf( "\t\tGlyph %d (%s) -> %d (%s)\n", glyphs[i],
		    glyphs[i]>=info->glyph_cnt ? "!!! Bad glyph !!!" : info->glyph_names == NULL ? "" : info->glyph_names[glyphs[i]],
		    val,
		    val >= info->glyph_cnt ? "!!! Bad glyph !!!" : info->glyph_names == NULL ? "" : info->glyph_names[val]);
	}
    }
    free(glyphs);
}

static void gsubMultipleSubTable(FILE *ttf, int which, int stoffset, struct ttfinfo *info) {
    int coverage, cnt, i, j, glyph_cnt;
    uint16_t *seq_offsets;
    uint16_t *glyphs;

    printf( "\t  Multiple Sub Table[%d] (ligature decomposition)\n", which );
    printf( "\t   Type=%d\n", getushort(ttf));
    printf( "\t   Coverage Offset=%d\n", coverage = getushort(ttf));
    printf( "\t   Count=%d\n", cnt = getushort(ttf));
    seq_offsets = malloc(cnt*sizeof(uint16_t));
    for ( i=0; i<cnt; ++i )
	printf( "\t    Sequence Offsets[%d]=%d\n", i, seq_offsets[i]=getushort(ttf));
    glyphs = showCoverageTable(ttf,stoffset+coverage, cnt);
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,stoffset+seq_offsets[i],SEEK_SET);
	printf( "\t    Glyph sequence[%d] for glyph %d (%s)\n", i, glyphs[i], glyphs[i]>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[glyphs[i]]);
	printf( "\t     Count=%d\n", glyph_cnt = getushort(ttf));
	printf( "\t     Glyph %d (%s) -> ", glyphs[i], glyphs[i]>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[glyphs[i]] );
	for ( j=0; j<glyph_cnt; ++j ) {
	    int de = getushort(ttf);
	    printf( "%d (%s) ", de, de>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[de]);
	}
	putchar('\n');
    }
    free(glyphs);
    free(seq_offsets);
}

static void gsubAlternateSubTable(FILE *ttf, int which, int stoffset, struct ttfinfo *info) {
    int coverage, cnt, i, j, glyph_cnt;
    uint16_t *seq_offsets;
    uint16_t *glyphs;

    printf( "\t  Alternate Sub Table[%d] (variant forms)\n", which );
    printf( "\t   Type=%d\n", getushort(ttf));
    printf( "\t   Coverage Offset=%d\n", coverage = getushort(ttf));
    printf( "\t   Count=%d\n", cnt = getushort(ttf));
    seq_offsets = malloc(cnt*sizeof(uint16_t));
    for ( i=0; i<cnt; ++i )
	printf( "\t    Alternate Set Offsets[%d]=%d\n", i, seq_offsets[i]=getushort(ttf));
    glyphs = showCoverageTable(ttf,stoffset+coverage, cnt);
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,stoffset+seq_offsets[i],SEEK_SET);
	printf( "\t    Glyph alternates[%d] for glyph %d (%s)\n", i, glyphs[i], glyphs[i]>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[glyphs[i]]);
	printf( "\t     Count=%d\n", glyph_cnt = getushort(ttf));
	printf( "\t     Glyph %d (%s) -> ", glyphs[i], glyphs[i]>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[glyphs[i]] );
	for ( j=0; j<glyph_cnt; ++j ) {
	    int de = getushort(ttf);
	    printf( "%d (%s) | ", de, de>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[de]);
	}
	putchar('\n');
    }
    free(glyphs);
    free(seq_offsets);
}

static void gsubLigatureSubTable(FILE *ttf, int which, int stoffset, struct ttfinfo *info) {
    int coverage, cnt, i, j, k, lig_cnt, cc, gl;
    uint16_t *ls_offsets, *lig_offsets;
    uint16_t *glyphs;

    printf( "\t  Ligature Sub Table[%d]\n", which );
    printf( "\t   Type=%d\n", getushort(ttf));
    printf( "\t   Coverage Offset=%d\n", coverage = getushort(ttf));
    printf( "\t   Lig Set Count=%d\n", cnt = getushort(ttf));
    ls_offsets = malloc(cnt*sizeof(uint16_t));
    for ( i=0; i<cnt; ++i )
	printf( "\t    Lig Set Offsets[%d]=%d\n", i, ls_offsets[i]=getushort(ttf));
    glyphs = showCoverageTable(ttf,stoffset+coverage,cnt);
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,stoffset+ls_offsets[i],SEEK_SET);
	printf( "\t    Ligature Set[%d] for glyph %d %s\n", i, glyphs[i], glyphs[i]>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[glyphs[i]]);
	printf( "\t     Count=%d\n", lig_cnt = getushort(ttf));
	lig_offsets = malloc(lig_cnt*sizeof(uint16_t));
	for ( j=0; j<lig_cnt; ++j )
	    printf("\t     Offsets[%d]=%d\n", j, lig_offsets[j] = getushort(ttf));
	for ( j=0; j<lig_cnt; ++j ) {
	    fseek(ttf,stoffset+ls_offsets[i]+lig_offsets[j],SEEK_SET);
	    gl = getushort(ttf);
	    printf( "\t     Ligature glyph %d (%s) <- %d (%s) ",
		    gl, gl>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[gl], glyphs[i], glyphs[i]>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[glyphs[i]]);
	    cc = getushort(ttf);
	    for ( k=1; k<cc; ++k ) {
		gl = getushort(ttf);
		printf( "%d (%s) ", gl, gl>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names == NULL ? "" : info->glyph_names[gl]);
	    }
	    putchar('\n');
	}
	free(lig_offsets);
    }
    free(ls_offsets); free(glyphs);
}

static void gsubContextSubTable(FILE *ttf, int which, int stoffset, struct ttfinfo *info) {
}

static void gsubChainingContextSubTable(FILE *ttf, int which, int stoffset, struct ttfinfo *info) {
}

static void showgpossublookup(FILE *ttf,int base, int lkoffset,
	struct ttfinfo *info, int gpos ) {
    int lu_type, flags, cnt, j, st, is_exten_lu;
    uint16_t *st_offsets;

    fseek(ttf,base+lkoffset,SEEK_SET);
    lu_type = getushort(ttf);
    flags = getushort(ttf);
    cnt = getushort(ttf);
    if ( gpos ) {
	printf( "\t  Type=%d %s\n", lu_type,
		lu_type==1?"Single adjustment":
		lu_type==2?"Pair adjustment":
		lu_type==3?"Cursive attachment":
		lu_type==4?"MarkToBase attachment":
		lu_type==5?"MarkToLigature attachment":
		lu_type==6?"MarkToMark attachment":
		lu_type==7?"Context positioning":
		lu_type==8?"Chained Context positioning":
		lu_type==9?"Extension positioning":
		    "Reserved");
	is_exten_lu = lu_type==9;
    } else {
	printf( "\t  Type=%d %s\n", lu_type,
		lu_type==1?"Single":
		lu_type==2?"Multiple":
		lu_type==3?"Alternate":
		lu_type==4?"Ligature":
		lu_type==5?"Context":
		lu_type==6?"Chaining Context":
		lu_type==7?"Extension":
		lu_type==8?"Reverse chaining":
		    "Reserved");
	is_exten_lu = lu_type==7;
    }
    printf( "\t  Flags=0x%x %s|%s|%s|%s\n", (unsigned int)(flags),
	    (flags&0x1)?"RightToLeft":"LeftToRight",
	    (flags&0x2)?"IgnoreBaseGlyphs":"",
	    (flags&0x4)?"IgnoreLigatures":"",
	    (flags&0x8)?"IgnoreCombiningMarks":"");
    printf( "\t  Sub Table Count=%d\n", cnt);
    st_offsets = malloc(cnt*sizeof(uint16_t));
    for ( j=0; j<cnt; ++j )
	printf( "\t   Sub Table Offsets[%d]=%d\n", j, st_offsets[j] = getushort(ttf));
    for ( j=0; j<cnt; ++j ) {
	fseek(ttf,st = base+lkoffset+st_offsets[j],SEEK_SET);
	if ( gpos ) {
	    if ( is_exten_lu ) {	/* Extension lookup */
		int format = getushort(ttf);
		int subtype = getushort(ttf);
		int offset = getlong(ttf);
		printf( "\t    (extension format)=%d\n", format );
		lu_type = subtype;
		printf( "\t    (extension type)=%d %s\n", lu_type,
			lu_type==1?"Single adjustment":
			lu_type==2?"Pair adjustment":
			lu_type==3?"Cursive attachment":
			lu_type==4?"MarkToBase attachment":
			lu_type==5?"MarkToLigature attachment":
			lu_type==6?"MarkToMark attachment":
			lu_type==7?"Context positioning":
			lu_type==8?"Chained Context positioning":
			lu_type==9?"Extension !!! Illegal here !!!":
			    "Reserved");
		printf( "\t    (extension offset)=%d\n", offset );
		st += offset;
		fseek(ttf,st,SEEK_SET);
	    }
	    switch ( lu_type ) {
	      case 2: gposPairSubTable(ttf,j,st,info); break;
	      case 4: gposMarkToBaseSubTable(ttf,j,st,info,true); break;
	      case 6: gposMarkToBaseSubTable(ttf,j,st,info,false); break;
	    }
	} else {
	    if ( is_exten_lu ) {	/* Extension lookup */
		int format = getushort(ttf);
		int subtype = getushort(ttf);
		int offset = getlong(ttf);
		printf( "\t    (extension format)=%d\n", format );
		lu_type = subtype;
		printf( "\t    (extension type)=%d %s\n", lu_type,
			lu_type==1?"Single":
			lu_type==2?"Multiple":
			lu_type==3?"Alternate":
			lu_type==4?"Ligature":
			lu_type==5?"Context":
			lu_type==6?"Chaining Context":
			lu_type==7?"Extension !!! Illegal here !!!":
			lu_type==8?"Reverse chaining":
			    "Reserved");
		st += offset;
		fseek(ttf,st,SEEK_SET);
	    }
	    switch ( lu_type ) {
	      case 1: gsubSingleSubTable(ttf,j,st,info); break;
	      case 2: gsubMultipleSubTable(ttf,j,st,info); break;
	      case 3: gsubAlternateSubTable(ttf,j,st,info); break;
	      case 4: gsubLigatureSubTable(ttf,j,st,info); break;
	      case 5: gsubContextSubTable(ttf,j,st,info); break;
	      case 6: gsubChainingContextSubTable(ttf,j,st,info); break;
	    }
	}
    }
    free(st_offsets);
}

static void showgpossublookups(FILE *ttf,int lookup_start, struct ttfinfo *info, int gpos ) {
    int i, lu_cnt;
    uint16_t *lu_offsets;

    fseek(ttf,lookup_start,SEEK_SET);
    printf( "\t%s Lookup List Table\n", gpos?"GPOS":"GSUB" );
    printf( "\t Lookup Count=%d\n", lu_cnt = getushort(ttf));
    lu_offsets = malloc(lu_cnt*sizeof(uint16_t));
    for ( i=0; i<lu_cnt; ++i )
	printf( "\t Lookup Offset[%d]=%d\n", i, lu_offsets[i] = getushort(ttf));
    printf( "\t--\n");
    for ( i=0; i<lu_cnt; ++i ) {
	printf( "\t Lookup Table[%d]\n", i );
	showgpossublookup(ttf,lookup_start, lu_offsets[i], info, gpos);
    }
    free(lu_offsets);
}

static void readttfgsub(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int slo, flo, llo;

    fseek(ttf,info->gsub_start,SEEK_SET);
    printf( "\nGSUB table (at %d) (Glyph substitution)\n", info->gsub_start );
    printf( "\t version=%g\n", getfixed(ttf));
    printf( "\t Script List Offset=%d\n", slo = getushort(ttf));
    printf( "\t Feature List Offset=%d\n", flo = getushort(ttf));
    printf( "\t Lookup List Offset=%d\n", llo = getushort(ttf));
    showscriptlist(ttf,info->gsub_start+slo );
    showfeaturelist(ttf,info->gsub_start+flo );
    showgpossublookups(ttf,info->gsub_start+llo,info,false );
}

static void readttfgpos(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int slo, flo, llo;

    fseek(ttf,info->gpos_start,SEEK_SET);
    printf( "\nGPOS table (at %d) (Glyph positioning)\n", info->gpos_start );
    printf( "\t version=%g\n", getfixed(ttf));
    printf( "\t Script List Offset=%d\n", slo = getushort(ttf));
    printf( "\t Feature List Offset=%d\n", flo = getushort(ttf));
    printf( "\t Lookup List Offset=%d\n", llo = getushort(ttf));
    showscriptlist(ttf,info->gpos_start+slo );
    showfeaturelist(ttf,info->gpos_start+flo );
    showgpossublookups(ttf,info->gpos_start+llo,info,true );
}

static void gdefshowglyphclassdef(FILE *ttf,int offset,struct ttfinfo *info) {
    uint16_t *glist=NULL;
    int i;
    static const char * const classes[] = { "Unspecified", "Base", "Ligature", "Mark", "Component" };
    const int max_class = sizeof(classes)/sizeof(classes[0]);

    printf( "  Glyph Class Definitions\n" );
    glist = getClassDefTable(ttf,offset,info->glyph_cnt);
    for ( i=0; i<info->glyph_cnt; ++i ) if ( glist[i]>0 && glist[i]<max_class ) {
	printf( "\t  Glyph %d (%s) is a %s\n", i,
		i>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names!=NULL ? info->glyph_names[i] : "",
		classes[glist[i]]);
    }
    free(glist);
}

static void gdefshowligcaretlist(FILE *ttf,int offset,struct ttfinfo *info) {
    int coverage, cnt, i, j, cc, format;
    uint16_t *lc_offsets, *glyphs, *offsets;
    uint32_t caret_base;

    fseek(ttf,offset,SEEK_SET);
    printf( "  Ligature Caret List\n" );
    printf( "\t   Coverage Offset=%d\n", coverage = getushort(ttf));
    printf( "\t   Ligature Count=%d\n", cnt = getushort(ttf));
    lc_offsets = malloc(cnt*sizeof(uint16_t));
    for ( i=0; i<cnt; ++i )
	printf( "\t    Lig Caret Offsets[%d]=%d\n", i, lc_offsets[i]=getushort(ttf));
    glyphs = showCoverageTable(ttf,offset+coverage,cnt);
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,offset+lc_offsets[i],SEEK_SET);
	printf("\t    Carets for glyph %d (%s)\n", glyphs[i],
		glyphs[i]>=info->glyph_cnt ? "!!! Bad Glyph !!!" : info->glyph_names==NULL ? "" : info->glyph_names[glyphs[i]]);
	caret_base = ftell(ttf);
	printf("\t     Count = %d\n", cc = getushort(ttf));
	offsets = malloc(cc*sizeof(uint16_t));
	for ( j=0; j<cc; ++j )
	    offsets[j] = getushort(ttf);
	for ( j=0; j<cc; ++j ) {
	    fseek(ttf,caret_base+offsets[j],SEEK_SET);
	    format=getushort(ttf);
	    if ( format==1 ) {
		printf("\t\tCaret[%d] at %d\n", j, (short) getushort(ttf));
	    } else if ( format==2 ) {
		printf("\t\tCaret[%d] at point #%d\n", j, getushort(ttf));
	    } else if ( format==3 ) {
		printf("\t\tCaret[%d] at %d ", j, (short) getushort(ttf));
		printf(" in device table at offset %d\n", getushort(ttf));
	    } else {
		printf( "!!!! Unknown caret format %d !!!!\n", format );
	    }
	}
	free(offsets);
    }
    free(lc_offsets);
    free(glyphs);
}

static void readttfgdef(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int gco, alo, lco, maco;

    fseek(ttf,info->gdef_start,SEEK_SET);
    printf( "\nGDEF table (at %d) (Glyph definitions)\n", info->gdef_start );
    printf( "\t version=%g\n", getfixed(ttf));
    printf( "\t Glyph class Def Offset=%d\n", gco = getushort(ttf));
    printf( "\t Attach List Offset=%d\n", alo = getushort(ttf));
    printf( "\t Ligature Caret List Offset=%d\n", lco = getushort(ttf));
    printf( "\t Mark Attach Class Def Offset=%d\n", maco = getushort(ttf));
    if ( gco!=0 ) gdefshowglyphclassdef(ttf,info->gdef_start+gco,info);
    if ( lco!=0 ) gdefshowligcaretlist(ttf,info->gdef_start+lco,info);
}

static void readttfkern_context(FILE *ttf, FILE *util, struct ttfinfo *info, int stab_len);
static void readttfkern(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int version, ntables;
    int header_size, len, coverage, i, j;
    uint32_t begin;
    int left, right, val, array, n, sr, es, rs;

    fseek(ttf,info->kern_start,SEEK_SET);
    printf( "\nkern table (at %d)\n", info->kern_start );
    version = getushort(ttf);
    if ( version!=0 ) {
	fseek(ttf,info->kern_start,SEEK_SET);
	version = getlong(ttf);
	ntables = getlong(ttf);
	printf( "\t version=%g (Apple format)\n\t num_tables=%d\n", ((double) version)/65536., ntables);
	header_size = 8;
    } else {
	ntables = getushort(ttf);
	printf( "\t version=%d (Old style)\n\t num_tables=%d\n", version, ntables);
	header_size = 6;
    }
    for ( i=0; i<ntables; ++i ) {
	begin = ftell(ttf);
	if ( version==0 ) {
	    printf( "\t Sub-table %d, version=%d\n", i, getushort(ttf));
	    len = getushort(ttf);
	    coverage = getushort(ttf);
	    printf( "\t  len=%d coverage=%x %s%s%s%s sub table format=%d\n",
		    len, (unsigned int)(coverage),
		    ( coverage&1 ) ? "Horizontal": "Vertical",
		    ( coverage&2 ) ? " Minimum" : "",
		    ( coverage&4 ) ? " cross-stream" : "",
		    ( coverage&8 ) ? " override" : "",
		    ( coverage>>8 ));
	    if ( (coverage>>8)==0 ) {
		/* Kern pairs */
		n = getushort(ttf);
		sr = getushort(ttf);
		es = getushort(ttf);
		rs = getushort(ttf);
		printf( "\t   npairs=%d searchRange=%d entrySelector=%d rangeShift=%d\n",
			n, sr, es, rs );
		for ( i=0; i<n; ++i ) {
		    left = getushort(ttf);
		    right = getushort(ttf);
		    val = (short) getushort(ttf);
		    if ( info->glyph_names!=NULL && left<info->glyph_cnt && right<info->glyph_cnt ) {
			printf( "\t\t%s %s %d\n", info->glyph_names[left],
				info->glyph_names[right], val );
		    } else {
			printf( "\t\t%d %d %d\n", left, right, val );
		    }
		}
	    }
	    fseek(ttf,begin+header_size,SEEK_SET);
	} else {
	    len = getlong(ttf);
	    coverage = getushort(ttf);
	    printf( "\t  len=%d coverage=%x %s%s%s sub table format=%d\n",
		    len, (unsigned int)(coverage),
		    ( coverage&0x8000 ) ? "Vertical": "Horizontal",
		    ( coverage&0x4000 ) ? " cross-stream" : "",
		    ( coverage&0x2000 ) ? " kern-variation" : "",
		    ( coverage&0xff ));
	    printf( "\t  tuple index=%d\n", getushort(ttf));
	    switch ( (coverage&0xff) ) {
	      case 1:
		readttfkern_context(ttf,util, info, len-6);
	      break;
	      case 2:
		printf( "\t  row Width=%d\n", getushort(ttf));
		printf( "\t  left class offset=%d\n", left =getushort(ttf));
		printf( "\t  right class offset=%d\n", right = getushort(ttf));
		printf( "\t  array offset=%d\n", array = getushort(ttf));
		fseek(ttf,begin+left,SEEK_SET);
		printf( "\t  left class table\n" );
		printf( "\t   first Glyph=%d\n", getushort(ttf) );
		printf( "\t   number of glyphs=%d\n", n = getushort(ttf) );
		printf( "\t   " );
		for ( j=0; j<n; ++j )
		    printf( " %d", getushort(ttf));
		printf( "\n" );
		fseek(ttf,begin+right,SEEK_SET);
		printf( "\t  right class table\n" );
		printf( "\t   first Glyph=%d\n", getushort(ttf) );
		printf( "\t   number of glyphs=%d\n", n = getushort(ttf) );
		printf( "\t   " );
		for ( j=0; j<n; ++j )
		    printf( " %d", getushort(ttf));
		printf( "\n" );
	      break;
	    }
	    fseek(ttf,begin+header_size,SEEK_SET);
	}
	fseek(ttf,len-header_size,SEEK_CUR);
    }
}

static void readttffontdescription(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int n, i;
    uint32_t tag, lval;
    double val;

    fseek(ttf,info->fdsc_start,SEEK_SET);
    printf( "\nfdsc table (at %d) (font description)\n", info->fdsc_start );
    printf( "\t version=%g\n", getfixed(ttf));
    n = getushort(ttf);
    printf( "\t number of descriptions=%d\n", n);
    for ( i=0; i<n; ++i ) {
	tag = getlong(ttf);
	val = getfixed(ttf);
	fseek(ttf,-4,SEEK_CUR);
	lval = getlong(ttf);
	printf("\t  %c%c%c%c %s %08lx ", 
		(char)((tag>>24)&0xff), (char)((tag>>16)&0xff),
		(char)((tag>>8)&0xff), (char)(tag&0xff),
		tag==CHR('w','g','h','t')? "Weight" :
		tag==CHR('w','d','t','h')? "Width" :
		tag==CHR('s','l','n','t')? "Slant" :
		tag==CHR('o','p','s','z')? "Optical Size" :
		tag==CHR('n','a','l','f')? "Non-alphabetic" :
		"Unknown",
		(long unsigned int)(lval) );
	switch ( tag ) {
	  case CHR('w','g','h','t'):
	  case CHR('w','d','t','h'):
	  case CHR('s','l','n','t'):
	  case CHR('o','p','s','z'):
	  default:
	    printf( "%g\n", val );
	  break;
	  case CHR('n','a','l','f'):
	    printf( lval==0 ? "Alphabetic\n" :
		    lval==1 ? "Dingbats\n" :
		    lval==2 ? "Pi characters\n" :
		    lval==3 ? "Fleurons\n" :
		    lval==4 ? "Decorative borders\n" :
		    lval==5 ? "International symbols\n" :
		    lval==6 ? "Math symbols\n" :
		      "Unknown" );
	  break;
	}
    }
}

static void readttffeatures(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int n, i, j, nameid;
    uint32_t setting_offset;

    fseek(ttf,info->feat_start,SEEK_SET);
    printf( "\nfeat table (at %d) (feature names)\n", info->feat_start );
    printf( "\t version=%g\n", getfixed(ttf));
    n = getushort(ttf);
    printf( "\t number of features=%d\n", n);
    printf( "\t must be zero: %x\n", (unsigned int)(getushort(ttf)) );
    printf( "\t must be zero: %x\n", (unsigned int)(getlong(ttf)) );
    info->features = calloc(n+1,sizeof(struct features));
    info->features[n].feature = -1;
    for ( i=0; i<n; ++i ) {
	info->features[i].feature = getushort(ttf);
	info->features[i].nsettings = getushort(ttf);
	info->features[i].settings = calloc(info->features[i].nsettings,sizeof(struct settings));
	setting_offset = getlong(ttf);
	info->features[i].featureflags = getushort(ttf);
	nameid = getushort(ttf);
	info->features[i].name = getttfname(util,info,nameid);
	printf( "\t Feature %d\n", i );
	printf( "\t  Feature Id %d\n", info->features[i].feature );
	printf( "\t  number Settings %d\n", info->features[i].nsettings );
	printf( "\t  setting offset %d\n", (int)(setting_offset) );
	printf( "\t  feature flags %d ", info->features[i].featureflags );
	if ( !(info->features[i].featureflags&0x8000) )
	    printf( "Non-Exclusive settings\n" );
	else if ( !(info->features[i].featureflags&0x4000) )
	    printf( "Exclusive Settings, Default=0\n" );
	else
	    printf( "Exclusive Settings, Default=%d\n", info->features[i].featureflags&0xff);
	printf( "\t  name index %d (%s)\n", nameid, info->features[i].name==NULL?"Not found":info->features[i].name );
	fseek(util,info->feat_start+setting_offset,SEEK_SET);
	for ( j=0; j<info->features[i].nsettings; ++j ) {
	    info->features[i].settings[j].setting = getushort(util);
	    info->features[i].settings[j].nameid = getushort(util);
	}
	for ( j=0; j<info->features[i].nsettings; ++j )
	    info->features[i].settings[j].name = getttfname(util,info,info->features[i].settings[j].nameid);
	for ( j=0; j<info->features[i].nsettings; ++j )
	    printf( "\t   Setting %d nameid=%d name=%s\n",
		    info->features[i].settings[j].setting,
		    info->features[i].settings[j].nameid,
		    info->features[i].settings[j].name==NULL ? "Not found" :
		      info->features[i].settings[j].name );
    }
}

static char *getfeaturename(struct ttfinfo *info, int type) {
    int k;
    char *name;

    name = NULL;
    if ( info->features!=NULL ) {
	for ( k=0; info->features[k].feature!=-1 && info->features[k].feature!=type; ++k );
	name = info->features[k].name;		/* will be null at end of list */
    }
    if ( name!=NULL )
return( name );
/* This list is taken from https://developer.apple.com/fonts/TrueType-Reference-Manual/RM09/AppendixF.html */
return( (char *)((
	type==0 ? "All typographic features" :
	type==1 ? "Ligature" :
	type==2 ? "Cursive connection" :
	type==3 ? "Letter case" :
	type==4 ? "Vertical substitution" :
	type==5 ? "Linguistic rearrangement" :
	type==6 ? "Number spacing" :
	type==7 ? "apple, reserved" :
	type==8 ? "Smart swashes" :
	type==9 ? "Diacritics" :
	type==10 ? "Vertical Position" :
	type==11 ? "Fractions" :
	type==13 ? "Overlapping characters" :
	type==14 ? "Typographic extras" :
	type==15 ? "Mathematical extras" :
	type==16 ? "Ornament sets" :
	type==17 ? "Character alternatives" :
	type==18 ? "Design complexity" :
	type==19 ? "Style options" :
	type==20 ? "Character shape" :
	type==21 ? "Number case" :
	type==22 ? "Text/Letter spacing" :
	type==23 ? "Transliteration" :
	type==24 ? "Annotation" :
	type==25 ? "Kana Spacing" :
	type==26 ? "Ideographic Spacing" :
	type==27 ? "Unicode Decomposition" :
	type==28 ? "Ruby Kana" :
	type==29 ? "CJK Symbol Alternatives" :
	type==30 ? "Ideographic Alternatives" :
	type==31 ? "CJK Vertical Roman Placement" :
	type==32 ? "Italic CJK Roman" :
	type==33 ? "Case Sensitive Layout" :
	type==34 ? "Alternate Kana" :
	type==35 ? "Stylistic Alternatives" :
	type==36 ? "Contextual Alternates" :
	type==37 ? "Lower Case" :
	type==38 ? "Upper Case" :
	type==39 ? "Language Tag" :
	type==103 ? "CJK Roman spacing" :
/* Compatibility (deprecated) ... */
	type==100 ? "(Adobe) Character Spacing" :
	type==101 ? "(Adobe) Kana Spacing" :
	type==102 ? "(Adobe) Kanji Spacing" :
	type==104 ? "(Adobe) Square Ligatures" :
/* End */
	type==16000 ? "?Decompose Unicode (undocumented)?" :
	type==16001 ? "?Combining character (undocumented)?" :
	    "Unknown feature type")) );
}

static char *getsettingname(struct ttfinfo *info, int type, int setting) {
    int k,l;
    char *name;

    name = NULL;
    if ( info->features!=NULL ) {
	for ( k=0; info->features[k].feature!=-1 && info->features[k].feature!=type; ++k );
	if ( info->features[k].feature!=-1 ) {
	    /*name = info->features[k].name;		will be null at end of list */
	    for ( l=0 ; l<info->features[k].nsettings && info->features[k].settings[l].setting!=setting; ++l );
	    if ( l<info->features[k].nsettings )
		name = info->features[k].settings[l].name;
	}
    }
    if ( name )
return( name );
/* These lists are taken from https://developer.apple.com/fonts/TrueType-Reference-Manual/RM09/AppendixF.html */
/*  the numeric values are at the bottom of the page */
    else switch ( type ) {
      case 0:	/* All Typographic Features */
return( (char *)((
	setting==0 ? "On" :
	setting==1 ? "Off" :
	"Unknown")) );
      break;
      case 1:	/* Ligatures */
return( (char *)((
	setting==0 ? "Required ligatures On" :
	setting==1 ? "Required ligatures Off" :
	setting==2 ? "Common Ligatures On" :
	setting==3 ? "Common Ligatures Off" :
	setting==4 ? "Rare Ligatures On" :
	setting==5 ? "Rare Ligatures Off" :
	setting==6 ? "Logos On" :
	setting==7 ? "Logos Off" :
	setting==8 ? "Rebus pictures On" :
	setting==9 ? "Rebus pictures Off" :
	setting==10 ? "Diphthong ligatures On" :
	setting==11 ? "Diphthong ligatures Off" :
	setting==12 ? "Squared ligatures On" :
	setting==13 ? "Squared ligatures Off" :
	setting==14 ? "Squared ligatures, abbreviated On" :
	setting==15 ? "Squared ligatures, abbreviated Off" :
	setting==16 ? "Symbol ligatures On" :
	setting==17 ? "Symbol ligatures Off" :
	setting==18 ? "Contextual ligatures On" :
	setting==19 ? "Contextual ligatures Off" :
	setting==20 ? "Historical ligatures On" :
	setting==21 ? "Historical ligatures Off" :
	    "Unknown")) );
      break;
      case 2:	/* cursive */
return( (char *)((
	setting==0 ? "Unconnected" :
	setting==1 ? "Partially connected" :
	setting==2 ? "Cursive" :
	    "Unknown")) );
      break;
      case 3:	/* Letter case */
return( (char *)((
	setting==0 ? "Upper & Lower case" :
	setting==1 ? "All Caps" :
	setting==2 ? "All Lower Case" :
	setting==3 ? "Small Caps" :
	setting==4 ? "Initial Caps" :
	setting==5 ? "Initial Caps & Small Caps" :
	    "Unknown")) );
      break;
      case 4:	/* Vertical Substitution */
return( (char *)((
	setting==0 ? "On" :
	setting==1 ? "Off" :
	    "Unknown")) );
      break;
      case 5:	/* Linguistic Rearrangement */
return( (char *)((
	setting==0 ? "On" :
	setting==1 ? "Off" :
	    "Unknown")) );
      break;
      case 6:	/* Number spacing */
return( (char *)((
	setting==0 ? "Monospaced Numbers" :
	setting==1 ? "Proportional Numbers" :
	setting==2 ? "Third-width Numerals" :
	setting==3 ? "Quarter-width Numerals" :
	    "Unknown")) );
      break;
      case 8:	/* Smart swash */
return( (char *)((
	setting==0 ? "Initial swashes On" :
	setting==1 ? "Initial swashes Off" :
	setting==2 ? "Final swashes On" :
	setting==3 ? "Final swashes Off" :
	setting==4 ? "line initial swashes On" :
	setting==5 ? "line initial swashes Off" :
	setting==6 ? "line final swashes On" :
	setting==7 ? "line final swashes Off" :
	setting==8 ? "non-final swashes On" :
	setting==9 ? "non-final swashes Off" :
	    "Unknown")) );
      break;
      case 9:	/* diacritics */
return( (char *)((
	setting==0 ? "show Diacritics" :
	setting==1 ? "hide Diacritics" :
	setting==2 ? "decompose Diacritics" :
	    "Unknown")) );
      break;
      case 10:	/* vertical positioning */
return( (char *)((
	setting==0 ? "normal position" :
	setting==1 ? "superiors" :
	setting==2 ? "inferiors" :
	setting==3 ? "ordinals" :
	setting==4 ? "scientific inferiors" :
	    "Unknown")) );
      break;
      case 11:	/* fractions */
return( (char *)((
	setting==0 ? "no fractions" :
	setting==1 ? "vertical fractions" :
	setting==2 ? "diagonal fractions" :
	    "Unknown")) );
      break;
      case 13:	/* overlapping chars */
return( (char *)((
	setting==0 ? "prevent Overlap On" :
	setting==1 ? "prevent Overlap Off" :
	    "Unknown")) );
      break;
      case 14:	/* typographic extras */
return( (char *)((
	setting==0 ? "hyphens to Em dashes On" :
	setting==1 ? "hyphens to Em dashes Off" :
	setting==2 ? "hyphens to En dashes On" :
	setting==3 ? "hyphens to En dashes Off" :
	setting==4 ? "unslashed Zero On" :
	setting==5 ? "unslashed Zero Off" :
	setting==6 ? "form Interrobang On" : 
	setting==7 ? "form Interrobang Off" :
	setting==8 ? "smart Quotes On" :
	setting==9 ? "smart Quotes Off" :
	setting==10 ? "periods to ellipsis On" :
	setting==11 ? "periods to ellipsis Off" :
	    "Unknown")) );
      break;
      case 15:	/* mathematical extras */
return( (char *)((
	setting==0 ? "hyphen to minus On" :
	setting==1 ? "hyphen to minus Off" :
	setting==2 ? "asterisk to multiply On" :
	setting==3 ? "asterisk to multiply Off" :
	setting==4 ? "slash to divide On" :
	setting==5 ? "slash to divide Off" :
	setting==6 ? "inequality ligatures On" :
	setting==7 ? "inequality ligatures Off" :
	setting==8 ? "exponents On" :
	setting==9 ? "exponents Off" :
	setting==10 ? "mathematical Greek Off" :
	setting==11 ? "mathematical Greek Off" :
	    "Unknown")) );
      break;
      case 16:	/* ornament sets */
return( (char *)((
	setting==0 ? "no ornaments" :
	setting==1 ? "dingbats" :
	setting==2 ? "pi Characters" :
	setting==3 ? "fleurons" :
	setting==4 ? "decorative borders" :
	setting==5 ? "international symbols" :
	setting==6 ? "math symbols" :
	    "Unknown")) );
      break;
      case 17:	/* character alternates */
return( (char *)((
	setting==0 ? "no alternates" :
	    "Unknown")) );
      break;
      case 18:	/* design complexity */
return( (char *)((
	setting==0 ? "design level 1" :
	setting==1 ? "design level 2" :
	setting==2 ? "design level 3" :
	setting==3 ? "design level 4" :
	setting==4 ? "design level 5" :
	    "Unknown")) );
      break;
      case 19:	/* style options */
return( (char *)((
	setting==0 ? "no style options" :
	setting==1 ? "display text" :
	setting==2 ? "engraved text" :
	setting==3 ? "illuminated Caps" :
	setting==4 ? "titling caps" :
	setting==5 ? "tall caps" :
	    "Unknown")) );
      break;
      case 20:	/* character shape */
return( (char *)((
	setting==0 ? "traditional characters" :
	setting==1 ? "simplified characters" :
	setting==2 ? "JIS1978 characters" :
	setting==3 ? "JIS1983 characters" :
	setting==4 ? "JIS1990 characters" :
	setting==5 ? "traditional alt 1" :
	setting==6 ? "traditional alt 2" :
	setting==7 ? "traditional alt 3" :
	setting==8 ? "traditional alt 4" :
	setting==9 ? "traditional alt 5" :
	setting==10 ? "expert characters" :
	setting==11 ? "JIS2004 characters" :
	setting==12 ? "hojo characters" :
	setting==13 ? "NLC characters" :
	setting==14 ? "traditional name characters" :
	    "Unknown")) );
      break;
      case 21:	/* number case */
return( (char *)((
	setting==0 ? "lower case numbers" :
	setting==1 ? "upper case numbers" :
	    "Unknown")) );
      break;
      case 22:	/* text spacing */
return( (char *)((
	setting==0 ? "proportional" :
	setting==1 ? "monospace" :
	setting==2 ? "halfwidth" :
	setting==3 ? "third-width" :
	setting==4 ? "quarter-width" :
	setting==5 ? "alternate proportional" :
	setting==6 ? "alternate halfwidth" :
	    "Unknown")) );
      break;
      case 23:	/* transliteration */
return( (char *)((
	setting==0 ? "no transliteration" :
	setting==1 ? "hanja to hangul" :
	setting==2 ? "hiragana to katakana" :
	setting==3 ? "katakana to hiragana" :
	setting==4 ? "kana to Romanization" :
	setting==5 ? "romanization to Hiragana" :
	setting==6 ? "romanization to Katakana" :
	setting==7 ? "hanja to hangul alt 1" :
	setting==8 ? "hanja to hangul alt 2" :
	setting==9 ? "hanja to hangul alt 3" :
	    "Unknown")) );
      break;
      case 24:	/* anotation */
return( (char *)((
	setting==0 ? "no annotation" :
	setting==1 ? "box annotation" :
	setting==2 ? "rounded box annotation" :
	setting==3 ? "circle annotation" :
	setting==4 ? "inverted circle annotation" :
	setting==5 ? "parenthesis Annotation" :
	setting==6 ? "period annotation" :
	setting==7 ? "roman numeral annotation" :
	setting==8 ? "diamond annotation" :
	setting==9 ? "inverted box annotation" :
	setting==10 ? "inverted rounded box annotation" :
	"Unknown")) );
      break;
      case 25:	/* kana spacing */
return( (char *)((
	setting==0 ? "full width kana" :
	setting==1 ? "proportional kana" : /* Proportional Japanese glyphs */
	"Unknown")) );
      break;
      case 26:	/* ideograph spacing */
return( (char *)((
	setting==0 ? "full width ideograph" :
	setting==1 ? "proportional ideograph" :
	setting==2 ? "half width ideograph" :
	"Unknown")) );
      break;
      case 27:	/* Unicode Decomposition */
return( (char *)((
	setting==0 ? "Canonical Composition On" :
	setting==1 ? "Canonical Composition Off" :
	setting==2 ? "Compatibility Composition On" :
	setting==3 ? "Compatibility Composition Off" :
	setting==4 ? "Transcoding Composition On" :
	setting==5 ? "Transcoding Composition Off" :
	"Unknown")) );
      break;
      case 28:	/* Ruby Kana */
return( (char *)((
	setting==0 ? "No Ruby Kana" :
	setting==1 ? "Ruby Kana" :
	setting==2 ? "Ruby Kana On" :
	setting==3 ? "Ruby Kana Off" :
	"Unknown")) );
      break;
      case 29:	/* CJK Symbol Alternatives */
return( (char *)((
	setting==0 ? "No CJK Symbol Alternatives" :
	setting==1 ? "CJK Symbol Alt One" :
	setting==2 ? "CJK Symbol Alt Two" :
	setting==3 ? "CJK Symbol Alt Three" :
	setting==4 ? "CJK Symbol Alt Four" :
	setting==5 ? "CJK Symbol Alt Five" :
	"Unknown")) );
      break;
      case 30:	/* Ideographic Alternatives */
return( (char *)((
	setting==0 ? "No Ideographic Alternatives" :
	setting==1 ? "Ideographic Alt One" :
	setting==2 ? "Ideographic Alt Two" :
	setting==3 ? "Ideographic Alt Three" :
	setting==4 ? "Ideographic Alt Four" :
	setting==5 ? "Ideographic Alt Five" :
	"Unknown")) );
      break;
      case 31:	/* CJK Vertical Roman Placement */
return( (char *)((
	setting==0 ? "CJK Vertical Roman Centered" :
	setting==1 ? "CJK Vertical Roman H Baseline" :
	"Unknown")) );
      break;
      case 32:	/* Italic CJK Roman */
return( (char *)((
	setting==0 ? "No Italic CJK Roman" :
	setting==1 ? "Italic CJK Roman" :
	setting==2 ? "Italic CJK Roman On" :
	setting==3 ? "Italic CJK Roman Off" :
	"Unknown")) );
      break;
      case 33:	/* Case Sensitive Layout */
return( (char *)((
	setting==0 ? "Case Sensitive Layout On" :
	setting==1 ? "Case Sensitive Layout Off" :
	setting==2 ? "Case Sensitive Spacing On" :
	setting==3 ? "Case Sensitive Spacing Off" :
	"Unknown")) );
      break;
      case 34:	/* Alternate Kana */
return( (char *)((
	setting==0 ? "Alternate Horiz Kana On" :
	setting==1 ? "Alternate Horiz Kana Off" :
	setting==2 ? "Alternate Vert Kana On" :
	setting==3 ? "Alternate Vert Kana Off" :
	"Unknown")) );
      break;
      case 35:	/* Stylistic Alternatives */
return( (char *)((
	setting==0 ? "No Stylistic Alternates" :
	setting==1 ? "Stylistic Alt One" :
	setting==2 ? "Stylistic Alt Two" :
	setting==3 ? "Stylistic Alt Three" :
	setting==4 ? "Stylistic Alt Four" :
	setting==5 ? "Stylistic Alt Five" :
	setting==6 ? "Stylistic Alt Six" :
	setting==7 ? "Stylistic Alt Seven" :
	setting==8 ? "Stylistic Alt Eight" :
	setting==9 ? "Stylistic Alt Nine" :
	setting==10 ? "Stylistic Alt Ten" :
	setting==11 ? "Stylistic Alt Eleven" :
	setting==12 ? "Stylistic Alt Twelve" :
	setting==13 ? "Stylistic Alt Thirteen" :
	setting==14 ? "Stylistic Alt Fourteen" :
	setting==15 ? "Stylistic Alt Fifteen" :
	setting==16 ? "Stylistic Alt Sixteen" :
	setting==17 ? "Stylistic Alt Seventeen" :
	setting==18 ? "Stylistic Alt Eighteen" :
	setting==19 ? "Stylistic Alt Nineteen" :
	setting==20 ? "Stylistic Alt Twenty" :
	"Unknown")) );
      break;
      case 36:	/* Contextual Alternates */
return( (char *)((
	setting==0 ? "Contextual Alternates On" :
	setting==1 ? "Contextual Alternates Off" :
	setting==2 ? "Swash Alternates On" :
	setting==3 ? "Swash Alternates Off" :
	setting==4 ? "Contextual Swash Alternates On" :
	setting==5 ? "Contextual Swash Alternates Off" :
	"Unknown")) );
      break;
      case 37:	/* Lower Case */
return( (char *)((
	setting==0 ? "Default Lower Case" :
	setting==1 ? "Lower Case Small Caps" :
	setting==2 ? "Lower Case Petite Caps" :
	"Unknown")) );
      break;
      case 38:	/* Upper Case */
return( (char *)((
	setting==0 ? "Default Upper Case" :
	setting==1 ? "Upper Case Small Caps" :
	setting==2 ? "Upper Case Petite Caps" :
	"Unknown")) );
      break;
      case 103:	/* CJK Spacing */
return( (char *)((
	setting==0 ? "halfwidth CJK Roman" : /* No change */
	setting==1 ? "proportional CJK Roman" :
	setting==2 ? "default CJK Roman" : /* Ideal metrics */
	setting==3 ? "fullwidth CJK Roman" :
	"Unknown")) );
      break;
      case 16000: /* Decomposed Unicode (determined empirically) */
return( (char *)((
	setting==0 ? "Compose" :
	setting==1 ? "Off" :
	"Unknown")) );
      break;
      case 16001: /* Combining character (determined empirically) */
return( (char *)((
	setting==0 ? "Combine" :
	setting==1 ? "Off" :
	"Unknown")) );
      break;
      default:
return( (char *)((
	setting==0 ? "Unknown (?On?)" :
	setting==1 ? "Unknown (?Off?)" :
	"Unknown")) );
      break;
    }
return( (char *)("Unknown") );
}

static int showbinsearchheader(FILE *ttf) {
    int size, cnt;

    printf( "\t  Binary search header\n" );
    printf( "\t   Entry size=%d\n", size = getushort(ttf));
    printf( "\t   Number of entries=%d\n", cnt = getushort(ttf));
    printf( "\t   Search range=%d\n", getushort(ttf));
    printf( "\t   Log2(nUnits)=%d\n", getushort(ttf));
    printf( "\t   Range Shift=%d\n", getushort(ttf));
return( cnt );
}

static void show_applelookuptable(FILE *ttf,struct ttfinfo *info,void (*show)(FILE *,struct ttfinfo *)) {
    int i, j;
    int format;
    int first, last, cnt, glyph, data_offset;
    uint32_t here;
    uint32_t base = ftell(ttf);

    printf( "\t Lookup table format=%d ", format = getushort(ttf));
    switch ( format ) {
      case 0:
	printf( "Simple array\n" );
	for ( i=0; i<info->glyph_cnt; ++i ) {
	    printf( "Glyph %d (%s)=", i, i>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names!=NULL ? info->glyph_names[i]: "" );
	    show( ttf,info );
	}
      break;
      case 2:
	printf( "Segment Single\n" );
	cnt = showbinsearchheader(ttf);
	for ( i=0; i<cnt; ++i ) {
	    last = getushort(ttf);
	    first = getushort(ttf);
	    printf( "All glyphs between %d (%s) and %d (%s)=",
		    first, 
		    first>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names!=NULL ?
		     info->glyph_names[first]: "",
		    last, 
		    last>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names!=NULL ?
		     info->glyph_names[last]: "" );
	    show( ttf,info );
	}
      break;
      case 4:
	printf( "Segment Array\n" );
	cnt = showbinsearchheader(ttf);
	for ( i=0; i<cnt; ++i ) {
	    last = getushort(ttf);
	    first = getushort(ttf);
	    data_offset = getushort(ttf);
	    here = ftell(ttf);
	    fseek(ttf,base+data_offset,SEEK_SET);
	    printf( "\t\tSegment %d, from glyph %d to glyph %d. Data offset=%d\n",
		    i, first, last, data_offset );
	    for ( j=first; j<=last; ++j ) {
		printf( "Glyph %d (%s)=", j,
			j>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names!=NULL ?
			 info->glyph_names[j]: "" );
		show( ttf,info );
	    }
	    fseek(ttf,here,SEEK_SET);
	}
      break;
      case 6:
	printf( "Single table\n" );
	cnt = showbinsearchheader(ttf);
	for ( i=0; i<cnt; ++i ) {
	    glyph = getushort(ttf);
	    printf( "Glyph %d (%s)=", glyph,
		    glyph>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names!=NULL ?
		     info->glyph_names[glyph]: "" );
	    show( ttf,info );
	}
      break;
      case 8:
	printf( "Trimmed array\n" );
	first = getushort(ttf);
	cnt = getushort(ttf);
	for ( i=0; i<cnt; ++i ) {
	    printf( "Glyph %d (%s)=", i+first,
		    i+first>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names!=NULL ?
		     info->glyph_names[i+first]: "" );
	    show( ttf,info );
	}
      break;
      default:
	printf( "Unknown format for lookup table %d\n", format );
    }
}

/* some class codes are predefined:
	0 => End of text
	1 => out of bounds (anything outside the glyph range in the class header, and possibly some glyphs within
	2 => deleted glyph (0xffff => a glyph has been deleted, should not be in array)
	3 => End of line
	4 - nclasses-1 => user defined
*/
struct classes {
};

struct statetable {
    uint32_t state_start;
    int nclasses;
    int nstates;
    int nentries;
    int state_offset;
    int entry_size;	/* size of individual entry */
    int entry_extras;	/* Number of extra glyph offsets */
    int first_glyph;	/* that's classifyable */
    int nglyphs;
    uint8_t *classes;
    uint8_t *state_table;	/* state_table[nstates][nclasses], each entry is an */
	/* index into the following array */
    uint16_t *state_table2;	/* morx version. States are have 2 byte entries */
    uint16_t *classes2;
    uint8_t *transitions;
    uint32_t extra_offsets[3];
    int len;		/* Size of the entire subtable */
};

static void show_statetable(struct statetable *st, struct ttfinfo *info, FILE *ttf,
	void (*entry_print)(uint8_t *entry,struct statetable *st,struct ttfinfo *info,FILE *ttf)) {
    int i, j;
    uint8_t *pt;

    printf( "\t State table\n" );
    printf( "\t  num classes = %d\n", st->nclasses );
    printf( "\t  num states = %d (derived)\n", st->nstates );
    printf( "\t  num entries = %d (derived)\n", st->nentries );
    printf( "\t  entry size = %d (derived)\n", st->entry_size );
    printf( "\t  first classified glyph = %d (%s), glyph_cnt=%d\n", st->first_glyph,
	    st->first_glyph>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names!=NULL?info->glyph_names[st->first_glyph]:"",
	    st->nglyphs);
    if ( info->glyph_names!=NULL ) {
	for ( i=0; i<st->nglyphs; ++i )
	    printf( "\t   Glyph %4d -> Class %d (%s)\n",
		    st->first_glyph+i, st->classes[i],
		    st->first_glyph+i>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names[st->first_glyph+i]);
    } else {
	for ( i=0; i<st->nglyphs; ++i )
	    printf( "\t   Glyph %4d -> Class %d\n",
		    st->first_glyph+i, st->classes[i] );
    }

    /* Mapping from state x class => transition entry */
    printf( "Classes:  " );
    for ( j=0; j<st->nclasses; ++j )
	printf( "%4d", j );
    printf( "\n" );
    for ( i=0; i<st->nstates; ++i ) {
	printf( "State %2d: ", i );
	for ( j=0; j<st->nclasses; ++j )
	    printf( "%4d", st->state_table[i*st->nclasses+j]);
	printf( "\n" );
    }

    /* Transition entries */
    for ( i=0; i<st->nentries; ++i ) {
	pt = st->transitions + i*st->entry_size;
	printf( "\t  Transition Entry %d\n", i );
	printf( "\t   New State %d\n", (((pt[0]<<8)|pt[1])-st->state_offset)/st->nclasses );
	if ( entry_print!=NULL )
	    entry_print(pt,st,info,ttf);
	else {
	    printf( "\t   Flags %04x\n", (unsigned int)(((pt[2]<<8)|pt[3])) );
	    for ( j=0; j<st->entry_extras; ++j )
		printf( "\t   GlyphOffset[%d] = %d\n", j, (pt[2*j+4]<<8)|pt[2*j+5]);
	}
    }
    printf( "\n" );
}

static void show_statetablex(struct statetable *st, struct ttfinfo *info, FILE *ttf,
	void (*entry_print)(uint8_t *entry,struct statetable *st,struct ttfinfo *info,FILE *ttf)) {
    int i, j;
    uint8_t *pt;

    printf( "\t State table\n" );
    printf( "\t  num classes = %d\n", st->nclasses );
    printf( "\t  num states = %d (derived)\n", st->nstates );
    printf( "\t  num entries = %d (derived)\n", st->nentries );
    printf( "\t  entry size = %d (derived)\n", st->entry_size );
    for ( i=0; i<info->glyph_cnt ; ++i ) if ( st->classes2[i]!=1 ) {
	if ( info->glyph_names!=NULL ) {
	    printf( "\t   Glyph %4d -> Class %d (%s)\n",
		    i, st->classes2[i],
		    i>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names[i]);
	} else {
	    printf( "\t   Glyph %4d -> Class %d\n",
		    i, st->classes2[i] );
	}
    }

    /* Mapping from state x class => transition entry */
    printf( "Classes:  " );
    for ( j=0; j<st->nclasses; ++j )
	printf( "%4d", j );
    printf( "\n" );
    for ( i=0; i<st->nstates; ++i ) {
	printf( "State %2d: ", i );
	for ( j=0; j<st->nclasses; ++j )
	    printf( "%4d", st->state_table2[i*st->nclasses+j]);
	printf( "\n" );
    }

    /* Transition entries */
    for ( i=0; i<st->nentries; ++i ) {
	pt = st->transitions + i*st->entry_size;
	printf( "\t  Transition Entry %d\n", i );
	printf( "\t   New State %d\n", ((pt[0]<<8)|pt[1]) );
	if ( entry_print!=NULL )
	    entry_print(pt,st,info,ttf);
	else {
	    printf( "\t   Flags %04x\n", (unsigned int)(((pt[2]<<8)|pt[3])) );
	    for ( j=0; j<st->entry_extras; ++j )
		printf( "\t   GlyphOffset[%d] = %d\n", j, (pt[2*j+4]<<8)|pt[2*j+5]);
	}
    }
    printf( "\n" );
}

static void readttf_applelookup(FILE *ttf,struct ttfinfo *info,
	void (*apply_values)(struct ttfinfo *info, int gfirst, int glast,FILE *ttf),
	void (*apply_value)(struct ttfinfo *info, int gfirst, int glast,FILE *ttf),
	void (*apply_default)(struct ttfinfo *info, int gfirst, int glast,void *def),
	void *def) {
    int format, i, first, last, data_off, cnt, prev;
    uint32_t here;
    uint32_t base = ftell(ttf);

    switch ( format = getushort(ttf)) {
      case 0:	/* Simple array */
	apply_values(info,0,info->glyph_cnt-1,ttf);
      break;
      case 2:	/* Segment Single */
	/* Entry size  */ getushort(ttf);
	cnt = getushort(ttf);
	/* search range */ getushort(ttf);
	/* log2(cnt)    */ getushort(ttf);
	/* range shift  */ getushort(ttf);
	prev = 0;
	for ( i=0; i<cnt; ++i ) {
	    last = getushort(ttf);
	    first = getushort(ttf);
	    if ( apply_default!=NULL )
		apply_default(info,prev,first-1,def);
	    apply_value(info,first,last,ttf);
	    prev = last+1;
	}
      break;
      case 4:	/* Segment multiple */
	/* Entry size  */ getushort(ttf);
	cnt = getushort(ttf);
	/* search range */ getushort(ttf);
	/* log2(cnt)    */ getushort(ttf);
	/* range shift  */ getushort(ttf);
	prev = 0;
	for ( i=0; i<cnt; ++i ) {
	    last = getushort(ttf);
	    first = getushort(ttf);
	    data_off = getushort(ttf);
	    here = ftell(ttf);
	    if ( apply_default!=NULL )
		apply_default(info,prev,first-1,def);
	    fseek(ttf,base+data_off,SEEK_SET);
	    apply_values(info,first,last,ttf);
	    fseek(ttf,here,SEEK_SET);
	    prev = last+1;
	}
      break;
      case 6:	/* Single table */
	/* Entry size  */ getushort(ttf);
	cnt = getushort(ttf);
	/* search range */ getushort(ttf);
	/* log2(cnt)    */ getushort(ttf);
	/* range shift  */ getushort(ttf);
	prev = 0;
	for ( i=0; i<cnt; ++i ) {
	    first = getushort(ttf);
	    if ( apply_default!=NULL )
		apply_default(info,prev,first-1,def);
	    apply_value(info,first,first,ttf);
	    prev = first+1;
	}
      break;
      case 8:	/* Simple array */
	first = getushort(ttf);
	cnt = getushort(ttf);
	if ( apply_default!=NULL ) {
	    apply_default(info,0,first-1,def);
	    apply_default(info,first+cnt,info->glyph_cnt-1,def);
	}
	apply_values(info,first,first+cnt-1,ttf);
      break;
      default:
	fprintf( stderr, "Invalid lookup table format. %d\n", format );
      break;
    }
}

static void mortclass_apply_values(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    int i;

    for ( i=gfirst; i<=glast; ++i )
	info->morx_classes[i] = getushort(ttf);
}

static void mortclass_apply_value(struct ttfinfo *info, int gfirst, int glast,FILE *ttf) {
    uint16_t class;
    int i;

    class = getushort(ttf);

    for ( i=gfirst; i<=glast; ++i )
	info->morx_classes[i] = class;
}

static struct statetable *read_statetable(FILE *ttf, int ent_extras, int ismorx, struct ttfinfo *info) {
    struct statetable *st = calloc(1,sizeof(struct statetable));
    uint32_t here = ftell(ttf);
    int nclasses, class_off, state_off, entry_off;
    int state_max, ent_max, old_state_max, old_ent_max;
    int i, j, ent, new_state, ent_size;

    st->state_start = here;

    if ( ismorx ) {
	nclasses = getlong(ttf);
	class_off = getlong(ttf);
	state_off = getlong(ttf);
	entry_off = getlong(ttf);
	st->extra_offsets[0] = getlong(ttf);
	st->extra_offsets[1] = getlong(ttf);
	st->extra_offsets[2] = getlong(ttf);
    } else {
	nclasses = getushort(ttf);	/* Number of bytes per state in state subtable, equal to number of classes */
	class_off = getushort(ttf);
	state_off = getushort(ttf);
	entry_off = getushort(ttf);
	st->extra_offsets[0] = getushort(ttf);
	st->extra_offsets[1] = getushort(ttf);
	st->extra_offsets[2] = getushort(ttf);
    }
    st->nclasses = nclasses;
    st->state_offset = state_off;

	/* parse class subtable */
    fseek(ttf,here+class_off,SEEK_SET);
    if ( ismorx ) {
	st->classes2 = info->morx_classes = malloc(info->glyph_cnt*sizeof(uint16_t));
	for ( i=0; i<info->glyph_cnt; ++i )
	    st->classes2[i] = 1;			/* Out of bounds */
	readttf_applelookup(ttf,info,
		mortclass_apply_values,mortclass_apply_value,NULL,NULL);
    } else {
	st->first_glyph = getushort(ttf);
	st->nglyphs = getushort(ttf);
	st->classes = malloc(st->nglyphs);
	fread(st->classes,1,st->nglyphs,ttf);
    }

    /* The size of an entry is variable. There are 2 uint16_t fields at the begin-*/
    /*  ning of all entries. There may be some number of shorts following these*/
    /*  used for indexing special tables. */
    ent_size = 4 + 2*ent_extras;
    st->entry_size = ent_size;
    st->entry_extras = ent_extras;

    /* Apple does not provide a way of figuring out the size of either of the */
    /*  state or entry tables, so we must parse both as we go and try to work */
    /*  out the maximum values... */
    /* There are always at least 2 states defined. Parse them and find what */
    /*  is the biggest entry they use, then parse those entries and find what */
    /*  is the biggest state they use, and then repeat until we don't find any*/
    /*  more states or entries */
    old_state_max = 0; old_ent_max = 0;
    state_max = 2; ent_max = 0;
    while ( old_state_max!=state_max ) {
	i = old_state_max*nclasses;
	fseek(ttf,here+state_off+(ismorx?i*sizeof(uint16_t):i),SEEK_SET);
	old_state_max = state_max;
	for ( ; i<state_max*nclasses; ++i ) {
	    ent = ismorx ? getushort(ttf) : getc(ttf);
	    if ( ent+1 > ent_max )
		ent_max = ent+1;
	}
	if ( ent_max==old_ent_max )		/* Nothing more */
    break;
	if ( ent_max>1000 ) {
	    fprintf( stderr, "It looks to me as though there's a morx sub-table with more than 1000\n transitions. Which makes me think there's probably an error\n" );
	    free(st);
return( NULL );
	}
	fseek(ttf,here+entry_off+old_ent_max*ent_size,SEEK_SET);
	i = old_ent_max;
	old_ent_max = ent_max;
	for ( ; i<ent_max; ++i ) {
	    new_state = getushort(ttf);
	    if ( !ismorx )
		new_state = (new_state-state_off)/nclasses;
	    /* flags = */ getushort(ttf);
	    for ( j=0; j<ent_extras; ++j )
		/* glyphOffsets[j] = */ getushort(ttf);
	    if ( new_state+1>state_max )
		state_max = new_state+1;
	}
	if ( state_max>1000 ) {
	    fprintf( stderr, "It looks to me as though there's a morx sub-table with more than 1000\n states. Which makes me think there's probably an error\n" );
	    free(st);
return( NULL );
	}
    }

    st->nstates = state_max;
    st->nentries = ent_max;
    
    fseek(ttf,here+state_off,SEEK_SET);
    /* an array of arrays of state transitions, each represented by one byte */
    /*  which is an index into the Entry subtable, which comes next. */
    /* One dimension is the number of states, and the other the */
    /*  number of classes (classes vary faster than states) */
    /* The first two states are predefined, 0 is start of text, 1 start of line*/
    if ( ismorx ) {
	st->state_table2 = malloc(st->nstates*st->nclasses*sizeof(uint16_t));
	for ( i=0; i<st->nstates*st->nclasses; ++i )
	    st->state_table2[i] = getushort(ttf);
    } else {
	st->state_table = malloc(st->nstates*st->nclasses);
	fread(st->state_table,1,st->nstates*st->nclasses,ttf);
    }

	/* parse the entry subtable */
    fseek(ttf,here+entry_off,SEEK_SET);
    st->transitions = malloc(st->nentries*st->entry_size);
    fread(st->transitions,1,st->nentries*st->entry_size,ttf);
return( st );
}

static void free_statetable(struct statetable *st) {
    if ( st==NULL )
return;
    free( st->state_table );
    free( st->state_table2 );
    free( st->transitions );
    free( st->classes );
    free( st->classes2 );
    free( st );
}

static void show_contextkerndata(uint8_t *entry,struct statetable *st,struct ttfinfo *info, FILE *ttf) {
    int flags = (entry[2]<<8)|entry[3];
    int offset = flags&0x3fff;
    int i, k;

    printf( "\t   Flags %04x ", (unsigned int)(flags) );
    if ( flags&0x8000 )
	printf( "Add to Kern Stack | ");
    if ( flags&0x4000 )
	printf( "Don't Advance Glyph" );
    else
	printf( "Advance Glyph" );
    printf( ",  ValueOffset = %d\n", offset );
    if ( offset!=0 ) {
	printf( "Offset=%d, len=%d\n", offset, st->len );
	fseek(ttf,offset+st->state_start,SEEK_SET);
	printf( "Kerns: " );
	for ( i=0; i<8; ++i ) {
	    printf( "%d ", (k = (short) getushort(ttf)) & ~1 );
	    if ( k&1 )	/* list is terminated by an odd number */
	break;
	}
	printf( "\n" );
    }
}

static void readttfkern_context(FILE *ttf, FILE *util, struct ttfinfo *info, int stab_len) {
    struct statetable *st;

    st = read_statetable(ttf,0,false,info);
    st->len = stab_len;
    show_statetable(st, info, ttf, show_contextkerndata);
    free_statetable(st);
}

static void show_indicflags(uint8_t *entry,struct statetable *st,struct ttfinfo *info,FILE *ttf) {
    int flags = (entry[2]<<8)|entry[3];

    printf( "\t   Flags %04x ", (unsigned int)(flags) );
    if ( flags&0x8000 )
	printf( "Mark First | ");
    if ( flags&0x2000 )
	printf( "Mark Last | " );
    if ( flags&0x4000 )
	printf( "Don't Advance Glyph " );
    else
	printf( "Advance Glyph       " );
    switch( flags&0xf ) {
      case 0 :
	printf( "No action\n" );
      break;
      case 1 :
	printf( "Ax => xA\n" );
      break;
      case 2 :
	printf( "xD => Dx\n" );
      break;
      case 3 :
	printf( "AxD => DxA\n" );
      break;
      case 4 :
	printf( "ABx => xAB\n" );
      break;
      case 5 :
	printf( "ABx => xBA\n" );
      break;
      case 6 :
	printf( "xCD => CDx\n" );
      break;
      case 7 :
	printf( "xCD => DCx\n" );
      break;
      case 8 :
	printf( "AxCD => CDxA\n" );
      break;
      case 9 :
	printf( "AxCD => DCxA\n" );
      break;
      case 10 :
	printf( "ABxD => DxAB\n" );
      break;
      case 11 :
	printf( "ABxD => DxBA\n" );
      break;
      case 12 :
	printf( "ABxCD => CDxAB\n" );
      break;
      case 13 :
	printf( "ABxCD => CDxBA\n" );
      break;
      case 14 :
	printf( "ABxCD => DCxAB\n" );
      break;
      case 15 :
	printf( "ABxCD => DCxBA\n" );
      break;
    }
}

static void readttfmort_indic(FILE *ttf, FILE *util, struct ttfinfo *info, int stab_len) {
    struct statetable *st;

    st = read_statetable(ttf,0,false,info);
    show_statetable(st, info, ttf, show_indicflags);
    free_statetable(st);
}

static void readttfmorx_indic(FILE *ttf, FILE *util, struct ttfinfo *info, int stab_len) {
    struct statetable *st;

    st = read_statetable(ttf,2,true,info);
    show_statetablex(st, info, ttf, show_indicflags);
    free_statetable(st);
}

static void show_contextflags(uint8_t *entry,struct statetable *st,struct ttfinfo *info, FILE *ttf) {
    int flags = (entry[2]<<8)|entry[3];
/* the docs say this is unsigned, but that appears not to be the case */
    int mark_offset = (int16_t) ((entry[4]<<8)|entry[5]);
    int cur_offset = (int16_t) ((entry[6]<<8)|entry[7]);
    int i, sub;

    printf( "\t   Flags %04x ", (unsigned int)(flags) );
    if ( flags&0x8000 )
	printf( "Set Mark | ");
    if ( flags&0x4000 )
	printf( "Don't Advance Glyph\n" );
    else
	printf( "Advance Glyph\n" );
/* I hope this is right, docs leave much to the imagination */
/* (apple does not document the "per-glyph substitution table" used by the */
/*  contextual glyph substitution sub-table. */
/* My initial assumption is that there is essentially an big array with one */
/*  entry for every glyph indicating what glyph it will be replaced with */
/*  Since not all glyphs would be valid the tables are probably trimmed and */
/*  the offsets proporting to point to it actually point to garbage until */
/*  adjusted by the appropriate glyph indeces */
/* user will need to look at the table carefully to try and guess what is */
/*  meaningful and what isn't */
    if ( mark_offset!=0 || cur_offset!=0 ) {
	printf( "!!!! Caveat !!!! I am printing out entries that look as though they might\n" );
	printf( "!!!! be meaningful, but that is no guarantee. Examine them carefully to\n" );
	printf( "!!!! find those sections which are actually used.\n" );
    }
    printf( "\t   Offset to substitution table for marked glyph: %d\n", mark_offset );
    if ( mark_offset!=0 ) {
	fseek(ttf,2*(mark_offset+st->first_glyph)+st->state_start,SEEK_SET);
	for ( i=0; i<st->nglyphs; ++i ) {
	    sub = getushort(ttf);
	    if ( sub==0 || (sub>=info->glyph_cnt && sub!=0xffff) )
	continue;
	    printf( "\t    Glyph %d ", st->first_glyph+i );
	    if ( st->first_glyph+i>=info->glyph_cnt )
		printf( "!!! Bad Glyph !!! " );
	    else if ( info->glyph_names!=NULL )
		printf( "%s ", info->glyph_names[st->first_glyph+i]);
	    if ( sub==0xffff )
		printf( "-> Deleted" );
	    else {
		printf( "-> Glyph %d ", sub );
		if ( sub>=info->glyph_cnt )
		    printf( "!!! Bad Glyph !!! " );
		else if ( info->glyph_names!=NULL )
		    printf( "%s", info->glyph_names[sub]);
	    }
	    putchar('\n');
	}
    }
    printf( "\t   Offset to substitution table for current glyph: %d\n", cur_offset );
    if ( cur_offset!=0 ) {
	fseek(ttf,2*(cur_offset+st->first_glyph)+st->state_start,SEEK_SET);
	for ( i=0; i<st->nglyphs; ++i ) {
	    sub = getushort(ttf);
	    if ( sub==0 || (sub>=info->glyph_cnt && sub!=0xffff) )
	continue;
	    printf( "\t    Glyph %d ", st->first_glyph+i );
	    if ( st->first_glyph+i>=info->glyph_cnt )
		printf( "!!! Bad Glyph !!! " );
	    else if ( info->glyph_names!=NULL )
		printf( "%s ", info->glyph_names[st->first_glyph+i]);
	    if ( sub==0xffff )
		printf( "-> Deleted" );
	    else {
		printf( "-> Glyph %d ", sub );
		if ( sub>=info->glyph_cnt )
		    printf( "!!! Bad Glyph !!! " );
		else if ( info->glyph_names!=NULL )
		    printf( "%s", info->glyph_names[sub]);
	    }
	    putchar('\n');
	}
    }
}

static void readttfmort_context(FILE *ttf, FILE *util, struct ttfinfo *info, int stab_len) {
    struct statetable *st;

    st = read_statetable(ttf,2,false,info);
    show_statetable(st, info, ttf, show_contextflags);
    free_statetable(st);
}

static void mort_noncontextualsubs_glyph( FILE *ttf, struct ttfinfo *info ) {
    int gnum = getushort(ttf);

    printf( " Glyph %d (%s)\n", gnum,
	    gnum>=info->glyph_cnt ? "!!!! Bad Glyph !!!!" : info->glyph_names!=NULL ?
	    info->glyph_names[gnum]: "" );
}

static void show_contextflagsx(uint8_t *entry,struct statetable *st,struct ttfinfo *info, FILE *ttf) {
    int flags = (entry[2]<<8)|entry[3];
    int mark_index = ((entry[4]<<8)|entry[5]);
    int cur_index = ((entry[6]<<8)|entry[7]);

    printf( "\t   Flags %04x ", (unsigned int)(flags) );
    if ( flags&0x8000 )
	printf( "Set Mark | ");
    if ( flags&0x4000 )
	printf( "Don't Advance Glyph\n" );
    else
	printf( "Advance Glyph\n" );
    printf( "\t   Index to substitution table for marked glyph: %d\n", mark_index );
    if ( mark_index!=0xffff ) {
	fseek(ttf,st->state_start+st->extra_offsets[0]+4*mark_index,SEEK_SET);
	fseek(ttf,st->state_start+st->extra_offsets[0]+getlong(ttf),SEEK_SET);
	show_applelookuptable(ttf,info,
		mort_noncontextualsubs_glyph);
    }
    printf( "\t   Offset to substitution table for current glyph: %d\n", cur_index );
    if ( cur_index!=0xffff ) {
	fseek(ttf,st->state_start+st->extra_offsets[0]+4*cur_index,SEEK_SET);
	fseek(ttf,st->state_start+st->extra_offsets[0]+getlong(ttf),SEEK_SET);
	show_applelookuptable(ttf,info,
		mort_noncontextualsubs_glyph);
    }
}

static void readttfmorx_context(FILE *ttf, FILE *util, struct ttfinfo *info, int stab_len) {
    struct statetable *st;

    st = read_statetable(ttf,2,true,info);
    show_statetablex(st, info, ttf, show_contextflagsx);
    free_statetable(st);
}

static void show_ligflags(uint8_t *entry,struct statetable *st,struct ttfinfo *info, FILE *ttf) {
    int flags = (entry[2]<<8)|entry[3];
    uint32_t val;

    printf( "\t   Flags %04x ", (unsigned int)(flags) );
    if ( flags&0x8000 )
	printf( "Set Component | ");
    if ( flags&0x4000 )
	printf( "Don't Advance Glyph " );
    else
	printf( "Advance Glyph       " );
    printf( "Offset=%d\n", flags&0x3fff );
    if ( (flags&0x3fff)==0 )
return;

    fseek(ttf,st->state_start+(flags&0x3fff),SEEK_SET);
    do {
	val = getlong(ttf);
	printf( "\t    lig action %08x %s offset=%d\n", val,
		(val&0x80000000)?"last (& store)": 
		(val&0x40000000)?"store": "delete",
		(((int32_t)val)<<2)>>2 );		/* Sign extend */
	/* I think we take 2 * (glyph_id-st->first_glyph + offset) + state_start */
	/*  we get the ?ushort? at this file address and we add it to an */
	/*  accumulated total. When we finally get to a store (or last) */
	/*  we take this accumulated total and ?look it up in the ligature */
	/*  table? to get a glyph index which is the ligature itself. */
	/* Maybe. */
    } while ( !(val&0x80000000) );
}

static void readttfmort_lig(FILE *ttf, FILE *util, struct ttfinfo *info, int stab_len) {
    struct statetable *st;

    st = read_statetable(ttf,0,false,info);
    show_statetable(st, info, ttf, show_ligflags);
    free_statetable(st);
}

static void show_ligxflags(uint8_t *entry,struct statetable *st,struct ttfinfo *info, FILE *ttf) {
    int flags = (entry[2]<<8)|entry[3];
    int index = (entry[4]<<8)|entry[5];
    uint32_t val;

    printf( "\t   Flags %04x ", (unsigned int)(flags) );
    if ( flags&0x8000 )
	printf( "Set Component | ");
    if ( flags&0x2000 )
	printf( "Perform | ");
    if ( flags&0x4000 )
	printf( "Don't Advance Glyph " );
    else
	printf( "Advance Glyph       " );
    if ( flags&0x2000 )
	printf( "Index=%d\n", index );
    else
	printf( "\n");
    if ( (flags&0x2000)==0 )
return;

    fseek(ttf,st->state_start+st->extra_offsets[0]+4*index,SEEK_SET);
    do {
	val = getlong(ttf);
	printf( "\t    lig action %08x %s offset=%d\n", val,
		(val&0x80000000)?"last (& store)": 
		(val&0x40000000)?"store": "delete",
		(((int32_t)val)<<2)>>2 );		/* Sign extend */
	/* I think we take 2 * (glyph_id-st->first_glyph + offset) + state_start */
	/*  we get the ?ushort? at this file address and we add it to an */
	/*  accumulated total. When we finally get to a store (or last) */
	/*  we take this accumulated total and ?look it up in the ligature */
	/*  table? to get a glyph index which is the ligature itself. */
	/* Maybe. */
    } while ( !(val&0x80000000) );
}

static void readttfmorx_lig(FILE *ttf, FILE *util, struct ttfinfo *info, int stab_len) {
    struct statetable *st;

    st = read_statetable(ttf,1,true,info);
    show_statetablex(st, info, ttf, show_ligxflags);
    free_statetable(st);
}

static int32_t memlong(uint8_t *data,int offset) {
    int ch1 = data[offset], ch2 = data[offset+1], ch3 = data[offset+2], ch4 = data[offset+3];
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

static int memushort(uint8_t *data,int offset) {
    int ch1 = data[offset], ch2 = data[offset+1];
return( (ch1<<8)|ch2 );
}

#define MAX_LIG_COMP	16
struct statemachine {
    uint8_t *data;
    int length;
    uint32_t nClasses;
    uint32_t classOffset, stateOffset, entryOffset, ligActOff, compOff, ligOff;
    uint16_t *classes;
    uint16_t lig_comp_classes[MAX_LIG_COMP];
    uint16_t lig_comp_glyphs[MAX_LIG_COMP];
    int lcp;
    uint8_t *states_in_use;
    int smax;
    struct ttfinfo *info;
    int cnt;
};

static void mort_figure_ligatures(struct statemachine *sm, int lcp, int off, int32_t lig_offset) {
    uint32_t lig;
    int i, j, lig_glyph;

    if ( lcp<0 || off+3>sm->length )
return;

    lig = memlong(sm->data,off);
    off += sizeof(long);

    for ( i=0; i<sm->info->glyph_cnt; ++i ) if ( sm->classes[i]==sm->lig_comp_classes[lcp] ) {
	sm->lig_comp_glyphs[lcp] = i;
	lig_offset += memushort(sm->data,2*( ((((int32_t) lig)<<2)>>2) + i ) );
	if ( lig&0xc0000000 ) {
	    if ( lig_offset+1 > sm->length ) {
		fprintf( stderr, "Invalid ligature offset\n" );
    break;
	    }
	    lig_glyph = memushort(sm->data,lig_offset);
	    if ( lig_glyph>=sm->info->glyph_cnt ) {
		fprintf(stderr, "Attempt to make a ligature for glyph %d out of ", lig_glyph );
		for ( j=lcp; j<sm->lcp; ++j )
		    fprintf(stderr,"%d ",sm->lig_comp_glyphs[j]);
		fprintf(stderr,"\n");
	    } else {
		printf( "\t\tGlyph %d (%s) is a ligature of:\n",
			lig_glyph, lig_glyph>=sm->info->glyph_cnt ? "!!!! Bad Glyph !!!!" : sm->info->glyph_names!=NULL ? sm->info->glyph_names[lig_glyph] : "" );
		for ( j=lcp; j<sm->lcp; ++j )
		    printf( "\t\t\t%d (%s)\n", sm->lig_comp_glyphs[j],
			    sm->lig_comp_glyphs[j]>=sm->info->glyph_cnt ? "!!!! Bad Glyph !!!!" : sm->info->glyph_names!=NULL ? sm->info->glyph_names[sm->lig_comp_glyphs[j]] : "" );
	    }
	} else
	    mort_figure_ligatures(sm,lcp-1,off,lig_offset);
	lig_offset -= memushort(sm->data,2*( ((((int32_t) lig)<<2)>>2) + i ) );
    }
}

static void follow_mort_state(struct statemachine *sm,int offset,int class) {
    int state = (offset-sm->stateOffset)/sm->nClasses;
    int class_top, class_bottom;

    if ( state<0 || state>=sm->smax || sm->states_in_use[state] || sm->lcp>=MAX_LIG_COMP )
return;
    ++ sm->cnt;
    if ( sm->cnt>=max_lig_nest ) {
	if ( sm->cnt==max_lig_nest )
	    fprintf( stderr, "ligature state machine too complex, giving up\n" );
return;
    }

    sm->states_in_use[state] = true;

    if ( class==-1 ) { class_bottom = 0; class_top = sm->nClasses; }
    else { class_bottom = class; class_top = class+1; }
    for ( class=class_bottom; class<class_top; ++class ) {
	int ent = sm->data[offset+class];
	int newState = memushort(sm->data,sm->entryOffset+4*ent);
	int flags = memushort(sm->data,sm->entryOffset+4*ent+2);
	if (( state!=0 && sm->data[sm->stateOffset+class] == ent ) ||	/* If we have the same entry as state 0, then presumably we are ignoring the components read so far and starting over with a new lig */
		(state>1 && sm->data[sm->stateOffset+sm->nClasses+class]==ent ))	/* Similarly for state 1 */
    continue;
	if ( flags&0x8000 )	/* Set component */
	    sm->lig_comp_classes[sm->lcp++] = class;
	if ( flags&0x3fff ) {
	    mort_figure_ligatures(sm, sm->lcp-1, flags & 0x3fff, 0);
	} else if ( flags&0x8000 )
	    follow_mort_state(sm,newState,(flags&0x4000)?class:-1);
	if ( flags&0x8000 )
	    --sm->lcp;
    }
    sm->states_in_use[state] = false;
}

static void morx_figure_ligatures(struct statemachine *sm, int lcp, int ligindex, int32_t lig_offset) {
    uint32_t lig;
    int i, j, lig_glyph;

    if ( lcp<0 || sm->ligActOff+4*ligindex+3>sm->length )
return;

    lig = memlong(sm->data,sm->ligActOff+4*ligindex);
    ++ligindex;

    for ( i=0; i<sm->info->glyph_cnt; ++i ) if ( sm->classes[i]==sm->lig_comp_classes[lcp] ) {
	sm->lig_comp_glyphs[lcp] = i;
	lig_offset += memushort(sm->data,sm->compOff + 2*( ((((int32_t) lig)<<2)>>2) + i ) );
	if ( lig&0xc0000000 ) {
	    if ( sm->ligOff+2*lig_offset+1 > sm->length ) {
		fprintf( stderr, "Invalid ligature offset\n" );
    break;
	    }
	    lig_glyph = memushort(sm->data,sm->ligOff+2*lig_offset);
	    if ( lig_glyph>=sm->info->glyph_cnt ) {
		fprintf(stderr, "Attempt to make a ligature for glyph %d out of ", lig_glyph );
		for ( j=lcp; j<sm->lcp; ++j )
		    fprintf(stderr,"%d ",sm->lig_comp_glyphs[j]);
		fprintf(stderr,"\n");
	    } else {
		printf( "\t\tGlyph %d (%s) is a ligature of:\n",
			lig_glyph, lig_glyph>=sm->info->glyph_cnt ? "!!!! Bad Glyph !!!!" : sm->info->glyph_names!=NULL ? sm->info->glyph_names[lig_glyph] : "" );
		for ( j=lcp; j<sm->lcp; ++j )
		    printf( "\t\t\t%d (%s)\n", sm->lig_comp_glyphs[j],
			    sm->lig_comp_glyphs[j]>=sm->info->glyph_cnt ? "!!!! Bad Glyph !!!!" : sm->info->glyph_names!=NULL ? sm->info->glyph_names[sm->lig_comp_glyphs[j]] : "" );
	    }
	} else
	    morx_figure_ligatures(sm,lcp-1,ligindex,lig_offset);
	lig_offset -= memushort(sm->data,sm->compOff + 2*( ((((int32_t) lig)<<2)>>2) + i ) );
    }
}

static void follow_morx_state(struct statemachine *sm,int state,int class) {
    int class_top, class_bottom;

    if ( state<0 || state>=sm->smax || sm->states_in_use[state] || sm->lcp>=MAX_LIG_COMP )
return;
    ++ sm->cnt;
    if ( sm->cnt>=max_lig_nest ) {
	if ( sm->cnt==max_lig_nest )
	    fprintf( stderr, "ligature state machine too complex, giving up\n" );
return;
    }

    sm->states_in_use[state] = true;

    if ( class==-1 ) { class_bottom = 0; class_top = sm->nClasses; }
    else { class_bottom = class; class_top = class+1; }
    for ( class=class_bottom; class<class_top; ++class ) {
	int ent = memushort(sm->data, sm->stateOffset + 2*(state*sm->nClasses+class) );
	int newState = memushort(sm->data,sm->entryOffset+6*ent);
	int flags = memushort(sm->data,sm->entryOffset+6*ent+2);
	int ligindex = memushort(sm->data,sm->entryOffset+6*ent+4);
	/* If we have the same entry as state 0, then presumably we are */
	/*  ignoring the components read so far and starting over with a new */
	/*  lig (similarly for state 1) */
	if (( state!=0 && memushort(sm->data, sm->stateOffset + 2*class) == ent ) ||
		(state>1 && memushort(sm->data, sm->stateOffset + 2*(sm->nClasses+class))==ent ))
    continue;
	if ( flags&0x8000 )	/* Set component */
	    sm->lig_comp_classes[sm->lcp++] = class;
	if ( flags&0x2000 ) {
	    morx_figure_ligatures(sm, sm->lcp-1, ligindex, 0);
	} else if ( flags&0x8000 )
	    follow_morx_state(sm,newState,(flags&0x4000)?class:-1);
	if ( flags&0x8000 )
	    --sm->lcp;
    }
    sm->states_in_use[state] = false;
}

static void readttf_mortx_lig(FILE *ttf,struct ttfinfo *info,int ismorx,uint32_t base,uint32_t length) {
    uint32_t here;
    struct statemachine sm;
    int first, cnt, i;

    memset(&sm,0,sizeof(sm));
    sm.info = info;
    here = ftell(ttf);
    length -= here-base;
    sm.data = malloc(length);
    sm.length = length;
    if ( fread(sm.data,1,length,ttf)!=length ) {
	free(sm.data);
	fprintf( stderr, "Bad mort/morx ligature table. Not long enough\n");
return;
    }
    fseek(ttf,here,SEEK_SET);
    if ( ismorx ) {
	sm.nClasses = memlong(sm.data,0);
	sm.classOffset = memlong(sm.data,sizeof(long));
	sm.stateOffset = memlong(sm.data,2*sizeof(long));
	sm.entryOffset = memlong(sm.data,3*sizeof(long));
	sm.ligActOff = memlong(sm.data,4*sizeof(long));
	sm.compOff = memlong(sm.data,5*sizeof(long));
	sm.ligOff = memlong(sm.data,6*sizeof(long));
	fseek(ttf,here+sm.classOffset,SEEK_SET);
	sm.classes = info->morx_classes = malloc(info->glyph_cnt*sizeof(uint16_t));
	for ( i=0; i<info->glyph_cnt; ++i )
	    sm.classes[i] = 1;			/* Out of bounds */
	readttf_applelookup(ttf,info,
		mortclass_apply_values,mortclass_apply_value,NULL,NULL);
	sm.smax = length/(2*sm.nClasses);
	sm.states_in_use = calloc(sm.smax,sizeof(uint8_t));
	follow_morx_state(&sm,0,-1);
    } else {
	sm.nClasses = memushort(sm.data,0);
	sm.classOffset = memushort(sm.data,sizeof(uint16_t));
	sm.stateOffset = memushort(sm.data,2*sizeof(uint16_t));
	sm.entryOffset = memushort(sm.data,3*sizeof(uint16_t));
	sm.ligActOff = memushort(sm.data,4*sizeof(uint16_t));
	sm.compOff = memushort(sm.data,5*sizeof(uint16_t));
	sm.ligOff = memushort(sm.data,6*sizeof(uint16_t));
	sm.classes = malloc(info->glyph_cnt*sizeof(uint16_t));
	for ( i=0; i<info->glyph_cnt; ++i )
	    sm.classes[i] = 1;			/* Out of bounds */
	first = memushort(sm.data,sm.classOffset);
	cnt = memushort(sm.data,sm.classOffset+sizeof(uint16_t));
	for ( i=0; i<cnt; ++i )
	    sm.classes[first+i] = sm.data[sm.classOffset+2*sizeof(uint16_t)+i];
	sm.smax = length/sm.nClasses;
	sm.states_in_use = calloc(sm.smax,sizeof(uint8_t));
	follow_mort_state(&sm,sm.stateOffset,-1);
    }
    free(sm.data);
    free(sm.states_in_use);
    free(sm.classes);
}

static void readttfmetamorph(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int n, i, j, k, l, nf, ns, type, setting, stab_len, coverage;
    uint32_t chain_start, len, temp, stab_start, flags, here;
    int features[32], settings[32], masks[32];
    int ismorx = false;

    if ( info->morx_start!=0 ) {
	fseek(ttf,info->morx_start,SEEK_SET);
	printf( "\nmorx table (at %d) (Glyph metamorphosis extended)\n", info->morx_start );
	ismorx = true;
    } else {
	fseek(ttf,info->mort_start,SEEK_SET);
	printf( "\nmort table (at %d) (Glyph metamorphosis)\n", info->mort_start );
    }
    printf( "\t version=%g\n", getfixed(ttf));
    n = getlong(ttf);
    printf( "\t number of chains=%d\n", n);
    for ( i=0; i<n; ++i ) {
	printf( "\t For Chain %d\n", i );
	chain_start = ftell(ttf);
	printf( "\t  default flags=%lx\n", (long unsigned int)(getlong(ttf)) );
	printf( "\t  chain length=%ld\n", (long)((len = getlong(ttf))) );
	printf( "\t  number Feature Entries=%d\n", nf = ismorx ? getlong(ttf) : getushort(ttf));
	printf( "\t  number Subtables=%d\n", ns = ismorx ? (int) getlong(ttf) : getushort(ttf));
	for ( j=k=0; j<nf; ++j ) {
	    printf( "\t  For Feature %d of Chain %d\n", j, i );
	    printf( "\t   Feature Type=%d ", type = getushort(ttf));
	    printf( "%s\n", getfeaturename(info,type));
	    printf( "\t   Feature Setting=%d ", setting=getushort(ttf));
	    printf( "%s\n", getsettingname(info,type, setting));
	    printf( "\t   Enable Flags=%08lx\n", (long unsigned int)((flags = getlong(ttf))) );
	    printf( "\t   Disable Flags=%08lx ", (long unsigned int)((temp=getlong(ttf))) );
	    printf( "(Complement=%08lx)\n", (long unsigned int)(~temp) );
	    /* try to get a unique flag value for this feature setting */
	    for ( l=0; l<k; ++l )
		flags &= ~masks[l];
	    if ( k<32 && flags!=0 ) {
		features[k] = type;
		settings[k] = setting;
		masks[k++] = flags;
	    }
	}
	for ( j=0; j<ns; ++j ) {
	    stab_start = ftell(ttf);
	    if ( ismorx ) {
		stab_len = getlong(ttf);
		coverage = getlong(ttf);
	    } else {
		stab_len = getushort(ttf);
		coverage = getushort(ttf);
		coverage = ((coverage&0xe000)<<16) | (coverage&7);	/* convert to morx format */
	    }
	    flags = getlong(ttf);
	    for ( l=0; l<k; ++l )
		if ( masks[l]==flags )
	    break;
	    printf( "\t Subtable %d ", j);
	    if ( l!=k )
		printf( "Probably for feature=%d (%s)\n\t\t setting=%d (%s)\n",
			features[l], getfeaturename(info,features[l]),
			settings[l], getsettingname(info,features[l],settings[l]));
	    else
		printf( "\n" );
	    printf( "\t  Length = %d\n", stab_len );
	    printf( "\t  Coverage = %08x, Apply=%s Search=%s\n\t\tType=%s\n",
		(unsigned int)(coverage),
		(coverage&0x20000000) ? "Always" : (coverage&0x80000000) ? "Vertical" : "Horizontal",
		(coverage&0x40000000) ? "Descending (?Right2Left?)" : "Ascending (?Left2Right?)",
		(coverage&0xff)==0 ? "Indic rearrangement" :
		(coverage&0xff)==1 ? "contextual glyph substitution" :
		(coverage&0xff)==2 ? "Ligature substitution" :
		(coverage&0xff)==4 ? "non-contextual glyph substitution" :
		(coverage&0xff)==5 ? "contextual glyph insertion" :
		    "Unknown" );
	    printf( "\t  Flags=%08lx\n", (long unsigned int)(flags) );
	    switch( (coverage&0x7) ) {
	      case 0:
		if ( !ismorx )
		    readttfmort_indic(ttf,util,info,stab_len);
		else
		    readttfmorx_indic(ttf,util,info,stab_len);
	      break;
	      case 1:
		if ( !ismorx )
		    readttfmort_context(ttf,util,info,stab_len);
		else
		    readttfmorx_context(ttf,util,info,stab_len);
	      break;
	      case 2:
		here = ftell(ttf);
		readttf_mortx_lig(ttf,info,ismorx,stab_start,stab_len);
		if ( verbose ) {
		    fseek(ttf,here,SEEK_SET);
		    if ( !ismorx )
			readttfmort_lig(ttf,util,info,stab_len);
		    else
			readttfmorx_lig(ttf,util,info,stab_len);
		}
	      break;
	      case 4:
		show_applelookuptable(ttf,info,
			mort_noncontextualsubs_glyph);
	      break;
	      case 5:
	      break;
	    }
	    fseek(ttf,stab_start+stab_len,SEEK_SET);
	}
	len = ((len+3)>>2)<<2;
	fseek(ttf,chain_start+len,SEEK_SET);
    }
}

static void showagproperties( uint16_t props ) {

    printf( "%04x=", props );
    if ( props&0x8000 ) printf( "Floater|" );
    if ( props&0x4000 ) printf( "HangLeft|" );
    if ( props&0x2000 ) printf( "HangRight|" );
    if ( props&0x1000 ) printf( "Mirror += %d|", (((int32_t) props)<<20)>>28 );
    if ( props&0x0080 ) printf( "AttachRight|" );
    switch (props&0x1f ) {
      case 0: printf( "Strong L2R" ); break;
      case 1: printf( "Strong Hebrew" ); break;
      case 2: printf( "Strong Arabic" ); break;
      case 3: printf( "Euro Digit" ); break;
      case 4: printf( "Euro Num Sep" ); break;
      case 5: printf( "Euro Num Term" ); break;
      case 6: printf( "Arabic Digit" ); break;
      case 7: printf( "Common Num Sep" ); break;
      case 8: printf( "Block Sep" ); break;
      case 9: printf( "Segment Sep" ); break;
      case 10: printf( "White Space" ); break;
      case 11: printf( "Other Neutral" ); break;
      default: printf( "Undocumented Unicode 3 direction %d", props&0x1f ); break;
    }
    putchar('\n');
}

static void prop_show( FILE *ttf, struct ttfinfo *info ) {
    showagproperties( getushort(ttf));
}

static void readttfappleprop(FILE *ttf, FILE *util, struct ttfinfo *info) {

    fseek(ttf,info->prop_start,SEEK_SET);
    printf( "\nprop table (at %d) (Glyph properties)\n", info->prop_start );
    printf( "\t version=%g\n", getfixed(ttf));
    printf( "\t has lookup data=%d\n", getushort(ttf));
    printf( "\t default properties=" );
    showagproperties( getushort(ttf));
    show_applelookuptable(ttf,info,prop_show);
}

static void lcar_show( FILE *ttf, struct ttfinfo *info ) {
    uint32_t here;
    int cnt,i,off;

    off = getushort(ttf);
    here = ftell(ttf);
    fseek(ttf, info->lcar_start+off,SEEK_SET );
    printf( " caret cnt=%d\n", cnt = getushort(ttf));
    for ( i=0; i<cnt; ++i )
	printf( "\t\tcaret %d at %d\n", i, getushort(ttf));
    fseek(ttf,here,SEEK_SET);
}

static void readttfapplelcar(FILE *ttf, FILE *util, struct ttfinfo *info) {

    fseek(ttf,info->lcar_start,SEEK_SET);
    printf( "\nlcar table (at %d) (Ligature carets)\n", info->lcar_start );
    printf( "\t version=%g\n", getfixed(ttf));
    printf( "\t data are points=%d\n", getushort(ttf));
    show_applelookuptable(ttf,info,lcar_show);
}

static void opbd_show( FILE *ttf, struct ttfinfo *info ) {
    uint32_t here;
    int off;

    off = getushort(ttf);
    here = ftell(ttf);
    fseek(ttf, info->opbd_start+off,SEEK_SET );
    printf( " optical left=%d\n", (short) getushort(ttf));
    printf( " optical top=%d\n", (short) getushort(ttf));
    printf( " optical right=%d\n", (short) getushort(ttf));
    printf( " optical bottom=%d\n", (short) getushort(ttf));
    fseek(ttf,here,SEEK_SET);
}

static void readttfappleopbd(FILE *ttf, FILE *util, struct ttfinfo *info) {

    fseek(ttf,info->opbd_start,SEEK_SET);
    printf( "\nopbd table (at %d) (Optical Bounds)\n", info->opbd_start );
    printf( "\t version=%g\n", getfixed(ttf));
    printf( "\t data are points=%d\n", getushort(ttf));
    show_applelookuptable(ttf,info,opbd_show);
}

static void readttfapplefvar(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int dataoff, countsizepairs, axiscount, instancecount, instancesize, nameid;
    uint32_t tag;
    char *name;
    int i,j;

    fseek(ttf,info->fvar_start,SEEK_SET);
    printf( "\nfvar table (at %d) (Font Variations)\n", info->fvar_start );
    printf( "\t version=%g\n", getfixed(ttf));
    printf( "\t offset to data=%d\n", dataoff = getushort(ttf));
    printf( "\t # size pairs=%d\n", countsizepairs = getushort(ttf));
    printf( "\t Axis count=%d\n", axiscount = getushort(ttf));
    info->fvar_axiscount = axiscount;
    printf( "\t Axis size=%d\n", getushort(ttf));
    printf( "\t Instance count=%d\n", instancecount = getushort(ttf));
    printf( "\t Instance size=%d\n", instancesize = getushort(ttf));
    for ( i=0; i<axiscount; ++i ) {
	printf( "\t  Axis %d\n", i );
	tag = getlong(ttf);
	printf( "\t    Axis Tag '%c%c%c%c'\n",
		(char)((tag>>24)&0xff), (char)((tag>>16)&0xff),
		(char)((tag>>8)&0xff), (char)(tag&0xff) );
	printf( "\t    minValue=%g\n", getfixed(ttf));
	printf( "\t    defaultValue=%g\n", getfixed(ttf));
	printf( "\t    maxValue=%g\n", getfixed(ttf));
	printf( "\t    flags=%x\n", (unsigned int)(getushort(ttf)) );
	nameid = getushort(ttf);
	name = getttfname(util,info,nameid);
	printf( "\t    nameid=%d (%s)\n", nameid, name==NULL ? "Not Found" : name );
        free(name);
    }
    for ( i=0; i<instancecount; ++i ) {
	printf( "\t  Instance %d\n", i );
	nameid = getushort(ttf);
	name = getttfname(util,info,nameid);
	printf( "\t    nameid=%d (%s)\n", nameid, name==NULL ? "Not Found" : name );
	printf( "\t    flags=%x\n", (unsigned int)(getushort(ttf)) );
	printf( "\t    Blend coefficients: ");
	for ( j=0; j<axiscount; ++j )
	    printf( "%g, ", getfixed(ttf));
	printf( "\n" );
        free(name);
    }
    printf( "\n" );
}

static void readttfapplegvar(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int axiscount, gcc, glyphCount, flags;
    uint32_t *offsets, offset2Coord, offset2Data;
    int tupleCount, offset;
    int i, j, k, index;

    fseek(ttf,info->gvar_start,SEEK_SET);
    printf( "\ngvar table (at %d) (Glyph Variations)\n", info->gvar_start );
    printf( "\t version=%g\n", getfixed(ttf));
    printf( "\t Axis count=%d\n", axiscount = getushort(ttf));
    if ( axiscount!=info->fvar_axiscount )
	fprintf( stderr, "The axis count in the gvar table differs from that in the fvar table.\n 'gvar' axes=%d, 'fvar' axes=%d\n", axiscount, info->fvar_axiscount );
    printf( "\t global coord count=%d\n", gcc = getushort(ttf));
    printf( "\t offset to coord=%d\n", (int)((offset2Coord = getlong(ttf))) );
    printf( "\t glyph count=%d\n", glyphCount = getushort(ttf));
    printf( "\t flags=%x\n", (unsigned int)((flags = getushort(ttf))) );
    printf( "\t offset to data=%x\n", offset2Data = getlong(ttf));
    offsets = malloc(glyphCount*sizeof(uint32_t));
    if ( flags&1 ) {
	for ( i=0; i<glyphCount; ++i )
	    offsets[i] = getlong(ttf) + offset2Data + info->gvar_start;
    } else {
	for ( i=0; i<glyphCount; ++i )
	    offsets[i] = getushort(ttf)*2 + offset2Data + info->gvar_start;
    }
    for ( i=0; i<glyphCount; ++i ) if ( offsets[i]!=offsets[i+1] ) {
	fseek(ttf,offsets[i],SEEK_SET);
	printf( "\t  Glyph %d %s\n", i, info->glyph_names!=NULL? info->glyph_names[i]: "" );
	tupleCount = getushort(ttf);
	offset = getushort(ttf);
	printf( "\t    Tuple count=%x, (count=%d) tuples %sshare points\n",
		(unsigned int)(tupleCount), tupleCount&0xfff,
		(tupleCount&0x8000)? "" : "do not " );
	printf( "\t    Offset=%d\n", offset );
	for ( j=0; j<(tupleCount&0xfff); ++j ) {
	    printf( "\t     Tuple %d\n", j );
	    printf( "\t      Size %d\n", getushort(ttf) );
	    index = getushort(ttf);
	    printf( "\t      Index %d%s%s%s\n", index&0xfff,
		    index&0x8000 ? ", Embedded tuple" : "",
		    index&0x4000 ? ", Intermediate tuple" : "",
		    index&0x2000 ? ", Private points" : "" );
	    if ( index&0x8000 ) {
		printf( "\t      Coords[%g", ((short) getushort(ttf))/16384.0 );
		for ( k=1; k<axiscount; ++k )
		    printf( ",%g", ((short) getushort(ttf))/16384.0 );
		printf( "]\n" );
	    }
	    if ( index&0x4000 ) {
		printf( "\t      Intermediate[%g-%g", ((short) getushort(ttf)/16384.0),
			((short) getushort(ttf)/16384.0));
		for ( k=1; k<axiscount; ++k )
		    printf( ",%g-%g", ((short) getushort(ttf)/16384.0),
			    ((short) getushort(ttf)/16384.0));
		printf( "]\n" );
	    }
	    if ( !(index&0xc000) ) {
		int here = ftell(ttf);
		fseek(ttf,info->gvar_start+offset2Coord+(index&0xfff)*axiscount*2,SEEK_SET);
		printf( "\t      Global Coords[%g", ((short) getushort(ttf))/16384.0 );
		for ( k=1; k<axiscount; ++k )
		    printf( ",%g", ((short) getushort(ttf))/16384.0 );
		printf( "]\n" );
		fseek(ttf,here,SEEK_SET);
	    }
	}
    }

    printf( "\n" );
    free(offsets);
}

static void readttfgasp(FILE *ttf, FILE *util, struct ttfinfo *info) {
    int n, i, ppem, flags, last=0;

    fseek(ttf,info->gasp_start,SEEK_SET);
    printf( "\ngasp table (at %d) (grid fitting and scan conversion table)\n", info->gasp_start );
    printf( "\t version=%d\n", getushort(ttf));
    printf( "\t Number of gasp entries=%d\n", n = getushort(ttf));
    for ( i=0; i<n; ++i ) {
	ppem = getushort(ttf);
	flags = getushort(ttf);
	if ( i==0 && ppem==0xffff )
	    printf( "\t  All sizes: " );
	else if ( i==0 )
	    printf( "\t  Sizes below %d:     \t", ppem );
	else if ( ppem!=0xffff )
	    printf( "\t  Sizes >= %d and <%d:\t", last, ppem );
	else
	    printf( "\t  All sizes >= %d:    \t", last );
	last = ppem;
	if ( flags==0 )
	    printf( "Nothing (No gridfitting, no anti-aliasing)\n" );
	else if ( flags==1 )
	    printf( "Grid Fit (no anti-aliasing)\n" );
	else if ( flags==2 )
	    printf( "Anti-alias (no grid fitting)\n" );
	else
	    printf( "Both Grid Fitting and Anti-Aliasing\n" );
    }
}

static void readtableinstr(FILE *ttf, int start, int len, const char *string) {
    int i, j, ch, n, ch1, ch2;

    if ( start==0 )
return;

    fseek(ttf,start,SEEK_SET);
    printf( "\n%s table (at %d for %d bytes)\n\t", string, start, len );
    for ( i=0; i<len; ++i ) {
	printf( "%s ", instrs[ch = getc(ttf)]);
	if ( ch==0x40 ) {		/* NPUSHB */
	    printf( "(%d) ", n = getc(ttf)); ++i;
	    for ( j=0; j<n; ++j, ++i )
		printf( "%d ", getc(ttf));
	    printf( "\n" );
	} else if ( ch==0x41 ) {	/* NPUSHW */
	    printf( "(%d) ", n = getc(ttf)); ++i;
	    for ( j=0; j<n; ++j, i += 2 ) {
		ch1=getc(ttf); ch2 = getc(ttf);
		printf( "%d ", (short) ((ch1<<8)|ch2) );
	    }
	    printf( "\n" );
	} else if ( ch>=0xb0 && ch<=0xb7 ) {	/* PUSHBn */
	    n = (ch-0xb0)+1;
	    for ( j=0; j<n; ++j, ++i )
		printf( "%d ", getc(ttf));
	    printf( "\n" );
	} else if ( ch>=0xb8 && ch<=0xbf ) {
	    n = (ch-0xb8)+1;
	    for ( j=0; j<n; ++j, i += 2 ) {
		ch1=getc(ttf); ch2 = getc(ttf);
		printf( "%d ", (short) ((ch1<<8)|ch2) );
	    }
	    printf( "\n" );
	}
    }
    printf("\n");
}

static char **readcfffontnames(FILE *ttf, int ltype) {
    uint16_t count = getushort(ttf);
    int offsize;
    uint32_t *offsets;
    char **names;
    int i,j;
    const char *labels[] = { "Font Name", "String", NULL };
    const char *lab2[] = { "fontnames", "strings", NULL };

    printf( "\nThere %s %d %s in this cff\n", count==1?"is":"are", count, lab2[ltype] );
    if ( count==0 )
return( NULL );
    offsets = malloc((count+1)*sizeof(uint32_t));
    offsize = getc(ttf);
    printf( " Name Index Offset Size: %d\n Offsets: ", offsize );
    for ( i=0; i<=count; ++i ) {
	offsets[i] = getoffset(ttf,offsize);
	if ( i==0 && offsets[0]!=1 )
	    fprintf(stderr, "!! Initial offset must be one in %s\n", labels[ltype]);
	printf( "%d ", (int)(offsets[i]) );
    }
    putchar('\n');
    names = malloc((count+1)*sizeof(char *));
    for ( i=0; i<count; ++i ) {
	names[i] = malloc(offsets[i+1]-offsets[i]+1);
	for ( j=0; j<offsets[i+1]-offsets[i]; ++j )
	    names[i][j] = getc(ttf);
	names[i][j] = '\0';
	printf( " %s %2d: %s\n", labels[ltype], i, names[i]);
    }
    names[i] = NULL;
    free(offsets);
return( names );
}

static char *addnibble(char *pt, int nib) {
    if ( nib<=9 )
	*pt++ = nib+'0';
    else if ( nib==10 )
	*pt++ = '.';
    else if ( nib==11 )
	*pt++ = 'E';
    else if ( nib==12 ) {
	*pt++ = 'E';
	*pt++ = '-';
    } else if ( nib==14 )
	*pt++ = '-';
    else if ( nib==15 )
	*pt++ = '\0';
return( pt );
}

static int readcffthing(FILE *ttf,int *_ival,double *dval,int *operand) {
    char buffer[50], *pt;
    int ch, ival;

    ch = getc(ttf);
    if ( ch==12 ) {
	*operand = (12<<8) | getc(ttf);
return( 3 );
    } else if ( ch<=21 ) {
	*operand = ch;
return( 3 );
    } else if ( ch==30 ) {
	pt = buffer;
	do {
	    ch = getc(ttf);
	    pt = addnibble(pt,ch>>4);
	    pt = addnibble(pt,ch&0xf);
	} while ( pt[-1]!='\0' );
	*dval = strtod(buffer,NULL);
return( 2 );
    } else if ( ch>=32 && ch<=246 ) {
	*_ival = ch-139;
return( 1 );
    } else if ( ch>=247 && ch<=250 ) {
	*_ival = ((ch-247)<<8) + getc(ttf)+108;
return( 1 );
    } else if ( ch>=251 && ch<=254 ) {
	*_ival = -((ch-251)<<8) - getc(ttf)-108;
return( 1 );
    } else if ( ch==28 ) {
	ival = getc(ttf)<<8;
	*_ival = (short) (ival | getc(ttf));
return( 1 );
    } else if ( ch==29 ) {
	ival = getc(ttf)<<24;
	ival = ival | getc(ttf)<<16;
	ival = ival | getc(ttf)<<8;
	*_ival = (int) (ival | getc(ttf));
return( 1 );
    }
    printf( "Unexpected value in dictionary %d\n", ch );
    *_ival = 0;
return( 0 );
}

struct pschars {
    int cnt, next;
    char **keys;
    uint8_t **values;
    int *lens;
    int bias;
};

struct topdicts {
    int32_t cff_start;

    char *fontname;	/* From Name Index */

    int version;
    int notice;		/* SID */
    int copyright;	/* SID */
    int fullname;	/* SID */
    int familyname;	/* SID */
    int weight;		/* SID */
    int isfixedpitch;
    double italicangle;
    double underlinepos;
    double underlinewidth;
    int painttype;
    int charstringtype;
    double fontmatrix[6];
    int uniqueid;
    double fontbb[4];
    double strokewidth;
    int xuid[20];
    int charsetoff;	/* from start of file */
    int encodingoff;	/* from start of file */
    int charstringsoff;	/* from start of file */
    int private_size;
    int private_offset;	/* from start of file */
    int synthetic_base;	/* font index */
    int postscript_code;	/* SID */
 /* synthetic fonts only */
    int basefontname;		/* SID */
    int basefontblend[16];	/* delta */
 /* CID fonts only (what we expect to get) */
    int ros_registry;		/* SID */
    int ros_ordering;		/* SID */
    int ros_supplement;
    int cidfontversion;
    int cidfontrevision;
    int cidfonttype;
    int cidcount;
    int uidbase;
    int fdarrayoff;	/* from start of file */
    int fdselectoff;	/* from start of file */
    int sid_fontname;	/* SID */
/* Private stuff */
    double bluevalues[14];
    double otherblues[10];
    double familyblues[14];
    double familyotherblues[10];
    double bluescale;
    double blueshift;
    double bluefuzz;
    int stdhw;
    int stdvw;
    double stemsnaph[10];
    double stemsnapv[10];
    int forcebold;
    int languagegroup;
    double expansionfactor;
    int initialRandomSeed;
    int subrsoff;	/* from start of this private table */
    int defaultwidthx;
    int nominalwidthx;

    struct pschars glyphs;
    struct pschars local_subrs;
    uint16_t *charset;
};

static void ShowCharString(uint8_t *str,int len,int type) {
    int v;
    int val;
    /* most things are the same about type1 and type2 strings. Type2 just has */
    /*  extra operators (and gets rid of seac). The 5 byte number sequence is */
    /*  different though... */

    do {
	if ( (v = *str++)>=32 ) {
	    if ( v<=246) {
		val = v - 139;
	    } else if ( v<=250 ) {
		val = (v-247)*256 + *str++ + 108;
		--len;
	    } else if ( v<=254 ) {
		val = -(v-251)*256 - *str++ - 108;
		--len;
	    } else {
		val = (*str<<24) | (str[1]<<16) | (str[2]<<8) | str[3];
		str += 4;
		len -= 4;
	    }
	    if ( v==0xff && type==2 )
		printf( "%g ", val/65536. );
	    else
		printf( "%d ", val );
	} else if ( v==28 ) {
	    val = (short) ((str[0]<<8) | str[1]);
	    str += 2;
	    len -= 2;
	    printf( "%d ", val );
	} else if ( v==12 ) {
	    v = *str++;
	    --len;
	    switch ( v ) {
	      case 3: printf( "and " ); break;
	      case 4: printf( "or " ); break;
	      case 5: printf( "not " ); break;
	      case 9: printf( "abs " ); break;
	      case 10: printf( "add " ); break;
	      case 11: printf( "sub " ); break;
	      case 12: printf( "div " ); break;
	      case 14: printf( "neg " ); break;
	      case 15: printf( "eq " ); break;
	      case 18: printf( "drop " ); break;
	      case 20: printf( "put " ); break;
	      case 21: printf( "get " ); break;
	      case 22: printf( "ifelse " ); break;
	      case 23: printf( "random " ); break;
	      case 24: printf( "mul " ); break;
	      case 26: printf( "sqrt " ); break;
	      case 27: printf( "dup " ); break;
	      case 28: printf( "exch " ); break;
	      case 29: printf( "index " ); break;
	      case 30: printf( "roll " ); break;
	      case 34: printf( "hflex " ); break;
	      case 35: printf( "flex " ); break;
	      case 36: printf( "hflex1 " ); break;
	      case 37: printf( "flex1 " ); break;
/* Type 1 codes */
	      case 0: printf( "dotsection " ); break;
	      case 1: printf( "vstem3 " ); break;
	      case 2: printf( "hstem3 " ); break;
	      case 6: printf( "seac " ); break;
	      case 7: printf( "sbw " ); break;
	      case 16: printf( "callothersubr " ); break;
	      case 17: printf( "pop " ); break;
	      case 33: printf( "setcurrentpoint " ); break;
/* End obsolete codes */
	      default: printf( "?\?\?-12-%d-??? ", v ); break;
	    }
	} else switch ( v ) {
	  case 1: printf( "hstem " ); break;
	  case 3: printf( "vstem " ); break;
	  case 4: printf( "vmoveto " ); break;
	  case 5: printf( "rlineto " ); break;
	  case 6: printf( "hlineto " ); break;
	  case 7: printf( "vlineto " ); break;
	  case 8: printf( "rrcurveto " ); break;
	  case 10: printf( "callsubr " ); break;
	  case 11: printf( "return " ); break;
	  case 14: printf( "endchar " ); break;
	  case 18: printf( "hstemhm " ); break;
	  case 19: printf( "hintmask " ); break;
	  case 20: printf( "cntrmask " ); break;
	  case 21: printf( "rmoveto " ); break;
	  case 22: printf( "hmoveto " ); break;
	  case 23: printf( "vstemhm " ); break;
	  case 24: printf( "rcurveline " ); break;
	  case 25: printf( "rlinecurve " ); break;
	  case 26: printf( "vvcurveto " ); break;
	  case 27: printf( "hhcurveto " ); break;
	  case 29: printf( "callgsubr " ); break;
	  case 30: printf( "vhcurveto " ); break;
	  case 31: printf( "hvcurveto " ); break;
/* Type 1 codes */
	  case 9: printf( "closepath " ); break;
	  case 13: printf( "hsbw " ); break;
/* End obsolete codes */
	  default: printf( "?\?\?-%d-??? ", v );
	}
	--len;
	if ( v==19 || v==20 ) {
	    /* I need to skip as many bits (rounded into bytes) as there are */
	    /*  hints. But figuring out the hint count is impossible in a subr */
	    /*  and difficult in a glyph that calls subrs */
	    /* .... So we pretend the hintmask fits into one byte */
	    printf( "0x%02x ", *str++ );
	    --len;
	}
    } while ( len>0 );
    printf( "\n" );
}

static void readcffsubrs(FILE *ttf,struct topdicts *dict,struct pschars *subs,
	int type, const char *label) {
    uint16_t count = getushort(ttf);
    int offsize;
    uint32_t *offsets;
    int i,j;
    const char *text[] = { "char strings", "subrs", NULL };
    uint8_t *temp;

    printf( "\nThere are %d %s in the index associated with %s\n",
	    count, text[type], label );
    memset(subs,'\0',sizeof(struct pschars));
    if ( count==0 )
return;
    subs->cnt = count;
    /*subs->lens = malloc(count*sizeof(int));*/
    /*subs->values = malloc(count*sizeof(uint8_t *));*/
    subs->bias = dict->charstringtype==1 ? 0 :
	    count<1240 ? 107 :
	    count<33900 ? 1131 : 32768;
    if ( type==1 )
	printf( " Bias = %d\n", subs->bias );
    offsets = malloc((count+1)*sizeof(uint32_t));
    offsize = getc(ttf);
    printf( " Subr Index Offset Size: %d\n Offsets: ", offsize );
    for ( i=0; i<=count; ++i ) {
	offsets[i] = getoffset(ttf,offsize);
	if ( i==0 && offsets[0]!=1 )
	    fprintf( stderr, "!!! Initial offset must be 1 in %s in %s\n", text[type], label);
	else if ( i!=0 && offsets[i]<offsets[i-1] )
	    fprintf( stderr, "!!! bad length for %d, %d in %s in %s\n",
		    i-1, (int)((offsets[i]-offsets[i-1])), text[type], label);
	printf( "%d ", (int)(offsets[i]) );
    }
    putchar('\n');
    for ( i=0; i<count; ++i ) {
	/*subs->lens[i] = offsets[i+1]-offsets[i];*/
	/*subs->values[i] = malloc(offsets[i+1]-offsets[i]+1);*/
	temp = malloc(offsets[i+1]-offsets[i]+1);
	for ( j=0; j<offsets[i+1]-offsets[i]; ++j )
	    temp[j] = getc(ttf);
	temp[j] = '\0';
	printf( "  %s %d: ", text[type], i );
	ShowCharString(temp,offsets[i+1]-offsets[i],dict->charstringtype);
	free(temp);
    }
    free(offsets);
}

static struct topdicts *readcfftopdict(FILE *ttf, char *fontname, int len) {
    struct topdicts *td = calloc(1,sizeof(struct topdicts));
    long base = ftell(ttf);
    int ival, oval, sp, ret, i;
    double stack[50];

    td->fontname = fontname;
    td->underlinepos = -100;
    td->underlinewidth = 50;
    td->charstringtype = 2;
    td->fontmatrix[0] = td->fontmatrix[3] = .001;
    td->cidcount = 8720;

    td->notice = td->copyright = td->fullname = td->familyname = td->weight = -1;
    td->postscript_code = td->basefontname = -1;
    td->synthetic_base = -1;
    td->ros_registry = td->ros_ordering = td->fdarrayoff = td->fdselectoff = -1;
    td->sid_fontname = -1;

    if ( fontname!=NULL ) printf( " Top Dict for %s\n", fontname );
    while ( ftell(ttf)<base+len ) {
	sp = 0;
	while ( (ret=readcffthing(ttf,&ival,&stack[sp],&oval))!=3 && ftell(ttf)<base+len ) {
	    if ( sp==0 ) putchar(' ');
	    if ( ret==1 ) printf( " %d", ival );
	    else printf( " %g", stack[sp] );
	    if ( ret==1 )
		stack[sp]=ival;
	    if ( ret!=0 && sp<45 )
		++sp;
	}
	if ( sp==0 )
	    fprintf( stderr, "No argument to operator\n" );
	else if ( ret==3 ) switch( oval ) {
	  case 0:
	    printf( " Version\n" );
	    td->version = stack[sp-1];
	  break;
	  case 1:
	    printf( " notice\n" );
	    td->notice = stack[sp-1];
	  break;
	  case (12<<8)+0:
	    printf( " copyright\n" );
	    td->copyright = stack[sp-1];
	  break;
	  case 2:
	    printf( " fullname\n" );
	    td->fullname = stack[sp-1];
	  break;
	  case 3:
	    printf( " familyname\n" );
	    td->familyname = stack[sp-1];
	  break;
	  case 4:
	    printf( " weight\n" );
	    td->weight = stack[sp-1];
	  break;
	  case (12<<8)+1:
	    printf( " isfixedpitch\n" );
	    td->isfixedpitch = stack[sp-1];
	  break;
	  case (12<<8)+2:
	    printf( " italicangle\n" );
	    td->italicangle = stack[sp-1];
	  break;
	  case (12<<8)+3:
	    printf( " underlinepos\n" );
	    td->underlinepos = stack[sp-1];
	  break;
	  case (12<<8)+4:
	    printf( " underlinewidth\n" );
	    td->underlinewidth = stack[sp-1];
	  break;
	  case (12<<8)+5:
	    printf( " painttype\n" );
	    td->painttype = stack[sp-1];
	  break;
	  case (12<<8)+6:
	    printf( " charstringtype\n" );
	    td->charstringtype = stack[sp-1];
	  break;
	  case (12<<8)+7:
	    printf( " fontmatrix\n" );
	    memcpy(td->fontmatrix,stack,(sp>=6?6:sp)*sizeof(double));
	  break;
	  case 13:
	    printf( " uniqueid\n" );
	    td->uniqueid = stack[sp-1];
	  break;
	  case 5:
	    printf( " fontbb\n" );
	    memcpy(td->fontbb,stack,(sp>=4?4:sp)*sizeof(double));
	  break;
	  case (12<<8)+8:
	    printf( " strokewidth\n" );
	    td->strokewidth = stack[sp-1];
	  break;
	  case 14:
	    printf( " xuid\n" );
	    for ( i=0; i<sp && i<20; ++i )
		td->xuid[i] = stack[i];
	  break;
	  case 15:
	    printf( " charsetoff\n" );
	    td->charsetoff = stack[sp-1];
	  break;
	  case 16:
	    printf( " encodingoff\n" );
	    td->encodingoff = stack[sp-1];
	  break;
	  case 17:
	    printf( " charstringsoff\n" );
	    td->charstringsoff = stack[sp-1];
	  break;
	  case 18:
	    printf( " private\n" );
	    td->private_size = stack[0];
	    td->private_offset = stack[1];
	  break;
	  case (12<<8)+20:
	    printf( " synthetic_base\n" );
	    td->synthetic_base = stack[sp-1];
	  break;
	  case (12<<8)+21:
	    printf( " postscript_code\n" );
	    td->postscript_code = stack[sp-1];
	  break;
	  case (12<<8)+22:
	    printf( " basefontname\n" );
	    td->basefontname = stack[sp-1];
	  break;
	  case (12<<8)+23:
	    printf( " basefontblend\n" );
	    for ( i=0; i<sp && i<16; ++i )
		td->basefontblend[i] = stack[i];
	  break;
	  case (12<<8)+30:
	    printf( " ROS\n" );
	    td->ros_registry = stack[0];
	    td->ros_ordering = stack[1];
	    td->ros_supplement = stack[2];
	  break;
	  case (12<<8)+31:
	    printf( " CIDFontVersion\n" );
	    td->cidfontversion = stack[sp-1];
	  break;
	  case (12<<8)+32:
	    printf( " CIDFontRevision\n" );
	    td->cidfontrevision = stack[sp-1];
	  break;
	  case (12<<8)+33:
	    printf( " CIDFontType\n" );
	    td->cidfonttype = stack[sp-1];
	  break;
	  case (12<<8)+34:
	    printf( " CIDCount\n" );
	    td->cidcount = stack[sp-1];
	  break;
	  case (12<<8)+35:
	    printf( " UIDBase\n" );
	    td->uidbase = stack[sp-1];
	  break;
	  case (12<<8)+36:
	    printf( " FDArray Off\n" );
	    td->fdarrayoff = stack[sp-1];
	  break;
	  case (12<<8)+37:
	    printf( " FDSelect Off\n" );
	    td->fdselectoff = stack[sp-1];
	  break;
	  case (12<<8)+38:
	    printf( " Fontname\n" );
	    td->sid_fontname = stack[sp-1];
	  break;
	  default:
	    fprintf(stderr,"Unknown operator in %s: %x\n", fontname, (unsigned int)(oval) );
	  break;
	}
    }
return( td );
}

static void dumpsid(const char *label, int sid, char **strings, int smax ) {
    if ( sid==-1 )
return;

    if ( sid<nStdStrings )
	printf( "%s%d %s\n", label, sid, cffnames[sid]);
    else if ( sid<nStdStrings+smax )
	printf( "%s%d %s\n", label, sid, strings[sid-nStdStrings]);
    else
	printf( "%s%d >> Bad SID (max=%d) <<\n", label, sid, smax );
}

static char *getsid(int sid, char **strings,int smax ) {
    if ( sid==-1 )
return(NULL);

    if ( sid<nStdStrings )
return( strdup(cffnames[sid]));
    else if ( sid<nStdStrings+smax )
return( strdup(strings[sid-nStdStrings]));
    else
return( strdup(">> Bad SID <<"));
}

static void readcffprivate(FILE *ttf, struct topdicts *td, char **strings, int smax) {
    int ival, oval, sp, ret, i;
    double stack[50];
    int32_t end = td->cff_start+td->private_offset+td->private_size;
    char *name = NULL;

    char *nameless_str = "<Nameless>";

    fseek(ttf,td->cff_start+td->private_offset,SEEK_SET);

    td->subrsoff = -1;
    td->expansionfactor = .06;
    td->bluefuzz = 1;
    td->blueshift = 7;
    td->bluescale = .039625;

    name = td->fontname?td->fontname:
		td->sid_fontname?getsid(td->sid_fontname,strings,smax):nameless_str;

    printf( "\n Private Dict for %s\n", name );
    while ( ftell(ttf)<end ) {
	sp = 0;
	while ( (ret=readcffthing(ttf,&ival,&stack[sp],&oval))!=3 && ftell(ttf)<end ) {
	    if ( sp==0 ) putchar(' ');
	    if ( ret==1 ) printf( " %d", ival );
	    else printf( " %g", stack[sp] );
	    if ( ret==1 )
		stack[sp]=ival;
	    if ( ret!=0 && sp<45 )
		++sp;
	}
	if ( sp==0 )
	    fprintf( stderr, "No argument to operator\n" );
	else if ( ret==3 ) switch( oval ) {
	  case 6:
	    printf( " BlueValues\n" );
	    for ( i=0; i<sp && i<14; ++i ) {
		td->bluevalues[i] = stack[i];
		if ( i!=0 )
		    td->bluevalues[i] += td->bluevalues[i-1];
	    }
	  break;
	  case 7:
	    printf( " OtherBlues\n" );
	    for ( i=0; i<sp && i<10; ++i ) {
		td->otherblues[i] = stack[i];
		if ( i!=0 )
		    td->otherblues[i] += td->otherblues[i-1];
	    }
	  break;
	  case 8:
	    printf( " FamilyBlues\n" );
	    for ( i=0; i<sp && i<14; ++i ) {
		td->familyblues[i] = stack[i];
		if ( i!=0 )
		    td->familyblues[i] += td->familyblues[i-1];
	    }
	  break;
	  case 9:
	    printf( " FamilyOtherBlues\n" );
	    for ( i=0; i<sp && i<10; ++i ) {
		td->familyotherblues[i] = stack[i];
		if ( i!=0 )
		    td->familyotherblues[i] += td->familyotherblues[i-1];
	    }
	  break;
	  case (12<<8)+9:
	    printf( " BlueScale\n" );
	    td->bluescale = stack[sp-1];
	  break;
	  case (12<<8)+10:
	    printf( " BlueShift\n" );
	    td->blueshift = stack[sp-1];
	  break;
	  case (12<<8)+11:
	    printf( " BlueFuzz\n" );
	    td->bluefuzz = stack[sp-1];
	  break;
	  case 10:
	    printf( " StdHW\n" );
	    td->stdhw = stack[sp-1];
	  break;
	  case 11:
	    printf( " StdVW\n" );
	    td->stdvw = stack[sp-1];
	  break;
	  case (12<<8)+12:
	    printf( " StemSnapH\n" );
	    for ( i=0; i<sp && i<10; ++i ) {
		td->stemsnaph[i] = stack[i];
		if ( i!=0 )
		    td->stemsnaph[i] += td->stemsnaph[i-1];
	    }
	  break;
	  case (12<<8)+13:
	    printf( " StemSnapV\n" );
	    for ( i=0; i<sp && i<10; ++i ) {
		td->stemsnapv[i] = stack[i];
		if ( i!=0 )
		    td->stemsnapv[i] += td->stemsnapv[i-1];
	    }
	  break;
	  case (12<<8)+14:
	    printf( " ForceBold\n" );
	    td->forcebold = stack[sp-1];
	  break;
	  case (12<<8)+17:
	    printf( " LanguageGroup\n" );
	    td->languagegroup = stack[sp-1];
	  break;
	  case (12<<8)+18:
	    printf( " ExpansionFactor\n" );
	    td->expansionfactor = stack[sp-1];
	  break;
	  case (12<<8)+19:
	    printf( " InitialRandomSeed\n" );
	    td->initialRandomSeed = stack[sp-1];
	  break;
	  case 19:
	    printf( " Subrs\n" );
	    td->subrsoff = stack[sp-1];
	  break;
	  case 20:
	    printf( " DefaultWidthX\n" );
	    td->defaultwidthx = stack[sp-1];
	  break;
	  case 21:
	    printf( " NominalWidthX\n" );
	    td->nominalwidthx = stack[sp-1];
	  break;
	  default:
	    fprintf(stderr,"Unknown operator in %s: %x\n", td->fontname, (unsigned int)(oval) );
	  break;
	}
    }

    if ( td->subrsoff!=-1 ) {
	fseek(ttf,td->cff_start+td->private_offset+td->subrsoff,SEEK_SET);
	readcffsubrs(ttf,td,&td->local_subrs, 1, name );
    }
    if (name != nameless_str) free(name);
}

static struct topdicts **readcfftopdicts(FILE *ttf, char **fontnames, int cff_start) {
    uint16_t count = getushort(ttf);
    int offsize;
    uint32_t *offsets;
    struct topdicts **dicts;
    int i;

    if ( fontnames!=NULL )
	printf( "There %s %d top dictionar%s in this cff\n", count==1?"is":"are",count, count==1?"y":"ies" );
    else
	printf( "There %s %d subdictionary dictionar%s in this font\n", count==1?"is":"are",count, count==1?"y":"ies" );
    if ( count==0 )
return( NULL );
    offsets = malloc((count+1)*sizeof(uint32_t));
    offsize = getc(ttf);
    printf( " %s Dict Index Offset Size: %d\n Offsets: ",
	    fontnames!=NULL?"Top":"Sub", offsize );
    for ( i=0; i<=count; ++i ) {
	offsets[i] = getoffset(ttf,offsize);
	if ( i==0 && offsets[0]!=1 )
	    fprintf(stderr, "!! Initial offset must be one in Top Dict Index\n" );
	printf( "%d ", (int)(offsets[i]) );
    }
    putchar('\n');
    dicts = malloc((count+1)*sizeof(struct topdicts *));
    for ( i=0; i<count; ++i ) {
	dicts[i] = readcfftopdict(ttf,fontnames!=NULL?fontnames[i]:NULL,
		offsets[i+1]-offsets[i]);
	dicts[i]->cff_start = cff_start;
    }
    dicts[i] = NULL;
    free(offsets);
return( dicts );
}

static void showdict(struct topdicts *dict, char **strings, int smax) {
    int i, j;

    if ( dict->sid_fontname==-1 )
	printf( "\nDump of top dictionary for %s\n", dict->fontname);
    else
	dumpsid("\nDump of sub dictionary ", dict->sid_fontname, strings, smax );
    dumpsid(" Version=", dict->version, strings, smax );
    dumpsid(" Notice=", dict->notice, strings, smax );
    dumpsid(" copyright=", dict->copyright, strings, smax );
    dumpsid(" fullname=", dict->fullname, strings, smax );
    dumpsid(" familyname=", dict->familyname, strings, smax );
    dumpsid(" weight=", dict->weight, strings, smax );
    printf( " fixedpitch=%d\n", dict->isfixedpitch);
    printf( " ItalicAngle=%g\n", dict->italicangle);
    printf( " underlinepos=%g\n", dict->underlinepos);
    printf( " underlinewidth=%g\n", dict->underlinewidth);
    printf( " painttype=%d\n", dict->painttype);
    printf( " charstringtype=%d\n", dict->charstringtype);
    printf( " fontmatrix=[%g %g %g %g %g %g]\n",
	    dict->fontmatrix[0], dict->fontmatrix[1],
	    dict->fontmatrix[2], dict->fontmatrix[3],
	    dict->fontmatrix[4], dict->fontmatrix[5]);
    printf( " uniqueid=%d\n", dict->uniqueid);
    printf( " fontbb=[%g %g %g %g]\n",
	    dict->fontbb[0], dict->fontbb[1],
	    dict->fontbb[2], dict->fontbb[3]);
    printf( " strokewidth=%g\n", dict->strokewidth);
    for ( i=19; i>=0; --i )
	if ( dict->xuid[i]!=0 )
    break;
    if ( i>=0 ) {
	printf( " XUID=[" );
	for ( j=0; j<=i; ++j )
	    printf("%d ", dict->xuid[j]);
	printf("]\n");
    }
    printf( " charsetoff=%d\n", dict->charsetoff);
    printf( " encodingoff=%d\n", dict->encodingoff);
    printf( " charstringsoff=%d\n", dict->charstringsoff);
    printf( " private size=%d off=%d\n", dict->private_size, dict->private_offset);
    if ( dict->synthetic_base!=-1 )
	printf( " synthetic_base=%d\n", dict->synthetic_base );
    dumpsid(" postscript_code=", dict->postscript_code, strings, smax );
    dumpsid(" basefontname=", dict->basefontname, strings, smax );
    for ( i=15; i>=0; --i )
	if ( dict->basefontblend[i]!=0 )
    break;
    if ( i>=0 ) {
	printf( " basefontblend=[" );
	for ( j=0; j<=i; ++j )
	    printf("%d ", dict->basefontblend[i]);
	printf("]\n");
    }
    if ( dict->ros_registry!=-1 ) {
	dumpsid(" ros (Registry)=", dict->ros_registry, strings, smax );
	dumpsid("     (Ordering)=", dict->ros_ordering, strings, smax );
	printf("     (Supplement)=%d\n", dict->ros_supplement );
	printf( " cidfontversion=%d\n", dict->cidfontversion);
	printf( " cidfontrevision=%d\n", dict->cidfontrevision);
	printf( " cidfonttype=%d\n", dict->cidfonttype);
	printf( " cidcount=%d\n", dict->cidcount);
	printf( " uidbase=%d\n", dict->uidbase);
	printf( " FDArray off=%d\n", dict->fdarrayoff );
    }
    if ( dict->fdselectoff!=-1 )
	printf( " FDSelect off=%d\n", dict->fdselectoff );
    printf( "\n" );
}

/* The real encoding is done in the cmap ttf table, not sure why we bother here */
static void readcffenc(FILE *ttf,struct topdicts *dict,char **strings,int smax) {
    int format, cnt, i;

    if ( dict->encodingoff==0 ) {
	printf( "\nAdobe Standard Encoding\n" );
return;
    } else if ( dict->encodingoff==1 ) {
	printf( "\nExpert Encoding\n" );
return;
    }
    fseek(ttf,dict->cff_start+dict->encodingoff,SEEK_SET);
    format = getc(ttf);
    printf( "\nCFF Encoding format=%x\n", (unsigned int)(format) );
    if ( (format&0x7f)==0 ) {
	cnt = getc(ttf);
	printf( " Enc cnt=%d\n Enc: ", cnt );
	for ( i=0; i<cnt; ++i )
	    printf( "%02x ", (unsigned int)(getc(ttf)) );
	printf("\n");
    } else if ( (format&0x7f)==1 ) {
	cnt = getc(ttf);
	printf( " Enc range cnt=%d\n", cnt );
	for ( i=0; i<cnt; ++i ) {
	    printf( "  Enc Range %d: First=%02x ", i, (unsigned int)(getc(ttf)) );
	    printf( "nLeft=%d\n", getc(ttf));
	}
    }
    if ( format&0x80 ) {
	cnt = getc(ttf);
	printf( " Supplemental entries[%d]\n", cnt );
	for ( i=0; i<cnt; ++i ) {
	    printf( "  Supplement[%d]: Encoding %d -> ", i, getc(ttf));
	    dumpsid("",getushort(ttf),strings, smax);
	}
    }
}

static void readcffset(FILE *ttf,struct topdicts *dict,char **strings,int smax,
	struct ttfinfo *info) {
    int len = dict->glyphs.cnt;
    int i=0;
    int format, cnt, j, first;

    if ( dict->charsetoff==0 ) {
	/* ISO Adobe charset */
	printf( "\nISOAdobe charset\n" );
	dict->charset = malloc(len*sizeof(uint16_t));
	for ( i=0; i<len && i<=228; ++i )
	    dict->charset[i] = i;
    } else if ( dict->charsetoff==1 ) {
	printf( "\nExpert charset\n" );
	/* Expert charset */
	dict->charset = malloc((len<162?162:len)*sizeof(uint16_t));
	dict->charset[0] = 0;		/* .notdef */
	dict->charset[1] = 1;
	for ( i=2; i<len && i<=238-227; ++i )
	    dict->charset[i] = i+227;
	dict->charset[12] = 13;
	dict->charset[13] = 14;
	dict->charset[14] = 15;
	dict->charset[15] = 99;
	for ( i=16; i<len && i<=248-223; ++i )
	    dict->charset[i] = i+223;
	dict->charset[25] = 27;
	dict->charset[26] = 28;
	for ( i=27; i<len && i<=266-222; ++i )
	    dict->charset[i] = i+222;
	dict->charset[44] = 109;
	dict->charset[45] = 110;
	for ( i=46; i<len && i<=318-221; ++i )
	    dict->charset[i] = i+221;
	dict->charset[96] = 158;
	dict->charset[97] = 155;
	dict->charset[98] = 163;
	for ( i=99; i<len && i<=326-220; ++i )
	    dict->charset[i] = i+220;
	dict->charset[107] = 150;
	dict->charset[108] = 164;
	dict->charset[109] = 169;
	for ( i=110; i<len && i<=378-217; ++i )
	    dict->charset[i] = i+217;
    } else if ( dict->charsetoff==2 ) {
	printf( "\nExpert subset charset\n" );
	/* Expert subset charset */
	dict->charset = malloc((len<130?130:len)*sizeof(uint16_t));
	dict->charset[0] = 0;		/* .notdef */
	dict->charset[1] = 1;
	for ( i=2; i<len && i<=238-227; ++i )
	    dict->charset[i] = i+227;
	dict->charset[12] = 13;
	dict->charset[13] = 14;
	dict->charset[14] = 15;
	dict->charset[15] = 99;
	for ( i=16; i<len && i<=248-223; ++i )
	    dict->charset[i] = i+223;
	dict->charset[25] = 27;
	dict->charset[26] = 28;
	for ( i=27; i<len && i<=266-222; ++i )
	    dict->charset[i] = i+222;
	dict->charset[44] = 109;
	dict->charset[45] = 110;
	for ( i=46; i<len && i<=272-221; ++i )
	    dict->charset[i] = i+221;
	dict->charset[51] = 300;
	dict->charset[52] = 301;
	dict->charset[53] = 302;
	dict->charset[54] = 305;
	dict->charset[55] = 314;
	dict->charset[56] = 315;
	dict->charset[57] = 158;
	dict->charset[58] = 155;
	dict->charset[59] = 163;
	for ( i=60; i<len && i<=326-260; ++i )
	    dict->charset[i] = i+260;
	dict->charset[67] = 150;
	dict->charset[68] = 164;
	dict->charset[69] = 169;
	for ( i=110; i<len && i<=346-217; ++i )
	    dict->charset[i] = i+217;
    } else {
	dict->charset = malloc(len*sizeof(uint16_t));
	dict->charset[0] = 0;		/* .notdef */
	fseek(ttf,dict->cff_start+dict->charsetoff,SEEK_SET);
	format = getc(ttf);
	printf( "\nCharset format=%d\n", format );
	if ( format==0 ) {
	    for ( i=1; i<len; ++i )
		dict->charset[i] = getushort(ttf);
	} else if ( format==1 ) {
	    for ( i = 1; i<len; ) {
		first = dict->charset[i++] = getushort(ttf);
		cnt = getc(ttf);
		for ( j=0; j<cnt; ++j )
		    dict->charset[i++] = ++first;
	    }
	} else if ( format==2 ) {
	    for ( i = 1; i<len; ) {
		first = dict->charset[i++] = getushort(ttf);
		cnt = getushort(ttf);
		for ( j=0; j<cnt; ++j )
		    dict->charset[i++] = ++first;
	    }
	}
    }
    while ( i<len ) dict->charset[i++] = 0;
    printf( " Which means...\n" );
    if ( dict->ros_registry==-1 ) {
	info->glyph_names = calloc(len,sizeof(char *));
	for ( i=0; i<len; ++i ) {
	    printf( "Glyph %d is named ", i );
	    dumpsid("", dict->charset[i], strings, smax);
	    info->glyph_names[i] = getsid(dict->charset[i], strings,smax);
	}
    } else
	for ( i=0; i<len; ++i )
	    printf( "Glyph %d -> CID %d\n", i, dict->charset[i]);
}

static int readcff(FILE *ttf,FILE *util, struct ttfinfo *info) {
    int offsize;
    int hdrsize;
    char **fontnames;
    char **strings;
    struct topdicts **dicts, **subdicts;
    int i, j, smax;
    struct pschars gsubs;

    fseek(ttf,info->cff_start,SEEK_SET);
    printf( "\nPostscript CFF table (at %d for %d bytes)\n", info->cff_start, info->cff_length);
    printf( "\tMajor Version: %d\n", getc(ttf));
    printf( "\tMinor Version: %d\n", getc(ttf));
    printf( "\tTable Header size: %d\n", hdrsize = getc(ttf));
    printf( "\tOffset size: %d\n", offsize = getc(ttf));	/* Er is this ever used? */
    if ( hdrsize!=4 )
	fseek(ttf,info->cff_start+hdrsize,SEEK_SET);
    fontnames = readcfffontnames(ttf,0);
    dicts = readcfftopdicts(ttf,fontnames,info->cff_start);
	/* String index is just the same as fontname index */
    strings = readcfffontnames(ttf,1);
    for ( smax=0; strings[smax]!=NULL; ++smax );
    for ( i=0; fontnames[i]!=NULL; ++i )
	showdict(dicts[i],strings,smax);
    readcffsubrs(ttf,dicts[0],&gsubs, 1, "Global" );
    for ( i=0; fontnames[i]!=NULL; ++i ) {
	if ( dicts[i]->charstringsoff!=-1 ) {
	    fseek(ttf,info->cff_start+dicts[i]->charstringsoff,SEEK_SET);
	    readcffsubrs(ttf,dicts[i],&dicts[i]->glyphs, 0, fontnames[i]);
	}
	if ( dicts[i]->private_offset!=-1 )
	    readcffprivate(ttf,dicts[i],strings,smax);
	if ( dicts[i]->charsetoff!=-1 )
	    readcffset(ttf,dicts[i],strings,smax,info);
	if ( dicts[i]->ros_registry==-1 )
	    readcffenc(ttf,dicts[i],strings,smax);
	if ( dicts[i]->fdarrayoff!=-1 ) {
	    fseek(ttf,info->cff_start+dicts[i]->fdarrayoff,SEEK_SET);
	    subdicts = readcfftopdicts(ttf,NULL,info->cff_start);
	    for ( j=0; subdicts[j]!=NULL; ++j ) {
		showdict(subdicts[j],strings,smax);
		if ( subdicts[j]->private_offset!=-1 )
		    readcffprivate(ttf,subdicts[j],strings,smax);
	    }
	}
    }
    for ( i = 0; strings[i] != NULL; ++i)
        free(strings[i]);
    free(strings);

    for ( i = 0; dicts[i] != NULL; ++i)
        free(dicts[i]);
    free(dicts);

    for ( i = 0; fontnames[i] != NULL; ++i)
        free(fontnames[i]);
    free(fontnames);
return( 1 );
}

static void readttfBigGlyphMetrics(FILE *ttf,const char *indent) {
    printf( "%sBitmap rows=%d\n", indent, getc(ttf));
    printf( "%sBitmap columns=%d\n", indent, getc(ttf));
    printf( "%shoriBearingX=%d\n", indent, (signed char) getc(ttf));
    printf( "%shoriBearingY=%d\n", indent, (signed char) getc(ttf));
    printf( "%shoriAdvance=%d\n", indent, getc(ttf));
    printf( "%svertBearingX=%d\n", indent, (signed char) getc(ttf));
    printf( "%svertBearingY=%d\n", indent, (signed char) getc(ttf));
    printf( "%svertAdvance=%d\n", indent, getc(ttf));
}

static void ShowGlyph(FILE *ttf,long offset,long len,int imageFormat, struct ttfinfo *info) {
    int h,w,sbX,sbY,advance;
    int i,j,k,ch;
    long here;

    if ( imageFormat!=1 && imageFormat!=2 )
return;
    here = ftell(ttf);
    fseek(ttf,info->bitmapdata_start+offset,SEEK_SET);

    h = getc(ttf);
    w = getc(ttf);
    sbX = (signed char) getc(ttf);
    sbY = (signed char) getc(ttf);
    advance = getc(ttf);
    printf( "\t\theight=%d width=%d sbX=%d sbY=%d advance=%d %s aligned\n",
	    h,w,sbX,sbY,advance,imageFormat==1?"Byte":"Bit");
    len -= 5; ch = 0;
    if ( imageFormat==1 ) {
	/* Byte aligned data */
	for ( i=0; i<h; ++i ) {
	    putchar('\t');
	    for ( j=0; j<(w+7)/8; ++j ) {
		ch = getc(ttf); --len;
		for ( k=0; k<8 && j*8+k<w; ++k ) {
		    if ( ch&(0x80>>k))
			putchar('*');
		    else
			putchar('.');
		}
	    }
	    putchar('\n');
	}
    } else {
	k=8;
	for ( i=0; i<h; ++i ) {
	    putchar('\t');
	    for ( j=0; j<w; ++j ) {
		if ( ++k>=8 ) {
		    ch = getc(ttf); --len;
		    k=0;
		}
		if ( ch&(0x80>>k))
		    putchar('*');
		else
		    putchar('.');
	    }
	    putchar('\n');
	}
    }
    if ( len!=0 )
	printf("!!!\tLength field wrong. %d left\n", (int) len );
    fseek(ttf,here,SEEK_SET);
}

static void readttfIndexSubTab(FILE *ttf,long offset, int first, int last,
	struct ttfinfo *info) {
    int i, indexFormat, imageFormat, bdatOff;
    long curstart, nextstart;

    fseek(ttf,offset,SEEK_SET);
    printf( "\t   index Format=%d ", indexFormat=getushort(ttf));
    if ( indexFormat==1 || indexFormat==3 ) printf( "Proportional\n" );
    else if ( indexFormat==2 ) printf( "Monospace\n" );
    else printf( "Unknown\n" );
    printf( "\t   image Format=%d\n", imageFormat=getushort(ttf));
    printf( "\t   Offset in bitmap data table=%d\n", bdatOff=getlong(ttf));
    if ( indexFormat==1 || indexFormat==3 ) {
	curstart = (indexFormat==1?getlong(ttf):getushort(ttf));
	for ( i=first; i<=last; ++i ) {
	    nextstart = (indexFormat==1?getlong(ttf):getushort(ttf));
	    if ( info->glyph_names==NULL )
		printf( "\t    Glyph %d starts at %5d length=%d\n", i, (int) curstart, (int) (nextstart-curstart) );
	    else if ( i<info->glyph_cnt && info->glyph_names[i]!=NULL )
		printf( "\t    Glyph %d (%10s) starts at %5d length=%d\n", i,
			info->glyph_names[i], (int) curstart, (int) (nextstart-curstart) );
	    else
		printf( "\t    Glyph %d (          ) starts at %5d length=%d\n", i,
			(int) curstart, (int) (nextstart-curstart) );
	    ShowGlyph(ttf,bdatOff+curstart,nextstart-curstart,imageFormat,info);
	    curstart = nextstart;
	}
    } else if ( indexFormat==2 ) {
	printf( "\t   Bitmap Image Size=%d\n", (int) getlong(ttf));
	printf( "\t   big Metrics for any glyph\n" );
	readttfBigGlyphMetrics(ttf,"\t    ");
    }
}

static void readttfIndexSizeSubTab(FILE *ttf,long offset, long size, long num,
	struct ttfinfo *info) {
    int i, first, last;
    long here, moreoff;

    fseek(ttf,offset,SEEK_SET);
    for ( i=0; i<num; ++i ) {
	printf( "\t indexSubTable[%d]\n", i );
	printf( "\t  first glyph=%d\n", first = getushort(ttf));
	printf( "\t  last glyph=%d\n", last = getushort(ttf));
	printf( "\t  additional offset=%d\n", (int) (moreoff = getlong(ttf)));
	here = ftell(ttf);
	readttfIndexSubTab(ttf,offset+moreoff,first,last,info);
	fseek(ttf,here,SEEK_SET);
    }
}

static void sbitLineMetrics(FILE *ttf) {

    printf( "\t  ascender: %d\n", (signed char) getc(ttf));
    printf( "\t  descender: %d\n", (signed char) getc(ttf));
    printf( "\t  widthMax: %d\n", getc(ttf));
    printf( "\t  caretSlopeNumerator: %d\n", (signed char) getc(ttf));
    printf( "\t  caretSlopeDenominator: %d\n", (signed char) getc(ttf));
    printf( "\t  caretOffset: %d\n", (signed char) getc(ttf));
    printf( "\t  minOriginSB: %d\n", (signed char) getc(ttf));
    printf( "\t  minAdvanceSB: %d\n", (signed char) getc(ttf));
    printf( "\t  maxBeforeBL: %d\n", (signed char) getc(ttf));
    printf( "\t  maxAfterBL: %d\n", (signed char) getc(ttf));
    /* padding */ getushort(ttf);
}

static int readttfbitmapscale(FILE *ttf,FILE *util, struct ttfinfo *info) {
    int cnt,i;
    int newx, newy, oldx, oldy;

    fseek(ttf,info->bitmapscale_start,SEEK_SET);
    printf( "\nBitmap scaling data (at %d)\n", info->bitmapscale_start);
    printf( "\tVersion: 0x%08x\n", (unsigned int)(getlong(ttf)) );
    printf( "\tnum Sizes: %d\n", cnt = (int) getlong(ttf));
    for ( i=0; i<cnt; ++i ) {
	printf( " Scaling Info %d\n", i );
	printf( "  Horizontal metrics\n" );
	sbitLineMetrics(ttf);
	printf( "  Vertical metrics\n" );
	sbitLineMetrics(ttf);
	newx = getc(ttf);
	newy = getc(ttf);
	oldx = getc(ttf);
	oldy = getc(ttf);
	printf( "  Scale original bitmap (xppem=%d,yppem=%d) to (xppem=%d,yppem=%d)\n",
	    oldx, oldy, newx, newy );
    }
return( 1 );
}

static int readttfbitmaps(FILE *ttf,FILE *util, struct ttfinfo *info) {
    int cnt,i, num;
    long here, offset, size;

    fseek(ttf,info->bitmaploc_start,SEEK_SET);
    printf( "\nBitmap location data (at %d for %d bytes)\n", info->bitmaploc_start, info->bitmaploc_length);
    printf( "\tVersion: 0x%08x\n", (unsigned int)(getlong(ttf)) );
    printf( "\tnumStrikes: %d\n", (int)((cnt = getlong(ttf))) );
    for ( i=0; i<cnt; ++i ) {
	printf( "\t indexSubTableArrayOffset: %d\n", (int)((offset = getlong(ttf))) );
	printf( "\t indexTableSize: %d\n", (int)((size = getlong(ttf))) );
	printf( "\t numberOfIndexSubTables: %d\n", (int)((num = getlong(ttf))) );
	printf( "\t colorRef: %d\n", (int)(getlong(ttf)) );
	printf( "\t horizontal metrics\n" );
	sbitLineMetrics(ttf);
	printf( "\t vertical metrics\n" );
	sbitLineMetrics(ttf);
	printf( "\t startGlyph: %d\n", getushort(ttf));
	printf( "\t endGlyph: %d\n", getushort(ttf));
	printf( "\t ppemX: %d\n", getc(ttf));
	printf( "\t ppemY: %d\n", getc(ttf));
	printf( "\t bitDepth: %d\n", getc(ttf));
	printf( "\t flags: 0x%x\n\n", (unsigned int)(getc(ttf)) );
	here = ftell(ttf);
	readttfIndexSizeSubTab(ttf,info->bitmaploc_start+offset,size,num,info);
	fseek(ttf,here,SEEK_SET);
    }
return( 1 );
}

static int readttfhdmx(FILE *ttf,FILE *util, struct ttfinfo *info) {
    int cnt,size,i;
    int gid;
    int pos;

    fseek(ttf,info->hdmx_start,SEEK_SET);
    printf( "\nHorizontal device metrics (at %d)\n", info->hdmx_start);
    printf( "\tVersion: 0x%08x\n", (unsigned int)(getushort(ttf)) );
    printf( "\tnum Records: %d\n", cnt = getushort(ttf));
    printf( "\trecord Size: %d\n", size = (int) getlong(ttf));
    pos = ftell(ttf);
    for ( i=0; i<cnt; ++i ) {
	fseek(ttf,pos+i*size,SEEK_SET);
	printf( " Device widths at %dppem\n", getc(ttf));
	printf( " Max Width %d\n", getc(ttf));
	for ( gid=0; gid<info->glyph_cnt; ++gid ) {
	    if ( info->glyph_names!=NULL && info->glyph_names[gid]!=NULL )
		printf("\t\t%s:\t%d\n", info->glyph_names[gid], getc(ttf));
	}
    }
return( 1 );
}

static void PrintDeviceTable(FILE *ttf, uint32_t start) {
    uint32_t here = ftell(ttf);
    int first, last, type;
    signed char *corrections;
    int i,b,c, w;
    int any;

    fseek(ttf,start,SEEK_SET);
    first = getushort(ttf);
    last = getushort(ttf);
    type = getushort(ttf);
    if ( type!=1 && type!=2 && type!=3 )
	fprintf( stderr, "! > Bad device table type: %d (must be 1,2, or 3)\n",
		type );
    if ( first>last )
	fprintf( stderr, "! > Bad device table first>last (first=%d last=%d)\n",
		first, last );
    else {
	c = last-first+1;
	corrections = malloc(c);
	if ( type==1 ) {
	    for ( i=0; i<c; i+=8 ) {
		w = getushort(ttf);
		for ( b=0; b<8 && i+b<c; ++b )
		    corrections[i+b] = ((int16_t) ((w<<(b*2))&0xc000))>>14;
	    }
	} else if ( type==2 ) {
	    for ( i=0; i<c; i+=4 ) {
		w = getushort(ttf);
		for ( b=0; b<4 && i+b<c; ++b )
		    corrections[i+b] = ((int16_t) ((w<<(b*4))&0xf000))>>12;
	    }
	} else {
	    for ( i=0; i<c; ++i )
		corrections[i] = (int8_t) getc(ttf);
	}
	putchar('{');
	any = false;
	for ( i=0; i<c; ++i ) {
	    if ( corrections[i]!=0 ) {
		if ( any ) putchar(' ');
		any = true;
		printf( "%d:%d", first+i, corrections[i]);
	    }
	}
	free( corrections );
	printf( "}[%d-%d sized %d]", first,last, type );
    }
    fseek( ttf, here, SEEK_SET );
}
    
static void PrintMathValueRecord(FILE *ttf, uint32_t start) {
    int val;
    uint32_t devtaboffset;

    val = (short) getushort(ttf);
    devtaboffset = getushort(ttf);
    printf( "%d", val );
    if ( devtaboffset!=0 )
	PrintDeviceTable(ttf,start+devtaboffset);
}

static void readttfmathConstants(FILE *ttf,uint32_t start, struct ttfinfo *info) {
    int i;

    fseek(ttf,start,SEEK_SET);
    printf( "\n MATH Constants sub-table (at %d)\n", (int)(start) );
    for ( i=0; i<4; ++i )
	printf( "\t    Constant %d: %d\n", i, getushort(ttf));
    for ( ; i<4+51; ++i ) {
	printf( "\t    Constant %d: ", i );
	PrintMathValueRecord(ttf,start);
	printf( "\n");
    }
    /* And one final constant */
    printf( "\t    Constant %d: %d\n", i, getushort(ttf));
    printf( "\n");
    if ( feof(ttf) ) {
	fprintf( stderr, "!> Unexpected end of file!\n" );
return;
    }
}

static void readttfmathICTA(FILE *ttf,uint32_t start, struct ttfinfo *info, int is_ic) {
    int coverage, cnt, i;
    uint16_t *glyphs;
    uint32_t here;

    fseek(ttf,start,SEEK_SET);
    if ( is_ic )
	printf( "\n  MATH Italics Correction sub-table (at %d)\n", (int)(start) );
    else
	printf( "\n  MATH Top Accent Attachment sub-table (at %d)\n", (int)(start) );
    printf( "\t   Coverage Offset=%d\n", coverage = getushort(ttf));
    printf( "\t   Count=%d\n", cnt = getushort(ttf));
    if ( feof(ttf) ) {
	fprintf( stderr, "!> Unexpected end of file!\n" );
return;
    }
    here = ftell(ttf);
    glyphs = showCoverageTable(ttf,start+coverage,cnt);
    fseek(ttf,here,SEEK_SET);
    for ( i=0; i<cnt; ++i ) {
	printf( "\t\tGlyph %s(%d): ",
		glyphs[i]>=info->glyph_cnt ? "!!! Bad Glyph !!!" : info->glyph_names==NULL ? "" : info->glyph_names[glyphs[i]],
		glyphs[i]);
	PrintMathValueRecord(ttf,start);
	printf( "\n");
    }
    free(glyphs);
    printf( "\n");
}

static void PrintMathKernTable(FILE *ttf,uint32_t start, struct ttfinfo *info) {
    int cnt, i;
    uint32_t here = ftell(ttf);

    fseek(ttf,start,SEEK_SET);
    cnt = getushort(ttf);
    for ( i=0; i<cnt; ++i ) {
	printf( "\t\t\tHeights %d: ", i );
	PrintMathValueRecord(ttf,start);
	printf( "\n");
    }
    for ( i=0; i<=cnt; ++i ) {
	printf( "\t\t\tKerns %d: ", i );
	PrintMathValueRecord(ttf,start);
	printf( "\n");
    }
    fseek(ttf,here,SEEK_SET);
}

static void readttfmathKern(FILE *ttf,uint32_t start, struct ttfinfo *info) {
    int coverage, cnt, i, j;
    uint16_t *glyphs;
    uint32_t here;
    const char *cornernames[] = { "TopRight:", "TopLeft:", "BottomRight:", "BottomLeft:" };

    fseek(ttf,start,SEEK_SET);
    printf( "\n  MATH Kerning sub-table (at %d)\n", (int)(start) );
    printf( "\t   Coverage Offset=%d\n", coverage = getushort(ttf));
    printf( "\t   Count=%d\n", cnt = getushort(ttf));
    if ( feof(ttf) ) {
	fprintf( stderr, "!> Unexpected end of file!\n" );
return;
    }
    here = ftell(ttf);
    glyphs = showCoverageTable(ttf,start+coverage,cnt);
    fseek(ttf,here,SEEK_SET);
    for ( i=0; i<cnt; ++i ) {
	printf( "\t\tGlyph %s(%d):\n",
		glyphs[i]>=info->glyph_cnt ? "!!! Bad Glyph !!!" : info->glyph_names==NULL ? "" : info->glyph_names[glyphs[i]],
		glyphs[i]);
	for ( j=0; j<4; ++j ) {
	    int offset = getushort(ttf);
	    if ( offset!=0 ) {
		printf( "\t\t %s\n", cornernames[j]);
		PrintMathKernTable(ttf,start+offset,info);
	    }
	}
    }
    free(glyphs);
    printf( "\n");
}

static void readttfmathGlyphInfo(FILE *ttf,uint32_t start, struct ttfinfo *info) {
    uint32_t ic, ta, es, mk;

    fseek(ttf,start,SEEK_SET);
    printf( "\n MATH Glyph Info sub-table (at %d)\n", (int)(start) );
    printf( "\tOffset to Italic Correction: %d\n", (int)((ic = getushort(ttf))) );
    printf( "\tOffset to Top Accent: %d\n", (int)((ta = getushort(ttf))) );
    printf( "\tOffset to Extended Shape: %d\n", (int)((es = getushort(ttf))) );
    printf( "\tOffset to Math Kern: %d\n", (int)((mk = getushort(ttf))) );
    if ( feof(ttf) ) {
	fprintf( stderr, "!> Unexpected end of file!\n" );
return;
    }

    if ( ic!=0 )
	readttfmathICTA(ttf,start+ic,info,1);
    if ( ta!=0 )
	readttfmathICTA(ttf,start+ta,info,0);
    if ( es!=0 ) {
	printf( "\n  MATH Extended Shape sub-table (at %d)\n", (int)((start+es)) );
	free( showCoverageTable(ttf,start+es,-1));
    }
    if ( mk!=0 )
	readttfmathKern(ttf,start+mk,info);
}

static void PrintMathGlyphConstruction(FILE *ttf,uint32_t start, struct ttfinfo *info) {
    uint32_t here = ftell(ttf);
    int offset, variant_cnt, part_cnt;
    int i;

    fseek(ttf,start,SEEK_SET);
    printf( "\t\t  Glyph Assembly Offset=%d\n", offset = getushort(ttf));
    printf( "\t\t  Variant Count=%d\n", variant_cnt = getushort(ttf));
    if ( feof(ttf) ) {
	fprintf( stderr, "!> Unexpected end of file!\n" );
return;
    }
    if ( variant_cnt!=0 ) {
	printf("\t\t  Variants: " );
	for ( i=0; i<variant_cnt; ++i ) {
	    int gid, adv;
	    gid = getushort(ttf);
	    adv = getushort(ttf);
	    printf( " %s:%d",
		    gid>=info->glyph_cnt ? "!!! Bad Glyph !!!" : info->glyph_names==NULL ? "" : info->glyph_names[gid],
		    adv);
	}
	printf( "\n" );
    }
    if ( offset!=0 ) {
	fseek(ttf,start+offset,SEEK_SET);
	printf( "\t\t  Glyph Assembly Italic Correction: " );
	PrintMathValueRecord(ttf,start+offset);
	printf( "\n\t\t  Part Count=%d\n", part_cnt = getushort(ttf));
	for ( i=0; i<part_cnt; ++i ) {
	    int gid = getushort(ttf), flags;
	    printf( "\t\t    %s",
		    gid>=info->glyph_cnt ? "!!! Bad Glyph !!!" : info->glyph_names==NULL ? "" : info->glyph_names[gid]);
	    printf( " start=%d", getushort(ttf));
	    printf( " end=%d", getushort(ttf));
	    printf( " full=%d", getushort(ttf));
	    flags = getushort(ttf);
	    printf( " flags=%04x(%s%s)\n", (unsigned int)(flags), (flags&1)?"Extender":"Required",
		    (flags&0xfffe)?",Unknown flags!!!!":"");
	}
    }
    fseek(ttf,here,SEEK_SET);
}

static void readttfmathVariants(FILE *ttf,uint32_t start, struct ttfinfo *info) {
    int vcoverage, vcnt, hcoverage, hcnt, i, offset;
    uint16_t *vglyphs=NULL, *hglyphs=NULL;
    uint32_t here;

    fseek(ttf,start,SEEK_SET);
    printf( "\n MATH Variants sub-table (at %d)\n", (int)(start) );
    printf( "\t  MinConnectorOverlap=%d\n", getushort(ttf));
    printf( "\t  VCoverage Offset=%d\n", vcoverage = getushort(ttf));
    printf( "\t  HCoverage Offset=%d\n", hcoverage = getushort(ttf));
    printf( "\t  VCount=%d\n", vcnt = getushort(ttf));
    printf( "\t  HCount=%d\n", hcnt = getushort(ttf));
    if ( feof(ttf) ) {
	fprintf( stderr, "!> Unexpected end of file!\n" );
return;
    }
    here = ftell(ttf);
    if ( vcoverage!=0 )
	vglyphs = showCoverageTable(ttf,start+vcoverage,vcnt);
    else {
	vglyphs = NULL;
	if ( vcnt!=0 )
	    fprintf( stderr, "! > Bad coverage table: Offset=NUL, but cnt=%d\n", vcnt );
    }
    if ( hcoverage!=0 )
	hglyphs = showCoverageTable(ttf,start+hcoverage,hcnt);
    else {
	hglyphs = NULL;
	if ( hcnt!=0 )
	    fprintf( stderr, "! > Bad coverage table: Offset=NUL, but cnt=%d\n", hcnt );
    }
    fseek(ttf,here,SEEK_SET);
    for ( i=0; i<vcnt; ++i ) {
	printf( "\t\tV Glyph %s(%d):\n",
		vglyphs[i]>=info->glyph_cnt ? "!!! Bad Glyph !!!" : info->glyph_names==NULL ? "" : info->glyph_names[vglyphs[i]],
		vglyphs[i]);
	offset = getushort(ttf);
	PrintMathGlyphConstruction(ttf,start+offset,info);
    }
    free(vglyphs);
    for ( i=0; i<hcnt; ++i ) {
	printf( "\t\tH Glyph %s(%d):\n",
		hglyphs[i]>=info->glyph_cnt ? "!!! Bad Glyph !!!" : info->glyph_names==NULL ? "" : info->glyph_names[hglyphs[i]],
		hglyphs[i]);
	offset = getushort(ttf);
	PrintMathGlyphConstruction(ttf,start+offset,info);
    }
    free(hglyphs);
    printf( "\n");
}

static int readttfmath(FILE *ttf,FILE *util, struct ttfinfo *info) {
    int version;
    uint32_t constants, glyphinfo, variants;

    fseek(ttf,info->math_start,SEEK_SET);
    printf( "\nMATH table (at %d)\n", info->math_start);
    printf( "\tVersion: 0x%08x\n", (unsigned int)((version = (int) getlong(ttf))) );
    if ( version!=0x00010000 )
	fprintf( stderr, "!> Bad version number for math table.\n" );
    printf( "\tOffset to Constants: %d\n", (int)((constants = getushort(ttf))) );
    printf( "\tOffset to GlyphInfo: %d\n", (int)((glyphinfo = getushort(ttf))) );
    printf( "\tOffset to Variants: %d\n", (int)((variants = getushort(ttf))) );
    if ( feof(ttf) ) {
	fprintf( stderr, "!> Unexpected end of file!\n" );
return( 0 );
    }
    if ( constants!=0 )
	readttfmathConstants(ttf,info->math_start+constants,info);
    else
	fprintf( stderr, "!> NUL Offset not expected for math constant table.\n" );
    if ( glyphinfo!=0 )
	readttfmathGlyphInfo(ttf,info->math_start+glyphinfo,info);
    else
	fprintf( stderr, "!> NUL Offset not expected for math Glyph Info table.\n" );
    if ( variants!=0 )
	readttfmathVariants(ttf,info->math_start+variants,info);
    else
	fprintf( stderr, "!> NUL Offset not expected for math Variants table.\n" );
return( 1 );
}

static void readttfbaseminmax(FILE *ttf,uint32_t offset,struct ttfinfo *info,
	uint32_t script_tag,uint32_t lang_tag) {
    int min, max;
    int j,feat_cnt;

    fseek(ttf,offset,SEEK_SET);
    min = (short) getushort(ttf);
    max = (short) getushort(ttf);
    if ( lang_tag == 0 )
	printf( "\t   min extent=%d  max extent=%d for script '%c%c%c%c'\n",
		min, max,
		(char)((script_tag>>24)&0xff), (char)((script_tag>>16)&0xff),
		(char)((script_tag>>8)&0xff), (char)((script_tag)&0xff) );
    else
	printf( "\t    min extent=%d  max extent=%d for language '%c%c%c%c' in script '%c%c%c%c'\n",
		min, max,
		(char)((lang_tag>>24)&0xff), (char)((lang_tag>>16)&0xff),
		(char)((lang_tag>>8)&0xff), (char)((lang_tag)&0xff),
		(char)((script_tag>>24)&0xff), (char)((script_tag>>16)&0xff),
		(char)((script_tag>>8)&0xff), (char)(script_tag&0xf) );
    feat_cnt = getushort(ttf);
    for ( j=0; j<feat_cnt; ++j ) {
	uint32_t feat_tag = getlong(ttf);
	min = (short) getushort(ttf);
	max = (short) getushort(ttf);
	if ( lang_tag == 0 )
	    printf( "\t    min extent=%d  max extent=%d in feature '%c%c%c%c' of script '%c%c%c%c'\n",
		    min, max,
		    (char)((feat_tag>>24)&0xff), (char)((feat_tag>>16)&0xff),
		    (char)((feat_tag>>8)&0xff), (char)(feat_tag&0xff),
		    (char)((script_tag>>24)&0xff), (char)((script_tag>>16)&0xff),
		    (char)((script_tag>>8)&0xff), (char)(script_tag&0xff) );
	else
	    printf( "\t     min extent=%d  max extent=%d in feature '%c%c%c%c' of language '%c%c%c%c' in script '%c%c%c%c'\n",
		    min, max,
		    (char)((feat_tag>>24)&0xff), (char)((feat_tag>>16)&0xff),
		    (char)((feat_tag>>8)&0xff), (char)(feat_tag&0xff),
		    (char)((lang_tag>>24)&0xff), (char)((lang_tag>>16)&0xff),
		    (char)((lang_tag>>8)&0xff), (char)((lang_tag)&0xff),
		    (char)((script_tag>>24)&0xff), (char)((script_tag>>16)&0xff),
		    (char)((script_tag>>8)&0xff), (char)(script_tag&0xff) );
    }
}

struct tagoff { uint32_t tag; uint32_t offset; };

static int readttfbase(FILE *ttf,FILE *util, struct ttfinfo *info) {
    int version;
    uint32_t axes[2];
    uint32_t basetags, basescripts;
    int basetagcnt, basescriptcnt;
    uint32_t *tags;
    struct tagoff *bs;
    int axis,i,j;

    fseek(ttf,info->base_start,SEEK_SET);
    printf( "\nBASE table (at %d)\n", info->base_start);
    printf( "\tVersion: 0x%08x\n", (unsigned int)((version = getlong(ttf))) );
    if ( version!=0x00010000 )
	fprintf( stderr, "!> Bad version number for BASE table.\n" );
    axes[0] = getushort(ttf);	/* Horizontal */
    axes[1] = getushort(ttf);	/* Vertical */

    for ( axis=0; axis<2; ++axis ) {
	if ( axes[axis]==0 ) {
	    printf("\tNO %s axis data\n", axis==0 ? "Horizontal": "Vertical" );
    continue;
	}
	printf("\t%s axis data\n", axis==0 ? "Horizontal": "Vertical" );
	fseek(ttf,info->base_start+axes[axis],SEEK_SET);
	basetags    = getushort(ttf);
	basescripts = getushort(ttf);
	if ( basetags==0 ) {
	    printf("\t NO %s base tags data\n", axis==0 ? "Horizontal": "Vertical" );
	    basetagcnt = 0;
	    tags = NULL;
	} else {
	    fseek(ttf,info->base_start+axes[axis]+basetags,SEEK_SET);
	    basetagcnt = getushort(ttf);
	    tags = calloc(basetagcnt,sizeof(uint32_t));
	    for ( i=0; i<basetagcnt; ++i )
		tags[i] = getlong(ttf);
	    printf( "\t %d Baseline tags for %s\n", basetagcnt, axis==0 ? "Horizontal": "Vertical" );
	    for ( i=0; i<basetagcnt; ++i )
		printf( "\t  '%c%c%c%c'\n",
			(char)((tags[i]>>24)&0xff), (char)((tags[i]>>16)&0xff),
			(char)((tags[i]>>8)&0xff), (char)(tags[i]&0xff) );
	}
	if ( basescripts==0 )
	    printf("\t NO %s base script data\n", axis==0 ? "Horizontal": "Vertical" );
	else {
	    fseek(ttf,info->base_start+axes[axis]+basescripts,SEEK_SET);
	    basescriptcnt = getushort(ttf);
	    bs = calloc(basescriptcnt,sizeof(struct tagoff));
	    for ( i=0; i<basescriptcnt; ++i ) {
		bs[i].tag    = getlong(ttf);
		bs[i].offset = getushort(ttf);
		if ( bs[i].offset != 0 )
		    bs[i].offset += info->base_start+axes[axis]+basescripts;
	    }
	    for ( i=0; i<basescriptcnt; ++i ) if ( bs[i].offset!=0 ) {
		int basevalues, defminmax;
		int langsyscnt;
		struct tagoff *ls;
		printf("\t %s baseline data for '%c%c%c%c' script\n",
			axis==0 ? "Horizontal": "Vertical",
			(char)((bs[i].tag>>24)&0xff), (char)((bs[i].tag>>16)&0xff),
			(char)((bs[i].tag>>8)&0xff), (char)(bs[i].tag&0xff) );
		fseek(ttf,bs[i].offset,SEEK_SET);
		basevalues = getushort(ttf);
		defminmax  = getushort(ttf);
		langsyscnt = getushort(ttf);
		ls = calloc(langsyscnt,sizeof(struct tagoff));
		for ( j=0; j<langsyscnt; ++j ) {
		    ls[j].tag    = getlong(ttf);
		    ls[j].offset = getushort(ttf);
		}
		if ( basevalues==0 )
		    printf("\t  No %s baseline positions for '%c%c%c%c' script\n",
			    axis==0 ? "Horizontal": "Vertical",
			    (char)((bs[i].tag>>24)&0xff), (char)((bs[i].tag>>16)&0xff),
			    (char)((bs[i].tag>>8)&0xff), (char)(bs[i].tag&0xff) );
		else {
		    int defbl, coordcnt;
		    int *coords;

		    fseek( ttf,bs[i].offset+basevalues,SEEK_SET);
		    defbl = getushort(ttf);
		    coordcnt = getushort(ttf);
		    printf("\t  The default baseline for '%c%c%c%c' script is '%c%c%c%c'\n",
			    (char)((bs[i].tag>>24)&0xff), (char)((bs[i].tag>>16)&0xff),
			    (char)((bs[i].tag>>8)&0xff), (char)(bs[i].tag&0xff),
			    (char)((tags[defbl]>>24)&0xff), (char)((tags[defbl]>>16)&0xff),
			    (char)((tags[defbl]>>8)&0xff), (char)(tags[defbl]&0xff) );
		    if ( coordcnt!=basetagcnt )
			fprintf( stderr, "!!!!! Coord count (%d) for '%c%c%c%c' script does not match base tag count (%d) in 'BASE' table\n",
				coordcnt,
				(char)((bs[i].tag>>24)&0xff), (char)((bs[i].tag>>16)&0xff),
				(char)((bs[i].tag>>8)&0xff), (char)(bs[i].tag&0xff),
				basetagcnt );
		    coords = calloc(coordcnt,sizeof(int));
		    for ( j=0; j<coordcnt; ++j )
			coords[j] = getushort(ttf);
		    for ( j=0; j<coordcnt; ++j ) if ( coords[j]!=0 ) {
			int format, coord, gid, pt;
			fseek( ttf,bs[i].offset+basevalues+coords[j],SEEK_SET);
			format = getushort(ttf);
			coord  = (short) getushort(ttf);
			if ( format==3 ) {
			    /* devtab_off = */ getushort(ttf);
			    printf("\t   Baseline '%c%c%c%c' in script '%c%c%c%c' is at %d, with device table\n",
				    (char)((tags[j]>>24)&0xff), (char)((tags[j]>>16)&0xff),
				    (char)((tags[j]>>8)&0xff), (char)(tags[j]&0xff),
				    (char)((bs[j].tag>>24)&0xff), (char)((bs[j].tag>>16)&0xff),
				    (char)((bs[j].tag>>8)&0xff), (char)(bs[j].tag&0xff),
			            coord );
			} else if ( format==2 ) {
			    gid = getushort(ttf);
			    pt  = getushort(ttf);
			    printf("\t   Baseline '%c%c%c%c' in script '%c%c%c%c' is at %d, modified by point %d in glyph %d\n",
				    (char)((tags[j]>>24)&0xff), (char)((tags[j]>>16)&0xff),
				    (char)((tags[j]>>8)&0xff), (char)(tags[j]&0xff),
				    (char)((bs[j].tag>>24)&0xff), (char)((bs[j].tag>>16)&0xff),
				    (char)((bs[j].tag>>8)&0xff), (char)(bs[j].tag&0xff),
			            coord, pt, gid );
			} else {
			    printf("\t   Baseline '%c%c%c%c' in script '%c%c%c%c' is at %d\n",
				    (char)((tags[j]>>24)&0xff), (char)((tags[j]>>16)&0xff),
				    (char)((tags[j]>>8)&0xff), (char)(tags[j]&0xff),
				    (char)((bs[j].tag>>24)&0xff), (char)((bs[j].tag>>16)&0xff),
				    (char)((bs[j].tag>>8)&0xff), (char)(bs[j].tag&0xff),
			            coord );
			    if ( format!=1 )
				fprintf( stderr, "!!!!! Bad Base Coord format (%d) for '%c%c%c%c' in '%c%c%c%c' script in 'BASE' table\n",
					format,
					(char)((tags[j]>>24)&0xff), (char)((tags[j]>>16)&0xff),
					(char)((tags[j]>>8)&0xff), (char)(tags[j]&0xff),
					(char)((bs[j].tag>>24)&0xff), (char)((bs[j].tag>>16)&0xff),
					(char)((bs[j].tag>>8)&0xff), (char)(bs[j].tag&0xff) );
			}
		    }
		    free(coords);
		}
		if ( defminmax==0 )
		    printf("\t  No %s min/max extents for '%c%c%c%c' script\n",
			    axis==0 ? "Horizontal": "Vertical",
			    (char)((bs[i].tag>>24)&0xff), (char)((bs[i].tag>>16)&0xff),
			    (char)((bs[i].tag>>8)&0xff), (char)(bs[i].tag&0xff) );
		else
		    readttfbaseminmax(ttf,bs[i].offset+defminmax,info,bs[i].tag,0);
		if ( langsyscnt==0 )
		    printf("\t  No %s min/max extents for specific langs in '%c%c%c%c' script\n",
			    axis==0 ? "Horizontal": "Vertical",
			    (char)((bs[i].tag>>24)&0xff), (char)((bs[i].tag>>16)&0xff),
			    (char)((bs[i].tag>>8)&0xff), (char)(bs[i].tag&0xff) );
		else
		    for ( j=0; j<langsyscnt; ++j ) if ( ls[j].offset!=0 )
			readttfbaseminmax(ttf,bs[i].offset+ls[j].offset,info,bs[i].tag,ls[j].tag);
		free(ls);
	    }
	    free(bs);
	}
	free(tags);
    }
return( 1 );
}

static void readttfjustmax(const char *label,FILE *ttf,int base,int offset, struct ttfinfo *info) {
    int lcnt,i;
    int *offsets;

    if ( offset==0 ) {
	printf( "\t    No %s data\n", label );
return;
    }
    base += offset;
    fseek(ttf,base,SEEK_SET);
    lcnt = getushort(ttf);
    offsets = malloc(lcnt*sizeof(int));
    printf( "\t    %d lookup%s for %s\n", lcnt, lcnt==1? "" : "s", label );
    for ( i=0; i<lcnt; ++i )
	printf( "\t\tOffset to lookup %d\n", offsets[i] = getushort(ttf));
    for ( i=0; i<lcnt; ++i )
	showgpossublookup(ttf,base, offsets[i], info, true);
    free(offsets);
}

static void readttfjustlookups(const char *label,FILE *ttf,int base,int offset) {
    int lcnt,i;

    if ( offset==0 ) {
	printf( "\t    No %s data\n", label );
return;
    }
    base += offset;
    fseek(ttf,base,SEEK_SET);
    lcnt = getushort(ttf);
    printf( "\t    %d lookup%s for %s\n", lcnt, lcnt==1? "" : "s", label );
    for ( i=0; i<lcnt; ++i )
	printf( "\t\tLookup %d\n", getushort(ttf));
}

static void readttfjustlangsys(FILE *ttf,int offset,uint32_t stag, uint32_t ltag, struct ttfinfo *info) {
    int pcnt,j;
    int *offsets;
    int shrinkenablesub, shrinkenablepos, shrinkdisablesub, shrinkdisablepos;
    int extendenablesub, extendenablepos, extenddisablesub, extenddisablepos;
    int shrinkmax, extendmax;

    fseek(ttf,offset,SEEK_SET);
    printf("\t  Justification priority data for '%c%c%c%c' script, '%c%c%c%c' lang.\n",
	    (char)((stag>>24)&0xff), (char)((stag>>16)&0xff),
	    (char)((stag>>8)&0xff), (char)(stag&0xff),
	    (char)((ltag>>24)&0xff), (char)((ltag>>16)&0xff),
	    (char)((ltag>>8)&0xff), (char)(ltag&0xff) );
    pcnt = getushort(ttf);
    offsets = malloc(pcnt*sizeof(int));
    for ( j=0; j<pcnt; ++j )
	offsets[j] = getushort(ttf);
    printf( "\t  %d Priority level%s\n", pcnt, pcnt==1 ? "" : "s" );
    for ( j=0; j<pcnt; ++j ) {
	if ( offsets[j]==0 ) {
	    printf( "\t   No data for priority level %d\n", j );
    continue;
	}
	printf( "\t   Priority level %d\n", j );
	fseek(ttf,offset+offsets[j],SEEK_SET);
	shrinkenablesub = getushort(ttf);
	shrinkdisablesub = getushort(ttf);
	shrinkenablepos = getushort(ttf);
	shrinkdisablepos = getushort(ttf);
	shrinkmax = getushort(ttf);
	extendenablesub = getushort(ttf);
	extenddisablesub = getushort(ttf);
	extendenablepos = getushort(ttf);
	extenddisablepos = getushort(ttf);
	extendmax = getushort(ttf);
	readttfjustlookups("ShrinkageEnableGSUB",ttf,offset+offsets[j],shrinkenablesub);
	readttfjustlookups("ShrinkageDisableGSUB",ttf,offset+offsets[j],shrinkdisablesub);
	readttfjustlookups("ShrinkageEnableGPOS",ttf,offset+offsets[j],shrinkenablepos);
	readttfjustlookups("ShrinkageDisableGPOS",ttf,offset+offsets[j],shrinkdisablepos);
	readttfjustmax("ShrinkageMax",ttf,offset+offsets[j],shrinkmax,info);
	readttfjustlookups("ExtensionEnableGSUB",ttf,offset+offsets[j],extendenablesub);
	readttfjustlookups("ExtensionDisableGSUB",ttf,offset+offsets[j],extenddisablesub);
	readttfjustlookups("ExtensionEnableGPOS",ttf,offset+offsets[j],extendenablepos);
	readttfjustlookups("ExtensionDisableGPOS",ttf,offset+offsets[j],extenddisablepos);
	readttfjustmax("ExtensionMax",ttf,offset+offsets[j],extendmax,info);
    }
    free(offsets);
}

static void readttfjstf(FILE *ttf,FILE *util, struct ttfinfo *info) {
    int version;
    int i,j,cnt, lcnt, lmax=0;
    struct tagoff *scripts, *langs=NULL;
    int extenderOff, defOff;

    fseek(ttf,info->JSTF_start,SEEK_SET);
    printf( "\nJSTF table (at %d)\n", info->JSTF_start);
    printf( "\tVersion: 0x%08x\n", (unsigned int)(version = getlong(ttf)) );
    if ( version!=0x00010000 )
	fprintf( stderr, "!> Bad version number for BASE table.\n" );
    cnt = getushort(ttf);		/* Script count */
    printf( "\t %d scripts\n", cnt );
    scripts = malloc(cnt*sizeof(struct tagoff));
    for ( i=0; i<cnt; ++i ) {
	scripts[i].tag = getlong(ttf);
	scripts[i].offset = getushort(ttf);
    }

    for ( i=0; i<cnt; ++i ) {
	printf("\t Justification data for '%c%c%c%c' script\n",		\
		(char)(scripts[i].tag>>24), (char)(scripts[i].tag>>16),	\
	        (char)(scripts[i].tag>>8), (char)(scripts[i].tag) );
	if ( scripts[i].offset==0 ) {
	    printf("\t  Nothing for this script\n" );
    continue;
	}
	fseek(ttf,info->JSTF_start+scripts[i].offset,SEEK_SET);
	extenderOff = getushort(ttf);
	defOff      = getushort(ttf);
	lcnt        = getushort(ttf);
	if ( lcnt>lmax )
	    langs = realloc(langs,(lmax = lcnt+20)*sizeof(struct tagoff));
	for ( j=0; j<lcnt; ++j ) {
	    langs[j].tag = getlong(ttf);
	    langs[j].offset = getushort(ttf);
	}
	if ( extenderOff==0 )
	    printf( "\t  No extension glyphs.\n" );
	else {
	    int gcnt;
	    fseek(ttf,info->JSTF_start+scripts[i].offset+extenderOff,SEEK_SET);
	    gcnt = getushort(ttf);
	    printf( "\t  %d extension glyph%s.\n", gcnt, gcnt==1?"":"s" );
	    for ( j=0; j<gcnt; ++j ) {
		int gid = getushort(ttf);
		printf( "\t   Extension Glyph %d (%s)\n", gid,
			gid>=info->glyph_cnt ? "!!! Bad glyph !!!" :
			info->glyph_names == NULL ? "" : info->glyph_names[gid]);
	    }
	}
	if ( defOff==0 )
	    printf( "\t  No default just language system table\n" );
	else
	    readttfjustlangsys(ttf,info->JSTF_start+scripts[i].offset+defOff,
		    scripts[i].tag, CHR('d','f','l','t'),info);
	printf( "\t  %d langsys table%s.\n", lcnt, lcnt==1?"":"s" );
	for ( j=0; j<lcnt; ++j ) {
	    readttfjustlangsys(ttf,info->JSTF_start+scripts[i].offset+langs[j].offset,
		    scripts[i].tag, langs[j].tag,info);
	}
    }
    free( langs );
    free( scripts );
}


static void readit(FILE *ttf, FILE *util) {
    struct ttfinfo info;
    int i, pos;

    memset(&info,'\0',sizeof(info));
    readttfheader(ttf,util,&info);
    if ( info.ttccnt!=0 ) {
	for ( i=0; i<info.ttccnt; ++i ) {
	    printf( "\n=============================> TTC font %d\n", i );
	    fseek(ttf,info.ttcoffsets[i],SEEK_SET);
	    readit(ttf,util);
	}
return;
    }
    if ( just_headers )
return;
    if ( head_check ) {
	if ( info.head_start!=0 )
	    readttfhead(ttf,util,&info);
return;
    }

    if ( info.fftm_start!=0 )
	readttfFFTM(ttf,util,&info);
    if ( info.head_start!=0 )
	readttfhead(ttf,util,&info);
    if ( info.hhea_start!=0 )
	readttfhhead(ttf,util,&info);
    if ( info.copyright_start!=0 )
	readttfname(ttf,util,&info);
    if ( info.os2_start!=0 )
	readttfos2(ttf,util,&info);
    if ( info.maxp_start!=0 )
	readttfmaxp(ttf,util,&info);
    if ( info.gasp_start!=0 )
	readttfgasp(ttf,util,&info);
    if ( info.encoding_start!=0 )
	readttfencodings(ttf,util,&info);
    if ( info.postscript_len!= 0 )
	readttfpost(ttf,util,&info);
    if ( info.cvt_length!=0 )
	readttfcvt(ttf,util,&info);
    if ( info.cff_start!=0 )
	readcff(ttf,util,&info);
    if ( info.gsub_start!=0 )
	readttfgsub(ttf,util,&info);
    if ( info.gpos_start!=0 )
	readttfgpos(ttf,util,&info);
    if ( info.kern_start!=0 )
	readttfkern(ttf,util,&info);
    if ( info.gdef_start!=0 )
	readttfgdef(ttf,util,&info);
    if ( info.base_start!=0 )
	readttfbase(ttf,util,&info);
    if ( info.JSTF_start!=0 )
	readttfjstf(ttf,util,&info);
    if ( info.bitmaploc_start!=0 && info.bitmapdata_start!=0 )
	readttfbitmaps(ttf,util,&info);
    if ( info.bitmapscale_start!=0 )
	readttfbitmapscale(ttf,util,&info);
    if ( info.prep_length!=0 )
	readtableinstr(ttf,info.prep_start,info.prep_length,"prep");
    if ( info.fpgm_length!=0 )
	readtableinstr(ttf,info.fpgm_start,info.fpgm_length,"fpgm");
    if ( info.fdsc_start!=0 )
	readttffontdescription(ttf,util,&info);
    if ( info.feat_start!=0 )
	readttffeatures(ttf,util,&info);
    if ( info.mort_start!=0 || info.morx_start!=0 )
	readttfmetamorph(ttf,util,&info);
    if ( info.prop_start!=0 )
	readttfappleprop(ttf,util,&info);
    if ( info.lcar_start!=0 )
	readttfapplelcar(ttf,util,&info);
    if ( info.opbd_start!=0 )
	readttfappleopbd(ttf,util,&info);
    if ( info.fvar_start!=0 )
	readttfapplefvar(ttf,util,&info);
    if ( info.gvar_start!=0 )
	readttfapplegvar(ttf,util,&info);
    if ( info.hdmx_start!=0 )
	readttfhdmx(ttf,util,&info);
    if ( info.math_start!=0 )
	readttfmath(ttf,util,&info);
    if ( info.glyphlocations_start!=0 ) {
	fseek(ttf,info.glyphlocations_start,SEEK_SET);
	if ( info.index_to_loc_is_long ) {
	    for ( i=0; 4*i<info.loca_length; ++i ) {
		pos = getlong(ttf);
		if ( pos&3 )
		    fprintf( stderr, "Not aligned GID=%d, pos=0x%x\n", i, (unsigned int)(pos));
	    }
	} else {
	    for ( i=0; 2*i<info.loca_length; ++i ) {
		pos = getushort(ttf);
		if ( pos&1 )
		    fprintf( stderr, "Not aligned GID=%d, pos=0x%x\n", i, (unsigned int)(pos<<1) );
	    }
	}
    }
}

int main(int argc, char **argv) {
    FILE *ttf, *util;
    char *filename = NULL;
    int i;
    char *pt;

    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' ) {
	    pt = argv[i]+1;
	    if ( *pt=='-' ) ++pt;
	    if ( strcmp(pt,"v")==0 || strcmp(pt,"verbose")==0 )
		verbose = true;
	    else if ( strcmp(pt,"h")==0 || strcmp(pt,"headers")==0 )
		just_headers = true;
	    else if ( strcmp(pt,"c")==0 || strcmp(pt,"checkhead")==0 )
		head_check = true;
	    else {
		fprintf( stderr, "%s [-verbose] ttf-file\n", argv[0]);
		exit(1);
	    }
	} else {
	    if ( filename!=NULL )
		printf( "\n\n\n******************** %s *****************\n\n\n", argv[i]);
	    filename = argv[i];
	    ttf = fopen(filename,"rb");
	    if ( ttf==NULL ) {
		fprintf( stderr, "Can't open %s\n", argv[1]);
return( 1 );
	    }
	    util = fopen(filename,"rb");

	    readit(ttf,util);
	    fclose(ttf);
	    fclose(util);
	}
    }
    if ( filename==NULL ) {
	fprintf( stderr, "%s [-verbose] [-headers] ttf-file\n", argv[0]);
	exit(1);
    }
return( 0 );
}
