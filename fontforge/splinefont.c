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

#include <fontforge-config.h>

#include "autohint.h"
#include "autotrace.h"
#include "dumppfa.h"
#include "encoding.h"
#include "fontforgevw.h"
#include "fvcomposite.h"
#include "fvfonts.h"
#include "fvimportbdf.h"
#include "gfile.h"
#include "gutils.h"
#include "ikarus.h"
#include "macbinary.h"
#include "namelist.h"
#include "palmfonts.h"
#include "parsepdf.h"
#include "parsepfa.h"
#include "parsettf.h"
#include "psfont.h"
#include "pua.h"
#include "sfd.h"
#include "splinefill.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "svg.h"
#include "unicoderange.h"
#include "ustring.h"
#include "utype.h"
#include "winfonts.h"
#include "woff.h"

#include <locale.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void SFUntickAll(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;
}

void SFOrderBitmapList(SplineFont *sf) {
    BDFFont *bdf, *prev, *next;
    BDFFont *bdf2, *prev2;

    for ( prev = NULL, bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	for ( prev2=NULL, bdf2=bdf->next; bdf2!=NULL; bdf2 = bdf2->next ) {
	    if ( bdf->pixelsize>bdf2->pixelsize ||
		    (bdf->pixelsize==bdf2->pixelsize && BDFDepth(bdf)>BDFDepth(bdf2)) ) {
		if ( prev==NULL )
		    sf->bitmaps = bdf2;
		else
		    prev->next = bdf2;
		if ( prev2==NULL ) {
		    bdf->next = bdf2->next;
		    bdf2->next = bdf;
		} else {
		    next = bdf->next;
		    bdf->next = bdf2->next;
		    bdf2->next = next;
		    prev2->next = bdf;
		}
		next = bdf;
		bdf = bdf2;
		bdf2 = next;
	    }
	    prev2 = bdf2;
	}
	prev = bdf;
    }
}

SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,EncMap *map,int i) {
    static char namebuf[100];
    static Layer layers[2];

    memset(dummy,'\0',sizeof(*dummy));
    dummy->color = COLOR_DEFAULT;
    dummy->layer_cnt = 2;
    dummy->layers = layers;
    if ( sf->cidmaster!=NULL ) {
	/* CID fonts don't have encodings, instead we must look up the cid */
	if ( sf->cidmaster->loading_cid_map )
	    dummy->unicodeenc = -1;
	else
	    dummy->unicodeenc = CID2NameUni(FindCidMap(sf->cidmaster->cidregistry,sf->cidmaster->ordering,sf->cidmaster->supplement,sf->cidmaster),
		    i,namebuf,sizeof(namebuf));
    } else
	dummy->unicodeenc = UniFromEnc(i,map->enc);

    if ( sf->cidmaster!=NULL )
	dummy->name = namebuf;
    else if ( map->enc->psnames!=NULL && i<map->enc->char_cnt &&
	    map->enc->psnames[i]!=NULL )
	dummy->name = map->enc->psnames[i];
    else if ( dummy->unicodeenc==-1 )
	dummy->name = NULL;
    else
	dummy->name = (char *) StdGlyphName(namebuf,dummy->unicodeenc,sf->uni_interp,sf->for_new_glyphs);
    if ( dummy->name==NULL ) {
	/*if ( dummy->unicodeenc!=-1 || i<256 )
	    dummy->name = ".notdef";
	else*/ {
	    int j;
	    sprintf( namebuf, "NameMe.%d", i);
	    j=0;
	    while ( SFFindExistingSlot(sf,-1,namebuf)!=-1 )
		sprintf( namebuf, "NameMe.%d.%d", i, ++j);
	    dummy->name = namebuf;
	}
    }
    dummy->width = dummy->vwidth = sf->ascent+sf->descent;
    if ( dummy->unicodeenc>0 && dummy->unicodeenc<0x10000 &&
	    iscombining(dummy->unicodeenc)) {
	/* Mark characters should be 0 width */
	dummy->width = 0;
	/* Except in monospaced fonts on windows, where they should be the */
	/*  same width as everything else */
    }
    /* Actually, in a monospace font, all glyphs should be the same width */
    /*  whether mark or not */
    if ( sf->pfminfo.panose_set && sf->pfminfo.panose[3]==9 &&
	    sf->glyphcnt>0 ) {
	for ( i=sf->glyphcnt-1; i>=0; --i )
	    if ( SCWorthOutputting(sf->glyphs[i])) {
		dummy->width = sf->glyphs[i]->width;
	break;
	    }
    }
    dummy->parent = sf;
    dummy->orig_pos = 0xffff;
return( dummy );
}

static SplineChar *_SFMakeChar(SplineFont *sf,EncMap *map,int enc) {
    SplineChar dummy, *sc;
    SplineFont *ssf;
    int j, real_uni, gid;

    if ( enc>=map->enccount )
	gid = -1;
    else
	gid = map->map[enc];
    if ( sf->subfontcnt!=0 && gid!=-1 ) {
	ssf = NULL;
	for ( j=0; j<sf->subfontcnt; ++j )
	    if ( gid<sf->subfonts[j]->glyphcnt ) {
		ssf = sf->subfonts[j];
		if ( ssf->glyphs[gid]!=NULL ) {
return( ssf->glyphs[gid] );
		}
	    }
	sf = ssf;
    }

    if ( gid==-1 || (sc = sf->glyphs[gid])==NULL ) {
	if (( map->enc->is_unicodebmp || map->enc->is_unicodefull ) &&
		( enc>=0xe000 && enc<=0xf8ff ) &&
		( sf->uni_interp==ui_ams || sf->uni_interp==ui_trad_chinese ) &&
		( real_uni = (sf->uni_interp==ui_ams ? amspua : cns14pua)[enc-0xe000])!=0 ) {
	    if ( real_uni<map->enccount ) {
		SplineChar *sc;
		/* if necessary, create the real unicode code point */
		/*  and then make us be a duplicate of it */
		sc = _SFMakeChar(sf,map,real_uni);
		map->map[enc] = gid = sc->orig_pos;
		SCCharChangedUpdate(sc,ly_all);
return( sc );
	    }
	}

	SCBuildDummy(&dummy,sf,map,enc);
	/* Let's say a user has a postscript encoding where the glyph ".notdef" */
	/*  is assigned to many slots. Once the user creates a .notdef glyph */
	/*  all those slots should fill in. If they don't they damn well better*/
	/*  when the user clicks on one to edit it */
	/* Used to do that with all encodings. It just confused people */
	if ( map->enc->psnames!=NULL &&
		(sc = SFGetChar(sf,dummy.unicodeenc,dummy.name))!=NULL ) {
	    map->map[enc] = sc->orig_pos;
	    AltUniAdd(sc,dummy.unicodeenc);
return( sc );
	}
	sc = SFSplineCharCreate(sf);
	sc->unicodeenc = dummy.unicodeenc;
	sc->name = copy(dummy.name);
	sc->width = dummy.width;
	sc->vwidth = dummy.vwidth;
	sc->orig_pos = 0xffff;
	if ( sf->cidmaster!=NULL )
	    sc->altuni = CIDSetAltUnis(FindCidMap(sf->cidmaster->cidregistry,sf->cidmaster->ordering,sf->cidmaster->supplement,sf->cidmaster),enc);
	/*SCLigDefault(sc);*/
	SFAddGlyphAndEncode(sf,sc,map,enc);
    }
return( sc );
}

SplineChar *SFMakeChar(SplineFont *sf,EncMap *map, int enc) {
    int gid;

    if ( enc==-1 )
return( NULL );
    if ( enc>=map->enccount )
	gid = -1;
    else
	gid = map->map[enc];
    if ( sf->mm!=NULL && (gid==-1 || sf->glyphs[gid]==NULL) ) {
	int j;
	_SFMakeChar(sf->mm->normal,map,enc);
	for ( j=0; j<sf->mm->instance_count; ++j )
	    _SFMakeChar(sf->mm->instances[j],map,enc);
    }
return( _SFMakeChar(sf,map,enc));
}

struct unicoderange specialnames[] = {
    UNICODERANGE_EMPTY
};

int NameToEncoding(SplineFont *sf,EncMap *map,const char *name) {
    int enc, uni, i, ch;
    char *end, *freeme=NULL;
    const char *upt = name;

    ch = utf8_ildb(&upt);
    if ( *upt=='\0' ) {
	enc = SFFindSlot(sf,map,ch,NULL);
	if ( enc!=-1 )
return( enc );
    }

    enc = uni = -1;

    enc = SFFindSlot(sf,map,-1,name);
    if ( enc!=-1 ) {
	free(freeme);
return( enc );
    }
    if ( (*name=='U' || *name=='u') && name[1]=='+' ) {
	uni = strtol(name+2,&end,16);
	if ( *end!='\0' )
	    uni = -1;
    } else if ( name[0]=='u' && name[1]=='n' && name[2]=='i' ) {
	uni = strtol(name+3,&end,16);
	if ( *end!='\0' )
	    uni = -1;
    } else if ( name[0]=='g' && name[1]=='l' && name[2]=='y' && name[3]=='p' && name[4]=='h' ) {
	int orig = strtol(name+5,&end,10);
	if ( *end!='\0' )
	    orig = -1;
	if ( orig!=-1 )
	    enc = map->backmap[orig];
    } else if ( isdigit(*name)) {
	enc = strtoul(name,&end,0);
	if ( *end!='\0' )
	    enc = -1;
	if ( map->remap!=NULL && enc!=-1 ) {
	    struct remap *remap = map->remap;
	    while ( remap->infont!=-1 ) {
		if ( enc>=remap->firstenc && enc<=remap->lastenc ) {
		    enc += remap->infont-remap->firstenc;
	    break;
		}
		++remap;
	    }
	}
    } else {
	if ( enc==-1 ) {
	    uni = UniFromName(name,sf->uni_interp,map->enc);
	    if ( uni<0 ) {
		for ( i=0; specialnames[i].name!=NULL; ++i )
		    if ( strcmp(name,specialnames[i].name)==0 ) {
			uni = specialnames[i].first;
		break;
		    }
	    }
	    if ( uni<0 && name[1]=='\0' )
		uni = name[0];
	}
    }
    if ( enc>=map->enccount || enc<0 )
	enc = -1;
    if ( enc==-1 && uni!=-1 )
	enc = SFFindSlot(sf,map,uni,NULL);
    /* Used to have code to remove dotted variant names and apply extensions */
    /*  like ".initial" to get the unicode for arabic init/medial/final variants */
    /*  But that doesn't sound like a good idea. And it would also map "a.sc" */
    /*  to "a" -- which was confusing */
return( enc );
}

