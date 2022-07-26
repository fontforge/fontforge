#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

static int leniv=4;
static int useshex;

static char *myfgets(char *str, int len, FILE *file) {
    char *pt, *end;
    int ch;

    for ( pt = str, end = str+len-1; pt<end && (ch=getc(file))!=EOF && ch!='\r' && ch!='\n';
	*pt++ = ch );
    if ( ch=='\n' )
	*pt++ = '\n';
    else if ( ch=='\r' ) {
	*pt++ = '\n';
	if ((ch=getc(file))!='\n' )
	    ungetc(ch,file);
    }
    if ( pt==str )
return( NULL );
    *pt = '\0';
return( str );
}

static int hex(int ch1, int ch2) {
    if ( ch1>='0' && ch1<='9' )
	ch1 -= '0';
    else if ( ch1>='a' )
	ch1 -= 'a'-10;
    else 
	ch1 -= 'A'-10;
    if ( ch2>='0' && ch2<='9' )
	ch2 -= '0';
    else if ( ch2>='a' )
	ch2 -= 'a'-10;
    else 
	ch2 -= 'A'-10;
return( (ch1<<4)|ch2 );
}

static unsigned short r;
#define c1	(unsigned short) 52845
#define c2	(unsigned short) 22719

static void initcode() {
    r = 55665;
}

static int decode(unsigned char cypher) {
    unsigned char plain = ( cypher ^ (r>>8));
    r = (cypher + r) * c1 + c2;
return( plain );
}

static void dumpzeros(FILE *out, unsigned char *zeros, int zcnt) {
    while ( --zcnt >= 0 )
	fputc(*zeros++,out);
}

static void decodestr(unsigned char *str, int len) {
    unsigned short rc = 4330;
    unsigned char plain, cypher;

    while ( len-->0 ) {
	cypher = *str;
	plain = ( cypher ^ (rc>>8));
	rc = (cypher + rc) * c1 + c2;
	*str++ = plain;
    }
}

static void decodebytes(FILE *out,unsigned char *binpt, int binlen ) {
    unsigned char *end = binpt+binlen;
    static const char *commands[32] = { "?0", "hstem", "?2", "vstem", "vmoveto",
	    "rlineto", "hlineto", "vlineto", "rrcurveto", "closepath",
	    "callsubr", "return", "escape", "hsbw", "endchar", "?15", "?16",
	    "?17", "?18", "?19", "?20", "rmoveto", "hmoveto", "?23", "?24",
	    "?25", "?26", "?27", "?28", "?29", "vhcurveto", "hvcurveto" };

    decodestr(binpt,binlen);
    binpt += leniv;
    while ( binpt<end ) {
	int ch = *binpt++;
	if ( ch>=32 && ch<=246 ) {
	    fprintf( out, " %d", ch-139 );
	} else if ( ch>=247 && ch<=250 ) {
	    int ch2 = *binpt++;
	    fprintf( out, " %d", (ch-247)*256+ch2+108 );
	} else if ( ch>=251 && ch<=254 ) {
	    int ch2 = *binpt++;
	    fprintf( out, " %d", -(ch-251)*256-ch2-108 );
	} else if ( ch==255 ) {
	    int val;
	    val = *binpt++;
	    val = (val<<8) | *binpt++;
	    val = (val<<8) | *binpt++;
	    val = (val<<8) | *binpt++;
	    fprintf( out, " %d", val );
	} else if ( ch!=12 ) {
	    fprintf( out, " %s", commands[ch]);
	} else {
	    int ch2 = *binpt++;
	    if ( ch2==0 )
		fprintf( out, " dotsection" );
	    else if ( ch2==1 )
		fprintf( out, " vstem3" );
	    else if ( ch2==2 )
		fprintf( out, " hstem3" );
	    else if ( ch2==6 )
		fprintf( out, " seac" );
	    else if ( ch2==7 )
		fprintf( out, " sbw" );
	    else if ( ch2==12 )
		fprintf( out, " div" );
	    else if ( ch2==16 )
		fprintf( out, " callothersubr" );
	    else if ( ch2==17 )
		fprintf( out, " pop" );
	    else if ( ch2==33 )
		fprintf( out, " setcurrentpoint" );
	    else
		fprintf( out, " ?12-%d", ch2 );
	}
    }
}

