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
#include <stdlib.h>

#include "gdrawP.h"
#include "fontP.h"
#include "gxcdrawP.h"
#include "ustring.h"
#include "chardata.h"
#include "utype.h"

/*#define GREEK_BUG	1*/

static unichar_t *normalize_font_names(const unichar_t *names) {
    unichar_t *n;
    unichar_t *ipt, *opt;

    ipt = opt = n = u_copy(names);
    while ( *ipt>0 && isspace(*ipt)) ++ipt;
    while ( *ipt ) {
	if ( *ipt>0 && isspace(*ipt)) {
	    while ( *ipt>0 && isspace(*ipt)) ++ipt;
	    *opt++ = ' ';
	} else
	    *opt++ = *ipt++;
    }
    *opt = '\0';
return( n );
}

static struct family_info *FindFamily(FState *fs,unichar_t *names) {
    struct family_info *fam;
    int ch;

    ch = *names;
    if ( ch=='"' ) ch = names[1];
    if ( isupper(ch)) ch = tolower(ch);
    if ( ch<'a' ) ch = 'q'; else if ( ch>'z' ) ch='z';
    ch -= 'a';

    for ( fam = fs->fam_hash[ch]; fam!=NULL; fam=fam->next )
	if ( u_strmatch(names,fam->family_names)==0 )
return( fam );

return( NULL );
}

static int CountFamilyNames(unichar_t *names) {
    int tot=0;
    unichar_t quote=0;

    forever {
	if ( *names=='"' || *names=='\'' ) {
	    quote = *names++;
	    while ( *names!='\0' && *names!=quote ) ++names;
	    if ( *names==quote ) ++names;
	}
	while ( *names!=',' && *names!='\0' ) ++names;
	++tot;
	if ( *names=='\0' )
return( tot );
	if ( *names==',' ) ++names;
    }
}

static struct font_name *FindFontName(FState *fs, unichar_t *name, enum font_type *ft) {
    int ch;
    struct font_name *fn;
    extern struct fontabbrev _gdraw_fontabbrev[];
    int i;

    ch = *name;
    if ( isupper(ch)) ch = tolower(ch);
    if ( ch<'a' ) ch = 'q'; else if ( ch>'z' ) ch='z';
    ch -= 'a';

    for ( fn = fs->font_names[ch]; fn!=NULL; fn=fn->next )
	if ( u_strmatch(fn->family_name,name)==0 ) {
	    if ( fn->ft!=ft_unknown )
		*ft = fn->ft;
return( fn );
	}

    for ( i=0; _gdraw_fontabbrev[i].abbrev!=NULL; ++i )
	if ( uc_strstrmatch(name,_gdraw_fontabbrev[i].abbrev)!=NULL ) {
	    if ( _gdraw_fontabbrev[i].searched )
return( _gdraw_fontabbrev[i].found );
	    *ft = _gdraw_fontabbrev[i].ft;
	    if ( _gdraw_fontabbrev[i].dont_search )
    continue;
	    _gdraw_fontabbrev[i].searched = true;
	    for ( ch=0; ch<26; ++ch ) {
		for ( fn = fs->font_names[ch]; fn!=NULL; fn=fn->next )
		    if ( uc_strstrmatch(fn->family_name,_gdraw_fontabbrev[i].abbrev)!=NULL ) {
			_gdraw_fontabbrev[i].found = fn;
return( fn );
		    }
	    }
return( NULL );
	}

return( NULL );
}

static enum font_type FindFonts(FState *fs, unichar_t *names, struct font_name **fonts) {
    int cnt=0;
    unichar_t quote=0, ch;
    unichar_t *strt, *end;
    enum font_type ft = ft_unknown;

    forever {
	if ( *names=='"' || *names=='\'' ) {
	    quote = *names++;
	    strt = names;
	    while ( *names!='\0' && *names!=quote ) ++names;
	    end = names;
	    if ( *names==quote ) ++names;
	    while ( *names!='\0' && *names!=',' ) ++names;
	} else {
	    if ( *names==' ' ) ++names;
	    strt = names;
	    while ( *names!='\0' && *names!=',' ) ++names;
	    end = names;
	    if ( end>strt && end[-1]==' ' ) --end;
	}
	ch = *end; *end = '\0';
	fonts[cnt] = FindFontName(fs,strt, &ft);
	*end = ch;
	++cnt;
	if ( *names=='\0' )
return( ft );
	if ( *names==',' ) ++names;
    }
}

