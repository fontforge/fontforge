/* Copyright (C) 2005 by George Williams */
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <signal.h>
#include <time.h>

#ifdef TEST_FREETYPE
# include <ft2build.h>
# include FT_FREETYPE_H
# include FT_OUTLINE_H
#endif

#define true	1
#define false	0


#ifdef TEST_FREETYPE
static int check_outlines = false;
static int nohints = false;
static int rasterize = false;
#else
static char *command = "fontforge";
#endif
static char *results_dir = "results";
#define GOOD_FONTS_DIR	"/home/gww/myfonts/fontforge/others"
static char *default_dir_list[] = {
    GOOD_FONTS_DIR,
    NULL
};
static char *default_ext_list[] = {
    "ttf",
    "otf",
    "ttc",
    "cid",
    "pfb",
    "pfa",
    "bdf",
#ifdef TEST_FREETYPE
#else
    "sfd",
    "svg",
    "pt3",
#endif
    NULL
};
static int error_count = 1;
static int error_fraction = 0;

static struct fontlist {
    char *name;
    int len;
    unsigned int isbinary: 1;
    unsigned int isascii: 1;
    unsigned int ishex: 1;
} *fontlist;
static int fcnt;

static int extmatch(char *filename,char **extensions) {
    int i;
    char *pt;

    if ( extensions==NULL )
return( true );
    pt = strrchr(filename,'.');
    if ( pt==NULL )
return( false );
    if ( pt<strrchr(filename,'/'))
return( false );
    for ( i=0; extensions[i]!=NULL; ++i )
	if ( strcasecmp(pt+1,extensions[i])==0 || strcasecmp(pt,extensions[i])==0 )
return( true );

return( false );
}

