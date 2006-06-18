/* A set of routines used in pfaedit that I don't want/need to bother with for*/
/*  sfddiff, but which get called by it */

#include "pfaeditui.h"
#include <stdarg.h>
#include <math.h>
#include <charset.h>
#include <ustring.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

int local_encoding = e_iso8859_1;
char *iconv_local_encoding_name = NULL;


static int32 tex_base_encoding[] = {
    0x0000, 0x02d9, 0xfb01, 0xfb02, 0x2044, 0x02dd, 0x0141, 0x0142,
    0x02db, 0x02da, 0x000a, 0x02d8, 0x2212, 0x000d, 0x017d, 0x017e,
    0x02c7, 0x0131, 0xf6be, 0xfb00, 0xfb03, 0xfb04, 0x2260, 0x221e,
    0x2264, 0x2265, 0x2202, 0x2211, 0x220f, 0x03c0, 0x0060, 0x0027,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x2019,
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    0x2018, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
    0x20ac, 0x222b, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x2126, 0x221a, 0x2248,
    0x0090, 0x0091, 0x0092, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x2206, 0x25ca, 0x0178,
    0x0000, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x002d, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};

static int32 unicode_from_MacSymbol[] = {
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
  0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
  0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
  0x0020, 0x0021, 0x2200, 0x0023, 0x2203, 0x0025, 0x0026, 0x220d,
  0x0028, 0x0029, 0x2217, 0x002b, 0x002c, 0x2212, 0x002e, 0x002f,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
  0x2245, 0x0391, 0x0392, 0x03a7, 0x0394, 0x0395, 0x03a6, 0x0393,
  0x0397, 0x0399, 0x03d1, 0x039a, 0x039b, 0x039c, 0x039d, 0x039f,
  0x03a0, 0x0398, 0x03a1, 0x03a3, 0x03a4, 0x03a5, 0x03c2, 0x03a9,
  0x039e, 0x03a8, 0x0396, 0x005b, 0x2234, 0x005d, 0x22a5, 0x005f,
  0xf8e5, 0x03b1, 0x03b2, 0x03c7, 0x03b4, 0x03b5, 0x03c6, 0x03b3,
  0x03b7, 0x03b9, 0x03d5, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03bf,
  0x03c0, 0x03b8, 0x03c1, 0x03c3, 0x03c4, 0x03c5, 0x03d6, 0x03c9,
  0x03be, 0x03c8, 0x03b6, 0x007b, 0x007c, 0x007d, 0x223c, 0x007f,
  0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
  0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
  0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
  0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
  0x0000, 0x03d2, 0x2032, 0x2264, 0x2044, 0x221e, 0x0192, 0x2663,
  0x2666, 0x2665, 0x2660, 0x2194, 0x2190, 0x2191, 0x2192, 0x2193,
  0x00b0, 0x00b1, 0x2033, 0x2265, 0x00d7, 0x221d, 0x2202, 0x2022,
  0x00f7, 0x2260, 0x2261, 0x2248, 0x2026, 0xf8e6, 0xf8e7, 0x21b5,
  0x2135, 0x2111, 0x211c, 0x2118, 0x2297, 0x2295, 0x2205, 0x2229,
  0x222a, 0x2283, 0x2287, 0x2284, 0x2282, 0x2286, 0x2208, 0x2209,
  0x2220, 0x2207, 0x00ae, 0x00a9, 0x2122, 0x220f, 0x221a, 0x22c5,
  0x00ac, 0x2227, 0x2228, 0x21d4, 0x21d0, 0x21d1, 0x21d2, 0x21d3,
  0x22c4, 0x2329, 0xf8e8, 0xf8e9, 0xf8ea, 0x2211, 0xf8eb, 0xf8ec,
  0xf8ed, 0xf8ee, 0xf8ef, 0xf8f0, 0xf8f1, 0xf8f2, 0xf8f3, 0xf8f4,
  0xf8ff, 0x232a, 0x222b, 0x2320, 0xf8f5, 0x2321, 0xf8f6, 0xf8f7,
  0xf8f8, 0xf8f9, 0xf8fa, 0xf8fb, 0xf8fc, 0xf8fd, 0xf8fe, 0x02c7
};

/* I don't think iconv provides encodings for zapfdingbats nor jis201 */
/*  Perhaps I should list them here for compatability, but I think I'll just */
/*  leave them out. I doubt they get used.				     */
static Encoding texbase = { "TeX-Base-Encoding", 256, tex_base_encoding, NULL, NULL, 1, 1, 1, 1 };
       Encoding custom = { "Custom", 0, NULL, NULL, &texbase,			  1, 1, 0, 0, 0, 0, 0, 1, 0, 0 };
static Encoding original = { "Original", 0, NULL, NULL, &custom,		  1, 1, 0, 0, 0, 0, 0, 0, 1, 0 };
static Encoding compact = { "Compacted", 0, NULL, NULL, &original,		  1, 1, 0, 0, 0, 0, 0, 0, 0, 1 };
static Encoding unicodebmp = { "UnicodeBmp", 65536, NULL, NULL, &compact, 	  1, 1, 0, 0, 1, 1, 0, 0, 0, 0 };
static Encoding unicodefull = { "UnicodeFull", 17*65536, NULL, NULL, &unicodebmp, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0 };
static Encoding symbol = { "Symbol", 256, unicode_from_MacSymbol, NULL, &unicodefull,1, 1, 1, 1, 0, 0, 1, 0, 0, 0 };
Encoding *enclist = &symbol;

