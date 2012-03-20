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

static int UnicodeCharExists(GDisplay *disp, struct font_data *fd, unichar_t ch,
	FontInstance *fi) {
    long minch, maxch;
    XFontStruct *fs;

    if ( ch=='\0' )
return( true );
    if ( fd==NULL )
return( false );

    if ( fd->info==NULL )
	_loadFontMetrics(disp,fd,fi);

    if (( fs = fd->info) == NULL )
return( false );

    minch = (fs->min_byte1<<8) + fs->min_char_or_byte2;
    maxch = (fs->max_byte1<<8) + fs->max_char_or_byte2;
    if ( ch<minch || ch >maxch )
return( false );
    if ( (ch&0xff)<fs->min_char_or_byte2 || (ch&0xff)>fs->max_char_or_byte2 )
return( false );

    if ( fd->exists!=NULL ) {
	int index =
	    ((ch>>8)-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1)+
	    (ch&0xff)-fs->min_char_or_byte2;
return( (fd->exists[index>>3]&(1<<(index&7))) ? 1 : 0 );
    }

    if ( fs->per_char == NULL ) {	/* this means they're all the same (presumably all exist) */

return( true );
    }

    if ( minch==0 && fs->max_char_or_byte2==0xff ) {
return( fs->per_char[ch].attributes & AFM_EXISTS );
    } else {
return( fs->per_char[
	    ((ch>>8)-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1)+
	    (ch&0xff)-fs->min_char_or_byte2].attributes & AFM_EXISTS );
    }
}

/* Returns the charset (index into fi->fonts) of a mapping that works for some*/
/*  of the text. If no good mapping is found it looks hard at a selection of */
/*  unicode fonts and if one of them matches then it returns em_max+index into*/
/*  fi->unifonts array. If absolutely nothing is found we return -1 */
/* If the next argument is not NULL, then it will be set to the character after*/
/*  the run of characters which matches the encoding we found (in the case of */
/*  no encoding, it skips all bad characters) */
enum charset GDrawFindEncoding(unichar_t *text, int32 len, FontInstance *fi,
	unichar_t **next, int *ulevel) {
    uint32 sofar, some, above, last_nonu=0, ourlevel;
    unichar_t ch;
    unichar_t *strt = text, *end = text+(len<0?u_strlen(text):len);
    unichar_t *any_nonu, *nonu_end=NULL;
    int level, i;
    const unsigned long * plane;
    int name_cnt = fi->fam->name_cnt;

    for ( ; text<end && (ch = *text)<0x10000 && iszerowidth(ch) ; ++text );
    if ( text==end ) {
	if ( next!=NULL )
	    *next = text;
return( em_iso8859_1 );		/* Anything will do, no characters */
    }

    above = 0;
    if ( ch=='\t' ) some = 0;
    else for ( level=0; level<name_cnt+3; ++level ) {
	some = (ch>=0x10000)? 0 : unicode_backtrans[ch>>8][ch&0xff] | (1<<em_unicode);
	for ( ; level<name_cnt+3; ++level ) {
	    if ( some&fi->level_masks[level] )
	break;
	    above |= fi->level_masks[level];
	}

	if ( level==name_cnt+3 )
    break;

	sofar = some & (ourlevel = fi->level_masks[level]);
	ourlevel &= ~(1<<em_unicode);
	any_nonu = NULL; nonu_end = NULL;
	if ( sofar & ~(1<<em_unicode)) {
	    any_nonu = text;
	    nonu_end = text;
	    last_nonu = sofar;
	}
	for ( ++text ; text<end; ++text ) {
	    ch = *text;
	    if ( ch=='\t' )
		some = 0;
	    else if ( ch<0x10000 && iscombining(ch))
		/* Combining accents MUST be in the same text clump as the */
		/*  letter they combine with. Even if they aren't in that font*/
	continue;
	    else {
		some = (ch>=0x10000) ? 0 :
			(unicode_backtrans[ch>>8][ch&0xff] | (1<<em_unicode));
	    }
	    if ( some&above )		/* a better font matches this character*/
	break;
	    if ( !(sofar&some) )	/* current font doesn't match */
	break;
	    sofar &= some;
	    if ( sofar & ~(1<<em_unicode)) {
		any_nonu = text;
		last_nonu = sofar;
	    }
	    if ( (some&ourlevel) && nonu_end == NULL )
		nonu_end = text;
	}

	/* unicode font is a last resort, and may be missing characters */
      retry:
	for ( i=0; i<em_unicode; ++i ) {
	    if ( sofar&(1<<i)) {
		if ( next!=NULL ) *next = text;
return( i );
	    }
	}

	/* well, we're supposed to use unicode. Does this font have the chars we need? */
	if ( sofar & (1<<em_unicode) ) {
	    unichar_t *curend = text;
	    for ( text = strt; text<curend; ++text )
		if ( *text<0x10000 && !iscombining(*text) &&
			!UnicodeCharExists(fi->mapped_to,fi->unifonts[level],*text,fi ))
	    break;
	    if ( text!=strt ) {
		if ( text<=any_nonu+1 ) {
		    /* ok the unicode font isn't perfect. Perhaps one of the */
		    /*  other encodings will do better */
		    curend = any_nonu+1;
		    sofar = last_nonu;
      goto retry;
		}
		if ( next!=NULL ) {
		    *next = text;
		    if ( ulevel!=NULL )
			*ulevel = level;
return( em_unicode );
		}
	    }
	}

	/* Ok, we failed to find a unicode font. But that's not the end of the*/
	/*  world, we might have matched with some other font, just not for as*/
	/*  long a string. Try that */
	if ( any_nonu!=NULL ) {
	    for ( i=0; i<em_unicode; ++i ) {
		if ( last_nonu&(1<<i)) {
		    if ( next!=NULL ) *next = any_nonu+1;
return( i );
		}
	    }
	}

	above |= fi->level_masks[level];
	text = strt;
	ch = *text;
	/* level itself is incremented in the for-loop */
    }

    /* ok. If we got here then we didn't find a match. Let's try some other */
    /*  unicode fonts */
    /* We know that the first char doesn't match any mapping we've got other */
    /*  than unicode, but the next char might, so we don't want to return */
    /*  that we can't find a unicode font for it when we've got a perfectly */
    /*  good non-unicode font. nonu_end holds a pointer to the first char */
    /*  for which there's a nonunicode font */
    if ( nonu_end!=NULL && nonu_end<end )
	end = nonu_end;
    above &= ~(1<<em_unicode);
    if ( fi->unifonts!=NULL ) for ( level = 0; level<name_cnt+ft_max; ++level ) if ( fi->unifonts[level]!=NULL ) {
	for ( text = strt; text<end; ++text ) {
	    ch = *text;
	    some = 0;
	    if ( ch>=0x10000 || (text!=strt && iscombining(ch)))
	continue;
	    if ( (plane = unicode_backtrans[ch>>8])!=NULL )
		some = plane[ch&0xff];
	    if ( (some&above) ||
		    UnicodeCharExists(fi->mapped_to,fi->fonts[em_unicode],ch,fi ) ||
		    !UnicodeCharExists(fi->mapped_to,fi->unifonts[level],ch,fi ))
	break;
	}
	if ( text!=strt ) {
	    if ( next!=NULL ) {
	    *next = text;
return( em_max+level );
	    }
	}
    }

    if ( next!=NULL ) {
	for ( text = strt+1; text<end; ++text ) {
	    ch = *text;
	    some = 0;
	    if ( ch>=0x10000 || (text!=strt && iscombining(ch)))
	continue;
	    if ( (plane = unicode_backtrans[ch>>8])!=NULL )
		some = plane[ch&0xff];
	    if ( (some&above) )
	break;
	    if ( UnicodeCharExists(fi->mapped_to,fi->fonts[em_unicode],ch,fi ))
	break;
	    if ( fi->unifonts!=NULL ) for ( level = 0; level<name_cnt; ++level ) {
		if ( UnicodeCharExists(fi->mapped_to,fi->unifonts[level],ch,fi ))
	goto break_out;
	    }
	}
	break_out:;
	*next = text;
    }

return( -1 );
}

#define BottomAccent	0x300
#define TopAccent	0x345

/* some synonems for accents in the range above. Postscript fonts tend to */
/*  have accents in the 0x2xx range rather than the 0x3xx */
static const unichar_t accents[][3] = {
    { 0x2cb/*, 0x60*/ },	/* grave */
    { 0x2ca, 0xb4 },		/* acute */
    { 0x2c6, 0x5e },		/* circumflex */
    { 0x2dc, 0x7e },		/* tilde */
    { 0x2c9, 0xaf },		/* macron */
    { 0x305, 0xaf },		/* overline, (macron is suggested as a syn, but it's not quite right) */
    { 0x2d8 },			/* breve */
    { 0x2d9, '.' },		/* dot above */
    { 0xa8 },			/* diaeresis */
    { 0x2c0 },			/* hook above */
    { 0x2da, 0xb0 },		/* ring above */
    { 0x2dd },			/* double acute */
    { 0x2c7 },			/* caron */
    { 0x2c8, 0x384, '\''  },	/* vertical line, tonos */
    { '"' },			/* double vertical line */
    { 0 },			/* double grave */
    { 0 },			/* cand... */		/* 310 */
    { 0 },			/* inverted breve */
    { 0x2bb },			/* turned comma */
    { 0x2bc, ',' },		/* comma above */
    { 0x2bd },			/* reversed comma */
    { 0x2bc, ',' },		/* comma above right */
    { 0x2cb, 0x60, 0x300 },	/* grave below */
    { 0x2ca, 0xb4, 0x301 },	/* acute below */
    { 0 },			/* left tack */
    { 0 },			/* right tack */
    { 0 },			/* left angle */
    { ',' },			/* horn */
    { 0 },			/* half ring */
    { 0x2d4 },			/* up tack */
    { 0x2d5 },			/* down tack */
    { 0x2d6, '+' },		/* plus below */
    { 0x2d7, '-' },		/* minus below */	/* 320 */
    { 0x2b2 },			/* hook */
    { 0 },			/* back hook */
    { 0x2d9, '.' },		/* dot below */
    { 0xa8 },			/* diaeresis below */
    { 0x2da, 0xb0 },		/* ring below */
    { 0x2bc, ',' },		/* comma below */
    { 0xb8 },			/* cedilla */
    { 0x2db },			/* ogonek */		/* 0x328 */
    { 0x2c8, 0x384, '\''  },	/* vertical line below */
    { 0 },			/* bridge below */
    { 0 },			/* double arch below */
    { 0x2c7 },			/* caron below */
    { 0x2c6, 0x52 },		/* circumflex below */
    { 0x2d8 },			/* breve below */
    { 0 },			/* inverted breve below */
    { 0x2dc, 0x7e },		/* tilde below */	/* 0x330 */
    { 0x2c9, 0xaf },		/* macron below */
    { '_' },			/* low line */
    { 0 },			/* double low line */
    { 0x2dc, 0x7e },		/* tilde overstrike */
    { '-' },			/* line overstrike */
    { '_' },			/* long line overstrike */
    { '/' },			/* short solidus overstrike */
    { '/' },			/* long solidus overstrike */	/* 0x338 */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0x2cb, 0x60 },		/* tone mark, left of circumflex */ /* 0x340 */
    { 0x2ca, 0xb4 },		/* tone mark, right of circumflex */
    { 0x2dc, 0x7e },		/* perispomeni (tilde) */
    { 0x2bc, ',' },		/* koronis */
    { 0 },			/* dialytika tonos (two accents) */
    { 0x37a },			/* ypogegrammeni */
    { 0xffff }
};

