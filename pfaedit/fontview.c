/* Copyright (C) 2000,2001 by George Williams */
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
#include "psfont.h"
#include <ustring.h>
#include <gkeysym.h>
#include <utype.h>
#include <gfile.h>
#include <chardata.h>
#include <math.h>

#define XOR_COLOR	0x505050
#define	FV_LAB_HEIGHT	15

#ifdef BIGICONS
#define fontview_width 32
#define fontview_height 32
static unsigned char fontview_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x02, 0x20, 0x80, 0x00,
   0x82, 0x20, 0x86, 0x08, 0x42, 0x21, 0x8a, 0x14, 0xc2, 0x21, 0x86, 0x04,
   0x42, 0x21, 0x8a, 0x14, 0x42, 0x21, 0x86, 0x08, 0x02, 0x20, 0x80, 0x00,
   0xaa, 0xaa, 0xaa, 0xaa, 0x02, 0x20, 0x80, 0x00, 0x82, 0xa0, 0x8f, 0x18,
   0x82, 0x20, 0x91, 0x24, 0x42, 0x21, 0x91, 0x02, 0x42, 0x21, 0x91, 0x02,
   0x22, 0x21, 0x8f, 0x02, 0xe2, 0x23, 0x91, 0x02, 0x12, 0x22, 0x91, 0x02,
   0x3a, 0x27, 0x91, 0x24, 0x02, 0xa0, 0x8f, 0x18, 0x02, 0x20, 0x80, 0x00,
   0xfe, 0xff, 0xff, 0xff, 0x02, 0x20, 0x80, 0x00, 0x42, 0x20, 0x86, 0x18,
   0xa2, 0x20, 0x8a, 0x04, 0xa2, 0x20, 0x86, 0x08, 0xa2, 0x20, 0x8a, 0x10,
   0x42, 0x20, 0x8a, 0x0c, 0x82, 0x20, 0x80, 0x00, 0x02, 0x20, 0x80, 0x00,
   0xaa, 0xaa, 0xaa, 0xaa, 0x02, 0x20, 0x80, 0x00};
#else
#define fontview2_width 16
#define fontview2_height 16
static unsigned char fontview2_bits[] = {
   0x00, 0x07, 0x80, 0x08, 0x40, 0x17, 0x40, 0x15, 0x60, 0x09, 0x10, 0x02,
   0xa0, 0x01, 0xa0, 0x00, 0xa0, 0x00, 0xa0, 0x00, 0x50, 0x00, 0x52, 0x00,
   0x55, 0x00, 0x5d, 0x00, 0x22, 0x00, 0x1c, 0x00};
#endif

extern int _GScrollBar_Width;

int default_fv_font_size = 24, default_fv_antialias=false;
FontView *fv_list=NULL;

void FVToggleCharChanged(FontView *fv,SplineChar *sc) {
    int i, j;

    i = sc->enc / fv->colcnt;
    j = sc->enc - i*fv->colcnt;
    i -= fv->rowoff;
    if ( i>=0 && i<fv->rowcnt ) {
	GRect r;
	r.x = j*fv->cbw+1; r.width = fv->cbw;
	r.y = i*fv->cbh-1; r.height = FV_LAB_HEIGHT+1;
	GDrawSetXORBase(fv->v,GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(fv->v)));
	GDrawSetXORMode(fv->v);
	GDrawFillRect(fv->v,&r,0x000000);
	GDrawSetCopyMode(fv->v);
    }
}

static void FVToggleCharSelected(FontView *fv,int enc) {
    int i, j;

    i = enc / fv->colcnt;
    j = enc - i*fv->colcnt;
    i -= fv->rowoff;
    if ( i>=0 && i<fv->rowcnt ) {
	GRect r;
	r.x = j*fv->cbw+1; r.width = fv->cbw-1;
	r.y = i*fv->cbh+FV_LAB_HEIGHT+1; r.height = fv->cbw;
	GDrawSetXORBase(fv->v,GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(fv->v)));
	GDrawSetXORMode(fv->v);
	GDrawFillRect(fv->v,&r,XOR_COLOR);
	GDrawSetCopyMode(fv->v);
    }
}

static void FVDeselectAll(FontView *fv) {
    int i;

    for ( i=0; i<fv->sf->charcnt; ++i ) {
	if ( fv->selected[i] ) {
	    fv->selected[i] = false;
	    FVToggleCharSelected(fv,i);
	}
    }
}

static void FVSelectAll(FontView *fv) {
    int i;

    for ( i=0; i<fv->sf->charcnt; ++i ) {
	if ( !fv->selected[i] ) {
	    fv->selected[i] = true;
	    FVToggleCharSelected(fv,i);
	}
    }
}

static void FVReselect(FontView *fv, int newpos) {
    int i, start, end;

    start = fv->pressed_pos; end = fv->end_pos;

    if ( fv->pressed_pos<fv->end_pos ) {
	if ( newpos>fv->end_pos ) {
	    for ( i=fv->end_pos+1; i<=newpos; ++i ) if ( !fv->selected[i] ) {
		fv->selected[i] = true;
		FVToggleCharSelected(fv,i);
	    }
	} else if ( newpos<fv->pressed_pos ) {
	    for ( i=fv->end_pos; i>fv->pressed_pos; --i ) if ( fv->selected[i] ) {
		fv->selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	    for ( i=fv->pressed_pos-1; i>=newpos; --i ) if ( !fv->selected[i] ) {
		fv->selected[i] = true;
		FVToggleCharSelected(fv,i);
	    }
	} else {
	    for ( i=fv->end_pos; i>newpos; --i ) if ( fv->selected[i] ) {
		fv->selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	}
    } else {
	if ( newpos<fv->end_pos ) {
	    for ( i=fv->end_pos-1; i>=newpos; --i ) if ( !fv->selected[i] ) {
		fv->selected[i] = true;
		FVToggleCharSelected(fv,i);
	    }
	} else if ( newpos>fv->pressed_pos ) {
	    for ( i=fv->end_pos; i<fv->pressed_pos; ++i ) if ( fv->selected[i] ) {
		fv->selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	    for ( i=fv->pressed_pos+1; i<=newpos; ++i ) if ( !fv->selected[i] ) {
		fv->selected[i] = true;
		FVToggleCharSelected(fv,i);
	    }
	} else {
	    for ( i=fv->end_pos; i<newpos; ++i ) if ( fv->selected[i] ) {
		fv->selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	}
    }
    fv->end_pos = newpos;
}

void SplineFontSetUnChanged(SplineFont *sf) {
    int i, was = sf->changed;
    BDFFont *bdf;

    sf->changed = false;
    SFClearAutoSave(sf);
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	sf->chars[i]->changed = false;
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	for ( i=0; i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL )
	    bdf->chars[i]->changed = false;
    if ( was )
	GDrawRequestExpose(sf->fv->v,NULL,false);
}

static void FVFlattenAllBitmapSelections(FontView *fv) {
    BDFFont *bdf;
    int i;

    for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; i<bdf->charcnt; ++i )
	    if ( bdf->chars[i]!=NULL && bdf->chars[i]->selection!=NULL )
		BCFlattenFloat(bdf->chars[i]);
    }
}

static int AskChanged(SplineFont *sf) {
    char buffer[300];
    unichar_t *ubuf;
    int ret;
    static unichar_t save[] = { 'S', 'a', 'v', 'e', '\0' };
    static unichar_t dontsave[] = { 'D', 'o', 'n', '\'', 't', ' ', 'S', 'a', 'v', 'e', '\0' };
    static unichar_t cancel[] = { 'C', 'a', 'n', 'c', 'e', 'l', '\0' };
    static unichar_t *buts[] = { save, dontsave, cancel, NULL };
    static unichar_t title[] = { 'F', 'o', 'n', 't', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 'd',  '\0' };
    static unichar_t sdcmn[] = { 'S', 'D', 'C',  '\0' };
    char *filename = sf->filename, *fontname = sf->fontname;

    if ( filename==NULL && sf->origname!=NULL &&
	    sf->onlybitmaps && sf->bitmaps!=NULL && sf->bitmaps->next==NULL )
	filename = sf->origname;
    if ( filename==NULL ) filename = "untitled.sfd";
    filename = GFileNameTail(filename);
    sprintf( buffer, "Font %.70s in file %.70s has been changed.\nDo you want to save it?",
	    fontname, filename );
    ubuf = uc_copy(buffer);
    ret = GWidgetAsk( title,ubuf,buts,sdcmn,0,2);
    free(ubuf);
return( ret );
}

static int RevertAskChanged(char *fontname,char *filename) {
    char buffer[350];
    unichar_t *ubuf;
    int ret;
    static unichar_t save[] = { 'R', 'e', 'v', 'e','r','t', '\0' };
    static unichar_t cancel[] = { 'C', 'a', 'n', 'c', 'e', 'l', '\0' };
    static unichar_t *buts[] = { save, cancel, NULL };
    static unichar_t title[] = { 'F', 'o', 'n', 't', ' ', 'c', 'h', 'a', 'n', 'g', 'e', 'd',  '\0' };
    static unichar_t rcmn[] = { 'R', 'C',  '\0' };

    if ( filename==NULL ) filename = "untitled.sfd";
    filename = GFileNameTail(filename);
    sprintf( buffer, "Font %.70s in file %.70s has been changed.\nReverting the file will lose those changes.\nIs that what you want?",
	    fontname, filename );
    ubuf = uc_copy(buffer);
    ret = GWidgetAsk( title,ubuf,buts,rcmn,0,1);
    free(ubuf);
return( ret==0 );
}

int _FVMenuGenerate(FontView *fv) {
    FVFlattenAllBitmapSelections(fv);
return( FontMenuGeneratePostscript(fv->sf) );
}

static void FVMenuGenerate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    _FVMenuGenerate(fv);
}

int _FVMenuSaveAs(FontView *fv) {
    static unichar_t title[] = { 'S','a','v','e',' ','A','s', '\0' };
    unichar_t *temp;
    unichar_t *ret;
    char *filename;
    int ok;

    if ( fv->sf->filename!=NULL )
	temp=uc_copy(fv->sf->filename);
    else {
	temp = galloc(sizeof(unichar_t)*(strlen(fv->sf->fontname)+8));
	uc_strcpy(temp,fv->sf->fontname);
	uc_strcat(temp,".sfd");
    }
    ret = GWidgetSaveAsFile(title,temp,NULL,NULL);
    free(temp);
    if ( ret==NULL )
return( 0 );
    filename = cu_copy(ret);
    free(ret);
    FVFlattenAllBitmapSelections(fv);
    ok = SFDWrite(filename,fv->sf);
    if ( ok ) {
	free(fv->sf->filename);
	fv->sf->filename = filename;
	free(fv->sf->origname);
	fv->sf->origname = copy(filename);
	SplineFontSetUnChanged(fv->sf);
    } else
	free(filename);
return( ok );
}

static void FVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    _FVMenuSaveAs(fv);
}

int _FVMenuSave(FontView *fv) {
    static unichar_t failedtitle[] = { 'S','a','v','e',' ','F','a','i','l','e','d', '\0' };
    int ret = 0;

    if ( fv->sf->filename==NULL && fv->sf->origname!=NULL &&
	    fv->sf->onlybitmaps && fv->sf->bitmaps!=NULL && fv->sf->bitmaps->next==NULL ) {
	/* If it's a single bitmap font then just save back to the bdf file */
	FVFlattenAllBitmapSelections(fv);
	ret = BDFFontDump(fv->sf->origname,fv->sf->bitmaps,EncodingName(fv->sf->encoding_name));
	if ( ret )
	    SplineFontSetUnChanged(fv->sf);
    } else if ( fv->sf->filename==NULL )
	ret = _FVMenuSaveAs(fv);
    else {
	FVFlattenAllBitmapSelections(fv);
	if ( !SFDWriteBak(fv->sf) )
	    GWidgetPostNotice(failedtitle,failedtitle);
	else {
	    SplineFontSetUnChanged(fv->sf);
	    ret = true;
	}
    }
return( ret );
}

static void FVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    _FVMenuSave(fv);
}

