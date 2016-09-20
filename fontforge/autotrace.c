/* Copyright (C) 2000-2012 by George Williams */
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

#include "autotrace.h"

#include "fontforgevw.h"
#include "cvundoes.h"
#include <math.h>
#include <ustring.h>
#include <utype.h>
#include "sd.h"
#include "gfile.h"

#include <sys/types.h>		/* for waitpid */
#if !defined(__MINGW32__)
#include <sys/wait.h>		/* for waitpid */
#endif
#include <unistd.h>		/* for access, unlink, fork, execvp, getcwd */
#include <sys/stat.h>		/* for open */
#include <fcntl.h>		/* for open */
#include <stdlib.h>		/* for getenv */
#include <errno.h>		/* for errors */
#include <dirent.h>		/* for opendir,etc. */

#include "ffglib.h"

int preferpotrace = false;

/* Interface to Martin Weber's autotrace program   */
/*  http://homepages.go.com/~martweb/AutoTrace.htm */
/*  Oops, now at http://sourceforge.net/projects/autotrace/ */

/* Also interface to Peter Selinger's potrace program (which does the same thing */
/*  and has a cleaner interface) */
/* http://potrace.sf.net/ */


static SplinePointList *localSplinesFromEntities(Entity *ent, Color bgcol, int ispotrace) {
    Entity *enext;
    SplinePointList *head=NULL, *last, *test, *next, *prev, *new, *nlast, *temp;
    int clockwise;
    SplineChar sc;
    StrokeInfo si;
    DBounds bb, sbb;
    int removed;
    real fudge;
    Layer layers[2];

    /* We have a problem. The autotrace program includes contours for the */
    /*  background color (there's supposed to be a way to turn that off, but */
    /*  it didn't work when I tried it, so...). I don't want them, so get */
    /*  rid of them. But we must be a bit tricky. If the contour specifies a */
    /*  counter within the letter (the hole in the O for instance) then we */
    /*  do want the contour, but we want it to be counterclockwise. */
    /* So first turn all background contours counterclockwise, and flatten */
    /*  the list */
    /* potrace does not have this problem */
    /* But potrace does not close its paths (well, it closes the last one) */
    /*  and as with standard postscript fonts I need to reverse the splines */
    unsigned bgr = COLOR_RED(bgcol), bgg = COLOR_GREEN(bgcol), bgb = COLOR_BLUE(bgcol);

    memset(&sc,'\0',sizeof(sc));
    memset(layers,0,sizeof(layers));
    sc.layers = layers;
    for ( ; ent!=NULL; ent = enext ) {
	enext = ent->next;
	if ( ent->type == et_splines ) {
	    if ( /* ent->u.splines.fill.col==0xffffffff && */ ent->u.splines.stroke.col!=0xffffffff ) {
		memset(&si,'\0',sizeof(si));
		si.join = ent->u.splines.join;
		si.cap = ent->u.splines.cap;
		si.radius = ent->u.splines.stroke_width/2;
		new = NULL;
		for ( test = ent->u.splines.splines; test!=NULL; test=test->next ) {
		    temp = SplineSetStroke(test,&si,false);
		    if ( new==NULL )
			new=temp;
		    else
			nlast->next = temp;
		    for ( nlast=temp; nlast->next!=NULL; nlast=nlast->next );
		}
		SplinePointListsFree(ent->u.splines.splines);
		ent->u.splines.fill.col = ent->u.splines.stroke.col;
	    } else {
		new = ent->u.splines.splines;
	    }
	    if ( head==NULL )
		head = new;
	    else
		last->next = new;
	    if ( ispotrace ) {
		for ( test = new; test!=NULL; test=test->next ) {
		    if ( test->first!=test->last &&
			    RealNear(test->first->me.x,test->last->me.x) &&
			    RealNear(test->first->me.y,test->last->me.y)) {
			test->first->prevcp = test->last->prevcp;
			test->first->noprevcp = test->last->noprevcp;
			test->first->prevcpdef = test->last->prevcpdef;
			test->first->prev = test->last->prev;
			test->last->prev->to = test->first;
			SplinePointFree(test->last);
			test->last=test->first;
		    }
		    SplineSetReverse(test);
		    last = test;
		}
	    } else {
		for ( test = new; test!=NULL; test=test->next ) {
		    clockwise = SplinePointListIsClockwise(test)==1;
		    /* colors may get rounded a little as we convert from RGB to */
		    /*  a postscript color and back. */
		    if ( COLOR_RED(ent->u.splines.fill.col)>=bgr-2 && COLOR_RED(ent->u.splines.fill.col)<=bgr+2 &&
			    COLOR_GREEN(ent->u.splines.fill.col)>=bgg-2 && COLOR_GREEN(ent->u.splines.fill.col)<=bgg+2 &&
			    COLOR_BLUE(ent->u.splines.fill.col)>=bgb-2 && COLOR_BLUE(ent->u.splines.fill.col)<=bgb+2 ) {
			if ( clockwise )
			    SplineSetReverse(test);
		    } else {
			if ( !clockwise )
			    SplineSetReverse(test);
			clockwise = SplinePointListIsClockwise(test)==1;
		    }
		    last = test;
		}
	    }
	}
	SplinePointListsFree(ent->clippath);
	free(ent);
    }

    /* Then remove all counter-clockwise (background) contours which are at */
    /*  the edge of the character */
    if ( !ispotrace ) do {
	removed = false;
	sc.layers[ly_fore].splines = head;
	SplineCharFindBounds(&sc,&bb);
	fudge = (bb.maxy-bb.miny)/64;
	if ( (bb.maxx-bb.minx)/64 > fudge )
	    fudge = (bb.maxx-bb.minx)/64;
	for ( last=head, prev=NULL; last!=NULL; last=next ) {
	    next = last->next;
	    if ( SplinePointListIsClockwise(last)==0 ) {
		last->next = NULL;
		SplineSetFindBounds(last,&sbb);
		last->next = next;
		if ( sbb.minx<=bb.minx+fudge || sbb.maxx>=bb.maxx-fudge ||
			sbb.maxy >= bb.maxy-fudge || sbb.miny <= bb.miny+fudge ) {
		    if ( prev==NULL )
			head = next;
		    else
			prev->next = next;
		    last->next = NULL;
		    SplinePointListFree(last);
		    removed = true;
		} else
		    prev = last;
	    } else
		prev = last;
	}
    } while ( removed );
return( head );
}

