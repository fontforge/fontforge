/* Copyright (C) 2001 by George Williams */
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
#include "pfaedit.h"
#include <math.h>
#include <ustring.h>
#include <utype.h>
#include <gwidget.h>
#include "sd.h"
#include "views.h"

#include <sys/types.h>		/* for waitpid */
#include <sys/wait.h>		/* for waitpid */
#include <unistd.h>		/* for access, unlink, fork, execvp */
#include <stdlib.h>		/* for getenv */

/* Interface to Martin Weber's autotrace program   */
/*  http://homepages.go.com/~martweb/AutoTrace.htm */


static SplinePointList *SplinesFromEntities(Entity *ent, Color bgcol) {
    Entity *enext;
    SplinePointList *head=NULL, *last, *test, *next, *prev, *new, *nlast, *temp;
    int clockwise;
    SplineChar sc;
    StrokeInfo si;
    DBounds bb, sbb;
    int removed;
    /* We have a problem. The autotrace program includes contours for the */
    /*  background color (there's supposed to be a way to turn that off, but */
    /*  it didn't work when I tried it, so...). I don't want them, so get */
    /*  rid of them. But we must be a bit tricky. If the contour specifies a */
    /*  counter within the letter (the hole in the O for instance) then we */
    /*  do want to contour, but we want it to be counterclockwise. */
    /* So first turn all background contours counterclockwise, and flatten */
    /*  the list */
    int bgr = COLOR_RED(bgcol), bgg = COLOR_GREEN(bgcol), bgb = COLOR_BLUE(bgcol);

    memset(&sc,'\0',sizeof(sc));
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
		    temp = SplineSetStroke(test,&si,NULL);
		    if ( new==NULL )
			new=temp;
		    else
			nlast->next = temp;
		    for ( nlast=temp; nlast->next!=NULL; nlast=nlast->next );
		}
		SplinePointListFree(ent->u.splines.splines);
		ent->u.splines.fill.col = ent->u.splines.stroke.col;
	    } else {
		new = ent->u.splines.splines;
	    }
	    if ( head==NULL )
		head = new;
	    else
		last->next = new;
	    for ( test = new; test!=NULL; test=test->next ) {
		clockwise = SplinePointListIsClockwise(test);
		/* colors may get rounded a little as we convert from RGB to */
		/*  a postscript color and back. */
		if ( COLOR_RED(ent->u.splines.fill.col)>=bgr-2 && COLOR_RED(ent->u.splines.fill.col)<=bgr+2 &&
			COLOR_GREEN(ent->u.splines.fill.col)>=bgg-2 && COLOR_GREEN(ent->u.splines.fill.col)<=bgg+2 &&
			COLOR_BLUE(ent->u.splines.fill.col)>=bgb-2 && COLOR_BLUE(ent->u.splines.fill.col)<=bgb+2 ) {
		    if ( clockwise )
			SplineSetReverse(test);
		} else {
		    clockwise = SplinePointListIsClockwise(test);
		    if ( !clockwise )
			SplineSetReverse(test);
		    clockwise = SplinePointListIsClockwise(test);
		}
		last = test;
	    }
	}
	free(ent);
    }

    /* Then remove all counter-clockwise (background) contours which are at */
    /*  the edge of the character */
    do {
	removed = false;
	sc.splines = head;
	SplineCharFindBounds(&sc,&bb);
	for ( last=head, prev=NULL; last!=NULL; last=next ) {
	    next = last->next;
	    if ( !SplinePointListIsClockwise(last)) {
		last->next = NULL;
		SplineSetFindBounds(last,&sbb);
		last->next = next;
		if ( sbb.minx<=bb.minx+2 || sbb.maxx>=bb.maxx-2 ||
			sbb.maxy >= bb.maxy-2 || sbb.miny <= bb.miny+2 ) {
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

/* I think this is total paranoia. but it's annoying to have linker complaints... */
static int mytempnam(char *buffer) {
    char *dir;

    if ( (dir=getenv("TMPDIR"))!=NULL )
	strcpy(buffer,dir);
#ifndef P_tmpdir
#define P_tmpdir	"/tmp"
#endif
    else
	strcpy(buffer,P_tmpdir);
    strcat(buffer,"/PfaEdXXXXXX");
return( mkstemp(buffer));
}

static void SCAutoTrace(SplineChar *sc, char **args) {
    ImageList *images;
    char *prog;
    SplineSet *new, *last;
    struct _GImage *ib;
    Color bgcol;
    real transform[6];
    int changed = false;
    char tempname[1025];
    char *arglist[30];
    int ac,i;
    FILE *ps;
    int pid, status, fd;

    if ( sc->backimages==NULL )
return;
    prog = FindAutoTraceName();
    if ( prog==NULL )
return;
    for ( images = sc->backimages; images!=NULL; images=images->next ) {
/* the linker tells me not to use tempnam(). Which does almost exactly what */
/*  I want. So we go through a much more complex set of machinations to make */
/*  it happy. */
	fd = mytempnam(tempname);
	GImageWriteBmp(images->image,tempname);
	ib = images->image->list_len==0 ? images->image->u.image : images->image->u.images[0];
	if ( ib->trans==-1 )
	    bgcol = 0xffffff;		/* reasonable guess */
	else if ( ib->image_type==it_true )
	    bgcol = ib->trans;
	else if ( ib->clut!=NULL )
	    bgcol = ib->clut->clut[ib->trans];
	else
	    bgcol = 0xffffff;

	ac = 0;
	arglist[ac++] = prog;
	arglist[ac++] = tempname;
	arglist[ac++] = "--output-format=eps";
	arglist[ac++] = "--input-format=BMP";
	if ( args ) {
	    for ( i=0; args[i]!=NULL && ac<sizeof(arglist)/sizeof(arglist[0])-2; ++i )
		arglist[ac++] = args[i];
	}
	arglist[ac] = NULL;
	/* We can't use AutoTrace's own "background-color" ignorer because */
	/*  it ignores counters as well as surrounds. So "O" would be a dark */
	/*  oval, etc. */
	ps = tmpfile();
	if ( (pid=fork())==0 ) {
	    /* Child */
	    close(1);
	    dup2(fileno(ps),1);
	    exit(execvp(prog,arglist)==-1);	/* If exec fails, then die */
	} else if ( pid!=-1 ) {
	    waitpid(pid,&status,0);
	    if ( WIFEXITED(status)) {
		rewind(ps);
		new = SplinesFromEntities(EntityInterpretPS(ps),bgcol);
		transform[0] = images->xscale; transform[3] = images->yscale;
		transform[1] = transform[2] = 0;
		transform[4] = images->xoff;
		transform[5] = images->yoff - images->yscale*ib->height;
		new = SplinePointListTransform(new,transform,true);
		if ( new!=NULL ) {
		    sc->parent->onlybitmaps = false;
		    if ( !changed )
			SCPreserveState(sc,false);
		    for ( last=new; last->next!=NULL; last=last->next );
		    last->next = sc->splines;
		    sc->splines = new;
		    changed = true;
		}
	    }
	}
	fclose(ps);
	close(fd);
	unlink(tempname);		/* Might not be needed, but probably is*/
    }
    if ( changed )
	SCCharChangedUpdate(sc);
}

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
	vector = galloc((cnt+1)*sizeof(char *));
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
	ret = rpt = galloc(len);
    }
return( NULL );
}

static char **args=NULL;

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

static char **AutoTraceArgs(int ask) {

    if ( ask ) {
	char *cdef = flatten(args);
	unichar_t *def = uc_copy(cdef);
	unichar_t *ret;
	char *cret;

	ret = GWidgetAskStringR(_STR_AdditionalAutotraceArgs, def,_STR_AdditionalAutotraceArgs);
	free(def); free(cdef);
	if ( ret==NULL )
return( (char **) -1 );
	cret = cu_copy(ret); free(ret);
	args = makevector(cret);
	free(cret);
	SavePrefs();
    }
return( args );
}

void CVAutoTrace(CharView *cv,int ask) {
    char **args;
    GCursor ct;

    if ( cv->sc->backimages==NULL ) {
	GWidgetErrorR(_STR_NothingToTrace,_STR_NothingToTrace);
return;
    } else if ( FindAutoTraceName()==NULL ) {
	GWidgetErrorR(_STR_NoAutotrace,_STR_NoAutotraceProg);
return;
    }

    args = AutoTraceArgs(ask);
    if ( args==(char **) -1 )
return;
    ct = GDrawGetCursor(cv->v);
    GDrawSetCursor(cv->v,ct_watch);
    GDrawSync(NULL);
    SCAutoTrace(cv->sc, args);
    GDrawSetCursor(cv->v,ct);
}

void FVAutoTrace(FontView *fv,int ask) {
    char **args;
    int i;

    if ( FindAutoTraceName()==NULL ) {
	GWidgetErrorR(_STR_NoAutotrace,_STR_NoAutotraceProg);
return;
    }

    args = AutoTraceArgs(ask);
    if ( args==(char **) -1 )
return;
    for ( i=0; i<fv->sf->charcnt; ++i )
	if ( fv->sf->chars[i] && fv->selected[i] )
	    SCAutoTrace(fv->sf->chars[i], args);
}

char *ProgramExists(char *prog,char *buffer) {
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

char *FindAutoTraceName(void) {
    static int searched=0;
    static char *name = NULL;
    char buffer[1025];

    if ( searched )
return( name );

    searched = true;
    if (( name = getenv("AUTOTRACE"))!=NULL )
return( name );
    if ( ProgramExists("autotrace",buffer)!=NULL )
	name = "autotrace";
return( name );
}
