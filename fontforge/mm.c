/* Copyright (C) 2003-2012 by George Williams */
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

#include "mm.h"

#include "dumppfa.h"
#include "fontforgevw.h"
#include "lookups.h"
#include "macenc.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "tottfgpos.h"
#include <ustring.h>
#include <math.h>
#include <utype.h>
#include "ttf.h"

const char *MMAxisAbrev(char *axis_name) {
    if ( strcmp(axis_name,"Weight")==0 )
return( "wt" );
    if ( strcmp(axis_name,"Width")==0 )
return( "wd" );
    if ( strcmp(axis_name,"OpticalSize")==0 )
return( "op" );
    if ( strcmp(axis_name,"Slant")==0 )
return( "sl" );

return( axis_name );
}

bigreal MMAxisUnmap(MMSet *mm,int axis,bigreal ncv) {
    struct axismap *axismap = &mm->axismaps[axis];
    int j;

    if ( ncv<=axismap->blends[0] )
return(axismap->designs[0]);

    for ( j=1; j<axismap->points; ++j ) {
	if ( ncv<=axismap->blends[j]) {
	    bigreal t = (ncv-axismap->blends[j-1])/(axismap->blends[j]-axismap->blends[j-1]);
return( axismap->designs[j-1]+ t*(axismap->designs[j]-axismap->designs[j-1]) );
	}
    }

return(axismap->designs[axismap->points-1]);
}

static char *_MMMakeFontname(MMSet *mm,real *normalized,char **fullname) {
    char *pt, *pt2, *hyphen=NULL;
    char *ret = NULL;
    int i,j;

    if ( mm->apple ) {
	for ( i=0; i<mm->named_instance_count; ++i ) {
	    for ( j=0; j<mm->axis_count; ++j ) {
		if (( normalized[j] == -1 &&
			RealApprox(mm->named_instances[i].coords[j],mm->axismaps[j].min) ) ||
		    ( normalized[j] ==  0 &&
			RealApprox(mm->named_instances[i].coords[j],mm->axismaps[j].def) ) ||
		    ( normalized[j] ==  1 &&
			RealApprox(mm->named_instances[i].coords[j],mm->axismaps[j].max) ))
		    /* A match so far */;
		else
	    break;
	    }
	    if ( j==mm->axis_count )
	break;
	}
	if ( i!=mm->named_instance_count ) {
	    char *styles = PickNameFromMacName(mm->named_instances[i].names);
	    if ( styles==NULL )
		styles = FindEnglishNameInMacName(mm->named_instances[i].names);
	    if ( styles!=NULL ) {
		ret = malloc(strlen(mm->normal->familyname)+ strlen(styles)+3 );
		strcpy(ret,mm->normal->familyname);
		hyphen = ret+strlen(ret);
		strcpy(hyphen," ");
		strcpy(hyphen+1,styles);
		free(styles);
	    }
	}
    }

    if ( ret==NULL ) {
	pt = ret = malloc(strlen(mm->normal->familyname)+ mm->axis_count*15 + 1);
	strcpy(pt,mm->normal->familyname);
	pt += strlen(pt);
	*pt++ = '_';
	for ( i=0; i<mm->axis_count; ++i ) {
	    if ( !mm->apple )
		sprintf( pt, " %d%s", (int) rint(MMAxisUnmap(mm,i,normalized[i])),
			MMAxisAbrev(mm->axes[i]));
	    else
		sprintf( pt, " %.1f%s", (double) MMAxisUnmap(mm,i,normalized[i]),
			MMAxisAbrev(mm->axes[i]));
	    pt += strlen(pt);
	}
	if ( pt>ret && pt[-1]==' ' )
	    --pt;
	*pt = '\0';
    }

    *fullname = ret;

    ret = copy(ret);
    for ( pt=*fullname, pt2=ret; *pt!='\0'; ++pt )
	if ( pt==hyphen )
	    *pt2++ = '-';
	else if ( *pt!=' ' )
	    *pt2++ = *pt;
    *pt2 = '\0';
return( ret );
}

char *MMMakeMasterFontname(MMSet *mm,int ipos,char **fullname) {
return( _MMMakeFontname(mm,&mm->positions[ipos*mm->axis_count],fullname));
}

static char *_MMGuessWeight(MMSet *mm,real *normalized,char *def) {
    int i;
    char *ret;
    real design;

    for ( i=0; i<mm->axis_count; ++i ) {
	if ( strcmp(mm->axes[i],"Weight")==0 )
    break;
    }
    if ( i==mm->axis_count )
return( def );
    design = MMAxisUnmap(mm,i,normalized[i]);
    if ( design<50 || design>1500 )	/* Er... Probably not the 0...1000 range I expect */
return( def );
    ret = NULL;
    if ( design<150 )
	ret = "Thin";
    else if ( design<350 )
	ret = "Light";
    else if ( design<550 )
	ret = "Medium";
    else if ( design<650 )
	ret = "DemiBold";
    else if ( design<750 )
	ret = "Bold";
    else if ( design<850 )
	ret = "Heavy";
    else
	ret = "Black";
    free( def );
return( copy(ret) );
}

char *MMGuessWeight(MMSet *mm,int ipos,char *def) {
return( _MMGuessWeight(mm,&mm->positions[ipos*mm->axis_count],def));
}

/* Given a postscript array of scalars, what's the ipos'th element? */
char *MMExtractNth(char *pt,int ipos) {
    int i;
    char *end;

    while ( *pt==' ' ) ++pt;
    if ( *pt=='[' ) ++pt;
    for ( i=0; *pt!=']' && *pt!='\0'; ++i ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt==']' || *pt=='\0' )
return( NULL );
	for ( end=pt; *end!=' ' && *end!=']' && *end!='\0'; ++end );
	if ( i==ipos )
return( copyn(pt,end-pt));
	pt = end;
    }
