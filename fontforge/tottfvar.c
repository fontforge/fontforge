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
#include "fontforge.h"
#include "mem.h"
#include "ttf.h"
#include "splinesaveafm.h"
#include <math.h>
#include <ustring.h>

static int PtNumbersAreSet(SplineChar *sc) {
    struct splinecharlist *dep;

    if ( sc==NULL )
return( false );

    if ( sc->ttf_instrs!=NULL )
return( true );
    for ( dep= sc->dependents; dep!=NULL ; dep=dep->next )
	if ( dep->sc->ttf_instrs!=NULL )
return( true );

return( false );
}

static int AssignPtNumbers(MMSet *mm,int gid) {
    /* None of the instances has fixed point numbers. Make them match */
    int cnt=0;
    SplineSet **ss;
    SplinePoint **sp;
    int i;
    int allavg, alllines, stillmore, ret=true;

    ss = malloc((mm->instance_count+1)*sizeof(SplineSet *));
    sp = malloc((mm->instance_count+1)*sizeof(SplinePoint *));
    for ( i=0; i<mm->instance_count; ++i )
	ss[i] = mm->instances[i]->glyphs[gid]->layers[ly_fore].splines;
    ss[i] = mm->normal->glyphs[gid]->layers[ly_fore].splines;

    if ( ss[0]==NULL ) {
	stillmore = false;
	for ( i=0; i<=mm->instance_count; ++i )
	    if ( ss[i]!=NULL ) stillmore = true;
        free(ss);
        free(sp);
	if ( stillmore )
return( false );
return( true );
    } else {
	stillmore = true;
	for ( i=0; i<=mm->instance_count; ++i )
	    if ( ss[i]==NULL ) stillmore = false;
	if ( !stillmore )
{
free(ss);
free(sp);
return( false );
}
    }
	    
    for (;;) {
	for ( i=0; i<=mm->instance_count; ++i )
	    sp[i] = ss[i]->first;
	for (;;) {
	    allavg = alllines = true;
	    for ( i=0; i<=mm->instance_count; ++i ) {
		if ( !RealNear(sp[i]->me.x,(sp[i]->nextcp.x+sp[i]->prevcp.x)/2) ||
			!RealNear(sp[i]->me.y,(sp[i]->nextcp.y+sp[i]->prevcp.y)/2) )
		    allavg = false;
		if ( !sp[i]->nonextcp )
		    alllines = false;
	    }
	    if ( sp[0] == ss[0]->first )
		allavg = false;
	    for ( i=0; i<=mm->instance_count; ++i ) {
		if ( allavg )
		    sp[i]->ttfindex = 0xffff;
		else
		    sp[i]->ttfindex = cnt;
	    }
	    if ( !allavg )
		++cnt;
	    for ( i=0; i<=mm->instance_count; ++i ) {
		if ( alllines )
		    sp[i]->nextcpindex = 0xffff;
		else
		    sp[i]->nextcpindex = cnt;
	    }
	    if ( !alllines )
		++cnt;

	    if ( sp[0]->next==NULL ) {
		stillmore = false;
		for ( i=1; i<=mm->instance_count; ++i )
		    if ( sp[i]->next!=NULL )
			stillmore = true;
		if ( stillmore )
		    ret = false;
	break;
	    }
	    for ( i=1; i<=mm->instance_count; ++i )
		if ( sp[i]->next==NULL )
		    stillmore = false;
	    if ( !stillmore ) {
		ret = false;
	break;
	    }
	    sp[0] = sp[0]->next->to;
	    for ( i=1; i<=mm->instance_count; ++i )
		sp[i] = sp[i]->next->to;
	    if ( sp[0]==ss[0]->first ) {
		stillmore = false;
		for ( i=1; i<=mm->instance_count; ++i )
		    if ( sp[i]!=ss[i]->first )
			stillmore = true;
		if ( stillmore )
		    ret = false;
	break;
	    }
	    for ( i=1; i<=mm->instance_count; ++i ) {
		if ( sp[i]==ss[i]->first )
		    stillmore = false;
	    }
	    if ( !stillmore ) {
		ret = false;
	break;
	    }
	}
	if ( !ret )
    break;
	stillmore = true;
	for ( i=0; i<=mm->instance_count; ++i )
	    ss[i] = ss[i]->next;
	if ( ss[0]==NULL ) {
	    stillmore=false;
	    for ( i=1; i<=mm->instance_count; ++i )
		if ( ss[i]!=NULL )
		    stillmore = true;
	    if ( stillmore )
		ret = true;
    break;
	}
	for ( i=1; i<=mm->instance_count; ++i )
	    if ( ss[i]==NULL )
		stillmore = false;
	if ( !stillmore ) {
	    ret = true;
    break;
	}
    }
    free(ss);
    free(sp);
return( ret );
}

