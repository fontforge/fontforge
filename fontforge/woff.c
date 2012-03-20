/* Copyright (C) 2010-2012 by George Williams */
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

/* Support for woff files */
/* Which are defined here: http://people.mozilla.com/~jkew/woff/woff-2009-09-16.html */
/* Basically sfnts with compressed tables and some more metadata */

#include "fontforge.h"
#include <math.h>
#include <ctype.h>

#ifdef _NO_LIBPNG
SplineFont *_SFReadWOFF(FILE *woff,int flags,enum openflags openflags, char *filename,struct fontdict *fd) {
    ff_post_error(_("WOFF not supported"), _("This version of fontforge cannot handle WOFF files. You need to recompile it with libpng and zlib") );
return( NULL );
}

int _WriteWOFFFont(FILE *woff,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags,EncMap *enc,int layer) {
    ff_post_error(_("WOFF not supported"), _("This version of fontforge cannot handle WOFF files. You need to recompile it with libpng and zlib") );
return( 1 );
}

int WriteWOFFFont(char *fontname,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags,EncMap *enc,int layer) {
    ff_post_error(_("WOFF not supported"), _("This version of fontforge cannot handle WOFF files. You need to recompile it with libpng and zlib") );
return( 1 );
}

int CanWoff(void) {
return( 0 );
}
#else
# if !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC)
#  include <dynamic.h>
# endif
# include <zlib.h>

# if !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC)

static DL_CONST void *zlib=NULL;
static int (*_inflateInit_)(z_stream *,const char *,int);
static int (*_inflate)(z_stream *,int flags);
static int (*_inflateEnd)(z_stream *);
static int (*_deflateInit_)(z_stream *,int level, const char *,int);
static int (*_deflate)(z_stream *,int flags);
static int (*_deflateEnd)(z_stream *);
static int (*_uncompress)(Bytef *, uLongf *, const Bytef *, uLong);
static int (*_compress)(Bytef *, uLongf *, const Bytef *, uLong);

static int haszlib(void) {
    if ( zlib!=NULL )
return( true );

    if ( (zlib = dlopen("libz" SO_EXT,RTLD_GLOBAL|RTLD_LAZY))==NULL ) {
	LogError( "%s", dlerror());
return( false );
    }
    _inflateInit_ = (int (*)(z_stream *,const char *,int)) dlsym(zlib,"inflateInit_");
    _inflate = (int (*)(z_stream *,int )) dlsym(zlib,"inflate");
    _inflateEnd = (int (*)(z_stream *)) dlsym(zlib,"inflateEnd");
    _deflateInit_ = (int (*)(z_stream *,int,const char *,int)) dlsym(zlib,"deflateInit_");
    _deflate = (int (*)(z_stream *,int )) dlsym(zlib,"deflate");
    _deflateEnd = (int (*)(z_stream *)) dlsym(zlib,"deflateEnd");
    _uncompress = (int (*)(Bytef *, uLongf *, const Bytef *, uLong)) dlsym(zlib,"uncompress");
    _compress = (int (*)(Bytef *, uLongf *, const Bytef *, uLong)) dlsym(zlib,"compress");
    if ( _inflateInit_==NULL || _inflate==NULL || _inflateEnd==NULL ||
	    _deflateInit_==NULL || _deflate==NULL || _deflateEnd==NULL ) {
	LogError( "%s", dlerror());
	dlclose(zlib); zlib=NULL;
return( false );
    }
return( true );
}

/* Grump. zlib defines this as a macro */
#define _inflateInit(strm) \
        _inflateInit_((strm),                ZLIB_VERSION, sizeof(z_stream))
#define _deflateInit(strm,level) \
        _deflateInit_((strm),level,          ZLIB_VERSION, sizeof(z_stream))

# else
/* Either statically linked, or loaded at start up */
static int haszlib(void) {
return( true );
}

#define _inflateInit	inflateInit
#define _inflate	inflate
#define _inflateEnd	inflateEnd
#define _deflateInit	deflateInit
#define _deflate	deflate
#define _deflateEnd	deflateEnd
#define _compress	compress
#define _uncompress	uncompress