static int TryEscape( Encoding *enc,char *escape_sequence ) {
    char from[20], ucs2[20];
    size_t fromlen, tolen;
    ICONV_CONST char *fpt;
    char *upt;
    int i, j, low;
    int esc_len = strlen(escape_sequence);

    strcpy(from,escape_sequence);

    enc->has_2byte = false;
    low = -1;
    for ( i=0; i<256; ++i ) if ( i!=escape_sequence[0] ) {
	for ( j=0; j<256; ++j ) {
	    from[esc_len] = i; from[esc_len+1] = j; from[esc_len+2] = 0;
	    fromlen = esc_len+2;
	    fpt = from;
	    upt = ucs2;
	    tolen = sizeof(ucs2);
	    if ( iconv( enc->tounicode , &fpt, &fromlen, &upt, &tolen )!= (size_t) (-1) &&
		    upt-ucs2==2 /* Exactly one character */ ) {
		if ( low==-1 ) {
		    enc->low_page = low = i;
		    enc->has_2byte = true;
		}
		enc->high_page = i;
	break;
	    }
	}
    }
    if ( enc->low_page==enc->high_page )
	enc->has_2byte = false;
    if ( enc->has_2byte ) {
	strcpy(enc->iso_2022_escape, escape_sequence);
	enc->iso_2022_escape_len = esc_len;
    }
return( enc->has_2byte );
}