void SFRemoveUndoes(SplineFont *sf,uint8 *selected, EncMap *map) {
    SplineFont *main = sf->cidmaster? sf->cidmaster : sf, *ssf;
    int i,k, max, layer, gid;
    SplineChar *sc;
    BDFFont *bdf;

    if ( selected!=NULL || main->subfontcnt==0 )
	max = sf->glyphcnt;
    else {
	max = 0;
	for ( k=0; k<main->subfontcnt; ++k )
	    if ( main->subfonts[k]->glyphcnt>max ) max = main->subfonts[k]->glyphcnt;
    }
    for ( i=0; ; ++i ) {
	if ( selected==NULL || main->subfontcnt!=0 ) {
	    if ( i>=max )
    break;
	    gid = i;
	} else {
	    if ( i>=map->enccount )
    break;
	    if ( !selected[i])
    continue;
	    gid = map->map[i];
	    if ( gid==-1 )
    continue;
	}
	for ( bdf=main->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->glyphs[gid]!=NULL ) {
		UndoesFree(bdf->glyphs[gid]->undoes); bdf->glyphs[gid]->undoes = NULL;
		UndoesFree(bdf->glyphs[gid]->redoes); bdf->glyphs[gid]->redoes = NULL;
	    }
	}
	k = 0;
	do {
	    ssf = main->subfontcnt==0? main: main->subfonts[k];
	    if ( gid<ssf->glyphcnt && ssf->glyphs[gid]!=NULL ) {
		sc = ssf->glyphs[gid];
		for ( layer = 0; layer<sc->layer_cnt; ++layer ) {
		    UndoesFree(sc->layers[layer].undoes); sc->layers[layer].undoes = NULL;
		    UndoesFree(sc->layers[layer].redoes); sc->layers[layer].redoes = NULL;
		}
	    }
	    ++k;
	} while ( k<main->subfontcnt );
    }
}

static void _SplineFontSetUnChanged(SplineFont *sf) {
    int i;
    int was = sf->changed;
    BDFFont *bdf;

    sf->changed = false;
    SFClearAutoSave(sf);
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	if ( sf->glyphs[i]->changed ) {
	    sf->glyphs[i]->changed = false;
	    SCRefreshTitles(sf->glyphs[i]);
	}
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL )
	    bdf->glyphs[i]->changed = false;
    if ( was )
	FVRefreshAll(sf);
    if ( was )
	FVSetTitles(sf);
    for ( i=0; i<sf->subfontcnt; ++i )
	_SplineFontSetUnChanged(sf->subfonts[i]);
}

void SplineFontSetUnChanged(SplineFont *sf) {
    int i;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    if ( sf->mm!=NULL ) sf = sf->mm->normal;
    _SplineFontSetUnChanged(sf);
    if ( sf->mm!=NULL )
	for ( i=0; i<sf->mm->instance_count; ++i )
	    _SplineFontSetUnChanged(sf->mm->instances[i]);
}

static char *scaleString(char *string, double scale) {
    char *result;
    char *pt;
    char *end;
    double val;

    if ( string==NULL )
return( NULL );

    while ( *string==' ' ) ++string;
    result = malloc(10*strlen(string)+1);
    if ( *string!='[' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free( result );
return( NULL );
	}
	sprintf( result, "%g", val*scale);
return( result );
    }

    pt = result;
    *pt++ = '[';
    ++string;
    while ( *string!='\0' && *string!=']' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free(result);
return( NULL );
	}
	sprintf( pt, "%g ", val*scale);
	pt += strlen(pt);
	string = end;
    }
    if ( pt[-1] == ' ' ) pt[-1] = ']';
    else *pt++ = ']';
    *pt = '\0';
return( result );
}

static char *iscaleString(char *string, double scale) {
    char *result;
    char *pt;
    char *end;
    double val;

    if ( string==NULL )
return( NULL );

    while ( *string==' ' ) ++string;
    result = malloc(10*strlen(string)+1);
    if ( *string!='[' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free( result );
return( NULL );
	}
	sprintf( result, "%g", rint(val*scale));
return( result );
    }

    pt = result;
    *pt++ = '[';
    ++string;
    while ( *string!='\0' && *string!=']' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free(result);
return( NULL );
	}
	sprintf( pt, "%g ", rint(val*scale));
	pt += strlen(pt);
	string = end;
	while ( *string==' ' ) ++string;
    }
    if ( pt[-1] == ' ' ) pt[-1] = ']';
    else *pt++ = ']';
    *pt = '\0';
return( result );
}

static void SFScalePrivate(SplineFont *sf,double scale) {
    static char *scalethese[] = {
	NULL
    };
    static char *integerscalethese[] = {
	"BlueValues", "OtherBlues",
	"FamilyBlues", "FamilyOtherBlues",
	"BlueShift", "BlueFuzz",
	"StdHW", "StdVW", "StemSnapH", "StemSnapV",
	NULL
    };
    int i;

    for ( i=0; integerscalethese[i]!=NULL; ++i ) {
	char *str = PSDictHasEntry(sf->private,integerscalethese[i]);
	char *new = iscaleString(str,scale);
	if ( new!=NULL )
	    PSDictChangeEntry(sf->private,integerscalethese[i],new);
	free(new);
    }
    for ( i=0; scalethese[i]!=NULL; ++i ) {
	char *str = PSDictHasEntry(sf->private,scalethese[i]);
	char *new = scaleString(str,scale);
	if ( new!=NULL )
	    PSDictChangeEntry(sf->private,scalethese[i],new);
	free(new);
    }
}

static void ScaleBase(struct Base *base, double scale) {
    struct basescript *bs;
    struct baselangextent *bl, *feat;
    int i;

    for ( bs=base->scripts; bs!=NULL; bs=bs->next ) {
	for ( i=0 ; i<base->baseline_cnt; ++i )
	    bs->baseline_pos[i] = (int) rint(bs->baseline_pos[i]*scale);
	for ( bl = bs->langs; bl!=NULL; bl=bl->next ) {
	    bl->ascent  = (int) rint( scale*bl->ascent );
	    bl->descent = (int) rint( scale*bl->descent );
	    for ( feat = bl->features; feat!=NULL; feat = feat->next ) {
		feat->ascent  = (int) rint( scale*feat->ascent );
		feat->descent = (int) rint( scale*feat->descent );
	    }
	}
    }
}

int SFScaleToEm(SplineFont *sf, int as, int des) {
    bigreal scale;
    real transform[6];
    BVTFunc bvts;
    uint8 *oldselected = sf->fv->selected;
    enum fvtrans_flags trans_flags =
	fvt_alllayers|fvt_round_to_int|fvt_dontsetwidth|fvt_scalekernclasses|fvt_scalepstpos|fvt_dogrid;

    scale = (as+des)/(bigreal) (sf->ascent+sf->descent);
    sf->pfminfo.hhead_ascent = rint( sf->pfminfo.hhead_ascent * scale);
    sf->pfminfo.hhead_descent = rint( sf->pfminfo.hhead_descent * scale);
    sf->pfminfo.linegap = rint( sf->pfminfo.linegap * scale);
    sf->pfminfo.vlinegap = rint( sf->pfminfo.vlinegap * scale);
    sf->pfminfo.os2_winascent = rint( sf->pfminfo.os2_winascent * scale);
    sf->pfminfo.os2_windescent = rint( sf->pfminfo.os2_windescent * scale);
    sf->pfminfo.os2_typoascent = rint( sf->pfminfo.os2_typoascent * scale);
    sf->pfminfo.os2_typodescent = rint( sf->pfminfo.os2_typodescent * scale);
    sf->pfminfo.os2_typolinegap = rint( sf->pfminfo.os2_typolinegap * scale);

    sf->pfminfo.os2_subxsize = rint( sf->pfminfo.os2_subxsize * scale);
    sf->pfminfo.os2_subysize = rint( sf->pfminfo.os2_subysize * scale);
    sf->pfminfo.os2_subxoff = rint( sf->pfminfo.os2_subxoff * scale);
    sf->pfminfo.os2_subyoff = rint( sf->pfminfo.os2_subyoff * scale);
    sf->pfminfo.os2_supxsize = rint( sf->pfminfo.os2_supxsize * scale);
    sf->pfminfo.os2_supysize = rint(sf->pfminfo.os2_supysize *  scale);
    sf->pfminfo.os2_supxoff = rint( sf->pfminfo.os2_supxoff * scale);
    sf->pfminfo.os2_supyoff = rint( sf->pfminfo.os2_supyoff * scale);
    sf->pfminfo.os2_strikeysize = rint( sf->pfminfo.os2_strikeysize * scale);
    sf->pfminfo.os2_strikeypos = rint( sf->pfminfo.os2_strikeypos * scale);
    sf->pfminfo.os2_xheight = rint( sf->pfminfo.os2_xheight * scale);
    sf->pfminfo.os2_capheight = rint( sf->pfminfo.os2_capheight * scale);
    sf->upos *= scale;
    sf->uwidth *= scale;
    sf->ufo_ascent *= scale;
    sf->ufo_descent *= scale;

    if ( sf->private!=NULL )
	SFScalePrivate(sf,scale);
    if ( sf->horiz_base!=NULL )
	ScaleBase(sf->horiz_base, scale);
    if ( sf->vert_base!=NULL )
	ScaleBase(sf->vert_base, scale);

    if ( as+des == sf->ascent+sf->descent ) {
	if ( as!=sf->ascent && des!=sf->descent ) {
	    sf->ascent = as; sf->descent = des;
	    sf->changed = true;
	}
return( false );
    }

    transform[0] = transform[3] = scale;
    transform[1] = transform[2] = transform[4] = transform[5] = 0;
    bvts.func = bvt_none;
    sf->fv->selected = malloc(sf->fv->map->enccount);
    memset(sf->fv->selected,1,sf->fv->map->enccount);

    sf->ascent = as; sf->descent = des;

    /* If someone has set an absurdly small em size, try to contain
       the damage by not rounding to int. */
    if ((as+des) < 32) {
	trans_flags &= ~fvt_round_to_int;
    }

    FVTransFunc(sf->fv,transform,0,&bvts,trans_flags);
    free(sf->fv->selected);
    sf->fv->selected = oldselected;

    if ( !sf->changed ) {
	sf->changed = true;
	FVSetTitles(sf);
    }

return( true );
}