/* In the book the token which starts a character description is always RD but*/
/*  it's just the name of a subroutine which is defined in the private diction*/
/*  and it could be anything. in one case it was "-|" (hyphen bar) so we can't*/
/*  just look for RD we must be a bit smarter and figure out what the token is*/
/* It's defined as {string currentfile exch readstring pop} so look for that */
static int glorpline(FILE *temp, FILE *out,char *rdtok) {
    char buffer[3000], *pt, *binstart;
    int binlen;
    int ch;
    int innum, val, inbinary, inhex, cnt, inr, wasspace, nownum, nowr, nowspace, sptok;
    const char *rdline = "{string currentfile exch readstring pop}", *rpt;
    const char *rdline2 = "{string currentfile exch readhexstring pop}";
    char *tokpt = NULL, *rdpt;
    char temptok[255];
    int intok, willbehex = 0;
    int nibble=0, firstnibble=1;

    ch = getc(temp);
    if ( ch==EOF )
return( 0 );
    ungetc(ch,temp);

    innum = inr = 0; wasspace = 0; inbinary = inhex = 0; rpt = NULL; rdpt = NULL;
    pt = buffer; binstart=NULL; binlen = 0; intok=0; sptok=0;
    while ( (ch=getc(temp))!=EOF ) {
	*pt++ = ch;
	if ( pt>=buffer+sizeof(buffer)) {
	    fprintf(stderr,"Buffer overrun\n" );
	    exit(1);
	}
	nownum = nowspace = nowr = 0;
	if ( rpt!=NULL && ch!=*rpt && ch=='h' && rpt-rdline>25 && rpt-rdline<30 &&
		rdline2[rpt-rdline]=='h' ) {
	    rpt = rdline2 + (rpt-rdline);
	    willbehex = 1;
	}
	if ( inbinary ) {
	    if ( --cnt==0 )
		inbinary = 0;
	} else if ( inhex ) {
	    if ( isdigit(ch) || (ch>='a'&&ch<='f') || (ch>='A' && ch<='F')) {
		int h;
		if ( isdigit(ch)) h = ch-'0';
		else if ( ch>='a' && ch<='f' ) h = ch-'a'+10;
		else h = ch-'A'+10;
		if ( firstnibble ) {
		    nibble = h;
		    --pt;
		} else {
		    pt[-1] = (nibble<<4)|h;
		    if ( --cnt==0 )
			inbinary = inhex = 0;
		}
		firstnibble = !firstnibble;
	    } else {
		--pt;
		/* skip everything not hex */
	    }
	} else if ( ch=='/' ) {
	    intok = 1;
	    tokpt = temptok;
	} else if ( intok && !isspace(ch) && ch!='{' && ch!='[' ) {
	    *tokpt++ = ch;
	} else if ( (intok||sptok) && (ch=='{' || ch=='[')) {
	    *tokpt = '\0';
	    rpt = rdline+1;
	    intok = sptok = 0;
	} else if ( intok ) {
	    *tokpt = '\0';
	    intok = 0;
	    sptok = 1;
	} else if ( sptok && isspace(ch)) {
	    nowspace = 1;
	    if ( ch=='\n' || ch=='\r' )
    break;
	} else if ( sptok && !isdigit(ch) )
	    sptok = 0;
	else if ( rpt!=NULL && ch==*rpt ) {
	    if ( *++rpt=='\0' ) {
		/* it matched the character definition string so this is the */
		/*  token we want to search for */
		strcpy(rdtok,temptok);
		useshex = willbehex;
	    }
	} else if ( isdigit(ch)) {
	    nownum = 1;
	    sptok = 0;
	    if ( innum )
		val = 10*val + ch-'0';
	    else
		val = ch-'0';
	} else if ( isspace(ch)) {
	    nowspace = 1;
	    if ( ch=='\n' || ch=='\r' )
    break;
	} else if ( wasspace && ch==*rdtok ) {
	    nowr = 1;
	    rdpt = rdtok+1;
	} else if ( inr && ch==*rdpt ) {
	    if ( *++rdpt =='\0' ) {
		ch = getc(temp);
		*pt++ = ch;
		if ( isspace(ch) && val!=0 ) {
		    inhex = useshex;
		    inbinary = !useshex;
		    cnt = val;
		    binstart = pt;
		    binlen = val;
		    if ( binlen>sizeof(buffer)) {
			fprintf(stderr, "Buffer overflow needs to be at least %d\ndying gracefully.\n", binlen);
			exit(1);
		    }
		}
	    } else
		nowr = 1;
	}
	innum = nownum; wasspace = nowspace; inr = nowr;
    }
    if ( ch=='\r' ) {
	ch=getc(temp);
	if ( ch!='\n' )
	    ungetc(ch,temp);
	pt[-1]='\n';
    }
    *pt = '\0';
    if ( binstart==NULL ) {
	if (( pt = strstr(buffer,"/lenIV"))!=NULL )
	    leniv = strtol(pt+6,NULL,0);
	fputs(buffer,out);
    } else {
	for ( pt=buffer; pt<binstart; ++pt )
	    putc(*pt,out);
	decodebytes(out,(unsigned char *) binstart,binlen);
	for ( pt=binstart+binlen; *pt; ++pt )
	    putc(*pt,out);
    }
return( 1 );
}

