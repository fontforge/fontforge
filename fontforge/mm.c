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
#include <gkeysym.h>
#include <locale.h>
#include <utype.h>
#include "ttf.h"

/* As far as I can tell, the CDV in AdobeSansMM is half gibberish */
/* This is disturbing */
/* But at least the CDV in Type1_supp.pdf for Myriad appears correct */
static char *standard_cdvs[5] = {
/* 0 axes? Impossible */
    "{}",
/* 1 axis */
    "{\n"
    "  1 1 index sub 2 1 roll\n"
    "  0 index 2 1 roll\n"
    "  pop\n"
    "}",
/* 2 axes */
    "{\n"
    "  1 2 index sub 1 2 index sub mul 3 1 roll\n"
    "  1 index 1 2 index sub mul 3 1 roll\n"
    "  1 2 index sub 1 index mul 3 1 roll\n"
    "  1 index 1 index mul 3 1 roll\n"
    "  pop pop\n"
    "}",
/* 3 axes */
    "{\n"
    "  1 3 index sub 1 3 index sub mul 1 3 index sub mul 4 1 roll\n"
    "  2 index 1 3 index sub mul 1 3 index sub mul 4 1 roll\n"
    "  1 3 index sub 2 index mul 1 3 index sub mul 4 1 roll\n"
    "  2 index 2 index mul 1 3 index sub mul 4 1 roll\n"
    "  1 3 index sub 1 3 index sub mul 2 index mul 4 1 roll\n"
    "  2 index 1 3 index sub mul 2 index mul 4 1 roll\n"
    "  1 3 index sub 2 index mul 2 index mul 4 1 roll\n"
    "  2 index 2 index mul 2 index mul 4 1 roll\n"
    "  pop pop pop\n"
    "}",
/* 4 axes */
/* This requires too big a string. We must build it at runtime */
    NULL
};
static char *cdv_4axis[3] = {
    "{\n"
    "  1 4 index sub 1 4 index sub mul 1 4 index sub 1 4 index sub mul 5 1 roll\n"
    "  3 index 1 4 index sub mul 1 4 index sub mul 1 4 index sub mul 5 1 roll\n"
    "  1 4 index sub 3 index mul 1 4 index sub mul 1 4 index sub mul 5 1 roll\n"
    "  3 index 3 index mul 1 4 index sub mul 1 4 index sub mul 5 1 roll\n"
    "  1 4 index sub 1 4 index sub mul 3 index mul 1 4 index sub mul 5 1 roll\n"
    "  3 index 1 4 index sub mul 3 index mul 1 4 index sub mul 5 1 roll\n",
    "  1 4 index sub 3 index mul 3 index mul 1 4 index sub mul 5 1 roll\n"
    "  3 index 3 index mul 3 index mul 1 4 index sub mul 5 1 roll\n"
    "  1 4 index sub 1 4 index sub mul 1 4 index sub 3 index mul 5 1 roll\n"
    "  3 index 1 4 index sub mul 1 4 index sub mul 3 index mul 5 1 roll\n"
    "  1 4 index sub 3 index mul 1 4 index sub mul 3 index mul 5 1 roll\n",
    "  3 index 3 index mul 1 4 index sub mul 3 index mul 5 1 roll\n"
    "  1 4 index sub 1 4 index sub mul 3 index mul 3 index mul 5 1 roll\n"
    "  3 index 1 4 index sub mul 3 index mul 3 index mul 5 1 roll\n"
    "  1 4 index sub 3 index mul 3 index mul 3 index mul 5 1 roll\n"
    "  3 index 3 index mul 3 index mul 3 index mul 5 1 roll\n"
    "  pop pop pop pop\n"
    "}"
};