static struct family_info *CreateFamily(FState *fs, unichar_t *requested_families) {
    int cnt, ch;
    struct font_name **fonts;
    enum font_type ft;
    struct family_info *fam;

    cnt = CountFamilyNames(requested_families);

    fonts = galloc((cnt+1) * sizeof(struct font_name **));
    if ( fonts==NULL )
return( NULL );
    fonts[cnt]=NULL;
    ft = FindFonts(fs,requested_families,fonts);
    fam = galloc(sizeof(struct family_info));
    if ( fam==NULL ) {
	gfree(fonts);
return( NULL );
    }
    fam->family_names = requested_families;
    fam->name_cnt = cnt;
    fam->ft = ft;
    fam->fonts = fonts;
    fam->instanciations = NULL;

    ch = *requested_families;
    if ( isupper(ch)) ch = tolower(ch);
    if ( ch<'a' ) ch = 'q'; else if ( ch>'z' ) ch='z';
    ch -= 'a';
    fam->next = fs->fam_hash[ch];
    fs->fam_hash[ch] = fam;
return( fam );
}

static int FindFontDiff(struct font_data *try,FontRequest *rq) {
    int diff, temp;
    /* this routine is inlined in FindBest */

    if ( try->configuration_error )
return( 0x7ffffff );
    if (( diff = try->weight - rq->weight )<0 ) diff = -diff;
    diff *= 2;
    if ( (try->style & fs_italic) != ( rq->style & fs_italic )) diff += 500;
    if ( (try->style & fs_smallcaps) != ( rq->style & fs_smallcaps )) diff += 200;
    if ( (try->style & fs_condensed) != ( rq->style & fs_condensed )) diff += 200;
    if ( (try->style & fs_extended) != ( rq->style & fs_extended )) diff += 200;
    if ( try->is_scalable )
	temp = 0;
    else if ( (temp = try->point_size - rq->point_size)<0 ) temp = -temp;
    if ( try->is_scalable || try->was_scaled )
	diff += 200;		/* A non-scaled font is better than a scaled one */
    diff += temp*200;
return( diff );
}

static struct font_data *FindBest(GDisplay *gdisp,struct font_name *fn,FontRequest *rq,int enc,
	struct font_data *best, int *best_pos, int *best_val, int i) {
    struct font_data *try;
    int diff, temp;

    if ( fn==NULL )
return( best );
    for ( try = fn->data[enc]; try!=NULL; try = try->next ) {
	/* this is an inline version of FindFontDiff */
	if ( try->configuration_error )
    continue;
	if (( diff = try->weight - rq->weight )<0 ) diff = -diff;
	if ( (try->style & fs_italic) != ( rq->style & fs_italic )) diff += 500;
	if ( (try->style & fs_smallcaps) != ( rq->style & fs_smallcaps )) diff += 200;
	if ( (try->style & fs_condensed) != ( rq->style & fs_condensed )) diff += 200;
	if ( (try->style & fs_extended) != ( rq->style & fs_extended )) diff += 200;
	if ( try->is_scalable ) {
	    temp = 0;
	} else if ( (temp = try->point_size - rq->point_size)<0 ) temp = -temp;
	if ( try->is_scalable || try->was_scaled ) {
	    diff += 200;		/* A non-scaled font is better than a scaled one */
	    if ( !gdisp->fontstate->allow_scaling )
    continue;
	}
	diff += temp*200;
	/* End inline version */
	if ( diff+(i-*best_pos)*100< *best_val ) {
	    best = try;
	    *best_val = diff;
	    *best_pos = i;
	}
    }
    if ( best==NULL )
return( NULL );

    try = best;
    if ( best->is_scalable )
	try = (gdisp->funcs->scaleFont)(gdisp,best,rq);
    else if ( best->style!=rq->style || best->weight!=rq->weight )
	try = (gdisp->funcs->stylizeFont)(gdisp,best,rq);
    if ( try!=best && try!=NULL ) {
	best = try;
	best->was_scaled = true;
	best->next = fn->data[enc]; fn->data[enc] = best;
	*best_val = FindFontDiff(best,rq);
    }
return( best );
}

static struct font_data *PickMatchingSmallCaps(GDisplay *gdisp,
	struct font_name *fn,FontRequest *rq,int enc) {
    struct font_data *best; int best_pos; int best_val;

    best = NULL; best_pos = 0x7fff; best_val = 0x7fff;
return( FindBest(gdisp,fn,rq,enc,best,&best_pos,&best_val,0));
}