return( NULL );
}

/* Given a postscript array of arrays, such as those found in Blend Private BlueValues */
/* return the array composed of the ipos'th element of each sub-array */
char *MMExtractArrayNth(char *pt,int ipos) {
    char *hold[40], *ret;
    int i,j,len;

    while ( *pt==' ' ) ++pt;
    if ( *pt=='[' ) ++pt;
    i = 0;
    while ( *pt!=']' && *pt!=' ' ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='[' ) {
	    if ( i<sizeof(hold)/sizeof(hold[0]) )
		hold[i++] = MMExtractNth(pt,ipos);
	    ++pt;
	    while ( *pt!=']' && *pt!='\0' ) ++pt;
	}
	if ( *pt!='\0' )
	    ++pt;
    }
    if ( i==0 )
return( NULL );
    for ( j=len=0; j<i; ++j ) {
	if ( hold[j]==NULL ) {
	    for ( j=0; j<i; ++j )
		free(hold[j]);
return( NULL );
	}
	len += strlen( hold[j] )+1;
    }

    pt = ret = malloc(len+4);
    *pt++ = '[';
    for ( j=0; j<i; ++j ) {
	strcpy(pt,hold[j]);
	free(hold[j]);
	pt += strlen(pt);
	if ( j!=i-1 )
	    *pt++ = ' ';
    }
    *pt++ = ']';
    *pt++ = '\0';
return( ret );
}

void MMKern(SplineFont *sf,SplineChar *first,SplineChar *second,int diff,
	struct lookup_subtable *sub,KernPair *oldkp) {
    MMSet *mm = sf->mm;
    KernPair *kp;
    int i;
    /* If the user creates a kern pair in one font of a multiple master set */
    /*  then we should create the same kern pair in all the others. Similarly */
    /*  if s/he modifies a kern pair in the weighted version then apply that */
    /*  mod to all the others */

    if ( mm==NULL )
return;
    if ( sf==mm->normal || oldkp==NULL ) {
	for ( i= -1; i<mm->instance_count; ++i ) {
	    SplineFont *cur = i==-1 ? mm->normal : mm->instances[i];
	    SplineChar *psc, *ssc;
	    if ( cur==sf )	/* Done in caller */
	continue;
	    psc = cur->glyphs[first->orig_pos];
	    ssc = cur->glyphs[second->orig_pos];
	    if ( psc==NULL || ssc==NULL )		/* Should never happen*/
	continue;
	    for ( kp = psc->kerns; kp!=NULL && kp->sc!=ssc; kp = kp->next );
	    /* No mm support for vertical kerns */
	    if ( kp==NULL ) {
		kp = chunkalloc(sizeof(KernPair));
		if ( oldkp!=NULL )
		    *kp = *oldkp;
		else {
		    kp->off = diff;
		    if ( sub==NULL )
			sub = SFSubTableFindOrMake(cur,
				CHR('k','e','r','n'), SCScriptFromUnicode(psc), gpos_pair);
		    kp->subtable = sub;
		}
		kp->sc = ssc;
		kp->next = psc->kerns;
		psc->kerns = kp;
	    } else
		kp->off += diff;
	}
    }
}

static SplineChar *SFMakeGlyphLike(SplineFont *sf, int gid, SplineFont *base) {
    SplineChar *sc = SFSplineCharCreate(sf), *bsc = base->glyphs[gid];
    sc->orig_pos = gid;
    sf->glyphs[gid] = sc;

    sc->width = bsc->width; sc->widthset = true; sc->vwidth = bsc->vwidth;
    free(sc->name); sc->name = copy(bsc->name);
    sc->unicodeenc = bsc->unicodeenc;
return( sc );
}

