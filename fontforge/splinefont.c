/* Copyright (C) 2000-2008 by George Williams */
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

#include "fontforgevw.h"
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <unistd.h>
#include <gfile.h>
#include <time.h>
#include "unicoderange.h"
#include "psfont.h"

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
#ifdef FONTFORGE_CONFIG_TYPE3
    static Layer layers[2];
#endif

    memset(dummy,'\0',sizeof(*dummy));
    dummy->color = COLOR_DEFAULT;
    dummy->layer_cnt = 2;
#ifdef FONTFORGE_CONFIG_TYPE3
    dummy->layers = layers;
#endif
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
    extern const int cns14pua[], amspua[];

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
		SCCharChangedUpdate(sc);
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
	sc = SplineCharCreate();
	sc->unicodeenc = dummy.unicodeenc;
	sc->name = copy(dummy.name);
	sc->width = dummy.width;
	sc->parent = sf;
	sc->orig_pos = 0xffff;
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
    { NULL }
};

int NameToEncoding(SplineFont *sf,EncMap *map,const char *name) {
    int enc, uni, i, ch;
    char *end, *dot=NULL, *freeme=NULL;
    const char *upt = name;

    ch = utf8_ildb(&upt);
    if ( *upt=='\0' ) {
	enc = SFFindSlot(sf,map,ch,NULL);
	if ( enc!=-1 )
return( enc );
    }

    enc = uni = -1;
	
    while ( 1 ) {
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
	if ( enc!=-1 && uni==-1 ) {
	    int gid = map->map[enc];
	    if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		uni = sf->glyphs[gid]->unicodeenc;
	    else if ( map->enc->is_unicodebmp || map->enc->is_unicodefull )
		uni = enc;
	}
	if ( dot!=NULL ) {
	    free(freeme);
	    if ( uni==-1 )
return( -1 );
	    if ( uni<0x600 || uni>0x6ff )
return( -1 );
	    if ( strmatch(dot,".begin")==0 || strmatch(dot,".initial")==0 )
		uni = ArabicForms[uni-0x600].initial;
	    else if ( strmatch(dot,".end")==0 || strmatch(dot,".final")==0 )
		uni = ArabicForms[uni-0x600].final;
	    else if ( strmatch(dot,".medial")==0 )
		uni = ArabicForms[uni-0x600].medial;
	    else if ( strmatch(dot,".isolated")==0 )
		uni = ArabicForms[uni-0x600].isolated;
	    else
return( -1 );
return( SFFindSlot(sf,map,uni,NULL) );
	} else 	if ( enc!=-1 ) {
	    free(freeme);
return( enc );
	}
	dot = strrchr(name,'.');
	if ( dot==NULL )
return( -1 );
	free(freeme);
	name = freeme = copyn(name,dot-name);
    }
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
    result = galloc(10*strlen(string)+1);
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
    result = galloc(10*strlen(string)+1);
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

int SFScaleToEm(SplineFont *sf, int as, int des) {
    double scale;
    real transform[6];
    BVTFunc bvts;
    uint8 *oldselected = sf->fv->selected;

    scale = (as+des)/(double) (sf->ascent+sf->descent);
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
    sf->upos *= scale;
    sf->uwidth *= scale;

    if ( sf->private!=NULL )
	SFScalePrivate(sf,scale);

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
    sf->fv->selected = galloc(sf->fv->map->enccount);
    memset(sf->fv->selected,1,sf->fv->map->enccount);

    sf->ascent = as; sf->descent = des;

    FVTransFunc(sf->fv,transform,0,&bvts,
	    fvt_dobackground|fvt_round_to_int|fvt_dontsetwidth|fvt_scalekernclasses|fvt_scalepstpos|fvt_dogrid);
    free(sf->fv->selected);
    sf->fv->selected = oldselected;

    if ( !sf->changed ) {
	sf->changed = true;
	FVSetTitles(sf);
    }
	
return( true );
}

void SFSetModTime(SplineFont *sf) {
    time_t now;
    time(&now);
    sf->modificationtime = now;
}

static SplineFont *SFReadPostscript(char *filename) {
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
	    CheckAfmOfPostscript(sf,filename,sf->map);
    }
return( sf );
}

extern struct compressors compressors[];

