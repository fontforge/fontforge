/* Copyright (C) 2000-2005 by George Williams */
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
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <gwidget.h>
#include "psfont.h"


struct pdfcontext {
    char *tokbuf;
    int tblen;
    FILE *pdf;
    struct psdict pdfdict;
    long *objs;
    int ocnt;
    long *fonts;
    char **fontnames;		/* theoretically in utf-8 */
    int fcnt;
};

static long FindXRef(FILE *pdf) {
    int ch;
    long xrefpos;

    fseek(pdf,-5-2-8-2-10-2,SEEK_END);
    forever {
	while ( (ch=getc(pdf))!=EOF ) {
	    if ( ch=='s' )
	break;
	}
	if ( ch==EOF )
return( -1 );
	while ( ch=='s' ) {
	    if ( (ch=getc(pdf))!='t' )
	continue;
	    if ( (ch=getc(pdf))!='a' )
	continue;
	    if ( (ch=getc(pdf))!='r' )
	continue;
	    if ( (ch=getc(pdf))!='t' )
	continue;
	    if ( (ch=getc(pdf))!='x' )
	continue;
	    if ( (ch=getc(pdf))!='r' )
	continue;
	    if ( (ch=getc(pdf))!='e' )
	continue;
	    if ( (ch=getc(pdf))!='f' )
	continue;
	    if ( fscanf(pdf,"%ld",&xrefpos)!=1 )
return( -1 );

return( xrefpos );
	}
    }
}

static long *FindObjects(struct pdfcontext *pc) {
    FILE *pdf = pc->pdf;
    long xrefpos = FindXRef(pdf);
    long *ret=NULL;
    int *gen=NULL;
    int cnt = 0, i, start, num;
    long offset; int gennum; char f;

    if ( xrefpos == -1 )
return( NULL );
    fseek(pdf, xrefpos,SEEK_SET );

    if ( fscanf(pdf, "xref %d %d", &start, &num )!=2 )
return( ret );
    forever {
	if ( start+num>cnt ) {
	    ret = grealloc(ret,(start+num+1)*sizeof(long));
	    memset(ret+cnt,-1,sizeof(long)*(start+num-cnt));
	    gen = grealloc(gen,(start+num)*sizeof(int));
	    memset(gen+cnt,-1,sizeof(int)*(start+num-cnt));
	    cnt = start+num;
	    pc->ocnt = cnt;
	    ret[cnt] = -2;
	}
	for ( i=start; i<start+num; ++i ) {
	    if ( fscanf(pdf,"%ld %d %c", &offset, &gennum, &f )!=3 )
return( ret );
	    if ( f=='f' ) {
		if ( gennum > gen[i] ) {
		    ret[i] = -1;
		    gen[i] = gennum;
		}
	    } else if ( f=='n' ) {
		if ( gennum > gen[i] ) {
		    ret[i] = offset;
		    gen[i] = gennum;
		}
	    } else
return( ret );
	}
	if ( fscanf(pdf, "%d %d", &start, &num )!=2 )
return( ret );
    }
}

#define pdf_space(ch) (ch=='\0' || ch=='\t' || ch=='\n' || ch=='\r' || ch=='\f' || ch==' ' )
#define pdf_oper(ch) (ch=='(' || ch==')' || ch=='<' || ch=='>' || ch=='[' || ch==']' || ch=='{' || ch=='}' || ch=='/' || ch=='%' )

static int pdf_peekch(FILE *pdf) {
    int ch = getc(pdf);
    ungetc(ch,pdf);
return( ch );
}

static void pdf_skipwhitespace(struct pdfcontext *pc) {
    FILE *pdf = pc->pdf;
    int ch;

    forever {
	ch = getc(pdf);
	if( ch=='%' ) {
	    while ( (ch=getc(pdf))!=EOF && ch!='\n' && ch!='\r' );
	} else if ( !pdf_space(ch) )
    break;
    }
    ungetc(ch,pdf);
}

static char *pdf_getname(struct pdfcontext *pc) {
    FILE *pdf = pc->pdf;
    int ch;
    char *pt = pc->tokbuf, *end = pc->tokbuf+pc->tblen;

    pdf_skipwhitespace(pc);
    ch = getc(pdf);
    if ( ch!='/' ) {
	ungetc(ch,pdf);
return( NULL );
    }
    for ( ch=getc(pdf) ;; ch=getc(pdf) ) {
	if ( pt>=end ) {
	    char *temp = grealloc(pc->tokbuf,(pc->tblen+=300));
	    pt = temp + (pt-pc->tokbuf);
	    pc->tokbuf = temp;
	    end = temp+pc->tblen;
	}
	if ( pdf_space(ch) || pdf_oper(ch) ) {
	    ungetc(ch,pdf);
	    *pt = '\0';
return( pc->tokbuf );
	}
	*pt++ = ch;
    }
}