Encoding *FindOrMakeEncoding(const char *name) {
    Encoding *enc;
    char buffer[20];
    const char *iconv_name;
    Encoding temp;
    uint8 good[256];
    int i, j, any, all;
    char from[8], ucs2[20];
    size_t fromlen, tolen;
    ICONV_CONST char *fpt;
    char *upt;
    /* iconv is not case sensitive */

    if ( strncmp(name,"iso8859_",8)==0 || strncmp(name,"koi8_",5)==0 ) {
	/* Fixup for old naming conventions */
	strncpy(buffer,name,sizeof(buffer));
	*strchr(buffer,'_') = '-';
	name = buffer;
    } else if ( strncmp(name,"iso-8859",8)==0 ) {
	/* Fixup for old naming conventions */
	strncpy(buffer,name,3);
	strncpy(buffer+3,name+4,sizeof(buffer)-3);
	name = buffer;
    } else if ( strcmp(name,"AdobeStandardEncoding")==0 )
	name = "AdobeStandard";
    for ( enc=enclist; enc!=NULL; enc=enc->next )
	if ( strmatch(name,enc->enc_name)==0 ||
		(enc->iconv_name!=NULL && strmatch(name,enc->iconv_name)==0))
return( enc );
    if ( strmatch(name,"unicode")==0 )
return( &unicodebmp );
    if ( strmatch(name,"unicode4")==0 )
return( &unicodefull );

    iconv_name = name;
    /* Mac seems to work ok */
    if ( strcmp(name,"win")==0 )
	iconv_name = "MS-ANSI";		/* "WINDOWS-1252";*/
    else if ( strcmp(name,"jis208")==0 )
	iconv_name = "ISO-2022-JP";
    else if ( strcmp(name,"jis212")==0 )
	iconv_name = "ISO-2022-JP-2";
    else if ( strcmp(name,"ksc5601")==0 )
	iconv_name = "ISO-2022-KR";
    else if ( strcmp(name,"gb2312")==0 )
	iconv_name = "ISO-2022-CN";
    else if ( strcmp(name,"wansung")==0 )
	iconv_name = "EUC-KR";
    else if ( strcmp(name,"gb2312pk")==0 )
	iconv_name = "EUC-CN";

/* Escape sequences:					*/
/*	ISO-2022-CN:     \e $ ) A ^N			*/
/*	ISO-2022-KR:     \e $ ) C ^N			*/
/*	ISO-2022-JP:     \e $ B				*/
/*	ISO-2022-JP-2:   \e $ ( D			*/
/*	ISO-2022-JP-3:   \e $ ( O			*/ /* Capital "O", not zero */
/*	ISO-2022-CN-EXT: \e $ ) E ^N			*/ /* Not sure about this, also uses CN escape */

    memset(&temp,0,sizeof(temp));
    temp.builtin = true;
    temp.tounicode = iconv_open("UCS2",iconv_name);
    if ( temp.tounicode==NULL )
return( NULL );			/* Iconv doesn't recognize this name */
    temp.fromunicode = iconv_open(iconv_name,"UCS2");

    memset(good,0,sizeof(good));
    any = false; all = true;
    for ( i=1; i<256; ++i ) {
	from[0] = i; from[1] = 0;
	fromlen = 1;
	fpt = from;
	upt = ucs2;
	tolen = sizeof(ucs2);
	if ( iconv( temp.tounicode , &fpt, &fromlen, &upt, &tolen )!= (size_t) (-1)) {
	    good[i] = true;
	    any = true;
	} else
	    all = false;
    }
    if ( any )
	temp.has_1byte = true;
    if ( all )
	temp.only_1byte = true;

    if ( !all ) {
	for ( i=temp.has_1byte; i<256; ++i ) if ( !good[i] ) {
	    for ( j=0; j<256; ++j ) {
		from[0] = i; from[1] = j; from[2] = 0;
		fromlen = 2;
		fpt = from;
		upt = ucs2;
		tolen = sizeof(ucs2);
		if ( iconv( temp.tounicode , &fpt, &fromlen, &upt, &tolen )!= (size_t) (-1) &&
			upt-ucs2==2 /* Exactly one character */ ) {
		    if ( temp.low_page==-1 )
			temp.low_page = i;
		    temp.high_page = i;
		    temp.has_2byte = true;
	    break;
		}
	    }
	}
	if ( temp.low_page==temp.high_page ) {
	    temp.has_2byte = false;
	    temp.low_page = temp.high_page = -1;
	}
	if ( !temp.has_2byte && !good[033]/* escape */ ) {
	    if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP3")!=NULL &&
		    TryEscape( &temp,"\33$(O" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP2")!=NULL &&
		    TryEscape( &temp,"\33$(D" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP")!=NULL &&
		    TryEscape( &temp,"\33$B" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"KR")!=NULL &&
		    TryEscape( &temp,"\33$)C\16" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"CN")!=NULL &&
		    TryEscape( &temp,"\33$)A\16" ))
		;
	}
    }
    if ( !temp.has_1byte && !temp.has_2byte )
return( NULL );

    enc = chunkalloc(sizeof(Encoding));
    *enc = temp;
    enc->enc_name = copy(name);
    if ( iconv_name!=name )
	enc->iconv_name = copy(name);
    enc->next = enclist;
    enc->builtin = true;
    enclist = enc;
    if ( enc->has_2byte )
	enc->char_cnt = (enc->high_page<<8) + 256;
    else {
	enc->char_cnt = 256;
	enc->only_1byte = true;
    }
    if ( strstrmatch(iconv_name,"JP")!=NULL )
	enc->is_japanese = true;
    else if ( strstrmatch(iconv_name,"KR")!=NULL )
	enc->is_korean = true;
    else if ( strstrmatch(iconv_name,"CN")!=NULL )
	enc->is_simplechinese = true;
    else if ( strstrmatch(iconv_name,"BIG")!=NULL && strstrmatch(iconv_name,"5")!=NULL )
	enc->is_tradchinese = true;

    if ( strstrmatch(name,"ISO8859")!=NULL &&
	    strtol(name+strlen(name)-2,NULL,10)>=16 )
	/* Not in our menu, don't hide */;
    else if ( iconv_name!=name || strmatch(name,"mac")==0 || strstrmatch(name,"ISO8859")!=NULL ||
	    strmatch(name,"koi8-r")==0 || strmatch(name,"sjis")==0 ||
	    strmatch(name,"big5")==0 || strmatch(name,"big5hkscs")==0 )
	enc->hidden = true;

return( enc );
}

#if defined(FONTFORGE_CONFIG_GDRAW)
void gwwv_progress_start_indicator(int delay, const char *title, const char *line1,
	const char *line2, int tot, int stages) {}
void GProgressEnableStop(int enabled) {}
void gwwv_progress_end_indicator(void) {}
int gwwv_progress_next_stage(void) { return(1); }
int gwwv_progress_next(void) { return( 1 ); }
void gwwv_progress_change_line2(const char *line2) {}
void gwwv_progress_change_total(int tot) { }
void gwwv_progress_change_stages(int stages) { }
int gwwv_ask(const char *title, const char **answers,
	int def, int cancel,const char *question,...) { return cancel; }
#elif defined(FONTFORGE_CONFIG_GTK)
void gwwv_progress_start_indicator(
    int delay,			/* in tenths of seconds */
    const unichar_t *win_title,	/* for the window decoration */
    const unichar_t *line1,	/* First line of description */
    const unichar_t *line2,	/* Second line */
    int tot,			/* Number of sub-entities in the operation */
    int stages			/* Number of stages, each processing tot sub-entities */
) {}
void gwwv_progress_enable_stop(int enabled) {}
void gwwv_progress_end_indicator(void) {}
int gwwv_progress_next_stage(void) { return(1); }
int gwwv_progress_next(void) { return( 1 ); }
void gwwv_progress_change_line2(int line2r) {}
void gwwv_progress_change_total(int tot) { }
void gwwv_progress_change_stages(int stages) { }
#endif

SplineFont *LoadSplineFont(char *filename, enum openflags of) { return NULL; }
void RefCharFree(RefChar *ref) {}
void LinearApproxFree(LinearApprox *la) {}
SplineFont *SplineFontNew(void) {return NULL; }
void SplineFontFree(SplineFont *sf) { }
void BDFCharFree(BDFChar *bc) { }
void SplineCharFree(SplineChar *sc) { }
void SCMakeDependent(SplineChar *dependent,SplineChar *base) {}
void SCReinstanciateRefChar(SplineChar *sc,RefChar *rf) {}
void SCGuessHHintInstancesList(SplineChar *sc) { }
void SCGuessVHintInstancesList(SplineChar *sc) { }
int StemListAnyConflicts(StemInfo *stems) { return 0 ; }
int getAdobeEnc(char *name) { return -1; }
SplineChar *SFMakeChar(SplineFont *sf, EncMap *map, int enc) { return NULL; }
GDisplay *screen_display=NULL;
int no_windowing_ui = true;
int FVWinInfo(struct fontview *sf,int *cc,int *rc) { return -1 ; }
EncMap *EncMapFromEncoding(SplineFont *sf,Encoding *enc) { return NULL; }

/* ************************************************************************** */
/* And some routines we actually do need */

void IError(const char *format, ... ) {
    va_list ap;
    va_start(ap,format);
    vfprintf(stderr,format,ap);
    va_end(ap);
}

void LogError(const char *format,...) {
    va_list ap;

    va_start(ap,format);
    vfprintf(stderr,format,ap);
    va_end(ap);
}

int RealNear(real a,real b) {
    real d;

#if 0 /* def FONTFORGE_CONFIG_USE_DOUBLE */
    if ( a==0 )
return( b>-1e-8 && b<1e-8 );
    if ( b==0 )
return( a>-1e-8 && a<1e-8 );

    d = a/(1024*1024.);
    if ( d<0 ) d = -d;
return( b>a-d && b<a+d );
#else		/* For floats */
    if ( a==0 )
return( b>-1e-5 && b<1e-5 );
    if ( b==0 )
return( a>-1e-5 && a<1e-5 );

    d = a/(1024*64.);
    if ( d<0 ) d = -d;
return( b>a-d && b<a+d );
#endif
}

int RealApprox(real a,real b) {

    if ( a==0 ) {
	if ( b<.0001 && b>-.0001 )
return( true );
    } else if ( b==0 ) {
	if ( a<.0001 && a>-.0001 )
return( true );
    } else {
	a /= b;
	if ( a>=.95 && a<=1.05 )
return( true );
    }
return( false );
}

int RealWithin(real a,real b,real fudge) {

return( b>=a-fudge && b<=a+fudge );
}

int SplineIsLinear(Spline *spline) {
    real t1,t2;
    int ret;

    if ( spline->knownlinear )
return( true );
    if ( spline->knowncurved )
return( false );

    if ( spline->splines[0].a==0 && spline->splines[0].b==0 &&
	    spline->splines[1].a==0 && spline->splines[1].b==0 )
return( true );

    /* Something is linear if the control points lie on the line between the */
    /*  two base points */

    /* Vertical lines */
    if ( RealNear(spline->from->me.x,spline->to->me.x) ) {
	ret = RealNear(spline->from->me.x,spline->from->nextcp.x) &&
	    RealNear(spline->from->me.x,spline->to->prevcp.x) &&
	    ((spline->from->nextcp.y >= spline->from->me.y &&
	      spline->from->nextcp.y <= spline->to->prevcp.y &&
	      spline->to->prevcp.y <= spline->to->me.y ) ||
	     (spline->from->nextcp.y <= spline->from->me.y &&
	      spline->from->nextcp.y >= spline->to->prevcp.y &&
	      spline->to->prevcp.y >= spline->to->me.y ));
    /* Horizontal lines */
    } else if ( RealNear(spline->from->me.y,spline->to->me.y) ) {
	ret = RealNear(spline->from->me.y,spline->from->nextcp.y) &&
	    RealNear(spline->from->me.y,spline->to->prevcp.y) &&
	    ((spline->from->nextcp.x >= spline->from->me.x &&
	      spline->from->nextcp.x <= spline->to->prevcp.x &&
	      spline->to->prevcp.x <= spline->to->me.x) ||
	     (spline->from->nextcp.x <= spline->from->me.x &&
	      spline->from->nextcp.x >= spline->to->prevcp.x &&
	      spline->to->prevcp.x >= spline->to->me.x));
    } else {
	ret = true;
	t1 = (spline->from->nextcp.y-spline->from->me.y)/(spline->to->me.y-spline->from->me.y);
	if ( t1<0 || t1>1.0 )
	    ret = false;
	else {
	    t2 = (spline->from->nextcp.x-spline->from->me.x)/(spline->to->me.x-spline->from->me.x);
	    if ( t2<0 || t2>1.0 )
		ret = false;
	    ret = RealApprox(t1,t2);
	}
	if ( ret ) {
	    t1 = (spline->to->me.y-spline->to->prevcp.y)/(spline->to->me.y-spline->from->me.y);
	    if ( t1<0 || t1>1.0 )
		ret = false;
	    else {
		t2 = (spline->to->me.x-spline->to->prevcp.x)/(spline->to->me.x-spline->from->me.x);
		if ( t2<0 || t2>1.0 )
		    ret = false;
		else
		    ret = RealApprox(t1,t2);
	    }
	}
    }
    spline->knowncurved = !ret;
    spline->knownlinear = ret;
return( ret );
}

void SplineRefigure3(Spline *spline) {
    SplinePoint *from = spline->from, *to = spline->to;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];

#ifdef DEBUG
    if ( RealNear(from->me.x,to->me.x) && RealNear(from->me.y,to->me.y))
	IError("Zero length spline created");
#endif
    xsp->d = from->me.x; ysp->d = from->me.y;
    if ( from->nonextcp ) from->nextcp = from->me;
    else if ( from->nextcp.x==from->me.x && from->nextcp.y == from->me.y ) from->nonextcp = true;
    if ( to->noprevcp ) to->prevcp = to->me;
    else if ( to->prevcp.x==to->me.x && to->prevcp.y == to->me.y ) to->noprevcp = true;
    if ( from->nonextcp && to->noprevcp ) {
	spline->islinear = true;
	xsp->c = to->me.x-from->me.x;
	ysp->c = to->me.y-from->me.y;
	xsp->a = xsp->b = 0;
	ysp->a = ysp->b = 0;
    } else {
	/* from p. 393 (Operator Details, curveto) Postscript Lang. Ref. Man. (Red book) */
	xsp->c = 3*(from->nextcp.x-from->me.x);
	ysp->c = 3*(from->nextcp.y-from->me.y);
	xsp->b = 3*(to->prevcp.x-from->nextcp.x)-xsp->c;
	ysp->b = 3*(to->prevcp.y-from->nextcp.y)-ysp->c;
	xsp->a = to->me.x-from->me.x-xsp->c-xsp->b;
	ysp->a = to->me.y-from->me.y-ysp->c-ysp->b;
	if ( RealNear(xsp->c,0)) xsp->c=0;
	if ( RealNear(ysp->c,0)) ysp->c=0;
	if ( RealNear(xsp->b,0)) xsp->b=0;
	if ( RealNear(ysp->b,0)) ysp->b=0;
	if ( RealNear(xsp->a,0)) xsp->a=0;
	if ( RealNear(ysp->a,0)) ysp->a=0;
	spline->islinear = false;
	if ( ysp->a==0 && xsp->a==0 && ysp->b==0 && xsp->b==0 )
	    spline->islinear = true;	/* This seems extremely unlikely... */
    }
    if ( isnan(ysp->a) || isnan(xsp->a) )
	IError("NaN value in spline creation");
    LinearApproxFree(spline->approx);
    spline->approx = NULL;
    spline->knowncurved = false;
    spline->knownlinear = spline->islinear;
    SplineIsLinear(spline);
    spline->isquadratic = false;
    if ( !spline->islinear && xsp->a==0 && ysp->a==0 )
	spline->isquadratic = true;	/* Only likely if we read in a TTF */
}