static int EncodingPosInMapping(int enc,unichar_t ch, FontMods *mods) {
    struct charmap2 *table2 = NULL;
    unsigned short *plane2;
    int highch;

    if ( mods!=NULL && ( mods->mods&(tm_initialcaps|tm_upper|tm_lower) )) {
	int started = mods->starts_word;
	mods->starts_word = isspace(ch) && !isnobreak(ch);
	if ( mods->mods&tm_initialcaps ) {
	    if ( started ) ch = totitle(ch);
	} else if ( mods->mods&tm_upper ) {
	    ch = toupper(ch);
	} else if ( mods->mods&tm_lower ) {
	    ch = tolower(ch);
	}
    }
    if ( ch==0xa0 ) ch=' ';	/* Many iso8859-1 fonts have a zero width nbsp */

    if ( enc==em_unicode )
return( ch );

    if ( enc<em_first2byte ) {
	struct charmap *table = alphabets_from_unicode[enc+3];	/* Skip the 3 asciis */
	unsigned char *plane;
	highch = ch>>8;
	if ( highch>=table->first && highch<=table->last &&
		(plane = table->table[highch-table->first])!=NULL &&
		(ch=plane[ch&0xff])!=0 )
return( ch );

return( -1 );		/* Not in encoding */
    }

    if ( enc<=em_jis212 )
	table2 = &jis_from_unicode;
    else if ( enc==em_gb2312 )
	table2 = &gb2312_from_unicode;
    else if ( enc==em_ksc5601 )
	table2 = &ksc5601_from_unicode;
    else if ( enc==em_big5 )
	table2 = &big5_from_unicode;
    else if ( enc==em_johab )
	table2 = &johab_from_unicode;
    else
return( -1 );

    highch = ch>>8;
    if ( highch>=table2->first && highch<=table2->last &&
	    (plane2 = table2->table[highch-table2->first])!=NULL &&
	    (ch=plane2[ch&0xff])!=0 ) {
	if ( enc==em_jis212 ) {
	    if ( !(ch&0x8000 ) )
return( -1 );
	    ch &= 0x8000;
	} else if ( enc==em_jis208 && (ch&0x8000) )
return( -1 );

return( ch );
    }
return( -1 );
}

static struct font_data *PickAccentFont(struct font_instance *fi,
	struct font_data *tryme, unichar_t ch, unichar_t *accent) {
    const unichar_t *apt, *aend;
    int i, level, some;

    if ( tryme!=NULL ) {
	if ( EncodingPosInMapping(tryme->map,ch, NULL)!=-1 ) {
	    *accent = ch;
return( tryme );
	}
	if ( ch>=BottomAccent && ch<=TopAccent ) {
	    apt = accents[ch-BottomAccent]; aend = apt+3;
	    while ( apt<aend && *apt!='\0' ) {
		if ( EncodingPosInMapping(tryme->map,*apt, NULL)!=-1 ) {
		    *accent = *apt;
return( tryme );
		}
		++apt;
	    }
	}
    }

    for ( level=0; level<fi->fam->name_cnt+3; ++level ) {
	some = 0;
	if ( ch<0x10000 )
	    some = unicode_backtrans[ch>>8][ch&0xff] | (1<<em_unicode);
	some &= fi->level_masks[level];
	if ( some==(1<<em_unicode) ) {
	    if ( UnicodeCharExists(fi->mapped_to,fi->unifonts[level],ch,fi)) {
		*accent = ch;
return( fi->unifonts[level]);
	    }
	} else if ( some!=0 ) {
	    for ( i=0; i<em_unicode; ++i ) {
		if ( some&(1<<i)) {
		    *accent = ch;
return( fi->fonts[i]);
		}
	    }
	}
	if ( ch>=BottomAccent && ch<=TopAccent ) {
	    apt = accents[ch-BottomAccent]; aend = apt+3;
	    while ( apt<aend && *apt!='\0' ) {
		some = unicode_backtrans[ch>>8][ch&0xff] | (1<<em_unicode);
		some &= fi->level_masks[level];
		if ( some==(1<<em_unicode) &&
			UnicodeCharExists(fi->mapped_to,fi->unifonts[level],*apt,fi)) {
		    *accent = *apt;
return( fi->unifonts[level]);
		} else if ( some!=0 ) {
		    for ( i=0; i<em_unicode; ++i ) {
			if ( some&(1<<i)) {
			    *accent = *apt;
return( fi->fonts[i]);
			}
		    }
		}
		++apt;
	    }
	}
    }
return( NULL );
}

/* ************************************************************************** */

static int TextWidth1(struct font_data *fd,char *transbuf,int len) {
    int offset = fd->info->min_char_or_byte2, ch1, ch2, bound = fd->info->max_char_or_byte2;
    register XCharStruct *per_char = fd->info->per_char;
    register unsigned char *tpt = (unsigned char *) transbuf, *end = tpt+len;
    register struct kern_info **kerns=fd->kerns, *kpt;
    int width = 0;

    if ( per_char==NULL )
return( len*fd->info->max_bounds.width );

    if ( kerns==NULL ) {
	ch1 = *tpt-offset;
	while ( tpt<end ) {
	    if ( ch1>=0 && *tpt<bound )
		width += per_char[ch1].width;
	    ch1 = *++tpt-offset;
	}
    } else {
	ch1 = *tpt-offset;
	while ( tpt<end ) {
	    if ( ch1>=0 )
		width += per_char[ch1].width;
	    ch2 = *++tpt;
	    if ( (kpt=kerns[ch1])!=NULL && tpt<end ) {
		while ( kpt!=NULL && kpt->following!=ch2 ) kpt = kpt->next;
		if ( kpt ) width += kpt->kern;
	    }
	    ch1 = ch2-offset;
	}
    }
    if ( fd->scale_metrics_by )
	width = (width*fd->scale_metrics_by) / 72000L;
return( width );
}

static int TextWidth2(struct font_data *fd,GChar2b *transbuf,int len) {
    XFontStruct *info = fd->info;
    int offset1 = info->min_byte1, offset2 = info->min_char_or_byte2;
    int row = (info->max_char_or_byte2-offset2+1);
    int tot = row*(info->max_byte1-offset1+1);
    int ch1;
    register XCharStruct *per_char = info->per_char;
    register GChar2b *tpt = transbuf, *end = tpt+len;
    int width = 0;

    /* if we scale jis208 then the XFontStruct comes back with per_char NULL */
    /*  I speculate that this only happens for monospace fonts and use */
    /*  the max_bounds */ /* Ah, yes. It does */
    if ( per_char!=NULL ) {
	ch1 = (tpt->byte1-offset1)*row+tpt->byte2-offset2;
	while ( tpt<end ) {
	    if ( ch1>=0 && ch1<tot )
		width += per_char[ch1].width;
	    else
		/*width += info->max_bounds.width*/;
	    ++tpt;
	    ch1 = (tpt->byte1-offset1)*row+tpt->byte2-offset2;
	}
    } else
	width = len*info->max_bounds.width;
    if ( fd->scale_metrics_by )
	width = (width*fd->scale_metrics_by) / 72000L;
return( width );
}

static struct font_data *UnicodeFontWithReplacementChar(FontInstance *fi) {
    struct font_data *ufont;
    int i;

    ufont = fi->fonts[em_unicode];
    if ( ufont!=NULL && !UnicodeCharExists(fi->mapped_to, ufont, 0xfffd,fi))
	ufont = NULL;
    if ( fi->unifonts!=NULL ) {
	for ( i=0; i<fi->fam->name_cnt+ft_max-1 && ufont==NULL; ++i ) {
	    ufont = fi->unifonts[i];
	    if ( ufont!=NULL && !UnicodeCharExists(fi->mapped_to, ufont, 0xfffd,fi))
		ufont = NULL;
	}
    }
return( ufont );
}
#if 0
static struct font_data *AsciiFont(FontInstance *fi) {
    struct font_data *afont=NULL;
    int i;

    for ( i=em_iso8859_1 ; afont==NULL && i<em_symbol; ++i )
	afont = fi->fonts[i];
return( afont );
}
#endif

static int RealAsDs(struct font_data *fd,char *transbuf,int len,struct tf_arg *arg) {
    XFontStruct *info = fd->info;
    int offset = info->min_char_or_byte2, ch1, maxc = info->max_char_or_byte2-offset;
    register XCharStruct *per_char = info->per_char;
    register unsigned char *tpt = (unsigned char *) transbuf, *end = tpt+len;
    int ds = -8000, as = -8000;
    int first = arg->first;
    int rbearing = 0;

    if ( tpt==end )
return( 0 );

    if ( per_char==NULL ) {
	if ( first ) {
	    arg->first = false;
	    if ( fd->scale_metrics_by )
		arg->size.lbearing = (fd->scale_metrics_by*info->max_bounds.lbearing)/72000;
	    else
		arg->size.lbearing = info->max_bounds.lbearing;
	}
	ds = info->max_bounds.descent;
	as = info->max_bounds.ascent;
	rbearing = info->max_bounds.rbearing-info->max_bounds.width;
    } else {
	ch1 = *tpt-offset;
	while ( tpt<end ) {
	    if ( ch1<=maxc ) {
		if ( first ) {
		    arg->first = first = false;
		    if ( fd->scale_metrics_by )
			arg->size.lbearing = (fd->scale_metrics_by*per_char[ch1].lbearing)/72000;
		    else
			arg->size.lbearing = per_char[ch1].lbearing;
		}
		if ( ch1>=0 ) {
		    if ( per_char[ch1].descent>ds )
			ds = per_char[ch1].descent;
		    if ( per_char[ch1].ascent>as )
			as = per_char[ch1].ascent;
		    rbearing = per_char[ch1].rbearing-per_char[ch1].width;
		}
	    }
	    ch1 = *++tpt - offset;
	}
    }
    if ( fd->scale_metrics_by ) {
	/* different fonts may scale by different amounts, so we can't just */
	/*  resolve scaling at end of entire process */
	as = (as*fd->scale_metrics_by)/72000;
	ds = (ds*fd->scale_metrics_by)/72000;
	rbearing = (rbearing*fd->scale_metrics_by)/72000;
    }
    if ( as>arg->size.as ) arg->size.as = as;
    if ( ds>arg->size.ds ) arg->size.ds = ds;
    arg->size.rbearing = rbearing;
return( ds );
}

static int RealAsDs16(struct font_data *fd,GChar2b *transbuf,int len, struct tf_arg *arg) {
    XFontStruct *info = fd->info;
    int offset1 = info->min_byte1, offset2 = info->min_char_or_byte2;
    int row = (info->max_char_or_byte2-offset2+1);
    int tot = row*(info->max_byte1-offset1+1);
    int ch1;
    register XCharStruct *per_char = info->per_char;
    register XChar2b *tpt = (XChar2b *) transbuf, *end = tpt+len;
    int ds = -8000, as = -8000;
    int first = arg->first;
    int rbearing = 0;

    if ( tpt==end )
return( 0 );

    if ( per_char!=NULL ) {
	ch1 = (tpt->byte1-offset1)*row+tpt->byte2-offset2;
	while ( tpt<end ) {
	    if ( ch1>=0 && ch1<tot ) {
		if ( first ) {
		    arg->first = first = false;
		    if ( fd->scale_metrics_by )
			arg->size.lbearing = (fd->scale_metrics_by*per_char[ch1].lbearing)/72000;
		    else
			arg->size.lbearing = per_char[ch1].lbearing;
		}
		if ( per_char[ch1].descent>ds )
		    ds = per_char[ch1].descent;
		if ( per_char[ch1].ascent>as )
		    as = per_char[ch1].ascent;
		rbearing = per_char[ch1].rbearing-per_char[ch1].width;
	    }
	    ++tpt;
	    ch1 = (tpt->byte1-offset1)*row+tpt->byte2-offset2;
	}
    } else {
	if ( first ) {
	    arg->first = false;
	    if ( fd->scale_metrics_by )
		arg->size.lbearing = (fd->scale_metrics_by*info->max_bounds.lbearing)/72000;
	    else
		arg->size.lbearing = info->max_bounds.lbearing;
	}
	ds = info->max_bounds.descent;
	as = info->max_bounds.ascent;
	rbearing = info->max_bounds.rbearing-info->max_bounds.width;
    }
    if ( fd->scale_metrics_by ) {
	/* different fonts may scale by different amounts, so we can't just */
	/*  resolve scaling at end of entire process */
	as = (as*fd->scale_metrics_by)/72000;
	ds = (ds*fd->scale_metrics_by)/72000;
	rbearing = (rbearing*fd->scale_metrics_by)/72000;
    }
    if ( as>arg->size.as ) arg->size.as = as;
    if ( ds>arg->size.ds ) arg->size.ds = ds;
    arg->size.rbearing = rbearing;
return( ds );
}