# endif /* !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC) */

static void copydata(FILE *to,int off_to,FILE *from,int off_from, int len) {
    int ch, i;

    fseek(to  ,off_to  ,SEEK_SET);
    fseek(from,off_from,SEEK_SET);
    for ( i=0; i<len; ++i ) {
	ch = getc(from);
	putc(ch,to);
    }
}

#define CHUNK	(128*1024)
/* Copied with few mods from the zlib usage examples */

static int decompressdata(FILE *to,int off_to,FILE *from,int off_from,
	int len, int uncomplen ) {
    /* Return whether an error occurred */
    char in[CHUNK];
    char out[CHUNK];
    z_stream strm;
    int ret;
    int amount;

    fseek(to  ,off_to  ,SEEK_SET);
    fseek(from,off_from,SEEK_SET);
    memset(&strm,0,sizeof(strm));
    ret = _inflateInit(&strm);
    if ( ret!=Z_OK )
return( true );

    do {
	if ( len<=0 ) {
            (void)_inflateEnd(&strm);
return( true );
        }
	amount = len;
	if ( amount>CHUNK )
	    amount = CHUNK;
        strm.avail_in = fread(in, 1, amount, from);
	len -= strm.avail_in;
        if (ferror(from)) {
            (void)_inflateEnd(&strm);
return( true );
        }
        if (strm.avail_in == 0)
    break;
        strm.next_in = in;
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = _inflate(&strm, Z_NO_FLUSH);
	    if ( ret!=Z_OK && ret!=Z_STREAM_END ) {
		(void)_inflateEnd(&strm);
return( true );
	    }
	    amount = CHUNK - strm.avail_out;
	    if ( fwrite(out,1,amount,to)!= amount || ferror(to) ) {
		(void)_inflateEnd(&strm);
return( true );
	    }
	} while ( strm.avail_out==0 );
    } while ( ret!=Z_STREAM_END );
    (void)_inflateEnd(&strm);
    if ( uncomplen!=strm.total_out ) {
	LogError(_("Decompressed length did not match expected length for table"));
return( true );
    }

return( false );
}

static int compressOrNot(FILE *to,int off_to, FILE *from,int off_from,
	int len, int forcecompress ) {
    char in[CHUNK];
    char out[CHUNK];
    z_stream strm;
    int ret, err=0;
    int amount;
    FILE *tmp;
    int uncompLen = len;

    /* Empty table, nothing to do */
    if ( len==0 )
return(0);

    fseek(from,off_from,SEEK_SET);
    memset(&strm,0,sizeof(strm));
    ret = _deflateInit(&strm,Z_DEFAULT_COMPRESSION);
    if ( ret!=Z_OK ) {
	fprintf( stderr,"Compression initialization failed.\n" );
return(0);
    }
    tmp = tmpfile();

    do {
	if ( len<=0 ) {
            (void)_deflateEnd(&strm);
    break;
        }
	amount = len;
	if ( amount>CHUNK )
	    amount = CHUNK;
        strm.avail_in = fread(in, 1, amount, from);
	len -= strm.avail_in;
        if (ferror(from)) {
            (void)_deflateEnd(&strm);
	    fprintf( stderr, "IO error.\n" );
    break;
        }
        if (strm.avail_in == 0)
    break;
        strm.next_in = in;
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = _deflate(&strm, len==0 ? Z_FINISH : Z_NO_FLUSH);
	    if ( ret==Z_STREAM_ERROR ) {
		(void)_deflateEnd(&strm);
		fprintf( stderr, "Compression failed somehow.\n");
		err = 1;
	break;
	    }
	    amount = CHUNK - strm.avail_out;
	    if ( fwrite(out,1,amount,tmp)!= amount || ferror(tmp) ) {
		(void)_deflateEnd(&strm);
		fprintf( stderr, "IO Error.\n");
		err=1;
	break;
	    }
	} while ( strm.avail_out==0 );
	if ( err )
    break;
    } while ( ret!=Z_STREAM_END );
    (void)_deflateEnd(&strm);

    if ( strm.total_out>=uncompLen ) {
	/* Didn't actually make the data smaller, so store uncompressed */
	fclose(tmp);
	copydata(to,off_to,from,off_from,uncompLen);
return( uncompLen );
    } else {
	copydata(to,off_to,tmp,0,strm.total_out);
	fclose(tmp);
return( strm.total_out );
    }
}