void SplineRefigure2(Spline *spline) {
    SplinePoint *from = spline->from, *to = spline->to;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];

#ifdef DEBUG
    if ( RealNear(from->me.x,to->me.x) && RealNear(from->me.y,to->me.y))
	IError("Zero length spline created");
#endif

    if ( from->nonextcp ) from->nextcp = from->me;
    else if ( from->nextcp.x==from->me.x && from->nextcp.y == from->me.y ) from->nonextcp = true;
    if ( to->noprevcp ) to->prevcp = to->me;
    else if ( to->prevcp.x==to->me.x && to->prevcp.y == to->me.y ) to->noprevcp = true;

    if ( from->nonextcp && to->noprevcp )
	/* Ok */;
    else if ( from->nonextcp || to->noprevcp || from->nextcp.x!=to->prevcp.x ||
	    from->nextcp.y!=to->prevcp.y )
	IError("Invalid 2nd order spline in SplineRefigure2" );

    xsp->d = from->me.x; ysp->d = from->me.y;
    if ( from->nonextcp && to->noprevcp ) {
	spline->islinear = true;
	xsp->c = to->me.x-from->me.x;
	ysp->c = to->me.y-from->me.y;
	xsp->a = xsp->b = 0;
	ysp->a = ysp->b = 0;
    } else {
	/* from p. 393 (Operator Details, curveto) Postscript Lang. Ref. Man. (Red book) */
	xsp->c = 2*(from->nextcp.x-from->me.x);
	ysp->c = 2*(from->nextcp.y-from->me.y);
	xsp->b = to->me.x-from->me.x-xsp->c;
	ysp->b = to->me.y-from->me.y-ysp->c;
	xsp->a = 0;
	ysp->a = 0;
	if ( RealNear(xsp->c,0)) xsp->c=0;
	if ( RealNear(ysp->c,0)) ysp->c=0;
	if ( RealNear(xsp->b,0)) xsp->b=0;
	if ( RealNear(ysp->b,0)) ysp->b=0;
	spline->islinear = false;
	if ( ysp->b==0 && xsp->b==0 )
	    spline->islinear = true;	/* This seems extremely unlikely... */
    }
    if ( isnan(ysp->b) || isnan(xsp->b) )
	IError("NaN value in spline creation");
    LinearApproxFree(spline->approx);
    spline->approx = NULL;
    spline->knowncurved = false;
    spline->knownlinear = spline->islinear;
    SplineIsLinear(spline);
    spline->isquadratic = !spline->knownlinear;
    spline->order2 = true;
}