static void PickUnicodeFonts(GDisplay *gdisp,struct font_data **unifonts,
	struct family_info *fam, FontRequest *rq ) {
    FState *fs = gdisp->fontstate;
    int best_pos; int best_val;
    int i;

    for ( i = 0; i<fam->name_cnt; ++i ) {
	best_pos = 0x7fff; best_val = 0x7fff;
	unifonts[i] = FindBest(gdisp,fam->fonts[i],rq,em_unicode,NULL,&best_pos,&best_val,0);
    }
    for ( i=ft_serif; i<ft_max; ++i ) {
	int j = fam->name_cnt+i-ft_serif;
	best_pos = 0x7fff; best_val = 0x7fff;
	unifonts[j] = FindBest(gdisp,fs->lastchance[em_unicode][i],rq,em_unicode,NULL,&best_pos,&best_val,0);
	unifonts[j] = FindBest(gdisp,fs->lastchance2[em_unicode][i],rq,em_unicode,unifonts[j],&best_pos,&best_val,0);
    }
}

static struct font_data *PickFontForEncoding(GDisplay *gdisp, struct family_info *fam,
	FontRequest *rq,int enc,int *pos, int *vals) {
    FState *fs = gdisp->fontstate;
    int i, ch;
    struct font_data *best; int best_pos; int best_val;
    struct font_name *fn;

    best = NULL; best_pos = 0x7fff; best_val = 0x7fff;
    for ( i=0; i<fam->name_cnt; ++i ) {
	fn = fam->fonts[i];
	if ( fn==NULL )
    continue;
	best = FindBest(gdisp,fn,rq,enc,best,&best_pos,&best_val,i);
	if ( best_val==0 )
    break;
    }
    if ( best==NULL ) {
	for ( ch=0; ch<26; ++ch ) {
	    for ( fn=fs->font_names[ch]; fn!=NULL; fn=fn->next )
		if ( fn->ft == fam->ft && fn->data[enc]!=NULL ) {
		    best = FindBest(gdisp,fn,rq,enc,best,&best_pos,&best_val,i);
		    if ( best_val==0 )
	goto break_many;
		}
	}
	break_many:
	if ( best==NULL ) {
	    best = FindBest(gdisp,fs->lastchance[enc][ft_unknown],rq,enc,best,&best_pos,&best_val,i+1);
	    best = FindBest(gdisp,fs->lastchance2[enc][ft_unknown],rq,enc,best,&best_pos,&best_val,i+1);
	}
    }
#if GREEK_BUG
 if ( best!=NULL )
  printf( "Best for %d pos=%d: %s\n", enc, best_pos, best->localname );
#endif
    pos[enc] = best_pos; vals[enc] = best_val;
return( best );
}
    
FontInstance *GDrawSetFont(GWindow gw, FontInstance *fi) {
    FontInstance *old = gw->ggc->fi;
    gw->ggc->fi = fi;
return( old );
}

static struct font_data *MakeFontFromScreen(GDisplay *disp,int ps,struct family_info *fam,
	FontRequest *srq, int map, struct font_data *screen_font) {
    FState *fonts = disp->fontstate;
    struct font_data *fd;

    if ( screen_font->was_scaled ) {
	/* even though we turned off scaling when we asked for a screen font */
	/*  if we get something that's already been instanciated we may get */
	/*  something scaled */
	int pos[em_max+1], val[em_max+1];
	if ( fam==NULL )
return( NULL );
	screen_font = PickFontForEncoding(disp,fam,srq,map,pos,val);
	if ( screen_font==NULL )
return(NULL);
    }

    for ( fd=fonts->StolenFromScreen; fd!=NULL; fd = fd->next )
	if ( fd->point_size==ps && fd->screen_font==screen_font )
return( fd );

    fd = galloc(sizeof(struct font_data));
    *fd = *screen_font;
    fd->next = fonts->StolenFromScreen;
    fonts->StolenFromScreen = fd;
    fd->point_size = ps;
    fd->scale_metrics_by = (1000L*ps*disp->res)/
	    PointToPixel(screen_font->point_size,screen_display->res);
    fd->charmap_name = u_copy(fd->charmap_name);
    fd->localname = fd->fontfile = fd->metricsfile = NULL;
    fd->copy_from_screen = true;
    fd->screen_font = screen_font;
return( fd );
}