static int _FVMenuClose(FontView *fv) {
    int i;
    BDFFont *bdf;
    MetricsView *mv, *mnext;

    if ( fv->sf->changed ) {
	i = AskChanged(fv->sf);
	if ( i==2 )	/* Cancel */
return( false );
	if ( i==0 && !_FVMenuSave(fv))		/* Save */
return(false);
	else
	    SFClearAutoSave(fv->sf);		/* if they didn't save it, remove change record */
    }
    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL ) {
	CharView *cv, *next;
	for ( cv = fv->sf->chars[i]->views; cv!=NULL; cv = next ) {
	    next = cv->next;
	    GDrawDestroyWindow(cv->gw);
	}
    }
    for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL ) {
	    BitmapView *bv, *next;
	    for ( bv = bdf->chars[i]->views; bv!=NULL; bv = next ) {
		next = bv->next;
		GDrawDestroyWindow(bv->gw);
	    }
	}
    }
    for ( mv=fv->metrics; mv!=NULL; mv = mnext ) {
	mnext = mv->next;
	GDrawDestroyWindow(mv->gw);
    }
    if ( fv->sf->filename!=NULL )
	RecentFilesRemember(fv->sf->filename);
    GDrawDestroyWindow(fv->gw);
return( true );
}

void MenuNew(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontNew();
}

static void FVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    _FVMenuClose(fv);
}

void FVRevert(FontView *fv) {
    SplineFont *temp, *old = fv->sf;
    BDFFont *tbdf, *fbdf;
    BDFChar *bc;
    BitmapView *bv, *bvnext;
    CharView *cv, *cvnext;
    MetricsView *mv, *mvnext;
    int i, enc;

    if ( fv->sf->origname==NULL )
return;
    if ( fv->sf->changed )
	if ( !RevertAskChanged(fv->sf->fontname,fv->sf->origname))
return;
    temp = ReadSplineFont(fv->sf->origname);
    if ( temp==NULL ) {
return;
    }
    for ( i=0; i<fv->sf->charcnt; ++i ) {
	if ( fv->sf->chars[i]!=NULL && fv->sf->chars[i]->views!=NULL ) {
	    enc = SFFindChar(temp,fv->sf->chars[i]->unicodeenc,fv->sf->chars[i]->name);
	    if ( enc==-1 ) {
		for ( cv=fv->sf->chars[i]->views; cv!=NULL; cv = cvnext ) {
		    cvnext = cv->next;
		    GDrawDestroyWindow(cv->gw);
		}
	    } else {
		for ( cv=fv->sf->chars[i]->views; cv!=NULL; cv = cvnext ) {
		    cvnext = cv->next;
		    CVChangeSC(cv,temp->chars[enc]);
		}
	    }
	    GDrawProcessPendingEvents(NULL);
	}
    }
    for ( fbdf=fv->sf->bitmaps; fbdf!=NULL; fbdf=fbdf->next ) {
	for ( tbdf=temp->bitmaps; tbdf!=NULL; tbdf=tbdf->next )
	    if ( tbdf->pixelsize==fbdf->pixelsize )
	break;
	for ( i=0; i<fv->sf->charcnt; ++i ) {
	    if ( fbdf->chars[i]!=NULL && fbdf->chars[i]->views!=NULL ) {
		enc = SFFindChar(temp,fv->sf->chars[i]->unicodeenc,fv->sf->chars[i]->name);
		bc = enc==-1 || tbdf==NULL?NULL:tbdf->chars[enc];
		if ( bc==NULL ) {
		    for ( bv=fbdf->chars[i]->views; bv!=NULL; bv = bvnext ) {
			bvnext = bv->next;
			GDrawDestroyWindow(bv->gw);
		    }
		} else {
		    for ( bv=fbdf->chars[i]->views; bv!=NULL; bv = bvnext ) {
			bvnext = bv->next;
			BVChangeBC(bv,bc,true);
		    }
		}
		GDrawProcessPendingEvents(NULL);
	    }
	}
    }
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    for ( mv=fv->metrics; mv!=NULL; mv = mvnext ) {
	/* Don't bother trying to fix up metrics views, just not worth it */
	mvnext = mv->next;
	GDrawDestroyWindow(mv->gw);
    }
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    if ( temp->charcnt>fv->sf->charcnt ) {
	fv->selected = grealloc(fv->selected,temp->charcnt);
	for ( i=fv->sf->charcnt; i<temp->charcnt; ++i )
	    fv->selected[i] = false;
    }
    SFClearAutoSave(old);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    fv->sf = temp;
    FontViewReformat(fv);
    SplineFontFree(old);
}

static void FVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVRevert(fv);
}

void MenuPrefs(GWindow base,struct gmenuitem *mi,GEvent *e) {
    DoPrefs();
}

void MenuExit(GWindow base,struct gmenuitem *mi,GEvent *e) {
    FontView *fv, *next;

    for ( fv = fv_list; fv!=NULL; fv = next ) {
	next = fv->next;
	if ( !_FVMenuClose(fv))
return;
    }
    exit(0);
}

char *GetPostscriptFontName(int mult) {
    /* Some people use pf3 as an extension for postscript type3 fonts */
    static unichar_t wild[] = { '*', '.', '{', 'p','f','a',',','p','f','b',',','s','f','d',',','t','t','f',',','b','d','f',',','o','t','f',',','p','f','3','}', '\0' };
    static unichar_t title[] = { 'O','p','e','n',' ','p','o','s','t','s','c','r','i','p','t',' ','f','o','n','t', '\0' };
    unichar_t *ret = FVOpenFont(title,NULL,wild,NULL,mult);
    char *temp = cu_copy(ret);

    free(ret);
return( temp );
}

void MergeKernInfo(SplineFont *sf) {
    static unichar_t wild[] = { '*', '.', 'a', 'f','m',  '\0' };
    static unichar_t title[] = { 'M','e','r','g','e',' ','K','e','r','n',' ','I','n','f','o', '\0' };
    unichar_t *ret = GWidgetOpenFile(title,NULL,wild,NULL);
    char *temp = cu_copy(ret);

    if ( !LoadKerningDataFromAfm(sf,temp))
	GDrawError( "Failed to load data from %s", temp);
    free(ret); free(temp);
}

static void FVMenuMergeKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    MergeKernInfo(fv->sf);
}

void MenuOpen(GWindow base,struct gmenuitem *mi,GEvent *e) {
    char *temp = GetPostscriptFontName(true);
    char *eod, *fpt, *file, *full;

    if ( temp==NULL )
return;
    eod = strrchr(temp,'/');
    *eod = '\0';
    file = eod+1;
    do {
	fpt = strstr(file,"; ");
	if ( fpt!=NULL ) *fpt = '\0';
	full = galloc(strlen(temp)+1+strlen(file)+1);
	strcpy(full,temp); strcat(full,"/"); strcat(full,file);
	ViewPostscriptFont(full);
	file = fpt+2;
    } while ( fpt!=NULL );
    free(temp);
}

static void FVMenuImport(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVImport(fv);
}

static int FVSelCount(FontView *fv) {
    int i, cnt=0;
    static unichar_t manywin[] = { 'M','a','n','y',' ','W','i','n','d','o','w','s',  '\0' };
    static unichar_t toomany[] = { 'T','h','i','s',' ','i','n','v','o','l','v','e','s',' ','o','p','e','n','i','n','g',' ','m','o','r','e',' ','t','h','a','n',' ','1','0',' ','w','i','n','d','o','w','s','.','\n','I','s',' ','t','h','a','t',' ','r','e','a','l','l','y',' ','w','h','a','t',' ','y','o','u',' ','w','a','n','t','?',  '\0' };
    static unichar_t ok[] = { 'O', 'k', '\0' };
    static unichar_t cancel[] = { 'C', 'a', 'n', 'c', 'e', 'l', '\0' };
    static unichar_t ocmn[] = { 'O', 'C', '\0' };
    static unichar_t *buts[] = { ok, cancel, NULL };

    for ( i=0; i<fv->sf->charcnt; ++i )
	if ( fv->selected[i] ) ++cnt;
    if ( cnt>10 ) {
	if ( GWidgetAsk(manywin,toomany,buts,ocmn,0,1)==1 )
return( false );
    }
return( true );
}
	
static void FVMenuOpenOutline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;

    if ( !FVSelCount(fv))
return;
    for ( i=0; i<fv->sf->charcnt; ++i )
	if ( fv->selected[i] ) {
	    sc = SFMakeChar(fv->sf,i);
	    CharViewCreate(sc,fv);
	}
}

static void FVMenuOpenBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i;

    if ( fv->sf->bitmaps==NULL )
return;
    if ( !FVSelCount(fv))
return;
    for ( i=0; i<fv->sf->charcnt; ++i )
	if ( fv->selected[i] ) {
	    SFMakeChar(fv->sf,i);
	    BitmapViewCreatePick(i,fv);
	}
}

static void FVMenuOpenMetrics(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    MetricsViewCreate(fv,NULL,fv->filled==fv->show?NULL:fv->show);
}

static void FVMenuFontInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FontMenuFontInfo(fv->sf,fv);
}

static void FVMenuPrivateInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    SFPrivateInfo(fv->sf);
}

static void FVMenuFindProblems(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FindProblems(fv,NULL);
}

#define MID_24	2001
#define MID_36	2002
#define MID_48	2004
#define MID_AntiAlias	2005
#define MID_Next	2006
#define MID_Prev	2007
#define MID_CharInfo	2201
#define MID_FindProblems	2216
#define MID_Transform	2202
#define MID_Stroke	2203
#define MID_RmOverlap	2204
#define MID_Simplify	2205
#define MID_Correct	2206
#define MID_BuildAccent	2208
#define MID_AvailBitmaps	2210
#define MID_RegenBitmaps	2211
#define MID_Autotrace	2212
#define MID_Round	2213
#define MID_MergeFonts	2214
#define MID_InterpolateFonts	2215
#define MID_Center	2600
#define MID_SetWidth	2601
#define MID_SetLBearing	2602
#define MID_SetRBearing	2603
#define MID_Thirds	2604
#define MID_AutoHint	2501
#define MID_OpenBitmap	2700
#define MID_OpenOutline	2701
#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_SelAll	2106
#define MID_CopyRef	2107
#define MID_UnlinkRef	2108
#define MID_Undo	2109
#define MID_Redo	2110
#define MID_CopyWidth	2111
#define MID_AllFonts	2112
#define MID_DisplayedFont	2113

/* returns -1 if nothing selected, the char if exactly one, -2 if more than one */
static int FVAnyCharSelected(FontView *fv) {
    int i, val=-1;

    for ( i=0; i<fv->sf->charcnt; ++i ) {
	if ( fv->selected[i]) {
	    if ( val==-1 )
		val = i;
	    else
return( -2 );
	}
    }
return( val );
}

static void FVMenuCopyFrom(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    fv->onlycopydisplayed = (mi->mid==MID_DisplayedFont);
}

static void FVMenuCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopy(fv,true);
}

static void FVMenuCopyRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopy(fv,false);
}

static void FVMenuCopyWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopyWidth(fv);
}

static void FVMenuPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    PasteIntoFV(fv);
}

static void BCClearAll(BDFChar *bc,FontView *fv) {
    if ( bc==NULL )
return;
    BCPreserveState(bc);
    BCFlattenFloat(bc);
    memset(bc->bitmap,'\0',bc->bytes_per_line*(bc->ymax-bc->ymin+1));
    BCCompressBitmap(bc);
    BCCharChangedUpdate(bc,fv);
}

static void SCClearAll(SplineChar *sc,FontView *fv) {
    RefChar *refs, *next;

    if ( sc==NULL )
return;
    SCPreserveState(sc);
    sc->widthset = false;
	sc->width = sc->parent->ascent+sc->parent->descent;
    if ( sc->splines==NULL && sc->refs==NULL )
return;
    SplinePointListsFree(sc->splines);
    sc->splines = NULL;
    for ( refs=sc->refs; refs!=NULL; refs = next ) {
	next = refs->next;
	RefCharFree(refs);
    }
    sc->refs = NULL;
    SCCharChangedUpdate(sc,fv);
}

static void FVMenuClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i;
    BDFFont *bdf;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] ) {
	if ( fv->onlycopydisplayed && fv->filled==fv->show ) {
	    SCClearAll(fv->sf->chars[i],fv);
	} else if ( fv->onlycopydisplayed ) {
	    BCClearAll(fv->show->chars[i],fv);
	} else {
	    SCClearAll(fv->sf->chars[i],fv);
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next )
		BCClearAll(bdf->chars[i],fv);
	}
    }
}

static void FVMenuUnlinkRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    RefChar *rf, *next;

    for ( i=0; i<fv->sf->charcnt; ++i )
	    if ( fv->selected[i] && (sc=fv->sf->chars[i])!=NULL && sc->refs!=NULL ) {
	SCPreserveState(sc);
	for ( rf=sc->refs; rf!=NULL ; rf=next ) {
	    next = rf->next;
	    SCRefToSplines(sc,rf);
	}
	SCCharChangedUpdate(sc,fv);
    }
}