static int MatchPoints(SplineFont *sffixed, SplineFont *sfother, int gid) {
    SplineChar *fixed, *other;
    SplineSet *ss1, *ss2;
    SplinePoint *sp1, *sp2;

    fixed = sffixed->glyphs[gid]; other = sfother->glyphs[gid];

    if ( PtNumbersAreSet(other)) {
	/* Point numbers must match exactly, both are fixed */
	for ( ss1=fixed->layers[ly_fore].splines,
		  ss2=other->layers[ly_fore].splines;
		ss1!=NULL && ss2!=NULL ;
		ss1 = ss1->next, ss2=ss2->next ) {
	    for ( sp1=ss1->first, sp2=ss2->first; ; ) {
		if ( sp1->ttfindex!=sp2->ttfindex ||
			sp1->nextcpindex!=sp2->nextcpindex )
return( false );
		if ( sp1->next==NULL || sp2->next==NULL ) {
		    if ( sp1->next!=NULL || sp2->next!=NULL )
return( false );
	    break;
		}
		sp1 = sp1->next->to; sp2=sp2->next->to;
		if ( sp1==ss1->first || sp2==ss2->first ) {
		    if ( sp1!=ss1->first || sp2!=ss2->first )
return( false );
	    break;
		}
	    }
	}
return( ss1==NULL && ss2==NULL );
    } else {
	for ( ss1=fixed->layers[ly_fore].splines,
		  ss2=other->layers[ly_fore].splines;
		ss1!=NULL && ss2!=NULL ;
		ss1 = ss1->next, ss2=ss2->next ) {
	    for ( sp1=ss1->first, sp2=ss2->first; ; ) {
		if ( sp1->ttfindex!=0xffff )
		    sp2->ttfindex = sp1->ttfindex;
		else if ( !RealNear(sp2->me.x,(sp2->nextcp.x+sp2->prevcp.x)/2) ||
			!RealNear(sp2->me.y,(sp2->nextcp.y+sp2->prevcp.y)/2) )
return( false );
		else
		    sp2->ttfindex = 0xffff;
		if ( sp1->nextcpindex!=0xffff )
		    sp2->nextcpindex = sp1->nextcpindex;
		else if ( !sp2->nonextcp )
return( false );
		else
		    sp2->nextcpindex = 0xffff;
		if ( sp1->next==NULL || sp2->next==NULL ) {
		    if ( sp1->next!=NULL || sp2->next!=NULL )
return( false );
	    break;
		}
		sp1 = sp1->next->to; sp2=sp2->next->to;
		if ( sp1==ss1->first || sp2==ss2->first ) {
		    if ( sp1!=ss1->first || sp2!=ss2->first )
return( false );
	    break;
		}
	    }
	}
return( ss1==NULL && ss2==NULL );
    }
}