#if !defined(__MINGW32__)
/* I think this is total paranoia. but it's annoying to have linker complaints... */
static int mytempnam(char *buffer) {
    char *dir;
    int fd;
    /* char *old; */

    if ( (dir=getenv("TMPDIR"))!=NULL )
	strcpy(buffer,dir);
#ifndef P_tmpdir
#define P_tmpdir	"/tmp"
#endif
    else
	strcpy(buffer,P_tmpdir);
    strcat(buffer,"/PfaEdXXXXXX");
    fd = g_mkstemp(buffer);
return( fd );
}

static char *mytempdir(void) {
    char buffer[1025];
    char *dir, *eon;
    static int cnt=0;
    int tries=0;

    if ( (dir=getenv("TMPDIR"))!=NULL )
	strncpy(buffer,dir,sizeof(buffer)-1-5);
#ifndef P_tmpdir
#define P_tmpdir	"/tmp"
#endif
    else
	strcpy(buffer,P_tmpdir);
    strcat(buffer,"/PfaEd");
    eon = buffer+strlen(buffer);
    while ( 1 ) {
	sprintf( eon, "%04X_mf%d", getpid(), ++cnt );
	if ( mkdir(buffer,0770)==0 )
return( copy(buffer) );
	else if ( errno!=EEXIST )
return( NULL );
	if ( ++tries>100 )
return( NULL );
    }
}
#endif