char *MMAxisAbrev(char *axis_name) {
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

#include <chardata.h>
static char *ASCIIName(unichar_t *str) {
    char *pt, *ret;
    const unichar_t *upt;

    pt = ret = galloc(2*u_strlen(str)+1);
    while ( *str ) {
	if ( *str=='/' || *str=='[' || *str==']' || *str=='{' || *str=='}' ||
		*str=='(' || *str==')' || *pt<=' ' )
	    *pt++ = ' ';
	else if ( *str<0x7f )
	    *pt++ = *str;
	else if ( *str==(uint8) 'Æ' ) {
	    *pt++ = 'A'; *pt++ = 'E';
	} else if ( *str==(uint8) 'æ' ) {
	    *pt++ = 'a'; *pt++ = 'e';
	} else if ( *str==0152 ) {
	    *pt++ = 'O'; *pt++ = 'E';
	} else if ( *str==0153 ) {
	    *pt++ = 'o'; *pt++ = 'e';
	} else if ( unicode_alternates[*str>>8]!=NULL &&
		(upt = unicode_alternates[*str>>8][*str&0xff])!=NULL &&
		((*upt>='A' && *upt<='Z') || (*upt>='a' && *upt<='z')))
	    *pt++ = *upt;
	else {
	    free(ret);
return( NULL );
	}
	++str;
    }
    *pt = '\0';
return( ret );
}

char *MMMakeMasterFontname(MMSet *mm,int ipos,char **fullname) {
    char *pt, *pt2, *hyphen=NULL;
    char *ret = NULL;
    int i,j;

    if ( mm->apple ) {
	for ( i=0; i<mm->named_instance_count; ++i ) {
	    for ( j=0; j<mm->axis_count; ++j ) {
		if (( mm->positions[ipos*mm->axis_count+j] == -1 &&
			RealApprox(mm->named_instances[i].coords[j],mm->axismaps[j].min) ) ||
		    ( mm->positions[ipos*mm->axis_count+j] ==  0 &&
			RealApprox(mm->named_instances[i].coords[j],mm->axismaps[j].def) ) ||
		    ( mm->positions[ipos*mm->axis_count+j] ==  1 &&
			RealApprox(mm->named_instances[i].coords[j],mm->axismaps[j].max) ))
		    /* A match so far */;
		else
	    break;
	    }
	    if ( j==mm->axis_count )
	break;
	}
	if ( i!=mm->named_instance_count ) {
	    unichar_t *styles = PickNameFromMacName(mm->named_instances[i].names);
	    char *cstyles = ASCIIName(styles);
	    free(styles);
	    if ( cstyles==NULL ) {
		styles = FindEnglishNameInMacName(mm->named_instances[i].names);
		cstyles = ASCIIName(styles);
		free(styles);
	    }
	    if ( cstyles!=NULL ) {
		ret = galloc(strlen(mm->normal->familyname)+ strlen(cstyles)+3 );
		strcpy(ret,mm->normal->familyname);
		hyphen = ret+strlen(ret);
		strcpy(hyphen," ");
		strcpy(hyphen+1,cstyles);
		free(cstyles);
	    }
	}
    }

    if ( ret==NULL ) {
	pt = ret = galloc(strlen(mm->normal->familyname)+ mm->axis_count*15 + 1);
	strcpy(pt,mm->normal->familyname);
	pt += strlen(pt);
	*pt++ = '_';
	for ( i=0; i<mm->axis_count; ++i ) {
	    if ( !mm->apple )
		sprintf( pt, " %d%s", (int) rint(MMAxisUnmap(mm,i,mm->positions[ipos*mm->axis_count+i])),
			MMAxisAbrev(mm->axes[i]));
	    else
		sprintf( pt, " %.1f%s", MMAxisUnmap(mm,i,mm->positions[ipos*mm->axis_count+i]),
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
	    if ( ref2->sc->enc==ref1->sc->enc && !ref2->checked )
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
	if ( mm->instances[i]->order2 != mm->apple ) {
	    if ( complain ) {
		if ( mm->apple )
		    GWidgetErrorR(_STR_BadMM,_STR_MMOrder3,
			    mm->instances[i]->fontname);
		else
		    GWidgetErrorR(_STR_BadMM,_STR_MMOrder2,
			    mm->instances[i]->fontname);
	    }
return( false );
	}

    sf = mm->apple ? mm->normal : mm->instances[0];

    if ( !mm->apple && PSDictHasEntry(sf->private,"ForceBold")!=NULL &&
	    PSDictHasEntry(mm->normal->private,"ForceBoldThreshold")==NULL) {
	if ( complain )
	    GWidgetErrorR(_STR_BadMM,_STR_MMNeedsBoldThresh,
		    sf->fontname);
return( false );
    }

    for ( j=mm->apple ? 0 : 1; j<mm->instance_count; ++j ) {
	if ( sf->charcnt!=mm->instances[j]->charcnt ||
		sf->encoding_name!=mm->instances[j]->encoding_name ) {
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
	if ( !mm->apple ) {
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
    }

    for ( i=0; i<sf->charcnt; ++i ) {
	for ( j=mm->apple?0:1; j<mm->instance_count; ++j ) {
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
	    if ( mm->apple && sf->chars[i]->layers[ly_fore].refs!=NULL && sf->chars[i]->layers[ly_fore].splines!=NULL ) {
		if ( complain ) {
		    FVChangeChar(sf->fv,i);
		    GWidgetErrorR(_STR_BadMM,_STR_MMBothRefSplines,
			    sf->chars[i]->name,sf->fontname);
		}
return( false );
	    }
	    for ( j=mm->apple?0:1; j<mm->instance_count; ++j ) {
		if ( mm->apple && mm->instances[j]->chars[i]->layers[ly_fore].refs!=NULL &&
			mm->instances[j]->chars[i]->layers[ly_fore].splines!=NULL ) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMBothRefSplines,
				sf->chars[i]->name,mm->instances[j]->fontname);
		    }
return( false );
		}
		if ( ContourCount(sf->chars[i])!=ContourCount(mm->instances[j]->chars[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMWrongContourCount,
				sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !mm->apple && !ContourPtMatch(sf->chars[i],mm->instances[j]->chars[i])) {
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
		} else if ( mm->apple && !RefTransformsMatch(sf->chars[i],mm->instances[j]->chars[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchRefTrans,
				sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		} else if ( !mm->apple && !KernsMatch(sf->chars[i],mm->instances[j]->chars[i])) {
		    if ( complain ) {
			FVChangeChar(sf->fv,i);
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchKerns,
				"vertical", sf->chars[i]->name,sf->fontname, mm->instances[j]->fontname);
		    }
return( false );
		}
	    }
	    if ( mm->apple && !ContourPtNumMatch(mm,i)) {
		if ( complain ) {
		    FVChangeChar(sf->fv,i);
		    GWidgetErrorR(_STR_BadMM,_STR_MMMismatchContoursPtNum,
			    sf->chars[i]->name);
		}
return( false );
	    }
	    if ( !mm->apple ) {
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
    }
    if ( mm->apple ) {
	struct ttf_table *cvt;
	for ( cvt = mm->normal->ttf_tables; cvt!=NULL && cvt->tag!=CHR('c','v','t',' '); cvt=cvt->next );
	if ( cvt==NULL ) {
	    for ( j=0; j<mm->instance_count; ++j ) {
		if ( mm->instances[j]->ttf_tables!=NULL ) {
		    if ( complain )
			GWidgetErrorR(_STR_BadMM,_STR_MMMissingCVT,
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
			GWidgetErrorR(_STR_BadMM,_STR_MMBadTable,
				mm->instances[j]->fontname);
return( false );
		}
		if ( mm->instances[j]->ttf_tables!=NULL &&
			mm->instances[j]->ttf_tables->len!=cvt->len ) {
		    if ( complain )
			GWidgetErrorR(_STR_BadMM,_STR_MMMismatchCVT,
				mm->instances[j]->fontname);
return( false );
		}
	    }
	}
    }

    if ( complain )
	GWidgetPostNoticeR(_STR_OK,_STR_NoProblems);
return( true );
}

static int _MMBlendChar(MMSet *mm, int enc) {
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
	if ( mm->instances[i]->order2 )
return( _STR_MMOrder2NoName );
	if ( enc>=mm->instances[i]->charcnt )
return( _STR_MMDifferentNumCharsNoName );
	if ( SCWorthOutputting(mm->instances[i]->chars[enc]) ) {
	    if ( worthit == -1 ) worthit = true;
	    else if ( worthit != true )
return( _STR_MMUndefCharNoName );
	} else {
	    if ( worthit == -1 ) worthit = false;
	    else if ( worthit != false )
return( _STR_MMUndefCharNoName );
	}
    }

    sc = mm->normal->chars[enc];
    if ( sc!=NULL ) {
	SCClearContents(sc);
	KernPairsFree(sc->kerns);
	sc->kerns = NULL;
	KernPairsFree(sc->vkerns);
	sc->vkerns = NULL;
    }

    if ( !worthit )
return( 0 );

    if ( sc==NULL )
	sc = SFMakeChar(mm->normal,enc);

	/* Blend references => blend transformation matrices */
    diff = false;
    any = false; all = true;
    for ( i=0; i<mm->instance_count; ++i ) {
	refs[i] = mm->instances[i]->chars[enc]->layers[ly_fore].refs;
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
	ref->sc = mm->normal->chars[refs[0]->sc->enc];
	for ( i=0; i<mm->instance_count; ++i ) {
	    if ( ref->sc->enc!=refs[i]->sc->enc )
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
return( _STR_MMDiffNumRefs );
    if ( diff )
return( _STR_MMDiffRefEncodings );

	/* Blend Width */
    width = 0;
    for ( i=0; i<mm->instance_count; ++i )
	width += mm->instances[i]->chars[enc]->width*mm->defweights[i];
    sc->width = width;
    width = 0;
    for ( i=0; i<mm->instance_count; ++i )
	width += mm->instances[i]->chars[enc]->vwidth*mm->defweights[i];
    sc->vwidth = width;

	/* Blend Splines */
    any = false; all = true;
    for ( i=0; i<mm->instance_count; ++i ) {
	spls[i] = mm->instances[i]->chars[enc]->layers[ly_fore].splines;
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
return( _STR_MMPointMismatch );
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
return( _STR_MMPointMismatch );
	}
	any = false; all = true;
	for ( i=0; i<mm->instance_count; ++i ) {
	    spls[i] = spls[i]->next;
	    if ( spls[i]!=NULL ) any = true;
	    else all = false;
	}
    }
    if ( any )
return( _STR_MMContourMismatch );

	/* Blend hints */
    for ( j=0; j<2; ++j ) {
	any = false; all = true;
	for ( i=0; i<mm->instance_count; ++i ) {
	    hs[i] = j ? mm->instances[i]->chars[enc]->hstem : mm->instances[i]->chars[enc]->vstem;
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
return( _STR_MMDiffNumHints );
    }

	/* Blend kernpairs */
    /* I'm not requiring ordered kerning pairs */
    /* I'm not bothering with vertical kerning */
    kp0 = mm->instances[0]->chars[enc]->kerns;
    kplast = NULL;
    while ( kp0!=NULL ) {
	int off = kp0->off*mm->defweights[0];
	for ( i=1; i<mm->instance_count; ++i ) {
	    for ( kptest=mm->instances[i]->chars[enc]->kerns; kptest!=NULL; kptest=kptest->next )
		if ( kptest->sc->enc==kp0->sc->enc )
	    break;
	    if ( kptest==NULL )
return( _STR_MMDiffKerns );
	    off += kptest->off*mm->defweights[i];
	}
	kp = chunkalloc(sizeof(KernPair));
	kp->sc = mm->normal->chars[kp0->sc->enc];
	kp->off = off;
	kp->sli = kp0->sli;
	kp->flags = kp0->flags;
	if ( kplast!=NULL )
	    kplast->next = kp;
	else
	    sc->kerns = kp;
	kplast = kp;
	kp0 = kp0->next;
    }
return( 0 );
}

int MMBlendChar(MMSet *mm, int enc) {
    int ret;
    RefChar *ref;

    if ( enc>=mm->normal->charcnt )
return( _STR_MMDifferentNumCharsNoName );
    ret = _MMBlendChar(mm,enc);
    if ( mm->normal->chars[enc]!=NULL ) {
	SplineChar *sc = mm->normal->chars[enc];
	for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	    SCReinstanciateRefChar(sc,ref);
	    SCMakeDependent(sc,ref->sc);
	}
    }
return(ret);
}

int MMReblend(FontView *fv, MMSet *mm) {
    int olderr, err, i, first = -1;
    SplineFont *sf = mm->instances[0];
    RefChar *ref;

    olderr = 0;
    for ( i=0; i<sf->charcnt; ++i ) {
	if ( i>=mm->normal->charcnt )
    break;
	err = MMBlendChar(mm,i);
	if ( mm->normal->chars[i]!=NULL )
	    _SCCharChangedUpdate(mm->normal->chars[i],-1);
	if ( err==0 )
    continue;
	if ( olderr==0 ) {
	    if ( fv!=NULL )
		FVDeselectAll(fv);
	    first = i;
	}
	if ( olderr==0 || olderr == err )
	    olderr = err;
	else
	    olderr = -1;
	if ( fv!=NULL )
	    fv->selected[i] = true;
    }

    sf = mm->normal;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( ref=sf->chars[i]->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	    SCReinstanciateRefChar(sf->chars[i],ref);
	    SCMakeDependent(sf->chars[i],ref->sc);
	}
    }

    if ( olderr == 0 )	/* No Errors */
return( true );

    if ( fv!=NULL ) {
	FVScrollToChar(fv,first);
	if ( olderr==-1 )
	    GWidgetErrorR(_STR_BadMM,_STR_MMVariousErrors);
	else
	    GWidgetErrorR(_STR_BadMM,_STR_MMSelErr,GStringGetResource(olderr,NULL));
    }
return( false );
}

static int ExecConvertDesignVector(real *designs, int dcnt, char *ndv, char *cdv,
	real *stack) {
    char *temp, dv[101];
    int j, len, cnt;
    char *oldloc;

    /* PostScript parses things in "C" locale too */
    oldloc = setlocale(LC_NUMERIC,"C");
    len = 0;
    for ( j=0; j<dcnt; ++j ) {
	sprintf(dv+len, "%g ", designs[j]);
	len += strlen(dv+len);
    }
    setlocale(LC_NUMERIC,oldloc);

    temp = galloc(len+strlen(ndv)+strlen(cdv)+20);
    strcpy(temp,dv);
    /*strcpy(temp+len++," ");*/		/* dv always will end in a space */

    while ( isspace(*ndv)) ++ndv;
    if ( *ndv=='{' )
	++ndv;
    strcpy(temp+len,ndv);
    len += strlen(temp+len);
    while ( len>0 && (temp[len-1]==' '||temp[len-1]=='\n') ) --len;
    if ( len>0 && temp[len-1]=='}' ) --len;

    while ( isspace(*cdv)) ++cdv;
    if ( *cdv=='{' )
	++cdv;
    strcpy(temp+len,cdv);
    len += strlen(temp+len);
    while ( len>0 && (temp[len-1]==' '||temp[len-1]=='\n') ) --len;
    if ( len>0 && temp[len-1]=='}' ) --len;

    cnt = EvaluatePS(temp,stack,MmMax);
    free(temp);
return( cnt );
}

static int StandardPositions(MMSet *mm,int instance_count, int axis_count) {
    int i,j;
    for ( i=0; i<instance_count; ++i ) {
	for ( j=0; j<axis_count; ++j )
	    if ( mm->positions[i*mm->axis_count+j]!= ( (i&(1<<j)) ? 1 : 0 ))
return( false );
    }
return( true );
}

static int OrderedPositions(MMSet *mm,int instance_count) {
    /* For a 1 axis system, check that the positions are ordered */
    int i;

    if ( mm->positions[0]!=0 )				/* must start at 0 */
return( false );
    if ( mm->positions[(instance_count-1)*4]!=1 )	/* and end at 1 */
return( false );
    for ( i=1; i<mm->instance_count; ++i )
	if ( mm->positions[i*mm->axis_count]<=mm->positions[(i-1)*mm->axis_count] )
return( false );

return( true );
}

static void MMWeightsUnMap(real weights[MmMax], real axiscoords[4],
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

static unichar_t *MMDesignCoords(MMSet *mm) {
    char buffer[80], *pt;
    int i;
    real axiscoords[4];

    if ( mm->instance_count!=(1<<mm->axis_count) ||
	    !StandardPositions(mm,mm->instance_count,mm->axis_count))
return( uc_copy(""));
    MMWeightsUnMap(mm->defweights,axiscoords,mm->axis_count);
    pt = buffer;
    for ( i=0; i<mm->axis_count; ++i ) {
	sprintf( pt,"%g ", MMAxisUnmap(mm,i,axiscoords[i]));
	pt += strlen(pt);
    }
    pt[-1] = ' ';
return( uc_copy( buffer ));
}

struct mmcb {
    int done;
    GWindow gw;
    MMSet *mm;
    FontView *fv;
    int tonew;
};

#define CID_Explicit		6001
#define	CID_ByDesign		6002
#define CID_NewBlends		6003
#define CID_NewDesign		6004

static int MMCB_Changed(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	int explicitblends = GGadgetIsChecked(GWidgetGetControl(gw,CID_Explicit));
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NewBlends),explicitblends);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_NewDesign),!explicitblends);
    }
return( true );
}

static int GetWeights(GWindow gw, real blends[MmMax], MMSet *mm,
	int instance_count, int axis_count) {
    int explicitblends = GGadgetIsChecked(GWidgetGetControl(gw,CID_Explicit));
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,
	    explicitblends?CID_NewBlends:CID_NewDesign)), *upt;
    unichar_t *uend;
    int i;
    real sum;

    sum = 0;
    for ( i=0, upt = ret; i<instance_count && *upt; ++i ) {
	blends[i] = u_strtod(upt,&uend);
	sum += blends[i];
	if ( upt==uend )
    break;
	upt = uend;
	while ( *upt==',' || *upt==' ' ) ++upt;
    }
    if ( (explicitblends && i!=instance_count ) ||
	    (!explicitblends && i!=axis_count ) ||
	    *upt!='\0' ) {
	GWidgetErrorR(_STR_BadMMWeights,_STR_BadOrFewWeights);
return(false);
    }
    if ( explicitblends ) {
	if ( sum<.99 || sum>1.01 ) {
	    GWidgetErrorR(_STR_BadMMWeights,_STR_WeightsMustBe1);
return(false);
	}
    } else {
	i = ExecConvertDesignVector(blends, i, mm->ndv, mm->cdv,
		blends);
	if ( i!=instance_count ) {
	    GWidgetErrorR(_STR_BadMMWeights,_STR_BadNDVCDV);
return(false);
	}
    }
return( true );
}

static int MMCB_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct mmcb *mmcb = GDrawGetUserData(GGadgetGetWindow(g));
	real blends[MmMax];
	int i;

	if ( !GetWeights(mmcb->gw, blends, mmcb->mm, mmcb->mm->instance_count, mmcb->mm->axis_count))
return( true );

	for ( i=0; i<mmcb->mm->instance_count; ++i )
	    mmcb->mm->defweights[i] = blends[i];
	mmcb->mm->changed = true;
	MMReblend(mmcb->fv,mmcb->mm);
	mmcb->done = true;
    }
return( true );
}

static int MMCB_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct mmcb *mmcb = GDrawGetUserData(GGadgetGetWindow(g));
	mmcb->done = true;
    }
return( true );
}

static int mmcb_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct mmcb *mmcb = GDrawGetUserData(gw);
	mmcb->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("mmmenu.html");
return( true );
	}
return( false );
    }
return( true );
}

void MMChangeBlend(MMSet *mm,FontView *fv,int tonew) {
    char buffer[MmMax*20], *pt;
    unichar_t ubuf[MmMax*20];
    int i, k;
    struct mmcb mmcb;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12];
    GTextInfo label[12];
    unichar_t *utemp;

    if ( mm==NULL )