void SFSetModTime(SplineFont *sf) {
    sf->modificationtime = GetTime();
}

static SplineFont *_SFReadPostScript(FILE *file,char *filename) {
    FontDict *fd=NULL;
    SplineFont *sf=NULL;

    ff_progress_change_stages(2);
    fd = _ReadPSFont(file);
    ff_progress_next_stage();
    ff_progress_change_line2(_("Interpreting Glyphs"));
    if ( fd!=NULL ) {
	sf = SplineFontFromPSFont(fd);
	PSFontFree(fd);
	if ( sf!=NULL )
	    CheckAfmOfPostScript(sf,filename);
    }
return( sf );
}

static SplineFont *SFReadPostScript(char *filename) {
    FontDict *fd=NULL;
    SplineFont *sf=NULL;

    ff_progress_change_stages(2);
    fd = ReadPSFont(filename);
    ff_progress_next_stage();
    ff_progress_change_line2(_("Interpreting Glyphs"));
    if ( fd!=NULL ) {
	sf = SplineFontFromPSFont(fd);
	PSFontFree(fd);
	if ( sf!=NULL )
	    CheckAfmOfPostScript(sf,filename);
    }
return( sf );
}

struct archivers archivers[] = {
    { ".tar", "tar", "tar", "tf", "xf", "rf", ars_tar },
    { ".tgz", "tar", "tar", "tfz", "xfz", "rfz", ars_tar },
    { ".tar.gz", "tar", "tar", "tfz", "xfz", "rfz", ars_tar },
    { ".tar.bz2", "tar", "tar", "tfj", "xfj", "rfj", ars_tar },
    { ".tbz2", "tar", "tar", "tfj", "xfj", "rfj", ars_tar },
    { ".tbz", "tar", "tar", "tfj", "xfj", "rfj", ars_tar },
    { ".zip", "unzip", "zip", "-l", "", "", ars_zip },
    /* { ".tar.lzma", ? } */
    ARCHIVERS_EMPTY
};

void ArchiveCleanup(char *archivedir) {
    /* Free this directory and all files within it */
    GFileRemove(archivedir, true);
    free(archivedir);
}

static char *ArchiveParseTOC(char *listfile, enum archive_list_style ars, int *doall) {
    FILE *file;
    int nlcnt, ch, linelen, linelenmax, fcnt, choice, i, def, def_prio, prio;
    char **files, *linebuffer, *pt, *name;

    *doall = false;
    file = fopen(listfile,"r");
    if ( file==NULL )
return( NULL );

    nlcnt=linelenmax=linelen=0;
    while ( (ch=getc(file))!=EOF ) {
	if ( ch=='\n' ) {
	    ++nlcnt;
	    if ( linelen>linelenmax ) linelenmax = linelen;
	    linelen = 0;
	} else
	    ++linelen;
    }
    rewind(file);

    /* tar outputs its table of contents as a simple list of names */
    /* zip includes a bunch of other info, headers (and lines for directories)*/

    linebuffer = malloc(linelenmax+3);
    fcnt = 0;
    files = malloc((nlcnt+1)*sizeof(char *));

    if ( ars == ars_tar ) {
	pt = linebuffer;
	while ( (ch=getc(file))!=EOF ) {
	    if ( ch=='\n' ) {
		*pt = '\0';
		/* Blessed if I know what encoded was used for filenames */
		/*  inside the tar file. I shall assume utf8, faut de mieux */
		files[fcnt++] = copy(linebuffer);
		pt = linebuffer;
	    } else
		*pt++ = ch;
	}
    } else {
	/* Skip the first three lines, header info */
	fgets(linebuffer,linelenmax+3,file);
	fgets(linebuffer,linelenmax+3,file);
	fgets(linebuffer,linelenmax+3,file);
	pt = linebuffer;
	while ( (ch=getc(file))!=EOF ) {
	    if ( ch=='\n' ) {
		*pt = '\0';
		if ( linebuffer[0]==' ' && linebuffer[1]=='-' && linebuffer[2]=='-' )
	break;		/* End of file list */
		/* Blessed if I know what encoded was used for filenames */
		/*  inside the zip file. I shall assume utf8, faut de mieux */
		if ( pt-linebuffer>=28 && pt[-1]!='/' )
		    files[fcnt++] = copy(linebuffer+28);
		pt = linebuffer;
	    } else
		*pt++ = ch;
	}
    }
    files[fcnt] = NULL;
    fclose(file);

    free(linebuffer);
    if ( fcnt==0 ) {
	free(files);
return( NULL );
    } else if ( fcnt==1 ) {
	char *onlyname = files[0];
	free(files);
return( onlyname );
    }

    /* Suppose they've got an archive of a directory format font? I mean a ufo*/
    /*  or a sfdir. It won't show up in the list of files (because either     */
    /*  tar or I have removed all directories from that list) */
    pt = strrchr(files[0],'/');
    if ( pt!=NULL ) {
	if (( pt-files[0]>4 && (strncasecmp(pt-4,".ufo",4)==0 || strncasecmp(pt-4,"_ufo",4)==0)) ||
 		( pt-files[0]>5 && (strncasecmp(pt-5,".ufo2",5)==0 || strncasecmp(pt-5,"_ufo2",5)==0)) ||
 		( pt-files[0]>5 && (strncasecmp(pt-5,".ufo3",5)==0 || strncasecmp(pt-5,"_ufo3",5)==0)) ||
		( pt-files[0]>6 && (strncasecmp(pt-6,".sfdir",6)==0 || strncasecmp(pt-6,"_sfdir",6)==0)) ) {
	    /* Ok, looks like a potential directory font. Now is EVERYTHING */
	    /*  in the archive inside this guy? */
	    for ( i=0; i<fcnt; ++i )
		if ( strncmp(files[i],files[0],pt-files[0]+1)!=0 )
	    break;
	    if ( i==fcnt ) {
		char *onlydirfont = copyn(files[0],pt-files[0]+1);
		for ( i=0; i<fcnt; ++i )
		    free(files[i]);
		free(files);
		*doall = true;
return( onlydirfont );
	    }
	}
    }

    def=0; def_prio = -1;
    for ( i=0; i<fcnt; ++i ) {
	pt = strrchr(files[i],'.');
	if ( pt==NULL )
    continue;
	if ( strcasecmp(pt,".svg")==0 )
	    prio = 10;
	else if ( strcasecmp(pt,".pfb")==0 || strcasecmp(pt,".pfa")==0 ||
		strcasecmp(pt,".cff")==0 || strcasecmp(pt,".cid")==0 )
	    prio = 20;
	else if ( strcasecmp(pt,".otf")==0 || strcasecmp(pt,".ttf")==0 || strcasecmp(pt,".ttc")==0 )
	    prio = 30;
	else if ( strcasecmp(pt,".sfd")==0 )
	    prio = 40;
	else
    continue;
	if ( prio>def_prio ) {
	    def = i;
	    def_prio = prio;
	}
    }

    choice = ff_choose(_("Which archived item should be opened?"),(const char **) files,fcnt,def,_("There are multiple files in this archive, pick one"));
    if ( choice==-1 )
	name = NULL;
    else
	name = copy(files[choice]);

    for ( i=0; i<fcnt; ++i )
	free(files[i]);
    free(files);
return( name );
}

#define TOC_NAME	"ff-archive-table-of-contents"

char *Unarchive(char *name, char **_archivedir) {
    char *dir = getenv("TMPDIR");
    char *pt, *archivedir, *listfile, *listcommand, *unarchivecmd, *desiredfile;
    char *finalfile;
    int i;
    int doall=false;
    static int cnt=0;

    *_archivedir = NULL;

    pt = strrchr(name,'.');
    if ( pt==NULL )
return( NULL );
    for ( i=0; archivers[i].ext!=NULL; ++i )
	if ( strcmp(archivers[i].ext,pt)==0 )
    break;
    if ( archivers[i].ext==NULL ) {
	/* some of those endings have two extensions... */
	while ( pt>name && pt[-1]!='.' )
	    --pt;
	if ( pt==name )
return( NULL );
	--pt;
	for ( i=0; archivers[i].ext!=NULL; ++i )
	    if ( strcmp(archivers[i].ext,pt)==0 )
	break;
	if ( archivers[i].ext==NULL )
return( NULL );
    }

    if ( dir==NULL ) dir = P_tmpdir;
    archivedir = malloc(strlen(dir)+100);
    sprintf( archivedir, "%s/ffarchive-%d-%d", dir, getpid(), ++cnt );
    if ( GFileMkDir(archivedir, 0755)!=0 ) {
	free(archivedir);
return( NULL );
    }

    listfile = malloc(strlen(archivedir)+strlen("/" TOC_NAME)+1);
    sprintf( listfile, "%s/" TOC_NAME, archivedir );

    listcommand = malloc( strlen(archivers[i].unarchive) + 1 +
			strlen( archivers[i].listargs) + 1 +
			strlen( name ) + 3 +
			strlen( listfile ) +4 );
    sprintf( listcommand, "%s %s %s > %s", archivers[i].unarchive,
	    archivers[i].listargs, name, listfile );
    if ( system(listcommand)!=0 ) {
	free(listcommand); free(listfile);
	ArchiveCleanup(archivedir);
return( NULL );
    }
    free(listcommand);

    desiredfile = ArchiveParseTOC(listfile, archivers[i].ars, &doall);
    free(listfile);
    if ( desiredfile==NULL ) {
	ArchiveCleanup(archivedir);
return( NULL );
    }

    /* I tried sending everything to stdout, but that doesn't work if the */
    /*  output is a directory file (ufo, sfdir) */
    unarchivecmd = malloc( strlen(archivers[i].unarchive) + 1 +
			strlen( archivers[i].listargs) + 1 +
			strlen( name ) + 1 +
			strlen( desiredfile ) + 3 +
			strlen( archivedir ) + 30 );
    sprintf( unarchivecmd, "( cd %s ; %s %s %s %s ) > /dev/null", archivedir,
	    archivers[i].unarchive,
	    archivers[i].extractargs, name, doall ? "" : desiredfile );
    if ( system(unarchivecmd)!=0 ) {
	free(unarchivecmd); free(desiredfile);
	ArchiveCleanup(archivedir);
return( NULL );
    }
    free(unarchivecmd);

    finalfile = malloc( strlen(archivedir) + 1 + strlen(desiredfile) + 1);
    sprintf( finalfile, "%s/%s", archivedir, desiredfile );
    free( desiredfile );

    *_archivedir = archivedir;
return( finalfile );
}