#if defined(__MINGW32__)
static char* add_arg(char* buffer, const char* s)
{
    while( *s ) *buffer++ = *s++;
    *buffer = '\0';
    return buffer;
}
void _SCAutoTrace(SplineChar *sc, int layer, char **args) {
    ImageList *images;
    SplineSet *new, *last;
    struct _GImage *ib;
    Color bgcol;
    int   ispotrace;
    real  transform[6];
    char  tempname_in[1025];
    char  tempname_out[1025];
    const char *prog;
    char  *command, *cmd;
    FILE  *ps;
    int i, changed = false;

    if ( sc->layers[ly_back].images==NULL )
	return;
    prog = FindAutoTraceName();
    if ( prog==NULL )
	return;
    ispotrace = (strstrmatch(prog,"potrace")!=NULL );
    for ( images = sc->layers[ly_back].images; images!=NULL; images=images->next ) {
	ib = images->image->list_len==0 ? images->image->u.image : images->image->u.images[0];
	if ( ib->width==0 || ib->height==0 ) {
	    continue;
	}

	strcpy(tempname_in,  _tempnam(NULL, "FontForge_in_"));
	strcpy(tempname_out, _tempnam(NULL, "FontForge_out_"));
	GImageWriteBmp(images->image, tempname_in);

	if ( ib->trans==-1 )
	    bgcol = 0xffffff;		/* reasonable guess */
	else if ( ib->image_type==it_true )
	    bgcol = ib->trans;
	else if ( ib->clut!=NULL )
	    bgcol = ib->clut->clut[ib->trans];
	else
	    bgcol = 0xffffff;

	command = malloc(32768);
	cmd = add_arg(command, prog);
	cmd = add_arg(cmd, " ");
	if(args){
	    for(i=0; args[i]; i++){
		    cmd = add_arg(cmd, args[i]);
		cmd = add_arg(cmd, " ");
	    }
	}
	if ( ispotrace )
		cmd = add_arg(cmd, "-c --eps -r 72 --output=\"");
	else
		cmd = add_arg(cmd, "--output-format=eps --input-format=BMP --output-file \"");

	cmd = add_arg(cmd, tempname_out);
	cmd = add_arg(cmd, "\" \"");
	cmd = add_arg(cmd, tempname_in);
	cmd = add_arg(cmd, "\"");
	/*fprintf(stdout, "---EXEC---\n%s\n----------\n", command);fflush(stdout);*/
	system(command);
	free(command);

	ps = fopen(tempname_out, "r");
	if(ps){
	    new = localSplinesFromEntities(EntityInterpretPS(ps,NULL),bgcol,ispotrace);
	    transform[0] = images->xscale; transform[3] = images->yscale;
	    transform[1] = transform[2] = 0;
	    transform[4] = images->xoff;
	    transform[5] = images->yoff - images->yscale*ib->height;
	    new = SplinePointListTransform(new,transform,tpt_AllPoints);
	    if ( sc->layers[layer].order2 ) {
		SplineSet *o2 = SplineSetsTTFApprox(new);
		SplinePointListsFree(new);
		new = o2;
	    }
	    if ( new!=NULL ) {
		sc->parent->onlybitmaps = false;
		if ( !changed )
		    SCPreserveLayer(sc,layer,false);
		for ( last=new; last->next!=NULL; last=last->next );
		last->next = sc->layers[layer].splines;
		sc->layers[layer].splines = new;
		changed = true;
	    }
	    fclose(ps);
	}

	unlink(tempname_in);
	unlink(tempname_out);
    }
    if ( changed )
	SCCharChangedUpdate(sc,layer);

}
#else
void _SCAutoTrace(SplineChar *sc, int layer, char **args) {
    ImageList *images;
    const char *prog;
    char *pt;
    SplineSet *new, *last;
    struct _GImage *ib;
    Color bgcol;
    real transform[6];
    int changed = false;
    char tempname[1025];
    const char (* arglist[30]);
    size_t ac,i;
    FILE *ps;
    int pid, status, fd;
    int ispotrace;

    if ( sc->layers[ly_back].images==NULL )
return;
    prog = FindAutoTraceName();
    if ( prog==NULL )
return;
    ispotrace = (strstrmatch(prog,"potrace")!=NULL );
    for ( images = sc->layers[ly_back].images; images!=NULL; images=images->next ) {
/* the linker tells me not to use tempnam(). Which does almost exactly what */
/*  I want. So we go through a much more complex set of machinations to make */
/*  it happy. */
	ib = images->image->list_len==0 ? images->image->u.image : images->image->u.images[0];
	if ( ib->width==0 || ib->height==0 ) {
	    /* pk fonts can have 0 sized bitmaps for space characters */
	    /*  but autotrace gets all snooty about being given an empty image */
	    /*  so if we find one, then just ignore it. It won't produce any */
	    /*  results anyway */
    continue;
	}
	fd = mytempnam(tempname);
	GImageWriteBmp(images->image,tempname);
	if ( ib->trans==(Color)-1 )
	    bgcol = 0xffffff;		/* reasonable guess */
	else if ( ib->image_type==it_true )
	    bgcol = ib->trans;
	else if ( ib->clut!=NULL )
	    bgcol = ib->clut->clut[ib->trans];
	else
	    bgcol = 0xffffff;

	ac = 0;
	arglist[ac++] = prog;
	if ( ispotrace ) {
	    /* If I use the long names (--cleartext) potrace hangs) */
	    /*  version 1.1 */
	    arglist[ac++] = "-c";
	    arglist[ac++] = "--output=-";		/* output to stdout */
	    arglist[ac++] = "--eps";
	    arglist[ac++] = "-r";
	    arglist[ac++] = "72";
	} else {
	    arglist[ac++] = "--output-format=eps";
	    arglist[ac++] = "--input-format=BMP";
	}
	if ( args ) {
	    for ( i=0; args[i]!=NULL && ac<sizeof(arglist)/sizeof(arglist[0])-2; ++i )
		arglist[ac++] = args[i];
	}
/* On windows potrace is now compiled with MinGW (whatever that is) which */
/*  means it can't handle cygwin's idea of "/tmp". So cd to /tmp in the child */
/*  and use the local filename rather than full pathspec. */
	pt = strrchr(tempname,'/')==NULL?tempname:strrchr(tempname,'/')+1;
	arglist[ac++] = pt;
	arglist[ac] = NULL;
	/* We can't use AutoTrace's own "background-color" ignorer because */
	/*  it ignores counters as well as surrounds. So "O" would be a dark */
	/*  oval, etc. */
	ps = tmpfile();
	if ( (pid=fork())==0 ) {
	    /* Child */
	    close(1);
	    dup2(fileno(ps),1);
	    if ( strrchr(tempname,'/')!=NULL ) {	/* See comment above */
		*strrchr(tempname,'/') = '\0';
		chdir(tempname);
	    }
	    exit(execvp(prog,(char * const *)arglist)==-1);	/* If exec fails, then die */
	} else if ( pid!=-1 ) {
	    waitpid(pid,&status,0);
	    if ( WIFEXITED(status)) {
		rewind(ps);
		new = localSplinesFromEntities(EntityInterpretPS(ps,NULL),bgcol,ispotrace);
		transform[0] = images->xscale; transform[3] = images->yscale;
		transform[1] = transform[2] = 0;
		transform[4] = images->xoff;
		transform[5] = images->yoff - images->yscale*ib->height;
		new = SplinePointListTransform(new,transform,tpt_AllPoints);
		if ( sc->layers[layer].order2 ) {
		    SplineSet *o2 = SplineSetsTTFApprox(new);
		    SplinePointListsFree(new);
		    new = o2;
		}
		if ( new!=NULL ) {
		    sc->parent->onlybitmaps = false;
		    if ( !changed )
			SCPreserveLayer(sc,layer,false);
		    for ( last=new; last->next!=NULL; last=last->next );
		    last->next = sc->layers[layer].splines;
		    sc->layers[layer].splines = new;
		    changed = true;
		}
	    }
	}
	fclose(ps);
	close(fd);
	unlink(tempname);		/* Might not be needed, but probably is*/
    }
    if ( changed )
	SCCharChangedUpdate(sc,layer);
}
#endif