return;
    pt = buffer;
    for ( i=0; i<mm->instance_count; ++i ) {
	sprintf( pt, "%g, ", mm->defweights[i]);
	pt += strlen(pt);
    }
    if ( pt>buffer )
	pt[-2] = '\0';
    uc_strcpy(ubuf,buffer);

    memset(&mmcb,0,sizeof(mmcb));
    mmcb.mm = mm;
    mmcb.fv = fv;
    mmcb.tonew = tonew;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_ChangeMMBlend,NULL);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(270));
    pos.height = GDrawPointsToPixels(NULL,200);
    mmcb.gw = gw = GDrawCreateTopWindow(NULL,&pos,mmcb_e_h,&mmcb,&wattrs);

    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    k=0;
    label[k].text = (unichar_t *) _STR_ChangeBlend1;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 10;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _STR_ChangeBlend2;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _STR_ChangeBlend3;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _STR_ChangeBlend4;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _STR_ContribOfMaster;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    gcd[k].gd.cid = CID_Explicit;
    gcd[k].gd.handle_controlevent = MMCB_Changed;
    gcd[k++].creator = GRadioCreate;

    label[k].text = (unichar_t *) _STR_DesignAxisValues;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+45;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_ByDesign;
    gcd[k].gd.handle_controlevent = MMCB_Changed;
    gcd[k++].creator = GRadioCreate;

    label[k].text = ubuf;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y+18;
    gcd[k].gd.pos.width = 240;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_NewBlends;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = utemp = MMDesignCoords(mm);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y+18;
    gcd[k].gd.pos.width = 240;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_NewDesign;
    gcd[k++].creator = GTextFieldCreate;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _STR_OK;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = MMCB_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _STR_Cancel;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = MMCB_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[k].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    free(utemp);

    GDrawSetVisible(gw,true);

    while ( !mmcb.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

GTextInfo axiscounts[] = {
    { (unichar_t *) "1", NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "2", NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "3", NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "4", NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

/* These names are fixed by Adobe & Apple and are not subject to translation */
GTextInfo axistypes[] = {
    { (unichar_t *) "Weight",		NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "Width",		NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "OpticalSize",	NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "Slant",		NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

GTextInfo mastercounts[] = {
    { (unichar_t *) "1", NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "2", NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "3", NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "4", NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "5", NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "6", NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "16", NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

typedef struct mmw {
    GWindow gw;
    enum mmw_state { mmw_counts, mmw_axes, mmw_designs, mmw_funcs, mmw_others } state;
    GWindow subwins[mmw_others+1];
    MMSet *mm, *old;
    int isnew;
    int done;
    int old_axis_count;
    int axis_count, instance_count;	/* The data in mm are set to the max for each */
    int last_instance_count, last_axis_count, lastw_instance_count;
    struct axismap last_axismaps[4];
    int canceldrop, subheightdiff;
    int lcnt, lmax;
    SplineFont **loaded;
} MMW;

#define MMW_Width	340
#define MMW_Height	300

#define CID_OK		1001
#define CID_Prev	1002
#define CID_Next	1003
#define CID_Cancel	1004
#define CID_Group	1005

#define CID_AxisCount	2001
#define CID_MasterCount	2002
#define CID_Adobe	2003
#define CID_Apple	2004

#define CID_WhichAxis	3000
#define CID_AxisType	3001	/* +[0,3]*100 */
#define CID_AxisBegin	3002	/* +[0,3]*100 */
#define CID_AxisEnd	3003	/* +[0,3]*100 */
#define CID_IntermediateDesign		3004	/* +[0,3]*100 */
#define CID_IntermediateNormalized	3005	/* +[0,3]*100 */

#define CID_WhichDesign	4000
#define CID_DesignFonts	4001	/* +[0,16]*50 */
#define CID_AxisWeights	4002	/* +[0,16]*50 */

#define CID_NDV			5002
#define CID_CDV			5003

/* CID_Explicit-CID_NewDesign already defined */
#define CID_ForceBoldThreshold	6005
#define CID_FamilyName		6006

static int MM_TypeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
    }
return( true );
}

static void SetMasterToAxis(MMW *mmw, int initial) {
    int i, cnt, def;

    cnt = GGadgetGetFirstListSelectedItem(GWidgetGetControl(mmw->subwins[mmw_counts],CID_AxisCount))
	    +1;
    if ( cnt!=mmw->old_axis_count ) {
	GGadget *list = GWidgetGetControl(mmw->subwins[mmw_counts],CID_MasterCount);
	int32 len;
	GTextInfo **ti = GGadgetGetList(list,&len);
	if ( GGadgetIsChecked(GWidgetGetControl(mmw->subwins[mmw_counts],CID_Adobe)) ) {
	    for ( i=0; i<16; ++i )
		ti[i]->disabled = (i+1) < (1<<cnt);
	    def = (1<<cnt);
	} else {
	    for ( i=0; i<16; ++i )
		ti[i]->disabled = (i+1) < cnt;
	    def = 1;
	    for ( i=0; i<cnt; ++i )
		def *= 3;
	    --def;
	}
	if ( !initial )
	    GGadgetSelectOneListItem(list,def-1);
	mmw->old_axis_count = cnt;
    }
}

static int MMW_AxisCntChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	SetMasterToAxis(GDrawGetUserData(GGadgetGetWindow(g)),false);
    }
return( true );
}

static void MMUsurpNew(SplineFont *sf) {
    /* This is a font that wasn't in the original MMSet */
    /* We ARE going to use it in the final result */
    /* So if it is attached to a fontview, we must close that window and */
    /*  claim the splinefont for ourselves */
    FontView *fv, *nextfv;

    if ( sf->fv!=NULL ) {
	if ( sf->kcld!=NULL )
	    KCLD_End(sf->kcld);
	if ( sf->vkcld!=NULL )
	    KCLD_End(sf->vkcld);
	sf->kcld = sf->vkcld = NULL;

	for ( fv=sf->fv; fv!=NULL; fv=nextfv ) {
	    nextfv = fv->nextsame;
	    fv->nextsame = NULL;
	    _FVCloseWindows(fv);
	    fv->sf = NULL;
	    GDrawDestroyWindow(fv->gw);
	}
	sf->fv = NULL;
	SFClearAutoSave(sf);
    }
}

static void MMDetachNew(SplineFont *sf) {
    /* This is a font that wasn't in the original MMSet */
    /* We aren't going to use it in the final result */
    /* If it is attached to a fontview, then the fontview retains control */
    /* If not, then free it */
    if ( sf->fv==NULL )
	SplineFontFree(sf);
}

static void MMDetachOld(SplineFont *sf) {
    /* This is a font that was in the original MMSet */
    /* We aren't going to use it in the final result */
    /* So then free it */
    sf->mm = NULL;
    SplineFontFree(sf);
}

static void MMW_Close(MMW *mmw) {
    int i;
    for ( i=0; i<mmw->lcnt; ++i )
	MMDetachNew(mmw->loaded[i]);
    free(mmw->loaded);
    MMSetFreeContents(mmw->mm);
    chunkfree(mmw->mm,sizeof(MMSet));
    mmw->done = true;
}

static int MMW_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	MMW_Close(mmw);
    }
return( true );
}

static void MMW_SetState(MMW *mmw) {
    int i;

    GDrawSetVisible(mmw->subwins[mmw->state],true);
    for ( i=mmw_counts; i<=mmw_others; ++i )
	if ( i!=mmw->state )
	    GDrawSetVisible(mmw->subwins[i],false);

    GGadgetSetEnabled(GWidgetGetControl(mmw->gw,CID_Prev),mmw->state!=mmw_counts);
    GGadgetSetEnabled(GWidgetGetControl(mmw->gw,CID_Next),mmw->state!=mmw_others);
    GGadgetSetEnabled(GWidgetGetControl(mmw->gw,CID_OK),mmw->state==mmw_others);
}

static int ParseWeights(GWindow gw,int cid, int str_r,
	real *list, int expected, int tabset_cid, int aspect ) {
    int cnt=0;
    const unichar_t *ret, *pt; unichar_t *endpt;

    ret= _GGadgetGetTitle(GWidgetGetControl(gw,cid));

    for ( pt=ret; *pt==' '; ++pt );
    for ( ; *pt; ) {
	list[cnt++] = u_strtod(pt,&endpt);
	if ( pt==endpt || ( *endpt!='\0' && *endpt!=' ' )) {
	    if ( tabset_cid!=-1 )
		GTabSetSetSel(GWidgetGetControl(gw,tabset_cid),aspect);
	    GWidgetErrorR(_STR_BadAxis,_STR_BadNumberIn_s,
		    GStringGetResource(str_r,NULL));
return( 0 );
	}
	for ( pt = endpt; *pt==' '; ++pt );
    }
    if ( cnt!=expected && expected!=-1 ) {
	if ( tabset_cid!=-1 )
	    GTabSetSetSel(GWidgetGetControl(gw,tabset_cid),aspect);
	GWidgetErrorR(_STR_BadAxis,_STR_WrongNumberOfEntriesIn_s,
		GStringGetResource(str_r,NULL));
return( 0 );
    }

return( cnt );
}

static int ParseList(GWindow gw,int cid, int str_r, int *err, real start,
	real end, real **_list, int tabset_cid, int aspect ) {
    int i, cnt;
    const unichar_t *ret, *pt; unichar_t *endpt;
    real *list;

    *_list = NULL;

    ret= _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    for ( pt=ret; *pt==' '; ++pt );
    cnt = *pt=='\0'?0:1 ;
    for ( ; *pt; ++pt ) {
	if ( *pt==' ' ) ++cnt;
	while ( *pt==' ' ) ++pt;
    }
    if ( start!=end )
	cnt+=2;
    list = galloc(cnt*sizeof(real));
    if ( start==end )
	cnt = 0;
    else {
	list[0] = start;
	cnt = 1;
    }

    for ( pt=ret; *pt==' '; ++pt );
    for ( ; *pt; ) {
	list[cnt++] = u_strtod(pt,&endpt);
	if ( pt==endpt || ( *endpt!='\0' && *endpt!=' ' )) {
	    GTabSetSetSel(GWidgetGetControl(gw,tabset_cid),aspect);
	    free(list);
	    GWidgetErrorR(_STR_BadAxis,_STR_BadNumberIn_s,
		    GStringGetResource(str_r,NULL));
	    *err = true;
return( 0 );
	}
	for ( pt = endpt; *pt==' '; ++pt );
    }
    if ( start!=end )
	list[cnt++] = end;
    for ( i=1; i<cnt; ++i )
	if ( list[i-1]>list[i] ) {
	    GTabSetSetSel(GWidgetGetControl(gw,tabset_cid),aspect);
	    GWidgetErrorR(_STR_BadAxis,_STR_ListOutOfOrder,
		    GStringGetResource(str_r,NULL));
	    free(list);
	    *err = true;
return( 0 );
	}

    *_list = list;
return( cnt );
}

static char *_ChooseFonts(char *buffer, SplineFont **sfs, real *positions,
	int i, int cnt) {
    char *elsepart=NULL, *ret;
    int pos;
    int k;

    if ( i<cnt-2 )
	elsepart = _ChooseFonts(buffer,sfs,positions,i+1,cnt);

    pos = 0;
    if ( positions[i]!=0 ) {
	sprintf(buffer, "%g sub ", positions[i]);
	pos += strlen(buffer);
    }
    sprintf(buffer+pos, "%g div dup 1 sub exch ", positions[i+1]-positions[i]);
    pos += strlen( buffer+pos );
    for ( k=0; k<i; ++k ) {
	strcpy(buffer+pos, "0 ");
	pos += 2;
    }
    if ( i!=0 ) {
	sprintf(buffer+pos, "%d -2 roll ", i+2 );
	pos += strlen(buffer+pos);
    }
    for ( k=i+2; k<cnt; ++k ) {
	strcpy(buffer+pos, "0 ");
	pos += 2;
    }

    if ( elsepart==NULL )
return( copy(buffer));

    ret = galloc(strlen(buffer)+strlen(elsepart)+40);
    sprintf(ret,"dup %g le {%s} {%s} ifelse", positions[i+1], buffer, elsepart );
    free(elsepart);
return( ret );
}

static unichar_t *Figure1AxisCDV(MMW *mmw) {
    real positions[MmMax];
    SplineFont *sfs[MmMax];
    int i;
    char *temp;
    unichar_t *ret;
    char buffer[400];

    if ( mmw->axis_count!=1 )
return( uc_copy(""));
    if ( mmw->instance_count==2 )
return( uc_copy( standard_cdvs[1]));

    for ( i=0; i<mmw->instance_count; ++i ) {
	positions[i] = mmw->mm->positions[4*i];
	sfs[i] = mmw->mm->instances[i];
	if ( i>0 && positions[i-1]>=positions[i] )
return( uc_copy(""));
    }
    temp = _ChooseFonts(buffer,sfs,positions,0,mmw->instance_count);
    ret = uc_copy(temp);
    free(temp);
return( ret );
}

static char *_NormalizeAxis(char *buffer, struct axismap *axis, int i) {
    char *elsepart=NULL, *ret;
    int pos;

    if ( i<axis->points-2 )
	elsepart = _NormalizeAxis(buffer,axis,i+1);

    pos = 0;
    if ( axis->blends[i+1]==axis->blends[i] ) {
	sprintf( buffer, "%g ", axis->blends[i] );
	pos = strlen(buffer);
    } else {
	if ( axis->designs[i]!=0 ) {
	    sprintf(buffer, "%g sub ", axis->designs[i]);
	    pos += strlen(buffer);
	}
	sprintf(buffer+pos, "%g div ", (axis->designs[i+1]-axis->designs[i])/
		    (axis->blends[i+1]-axis->blends[i]));
	pos += strlen( buffer+pos );
	if ( axis->blends[i]!=0 ) {
	    sprintf(buffer+pos, "%g add ", axis->blends[i]);
	    pos += strlen(buffer+pos);
	}
    }

    if ( elsepart==NULL )
return( copy(buffer));

    ret = galloc(strlen(buffer)+strlen(elsepart)+40);
    sprintf(ret,"dup %g le {%s} {%s} ifelse", axis->designs[i+1], buffer, elsepart );
    free(elsepart);
return( ret );
}
    
static char *NormalizeAxis(char *header,struct axismap *axis) {
    char *ret;
    char buffer[200];

    ret = _NormalizeAxis(buffer,axis,0);
    if ( *header ) {
	char *temp;
	temp = galloc(strlen(header)+strlen(ret)+2);
	strcpy(temp,header);
	strcat(temp,ret);
	strcat(temp,"\n");
	free(ret);
	ret = temp;
    }
return( ret );
}

static int SameAxes(int cnt1,int cnt2,struct axismap *axismaps1,struct axismap *axismaps2) {
    int i,j;

    if ( cnt1!=cnt2 )
return( false );
    for ( i=0; i<cnt1; ++i ) {
	if ( axismaps1[i].points!=axismaps2[i].points )
return( false );
	for ( j=0; j<axismaps1[i].points; ++j ) {
	    if ( axismaps1[i].designs[j]>=axismaps2[i].designs[j]+.01 ||
		    axismaps1[i].designs[j]<=axismaps2[i].designs[j]-.01 )
return( false );
	    if ( axismaps1[i].blends[j]>=axismaps2[i].blends[j]+.001 ||
		    axismaps1[i].blends[j]<=axismaps2[i].blends[j]-.001 )
return( false );
	}
    }
return( true );
}

static void AxisDataCopyFree(struct axismap *into,struct axismap *from,int count) {
    int i;

    for ( i=0; i<4; ++i ) {
	free(into->blends); free(into->designs);
	into->blends = NULL; into->designs = NULL;
	into->points = 0;
    }
    for ( i=0; i<count; ++i ) {
	into[i].points = from[i].points;
	into[i].blends = galloc(into[i].points*sizeof(real));
	memcpy(into[i].blends,from[i].blends,into[i].points*sizeof(real));
	into[i].designs = galloc(into[i].points*sizeof(real));
	memcpy(into[i].designs,from[i].designs,into[i].points*sizeof(real));
    }
}

static int PositionsMatch(MMSet *old,MMSet *mm) {
    int i,j;

    for ( i=0; i<old->instance_count; ++i ) {
	for ( j=0; j<old->axis_count; ++j )
	    if ( old->positions[i*old->axis_count+j] != mm->positions[i*mm->axis_count+j] )
return( false );
    }
return( true );
}

static void MMW_FuncsValid(MMW *mmw) {
    unichar_t *ut;
    int pos, i;

    if ( !SameAxes(mmw->axis_count,mmw->last_axis_count,mmw->mm->axismaps,mmw->last_axismaps)) {
	if ( mmw->old!=NULL &&
		SameAxes(mmw->axis_count,mmw->old->axis_count,mmw->mm->axismaps,mmw->old->axismaps)) {
	    ut = uc_copy(mmw->old->ndv);
	} else {
	    char *header = mmw->axis_count==1 ?  "  " :
			    mmw->axis_count==2 ? "  exch " :
			    mmw->axis_count==3 ? "  3 -1 roll " :
						 "  4 -1 roll ";
	    char *lines[4];
	    for ( i=0; i<mmw->axis_count; ++i )
		lines[i] = NormalizeAxis(header,&mmw->mm->axismaps[i]);
	    pos = 0;
	    for ( i=0; i<mmw->axis_count; ++i )
		pos += strlen(lines[i]);
	    ut = galloc((pos+20)*sizeof(unichar_t));
	    uc_strcpy(ut,"{\n" ); pos = 2;
	    for ( i=0; i<mmw->axis_count; ++i ) {
		uc_strcpy(ut+pos,lines[i]);
		pos += strlen(lines[i]);
	    }
	    uc_strcpy(ut+pos,"}" );
	}
	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_NDV),
		ut);
	free(ut);
	AxisDataCopyFree(mmw->last_axismaps,mmw->mm->axismaps,mmw->axis_count);
	mmw->last_axis_count = mmw->axis_count;
    }
    if ( mmw->last_instance_count!=mmw->instance_count ) {
	if ( standard_cdvs[4]==NULL ) {
	    standard_cdvs[4] = galloc(strlen(cdv_4axis[0])+strlen(cdv_4axis[1])+
		    strlen(cdv_4axis[2])+2);
	    strcpy(standard_cdvs[4],cdv_4axis[0]);
	    strcat(standard_cdvs[4],cdv_4axis[1]);
	    strcat(standard_cdvs[4],cdv_4axis[2]);
	}
	if ( mmw->old!=NULL &&
		mmw->axis_count==mmw->old->axis_count &&
		mmw->instance_count==mmw->old->instance_count &&
		PositionsMatch(mmw->old,mmw->mm)) {
	    ut = uc_copy(mmw->old->cdv);
	} else if ( mmw->instance_count==(1<<mmw->axis_count) &&
		StandardPositions(mmw->mm,mmw->instance_count,mmw->axis_count)) {
	    ut = uc_copy(standard_cdvs[mmw->axis_count]);
	} else if ( mmw->axis_count==1 &&
		OrderedPositions(mmw->mm,mmw->instance_count)) {
	    ut = Figure1AxisCDV(mmw);
	} else {
	    ut = uc_copy("");
	}
	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_CDV),
		ut);
	free(ut);
    }
    mmw->last_instance_count = mmw->instance_count;
}