static char *pdf_getdictvalue(struct pdfcontext *pc) {
    FILE *pdf = pc->pdf;
    int ch;
    char *pt = pc->tokbuf, *end = pc->tokbuf+pc->tblen;
    int dnest=0, anest=0, strnest;

    pdf_skipwhitespace(pc);
    ch = getc(pdf);
    forever {
	if ( pt>=end ) {
	    char *temp = grealloc(pc->tokbuf,(pc->tblen+=300));
	    pt = temp + (pt-pc->tokbuf);
	    pc->tokbuf = temp;
	    end = temp+pc->tblen;
	}
	*pt++ = ch;
	if ( ch=='(' ) {
	    strnest = 0;
	    while ( (ch=getc(pdf))!=EOF ) {
		if ( pt>=end ) {
		    char *temp = grealloc(pc->tokbuf,(pc->tblen+=300));
		    pt = temp + (pt-pc->tokbuf);
		    pc->tokbuf = temp;
		    end = temp+pc->tblen;
		}
		*pt++ = ch;
		if ( ch=='(' ) ++strnest;
		else if ( ch==')' && strnest==0 )
	    break;
		else if ( ch==')' ) --strnest;
	    }
	} else if ( ch=='[' )
	    ++ anest;
	else if ( ch==']' && anest>0 )
	    -- anest;
	else if ( ch=='<' && pdf_peekch(pdf)=='<' )
	    ++dnest;
	else if ( ch=='>' && pdf_peekch(pdf)=='>' ) {
	    if ( dnest==0 ) {
		ungetc(ch,pdf);
		pt[-1] = '\0';
		if ( pt>pc->tokbuf+1 && pt[-2]==' ' ) pt[-2] = '\0';
return( pc->tokbuf );
	    }
	    --dnest;
	} else if ( ch=='/' && anest==0 && dnest==0 && pt!=pc->tokbuf+1 ) {
	    /* A name token may be a value if it is the first thing */
	    /*  otherwise it is the start of the next key */
	    ungetc(ch,pdf);
	    pt[-1] = '\0';
	    if ( pt>pc->tokbuf+1 && pt[-2]==' ' ) pt[-2] = '\0';
return( pc->tokbuf );
	} else if ( ch=='%' || pdf_space(ch) ) {
	    pt[-1] = ' ';
	    ungetc(ch,pdf);
	    pdf_skipwhitespace(pc);
	} else if ( ch==EOF ) {
	    pt[-1] = '\0';
return( pc->tokbuf );
	}
	ch = getc(pdf);
    }
}   

static void PSDictClear(struct psdict *dict) {
    int i;

    for ( i=0; i<dict->next; ++i ) {
	free(dict->keys[i]);
	free(dict->values[i]);
    }
    dict->next = 0;
}

static int pdf_readdict(struct pdfcontext *pc) {
    FILE *pdf = pc->pdf;
    char *key, *value;
    int ch;

    PSDictClear(&pc->pdfdict);

    pdf_skipwhitespace(pc);
    ch = getc(pdf);
    if ( ch!='<' || pdf_peekch(pdf)!='<' )
return( false );
    getc(pdf);		/* Eat the second '<' */

    forever {
	key = copy(pdf_getname(pc));
	if ( key==NULL )
return( true );
	value = copy(pdf_getdictvalue(pc));
	if ( value==NULL || strcmp(value,"null")==0 )
	    free(key);
	else {
	    if ( pc->pdfdict.next>=pc->pdfdict.cnt ) {
		pc->pdfdict.keys = grealloc(pc->pdfdict.keys,(pc->pdfdict.cnt+=20)*sizeof(char *));
		pc->pdfdict.values = grealloc(pc->pdfdict.values,pc->pdfdict.cnt*sizeof(char *));
	    }
	    pc->pdfdict.keys  [pc->pdfdict.next] = key  ;
	    pc->pdfdict.values[pc->pdfdict.next] = value;
	    ++pc->pdfdict.next;
	}
    }
}