static char **makevector(const char *str) {
    char **vector;
    const char *start, *pt;
    int i,cnt;

    if ( str==NULL )
return( NULL );

    vector = NULL;
    for ( i=0; i<2; ++i ) {
	cnt = 0;
	for ( start=str; isspace(*start); ++start );
	while ( *start ) {
	    for ( pt=start; !isspace(*pt) && *pt!='\0'; ++pt );
	    if ( vector!=NULL )
		vector[cnt] = copyn(start,pt-start);
	    ++cnt;
	    for ( start=pt; isspace(*start); ++start);
	}
	if ( cnt==0 )
return( NULL );
	if ( vector ) {
	    vector[cnt] = NULL;
return( vector );
	}
	vector = malloc((cnt+1)*sizeof(char *));
    }
return( NULL );
}

static char *flatten(char *const *args) {
    char *ret, *rpt;
    int j, i, len;

    if ( args==NULL )
return( NULL );

    ret = rpt = NULL;
    for ( i=0; i<2; ++i ) {
	for ( j=0, len=0; args[j]!=NULL; ++j ) {
	    if ( rpt!=NULL ) {
		strcpy(rpt,args[j]);
		rpt += strlen( args[j] );
		*rpt++ = ' ';
	    } else
		len += strlen(args[j])+1;
	}
	if ( rpt ) {
	    rpt[-1] = '\0';
return( ret );
	} else if ( len<=1 )
return( NULL );
	ret = rpt = malloc(len);
    }
return( NULL );
}