int ContourPtNumMatch(MMSet *mm, int gid) {
    SplineFont *sf;
    int i;

    if ( !mm->apple )
return( false );

    if ( gid>=mm->normal->glyphcnt )
return( false );
    if ( !SCWorthOutputting(mm->normal->glyphs[gid] ) ) {
	for ( i=0; i<mm->instance_count; ++i ) {
	    if ( gid>=mm->instances[i]->glyphcnt )
return( false );
	    if ( SCWorthOutputting(mm->instances[i]->glyphs[gid]))
return( false );
	}
return( true );		/* None is not worth outputting, and that's ok, they match */
    } else {
	for ( i=0; i<mm->instance_count; ++i ) {
	    if ( gid>=mm->instances[i]->glyphcnt )
return( false );
	    if ( !SCWorthOutputting(mm->instances[i]->glyphs[gid]))
return( false );
	}
	    /* All are worth outputting */
    }

    if ( mm->normal->glyphs[gid]->layers[ly_fore].refs!=NULL && mm->normal->glyphs[gid]->layers[ly_fore].splines!=NULL )
return( false );
    for ( i=0; i<mm->instance_count; ++i ) {
	if ( mm->instances[i]->glyphs[gid]->layers[ly_fore].refs!=NULL && mm->instances[i]->glyphs[gid]->layers[ly_fore].splines!=NULL )
return( false );
    }
    if ( mm->normal->glyphs[gid]->layers[ly_fore].refs!=NULL ) {
	RefChar *r;
	int cnt, c;
	for ( r=mm->normal->glyphs[gid]->layers[ly_fore].refs, cnt=0; r!=NULL; r=r->next )
	    ++cnt;
	for ( i=0; i<mm->instance_count; ++i ) {
	    for ( r=mm->instances[i]->glyphs[gid]->layers[ly_fore].refs, c=0; r!=NULL; r=r->next )
		++c;
	    if ( c!=cnt )
return( false );
	}
    }

    sf = NULL;
    if ( PtNumbersAreSet(mm->normal->glyphs[gid]) )
	sf = mm->normal;
    else {
	for ( i=0; i<mm->instance_count; ++i ) {
	    if ( PtNumbersAreSet(mm->instances[i]->glyphs[gid])) {
		sf = mm->instances[i];
	break;
	    }
	}
    }
    if ( sf==NULL )
	/* No instance has fixed points. Make sure all fonts are consistent */
return( AssignPtNumbers(mm,gid));

    if ( sf!=mm->normal && !MatchPoints(sf,mm->normal,gid))
return( false );
    for ( i=0; i<mm->instance_count; ++i ) if ( sf!=mm->instances[i] ) {
	if ( !MatchPoints(sf, mm->instances[i],gid) )
return( false );
    }
return( true );
}

static int SCPointCount(SplineChar *sc) {
    int ptcnt=0;
    RefChar *r;

    ptcnt = SSTtfNumberPoints(sc->layers[ly_fore].splines);
    for ( r=sc->layers[ly_fore].refs; r!=NULL ; r=r->next )
	++ptcnt;
return( ptcnt );
}

