/* Copyright (C) 2000-2004 by George Williams */
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
#include "pfaeditui.h"
#include <ustring.h>
#include <math.h>

static char *MMAxisAbrev(char *axis_name) {
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

static double MMAxisUnmap(MMSet *mm,int axis,double ncv) {
    struct axismap *axismap = &mm->axismaps[axis];
    int j;

    if ( ncv<=axismap->blends[0] )
return(axismap->designs[0]);

    for ( j=1; j<axismap->points; ++j ) {
	if ( ncv<=axismap->blends[j]) {
	    double t = (ncv-axismap->blends[j-1])/(axismap->blends[j]-axismap->blends[j-1]);
return( axismap->designs[j-1]+ t*(axismap->designs[j]-axismap->designs[j-1]) );
	}
    }

return(axismap->designs[axismap->points-1]);
}

char *MMMakeMasterFontname(MMSet *mm,int ipos,char **fullname) {
    char *pt, *pt2;
    char *ret = galloc(strlen(mm->normal->familyname)+ mm->axis_count*15 + 1);
    int i;

    pt = ret;
    strcpy(pt,mm->normal->familyname);
    pt += strlen(pt);
    *pt++ = '_';
    for ( i=0; i<mm->axis_count; ++i ) {
	sprintf( pt, " %d%s", (int) rint(MMAxisUnmap(mm,i,mm->positions[ipos*mm->axis_count+i])),
		MMAxisAbrev(mm->axes[i]));
	pt += strlen(pt);
    }
    if ( pt>ret && pt[-1]==' ' )
	--pt;
    *pt = '\0';
    *fullname = ret;

    ret = copy(ret);
    for ( pt=pt2=ret; *pt!='\0'; ++pt )
	if ( *pt!=' ' )
	    *pt2++ = *pt;
    *pt2 = '\0';
return( ret );
}

char *MMGuessWeight(MMSet *mm,int ipos,char *def) {
    int i;
    char *ret;
    real design;

    for ( i=0; i<mm->axis_count; ++i ) {
	if ( strcmp(mm->axes[i],"Weight")==0 )
    break;
    }
    if ( i==mm->axis_count )
return( def );
    design = mm->positions[ipos*mm->axis_count + i];
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

    pt = ret = galloc(len+4);
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
	int sli,KernPair *oldkp) {
    MMSet *mm = sf->mm;
    KernPair *kp;
    int i;
    /* If the user creates a kern pair in one font of a multiple master set */
    /*  then we should create the same kern pair in all the others. Similarly */
    /*  if s/he modifies a kern pair in the weighted version then apply that */
    /*  mod to all the others */

    if ( mm==NULL )
return;
    if ( sf==mm->normal || kp==NULL ) {
	for ( i= -1; i<mm->instance_count; ++i ) {
	    SplineFont *cur = i==-1 ? mm->normal : mm->instances[i];
	    SplineChar *psc, *ssc;
	    if ( cur==sf )	/* Done in caller */
	continue;
	    psc = SFMakeChar(cur,first->enc);
	    ssc = SFMakeChar(cur,second->enc);
	    for ( kp = psc->kerns; kp!=NULL && kp->sc!=ssc; kp = kp->next );
	    /* No mm support for vertical kerns */
	    if ( kp==NULL ) {
		kp = chunkalloc(sizeof(KernPair));
		kp->sc = ssc;
		kp->next = psc->kerns;
		psc->kerns = kp;
	    }
	    kp->off += diff;
	    if ( sli==-1 )
		sli = SFAddScriptLangIndex(cur,
			    SCScriptFromUnicode(psc),DEFAULT_LANG);
	    kp->sli = sli;
	}
    }
}

static int ContourCount(SplineChar *sc) {
    SplineSet *spl;
    int i;

    for ( spl=sc->splines, i=0; spl!=NULL; spl=spl->next, ++i );
return( i );
}

static int ContourPtMatch(SplineChar *sc1, SplineChar *sc2) {
    SplineSet *spl1, *spl2;
    SplinePoint *sp1, *sp2;

    for ( spl1=sc1->splines, spl2=sc2->splines; spl1!=NULL && spl2!=NULL; spl1=spl1->next, spl2=spl2->next ) {
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

    for ( spl1=sc1->splines, spl2=sc2->splines; spl1!=NULL && spl2!=NULL; spl1=spl1->next, spl2=spl2->next ) {
	if ( SplinePointListIsClockwise(spl1)!=SplinePointListIsClockwise(spl2) )
return( false );
    }
return( true );
}

static int ContourHintMaskMatch(SplineChar *sc1, SplineChar *sc2) {
    SplineSet *spl1, *spl2;
    SplinePoint *sp1, *sp2;

    for ( spl1=sc1->splines, spl2=sc2->splines; spl1!=NULL && spl2!=NULL; spl1=spl1->next, spl2=spl2->next ) {
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

    for ( ref1=sc1->refs, ref2=sc2->refs; ref1!=NULL && ref2!=NULL; ref1=ref1->next, ref2=ref2->next )
	ref2->checked = false;
    if ( ref1!=NULL || ref2!=NULL )
return( false );

    for ( ref1=sc1->refs; ref1!=NULL ; ref1=ref1->next ) {
	for ( ref2=sc2->refs; ref2!=NULL ; ref2=ref2->next ) {
	    if ( ref2->local_enc==ref1->local_enc && !ref2->checked )
	break;
	}
	if ( ref2==NULL )
return( false );
	ref2->checked = true;
    }

return( true );
}

SplineChar *SCMostConflictsMM(MMSet *mm,int enc, int ishor, int *index) {
    SplineChar *most = NULL, *sc;
    int cnt, max = -1, i, in= -1;
    StemInfo *h;

    for ( i=0 ; i<mm->instance_count; ++i ) {
	sc = mm->instances[i]->chars[enc];
	h = ishor ? sc->hstem : sc->vstem;
	cnt = 0;
	while ( h!=NULL ) {
	    if ( h->hasconflicts ) ++cnt;
	    h = h->next;
	}
	if ( cnt>max ) {
	    max = cnt;
	    most = sc;
	    in = i;
	}
    }
    if ( index!=NULL ) *index = in;
return( most );
}

static int HintsMatch(StemInfo *h1,StemInfo *h2) {
    while ( h1!=NULL && h2!=NULL ) {
#if 0		/* Nope. May conflict in one instance and not in another */
	if ( h1->hasconflicts != h2->hasconflicts )
return( false );
#endif
	h1 = h1->next;
	h2 = h2->next;
    }
    if ( h1!=NULL || h2!=NULL )
return( false );

return( true );
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
	    if ( k2->sc->enc==k1->sc->enc && !k2->kcid )
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
	if ( mm->instances[i]->order2 ) {
	    if ( complain )
		GWidgetErrorR(_STR_BadMM,_STR_MMOrder2,
			sf->fontname);
return( false );
	}

    sf = mm->instances[0];

    if ( PSDictHasEntry(sf->private,"ForceBold")!=NULL &&
	    PSDictHasEntry(mm->normal->private,"ForceBoldThreshold")==NULL) {
	if ( complain )
	    GWidgetErrorR(_STR_BadMM,_STR_MMNeedsBoldThresh,
		    sf->fontname);
return( false );
    }

    for ( j=1; j<mm->instance_count; ++j ) {
	if ( sf->charcnt!=mm->instances[j]->charcnt ) {
	    if ( complain )
		GWidgetErrorR(_STR_BadMM,_STR_MMDifferentNumChars,
			sf->fontname, mm->instances[j]->fontname);
return( false );
	} else if ( sf->order2!=mm->instances[j]->order2 ) {
	    if ( complain )
		GWidgetErrorR(_STR_BadMM,_STR_MMDifferentOrder,
			sf->fontname, mm->instances[j]->fontname);
return( false );
	}
	if ( PSDictHasEntry(mm->instances[j]->private,"ForceBold")!=NULL &&
		PSDictHasEntry(mm->normal->private,"ForceBoldThreshold")==NULL) {
	    if ( complain )
		GWidgetErrorR(_STR_BadMM,_STR_MMNeedsBoldThresh,
			mm->instances[j]->fontname);
return( false );
	}
	for ( i=0; arrnames[i]!=NULL; ++i ) {
	    if ( ArrayCount(PSDictHasEntry(mm->instances[j]->private,arrnames[i]))!=
			    ArrayCount(PSDictHasEntry(sf->private,arrnames[i])) ) {
		if ( complain )
		    GWidgetErrorR(_STR_BadMM,_STR_MMPrivateMismatch,
			    arrnames[i], sf->fontname, mm->instances[j]->fontname);
return( false );
	    }
	}
    }

    for ( i=0; i<sf->charcnt; ++i ) {
	for ( j=1; j<mm->instance_count; ++j ) {
	    if ( SCWorthOutputting(sf->chars[i])!=SCWorthOutputting(mm->instances[j]->chars[i]) ) {
		if ( complain ) {
		    FVChangeChar(sf->fv,i);
		    if ( SCWorthOutputting(sf->chars[i]) )
			GWidgetErrorR(_STR_BadMM,_STR_MMUndefChar,
				sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    else
			GWidgetErrorR(_STR_BadMM,_STR_MMUndefChar,
				mm->instances[j]->chars[i]->name, mm->instances[j]->fontname,sf->fontname);
		}
return( false );
	    }
	}
	if ( SCWorthOutputting(sf->chars[i]) ) {
	    for ( j=1; j<mm->instance_count; ++j ) {
		if ( ContourCount(sf->chars[i])!=ContourCount(mm->instances[j]->chars[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMWrongContourCount,
				sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !ContourPtMatch(sf->chars[i],mm->instances[j]->chars[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchContoursPt,
				sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !ContourDirMatch(sf->chars[i],mm->instances[j]->chars[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchContoursDir,
				sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !RefMatch(sf->chars[i],mm->instances[j]->chars[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchRefs,
				sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !KernsMatch(sf->chars[i],mm->instances[j]->chars[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchKerns,
				"vertical", sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		}
	    }
	    for ( j=1; j<mm->instance_count; ++j ) {
		if ( !HintsMatch(sf->chars[i]->hstem,mm->instances[j]->chars[i]->hstem)) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchHints,
				"horizontal", sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !HintsMatch(sf->chars[i]->vstem,mm->instances[j]->chars[i]->vstem)) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchHints,
				"vertical", sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		}
	    }
	    for ( j=1; j<mm->instance_count; ++j ) {
		if ( !ContourHintMaskMatch(sf->chars[i],mm->instances[j]->chars[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchHintMask,
				sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		}
	    }
	}
    }
    if ( complain )
	GWidgetPostNoticeR(_STR_OK,_STR_NoProblems);
return( true );
}