static char **args=NULL;
int autotrace_ask=0, mf_ask=0, mf_clearbackgrounds=0, mf_showerrors=0;
char *mf_args = NULL;

void *GetAutoTraceArgs(void) {
return( flatten(args));
}

void SetAutoTraceArgs(void *a) {
    int i;

    if ( args!=NULL ) {
	for ( i=0; args[i]!=NULL; ++i )
	    free(args[i]);
	free(args);
    }
    args = makevector((char *) a);
}

char **AutoTraceArgs(int ask) {

    if (( ask || autotrace_ask ) && !no_windowing_ui ) {
	char *cdef = flatten(args);
	char *cret;

	cret = ff_ask_string(_("Additional arguments for autotrace program:"),
		cdef,_("Additional arguments for autotrace program:"));
	free(cdef);
	if ( cret==NULL )
return( (char **) -1 );
	args = makevector(cret);
	free(cret);
	SavePrefs(true);
    }
return( args );
}

void FVAutoTrace(FontViewBase *fv,int ask) {
    char **args;
    int i,cnt,gid;

    if ( FindAutoTraceName()==NULL ) {
	ff_post_error(_("Can't find autotrace"),_("Can't find autotrace program (set AUTOTRACE environment variable) or download from:\n  http://sf.net/projects/autotrace/"));
return;
    }

    args = AutoTraceArgs(ask);
    if ( args==(char **) -1 )
return;
    for ( i=cnt=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		fv->sf->glyphs[gid]!=NULL &&
		fv->sf->glyphs[gid]->layers[ly_back].images )
	    ++cnt;

    ff_progress_start_indicator(10,_("Autotracing..."),_("Autotracing..."),0,cnt,1);

    SFUntickAll(fv->sf);
    for ( i=cnt=0; i<fv->map->enccount; ++i ) {
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		fv->sf->glyphs[gid]!=NULL &&
		fv->sf->glyphs[gid]->layers[ly_back].images &&
		!fv->sf->glyphs[gid]->ticked ) {
	    _SCAutoTrace(fv->sf->glyphs[gid], fv->active_layer, args);
	    if ( !ff_progress_next())
    break;
	}
    }
    ff_progress_end_indicator();
}

void SCAutoTrace(SplineChar *sc,int layer, int ask) {
    char **args;

    if ( sc->layers[ly_back].images==NULL ) {
	ff_post_error(_("Nothing to trace"),_("Nothing to trace"));
return;
    } else if ( FindAutoTraceName()==NULL ) {
	ff_post_error(_("Can't find autotrace"),_("Can't find autotrace program (set AUTOTRACE environment variable) or download from:\n  http://sf.net/projects/autotrace/"));
return;
    }

    args = AutoTraceArgs(ask);
    if ( args==(char **) -1 )
return;
    _SCAutoTrace(sc, layer, args);
}