static char *_MMBlendChar(MMSet *mm, int gid) {
    int i, j, worthit = -1;
    int all, any, any2, all2, anyend, allend, diff;
    SplineChar *sc;
    SplinePointList *spls[MmMax], *spl, *spllast;
    SplinePoint *tos[MmMax], *to;
    RefChar *refs[MmMax], *ref, *reflast;
    KernPair *kp0, *kptest, *kp, *kplast;
    StemInfo *hs[MmMax], *h, *hlast;
    real width;

    for ( i=0; i<mm->instance_count; ++i ) {
	if ( mm->instances[i]->layers[ly_fore].order2 )
return( _("One of the multiple master instances contains quadratic splines. It must be converted to cubic splines before it can be used in a multiple master") );
	if ( gid>=mm->instances[i]->glyphcnt )
return( _("The different instances of this mm have a different number of glyphs") );
	if ( SCWorthOutputting(mm->instances[i]->glyphs[gid]) ) {
	    if ( worthit == -1 ) worthit = true;
	    else if ( worthit != true )
return( _("This glyph is defined in one instance font but not in another") );
	} else {
	    if ( worthit == -1 ) worthit = false;
	    else if ( worthit != false )
return( _("This glyph is defined in one instance font but not in another") );
	}
    }

    sc = mm->normal->glyphs[gid];
    if ( sc!=NULL ) {
	SCClearContents(sc,ly_fore);
	KernPairsFree(sc->kerns);
	sc->kerns = NULL;
	KernPairsFree(sc->vkerns);
	sc->vkerns = NULL;
    }

    if ( !worthit )
return( 0 );

    if ( sc==NULL )
	sc = SFMakeGlyphLike(mm->normal,gid,mm->instances[0]);

	/* Blend references => blend transformation matrices */
    diff = false;
    any = false; all = true;
    for ( i=0; i<mm->instance_count; ++i ) {
	refs[i] = mm->instances[i]->glyphs[gid]->layers[ly_fore].refs;
	if ( refs[i]!=NULL ) any = true;
	else all = false;
    }
    reflast = NULL;
    while ( all ) {
	ref = RefCharCreate();
	*ref = *refs[0];
	ref->layers[0].splines = NULL;
	ref->next = NULL;
	memset(ref->transform,0,sizeof(ref->transform));
	ref->sc = mm->normal->glyphs[refs[0]->sc->orig_pos];
	for ( i=0; i<mm->instance_count; ++i ) {
	    if ( ref->sc->orig_pos!=refs[i]->sc->orig_pos )
		diff = true;
	    for ( j=0; j<6; ++j )
		ref->transform[j] += refs[i]->transform[j]*mm->defweights[i];
	}
	if ( reflast==NULL )
	    sc->layers[ly_fore].refs = ref;
	else
	    reflast->next = ref;
	reflast = ref;
	any = false; all = true;
	for ( i=0; i<mm->instance_count; ++i ) {
	    refs[i] = refs[i]->next;
	    if ( refs[i]!=NULL ) any = true;
	    else all = false;
	}
    }
    if ( any )
return( _("This glyph contains a different number of references in different instances") );
    if ( diff )
return( _("A reference in this glyph refers to a different encoding in different instances") );

	/* Blend Width */
    width = 0;
    for ( i=0; i<mm->instance_count; ++i )
	width += mm->instances[i]->glyphs[gid]->width*mm->defweights[i];
    sc->width = width;
    width = 0;
    for ( i=0; i<mm->instance_count; ++i )
	width += mm->instances[i]->glyphs[gid]->vwidth*mm->defweights[i];
    sc->vwidth = width;

	/* Blend Splines */
    any = false; all = true;
    for ( i=0; i<mm->instance_count; ++i ) {
	spls[i] = mm->instances[i]->glyphs[gid]->layers[ly_fore].splines;
	if ( spls[i]!=NULL ) any = true;
	else all = false;
    }
    spllast = NULL;
    while ( all ) {
	spl = chunkalloc(sizeof(SplinePointList));
	if ( spllast==NULL )
	    sc->layers[ly_fore].splines = spl;
	else
	    spllast->next = spl;
	spllast = spl;
	for ( i=0; i<mm->instance_count; ++i )
	    tos[i] = spls[i]->first;
	all2 = true;
	spl->last = NULL;
	while ( all2 ) {
	    to = chunkalloc(sizeof(SplinePoint));
	    to->nonextcp = tos[0]->nonextcp;
	    to->noprevcp = tos[0]->noprevcp;
	    to->nextcpdef = tos[0]->nextcpdef;
	    to->prevcpdef = tos[0]->prevcpdef;
	    to->pointtype = tos[0]->pointtype;
	    if ( tos[0]->hintmask!=NULL ) {
		to->hintmask = chunkalloc(sizeof(HintMask));
		memcpy(to->hintmask,tos[0]->hintmask,sizeof(HintMask));
	    }
	    for ( i=0; i<mm->instance_count; ++i ) {
		to->me.x += tos[i]->me.x*mm->defweights[i];
		to->me.y += tos[i]->me.y*mm->defweights[i];
		to->nextcp.x += tos[i]->nextcp.x*mm->defweights[i];
		to->nextcp.y += tos[i]->nextcp.y*mm->defweights[i];
		to->prevcp.x += tos[i]->prevcp.x*mm->defweights[i];
		to->prevcp.y += tos[i]->prevcp.y*mm->defweights[i];
	    }
	    if ( spl->last==NULL )
		spl->first = to;
	    else
		SplineMake3(spl->last,to);
	    spl->last = to;
	    any2 = false; all2 = true;
	    for ( i=0; i<mm->instance_count; ++i ) {
		if ( tos[i]->next==NULL ) tos[i] = NULL;
		else tos[i] = tos[i]->next->to;
		if ( tos[i]!=NULL ) any2 = true;
		else all2 = false;
	    }
	    if ( !all2 && any2 )
return( _("A contour in this glyph contains a different number of points in different instances") );
	    anyend = false; allend = true;
	    for ( i=0; i<mm->instance_count; ++i ) {
		if ( tos[i]==spls[i]->first ) anyend = true;
		else allend = false;
	    }
	    if ( allend ) {
		SplineMake3(spl->last,spl->first);
		spl->last = spl->first;
	break;
	    }
	    if ( anyend )
return( _("A contour in this glyph contains a different number of points in different instances") );
	}
	any = false; all = true;
	for ( i=0; i<mm->instance_count; ++i ) {
	    spls[i] = spls[i]->next;
	    if ( spls[i]!=NULL ) any = true;
	    else all = false;
	}
    }
    if ( any )
return( _("This glyph contains a different number of contours in different instances") );

	/* Blend hints */
    for ( j=0; j<2; ++j ) {
	any = false; all = true;
	for ( i=0; i<mm->instance_count; ++i ) {
	    hs[i] = j ? mm->instances[i]->glyphs[gid]->hstem : mm->instances[i]->glyphs[gid]->vstem;
	    if ( hs[i]!=NULL ) any = true;
	    else all = false;
	}
	hlast = NULL;
	while ( all ) {
	    h = chunkalloc(sizeof(StemInfo));
	    *h = *hs[0];
	    h->where = NULL;
	    h->next = NULL;
	    h->start = h->width = 0;
	    for ( i=0; i<mm->instance_count; ++i ) {
		h->start += hs[i]->start*mm->defweights[i];
		h->width += hs[i]->width*mm->defweights[i];
	    }
	    if ( hlast!=NULL )
		hlast->next = h;
	    else if ( j )
		sc->hstem = h;
	    else
		sc->vstem = h;
	    hlast = h;
	    any = false; all = true;
	    for ( i=0; i<mm->instance_count; ++i ) {
		hs[i] = hs[i]->next;
		if ( hs[i]!=NULL ) any = true;
		else all = false;
	    }
	}
	if ( any )
return( _("This glyph contains a different number of hints in different instances") );
    }

	/* Blend kernpairs */
    /* I'm not requiring ordered kerning pairs */
    /* I'm not bothering with vertical kerning */
    kp0 = mm->instances[0]->glyphs[gid]->kerns;
    kplast = NULL;
    while ( kp0!=NULL ) {
	int off = kp0->off*mm->defweights[0];
	for ( i=1; i<mm->instance_count; ++i ) {
	    for ( kptest=mm->instances[i]->glyphs[gid]->kerns; kptest!=NULL; kptest=kptest->next )
		if ( kptest->sc->orig_pos==kp0->sc->orig_pos )
	    break;
	    if ( kptest==NULL )
return( _("This glyph contains different kerning pairs in different instances") );
	    off += kptest->off*mm->defweights[i];
	}
	kp = chunkalloc(sizeof(KernPair));
	kp->sc = mm->normal->glyphs[kp0->sc->orig_pos];
	kp->off = off;
	kp->subtable = kp0->subtable;
	if ( kplast!=NULL )
	    kplast->next = kp;
	else
	    sc->kerns = kp;
	kplast = kp;
	kp0 = kp0->next;
    }
return( 0 );
}