Spline *SplineMake3(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure3(spline);
return( spline );
}

Spline *SplineMake2(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure2(spline);
return( spline );
}

Spline *SplineMake(SplinePoint *from, SplinePoint *to, int order2) {
    if ( order2 )
return( SplineMake2(from,to));
    else
return( SplineMake3(from,to));
}

int SCDrawsSomething(SplineChar *sc) {
    int layer,l;
    RefChar *ref;

    if ( sc==NULL )
return( false );
    for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
	if ( sc->layers[layer].splines!=NULL || sc->layers[layer].images!=NULL )
return( true );
	for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	    for ( l=0; l<ref->layer_cnt; ++l )
		if ( ref->layers[l].splines!=NULL )
return( true );
    }
return( false );
}

int SCWorthOutputting(SplineChar *sc) {
return( sc!=NULL &&
	( SCDrawsSomething(sc) || sc->widthset || sc->anchor!=NULL ||
#if HANYANG
	    sc->compositionunit ||
#endif
	    sc->dependents!=NULL ||
	    sc->width!=sc->parent->ascent+sc->parent->descent ) );
}

SplineFont *SplineFontEmpty(void) {
    SplineFont *sf;
    sf = gcalloc(1,sizeof(SplineFont));
    sf->pfminfo.fstype = -1;
     sf->map = EncMapNew(0,0,&custom);
return( sf );
}

RefChar *RefCharCreate(void) {
    RefChar *ref = chunkalloc(sizeof(RefChar));
#ifdef FONTFORGE_CONFIG_TYPE3
    ref->layers = gcalloc(1,sizeof(struct reflayer));
    ref->layers[0].fill_brush.opacity = ref->layers[0].stroke_pen.brush.opacity = 1.0;
    ref->layers[0].fill_brush.col = ref->layers[0].stroke_pen.brush.col = COLOR_INHERITED;
    ref->layers[0].stroke_pen.width = WIDTH_INHERITED;
    ref->layers[0].stroke_pen.linecap = lc_inherited;
    ref->layers[0].stroke_pen.linejoin = lj_inherited;
    ref->layers[0].dofill = true;
#endif
    ref->layer_cnt = 1;
return( ref );
}

void *chunkalloc(int size) {
return( gcalloc(1,size));
}

void chunkfree(void *item,int size) {
    free(item);
}

void LayerDefault(Layer *layer) {
    memset(layer,0,sizeof(Layer));
#ifdef FONTFORGE_CONFIG_TYPE3
    layer->fill_brush.opacity = layer->stroke_pen.brush.opacity = 1.0;
    layer->fill_brush.col = layer->stroke_pen.brush.col = COLOR_INHERITED;
    layer->stroke_pen.width = WIDTH_INHERITED;
    layer->stroke_pen.linecap = lc_inherited;
    layer->stroke_pen.linejoin = lj_inherited;
    layer->dofill = true;
    layer->fillfirst = true;
    layer->stroke_pen.trans[0] = layer->stroke_pen.trans[3] = 1.0;
    layer->stroke_pen.trans[1] = layer->stroke_pen.trans[2] = 0.0;
#endif
}