SplineFont *_SFReadWOFF(FILE *woff,int flags,enum openflags openflags, char *filename,struct fontdict *fd) {
    int flavour;
    int iscff;
    int len, len_stated;
    int num_tabs;
    int major, minor;
    int metaOffset, metaLenCompressed, metaLenUncompressed;
    int privOffset, privLength;
    int i,j,err;
    int tag, offset, compLen, uncompLen, checksum;
    FILE *sfnt;
    int here, next, tab_start;
    int head_pos = -1;
    SplineFont *sf;

    if ( !haszlib()) {
	ff_post_error(_("WOFF not supported"), _("Could not find the zlib library which is needed to understand WOFF") );
return( NULL );
    }

    fseek(woff,0,SEEK_END);
    len = ftell(woff);
    rewind(woff);
    if ( getlong(woff)!=CHR('w','O','F','F') ) {
	LogError(_("Bad signature in WOFF"));
return( NULL );
    }
    flavour = getlong(woff);
    iscff = (flavour==CHR('O','T','T','O'));
    len_stated = getlong(woff);
    if ( len!=len_stated ) {
	LogError(_("File length as specified in the WOFF header does not match the actual file length."));
return( NULL );
    }

    num_tabs = getushort(woff);
    if ( getushort(woff)!=0 ) {
	LogError(_("Bad WOFF header, a field which must be 0 is not."));
return( NULL );
    }

    /* total_uncompressed_sfnt_size = */ getlong(woff);
    major = getushort(woff);
    minor = getushort(woff);
    metaOffset = getlong(woff);
    metaLenCompressed = getlong(woff);
    metaLenUncompressed = getlong(woff);
    privOffset = getlong(woff);
    privLength = getlong(woff);

    sfnt = tmpfile();
    if ( sfnt==NULL ) {
	LogError(_("Could not open temporary file."));
return( NULL );
    }

    putlong(sfnt,flavour);
    putshort(sfnt,num_tabs);
    for ( i=1, j=0; 2*i<=num_tabs; i<<=1, ++j );
    putshort(sfnt,i*16);
    putshort(sfnt,j);
    putshort(sfnt,(num_tabs-i)*16);

    /* dummy space for table pointers */
    tab_start = ftell(sfnt);
    for ( i=0; i<4*num_tabs; ++i )
	putlong(sfnt,0);

    for ( i=0; i<num_tabs; ++i ) {
	tag = getlong(woff);
	offset = getlong(woff);
	compLen = getlong(woff);
	uncompLen = getlong(woff);
	checksum = getlong(woff);
	if ( compLen>uncompLen || offset+compLen>len ) {
	    fclose(sfnt);
	    if ( compLen>uncompLen )
		LogError(_("Invalid compressed table length for '%c%c%c%c'."),
			tag>>24, tag>>16, tag>>8, tag);
	    else
		LogError(_("Table length stretches beyond end of file for '%c%c%c%c'."),
			tag>>24, tag>>16, tag>>8, tag);
return( NULL );
	}
	here = ftell(woff);
	next = ftell(sfnt);
	fseek(sfnt,tab_start,SEEK_SET);
	putlong(sfnt,tag);
	putlong(sfnt,checksum);
	putlong(sfnt,next);
	putlong(sfnt,uncompLen);
	if ( tag==CHR('h','e','a','d'))
	    head_pos = next;
	tab_start = ftell(sfnt);
	fseek(sfnt,next,SEEK_SET);
	if ( compLen==uncompLen ) {
	    /* Not compressed, copy verbatim */
	    copydata(sfnt,next,woff,offset,compLen);
	} else {
	    err = decompressdata(sfnt,next,woff,offset,compLen,uncompLen);
	    if ( err ) {
		LogError(_("Problem decompressing '%c%c%c%c' table."),
			tag>>24, tag>>16, tag>>8, tag);
		fclose(sfnt);
return( NULL );
	    }
	}
	if ( (ftell(sfnt)&3)!=0 ) {
	    /* Pad to a 4 byte boundary */
	    if ( ftell(sfnt)&1 )
		putc('\0',sfnt);
	    if ( ftell(sfnt)&2 )
		putshort(sfnt,0);
	}
	fseek(woff,here,SEEK_SET);
    }
    /* I assumed at first that the check sum would just be right */
    /*  but I've reordered the tables (probably) so I've got a different */
    /*  set of offsets and I must figure it out for myself */
    /* Usually, I don't care. But if fontlint is on, then do it right */
    if ( (openflags & of_fontlint) && head_pos!=-1 ) {
	int checksum;
	fseek(sfnt,head_pos+8,SEEK_SET);
	putlong(sfnt,0);		/* Clear what was there */
	checksum = filechecksum(sfnt);	/* Recalc */
	checksum = 0xb1b0afba-checksum;
	fseek(sfnt,head_pos+8,SEEK_SET);
	putlong(sfnt,checksum);
    }
    rewind(sfnt);
    sf = _SFReadTTF(sfnt,flags,openflags,filename,fd);
    fclose(sfnt);

    if ( sf!=NULL ) {
	sf->woffMajor = major;
	sf->woffMinor = minor;
    }

    if ( sf!=NULL && metaOffset!=0 ) {
	char *temp = galloc(metaLenCompressed+1);
	uLongf len = metaLenUncompressed;
	fseek(woff,metaOffset,SEEK_SET);
	fread(temp,1,metaLenCompressed,woff);
	sf->woffMetadata = galloc(metaLenUncompressed+1);
	sf->woffMetadata[metaLenUncompressed] ='\0';
	_uncompress(sf->woffMetadata,&len,temp,metaLenCompressed);
	sf->woffMetadata[len] ='\0';
	free(temp);
    }

return( sf );
}	

