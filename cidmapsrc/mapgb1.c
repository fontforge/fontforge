#include <stdio.h>
#include <ctype.h>
#include <string.h>

static char used[0x10000];
static int cid_2_unicode[0x10000];
static char *nonuni_names[0x10000];
static int cid_2_rotunicode[0x10000];

#define VERTMARK 0x1000000

static int allstars(char *buffer) {
    while ( isdigit(*buffer))
	++buffer;
    while ( *buffer=='\t' || *buffer=='*' )
	++buffer;
    if ( *buffer=='\r' ) ++buffer;
    if ( *buffer=='\n' ) ++buffer;
return( *buffer=='\0' );
}

static int getnth(char *buffer, int col) {
    int i, val=0;
    char *end;

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
    if ( *end=='v' )
	val += VERTMARK;

return( val );
}

int main(int argc, char **argv) {
    char buffer[600];
    int cid, uni, max=0, maxcid=0, i,j;
    extern char *psunicodenames[];

    nonuni_names[0] = ".notdef";
    for ( cid=0; cid<0x10000; ++cid ) cid_2_unicode[cid] = -1;

    while ( fgets(buffer,sizeof(buffer),stdin)!=NULL ) {
	if ( *buffer=='#' /*|| allstars(buffer)*/)
    continue;
	cid = getnth(buffer,1);
	uni = getnth(buffer,11);
	if ( cid==-1 )
    continue;
	maxcid = cid;
	if ( ( (cid>=814 && cid<=907) || cid==7716 ) && uni!=-1 ) {
/* 814-907, 7716 are halfwidth */
	    sprintf( buffer,"uni%04X.hw", uni );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( cid>=22226 && cid<=22319 ) {
/* 22226-22352 are rotated halfwidth latin */
	    sprintf( buffer,"cid_%d.vert", cid-22226+814 );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( cid>=22127 && cid<=22221 ) {
/* 22127-22225 are rotated proportional latin */
	    sprintf( buffer,"cid_%d.vert", cid-22127+1 );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( cid>=29059 && cid<=29063 ) {
/* 29059-29063 rotated 22353-22357 */
	    sprintf( buffer,"cid_%d.vert", cid-29059+22353 );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( uni==-1 ) {
    continue;
	    sprintf( buffer,"gb1_%d", cid );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( uni>VERTMARK ) {
	    /* rotated */
	    cid_2_rotunicode[cid] = uni-VERTMARK;
	} else if ( !used[uni] ) {
	    used[uni] = 1;
	    cid_2_unicode[cid] = uni;
	} else {
	    sprintf( buffer, "uni%04X.dup%d", uni, ++used[uni] );
	    nonuni_names[cid] = strdup(buffer);
	}
	max = cid;
    }
    for ( i=0; i<maxcid; ++i ) if ( cid_2_rotunicode[i]!=0 ) {
	for ( j=0; j<maxcid; ++j )
	    if ( cid_2_unicode[j] == cid_2_rotunicode[i] )
	break;
	if ( j==maxcid )
	    sprintf( buffer, "uni%04X.vert", cid_2_rotunicode[i]);
	else
	    sprintf( buffer, "cid_%d.vert", j);
	nonuni_names[i] = strdup(buffer);
    }

    printf("%d %d\n",maxcid, max );

    for ( cid=0; cid<=max; ++cid ) {
	if ( cid_2_unicode[cid]!=-1 ) {
	    for ( i=1; cid+i<=max && cid_2_unicode[cid+i]==cid_2_unicode[cid]+i; ++i );
	    --i;
	    if ( i!=0 ) {
		printf( "%d..%d %04x\n", cid, cid+i, cid_2_unicode[cid] );
		cid += i;
	    } else
		printf( "%d %04x\n", cid, cid_2_unicode[cid] );
	} else if ( nonuni_names[cid]!=NULL )
	    printf( "%d /%s\n", cid, nonuni_names[cid] );
    }
return( 0 );
}