static void pdf_skipobjectheader(struct pdfcontext *pc) {

    fscanf( pc->pdf, "%*d %*d obj" );
}

static int hex(int ch1, int ch2) {
    int val;

    if ( ch1<='9' )
	val = (ch1-'0')<<4;
    else if ( ch1>='a' && ch1<='f' )
	val = (ch1-'a'+10)<<4;
    else
	val = (ch1-'A'+10)<<4;
    if ( ch2<='9' )
	val |= (ch2-'0');
    else if ( ch2>='a' && ch2<='f' )
	val |= (ch2-'a'+10);
    else
	val |= (ch2-'A'+10);
return( val );
}

static int pdf_findfonts(struct pdfcontext *pc) {
    FILE *pdf = pc->pdf;
    int i, k=0;
    char *pt, *tpt;

    pc->fonts = galloc(pc->ocnt*sizeof(long));
    pc->fontnames = galloc(pc->ocnt*sizeof(char *));
    for ( i=1; i<pc->ocnt; ++i ) if ( pc->objs[i]!=-1 ) {	/* Object 0 is always unused */
	fseek(pdf,pc->objs[i],SEEK_SET);
	pdf_skipobjectheader(pc);
	if ( pdf_readdict(pc) ) {
	    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Type"))!=NULL && strcmp(pt,"/Font")==0 &&
		    (PSDictHasEntry(&pc->pdfdict,"FontDescriptor")!=NULL ||
		     ((pt=PSDictHasEntry(&pc->pdfdict,"Subtype"))!=NULL && strcmp(pt,"/Type3")==0)) &&
		    (pt=PSDictHasEntry(&pc->pdfdict,"BaseFont"))!=NULL ) {
		pc->fonts[k] = pc->objs[i];
		if ( *pt=='/' || *pt=='(' )
		    ++pt;
		pc->fontnames[k++] = tpt = copy(pt);
		for ( pt=tpt; *pt; ++pt ) {
		    if ( *pt=='#' && ishexdigit(pt[1]) && ishexdigit(pt[2])) {
			*tpt++ = hex(pt[1],pt[2]);
			pt += 2;
		    } else
			*tpt++ = *pt;
		}
		*tpt = '\0';
	    }
	}
    }
    pc->fcnt = k;
return( k>0 );
}

static int pdf_getinteger(char *pt,struct pdfcontext *pc) {
    int val,ret;
    long here;

    if ( pt==NULL )
return( 0 );
    val = strtol(pt,NULL,10);
    if ( pt[strlen(pt)-1]!='R' )
return( val );
    if ( val<0 || val>=pc->ocnt || pc->objs[val]==-1 )
return( 0 );
    here = ftell(pc->pdf);
    fseek(pc->pdf,pc->objs[val],SEEK_SET);
    pdf_skipobjectheader(pc);
    ret = fscanf(pc->pdf,"%d",&val);
    fseek(pc->pdf,here,SEEK_SET);
    if ( ret!=1 )
return( 0 );
return( val );
}

/* ************************************************************************** */
/* *********************** Simplistic filter decoders *********************** */
/* ************************************************************************** */
static void pdf_hexfilter(FILE *to,FILE *from) {
    int ch1, ch2;

    rewind(from);
    while ( (ch1=getc(from))!=EOF ) {
	while ( !ishexdigit(ch1) && ch1!=EOF ) ch1 = getc(from);
	while ( (ch2=getc(from))!=EOF && !ishexdigit(ch2));
	if ( ch2==EOF )
    break;
	putc(hex(ch1,ch2),to);
    }
}

static void pdf_85filter(FILE *to,FILE *from) {
    int ch1, ch2, ch3, ch4, ch5;
    unsigned int val;
    int cnt;

    rewind(from);
    forever {
	while ( isspace(ch1=getc(from)));
	if ( ch1==EOF || ch1=='~' )
    break;
	if ( ch1=='z' ) {
	    putc(0,to);
	    putc(0,to);
	    putc(0,to);
	    putc(0,to);
	} else {
	    while ( isspace(ch2=getc(from)));
	    while ( isspace(ch3=getc(from)));
	    while ( isspace(ch4=getc(from)));
	    while ( isspace(ch5=getc(from)));
	    cnt = 4;
	    if ( ch3=='~' && ch4=='>' ) {
		cnt=1;
		ch3 = ch4 = ch5 = '!';
	    } else if ( ch4=='~' && ch5=='>' ) {
		cnt = 2;
		ch4 = ch5 = '!';
	    } else if ( ch5=='~' ) {
		cnt = 3;
		ch5 = '!';
	    }
	    val = ((((ch1-'!')*85+ ch2-'!')*85 + ch3-'!')*85 + ch4-'!')*85 + ch5-'!';
	    putc(val>>24,to);
	    if ( cnt>1 )
		putc((val>>16)&0xff,to);
	    if ( cnt>2 )
		putc((val>>8)&0xff,to);
	    if ( cnt>3 )
		putc(val&0xff,to);
	    if ( cnt!=4 )
    break;
	}
    }
}