SplineChar *SplineCharCreate(void) {
    SplineChar *sc = gcalloc(1,sizeof(SplineChar));
    sc->color = COLOR_DEFAULT;
    sc->orig_pos = 0xffff;
    sc->unicodeenc = -1;
    sc->layer_cnt = 2;
#ifdef FONTFORGE_CONFIG_TYPE3
    sc->layers = gcalloc(2,sizeof(Layer));
    LayerDefault(&sc->layers[0]);
    LayerDefault(&sc->layers[1]);
#endif
return( sc );    
}

void EncMapFree(EncMap *map) {
    if ( map==NULL )
return;

    free(map->map);
    free(map->backmap);
    free(map->remap);
    chunkfree(map,sizeof(EncMap));
}

EncMap *EncMapNew(int enccount,int backmax,Encoding *enc) {
    EncMap *map = chunkalloc(sizeof(EncMap));
    
    map->enccount = map->encmax = enccount;
    map->backmax = backmax;
    map->map = galloc(enccount*sizeof(int));
    memset(map->map,-1,enccount*sizeof(int));
    map->backmap = galloc(backmax*sizeof(int));
    memset(map->backmap,-1,backmax*sizeof(int));
    map->enc = enc;
return(map);
}

EncMap *EncMap1to1(int enccount) {
    EncMap *map = chunkalloc(sizeof(EncMap));
    /* Used for CID fonts where CID is same as orig_pos */
    int i;
    
    map->enccount = map->encmax = map->backmax = enccount;
    map->map = galloc(enccount*sizeof(int));
    map->backmap = galloc(enccount*sizeof(int));
    for ( i=0; i<enccount; ++i )
	map->map[i] = map->backmap[i] = i;
    map->enc = &custom;
return(map);
}

GImage *GImageCreate(enum image_type type, int32 width, int32 height) {
    GImage *gi;
    struct _GImage *base;

    if ( type<it_mono || type>it_true )
return( NULL );

    gi = gcalloc(1,sizeof(GImage));
    base = galloc(sizeof(struct _GImage));
    if ( gi==NULL || base==NULL ) {
	free(gi); free(base);
return( NULL );
    }
    gi->u.image = base;
    base->image_type = type;
    base->width = width;
    base->height = height;
    base->bytes_per_line = type==it_true?4*width:type==it_index?width:(width+7)/8;
    base->data = NULL;
    base->clut = NULL;
    base->trans = COLOR_UNKNOWN;
    base->data = galloc(height*base->bytes_per_line);
    if ( base->data==NULL ) {
	free(base);
	free(gi);
return( NULL );
    }
    if ( type==it_index ) {
	base->clut = gcalloc(1,sizeof(GClut));
	base->clut->trans_index = COLOR_UNKNOWN;
    }
return( gi );
}

int GImageGetWidth(GImage *img) {
    if ( img->list_len==0 ) {
return( img->u.image->width );
    } else {
return( img->u.images[0]->width );
    }
}

int GImageGetHeight(GImage *img) {
    if ( img->list_len==0 ) {
return( img->u.image->height );
    } else {
return( img->u.images[0]->height );
    }
}

void BDFClut(BDFFont *bdf, int linear_scale) {
    int scale = linear_scale*linear_scale, i;
    Color bg = 0xffffff;
    int bgr=COLOR_RED(bg), bgg=COLOR_GREEN(bg), bgb=COLOR_BLUE(bg);
    GClut *clut;

    bdf->clut = clut = gcalloc(1,sizeof(GClut));
    clut->clut_len = scale;
    clut->is_grey = (bgr==bgg && bgb==bgr);
    clut->trans_index = -1;
    for ( i=0; i<scale; ++i ) {
	clut->clut[i] =
		COLOR_CREATE( bgr- (i*(bgr))/(scale-1),
				bgg- (i*(bgg))/(scale-1),
				bgb- (i*(bgb))/(scale-1));
    }
    clut->clut[scale-1] = 0;	/* avoid rounding errors */
}

GImage *ImageAlterClut(GImage *image) { return NULL; }

int BDFDepth(BDFFont *bdf) {
    if ( bdf->clut==NULL )
return( 1 );

return( bdf->clut->clut_len==256 ? 8 :
	bdf->clut->clut_len==16 ? 4 : 2);
}

/* scripts (for opentype) that I understand */