static int32 _GDraw_DoText(GWindow gw, int32 x, int32 y,
	unichar_t *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg);
static int32 _GDraw_ScreenDrawToImage(GWindow gw, struct font_data *fd, struct font_data *sc,
	int enc, int32 x, int32 y,
	unichar_t *text, unichar_t *end, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *oldarg);

static int32 _GDraw_DrawUnencoded(GWindow gw, FontInstance *fi,
	int32 x, int32 y,
	unichar_t *text, unichar_t *end, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    unichar_t *pt, *npt=NULL;
    int usearched=false, uwidth=0;
    long ch;
    struct font_data *ufont = NULL;
    int cw=0;
    int pixel_size = PointToPixel(fi->rq.point_size,fi->mapped_to->res);
    int letter_spacing = PointTenthsToPixel(mods->letter_spacing,fi->mapped_to->res);
    int32 xbase = x;
    GRect rct;

    while ( text<end ) {

	/* first skip over until we can find a character we know how to draw */
	/*  from first principles (ie. a bullet is a filled circle) */
	for ( pt=text; pt<end; ++pt ) {
	    ch = *pt;
	    if ( ch<0x20 || ( ch>=0x80 && ch<0xa0 ) ||
		    ( ch>=0x2000 && ch<=0x2015 ) ||
		    ( ch>=0x2022 && ch<=0x2023 ) ||
		    ( ch>=0x2028 && ch<=0x202e ) ||
		    ch == 0x2043 ||
		    ( ch>=0x206a && ch<=0x206f ) ||
		    ( ch>=0x2190 && ch<=0x2193 ) ||
		    ch == 0xfeff ||
		    (ch<0x10000 && iszerowidth(ch)) ||
		    (ch<0x10000 && iscombining(ch) && pt!=text) )
	break;
	}

	/* ok the characters between pt and text aren't things we know about */
	/* now there are three cases: */
	/* some of those characters have an alternate representation */
	/*  for these we convert and recurse, trying to print alternates */
	/* for the rest we either print as a bunch of unicode replacement chars*/
	/*  (if we have a replacement char) */
	/* or we create a rectangle */
	if ( pt!=text ) {
	    unichar_t *strt;
	    unichar_t altbuf[40], *apt, *aend = altbuf+sizeof(altbuf)/sizeof(unichar_t)-18;
	    const unichar_t *const *ua_plane, *str;
	    static GChar2b replacement_chr = { 0xff, 0xfd };
	    int subdrawit = drawit;
	    unichar_t accent;

	    apt = altbuf;
	    if ( arg!=NULL && !arg->dont_replace ) for ( npt=text; npt<pt && apt<aend ; ++npt ) {
		ch = *npt;
		/* If it's not in a mapping see if there's an alternate */
		/*  representation which might be (ie half vs. full width) */
		/*  Also decomposes accented letters, long s to short, ligs, etc. */
		if ( ch<0x10000 && (ua_plane = unicode_alternates[ch>>8])!=NULL &&
			(str=ua_plane[ch&0xff])!=NULL &&
			PickAccentFont(fi,NULL,*str,&accent)!=NULL ) {
		    /* The final check means we've got at least one character*/
		    /*  that can be drawn in some font. Without this there's */
		    /*  no point in doing an alternate representation */
		    while ( *str )
			*apt++ = *str++;
		} else
	    break;
		if ( drawit>=tf_stopat ) {
		    ++npt;
		    subdrawit = tf_width;
	    break;
		}
	    }
	    if ( apt!= altbuf ) {
		text = npt;
		arg->dont_replace = true;
		x += cw = _GDraw_DoText(gw,x,y,altbuf,apt-altbuf,mods,col,
			subdrawit,arg)+letter_spacing;
		arg->dont_replace = false;
		if ( drawit>=tf_stopat ) {
		    if ( arg->width+cw >= arg->maxwidth ) {
			unichar_t *stop = npt;
			if (( drawit==tf_stopat && arg->width+cw/2>arg->maxwidth) ||
				drawit==tf_stopafter ) {
			    /* Do Nothing */;
			} else {
			    x -= cw;
			    --stop;
			}
			arg->last = stop; arg->width += cw;
return( x-xbase-letter_spacing );
		    } else
			arg->width += cw;
	continue;			/* In this mode we do one char at a time */
		} else if ( apt>=aend )	/* we didn't finish, go back for more */
	continue;
	    }
	    /* We've done all the chars for which we've got alternatives */
	    /*  now find the longest string we can which have no alternates */
	    /*  (and which aren't in our magic list of known chars) */
	    if ( arg!=NULL && !arg->dont_replace && npt<pt ) {
		/* the first character is allowed to have an alternate */
		/*  if we failed to find a font for it we'll drop down to here*/
		for ( npt=text+1 ; npt<pt; ++npt )
		    if ( *npt<0x10000 && (ua_plane = unicode_alternates[*npt>>8])!=NULL &&
			    ua_plane[*npt&0xff]!=NULL )
		break;
	    } else
		npt = pt;

	    if ( !usearched ) {
		usearched = true;
		ufont = UnicodeFontWithReplacementChar(fi);
		if ( ufont==NULL )
		    uwidth = pixel_size/2;
		else if ( ufont->info->per_char==NULL ) {
		    uwidth = ufont->info->max_bounds.width;
		    if ( ufont->scale_metrics_by )
			uwidth = (uwidth*ufont->scale_metrics_by)/ 72000L;
		} else {
		    uwidth = ufont->info->per_char[0xfffd-ufont->info->min_char_or_byte2].width;
		    if ( ufont->scale_metrics_by )
			uwidth = (uwidth*ufont->scale_metrics_by)/ 72000L;
		}
	    }
	    if ( npt==text )
		;
	    else if ( drawit==tf_width )
		;
	    else if ( drawit==tf_rect ) {
		arg->size.rbearing = x-xbase;
		if ( ufont!=NULL ) {
		    RealAsDs16(ufont,&replacement_chr,1,arg);
		} else {
		    int as = 8*pixel_size/10, ds = -pixel_size/10;
		    if ( arg->size.as<as ) arg->size.as = as;
		    if ( arg->size.ds<ds ) arg->size.ds = ds;
		}
	    } else if ( drawit>=tf_stopat ) {
		if ( arg->width+(npt-text)*(uwidth+letter_spacing) >= arg->maxwidth ) {
		    int off = (arg->maxwidth-arg->width)/(uwidth+letter_spacing);
		    unichar_t *stop = text+off;
		    int wid = arg->width+off*(uwidth+letter_spacing);
		    if ( wid==arg->maxwidth )
			/* all is happy */;
		    else if (( drawit==tf_stopat && wid+uwidth/2>arg->maxwidth) ||
			    drawit==tf_stopafter ) {
			++stop;
			wid += uwidth;
		    }
		    arg->last = stop; arg->width += arg->maxwidth;
return( x-xbase+(stop-text)*(uwidth+letter_spacing)-letter_spacing );
		} else
		    arg->width += (npt-text)*(uwidth+letter_spacing);
	    } else if ( ufont!=NULL ) {
		while ( text<npt ) {
		    if ( ufont->screen_font==NULL ) {
			GChar2b buf[40], *bpt; 
			for ( strt=text, bpt=buf; strt<npt && strt<text+30; ++strt ) {
			    bpt->byte1 = 0xff; bpt++->byte2 = 0xfd;
			}
			(gw->display->funcs->drawText2)(gw,ufont,x,y,buf,bpt-buf,mods,col);
		    } else {
			unichar_t buf[40], *bpt; 
			for ( strt=text, bpt=buf; strt<npt && strt<text+30; ++strt ) {
			    *bpt++ = 0xfffd;
			}
			_GDraw_ScreenDrawToImage(gw,ufont,NULL,em_unicode,x,y,
				buf,bpt,mods,col,tf_drawit,NULL);
		    }
		    x += (strt-text)*(uwidth+letter_spacing);
		    text = strt;
		}
	    } else {
		rct.x = x+1; rct.y = y-8*pixel_size/10;
		rct.width = (npt-text)*uwidth + (npt-text-1)*letter_spacing - 3;
		rct.height = 7*pixel_size/10;
		GDrawDrawRect(gw,&rct,col);
	    }
	    x += (npt-text)*(uwidth+letter_spacing);
	    text = npt;
	    if ( npt!=pt )
    continue;
	}

	if ( pt<end ) {
	    GTextBounds sz;
	    if ( drawit==tf_rect ) {
		sz.as = sz.ds = -8000;
		sz.rbearing = sz.lbearing = 0;
	    }
	    ch = *pt;
	    if ( ch=='\t' ) {
		/* tab hack */
		cw = 3*pixel_size/2;
	    } else if ( ch<0x10000 && (iszerowidth(ch) || (iscombining(ch) && pt!=text)) )
		cw = 0;
		/* zero width spaces, control characters, etc. */
		/* accents modifying something */
	    else if ( ch==0x2000 || ch==0x2002 )/* En Space */
		cw = pixel_size/2;
	    else if ( ch==0x2001 || ch==0x2003 )/* Em Space */
		cw = pixel_size;
	    else if ( ch==0x2004 || ch==0x2007 )/* 3 per em */
		cw = pixel_size/3;
	    else if ( ch==0x2005 )		/* 4 per em */
		cw = pixel_size/4;
	    else if ( ch==0x2006 || ch==0x2008 )/* 6 per em */
		cw = pixel_size/6;
	    else if ( ch==0x2009 )		/* thin */
		cw = pixel_size/8;
	    else if ( ch==0x200a )		/* hair */
		cw = pixel_size/16;
	    else if ( ch==0x2010 || ch==0x2011 ) {/* hyphens */
		if ( drawit==tf_drawit )
		    GDrawDrawLine(gw,x,y-pixel_size/3,
			    x+pixel_size/4-1,y-pixel_size/3,col);
		else if ( drawit==tf_rect ) {
		    sz.as = pixel_size/3; sz.ds = -pixel_size/3;
		    sz.rbearing = pixel_size/4-1;
		}
		cw = pixel_size/4+pixel_size/10;
	    } else if ( ch==0x2012 || ch==0x2013 ) {/* En, figure dash */
		if ( drawit==tf_drawit )
		    GDrawDrawLine(gw,x,y-pixel_size/3,
			    x+pixel_size/2-1,y-pixel_size/3,col);
		else if ( drawit==tf_rect ) {
		    sz.as = pixel_size/3; sz.ds = -pixel_size/3;
		    sz.rbearing = pixel_size/2-1;
		}
		cw = pixel_size/2+pixel_size/10;
	    } else if ( ch==0x2014 ) {		/* Em dash */
		if ( drawit==tf_drawit )
		    GDrawDrawLine(gw,x,y-pixel_size/3,
			    x+pixel_size-1,y-pixel_size/3,col);
		else if ( drawit==tf_rect ) {
		    sz.as = pixel_size/3; sz.ds = -pixel_size/3; sz.rbearing = pixel_size-1;
		}
		cw = pixel_size+pixel_size/10;
	    } else if ( ch==0x2015 ) {		/* Quote dash */
		if ( drawit==tf_drawit )
		    GDrawDrawLine(gw,x,y-pixel_size/3,
			    x+(4*pixel_size)/3-1,y-pixel_size/3,col);
		else if ( drawit==tf_rect ) {
		    sz.as = pixel_size/3; sz.ds = -pixel_size/3; sz.rbearing = (4*pixel_size)/3-1;
		}
		cw = (4*pixel_size)/3+pixel_size/10;
	    } else if ( ch==0x2022 ) {		/* Bullet */
		if ( drawit==tf_drawit ) {
		    rct.x = x+1; 			rct.y = y-pixel_size/5-pixel_size/3;
		    rct.width = pixel_size/3;	rct.height = pixel_size/3-1;
		    GDrawFillElipse(gw,&rct,col);
		    GDrawDrawElipse(gw,&rct,col);
		} else if ( drawit==tf_rect ) {
		    sz.as = pixel_size/3+pixel_size/5; sz.ds = -pixel_size/5; sz.rbearing = pixel_size/3;
		}
		cw = pixel_size/3+pixel_size/10;
	    } else if ( ch==0x2023 ) {		/* Triangle Bullet */
		if ( drawit==tf_drawit ) {
		    GPoint tri[4];
		    tri[0].x = x+1;			tri[0].y = y-7*pixel_size/10;
		    tri[1].x = x+1+3*pixel_size/10;	tri[1].y = y-4*pixel_size/10;
		    tri[2].x = x+1;			tri[2].y = y-1*pixel_size/10;
		    tri[3] = tri[0];
		    GDrawFillPoly(gw,tri,4,col);
		} else if ( drawit==tf_rect ) {
		    sz.as = 7*pixel_size/10; sz.ds = -pixel_size/10; sz.rbearing = 3*pixel_size/10;
		}
		cw = 4*pixel_size/10;
	    } else if ( ch==0x2043 ) {		/* Hyphen Bullet */
		if ( drawit==tf_drawit ) {
		    rct.x = x+1; 			rct.y = y-pixel_size/3-pixel_size/10;
		    rct.width = pixel_size/4-1;		rct.height = pixel_size/5;
		    GDrawFillRect(gw,&rct,col);
		    GDrawDrawRect(gw,&rct,col);
		} else if ( drawit==tf_rect ) {
		    sz.as = pixel_size/3; sz.ds = -pixel_size/3; sz.rbearing = pixel_size/4-1;
		}
		cw = pixel_size/4;
	    } else if ( ch==0x2191 || ch==0x2193 ) {	/* Vertical Arrows */
		int awid = pixel_size/6;
		int xmid = x+pixel_size/10+awid;
		int ytop = y-7*pixel_size/10;
		if ( drawit==tf_drawit )
		    GDrawDrawLine(gw,xmid,y,xmid,ytop,col);
		if ( drawit==tf_drawit && ch==0x2191 ) {
		    GDrawDrawLine(gw,xmid,ytop,xmid-awid,ytop+awid,col);
		    GDrawDrawLine(gw,xmid,ytop,xmid+awid,ytop+awid,col);
		} else if ( drawit==tf_rect ) {
		    sz.as = 7*pixel_size/10; sz.ds = 0; sz.rbearing = 2*awid;
		    sz.lbearing = xmid-awid-x;
		} else {
		    GDrawDrawLine(gw,xmid,y,xmid-awid,y-awid,col);
		    GDrawDrawLine(gw,xmid,y,xmid+awid,y-awid,col);
		}
		x = xmid + awid + pixel_size/10;
	    } else if ( ch==0x2190 || ch==0x2192 ) {	/* Horizontal Arrows */
		int awid = pixel_size/6;
		int ymid = y-3*pixel_size/10;
		int xend = x+7*pixel_size/10;
		if ( drawit==tf_drawit )
		    GDrawDrawLine(gw,x+1,ymid,xend,ymid,col);
		if ( drawit==tf_drawit && ch==0x2190 ) {
		    GDrawDrawLine(gw,x+1,ymid,x+1+awid,ymid-awid,col);
		    GDrawDrawLine(gw,x+1,ymid,x+1+awid,ymid+awid,col);
		} else if ( drawit==tf_drawit && ch==0x2192 ) {
		    GDrawDrawLine(gw,xend,ymid,xend-awid,ymid-awid,col);
		    GDrawDrawLine(gw,xend,ymid,xend-awid,ymid+awid,col);
		} else if ( drawit==tf_rect ) {
		    sz.as = pixel_size/2; sz.ds = -awid; sz.rbearing = xend-x;
		    sz.lbearing = 1;
		}
		cw = 8*pixel_size/10;
	    }
	    if ( drawit==tf_rect ) {
		if ( arg->size.as<sz.as ) arg->size.as = sz.as;
		if ( arg->size.ds<sz.ds ) arg->size.ds = sz.ds;
		arg->size.rbearing = cw-sz.rbearing;
		if ( arg->first ) {
		    arg->size.lbearing = sz.lbearing;
		    arg->first = false;
		}
	    } else if ( drawit>=tf_stopat ) {
		if ( arg->width+cw >= arg->maxwidth ) {
		    unichar_t *stop = pt;
		    if (( drawit==tf_stopat && arg->width+cw/2>arg->maxwidth) ||
			    drawit==tf_stopafter ) {
			++stop;
			x+= cw+letter_spacing;
		    }
		    arg->last = stop; arg->width += cw+letter_spacing;
return( x-xbase-letter_spacing );
		} else
		    arg->width += (cw+letter_spacing);
	    }
	    x += cw+letter_spacing;
	    text = pt+1;
	} else
	    text = pt;
    }
return( x-xbase-letter_spacing );
}