static void MMW_WeightsValid(MMW *mmw) {
    char *temp;
    unichar_t *ut, *utc;
    int pos, i;
    real axiscoords[4], weights[MmMax];

    if ( mmw->lastw_instance_count!=mmw->instance_count ) {
	temp = galloc(mmw->instance_count*20+1);
	pos = 0;
	if ( mmw->old!=NULL && mmw->instance_count==mmw->old->instance_count ) {
	    for ( i=0; i<mmw->instance_count; ++i ) {
		sprintf(temp+pos,"%g ", mmw->old->defweights[i] );
		pos += strlen(temp+pos);
	    }
	    utc = MMDesignCoords(mmw->old);
	} else {
	    for ( i=0; i<mmw->axis_count; ++i ) {
		if ( strcmp(mmw->mm->axes[i],"Weight")==0 &&
			400>=mmw->mm->axismaps[i].designs[0] &&
			400<=mmw->mm->axismaps[i].designs[mmw->mm->axismaps[i].points-1])
		    axiscoords[i] = 400;
		else if ( strcmp(mmw->mm->axes[i],"OpticalSize")==0 &&
			12>=mmw->mm->axismaps[i].designs[0] &&
			12<=mmw->mm->axismaps[i].designs[mmw->mm->axismaps[i].points-1])
		    axiscoords[i] = 12;
		else
		    axiscoords[i] = (mmw->mm->axismaps[i].designs[0]+
			    mmw->mm->axismaps[i].designs[mmw->mm->axismaps[i].points-1])/2;
	    }
	    i = ExecConvertDesignVector(axiscoords,mmw->axis_count,mmw->mm->ndv,mmw->mm->cdv,
		weights);
	    if ( i!=mmw->instance_count ) {	/* The functions don't work */
		for ( i=0; i<mmw->instance_count; ++i )
		    weights[i] = 1.0/mmw->instance_count;
		utc = uc_copy("");
	    } else {
		for ( i=0; i<mmw->axis_count; ++i ) {
		    sprintf(temp+pos,"%g ", axiscoords[i] );
		    pos += strlen(temp+pos);
		}
		temp[pos-1] = '\0';
		utc = uc_copy(temp);
		pos = 0;
	    }
	    for ( i=0; i<mmw->instance_count; ++i ) {
		sprintf(temp+pos,"%g ", weights[i] );
		pos += strlen(temp+pos);
	    }
	}
	temp[pos-1] = '\0';
	ut = uc_copy(temp);
	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_others],CID_NewBlends),
		ut);
	free(temp); free(ut);

	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_others],CID_NewDesign),utc);
	free(utc);
	mmw->lastw_instance_count = mmw->instance_count;
    }
}

static GTextInfo *TiFromFont(SplineFont *sf) {
    GTextInfo *ti = gcalloc(1,sizeof(GTextInfo));
    ti->text = uc_copy(sf->fontname);
    ti->fg = ti->bg = COLOR_DEFAULT;
    ti->userdata = sf;
return( ti );
}

static GTextInfo **FontList(MMW *mmw, int instance, int *sel) {
    FontView *fv;
    int cnt, i, pos;
    GTextInfo **ti;

    cnt = 0;
    if ( mmw->old!=NULL ) {
	cnt = mmw->old->instance_count;
    }
    for ( fv=fv_list; fv!=NULL; fv=fv->next ) {
	if ( fv->cidmaster==NULL && fv->sf->mm==NULL )
	    ++cnt;
    }
    cnt += mmw->lcnt;
    
    ++cnt;	/* New */
    ++cnt;	/* Browse... */

    ti = galloc((cnt+1)*sizeof(GTextInfo *));
    pos = -1;
    cnt = 0;
    if ( mmw->old!=NULL ) {
	for ( i=0; i<mmw->old->instance_count; ++i ) {
	    if ( mmw->old->instances[i]==mmw->mm->instances[instance] ) pos = cnt;
	    ti[cnt++] = TiFromFont(mmw->old->instances[i]);
	}
    }
    for ( fv=fv_list; fv!=NULL; fv=fv->next ) {
	if ( fv->cidmaster==NULL && fv->sf->mm==NULL ) {
	    if ( fv->sf==mmw->mm->instances[instance] ) pos = cnt;
	    ti[cnt++] = TiFromFont(fv->sf);
	}
    }
    for ( i=0; i<mmw->lcnt; ++i ) {
	if ( mmw->loaded[i]==mmw->mm->instances[instance] ) pos = cnt;
	ti[cnt++] = TiFromFont( mmw->loaded[i]);
    }
    if ( pos==-1 ) pos=cnt;
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
    ti[cnt]->text = u_copy(GStringGetResource(_STR_New,NULL));
    ti[cnt]->bg = ti[cnt]->fg = COLOR_DEFAULT;
    ++cnt;
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
    ti[cnt]->text = u_copy(GStringGetResource(_STR_Browse,NULL));
    ti[cnt]->bg = ti[cnt]->fg = COLOR_DEFAULT;
    ti[cnt]->userdata = (void *) (-1);
    ++cnt;
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));

    ti[pos]->selected = true;
    *sel = pos;

