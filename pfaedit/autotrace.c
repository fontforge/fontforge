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
#include "utype.h"
#include "sd.h"
#include "views.h"

#include <unistd.h>		/* for access, unlink */
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
	    if ( ent->u.splines.fill.col==0xffffffff && ent->u.splines.stroke.col!=0xffffffff ) {
		memset(&si,'\0',sizeof(si));
		si.join = ent->u.splines.join;
		si.cap = ent->u.splines.cap;
		si.radius = ent->u.splines.stroke_width/2;
		new = NULL;
		for ( last = ent->u.splines.splines; last!=NULL; last=last->next ) {
		    temp = SplineSetStroke(last,&si,NULL);
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

static void SCAutoTrace(SplineChar *sc, char *args) {
    ImageList *images;
    char *bmp, *eps, *base, *prog, *cmd;
    int len;
    SplineSet *new, *last;
    struct _GImage *ib;
    Color bgcol;
    double transform[6];
    int changed = false;

    if ( sc->backimages==NULL )
return;
    prog = FindAutoTraceName();
    if ( prog==NULL )
return;
    for ( images = sc->backimages; images!=NULL; images=images->next ) {
	base = tempnam(NULL,"PfaEd");
	bmp = galloc(strlen(base)+8);
	eps = galloc(strlen(base)+8);
	strcpy(bmp,base); strcat(bmp,".bmp");
	strcpy(eps,base); strcat(eps,".eps");
	free(base);
	GImageWriteBmp(images->image,bmp);
	ib = images->image->list_len==0 ? images->image->u.image : images->image->u.images[0];
	if ( ib->trans==-1 )
	    bgcol = 0xffffff;		/* reasonable guess */
	else if ( ib->image_type==it_true )
	    bgcol = ib->trans;
	else if ( ib->clut!=NULL )
	    bgcol = ib->clut->clut[ib->trans];
	else
	    bgcol = 0xffffff;

	len = strlen(prog);
	if ( args!=NULL ) len = 1+strlen(args);
	len += strlen(" --output-format=eps");
	len += strlen(" --output-file ");
	len += strlen(eps);
	len += 1+strlen(bmp);
	cmd = galloc(len+10);
	strcpy(cmd,prog);
	if ( args!=NULL ) {
	    strcat(cmd," ");
	    strcat(cmd,args);
	}
	strcat(cmd," "); strcat(cmd,bmp);
	strcat(cmd, " --output-format=eps --output-file=");
	strcat(cmd,eps);
	if ( system(cmd)==0 ) {
	    FILE *ps = fopen(eps,"r");
	    new = SplinesFromEntities(EntityInterpretPS(ps),bgcol);
	    transform[0] = images->xscale; transform[3] = images->yscale;
	    transform[1] = transform[2] = 0;
	    transform[4] = images->xoff;
	    transform[5] = images->yoff - images->yscale*ib->height;
	    new = SplinePointListTransform(new,transform,true);
	    if ( new!=NULL ) {
		if ( !changed )
		    SCPreserveState(sc);
		for ( last=new; last->next!=NULL; last=last->next );
		last->next = sc->splines;
		sc->splines = new;
		changed = true;
	    }
	    fclose(ps);
	}
	unlink(bmp); unlink(eps);
	free(bmp); free(eps); free(cmd);
    }
    if ( changed )
	SCCharChangedUpdate(sc,sc->parent->fv);
}

static char *AutoTraceArgs(void) {
    /* We could pop up a dlg here */
return( NULL );
}

void CVAutoTrace(CharView *cv) {
    char *args;

    if ( cv->sc->backimages==NULL ) {
	GDrawError("Nothing to trace");
return;
    } else if ( FindAutoTraceName()==NULL ) {
	GDrawError("Can't find autotrace program (set AUTOTRACE environment variable)");
return;
    }

    args = AutoTraceArgs();
    SCAutoTrace(cv->sc, args);
}

void FVAutoTrace(FontView *fv) {
    char *args;
    int i;

    if ( FindAutoTraceName()==NULL ) {
	GDrawError("Can't find autotrace program (set AUTOTRACE environment variable)");
return;
    }

    args = AutoTraceArgs();
    for ( i=0; i<fv->sf->charcnt; ++i )
	if ( fv->sf->chars[i] && fv->selected[i] )
	    SCAutoTrace(fv->sf->chars[i], args);
}

char *FindAutoTraceName(void) {
    static int searched=0;
    static char *name;
    char *path, *pt;
    char buffer[1025];

    if ( searched )
return( name );

    searched = true;
    if (( name = getenv("AUTOTRACE"))!=NULL )
return( name );
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
	    strcat(buffer,"autotrace");
	    if ( access(buffer,X_OK)!=-1 ) {
		name = "autotrace";
return( name );
	    }
	}
	if ( *pt=='\0' )
    break;
	path = pt+1;
    }
return( NULL );
}
