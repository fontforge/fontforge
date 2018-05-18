/* Copyright (C) 2003 by George Williams */
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

/* This little program is designed to fix up a bug in versions of pfaedit */
/*  before 15 June 2003. PfaEdit would set the ignore-combining-marks flag */
/*  bit on all ligatures whether appropriate or not */
/* This program will unset that bit... whether inappropriate or not */

/* Takes a list of sfd filenames as arguments. Produces a set of files with */
/*  -new added before the extension (Caslon.sfd -> Caslon-new.sfd) */
/* Will also read from stdin and write to stdout */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ProcessFiles(FILE *in, FILE *out) {
    char line[1000];
    const char *match1 = "Ligature: 8", *match2 = "Ligature: 9";
    int matchlen = strlen(match1);

    while ( fgets(line,sizeof(line),in)!=NULL ) {
	if ( strncmp(line,match1,matchlen)==0 )
	    line[matchlen-1] = '0';
	else if ( strncmp(line,match2,matchlen)==0 )
	    line[matchlen-1] = '1';
	fputs(line,out);
    }
}

static void ProcessFilename(char *name) {
    char buffer[1000];
    FILE *in, *out;

    strcpy(buffer,name);
    if ( strlen(buffer)>4 && strcmp(buffer+strlen(buffer)-4,".sfd")==0 )
	strcpy(buffer+strlen(buffer)-4,"-new.sfd");
    else
	strcat(buffer,"-new");
    in = fopen(name,"r");
    if ( in==NULL ) {
	fprintf( stderr, "Could not open %s for reading\n", name );
exit(1);
    }
    out = fopen(buffer,"w");
    if ( out==NULL ) {
	fclose(in);
	fprintf( stderr, "Could not open %s for writing\n", buffer );
exit(1);
    }
    ProcessFiles(in,out);
    fclose(in);
    fclose(out);
}
	
int main(int argc, char **argv) {
    int i;

    if ( argc==1 )
	ProcessFiles(stdin,stdout);
    else {
	for ( i=1; i<argc; ++i )
	    ProcessFilename(argv[i]);
    }
return( 0 );
}