return(ti);
}

static void MMW_DesignsSetup(MMW *mmw) {
    int i,j,sel;
    char buffer[80], *pt;
    unichar_t ubuf[80];
    GTextInfo **ti;

    for ( i=0; i<mmw->instance_count; ++i ) {
	GGadget *list = GWidgetGetControl(mmw->subwins[mmw_designs],CID_DesignFonts+i*50);
	ti = FontList(mmw,i,&sel);
	GGadgetSetList(list, ti,false);
	GGadgetSetTitle(list, ti[sel]->text);
	pt = buffer;
	for ( j=0; j<mmw->axis_count; ++j ) {
	    sprintf(pt,"%g ",mmw->mm->positions[i*4+j]);
	    pt += strlen(pt);
	}
	if ( pt>buffer ) pt[-1] = '\0';
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(mmw->subwins[mmw_designs],CID_AxisWeights+i*50),
		ubuf);
    }
}

static SplineFont *MMNewFont(MMSet *mm,int index,char *familyname) {
    SplineFont *sf, *base;
    char *pt1, *pt2;
    int i;

    sf = SplineFontNew();
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
	sf->fontname = MMMakeMasterFontname(mm,index,&sf->fullname);
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
	sf->encoding_name = base->encoding_name;
	free(sf->chars);
	sf->chars = gcalloc(base->charcnt,sizeof(SplineChar *));
	sf->charcnt = base->charcnt;
	sf->new = base->new;
	sf->ascent = base->ascent;
	sf->descent = base->descent;
	free(sf->origname);
	sf->origname = copy(base->origname);
	if ( index==-1 ) {
	    free(sf->copyright);
	    sf->copyright = copy(base->copyright);
	}
	/* Make sure we get the encoding exactly right */
	for ( i=0; i<base->charcnt; ++i ) if ( base->chars[i]!=NULL ) {
	    SplineChar *sc = SFMakeChar(sf,i), *bsc = base->chars[i];
	    sc->width = bsc->width; sc->widthset = true;
	    free(sc->name); sc->name = copy(bsc->name);
	    sc->unicodeenc = bsc->unicodeenc;
	}
    }
    sf->onlybitmaps = false;
    sf->order2 = false;
    sf->mm = mm;
return( sf );
}

static void MMW_DoOK(MMW *mmw) {
    real weights[MmMax];
    real fbt;
    int err = false;
    char *familyname, *fn, *origname=NULL;
    int i,j;
    MMSet *setto, *dlgmm;
    FontView *fv = NULL;

    if ( !GetWeights(mmw->gw, weights, mmw->mm, mmw->instance_count, mmw->axis_count))
return;
    fbt = GetRealR(mmw->subwins[mmw_others],CID_ForceBoldThreshold,
		    _STR_ForceBoldThreshold,&err);
    if ( err )
return;
    familyname = cu_copy(_GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_others],CID_FamilyName)));
    /* They only need specify a family name if there are new fonts */
    if ( *familyname=='\0' ) {
	free(familyname);
	for ( i=0; i<mmw->instance_count; ++i )
	    if ( mmw->mm->instances[i]==NULL )
	break;
	    else
		fn = mmw->mm->instances[i]->familyname;
	if ( i!=mmw->instance_count ) {
	    GWidgetErrorR(_STR_BadMM,_STR_FamilyNameRequired);
return;
	}
	familyname = copy(fn);
    }

    /* Did we have a fontview open on this mm? */
    if ( mmw->old!=NULL ) {
	for ( j=0; j<mmw->old->instance_count; ++j )
	    if ( mmw->old->instances[j]->fv!=NULL ) {
		fv = mmw->old->instances[j]->fv;
		origname = copy(mmw->old->instances[j]->origname);
	break;
	    }
    }

    /* Make sure we free all fonts that we have lying around and aren't going */
    /* to be using. (ones we opened, ones in the old version of the mm). Also */
    /* if any font we want to use is attached to a fontview, then close the */
    /* window */
    for ( i=0; i<mmw->instance_count; ++i ) if ( mmw->mm->instances[i]!=NULL ) {
	if ( mmw->old!=NULL ) {
	    for ( j=0; j<mmw->old->instance_count; ++j )
		if ( mmw->mm->instances[i]==mmw->old->instances[j] )
	    break;
	    if ( j!=mmw->old->instance_count ) {
		mmw->old->instances[j] = NULL;
    continue;
	    }
	}
	for ( j=0; j<mmw->lcnt; ++j )
	    if ( mmw->mm->instances[i]==mmw->loaded[j] )
	break;
	if ( j!=mmw->lcnt ) {
	    mmw->loaded[j] = NULL;
continue;
	}
	MMUsurpNew(mmw->mm->instances[i]);
    }
    if ( mmw->old!=NULL ) {
	for ( j=0; j<mmw->old->instance_count; ++j )
	    if ( mmw->old->instances[j]!=NULL ) {
		MMDetachOld(mmw->old->instances[j]);
		mmw->old->instances[j] = NULL;
	    }
	MMDetachOld(mmw->old->normal);
	mmw->old->normal = NULL;
    }
    for ( j=0; j<mmw->lcnt; ++j ) {
	if ( mmw->loaded[j]!=NULL ) {
	    MMDetachNew(mmw->loaded[j]);
	    mmw->loaded[j] = NULL;
	}
    }

    dlgmm = mmw->mm;
    setto = mmw->old;
    if ( setto!=NULL ) {
	MMSetFreeContents(setto);
	memset(setto,0,sizeof(MMSet));
    } else
	setto = chunkalloc(sizeof(MMSet));
    setto->axis_count = mmw->axis_count;
    setto->instance_count = mmw->instance_count;
    for ( i=0; i<setto->axis_count; ++i )
	setto->axes[i] = dlgmm->axes[i];
    setto->axismaps = dlgmm->axismaps;
    setto->defweights = galloc(setto->instance_count*sizeof(real));
    memcpy(setto->defweights,weights,setto->instance_count*sizeof(real));
    free(dlgmm->defweights);
    setto->positions = galloc(setto->instance_count*setto->axis_count*sizeof(real));
    for ( i=0; i<setto->instance_count; ++i )
	memcpy(setto->positions+i*setto->axis_count,dlgmm->positions+i*dlgmm->axis_count,
		setto->axis_count*sizeof(real));
    free(dlgmm->positions);
    setto->instances = gcalloc(setto->instance_count,sizeof(SplineFont *));
    for ( i=0; i<setto->instance_count; ++i ) {
	if ( dlgmm->instances[i]!=NULL ) {
	    setto->instances[i] = dlgmm->instances[i];
	    setto->instances[i]->mm = setto;
	}
    }
    setto->normal = MMNewFont(setto,-1,familyname);
    if ( fbt>0 && fbt<=1 ) {
	char buffer[20];
	sprintf(buffer,"%g", fbt );
	setto->normal->private = gcalloc(1,sizeof(struct psdict));
	PSDictChangeEntry(setto->normal->private,"ForceBoldThreshold",buffer);
    }
    for ( i=0; i<setto->instance_count; ++i ) {
	if ( dlgmm->instances[i]==NULL )
	    setto->instances[i] = MMNewFont(setto,i,familyname);
	setto->instances[i]->fv = fv;
    }
    free(dlgmm->instances);
    setto->cdv = dlgmm->cdv;
    setto->ndv = dlgmm->ndv;
    chunkfree(dlgmm,sizeof(MMSet));
    if ( origname!=NULL ) {
	for ( i=0; i<setto->instance_count; ++i ) {
	    free(setto->instances[i]->origname);
	    setto->instances[i]->origname = copy(origname);
	}
	free(setto->normal->origname);
	setto->normal->origname = origname;
    } else {
	for ( i=0; i<setto->instance_count; ++i ) {
	    free(setto->instances[i]->origname);
	    setto->instances[i]->origname = copy(setto->normal->origname);
	}
    }
    if ( fv!=NULL ) {
	for ( i=0; i<setto->instance_count; ++i )
	    if ( fv->sf==setto->instances[i])
	break;
	if ( i==setto->instance_count ) {
	    SplineFont *sf = setto->normal;
	    BDFFont *bdf;
	    int same = fv->filled == fv->show;
	    fv->sf = sf;
	    bdf = SplineFontPieceMeal(fv->sf,sf->display_size<0?-sf->display_size:default_fv_font_size,
		    (fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0),
		    NULL);
	    BDFFontFree(fv->filled);
	    fv->filled = bdf;
	    if ( same )
		fv->show = bdf;
	}
    }
    free(familyname);
    MMReblend(fv,setto);

    /* Multi-Mastered bitmaps don't make much sense */
    /* Well, maybe grey-scaled could be interpolated, but yuck */
    for ( i=0; i<setto->instance_count; ++i ) {
	BDFFont *bdf, *bnext;
	for ( bdf = setto->instances[i]->bitmaps; bdf!=NULL; bdf = bnext ) {
	    bnext = bdf->next;
	    BDFFontFree(bdf);
	}
	setto->instances[i]->bitmaps = NULL;
    }

    if ( fv==NULL )
	fv = FontViewCreate(setto->normal);
    mmw->done = true;
}

static void MMW_DoNext(MMW *mmw) {
    int i, err;
    real start, end, *designs, *norm;
    int n, n2;

    if ( mmw->state==mmw_others )
return;

    /* Parse stuff!!!!! */
    if ( mmw->state==mmw_counts ) {
	mmw->axis_count = GGadgetGetFirstListSelectedItem(GWidgetGetControl(mmw->subwins[mmw_counts],CID_AxisCount))+1;
	mmw->instance_count = GGadgetGetFirstListSelectedItem(GWidgetGetControl(mmw->subwins[mmw_counts],CID_MasterCount))+1;
	/* Arrays are already allocated out to maximum, and we will just leave*/
	/*  them there until user hits OK, then we make them the right size */
	for ( i=0; i<4; ++i )
	    GTabSetSetEnabled(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
		    i,i<mmw->axis_count);
	for ( i=0; i<MmMax; ++i )
	    GTabSetSetEnabled(GWidgetGetControl(mmw->subwins[mmw_designs],CID_WhichDesign),
		    i,i<mmw->instance_count);
	/* If we've changed the axis count, and the old selected axis isn't */
	/*  available any more, choose another one */
	if ( GTabSetGetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis))>=
		mmw->axis_count )
	    GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
		    0);
	if ( mmw->instance_count==(1<<mmw->axis_count) ) {
	    for ( i=(mmw->old==NULL)?0:mmw->old->instance_count; i<mmw->instance_count; ++i ) {
		mmw->mm->positions[i*4  ] = (i&1) ? 1 : 0;
		mmw->mm->positions[i*4+1] = (i&2) ? 1 : 0;
		mmw->mm->positions[i*4+2] = (i&4) ? 1 : 0;
		mmw->mm->positions[i*4+3] = (i&8) ? 1 : 0;
	    }
	}
    } else if ( mmw->state==mmw_axes ) {
	for ( i=0; i<mmw->axis_count; ++i ) {
	    free(mmw->mm->axes[i]);
	    mmw->mm->axes[i] = cu_copy(_GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_axes],CID_AxisType+i*100)));
	    if ( *mmw->mm->axes[i]=='\0' ) {
		GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
			i);
		GWidgetErrorR(_STR_BadAxis,_STR_SetAxisType);