int16 **SCFindDeltas(MMSet *mm, int gid, int *_ptcnt) {
    /* When figuring out the deltas the first thing we must do is figure */
    /*  out each point's number */
    int i, j, k, l, cnt, ptcnt;
    int16 **deltas;
    SplineSet *ss1, *ss2;
    SplinePoint *sp1, *sp2;
    RefChar *r1, *r2;

    if ( !ContourPtNumMatch(mm,gid))
return( NULL );
    if ( !SCWorthOutputting(mm->normal->glyphs[gid]))
return( NULL );

    *_ptcnt = ptcnt = SCPointCount(mm->normal->glyphs[gid])+4;
    deltas = malloc(2*mm->instance_count*sizeof(int16 *));
    for ( i=0; i<2*mm->instance_count; ++i )
	deltas[i] = calloc(ptcnt,sizeof(int16));
    for ( i=0; i<mm->instance_count; ++i ) {
	for ( ss1=mm->normal->glyphs[gid]->layers[ly_fore].splines,
		  ss2=mm->instances[i]->glyphs[gid]->layers[ly_fore].splines;
		ss1!=NULL && ss2!=NULL ;
		ss1 = ss1->next, ss2=ss2->next ) {
	    for ( sp1=ss1->first, sp2=ss2->first; ; ) {
		if ( sp1->ttfindex!=0xffff ) {
		    deltas[2*i][sp1->ttfindex] = rint(sp2->me.x)-rint(sp1->me.x);
		    deltas[2*i+1][sp1->ttfindex] = rint(sp2->me.y)-rint(sp1->me.y);
		}
		if ( sp1->nextcpindex != 0xffff ) {
		    deltas[2*i][sp1->nextcpindex] = rint(sp2->nextcp.x)-rint(sp1->nextcp.x);
		    deltas[2*i+1][sp1->nextcpindex] = rint(sp2->nextcp.y)-rint(sp1->nextcp.y);
		}
		if ( sp1->next==NULL )
	    break;
		sp1 = sp1->next->to; sp2 = sp2->next->to;
		if ( sp1==ss1->first )
	    break;
	    }
	}
	for ( cnt=0,
		r1=mm->normal->glyphs[gid]->layers[ly_fore].refs,
		r2=mm->instances[i]->glyphs[gid]->layers[ly_fore].refs;
		r1!=NULL && r2!=NULL;
		r1=r1->next, r2=r2->next, ++cnt ) {
	    deltas[2*i][cnt] = r2->transform[4]-r1->transform[4];
	    deltas[2*i+1][cnt] = r2->transform[5]-r1->transform[5];
	}
	/* Phantom points */
	deltas[2*i][ptcnt-4] = 0; deltas[2*i+1][ptcnt-4] = 0;	/* lbearing */
	deltas[2*i][ptcnt-3] = mm->instances[i]->glyphs[gid]->width -mm->normal->glyphs[gid]->width;
		deltas[2*i+1][ptcnt-3] = 0;			/* horizontal advance */
	deltas[2*i][ptcnt-2] = 0; deltas[2*i+1][ptcnt-2] = 0;	/* top bearing */
	deltas[2*i][ptcnt-1] = 0;				/* vertical advance */
		deltas[2*i+1][ptcnt-1] = mm->instances[i]->glyphs[gid]->vwidth -mm->normal->glyphs[gid]->vwidth;	/* horizontal advance */
    }

    /* Ok, each delta now contains the difference between the instance[i] points */
    /*  and the base points. But that isn't good enough. We must subtract */
    /*  [0,1] and [1,0] from [1,1], and then subtract [1,1,0] [1,0,1] [0,1,1] */
    /*  from [1,1,1] and so on (also [-1,0] from [-1,1], etc.) */
    for ( j=1; j<mm->axis_count; ++j ) {
	for ( i=0; i<mm->instance_count; ++i ) {
	    for ( k=cnt=0; k<mm->axis_count; ++k )
		if ( mm->positions[i*mm->axis_count+k]!=0 )
		    ++cnt;
	    if ( cnt==j ) {
		for ( l = 0; l<mm->instance_count; ++l ) if ( l!=i ) {
		    for ( k=0; k<mm->axis_count; ++k )
			if ( mm->positions[i*mm->axis_count+k]!=0 &&
				mm->positions[l*mm->axis_count+k]!=mm->positions[i*mm->axis_count+k])
		    break;
		    if ( k==mm->axis_count ) {
			for ( k=0; k<ptcnt; ++k ) {
			    deltas[2*l][k] -= deltas[2*i][k];
			    deltas[2*l+1][k] -= deltas[2*i+1][k];
			}
		    }
		}
	    }
	}
    }

    /* If all variants of the glyph are the same, no point in having a gvar */
    /*  entry for it */
    for ( i=0 ; i<mm->instance_count; ++i ) {
	for ( j=0; j<ptcnt; ++j )
	    if ( deltas[i][j]!=0 )
	break;
	if ( j!=ptcnt )
    break;
    }
    if ( i==mm->instance_count ) {
	/* All zeros */
	for ( i=0 ; i<mm->instance_count; ++i )
	    free(deltas[i]);
	free(deltas);
return( NULL );
    }

return( deltas );
}