static void FVMenuCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVMenuCopy(gw,mi,e);
    FVMenuClear(gw,mi,e);
}

static void FVSelectAll(FontView *);

static void FVMenuSelectAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVSelectAll(fv);
}

static void cflistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AllFonts:
	    mi->ti.checked = !fv->onlycopydisplayed;
	  break;
	  case MID_DisplayedFont:
	    mi->ti.checked = fv->onlycopydisplayed;
	  break;
	}
    }
}

static void edlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int pos = FVAnyCharSelected(fv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Cut: case MID_Copy: case MID_Clear: case MID_CopyWidth:
	  case MID_Paste: case MID_CopyRef: case MID_UnlinkRef:
	    mi->ti.disabled = pos==-1;
	  break;
	  case MID_Undo: case MID_Redo:
	    mi->ti.disabled = true;
	  break;
	}
    }
}

static void FVMenuCharInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int pos = FVAnyCharSelected(fv);
    if ( pos<0 )
return;
    SFMakeChar(fv->sf,pos);
    SCGetInfo(fv->sf->chars[pos],true);
}

static int getorigin(void *d,BasePoint *base,int index) {
    /*FontView *fv = (FontView *) d;*/

    base->x = base->y = 0;
    switch ( index ) {
      case 0:		/* Character origin */
	/* all done */
      break;
      case 1:		/* Center of selection */
	/*CVFindCenter(cv,base,!CVAnySel(cv,NULL,NULL,NULL));*/
      break;
      default:
return( false );
    }
return( true );
}

void FVTrans(FontView *fv,SplineChar *sc,double transform[6], char *sel) {
    RefChar *refs;
    double t[6];

    SCPreserveState(sc);
    if ( transform[0]>0 && transform[3]>0 && transform[1]==0 && transform[2]==0 ) {
	SCSynchronizeWidth(sc,sc->width*transform[0],sc->width,fv);
    }
    SplinePointListTransform(sc->splines,transform,true);
    for ( refs = sc->refs; refs!=NULL; refs=refs->next ) {
	if ( sel!=NULL && sel[refs->sc->enc] ) {
	    /* if the character referred to is selected then it's going to */
	    /*  be scaled too (or will have been) so we don't want to scale */
	    /*  it twice */
	    t[4] = refs->transform[4]*transform[0] +
			refs->transform[5]*transform[2] +
			/*transform[4]*/0;
	    t[5] = refs->transform[4]*transform[1] +
			refs->transform[5]*transform[3] +
			/*transform[5]*/0;
	    refs->transform[4] = t[4];
	    refs->transform[5] = t[5];
	} else {
	    SplinePointListTransform(refs->splines,transform,true);
	    t[0] = refs->transform[0]*transform[0] +
			refs->transform[1]*transform[2];
	    t[1] = refs->transform[0]*transform[1] +
			refs->transform[1]*transform[3];
	    t[2] = refs->transform[2]*transform[0] +
			refs->transform[3]*transform[2];
	    t[3] = refs->transform[2]*transform[1] +
			refs->transform[3]*transform[3];
	    t[4] = refs->transform[4]*transform[0] +
			refs->transform[5]*transform[2] +
			transform[4];
	    t[5] = refs->transform[4]*transform[1] +
			refs->transform[5]*transform[3] +
			transform[5];
	    memcpy(refs->transform,t,sizeof(t));
	}
	SplineSetFindBounds(refs->splines,&refs->bb);
    }
    if ( transform[0]==1 && transform[3]==1 && transform[1]==0 &&
	    transform[2]==0 && transform[5]==0 &&
	    transform[4]!=0 && 
	    sc->unicodeenc!=-1 && isalpha(sc->unicodeenc)) {
	SCUndoSetLBearingChange(sc,(int) rint(transform[4]));
	SCSynchronizeLBearing(sc,NULL,transform[4]);
    }
    SCCharChangedUpdate(sc,fv);
}

static void FVTransFunc(void *_fv,double transform[6],int otype, BVTFunc *bvts) {
    FontView *fv = _fv;
    double transx = transform[4], transy=transform[5];
    DBounds bb;
    BasePoint base;
    int i, cnt=0;
    static unichar_t trans[] = { 'T','r','a','n','s','f','o','r','m','i','n','g',' ','.','.','.',  '\0' };
    BDFFont *bdf;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] )
	++cnt;
    GProgressStartIndicator(10,trans,trans,NULL,cnt,1);

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] ) {
	SplineChar *sc = fv->sf->chars[i];

	if ( fv->onlycopydisplayed && fv->show!=fv->filled ) {
	    if ( fv->show->chars[i]!=NULL )
		BCTrans(bdf,fv->show->chars[i],bvts,fv);
	} else {
	    if ( otype==1 ) {
		SplineCharFindBounds(sc,&bb);
		base.x = (bb.minx+bb.maxx)/2;
		base.y = (bb.miny+bb.maxy)/2;
		transform[4]=transx+base.x-
		    (transform[0]*base.x+transform[2]*base.y);
		transform[5]=transy+base.y-
		    (transform[1]*base.x+transform[3]*base.y);
	    }
	    FVTrans(fv,sc,transform,fv->selected);
	    if ( !fv->onlycopydisplayed ) {
		for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next ) if ( bdf->chars[i]!=NULL )
		    BCTrans(bdf,bdf->chars[i],bvts,fv);
	    }
	}
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void FVMenuBitmaps(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    BitmapDlg(fv,NULL,mi->mid==MID_AvailBitmaps );
}

static void FVMenuTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    TransformDlgCreate(fv,FVTransFunc,getorigin);
}

static void FVMenuStroke(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVStroke(fv);
}

static void FVMenuOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i, cnt=0;
    static unichar_t over[] = { 'R','e','m','o','v','i','n','g',' ','o','v','e','l','a','p','s',  '\0' };

    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    DoAutoSaves();

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] )
	++cnt;
    GProgressStartIndicator(10,over,over,NULL,cnt,1);

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] ) {
	SplineChar *sc = fv->sf->chars[i];
	SCPreserveState(sc);
	sc->splines = SplineSetRemoveOverlap(sc->splines);
	SCCharChangedUpdate(sc,fv);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void FVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i, cnt=0;
    static unichar_t simplifying[] = { 'S','i','m','p','l','i','f','y','i','n','g',' ','.','.','.',  '\0' };

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] )
	++cnt;
    GProgressStartIndicator(10,simplifying,simplifying,NULL,cnt,1);

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] ) {
	SplineChar *sc = fv->sf->chars[i];
	SCPreserveState(sc);
	SplineCharSimplify(sc->splines);
	SCCharChangedUpdate(sc,fv);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void FVMenuCorrectDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i, cnt=0;
    static unichar_t correcting[] = { 'C','o','r','r','e','c','t','i','n','g',' ','D','i','r','e','c','t','i','o','n','.','.','.',  '\0' };

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] )
	++cnt;
    GProgressStartIndicator(10,correcting,correcting,NULL,cnt,1);

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] ) {
	SplineChar *sc = fv->sf->chars[i];
	SCPreserveState(sc);
	sc->splines = SplineSetsCorrect(sc->splines);
	SCCharChangedUpdate(sc,fv);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void FVMenuRound2Int(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] )
	SCRound2Int( fv->sf->chars[i], fv);
}

static void FVMenuAutotrace(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVAutoTrace(fv);
}

int hascomposing(SplineFont *sf,int u) {
    const unichar_t *upt;

    if ( u>=0 && u<=65535 && unicode_alternates[u>>8]!=NULL &&
	    (upt = unicode_alternates[u>>8][u&0xff])!=NULL ) {
	while ( *upt ) {
	    if ( iscombining(*upt) || *upt==0xb7 ||	/* b7, centered dot is used as a combining accent for Ldot */
		    *upt==0x1fcd || *upt==0x1fdd || *upt==0x1fce || *upt==0x1fde )	/* Special greek accents */
return( true );
	    ++upt;
	}

	if ( u>=0x1f70 && u<0x2000 ) {
	    upt = SFGetAlternate(sf,u);
	    while ( *upt ) {
		if ( iscombining(*upt) )
return( true );
		++upt;
	    }
	}
    }
return( false );
}
    
static void FVMenuBuildAccent(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i, cnt=0;
    SplineChar dummy;
    static unichar_t accents[] = { 'B','u','i','l','d','i','n','g',' ','a','c','c','e','n','t','e','d',' ','l','e','t','t','e','r','s',  '\0' };
    int onlyaccents = e==NULL || !(e->u.mouse.state&ksm_shift);
    static unichar_t title[] = { 'R','e','p','l','a','c','e',' ',0xC5,  '\0' };
    static unichar_t replace[] = { 'A','r','e',' ','y','o','u',' ','s','u','r','e',' ','y','o','u',' ','w','a','n','t',' ','t','o',' ','r','e','p','l','a','c','e',' ',0xc5,'?','\n','T','h','e',' ','r','i','n','g',' ','w','i','l','l',' ','n','o','t',' ','j','o','i','n',' ','t','o',' ','t','h','e',' ','A','.',  '\0' };
    static unichar_t yes[] = { 'Y','e','s',  '\0' };
    static unichar_t no[] = { 'N','o',  '\0' };
    static unichar_t *buts[] = { yes, no, NULL };
    static unichar_t mn[] = { 'Y', 'N', '\0' };

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] )
	++cnt;
    GProgressStartIndicator(10,accents,accents,NULL,cnt,1);

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] ) {
	SplineChar *sc = fv->sf->chars[i];
	if ( sc==NULL )
	    sc = SCBuildDummy(&dummy,fv->sf,i);
	else if ( sc->unicodeenc == 0x00c5 /* Aring */ && sc->splines!=NULL ) {
	    if ( GWidgetAsk(title,replace,buts,mn,0,1)==1 )
    continue;
	}
	if ( SFIsCompositBuildable(fv->sf,sc->unicodeenc) &&
		(!onlyaccents || hascomposing(fv->sf,sc->unicodeenc))) {
	    sc = SFMakeChar(fv->sf,i);
	    SCBuildComposit(fv->sf,sc,!fv->onlycopydisplayed,fv);
	}
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void FVMenuMergeFonts(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVMergeFonts(fv);
}

static void FVMenuInterpFonts(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVInterpolateFonts(fv);
}

static void FVScrollToChar(FontView *fv,int i) {
    if ( i!=-1 ) {
	if ( i/fv->colcnt<fv->rowoff || i/fv->colcnt >= fv->rowoff+fv->rowcnt ) {
	    fv->rowoff = i/fv->colcnt;
	    if ( fv->rowltot>= 3 )
		--fv->rowoff;
	    if ( fv->rowoff+fv->rowcnt>=fv->rowltot )
		fv->rowoff = fv->rowltot-fv->rowcnt;
	    if ( fv->rowoff<0 ) fv->rowoff = 0;
	    GScrollBarSetPos(fv->vsb,fv->rowoff);
	    GDrawRequestExpose(fv->v,NULL,false);
	}
    }
}

static void FVShowInfo(FontView *fv);
static void FVChangeChar(FontView *fv,int i) {

    if ( i!=-1 ) {
	FVDeselectAll(fv);
	fv->selected[i] = true;
	fv->end_pos = fv->pressed_pos = i;
	FVToggleCharSelected(fv,i);
	FVScrollToChar(fv,i);
	FVShowInfo(fv);
    }
}

static void FVMenuChangeChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int pos = FVAnyCharSelected(fv);
    if ( pos>=0 ) {
	if ( mi->mid==MID_Next )
	    ++pos;
	else
	    --pos;
    }
    if ( pos<0 ) pos = fv->sf->charcnt-1;
    else if ( pos>= fv->sf->charcnt ) pos = 0;
    if ( pos>=0 && pos<fv->sf->charcnt )
	FVChangeChar(fv,pos);
}

static void FVMenuGotoChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int pos = GotoChar(fv->sf);
    FVChangeChar(fv,pos);
}