static int32 _GDraw_Transform(GWindow gw, struct font_data *fd, struct font_data *sc,
	int enc, int32 x, int32 y,
	unichar_t *text, unichar_t *end, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    long ch;
    int olen=0;
    int32 dist = 0;
    int started, lcstate, starts_word = mods->starts_word;
    struct charmap *table = NULL;
    unsigned char *plane; char *opt;
    struct charmap2 *table2 = NULL;
    unsigned short *plane2; GChar2b *opt2;
    struct font_data *tempfd;
    GDisplay *disp = gw->display;
    int letter_spacing = mods->letter_spacing;
    unichar_t *strt = text;
    int cw=0;
#define TB_MAX	512
    union {
	char tb[TB_MAX];
	GChar2b tb2[TB_MAX];
    } tb;
#define transbuf	(tb.tb)
#define transbuf2	(tb.tb2)

    if ( enc<em_first2byte )
	table = alphabets_from_unicode[enc+3];	/* Skip the 3 asciis */
    else if ( enc<=em_jis212 )
	table2 = &jis_from_unicode;
    else if ( enc==em_gb2312 )
	table2 = &gb2312_from_unicode;
    else if ( enc==em_ksc5601 )
	table2 = &ksc5601_from_unicode;
    else if ( enc==em_big5 )
	table2 = &big5_from_unicode;

    while ( text<end ) {
	opt = transbuf; opt2 = transbuf2; olen = 0;
	lcstate = -1;
	while ( olen<TB_MAX-4 && text<end ) {
	    ch = *text++;
	    if ( !mods->has_charset && enc<em_max ) {
		started = starts_word;
		starts_word = isspace(ch) && !isnobreak(ch);
		if ( ch==_SOFT_HYPHEN && !(mods->mods&tm_showsofthyphen))
	continue;
		if ( ch<0x10000 && ( iszerowidth(ch) || (iscombining(ch) && text-1!=strt)))
	continue;
		if ( mods->mods&(tm_initialcaps|tm_upper|tm_lower) ) {
		    if ( mods->mods&tm_initialcaps ) {
			if ( started && ch==_DOUBLE_S ) { *opt++ = opt2->byte2 = 'S'; opt2++->byte1 = '\0'; ++olen; ch='s'; }
			else if ( started ) ch = totitle(ch);
		    } else if ( mods->mods&tm_upper ) {
			if ( ch==_DOUBLE_S ) { *opt++ = opt2->byte2 = 'S'; opt2++->byte1 = '\0'; ++olen; ch='S'; }
			else ch = toupper(ch);
		    } else if ( mods->mods&tm_lower ) {
			ch = tolower(ch);
		    }
		}
		if ( sc!=NULL ) {
		    if ( lcstate==-1 ) lcstate = islower(ch);
		    else if ( lcstate!=islower(ch)) {
			--text;
	break;
		    }
		    if ( lcstate ) {
			if ( ch==_DOUBLE_S ) { *opt++ = opt2->byte2 = 'S'; opt2++->byte1 = '\0'; ++olen; ch='S'; }
			else if ( islower(ch)) ch = toupper(ch);
		    }
		}

		if ( ch==0xa0 ) ch=' ';	/* Many iso8859-1 fonts have a zero width nbsp */
	    }
	    if ( enc<em_first2byte ) {
		int highch = ch>>8;
		if ( highch>=table->first && highch<=table->last &&
			(plane = table->table[highch-table->first])!=NULL &&
			(ch=plane[ch&0xff])!=0 ) {
		    *opt++ = ch;
		    ++olen;
		    if ( drawit>=tf_stopat ) cw = TextWidth1((lcstate>0)?sc:fd,opt-1,1)+letter_spacing;
		}
	    } else if ( enc!=em_unicode && enc<em_max && !mods->has_charset ) {
		int highch = ch>>8;
		if ( highch>=table2->first && highch<=table2->last &&
			(plane2 = table2->table[highch-table2->first])!=NULL &&
			(ch=plane2[ch&0xff])!=0 ) {
		    /* Mac, PC do we need to convert to shift jis here? */
		    if ( enc==em_jis212 )
			ch &= ~0x8000;
		    opt2->byte1 = ch>>8;
		    opt2->byte2 = ch&0xff;
		    ++opt2;
		    ++olen;
		    if ( drawit>=tf_stopat ) cw = TextWidth2((lcstate>0)?sc:fd,opt2-1,1)+letter_spacing;
		}
	    } else {
		opt2->byte1 = ch>>8;
		opt2->byte2 = ch&0xff;
		++opt2;
		++olen;
		if ( drawit>=tf_stopat ) cw = TextWidth2((lcstate>0)?sc:fd,opt2-1,1)+letter_spacing;
	    }

	    if ( drawit>=tf_stopat ) {
		if ( arg->width+cw >= arg->maxwidth ) {
		    unichar_t *stop = text;
		    if (( drawit==tf_stopat && arg->width+cw/2>arg->maxwidth) ||
			    drawit==tf_stopafter ) {
			x += cw;
			--stop;
		    }
		    arg->last = stop; arg->width += cw;
		    if ( enc<em_first2byte )
return( TextWidth1((lcstate>0)?sc:fd,transbuf,olen)+(olen-1)*letter_spacing );
		    else
return( TextWidth2((lcstate>0)?sc:fd,transbuf2,olen)+(olen-1)*letter_spacing );
		} else
		    arg->width += cw;
	    }
	}

	tempfd = (lcstate>0)?sc:fd;
	if ( enc<em_first2byte ) {
	    if ( drawit==tf_drawit )
		(disp->funcs->drawText1)(gw,tempfd,x+dist,y,transbuf,olen,mods,col);
	    else if ( drawit==tf_rect )
		RealAsDs(tempfd,transbuf,olen,arg);
	    dist += TextWidth1(tempfd,transbuf,olen) + olen*letter_spacing;
	} else {
	    if ( drawit==tf_drawit )
		(disp->funcs->drawText2)(gw,tempfd,x+dist,y,transbuf2,olen,mods,col);
	    else if ( drawit==tf_rect )
		RealAsDs16(tempfd,transbuf2,olen,arg);
	    dist += TextWidth2(tempfd,transbuf2,olen) + olen*letter_spacing;
	}
    }
    mods->starts_word = starts_word;
#undef TB_MAX
#undef transbuf
#undef transbuf2
return( dist-letter_spacing );
}