int16 **CvtFindDeltas(MMSet *mm, int *_ptcnt) {
    int i, j, k, l, cnt, ptcnt;
    int16 **deltas;
    struct ttf_table *cvt, *icvt;
    for ( cvt = mm->normal->ttf_tables; cvt!=NULL && cvt->tag!=CHR('c','v','t',' '); cvt=cvt->next );

    if ( cvt==NULL )
return( NULL );

    icvt = NULL;
    for ( i=0; i<mm->instance_count; ++i )
	if ( (icvt=mm->instances[i]->ttf_tables)!=NULL )
    break;
    if ( icvt==NULL )		/* No other cvt tables => no variation */
return( NULL );

    *_ptcnt = ptcnt = cvt->len/2;
    deltas = calloc(mm->instance_count,sizeof(int16 *));
    for ( i=0; i<mm->instance_count; ++i ) if ( (icvt=mm->instances[i]->ttf_tables)!=NULL ) {
	deltas[i] = calloc(ptcnt,sizeof(int16));
	for ( j=0; j<ptcnt; ++j )
	    deltas[i][j] = memushort(icvt->data,icvt->len, sizeof(uint16)*j)-
		    memushort(cvt->data,cvt->len, sizeof(uint16)*j);
    }

    /* Ok, each delta now contains the difference between the instance[i] points */
    /*  and the base points. But that isn't good enough. We must subtract */
    /*  [0,1] and [1,0] from [1,1], and then subtract [1,1,0] [1,0,1] [0,1,1] */
    /*  from [1,1,1] and so on (also [-1,0] from [-1,1], etc.) */
    for ( j=1; j<mm->axis_count; ++j ) {
	for ( i=0; i<mm->instance_count; ++i ) if ( deltas[i]!=NULL ) {
	    for ( k=cnt=0; k<mm->axis_count; ++k )
		if ( mm->positions[i*mm->axis_count+k]!=0 )
		    ++cnt;
	    if ( cnt==j ) {
		for ( l = 0; l<mm->instance_count; ++l ) if ( l!=i && deltas[l]!=NULL ) {
		    for ( k=0; k<mm->axis_count; ++k )
			if ( mm->positions[i*mm->axis_count+k]!=0 &&
				mm->positions[l*mm->axis_count+k]!=mm->positions[i*mm->axis_count+k])
		    break;
		    if ( k==mm->axis_count ) {
			for ( k=0; k<ptcnt; ++k )
			    deltas[l][k] -= deltas[i][k];
		    }
		}
	    }
	}
    }

    /* If all variants of the cvt are the same, no point in having a gvar */
    /*  entry for it */
    for ( i=0 ; i<mm->instance_count; ++i ) if ( deltas[i]!=NULL ) {
	for ( j=0; j<ptcnt; ++j )
	    if ( deltas[i][j]!=0 )
	break;
	if ( j==ptcnt ) {
	    free(deltas[i]);
	    deltas[i] = NULL;
	}
    }
    for ( i=0 ; i<mm->instance_count; ++i )
	if ( deltas[i]!=NULL )
    break;
    if ( i==mm->instance_count ) {
	/* All zeros */
	free(deltas);
return( NULL );
    }

return( deltas );
}