static void GDrawFillInInstanciationFromScreen(GDisplay *disp,
	struct font_instance *fi, FontRequest *rq) {
    struct font_instance *sfi;
    FontRequest srq;
    int i, fam_name_cnt = fi->fam->name_cnt;
    int old_scale = screen_display->fontstate->allow_scaling;

    /* Does the screen have some fonts that we don't? If it doesn't don't */
    /*  bother, if we are asked for times, it's probably better to use a */
    /*  printer courier than a jaggy scaled times from the screen */
    if ( (screen_display->fontstate->mappings_avail&~disp->fontstate->mappings_avail )==0 )
return;

    srq = *rq;
    srq.point_size = (rq->point_size*disp->res)/screen_display->res;
    screen_display->fontstate->allow_scaling = false;
    sfi = GDrawInstanciateFont(screen_display,rq);

    for ( i=0; i<em_max; ++i ) {
	if ( fi->fonts[i]==NULL && sfi->fonts[i]!=NULL ) {
	    if (( fi->fonts[i] = MakeFontFromScreen(disp,rq->point_size,fi->fam,&srq,i,sfi->fonts[i]))!=NULL )
		fi->level_masks[fam_name_cnt+2] |= (1<<i);
	}
    }
    if ( sfi->unifonts!=NULL ) {
	if ( fi->unifonts==NULL ) {
	    fi->unifonts = gcalloc(fam_name_cnt+ft_max,sizeof(struct font_data *));
	    for ( i=0; i<fam_name_cnt+ft_max; ++i ) {
		if ( fi->unifonts[i]==NULL && sfi->unifonts[i]!=NULL )
		    fi->unifonts[i] = MakeFontFromScreen(disp,rq->point_size,NULL,NULL,0,sfi->unifonts[i]);
	    }
	} else {
	    for ( i=fam_name_cnt; i<fam_name_cnt+ft_max; ++i ) {
		if ( fi->unifonts[i]==NULL && sfi->unifonts[i]!=NULL )
		    fi->unifonts[i] = MakeFontFromScreen(disp,rq->point_size,NULL,NULL,0,sfi->unifonts[i]);
	    }
	}
    }
    screen_display->fontstate->allow_scaling = old_scale;
}

static FontInstance *_GDrawBuildFontInstance(GDisplay *disp, FontRequest *rq,
	struct family_info *fam, FontInstance *fi) {
    struct font_data *main[em_uplanemax], *smallcaps[em_uplanemax];
    int pos[em_uplanemax], val[em_uplanemax];
    FontRequest smallrq;
    struct font_data **unifonts=NULL;
    int i;

    if ( rq->style & fs_smallcaps ) {
	smallrq = *rq;
	smallrq.style &= ~fs_smallcaps;
	if ( smallrq.point_size<12 ) -- smallrq.point_size;
	else if ( smallrq.point_size<15 ) smallrq.point_size -= 2;
	else if ( smallrq.point_size<=24 ) smallrq.point_size -= 4;
	else smallrq.point_size -= smallrq.point_size/6;
    }

    for ( i=0; i<em_uplanemax; ++i ) {
	main[i] = smallcaps[i] = NULL;
	pos[i] = 0x7fff;
    }
    for ( i=0; i<em_uplanemax; ++i ) {
	main[i] = PickFontForEncoding(disp,fam,rq,i,pos,val);
	if ( (rq->style&fs_smallcaps) && main[i]!=NULL && !(main[i]->style&fs_smallcaps)) {
	    int old_size = smallrq.point_size;
	    if ( main[i]->x_height!=0 && main[i]->cap_height!=0 )
		smallrq.point_size = (rq->point_size*main[i]->x_height)/main[i]->cap_height;
	    smallcaps[i] = PickMatchingSmallCaps(disp,main[i]->parent,&smallrq,i);
	    if ( smallcaps[i]==main[i] )
		smallcaps[i]=NULL;
	    smallrq.point_size = old_size;
	}
    }
    /* We have a special unicode hack here because we expect unicode fonts*/
    /*  to be incomplete, and are prepared to try other fonts if they are */
    /*  we expect other fonts to be complete (ie. supply all of their charset) */
    /*  so we don't need alternates for them */
    if ( main[em_unicode]!=NULL &&
	    (unifonts = gcalloc(fam->name_cnt+ft_max,sizeof(struct font_data *)))!=NULL )
	PickUnicodeFonts(disp,unifonts,fam,rq );

    if ( fi==NULL ) {
	fi = gcalloc(1,sizeof(struct font_instance));
	if ( fi==NULL ) {
	    gfree(unifonts);
return( NULL );
	}
	if (( fi->level_masks = gcalloc(fam->name_cnt+4,sizeof(int32)) )==NULL ) {
	    gfree(fi); gfree(unifonts);
return( NULL );
	}
	fi->rq = *rq;
	fi->rq.family_name = u_copy( fi->rq.family_name );
	fi->rq.utf8_family_name = copy( fi->rq.utf8_family_name );
	fi->fam = fam;
	fi->next = fam->instanciations;
	fam->instanciations = fi;
    }

    fi->smallcaps = NULL;
    if ( rq->style&fs_smallcaps ) {
	for ( i=0; i<em_uplanemax; ++i )
	    if ( smallcaps[i]!=NULL )
	break;
	if ( i!=em_uplanemax ) fi->smallcaps = galloc(em_uplanemax*sizeof(struct font_data *));
    }
    for ( i=0; i<em_uplanemax; ++i ) {
	fi->fonts[i] = main[i];
	if ( fi->smallcaps ) fi->smallcaps[i] = smallcaps[i];
	/* If unicode is a better match than the encoding then use it */
	if ( pos[i]!=0x7fff && i<em_max ) {
	    if ( i==em_unicode || pos[i]!=pos[em_unicode] || val[i]<=val[em_unicode] )
		fi->level_masks[pos[i]] |= (1<<i);
	    else
		fi->level_masks[pos[i]+1] |= (1<<i);
	}
    }
    if ( unifonts!=NULL )
	for ( i=0; i<fam->name_cnt+3; ++i )
	    if ( unifonts[i]!=NULL )
		fi->level_masks[i] |= 1<<em_unicode;
    fi->mapped_to = disp;
    fi->unifonts = unifonts;
return( fi );
}

