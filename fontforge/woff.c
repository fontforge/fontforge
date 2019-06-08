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

#include <fontforge-config.h>

/* Support for woff files */
/* Which are defined here: http://people.mozilla.com/~jkew/woff/woff-2009-09-16.html */
/* Basically sfnts with compressed tables and some more metadata */

#include "woff.h"

#include "fontforge.h"
#include "gfile.h"
#include "mem.h"
#include "parsettf.h"
#include "tottf.h"

#include <ctype.h>
#include <math.h>
#include <zlib.h>

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
    ret = inflateInit(&strm);
    if ( ret!=Z_OK )
return( true );

    do {
	if ( len<=0 ) {
            (void)inflateEnd(&strm);
return( true );
        }
	amount = len;
	if ( amount>CHUNK )
	    amount = CHUNK;
        strm.avail_in = fread(in, 1, amount, from);
	len -= strm.avail_in;
        if (ferror(from)) {
            (void)inflateEnd(&strm);
return( true );
        }
        if (strm.avail_in == 0)
    break;
        strm.next_in = in;
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
	    if ( ret!=Z_OK && ret!=Z_STREAM_END ) {
		(void)inflateEnd(&strm);
return( true );
	    }
	    amount = CHUNK - strm.avail_out;
	    if ( fwrite(out,1,amount,to)!= amount || ferror(to) ) {
		(void)inflateEnd(&strm);
return( true );
	    }
	} while ( strm.avail_out==0 );
    } while ( ret!=Z_STREAM_END );
    (void)inflateEnd(&strm);
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
    ret = deflateInit(&strm,Z_DEFAULT_COMPRESSION);
    if ( ret!=Z_OK ) {
	fprintf( stderr,"Compression initialization failed.\n" );
return(0);
    }
    tmp = GFileTmpfile();

    do {
	if ( len<=0 ) {
            (void)deflateEnd(&strm);
    break;
        }
	amount = len;
	if ( amount>CHUNK )
	    amount = CHUNK;
        strm.avail_in = fread(in, 1, amount, from);
	len -= strm.avail_in;
        if (ferror(from)) {
            (void)deflateEnd(&strm);
	    fprintf( stderr, "IO error.\n" );
    break;
        }
        if (strm.avail_in == 0)
    break;
        strm.next_in = in;
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, len==0 ? Z_FINISH : Z_NO_FLUSH);
	    if ( ret==Z_STREAM_ERROR ) {
		(void)deflateEnd(&strm);
		fprintf( stderr, "Compression failed somehow.\n");
		err = 1;
	break;
	    }
	    amount = CHUNK - strm.avail_out;
	    if ( fwrite(out,1,amount,tmp)!= amount || ferror(tmp) ) {
		(void)deflateEnd(&strm);
		fprintf( stderr, "IO Error.\n");
		err=1;
	break;
	    }
	} while ( strm.avail_out==0 );
	if ( err )
    break;
    } while ( ret!=Z_STREAM_END );
    (void)deflateEnd(&strm);

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

