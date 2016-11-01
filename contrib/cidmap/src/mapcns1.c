#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char used[0x110000];
static int cid_2_unicode[0x10000];
static char *nonuni_names[0x10000];
static int cid_2_rotunicode[0x10000];
static int puamap[0x2000];
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

static int allstars(char *buffer) {
    while ( isdigit(*buffer))
	++buffer;
    while ( *buffer=='\t' || *buffer=='*' )
	++buffer;
    if ( *buffer=='\r' ) ++buffer;
    if ( *buffer=='\n' ) ++buffer;
return( *buffer=='\0' );
}

static int getnth(char *buffer, int col, int *mults) {
    int i,j=0, val=0, best;
    char *end;
    int vals[10];
    int pua, nonbmp;

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
	pua = nonbmp = 0;
	for ( i=0; vals[i]!=0; ++i ) {
	    if ( ucs2_score(vals[i])>best ) {
		val = vals[i];
		best = ucs2_score(vals[i]);
	    }
	    if ( vals[i]>=0xe000 && vals[i]<=0xf8ff )
		pua = vals[i];
	    else if ( vals[i]>0x10000 && vals[i]<VERTMARK )
		nonbmp = vals[i];
	}
	if ( pua!=0 && nonbmp!=0 )
	    puamap[pua-0xe000] = nonbmp;
	if ( mults!=NULL ) for ( i=j=0; vals[i]!=0; ++i ) {
	    if ( vals[i]!=val && !(vals[i]&VERTMARK))
		mults[j++] = vals[i];
	}
	if ( j>MULT_MAX ) {
	    fprintf( stderr, "Too many multiple values for %04x, need %d slots\n", val, j );
exit(1);
	}
    }

return( val );
}

int main(int argc, char **argv) {
    char buffer[600];
    int cid, uni, max=0, maxcid=0, i,j;
    extern char *psunicodenames[];
    FILE *pua;

    nonuni_names[0] = ".notdef";
    for ( cid=0; cid<0x10000; ++cid ) cid_2_unicode[cid] = -1;

    while ( fgets(buffer,sizeof(buffer),stdin)!=NULL ) {
	if ( *buffer=='#' /*|| allstars(buffer)*/)
    continue;
	cid = getnth(buffer,1,NULL);
	if ( cid==-1 )
    continue;
	uni = getnth(buffer,12,cid_2_unicodemult[cid]);
	maxcid = cid;
	if ( (cid>=17408 && cid<=17599) || cid==17604 || cid==17605 ) {
	    if ( cid>=17408 && cid<=17505 )		/* proportional */
		sprintf( buffer,"CNS1.%d.vert", cid-17408+1 );
	    else if ( cid>=17506 && cid<=17599 )	/* Half width */
		sprintf( buffer,"CNS1.%d.vert", cid-17506+13648 );
	    else if ( cid==17604 )
		strcpy( buffer, "CNS1.17601.vert" );
	    else if ( cid==17605 )
		strcpy( buffer, "CNS1.17603.vert" );
	    nonuni_names[cid] = strdup(buffer);
	/* Adobe's CNS cids have the rotated and non-rotated forms intermixed */
	} else if ( (cid>=120 && cid<=127 ) && (cid&1) ) {
	    sprintf( buffer,"CNS1.%d.vert", cid-1 );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( (cid>=128 && cid<=159) && (cid&2) ) {
	    sprintf( buffer,"CNS1.%d.vert", cid-2 );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( cid>=13648 && cid<=13741 ) {
	    sprintf( buffer, "uni%04X.hw", cid-13648+' ' );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( cid==13742 ) {
	    strcpy( buffer, "uni203E.hw" );
	} else if ( uni==-1 ) {
    continue;
	    sprintf( buffer,"CNS1.%d", cid );
	    nonuni_names[cid] = strdup(buffer);
	} else if ( uni>=VERTMARK ) {
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
	    sprintf( buffer, "CNS1.%d.vert", j);
	nonuni_names[i] = strdup(buffer);
    }

    printf("%d %d\n",maxcid, max );

    for ( cid=0; cid<=max; ++cid ) {
	if ( cid_2_unicode[cid]!=-1 && cid_2_unicodemult[cid][0]==0 ) {
	    for ( i=1; cid+i<=max && cid_2_unicode[cid+i]==cid_2_unicode[cid]+i && cid_2_unicodemult[cid+i][0]==0; ++i );
	    --i;
	    if ( i!=0 ) {
		printf( "%d..%d %04x\n", cid, cid+i, cid_2_unicode[cid] );
		cid += i;
	    } else
		printf( "%d %04x\n", cid, cid_2_unicode[cid] );
	} else if ( cid_2_unicode[cid]!=-1 ) {
	    printf( "%d %04x", cid, cid_2_unicode[cid]);
	    for ( i=0; cid_2_unicodemult[cid][i]!=0; ++i )
		printf( ",%04x", cid_2_unicodemult[cid][i]);
	    printf( "\n");
	} else if ( nonuni_names[cid]!=NULL )
	    printf( "%d /%s\n", cid, nonuni_names[cid] );
    }

    pua = fopen("cns14.pua","w");
    if (pua) {
        for ( i=0; i<0xf8ff-0xe000; i+=8 ) {
            int j;
            fprintf(pua, "/* %0X */\t", i+0xe000 );
            for ( j=0; j<8; ++j ) {
                if ( puamap[i+j]!=0 )
                    fprintf(pua, "0x%05x,%s", puamap[i+j], j==7 ? "\n" : " " );
                else
                    fprintf(pua, "    0x0,%s", j==7 ? "\n" : " " );
            }
        }
        fclose(pua);
    }

return( 0 );
}
