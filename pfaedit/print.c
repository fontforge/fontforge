/* Copyright (C) 2001 by George Williams */
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
#include "pfaeditui.h"
#include <math.h>
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <ustring.h>

/* ************************************************************************** */
/* ************************* Code for full font dump ************************ */
/* ************************************************************************** */

typedef struct printinfo {
    SplineFont *sf;
    SplineChar *sc;
    int pointsize;
    int extrahspace, extravspace;
    FILE *out;
    FILE *fontfile;
    char psfontname[300];
    unsigned int showvm: 1;
    unsigned int twobyte: 1;
    unsigned int overflow: 1;
    int ypos;
    int max;		/* max chars per line */
    int chline;		/* High order bits of characters we're outputting */
    int page;
    int lastbase;
    double xoff, yoff, scale;
} PI;

static void dump_prologue(PI *pi) {
    time_t now;
    struct passwd *pwd;
    char *pt;
    int ch, i, j, base;

    fprintf( pi->out, "%%!PS-Adobe-3.0\n" );
    fprintf( pi->out, "%%%%BoundingBox: 40 20 580 770\n" );
    fprintf( pi->out, "%%%%Creator: PfaEdit\n" );
    time(&now);
    fprintf( pi->out, "%%%%CreationDate: %s", ctime(&now) );
    fprintf( pi->out, "%%%%DocumentData: Clean7Bit\n" );
/* Can all be commented out if no pwd routines */
    pwd = getpwuid(getuid());
    if ( pwd!=NULL && pwd->pw_gecos!=NULL && *pwd->pw_gecos!='\0' )
	fprintf( pi->out, "%%%%For: %s\n", pwd->pw_gecos );
    else if ( pwd!=NULL && pwd->pw_name!=NULL && *pwd->pw_name!='\0' )
	fprintf( pi->out, "%%%%For: %s\n", pwd->pw_name );
    else if ( (pt=getenv("USER"))!=NULL )
	fprintf( pi->out, "%%%%For: %s\n", pt );
    endpwent();
/* End pwd section */
    fprintf( pi->out, "%%%%LanguageLevel: 2\n" );
    fprintf( pi->out, "%%%%Orientation: Portrait\n" );
    fprintf( pi->out, "%%%%Pages: atend\n" );
    if ( pi->sc==NULL ) {
	fprintf( pi->out, "%%%%Title: Font Display for %s\n", pi->sf->fullname );
	fprintf( pi->out, "%%%%DocumentSuppliedResources: font %s\n", pi->sf->fontname );
    } else
	fprintf( pi->out, "%%%%Title: Character Display for %s from %s\n", pi->sc->name, pi->sf->fullname );
    fprintf( pi->out, "%%%%DocumentNeededResources: font Times-Bold\n" );
    fprintf( pi->out, "%%%%EndComments\n\n" );

    fprintf( pi->out, "%%%%BeginSetup\n" );
/*    fprintf( pi->out, "<< /PageSize [612 792] >> setpagedevice\n" );	/* US Letter */
/*    fprintf( pi->out, "<< /PageSize [595 841] >> setpagedevice\n" );	/* A4 210x297mm */
    fprintf( pi->out, "<< /PageSize [595 792] >> setpagedevice\n" );	/* Minimum of Letter,A4 */

    fprintf( pi->out, "%% <font> <encoding> font_remap <font>	; from the cookbook\n" );
    fprintf( pi->out, "/reencodedict 5 dict def\n" );
    fprintf( pi->out, "/font_remap { reencodedict begin\n" );
    fprintf( pi->out, "  /newencoding exch def\n" );
    fprintf( pi->out, "  /basefont exch def\n" );
    fprintf( pi->out, "  /newfont basefont  maxlength dict def\n" );
    fprintf( pi->out, "  basefont {\n" );
    fprintf( pi->out, "    exch dup dup /FID ne exch /Encoding ne and\n" );
    fprintf( pi->out, "	{ exch newfont 3 1 roll put }\n" );
    fprintf( pi->out, "	{ pop pop }\n" );
    fprintf( pi->out, "    ifelse\n" );
    fprintf( pi->out, "  } forall\n" );
    fprintf( pi->out, "  newfont /Encoding newencoding put\n" );
    fprintf( pi->out, "  /foo newfont definefont	%%Leave on stack\n" );
    fprintf( pi->out, "  end } def\n" );
    fprintf( pi->out, "/n_show { moveto show } bind def\n" );

    fprintf( pi->out, "%%%%IncludeResource: font Times-Bold\n" );
    fprintf( pi->out, "/MyFontDict 100 dict def\n" );
    fprintf( pi->out, "/Times-Bold-ISO-8859-1 /Times-Bold findfont ISOLatin1Encoding font_remap definefont\n" );
    fprintf( pi->out, "MyFontDict /Times-Bold__12 /Times-Bold-ISO-8859-1 findfont 12 scalefont put\n" );

    if ( pi->fontfile!=NULL ) {
	fprintf( pi->out, "%%%%BeginResource: font %s\n", pi->sf->fontname );
	if ( pi->showvm )
	    fprintf( pi->out, " vmstatus pop /VM1 exch def pop\n" );
	while ( (ch=getc(pi->fontfile))!=EOF )
	    putc(ch,pi->out);
	fprintf( pi->out, "%%%%EndResource\n" );
	sprintf(pi->psfontname,"%s__%d", pi->sf->fontname, pi->pointsize );
	fprintf(pi->out,"MyFontDict /%s /%s findfont %d scalefont put\n", pi->psfontname, pi->sf->fontname, pi->pointsize );
	if ( pi->showvm )
	    fprintf( pi->out, "vmstatus pop dup VM1 sub (Max VMusage: ) print == flush\n" );

	/* Now see if there are any unencoded characters in the font, and if so */
	/*  reencode enough fonts to display them all. These will all be 256 char fonts */
	if ( pi->twobyte )
	    i = 65536;
	else
	    i = 256;
	for ( ; i<pi->sf->charcnt; ++i ) {
	    if ( SCWorthOutputting(pi->sf->chars[i]) ) {
		base = i&~0xff;
		fprintf( pi->out, "MyFontDict /%s-%x__%d /%s-%x /%s%s findfont [\n",
			pi->sf->fontname, base>>8, pi->pointsize,
			pi->sf->fontname, base>>8,
			pi->sf->fontname, pi->twobyte?"Base":"" );
		for ( j=0; j<0x100 && base+j<pi->sf->charcnt; ++j )
		    if ( SCWorthOutputting(pi->sf->chars[base+j]))
			fprintf( pi->out, "\t/%s\n", pi->sf->chars[base+j]->name );
		    else
			fprintf( pi->out, "\t/.notdef\n" );
		for ( ;j<0x100; ++j )
		    fprintf( pi->out, "\t/.notdef\n" );
		fprintf( pi->out, " ] font_remap definefont %d scalefont put\n",
			pi->pointsize );
		i = base+0xff;
	    }
	}
    }

    fprintf( pi->out, "%%%%EndSetup\n\n" );
}