char *Decompress(char *name, int compression) {
    char *dir = getenv("TMPDIR");
    char buf[1500];
    char *tmpfile;

    if ( dir==NULL ) dir = P_tmpdir;
    tmpfile = galloc(strlen(dir)+strlen(GFileNameTail(name))+2);
    strcpy(tmpfile,dir);
    strcat(tmpfile,"/");
    strcat(tmpfile,GFileNameTail(name));
    *strrchr(tmpfile,'.') = '\0';
#if defined( _NO_SNPRINTF ) || defined( __VMS )
    sprintf( buf, "%s < %s > %s", compressors[compression].decomp, name, tmpfile );
#else
    snprintf( buf, sizeof(buf), "%s < %s > %s", compressors[compression].decomp, name, tmpfile );
#endif
    if ( system(buf)==0 )
return( tmpfile );
    free(tmpfile);
return( NULL );
}

/* This does not check currently existing fontviews, and should only be used */
/*  by LoadSplineFont (which does) and by RevertFile (which knows what it's doing) */
SplineFont *ReadSplineFont(char *filename,enum openflags openflags) {
    SplineFont *sf;
    char ubuf[250], *temp;
    int fromsfd = false;
    int i;
    char *pt, *strippedname, *oldstrippedname, *tmpfile=NULL, *paren=NULL, *fullname=filename, *rparen;
    int len;
    FILE *foo;
    int checked;
    int compression=0;

    if ( filename==NULL )
return( NULL );

    strippedname = filename;
    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    /* Someone gave me a font "Nafees Nastaleeq(Updated).ttf" and complained */
    /*  that ff wouldn't open it */
    /* Now someone will complain about "Nafees(Updated).ttc(fo(ob)ar)" */
    if ( (paren = strrchr(pt,'('))!=NULL &&
	    (rparen = strrchr(paren,')'))!=NULL &&
	    rparen[1]=='\0' ) {
	strippedname = copy(filename);
	strippedname[paren-filename] = '\0';
    }

    pt = strrchr(strippedname,'.');
    i = -1;
    if ( pt!=NULL ) for ( i=0; compressors[i].ext!=NULL; ++i )
	if ( strcmp(compressors[i].ext,pt)==0 )
    break;
    oldstrippedname = strippedname;
    if ( i==-1 || compressors[i].ext==NULL ) i=-1;
    else {
	tmpfile = Decompress(strippedname,i);
	if ( tmpfile!=NULL ) {
	    strippedname = tmpfile;
	} else {
	    ff_post_error(_("Decompress Failed!"),_("Decompress Failed!"));
return( NULL );
	}
	compression = i+1;
	if ( strippedname!=filename && paren!=NULL ) {
	    fullname = galloc(strlen(strippedname)+strlen(paren)+1);
	    strcpy(fullname,strippedname);
	    strcat(fullname,paren);
	} else
	    fullname = strippedname;
    }

    /* If there are no pfaedit windows, give them something to look at */
    /*  immediately. Otherwise delay a bit */
    strcpy(ubuf,_("Loading font from "));
    len = strlen(ubuf);
    strncat(ubuf,temp = def2utf8_copy(GFileNameTail(fullname)),100);
    free(temp);
    ubuf[100+len] = '\0';
    ff_progress_start_indicator(FontViewFirst()==NULL?0:10,_("Loading..."),ubuf,_("Reading Glyphs"),0,1);
    ff_progress_enable_stop(0);
    if ( FontViewFirst()==NULL && !no_windowing_ui )
	ff_progress_allow_events();

    sf = NULL;
    foo = fopen(strippedname,"rb");
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
	char *temp = galloc(strlen(strippedname)+strlen("/glyphs/contents.plist")+1);
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
	if ( foo!=NULL )
	    fclose(foo);
    } else if ( foo!=NULL ) {
	/* Try to guess the file type from the first few characters... */
	int ch1 = getc(foo);
	int ch2 = getc(foo);
	int ch3 = getc(foo);
	int ch4 = getc(foo);
	int ch5 = getc(foo);
	int ch6 = getc(foo);
	int ch7 = getc(foo);
	int ch9, ch10;
	fseek(foo, 98, SEEK_SET);
	ch9 = getc(foo);
	ch10 = getc(foo);
	fclose(foo);
	if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		(ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		(ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ||
		(ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f') ) {
	    sf = SFReadTTF(fullname,0,openflags);
	    checked = 't';
	} else if (( ch1=='%' && ch2=='!' ) ||
		    ( ch1==0x80 && ch2=='\01' ) ) {	/* PFB header */
	    sf = SFReadPostscript(fullname);
	    checked = 'p';
	} else if ( ch1=='%' && ch2=='P' && ch3=='D' && ch4=='F' ) {
	    sf = SFReadPdfFont(fullname,openflags);
	    checked = 'P';
	} else if ( ch1==1 && ch2==0 && ch3==4 ) {
	    sf = CFFParse(fullname);
	    checked = 'c';
	} else if ( ch1=='<' && ch2=='?' && (ch3=='x'||ch3=='X') && (ch4=='m'||ch4=='M') ) {
	    sf = SFReadSVG(fullname,0);
	    checked = 'S';
	} else if ( ch1==0xef && ch2==0xbb && ch3==0xbf &&
		ch4=='<' && ch5=='?' && (ch6=='x'||ch6=='X') && (ch7=='m'||ch7=='M') ) {
	    /* UTF-8 SVG with initial byte ordering mark */
	    sf = SFReadSVG(fullname,0);
	    checked = 'S';
#if 0		/* I'm not sure if this is a good test for mf files... */
	} else if ( ch1=='%' && ch2==' ' ) {
	    sf = SFFromMF(fullname);
#endif
	} else if ( ch1=='S' && ch2=='p' && ch3=='l' && ch4=='i' ) {
	    sf = SFDRead(fullname);
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
    } else if ( strmatch(fullname+strlen(fullname)-4, ".svg")==0 && checked!='S' ) {
	sf = SFReadSVG(fullname,0);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".ufo")==0 && checked!='u' ) {
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
	sf = SFReadPalmPdb(fullname,0);
    } else if ( (strmatch(fullname+strlen(fullname)-4, ".pfa")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".pfb")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".pf3")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".cid")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".gsf")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".pt3")==0 ||
		strmatch(fullname+strlen(fullname)-3, ".ps")==0 ) && checked!='p' ) {
	sf = SFReadPostscript(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".cff")==0 && checked!='c' ) {
	sf = CFFParse(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-3, ".mf")==0 ) {
	sf = SFFromMF(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".pdf")==0 && checked!='P' ) {
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
	if ( sf->chosenname!=NULL && strippedname==filename ) {
	    norm->origname = galloc(strlen(filename)+strlen(sf->chosenname)+8);
	    strcpy(norm->origname,filename);
	    strcat(norm->origname,"(");
	    strcat(norm->origname,sf->chosenname);
	    strcat(norm->origname,")");
	} else
	    norm->origname = copy(filename);
	free( sf->chosenname ); sf->chosenname = NULL;
	if ( sf->mm!=NULL ) {
	    int j;
	    for ( j=0; j<sf->mm->instance_count; ++j ) {
		free(sf->mm->instances[j]->origname);
		sf->mm->instances[j]->origname = copy(norm->origname);
	    }
	}
    } else if ( !GFileExists(filename) )
	ff_post_error(_("Couldn't open font"),_("The requested file, %.100s, does not exist"),GFileNameTail(filename));
    else if ( !GFileReadable(filename) )
	ff_post_error(_("Couldn't open font"),_("You do not have permission to read %.100s"),GFileNameTail(filename));
    else
	ff_post_error(_("Couldn't open font"),_("%.100s is not in a known format (or is so badly corrupted as to be unreadable)"),GFileNameTail(filename));

    if ( oldstrippedname!=filename )
	free(oldstrippedname);
    if ( fullname!=filename && fullname!=strippedname )
	free(fullname);
    if ( tmpfile!=NULL ) {
	unlink(tmpfile);
	free(tmpfile);
    }
    if ( (openflags&of_fstypepermitted) && sf!=NULL && (sf->pfminfo.fstype&0xff)==0x0002 ) {
	/* Ok, they have told us from a script they have access to the font */
    } else if ( !fromsfd && sf!=NULL && (sf->pfminfo.fstype&0xff)==0x0002 ) {
	char *buts[3];
	buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;
	if ( ff_ask(_("Restricted Font"),(const char **) buts,1,1,_("This font is marked with an FSType of 2 (Restricted\nLicense). That means it is not editable without the\npermission of the legal owner.\n\nDo you have such permission?"))==1 ) {
	    SplineFontFree(sf);
return( NULL );
	}
    }