static void FVChangeDisplayFont(FontView *fv,BDFFont *bdf) {
    int samesize=0;

    if ( fv->show!=bdf ) {
	if ( fv->show!=NULL && fv->show->pixelsize==bdf->pixelsize )
	    samesize = true;
	fv->show = bdf;
	fv->cbw = bdf->pixelsize+1;
	fv->cbh = bdf->pixelsize+1+FV_LAB_HEIGHT+1;
	if ( samesize ) {
	    GDrawRequestExpose(fv->v,NULL,false);
	} else if ( bdf->pixelsize<=48 ) {
	    GDrawResize(fv->gw,
		    16*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
		    4*fv->cbh+1+fv->mbh+fv->infoh);
	} else if ( bdf->pixelsize<96 ) {
	    GDrawResize(fv->gw,
		    8*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
		    2*fv->cbh+1+fv->mbh+fv->infoh);
	} else {
	    GDrawResize(fv->gw,
		    4*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
		    fv->cbh+1+fv->mbh+fv->infoh);
	}
	/* colcnt, rowcnt, etc. will be set by the resize handler */
    }
}

static void FVMenuSize(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int dspsize = fv->filled->pixelsize;
    int changealias = false;

    if ( mi->mid == MID_24 )
	default_fv_font_size = dspsize = 24;
    else if ( mi->mid == MID_36 )
	default_fv_font_size = dspsize = 36;
    else if ( mi->mid == MID_48 )
	default_fv_font_size = dspsize = 48;
    else {
	default_fv_antialias = fv->antialias = !fv->antialias;
	fv->sf->display_antialias = fv->antialias;
	changealias = true;
    }
    if ( fv->filled!=fv->show || fv->filled->pixelsize != dspsize || changealias ) {
	BDFFont *new;
	if ( fv->antialias )
	    new = SplineFontAntiAlias(fv->sf,dspsize,4);
	else
	    new = SplineFontRasterize(fv->sf,dspsize,true);
	BDFFontFree(fv->filled);
	fv->filled = new;
	FVChangeDisplayFont(fv,new);
	fv->sf->display_size = -dspsize;
    }
}

static void FVMenuShowBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    BDFFont *bdf = mi->ti.userdata;

    FVChangeDisplayFont(fv,bdf);
    fv->sf->display_size = fv->show->pixelsize;
}

void FVShowFilled(FontView *fv) {
    if ( fv->show!=fv->filled )
	FVChangeDisplayFont(fv,fv->filled);
    fv->sf->display_size = -fv->filled->pixelsize;
}

static void ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);
    int anybuildable, anytraceable;
    int onlyaccents;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_CharInfo:
	    mi->ti.disabled = anychars<0;
	  break;
	  case MID_FindProblems:
	    mi->ti.disabled = anychars==-1;
	  break;
	  case MID_Transform:
	    mi->ti.disabled = anychars==-1;
	    /* some Transformations make sense on bitmaps now */
	  break;
	  case MID_Stroke: case MID_RmOverlap:
	  case MID_Simplify: case MID_Round: case MID_Correct:
	    mi->ti.disabled = anychars==-1 || fv->sf->onlybitmaps;
	  break;
	  case MID_RegenBitmaps:
	    mi->ti.disabled = fv->sf->bitmaps==NULL || fv->sf->onlybitmaps;
	  break;
	  case MID_BuildAccent:
	    anybuildable = false;
	    onlyaccents = e==NULL || !(e->u.mouse.state&ksm_shift);
	    if ( anychars!=-1 ) {
		int i;
		for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->selected[i] ) {
		    SplineChar *sc, dummy;
		    sc = fv->sf->chars[i];
		    if ( sc==NULL )
			sc = SCBuildDummy(&dummy,fv->sf,i);
		    if ( SFIsCompositBuildable(fv->sf,sc->unicodeenc) &&
			    (!onlyaccents || hascomposing(fv->sf,sc->unicodeenc)) ) {
			anybuildable = true;
		break;
		    }
		}
	    }
	    mi->ti.disabled = !anybuildable;
	    free(mi->ti.text);
	    mi->ti.text = uc_copy(onlyaccents?"Build Accented Char": "Build Composite Chars");
	  break;
	  case MID_Autotrace:
	    anytraceable = false;
	    if ( FindAutoTraceName()!=NULL && anychars!=-1 ) {
		int i;
		for ( i=0; i<fv->sf->charcnt; ++i )
		    if ( fv->selected[i] && fv->sf->chars[i]!=NULL &&
			    fv->sf->chars[i]->backimages!=NULL ) {
			anytraceable = true;
		break;
		    }
	    }
	    mi->ti.disabled = !anytraceable;
	  break;
	  case MID_MergeFonts: case MID_InterpolateFonts:
	    mi->ti.disabled = fv->sf->bitmaps==NULL || fv->sf->onlybitmaps;
	  break;
	}
    }
}

static void FVMenuCenter(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i;
    DBounds bb;
    double transform[6];

    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = transform[5] = 0.0;
    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] ) {
	SplineChar *sc = fv->sf->chars[i];
	SplineCharFindBounds(sc,&bb);
	if ( mi->mid==MID_Center )
	    transform[4] = (sc->width-(bb.maxx-bb.minx))/2 - bb.minx;
	else
	    transform[4] = (sc->width-(bb.maxx-bb.minx))/3 - bb.minx;
	if ( transform[4]!=0 )
	    FVTrans(fv,sc,transform,NULL);
    }
}

static void FVMenuSetWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVSetWidth(fv,mi->mid==MID_SetWidth?wt_width:
		  mi->mid==MID_SetLBearing?wt_lbearing:
		  wt_rbearing);
}

static void FVMenuAutoWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVAutoWidth(fv);
}

static void FVMenuAutoKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVAutoKern(fv);
}

static void FVMenuRemoveKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    FVRemoveKerns(fv);
}

static void FVMenuPrint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    PrintDlg(fv,NULL,NULL);
}

static void mtlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Center: case MID_Thirds: case MID_SetWidth: 
	  case MID_SetLBearing: case MID_SetRBearing:
	    mi->ti.disabled = anychars==-1;
	  break;
	}
    }
}