static void endpage(PI *pi ) {
    fprintf(pi->out,"showpage cleartomark restore\t\t%%End of Page\n" );
}

static void dump_trailer(PI *pi) {
    if ( pi->page!=0 )
	endpage(pi);
    fprintf( pi->out, "%%%%Trailer\n" );
    fprintf( pi->out, "%%%%Pages: %d\n", pi->page );
    fprintf( pi->out, "%%%%EOF\n" );
}

static void startpage(PI *pi ) {
    int i;
    /* I used to have a progress indicator here showing pages. But they went */
    /*  by so fast that even for CaslonRoman it was pointless */

    if ( pi->page!=0 )
	endpage(pi);
    ++pi->page;
    fprintf(pi->out,"%%%%Page: %d %d\n", pi->page, pi->page );
    fprintf(pi->out,"%%%%PageResources: font Times-Bold font %s\n", pi->sf->fontname );
    fprintf(pi->out,"save mark\n" );
    fprintf(pi->out,"54 738 translate\n" );
    fprintf(pi->out,"MyFontDict /Times-Bold__12 get setfont\n" );
    fprintf(pi->out,"(Font Display for %s) 193.2 -10.92 n_show\n", pi->sf->fontname);

    for ( i=0; i<pi->max; ++i )
	fprintf(pi->out,"(%X) %d -54.84 n_show\n", i, 60+(pi->pointsize+pi->extrahspace)*i );
    pi->ypos = -76;
}