static int nrandombytes[4];
#define EODMARKLEN	16

#define bgetc(extra,in)	(*(extra)=='\0' ? getc(in) : (unsigned char ) *(extra)++ )
static void decrypteexec(FILE *in,FILE *temp, int hassectionheads, char *extra) {
    int ch1, ch2, ch3, ch4, binary;
    int zcnt;
    unsigned char zeros[EODMARKLEN+6+1];
    int sect_len;

    while ( (ch1=bgetc(extra,in))!=EOF && (ch1==' ' || ch1=='\t' || ch1=='\n' || ch1=='\r'));
    /* Mac POST resources also have 6 bytes inserted here. They appear to be */
    /*  a four byte length followed by ^B ^@ */
    if ( ch1==0200 && hassectionheads ) {
	/* skip the 6 byte section header in pfb files that follows eexec */
	ch1 = bgetc(extra,in);
	sect_len = bgetc(extra,in);
	sect_len |= bgetc(extra,in)<<8;
	sect_len |= bgetc(extra,in)<<16;
	sect_len |= bgetc(extra,in)<<24;
	sect_len -= 3;
	ch1 = bgetc(extra,in);
    }
    ch2 = bgetc(extra,in); ch3 = bgetc(extra,in); ch4 = bgetc(extra,in);
    binary = 0;
    if ( ch1<'0' || (ch1>'9' && ch1<'A') || ( ch1>'F' && ch1<'a') || (ch1>'f') ||
	     ch2<'0' || (ch2>'9' && ch2<'A') || (ch2>'F' && ch2<'a') || (ch2>'f') ||
	     ch3<'0' || (ch3>'9' && ch3<'A') || (ch3>'F' && ch3<'a') || (ch3>'f') ||
	     ch4<'0' || (ch4>'9' && ch4<'A') || (ch4>'F' && ch4<'a') || (ch4>'f') )
	binary = 1;
    if ( ch1==EOF || ch2==EOF || ch3==EOF || ch4==EOF ) {
return;
    }

    initcode();
    if ( binary ) {
	nrandombytes[0] = decode(ch1);
	nrandombytes[1] = decode(ch2);
	nrandombytes[2] = decode(ch3);
	nrandombytes[3] = decode(ch4);
	zcnt = 0;
	while (( ch1=bgetc(extra,in))!=EOF ) {
	    --sect_len;
	    if ( hassectionheads ) {
		if ( sect_len==0 && ch1==0200 ) {
		    ch1 = bgetc(extra,in);
		    sect_len = bgetc(extra,in);
		    sect_len |= bgetc(extra,in)<<8;
		    sect_len |= bgetc(extra,in)<<16;
		    sect_len |= bgetc(extra,in)<<24;
		    sect_len += 1;
		    if ( ch1=='\1' )
	break;
		} else {
		    zcnt = 0;
		    putc(decode(ch1),temp);
		}
	    } else {
		if ( ch1=='0' || isspace(ch1) ) ++zcnt; else {dumpzeros(temp,zeros,zcnt); zcnt = 0; }
		if ( zcnt>EODMARKLEN )
	break;
		if ( zcnt==0 )
		    putc(decode(ch1),temp);
		else
		    zeros[zcnt-1] = decode(ch1);
	    }
	}
    } else {
	nrandombytes[0] = decode(hex(ch1,ch2));
	nrandombytes[1] = decode(hex(ch3,ch4));
	ch1 = bgetc(extra,in); ch2 = bgetc(extra,in); ch3 = bgetc(extra,in); ch4 = bgetc(extra,in);
	nrandombytes[2] = decode(hex(ch1,ch2));
	nrandombytes[3] = decode(hex(ch3,ch4));
	zcnt = 0;
	while (( ch1=bgetc(extra,in))!=EOF ) {
	    while ( ch1!=EOF && isspace(ch1)) ch1 = bgetc(extra,in);
	    while ( (ch2=bgetc(extra,in))!=EOF && isspace(ch2));
	    if ( ch1=='0' && ch2=='0' ) ++zcnt; else { dumpzeros(temp,zeros,zcnt); zcnt = 0;}
	    if ( zcnt>EODMARKLEN )
	break;
	    if ( zcnt==0 )
		putc(decode(hex(ch1,ch2)),temp);
	    else
		zeros[zcnt-1] = decode(hex(ch1,ch2));
	}
    }
    while (( ch1=bgetc(extra,in))=='0' || isspace(ch1) );
    if ( ch1!=EOF ) ungetc(ch1,in);
}