static uint32 scripts[][11] = {
/* Arabic */	{ CHR('a','r','a','b'), 0x0600, 0x06ff, 0xfb50, 0xfdff, 0xfe70, 0xfeff },
/* Armenian */	{ CHR('a','r','m','n'), 0x0530, 0x058f, 0xfb13, 0xfb17 },
/* Bengali */	{ CHR('b','e','n','g'), 0x0980, 0x09ff },
/* Bopomofo */	{ CHR('b','o','p','o'), 0x3100, 0x312f },
/* Braille */	{ CHR('b','r','a','i'), 0x2800, 0x28ff },
/* Byzantine M*/{ CHR('b','y','z','m'), 0x1d000, 0x1d0ff },
/* Canadian Syl*/{CHR('c','a','n','s'), 0x1400, 0x167f },
/* Cherokee */	{ CHR('c','h','e','r'), 0x13a0, 0x13ff },
/* Cyrillic */	{ CHR('c','y','r','l'), 0x0500, 0x052f },
/* Devanagari */{ CHR('d','e','v','a'), 0x0900, 0x097f },
/* Ethiopic */	{ CHR('e','t','h','i'), 0x1300, 0x139f },
/* Georgian */	{ CHR('g','e','o','r'), 0x1080, 0x10ff },
/* Greek */	{ CHR('g','r','e','k'), 0x0370, 0x03ff, 0x1f00, 0x1fff },
/* Gujarati */	{ CHR('g','u','j','r'), 0x0a80, 0x0aff },
/* Gurmukhi */	{ CHR('g','u','r','u'), 0x0a00, 0x0a7f },
/* Hangul */	{ CHR('h','a','n','g'), 0xac00, 0xd7af, 0x3130, 0x319f, 0xffa0, 0xff9f },
 /* I'm not sure what the difference is between the 'hang' tag and the 'jamo' */
 /*  tag. 'Jamo' is said to be the precomposed forms, but what's 'hang'? */
/* CJKIdeogra */{ CHR('h','a','n','i'), 0x3300, 0x9fff, 0xf900, 0xfaff, 0x020000, 0x02ffff },
/* Hebrew */	{ CHR('h','e','b','r'), 0x0590, 0x05ff, 0xfb1e, 0xfb4ff },
#if 0	/* Hiragana used to have its own tag, but has since been merged with katakana */
/* Hiragana */	{ CHR('h','i','r','a'), 0x3040, 0x309f },
#endif
/* Hangul Jamo*/{ CHR('j','a','m','o'), 0x1100, 0x11ff, 0x3130, 0x319f, 0xffa0, 0xffdf },
/* Katakana */	{ CHR('k','a','n','a'), 0x3040, 0x30ff, 0xff60, 0xff9f },
/* Khmer */	{ CHR('k','h','m','r'), 0x1780, 0x17ff },
/* Kannada */	{ CHR('k','n','d','a'), 0x0c80, 0x0cff },
/* Latin */	{ CHR('l','a','t','n'), 0x0000, 0x02af, 0x1d00, 0x1eff, 0xfb00, 0xfb0f, 0xff00, 0xff5f, 0, 0 },
/* Lao */	{ CHR('l','a','o',' '), 0x0e80, 0x0eff },
/* Malayalam */	{ CHR('m','l','y','m'), 0x0d00, 0x0d7f },
/* Mongolian */	{ CHR('m','o','n','g'), 0x1800, 0x18af },
/* Myanmar */	{ CHR('m','y','m','r'), 0x1000, 0x107f },
/* Ogham */	{ CHR('o','g','a','m'), 0x1680, 0x169f },
/* Oriya */	{ CHR('o','r','y','a'), 0x0b00, 0x0b7f },
/* Runic */	{ CHR('r','u','n','r'), 0x16a0, 0x16ff },
/* Sinhala */	{ CHR('s','i','n','h'), 0x0d80, 0x0dff },
/* Syriac */	{ CHR('s','y','r','c'), 0x0700, 0x074f },
/* Tamil */	{ CHR('t','a','m','l'), 0x0b80, 0x0bff },
/* Telugu */	{ CHR('t','e','l','u'), 0x0c00, 0x0c7f },
/* Thaana */	{ CHR('t','h','a','a'), 0x0780, 0x07bf },
/* Thai */	{ CHR('t','h','a','i'), 0x0e00, 0x0e7f },
/* Tibetan */	{ CHR('t','i','b','t'), 0x0f00, 0x0fff },
/* Yi */	{ CHR('y','i',' ',' '), 0xa000, 0xa73f },
		{ 0 }
};

uint32 ScriptFromUnicode(int u,SplineFont *sf) {
    int s, k;
    Encoding *enc;

    if ( u!=-1 ) {
	for ( s=0; scripts[s][0]!=0; ++s ) {
	    for ( k=1; scripts[s][k+1]!=0; k += 2 )
		if ( u>=scripts[s][k] && u<=scripts[s][k+1] )
	    break;
	    if ( scripts[s][k+1]!=0 )
	break;
	}
	if ( scripts[s][0]!=0 )
return( scripts[s][0] );
    }

    if ( sf==NULL )
return( 0 );
    enc = sf->map->enc;
    if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 ) {
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	if ( strmatch(sf->ordering,"Identity")==0 )
return( 0 );
	else if ( strmatch(sf->ordering,"Korean")==0 )
return( CHR('j','a','m','o'));
	else
return( CHR('h','a','n','i') );
    }


return( 0 );
}

uint32 SCScriptFromUnicode(SplineChar *sc) {
    PST *pst;
    SplineFont *sf;
    int i;

    if ( sc==NULL )
return( 0 );

    sf = sc->parent;
    if ( sc->unicodeenc!=-1 )
return( ScriptFromUnicode( sc->unicodeenc,sf ));
    if ( sf==NULL )
return( 0 );

    if ( sf->cidmaster ) sf=sf->cidmaster;
    for ( i=0; i<2; ++i ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->script_lang_index!=0xffff ) {
	    if ( i==1 || sf->script_lang[pst->script_lang_index][1].script==0 )
return( sf->script_lang[pst->script_lang_index]->script );
	}
    }
return( ScriptFromUnicode( sc->unicodeenc,sf ));
}

int SCRightToLeft(SplineChar *sc) {
    uint32 script;

    if ( sc->unicodeenc>=0x10800 && sc->unicodeenc<=0x10fff )
return( true );		/* Supplemental Multilingual Plane, RTL scripts */

    script = SCScriptFromUnicode(sc);
return( script==CHR('a','r','a','b') || script==CHR('h','e','b','r') );
}