static int DumpLine(PI *pi) {
    int i, line;

    /* First find the next line with stuff on it */
    for ( line = pi->chline ; line<pi->sf->charcnt; line += pi->max ) {
	for ( i=0; i<pi->max && line+i<pi->sf->charcnt; ++i )
	    if ( SCWorthOutputting(pi->sf->chars[line+i]) )
	break;
	if ( i!=pi->max )
    break;
    }
    if ( line+i>=pi->sf->charcnt )		/* Nothing more */
return(0);

    if ( (pi->twobyte && line>=65536) || ( !pi->twobyte && line>=256 ) ) {
	/* Nothing more encoded. Can't use the normal font, must use one of */
	/*  the funky reencodings we built up at the beginning */
	if ( pi->lastbase!=(line>>8) ) {
	    if ( !pi->overflow ) {
		fprintf( pi->out, "%d %d moveto %d %d rlineto stroke\n",
		    100, pi->ypos+8*pi->pointsize/10-1, 300, 0 );
		pi->ypos -= 4;
	    }
	    pi->overflow = true;
	    pi->lastbase = (line>>8);
	    sprintf(pi->psfontname,"%s-%x__%d", pi->sf->fontname, pi->lastbase,
		    pi->pointsize );
	}
    }

    if ( pi->chline==0 ) {
	/* start the first page */
	startpage(pi);
    } else {
	/* start subsequent pages by displaying the one before */
	if ( pi->ypos - pi->pointsize < -720 ) {
	    startpage(pi);
	}
    }
    pi->chline = line;

    if ( !pi->overflow ) {
	fprintf(pi->out,"MyFontDict /Times-Bold__12 get setfont\n" );
	fprintf(pi->out,"(%04X) 26.88 %d n_show\n", pi->chline, pi->ypos );
    }
    fprintf(pi->out,"MyFontDict /%s get setfont\n", pi->psfontname );
    for ( i=0; i<pi->max ; ++i ) {
	if ( i+pi->chline<pi->sf->charcnt && SCWorthOutputting(pi->sf->chars[i+pi->chline])) {
	    if ( pi->overflow ) {
		fprintf( pi->out, "<%02x> %d %d n_show\n", pi->chline +i-(pi->lastbase<<8),
			56+26*i, pi->ypos );
	    } else if ( pi->twobyte && !pi->overflow ) {
		fprintf( pi->out, "<%04x> %d %d n_show\n", pi->chline +i,
			56+(pi->extrahspace+pi->pointsize)*i, pi->ypos );
	    } else {
		fprintf( pi->out, "<%02x> %d %d n_show\n", pi->chline +i,
			56+26*i, pi->ypos );
	    }
	}
    }
    pi->ypos -= pi->pointsize+pi->extravspace;
    pi->chline += pi->max;
return(true);
}

