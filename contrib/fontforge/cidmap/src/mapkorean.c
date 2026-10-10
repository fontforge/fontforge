#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utype.h"

static char used[0x110000];
static int cid_2_unicode[0x10000];
static char *nonuni_names[0x10000];
static int cid_2_rotunicode[0x10000];
#define MULT_MAX	6
static int cid_2_unicodemult[0x100000][MULT_MAX];

#define VERTMARK 0x1000000

static int ucs2_score(int val) {		/* Supplied by KANOU Hiroki */
    if ( val>=0x2e80 && val<=0x2fff )
return( 1 );				/* New CJK Radicals are least important */
    else if ( val>=VERTMARK )
return( 2 );				/* Then vertical guys */
    else if ( val>=0xf000 && val<=0xffff )
return( 3 );
/*    else if (( val>=0x3400 && val<0x3dff ) || (val>=0x4000 && val<=0x4dff))*/
    else if ( val>=0x3400 && val<=0x4dff )
return( 4 );
    else
return( 5 );
}

#if 0
static int allstars(char *buffer) {
    while ( isdigit(*buffer))
	++buffer;
    while ( *buffer=='\t' || *buffer=='*' )
	++buffer;
    if ( *buffer=='\r' ) ++buffer;
    if ( *buffer=='\n' ) ++buffer;
return( *buffer=='\0' );
}
#endif

static int getnth(char *buffer, int col, int *mults) {
    int i,j=0, val=0, best;
    char *end;
    int vals[10];

    if ( col==1 ) {
	/* first column is decimal, others are hex */
	if ( !isdigit(*buffer))
return( -1 );
	while ( isdigit(*buffer))
	    val = 10*val + *buffer++-'0';
return( val );
    }
    for ( i=1; i<col; ++buffer ) {
	if ( *buffer=='\t' )
	    ++i;
	else if ( *buffer=='\0' )
return( -1 );
    }
    val = strtol(buffer,&end,16);
    if ( end==buffer )
return( -1 );
    if ( *end=='v' ) {
	val += VERTMARK;
	++end;
    }
    if ( *end==',' ) {
	/* Multiple guess... now we've got to pick one */
	vals[0] = val;
	i = 1;
	while ( *end==',' && i<9 ) {
	    buffer = end+1;
	    vals[i] = strtol(buffer,&end,16);
	    if ( *end=='v' ) {
		vals[i] += VERTMARK;
		++end;
	    }
	    ++i;
	}
	vals[i] = 0;
	best = 0; val = -1;
	for ( i=0; vals[i]!=0; ++i ) {
	    if ( ucs2_score(vals[i])>best ) {
		val = vals[i];
		best = ucs2_score(vals[i]);
	    }
	}
	if ( mults!=NULL ) for ( i=j=0; vals[i]!=0; ++i ) {
	    if ( vals[i]!=val && !(vals[i]&VERTMARK))
		mults[j++] = vals[i];
	}
	if ( j>MULT_MAX ) {
	    fprintf( stderr, "Too many multiple values for %04x, need %d slots\n", (unsigned int)(val), j );
exit(1);
	}
    }

return( val );
}

int main(int argc, char **argv) {
    char buffer[600];
    int cid, uni, max=0, maxcid=0, i,j, fakeuni;

    nonuni_names[0] = ".notdef";
    for ( cid=0; cid<0x10000; ++cid ) cid_2_unicode[cid] = -1;

    while ( fgets(buffer,sizeof(buffer),stdin)!=NULL ) {
	if ( *buffer=='#' /*|| allstars(buffer)*/)
    continue;
	cid = getnth(buffer,1,NULL);
	if ( cid==-1 )
    continue;
	uni = getnth(buffer,11,cid_2_unicodemult[cid]);
	fakeuni = -1;
	maxcid = cid;
	if ( cid>=8094 && cid<=8190 ) {
	    /* halfwidth latin */
	    if ( uni==-1 ) {
		fakeuni = cid-8094+' ';
		if ( cid==8154 ) fakeuni = 0x20a9;	/* Won sign */
		else if ( cid==8190 ) fakeuni = 8154-8094+' ';
	    }
	    /*if ( psunicodenames[uni]!=NULL )
		sprintf( buffer, "%s.hw", psunicodenames[uni]);
	    else*/
		sprintf( buffer, "uni%04X.hw", (unsigned int)(fakeuni) );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( cid>=18255 && cid<=18351 ) {
	    /* rotated halfwidth latin */
	    sprintf( buffer, "Korea1.%d.vert", cid-18255+8094 );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( cid>=18155 && cid<=18254 ) {
	    /* rotated proportional latin */
	    sprintf( buffer, "Korea1.%d.vert", cid-18155+1 );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( uni>=VERTMARK ) {
	    /* rotated */
	    cid_2_rotunicode[cid] = uni-VERTMARK;
	} else if ( uni==-1 ) {
    continue;
	    sprintf( buffer,"Korea1.%d", cid );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( !used[uni] ) {
	    used[uni] = 1;
	    cid_2_unicode[cid] = uni;
	} else {
	    sprintf( buffer, "uni%04X.dup%d", (unsigned int)(uni), ++used[uni] );
	    nonuni_names[cid] = strdup(buffer);
	}
	max = cid;
    }
    for ( i=0; i<maxcid; ++i ) if ( cid_2_rotunicode[i]!=0 ) {
	for ( j=0; j<maxcid; ++j )
	    if ( cid_2_unicode[j] == cid_2_rotunicode[i] )
	break;
	if ( j==maxcid )
	    sprintf( buffer, "uni%04X.vert", (unsigned int)(cid_2_rotunicode[i]) );
	else
	    sprintf( buffer, "Korea1.%d.vert", j);
	nonuni_names[i] = strdup(buffer);
    }

    printf("%d %d\n",maxcid, max );

    for ( cid=0; cid<=max; ++cid ) {
	if ( cid_2_unicode[cid]!=-1 && cid_2_unicodemult[cid][0]==0 ) {
	    for ( i=1; cid+i<=max && cid_2_unicode[cid+i]==cid_2_unicode[cid]+i && cid_2_unicodemult[cid+i][0]==0; ++i );
	    --i;
	    if ( i!=0 ) {
		printf( "%d..%d %04x\n", cid, cid+i, (unsigned int)(cid_2_unicode[cid]) );
		cid += i;
	    } else
		printf( "%d %04x\n", cid, (unsigned int)(cid_2_unicode[cid]) );
	} else if ( cid_2_unicode[cid]!=-1 ) {
	    printf( "%d %04x", cid, (unsigned int)(cid_2_unicode[cid]) );
	    for ( i=0; cid_2_unicodemult[cid][i]!=0; ++i )
		printf( ",%04x", (unsigned int)(cid_2_unicodemult[cid][i]) );
	    printf( "\n");
	} else if ( nonuni_names[cid]!=NULL )
	    printf( "%d /%s\n", cid, nonuni_names[cid] );
    }
return( 0 );
}