static void FVMenuAutoHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i, cnt=0;
    static unichar_t autohint[] = { 'A','u','t','o','h','i','n','t','i','n','g',' ','f','o','n','t',  '\0' };
    int removeOverlap = e==NULL || !(e->u.mouse.state&ksm_shift);

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] )
	++cnt;
    GProgressStartIndicator(10,autohint,autohint,NULL,cnt,1);

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] ) {
	SplineChar *sc = fv->sf->chars[i];
	sc->manualhints = false;
	SplineCharAutoHint(sc,removeOverlap);
	SCUpdateAll(sc);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void htlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);
    int removeOverlap;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AutoHint:
	    mi->ti.disabled = anychars==-1;
	    removeOverlap = e==NULL || !(e->u.mouse.state&ksm_shift);
	    free(mi->ti.text);
	    mi->ti.text = uc_copy(removeOverlap?"AutoHint": "Full AutoHint");
	  break;
	}
    }
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_OpenOutline:
	    mi->ti.disabled = !anychars;
	  break;
	  case MID_OpenBitmap:
	    mi->ti.disabled = !anychars || fv->sf->bitmaps==NULL;
	  break;
	  case MID_Revert:
	    mi->ti.disabled = fv->sf->origname==NULL;
	  break;
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static unichar_t file[] = { 'F', 'i', 'l', 'e', '\0' };
static unichar_t edit[] = { 'E', 'd', 'i', 't', '\0' };
static unichar_t view[] = { 'V', 'i', 'e', 'w', '\0' };
static unichar_t element[] = { 'E', 'l', 'e', 'm', 'e', 'n', 't', '\0' };
static unichar_t window[] = { 'W', 'i', 'n', 'd', 'o', 'w', '\0' };
static unichar_t hints[] = { 'H', 'i', 'n', 't', 's',  '\0' };
static unichar_t new[] = { 'N', 'e', 'w', '\0' };
static unichar_t open[] = { 'O', 'p', 'e', 'n', '.', '.', '.', '\0' };
static unichar_t recent[] = { 'R', 'e', 'c', 'e', 'n', 't',  '\0' };
static unichar_t openoutline[] = { 'O', 'p', 'e', 'n', ' ', 'O', 'u', 't', 'l', 'i', 'n', 'e', '\0' };
static unichar_t openbitmap[] = { 'O', 'p', 'e', 'n', ' ', 'B', 'i', 't', 'm', 'a', 'p', '\0' };
static unichar_t openmetrics[] = { 'O', 'p', 'e', 'n', ' ', 'M', 'e', 't', 'r', 'i', 'c', 's', '\0' };
static unichar_t revert[] = { 'R', 'e', 'v', 'e','r','t',' ', 'F','i','l','e','\0' };
static unichar_t mergekern[] = { 'M', 'e', 'r', 'g','e',' ','K', 'e','r','n',' ','I','n','f','o','.','.','.', '\0' };
static unichar_t save[] = { 'S', 'a', 'v', 'e',  '\0' };
static unichar_t saveas[] = { 'S', 'a', 'v', 'e', ' ', 'a', 's', '.', '.', '.', '\0' };
static unichar_t generate[] = { 'G', 'e', 'n', 'e', 'r', 'a', 't', 'e', ' ', 'F','o','n','t','s', '.', '.', '.', '\0' };
static unichar_t import[] = { 'I', 'm', 'p', 'o', 'r', 't',  '.', '.', '.', '\0' };
static unichar_t close[] = { 'C', 'l', 'o', 's', 'e', '\0' };
static unichar_t print[] = { 'P', 'r', 'i', 'n', 't', '.', '.', '.',  '\0' };
static unichar_t prefs[] = { 'P', 'r', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e', 's', '.', '.', '.', '\0' };
static unichar_t quit[] = { 'Q', 'u', 'i', 't', '\0' };
static unichar_t _24[] = { '2', '4', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'o', 'u','t','l','i','n','e', '\0' };
static unichar_t _36[] = { '3', '6', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'o', 'u','t','l','i','n','e', '\0' };
static unichar_t _48[] = { '4', '8', ' ', 'p', 'i', 'x', 'e', 'l', ' ', 'o', 'u','t','l','i','n','e', '\0' };
static unichar_t antialias[] = { 'A', 'n', 't', 'i', ' ', 'A', 'l', 'i', 'a', 's',  '\0' };
static unichar_t next[] = { 'N', 'e', 'x', 't', ' ', 'C', 'h', 'a', 'r', '\0' };
static unichar_t prev[] = { 'P', 'r', 'e', 'v', ' ', 'C', 'h', 'a', 'r', '\0' };
static unichar_t _goto[] = { 'G', 'o', 't', 'o',  '\0' };
static unichar_t fontinfo[] = { 'F','o','n','t',' ','I','n','f','o', '.', '.', '.', '\0' };
static unichar_t privateinfo[] = { 'P','r','i','v','a','t','e',' ','I','n','f','o', '.', '.', '.', '\0' };
static unichar_t findprobs[] = { 'F','i','n','d',' ','P','r','o','b','l','e','m','s', '.', '.', '.',  '\0' };
static unichar_t getinfo[] = { 'C','h','a','r',' ','I','n','f','o', '.', '.', '.', '\0' };
static unichar_t bitmapsavail[] = { 'B','i','t','m','a','p','s',' ','A','v','a','i','l','a','b','l','e','.', '.', '.',  '\0' };
static unichar_t regenbitmaps[] = { 'R','e','g','e','n','e','r','a','t','e', ' ', 'B','i','t','m','a','p','s','.', '.', '.',  '\0' };
static unichar_t autotrace[] = { 'A','u','t','o','t','r','a','c', 'e', /*'.', '.', '.',*/  '\0' };
static unichar_t transform[] = { 'T','r','a','n','s','f','o','r', 'm', '.', '.', '.',  '\0' };
static unichar_t stroke[] = { 'E','x','p','a','n','d',' ','S', 't', 'r', 'o','k', 'e', '.', '.', '.',  '\0' };
static unichar_t rmoverlap[] = { 'R','e','m','o','v','e',' ','O', 'v', 'e', 'r','l', 'a', 'p',  '\0' };
static unichar_t correct[] = { 'C','o','r','r','e','c','t',' ','D','i','r','e','c','t','i','o','n',  '\0' };
static unichar_t simplify[] = { 'S','i','m','p','l','i','f','y',  '\0' };
static unichar_t round2int[] = { 'R','o','u','n','d',' ','t','o',' ','I','n','t',  '\0' };
static unichar_t buildaccent[] = { 'B','u','i','l','d',' ','A','c','c','e','n','t','e','d',' ','C','h','a','r','s',  '\0' };
static unichar_t merge[] = { 'M', 'e', 'r', 'g', 'e', ' ',  'F','o','n','t','s','.', '.', '.', '\0' };
static unichar_t interp[] = { 'I', 'n', 't', 'e', 'r', 'p', 'o','l','a','t','e', ' ',  'F','o','n','t','s','.', '.', '.', '\0' };
static unichar_t undo[] = { 'U', 'n', 'd', 'o',  '\0' };
static unichar_t redo[] = { 'R', 'e', 'd', 'o',  '\0' };
static unichar_t cut[] = { 'C', 'u', 't',  '\0' };
static unichar_t _copy[] = { 'C', 'o', 'p', 'y',  '\0' };
static unichar_t copywidth[] = { 'C', 'o', 'p', 'y', ' ', 'W', 'i', 'd', 't', 'h',  '\0' };
static unichar_t copyref[] = { 'C', 'o', 'p', 'y', ' ', 'R', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e',  '\0' };
static unichar_t paste[] = { 'P', 'a', 's', 't', 'e',  '\0' };
static unichar_t clear[] = { 'C', 'l', 'e', 'a', 'r',  '\0' };
static unichar_t selectall[] = { 'S', 'e', 'l', 'e', 'c', 't', ' ', 'A', 'l', 'l', '\0' };
static unichar_t unlinkref[] = { 'U', 'n', 'l', 'i', 'n', 'k', ' ', 'R', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e',  '\0' };
static unichar_t copyfrom[] = { 'C', 'o', 'p', 'y', ' ', 'F', 'r', 'o', 'm',  '\0' };
static unichar_t allfonts[] = { 'A', 'l', 'l', ' ', 'F', 'o', 'n', 't', 's',  '\0' };
static unichar_t displayedfont[] = { 'D', 'i', 's', 'p', 'l', 'a', 'y', 'e', 'd', ' ', 'F', 'o', 'n', 't',  '\0' };
static unichar_t metric[] = { 'M','e','t','r', 'i','c','s',  '\0' };
static unichar_t center[] = { 'C','e','n','t','e','r',' ','i','n',' ','W','i','d','t','h',  '\0' };
static unichar_t thirds[] = { 'T','h','i','r','d','s',' ','i','n',' ','W','i','d','t','h',  '\0' };
static unichar_t setwidth[] = { 'S','e','t',' ','W','i','d','t','h','.','.','.', '\0' };
static unichar_t setlbearing[] = { 'S','e','t',' ','L','B','e','a','r','i','n','g','.','.','.', '\0' };
static unichar_t setrbearing[] = { 'S','e','t',' ','R','B','e','a','r','i','n','g','.','.','.', '\0' };
static unichar_t autokern[] = { 'A','u','t','o',' ','K','e','r','n','.','.','.', '\0' };
static unichar_t autowidth[] = { 'A','u','t','o',' ','W','i','d','t','h','.','.','.', '\0' };
static unichar_t removekern[] = { 'R','e','m','o','v','e',' ','A','l','l',' ','K','e','r','n',' ','P','a','i','r','s', '\0' };
static unichar_t autohint[] = { 'A', 'u', 't', 'o', 'H', 'i', 'n', 't',  '\0' };

static GMenuItem dummyitem[] = { { new }, NULL };
static GMenuItem fllist[] = {
    { { new, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'N' }, 'N', ksm_control, NULL, NULL, MenuNew },
    { { open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, FVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { openoutline, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'u' }, 'H', ksm_control, NULL, NULL, FVMenuOpenOutline, MID_OpenOutline },
    { { openbitmap, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'B' }, 'J', ksm_control, NULL, NULL, FVMenuOpenBitmap, MID_OpenBitmap },
    { { openmetrics, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'M' }, 'K', ksm_control, NULL, NULL, FVMenuOpenMetrics },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'S' }, 'S', ksm_control, NULL, NULL, FVMenuSave },
    { { saveas, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, FVMenuSaveAs },
    { { generate, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'G' }, 'G', ksm_control|ksm_shift, NULL, NULL, FVMenuGenerate },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { import, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'I' }, 'I', ksm_control|ksm_shift, NULL, NULL, FVMenuImport },
    { { mergekern, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'M' }, 'K', ksm_control|ksm_shift, NULL, NULL, FVMenuMergeKern },
    { { revert, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, FVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { print, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, 'P', ksm_control, NULL, NULL, FVMenuPrint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem cflist[] = {
    { { allfonts, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'A' }, '\0', ksm_control, NULL, NULL, FVMenuCopyFrom, MID_AllFonts },
    { { displayedfont, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'D' }, '\0', ksm_control, NULL, NULL, FVMenuCopyFrom, MID_DisplayedFont },
    { NULL }
};

static GMenuItem edlist[] = {
    { { undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 0, 'U' }, 'Z', ksm_control, NULL, NULL },
    { { redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 0, 'R' }, 'Y', ksm_control, NULL, NULL },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 't' }, 'X', ksm_control, NULL, NULL, FVMenuCut, MID_Cut },
    { { _copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, 'C', ksm_control, NULL, NULL, FVMenuCopy, MID_Copy },
    { { copyref, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'o' }, 'G', ksm_control, NULL, NULL, FVMenuCopyRef, MID_CopyRef },
    { { copywidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'W' }, '\0', ksm_control, NULL, NULL, FVMenuCopyWidth, MID_CopyWidth },
    { { paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, 'V', ksm_control, NULL, NULL, FVMenuPaste, MID_Paste },
    { { clear, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'l' }, 0, 0, NULL, NULL, FVMenuClear, MID_Clear },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { selectall, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'A' }, 'A', ksm_control, NULL, NULL, FVMenuSelectAll },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { unlinkref, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'U' }, '\0', 0, NULL, NULL, FVMenuUnlinkRef, MID_UnlinkRef },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { copyfrom, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'F' }, '\0', 0, cflist, cflistcheck, NULL, 0 },
    { NULL }
};

static GMenuItem ellist[] = {
    { { fontinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'F' }, 'F', ksm_control|ksm_shift, NULL, NULL, FVMenuFontInfo },
    { { privateinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, 'P', ksm_control|ksm_shift, NULL, NULL, FVMenuPrivateInfo },
    { { getinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'I' }, 'I', ksm_control, NULL, NULL, FVMenuCharInfo, MID_CharInfo },
    { { findprobs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'o' }, '\0', ksm_control, NULL, NULL, FVMenuFindProblems, MID_FindProblems },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { bitmapsavail, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'A' }, 'B', ksm_control|ksm_shift, NULL, NULL, FVMenuBitmaps, MID_AvailBitmaps },
    { { regenbitmaps, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'B' }, 'B', ksm_control, NULL, NULL, FVMenuBitmaps, MID_RegenBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { transform, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'T' }, '\\', ksm_control, NULL, NULL, FVMenuTransform, MID_Transform },
    { { stroke, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'E' }, 'E', ksm_control|ksm_shift, NULL, NULL, FVMenuStroke, MID_Stroke },
    { { rmoverlap, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'O' }, 'O', ksm_control|ksm_shift, NULL, NULL, FVMenuOverlap, MID_RmOverlap },
    { { simplify, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'S' }, 'M', ksm_control|ksm_shift, NULL, NULL, FVMenuSimplify, MID_Simplify },
    { { round2int, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'I' }, '_', ksm_control|ksm_shift, NULL, NULL, FVMenuRound2Int, MID_Round },
    { { autotrace, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'r' }, 'T', ksm_control|ksm_shift, NULL, NULL, FVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { correct, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, '\0', 0, NULL, NULL, FVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { buildaccent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'B' }, 'A', ksm_control|ksm_shift, NULL, NULL, FVMenuBuildAccent, MID_BuildAccent },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { merge, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'M' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuMergeFonts, MID_MergeFonts },
    { { interp, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'p' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuInterpFonts, MID_InterpolateFonts },
    { NULL }
};

static GMenuItem vwlist[] = {
    { { next, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'N' }, ']', ksm_control, NULL, NULL, FVMenuChangeChar, MID_Next },
    { { prev, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, '[', ksm_control, NULL, NULL, FVMenuChangeChar, MID_Prev },
    { { _goto, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'G' }, '>', ksm_shift|ksm_control, NULL, NULL, FVMenuGotoChar },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { _24, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, '2' }, '2', ksm_control, NULL, NULL, FVMenuSize, MID_24 },
    { { _36, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, '3' }, '3', ksm_control, NULL, NULL, FVMenuSize, MID_36 },
    { { _48, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, '4' }, '4', ksm_control, NULL, NULL, FVMenuSize, MID_48 },
    { { antialias, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'A' }, '5', ksm_control, NULL, NULL, FVMenuSize, MID_AntiAlias },
    { NULL },			/* Some extra room to show bitmaps */
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }
};

static void vwlistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);
    int i, base;
    BDFFont *bdf;
    char buffer[50];
    extern void GMenuItemArrayFree(GMenuItem *mi);
    extern GMenuItem *GMenuItemArrayCopy(GMenuItem *mi, uint16 *cnt);

    for ( i=0; vwlist[i].ti.text!=antialias; ++i );
    base = i+2;
    for ( i=base; vwlist[i].ti.text!=NULL; ++i ) {
	free( vwlist[i].ti.text);
	vwlist[i].ti.text = NULL;
    }

    vwlist[base-1].ti.fg = vwlist[base-1].ti.bg = COLOR_DEFAULT;
    if ( fv->sf->bitmaps==NULL ) {
	vwlist[base-1].ti.line = false;
    } else {
	vwlist[base-1].ti.line = true;
	for ( bdf = fv->sf->bitmaps, i=base;
		i<sizeof(vwlist)/sizeof(vwlist[0])-1 && bdf!=NULL;
		++i, bdf = bdf->next ) {
	    sprintf( buffer, "%d pixel bitmap", bdf->pixelsize );
	    vwlist[i].ti.text = uc_copy(buffer);
	    vwlist[i].ti.checkable = true;
	    vwlist[i].ti.checked = bdf==fv->show;
	    vwlist[i].ti.userdata = bdf;
	    vwlist[i].invoke = FVMenuShowBitmap;
	    vwlist[i].ti.fg = vwlist[i].ti.bg = COLOR_DEFAULT;
	}
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItemArrayCopy(vwlist,NULL);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Next: case MID_Prev:
	    mi->ti.disabled = anychars<0;
	  break;
	  case MID_24:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==24);
	    mi->ti.disabled = fv->sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case MID_36:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==36);
	    mi->ti.disabled = fv->sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case MID_48:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==48);
	    mi->ti.disabled = fv->sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case MID_AntiAlias:
	    mi->ti.checked = (fv->show!=NULL && fv->show->clut!=NULL);
	    mi->ti.disabled = fv->sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	}
    }
}

static GMenuItem htlist[] = {
    { { autohint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'H' }, 'H', ksm_control|ksm_shift, NULL, NULL, FVMenuAutoHint, MID_AutoHint },
    { NULL }
};

