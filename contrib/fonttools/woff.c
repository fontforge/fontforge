/* Copyright (c) 2010 by George Williams */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

/* Compile with: $ cc -o woff woff.c -lz */

/* http://people.mozilla.com/~jkew/woff/woff-2009-09-16.html */

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

#define true	1
#define false	0

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
/* ************************************************************************** */
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
    tmp = tmpfile();

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
        strm.next_in = (Bytef*)in;
        do {
            strm.avail_out = CHUNK;
            strm.next_out = (Bytef*)out;
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

static void woff(char *filename, char *metafile, char *privfile,
	int major, int minor) {
    int flavour, num_tabs, test;
    char *start, *ext, *outname;
    int filelen, len;
    int i,j;
    FILE *sfnt, *woff, *meta, *priv;
    int compLen, uncompLen, newoffset;
    int tag, checksum, offset;
    int tab_start, here;

    sfnt = fopen( filename,"rb");
    if ( sfnt==NULL ) {
	fprintf( stderr, "Can't open %s\n", filename );
return;
    }

    start = strrchr(filename,'/');
    if ( start==NULL )
	start = filename;
    else
	++start;
    ext = strrchr(start,'.');
    if ( ext==NULL )
	ext = start+strlen(start);

    fseek(sfnt,0,SEEK_END);
    filelen = ftell(sfnt);
    rewind(sfnt);

    flavour = getlong(sfnt);
    /* The woff standard says we should accept all flavours of sfnt, so can't */
    /*  test flavour to make sure we've got a valid sfnt */
    /* But we can test the rest of the header for consistency */
    num_tabs = getushort(sfnt);
    for ( i=1, j=0; 2*i<=num_tabs; i<<=1, ++j );
    test = getushort(sfnt);
    if ( test!=16*i ) {
	fprintf( stderr, "%s has an invalid sfnt header (1) %d!=%d\n", filename, test, 16*i );
	fclose(sfnt);
return;
    }
    test = getushort(sfnt);
    if ( test!=j ) {
	fprintf( stderr, "%s has an invalid sfnt header (2) %d!=%d\n", filename, test, j );
	fclose(sfnt);
return;
    }
    test = getushort(sfnt);
    if ( test!=(num_tabs-i)*16 ) {
	fprintf( stderr, "%s has an invalid sfnt header (3) %d!=%d\n", filename, test, (num_tabs-i)*16 );
	fclose(sfnt);
return;
    }

    outname = malloc(strlen(start)+20);
    strcpy(outname,start);
    strcpy(outname+(ext-start),".woff" );
    woff = fopen(outname,"wb");
    if ( woff==NULL ) {
	fprintf( stderr, "Failed to create output file: %s\n", outname );
	fclose(sfnt);
	free(outname);
return;
    }
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

    meta = NULL;
    if ( metafile!=NULL ) {
	meta = fopen(metafile,"w");
	if ( meta==NULL )
	    fprintf( stderr, "Failed to open meta data file: %s\n", metafile );
    }
    if ( meta!=NULL ) {
	newoffset = ftell(woff);
	fseek(meta,0,SEEK_END);
	uncompLen = ftell(meta);
	rewind( meta );
	compLen = compressOrNot(woff,newoffset,meta,0,uncompLen,true);
	if ( privfile!=NULL && (ftell(woff)&3)!=0 ) {
	    /* Pad to a 4 byte boundary */
	    if ( ftell(woff)&1 )
		putc('\0',woff);
	    if ( ftell(woff)&2 )
		putshort(woff,0);
	}
	fseek(woff,24,SEEK_SET);
	putlong(woff,newoffset);
	putlong(woff,compLen);
	putlong(woff,uncompLen);
	fseek(woff,0,SEEK_END);
	fclose(meta);
    }

    priv = NULL;
    if ( privfile!=NULL ) {
	priv = fopen(privfile,"w");
	if ( priv==NULL )
	    fprintf( stderr, "Failed to open private file: %s\n", privfile );
    }
    if ( priv!=NULL ) {
	newoffset = ftell(woff);
	fseek(priv,0,SEEK_END);
	uncompLen = ftell(priv);
	rewind( priv );
	copydata(woff,newoffset,priv,0,uncompLen);
	/* Not padded */
	fseek(woff,36,SEEK_SET);
	putlong(woff,newoffset);
	putlong(woff,uncompLen);
	fseek(woff,0,SEEK_END);
	fclose(priv);
    }
    len = ftell(woff);
    fseek(woff,8,SEEK_SET);
    putlong(woff,len);
    fclose(woff);
    free(outname);
}
	

static void usage(char *prog) {
    printf( "%s {[-meta xml-file] [-priv file] [-major num] [-minor num] otf/ttf-filename}\n", prog );
    printf( " Takes one (or several sfnt files, and converts them into their equivalent\n" );
    printf( "woff forms. The output files will be placed in the current directory\n" );
    printf( "with the same name as the original file and an extension \".woff\". If\n" );
    printf( "you wish to include a metadata table, or a private table put them in\n" );
    printf( "their own files and specify them with -meta or -priv arguments\n" );
}

int main(int argc, char **argv) {
    int i;
    char *metafile=NULL, *privfile=NULL, *pt;
    int major=1, minor=0;

    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' ) {
	    pt = argv[i];
	    if ( *pt=='-' && pt[1]=='-' )
		++pt;
	    if ( i+1<argc ) {
		if ( strcasecmp(pt,"-meta")==0 || strcasecmp(pt,"-metadata")==0 ) {
		    metafile = argv[++i];
    continue;
		} else if ( strcasecmp(pt,"-priv")==0 || strcasecmp(pt,"-private")==0 ) {
		    privfile = argv[++i];
    continue;
		} else if ( strcasecmp(pt,"-major")==0 ) {
		    major = strtol(argv[++i],NULL,10);
    continue;
		} else if ( strcasecmp(pt,"-minor")==0 ) {
		    minor = strtol(argv[++i],NULL,10);
    continue;
		}
	    }
	    usage(argv[0]);
    continue;
	}
	woff(argv[i],metafile,privfile,major,minor);
	metafile=NULL; privfile=NULL;
	major = 1; minor = 0;
    }
return( 0 );
}