static FontMods dummyfontmods = { 0 };
static int32 _GDraw_ScreenDrawToImage(GWindow gw, struct font_data *fd, struct font_data *sc,
	int enc, int32 x, int32 y,
	unichar_t *text, unichar_t *end, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *oldarg) {
    struct tf_arg arg;
    int width;
    GImage *gi;

    memset(&arg,'\0',sizeof(arg));
    arg.first = true;

    if ( drawit==tf_drawit ) {
	GWindow pixmap;
	width = _GDraw_Transform(gw,fd->screen_font,NULL,enc,
		x,y,text,end,&dummyfontmods,col,
		tf_rect,&arg);
	arg.size.rbearing += width;	/* the rbearing currently contains the difference between the last char's width & rbearing */
	if ( arg.size.rbearing!=0 && arg.size.as+arg.size.ds!=0 ) {
	    /* A bunch of spaces might have a zero rbearing */
	    pixmap = GDrawCreateBitmap(screen_display,
		    arg.size.rbearing-arg.size.lbearing,arg.size.as+arg.size.ds,NULL);
	    GDrawFillRect(pixmap,NULL,1);
	    width = _GDraw_Transform(pixmap,fd->screen_font,NULL,enc,
		    -arg.size.lbearing,arg.size.as,
		    text,end,&dummyfontmods,0,tf_drawit,NULL);
	    gi = GDrawCopyScreenToImage(pixmap,NULL);
	    GDrawDestroyWindow(pixmap);
	    gi->u.image->trans = 1;
	    if ( col!=COLOR_CREATE(0,0,0) ) {
		GClut *clut;
		gi->u.image->clut = clut = gcalloc(1,sizeof(GClut));
		clut->clut_len = 2;
		clut->clut[0] = col;
		clut->clut[1] = COLOR_CREATE(0xff,0xff,0xff);
	    }
	    GDrawDrawImageMagnified(gw,gi,NULL,
		    x+(arg.size.lbearing*fd->scale_metrics_by+36000)/72000,
		    y-(arg.size.as*fd->scale_metrics_by+36000)/72000,
		    ((arg.size.rbearing-arg.size.lbearing)*fd->scale_metrics_by+36000)/72000,
		    ((arg.size.as+arg.size.ds)*fd->scale_metrics_by+36000)/72000);
	}
    } else if ( drawit == tf_width ) {
	width = _GDraw_Transform(screen_display->groot,fd->screen_font,NULL,enc,
		0,0,
		text,end,&dummyfontmods,col,tf_width,NULL);
    } else if ( drawit == tf_rect ) {
	width = _GDraw_Transform(screen_display->groot,fd->screen_font,NULL,enc,
		0,0,text,end,&dummyfontmods,col,
		tf_rect,&arg);
	if ( oldarg->first ) {
	    oldarg->first = arg.first;
	    oldarg->size.lbearing = (arg.size.lbearing*fd->scale_metrics_by+36000)/72000;
	}
	arg.size.as = (arg.size.as*fd->scale_metrics_by+36000)/72000;
	arg.size.ds = (arg.size.ds*fd->scale_metrics_by+36000)/72000;
	if ( arg.size.as>oldarg->size.as )
	    oldarg->size.as = arg.size.as;
	if ( arg.size.ds>oldarg->size.ds )
	    oldarg->size.ds = arg.size.ds;
	oldarg->size.rbearing = (arg.size.rbearing*fd->scale_metrics_by+36000)/72000;
    } else {
	arg.first = oldarg->first;
	arg.width = (oldarg->width*72000)/fd->scale_metrics_by;
	arg.maxwidth = (oldarg->maxwidth*72000)/fd->scale_metrics_by;
	width = _GDraw_Transform(screen_display->groot,fd->screen_font,NULL,enc,
		0,0,text,end,&dummyfontmods,col,
		drawit,&arg);
	oldarg->last = arg.last;
	oldarg->width = (arg.width*fd->scale_metrics_by)/72000;
    }
    width = (width*fd->scale_metrics_by+36000)/72000;
return( width );
}

struct bounds {
    int xmin, xmax;
    int ymin, ymax;
};

static int GetCharBB(struct font_data *fd,struct font_data *sc, int ach,
	struct bounds *bb) {
    XCharStruct *cs;

    if ( ach==-1 ) {
	memset(bb,'\0',sizeof(struct bounds));
return( false );
    }

    if ( sc!=NULL && islower(ach)) {
	fd = sc;
	ach = toupper(ach);
    }
    if ( fd->info->per_char==NULL ) {
	/* all characters have the same bounds */
	cs = &fd->info->min_bounds;
    } else if ( fd->map<em_first2byte ) {
	if ( ach>fd->info->max_char_or_byte2 || ach<fd->info->min_char_or_byte2 )
	    cs = NULL;
	else
	    cs = &fd->info->per_char[ach-fd->info->min_char_or_byte2];
    } else {
	int highch=ach>>8, lowch = ach&0xff;
	if ( highch>fd->info->max_byte1 || highch<fd->info->min_byte1 ||
		lowch>fd->info->max_char_or_byte2 ||
		lowch<fd->info->min_char_or_byte2 )
	    cs = NULL;
	else
	    cs = &fd->info->per_char[(highch-fd->info->min_byte1)*
		    (fd->info->max_char_or_byte2-fd->info->min_char_or_byte2+1)+
		    (lowch-fd->info->min_char_or_byte2)];
    }
    if ( cs==NULL ) {
	memset(bb,'\0',sizeof(struct bounds));
return( false );
    }
	
    bb->xmin = cs->lbearing;
    bb->xmax = cs->rbearing;
    bb->ymin = -cs->descent;
    bb->ymax = cs->ascent;
return( true );
}

static int ComposingXOffset(unichar_t ch,struct bounds *bb,struct bounds *abb,int spacing) {
    int xoff = 0;

    if ( ____utype2[1+ch]&____LEFT )
	xoff = bb->xmin - spacing - abb->xmax;
    else if ( ____utype2[1+ch]&____RIGHT ) {
	xoff = bb->xmax - abb->xmin;
	if ( !( ____utype2[1+ch]&____TOUCHING) )
	    xoff += spacing;
    } else if ( ____utype2[1+ch]&____CENTERLEFT )
	xoff = bb->xmin + (bb->xmax-bb->xmin)/2 - abb->xmax;
    else if ( ____utype2[1+ch]&____LEFTEDGE )
	xoff = bb->xmin - abb->xmin;
    else if ( ____utype2[1+ch]&____CENTERRIGHT )
	xoff = bb->xmin + (bb->xmax-bb->xmin)/2 - abb->xmin;
    else if ( ____utype2[1+ch]&____RIGHTEDGE )
	xoff = bb->xmax - abb->xmax;
    else
	xoff = bb->xmin - abb->xmin + ((bb->xmax-bb->xmin)-(abb->xmax-abb->xmin))/2;
return( xoff );
}

static int ComposingYOffset(unichar_t ch,struct bounds *bb,struct bounds *abb,int spacing) {
    int yoff = 0;

    if ( (____utype2[1+ch]&____ABOVE) && (____utype2[1+ch]&(____LEFT|____RIGHT)) )
	yoff = bb->ymax - abb->ymax;
    else if ( ____utype2[1+ch]&____ABOVE )
	yoff = bb->ymax + spacing - abb->ymin;
    else if ( ____utype2[1+ch]&____BELOW ) {
	yoff = bb->ymin - abb->ymax;
	if ( !( ____utype2[1+ch]&____TOUCHING) )
	    yoff -= spacing;
    } else if ( ____utype2[1+ch]&____OVERSTRIKE )
	yoff = bb->ymin - abb->ymin + ((bb->ymax-bb->ymin)-(abb->ymax-abb->ymin))/2;
    else /* If neither Above, Below, nor overstrike then should use the same baseline */
	yoff = bb->ymin - abb->ymin;

return( yoff );
}

static int32 ComposeCharacter(GWindow gw, struct font_instance *fi,
	struct font_data *fd, struct font_data *sc,
	int enc, int32 x, int32 y,
	unichar_t *text, unichar_t *end, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    int dist;
    int oldstart = mods->starts_word;
    unichar_t accent, *pt; int ach;
    struct bounds bb, abb, resbb;
    int spacing;
    struct font_data *afd;
    int xoff, yoff;
    FontMods dummy;

    /* for lower case "i" and "j" we can't actually use the character reported*/
    /*  by the normative decomposition as the dot gets in the way. We must use*/
    /*  the dotless version instead (if we can find one, that is) */
    afd = fd;
    pt = text;
    if ( (*text=='i' || *text=='j') &&
	    sc==NULL && !(mods->mods&tm_upper) ) {
	unichar_t ch = (*text=='i') ? 0x131 : 0xf6be;
	afd = PickAccentFont(fi,fd,ch, &accent);
	if ( afd!=NULL && afd->info==NULL )
	    _loadFontMetrics(gw->display,afd,fi);
	if ( afd==NULL || afd->info==NULL )
	    afd = fd;
	else
	    pt = &accent;
    }
    
    /* first thing to do is to draw the base character itself */
    /* (this won't work for combiners which need extra space to the left */
    /*  but my algorithem only works for stuff which doesn't alter the width */
    /*  so we ignore that issue) */
    if ( fd->screen_font!=NULL )
	dist = _GDraw_ScreenDrawToImage(gw,afd,sc,enc,x,y,pt,pt+1,mods,col,drawit,arg);
    else
	dist = _GDraw_Transform(gw,afd,sc,enc,x,y,pt,pt+1,mods,col,drawit,arg);


    /* !!!!! I need to do something about combiners that actually add to the */
    /* !!!!!  character's width, but I'm not sure what's a good soln. yet */
    /* Until that happens, these guys are done */
    if ( drawit==tf_width || drawit>tf_stopat )
return( dist );

    mods->starts_word = oldstart;
    GetCharBB(fd,sc,EncodingPosInMapping(fd->map,*pt,mods),&bb);
    resbb = bb;

    if ( fi->rq.point_size<0 )
	spacing = -fi->rq.point_size;
    else
	spacing = GDrawPointsToPixels(gw,fi->rq.point_size);
    spacing = (spacing+10)/20;
    if ( spacing==0 ) spacing = 1;	/* this big a gap between char and accent */
    memset(&dummy,'\0',sizeof(dummy));

    for ( ++text ; text<end && *text<0x10000 && iscombining(*text); ++text ) {
	afd = PickAccentFont(fi,fd,*text, &accent);
	if ( afd==NULL )
	    /* Oh well, we couldn't find the accent, just skip it */;
	else if ( ( ach = EncodingPosInMapping(afd->map,accent,NULL))!=-1 &&
		GetCharBB(afd,NULL,ach,&abb)) {
	    xoff = ComposingXOffset(*text,&bb,&abb,spacing);
	    yoff = ComposingYOffset(*text,&resbb,&abb,spacing);
	    if ( drawit!=tf_drawit )
		/* Do Nothing */;
	    else if ( afd->screen_font!=NULL )
		_GDraw_ScreenDrawToImage(gw,afd,NULL,afd->map,x+xoff,y-yoff,
			&accent,&accent+1,&dummy,col,drawit,arg);
	    else
		_GDraw_Transform(gw,afd,NULL,afd->map,x+xoff,y-yoff,
			&accent,&accent+1,&dummy,col,drawit,arg);
	    if ( abb.xmin+xoff<resbb.xmin ) resbb.xmin = abb.xmin+xoff;
	    if ( abb.xmax+xoff>resbb.xmax ) resbb.xmax = abb.xmax+xoff;
	    if ( abb.ymin+yoff<resbb.ymin ) resbb.ymin = abb.ymin+yoff;
	    if ( abb.ymax+yoff>resbb.ymax ) resbb.ymax = abb.ymax+yoff;
	}
    }
    if ( drawit==tf_rect ) {
	if ( arg->size.as< resbb.ymax ) arg->size.as = resbb.ymax;
	if ( arg->size.ds< -resbb.ymin) arg->size.ds = -resbb.ymin;
    }
return( dist );
}