static GMenuItem mtlist[] = {
    { { center, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, '\0', ksm_control, NULL, NULL, FVMenuCenter, MID_Center },
    { { thirds, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'T' }, '\0', ksm_control, NULL, NULL, FVMenuCenter, MID_Thirds },
    { { setwidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'W' }, '\0', ksm_control, NULL, NULL, FVMenuSetWidth, MID_SetWidth },
    { { setlbearing, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'L' }, '\0', 0, NULL, NULL, FVMenuSetWidth, MID_SetLBearing },
    { { setrbearing, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, '\0', 0, NULL, NULL, FVMenuSetWidth, MID_SetRBearing },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { autowidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'A' }, 'W', ksm_control|ksm_shift, NULL, NULL, FVMenuAutoWidth },
    { { autokern, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'K' }, 'K', ksm_control|ksm_shift, NULL, NULL, FVMenuAutoKern },
    { { removekern, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuRemoveKern },
    { NULL }
};

static GMenuItem mblist[] = {
    { { file, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'E' }, 0, 0, edlist, edlistcheck },
    { { element, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'l' }, 0, 0, ellist, ellistcheck },
    { { hints, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'H' }, 0, 0, htlist, htlistcheck },
    { { view, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'V' }, 0, 0, vwlist, vwlistcheck },
    { { metric, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'M' }, 0, 0, mtlist, mtlistcheck },
    { { window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'W' }, 0, 0, NULL, WindowMenuBuild, NULL },
    { NULL }
};

void FVRefreshChar(FontView *fv,BDFFont *bdf,int enc) {
    BDFChar *bdfc = bdf->chars[enc];
    int i, j;
    MetricsView *mv;

    for ( mv=fv->metrics; mv!=NULL; mv=mv->next )
	MVRefreshChar(mv,fv->sf->chars[enc]);
    if ( fv->show != bdf )
return;
    i = enc / fv->colcnt;
    j = enc - i*fv->colcnt;
    i -= fv->rowoff;
    if ( i>=0 && i<fv->rowcnt ) {
	struct _GImage base;
	GImage gi;
	GClut clut;
	GRect old, box;

	memset(&gi,'\0',sizeof(gi));
	memset(&base,'\0',sizeof(base));
	if ( bdfc->byte_data ) {
	    gi.u.image = &base;
	    base.image_type = it_index;
	    base.clut = bdf->clut;
	    base.trans = -1;
	} else {
	    memset(&clut,'\0',sizeof(clut));
	    gi.u.image = &base;
	    base.image_type = it_mono;
	    base.clut = &clut;
	    clut.clut_len = 2;
	    clut.clut[0] = 0xffffff;
	}

	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	box.x = j*fv->cbw+1; box.width = fv->cbw-1;
	box.y = i*fv->cbh+14; box.height = fv->cbw;
	GDrawPushClip(fv->v,&box,&old);
	GDrawFillRect(fv->v,&box,GDrawGetDefaultBackground(NULL));
	GDrawDrawImage(fv->v,&gi,NULL,
		j*fv->cbw+(fv->cbw-1-base.width)/2,
		i*fv->cbh+FV_LAB_HEIGHT+1+fv->show->ascent-bdfc->ymax);
	GDrawPopClip(fv->v,&old);
	if ( fv->selected[enc] ) {
	    GDrawSetXORBase(fv->v,GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(fv->v)));
	    GDrawSetXORMode(fv->v);
	    GDrawFillRect(fv->v,&box,XOR_COLOR);
	    GDrawSetCopyMode(fv->v);
	}
    }
}

void FVRegenChar(FontView *fv,SplineChar *sc) {
    BDFChar *bdfc;
    struct splinecharlist *dlist;
    MetricsView *mv;

    if ( fv->antialias )
	bdfc = SplineCharAntiAlias(sc,fv->filled->pixelsize,4);
    else
	bdfc = SplineCharRasterize(sc,fv->filled->pixelsize);
    BDFCharFree(fv->filled->chars[sc->enc]);
    fv->filled->chars[sc->enc] = bdfc;
    for ( mv=fv->metrics; mv!=NULL; mv=mv->next )
	MVRegenChar(mv,sc);

    FVRefreshChar(fv,fv->filled,sc->enc);

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	FVRegenChar(fv,dlist->sc);
}

SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,int i) {
    extern unsigned short unicode_from_adobestd[256];
    static char namebuf[20];
    Encoding *item=NULL;

    memset(dummy,'\0',sizeof(*dummy));
    dummy->enc = i;
    if ( sf->encoding_name==em_unicode || sf->encoding_name==em_iso8859_1 )
	dummy->unicodeenc = i<65536 ? i : -1;
    else if ( sf->encoding_name==em_adobestandard )
	dummy->unicodeenc = i>=256?-1:unicode_from_adobestd[i];
    else if ( sf->encoding_name==em_none )
	dummy->unicodeenc = -1;
    else if ( sf->encoding_name>=em_base ) {
	dummy->unicodeenc = -1;
	for ( item=enclist; item!=NULL && item->enc_num!=sf->encoding_name; item=item->next );
	if ( item!=NULL && i>=item->char_cnt ) item = NULL;
	if ( item!=NULL )
	    dummy->unicodeenc = item->unicode[i];
    } else if ( sf->encoding_name==em_jis208 && i<96*94 && i%96!=0 && i%96!=95 )
	dummy->unicodeenc = unicode_from_jis208[(i/96)*94+(i%96-1)];
    else if ( sf->encoding_name==em_jis212 && i<96*94 && i%96!=0 && i%96!=95 )
	dummy->unicodeenc = unicode_from_jis212[(i/96)*94+(i%96-1)];
    else if ( sf->encoding_name==em_ksc5601 && i<96*94 && i%96!=0 && i%96!=95 )
	dummy->unicodeenc = unicode_from_ksc5601[(i/96)*94+(i%96-1)];
    else if ( sf->encoding_name==em_gb2312 && i<96*94 && i%96!=0 && i%96!=95 )
	dummy->unicodeenc = unicode_from_gb2312[(i/96)*94+(i%96-1)];
    else if ( sf->encoding_name>=em_jis208 )
	dummy->unicodeenc = -1;
    else
	dummy->unicodeenc = i>=256?-1:unicode_from_alphabets[sf->encoding_name+3][i];
    if ( dummy->unicodeenc==0 && i!=0 )
	dummy->unicodeenc = -1;
    if ( dummy->unicodeenc!=-1 ) {
	if ( dummy->unicodeenc<psunicodenames_cnt )
	    dummy->name = (char *) psunicodenames[dummy->unicodeenc];
	if ( dummy->name==NULL ) {
	    if ( dummy->unicodeenc==0x2d )
		dummy->name = "hyphen-minus";
	    else if ( dummy->unicodeenc==0xad )
		dummy->name = copy("softhyphen");
	    else if ( dummy->unicodeenc==0x00 )
		dummy->name = copy(".notdef");
	    else if ( dummy->unicodeenc==0xa0 )
		dummy->name = "nonbreakingspace";
	    else {
		sprintf( namebuf, "uni%04X", i);
		dummy->name = namebuf;
	    }
	}
    } else if ( item!=NULL && item->psnames!=NULL )
	dummy->name = item->psnames[i];
    if ( dummy->name==NULL )
	dummy->name = ".notdef";
    dummy->width = sf->ascent+sf->descent;
    dummy->parent = sf;
return( dummy );
}

Ligature *SCLigDefault(SplineChar *sc) {
    const unichar_t *alt, *pt;
    char *componants;
    int len;
    Ligature *lig;
    const unichar_t *uname;

    /* If it's not unicode we have no info on it */
    if ( sc->unicodeenc==-1 )
return( NULL );
    if ( !isdecompositionnormative(sc->unicodeenc) ||
	    unicode_alternates[sc->unicodeenc>>8]==NULL ||
	    (alt = unicode_alternates[sc->unicodeenc>>8][sc->unicodeenc&0xff])==NULL )
return( NULL );
    for ( pt=alt; *pt; ++pt )
	if ( iscombining(*pt))
return( NULL );			/* Don't treat accented letters as ligatures */
    if ( alt[1]=='\0' )
return( NULL );			/* Single replacements aren't ligatures */
    if ( UnicodeCharacterNames[sc->unicodeenc>>8]!=NULL &&
	    (uname = UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff])!=NULL &&
	    uc_strstr(uname,"LIGATURE")==NULL )
return( NULL );
	    

    if ( sc->unicodeenc==0xfb03 )
	componants = copy("f f i; ff i");
    else if ( sc->unicodeenc==0xfb04 )
	componants = copy("f f l; ff l");
    else {
	componants=NULL;
	while ( 1 ) {
	    len = 0;
	    for ( pt=alt; *pt; ++pt ) {
		if ( psunicodenames[*pt]!=NULL ) {
		    if ( componants!=NULL )
			strcpy(componants+len,psunicodenames[*pt]);
		    len += strlen( psunicodenames[*pt])+1;
		    if ( componants!=NULL )
			componants[len-1] = ' ';
		} else {
		    if ( componants!=NULL )
			sprintf(componants+len, "uni%04X ", *pt );
		    len += 8;
		}
	    }
	    if ( componants!=NULL )
	break;
	    componants = galloc(len+1);
	}
	componants[len-1] = '\0';
    }

    lig = gcalloc(1,sizeof(Ligature));
    lig->lig = sc;
    lig->componants = componants;
return( lig );
}

SplineChar *SFMakeChar(SplineFont *sf,int i) {
    SplineChar dummy, *sc;

    if ( (sc = sf->chars[i])==NULL ) {
	SCBuildDummy(&dummy,sf,i);
	sf->chars[i] = sc = galloc(sizeof(SplineChar));
	*sc = dummy;
	sc->name = copy(sc->name);
	sc->lig = SCLigDefault(sc);
	sc->parent = sf;
    }
return( sc );
}

static void FVExpose(FontView *fv,GWindow pixmap,GEvent *event) {
    int i, j, width;
    int changed;
    GRect old, old2, box;
    GClut clut;
    struct _GImage base;
    GImage gi;
    SplineChar dummy;

    memset(&gi,'\0',sizeof(gi));
    memset(&base,'\0',sizeof(base));
    if ( fv->show->clut!=NULL ) {
	gi.u.image = &base;
	base.image_type = it_index;
	base.clut = fv->show->clut;
	base.trans = -1;
    } else {
	memset(&clut,'\0',sizeof(clut));
	gi.u.image = &base;
	base.image_type = it_mono;
	base.clut = &clut;
	clut.clut_len = 2;
	clut.clut[0] = 0xffffff;
    }

    GDrawSetFont(pixmap,fv->header);
    GDrawPushClip(pixmap,&event->u.expose.rect,&old);
    GDrawFillRect(pixmap,NULL,GDrawGetDefaultBackground(NULL));
    for ( i=0; i<=fv->rowcnt; ++i ) {
	GDrawDrawLine(pixmap,0,i*fv->cbh,fv->width,i*fv->cbh,0);
	GDrawDrawLine(pixmap,0,i*fv->cbh+FV_LAB_HEIGHT,fv->width,i*fv->cbh+FV_LAB_HEIGHT,0x808080);
    }
    for ( i=0; i<=fv->colcnt; ++i )
	GDrawDrawLine(pixmap,i*fv->cbw,0,i*fv->cbw,fv->height,0);
    for ( i=event->u.expose.rect.y/fv->cbh; i<=fv->rowcnt &&
	    (event->u.expose.rect.y+event->u.expose.rect.height+fv->cbh-1)/fv->cbh; ++i ) for ( j=0; j<fv->colcnt; ++j ) {
	if ( (i+fv->rowoff)*fv->colcnt+j < fv->sf->charcnt ) {
	    int index = (i+fv->rowoff)*fv->colcnt+j;
	    SplineChar *sc = fv->sf->chars[index];
	    unichar_t buf[2];
	    Color fg = 0;
	    BDFChar *bdfc;
	    if ( sc==NULL )
		sc = SCBuildDummy(&dummy,fv->sf,index);
	    if ( sc->unicodeenc==0xad )
		buf[0] = '-';
	    else if ( sc->unicodeenc!=-1 )
		buf[0] = sc->unicodeenc;
	    else {
		buf[0] = '?';
		fg = 0xff0000;
	    }
	    width = GDrawGetTextWidth(pixmap,buf,1,NULL);
	    if ( sc->unicodeenc<0x80 || sc->unicodeenc>=0xa0 )
		GDrawDrawText(pixmap,j*fv->cbw+(fv->cbw-1-width)/2,i*fv->cbh+FV_LAB_HEIGHT-2,buf,1,NULL,fg);
	    changed = sc->changed;
	    if ( fv->sf->onlybitmaps )
		changed = fv->show->chars[index]==NULL? false : fv->show->chars[index]->changed;
	    if ( changed ) {
		GRect r;
		r.x = j*fv->cbw+1; r.width = fv->cbw;
		r.y = i*fv->cbh-1; r.height = FV_LAB_HEIGHT+1;
		GDrawSetXORBase(pixmap,GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(fv->v)));
		GDrawSetXORMode(pixmap);
		GDrawFillRect(pixmap,&r,0x000000);
		GDrawSetCopyMode(pixmap);
	    }

	    if ( fv->show!=NULL && fv->show->chars[index]==NULL &&
		    SCWorthOutputting(fv->sf->chars[index]) ) {
		/* If we have an outline but no bitmap for this slot */
		box.x = j*fv->cbw+1; box.width = fv->cbw-2;
		box.y = i*fv->cbh+14+2; box.height = box.width+1;
		GDrawDrawRect(pixmap,&box,0xff0000);
		++box.x; ++box.y; box.width -= 2; box.height -= 2;
		GDrawDrawRect(pixmap,&box,0xff0000);
	    } else if ( fv->show!=NULL && fv->show->chars[index]!=NULL ) {
		bdfc = fv->show->chars[index];
		base.data = bdfc->bitmap;
		base.bytes_per_line = bdfc->bytes_per_line;
		base.width = bdfc->xmax-bdfc->xmin+1;
		base.height = bdfc->ymax-bdfc->ymin+1;
		box.x = j*fv->cbw; box.width = fv->cbw;
		box.y = i*fv->cbh+14+1; box.height = box.width+1;
		GDrawPushClip(pixmap,&box,&old2);
		if ( !fv->sf->onlybitmaps &&
			sc->splines==NULL && sc->refs==NULL && !sc->widthset &&
			!(bdfc->xmax<=0 && bdfc->xmin==0 && bdfc->ymax<=0 && bdfc->ymax==0) ) {
		    /* If we have a bitmap but no outline character... */
		    GRect b;
		    b.x = box.x+1; b.y = box.y+1; b.width = box.width-2; b.height = box.height-2;
		    GDrawDrawRect(pixmap,&b,0x008000);
		    ++b.x; ++b.y; b.width -= 2; b.height -= 2;
		    GDrawDrawRect(pixmap,&b,0x008000);
		}
		GDrawDrawImage(pixmap,&gi,NULL,
			j*fv->cbw+(fv->cbw-1-base.width)/2,
			i*fv->cbh+FV_LAB_HEIGHT+1+fv->show->ascent-bdfc->ymax);
		GDrawPopClip(pixmap,&old2);
	    }
	    if ( fv->selected[index] ) {
		box.x = j*fv->cbw+1; box.width = fv->cbw-1;
		box.y = i*fv->cbh+FV_LAB_HEIGHT+1; box.height = fv->cbw;
		GDrawSetXORMode(pixmap);
		GDrawSetXORBase(pixmap,GDrawGetDefaultBackground(NULL));
		GDrawFillRect(pixmap,&box,XOR_COLOR);
		GDrawSetCopyMode(pixmap);
	    }
	}
    }
    GDrawPopClip(pixmap,&old);
}