FontInstance *GDrawInstanciateFont(GDisplay *disp, FontRequest *rq) {
    FState *fs;
    unichar_t *temp = NULL;
    unichar_t *requested_families = normalize_font_names(rq->family_name!=NULL?
	    rq->family_name :
	    (temp = utf82u_copy(rq->utf8_family_name)));
    struct family_info *fam;
    struct font_instance *fi;

    free(temp);

    if ( disp==NULL ) disp = screen_display;
    fs = disp->fontstate;

    if ( rq->point_size<0 )	/* It's in pixels, not points, convert to points */
	rq->point_size = PixelToPoint(-rq->point_size,fs->res);

    fam = FindFamily(fs,requested_families);
    if ( fam!=NULL )
	gfree(requested_families);
    else if (( fam = CreateFamily(fs,requested_families))==NULL ) {
	gfree(requested_families);
return( NULL );
    }

    for ( fi = fam->instanciations; fi!=NULL; fi=fi->next )
	if ( fi->rq.point_size == rq->point_size &&
		fi->rq.weight == rq->weight && fi->rq.style == rq->style )
    break;

    if ( fi==NULL ) {
	fi = _GDrawBuildFontInstance(disp,rq,fam,NULL);
	if ( fi==NULL )
return( NULL );
    }
    if ( disp!=screen_display && disp->fontstate->use_screen_fonts )
	GDrawFillInInstanciationFromScreen(disp,fi,rq);
return( fi );
}

FontInstance *GDrawAttachFont(GWindow gw, FontRequest *rq) {
    struct font_instance *fi = GDrawInstanciateFont(gw->display,rq);

    if ( fi!=NULL )
	GDrawSetFont(gw, fi);
return( fi );
}

FontRequest *GDrawDecomposeFont(FontInstance *fi, FontRequest *rq) {
    *rq = fi->rq;
return( rq );
}
/* ************************************************************************** */

static void *_loadFontMetrics(GDisplay *disp, struct font_data *fd, FontInstance *fi) {
    void *ret;
    ret = (disp->funcs->loadFontMetrics)(disp,fd);
    if ( fd->configuration_error && fi!=NULL )
	_GDrawBuildFontInstance(disp,&fi->rq,fi->fam,fi);
return( ret );
}

/* ************************************************************************** */

int32 __GXPDraw_DoText(GWindow w, int32 x, int32 y,
	const unichar_t *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg);
int32 __GXPDraw_DoText8(GWindow w, int32 x, int32 y,
	const char *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg);

static FontMods dummyfontmods = FONTMODS_EMPTY;

struct bounds {
    int xmin, xmax;
    int ymin, ymax;
};