static int32 _GDraw_DoText(GWindow gw, int32 x, int32 y,
	unichar_t *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    unichar_t *end = text+(cnt<0?u_strlen(text):cnt);
    unichar_t *next, *comb;
    int32 dist = 0;
    struct font_data *fd, *sc;
    struct font_instance *fi = gw->ggc->fi;
    GDisplay *disp = gw->display;
    int enc;
    int ulevel;

    if ( fi==NULL )
return( 0 );

    if ( mods==NULL ) mods = &dummyfontmods;

#ifndef _NO_LIBCAIRO
    if ( gw->usecairo )
return( _GXCDraw_DoText(gw,x,y,text,cnt,mods,col,drawit,arg));
#endif

    while ( text<end ) {
#ifndef UNICHAR_16
	if ( *text>=0x1f0000 ) {
	    /* Not a valid Unicode character */
	    ++text;
    continue;
	} else if ( *text&0x1f0000 ) {
	    int plane = (*text>>16);
	    unichar_t *start = text;
	    while ( text<end && (*text>>16)==plane )
		text++;
	    /* the "encoding" we want to use is "unicodeplane-plane" which is */
	    /* em_uplane+plane */
	    enc = em_uplane0 + plane;
	    fd = fi->fonts[enc];

	    if ( fd!=NULL && fd->info==NULL )
		_loadFontMetrics(disp,fd,fi);
	    if ( fd!=NULL ) {
		dist += _GDraw_Transform(gw,fd,NULL,enc,x+dist,y,start,text,mods,col,drawit,arg);
		if ( drawit==tf_rect ) {
		    arg->size.rbearing += dist;
		    arg->size.width = dist;
		}
	    } else {
		static unichar_t notfound[]= { 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0 };
		int i, len;
		for ( i=0; i<text-start; i += len ) {
		    len = text-(start+i);
		    if ( len>4 ) len = 4;
		    dist += _GDraw_DoText(gw, x+dist, y, notfound, len, mods, col, drawit, arg);
		    if ( arg->last>= notfound && arg->last<=notfound+4 )
			arg->last = start+i+(arg->last-notfound);
		    if ( drawit>=tf_stopat && arg->width>=arg->maxwidth )
return( dist );
		}
	    }
	    if ( drawit>=tf_stopat && arg->width>=arg->maxwidth )
return( dist );
    continue;
	}
#endif
	if ( mods->has_charset ) {
	    enc = mods->charset;
	    next = end;
	} else
	    enc = GDrawFindEncoding(text,end-text,fi,&next,&ulevel);

	fd = sc = NULL;
	if ( enc==em_unicode ) {
	    fd = fi->unifonts[ulevel];
	    if ( fd==fi->fonts[enc] && fi->smallcaps!=NULL ) sc = fi->smallcaps[enc];
	} else if ( enc>=0 && enc<em_max ) {
	    fd = fi->fonts[enc];
	    if ( fi->smallcaps!=NULL ) sc = fi->smallcaps[enc];
	} else if ( enc>=em_max ) {
	    fd = fi->unifonts[enc-em_max];
	    sc = NULL;
	    enc = em_unicode;
	}
	if ( fd!=NULL && fd->info==NULL )
	    _loadFontMetrics(disp,fd,fi);
	if ( sc!=NULL && sc->info==NULL ) {
	    _loadFontMetrics(disp,sc,fi);
	    if ( sc->info==NULL ) sc=NULL;
	}

#if GREEK_BUG
	if ( *text>=0xa0 && *text<=0xff ) {
	    printf( "Char 0x%x enc=%d ", *text, enc );
	    if ( fd!=NULL )
		printf( "Picked=%s\n", fd->localname );
	    else
		printf( "No font\n" );
	}
#endif

	while ( text<next ) {
	    /* if the string begins with a combining char, then just print it */
	    /*  otherwise stop one character before the first combiner */
	    /*  (as we'll join it with the combiner later) */
	    if ( mods->has_charset ) {
		if ( fd!=NULL && fd->info!=NULL )
		    dist += _GDraw_Transform(gw,fd,sc,enc,x+dist,y,text,next,mods,col,drawit,arg);
		text = comb = next;
	    } else {
		for ( comb=text+1; comb<next && *comb<0x10000 && !iscombining(*comb); ++comb );
		if ( comb<next )
		    --comb;
		if ( comb>text ) {
		    if ( fd==NULL || fd->info==NULL ) 
			dist += _GDraw_DrawUnencoded(gw,fi,x+dist,y,text,comb,mods,col,drawit,arg);
		    else if ( fd->screen_font!=NULL )
			dist += _GDraw_ScreenDrawToImage(gw,fd,sc,enc,x+dist,y,text,comb,mods,col,drawit,arg);
		    else
			dist += _GDraw_Transform(gw,fd,sc,enc,x+dist,y,text,comb,mods,col,drawit,arg);
		    text = comb;
		}
	    }
	    if ( drawit>=tf_stopat && arg->width>=arg->maxwidth )
return( dist );
	    if ( comb<next ) {
		if ( fd==NULL || fd->info==NULL )
		    /* if we have no encoding for the base character, then don't bother */
		    /*  to print any of the accents, just do one unencoded thing for the */
		    /*  base char, and ignore the others */
		    dist += _GDraw_DrawUnencoded(gw,fi,x+dist,y,text,comb,mods,col,drawit,arg);
		else
		    dist += ComposeCharacter(gw,fi,fd,sc,enc,x+dist,y,text,next,mods,col,drawit,arg);
		for ( text=comb+1; text<next && *text<0x10000 && iscombining(*text); ++text );
		if ( drawit>=tf_stopat && arg->width>=arg->maxwidth )
return( dist );
	    }
	}
    }
    if ( drawit==tf_rect ) {
	arg->size.rbearing += dist;
	arg->size.width = dist;
    }
return( dist );
}

int32 GDrawDrawText(GWindow gw, int32 x, int32 y,
	const unichar_t *text, int32 cnt, FontMods *mods, Color col) {
    struct tf_arg arg;
    memset(&arg,'\0',sizeof(arg));
return( _GDraw_DoText(gw,x,y,(unichar_t *) text,cnt,mods,col,tf_drawit,&arg));
}

int32 GDrawGetTextWidth(GWindow gw,const unichar_t *text, int32 cnt, FontMods *mods) {
    struct tf_arg arg;
    memset(&arg,'\0',sizeof(arg));
return( _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_width,&arg));
}

int32 GDrawGetTextBounds(GWindow gw,const unichar_t *text, int32 cnt, FontMods *mods,
	GTextBounds *size) {
    struct tf_arg arg;
    int width;
    memset(&arg,'\0',sizeof(arg));
    arg.first = true;
    width = _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_rect,&arg);
    *size = arg.size;
return( width );
}

int32 GDrawGetTextPtFromPos(GWindow gw,unichar_t *text, int32 cnt, FontMods *mods,
	int32 maxwidth, unichar_t **end) {
    struct tf_arg arg;
    int width;
    memset(&arg,'\0',sizeof(arg));
    arg.maxwidth = maxwidth;
    width = _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_stopat,&arg);
    if ( arg.last==NULL )
	arg.last = text + (cnt==-1?u_strlen(text):cnt);
    *end = arg.last;
return( width );
}

int32 GDrawGetTextPtBeforePos(GWindow gw,unichar_t *text, int32 cnt, FontMods *mods,
	int32 maxwidth, unichar_t **end) {
    struct tf_arg arg;
    int width;
    memset(&arg,'\0',sizeof(arg));
    arg.maxwidth = maxwidth;
    width = _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_stopbefore,&arg);
    if ( arg.last==NULL )
	arg.last = text + (cnt==-1?u_strlen(text):cnt);
    *end = arg.last;
return( width );
}

int32 GDrawGetTextPtAfterPos(GWindow gw,unichar_t *text, int32 cnt, FontMods *mods,
	int32 maxwidth, unichar_t **end) {
    struct tf_arg arg;
    int width;
    memset(&arg,'\0',sizeof(arg));
    arg.maxwidth = maxwidth;
    width = _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_stopafter,&arg);
    if ( arg.last==NULL )
	arg.last = text + (cnt==-1?u_strlen(text):cnt);
    *end = arg.last;
return( width );
}

/* UTF8 routines */

static int32 _GDraw_DoText8(GWindow gw, int32 x, int32 y,
	const char *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    const char *end = text+(cnt<0?strlen(text):cnt);
    int32 dist = 0;
    const char *start;
#ifdef UNICHAR_16
    const char *last;
    struct font_data *fd;
    GDisplay *disp = gw->display;
    int enc;
#endif
    int i;
    struct font_instance *fi = gw->ggc->fi;
    unichar_t ubuffer[200], *upt;
    uint32 val;

    if ( fi==NULL )
return( 0 );

    if ( mods==NULL ) mods = &dummyfontmods;

#ifndef _NO_LIBCAIRO
    if ( gw->usecairo )
return( _GXCDraw_DoText8(gw,x,y,text,cnt,mods,col,drawit,arg));
#endif

#ifdef UNICHAR_16
    forever {
	if ( text>=end )
    break;
	start = text;
	last = text;
	val = utf8_ildb(&text);
	if ( val<=0xffff ) {
	    upt = ubuffer;
	    while ( val<=0xffff && text<=end &&
		    upt<ubuffer+sizeof(ubuffer)/sizeof(ubuffer[0])) {
		*upt++ = val;
		last = text;
		val = utf8_ildb(&text);
	    }
	    text = last;
	    dist += _GDraw_DoText(gw,x+dist,y,ubuffer,upt-ubuffer,mods,col,drawit,arg);
	} else if ( val!=(uint32) -1 ) {
	    int plane = (val>>16);
	    upt = ubuffer;
	    while ( (val>>16)==plane && text<=end &&
		    upt<ubuffer+sizeof(ubuffer)/sizeof(ubuffer[0])) {
		*upt++ = val&0xffff;
		last = text;
		val = utf8_ildb(&text);
	    }
	    text = last;
	    /* the "encoding" we want to use is "unicodeplane-plane" which is */
	    /* em_uplane+plane */
	    enc = em_uplane0 + plane;
	    fd = fi->fonts[enc];

	    if ( fd!=NULL && fd->info==NULL )
		_loadFontMetrics(disp,fd,fi);
	    if ( fd!=NULL )
		dist += _GDraw_Transform(gw,fd,NULL,enc,x+dist,y,ubuffer,upt,mods,col,drawit,arg);
	    if ( drawit==tf_rect ) {
		arg->size.rbearing += dist;
		arg->size.width = dist;
	    }
	}
	if ( drawit>=tf_stopat && arg->width>=arg->maxwidth ) {
	    if ( arg->last!=upt ) {
		text = start;
		for ( i = arg->last-ubuffer; i>0 ; --i )
		    utf8_ildb(&text);
	    }
	    arg->utf8_last = (char *) text;
return( dist );
	}
    }
#else
    forever {
	if ( text>=end )
    break;
	start = text;
	upt = ubuffer;
	while ( text<end &&
		upt<ubuffer+sizeof(ubuffer)/sizeof(ubuffer[0])) {
	    val = utf8_ildb(&text);
	    if ( val==-1 )
	break;
	    *upt++ = val;
	}
	dist += _GDraw_DoText(gw,x+dist,y,ubuffer,upt-ubuffer,mods,col,drawit,arg);
	if ( drawit>=tf_stopat && arg->width>=arg->maxwidth ) {
	    if ( arg->last!=upt ) {
		text = start;
		for ( i = arg->last-ubuffer; i>0 ; --i )
		    utf8_ildb(&text);
	    }
	    arg->utf8_last = (char *) text;
return( dist );
	}
    }
#endif
return( dist );
}

int32 GDrawDrawText8(GWindow gw, int32 x, int32 y,
	const char *text, int32 cnt, FontMods *mods, Color col) {
    struct tf_arg arg;
    memset(&arg,'\0',sizeof(arg));
return( _GDraw_DoText8(gw,x,y,text,cnt,mods,col,tf_drawit,&arg));
}

int32 GDrawGetText8Width(GWindow gw,const char *text, int32 cnt, FontMods *mods) {
    struct tf_arg arg;
    memset(&arg,'\0',sizeof(arg));
return( _GDraw_DoText8(gw,0,0, text,cnt,mods,0,tf_width,&arg));
}