static void FVDrawInfo(FontView *fv,GWindow pixmap,GEvent *event) {
    GRect old, r;
    char buffer[200];
    unichar_t ubuffer[200];
    Color bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap));
    SplineChar *sc, dummy;

    if ( event->u.expose.rect.y+event->u.expose.rect.height<=fv->mbh )
return;

    GDrawSetFont(pixmap,fv->header);
    GDrawPushClip(pixmap,&event->u.expose.rect,&old);

    r.x = 0; r.width = fv->width; r.y = fv->mbh; r.height = fv->infoh;
    GDrawFillRect(pixmap,&r,bg);
    if ( fv->end_pos>=fv->sf->charcnt || fv->pressed_pos>=fv->sf->charcnt )
	fv->end_pos = fv->pressed_pos = -1;	/* Can happen after reencoding */
    if ( fv->end_pos == -1 )
return;

    sprintf( buffer, "%-5d ", fv->end_pos );
    sc = fv->sf->chars[fv->end_pos];
    if ( sc==NULL )
	sc = SCBuildDummy(&dummy,fv->sf,fv->end_pos);
    if ( sc->unicodeenc!=-1 )
	sprintf( buffer+strlen(buffer), "U+%04X", sc->unicodeenc );
    else
	sprintf( buffer+strlen(buffer), "U+????" );
    sprintf( buffer+strlen(buffer), "  %.80s", sc->name );

    uc_strcpy(ubuffer,buffer);
    if ( sc->unicodeenc!=-1 && UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]!=NULL ) {
	uc_strcat(ubuffer, "  ");
	u_strncat(ubuffer, UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff], 80);
    }

    GDrawDrawText(pixmap,10,fv->mbh+11,ubuffer,-1,NULL,0xff0000);
    GDrawPopClip(pixmap,&old);
}

static void FVShowInfo(FontView *fv) {
    GRect r;
    r.x = 0; r.width = fv->width; r.y = fv->mbh; r.height = fv->infoh;
    GDrawRequestExpose(fv->gw,&r,false);
}

static void FVChar(FontView *fv,GEvent *event) {
    int i,pos;

    if ( event->u.chr.keysym == GK_Left ||
	    event->u.chr.keysym == GK_Up ||
	    event->u.chr.keysym == GK_Right ||
	    event->u.chr.keysym == GK_Down ||
	    event->u.chr.keysym == GK_KP_Left ||
	    event->u.chr.keysym == GK_KP_Up ||
	    event->u.chr.keysym == GK_KP_Right ||
	    event->u.chr.keysym == GK_KP_Down ||
	    event->u.chr.keysym == GK_Home ||
	    event->u.chr.keysym == GK_KP_Home ||
	    event->u.chr.keysym == GK_End ||
	    event->u.chr.keysym == GK_KP_End ) {
	switch ( event->u.chr.keysym ) {
	  case GK_Left: case GK_KP_Left:
	    pos = fv->end_pos-1;
	  break;
	  case GK_Right: case GK_KP_Right:
	    pos = fv->end_pos+1;
	  break;
	  case GK_Up: case GK_KP_Up:
	    pos = fv->end_pos-fv->colcnt;
	  break;
	  case GK_Down: case GK_KP_Down:
	    pos = fv->end_pos+fv->colcnt;
	  break;
	  case GK_End: case GK_KP_End:
	    pos = fv->sf->charcnt;
	  break;
	  case GK_Home: case GK_KP_Home:
	    pos = 0;
	    for ( i=0; i<fv->sf->charcnt; ++i )
		if ( fv->sf->chars[i]!=NULL &&
			(fv->sf->chars[i]->unicodeenc=='A' ||
			 fv->sf->chars[i]->splines!=NULL ||
			 fv->sf->chars[i]->refs!=NULL ||
			 fv->sf->chars[i]->widthset )) {
		     pos = i;
	     break;
		 }
	  break;
	}
	if ( pos<0 ) pos = 0;
	if ( pos>=fv->sf->charcnt ) pos = fv->sf->charcnt-1;
	if ( event->u.chr.state&ksm_shift ) {
	    FVReselect(fv,pos);
	} else {
	    FVDeselectAll(fv);
	    fv->selected[pos] = true;
	    FVToggleCharSelected(fv,pos);
	    fv->pressed_pos = pos;
	}
	fv->end_pos = pos;
	FVShowInfo(fv);
	FVScrollToChar(fv,pos);
    } else if ( event->u.chr.chars[0]=='\0' || event->u.chr.chars[1]!='\0' ) {
	/* Do Nothing */;
    } else {
	if ( fv->sf->encoding_name==em_unicode || fv->sf->encoding_name==em_iso8859_1 ) {
	    if ( event->u.chr.chars[0]<fv->sf->charcnt ) {
		i = event->u.chr.chars[0];
		SFMakeChar(fv->sf,i);
	    } else
		i = -1;
	} else {
	    for ( i=fv->sf->charcnt-1; i>=0; --i ) if ( fv->sf->chars[i]!=NULL ) {
		if ( fv->sf->chars[i]->unicodeenc==event->u.chr.chars[0] )
	    break;
	    }
	}
	FVChangeChar(fv,i);
    }
}

void SCPreparePopup(GWindow gw,SplineChar *sc) {
    static unichar_t space[200];
    char cspace[40];
    int upos;

    if ( sc->unicodeenc!=-1 )
	upos = sc->unicodeenc;
    else if ( sc->enc<32 || (sc->enc>=127 && sc->enc<160) )
	upos = sc->enc;
    else {
	uc_strncpy(space,sc->name,sizeof(space)/sizeof(unichar_t)-2);
	GGadgetPreparePopup(gw,space);
return;
    }
    if ( UnicodeCharacterNames[upos>>8][upos&0xff]!=NULL ) {
	sprintf( cspace, "%04x \"%.20s\" ", upos, sc->name==NULL?"":sc->name );
	uc_strcpy(space,cspace);
	u_strcat(space,UnicodeCharacterNames[upos>>8][upos&0xff]);
    } else {
	sprintf( cspace, "%04x \"%.20s\" %s", upos, sc->name==NULL?"":sc->name,
	    upos=='\0'				? "Null":
	    upos=='\t'				? "Tab":
	    upos=='\r'				? "Return":
	    upos=='\n'				? "LineFeed":
	    upos=='\f'				? "FormFeed":
	    upos=='\33'				? "Escape":
	    upos<160				? "Control Char":
	    upos>=0x3400 && upos<=0x4db5	? "CJK Ideograph Extension A":
	    upos>=0x4E00 && upos<=0x9FA5	? "CJK Ideograph":
	    upos>=0xAC00 && upos<=0xD7A3	? "Hangul Syllable":
	    upos>=0xD800 && upos<=0xDB7F	? "Non Private Use High Surrogate":
	    upos>=0xDB80 && upos<=0xDBFF	? "Private Use High Surrogate":
	    upos>=0xDC00 && upos<=0xDFFF	? "Low Surrogate":
	    upos>=0xE000 && upos<=0xF8FF	? "Private Use":
					      "Unencoded Unicode" );
	uc_strcpy(space,cspace);
    }
    GGadgetPreparePopup(gw,space);
}

static void FVMouse(FontView *fv,GEvent *event) {
    int pos = (event->u.mouse.y/fv->cbh + fv->rowoff)*fv->colcnt + event->u.mouse.x/fv->cbw;
    SplineChar *sc, dummy;

    GGadgetEndPopup();
    if ( event->type==et_mousedown )
	CVPaletteDeactivate();
    if ( pos<0 )
	pos = 0;
    else if ( pos>=fv->sf->charcnt )
	pos = fv->sf->charcnt-1;

    sc = fv->sf->chars[pos];
    if ( sc==NULL )
	sc = SCBuildDummy(&dummy,fv->sf,pos);
    if ( event->type == et_mouseup && event->u.mouse.clicks==2 ) {
	if ( sc==&dummy ) {
	    fv->sf->chars[pos] = sc = galloc(sizeof(SplineChar));
	    *sc = dummy;
	    sc->name = copy(sc->name);
	    sc->parent = fv->sf;
	}
	if ( fv->show==fv->filled ) {
	    CharViewCreate(sc,fv);
	} else {
	    BDFFont *bdf = fv->show;
	    if ( bdf->chars[pos]==NULL ) {
		if ( fv->antialias )
		    bdf->chars[pos] = SplineCharAntiAlias(sc,bdf->pixelsize,4);
		else
		    bdf->chars[pos] = SplineCharRasterize(sc,bdf->pixelsize);
	    }
	    BitmapViewCreate(bdf->chars[pos],bdf,fv);
	}
    } else if ( event->type == et_mousemove ) {
	SCPreparePopup(fv->v,sc);
    }
    if ( event->type == et_mousedown ) {
	if ( !(event->u.mouse.state&ksm_shift) /*&&
		(fv->sf->chars[pos]==NULL || !fv->selected[pos] )*/)
	    FVDeselectAll(fv);
	if ( fv->pressed!=NULL )
	    GDrawCancelTimer(fv->pressed);
	fv->pressed = GDrawRequestTimer(fv->v,200,100,NULL);
	fv->pressed_pos = fv->end_pos = pos;
	FVShowInfo(fv);
	if ( event->u.mouse.state&ksm_shift ) {
	    fv->selected[pos] = !fv->selected[pos];
	    FVToggleCharSelected(fv,pos);
	} else if ( !fv->selected[pos] ) {
	    fv->selected[pos] = true;
	    FVToggleCharSelected(fv,pos);
	}
    } else if ( fv->pressed!=NULL ) {
	int showit = pos!=fv->end_pos;
	FVReselect(fv,pos);
	if ( showit )
	    FVShowInfo(fv);
	if ( event->type==et_mouseup ) {
	    GDrawCancelTimer(fv->pressed);
	    fv->pressed = NULL;
	}
    }
    if ( event->type==et_mouseup )
	SCPreparePopup(fv->v,sc);
}

static void FVTimer(FontView *fv,GEvent *event) {

    if ( event->u.timer.timer==fv->pressed ) {
	GEvent e;
	GDrawGetPointerPosition(fv->v,&e);
	if ( e.u.mouse.y<0 || e.u.mouse.y >= fv->height ) {
	    double dy = 0;
	    if ( e.u.mouse.y<0 )
		dy = -1;
	    else if ( e.u.mouse.y>=fv->height )
		dy = 1;
	    if ( fv->rowoff+dy<0 )
		dy = 0;
	    else if ( fv->rowoff+dy+fv->rowcnt > fv->rowltot )
		dy = 0;
	    fv->rowoff += dy;
	    if ( dy!=0 ) {
		GScrollBarSetPos(fv->vsb,fv->rowoff);
		GDrawScroll(fv->v,NULL,0,dy*fv->cbh);
	    }
	}
    } else if ( event->u.timer.userdata!=NULL ) {
	/* It's a delayed function call */
	void (*func)(FontView *) = (void (*)(FontView *)) (event->u.timer.userdata);
	func(fv);
    }
}