static void decryptbinary(FILE *in,FILE *temp, char *line, long solpos) {
    int i, cnt, ch;
    char *pt;

    fprintf( stderr, "This program does not handled cid-keyed fonts. Sorry\n" );
exit(1);
    pt = strstr(line,"(Binary)");
    pt += strlen("(Binary)");
    cnt = strtol(pt,NULL,10);

    pt = strstr(line,"StartData ");
    pt += strlen("StartData ");

    solpos += pt-line;
    fseek(in,SEEK_SET,solpos);

    initcode();
    nrandombytes[0] = decode(getc(in));
    nrandombytes[1] = decode(getc(in));
    nrandombytes[2] = decode(getc(in));
    nrandombytes[3] = decode(getc(in));
    for ( i = 0; ( ch=getc(in))!=EOF && i<cnt; ++i )
	putc(decode(ch),temp);
}

static void decryptagain(FILE *temp,FILE *out) {
    char rdtok[255];
    strcpy(rdtok,"RD");
    while ( glorpline(temp,out,rdtok));
}

static void doubledecrypt(char *outputfile,char *fontname) {
    FILE *in, *temp, *out;
    char buffer[256]/*, *tempname*/;
    char *pt;
    int first, hassectionheads;
    int mightbegsf = 1;
    long oldpos;

    leniv = 4;
    useshex = 0;

    in = fopen(fontname,"rb");
    if ( in==NULL ) {
	fprintf( stderr, "Cannot open %s\n", fontname );
return;
    }
    if ( outputfile==NULL ) {
	pt = strrchr(fontname,'/'); if ( pt==NULL ) pt = fontname; else ++pt;
	sprintf( buffer,"%s.decrypt", pt);
	outputfile=buffer;
    }
    out = fopen(outputfile,"wb");
    if ( out==NULL ) {
	fprintf( stderr, "Cannot open %s for output\n", outputfile );
	fclose(in);
return;
    }

    temp = tmpfile();
    if ( temp==NULL ) {
	fprintf( stderr, "Cannot open temporary file\n" );
	fclose(in); fclose(out);
return;
    }

    first = 1; hassectionheads = 0;
    oldpos = ftell(in);
    while ( myfgets(buffer,sizeof(buffer),in)!=NULL ) {
    /* Mac POST resources also have 6 bytes inserted here. They appear to be */
    /*  a four byte length followed by ^A ^@ */
	if ( first && buffer[0]=='\200' ) {
	    hassectionheads = 1;
	    fputs(buffer+6,out);
	} else
	    fputs(buffer,out);
	first = 0;
	if ( strstr(buffer,"currentfile")!=NULL && strstr(buffer, "eexec")!=NULL )
    break;
	if ( strstr(buffer,"(Binary)")!=NULL && strstr(buffer, "StartData")!=NULL )
    break;
	if ( strstr(buffer,"Blend")!=NULL )
	    mightbegsf = 0;
	if ( mightbegsf ) {
	    if ( strstr(buffer,"/Private")!=NULL || strstr(buffer,"/Subrs")!=NULL ||
		    strstr(buffer,"/CharStrings")!=NULL )
    break;
	    if ( strstr(buffer,"/CIDInit")!=NULL )
		mightbegsf = 0;
	}
	oldpos = ftell(in);
    }

    if ( strstr(buffer,"currentfile")!=NULL && strstr(buffer, "eexec")!=NULL ) {
	decrypteexec(in,temp,hassectionheads,strstr(buffer, "eexec")+5);
	rewind(temp);
	decryptagain(temp,out);
    } else if ( strstr(buffer,"(Binary)")!=NULL && strstr(buffer, "StartData")!=NULL ) {
	decryptbinary(in,temp,buffer,oldpos);
	rewind(temp);
	decryptagain(temp,out);
    } else
	decryptagain(in,out);
    while ( myfgets(buffer,sizeof(buffer),in)!=NULL ) {
	if ( buffer[0]!='\200' || !hassectionheads )
	    fputs(buffer,out);
    }
    fclose(in); fclose(out); fclose(temp);
}

int main( int argc, char **argv) {
    int i;
    char *outputfile=NULL;

    for ( i=1; i<argc; ++i ) {
	 if ( strcmp("-o",argv[i])==0 )
	    outputfile = argv[++i];
	else {
	    doubledecrypt(outputfile,argv[i]);
	}
    }
return( 0 );
}