struct compressors compressors[] = {
    { ".gz", "gunzip", "gzip" },
    { ".bz2", "bunzip2", "bzip2" },
    { ".bz", "bunzip2", "bzip2" },
    { ".Z", "gunzip", "compress" },
    { ".lzma", "unlzma", "lzma" },
/* file types which are both archived and compressed (.tgz, .zip) are handled */
/*  by the archiver above */
    COMPRESSORS_EMPTY
};

char *Decompress(char *name, int compression) {
    char *dir = getenv("TMPDIR");
    char buf[1500];
    char *tmpfn;

    if ( dir==NULL ) dir = P_tmpdir;
    tmpfn = malloc(strlen(dir)+strlen(GFileNameTail(name))+2);
    strcpy(tmpfn,dir);
    strcat(tmpfn,"/");
    strcat(tmpfn,GFileNameTail(name));
    *strrchr(tmpfn,'.') = '\0';
    snprintf( buf, sizeof(buf), "%s < %s > %s", compressors[compression].decomp, name, tmpfn );
    if ( system(buf)==0 )
return( tmpfn );
    free(tmpfn);
return( NULL );
}

static char *ForceFileToHaveName(FILE *file, char *exten) {
    char tmpfilename[L_tmpnam+100];
    static int try=0;
    FILE *newfile;

    for (;;) {
	sprintf( tmpfilename, P_tmpdir "/fontforge%d-%d", getpid(), try++ );
	if ( exten!=NULL )
	    strcat(tmpfilename,exten);
	if ( access( tmpfilename, F_OK )==-1 &&
		(newfile = fopen(tmpfilename,"w"))!=NULL ) {
	    char buffer[1024];
	    int len;
	    while ( (len = fread(buffer,1,sizeof(buffer),file))>0 )
		fwrite(buffer,1,len,newfile);
	    fclose(newfile);
	}
return(copy(tmpfilename));			/* The filename does not exist */
    }
}

/* Returns a pointer to the start of the parenthesized 
 * subfont name (optionally) used when opening TTC or Macbinary
 * files, or NULL if no subfont is specified.
 *
 * The subfont specification must extend to the end of the string
 * so that font filenames containing parentheses will not be
 * misinterpreted. 
 *
 * Because the name *must* be in parentheses, a non-NULL pointer
 * will be to a string of at least two characters.
 * 
 * The following won't work if the fontname has unbalanced
 * parentheses, but short of requiring those to be escaped
 * somehow that case is tricky. (One could guess the cutoff
 * by looking for a file extension.)
 * */
char *SFSubfontnameStart(char *fname) {
    char *rparen, *paren;
    int cnt = 1;
    if ( fname==NULL || (rparen = strrchr(fname,')'))==NULL || rparen[1]!='\0' )
	return NULL;
    paren = rparen;
    while ( --paren>=fname ) {
	if ( *paren == '(' )
	    --cnt;
	else if ( *paren == ')' ) 
	    ++cnt;
	if ( cnt==0 )
	    return paren;
    }
    return NULL;
}