char *MMBlendChar(MMSet *mm, int gid) {
    char *ret;
    RefChar *ref;

    if ( gid>=mm->normal->glyphcnt )
return( _("The different instances of this mm have a different number of glyphs") );
    ret = _MMBlendChar(mm,gid);
    if ( mm->normal->glyphs[gid]!=NULL ) {
	SplineChar *sc = mm->normal->glyphs[gid];
	for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	    SCReinstanciateRefChar(sc,ref,ly_fore);
	    SCMakeDependent(sc,ref->sc);
	}
    }
return(ret);
}

static struct psdict *BlendPrivate(struct psdict *private,MMSet *mm) {
    struct psdict *other;
    real sum, val;
    char *data;
    int i,j,k, cnt;
    char *values[MmMax], buffer[32], *space, *pt, *end;

    other = mm->instances[0]->private;
    if ( other==NULL )
return( private );

    if ( private==NULL )
	private = calloc(1,sizeof(struct psdict));

    i = PSDictFindEntry(private,"ForceBoldThreshold");
    if ( i!=-1 ) {
	val = strtod(private->values[i],NULL);
	sum = 0;
	for ( j=0; j<mm->instance_count; ++j ) {
	    i = PSDictFindEntry(mm->instances[j]->private,"ForceBold");
	    if ( i!=-1 && strcmp(mm->instances[j]->private->values[i],"true")==0 )
		sum += mm->defweights[j];
	}
	data = ( sum>=val ) ? "true" : "false";
	PSDictChangeEntry(private,"ForceBold",data);
    }
    for ( i=0; i<other->next; ++i ) {
	if ( *other->values[i]!='[' && !isdigit( *other->values[i]) && *other->values[i]!='.' )
    continue;
	for ( j=0; j<mm->instance_count; ++j ) {
	    k = PSDictFindEntry(mm->instances[j]->private,other->keys[i]);
	    if ( k==-1 )
	break;
	    values[j] = mm->instances[j]->private->values[k];
	}
	if ( j!=mm->instance_count )
    continue;
	if ( *other->values[i]!='[' ) {
	    /* blend a real number */
	    sum = 0;
	    for ( j=0; j<mm->instance_count; ++j ) {
		val = strtod(values[j],&end);
		if ( end==values[j])
	    break;
		sum += val*mm->defweights[j];
	    }
	    if ( j!=mm->instance_count )
    continue;
	    sprintf(buffer,"%g",(double) sum);
	    PSDictChangeEntry(private,other->keys[i],buffer);
	} else {
	    /* Blend an array of numbers */
	    for ( pt = values[0], cnt=0; *pt!='\0' && *pt!=']'; ++pt ) {
		if ( *pt==' ' ) {
		    while ( *pt==' ' ) ++pt;
		    --pt;
		    ++cnt;
		}
	    }
	    space = pt = malloc((cnt+2)*24+4);
	    *pt++ = '[';
	    for ( j=0; j<mm->instance_count; ++j )
		if ( *values[j]=='[' ) ++values[j];
	    while ( *values[0]!=']' ) {
		sum = 0;
		for ( j=0; j<mm->instance_count; ++j ) {
		    val = strtod(values[j],&end);
		    sum += val*mm->defweights[j];
		    while ( *end==' ' ) ++end;
		    values[j] = end;
		}
		sprintf( pt,"%g ", (double) sum);
		pt += strlen(pt);
	    }
	    if ( pt[-1]==' ' ) --pt;
	    *pt++ = ']';
	    *pt = '\0';
	    PSDictChangeEntry(private,other->keys[i],space);
	    free(space);
	}
    }

return( private );
}