static void ttf_dumpcvar(struct alltabs *at, MMSet *mm) {
    int16 **deltas;
    int ptcnt, cnt, pcnt;
    int i,j,rj,big;
    int tuple_size;
    uint32 start, end;
    uint16 *pts;

    deltas = CvtFindDeltas(mm,&ptcnt);
    if ( deltas == NULL ) return;
    for ( i=cnt=0; i<mm->instance_count; ++i )
	if ( deltas[i]!=NULL )
	    ++cnt;
    if ( cnt==0 ) {
	free(deltas);
return;
    }

    tuple_size = 4+2*mm->axis_count;
    at->cvar = tmpfile();
    putlong( at->cvar, 0x00010000 );	/* Format */
    putshort( at->cvar, cnt );		/* Number of instances with cvt tables (tuple count of interesting tuples) */
    putshort( at->cvar, 8+cnt*tuple_size );	/* Offset to data */

    for ( i=0; i<mm->instance_count; ++i ) if ( deltas[i]!=NULL ) {
	putshort( at->cvar, 0 );	/* tuple data size, figure out later */
	putshort( at->cvar, 0xa000 );	/* tuple coords follow, private points in data */
	for ( j=0; j<mm->axis_count; ++j )
	    putshort( at->cvar, rint(16384*mm->positions[i*mm->axis_count+j]) );
    }
    if ( ftell( at->cvar )!=8+cnt*tuple_size )
	IError( "Data offset wrong" );

    for ( i=cnt=0; i<mm->instance_count; ++i ) if ( deltas[i]!=NULL ) {
	start = ftell(at->cvar);
	for ( j=pcnt=0; j<ptcnt; ++j )
	    if ( deltas[i][j]!=0 )
		++pcnt;
	pts = malloc(pcnt*sizeof(uint16));
	for ( j=pcnt=0; j<ptcnt; ++j )
	    if ( deltas[i][j]!=0 )
		pts[pcnt++]=j;

	if ( pcnt>0x7f ) {
	    putc(0x80|(pcnt>>8), at->cvar );
	    putc(pcnt&0xff, at->cvar);
	} else
	    putc(pcnt, at->cvar);
	for ( j=0; j<pcnt; ) {
	    big = pts[j]>=0x80 ? 0x80 : 0;
	    for ( rj=j+1 ; rj<j+0x80 && rj<pcnt && !big; ++rj )
		if ( pts[rj]-pts[rj-1]>=0x80 )
		    big = 0x80;
	    
	    putc((rj-j-1)|big,at->cvar);
	    if ( big ) {
		putshort(at->cvar,pts[j]);
		for ( ++j; j<rj; ++j )
		    putshort(at->cvar,pts[j]-pts[j-1]);
	    } else {
		putc(pts[j],at->cvar);
		for ( ++j; j<rj; ++j )
		    putc(pts[j]-pts[j-1],at->cvar);
	    }
	}
	/* Now output the corresponding deltas for those points */
	for ( j=0; j<pcnt; ) {
	    if ( deltas[i][j]>0x7f || deltas[i][j]<0x80 ) {
		for ( rj=j+1; rj<j+0x40 && rj<pcnt; ++rj ) {
		    if ( deltas[i][pts[rj]]>0x7f || deltas[i][pts[rj]]<0x80 ||
			    (rj+1<j+0x40 && rj+1<pcnt && (deltas[i][pts[rj+1]]>0x7f || deltas[i][pts[rj+1]]<0x80)) )
			/* Keep going with a big run */;
		    else
		break;
		}
		putc( (rj-j-1)|0x40,at->cvar );
		for ( ; j<rj ; ++j )
		    putshort( at->cvar, deltas[i][pts[j]] );
	    } else {
		for ( rj=j+1; rj<j+0x40 && rj<pcnt; ++rj ) {
		    if ( deltas[i][pts[rj]]>0x7f || deltas[i][pts[rj]]<0x80 )
		break;
		}
		putc( rj-j-1,at->cvar );
		for ( ; j<rj ; ++j )
		    putc( deltas[i][pts[j]], at->cvar );
	    }
	}
	free(pts);
	end = ftell(at->cvar);
	fseek(at->cvar, 8+cnt*tuple_size, SEEK_SET);
	putshort(at->cvar,end-start);
	fseek(at->cvar, end, SEEK_SET);
	++cnt;
    }

    for ( i=0; i<mm->instance_count; ++i )
	free( deltas[i] );
    free(deltas);

    at->cvarlen = ftell(at->cvar);
    if ( at->cvarlen&1 )
	putc('\0',at->cvar );
    if ( ftell(at->cvar)&2 )
	putshort(at->cvar,0);
}

static void dumpdeltas(struct alltabs *at,int16 *deltas,int ptcnt) {
    int j,rj;

    for ( j=0; j<ptcnt; ) {
	for ( rj=j; rj<ptcnt && rj<j+0x40 && deltas[rj]==0; ++rj );
	if ( rj!=j ) {
	    putc((rj-j-1)|0x80,at->gvar);
	    j = rj;
    continue;
	}
	if ( deltas[j]>0x7f || deltas[j]<0x80 ) {
	    for ( rj=j+1; rj<j+0x40 && rj<ptcnt; ++rj ) {
		if ( deltas[rj]>0x7f || deltas[rj]<0x80 ||
			(rj+1<j+0x40 && rj+1<ptcnt && (deltas[rj+1]>0x7f || deltas[rj+1]<0x80)) )
		    /* Keep going with a big run */;
		else
	    break;
	    }
	    putc( (rj-j-1)|0x40,at->gvar );
	    for ( ; j<rj ; ++j )
		putshort( at->gvar, deltas[j] );
	} else {
	    for ( rj=j+1; rj<j+0x40 && rj<ptcnt; ++rj ) {
		if ( deltas[rj]>0x7f || deltas[rj]<0x80 ||
			(deltas[rj]==0 && rj+1<j+0x40 && rj+1<ptcnt &&
			    deltas[rj+1]<=0x7f && deltas[rj+1]>=0x80 && deltas[rj+1]!=0 ))
	    break;
	    }
	    putc( rj-j-1,at->gvar );
	    for ( ; j<rj ; ++j )
		putc( deltas[j], at->gvar );
	}
    }
}