/* This does not check currently existing fontviews, and should only be used */
/*  by LoadSplineFont (which does) and by RevertFile (which knows what it's doing) */
SplineFont *_ReadSplineFont(FILE *file, const char *filename, enum openflags openflags) {
    SplineFont *sf;
    char ubuf[251], *temp;
    int fromsfd = false;
    int i;
    char *pt, *ext2, *strippedname = NULL, *chosenname = NULL, *oldstrippedname = NULL;
    char *tmpfn=NULL, *paren=NULL, *fullname;
    char *archivedir=NULL;
    int len;
    int checked;
    int compression=0;
    int nowlocal = true, wasarchived=false;
    char * fname = copy(filename);
    fullname = fname;

    if ( filename==NULL ) return NULL;

    // Some explorers (PCManFM) may pass a file:// URL, special case it
    // here so that we don't defer to the HTTP code.
    if ( strncmp(filename,"file://",7)==0 ) {
        fname = g_uri_unescape_string(filename+7, NULL);
        free(fullname);
        fullname = fname;
    }

    // treat /whatever/foo.ufo/ as simply /whatever/foo.ufo
    len = strlen(filename);
    if( len>0 && filename[ len-1 ] == '/' ) {
	fname[len-1] = '\0';
    }

    strippedname = fname;
    pt = strrchr(fname,'/');
    if ( pt==NULL ) pt = fname;

    if ( (paren = SFSubfontnameStart(pt)) ) {
	strippedname = copy(fname);
	strippedname[paren-fname] = '\0';
	chosenname = copy(paren+1);
	chosenname[strlen(chosenname)-1] = '\0';
    }

    pt = strrchr(strippedname,'.');
    if ( pt!=NULL ) {
	for ( ext2 = pt-1; ext2>strippedname && *ext2!='.'; --ext2 );
	for ( i=0; archivers[i].ext!=NULL; ++i ) {
	    /* some of the archive "extensions" are actually two like ".tar.bz2" */
	    if ( strcmp(archivers[i].ext,pt)==0 || strcmp(archivers[i].ext,ext2)==0 ) {
		if ( file!=NULL ) {
		    char *spuriousname = ForceFileToHaveName(file,archivers[i].ext);
		    strippedname = Unarchive(spuriousname,&archivedir);
		    fclose(file); file = NULL;
		    unlink(spuriousname); free(spuriousname);
		} else
		    strippedname = Unarchive(strippedname,&archivedir);
		if ( strippedname==NULL )
            return NULL;
		if ( strippedname!=fname && paren!=NULL ) {
		    fullname = malloc(strlen(strippedname)+strlen(paren)+1);
		    strcpy(fullname,strippedname);
		    strcat(fullname,paren);
		} else
		    fullname = strippedname;
		pt = strrchr(strippedname,'.');
		wasarchived = true;
	    break;
	    }
	}
    }

    i = -1;
    if ( pt!=NULL ) for ( i=0; compressors[i].ext!=NULL; ++i )
	if ( strcmp(compressors[i].ext,pt)==0 )
    break;
    oldstrippedname = strippedname;
    if ( i==-1 || compressors[i].ext==NULL )
	i=-1;
    else {
	if ( file!=NULL ) {
	    char *spuriousname = ForceFileToHaveName(file,compressors[i].ext);
	    tmpfn = Decompress(spuriousname,i);
	    fclose(file); file = NULL;
	    unlink(spuriousname); free(spuriousname);
	} else
	    tmpfn = Decompress(strippedname,i);
	if ( tmpfn!=NULL ) {
	    strippedname = tmpfn;
	} else {
	    ff_post_error(_("Decompress Failed!"),_("Decompress Failed!"));
	    ArchiveCleanup(archivedir);
        return NULL;
	}
	compression = i+1;
	if ( strippedname!=fname && paren!=NULL ) {
	    fullname = malloc(strlen(strippedname)+strlen(paren)+1);
	    strcpy(fullname,strippedname);
	    strcat(fullname,paren);
	} else
	    fullname = strippedname;
    }

    /* If there are no pfaedit windows, give them something to look at */
    /*  immediately. Otherwise delay a bit */
    strncpy(ubuf,_("Loading font from "),sizeof(ubuf)-1);
    len = strlen(ubuf);
    if ( i==-1 )	/* If it wasn't compressed then the fullname is reasonable, else use the original name */
	    strncat(ubuf,temp = def2utf8_copy(GFileNameTail(fullname)),100);
    else
	    strncat(ubuf,temp = def2utf8_copy(GFileNameTail(fname)),100);
    free(temp);
    ubuf[100+len] = '\0';
    ff_progress_start_indicator(FontViewFirst()==NULL?0:10,_("Loading..."),ubuf,_("Reading Glyphs"),0,1);
    ff_progress_enable_stop(0);
    if ( FontViewFirst()==NULL && !no_windowing_ui )
	    ff_progress_allow_events();

    if ( file==NULL ) {
	    file = fopen(strippedname,"rb");
	    nowlocal = true;
    }

    sf = NULL;
    checked = false;
/* checked == false => not checked */
/* checked == 'u'   => UFO */
/* checked == 't'   => TTF/OTF */
/* checked == 'p'   => pfb/general postscript */
/* checked == 'P'   => pdf */
/* checked == 'c'   => cff */
/* checked == 'S'   => svg */
/* checked == 'f'   => sfd */
/* checked == 'F'   => sfdir */
/* checked == 'b'   => bdf */
/* checked == 'i'   => ikarus */
    if ( GFileIsDir(strippedname) ) {
	char *temp = malloc(strlen(strippedname)+strlen("/glyphs/contents.plist")+1);
	strcpy(temp,strippedname);
	strcat(temp,"/glyphs/contents.plist");
	if ( GFileExists(temp)) {
	    sf = SFReadUFO(strippedname,0);
	    checked = 'u';
	} else {
	    strcpy(temp,strippedname);
	    strcat(temp,"/font.props");
	    if ( GFileExists(temp)) {
		    sf = SFDirRead(strippedname);
		    checked = 'F';
	    }
	}
	free(temp);
	if ( file!=NULL )
	    fclose(file);
    } else if ( file!=NULL ) {
	/* Try to guess the file type from the first few characters... */
	int ch1 = getc(file);
	int ch2 = getc(file);
	int ch3 = getc(file);
	int ch4 = getc(file);
	int ch5 = getc(file);
	int ch6 = getc(file);
	int ch7 = getc(file);
	int ch9, ch10;
	fseek(file, 98, SEEK_SET);
	ch9 = getc(file);
	ch10 = getc(file);
	rewind(file);
	if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		(ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		(ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ||
		(ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f') ) {
	    sf = _SFReadTTF(file,0,openflags,strippedname,chosenname,NULL);
	    checked = 't';
	} else if ( ch1=='w' && ch2=='O' && ch3=='F' && ch4=='F' ) {
	    sf = _SFReadWOFF(file,0,openflags,strippedname,chosenname,NULL);
	    checked = 'w';
	} else if ( ch1=='w' && ch2=='O' && ch3=='F' && ch4=='2' ) {
#ifdef FONTFORGE_CAN_USE_WOFF2
	    sf = _SFReadWOFF2(file,0,openflags,strippedname,chosenname,NULL);
	    checked = 'w';
#endif
	} else if (( ch1=='%' && ch2=='!' ) ||
		    ( ch1==0x80 && ch2=='\01' ) ) {	/* PFB header */
	    sf = _SFReadPostScript(file,fullname);
	    checked = 'p';
	} else if ( ch1=='%' && ch2=='P' && ch3=='D' && ch4=='F' ) {
	    sf = _SFReadPdfFont(file,fullname,openflags);
	    checked = 'P';
	} else if ( ch1==1 && ch2==0 && ch3==4 ) {
	    int len;
	    fseek(file,0,SEEK_END);
	    len = ftell(file);
	    fseek(file,0,SEEK_SET);
	    sf = _CFFParse(file,len,NULL);
	    checked = 'c';
	} else if (( ch1=='<' && ch2=='?' && (ch3=='x'||ch3=='X') && (ch4=='m'||ch4=='M') ) ||
		/* or UTF-8 SVG with initial byte ordering mark */
			 (( ch1==0xef && ch2==0xbb && ch3==0xbf &&
			    ch4=='<' && ch5=='?' && (ch6=='x'||ch6=='X') && (ch7=='m'||ch7=='M') )) ) {
	    if ( nowlocal )
		sf = SFReadSVG(fullname,0);
	    else {
		char *spuriousname = ForceFileToHaveName(file,NULL);
		sf = SFReadSVG(spuriousname,0);
		unlink(spuriousname); free(spuriousname);
	    }
	    checked = 'S';
	} else if ( ch1=='S' && ch2=='p' && ch3=='l' && ch4=='i' ) {
	    sf = _SFDRead(fullname,file); file = NULL;
	    checked = 'f';
	    fromsfd = true;
	} else if ( ch1=='S' && ch2=='T' && ch3=='A' && ch4=='R' ) {
	    sf = SFFromBDF(fullname,0,false);
	    checked = 'b';
	} else if ( ch1=='\1' && ch2=='f' && ch3=='c' && ch4=='p' ) {
	    sf = SFFromBDF(fullname,2,false);
	} else if ( ch9=='I' && ch10=='K' && ch3==0 && ch4==55 ) {
	    /* Ikarus font type appears at word 50 (byte offset 98) */
	    /* Ikarus name section length (at word 2, byte offset 2) was 55 in the 80s at URW */
	    checked = 'i';
	    sf = SFReadIkarus(fullname);
	} /* Too hard to figure out a valid mark for a mac resource file */
	if ( file!=NULL ) fclose(file);
    }

    if ( sf!=NULL )
        /* good */;
    else if (( strmatch(fullname+strlen(fullname)-4, ".sfd")==0 ||
	      strmatch(fullname+strlen(fullname)-5, ".sfd~")==0 ) && checked!='f' ) {
	    sf = SFDRead(fullname);
	    fromsfd = true;
    } else if (( strmatch(fullname+strlen(fullname)-4, ".ttf")==0 ||
		  strmatch(fullname+strlen(strippedname)-4, ".ttc")==0 ||
		  strmatch(fullname+strlen(fullname)-4, ".gai")==0 ||
		  strmatch(fullname+strlen(fullname)-4, ".otf")==0 ||
		  strmatch(fullname+strlen(fullname)-4, ".otb")==0 ) && checked!='t') {
	    sf = SFReadTTF(fullname,0,openflags);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".svg")==0 && checked!='S' ) {
	    sf = SFReadSVG(fullname,0);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".ufo")==0 && checked!='u' ||
		 strmatch(fullname+strlen(fullname)-5, ".ufo2")==0 && checked!='u' ||
		 strmatch(fullname+strlen(fullname)-5, ".ufo3")==0 && checked!='u' ) {
	    sf = SFReadUFO(fullname,0);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".bdf")==0 && checked!='b' ) {
	    sf = SFFromBDF(fullname,0,false);
    } else if ( strmatch(fullname+strlen(fullname)-2, "pk")==0 ) {
	    sf = SFFromBDF(fullname,1,true);
    } else if ( strmatch(fullname+strlen(fullname)-2, "gf")==0 ) {
	    sf = SFFromBDF(fullname,3,true);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".pcf")==0 ||
		 strmatch(fullname+strlen(fullname)-4, ".pmf")==0 ) {
	/* Sun seems to use a variant of the pcf format which they call pmf */
	/*  the encoding actually starts at 0x2000 and the one I examined was */
	/*  for a pixel size of 200. Some sort of printer font? */
	sf = SFFromBDF(fullname,2,false);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".bin")==0 ||
		  strmatch(fullname+strlen(strippedname)-4, ".hqx")==0 ||
		  strmatch(fullname+strlen(strippedname)-6, ".dfont")==0 ) {
	    sf = SFReadMacBinary(fullname,0,openflags);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".fon")==0 ||
		  strmatch(fullname+strlen(strippedname)-4, ".fnt")==0 ) {
	    sf = SFReadWinFON(fullname,0);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".pdb")==0 ) {
	    sf = SFReadPalmPdb(fullname);
    } else if ( (strmatch(fullname+strlen(fullname)-4, ".pfa")==0 ||
		  strmatch(fullname+strlen(fullname)-4, ".pfb")==0 ||
		  strmatch(fullname+strlen(fullname)-4, ".pf3")==0 ||
		  strmatch(fullname+strlen(fullname)-4, ".cid")==0 ||
		  strmatch(fullname+strlen(fullname)-4, ".gsf")==0 ||
		  strmatch(fullname+strlen(fullname)-4, ".pt3")==0 ||
		  strmatch(fullname+strlen(fullname)-3, ".ps")==0 ) && checked!='p' ) {
	    sf = SFReadPostScript(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".cff")==0 && checked!='c' ) {
	    sf = CFFParse(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-3, ".mf")==0 ) {
	    sf = SFFromMF(fullname);
    } else if ( strmatch(strippedname+strlen(strippedname)-4, ".pdf")==0 && checked!='P' ) {
	    sf = SFReadPdfFont(fullname,openflags);
    } else if ( strmatch(fullname+strlen(fullname)-3, ".ik")==0 && checked!='i' ) {
	    sf = SFReadIkarus(fullname);
    } else {
	    sf = SFReadMacBinary(fullname,0,openflags);
    }
    ff_progress_end_indicator();

    if ( sf!=NULL ) {
	    SplineFont *norm = sf->mm!=NULL ? sf->mm->normal : sf;
	    if ( compression!=0 ) {
	        free(sf->filename);
	        *strrchr(oldstrippedname,'.') = '\0';
	        sf->filename = copy( oldstrippedname );
	    }
	    if ( fromsfd )
	        sf->compression = compression;
	    free( norm->origname );
	    if ( wasarchived ) {
	        norm->origname = NULL;
	        free(norm->filename); norm->filename = NULL;
	        norm->new = true;
	    } else if ( sf->chosenname!=NULL && strippedname==fname ) {
	        norm->origname = malloc(strlen(fname)+strlen(sf->chosenname)+8);
	        strcpy(norm->origname,fname);
	        strcat(norm->origname,"(");
	        strcat(norm->origname,sf->chosenname);
	        strcat(norm->origname,")");
	    } else
	        norm->origname = copy(fname);
	    free( norm->chosenname ); norm->chosenname = NULL;
	    if ( sf->mm!=NULL ) {
	        int j;
	        for ( j=0; j<sf->mm->instance_count; ++j ) {
	    	    free(sf->mm->instances[j]->origname);
	    	    sf->mm->instances[j]->origname = copy(norm->origname);
	        }
	    }
    } else if ( !GFileExists(fname) )
	    ff_post_error(_("Couldn't open font"),_("The requested file, %.100s, does not exist"),GFileNameTail(fname));
    else if ( !GFileReadable(fname) )
	    ff_post_error(_("Couldn't open font"),_("You do not have permission to read %.100s"),GFileNameTail(fname));
    else if ( GFileIsDir(fname) ) {
        LogError(_("Couldn't open directory as a font: %s"), fname);
    } else
	    ff_post_error(_("Couldn't open font"),_("%.100s is not in a known format (or uses features of that format fontforge does not support, or is so badly corrupted as to be unreadable)"),GFileNameTail(filename));

    if ( oldstrippedname!=fname )
	    free(oldstrippedname);
    if ( fullname!=fname && fullname!=strippedname )
	    free(fullname);
    if ( chosenname!=NULL )
	    free(chosenname);
    if ( tmpfn!=NULL ) {
	    unlink(tmpfn);
	    free(tmpfn);
    }
    if ( wasarchived )
	    ArchiveCleanup(archivedir);
    if ( (openflags&of_fstypepermitted) && sf!=NULL && (sf->pfminfo.fstype&0xff)==0x0002 ) {
	    /* Ok, they have told us from a script they have access to the font */
    } else if ( !fromsfd && sf!=NULL && (sf->pfminfo.fstype&0xff)==0x0002 ) {
	    char *buts[3];
	    buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;
	    if ( ff_ask(_("Restricted Font"),(const char **) buts,1,1,_("This font is marked with an FSType of 2 (Restricted\nLicense). That means it is not editable without the\npermission of the legal owner.\n\nDo you have such permission?"))==1 ) {
	        SplineFontFree(sf);
                sf = NULL;
	    }
    }
    if (fname != NULL && fname != filename) free(fname); fname = NULL;
    return sf;
}

SplineFont *ReadSplineFont(const char *filename,enum openflags openflags) {
return( _ReadSplineFont(NULL,filename,openflags));
}

char *ToAbsolute(char *filename) {
    char buffer[1025];

    GFileGetAbsoluteName(filename,buffer,sizeof(buffer));
return( copy(buffer));
}

SplineFont *LoadSplineFont(const char *filename,enum openflags openflags) {
    SplineFont *sf;
    const char *pt;
    char *ept, *tobefreed1=NULL, *tobefreed2=NULL;
    static char *extens[] = { ".sfd", ".pfa", ".pfb", ".ttf", ".otf", ".ps", ".cid", ".bin", ".dfont", ".PFA", ".PFB", ".TTF", ".OTF", ".PS", ".CID", ".BIN", ".DFONT", NULL };
    int i;
    char * fname = NULL;

    if ( filename==NULL )
return( NULL );

    if (( pt = strrchr(filename,'/'))==NULL ) pt = filename;
    if ( strchr(pt,'.')==NULL ) {
	/* They didn't give an extension. If there's a file with no extension */
	/*  see if it's a valid font file (and if so use the extensionless */
	/*  filename), otherwise guess at an extension */
	/* For some reason Adobe distributes CID keyed fonts (both OTF and */
	/*  postscript) as extensionless files */
	int ok = false;
	FILE *test = fopen(filename,"rb");
	if ( test!=NULL ) {
	    ok = true;		/* Mac resource files are too hard to check for */
		    /* If file exists, assume good */
	    fclose(test);
	}
	if ( !ok ) {
	    tobefreed1 = malloc(strlen(filename)+8);
	    strcpy(tobefreed1,filename);
	    ept = tobefreed1+strlen(tobefreed1);
	    for ( i=0; extens[i]!=NULL; ++i ) {
		strcpy(ept,extens[i]);
		if ( GFileExists(tobefreed1))
	    break;
	    }
	    if ( extens[i]!=NULL )
		fname = tobefreed1;
	    else {
		free(tobefreed1);
		fname = tobefreed1 = copy(filename);
	    }
	} else fname = tobefreed1 = copy(filename);
    } else fname = tobefreed1 = copy(filename);

    sf = NULL;
    sf = FontWithThisFilename(fname);
    if ( sf==NULL && *fname!='/' )
	fname = tobefreed2 = ToAbsolute(fname);
    if ( sf==NULL )
	sf = ReadSplineFont(fname,openflags);

    if (tobefreed1 != NULL) free(tobefreed1);
    if (tobefreed2 != NULL) free(tobefreed2);
return( sf );
}