return;		/* Failure */
	    }
	    err = false;
	    start = GetRealR(mmw->subwins[mmw_axes],CID_AxisBegin+i*100,
		    _STR_Begin,&err);
	    end = GetRealR(mmw->subwins[mmw_axes],CID_AxisEnd+i*100,
		    _STR_End,&err);
	    if ( start>=end ) {
		GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
			i);
		GWidgetErrorR(_STR_BadAxis,_STR_AxisRangeNotValid);
return;		/* Failure */
	    }
	    n = ParseList(mmw->subwins[mmw_axes],CID_IntermediateDesign+i*100,
		    _STR_DesignSettings,&err,start,end,&designs,CID_WhichAxis,i);
	    n2 = ParseList(mmw->subwins[mmw_axes],CID_IntermediateNormalized+i*100,
		    _STR_NormalizedSettings,&err,0,1,&norm,CID_WhichAxis,i);
	    if ( n!=n2 || err ) {
		GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_axes],CID_WhichAxis),
			i);
		if ( !err )
		    GWidgetErrorR(_STR_BadAxis,_STR_DesignNormMustCorrespond);
		free(designs); free(norm);
return;		/* Failure */
	    }
	    mmw->mm->axismaps[i].points = n;
	    free(mmw->mm->axismaps[i].blends); free(mmw->mm->axismaps[i].designs);
	    mmw->mm->axismaps[i].blends = norm; mmw->mm->axismaps[i].designs=designs;
	}
    } else if ( mmw->state==mmw_designs ) {
	real positions[MmMax][4];
	int used[MmMax];
	int j,k,mask;
	SplineFont *sfs[MmMax];
	GTextInfo *ti;

	memset(used,0,sizeof(used));
	memset(positions,0,sizeof(positions));
	for ( i=0; i<mmw->instance_count; ++i ) {
	    if ( !ParseWeights(mmw->subwins[mmw_designs],CID_AxisWeights+i*50,
		    _STR_AxisWeights,positions[i],mmw->axis_count,
		    CID_WhichDesign,i))
return;
	    mask = 0;
	    for ( j=0; j<mmw->axis_count; ++j ) {
		if ( positions[i][j]==0 )
		    /* Do Nothing */;
		else if ( positions[i][j]==1.0 )
		    mask |= (1<<j);
		else
	    break;
	    }
	    if ( j==mmw->axis_count )
		used[mask] = true;
	    for ( j=0; j<i-1; ++j ) {
		for ( k=0; k<mmw->axis_count; ++k )
		    if ( positions[j][k] != positions[i][k] )
		break;
		if ( k==mmw->axis_count ) {
		    GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_designs],CID_WhichDesign),i);
		    GWidgetErrorR(_STR_BadMM,_STR_PositionUsedTwice,
			    _GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_designs],CID_AxisWeights+i*50)));
return;
		}
	    }
	    ti = GGadgetGetListItemSelected(GWidgetGetControl(mmw->subwins[mmw_designs],CID_DesignFonts+i*50));
	    sfs[i] = ti->userdata;
	    if ( sfs[i]!=NULL ) {
		for ( j=0; j<i; ++j )
		    if ( sfs[i]==sfs[j] ) {
			GTabSetSetSel(GWidgetGetControl(mmw->subwins[mmw_designs],CID_WhichDesign),i);
			GWidgetErrorR(_STR_BadMM,_STR_FontUsedTwice,sfs[i]->fontname);
return;
		    }
	    }
	}
	for ( i=0; i<(1<<mmw->axis_count); ++i ) if ( !used[i] ) {
	    char buffer[20], *pt = buffer;
	    for ( j=0; j<mmw->axis_count; ++j ) {
		sprintf( pt, "%d ", (i&(1<<j))? 1: 0 );
		pt += 2;
	    }
	    GWidgetErrorR(_STR_BadMM,_STR_PositionUnused, buffer );
return;
	}
	memcpy(mmw->mm->positions,positions,sizeof(positions));
	for ( i=0; i<mmw->instance_count; ++i )
	    mmw->mm->instances[i] = sfs[i];
	if ( mmw->old!=NULL &&
		mmw->axis_count==mmw->old->axis_count &&
		mmw->instance_count==mmw->old->instance_count &&
		PositionsMatch(mmw->old,mmw->mm))
	    /* It's what the font started with, don't complain, already has a cdv */;
	else if ( mmw->instance_count==(1<<mmw->axis_count) &&
		StandardPositions(mmw->mm,mmw->instance_count,mmw->axis_count))
	    /* It's arranged as we expect it to be */;
	else if ( mmw->axis_count==1 &&
		OrderedPositions(mmw->mm,mmw->instance_count))
	    /* It's arranged according to our secondary expectations */;
	else if ( mmw->instance_count==(1<<mmw->axis_count) ||
		mmw->axis_count==1 ) {
	    static int buts[] = { _STR_Yes, _STR_No, 0 };
	    if ( GWidgetAskR(_STR_DisorderedDesigns,buts,0,1,_STR_DisorderedDesignsOk)==1 )
return;
	}
    } else if ( mmw->state==mmw_funcs ) {
	if ( *_GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_NDV))=='\0' ||
		*_GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_CDV))=='\0' ) {
	    GWidgetErrorR(_STR_BadFunction,_STR_BadFunction);
return;
	}
	free(mmw->mm->ndv); free(mmw->mm->cdv);
	mmw->mm->ndv = cu_copy( _GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_NDV)));
	mmw->mm->cdv = cu_copy( _GGadgetGetTitle(GWidgetGetControl(mmw->subwins[mmw_funcs],CID_CDV)));
    }

    ++mmw->state;
    if ( mmw->state==mmw_others )
	MMW_WeightsValid(mmw);
    else if ( mmw->state==mmw_funcs )
	MMW_FuncsValid(mmw);
    else if ( mmw->state==mmw_designs )
	MMW_DesignsSetup(mmw);
    MMW_SetState(mmw);
}

static void MMW_SimulateDefaultButton(MMW *mmw) {
    if ( mmw->state==mmw_others )
	MMW_DoOK(mmw);
    else
	MMW_DoNext(mmw);
}

static int MMW_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	MMW_DoOK(mmw);
    }
return( true );
}

static int MMW_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	MMW_DoNext(mmw);
    }
return( true );
}

static int MMW_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	if ( mmw->state!=mmw_counts ) {
	    --mmw->state;
	    MMW_SetState(mmw);
	}
    }
return( true );
}

static int MMW_CheckOptical(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	char *top, *bottom;
	unichar_t *ut;
	const unichar_t *ret = _GGadgetGetTitle(g);
	int di = (GGadgetGetCid(g)-CID_AxisType)/100;
	char buf1[20], buf2[20];

	if ( mmw->old!=NULL && di<mmw->old->axis_count &&
		uc_strcmp(ret,mmw->old->axes[di])==0 ) {
	    sprintf(buf1,"%g", mmw->old->axismaps[di].designs[0]);
	    sprintf(buf2,"%g", mmw->old->axismaps[di].designs[mmw->old->axismaps[di].points-1]);
	    top = buf2;
	    bottom = buf1;
	} else if ( uc_strcmp(ret,"OpticalSize")==0 ) {
	    top = "72";
	    bottom = "6";
	} else {
	    top = "999";
	    bottom = "50";
	}
	ut = uc_copy(top);
	GGadgetSetTitle(GWidgetGetControl(GGadgetGetWindow(g),
		GGadgetGetCid(g)-CID_AxisType + CID_AxisEnd), ut);
	free(ut);
	ut = uc_copy(bottom);
	GGadgetSetTitle(GWidgetGetControl(GGadgetGetWindow(g),
		GGadgetGetCid(g)-CID_AxisType + CID_AxisBegin), ut);
	free(ut);
    }
return( true );
}

static int MMW_CheckBrowse(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	MMW *mmw = GDrawGetUserData(GGadgetGetWindow(g));
	/*int di = (GGadgetGetCid(g)-CID_DesignFonts)/50;*/
	GTextInfo *ti = GGadgetGetListItemSelected(g);
	char *temp;
	SplineFont *sf;
	GTextInfo **tis;
	int i,sel,oldsel;
	unichar_t *ut;

	if ( ti!=NULL && ti->userdata == (void *) -1 ) {
	    temp = GetPostscriptFontName(NULL,false);
	    if ( temp==NULL )
return(true);
	    sf = LoadSplineFont(temp,0);
	    if ( sf==NULL )
return(true);
	    if ( sf->cidmaster!=NULL || sf->subfonts!=0 ) {
		GWidgetErrorR(_STR_BadMM,_STR_NoCIDinMM);
return(true);
	    } else if ( sf->mm!=NULL ) {
		GWidgetErrorR(_STR_BadMM,_STR_NoCIDinMM);
return(true);
	    }
	    if ( sf->fv==NULL ) {
		if ( mmw->lcnt>=mmw->lmax ) {
		    if ( mmw->lmax==0 )
			mmw->loaded = galloc((mmw->lmax=10)*sizeof(SplineFont *));
		    else
			mmw->loaded = grealloc(mmw->loaded,(mmw->lmax+=10)*sizeof(SplineFont *));
		}
		mmw->loaded[mmw->lcnt++] = sf;
		for ( i=0; i<mmw->instance_count; ++i ) {
		    GGadget *list = GWidgetGetControl(mmw->subwins[mmw_designs],CID_DesignFonts+i*50);
		    oldsel = GGadgetGetFirstListSelectedItem(list);
		    tis = FontList(mmw,i,&sel);
		    tis[sel]->selected = false;
		    tis[oldsel]->selected = true;
		    GGadgetSetList(list, tis, false);
		}
	    }
	    GGadgetSetTitle(g,ut = uc_copy(sf->fontname));
	    free(ut);
	}
    }
return( true );
}

static int mmwsub_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("multiplemaster.html");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		MMW_Close(GDrawGetUserData(gw));
return( true );
	} else if ( event->u.chr.chars[0]=='\r' ) {
	    MMW_SimulateDefaultButton( (MMW *) GDrawGetUserData(gw));
return( true );
	}
return( false );
    }
return( true );
}

static int mmw_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	MMW *mmw = GDrawGetUserData(gw);
	MMW_Close(mmw);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("multiplemaster.html");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		MMW_Close(GDrawGetUserData(gw));
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	} else if ( event->u.chr.chars[0]=='\r' ) {
	    MMW_SimulateDefaultButton( (MMW *) GDrawGetUserData(gw));
return( true );
	}
return( false );
    }
return( true );
}