void GDrawFontMetrics(FontInstance *fi,int *as, int *ds, int *ld) {
    struct font_data *fd;
    XFontStruct *fontinfo;
    int i, mi=0, mask=0;

    for ( i=0; i<fi->fam->name_cnt+3; ++i ) for ( mask=1, mi=0; mask!=0; mask<<=1, ++mi ) {
	if ( mask&fi->level_masks[i] )
    goto break_two_loops;
    }
    break_two_loops:;
    if ( mask==0 ) {
	*as = *ds = *ld = 0;
    } else {
	fd = fi->fonts[mi];
	if ( fi->level_masks[i]&(1<<em_unicode) )
	    fd = fi->fonts[em_unicode];
	if ( fd->info==NULL )
	    _loadFontMetrics(fi->mapped_to,fd,fi);
	fontinfo = fd->info;
	*ld = 0;
	*as = fontinfo->ascent;
	*ds = fontinfo->descent;
	if ( fd->scale_metrics_by ) {
	    *as = (*as*fd->scale_metrics_by) / 72000L;
	    *ds = (*ds*fd->scale_metrics_by) / 72000L;
	}
#if 0
	if ( fd->screen_font!=NULL ) {
	    *as = (*as*fd->point_size*printer_fonts.res)/(72*fontinfo->ascent+fontinfo->descent);
	    *ds = (*ds*fd->point_size*printer_fonts.res)/(72*fontinfo->ascent+fontinfo->descent);
	}
#endif
    }
}

int GDrawFontHasCharset(FontInstance *fi,/*enum charset*/int charset) {
return( fi!=NULL && fi->fonts[charset]!=NULL );
}

int32 __GXPDraw_DoText(GWindow w, int32 x, int32 y,
	const unichar_t *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    struct font_instance *fi = w->ggc->fi;

    if ( fi==NULL )
return( 0 );

    if ( mods==NULL )
	mods = &dummyfontmods;
return( _GXPDraw_DoText(w,x,y,text,cnt,mods,col,tf_drawit,arg));
}

int32 __GXPDraw_DoText8(GWindow w, int32 x, int32 y,
	const char *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    struct font_instance *fi = w->ggc->fi;

    if ( fi==NULL )
return( 0 );

    if ( mods==NULL )
	mods = &dummyfontmods;
return( _GXPDraw_DoText8(w,x,y,text,cnt,mods,col,drawit,arg));
}

int32 GDrawDrawText(GWindow gw, int32 x, int32 y,
	const unichar_t *text, int32 cnt, FontMods *mods, Color col) {
    struct tf_arg arg;

return( __GXPDraw_DoText(gw,x,y,text,cnt,mods,col,tf_drawit,&arg));
}

int32 GDrawGetTextWidth(GWindow gw,const unichar_t *text, int32 cnt, FontMods *mods) {
    struct tf_arg arg;

return( __GXPDraw_DoText(gw,0,0,text,cnt,mods,0x0,tf_width,&arg));
}

int32 GDrawGetTextBounds(GWindow gw,const unichar_t *text, int32 cnt, FontMods *mods, GTextBounds *bounds) {
    int ret;
    struct tf_arg arg;

    memset(&arg,'\0',sizeof(arg));
    arg.first = true;
    ret = __GXPDraw_DoText(gw,0,0,text,cnt,mods,0x0,tf_rect,&arg);
    *bounds = arg.size;
return( ret );
}

/* UTF8 routines */

int32 GDrawDrawText8(GWindow gw, int32 x, int32 y, const char *text, int32 cnt, FontMods *mods, Color col) {
    struct tf_arg arg;

return( __GXPDraw_DoText8(gw,x,y,text,cnt,mods,col,tf_drawit,&arg));
}

int32 GDrawGetText8Width(GWindow gw, const char *text, int32 cnt, FontMods *mods) {
    struct tf_arg arg;

return( __GXPDraw_DoText8(gw,0,0,text,cnt,mods,0x0,tf_width,&arg));
}

int32 GDrawGetText8Bounds(GWindow gw,const char *text, int32 cnt, FontMods *mods, GTextBounds *bounds) {
    int ret;
    struct tf_arg arg;

    memset(&arg,'\0',sizeof(arg));
    arg.first = true;
    ret = __GXPDraw_DoText8(gw,0,0,text,cnt,mods,0x0,tf_rect,&arg);
    *bounds = arg.size;
return( ret );
}

/* End UTF8 routines */