static void SFDumpFontList(SplineFont *sf,char *tofile,int pointsize) {
    PI pi;
    static unichar_t print[] = { 'P','r','i','n','t','i','n','g',' ','f','o','n','t',  '\0' };
    static unichar_t saveps[] = { 'G','e','n','e','r','a','t','i','n','g',' ','P','o','s','t','s','c','r','i','p','t',' ','F','o','n','t', '\0' };

    memset(&pi,'\0',sizeof(pi));
    pi.sf = sf;
    pi.pointsize = pointsize;
    pi.extravspace = pointsize/6;
    pi.extrahspace = pointsize/3;
    pi.max = 480/(pi.extrahspace+pointsize);
    if ( pi.max>=16 ) pi.max = 16;
    else if ( pi.max>=8 ) pi.max = 8;
    else pi.max = 4;
    pi.twobyte = sf->encoding_name>=e_first2byte;
    pi.fontfile = tmpfile();
    if ( pi.fontfile==NULL ) {
	GDrawError("Failed to open temporary output file" );
return;
    }
    pi.out = fopen(tofile,"w");
    if ( pi.out==NULL ) {
	GDrawError("Failed to open file %s for output", tofile );
	fclose(pi.fontfile);
return;
    }
    GProgressStartIndicator(10,print,print, saveps,sf->charcnt,1);
    GProgressEnableStop(false);
    if ( !_WritePSFont(pi.fontfile,sf,pi.twobyte?ff_ptype0:ff_pfa)) {
	GProgressEndIndicator();
	GDrawError("Failed to generate postscript font" );
	fclose(pi.fontfile); fclose(pi.out);
return;
    }
    GProgressEndIndicator();
    rewind(pi.fontfile);
    dump_prologue(&pi);
    fclose(pi.fontfile); pi.fontfile = NULL;

    while ( DumpLine(&pi))
	;

    if ( pi.chline==0 )
	GDrawError( "Warning: Font contained no characters" );
    else
	dump_trailer(&pi);
    if ( ferror(pi.out) || fclose(pi.out)!=0 )
	GDrawError("Failed to generate postscript in file %s", tofile );
}

void SFPrintFontList(SplineFont *sf) {
    unichar_t *ret;
    char *file;
    static unichar_t title[] = { 'P','r','i','n','t',' ','t','o',' ','F','i','l','e','.','.','.',  '\0' };
    static unichar_t filter[] = { '*','.','p','s',  '\0' };
    char buf[100];
    unichar_t ubuf[100];

    sprintf(buf,"pr-%.90s.ps", sf->fontname );
    uc_strcpy(ubuf,buf);
    ret = GWidgetSaveAsFile(title,ubuf,filter,NULL);
    if ( ret==NULL )
return;
    file = cu_copy(ret);
    free(ret);
    SFDumpFontList(sf,file,20);
    free(file);
}

/* ************************************************************************** */
/* ********************* Code for single character dump ********************* */
/* ************************************************************************** */

static void PIDumpSPL(PI *pi,SplinePointList *spl) {
    SplinePoint *first, *sp;

    for ( ; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( sp = spl->first; ; sp=sp->next->to ) {
	    if ( first==NULL )
		fprintf( pi->out, "%g %g moveto\n", sp->me.x*pi->scale+pi->xoff, sp->me.y*pi->scale+pi->yoff );
	    else if ( sp->prev->islinear )
		fprintf( pi->out, " %g %g lineto\n", sp->me.x*pi->scale+pi->xoff, sp->me.y*pi->scale+pi->yoff );
	    else
		fprintf( pi->out, " %g %g %g %g %g %g curveto\n",
			sp->prev->from->nextcp.x*pi->scale+pi->xoff, sp->prev->from->nextcp.y*pi->scale+pi->yoff,
			sp->prevcp.x*pi->scale+pi->xoff, sp->prevcp.y*pi->scale+pi->yoff,
			sp->me.x*pi->scale+pi->xoff, sp->me.y*pi->scale+pi->yoff );
	    if ( sp==first )
	break;
	    if ( first==NULL ) first = sp;
	    if ( sp->next==NULL )
	break;
	}
	fprintf( pi->out, "closepath\n" );
    }
    fprintf( pi->out, "fill\n" );
}