#ifdef _NO_LIBPNG
static int haszlib(void) {
return( false );
}

static void pdf_zfilter(FILE *to,FILE *from) {
}
#else
# if !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC)	/* I don't know how to deal with dynamic libs on mac OS/X, hence this */
#  include <dynamic.h>
# endif
# include <zlib.h>

# if !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC)

static DL_CONST void *zlib=NULL;
static int (*_inflateInit_)(z_stream *,const char *,int);
static int (*_inflate)(z_stream *,int flags);
static int (*_inflateEnd)(z_stream *);

static int haszlib(void) {
    if ( zlib!=NULL )
return( true );

    if ( (zlib = dlopen("libz" SO_EXT,RTLD_GLOBAL|RTLD_LAZY))==NULL ) {
	LogError("%s", dlerror());
return( false );
    }
    _inflateInit_ = (int (*)(z_stream *,const char *,int)) dlsym(zlib,"inflateInit_");
    _inflate = (int (*)(z_stream *,int )) dlsym(zlib,"inflate");
    _inflateEnd = (int (*)(z_stream *)) dlsym(zlib,"inflateEnd");
    if ( _inflateInit_==NULL || _inflate==NULL || _inflateEnd==NULL ) {
	LogError("%s", dlerror());
	dlclose(zlib); zlib=NULL;
return( false );
    }
return( true );
}

/* Grump. zlib defines this as a macro */
#define _inflateInit(strm) \
        _inflateInit_((strm),                ZLIB_VERSION, sizeof(z_stream))

# else
/* Either statically linked, or loaded at start up */
static int haszlib(void) {
return( true );
}

#define _inflateInit	inflateInit
#define _inflate	inflate
#define _inflateEnd	inflateEnd

# endif /* !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC) */

#define Z_CHUNK	65536
/* Copied with few mods from the zlib howto */
static int pdf_zfilter(FILE *to,FILE *from) {
    char *in;
    char *out;
    z_stream strm;
    int ret;

	/* Initialize */
    rewind(from);
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = _inflateInit(&strm);
    if (ret != Z_OK)
return ret;
    in = galloc(Z_CHUNK); out = galloc(Z_CHUNK);

    do {
	strm.avail_in = fread(in,1,Z_CHUNK,from);
	if ( strm.avail_in==0 )
    break;
	strm.next_in = in;
	do {
	    strm.avail_out = Z_CHUNK;
	    strm.next_out = out;
            ret = _inflate(&strm, Z_NO_FLUSH);
	    if ( ret==Z_NEED_DICT || ret==Z_DATA_ERROR || ret==Z_MEM_ERROR ) {
		(void)_inflateEnd(&strm);
return ret;
	    }
	    fwrite(out,1,Z_CHUNK-strm.avail_out,to);
	} while ( strm.avail_out == 0 );
    } while ( ret != Z_STREAM_END );
    (void)_inflateEnd(&strm);
    free(in); free(out);
return( ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR );
}
#endif /* _NO_LIBPNG */

static void pdf_rlefilter(FILE *to,FILE *from) {
    int ch1, ch2, i;

    rewind(from);
    while ( (ch1=getc(from))!=EOF && ch1!=0x80 ) {	/* 0x80 => EOD */
	if ( ch1<=127 ) {
	    for ( i=0; i<=ch1; ++i ) {	/* copy ch1+1 bytes directly */
		ch2 = getc(from);
		if ( ch2!=EOF )
		    putc(ch2,to);
	    }
	} else {			/* copy the next by 257-ch1 times */
	    ch2 = getc(from);
	    if ( ch2!=EOF )
		for ( i=0; i<257-ch1; ++i )
		    putc(ch2,to);
	}
    }
}

