/* A little program to help create the en_GB.po file. */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define true 1
#define false 0

struct {
    char *us, *gb;
} words[] = {
    { "color", "colour" },
/*    { "program", "programme" }, */		/* programme except for computer programs */
    { "CCW", "ACW" },	/* Counter/Anti Clockwise */
    { "neighbor", "neighbour" },
    { "Adobe now considers", "Adobe now consider" },
    { "behavior", "behaviour" },
    { "centered", "centred" },
    { "center", "centre" },
    { "dialog", "dialogue" },
    { "license", "licence" },
    { "argument", "arguement" },
    { "delimiter", "delimiter" },	/* Otherwise the miter transform would lead to misspellings */
    { "miter", "mitre" },
    { "recognize", "recognise" },
    { "realize", "realise" },
/* careful about meter/metre because parameter does not become parametre */
    NULL
};

/* If we were to call the following function strcasestr(), on some
 * systems (PC-BSD 9.0 in particular) the confusion with the native
 * strcasestr() causes compilation problems. */
static char *my_strcasestr(const char *haystack, const char *needle) {
    const char *npt, *hpt;
    int hch, nch;

    for ( ; *haystack!='\0' ; ++haystack ) {
	for ( hpt = haystack, npt = needle; *npt!='\0'; ++hpt, ++npt ) {
	    hch = *hpt; nch = *npt;
	    if ( isupper(hch)) hch = tolower(hch);
	    if ( isupper(nch)) nch = tolower(nch);
	    if ( nch!=hch )
	break;
	}
	if ( *npt=='\0' )
return( (char *) haystack );
    }
return( NULL );
}

static void caserpl(char *to,int flen, char *rpl) {
    int i, ch=0, och;

    for ( i=0 ; i<flen; ++i ) {
	ch = *rpl++;
	if ( ch=='\0' )
return;
	if ( isupper(to[i]) && islower(ch) )
	    ch = toupper(ch);
	else if ( isupper(ch) && islower(to[i]) )
	    ch = tolower(ch);
	to[i] = ch;
    }
    och = ch;
    while ( *rpl!='\0' ) {
	ch = *rpl++;
	if ( isupper(och) && islower(ch) )
	    ch = toupper(ch);
	else if ( isupper(ch) && islower(och) )
	    ch = tolower(ch);
	to[i++] = ch;
    }
}

static void replace(char *line, char *find, char *rpl) {
    char *pt=line, *mpt;
    int flen = strlen(find), rlen = strlen(rpl);
    int len = rlen - flen;

    while ( (pt = my_strcasestr(pt,find))!=NULL ) {
	if ( len>0 ) {
	    for ( mpt = line+strlen(line); mpt>=pt+flen; --mpt )
		mpt[len] = *mpt;
	    caserpl(pt,flen,rpl);
	} else if ( len<0 ) {
	    caserpl(pt,flen,rpl);
	    for ( mpt=pt+flen; *mpt; ++mpt )
		mpt[len] = *mpt;
	    mpt[len] = 0;
	} else
	    caserpl(pt,flen,rpl);
	pt += rlen;
    }
}

#define LINE_MAX	40
static char linebuffers[LINE_MAX][200];

static int anyneedles(int start, int end) {
    int l,i;

    for ( l=start; l<end; ++l ) {
	char *lpt = linebuffers[l], *pt;
	if ( l==start && (pt=strchr(lpt,'|'))!=NULL )
	    lpt = pt+1;
	for ( i=0; words[i].us!=NULL; ++i ) {
	    if ( my_strcasestr(lpt,words[i].us)!=NULL ) {
		if ( strcmp(words[i].us,words[i].gb)==0 )		/* The word delimiter is the same, but miter isn't. So we include delimiter first to make sure miter isn't found */
return( false );
		else
return( true );
	    }
	}
    }
return( false );
}

static void rplall(int l) {
    int i;

    for ( i=0; words[i].us!=NULL; ++i )
	replace( linebuffers[l], words[i].us, words[i].gb);
}