/* Use URW 4 letter abbreviations */
const char *knownweights[] = { "Demi", "Bold", "Regu", "Medi", "Book", "Thin",
	"Ligh", "Heav", "Blac", "Ultr", "Nord", "Norm", "Gras", "Stan", "Halb",
	"Fett", "Mage", "Mitt", "Buch", NULL };
const char *realweights[] = { "Demi", "Bold", "Regular", "Medium", "Book", "Thin",
	"Light", "Heavy", "Black", "Ultra", "Nord", "Normal", "Gras", "Standard", "Halbfett",
	"Fett", "Mager", "Mittel", "Buchschrift", NULL};
static const char *moreweights[] = { "ExtraLight", "VeryLight", NULL };
const char **noticeweights[] = { moreweights, realweights, knownweights, NULL };

static const char *modifierlist[] = { "Ital", "Obli", "Kursive", "Cursive", "Slanted",
	"Expa", "Cond", NULL };
static const char *modifierlistfull[] = { "Italic", "Oblique", "Kursive", "Cursive", "Slanted",
    "Expanded", "Condensed", NULL };
static const char **mods[] = { knownweights, modifierlist, NULL };
static const char **fullmods[] = { realweights, modifierlistfull, NULL };

const char *_GetModifiers(const char *fontname, const char *familyname, const char *weight) {
    const char *pt, *fpt;
    static char space[20];
    int i, j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt == NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }
    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( strcmp(fpt,mods[i][j])==0 ) {
		strncpy(space,fullmods[i][j],sizeof(space)-1);
return(space);
	    }
	}
	if ( strcmp(fpt,"BoldItal")==0 )
return( "BoldItalic" );
	else if ( strcmp(fpt,"BoldObli")==0 )
return( "BoldOblique" );

return( fpt );
    }

return( weight==NULL || *weight=='\0' ? "Regular": weight );
}

const char *SFGetModifiers(const SplineFont *sf) {
return( _GetModifiers(sf->fontname,sf->familyname,sf->weight));
}

const unichar_t *_uGetModifiers(const unichar_t *fontname, const unichar_t *familyname,
	const unichar_t *weight) {
    const unichar_t *pt, *fpt;
    static unichar_t regular[] = { 'R','e','g','u','l','a','r', 0 };
    static unichar_t space[20];
    int i,j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=u_strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt==NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = uc_strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }

    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( uc_strcmp(fpt,mods[i][j])==0 ) {
		uc_strcpy(space,fullmods[i][j]);
return( space );
	    }
	}
	if ( uc_strcmp(fpt,"BoldItal")==0 ) {
	    uc_strcpy(space,"BoldItalic");
return( space );
	} else if ( uc_strcmp(fpt,"BoldObli")==0 ) {
	    uc_strcpy(space,"BoldOblique");
return( space );
	}
return( fpt );
    }

return( weight==NULL || *weight=='\0' ? regular: weight );
}

int SFIsDuplicatable(SplineFont *sf, SplineChar *sc) {
    const int *pua = sf->uni_interp==ui_trad_chinese ? cns14pua : sf->uni_interp==ui_ams ? amspua : NULL;
    int baseuni = 0;
    const unichar_t *pt;

    if ( pua!=NULL && sc->unicodeenc>=0xe000 && sc->unicodeenc<=0xf8ff )
	baseuni = pua[sc->unicodeenc-0xe000];
    if ( baseuni==0 && ( pt = SFGetAlternate(sf,sc->unicodeenc,sc,false))!=NULL &&
	    pt[0]!='\0' && pt[1]=='\0' )
	baseuni = pt[0];
    if ( baseuni!=0 && SFGetChar(sf,baseuni,NULL)!=NULL )
return( true );

return( false );
}

enum flatness { mt_flat, mt_round, mt_pointy, mt_unknown };

static bigreal SPLMaxHeight(SplineSet *spl, enum flatness *isflat) {
    enum flatness f = mt_unknown;
    bigreal max = -1.0e23;
    Spline *s, *first;
    extended ts[2];
    int i;

    for ( ; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( s = spl->first->next; s!=first && s!=NULL; s=s->to->next ) {
	    if ( first==NULL ) first = s;
	    if ( s->from->me.y >= max ||
		    s->to->me.y >= max ||
		    s->from->nextcp.y > max ||
		    s->to->prevcp.y > max ) {
		if ( !s->knownlinear ) {
		    if ( s->from->me.y > max ) {
			f = mt_round;
			max = s->from->me.y;
		    }
		    if ( s->to->me.y > max ) {
			f = mt_round;
			max = s->to->me.y;
		    }
		    SplineFindExtrema(&s->splines[1],&ts[0],&ts[1]);
		    for ( i=0; i<2; ++i ) if ( ts[i]!=-1 ) {
			bigreal y = ((s->splines[1].a*ts[i]+s->splines[1].b)*ts[i]+s->splines[1].c)*ts[i]+s->splines[1].d;
			if ( y>max ) {
			    f = mt_round;
			    max = y;
			}
		    }
		} else if ( s->from->me.y == s->to->me.y ) {
		    if ( s->from->me.y >= max ) {
			max = s->from->me.y;
			f = mt_flat;
		    }
		} else {
		    if ( s->from->me.y > max ) {
			f = mt_pointy;
			max = s->from->me.y;
		    }
		    if ( s->to->me.y > max ) {
			f = mt_pointy;
			max = s->to->me.y;
		    }
		}
	    }
	}
    }
    *isflat = f;
return( max );
}

static bigreal SCMaxHeight(SplineChar *sc, int layer, enum flatness *isflat) {
    /* Find the max height of this layer of the glyph. Also find whether that */
    /* max is flat (as in "z", curved as in "o" or pointy as in "A") */
    enum flatness f = mt_unknown, curf;
    bigreal max = -1.0e23, test;
    RefChar *r;

    max = SPLMaxHeight(sc->layers[layer].splines,&curf);
    f = curf;
    for ( r = sc->layers[layer].refs; r!=NULL; r=r->next ) {
	test = SPLMaxHeight(r->layers[0].splines,&curf);
	if ( test>max || (test==max && curf==mt_flat)) {
	    max = test;
	    f = curf;
	}
    }
    *isflat = f;
return( max );
}

static bigreal SPLMinHeight(SplineSet *spl, enum flatness *isflat) {
    enum flatness f = mt_unknown;
    bigreal min = 1.0e23;
    Spline *s, *first;
    extended ts[2];
    int i;

    for ( ; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( s = spl->first->next; s!=first && s!=NULL; s=s->to->next ) {
	    if ( first==NULL ) first = s;
	    if ( s->from->me.y <= min ||
		    s->to->me.y <= min ||
		    s->from->nextcp.y < min ||
		    s->to->prevcp.y < min ) {
		if ( !s->knownlinear ) {
		    if ( s->from->me.y < min ) {
			f = mt_round;
			min = s->from->me.y;
		    }
		    if ( s->to->me.y < min ) {
			f = mt_round;
			min = s->to->me.y;
		    }
		    SplineFindExtrema(&s->splines[1],&ts[0],&ts[1]);
		    for ( i=0; i<2; ++i ) if ( ts[i]!=-1 ) {
			bigreal y = ((s->splines[1].a*ts[i]+s->splines[1].b)*ts[i]+s->splines[1].c)*ts[i]+s->splines[1].d;
			if ( y<min ) {
			    f = mt_round;
			    min = y;
			}
		    }
		} else if ( s->from->me.y == s->to->me.y ) {
		    if ( s->from->me.y <= min ) {
			min = s->from->me.y;
			f = mt_flat;
		    }
		} else {
		    if ( s->from->me.y < min ) {
			f = mt_pointy;
			min = s->from->me.y;
		    }
		    if ( s->to->me.y < min ) {
			f = mt_pointy;
			min = s->to->me.y;
		    }
		}
	    }
	}
    }
    *isflat = f;
return( min );
}

static bigreal SCMinHeight(SplineChar *sc, int layer, enum flatness *isflat) {
    /* Find the min height of this layer of the glyph. Also find whether that */
    /* min is flat (as in "z", curved as in "o" or pointy as in "A") */
    enum flatness f = mt_unknown, curf;
    bigreal min = 1.0e23, test;
    RefChar *r;

    min = SPLMinHeight(sc->layers[layer].splines,&curf);
    f = curf;
    for ( r = sc->layers[layer].refs; r!=NULL; r=r->next ) {
	test = SPLMinHeight(r->layers[0].splines,&curf);
	if ( test<min || (test==min && curf==mt_flat)) {
	    min = test;
	    f = curf;
	}
    }
    *isflat = f;
return( min );
}

#define RANGE	0x40ffffff

struct dimcnt { bigreal pos; int cnt; };

static int dclist_insert( struct dimcnt *arr, int cnt, bigreal val ) {
    int i;

    for ( i=0; i<cnt; ++i ) {
	if ( arr[i].pos == val ) {
	    ++arr[i].cnt;
return( cnt );
	}
    }
    arr[i].pos = val;
    arr[i].cnt = 1;
return( i+1 );
}