SplineFont *_SFReadWOFF(FILE *woff,int flags,enum openflags openflags, char *filename,char *chosenname,struct fontdict *fd) {
    int flavour;
    int iscff;
    int len, len_stated;
    int num_tabs;
    int major, minor;
    uint32_t metaOffset, metaLenCompressed, metaLenUncompressed;
    int privOffset, privLength;
    int i,j,err;
    int tag, offset, compLen, uncompLen, checksum;
    FILE *sfnt;
    int here, next, tab_start;
    int head_pos = -1;
    SplineFont *sf;

    fseek(woff,0,SEEK_END);
    len = ftell(woff);
    rewind(woff);
    {
        int signature = getlong(woff);
        if ( signature!=CHR('w','O','F','F') ) {
            LogError(_("Bad signature in WOFF header."));
            return NULL;
        }
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
    metaOffset = (uint32_t)getlong(woff);
    metaLenCompressed = (uint32_t)getlong(woff);
    metaLenUncompressed = (uint32_t)getlong(woff);
    privOffset = getlong(woff);
    privLength = getlong(woff);

    sfnt = GFileTmpfile();
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
    sf = _SFReadTTF(sfnt,flags,openflags,filename,chosenname,fd);
    fclose(sfnt);

    if ( sf!=NULL ) {
	sf->woffMajor = major;
	sf->woffMinor = minor;
    }

    if ( sf!=NULL && metaOffset!=0 ) {
	/*
	* Boundary/integer overflow checks:
	*
	* We don't want to actually dereference a null pointer (returned
	* by asking to allocate too much RAM) and we don't want to allocate
	* a 0-sized chunk (caused when one of the (metaLenxxx + 1) values overflows).
	*
	* We can safely pass sf->woffMetadata as a NULL pointer because
	* it's never accessed anywhere else without a check for it being
	* NULL first
	*/
	if(metaLenUncompressed == UINT32_MAX) {
		LogError(_("WOFF uncompressed metadata section too large.\n"));
		sf->woffMetadata = NULL; 
		return( sf );
	}
	if(metaLenCompressed == UINT32_MAX) {
		LogError(_("WOFF compressed metadata section too large.\n"));
		sf->woffMetadata = NULL;
		return( sf );
	}
	sf->woffMetadata = malloc(metaLenUncompressed+1);
	if(sf->woffMetadata == NULL) { 
		LogError(_("WOFF uncompressed metadata section too large.\n"));
		return( sf );
	}
	unsigned char *temp = malloc(metaLenCompressed+1);
	if(temp == NULL) { 
		LogError(_("WOFF compressed metadata section too large.\n"));
		free(sf->woffMetadata); 
		sf->woffMetadata = NULL;
		free(temp);
		return( sf );
	}
	uLongf len = metaLenUncompressed;
	fseek(woff,metaOffset,SEEK_SET);
	fread(temp,1,metaLenCompressed,woff);
	sf->woffMetadata[metaLenUncompressed] ='\0';
	uncompress((unsigned char *)sf->woffMetadata,&len,temp,metaLenCompressed);
	sf->woffMetadata[len] ='\0';
	free(temp);
    }

return( sf );
}	

typedef struct {
    int index;
    int offset;
} tableOrderRec;

static int
compareOffsets(const void * lhs, const void * rhs)
{
    const tableOrderRec * a = (const tableOrderRec *) lhs;
    const tableOrderRec * b = (const tableOrderRec *) rhs;
    /* don't simply return a->offset - b->offset because these are unsigned
       offset values; could convert to int, but possible integer overflow */
    return a->offset > b->offset ? 1 :
           a->offset < b->offset ? -1 :
           0;
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
    int tab_start;
    tableOrderRec *tableOrder = NULL;

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
    sfnt = GFileTmpfile();
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

    /*
     * At this point _WriteTTFFont should have generated an sfnt file with
     * valid checksums, correct padding and no extra gaps. However, the order
     * of the font tables in the original sfnt font must also be preserved so
     * that WOFF consumers can recover the original offsets as well as the
     * original font. Hence we will compress and write the font tables into
     * the WOFF file using the original offset order. Note that the order of
     * tables may not be the same as the one of table directory entries.
     * See https://github.com/fontforge/fontforge/issues/926
     */
    tableOrder = (tableOrderRec *) malloc(num_tabs * sizeof(tableOrderRec));
    if (!tableOrder) {
        fclose(sfnt);
        return false;
    }
    for ( i=0; i<num_tabs; ++i ) {
        fseek(sfnt,(3 + 4*i + 2)*sizeof(int32),SEEK_SET);
        tableOrder[i].index = i;
        tableOrder[i].offset = getlong(sfnt);
    }
    qsort(tableOrder, num_tabs, sizeof(tableOrderRec), compareOffsets);

    /* Now generate the WOFF file */
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
	fseek(sfnt,(3 + 4*tableOrder[i].index)*sizeof(int32),SEEK_SET);
	tag = getlong(sfnt);
	checksum = getlong(sfnt);
	offset = getlong(sfnt);
	uncompLen = getlong(sfnt);
	newoffset = ftell(woff);
	compLen = compressOrNot(woff,newoffset,sfnt,offset,uncompLen,false);
	if ( (ftell(woff)&3)!=0 ) {
	    /* Pad to a 4 byte boundary */
	    if ( ftell(woff)&1 )
		putc('\0',woff);
	    if ( ftell(woff)&2 )
		putshort(woff,0);
	}
	fseek(woff,tab_start+(5*tableOrder[i].index)*sizeof(int32),SEEK_SET);
	putlong(woff,tag);
	putlong(woff,newoffset);
	putlong(woff,compLen);
	putlong(woff,uncompLen);
	putlong(woff,checksum);
	fseek(woff,0,SEEK_END);
    }
    fclose(sfnt);

    if ( sf->woffMetadata!= NULL ) {
	int uncomplen = strlen(sf->woffMetadata);
	uLongf complen = 2*uncomplen;
	unsigned char *temp=malloc(complen+1);
	newoffset = ftell(woff);
	compress(temp,&complen,(unsigned char *)sf->woffMetadata,uncomplen);
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

    free(tableOrder);
return( true );		/* No errors */
}

int WriteWOFFFont(char *fontname,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags,EncMap *enc,int layer) {
    FILE *woff;
    int ret;

    if (( woff=fopen(fontname,"wb+"))==NULL )
return( 0 );
    ret = _WriteWOFFFont(woff,sf,format,bsizes,bf,flags,enc,layer);
    if ( fclose(woff)==-1 )
return( 0 );
return( ret );
}

#ifdef FONTFORGE_CAN_USE_WOFF2

/**
 * Read the contents of fp into a buffer, caller must free.
 */
static uint8_t *ReadFileToBuffer(FILE *fp, size_t *buflen)
{
    if (fseek(fp, 0, SEEK_END) < 0) {
        return NULL;
    }

    long length = ftell(fp);
    if (length <= 0 || fseek(fp, 0, SEEK_SET) < 0) {
        return NULL;
    }

    uint8_t *buf = calloc(length, 1);
    if (!buf) {
        return NULL;
    }

    *buflen = fread(buf, 1, length, fp);
    if (fgetc(fp) != EOF) {
        free(buf);
        return NULL;
    }
    return buf;
}

/**
 * Write the contents of buf into fp.
 * On success, the returned file pointer is equal to fp.
 * It will be positioned at the start of the file.
 */
static FILE *WriteBufferToFile(FILE *fp, const uint8_t *buf, size_t buflen)
{
    if (fp && fwrite(buf, 1, buflen, fp) == buflen) {
        if (fseek(fp, 0, SEEK_SET) >= 0) {
            return fp;
        }
    }
    return NULL;
}

/**
 * Write the contents of buf into a temporary file.
 * On success, the returned file pointer is at the start of the file.
 * The caller is responsible for closing it.
 */
static FILE *WriteBufferToTempFile(const uint8_t *buf, size_t buflen)
{
    FILE *fp = GFileTmpfile();
    if (!WriteBufferToFile(fp, buf, buflen)) {
        fclose(fp);
        return NULL;
    }
    return fp;
}

int WriteWOFF2Font(char *fontname, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer)
{
    FILE *woff = fopen(fontname, "wb");
    int ret = 0;
    if (woff) {
        ret = _WriteWOFF2Font(woff, sf, format, bsizes, bf, flags, enc, layer);
        fclose(woff);
    }
    return ret;
}

int _WriteWOFF2Font(FILE *fp, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer)
{
    FILE *tmp = GFileTmpfile();
    if (!tmp) {
        return 0;
    }
    int ret = _WriteTTFFont(tmp, sf, format, bsizes, bf, flags, enc, layer);
    if (!ret) {
        fclose(tmp);
        return 0;
    }

    size_t raw_input_length = 0;
    uint8_t *raw_input = ReadFileToBuffer(tmp, &raw_input_length);
    fclose(tmp);
    if (!raw_input) {
        return 0;
    }

    size_t comp_size = woff2_max_woff2_compressed_size(raw_input, raw_input_length);
    uint8_t *comp_buffer = calloc(comp_size, 1);
    if (!comp_buffer) {
        free(raw_input);
        return 0;
    }
    ret = woff2_convert_ttf_to_woff2(raw_input, raw_input_length, comp_buffer, &comp_size);
    free(raw_input);
    if (!ret) {
        free(comp_buffer);
        return 0;
    }

    ret = WriteBufferToFile(fp, comp_buffer, comp_size) != NULL;
    free(comp_buffer);
    return ret;
}

SplineFont *_SFReadWOFF2(FILE *fp, int flags, enum openflags openflags, char *filename,char *chosenname,struct fontdict *fd)
{
    size_t raw_input_length = 0;
    if (!fp) {
        return NULL;
    }

    uint8_t *raw_input = ReadFileToBuffer(fp, &raw_input_length);

    size_t decomp_size = woff2_compute_woff2_final_size(raw_input, raw_input_length);
    if (decomp_size > WOFF2_DEFAULT_MAX_SIZE) {
        decomp_size = WOFF2_DEFAULT_MAX_SIZE;
    }
    uint8_t *decomp_buffer = calloc(decomp_size, 1);
    if (!decomp_buffer) {
        free(raw_input);
        return NULL;
    }
    int success = woff2_convert_woff2_to_ttf(raw_input, raw_input_length, decomp_buffer, &decomp_size);
    free(raw_input);
    if (!success) {
        free(decomp_buffer);
        return NULL;
    }

    FILE *tmp = WriteBufferToTempFile(decomp_buffer, decomp_size);
    free(decomp_buffer);
    if (!tmp) {
        return NULL;
    }

    SplineFont *ret = _SFReadTTF(tmp, flags, openflags, filename, chosenname, fd);
    fclose(tmp);
    return ret;
}

#endif // FONTFORGE_CAN_USE_WOFF2