int MMReblend(FontViewBase *fv, MMSet *mm) {
    char *olderr, *err;
    int i, first = -1;
    SplineFont *sf = mm->instances[0];
    RefChar *ref;

    olderr = NULL;
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( i>=mm->normal->glyphcnt )
    break;
	err = MMBlendChar(mm,i);
	if ( mm->normal->glyphs[i]!=NULL )
	    _SCCharChangedUpdate(mm->normal->glyphs[i],ly_fore,-1);
	if ( err==NULL )
    continue;
	if ( olderr==NULL ) {
	    if ( fv!=NULL )
		(fv_interface->deselect_all)(fv);
	    first = i;
	}
	if ( olderr==NULL || olderr == err )
	    olderr = err;
	else
	    olderr = (char *) -1;
	if ( fv!=NULL ) {
	    int enc = fv->map->backmap[i];
	    if ( enc!=-1 )
		fv->selected[enc] = true;
	}
    }

    sf = mm->normal;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( ref=sf->glyphs[i]->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	    SCReinstanciateRefChar(sf->glyphs[i],ref,ly_fore);
	    SCMakeDependent(sf->glyphs[i],ref->sc);
	}
    }
    sf->private = BlendPrivate(sf->private,mm);

    if ( olderr == NULL )	/* No Errors */
return( true );

    if ( fv!=NULL ) {
	FVDisplayEnc(fv,first);
	if ( olderr==(char *) -1 )
	    ff_post_error(_("Bad Multiple Master Font"),_("Various errors occurred at the selected glyphs"));
	else
	    ff_post_error(_("Bad Multiple Master Font"),_("The following error occurred on the selected glyphs: %.100s"),olderr);
    }
return( false );
}

void MMWeightsUnMap(real weights[MmMax], real axiscoords[4],
	int axis_count) {

    if ( axis_count==1 )
	axiscoords[0] = weights[1];
    else if ( axis_count==2 ) {
	axiscoords[0] = weights[3]+weights[1];
	axiscoords[1] = weights[3]+weights[2];
    } else if ( axis_count==3 ) {
	axiscoords[0] = weights[7]+weights[5]+weights[3]+weights[1];
	axiscoords[1] = weights[7]+weights[6]+weights[3]+weights[2];
	axiscoords[2] = weights[7]+weights[6]+weights[5]+weights[4];
    } else {
	axiscoords[0] = weights[15]+weights[13]+weights[11]+weights[9]+
			weights[7]+weights[5]+weights[3]+weights[1];
	axiscoords[1] = weights[15]+weights[14]+weights[11]+weights[10]+
			weights[7]+weights[6]+weights[3]+weights[2];
	axiscoords[2] = weights[15]+weights[14]+weights[13]+weights[12]+
			weights[7]+weights[6]+weights[5]+weights[4];
	axiscoords[3] = weights[15]+weights[14]+weights[13]+weights[12]+
			weights[11]+weights[10]+weights[9]+weights[8];
    }
}

SplineFont *_MMNewFont(MMSet *mm,int index,char *familyname,real *normalized) {
    SplineFont *sf, *base;
    char *pt1, *pt2;
    int i;

    sf = SplineFontNew();
    sf->layers[ly_fore].order2 = sf->layers[ly_back].order2 = sf->grid.order2 =
	    mm->apple;
    free(sf->fontname); free(sf->familyname); free(sf->fullname); free(sf->weight);
    sf->familyname = copy(familyname);
    if ( index==-1 ) {
	sf->fontname = copy(familyname);
	for ( pt1=pt2=sf->fontname; *pt1; ++pt1 )
	    if ( *pt1!=' ' )
		*pt2++ = *pt1;
	*pt2='\0';
	sf->fullname = copy(familyname);
    } else
	sf->fontname = _MMMakeFontname(mm,normalized,&sf->fullname);
    sf->weight = copy( "All" );

    base = NULL;
    if ( mm->normal!=NULL )
	base = mm->normal;
    else {
	for ( i=0; i<mm->instance_count; ++i )
	    if ( mm->instances[i]!=NULL ) {
		base = mm->instances[i];
	break;
	}
    }

    if ( base!=NULL ) {
	free(sf->xuid);
	sf->xuid = copy(base->xuid);
	free(sf->glyphs);
	sf->glyphs = calloc(base->glyphcnt,sizeof(SplineChar *));
	sf->glyphcnt = sf->glyphmax = base->glyphcnt;
	sf->new = base->new;
	sf->ascent = base->ascent;
	sf->descent = base->descent;
	free(sf->origname);
	sf->origname = copy(base->origname);
	if ( index<0 ) {
	    free(sf->copyright);
	    sf->copyright = copy(base->copyright);
	}
	/* Make sure we get the encoding exactly right */
	for ( i=0; i<base->glyphcnt; ++i ) if ( base->glyphs[i]!=NULL )
	    SFMakeGlyphLike(sf,i,base);
    }
    sf->onlybitmaps = false;
    sf->mm = mm;
return( sf );
}

SplineFont *MMNewFont(MMSet *mm,int index,char *familyname) {
return( _MMNewFont(mm,index,familyname,&mm->positions[index*mm->axis_count]));
}