void FVDelay(FontView *fv,void (*func)(FontView *)) {
    GDrawRequestTimer(fv->v,100,0,(void *) func);
}

static void FVScroll(FontView *fv,struct sbevent *sb) {
    int newpos = fv->rowoff;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= fv->rowcnt;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += fv->rowcnt;
      break;
      case et_sb_bottom:
        newpos = fv->rowltot-fv->rowcnt;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>fv->rowltot-fv->rowcnt )
        newpos = fv->rowltot-fv->rowcnt;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=fv->rowoff ) {
	int diff = newpos-fv->rowoff;
	fv->rowoff = newpos;
	GScrollBarSetPos(fv->vsb,fv->rowoff);
	GDrawScroll(fv->v,NULL,0,diff*fv->cbh);
    }
}

static int v_e_h(GWindow gw, GEvent *event) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	FVExpose(fv,gw,event);
      break;
      case et_char:
	FVChar(fv,event);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	FVMouse(fv,event);
      break;
      case et_timer:
	FVTimer(fv,event);
      break;
      case et_focus:
#if 0
	if ( event->u.focus.gained_focus )
	    CVPaletteDeactivate();
#endif
      break;
    }
return( true );
}

static void FVResize(FontView *fv,GEvent *event) {
    GRect pos;
    int topchar;

    if ( fv->colcnt!=0 )
	topchar = fv->rowoff*fv->colcnt;
    else if ( fv->sf->encoding_name>=em_jis208 && fv->sf->encoding_name<=em_gb2312 )
	topchar = 1;
    else
	topchar = 'A';
    if ( (event->u.resize.size.width-
		GDrawPointsToPixels(fv->gw,_GScrollBar_Width)-1)%fv->cbw!=0 ||
	    (event->u.resize.size.height-fv->mbh-fv->infoh-1)%fv->cbh!=0 ) {
	int cc = (event->u.resize.size.width+fv->cbw/2-
		GDrawPointsToPixels(fv->gw,_GScrollBar_Width)-1)/fv->cbw;
	int rc = (event->u.resize.size.height-fv->mbh-fv->infoh-1)/fv->cbh;
	if ( cc<=0 ) cc = 1;
	if ( rc<=0 ) rc = 1;
	GDrawResize(fv->gw,
		cc*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
		rc*fv->cbh+1+fv->mbh+fv->infoh);
return;
    }

    pos.width = GDrawPointsToPixels(fv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-fv->mbh-fv->infoh;
    pos.x = event->u.resize.size.width-pos.width; pos.y = fv->mbh+fv->infoh;
    GGadgetResize(fv->vsb,pos.width,pos.height);
    GGadgetMove(fv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(fv->v,pos.width,pos.height);

    fv->width = pos.width; fv->height = pos.height;
    fv->colcnt = (fv->width-1)/fv->cbw;
    if ( fv->colcnt<1 ) fv->colcnt = 1;
    fv->rowcnt = (fv->height-1)/fv->cbh;
    if ( fv->rowcnt<1 ) fv->rowcnt = 1;
    fv->rowltot = (fv->sf->charcnt+fv->colcnt-1)/fv->colcnt;

    GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
    fv->rowoff = topchar/fv->colcnt;
    if ( fv->rowoff>=fv->rowltot-fv->rowcnt )
        fv->rowoff = fv->rowltot-fv->rowcnt-1;
    if ( fv->rowoff<0 ) fv->rowoff =0;
    GScrollBarSetPos(fv->vsb,fv->rowoff);
    GDrawRequestExpose(fv->gw,NULL,true);
    GDrawRequestExpose(fv->v,NULL,true);
}

void FontViewReformat(FontView *fv) {
    BDFFont *new;

    fv->rowltot = (fv->sf->charcnt+fv->colcnt-1)/fv->colcnt;
    GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
    if ( fv->rowoff>=fv->rowltot-fv->rowcnt ) {
        fv->rowoff = fv->rowltot-fv->rowcnt-1;
	if ( fv->rowoff<0 ) fv->rowoff =0;
	GScrollBarSetPos(fv->vsb,fv->rowoff);
    }
    if ( fv->antialias )
	new = SplineFontAntiAlias(fv->sf,fv->filled->pixelsize,4);
    else
	new = SplineFontRasterize(fv->sf,fv->filled->pixelsize,true);
    BDFFontFree(fv->filled);
    if ( fv->filled == fv->show )
	fv->show = new;
    fv->filled = new;
    GDrawRequestExpose(fv->v,NULL,false);
}

static int fv_e_h(GWindow gw, GEvent *event) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	FVDrawInfo(fv,gw,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    FVResize(fv,event);
      break;
      case et_char:
	FVChar(fv,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    FVScroll(fv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	FVMenuClose(gw,NULL,NULL);
      break;
      case et_create:
	fv->next = fv_list;
	fv_list = fv;
      break;
      case et_destroy:
	if ( fv_list==fv )
	    fv_list = fv->next;
	else {
	    FontView *n;
	    for ( n=fv_list; n->next!=fv; n=n->next );
	    n->next = fv->next;
	}
	FontViewFree(fv);
      break;
    }
return( true );
}

FontView *FontViewCreate(SplineFont *sf) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    GGadget *sb;
    GRect gsize;
    FontView *fv = calloc(1,sizeof(FontView));
    FontRequest rq;
    /* sadly, clearlyu is too big for the space I've got */
    /*static unichar_t clearly_monospace[] = { 'c','l','e','a','r','l','y','u',',',' ','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',  '\0' };*/
    static unichar_t monospace[] = { 'm', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    static GWindow icon = NULL;
    BDFFont *bdf;

    if ( icon==NULL )
#ifdef BIGICONS
	icon = GDrawCreateBitmap(NULL,fontview_width,fontview_height,fontview_bits);
#else
	icon = GDrawCreateBitmap(NULL,fontview2_width,fontview2_height,fontview2_bits);
#endif

    fv->sf = sf;
    fv->selected = gcalloc(sf->charcnt,sizeof(char));
    sf->fv = fv;
    fv->cbw = default_fv_font_size+1;
    fv->cbh = default_fv_font_size+1+FV_LAB_HEIGHT+1;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_pointer;
    wattrs.window_title = uc_copy(sf->fontname);
    wattrs.icon = icon;
    pos.x = pos.y = 0;
    pos.width = 16*fv->cbw+1;
    pos.height = 4*fv->cbh+1;
    fv->gw = gw = GDrawCreateTopWindow(NULL,&pos,fv_e_h,fv,&wattrs);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    fv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(fv->mb,&gsize);
    fv->mbh = gsize.height;
    fv->infoh = 14/*GDrawPointsToPixels(fv->gw,14)*/;
    fv->end_pos = -1;

    gd.pos.y = fv->mbh+fv->infoh; gd.pos.height = pos.height;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    fv->vsb = sb = GScrollBarCreate(gw,&gd,fv);

    wattrs.mask = wam_events|wam_cursor;
    pos.x = 0; pos.y = fv->mbh+fv->infoh;
    fv->v = GWidgetCreateSubWindow(gw,&pos,v_e_h,fv,&wattrs);
    GDrawSetVisible(fv->v,true);

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -13;
    rq.weight = 400;
    fv->header = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    fv->antialias = sf->display_antialias;
    GDrawSetFont(fv->v,fv->header);
    if ( fv->antialias )
	bdf = SplineFontAntiAlias(fv->sf,
		sf->display_size<0?-sf->display_size:default_fv_font_size,4);
    else
	bdf = SplineFontRasterize(fv->sf,
		sf->display_size<0?-sf->display_size:default_fv_font_size,true);
    fv->filled = bdf;
    if ( sf->display_size>0 ) {
	for ( bdf=sf->bitmaps; bdf!=NULL && bdf->pixelsize!=sf->display_size ;
		bdf=bdf->next );
	if ( bdf==NULL )
	    bdf = fv->filled;
    }
    FVChangeDisplayFont(fv,bdf);
    GMenuBarSetItemChecked(fv->mb,default_fv_font_size==24?MID_24:
				  default_fv_font_size==36?MID_36:
				  MID_48,true);

    /*GWidgetHidePalettes();*/
    GDrawSetVisible(gw,true);
return( fv );
}

/* This does not check currently existing fontviews, and should only be used */
/*  by LoadSplineFont (which does) and by RevertFile (which knows what it's doing) */
SplineFont *ReadSplineFont(char *filename) {
    FontDict *fd;
    SplineFont *sf;
    static unichar_t loading[] = { 'L','o','a','d','i','n','g','.','.','.',  '\0' };
    static unichar_t reading[] = { 'R','e','a','d','i','n','g',' ','G','l','y','p','h','s',  '\0' };
    static unichar_t interpret[] = { 'I','n','t','e','p','r','e','t','i','n','g',' ','G','l','y','p','h','s',  '\0' };
    unichar_t ubuf[120];
    char buf[120];

    if ( filename==NULL )
return( NULL );

    sprintf(buf, "Loading font from %.100s", GFileNameTail(filename));
    uc_strcpy(ubuf,buf);
    GProgressStartIndicator(10,loading,ubuf,reading,0,1);
    GProgressEnableStop(0);

    sf = NULL;
    if ( strmatch(filename+strlen(filename)-4, ".sfd")==0 ) {
	sf = SFDRead(filename);
    } else if ( strmatch(filename+strlen(filename)-4, ".ttf")==0 ||
		strmatch(filename+strlen(filename)-4, ".otf")==0 ) {
	sf = SFReadTTF(filename);
    } else if ( strmatch(filename+strlen(filename)-4, ".bdf")==0 ) {
	sf = SplineFontNew();
	SFImportBDF(sf,filename);
	sf->changed = false;
    } else {
	GProgressChangeStages(2);
	fd = ReadPSFont(filename);
	GProgressNextStage();
	GProgressChangeLine2(interpret);
	if ( fd!=NULL ) {
	    sf = SplineFontFromPSFont(fd);
	    PSFontFree(fd);
	    if ( sf!=NULL )
		CheckAfmOfPostscript(sf,filename);
	}
    }
    GProgressEndIndicator();

    if ( sf==NULL )
	GDrawError("Couldn't open font (or in bad format): %s", filename );
    else
	sf->origname = copy(filename);
return( sf );
}

SplineFont *LoadSplineFont(char *filename) {
    FontView *fv;
    SplineFont *sf;
    char *pt, *ept;
    static char *extens[] = { ".sfd", ".pfa", ".pfb", ".ttf", ".otf", ".ps", ".PFA", ".PFB", ".TTF", ".OTF", NULL };
    int i;

    if ( filename==NULL )
return( NULL );

    if (( pt = strrchr(filename,'/'))==NULL ) pt = filename;
    if ( strchr(pt,'.')==NULL ) {
	pt = galloc(strlen(filename)+8);
	strcpy(pt,filename);
	ept = pt+strlen(pt);
	for ( i=0; extens[i]!=NULL; ++i ) {
	    strcpy(ept,extens[i]);
	    if ( GFileExists(pt))
	break;
	}
	if ( extens[i]!=NULL )
	    filename = pt;
	else {
	    free(pt);
	    pt = NULL;
	}
    } else
	pt = NULL;

    sf = NULL;
    /* Only one view per font */
    for ( fv=fv_list; fv!=NULL && sf==NULL; fv=fv->next ) {
	if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,filename)==0 )
	    sf = fv->sf;
	else if ( fv->sf->origname!=NULL && strcmp(fv->sf->origname,filename)==0 )
	    sf = fv->sf;
    }

    if ( sf==NULL )
	sf = ReadSplineFont(filename);

    if ( pt==filename )
	free(filename);
return( sf );
}

FontView *ViewPostscriptFont(char *filename) {
    SplineFont *sf = LoadSplineFont(filename);
    if ( sf==NULL )
return( NULL );
    if ( sf->fv!=NULL ) {
	GDrawSetVisible(sf->fv->gw,true);
	GDrawRaise(sf->fv->gw);
return( sf->fv );
    }
return( FontViewCreate(sf));
}

FontView *FontNew(void) {
return( FontViewCreate(SplineFontNew()));
}

void FontViewFree(FontView *fv) {
    SplineFontFree(fv->sf);
    BDFFontFree(fv->filled);
    free(fv->selected);
    free(fv);
}