int SFAddScriptLangIndex(SplineFont *sf,uint32 script,uint32 lang) {
    int i;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    if ( script==0 ) script=DEFAULT_SCRIPT;
    if ( lang==0 ) lang=DEFAULT_LANG;
    if ( sf->script_lang==NULL )
	sf->script_lang = gcalloc(2,sizeof(struct script_record *));
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	if ( sf->script_lang[i][0].script==script && sf->script_lang[i][1].script==0 &&
		sf->script_lang[i][0].langs[0]==lang &&
		sf->script_lang[i][0].langs[1]==0 )
return( i );
    }
    sf->script_lang = grealloc(sf->script_lang,(i+2)*sizeof(struct script_record *));
    sf->script_lang[i] = gcalloc(2,sizeof(struct script_record));
    sf->script_lang[i][0].script = script;
    sf->script_lang[i][0].langs = galloc(2*sizeof(uint32));
    sf->script_lang[i][0].langs[0] = lang;
    sf->script_lang[i][0].langs[1] = 0;
    sf->script_lang[i+1] = NULL;
return( i );
}

int SFFindBiggestScriptLangIndex(SplineFont *sf,uint32 script,uint32 lang) {
return( SFAddScriptLangIndex(sf,script,lang));
}

#if HANYANG
void SFDDumpCompositionRules(FILE *sfd,struct compositionrules *rules) {
}

struct compositionrules *SFDReadCompositionRules(FILE *sfd) {
    char buffer[200];
    while ( fgets(buffer,sizeof(buffer),sfd)!=NULL )
	if ( strstr(buffer, "EndCompositionRules")!=NULL )
    break;
return( NULL );
}
#endif

void SFConvertToOrder2(SplineFont *_sf) {}
void SFConvertToOrder3(SplineFont *_sf) {}

static int UnicodeContainsCombiners(int uni) {

    if ( uni<0 || uni>0xffff )
return( -1 );
return( false );
}

uint16 PSTDefaultFlags(enum possub_type type,SplineChar *sc ) {
    uint16 flags = 0;

    if ( sc!=NULL ) {
	if ( SCRightToLeft(sc))
	    flags = pst_r2l;
	if ( type==pst_ligature ) {
	    int script = SCScriptFromUnicode(sc);
	    if ( script==CHR('h','e','b','r') || script==CHR('a','r','a','b')) {
		if ( !UnicodeContainsCombiners(sc->unicodeenc))
		    flags |= pst_ignorecombiningmarks;
	    }
	}
    }
return( flags );
}

void MinimumDistancesFree(MinimumDistance *md) {
    MinimumDistance *next;

    while ( md!=NULL ) {
	next = md->next;
	chunkfree(md,sizeof(MinimumDistance));
	md = next;
    }
}

void DStemInfosFree(DStemInfo *h) {
    DStemInfo *hnext;

    for ( ; h!=NULL; h = hnext ) {
	hnext = h->next;
	chunkfree(h,sizeof(DStemInfo));
    }
}

void AnchorPointsFree(AnchorPoint *ap) {
    AnchorPoint *anext;
    for ( ; ap!=NULL; ap = anext ) {
	anext = ap->next;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	free(ap->xadjust.corrections);
	free(ap->yadjust.corrections);
#endif
	chunkfree(ap,sizeof(AnchorPoint));
    }
}

#ifdef FONTFORGE_CONFIG_TYPE3
void SFSplinesFromLayers(SplineFont *sf,int flag) {
}
#endif

void AltUniFigure(SplineFont *sf,EncMap *map) {
}

void AltUniAdd(SplineChar *sc,int uni) {
    struct altuni *altuni;

    if ( sc!=NULL && uni!=-1 && uni!=sc->unicodeenc ) {
	for ( altuni = sc->altuni; altuni!=NULL && altuni->unienc!=uni; altuni=altuni->next );
	if ( altuni==NULL ) {
	    altuni = chunkalloc(sizeof(struct altuni));
	    altuni->next = sc->altuni;
	    sc->altuni = altuni;
	    altuni->unienc = uni;
	}
    }
}

void MMMatchGlyphs(MMSet *mm) {
}

void GWidgetError8(const char *title,const char *statement, ...) {
    va_list ap;

    va_start(ap, statement);
    vfprintf(stderr, statement, ap);
}

#ifdef FONTFORGE_CONFIG_GTK
# include <gtk/gtk.h>

static char *gethomedir(void) {
    static char *dir=NULL;

    if ( dir!=NULL )
return( dir );
return( dir = g_get_home_dir());
}
#else 
# include <pwd.h>

static char *gethomedir(void) {
    static char *dir;
    int uid;
    struct passwd *pw;

	dir = getenv("HOME");
    if ( dir!=NULL )
return( strdup(dir) );

    uid = getuid();
    while ( (pw=getpwent())!=NULL ) {
	if ( pw->pw_uid==uid ) {
	    dir = strdup(pw->pw_dir);
	    endpwent();
return( dir );
	}
    }
    endpwent();

return( NULL );
}
#endif

char *getPfaEditDir(char *buffer) {
    char *dir=gethomedir();

    if ( dir==NULL )
return( NULL );
    sprintf(buffer,"%s/.PfaEdit", dir);
    free(dir);
    if ( access(buffer,F_OK)==-1 )
	if ( mkdir(buffer,0700)==-1 )
return( NULL );
    dir = strdup(buffer);
return( dir );
}

SplineChar *SFGetChar(SplineFont *sf, int unienc, const char *name ) {
return( NULL );
}

int SPInterpolate(SplinePoint *sp) {
    /* Using truetype rules, can we interpolate this point? */
return( !sp->dontinterpolate && !sp->nonextcp && !sp->noprevcp &&
	    !sp->roundx && !sp->roundy &&
	    (RealWithin(sp->me.x,(sp->nextcp.x+sp->prevcp.x)/2,.1) &&
	     RealWithin(sp->me.y,(sp->nextcp.y+sp->prevcp.y)/2,.1)) );
}

int CanonicalCombiner(int uni) {
return( uni );
}