FontViewBase *MMCreateBlendedFont(MMSet *mm,FontViewBase *fv,real blends[MmMax],int tonew ) {
    real oldblends[MmMax];
    SplineFont *hold = mm->normal;
    int i;
    real axispos[4];

    for ( i=0; i<mm->instance_count; ++i ) {
	oldblends[i] = mm->defweights[i];
	mm->defweights[i] = blends[i];
    }
    if ( tonew ) {
	SplineFont *new;
	FontViewBase *oldfv = hold->fv;
	char *fn, *full;
	mm->normal = new = MMNewFont(mm,-1,hold->familyname);
	MMWeightsUnMap(blends,axispos,mm->axis_count);
	fn = _MMMakeFontname(mm,axispos,&full);
	free(new->fontname); free(new->fullname);
	new->fontname = fn; new->fullname = full;
	new->weight = _MMGuessWeight(mm,axispos,new->weight);
	new->private = BlendPrivate(PSDictCopy(hold->private),mm);
	new->fv = NULL;
	fv = FontViewCreate(new,false);
	MMReblend(fv,mm);
	new->mm = NULL;
	mm->normal = hold;
	for ( i=0; i<mm->instance_count; ++i ) {
	    mm->defweights[i] = oldblends[i];
	    mm->instances[i]->fv = oldfv;
	}
	hold->fv = oldfv;
    } else {
	for ( i=0; i<mm->instance_count; ++i )
	    mm->defweights[i] = blends[i];
	mm->changed = true;
	MMReblend(fv,mm);
    }
return( fv );
}

/******************************************************************************/
/*                                MM Validation                               */
/******************************************************************************/

static int ContourCount(SplineChar *sc) {
    SplineSet *spl;
    int i;

    for ( spl=sc->layers[ly_fore].splines, i=0; spl!=NULL; spl=spl->next, ++i );
return( i );
}

static int ContourPtMatch(SplineChar *sc1, SplineChar *sc2) {
    SplineSet *spl1, *spl2;
    SplinePoint *sp1, *sp2;

    for ( spl1=sc1->layers[ly_fore].splines, spl2=sc2->layers[ly_fore].splines; spl1!=NULL && spl2!=NULL; spl1=spl1->next, spl2=spl2->next ) {
	for ( sp1=spl1->first, sp2 = spl2->first; ; ) {
	    if ( sp1->nonextcp!=sp2->nonextcp || sp1->noprevcp!=sp2->noprevcp )
return( false );
	    if ( sp1->next==NULL || sp2->next==NULL ) {
		if ( sp1->next==NULL && sp2->next==NULL )
	break;
return( false );
	    }
	    sp1 = sp1->next->to; sp2 = sp2->next->to;
	    if ( sp1==spl1->first || sp2==spl2->first ) {
		if ( sp1==spl1->first && sp2==spl2->first )
	break;
return( false );
	    }
	}
    }
return( true );
}

static int ContourDirMatch(SplineChar *sc1, SplineChar *sc2) {
    SplineSet *spl1, *spl2;

    for ( spl1=sc1->layers[ly_fore].splines, spl2=sc2->layers[ly_fore].splines; spl1!=NULL && spl2!=NULL; spl1=spl1->next, spl2=spl2->next ) {
	if ( SplinePointListIsClockwise(spl1)!=SplinePointListIsClockwise(spl2) )
return( false );
    }
return( true );
}

static int ContourHintMaskMatch(SplineChar *sc1, SplineChar *sc2) {
    SplineSet *spl1, *spl2;
    SplinePoint *sp1, *sp2;

    for ( spl1=sc1->layers[ly_fore].splines, spl2=sc2->layers[ly_fore].splines; spl1!=NULL && spl2!=NULL; spl1=spl1->next, spl2=spl2->next ) {
	for ( sp1=spl1->first, sp2 = spl2->first; ; ) {
	    if ( (sp1->hintmask==NULL)!=(sp2->hintmask==NULL) )
return( false );
	    if ( sp1->hintmask!=NULL && memcmp(sp1->hintmask,sp2->hintmask,sizeof(HintMask))!=0 )
return( false );
	    if ( sp1->next==NULL || sp2->next==NULL ) {
		if ( sp1->next==NULL && sp2->next==NULL )
	break;
return( false );
	    }
	    sp1 = sp1->next->to; sp2 = sp2->next->to;
	    if ( sp1==spl1->first || sp2==spl2->first ) {
		if ( sp1==spl1->first && sp2==spl2->first )
	break;
return( false );
	    }
	}
    }
return( true );
}

static int RefMatch(SplineChar *sc1, SplineChar *sc2) {
    RefChar *ref1, *ref2;
    /* I don't require the reference list to be ordered */

    for ( ref1=sc1->layers[ly_fore].refs, ref2=sc2->layers[ly_fore].refs; ref1!=NULL && ref2!=NULL; ref1=ref1->next, ref2=ref2->next )
	ref2->checked = false;
    if ( ref1!=NULL || ref2!=NULL )
return( false );

    for ( ref1=sc1->layers[ly_fore].refs; ref1!=NULL ; ref1=ref1->next ) {
	for ( ref2=sc2->layers[ly_fore].refs; ref2!=NULL ; ref2=ref2->next ) {
	    if ( ref2->sc->orig_pos==ref1->sc->orig_pos && !ref2->checked )
	break;
	}
	if ( ref2==NULL )
return( false );
	ref2->checked = true;
    }

return( true );
}

static int RefTransformsMatch(SplineChar *sc1, SplineChar *sc2) {
    /* Apple only provides a means to change the translation of a reference */
    /*  so if rotation, skewing, scaling, etc. differ then we can't deal with */
    /*  it. */
    RefChar *r1 = sc1->layers[ly_fore].refs;
    RefChar *r2 = sc2->layers[ly_fore].refs;

    while ( r1!=NULL && r2!=NULL ) {
	if ( r1->transform[0]!=r2->transform[0] ||
		r1->transform[1]!=r2->transform[1] ||
		r1->transform[2]!=r2->transform[2] ||
		r1->transform[3]!=r2->transform[3] )
return( false );
	r1 = r1->next;
	r2 = r2->next;
    }
return( true );
}