static void ttf_dumpgvar(struct alltabs *at, MMSet *mm) {
    int i,j, last;
    uint32 gcoordoff, glyphoffs, start, here, tupledataend, tupledatastart;
    int16 **deltas;
    int ptcnt;

    at->gvar = tmpfile();
    putlong( at->gvar, 0x00010000 );	/* Format */
    putshort( at->gvar, mm->axis_count );
    putshort( at->gvar, mm->instance_count );	/* Number of global tuples */
    gcoordoff = ftell(at->gvar);
    putlong( at->gvar, 0 );		/* Offset to global tuples, fix later */
    putshort( at->gvar,at->maxp.numGlyphs );
    putshort( at->gvar, 1 );		/* always output 32bit offsets */
    putlong( at->gvar, ftell(at->gvar)+4 + (at->maxp.numGlyphs+1)*4);
    glyphoffs = ftell(at->gvar);
    for ( i=0; i<=at->maxp.numGlyphs; ++i )
	putlong( at->gvar,0 );

    start = ftell( at->gvar );
    last = -1;
    for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	deltas = SCFindDeltas(mm,at->gi.bygid[i],&ptcnt);
	if ( deltas==NULL )
    continue;
	here = ftell(at->gvar);
	fseek(at->gvar,glyphoffs+(last+1)*4,SEEK_SET);
	for ( ; last< i; ++last )
	    putlong(at->gvar,here-start);
	fseek(at->gvar,here,SEEK_SET);
	putshort(at->gvar,mm->instance_count);
	putshort(at->gvar,4+4*mm->instance_count);	/* offset to data */
	for ( j=0; j<mm->instance_count; ++j ) {
	    putshort(at->gvar,0);			/* tuple data size, fix later */
	    putshort(at->gvar,0x2000|j);		/* private points, tuple i */
	}
	for ( j=0; j<mm->instance_count; ++j ) {
	    tupledatastart = ftell(at->gvar);
	    putc('\0',at->gvar);			/* Point list, all points */
	    dumpdeltas(at,deltas[2*j],ptcnt);
	    dumpdeltas(at,deltas[2*j+1],ptcnt);
	    tupledataend = ftell(at->gvar);
	    fseek(at->gvar,here+4+4*j,SEEK_SET);
	    putshort( at->gvar,tupledataend-tupledatastart);
	    fseek(at->gvar,tupledataend,SEEK_SET);
	    free(deltas[2*j]); free(deltas[2*j+1]);
	}
	free(deltas);
    }
    here = ftell(at->gvar);
    fseek(at->gvar,glyphoffs+(last+1)*4,SEEK_SET);
    for ( ; last< at->maxp.numGlyphs; ++last )
	putlong(at->gvar,here-start);
    fseek(at->gvar,gcoordoff,SEEK_SET);
    putlong(at->gvar,here);
    fseek(at->gvar,here,SEEK_SET);
    for ( j=0; j<mm->instance_count; ++j ) {
	for ( i=0; i<mm->axis_count; ++i )
	    putshort(at->gvar,rint(16384*mm->positions[j*mm->axis_count+i]));
    }

    at->gvarlen = ftell(at->gvar);
    if ( at->gvarlen&1 )
	putc('\0',at->gvar );
    if ( ftell(at->gvar)&2 )
	putshort(at->gvar,0);
}
    