char *ProgramExists(const char *prog,char *buffer) {
    char *path, *pt;

    if (( path = getenv("PATH"))==NULL )
return( NULL );

    while ( 1 ) {
	pt = strchr(path,':');
	if ( pt==NULL ) pt = path+strlen(path);
	if ( pt-path<1000 ) {
	    strncpy(buffer,path,pt-path);
	    buffer[pt-path] = '\0';
	    if ( pt!=path && buffer[pt-path-1]!='/' )
		strcat(buffer,"/");
	    strcat(buffer,prog);
	    /* Under cygwin, applying access to "potrace" will find "potrace.exe" */
	    /*  no need for special check to add ".exe" */
	    if ( access(buffer,X_OK)!=-1 ) {
return( buffer );
	    }
	}
	if ( *pt=='\0' )
    break;
	path = pt+1;
    }
return( NULL );
}

const char *FindAutoTraceName(void) {
    static int searched=0;
    static int waspotraceprefered;
    static const char *name = NULL;
    char buffer[1025];

    if ( searched && waspotraceprefered==preferpotrace )
return( name );

    searched = true;
    waspotraceprefered = preferpotrace;
    if ( preferpotrace ) {
	if (( name = getenv("POTRACE"))!=NULL )
return( name );
    }
    if (( name = getenv("AUTOTRACE"))!=NULL )
return( name );
    if (( name = getenv("POTRACE"))!=NULL )
return( name );

    if ( preferpotrace ) {
	if ( ProgramExists("potrace",buffer)!=NULL )
	    name = "potrace";
    }
    if ( name==NULL && ProgramExists("autotrace",buffer)!=NULL )
	name = "autotrace";
    if ( name==NULL && ProgramExists("potrace",buffer)!=NULL )
	name = "potrace";
return( name );
}

const char *FindMFName(void) {
    static int searched=0;
    static const char *name = NULL;
    char buffer[1025];

    if ( searched )
return( name );

    searched = true;
    if (( name = getenv("MF"))!=NULL )
return( name );
    if ( ProgramExists("mf",buffer)!=NULL )
	name = "mf";
return( name );
}

static char *FindGfFile(char *tempdir) {
    DIR *temp;
    struct dirent *ent;
    char buffer[1025], *ret=NULL;

    temp = opendir(tempdir);
    if ( temp!=NULL ) {
	while ( (ent=readdir(temp))!=NULL ) {
	    if ( strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0 )
	continue;
	    if ( strlen(ent->d_name)>2 && strcmp(ent->d_name+strlen(ent->d_name)-2,"gf")==0 ) {
		strcpy(buffer,tempdir);
		strcat(buffer,"/");
		strcat(buffer,ent->d_name);
		ret = copy(buffer);
	break;
	    }
	}
	closedir(temp);
    }
return( ret );
}

static void cleantempdir(char *tempdir) {
    DIR *temp;
    struct dirent *ent;
    char buffer[1025], *eod;
    char *todelete[100];
    int cnt=0;

    temp = opendir(tempdir);
    if ( temp!=NULL ) {
	strcpy(buffer,tempdir);
	strcat(buffer,"/");
	eod = buffer+strlen(buffer);
	while ( (ent=readdir(temp))!=NULL ) {
	    if ( strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0 )
	continue;
	    strcpy(eod,ent->d_name);
	    /* Hmm... doing an unlink right here means changing the dir file */
	    /*  which might mean we could not read it properly. So save up the*/
	    /*  things we need to delete and trash them later */
	    if ( cnt<99 )
		todelete[cnt++] = copy(buffer);
	}
	closedir(temp);
	todelete[cnt] = NULL;
	for ( cnt=0; todelete[cnt]!=NULL; ++cnt ) {
	    unlink(todelete[cnt]);
	    free(todelete[cnt]);
	}
    }
    rmdir(tempdir);
}

void MfArgsInit(void) {
    if ( mf_args==NULL )
	mf_args = copy("\\scrollmode; mode=proof ; mag=2; input");
}