static int HintsMatch(StemInfo *h1,StemInfo *h2) {
    while ( h1!=NULL && h2!=NULL ) {
	h1 = h1->next;
	h2 = h2->next;
    }
return ( h1==NULL && h2==NULL );
}

static int KernsMatch(SplineChar *sc1, SplineChar *sc2) {
    /* I don't require the kern list to be ordered */
    /* Only interested in kerns that go into afm files (ie. no kernclasses) */
    KernPair *k1, *k2;

    for ( k1=sc1->kerns, k2=sc2->kerns; k1!=NULL && k2!=NULL; k1=k1->next, k2=k2->next )
	k2->kcid = false;
    if ( k1!=NULL || k2!=NULL )
return( false );

    for ( k1=sc1->kerns; k1!=NULL ; k1=k1->next ) {
	for ( k2=sc2->kerns; k2!=NULL ; k2=k2->next ) {
	    if ( k2->sc->orig_pos==k1->sc->orig_pos && !k2->kcid )
	break;
	}
	if ( k2==NULL )
return( false );
	k2->kcid = true;
    }

return( true );
}

static int ArrayCount(char *val) {
    char *end;
    int cnt;

    if ( val==NULL )
return( 0 );
    while ( *val==' ' ) ++val;
    if ( *val=='[' ) ++val;
    cnt=0;
    while ( *val ) {
	strtod(val,&end);
	if ( val==end )
    break;
	++cnt;
	val = end;
    }
return( cnt );
}