static void _SCPrintCharacter(SplineChar *sc,char *tofile) {
    PI pi;
    DBounds b, page;
    double scalex, scaley;
    RefChar *r;

    memset(&pi,'\0',sizeof(pi));
    pi.sf = sc->parent;
    pi.sc = sc;
    pi.out = fopen(tofile,"w");
    if ( pi.out==NULL ) {
	GDrawError("Failed to open file %s for output", tofile );
	fclose(pi.fontfile);
return;
    }
    dump_prologue(&pi);
    ++pi.page;
    fprintf(pi.out,"%%%%Page: %d %d\n", pi.page, pi.page );
    fprintf(pi.out,"%%%%PageResources: font Times-Bold\n", pi.sf->fontname );
    fprintf(pi.out,"save mark\n" );

    SplineCharFindBounds(sc,&b);
    if ( b.maxy<sc->parent->ascent+5 ) b.maxy = sc->parent->ascent + 5;
    if ( b.miny>-sc->parent->descent ) b.miny =-sc->parent->descent - 5;
    if ( b.minx>00 ) b.minx = -5;
    if ( b.maxx<=0 ) b.maxx = 5;
    if ( b.maxx<=sc->width+5 ) b.maxx = sc->width+5;

    /* From the default bounding box */
    page.minx = 40; page.maxx = 580;
    page.miny = 20; page.maxy = 770;

    fprintf(pi.out,"MyFontDict /Times-Bold__12 get setfont\n" );
    fprintf(pi.out,"(%s from %s) 80 758 n_show\n", sc->name, sc->parent->fullname );
    page.maxy = 750;

    scalex = (page.maxx-page.minx)/(b.maxx-b.minx);
    scaley = (page.maxy-page.miny)/(b.maxy-b.miny);
    pi.scale = (scalex<scaley)?scalex:scaley;
    pi.xoff = page.minx - b.minx*pi.scale;
    pi.yoff = page.miny - b.miny*pi.scale;

    fprintf(pi.out,".2 setlinewidth\n" );
    fprintf(pi.out,"%g %g moveto %g %g lineto stroke\n", page.minx, pi.yoff, page.maxx, pi.yoff );
    fprintf(pi.out,"%g %g moveto %g %g lineto stroke\n", pi.xoff, page.miny, pi.xoff, page.maxy );
    fprintf(pi.out,"%g %g moveto %g %g lineto stroke\n", page.minx, sc->parent->ascent*pi.scale+pi.yoff, page.maxx, sc->parent->ascent*pi.scale+pi.yoff );
    fprintf(pi.out,"%g %g moveto %g %g lineto stroke\n", page.minx, -sc->parent->descent*pi.scale+pi.yoff, page.maxx, -sc->parent->descent*pi.scale+pi.yoff );
    fprintf(pi.out,"%g %g moveto %g %g lineto stroke\n", pi.xoff+sc->width*pi.scale, page.miny, pi.xoff+sc->width*pi.scale, page.maxy );

    PIDumpSPL(&pi,sc->splines);
    for ( r=sc->refs; r!=NULL; r=r->next )
	PIDumpSPL(&pi,r->splines);

    dump_trailer(&pi);

    if ( ferror(pi.out) || fclose(pi.out)!=0 )
	GDrawError("Failed to generate postscript in file %s", tofile );
}

void SCPrintCharacter(SplineChar *sc) {
    unichar_t *ret;
    char *file;
    static unichar_t title[] = { 'P','r','i','n','t',' ','t','o',' ','F','i','l','e','.','.','.',  '\0' };
    static unichar_t filter[] = { '*','.','p','s',  '\0' };
    char buf[100];
    unichar_t ubuf[100];

    if ( sc==NULL )
return;
    sprintf(buf,"pr-%.90s.ps", sc->name );
    uc_strcpy(ubuf,buf);
    ret = GWidgetSaveAsFile(title,ubuf,filter,NULL);
    if ( ret==NULL )
return;
    file = cu_copy(ret);
    free(ret);
    _SCPrintCharacter(sc,file);
    free(file);
}