static bigreal SFStandardHeight(SplineFont *sf, int layer, int do_max, unichar_t *list) {
    struct dimcnt flats[200], curves[200];
    bigreal test;
    enum flatness curf;
    int fcnt=0, ccnt=0, cnt, tot, i, useit;
    unichar_t ch, top;
    bigreal result, bestheight, bestdiff, diff, val;
    char *blues, *end;

    while ( *list ) {
	ch = top = *list;
	if ( list[1]==RANGE && list[2]!=0 ) {
	    list += 2;
	    top = *list;
	}
	for ( ; ch<=top; ++ch ) {
	    SplineChar *sc = SFGetChar(sf,ch,NULL);
	    if ( sc!=NULL ) {
		if ( do_max )
		    test = SCMaxHeight(sc, layer, &curf );
		else
		    test = SCMinHeight(sc, layer, &curf );
		if ( curf==mt_flat )
		    fcnt = dclist_insert(flats, fcnt, test);
		else if ( curf!=mt_unknown )
		    ccnt = dclist_insert(curves, ccnt, test);
	    }
	}
	++list;
    }

    /* All flat surfaces at tops of glyphs are at the same level */
    if ( fcnt==1 )
	result = flats[0].pos;
    else if ( fcnt>1 ) {
	cnt = 0;
	for ( i=0; i<fcnt; ++i ) {
	    if ( flats[i].cnt>cnt )
		cnt = flats[i].cnt;
	}
	test = 0;
	tot = 0;
	/* find the mode. If multiple values have the same high count, average them */
	for ( i=0; i<fcnt; ++i ) {
	    if ( flats[i].cnt==cnt ) {
		test += flats[i].pos;
		++tot;
	    }
	}
	result = test/tot;
    } else if ( ccnt==0 )
return( do_max ? -1e23 : 1e23 );		/* We didn't find any glyphs */
    else {
	/* Italic fonts will often have no flat surfaces for x-height just wavies */
	test = 0;
	tot = 0;
	/* find the mean */
	for ( i=0; i<ccnt; ++i ) {
	    test += curves[i].pos;
	    ++tot;
	}
	result = test/tot;
    }

    /* Do we have a BlueValues entry? */
    /* If so, snap height to the closest alignment zone (bottom of the zone) */
    if ( sf->private!=NULL && (blues = PSDictHasEntry(sf->private,do_max ? "BlueValues" : "OtherBlues"))!=NULL ) {
	while ( *blues==' ' || *blues=='[' ) ++blues;
	/* Must get at least this close, else we'll just use what we found */
	bestheight = result; bestdiff = (sf->ascent+sf->descent)/100.0;
	useit = true;
	while ( *blues!='\0' && *blues!=']' ) {
	    val = strtod(blues,&end);
	    if ( blues==end )
	break;
	    blues = end;
	    while ( *blues==' ' ) ++blues;
	    if ( useit ) {
		if ( (diff = val-result)<0 ) diff = -diff;
		if ( diff<bestdiff ) {
		    bestheight = val;
		    bestdiff = diff;
		}
	    }
	    useit = !useit;	/* Only interested in every other BV entry */
	}
	result = bestheight;
    }
return( result );
}

static unichar_t capheight_str[] = { 'A', RANGE, 'Z',
    0x391, RANGE, 0x3a9,
    0x402, 0x404, 0x405, 0x406, 0x408, RANGE, 0x40b, 0x40f, RANGE, 0x418, 0x41a, 0x42f,
    0 };
static unichar_t xheight_str[] = { 'a', 'c', 'e', 'g', 'm', 'n', 'o', 'p', 'q', 'r', 's', 'u', 'v', 'w', 'x', 'y', 'z', 0x131,
    0x3b3, 0x3b9, 0x3ba, 0x3bc, 0x3bd, 0x3c0, 0x3c3, 0x3c4, 0x3c5, 0x3c7, 0x3c8, 0x3c9,
    0x432, 0x433, 0x438, 0x43a, RANGE, 0x43f, 0x442, 0x443, 0x445, 0x44c,0x44f, 0x459, 0x45a,
    0 };
static unichar_t ascender_str[] = { 'b','d','f','h','k','l',
    0x3b3, 0x3b4, 0x3b6, 0x3b8,
    0x444, 0x452,
    0 };
static unichar_t descender_str[] = { 'g','j','p','q','y',
    0x3b2, 0x3b3, 0x3c7, 0x3c8,
    0x434, 0x440, 0x443, 0x444, 0x452, 0x458,
    0 };

bigreal SFCapHeight(SplineFont *sf, int layer, int return_error) {
    bigreal result = SFStandardHeight(sf,layer,true,capheight_str);

    if ( result==-1e23 && !return_error )
	result = (8*sf->ascent)/10;
return( result );
}

bigreal SFXHeight(SplineFont *sf, int layer, int return_error) {
    bigreal result = SFStandardHeight(sf,layer,true,xheight_str);

    if ( result==-1e23 && !return_error )
	result = (6*sf->ascent)/10;
return( result );
}

bigreal SFAscender(SplineFont *sf, int layer, int return_error) {
    bigreal result = SFStandardHeight(sf,layer,true,ascender_str);

    if ( result==-1e23 && !return_error )
	result = (81*sf->ascent)/100;
return( result );
}

bigreal SFDescender(SplineFont *sf, int layer, int return_error) {
    bigreal result = SFStandardHeight(sf,layer,false,descender_str);

    if ( result==1e23 && !return_error )
	result = -sf->descent/2;
return( result );
}

static void arraystring(char *buffer,real *array,int cnt) {
    int i, ei;

    for ( ei=cnt; ei>1 && array[ei-1]==0; --ei );
    *buffer++ = '[';
    for ( i=0; i<ei; ++i ) {
	sprintf(buffer, "%d ", (int) array[i]);
	buffer += strlen(buffer);
    }
    if ( buffer[-1] ==' ' ) --buffer;
    *buffer++ = ']'; *buffer='\0';
}

static void SnapSet(struct psdict *private,real stemsnap[12], real snapcnt[12],
	char *name1, char *name2, int which ) {
    int i, mi;
    char buffer[211];

    mi = -1;
    for ( i=0; i<12 && stemsnap[i]!=0; ++i )
	if ( mi==-1 ) mi = i;
	else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
    if ( mi==-1 )
return;
    if ( which<2 ) {
	sprintf( buffer, "[%d]", (int) stemsnap[mi]);
	PSDictChangeEntry(private,name1,buffer);
    }
    if ( which==0 || which==2 ) {
	arraystring(buffer,stemsnap,12);
	PSDictChangeEntry(private,name2,buffer);
    }
}

int SFPrivateGuess(SplineFont *sf,int layer, struct psdict *private,char *name, int onlyone) {
    real bluevalues[14], otherblues[10];
    real snapcnt[12];
    real stemsnap[12];
    char buffer[211];
    int ret;

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    ret = true;

    if ( strcmp(name,"BlueValues")==0 || strcmp(name,"OtherBlues")==0 ) {
	FindBlues(sf,layer,bluevalues,otherblues);
	if ( !onlyone || strcmp(name,"BlueValues")==0 ) {
	    arraystring(buffer,bluevalues,14);
	    PSDictChangeEntry(private,"BlueValues",buffer);
	}
	if ( !onlyone || strcmp(name,"OtherBlues")==0 ) {
	    if ( otherblues[0]!=0 || otherblues[1]!=0 ) {
		arraystring(buffer,otherblues,10);
		PSDictChangeEntry(private,"OtherBlues",buffer);
	    } else
		PSDictRemoveEntry(private, "OtherBlues");
	}
    } else if ( strcmp(name,"StdHW")==0 || strcmp(name,"StemSnapH")==0 ) {
	FindHStems(sf,stemsnap,snapcnt);
	SnapSet(private,stemsnap,snapcnt,"StdHW","StemSnapH",
		!onlyone ? 0 : strcmp(name,"StdHW")==0 ? 1 : 0 );
    } else if ( strcmp(name,"StdVW")==0 || strcmp(name,"StemSnapV")==0 ) {
	FindVStems(sf,stemsnap,snapcnt);
	SnapSet(private,stemsnap,snapcnt,"StdVW","StemSnapV",
		!onlyone ? 0 : strcmp(name,"StdVW")==0 ? 1 : 0);
    } else if ( strcmp(name,"BlueScale")==0 ) {
	bigreal val = -1;
	if ( PSDictFindEntry(private,"BlueValues")!=-1 ) {
	    /* Can guess BlueScale if we've got a BlueValues */
	    val = BlueScaleFigureForced(private,NULL,NULL);
	}
	if ( val==-1 ) val = .039625;
	sprintf(buffer,"%g", (double) val );
	PSDictChangeEntry(private,"BlueScale",buffer);
    } else if ( strcmp(name,"BlueShift")==0 ) {
	PSDictChangeEntry(private,"BlueShift","7");
    } else if ( strcmp(name,"BlueFuzz")==0 ) {
	PSDictChangeEntry(private,"BlueFuzz","1");
    } else if ( strcmp(name,"ForceBold")==0 ) {
	int isbold = false;
	if ( sf->weight!=NULL &&
		(strstrmatch(sf->weight,"Bold")!=NULL ||
		 strstrmatch(sf->weight,"Heavy")!=NULL ||
		 strstrmatch(sf->weight,"Black")!=NULL ||
		 strstrmatch(sf->weight,"Grass")!=NULL ||
		 strstrmatch(sf->weight,"Fett")!=NULL))
	    isbold = true;
	if ( sf->pfminfo.pfmset && sf->pfminfo.weight>=700 )
	    isbold = true;
	PSDictChangeEntry(private,"ForceBold",isbold ? "true" : "false" );
    } else if ( strcmp(name,"LanguageGroup")==0 ) {
	PSDictChangeEntry(private,"LanguageGroup","0" );
    } else if ( strcmp(name,"ExpansionFactor")==0 ) {
	PSDictChangeEntry(private,"ExpansionFactor","0.06" );
    } else
	ret = false;

    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( ret );
}