static char *MfArgs(void) {
    MfArgsInit();

    if ( mf_ask && !no_windowing_ui ) {
	char *ret;

	ret = ff_ask_string(_("Additional arguments for autotrace program:"),
		mf_args,_("Additional arguments for autotrace program:"));
	if ( ret==NULL )
return( (char *) -1 );
	mf_args = ret;
	SavePrefs(true);
    }
return( mf_args );
}

SplineFont *SFFromMF(char *filename) {
#if defined(__MINGW32__)
return (NULL);
#else
    char *tempdir;
    char *arglist[8];
    int pid, status, ac, i;
    SplineFont *sf=NULL;
    SplineChar *sc;

    if ( FindMFName()==NULL ) {
	ff_post_error(_("Can't find mf"),_("Can't find mf program -- metafont (set MF environment variable) or download from:\n  http://www.tug.org/\n  http://www.ctan.org/\nIt's part of the TeX distribution"));
return( NULL );
    } else if ( FindAutoTraceName()==NULL ) {
	ff_post_error(_("Can't find autotrace"),_("Can't find autotrace program (set AUTOTRACE environment variable) or download from:\n  http://sf.net/projects/autotrace/"));
return( NULL );
    }
    if ( MfArgs()==(char *) -1 || AutoTraceArgs(false)==(char **) -1 )
return( NULL );

    /* I don't know how to tell mf to put its files where I want them. */
    /*  so instead I create a temporary directory, cd mf there, and it */
    /*  will put the files there. */
    tempdir = mytempdir();
    if ( tempdir==NULL ) {
	ff_post_error(_("Can't create temporary directory"),_("Can't create temporary directory"));
return( NULL );
    }

    ac = 0;
    arglist[ac++] = (char *)FindMFName();
    arglist[ac++] = malloc(strlen(mf_args)+strlen(filename)+20);
    arglist[ac] = NULL;
    strcpy(arglist[1],mf_args);
    strcat(arglist[1]," ");
    strcat(arglist[1],filename);
    if ( (pid=fork())==0 ) {
	/* Child */
	int fd;
	chdir(tempdir);
	if ( !mf_showerrors ) {
	    close(1);		/* mf generates a lot of verbiage to stdout. Throw it away */
	    fd = open("/dev/null",O_WRONLY);
	    if ( fd!=1 )
		dup2(fd,1);
	    close(0);		/* mf sometimes asks the user questions, but I have no answers... */
	    fd = open("/dev/null",O_RDONLY);
	    if ( fd!=0 )
		dup2(fd,0);
	}
	exit(execvp(arglist[0],arglist)==-1);	/* If exec fails, then die */
    } else if ( pid!=-1 ) {
	ff_progress_show();
	waitpid(pid,&status,0);
	if ( WIFEXITED(status)) {
	    char *gffile = FindGfFile(tempdir);
	    if ( gffile==NULL )
		ff_post_error(_("Can't run mf"),_("Could not read (or perhaps find) mf output file"));
	    else {
		sf = SFFromBDF(gffile,3,true);
		free(gffile);
		if ( sf!=NULL ) {
		    ff_progress_change_line1(_("Autotracing..."));
		    ff_progress_change_total(sf->glyphcnt);
		    for ( i=0; i<sf->glyphcnt; ++i ) {
			if ( (sc = sf->glyphs[i])!=NULL && sc->layers[ly_back].images ) {
			    _SCAutoTrace(sc, ly_fore, args);
			    if ( mf_clearbackgrounds ) {
				GImageDestroy(sc->layers[ly_back].images->image);
			        free(sc->layers[ly_back].images);
			        sc->layers[ly_back].images = NULL;
			    }
			}
			if ( !ff_progress_next())
		    break;
		    }
		} else
		    ff_post_error(_("Can't run mf"),_("Could not read (or perhaps find) mf output file"));
	    }
	} else
	    ff_post_error(_("Can't run mf"),_("MetaFont exited with an error"));
    } else
	ff_post_error(_("Can't run mf"),_("Can't run mf"));
    free(arglist[1]);
    cleantempdir(tempdir);
return( sf );
#endif
}