int32 GDrawGetText8Bounds(GWindow gw,char *text, int32 cnt, FontMods *mods,
	GTextBounds *size) {
    struct tf_arg arg;
    int width;
    memset(&arg,'\0',sizeof(arg));
    arg.first = true;
    width = _GDraw_DoText8(gw,0,0,text,cnt,mods,0,tf_rect,&arg);
    *size = arg.size;
return( width );
}

int32 GDrawGetText8PtFromPos(GWindow gw,char *text, int32 cnt, FontMods *mods,
	int32 maxwidth, char **end) {
    struct tf_arg arg;
    int width;
    memset(&arg,'\0',sizeof(arg));
    arg.maxwidth = maxwidth;
    width = _GDraw_DoText8(gw,0,0,text,cnt,mods,0,tf_stopat,&arg);
    if ( arg.utf8_last==NULL )
	arg.utf8_last = text + (cnt==-1?strlen(text):cnt);
    *end = arg.utf8_last;
return( width );
}

int32 GDrawGetText8PtBeforePos(GWindow gw,char *text, int32 cnt, FontMods *mods,
	int32 maxwidth, char **end) {
    struct tf_arg arg;
    int width;
    memset(&arg,'\0',sizeof(arg));
    arg.maxwidth = maxwidth;
    width = _GDraw_DoText8(gw,0,0,text,cnt,mods,0,tf_stopbefore,&arg);
    if ( arg.utf8_last==NULL )
	arg.utf8_last = text + (cnt==-1?strlen(text):cnt);
    *end = arg.utf8_last;
return( width );
}

int32 GDrawGetText8PtAfterPos(GWindow gw,char *text, int32 cnt, FontMods *mods,
	int32 maxwidth, char **end) {
    struct tf_arg arg;
    int width;
    memset(&arg,'\0',sizeof(arg));
    arg.maxwidth = maxwidth;
    width = _GDraw_DoText8(gw,0,0,text,cnt,mods,0,tf_stopafter,&arg);
    if ( arg.utf8_last==NULL )
	arg.utf8_last = text + (cnt==-1?strlen(text):cnt);
    *end = arg.utf8_last;
return( width );
}
/* End UTF8 routines */

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

/************************** Arabic Form Translation ***************************/
/* We must decide which form (initial, medial, final, isolated) to use for    */
/*  each arabic letter */

void GDrawArabicForms(GBiText *bd, int32 start, int32 end) {
    unichar_t *pt, *last = bd->text+end, *rpt, *alef_pt=NULL;
    int letter_left = false, was_alef=false;

    /* only pay attention to letters within 0x600 and 0x6ff. Ignore 0xfb00 */
    /*  these higher letters have already been assigned forms by the user */
    /*  don't mess with 'em */
    for ( pt=bd->text+start; pt<last; ++pt ) {
	int ch = *pt;
	if ( ch>=0x600 && ch<=0x6ff && ArabicForms[ch-0x600].isletter ) {
	    if ( !ArabicForms[ch-0x600].joindual ) letter_left = false;
	    for ( rpt = pt+1; rpt<last && *rpt<0x10000 && (iscombining(*rpt) || *rpt==0x200d); ++rpt );
	    if ( rpt==last || *rpt<0x600 || *rpt>0x6ff ||
		    !ArabicForms[*rpt-0x600].isletter ) {
		if ( letter_left )	/* letter left but none right */
		    *pt = ArabicForms[ch-0x600].initial;
		else			/* neither left nor right */
		    *pt = ArabicForms[ch-0x600].isolated;
	    } else {
		if ( letter_left )	/* both left and right */
		    *pt = ArabicForms[ch-0x600].medial;
		else			/* letter right but not left */
		    *pt = ArabicForms[ch-0x600].final;
	    }
	    if ( was_alef && ch==0x644 ) {
		/* Required ligature: Lam and Alef */
		if ( *pt==0xFEDF /* LAM Initial */ )
		    *alef_pt = 0xfefb;	/* Isolated */
		else		 /* LAM Medial FEE0 */
		    *alef_pt = 0xfefc;	/* Final */
#if 0
		/* We combined two letters into a ligature. */
		/*  Move everything after this up one character */
		/* NOPE! Maintain corespondance between orig and final lines */
		--end; --last;
		for ( i = pt-bd->text; i<end; ++i ) {
		    bd->text[i] = bd->text[i+1];
		    bd->original[i] = bd->original[i+1];
		    /* I don't care about anything else now */
		}
		bd->text[i] = 0x200d;	/* put in a zero width joiner to use up the extra character */
		--pt;
#else
		*pt = 0x200b;
#endif
	    }
	    if (( was_alef = ch==0x627 /*|| ch==0xFE8E*/ ))
		alef_pt = pt;
	    letter_left = true;
	} else if ( ch<0x10000 && !iscombining(ch) && ch!=0x200d )
	    was_alef = letter_left = false;
    }
}

/*********************** Bidirectional Text Algorithems ***********************/

/* returns
	-1 if the base direction is right2left
	0  if the base direction is left2right but there are r2l things
	1  if it's all l2r
*/
int32 GDrawIsAllLeftToRight(const unichar_t *text, int32 cnt) {
    const unichar_t *end;
    if ( cnt==-1 ) cnt = u_strlen(text);
    end = text+cnt;
    while ( text<end ) {
	if ( *text<0x10000 && isrighttoleft(*text))
return( -1 );
	if ( *text<0x10000 && islefttoright(*text))
    break;
	++text;
    }
    while ( text<end ) {
	if ( *text<0x10000 && isrighttoleft(*text))
return( 0 );
	++text;
    }
return( 1 );
}

int32 GDrawIsAllLeftToRight8(const char *text, int32 cnt) {
    const char *end;
    unichar_t ch;

    if ( cnt==-1 ) cnt = strlen(text);
    end = text+cnt;
    while ( text<end ) {
	ch = utf8_ildb( (const char **) &text);
	if ( ch<0x10000 && isrighttoleft(ch))
return( -1 );
	if ( ch<0x10000 && islefttoright(ch))
    break;
    }
    while ( text<end ) {
	ch = utf8_ildb( (const char **) &text);
	if ( ch<0x10000 && isrighttoleft(ch))
return( 0 );
    }
return( 1 );
}

#define MAXBI	200

/* Figure out the proper text ordering */
/* This routine must be called for each paragraph as a whole */
void GDrawBiText1(GBiText *bd, const unichar_t *text, int32 cnt) {
    int level=0, override=0;
    int levels[16], overrides[16];
    int stat=0;
    const unichar_t *pt=text, *end = text+cnt; unichar_t ch;
    int pos;

    /* handle overrides and embeddings */
    bd->interpret_arabic = false;
    pos = 0;
    if ( bd->base_right_to_left ) level = 1;
    while ( pt<end ) {
	ch = *pt;
	if ( ch == 0x202b || ch==0x202d || ch == 0x202a || ch == 0x202e ) {
		/*  RLE		   RLO		    LRE		     LRO */
		/* right to left (left to right) embedding (override) */
	    if ( stat<sizeof(levels)/sizeof(levels[0]) ) {
		levels[stat] = level; overrides[stat] = override;
		++stat;
	    }
	    if ( ch==0x202b || ch==0x202d )
		level = (level+1)|1;
	    else
		level = (level&~1)+2;
	    if ( ch==0x202b || ch == 0x202a )
		override = 0;
	    else if ( ch==0x202d )
		override = -1;
	    else
		override = 1;
	} else if ( ch == 0x202c ) {	 /* Pop directional format */
		    /*    PDF */
	    if ( stat>0 ) {
		--stat;
		level = levels[stat]; override = overrides[stat];
	    }
	}
	/* We could get rid of the format control codes here */
	/*  but if we do that then our string length no longer matches that */
	/*  of the original, which leads to various probs when we try to go */
	/*  back and forth */
	{
	    bd->text[pos] = ch;
	    bd->level[pos] = level;
	    bd->override[pos] = override;
	    bd->type[pos] = ____utype[ch+1];
	    bd->original[pos] = (unichar_t *) pt;
	    if ( ch>=0x621 && ch<=0x6ff )	/* The other arabic chars have already been interpreted, presumably user knows what he's doing */
		bd->interpret_arabic = true;
	    ++pos;
	}
	++pt;
    }
    bd->len = pos;
    bd->text[pos] = '\0';
    bd->original[pos] = (unichar_t *) (text+cnt);
}

/* This routine must be called for each line after GDrawBiText1 has been called */
/*  for the paragraph containing the lines. */
void _GDrawBiText2(GBiText *bd, int32 start, int32 end) {
    int level, maxlevel;
    int last, next, truelast;
    unichar_t ch;
    int pos, epos, i,j;

    if ( end==-1 || end>bd->len ) end = bd->len;

    /* Handle numbers */
    last = bd->base_right_to_left ? -1 : 1;
    for ( pos = start; pos<end; ++pos ) {
	if ( bd->override[pos] || (bd->type[pos] & (____L2R|____R2L)) ) {
	    if ( bd->override[pos] )
		last = bd->override[pos];
	    else if ( bd->type[pos]&____L2R )
		last = 1;
	    else
		last = -1;
	} else if ( bd->type[pos]&____ENUM && (last==-1 || (pos!=0 && (bd->type[pos-1]&____ANUM))))
	    bd->type[pos] = ____ANUM;
    }
    last = 0;
    for ( pos = start; pos<end; ++pos ) {
	if ( bd->override[pos] ) {
	    /* Ignore it */;
	} else if ( bd->type[pos]&____ANUM )
	    last = -1;
	else if ( bd->type[pos]&____ENUM )
	    last = 1;
	else if ( (bd->type[pos]&(____ENS|____CS)) && last==1 && pos<end-1 &&
		(bd->type[pos+1]&____ENUM) )
	    bd->type[pos] = ____ENUM;
	else if ( (bd->type[pos]&____CS) && last==-1 && pos<end-1 &&
		(bd->type[pos+1]&____ANUM) )
	    bd->type[pos] = ____ANUM;
	else if ( (bd->type[pos]&____ENT) &&
		(last==1 || (pos<end-1 && (bd->type[pos+1]&____ENUM) )))
	    bd->type[pos] = ____ENUM;
	else if ( bd->type[pos]&(____ENT|____ENS|____CS) )
	    bd->type[pos] = 0;
    }

    /* Handle neutrals */
    truelast = last = bd->base_right_to_left ? -1 : 1;
    for ( pos = start; pos<end; ++pos ) {
	if ( bd->override[pos] || (bd->type[pos] & (____L2R|____R2L)) ) {
	    if ( bd->override[pos] )
		truelast = last = bd->override[pos];
	    else if ( bd->type[pos]&____L2R )
		truelast = last = 1;
	    else
		truelast = last = -1;
	} else if ( bd->type[pos] & ____ENUM ) {
	    last = truelast;
	} else if ( bd->type[pos] & ____ANUM ) {
	    last = -1;
	} else {
	    for ( epos=pos;
		    epos<end && bd->override[epos]==0 && !(bd->type[epos]&(____L2R|____R2L|____ANUM|____ENUM));
		    ++epos );
	    if ( epos==end )
		next = 0;
	    else if ( bd->override[epos+1] )
		next = bd->override[epos+1];
	    else if ( bd->type[epos]&(____L2R|____ENUM) )
		next = 1;
	    else
		next = -1;
	    if ( next==last )
		for ( ; pos<epos; ++pos )
		    bd->type[pos] = last==-1 ? ____R2L : ____L2R;
	    else
		for ( ; pos<epos; ++pos )
		    bd->type[pos] = (bd->level[pos]&1) ? ____R2L : ____L2R;
	    --pos;
	}
    }

    /* Handle implicit levels */
    maxlevel = 0;
    for ( pos = start; pos<end; ++pos ) {
	if ( bd->override[pos] )
	    /* Ok the level is set by the override */;
	else if ( (bd->type[pos]&____R2L) && !(bd->level[pos]&1))
	    ++bd->level[pos];
	else if ( (bd->type[pos]&(____L2R|____ENUM|____ANUM)) && (bd->level[pos]&1))
	    ++bd->level[pos];
	if ( bd->level[pos]>maxlevel ) maxlevel = bd->level[pos];
    }

    /* do mirrors */
    for ( pos = start; pos<end; ++pos ) {
	if ( (bd->level[pos]&1) && (ch = tomirror(bd->text[pos]))!=0 )
	    bd->text[pos] = ch;
    }

    /* Reverse text */
    for ( level = maxlevel; level>0; --level ) {
	for ( pos=start; pos<end; ++pos ) if ( bd->level[pos]>=level ) {
	    for ( epos = pos; epos<end && bd->level[epos]>=level ; ++epos );
	    for ( i=pos,j=epos-1; i<j; ++i, --j ) {
		unichar_t temp = bd->text[i], *tpt = bd->original[i];
		bd->text[i] = bd->text[j];
		bd->text[j] = temp;
		bd->original[i] = bd->original[j];
		bd->original[j] = tpt;
	    }
	    pos = epos;
	}
    }
}