static void figurefiletype(struct fontlist *item) {
    FILE *foo;

    item->isbinary = item->isascii = item->ishex = false;

    foo = fopen(item->name,"rb");
    if ( foo!=NULL ) {
	/* Try to guess the file type from the first few characters... */
	int ch1 = getc(foo);
	int ch2 = getc(foo);
	int ch3 = getc(foo);
	int ch4 = getc(foo);
	int ch5 = getc(foo);
	int ch6 = getc(foo);
	int ch7 = getc(foo);
	fclose(foo);
	if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		(ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		(ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ||
		(ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f') ) {
	    /* ttf, otf, ttc files */
	    item->isbinary = true;
	} else if ( ch1==0x80 && ch2=='\01' ) {
	    /* PFB header */
	    item->isbinary = true;
	} else if ( ch1=='%' && ch2=='!' ) {
	    /* Random PostScript */
	    if ( strstr(item->name,".pfa")!=NULL || strstr(item->name,".PFA")!=NULL )
		item->ishex = true;
	    else
		item->isascii = true;
	} else if ( ch1==1 && ch2==0 && ch3==4 ) {
	    /* Bare CFF */
	    item->isbinary = true;
	} else if ( ch1=='<' && ch2=='?' && (ch3=='x'||ch3=='X') && (ch4=='m'||ch4=='M') ) {
	    /* SVG */
	    item->isascii = true;
	} else if ( ch1==0xef && ch2==0xbb && ch3==0xbf &&
		ch4=='<' && ch5=='?' && (ch6=='x'||ch6=='X') && (ch7=='m'||ch7=='M') ) {
	    /* UTF-8 SVG with initial byte ordering mark */
	    item->isascii = true;
	} else if ( ch1=='S' && ch2=='p' && ch3=='l' && ch4=='i' ) {
	    /* SFD */
	    item->isascii = true;
	} else if ( ch1=='S' && ch2=='T' && ch3=='A' && ch4=='R' ) {
	    /* BDF */
	    item->ishex = true;
	} else if ( ch1=='\1' && ch2=='f' && ch3=='c' && ch4=='p' ) {
	    /* Windows FON */
	    item->isbinary = true;
	} else {
	    fprintf( stderr, "Didn't recognize file type of %s, assumed binary\n", item->name );
	    item->isbinary = true;
	}
    } else {
	fprintf( stderr, "Could not open %s for typing the file.\n", item->name );
	item->isbinary = true;
    }
}

static void FindFonts(char **fontdirs,char **extensions) {
    DIR *examples;
    struct dirent *ent;
    int i, max;
    char buffer[1025];
    struct stat statb;

    max = 0;
    fcnt = 0;

    for ( i=0; fontdirs[i]!=NULL; ++i ) {
	examples = opendir(fontdirs[i]);
	if ( examples==NULL ) {
	    fprintf( stderr, "Could not open example font directory.\n\t%s\n", fontdirs[i] );
exit(1);
	}

	while ( (ent = readdir(examples))!=NULL ) {
	    snprintf(buffer,sizeof(buffer),"%s/%s", fontdirs[i], ent->d_name );
	    if ( stat(buffer,&statb)==-1 || S_ISDIR(statb.st_mode))
	continue;
	    if ( extensions==NULL || extmatch(buffer,extensions)) {
		if ( fcnt>=max ) {
		    max += 100;
		    fontlist = realloc(fontlist,max*sizeof(struct fontlist));
		    if ( fontlist==NULL ) {
			fprintf( stderr, "Couldn't allocate memory\n" );
exit(1);
		    }
		}
		fontlist[fcnt].name = strdup(buffer);
		fontlist[fcnt].len = statb.st_size;
		figurefiletype(&fontlist[fcnt]);
		++fcnt;
	    }
	}
	closedir(examples);
    }
    if ( fcnt==0 ) {
	fprintf( stderr, "No matching font files found.\n" );
exit(1);
    }
    fontlist[fcnt].name = NULL;
}

static int getErrorCnt(struct fontlist *item) {
    if ( error_count==0 && error_fraction==0 )
return( 0 );

return( error_count + ceil(error_fraction*item->len));
}

static int getRandom(int low, int high) {
    if ( low-high<0x10000 )
return( low + ((g_random_int()>>8)%(high+1-low)) );

return( low + (g_random_int()%(high+1-low)) );
}

static int copyfont(struct fontlist *item,char *newfont) {
    static char buffer[8096];
    FILE *good, *new;
    int len;
    int i, err_cnt;

    good = fopen(item->name,"r");
    if ( good==NULL ) {
	fprintf( stderr, "Could not open %s\n", item->name );
return( false );
    }
    new = fopen(newfont,"w+");
    if ( new==NULL ) {
	fprintf( stderr, "Could not create temporary output file %s\n", newfont );
exit( 1 );
    }

    while ( (len = fread(buffer,1,sizeof(buffer), good ))>0 )
	fwrite(buffer,1,len,new);
    fclose(good);

    err_cnt = getErrorCnt(item);
    for ( i=0; i<err_cnt; ++i ) {
	fseek(new,getRandom(0,item->len-1),SEEK_SET);
	if ( item->isbinary )
	    putc(getRandom(0,0xff),new);
	else if ( item->isascii )
	    putc(getRandom(0x20,0x7e),new);
	else {
	    int hex = getRandom(0,15);
	    if ( hex<10 ) hex += '0';
	    else hex += 'A'-10;
	    putc(hex,new);
	}
    }
    if ( ferror(new)) {
	fclose(new);
	unlink(newfont);
return( false );
    }
    fclose(new);
return( true );
}

#ifdef TEST_FREETYPE
static int FT_MoveTo(FT_Vector *to,void *user) {
return( 0 );
}

static int FT_LineTo(FT_Vector *to,void *user) {
return( 0 );
}

static int FT_ConicTo(FT_Vector *_cp, FT_Vector *to,void *user) {
return( 0 );
}

static int FT_CubicTo(FT_Vector *cp1, FT_Vector *cp2,FT_Vector *to,void *user) {
return( 0 );
}

static FT_Outline_Funcs outlinefuncs = {
    FT_MoveTo,
    FT_LineTo,
    FT_ConicTo,
    FT_CubicTo,
    0,0			/* No shift, no delta */
};

static void TestFace(FT_Face face) {
    int gid;
    int load_flags = FT_LOAD_DEFAULT;

    if ( check_outlines && (face->face_flags&FT_FACE_FLAG_SCALABLE))
	load_flags = FT_LOAD_NO_BITMAP;
    if ( nohints )
	load_flags |= FT_LOAD_NO_HINTING;

    FT_Set_Char_Size(face,0,(int) (12*64), 72, 72);

    for ( gid=0; gid<face->num_glyphs; ++gid ) {
	if ( check_outlines && (face->face_flags&FT_FACE_FLAG_SCALABLE)) {
	    if ( !FT_Load_Glyph(face,gid,load_flags))
		FT_Outline_Decompose(&face->glyph->outline,&outlinefuncs,NULL);
	} else {
	    FT_Load_Glyph(face,gid,load_flags);
	}
	if ( rasterize )
	    FT_Render_Glyph(face->glyph,ft_render_mode_normal);
    }
    FT_Done_Face(face);
}

static void ExecuteTest(char *testfont) {
    FT_Library context;
    FT_Face face;
    int i,num;

    if ( FT_Init_FreeType( &context )) {
	fprintf( stderr, "Could not initialize freetype.\n" );
	exit(1);
    }
    if ( FT_New_Face(context,testfont,0,&face)) {
	/* The font is erroneous, so if this fails that's ok */
exit( 0 );
    }
    if ( face->num_faces==1 )
	TestFace(face);
    else {
	num = face->num_faces;
	FT_Done_Face(face);
	for ( i=0; i<num; ++i ) {
	    if ( !FT_New_Face(context,testfont,i,&face)) {
		TestFace(face);
	    }
	}
    }

    exit(0);
}
#else
static void ExecuteTest(char *testfont) {
    /* Test FontForge */
    int error_fd;

    freopen("/dev/null","w",stdout);
    error_fd = dup(2);
    freopen("/dev/null","w",stderr);

    execlp(command,command,"-c","Open($1)",testfont,NULL);
    write(error_fd,"execlp failed\n", 14);
    abort();
}
#endif

static int child_pid;

static void abort_test(int sig) {
    /* If a time-out happens, then kill the child */
    kill(child_pid,SIGFPE);
    write(2,"Timeout... ",11);
}

static void do_test(void) {
    int i = getRandom(0,fcnt-1);
    static int test_num=0;
    char buffer[1024];

    sprintf( buffer, "%s/test%d", results_dir, test_num++ );
    if ( copyfont(&fontlist[i],buffer) ) {
	signal(SIGALRM,abort_test);
	alarm(60*60);		/* Anything that takes more than an hour to parse is an error */
	if (( child_pid = fork()) == 0 ) {
	    ExecuteTest(buffer);
	} else if ( child_pid!=-1 ) {
	    int status;
	    waitpid(child_pid,&status,0);
	    alarm(0);
	    if ( WIFSIGNALED(status))
		printf( "Error found in file: %s\n", buffer );
	    else
		unlink(buffer);
	} else {
	    fprintf( stderr, "Could not fork test case.\n" );
exit(1);
	}
	alarm(0);
    }
}

int main( int argc, char **argv) {
    char **dirs, **exts;
    char *pt, *end;
    int dcnt = 0, ecnt=0, rset=false, allexts = false;
    int i;
    time_t now;
    char *testfile = NULL;

    dirs = calloc(argc+1,sizeof(char **));
    exts = calloc(argc+1,sizeof(char **));
    for ( i=1; i<argc; ++i ) {
	pt = argv[i];
	if ( pt[0]=='-' && pt[1]=='-' )
	    ++pt;
	if ( strcmp(pt,"-dir")==0 ) {
	    dirs[dcnt++] = argv[++i];
	} else if ( strcmp(pt,"-ext")==0 ) {
	    exts[ecnt++] = argv[++i];
	} else if ( strcmp(pt,"-all")==0 ) {
	    allexts = true;
	} else if ( strcmp(pt,"-results")==0 ) {
	    results_dir = argv[++i];
	} else if ( strcmp(pt,"-test")==0 ) {
	    testfile = argv[++i];
	} else if ( strcmp(pt,"-error-count")==0 ) {
	    if ( !rset ) error_fraction = 0;
	    rset = true;
	    error_count = strtol(argv[++i],&end,10);
	    if ( *end!='\0' ) {
		fprintf( stderr, "Bad value for error-count: %s\n", argv[i] );
exit( 1 );
	    }
	} else if ( strcmp(pt,"-error-fraction")==0 ) {
	    if ( !rset ) error_count = 0;
	    rset = true;
	    error_fraction = strtod(argv[++i],&end);
	    if ( *end!='\0' ) {
		fprintf( stderr, "Bad value for error-fraction: %s\n", argv[i] );
exit( 1 );
	    }
#ifdef TEST_FREETYPE
	} else if ( strcmp(pt,"-check-outlines")==0 ) {
	    check_outlines = true;
	} else if ( strcmp(pt,"-nohints")==0 ) {
	    nohints = true;
	} else if ( strcmp(pt,"-rasterize")==0 ) {
	    rasterize = true;
#else
	} else if ( strcmp(pt,"-command")==0 ) {
	    command = argv[++i];
#endif
	} else {
	    fprintf( stderr, "%s [options]\tGenerate random erroneous fonts and attempt to parse them\n", argv[0] );
	    fprintf( stderr, "\t--dir <path-spec>\tAdds <path-spec> to list of dirs searched for fonts\n" );
	    fprintf( stderr, "\t--ext <extension>\tAdds <extension> to list of extension indicating fonts\n" );
	    fprintf( stderr, "\t--all \t\tAll non-directory files are assumed to be fonts\n" );
	    fprintf( stderr, "\t--results <dir>\tDirectory in which to place the test fonts.\n" );
	    fprintf( stderr, "\t--test <file>\tRun a single test on an already existing file.\n" );
	    fprintf( stderr, "\t--error-count <cnt>\tIntroduce <cnt> single byte errors into each font.\n" );
	    fprintf( stderr, "\t--error-fraction <frac>\tIntroduce <frac>*filesize single byte errors into each font.\n" );
#ifdef TEST_FREETYPE
	    fprintf( stderr, "\t--rasterize \t\tAttempt to rasterize each glyph.\n" );
	    fprintf( stderr, "\t--nohints \t\tTurn off hinting.\n" );
	    fprintf( stderr, "\t--check-outlines \t\tMake sure we can parse the outlines of each glyph.\n" );
#else
	    fprintf( stderr, "\t--command <cmd> \tProgram to execute.\n" );
#endif
	    fprintf( stderr, "\t--usage \t\tPrint this.\n" );
exit(1);
	}
    }
    if ( allexts )
	exts = NULL;
    else if ( ecnt==0 )
	exts = default_ext_list;
    if ( dcnt==0 )
	dirs = default_dir_list;

    if ( testfile!=NULL )
	ExecuteTest(testfile);		/* This should never return */

    time(&now);
    g_random_set_seed(now);

    FindFonts(dirs,exts);
    mkdir(results_dir,0755);
    for (;;)
	do_test();

return( 0 );
}
