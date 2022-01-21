/* Copyright (c) 2010 by George Williams */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

/* Compile with: $ cc -o dewoff dewoff.c -lz */

/* http://people.mozilla.com/~jkew/woff/woff-2009-09-16.html */

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

static int getushort(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

static int getlong(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    int ch4 = getc(ttf);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

static void putshort(FILE *file,int sval) {
    putc((sval>>8)&0xff,file);
    putc(sval&0xff,file);
}

static void putlong(FILE *file,int val) {
    putc((val>>24)&0xff,file);
    putc((val>>16)&0xff,file);
    putc((val>>8)&0xff,file);
    putc(val&0xff,file);
}

static int filecheck(FILE *file) {
    unsigned int sum = 0, chunk;

    rewind(file);
    while ( 1 ) {
	chunk = getlong(file);
	if ( feof(file) || ferror(file))
    break;
	sum += chunk;
    }
return( sum );
}
/* ************************************************************************** */

enum woff_errs { we_good, we_cantopen, we_cantopenout, we_badsignature,
    we_badwofflength, we_mbz, we_badtablen, we_decompressfailed,
    we_baddecompresslen };
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

static enum woff_errs decompressdata(FILE *to,int off_to,FILE *from,int off_from,
	int len, int uncomplen ) {
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
return( we_decompressfailed );

    do {
	if ( len<=0 ) {
            (void)inflateEnd(&strm);
return( we_decompressfailed );
        }
	amount = len;
	if ( amount>CHUNK )
	    amount = CHUNK;
        strm.avail_in = fread(in, 1, amount, from);
	len -= strm.avail_in;
        if (ferror(from)) {
            (void)inflateEnd(&strm);
return( we_decompressfailed );
        }
        if (strm.avail_in == 0)
    break;
        strm.next_in = (Bytef*)in;
        do {
            strm.avail_out = CHUNK;
            strm.next_out = (Bytef*)out;
            ret = inflate(&strm, Z_NO_FLUSH);
	    if ( ret!=Z_OK && ret!=Z_STREAM_END ) {
		(void)inflateEnd(&strm);
return( we_decompressfailed );
	    }
	    amount = CHUNK - strm.avail_out;
	    if ( fwrite(out,1,amount,to)!= amount || ferror(to) ) {
		(void)inflateEnd(&strm);
return( we_decompressfailed );
	    }
	} while ( strm.avail_out==0 );
    } while ( ret!=Z_STREAM_END );
    (void)inflateEnd(&strm);
    if ( uncomplen!=strm.total_out )
return( we_baddecompresslen );

return( we_good );
}

static int dewoff(char *filename) {
    int flavour;
    int iscff;
    char *outname;
    int len, len_stated;
    int num_tabs;
    int metaOffset, metaLenCompressed, metaLenUncompressed;
    int privOffset, privLength;
    int i,j,err;
    int tag, offset, compLen, uncompLen, checksum;
    FILE *woff, *sfnt, *meta, *priv;
    char *start, *ext;
    int here, next, tab_start;
    int head_pos = -1;

    woff = fopen( filename,"rb");
    if ( woff==NULL )
return( we_cantopen );

    start = strrchr(filename,'/');
    if ( start==NULL )
	start = filename;
    else
	++start;
    ext = strrchr(start,'.');
    if ( ext==NULL )
	ext = start+strlen(start);

    fseek(woff,0,SEEK_END);
    len = ftell(woff);
    rewind(woff);
    if ( getlong(woff)!=CHR('w','O','F','F') )
return( we_badsignature );
    flavour = getlong(woff);
    iscff = (flavour==CHR('O','T','T','O'));
    len_stated = getlong(woff);
    if ( len!=len_stated )
return( we_badwofflength );
    num_tabs = getushort(woff);
    if ( getushort(woff)!=0 )
return( we_mbz );
    /* total_uncompressed_sfnt_size = */ getlong(woff);
    /* major version of woff font   = */ getushort(woff);
    /* minor version of woff font   = */ getushort(woff);
    metaOffset = getlong(woff);
    metaLenCompressed = getlong(woff);
    metaLenUncompressed = getlong(woff);
    privOffset = getlong(woff);
    privLength = getlong(woff);

    outname = malloc(strlen(start)+20);
    strcpy(outname,start);
    strcpy(outname+(ext-start),iscff ? ".otf" : ".ttf" );
    sfnt = fopen( outname,"wb+" );
    if ( sfnt==NULL ) {
        free(outname);
        fclose(woff);
return( we_cantopenout );
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
	    fclose(woff);
	    free(outname);
return( we_badtablen );
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
	    if ( err!=we_good ) {
		fclose(sfnt);
		fclose(woff);
		free(outname);
return( err );
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
    if ( head_pos!=-1 ) {
	fseek(sfnt,head_pos+8,SEEK_SET);
	putlong(sfnt,0);		/* Clear what was there */
	checksum = filecheck(sfnt);	/* Recalc */
	checksum = 0xb1b0afba-checksum;
	fseek(sfnt,head_pos+8,SEEK_SET);
	putlong(sfnt,checksum);
    }
    fclose(sfnt);

    if ( metaOffset!=0 ) {
	strcpy(outname+(ext-start), "_meta.xml" );
	meta = fopen( outname,"wb" );
	if ( meta==NULL ) {
	    fclose(woff);
	    free(outname);
return( we_cantopenout );
	}
	err = decompressdata(meta,0,woff,metaOffset,metaLenCompressed,metaLenUncompressed);
	fclose(meta);
	if ( err!=we_good ) {
	    fclose(woff);
	    free(outname);
return( err );
	}
    }

    if ( privOffset!=0 ) {
	strcpy(outname+(ext-start), ".priv" );
	priv = fopen( outname,"wb" );
	if ( priv==NULL ) {
	    fclose(woff);
return( we_cantopenout );
	}
	copydata(priv,0,woff,privOffset,privLength);
	fclose(priv);
    }

    fclose(woff);
    free(outname);
return( we_good );
}

static void usage(char *prog) {
    printf( "%s {woff-filename}\n", prog );
    printf( " Takes one (or several woff files, and converts them into their equivalent\n" );
    printf( "ttf or otf forms. The output files will be placed in the current directory\n" );
    printf( "with the same name as the original file and an extension of either \".otf\" or \".ttf\"\n" );
    printf( "(as is appropriate). If the woff file contains metadata that will be stored\n" );
    printf( "with the extension \"_meta.xml\", and a private table will have \".priv\"\n" );
}

int main(int argc, char **argv) {
    int i, err;

    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' ) {
	    usage(argv[0]);
    continue;
	}
	err = dewoff(argv[i]);
	switch ( err ) {
	  case we_good:
	break;
	  case we_cantopen:
	    fprintf( stderr, "Cannot open %s\n", argv[i]);
	break;
	  case we_cantopenout:
	    fprintf( stderr, "Cannot open output file(s)\n" );
	break;
	  case we_badsignature:
	    fprintf( stderr, "This (%s) does not look like a WOFF file.\n", argv[i] );
	break;
	  case we_badwofflength:
	    fprintf( stderr, "Actual file length does not match expected length in %s.\n", argv[i] );
	break;
	  case we_badtablen:
	    fprintf( stderr, "Table stretches beyond end of file, or impossible values for\n compressed and uncompressed table lengths in %s.\n", argv[i] );
	break;
	  case we_mbz:
	    fprintf( stderr, "A field in the WOFF header must be zero, but is not in %s.\n", argv[i] );
	break;
	  case we_decompressfailed:
	    fprintf( stderr, "Table decompress failed in %s.\n", argv[i] );
	break;
	  case we_baddecompresslen:
	    fprintf( stderr, "Length of decompressed table does not match expected length in %s.\n", argv[i] );
	break;
	  default:
	    fprintf( stderr, "Unexpected error: %d on %s\n", err, argv[i]);
	}
    }
return( 0 );
}