static MMSet *MMCopy(MMSet *orig) {
    MMSet *mm;
    int i;
    /* Allocate the arrays out to maximum, we'll fix them up later, and we */
    /*  retain the proper counts in the mmw structure. This means we don't */
    /*  lose data when they shrink and then restore a value */

    mm = chunkalloc(sizeof(MMSet));
    mm->apple = orig->apple;
    mm->instance_count = MmMax;
    mm->axis_count = 4;
    for ( i=0; i<orig->axis_count; ++i )
	mm->axes[i] = copy(orig->axes[i]);
    mm->instances = gcalloc(MmMax,sizeof(SplineFont *));
    memcpy(mm->instances,orig->instances,orig->instance_count*sizeof(SplineFont *));
    mm->positions = gcalloc(MmMax*4,sizeof(real));
    for ( i=0; i<orig->instance_count; ++i )
	memcpy(mm->positions+i*4,orig->positions+i*orig->axis_count,orig->axis_count*sizeof(real));
    mm->defweights = gcalloc(MmMax,sizeof(real));
    memcpy(mm->defweights,orig->defweights,orig->instance_count*sizeof(real));
    mm->axismaps = gcalloc(4,sizeof(struct axismap));
    for ( i=0; i<orig->axis_count; ++i ) {
	mm->axismaps[i].points = orig->axismaps[i].points;
	mm->axismaps[i].blends = galloc(mm->axismaps[i].points*sizeof(real));
	memcpy(mm->axismaps[i].blends,orig->axismaps[i].blends,mm->axismaps[i].points*sizeof(real));
	mm->axismaps[i].designs = galloc(mm->axismaps[i].points*sizeof(real));
	memcpy(mm->axismaps[i].designs,orig->axismaps[i].designs,mm->axismaps[i].points*sizeof(real));
	mm->axismaps[i].min = orig->axismaps[i].min;
	mm->axismaps[i].max = orig->axismaps[i].max;
	mm->axismaps[i].def = orig->axismaps[i].def;
    }
    mm->cdv = copy(orig->cdv);
    mm->ndv = copy(orig->ndv);
return( mm );
}