static void ttf_dumpavar(struct alltabs *at, MMSet *mm) {
    int i,j;

    for ( i=0; i<mm->axis_count; ++i ) {
	if ( mm->axismaps[i].points>3 )
    break;
    }
    if ( i==mm->axis_count )		/* We only have simple axes */
return;					/* No need for a variation table */

    at->avar = tmpfile();
    putlong( at->avar, 0x00010000 );	/* Format */
    putlong( at->avar, mm->axis_count );
    for ( i=0; i<mm->axis_count; ++i ) {
	putshort( at->avar, mm->axismaps[i].points );
	for ( j=0; j<mm->axismaps[i].points; ++j ) {
	    if ( mm->axismaps[i].designs[j]<mm->axismaps[i].def )
		putshort( at->avar, (mm->axismaps[i].designs[j]-mm->axismaps[i].def)*16384/
			(mm->axismaps[i].def-mm->axismaps[i].min));
	    else
		putshort( at->avar, (mm->axismaps[i].designs[j]-mm->axismaps[i].def)*16384/
			(mm->axismaps[i].max-mm->axismaps[i].def));
	    putshort( at->avar, mm->axismaps[i].blends[j]*16384);
	}
    }

    at->avarlen = ftell(at->avar);
    if ( at->avarlen&2 )
	putshort(at->avar,0);
}

static uint32 AxisNameToTag(char *name) {
    char buf[4];
    int i;

    if ( strmatch(name,"Weight")==0 )
return( CHR('w','g','h','t'));
    if ( strmatch(name,"Width")==0 )
return( CHR('w','d','t','h'));
    if ( strmatch(name,"OpticalSize")==0 )
return( CHR('o','p','s','z'));
    if ( strmatch(name,"Slant")==0 )
return( CHR('s','l','n','t'));

    memset(buf,0,sizeof(buf));
    for ( i=0; i<4 && name[i]!='\0'; ++i )
	buf[i] = name[i];
return( CHR(buf[0],buf[1],buf[2],buf[3]));
}

static int AllocateStrId(struct alltabs *at,struct macname *mn) {
    struct other_names *on;

    if ( mn==NULL )
return( 0 );

    on = chunkalloc(sizeof(struct other_names));
    on->strid = at->next_strid++;
    on->mn = mn;
    on->next = at->other_names;
    at->other_names = on;
return( on->strid );
}

static void ttf_dumpfvar(struct alltabs *at, MMSet *mm) {
    int i,j;

    at->fvar = tmpfile();
    putlong( at->fvar, 0x00010000 );	/* Format */
    putshort( at->fvar, 16 );		/* Offset to first axis data */
    putshort( at->fvar, 2 );		/* Size count pairs */
    putshort( at->fvar, mm->axis_count );
    putshort( at->fvar, 20 );		/* Size of each axis record */
    putshort( at->fvar, mm->named_instance_count );
    putshort( at->fvar, 4+4*mm->axis_count );

    /* For each axis ... */
    for ( i=0; i<mm->axis_count; ++i ) {
	putlong( at->fvar, AxisNameToTag(mm->axes[i]) );
	putlong( at->fvar, rint(mm->axismaps[i].min*65536));
	putlong( at->fvar, rint(mm->axismaps[i].def*65536));
	putlong( at->fvar, rint(mm->axismaps[i].max*65536));
	putshort(at->fvar, 0 );			/* No flags defined for axes */
	putshort(at->fvar, AllocateStrId(at,mm->axismaps[i].axisnames));
    }

    /* For each named font ... */
    for ( i=0; i<mm->named_instance_count; ++i ) {
	putshort(at->fvar, AllocateStrId(at,mm->named_instances[i].names));
	putshort(at->fvar, 0 );			/* No flags here either */
	for ( j=0; j<mm->axis_count; ++j )
	    putlong(at->fvar, rint(65536*mm->named_instances[i].coords[j]));
    }

    at->fvarlen = ftell(at->fvar);
    if ( at->fvarlen&2 )		/* I don't think this is ever hit */
	putshort(at->fvar,0);
}

void ttf_dumpvariations(struct alltabs *at, SplineFont *sf) {
    MMSet *mm = sf->mm;
    int i,j;

    for ( j=0; j<sf->glyphcnt; ++j ) if ( sf->glyphs[j]!=NULL ) {
	for ( i=0; i<mm->instance_count; ++i ) if ( mm->instances[i]->glyphs[j]!=NULL )
	    mm->instances[i]->glyphs[j]->ttf_glyph = sf->glyphs[j]->ttf_glyph;
    }

    ttf_dumpfvar(at,mm);
    ttf_dumpgvar(at,mm);
    ttf_dumpcvar(at,mm);
    ttf_dumpavar(at,mm);
}