return( sf );
}

char *ToAbsolute(char *filename) {
    char buffer[1025];

    GFileGetAbsoluteName(filename,buffer,sizeof(buffer));
return( copy(buffer));
}

SplineFont *LoadSplineFont(char *filename,enum openflags openflags) {
    SplineFont *sf;
    char *pt, *ept, *tobefreed1=NULL, *tobefreed2=NULL;
    static char *extens[] = { ".sfd", ".pfa", ".pfb", ".ttf", ".otf", ".ps", ".cid", ".bin", ".dfont", ".PFA", ".PFB", ".TTF", ".OTF", ".PS", ".CID", ".BIN", ".DFONT", NULL };
    int i;

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
#if 0
	    int ch1 = getc(test);
	    int ch2 = getc(test);
	    int ch3 = getc(test);
	    int ch4 = getc(test);
	    if ( ch1=='%' ) ok = true;
	    else if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		    (  ch1==0 && ch2==2 && ch3==0 && ch4==0 ) ||
		    /* Windows 3.1 Chinese version used this version for some arphic fonts */
		    /* See discussion on freetype list, july 2004 */
		    (ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		    (ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ||
		    (ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f') ) ok = true;
	    else if ( ch1=='S' && ch2=='p' && ch3=='l' && ch4=='i' ) ok = true;
#endif
	    ok = true;		/* Mac resource files are too hard to check for */
		    /* If file exists, assume good */
	    fclose(test);
	}
	if ( !ok ) {
	    tobefreed1 = galloc(strlen(filename)+8);
	    strcpy(tobefreed1,filename);
	    ept = tobefreed1+strlen(tobefreed1);
	    for ( i=0; extens[i]!=NULL; ++i ) {
		strcpy(ept,extens[i]);
		if ( GFileExists(tobefreed1))
	    break;
	    }
	    if ( extens[i]!=NULL )
		filename = tobefreed1;
	    else {
		free(tobefreed1);
		tobefreed1 = NULL;
	    }
	}
    } else
	tobefreed1 = NULL;

    sf = NULL;
    sf = FontWithThisFilename(filename);
    if ( sf==NULL && *filename!='/' )
	filename = tobefreed2 = ToAbsolute(filename);

    if ( sf==NULL )
	sf = ReadSplineFont(filename,openflags);

    free(tobefreed1);
    free(tobefreed2);
return( sf );
}

/* Use URW 4 letter abbreviations */
char *knownweights[] = { "Demi", "Bold", "Regu", "Medi", "Book", "Thin",
	"Ligh", "Heav", "Blac", "Ultr", "Nord", "Norm", "Gras", "Stan", "Halb",
	"Fett", "Mage", "Mitt", "Buch", NULL };
char *realweights[] = { "Demi", "Bold", "Regular", "Medium", "Book", "Thin",
	"Light", "Heavy", "Black", "Ultra", "Nord", "Normal", "Gras", "Standard", "Halbfett",
	"Fett", "Mager", "Mittel", "Buchschrift", NULL};
static char *moreweights[] = { "ExtraLight", "VeryLight", NULL };
char **noticeweights[] = { moreweights, realweights, knownweights, NULL };

static char *modifierlist[] = { "Ital", "Obli", "Kursive", "Cursive", "Slanted",
	"Expa", "Cond", NULL };
static char *modifierlistfull[] = { "Italic", "Oblique", "Kursive", "Cursive", "Slanted",
    "Expanded", "Condensed", NULL };
static char **mods[] = { knownweights, modifierlist, NULL };
static char **fullmods[] = { realweights, modifierlistfull, NULL };

char *_GetModifiers(char *fontname, char *familyname,char *weight) {
    char *pt, *fpt;
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
	    if ( strcmp(fpt,mods[i][j])==0 )
return( fullmods[i][j]);
	}
	if ( strcmp(fpt,"BoldItal")==0 )
return( "BoldItalic" );
	else if ( strcmp(fpt,"BoldObli")==0 )
return( "BoldOblique" );

return( fpt );
    }

return( weight==NULL || *weight=='\0' ? "Regular": weight );
}

char *SFGetModifiers(SplineFont *sf) {
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
    extern const int cns14pua[], amspua[];
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