/* This routine must be called for each line after GDrawBiText1 has been called */
/*  for the paragraph containing the lines. */
void GDrawBiText2(GBiText *bd, int32 start, int32 end) {
    int pos, epos, i,j;

    if ( end==-1 || end>bd->len ) end = bd->len;

    _GDrawBiText2(bd,start,end);

    /* do combiners */
    /* combiners must always follow (in string order) the character they modify*/
    /*  but now combiners in r2l text will precede it */
    for ( pos = start; pos<end; ++pos ) {
	if ( bd->text[pos]<0x10000 && iscombining(bd->text[pos]) && (bd->level[pos]&1) /*&& pos!=start*/ ) {
	    for ( epos = pos; epos<end && bd->text[epos]<0x10000 && iscombining(bd->text[epos]) ; ++epos );
	    if ( epos<end ) {
		for ( i=pos,j=epos; i<j; ++i, --j ) {
		    unichar_t temp = bd->text[i], *tpt = bd->original[i];
		    bd->text[i] = bd->text[j];
		    bd->text[j] = temp;
		    bd->original[i] = bd->original[j];
		    bd->original[j] = tpt;
		}
	    }
	    pos = epos;
	}
    }

    if ( bd->interpret_arabic )
	GDrawArabicForms(bd,start,end);
}

static int32 _GDraw_DoBiText(GWindow gw, int32 x, int32 y,
	const unichar_t *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg, int start) {
    GBiText bd;
    unichar_t btext[MAXBI];
    uint8 level[MAXBI], override[MAXBI];
    uint16 type[MAXBI];
    unichar_t *orig[MAXBI];
    int32 width;

    if ( cnt==-1 ) cnt = u_strlen(text);
    if ( cnt<MAXBI-1 ) {
	bd.text = btext;
	bd.level = level;
	bd.override = override;
	bd.type = type;
	bd.original = orig;
    } else {
	++cnt;		/* for EOS */
	bd.text = malloc(cnt*sizeof(unichar_t));
	bd.level = malloc(cnt*sizeof(uint8));
	bd.override = malloc(cnt*sizeof(uint8));
	bd.type = malloc(cnt*sizeof(uint16));
	bd.original = malloc(cnt*sizeof(unichar_t *));
	--cnt;
    }
    bd.len = cnt;
    bd.base_right_to_left = start==-1 ? 1 : 0;
    GDrawBiText1(&bd, text, cnt);
    GDrawBiText2(&bd, 0, bd.len);

    width = _GDraw_DoText(gw,x,y,bd.text,bd.len,mods,col,drawit,arg);
    if ( arg!=NULL ) {
	if (arg->last==NULL )
	    arg->last = (unichar_t *) text + cnt;
	else
	    arg->last = bd.original[arg->last-bd.text];
    }
    if ( cnt>=MAXBI ) {
	free(bd.text); free(bd.level); free(bd.override); free(bd.type);
	free(bd.original);
    }
return( width );
}

static int32 _GDraw_DoBiWidth(GWindow gw, const unichar_t *text, int len, int32 cnt,
	FontMods *mods, enum text_funcs drawit, int start) {
    GBiText bd;
    unichar_t btext[MAXBI];
    uint8 level[MAXBI], override[MAXBI];
    uint16 type[MAXBI];
    unichar_t *orig[MAXBI];
    int32 width;
    int i;
    struct tf_arg arg;

    if ( cnt==-1 ) cnt = u_strlen(text);
    if ( cnt<MAXBI ) {
	bd.text = btext;
	bd.level = level;
	bd.override = override;
	bd.type = type;
	bd.original = orig;
    } else {
        ++cnt; /* for EOS */
	bd.text = malloc(cnt*sizeof(unichar_t));
	bd.level = malloc(cnt*sizeof(uint8));
	bd.override = malloc(cnt*sizeof(uint8));
	bd.type = malloc(cnt*sizeof(uint16));
	bd.original = malloc(cnt*sizeof(unichar_t *));
	--cnt;
    }
    bd.len = cnt;
    bd.base_right_to_left = start==-1 ? 1 : 0;
    GDrawBiText1(&bd, text, cnt);
    GDrawBiText2(&bd, 0, bd.len);
    for ( i=0; i<bd.len && bd.original[i]!=text+len; ++i );

    memset(&arg,'\0',sizeof(arg));
    width = _GDraw_DoText(gw,0,0,bd.text,i,mods,0,tf_width,&arg);
    if ( cnt>=MAXBI ) {
	free(bd.text); free(bd.level); free(bd.override); free(bd.type);
	free(bd.original);
    }
return( width );
}

int32 GDrawDrawBiText(GWindow gw, int32 x, int32 y,
	const unichar_t *text, int32 cnt, FontMods *mods, Color col) {
    int ret;
    struct tf_arg arg;

#ifndef _NO_LIBPANGO
    if ( gw->usepango )
return( _GXPDraw_DoText(gw,x,y,text,cnt,mods,col,tf_drawit,&arg));
    else
#endif
    if (( ret = GDrawIsAllLeftToRight(text,cnt))==1 ) {
	memset(&arg,'\0',sizeof(arg));
return( _GDraw_DoText(gw,x,y,(unichar_t *) text,cnt,mods,col,tf_drawit,&arg));
    }
return( _GDraw_DoBiText(gw,x,y,text,cnt,mods,col,tf_drawit,NULL,ret));
}

int32 GDrawGetBiTextWidth(GWindow gw,const unichar_t *text, int len, int32 cnt, FontMods *mods) {
    int ret;
    struct tf_arg arg;

#ifndef _NO_LIBPANGO
    if ( gw->usepango )
return( _GXPDraw_DoText(gw,0,0,text,cnt,mods,0x0,tf_width,&arg));
    else
#endif
    if ( len==-1 || len==cnt || ( ret = GDrawIsAllLeftToRight(text,cnt))==1 ) {
	memset(&arg,'\0',sizeof(arg));
return( _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_width,&arg));
    }
return( _GDraw_DoBiWidth(gw,text,len,cnt,mods,tf_width,ret));
}

int32 GDrawGetBiTextBounds(GWindow gw,const unichar_t *text, int32 cnt, FontMods *mods, GTextBounds *bounds) {
    int ret;
    struct tf_arg arg;

    memset(&arg,'\0',sizeof(arg));
    arg.first = true;
#ifndef _NO_LIBPANGO
    if ( gw->usepango )
	ret = _GXPDraw_DoText(gw,0,0,text,cnt,mods,0x0,tf_rect,&arg);
    else
#endif
	ret = _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_rect,&arg);
    *bounds = arg.size;
return( ret );
}

int32 GDrawGetBiTextPtFromPos(GWindow gw,const unichar_t *text, int32 cnt, FontMods *mods,
	int32 maxwidth, unichar_t **end) {
    struct tf_arg arg;
    int width, ret;
    memset(&arg,'\0',sizeof(arg));
    arg.maxwidth = maxwidth;
    if (( ret = GDrawIsAllLeftToRight(text,cnt))==1 )
	width = _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_stopat,&arg);
    else
	width = _GDraw_DoBiText(gw,0,0,text,cnt,mods,0,tf_stopat,&arg,ret);
    *end = arg.last;
return( width );
}

int32 GDrawGetBiTextPtBeforePos(GWindow gw,const unichar_t *text, int32 cnt, FontMods *mods,
	int32 maxwidth, unichar_t **end) {
    struct tf_arg arg;
    int width, ret;
    memset(&arg,'\0',sizeof(arg));
    arg.maxwidth = maxwidth;
    if (( ret = GDrawIsAllLeftToRight(text,cnt))==1 )
	width = _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_stopbefore,&arg);
    else
	width = _GDraw_DoBiText(gw,0,0,text,cnt,mods,0,tf_stopbefore,&arg,ret);
    *end = arg.last;
return( width );
}

int32 GDrawGetBiTextPtAfterPos(GWindow gw,const unichar_t *text, int32 cnt, FontMods *mods,
	int32 maxwidth, unichar_t **end) {
    struct tf_arg arg;
    int width, ret;
    memset(&arg,'\0',sizeof(arg));
    arg.maxwidth = maxwidth;
    if (( ret = GDrawIsAllLeftToRight(text,cnt))==1 )
	width = _GDraw_DoText(gw,0,0,(unichar_t *) text,cnt,mods,0,tf_stopafter,&arg);
    else
	width = _GDraw_DoBiText(gw,0,0,text,cnt,mods,0,tf_stopafter,&arg,ret);
    *end = arg.last;
return( width );
}

int32 GDrawDrawBiText8(GWindow gw, int32 x, int32 y,
	const char *text, int32 cnt, FontMods *mods, Color col) {
    int ret;
    struct tf_arg arg;

#ifndef _NO_LIBPANGO
    if ( gw->usepango )
return( _GXPDraw_DoText8(gw,x,y,text,cnt,mods,col,tf_drawit,&arg));
    else
#endif
    if (( ret = GDrawIsAllLeftToRight8(text,cnt))==1 ) {
	memset(&arg,'\0',sizeof(arg));
return( _GDraw_DoText8(gw,x,y,text,cnt,mods,col,tf_drawit,&arg));
    } else {
	unichar_t *temp = cnt==-1 ? utf82u_copy(text): utf82u_copyn(text,cnt);
	int width;
	width = _GDraw_DoBiText(gw,x,y,temp,-1,mods,col,tf_drawit,NULL,ret);
	free(temp);
return( width );
    }
}

int32 GDrawGetBiText8Width(GWindow gw, const char *text, int len, int32 cnt, FontMods *mods) {
    int ret;
    struct tf_arg arg;

#ifndef _NO_LIBPANGO
    if ( gw->usepango )
return( _GXPDraw_DoText8(gw,0,0,text,cnt,mods,0x0,tf_width,&arg));
    else
#endif
    if (( ret = GDrawIsAllLeftToRight8(text,cnt))==1 ) {
	memset(&arg,'\0',sizeof(arg));
return( _GDraw_DoText8(gw,0,0,text,cnt,mods,0x0,tf_width,&arg));
    } else {
	unichar_t *temp = cnt==-1 ? utf82u_copy(text): utf82u_copyn(text,cnt);
	int width;
	width = _GDraw_DoBiWidth(gw,temp,u_strlen(temp),u_strlen(temp),mods,tf_width,ret);
	free(temp);
return( width );
    }
}

int32 GDrawGetBiText8Bounds(GWindow gw,const char *text, int32 cnt, FontMods *mods, GTextBounds *bounds) {
    int ret;
    struct tf_arg arg;

    memset(&arg,'\0',sizeof(arg));
    arg.first = true;
#ifndef _NO_LIBPANGO
    if ( gw->usepango )
	ret = _GXPDraw_DoText8(gw,0,0,text,cnt,mods,0x0,tf_rect,&arg);
    else
#endif
	ret = _GDraw_DoText8(gw,0,0,text,cnt,mods,0,tf_rect,&arg);
    *bounds = arg.size;
return( ret );
}