void SFRemoveLayer(SplineFont *sf,int l) {
    int gid, i;
    SplineChar *sc;
    CharViewBase *cvs;
    FontViewBase *fvs;
    int layers, any_quads;

    if ( sf->subfontcnt!=0 || l<=ly_fore || sf->multilayer )
return;

    for ( layers = ly_fore, any_quads = false; layers<sf->layer_cnt; ++layers ) {
	if ( layers!=l && sf->layers[layers].order2 )
	    any_quads = true; // Check whether remaining layers have quadratics.
    }
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	// sc->layers may be less than sf->layers.
	if (l < sc->layer_cnt) {
	  LayerFreeContents(sc,l);
	  // Move the other layers and close the gap.
	  for ( i=l+1; i<sc->layer_cnt; ++i )
	    sc->layers[i-1] = sc->layers[i];
	  -- sc->layer_cnt; // Decrement the layer count.
        }
	for ( cvs = sc->views; cvs!=NULL; cvs=cvs->next ) {
	    if ( cvs->layerheads[dm_back] - sc->layers >= sc->layer_cnt )
		cvs->layerheads[dm_back] = &sc->layers[ly_back];
	    if ( cvs->layerheads[dm_fore] - sc->layers >= sc->layer_cnt )
		cvs->layerheads[dm_fore] = &sc->layers[ly_fore];
	}
	if ( !any_quads ) {
	    free(sc->ttf_instrs); sc->ttf_instrs = NULL;
	    sc->ttf_instrs_len = 0;
	}
    }

    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->next ) {
	if ( fvs->active_layer>=l ) {
	    --fvs->active_layer;
	    if ( fvs->active_layer+1==l )
		FontViewLayerChanged(fvs);
	}
    }
    MVDestroyAll(sf);

    free(sf->layers[l].name);
    if (sf->layers[l].ufo_path != NULL) free(sf->layers[l].ufo_path);
    for ( i=l+1; i<sf->layer_cnt; ++i )
	sf->layers[i-1] = sf->layers[i];
    -- sf->layer_cnt; // Decrement the layer count.
}

void SFAddLayer(SplineFont *sf,char *name,int order2,int background) {
    int gid, l;
    SplineChar *sc;
    CharViewBase *cvs;

    if ( sf->layer_cnt>=BACK_LAYER_MAX-1 ) {
	ff_post_error(_("Too many layers"),_("Attempt to have a font with more than %d layers"),
		BACK_LAYER_MAX );
return;
    }
    if ( name==NULL || *name=='\0' )
	name = _("Back");

    l = sf->layer_cnt;
    ++sf->layer_cnt;
    sf->layers = realloc(sf->layers,(l+1)*sizeof(LayerInfo));
    memset(&sf->layers[l],0,sizeof(LayerInfo));
    sf->layers[l].name = copy(name);
    sf->layers[l].order2 = order2;
    sf->layers[l].background = background;

    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	Layer *old = sc->layers;
	sc->layers = realloc(sc->layers,(l+1)*sizeof(Layer));
	memset(&sc->layers[l],0,sizeof(Layer));
	LayerDefault(&sc->layers[l]);
	sc->layers[l].order2 = order2;
	sc->layers[l].background = background;
	++ sc->layer_cnt;
	for ( cvs = sc->views; cvs!=NULL; cvs=cvs->next ) {
	    cvs->layerheads[dm_back] = sc->layers + (cvs->layerheads[dm_back]-old);
	    cvs->layerheads[dm_fore] = sc->layers + (cvs->layerheads[dm_fore]-old);
	}
    }
}

void SFLayerSetBackground(SplineFont *sf,int layer,int is_back) {
    int k,gid;
    SplineFont *_sf;
    SplineChar *sc;

    sf->layers[layer].background = is_back;
    k=0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	for ( gid=0; gid<_sf->glyphcnt; ++gid ) if ( (sc=_sf->glyphs[gid])!=NULL ) {
	    sc->layers[layer].background = is_back;
	    if ( !is_back && sc->layers[layer].images!=NULL ) {
		ImageListsFree(sc->layers[layer].images);
		sc->layers[layer].images = NULL;
		SCCharChangedUpdate(sc,layer);
	    }
	}
	++k;
    } while ( k<sf->subfontcnt );
}


int SplinePointListContains( SplinePointList* container, SplinePointList* sought )
{
    SplinePointList *spl;
    for ( spl = container; spl!=NULL; spl = spl->next )
    {
	if( spl == sought )
	    return 1;
    }
    return 0;
}


void SPLFirstVisitorDebug(SplinePoint* splfirst, Spline* spline, void* udata )
{
    printf("   splfirst:%p spline:%p udata:%p\n", splfirst, spline, udata );
}

void SPLFirstVisitorDebugSelectionState(SplinePoint* splfirst, Spline* spline, void* udata )
{
    printf("   splfirst:%p spline:%p udata:%p", splfirst, spline, udata );
    printf("   from.selected:%d n:%d p:%d to.selected:%d n:%d p:%d\n",
	   ( spline->from ? spline->from->selected       : -1 ),
	   ( spline->from ? spline->from->nextcpselected : -1 ),
	   ( spline->from ? spline->from->prevcpselected : -1 ),
	   ( spline->to   ? spline->to->selected         : -1 ),
	   ( spline->to   ? spline->to->nextcpselected   : -1 ),
	   ( spline->to   ? spline->to->prevcpselected   : -1 )
	);
}




void SPLFirstVisitSplines( SplinePoint* splfirst, SPLFirstVisitSplinesVisitor f, void* udata )
{
    Spline *spline=0;
    Spline *first=0;
    Spline *next=0;

    if ( splfirst!=NULL )
    {
	first = NULL;
	for ( spline = splfirst->next; spline!=NULL && spline!=first; spline = next )
	{
	    next = spline->to->next;

	    // callback
	    f( splfirst, spline, udata );

	    if ( first==NULL )
	    {
		first = spline;
	    }
	}
    }
}

void SPLFirstVisitPoints( SplinePoint* splfirst, SPLFirstVisitPointsVisitor f, void* udata )
{
    Spline *spline=0;
    Spline *first=0;
    Spline *next=0;

    if ( splfirst!=NULL )
    {
	first = NULL;
	for ( spline = splfirst->next; spline!=NULL && spline!=first; spline = next )
	{
	    next = spline->to->next;

	    // callback
	    if( splfirst && splfirst->next == spline )
		f( splfirst, spline, spline->from, udata );
	    f( splfirst, spline, spline->to, udata );

	    if ( first==NULL )
	    {
		first = spline;
	    }
	}
    }
}



typedef struct SPLFirstVisitorFoundSoughtDataS
{
    SplinePoint* sought;
    int found;
} SPLFirstVisitorFoundSoughtData;

static void SPLFirstVisitorFoundSought(SplinePoint* splfirst, Spline* spline, void* udata )
{
    SPLFirstVisitorFoundSoughtData* d = (SPLFirstVisitorFoundSoughtData*)udata;
//    printf("SPLFirstVisitorFoundSought()   splfirst:%p spline:%p udata:%p\n", splfirst, spline, udata );
//    printf("SPLFirstVisitorFoundSought()   sought:%p from:%p to:%p\n", d->sought, spline->from, spline->to );

    if( spline->from == d->sought || spline->to == d->sought )
    {
//	printf("got it!\n");
	d->found = 1;
    }
}

int SplinePointListContainsPoint( SplinePointList* container, SplinePoint* sought )
{
    if( !sought )
	return 0;

//    printf("\n\n\nSplinePointListContainsPoint(top) want:%p\n", sought );
    SplinePointList *spl;
    for ( spl = container; spl!=NULL; spl = spl->next )
    {
	//SplinePoint* p   = spl->first;
	//SplinePoint* end = spl->last;

	SPLFirstVisitorFoundSoughtData d;
	d.sought = sought;
	d.found  = 0;
	SPLFirstVisitSplines( spl->first, SPLFirstVisitorFoundSought, &d );
	if( d.found )
	    return 1;
    }
    return 0;
}


typedef struct SPLFirstVisitorFoundSoughtXYDataS
{
    int use_x;
    int use_y;
    real x;
    real y;

    // outputs
    int found;
    Spline* spline;
    SplinePoint* sp;

} SPLFirstVisitorFoundSoughtXYData;

static void SPLFirstVisitorFoundSoughtXY(SplinePoint* splfirst, Spline* spline, void* udata )
{
    SPLFirstVisitorFoundSoughtXYData* d = (SPLFirstVisitorFoundSoughtXYData*)udata;
    int found = 0;

    if( d->found )
	return;

    // printf("SPLFirstVisitorFoundSoughtXY() %f %f %f\n", d->x, spline->from->me.x, spline->to->me.x );
    if( d->use_x )
    {
	if( spline->from->me.x == d->x )
	{
	    found = 1;
	    d->spline = spline;
	    d->sp = spline->from;
	}

	if( spline->to->me.x == d->x )
	{
	    found = 1;
	    d->spline = spline;
	    d->sp = spline->to;
	}
    }
    if( d->use_x && found && d->use_y )
    {
	if( d->sp->me.y != d->y )
	{
	    found = 0;
	}
    }
    else if( d->use_y )
    {
	if( spline->from->me.y == d->y )
	{
	    found = 1;
	    d->spline = spline;
	    d->sp = spline->from;
	}

	if( spline->to->me.y == d->y )
	{
	    found = 1;
	    d->spline = spline;
	    d->sp = spline->to;
	}
    }

    if( found )
    {
	d->found = found;
	d->spline = spline;
    }
    else
    {
	d->sp = 0;
    }
}


SplinePoint* SplinePointListContainsPointAtX( SplinePointList* container, real x )
{
    SplinePointList *spl;
    for ( spl = container; spl!=NULL; spl = spl->next )
    {
	SPLFirstVisitorFoundSoughtXYData d;
	d.use_x  = 1;
	d.use_y  = 0;
	d.x      = x;
	d.y      = 0;
	d.found  = 0;
	SPLFirstVisitSplines( spl->first, SPLFirstVisitorFoundSoughtXY, &d );
	if( d.found )
	    return d.sp;
    }
    return 0;
}

SplinePoint* SplinePointListContainsPointAtY( SplinePointList* container, real y )
{
    SplinePointList *spl;
    for ( spl = container; spl!=NULL; spl = spl->next )
    {
	SPLFirstVisitorFoundSoughtXYData d;
	d.use_x  = 0;
	d.use_y  = 1;
	d.x      = 0;
	d.y      = y;
	d.found  = 0;
	SPLFirstVisitSplines( spl->first, SPLFirstVisitorFoundSoughtXY, &d );
	if( d.found )
	    return d.sp;
    }
    return 0;
}

SplinePoint* SplinePointListContainsPointAtXY( SplinePointList* container, real x, real y )
{
    SplinePointList *spl;
    for ( spl = container; spl!=NULL; spl = spl->next )
    {
	SPLFirstVisitorFoundSoughtXYData d;
	d.use_x  = 1;
	d.use_y  = 1;
	d.x      = x;
	d.y      = y;
	d.found  = 0;
	SPLFirstVisitSplines( spl->first, SPLFirstVisitorFoundSoughtXY, &d );
	if( d.found )
	    return d.sp;
    }
    return 0;
}