void MMWizard(MMSet *mm) {
    MMW mmw;
    GRect pos, subpos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo axisaspects[5], designaspects[17];
    GGadgetCreateData bgcd[8], cntgcd[9], axisgcd[4][13], designgcd[16][5],
	    agcd[2], dgcd[3], ogcd[9];
    GTextInfo blabel[8], cntlabel[9], axislabel[4][13], designlabel[16][5],
	    dlabel, olabels[9];
    char axisbegins[4][20], axisends[4][20];
    char *normalized[4], *designs[4];
    char *pt, *freeme;
    int i,k;
    int isadobe = mm==NULL || !mm->apple;
    int space,blen= GIntGetResource(_NUM_Buttonsize)*100/GGadgetScale(100);
    static int axistablab[] = { _STR_Axis1, _STR_Axis2, _STR_Axis3, _STR_Axis4 };
    static char *designtablab[] = { "1", "2", "3", "4", "5", "6", "7", "8",
	    "9", "10", "11", "12", "13", "14", "15", "16", NULL };

    memset(&mmw,0,sizeof(mmw));
    mmw.old = mm;
    if ( mm!=NULL ) {
	mmw.mm = MMCopy(mm);
	mmw.axis_count = mm->axis_count;
	mmw.instance_count = mm->instance_count;
    } else {
	mmw.mm = chunkalloc(sizeof(MMSet));
	mmw.axis_count = 1;
	mmw.instance_count = 2;
	mmw.mm->axis_count = 4; mmw.mm->instance_count = MmMax;
	mmw.mm->instances = gcalloc(MmMax,sizeof(SplineFont *));
	mmw.mm->positions = gcalloc(MmMax*4,sizeof(real));
	mmw.mm->defweights = gcalloc(MmMax,sizeof(real));
	mmw.mm->axismaps = gcalloc(4,sizeof(struct axismap));
	mmw.isnew = true;
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(mmw.isnew?_STR_CreateMM:_STR_MMInfo,NULL);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(MMW_Width));
    pos.height = GDrawPointsToPixels(NULL,MMW_Height);
    mmw.gw = gw = GDrawCreateTopWindow(NULL,&pos,mmw_e_h,&mmw,&wattrs);

    memset(&blabel,0,sizeof(blabel));
    memset(&bgcd,0,sizeof(bgcd));

    mmw.canceldrop = GDrawPointsToPixels(NULL,30);
    bgcd[0].gd.pos.x = 20; bgcd[0].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-33;
    bgcd[0].gd.pos.width = -1; bgcd[0].gd.pos.height = 0;
    bgcd[0].gd.flags = gg_visible | gg_enabled;
    blabel[0].text = (unichar_t *) _STR_OK;
    blabel[0].text_in_resource = true;
    bgcd[0].gd.label = &blabel[0];
    bgcd[0].gd.cid = CID_OK;
    bgcd[0].gd.handle_controlevent = MMW_OK;
    bgcd[0].creator = GButtonCreate;

    space = (MMW_Width-4*blen-40)/3;
    bgcd[1].gd.pos.x = bgcd[0].gd.pos.x+blen+space; bgcd[1].gd.pos.y = bgcd[0].gd.pos.y;
    bgcd[1].gd.pos.width = -1; bgcd[1].gd.pos.height = 0;
    bgcd[1].gd.flags = gg_visible;
    blabel[1].text = (unichar_t *) _STR_PrevArrow;
    blabel[1].text_in_resource = true;
    bgcd[1].gd.label = &blabel[1];
    bgcd[1].gd.handle_controlevent = MMW_Prev;
    bgcd[1].gd.cid = CID_Prev;
    bgcd[1].creator = GButtonCreate;

    bgcd[2].gd.pos.x = bgcd[1].gd.pos.x+blen+space; bgcd[2].gd.pos.y = bgcd[1].gd.pos.y;
    bgcd[2].gd.pos.width = -1; bgcd[2].gd.pos.height = 0;
    bgcd[2].gd.flags = gg_visible;
    blabel[2].text = (unichar_t *) _STR_NextArrow;
    blabel[2].text_in_resource = true;
    bgcd[2].gd.label = &blabel[2];
    bgcd[2].gd.handle_controlevent = MMW_Next;
    bgcd[2].gd.cid = CID_Next;
    bgcd[2].creator = GButtonCreate;

    bgcd[3].gd.pos.x = -20; bgcd[3].gd.pos.y = bgcd[1].gd.pos.y;
    bgcd[3].gd.pos.width = -1; bgcd[3].gd.pos.height = 0;
    bgcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    blabel[3].text = (unichar_t *) _STR_Cancel;
    blabel[3].text_in_resource = true;
    bgcd[3].gd.label = &blabel[3];
    bgcd[3].gd.handle_controlevent = MMW_Cancel;
    bgcd[3].gd.cid = CID_Cancel;
    bgcd[3].creator = GButtonCreate;

    bgcd[4].gd.pos.x = 2; bgcd[4].gd.pos.y = 2;
    bgcd[4].gd.pos.width = pos.width-4; bgcd[4].gd.pos.height = pos.height-4;
    bgcd[4].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    bgcd[4].gd.cid = CID_Group;
    bgcd[4].creator = GGroupCreate;

    GGadgetsCreate(gw,bgcd);

    subpos = pos;
    subpos.y = subpos.x = 4;
    subpos.width -= 8;
    mmw.subheightdiff = GDrawPointsToPixels(NULL,44)-8;
    subpos.height -= mmw.subheightdiff;
    wattrs.mask = wam_events;
    for ( i=mmw_counts; i<=mmw_others; ++i )
	mmw.subwins[i] = GWidgetCreateSubWindow(mmw.gw,&subpos,mmwsub_e_h,&mmw,&wattrs);

    memset(&cntlabel,0,sizeof(cntlabel));
    memset(&cntgcd,0,sizeof(cntgcd));

    k=0;
    cntlabel[k].text = (unichar_t *) _STR_TypeOfDistortableFont;
    cntlabel[k].text_in_resource = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 5; cntgcd[k].gd.pos.y = 11;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k++].creator = GLabelCreate;

    cntlabel[k].text = (unichar_t *) _STR_Adobe;
    cntlabel[k].text_in_resource = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 10; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+12;
    cntgcd[k].gd.flags = isadobe ? (gg_visible | gg_enabled | gg_cb_on) :
	    ( gg_visible | gg_enabled );
    cntgcd[k].gd.cid = CID_Adobe;
    cntgcd[k].gd.handle_controlevent = MM_TypeChanged;
    cntgcd[k++].creator = GRadioCreate;

    cntlabel[k].text = (unichar_t *) _STR_Apple;
    cntlabel[k].text_in_resource = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 70; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y;
    cntgcd[k].gd.flags = !isadobe ? (gg_visible | gg_enabled | gg_cb_on) :
	    ( gg_visible | gg_enabled );
    cntgcd[k].gd.cid = CID_Apple;
    cntgcd[k].gd.handle_controlevent = MM_TypeChanged;
    cntgcd[k++].creator = GRadioCreate;

    cntlabel[k].text = (unichar_t *) _STR_NumberOfAxes;
    cntlabel[k].text_in_resource = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 5; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+18;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k++].creator = GLabelCreate;

    cntgcd[k].gd.pos.x = 10; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+12;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k].gd.u.list = axiscounts;
    cntgcd[k].gd.label = &axiscounts[mmw.axis_count-1];
    cntgcd[k].gd.cid = CID_AxisCount;
    cntgcd[k].gd.handle_controlevent = MMW_AxisCntChanged;
    cntgcd[k++].creator = GListButtonCreate;
    for ( i=0; i<4; ++i )
	axiscounts[i].selected = false;
    axiscounts[mmw.axis_count-1].selected = true;

    cntlabel[k].text = (unichar_t *) _STR_NumberOfMasterDesigns;
    cntlabel[k].text_in_resource = true;
    cntgcd[k].gd.label = &cntlabel[k];
    cntgcd[k].gd.pos.x = 5; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+30;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k++].creator = GLabelCreate;

    cntgcd[k].gd.pos.x = 10; cntgcd[k].gd.pos.y = cntgcd[k-1].gd.pos.y+12;
    cntgcd[k].gd.flags = gg_visible | gg_enabled;
    cntgcd[k].gd.u.list = mastercounts;
    cntgcd[k].gd.label = &mastercounts[mmw.instance_count-1];
    cntgcd[k].gd.cid = CID_MasterCount;
    cntgcd[k++].creator = GListButtonCreate;
    for ( i=0; i<16; ++i )
	mastercounts[i].selected = false;
    mastercounts[mmw.instance_count-1].selected = true;

    GGadgetsCreate(mmw.subwins[mmw_counts],cntgcd);
    SetMasterToAxis(&mmw,true);

    memset(&axisgcd,0,sizeof(axisgcd));
    memset(&axislabel,0,sizeof(axislabel));
    memset(&agcd,0,sizeof(agcd));
    memset(&axisaspects,0,sizeof(axisaspects));

    for ( i=0; i<4; ++i ) {
	k=0;
	axislabel[i][k].text = (unichar_t *) _STR_AxisType;
	axislabel[i][k].text_in_resource = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 5; axisgcd[i][k].gd.pos.y = 11;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	axisgcd[i][k].gd.pos.x = 10; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+12;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.u.list = axistypes;
	axisgcd[i][k].gd.cid = CID_AxisType+i*100;
	axisgcd[i][k].gd.handle_controlevent = MMW_CheckOptical;
	if ( i<mmw.axis_count && mmw.mm->axes[i]!=NULL ) {
	    axislabel[i][k].text = uc_copy(mmw.mm->axes[i]);
	    axisgcd[i][k].gd.label = &axislabel[i][k];
	}
	axisgcd[i][k++].creator = GListFieldCreate;

	axislabel[i][k].text = (unichar_t *) _STR_AxisRange;
	axislabel[i][k].text_in_resource = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 5; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+30;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	if ( mmw.mm->axismaps[i].points<2 ) {
	    strcpy(axisbegins[i],"50");
	    strcpy(axisends[i],"999");
	} else {
	    sprintf(axisbegins[i],"%g", mmw.mm->axismaps[i].designs[0]);
	    sprintf(axisends[i],"%g", mmw.mm->axismaps[i].designs[mmw.mm->axismaps[i].points-1]);
	}

	axislabel[i][k].text = (unichar_t *) _STR_Begin;
	axislabel[i][k].text_in_resource = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 10; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+16;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	axislabel[i][k].text = (unichar_t *) axisbegins[i];
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 70; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y-4;
	axisgcd[i][k].gd.pos.width=50;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.cid = CID_AxisBegin+i*100;
	axisgcd[i][k++].creator = GTextFieldCreate;

	axislabel[i][k].text = (unichar_t *) _STR_End;
	axislabel[i][k].text_in_resource = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 130; axisgcd[i][k].gd.pos.y = axisgcd[i][k-2].gd.pos.y;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	axislabel[i][k].text = (unichar_t *) axisends[i];
	axislabel[i][k].text_is_1byte = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 170; axisgcd[i][k].gd.pos.y = axisgcd[i][k-2].gd.pos.y;
	axisgcd[i][k].gd.pos.width=50;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.cid = CID_AxisEnd+i*100;
	axisgcd[i][k++].creator = GTextFieldCreate;

	axislabel[i][k].text = (unichar_t *) _STR_IntermediatePoints;
	axislabel[i][k].text_in_resource = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 5; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+30;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	normalized[i]=NULL;
	designs[i]=NULL;
	if ( mmw.mm->axismaps[i].points>2 ) {
	    int l,j,len1, len2;
	    char buffer[30];
	    len1 = len2 = 0;
	    for ( l=0; l<2; ++l ) {
		for ( j=1; j<mmw.mm->axismaps[i].points-1; ++j ) {
		    /* I wanted to seperate things with commas, but that isn't*/
		    /*  a good idea in Europe (comma==decimal point) */
		    sprintf(buffer,"%g ",mmw.mm->axismaps[i].designs[j]);
		    if ( designs[i]!=NULL )
			strcpy(designs[i]+len1, buffer );
		    len1 += strlen(buffer);
		    sprintf(buffer,"%g ",mmw.mm->axismaps[i].blends[j]);
		    if ( normalized[i]!=NULL )
			strcpy(normalized[i]+len2, buffer );
		    len2 += strlen(buffer);
		}
		if ( l==0 ) {
		    normalized[i] = galloc(len2+2);
		    designs[i] = galloc(len1+2);
		} else {
		    normalized[i][len2-1] = '\0';
		    designs[i][len1-1] = '\0';
		}
	    }
	}

	axislabel[i][k].text = (unichar_t *) _STR_DesignSettings;
	axislabel[i][k].text_in_resource = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 10; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+16;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	if ( designs[i]!=NULL ) {
	    axislabel[i][k].text = (unichar_t *) designs[i];
	    axislabel[i][k].text_is_1byte = true;
	    axisgcd[i][k].gd.label = &axislabel[i][k];
	}
	axisgcd[i][k].gd.pos.x = 120; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y-4;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.cid = CID_IntermediateDesign+i*100;
	axisgcd[i][k++].creator = GTextFieldCreate;

	axislabel[i][k].text = (unichar_t *) _STR_NormalizedSettings;
	axislabel[i][k].text_in_resource = true;
	axisgcd[i][k].gd.label = &axislabel[i][k];
	axisgcd[i][k].gd.pos.x = 10; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y+34;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k++].creator = GLabelCreate;

	if ( normalized[i]!=NULL ) {
	    axislabel[i][k].text = (unichar_t *) normalized[i];
	    axislabel[i][k].text_is_1byte = true;
	    axisgcd[i][k].gd.label = &axislabel[i][k];
	}
	axisgcd[i][k].gd.pos.x = 120; axisgcd[i][k].gd.pos.y = axisgcd[i][k-1].gd.pos.y-4;
	axisgcd[i][k].gd.flags = gg_visible | gg_enabled;
	axisgcd[i][k].gd.cid = CID_IntermediateNormalized+i*100;
	axisgcd[i][k++].creator = GTextFieldCreate;

	axisaspects[i].text = (unichar_t *) axistablab[i];
	axisaspects[i].text_in_resource = true;
	axisaspects[i].gcd = axisgcd[i];
    }
    axisaspects[0].selected = true;

    agcd[0].gd.pos.x = 3; agcd[0].gd.pos.y = 3;
    agcd[0].gd.pos.width = MMW_Width-10;
    agcd[0].gd.pos.height = MMW_Height-45;
    agcd[0].gd.u.tabs = axisaspects;
    agcd[0].gd.flags = gg_visible | gg_enabled;
    agcd[0].gd.cid = CID_WhichAxis;
    agcd[0].creator = GTabSetCreate;

    GGadgetsCreate(mmw.subwins[mmw_axes],agcd);
    for ( i=0; i<4; ++i ) {
	free(axislabel[i][1].text);
	free(normalized[i]);
	free(designs[i]);
    }

    memset(&designgcd,0,sizeof(designgcd));
    memset(&designlabel,0,sizeof(designlabel));
    memset(&dgcd,0,sizeof(dgcd));
    memset(&dlabel,0,sizeof(dlabel));
    memset(&designaspects,0,sizeof(designaspects));

    for ( i=0; i<MmMax; ++i ) {
	designlabel[i][0].text = (unichar_t *) _STR_SourceOfDesign;
	designlabel[i][0].text_in_resource = true;
	designgcd[i][0].gd.label = &designlabel[i][0];
	designgcd[i][0].gd.pos.x = 3; designgcd[i][0].gd.pos.y = 4;
	designgcd[i][0].gd.flags = gg_visible | gg_enabled;
	designgcd[i][0].creator = GLabelCreate;

	designgcd[i][1].gd.pos.x = 15; designgcd[i][1].gd.pos.y = 18;
	designgcd[i][1].gd.pos.width = 200;
	designgcd[i][1].gd.flags = gg_visible | gg_enabled;
	designgcd[i][1].gd.cid = CID_DesignFonts + i*50;
	designgcd[i][1].gd.handle_controlevent = MMW_CheckBrowse;
	designgcd[i][1].creator = GListButtonCreate;

	designlabel[i][2].text = (unichar_t *) _STR_AxisWeights;
	designlabel[i][2].text_in_resource = true;
	designgcd[i][2].gd.label = &designlabel[i][2];
	designgcd[i][2].gd.pos.x = 3; designgcd[i][2].gd.pos.y = 50;
	designgcd[i][2].gd.flags = gg_visible | gg_enabled;
	designgcd[i][2].creator = GLabelCreate;

	designgcd[i][3].gd.pos.x = 15; designgcd[i][3].gd.pos.y = 64;
	designgcd[i][3].gd.pos.width = 200;
	designgcd[i][3].gd.flags = gg_visible | gg_enabled;
	designgcd[i][3].gd.cid = CID_AxisWeights + i*50;
	designgcd[i][3].creator = GTextFieldCreate;

	designaspects[i].text = (unichar_t *) designtablab[i];
	designaspects[i].text_is_1byte = true;
	designaspects[i].gcd = designgcd[i];
    }
    designaspects[0].selected = true;

    dlabel.text = (unichar_t *) _STR_MasterDesigns;
    dlabel.text_in_resource = true;
    dgcd[0].gd.label = &dlabel;
    dgcd[0].gd.pos.x = 3; dgcd[0].gd.pos.y = 4;
    dgcd[0].gd.flags = gg_visible | gg_enabled;
    dgcd[0].creator = GLabelCreate;

    dgcd[1].gd.pos.x = 3; dgcd[1].gd.pos.y = 18;
    dgcd[1].gd.pos.width = MMW_Width-10;
    dgcd[1].gd.pos.height = MMW_Height-60;
    dgcd[1].gd.u.tabs = designaspects;
    dgcd[1].gd.flags = gg_visible | gg_enabled;
    dgcd[1].gd.cid = CID_WhichDesign;
    dgcd[1].creator = GTabSetCreate;

    GGadgetsCreate(mmw.subwins[mmw_designs],dgcd);

    memset(&ogcd,0,sizeof(ogcd));
    memset(&olabels,0,sizeof(olabels));

    k=0;
    olabels[k].text = (unichar_t *) _STR_NDV;
    olabels[k].text_in_resource = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 3; ogcd[k].gd.pos.y = 4;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k++].creator = GLabelCreate;

    ogcd[k].gd.pos.x = 3; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+15;
    ogcd[k].gd.pos.width = MMW_Width-10;
    ogcd[k].gd.pos.height = 8*12+10;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_NDV;
    ogcd[k++].creator = GTextAreaCreate;

    olabels[k].text = (unichar_t *) _STR_CDV;
    olabels[k].text_in_resource = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 3; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+ogcd[k-1].gd.pos.height+5;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k++].creator = GLabelCreate;

    ogcd[k].gd.pos.x = 3; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+15;
    ogcd[k].gd.pos.width = MMW_Width-10;
    ogcd[k].gd.pos.height = 8*12+10;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_CDV;
    ogcd[k++].creator = GTextAreaCreate;

    GGadgetsCreate(mmw.subwins[mmw_funcs],ogcd);

    memset(&ogcd,0,sizeof(ogcd));
    memset(&olabels,0,sizeof(olabels));

    k=0;
    olabels[k].text = (unichar_t *) _STR_ContribOfMaster;
    olabels[k].text_in_resource = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 10; ogcd[k].gd.pos.y = 4;
    ogcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    ogcd[k].gd.cid = CID_Explicit;
    ogcd[k].gd.handle_controlevent = MMCB_Changed;
    ogcd[k++].creator = GRadioCreate;

    olabels[k].text = (unichar_t *) _STR_DesignAxisValues;
    olabels[k].text_in_resource = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 10; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+45;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_ByDesign;
    ogcd[k].gd.handle_controlevent = MMCB_Changed;
    ogcd[k++].creator = GRadioCreate;

    ogcd[k].gd.pos.x = 15; ogcd[k].gd.pos.y = ogcd[k-2].gd.pos.y+18;
    ogcd[k].gd.pos.width = 240;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_NewBlends;
    ogcd[k++].creator = GTextFieldCreate;

    ogcd[k].gd.pos.x = 15; ogcd[k].gd.pos.y = ogcd[k-2].gd.pos.y+18;
    ogcd[k].gd.pos.width = 240;
    ogcd[k].gd.flags = gg_visible;
    ogcd[k].gd.cid = CID_NewDesign;
    ogcd[k++].creator = GTextFieldCreate;

    olabels[k].text = (unichar_t *) _STR_ForceBoldThreshold;
    olabels[k].text_in_resource = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 10; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+45;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k++].creator = GLabelCreate;

    if ( mmw.old!=NULL &&
	    (pt = PSDictHasEntry(mmw.old->normal->private,"ForceBoldThreshold"))!=NULL )
	olabels[k].text = (unichar_t *) pt;
    else
	olabels[k].text = (unichar_t *) ".3";
    olabels[k].text_is_1byte = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 15; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+13;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_ForceBoldThreshold;
    ogcd[k++].creator = GTextFieldCreate;

    olabels[k].text = (unichar_t *) _STR_Familyname;
    olabels[k].text_in_resource = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 10; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+30;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k++].creator = GLabelCreate;

    freeme=NULL;
    if ( mmw.old!=NULL && mmw.old->normal->familyname!=NULL )
	olabels[k].text = (unichar_t *) (mmw.old->normal->familyname);
    else
	olabels[k].text = (unichar_t *) (freeme= GetNextUntitledName());
    olabels[k].text_is_1byte = true;
    ogcd[k].gd.label = &olabels[k];
    ogcd[k].gd.pos.x = 15; ogcd[k].gd.pos.y = ogcd[k-1].gd.pos.y+13;
    ogcd[k].gd.pos.width = 150;
    ogcd[k].gd.flags = gg_visible | gg_enabled;
    ogcd[k].gd.cid = CID_FamilyName;
    ogcd[k++].creator = GTextFieldCreate;

    GGadgetsCreate(mmw.subwins[mmw_others],ogcd);
    free(freeme);

    mmw.state = mmw_counts;
    MMW_SetState(&mmw);
    GDrawSetVisible(mmw.subwins[mmw.state],true);
    GDrawSetVisible(mmw.gw,true);

    while ( !mmw.done )
	GDrawProcessOneEvent(NULL);

    GDrawDestroyWindow(gw);
}
