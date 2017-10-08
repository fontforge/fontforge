/* Copyright (C) 2004-2012 by George Williams */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <gutils.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* Update copyright notices to a new year */

#define OLD	"2011"
#define NEW	"2012"

static int IsCopyright(char *orig,char *buffer) {
    char *pt;

    if ( strstr(orig,"/* Copyright (C)")==NULL )
return( 0 );
    if ( strstr(orig,NEW)!=NULL )		/* Already current? if so ignore */
return( 0 );
    if ( (pt=strstr(orig,OLD))==NULL )		/* Unexpected? if so, ignore */
return( 0 );

    strncpy(buffer,orig,pt-orig); buffer[pt-orig] = '\0';
    /* look for three forms of copyright: "(C) OLD by", "(C) 2002,OLD by", "(C) 2001-OLD by" */
    if ( pt[-1]=='-' )
	strcat(buffer,NEW);
    else if ( pt[-1]==',' ) {
	buffer[(pt-orig)-1]='-';
	strcat(buffer,NEW);
    } else if ( pt[-1]==' ' && pt[strlen(OLD)]==' ' ) {
	strcat(buffer,OLD);
	strcat(buffer,",");
	strcat(buffer,NEW);
    } else
return( 0 );

    strcat(buffer,pt+strlen(OLD));

return( 1 );
}

static void ProcessFile(char *filename,FILE *output, char *dir) {
    char buffer[1024];
    FILE *src;
    char *lines[4];
    int i,j;
    struct stat sb;
    time_t now;
    struct tm *tm;

    src = fopen(filename,"r");
    if ( src==NULL ) {
	fprintf( stderr, "Failed to open %s/%s\n", dir, filename);
exit(1);
    }
    for ( i=0; i<4; ++i ) {
	if ( fgets(buffer,sizeof(buffer),src)==NULL ) {
	    fclose(src);
	    for ( j=0; j<i; ++j ) free(lines[j]);
return;
	}
	lines[i] = strdup(buffer);
    }
    fclose(src);
    if ( IsCopyright(lines[0],buffer)) {
	stat(filename,&sb);
	tm = localtime(&sb.st_mtime);
	fprintf( output, "*** %s~\t%d-%0d-%0d %02d:%02d:%02d.000000000 -0800\n", filename,
	    tm->tm_year+1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
	now = GetTime();
	tm = localtime(&now);
	fprintf( output, "--- %s\t%d-%0d-%0d %02d:%02d:%02d.000000000 -0800\n", filename,
	    tm->tm_year+1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
	fprintf( output, "***************\n" );
	fprintf( output, "*** 1,4 ****\n" );
	fprintf( output, "! %s", lines[0] );
	fprintf( output, "  %s", lines[1] );
	fprintf( output, "  %s", lines[2] );
	fprintf( output, "  %s", lines[3] );
	fprintf( output, "--- 1,4 ----\n" );
	fprintf( output, "! %s", buffer );
	fprintf( output, "  %s", lines[1] );
	fprintf( output, "  %s", lines[2] );
	fprintf( output, "  %s", lines[3] );
    }
    for ( j=0; j<4; ++j ) free(lines[j]);
}

static void ProcessDir(char *dir) {
    char buffer[1025];
    char *here = strdup(getcwd(buffer,sizeof(buffer)));
    DIR *d;
    struct dirent *ent;
    FILE *output;

    if ( chdir(dir)==-1 ) {
	fprintf(stderr, "Failed on %s\n", dir );
exit( 1 );
    }
    d = opendir(".");
    if ( d==NULL ) {
	fprintf(stderr, "Failed on %s\n", dir );
exit( 1 );
    }
    output = fopen("copyright.patch","w");
    if ( output==NULL ) {
	fprintf(stderr, "Failed to create \"copyright.patch\" in %s\n", dir );
exit( 1 );
    }
    while ( (ent = readdir(d))!=NULL ) {
	int len = strlen( ent->d_name );
	if ( ent->d_name[len-2]=='.' &&
		(ent->d_name[len-1]=='c' || ent->d_name[len-1]=='h'))
	    ProcessFile(ent->d_name,output, dir);
    }
    if ( ftell(output)==0 ) {
	fprintf( stderr, "Nothing in %s\n", dir );
	fclose(output);
	unlink("copyright.patch");
    } else
	fclose(output);
    closedir(d);
    chdir(here);
    free(here);
}

int main(int argc, char **argv) {
    int i;

    if ( argc==1 )
	ProcessDir(".");
    else {
	for ( i=1 ; i<argc; ++i )
	    ProcessDir(argv[i]);
    }
return( 0 );
}