int main(int argc, char **argv) {
    time_t now;
    struct tm *tm;
    FILE *input, *output;
    int i,l, start, end;
    char *pt;

    input = fopen("FontForge.pot", "r");
    if ( input==NULL ) {
	fprintf( stderr, "No pot file\n" );
return( 1 );
    }
    output = fopen("en_GB.po", "w");
    if ( output==NULL ) {
	fprintf( stderr, "Can't create en_GB.po\n" );
	fclose( input );
return( 1 );
    }

    time(&now);
    tm = gmtime(&now);

    fprintf( output, "# (British) English User Interface strings for FontForge.\n" );
    fprintf( output, "# Copyright (C) 2000-2006 by George Williams\n" );
    fprintf( output, "# This file is distributed under the same license as the FontForge package.\n" );
    fprintf( output, "# George Williams, <pfaedit@users.sourceforge.net>, %d.\n", tm->tm_year+1900 );
    fprintf( output, "#\n" );
    fprintf( output, "#, fuzzy\n" );
    fprintf( output, "msgid \"\"\n" );
    fprintf( output, "msgstr \"\"\n" );
    fprintf( output, "\"Project-Id-Version: %4d%02d%02d\\n\"\n", tm->tm_year+1990, tm->tm_mon+1, tm->tm_mday );
    fprintf( output, "\"POT-Creation-Date: 2006-05-07 20:02-0700\\n\"\n" );
    fprintf( output, "\"PO-Revision-Date: %4d-%02d-%02d %02d:%02d-0800\\n\"\n",
	    tm->tm_year+1990, tm->tm_mon+1, tm->tm_mday,
	    tm->tm_hour, tm->tm_min );
    fprintf( output, "\"Last-Translator: George Williams, <pfaedit@users.sourceforge.net>\\n\"\n" );
    fprintf( output, "\"Language-Team: LANGUAGE <LL@li.org>\\n\"\n" );
    fprintf( output, "\"MIME-Version: 1.0\\n\"\n" );
    fprintf( output, "\"Content-Type: text/plain; charset=UTF-8\\n\"\n" );
    fprintf( output, "\"Content-Transfer-Encoding: 8bit\\n\"\n" );
    fprintf( output, "\"Plural-Forms: nplurals=2; plural=n!=1\\n\"\n" );

    while ( fgets(linebuffers[0],sizeof(linebuffers[0]),input)!=NULL ) {
	if ( linebuffers[0][0]=='\n' )
    break;
    }

    while ( !feof(input)) {
	l=0;
	while ( l<LINE_MAX && fgets(linebuffers[l],sizeof(linebuffers[0]),input)!=NULL ) {
	    if ( linebuffers[l][0]=='\n' )
	break;
	    ++l;
	}
	if ( l==0 )
    continue;
	if ( l==LINE_MAX ) {
	    fprintf( stderr, "Increase LINE_MAX. MSG:\n%s\n", linebuffers[0] );
	    fclose( output );
	    fclose( input );
return( 1 );
	}
	for ( i=0; i<l; ++i )
	    if ( strncmp(linebuffers[i],"msgid",5)==0 )
	break;
	if ( i==l ) {
	    for ( i=0; i<l; ++i )
		if ( linebuffers[l-1][0]!='#' )
	    break;
	    if ( i!=l )
		fprintf( stderr, "Didn't understand: %s\n", linebuffers[0] );
		/* But comments are ok */
    continue;
	}
	start = i;
	for (    ; i<l; ++i )
	    if ( strncmp(linebuffers[i],"msgstr",6)==0 )
	break;
	if ( i==l ) {
	    fprintf( stderr, "Didn't understand (2): %s\n", linebuffers[0] );
    continue;
	}
	end = i;
	if ( !anyneedles(start,end))
    continue;
	fprintf( output, "\n" );
	for ( i=0; i<end; ++i )
	    fprintf( output, "%s", linebuffers[i] );
	replace(linebuffers[start],"msgid","msgstr");
	pt = strchr(linebuffers[start],'|');
	if ( pt!=NULL ) {
	    char *qpt = strchr(linebuffers[start],'"');
	    int j;
	    if ( qpt!=NULL ) {
		j=1;
		do {
		    qpt[j] = pt[j];
		    ++j;
		} while ( pt[j]!='\0' );
		qpt[j] = '\0';
	    }
	}
	for ( i=start; i<end; ++i ) {
	    rplall(i);
	    fprintf( output, "%s", linebuffers[i] );
	}
    }

    fclose( output );
    fclose( input );
return( 0 );
}