/* Filters I shall support: ASCIIHexDecode ASCII85Decode FlateDecode RunLengthDecode */
static FILE *pdf_defilterstream(struct pdfcontext *pc) {
    /* First copy the stream data into a file. This isn't efficient, but */
    /*  we can live with that */
    /* Then apply each de-filter sequentially reading from one file, writing */
    /*  to another */
    FILE *res, *old, *pdf = pc->pdf;
    int i,length,ch;
    char *pt;

    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Length"))==NULL ) {
	LogError("A pdf stream object is missing a Length attribute");
return( NULL );
    }
    length = pdf_getinteger(pt,pc);

    while ( (ch=getc(pdf))!=EOF && ch!='m' );	/* Skip over >>\nstream */
    if ( (ch=getc(pdf))=='\r' ) ch = getc(pdf);	/* Skip the newline */

    res = tmpfile();
    for ( i=0; i<length; ++i ) {
	if ( (ch=getc(pdf))!=EOF )
	    putc(ch,res);
    }
    rewind(res);

    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Filter"))==NULL )
return( res );
    while ( *pt==' ' || *pt=='[' || *pt==']' || *pt=='/' ) ++pt;	/* Yes, I saw a null array once */
    while ( *pt!='\0' ) {
	old = res;
	res = tmpfile();
	if ( strmatch("ASCIIHexDecode",pt)==0 ) {
	    pdf_hexfilter(res,old);
	    pt += strlen("ASCIIHexDecode");
	} else if ( strmatch("ASCII85Decode",pt)==0 ) {
	    pdf_85filter(res,old);
	    pt += strlen("ASCII85Decode");
	} else if ( strmatch("FlateDecode",pt)==0 && haszlib()) {
	    pdf_zfilter(res,old);
	    pt += strlen("FlateDecode");
	} else if ( strmatch("RunLengthDecode",pt)==0 ) {
	    pdf_rlefilter(res,old);
	    pt += strlen("RunLengthDecode");
	} else {
	    LogError("Unsupported filter: %s", pt );
	    fclose(old); fclose(res);
return( NULL );
	}
	while ( *pt==' ' || *pt==']' || *pt=='/' ) ++pt;
	fclose(old);
    }
return( res );
}
/* ************************************************************************** */
/* ****************************** End filters ******************************* */
/* ************************************************************************** */

static FILE *pdf_insertpfbsections(FILE *file,struct pdfcontext *pc) {
    /* I don't think I need this */
return( file );
}

static SplineFont *pdf_loadtype3(struct pdfcontext *pc) {
    LogError("No support for type3 pdf fonts yet (and this is a type3 font)");
return( NULL );
}

static SplineFont *pdf_loadfont(struct pdfcontext *pc,int font_num) {
    char *pt;
    int fd, type, ff;
    FILE *file;
    SplineFont *sf;

    fseek(pc->pdf,pc->fonts[font_num],SEEK_SET);
    pdf_skipobjectheader(pc);
    if ( !pdf_readdict(pc) )
return( NULL );

    if ( (pt=PSDictHasEntry(&pc->pdfdict,"Subtype"))!=NULL && strcmp(pt,"/Type3")==0 )
return( pdf_loadtype3(pc));

    if ( (pt=PSDictHasEntry(&pc->pdfdict,"FontDescriptor"))==NULL )
  goto fail;
    fd = strtol(pt,NULL,10);
    if ( fd<=0 || fd>=pc->ocnt || pc->objs[fd]==-1 )
  goto fail;

    fseek(pc->pdf,pc->objs[fd],SEEK_SET);
    pdf_skipobjectheader(pc);
    if ( !pdf_readdict(pc) )
  goto fail;

    if ( (pt=PSDictHasEntry(&pc->pdfdict,"FontFile"))!=NULL )
	type = 1;
    else if ( (pt=PSDictHasEntry(&pc->pdfdict,"FontFile2"))!=NULL )
	type = 2;
    else if ( (pt=PSDictHasEntry(&pc->pdfdict,"FontFile3"))!=NULL )
	type = 3;
    else {
	LogError("The font %s is one of the standard fonts. It isn't actually in the file.", pc->fontnames[font_num]);
return( NULL );
    }
    ff = strtol(pt,NULL,10);
    if ( ff<=0 || ff>=pc->ocnt || pc->objs[ff]==-1 )
  goto fail;

    fseek(pc->pdf,pc->objs[ff],SEEK_SET);
    pdf_skipobjectheader(pc);
    if ( !pdf_readdict(pc) )
  goto fail;
    file = pdf_defilterstream(pc);
    if ( file==NULL )
return( NULL );
    rewind(file);
    if ( type==1 ) {
	FontDict *fd;
	file = pdf_insertpfbsections(file,pc);
	fd = _ReadPSFont(file);
	sf = SplineFontFromPSFont(fd);
	PSFontFree(fd);
    } else if ( type==2 ) {
	sf = _SFReadTTF(file,0,pc->fontnames[font_num],NULL);
    } else {
	int len;
	fseek(file,0,SEEK_END);
	len = ftell(file);
	rewind(file);
	sf = _CFFParse(file,len,pc->fontnames[font_num]);
    }
    fclose(file);
return( sf );

  fail:
    LogError("Unable to parse the pdf objects that make up %s",pc->fontnames[font_num]);
return( NULL );
}