int _WriteWOFFFont(FILE *woff,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags,EncMap *enc,int layer) {
    int ret;
    FILE *sfnt;
    int major=sf->woffMajor, minor=sf->woffMinor;
    int flavour, num_tabs;
    int filelen, len;
    int i;
    int compLen, uncompLen, newoffset;
    int tag, checksum, offset;
    int tab_start, here;

    if ( !haszlib()) {
	ff_post_error(_("WOFF not supported"), _("Could not find the zlib library which is needed to understand WOFF") );
return( 0 );
    }

    if ( major==woffUnset ) {
	struct ttflangname *useng;
	major = 1; minor = 0;
	for ( useng=sf->names; useng!=NULL; useng=useng->next )
	    if ( useng->lang==0x409 )
	break;
	if ( useng!=NULL && useng->names[ttf_version]!=NULL &&
		sscanf(useng->names[ttf_version], "Version %d.%d", &major, &minor)>=1 ) {
	    /* All done */
	} else if ( sf->subfontcnt!=0 ) {
	    major = floor(sf->cidversion);
	    minor = floor(1000.*(sf->cidversion-major));
	} else if ( sf->version!=NULL ) {
	    char *pt=sf->version;
	    char *end;
	    while ( *pt && !isdigit(*pt) && *pt!='.' ) ++pt;
	    if ( *pt ) {
		major = strtol(pt,&end,10);
		if ( *end=='.' )
		    minor = strtol(end+1,NULL,10);
	    }
	}
    }

    format = sf->subfonts!=NULL ? ff_otfcid :
		sf->layers[layer].order2 ? ff_ttf : ff_otf;
    sfnt = tmpfile();
    ret = _WriteTTFFont(sfnt,sf,format,bsizes,bf,flags,enc,layer);
    if ( !ret ) {
	fclose(sfnt);
return( ret );
    }

    fseek(sfnt,0,SEEK_END);
    filelen = ftell(sfnt);
    rewind(sfnt);

    flavour = getlong(sfnt);
    /* The woff standard says we should accept all flavours of sfnt, so can't */
    /*  test flavour to make sure we've got a valid sfnt */
    /* But we can test the rest of the header for consistancy */
    num_tabs = getushort(sfnt);
    (void) getushort(sfnt);
    (void) getushort(sfnt);
    (void) getushort(sfnt);

    rewind(woff);
    putlong(woff,CHR('w','O','F','F'));
    putlong(woff,flavour);
    putlong(woff,0);		/* Off: 8. total length of file, fill in later */
    putshort(woff,num_tabs);
    putshort(woff,0);		/* Must be zero */
    putlong(woff,filelen);
    putshort(woff,major);	/* Major and minor version numbers of font */
    putshort(woff,minor);
    putlong(woff,0);		/* Off: 24. Offset to metadata table */
    putlong(woff,0);		/* Off: 28. Length (compressed) of metadata */
    putlong(woff,0);		/* Off: 32. Length (uncompressed) */
    putlong(woff,0);		/* Off: 36. Offset to private data */
    putlong(woff,0);		/* Off: 40. Length of private data */

    tab_start = ftell(woff);
    for ( i=0; i<5*num_tabs; ++i )
	putlong(woff,0);

    for ( i=0; i<num_tabs; ++i ) {
	tag = getlong(sfnt);
	checksum = getlong(sfnt);
	offset = getlong(sfnt);
	uncompLen = getlong(sfnt);
	here = ftell(sfnt);
	newoffset = ftell(woff);
	compLen = compressOrNot(woff,newoffset,sfnt,offset,uncompLen,false);
	if ( (ftell(woff)&3)!=0 ) {
	    /* Pad to a 4 byte boundary */
	    if ( ftell(woff)&1 )
		putc('\0',woff);
	    if ( ftell(woff)&2 )
		putshort(woff,0);
	}
	fseek(sfnt,here,SEEK_SET);
	fseek(woff,tab_start,SEEK_SET);
	putlong(woff,tag);
	putlong(woff,newoffset);
	putlong(woff,compLen);
	putlong(woff,uncompLen);
	putlong(woff,checksum);
	tab_start = ftell(woff);
	fseek(woff,0,SEEK_END);
    }
    fclose(sfnt);

    if ( sf->woffMetadata!= NULL ) {
	int uncomplen = strlen(sf->woffMetadata);
	uLongf complen = 2*uncomplen;
	char *temp=galloc(complen+1);
	newoffset = ftell(woff);
	_compress(temp,&complen,sf->woffMetadata,uncomplen);
	fwrite(temp,1,complen,woff);
	free(temp);
	if ( (ftell(woff)&3)!=0 ) {
	    /* Pad to a 4 byte boundary */
	    if ( ftell(woff)&1 )
		putc('\0',woff);
	    if ( ftell(woff)&2 )
		putshort(woff,0);
	}
	fseek(woff,24,SEEK_SET);
	putlong(woff,newoffset);
	putlong(woff,complen);
	putlong(woff,uncomplen);
	fseek(woff,0,SEEK_END);
    }

    fseek(woff,0,SEEK_END);
    len = ftell(woff);
    fseek(woff,8,SEEK_SET);
    putlong(woff,len);
return( true );		/* No errors */
}

int WriteWOFFFont(char *fontname,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags,EncMap *enc,int layer) {
    FILE *woff;
    int ret;

    if ( strstr(fontname,"://")!=NULL ) {
	if (( woff = tmpfile())==NULL )
return( 0 );
    } else {
	if (( woff=fopen(fontname,"wb+"))==NULL )
return( 0 );
    }
    ret = _WriteWOFFFont(woff,sf,format,bsizes,bf,flags,enc,layer);
    if ( strstr(fontname,"://")!=NULL && ret )
	ret = URLFromFile(fontname,woff);
    if ( fclose(woff)==-1 )
return( 0 );
return( ret );
}

int CanWoff(void) {
return( haszlib());
}
#endif		/* NOLIBPNG */