int MMValid(MMSet *mm,int complain) {
    int i, j;
    SplineFont *sf;
    static char *arrnames[] = { "BlueValues", "OtherBlues", "FamilyBlues", "FamilyOtherBlues", "StdHW", "StdVW", "StemSnapH", "StemSnapV", NULL };

    if ( mm==NULL )
return( false );

    for ( i=0; i<mm->instance_count; ++i )
	if ( mm->instances[i]->layers[ly_fore].order2 != mm->apple ) {
	    if ( complain ) {
		if ( mm->apple )
		    ff_post_error(_("Bad Multiple Master Font"),_("The font %.30s contains cubic splines. It must be converted to quadratic splines before it can be used in an apple distortable font"),
			    mm->instances[i]->fontname);
		else
		    ff_post_error(_("Bad Multiple Master Font"),_("The font %.30s contains quadratic splines. It must be converted to cubic splines before it can be used in a multiple master"),
			    mm->instances[i]->fontname);
	    }
return( false );
	}

    sf = mm->apple ? mm->normal : mm->instances[0];

    if ( !mm->apple && PSDictHasEntry(sf->private,"ForceBold")!=NULL &&
	    PSDictHasEntry(mm->normal->private,"ForceBoldThreshold")==NULL) {
	if ( complain )
	    ff_post_error(_("Bad Multiple Master Font"),_("There is no ForceBoldThreshold entry in the weighted font, but there is a ForceBold entry in font %30s"),
		    sf->fontname);
return( false );
    }

    for ( j=mm->apple ? 0 : 1; j<mm->instance_count; ++j ) {
	if ( sf->glyphcnt!=mm->instances[j]->glyphcnt ) {
	    if ( complain )
		ff_post_error(_("Bad Multiple Master Font"),_("The fonts %1$.30s and %2$.30s have a different number of glyphs or different encodings"),
			sf->fontname, mm->instances[j]->fontname);
return( false );
	} else if ( sf->layers[ly_fore].order2!=mm->instances[j]->layers[ly_fore].order2 ) {
	    if ( complain )
		ff_post_error(_("Bad Multiple Master Font"),_("The fonts %1$.30s and %2$.30s use different types of splines (one quadratic, one cubic)"),
			sf->fontname, mm->instances[j]->fontname);
return( false );
	}
	if ( !mm->apple ) {
	    if ( PSDictHasEntry(mm->instances[j]->private,"ForceBold")!=NULL &&
		    PSDictHasEntry(mm->normal->private,"ForceBoldThreshold")==NULL) {
		if ( complain )
		    ff_post_error(_("Bad Multiple Master Font"),_("There is no ForceBoldThreshold entry in the weighted font, but there is a ForceBold entry in font %30s"),
			    mm->instances[j]->fontname);
return( false );
	    }
	    for ( i=0; arrnames[i]!=NULL; ++i ) {
		if ( ArrayCount(PSDictHasEntry(mm->instances[j]->private,arrnames[i]))!=
				ArrayCount(PSDictHasEntry(sf->private,arrnames[i])) ) {
		    if ( complain )
			ff_post_error(_("Bad Multiple Master Font"),_("The entry \"%1$.20s\" is not present in the private dictionary of both %2$.30s and %3$.30s"),
				arrnames[i], sf->fontname, mm->instances[j]->fontname);
return( false );
		}
	    }
	}
    }

    for ( i=0; i<sf->glyphcnt; ++i ) {
	for ( j=mm->apple?0:1; j<mm->instance_count; ++j ) {
	    if ( SCWorthOutputting(sf->glyphs[i])!=SCWorthOutputting(mm->instances[j]->glyphs[i]) ) {
		if ( complain ) {
		    FVChangeGID( sf->fv,i);
		    if ( SCWorthOutputting(sf->glyphs[i]) )
			ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s is defined in font %2$.30s but not in %3$.30s"),
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    else
			ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s is defined in font %2$.30s but not in %3$.30s"),
				mm->instances[j]->glyphs[i]->name, mm->instances[j]->fontname,sf->fontname);
		}
return( false );
	    }
	}
	if ( SCWorthOutputting(sf->glyphs[i]) ) {
	    if ( mm->apple && sf->glyphs[i]->layers[ly_fore].refs!=NULL && sf->glyphs[i]->layers[ly_fore].splines!=NULL ) {
		if ( complain ) {
		    FVChangeGID( sf->fv,i);
		    ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in %2$.30s has both references and contours. This is not supported in a font with variations"),
			    sf->glyphs[i]->name,sf->fontname);
		}
return( false );
	    }
	    for ( j=mm->apple?0:1; j<mm->instance_count; ++j ) {
		if ( mm->apple && mm->instances[j]->glyphs[i]->layers[ly_fore].refs!=NULL &&
			mm->instances[j]->glyphs[i]->layers[ly_fore].splines!=NULL ) {
		    if ( complain ) {
			FVChangeGID( sf->fv,i);
			ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in %2$.30s has both references and contours. This is not supported in a font with variations"),
				sf->glyphs[i]->name,mm->instances[j]->fontname);
		    }
return( false );
		}
		if ( ContourCount(sf->glyphs[i])!=ContourCount(mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeGID( sf->fv,i);
			ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s has a different number of contours in font %2$.30s than in %3$.30s"),
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !mm->apple && !ContourPtMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeGID( sf->fv,i);
			ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different number of points (or control points) on its contours than in %3$.30s"),
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !ContourDirMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeGID( sf->fv,i);
			ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has contours running in a different direction than in %3$.30s"),
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !RefMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeGID( sf->fv,i);
			ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different number of references than in %3$.30s"),
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( mm->apple && !RefTransformsMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeGID( sf->fv,i);
			ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has references with different scaling or rotation (etc.) than in %3$.30s"),
				sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !mm->apple && !KernsMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
		    if ( complain ) {
			FVChangeGID( sf->fv,i);
			ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different set of kern pairs than in %3$.30s"),
				"vertical", sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		}
	    }
	    if ( mm->apple && !ContourPtNumMatch(mm,i)) {
		if ( complain ) {
		    FVChangeGID( sf->fv,i);
		    ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s has a different numbering of points (and control points) on its contours than in the various instances of the font"),
			    sf->glyphs[i]->name);
		}
return( false );
	    }
	    if ( !mm->apple ) {
		for ( j=1; j<mm->instance_count; ++j ) {
		    if ( !HintsMatch(sf->glyphs[i]->hstem,mm->instances[j]->glyphs[i]->hstem)) {
			if ( complain ) {
			    FVChangeGID( sf->fv,i);
			    ff_post_error(_("Bad Multiple Master Font"),_("The %1$s hints in glyph \"%2$.30s\" in font %3$.30s do not match those in %4$.30s (different number or different overlap criteria)"),
				    "horizontal", sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
			}
return( false );
		    } else if ( !HintsMatch(sf->glyphs[i]->vstem,mm->instances[j]->glyphs[i]->vstem)) {
			if ( complain ) {
			    FVChangeGID( sf->fv,i);
			    ff_post_error(_("Bad Multiple Master Font"),_("The %1$s hints in glyph \"%2$.30s\" in font %3$.30s do not match those in %4$.30s (different number or different overlap criteria)"),
				    "vertical", sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
			}
return( false );
		    }
		}
		for ( j=1; j<mm->instance_count; ++j ) {
		    if ( !ContourHintMaskMatch(sf->glyphs[i],mm->instances[j]->glyphs[i])) {
			if ( complain ) {
			    FVChangeGID( sf->fv,i);
			    ff_post_error(_("Bad Multiple Master Font"),_("The glyph %1$.30s in font %2$.30s has a different hint mask on its contours than in %3$.30s"),
				    sf->glyphs[i]->name,sf->fontname, mm->instances[j]->fontname);
			}
return( false );
		    }
		}
	    }
	}
    }
    if ( mm->apple ) {
	struct ttf_table *cvt;
	for ( cvt = mm->normal->ttf_tables; cvt!=NULL && cvt->tag!=CHR('c','v','t',' '); cvt=cvt->next );
	if ( cvt==NULL ) {
	    for ( j=0; j<mm->instance_count; ++j ) {
		if ( mm->instances[j]->ttf_tables!=NULL ) {
		    if ( complain )
			ff_post_error(_("Bad Multiple Master Font"),_("The default font does not have a 'cvt ' table, but the instance %.30s does"),
				mm->instances[j]->fontname);
return( false );
		}
	    }
	} else {
	    /* Not all instances are required to have cvts, but any that do */
	    /*  must be the same size */
	    for ( j=0; j<mm->instance_count; ++j ) {
		if ( mm->instances[j]->ttf_tables!=NULL &&
			(mm->instances[j]->ttf_tables->next!=NULL ||
			 mm->instances[j]->ttf_tables->tag!=CHR('c','v','t',' '))) {
		    if ( complain )
			ff_post_error(_("Bad Multiple Master Font"),_("Instance fonts may only contain a 'cvt ' table, but %.30s has some other truetype table as well"),
				mm->instances[j]->fontname);
return( false );
		}
		if ( mm->instances[j]->ttf_tables!=NULL &&
			mm->instances[j]->ttf_tables->len!=cvt->len ) {
		    if ( complain )
			ff_post_error(_("Bad Multiple Master Font"),_("The 'cvt ' table in instance %.30s is a different size from that in the default font"),
				mm->instances[j]->fontname);
return( false );
		}
	    }
	}
    }

    if ( complain )
	ff_post_notice(_("OK"),_("No problems detected"));
return( true );
}