static void pcFree(struct pdfcontext *pc) {
    int i;

    PSDictClear(&pc->pdfdict);
    free(pc->objs);
    for ( i=0; i<pc->fcnt; ++i )
	free(pc->fontnames[i]);
    free(pc->fontnames);
    free(pc->fonts);
    free(pc->tokbuf);
}

SplineFont *_SFReadPdfFont(FILE *pdf,char *filename,char *select_this_font) {
    struct pdfcontext pc;
    SplineFont *sf = NULL;
    char *oldloc;
    int i;

    oldloc = setlocale(LC_NUMERIC,"C");
    memset(&pc,0,sizeof(pc));
    pc.pdf = pdf;
    if ( (pc.objs = FindObjects(&pc))==NULL ) {
	LogError("Doesn't look like a valid pdf file, couldn't find xref section" );
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pdf_findfonts(&pc)==0 ) {
	LogError("This pdf file has no fonts");
	pcFree(&pc);
	setlocale(LC_NUMERIC,oldloc);
return( NULL );
    }
    if ( pc.fcnt==1 ) {
	sf = pdf_loadfont(&pc,0);
    } else if ( select_this_font!=NULL ) {
	for ( i=0; i<pc.fcnt; ++i ) {
	    if ( strcmp(pc.fontnames[i],select_this_font)==0 )
	break;
	}
	if ( i<pc.fcnt )
	    sf = pdf_loadfont(&pc,i);
	else
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_NotInCollection,_STR_FontNotInCollection,
		    select_this_font, filename);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Not in Collection"),_("%s is not in %.100s"),
		    select_this_font, filename);
#else
	    ;
#endif
    } else {
	unichar_t **names;
	int choice;
	names = galloc((pc.fcnt+1)*sizeof(unichar_t *));
	for ( i=0; i<pc.fcnt; ++i )
	    names[i] = utf82u_copy(pc.fontnames[i]);
	names[i] = NULL;
#if defined(FONTFORGE_CONFIG_NO_WINDOWING_UI)
	choice = 0;
#elif defined(FONTFORGE_CONFIG_GDRAW)
	if ( no_windowing_ui )
	    choice = 0;
	else
	    choice = GWidgetChoicesR(_STR_PickFont,(const unichar_t **) names,pc.fcnt,0,_STR_MultipleFontsPick);
#else
	if ( no_windowing_ui )
	    choice = 0;
	else
	    choice = gwwv_choose(_("Pick a font, any font..."),(const unichar_t **) names,pc.fcnt,0,_("There are multiple fonts in this file, pick one"));
#endif
	for ( i=0; i<pc.fcnt; ++i )
	    free(names[i]);
	free(names);
	if ( choice!=-1 )
	    sf = pdf_loadfont(&pc,choice);
    }
    setlocale(LC_NUMERIC,oldloc);
    pcFree(&pc);
return( sf );
}

SplineFont *SFReadPdfFont(char *filename) {
    char *pt, *freeme=NULL, *freeme2=NULL, *select_this_font=NULL;
    SplineFont *sf;
    FILE *pdf;

    if ( (pt=strchr(filename,'('))!=NULL ) {
	freeme = filename = copyn(filename,pt-filename);
	freeme2 = select_this_font = copy(pt+1);
	if ( (pt=strchr(select_this_font,')'))!=NULL )
	    *pt = '\0';
    }

    pdf = fopen(filename,"r");
    if ( pdf==NULL )
	sf = NULL;
    else {
	sf = _SFReadPdfFont(pdf,filename,select_this_font);
	fclose(pdf);
    }
    free(freeme); free(freeme2);
return( sf );
}
	
