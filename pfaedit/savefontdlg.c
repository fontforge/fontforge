/* Copyright (C) 2000-2002 by George Williams */
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
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "ustring.h"
#include "gfile.h"
#include "gio.h"
#include "gicons.h"
#include "psfont.h"

struct gfc_data {
    int done;
    int ret;
    GGadget *gfc;
    GGadget *pstype;
    GGadget *bmptype;
    GGadget *bmpsizes;
    GGadget *doafm;
    GGadget *dopfm;
    SplineFont *sf;
};

#if __Mac
static char *extensions[] = { ".pfa", ".pfb", "", ".ps", ".ps", ".cid",
	".ttf", ".ttf", ".suit", ".dfont", ".otf", ".otf.dfont", ".otf",
	".otf.dfont", NULL };
#else
static char *extensions[] = { ".pfa", ".pfb", ".bin", ".ps", ".ps", ".cid",
	".ttf", ".ttf", ".ttf.bin", ".dfont", ".otf", ".otf.dfont", ".otf",
	".otf.dfont", NULL };
#endif
static GTextInfo formattypes[] = {
    { (unichar_t *) "PS Type 1 (Ascii)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Type 1 (Binary)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#if __Mac
    { (unichar_t *) "PS Type 1 (Resource)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#else
    { (unichar_t *) "PS Type 1 (MacBin)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#endif
    { (unichar_t *) "PS Type 3", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Type 0", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS CID", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "True Type", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "True Type (Symbol)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#if __Mac
    { (unichar_t *) "True Type (Resource)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#else
    { (unichar_t *) "True Type (MacBin)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#endif
    { (unichar_t *) "True Type (Mac dfont)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Open Type", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Open Type (Mac dfont)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Open Type CID", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Open Type CID (dfont)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) _STR_Nooutlinefont, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }
};
static GTextInfo bitmaptypes[] = {
    { (unichar_t *) "BDF", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "In TTF (MS)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "In TTF (Apple)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "sbits only (dfont)", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
#if __Mac
    { (unichar_t *) "NFNT (Resource)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#else
    { (unichar_t *) "NFNT (MacBin)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#endif
    { (unichar_t *) "NFNT (dfont)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) _STR_Nobitmapfonts, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }
};

static int oldafmstate = -1, oldpfmstate = false;
int oldformatstate = ff_pfb;
int oldbitmapstate = 0;

static int32 *ParseBitmapSizes(GGadget *g,int *err) {
    const unichar_t *val = _GGadgetGetTitle(g), *pt; unichar_t *end, *end2;
    int i;
    int32 *sizes;

    *err = false;
    end2 = NULL;
    for ( i=1, pt = val; (end = u_strchr(pt,',')) || (end2=u_strchr(pt,' ')); ++i ) {
	if ( end!=NULL && end2!=NULL ) {
	    if ( end2<end ) end = end2;
	} else if ( end2!=NULL )
	    end = end2;
	pt = end+1;
	end2 = NULL;
    }
    sizes = galloc((i+1)*sizeof(int32));

    for ( i=0, pt = val; *pt!='\0' ; ) {
	sizes[i]=rint(u_strtod(pt,&end));
	if ( *end!='@' )
	    sizes[i] |= 0x10000;
	else
	    sizes[i] |= (u_strtol(end+1,&end,10)<<16);
	if ( sizes[i]>0 ) ++i;
	if ( *end!=' ' && *end!=',' && *end!='\0' ) {
	    free(sizes);
	    ProtestR(_STR_PixelList);
	    *err = true;
return( NULL );
	}
	while ( *end==' ' || *end==',' ) ++end;
	pt = end;
    }
    sizes[i] = 0;
return( sizes );
}

static int WriteAfmFile(char *filename,SplineFont *sf, int formattype) {
    char *buf = galloc(strlen(filename)+6), *pt, *pt2;
    FILE *afm;
    int ret;
    unichar_t *temp;

    strcpy(buf,filename);
    pt = strrchr(buf,'.');
    if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	pt = NULL;
    if ( pt==NULL )
	strcat(buf,".afm");
    else
	strcpy(pt,".afm");
    GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
    afm = fopen(buf,"w");
    if ( afm==NULL )
return( false );
    ret = AfmSplineFont(afm,sf,formattype);
    if ( fclose(afm)==-1 )
return( 0 );
return( ret );
}

static int WritePfmFile(char *filename,SplineFont *sf, int type0) {
    char *buf = galloc(strlen(filename)+6), *pt, *pt2;
    FILE *pfm;
    int ret;
    unichar_t *temp;

    strcpy(buf,filename);
    pt = strrchr(buf,'.');
    if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	pt = NULL;
    if ( pt==NULL )
	strcat(buf,".pfm");
    else
	strcpy(pt,".pfm");
    GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
    pfm = fopen(buf,"w");
    if ( pfm==NULL )
return( false );
    ret = PfmSplineFont(pfm,sf,type0);
    if ( fclose(pfm)==-1 )
return( 0 );
return( ret );
}

#if 0
static int ad_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	int *done = GDrawGetUserData(gw);
	*done = -1;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	int *done = GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)==1001 )
	    *done = true;
	else
	    *done = -1;
    }
return( true );
}

static int AskDepth() {
    GRect pos;
    static GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9];
    GTextInfo label[9];
    int done=false;

    if ( gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_GreymapDepth,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
	pos.height = GDrawPointsToPixels(NULL,75);
	gw = GDrawCreateTopWindow(NULL,&pos,ad_e_h,&done,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) _STR_GreymapDepth;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 7; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	label[1].text = (unichar_t *) "2";
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 20; gcd[1].gd.pos.y = 13+7;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = 2;
	gcd[1].creator = GRadioCreate;

	label[2].text = (unichar_t *) "4";
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 50; gcd[2].gd.pos.y = gcd[1].gd.pos.y; 
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].gd.cid = 4;
	gcd[2].creator = GRadioCreate;

	label[3].text = (unichar_t *) "8";
	label[3].text_is_1byte = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 80; gcd[3].gd.pos.y = gcd[1].gd.pos.y;
	gcd[3].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[3].gd.cid = 8;
	gcd[3].creator = GRadioCreate;

	gcd[4].gd.pos.x = 15-3; gcd[4].gd.pos.y = gcd[1].gd.pos.y+20;
	gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
	gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[4].text = (unichar_t *) _STR_OK;
	label[4].text_in_resource = true;
	gcd[4].gd.mnemonic = 'O';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.cid = 1001;
	/*gcd[4].gd.handle_controlevent = CH_OK;*/
	gcd[4].creator = GButtonCreate;

	gcd[5].gd.pos.x = -15; gcd[5].gd.pos.y = gcd[4].gd.pos.y+3;
	gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
	gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[5].text = (unichar_t *) _STR_Cancel;
	label[5].text_in_resource = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.mnemonic = 'C';
	/*gcd[5].gd.handle_controlevent = CH_Cancel;*/
	gcd[5].gd.cid = 1002;
	gcd[5].creator = GButtonCreate;

	gcd[6].gd.pos.x = 2; gcd[6].gd.pos.y = 2;
	gcd[6].gd.pos.width = pos.width-4; gcd[6].gd.pos.height = pos.height-2;
	gcd[6].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[6].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
    }

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
    if ( done==-1 )
return( -1 );
    if ( GGadgetIsChecked(GWidgetGetControl(gw,2)) )
return( 2 );
    if ( GGadgetIsChecked(GWidgetGetControl(gw,4)) )
return( 4 );
    /*if ( GGadgetIsChecked(GWidgetGetControl(gw,8)) )*/
return( 8 );
}
#endif

static int br_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	int *done = GDrawGetUserData(gw);
	*done = -1;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	int *done = GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)==1001 )
	    *done = true;
	else
	    *done = -1;
    } else if ( event->type==et_controlevent && GGadgetGetCid(event->u.control.g)==1003 ) {
	static int first = true;
	if ( first )
	    first = false;
	else
	    GGadgetSetChecked(GWidgetGetControl(gw,1004),true);
    }
return( true );
}

static int AskResolution() {
    GRect pos;
    static GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[10];
    int done=-3;

    if ( screen_display==NULL )
return( -1 );

    if ( gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_BDFResolution,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
	pos.height = GDrawPointsToPixels(NULL,130);
	gw = GDrawCreateTopWindow(NULL,&pos,br_e_h,&done,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) _STR_BDFResolution;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 7; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	label[1].text = (unichar_t *) "75";
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.mnemonic = '7';
	gcd[1].gd.pos.x = 20; gcd[1].gd.pos.y = 13+7;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = 75;
	gcd[1].creator = GRadioCreate;

	label[2].text = (unichar_t *) "100";
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.mnemonic = '1';
	gcd[2].gd.pos.x = 20; gcd[2].gd.pos.y = gcd[1].gd.pos.y+17; 
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].gd.cid = 100;
	gcd[2].creator = GRadioCreate;

	label[3].text = (unichar_t *) _STR_Guess;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 20; gcd[3].gd.pos.y = gcd[2].gd.pos.y+17;
	gcd[3].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[3].gd.cid = -1;
	gcd[3].gd.popup_msg = GStringGetResource(_STR_GuessResPopup,NULL);
	gcd[3].creator = GRadioCreate;

	label[4].text = (unichar_t *) _STR_Other_;
	label[4].text_in_resource = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 20; gcd[4].gd.pos.y = gcd[3].gd.pos.y+17;
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.cid = 1004;
	gcd[4].creator = GRadioCreate;

	label[5].text = (unichar_t *) "96";
	label[5].text_is_1byte = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.pos.x = 70; gcd[5].gd.pos.y = gcd[4].gd.pos.y-3; gcd[5].gd.pos.width = 40;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.cid = 1003;
	gcd[5].creator = GTextFieldCreate;

	gcd[6].gd.pos.x = 15-3; gcd[6].gd.pos.y = gcd[4].gd.pos.y+24;
	gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
	gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[6].text = (unichar_t *) _STR_OK;
	label[6].text_in_resource = true;
	gcd[6].gd.mnemonic = 'O';
	gcd[6].gd.label = &label[6];
	gcd[6].gd.cid = 1001;
	/*gcd[6].gd.handle_controlevent = CH_OK;*/
	gcd[6].creator = GButtonCreate;

	gcd[7].gd.pos.x = -15; gcd[7].gd.pos.y = gcd[6].gd.pos.y+3;
	gcd[7].gd.pos.width = -1; gcd[7].gd.pos.height = 0;
	gcd[7].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[7].text = (unichar_t *) _STR_Cancel;
	label[7].text_in_resource = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.mnemonic = 'C';
	/*gcd[7].gd.handle_controlevent = CH_Cancel;*/
	gcd[7].gd.cid = 1002;
	gcd[7].creator = GButtonCreate;

	gcd[8].gd.pos.x = 2; gcd[8].gd.pos.y = 2;
	gcd[8].gd.pos.width = pos.width-4; gcd[8].gd.pos.height = pos.height-2;
	gcd[8].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[8].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
    }

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( true ) {
	done = 0;
	while ( !done )
	    GDrawProcessOneEvent(NULL);
	if ( GGadgetIsChecked(GWidgetGetControl(gw,1004)) ) {
	    int err = false;
	    int res = GetIntR(gw,1003,_STR_Other,&err);
	    if ( res<10 )
		ProtestR( _STR_Other_);
	    else if ( !err ) {
		GDrawSetVisible(gw,false);
return( res );
	    }
    continue;
	}
	GDrawSetVisible(gw,false);
	if ( done==-1 )
return( -1 );
	if ( GGadgetIsChecked(GWidgetGetControl(gw,75)) )
return( 75 );
	if ( GGadgetIsChecked(GWidgetGetControl(gw,100)) )
return( 100 );
	/*if ( GGadgetIsChecked(GWidgetGetControl(gw,-1)) )*/
return( -1 );
    }
}

static int WriteBitmaps(char *filename,SplineFont *sf, int32 *sizes,int res) {
    char *buf = galloc(strlen(filename)+30), *pt, *pt2;
    int i;
    BDFFont *bdf;
    unichar_t *temp;
    char buffer[100];
    /* res = -1 => Guess depending on pixel size of font */
    extern int ask_user_for_resolution;

    if ( ask_user_for_resolution ) {
	GProgressPauseTimer();
	res = AskResolution();
	GProgressResumeTimer();
	if ( res==-2 )
return( false );
    }

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( i=0; sizes[i]!=0; ++i );
    GProgressChangeStages(i);
    for ( i=0; sizes[i]!=0; ++i ) {
	buffer[0] = '\0';
	for ( bdf=sf->bitmaps; bdf!=NULL &&
		(bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16));
		bdf=bdf->next );
	if ( bdf==NULL )
	    sprintf(buffer,"Attempt to save a pixel size that has not been created (%d@%d)", sizes[i]&0xffff, sizes[i]>>16);
	if ( buffer[0]!='\0' ) {
	    temp = uc_copy(buffer);
	    GWidgetPostNotice(temp,temp);
	    free(temp);
	    free(buf);
return( false );
	}
	strcpy(buf,filename);
	pt = strrchr(buf,'.');
	if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	    pt = NULL;
	if ( pt==NULL )
	    pt = buf+strlen(buf);
	if ( strcmp(pt-4,".otf.dfont")==0 || strcmp(pt-4,".ttf.bin")==0 ) pt-=4;
	if ( bdf->clut==NULL )
	    sprintf( pt, "-%d.bdf", bdf->pixelsize );
	else
	    sprintf( pt, "-%d@%d.bdf", bdf->pixelsize, BDFDepth(bdf) );
	
	GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
	BDFFontDump(buf,bdf,EncodingName(sf->encoding_name),res);
	GProgressNextStage();
    }
    free(buf);
return( true );
}

static int _DoSave(SplineFont *sf,char *newname,int32 *sizes,int res) {
    unichar_t *path;
    int err=false;
    int iscid = oldformatstate==ff_cid || oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont;

    path = uc_copy(newname);
    GProgressStartIndicator(10,GStringGetResource(_STR_SavingFont,NULL),
		GStringGetResource(oldformatstate==ff_ttf || oldformatstate==ff_ttfsym ||
		     oldformatstate==ff_ttfmacbin ?_STR_SavingTTFont:
		 oldformatstate==ff_otf || oldformatstate==ff_otfdfont ?_STR_SavingOpenTypeFont:
		 oldformatstate==ff_cid || oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont ?_STR_SavingCIDFont:
		 _STR_SavingPSFont,NULL),
	    path,sf->charcnt,1);
    free(path);
    GProgressEnableStop(false);
    if ( oldformatstate!=ff_none || oldbitmapstate==bf_sfnt_dfont ) {
	int oerr = 0;
	switch ( oldformatstate ) {
	  case ff_pfa: case ff_pfb: case ff_ptype3: case ff_ptype0: case ff_cid:
	    oerr = !WritePSFont(newname,sf,oldformatstate);
	  break;
	  case ff_ttf: case ff_ttfsym: case ff_otf: case ff_otfcid:
	    oerr = !WriteTTFFont(newname,sf,oldformatstate,sizes,oldbitmapstate);
	  break;
	  case ff_pfbmacbin:
	    oerr = !WriteMacPSFont(newname,sf,oldformatstate);
	  break;
	  case ff_ttfmacbin: case ff_ttfdfont: case ff_otfdfont: case ff_otfciddfont:
	  case ff_none:		/* only if bitmaps, an sfnt wrapper for bitmaps */
	    oerr = !WriteMacTTFFont(newname,sf,oldformatstate,sizes,
		    oldbitmapstate);
	  break;
	}
	if ( oerr ) {
	    GWidgetErrorR(_STR_Savefailedtitle,_STR_Savefailedtitle);
	    err = true;
	}
    }
    if ( !err && oldafmstate ) {
	GProgressChangeLine1R(_STR_SavingAFM);
	GProgressIncrementBy(-sf->charcnt);
	if ( !WriteAfmFile(newname,sf,oldformatstate)) {
	    GWidgetErrorR(_STR_Afmfailedtitle,_STR_Afmfailedtitle);
	    err = true;
	}
    }
    if ( !err && oldpfmstate && !iscid ) {
	GProgressChangeLine1R(_STR_SavingPFM);
	GProgressIncrementBy(-sf->charcnt);
	if ( !WritePfmFile(newname,sf,oldformatstate==ff_ptype0)) {
	    GWidgetErrorR(_STR_Pfmfailedtitle,_STR_Pfmfailedtitle);
	    err = true;
	}
    }
    if ( oldbitmapstate==bf_bdf && !err ) {
	GProgressChangeLine1R(_STR_SavingBitmapFonts);
	GProgressIncrementBy(-sf->charcnt);
	if ( !WriteBitmaps(newname,sf,sizes,res))
	    err = true;
    } else if ( (oldbitmapstate==bf_nfntmacbin || oldbitmapstate==bf_nfntdfont) &&
	    !err ) {
	if ( !WriteMacBitmaps(newname,sf,sizes,oldbitmapstate==bf_nfntdfont))
	    err = true;
    }
    free( sizes );
    GProgressEndIndicator();
    if ( !err )
	SavePrefs();
return( err );
}

int GenerateScript(SplineFont *sf,char *filename,char *bitmaptype, int fmflags, int res) {
    int i;
    static char *bitmaps[] = {"bdf", "ms", "apple", "sbit", "bin", "dfont", NULL };
    int32 *sizes=NULL;
    char *end = filename+strlen(filename);

    for ( i=0; extensions[i]!=NULL; ++i ) {
	if ( strlen( extensions[i])>0 &&
		strmatch(end-strlen(extensions[i]),extensions[i])==0 )
    break;
    }
    if ( strmatch(end-strlen(".ttf.bin"),".ttf.bin")==0 )
	i = ff_ttfmacbin;
    else if ( strmatch(end-strlen(".suit"),".suit")==0 )	/* Different extensions for Mac/non Mac, support both always */
	i = ff_ttfmacbin;
    else if ( strmatch(end-strlen(".bin"),".bin")==0 )
	i = ff_pfbmacbin;
    else if ( strmatch(end-strlen(".res"),".res")==0 )
	i = ff_pfbmacbin;
    else if ( strmatch(end-strlen(".sym.ttf"),".sym.ttf")==0 )
	i = ff_ttfsym;
    if ( extensions[i]==NULL ) {
	for ( i=0; bitmaps[i]!=NULL; ++i ) {
	    if ( strmatch(end-strlen(bitmaps[i]),bitmaps[i])==0 )
	break;
	}
	if ( bitmaps[i]==NULL )
	    i = ff_pfb;
	else
	    i = ff_none;
    }
    if ( i==ff_ptype3 && (
	    (sf->encoding_name>=em_first2byte && sf->encoding_name<em_max2 ) ||
	    (sf->encoding_name>=em_unicodeplanes && sf->encoding_name<=em_unicodeplanesmax )) )
	i = ff_ptype0;
    else if ( i==ff_ttfdfont && strmatch(filename-strlen(".otf.dfont"),".otf.dfont")==0 )
	i = ff_otfdfont;
    if ( sf->cidmaster!=NULL ) {
	if ( i==ff_otf ) i = ff_otfcid;
	else if ( i==ff_otfdfont ) i = ff_otfciddfont;
    }
    oldformatstate = i;

    if ( sf->bitmaps==NULL ) i = bf_none;
    else if ( strmatch(bitmaptype,"ttf")==0 ) i = bf_ttf_ms;
    else for ( i=0; bitmaps[i]!=NULL; ++i ) {
	if ( strmatch(bitmaptype,bitmaps[i])==0 )
    break;
    }
    oldbitmapstate = i;
    if ( i==bf_sfnt_dfont )
	oldformatstate = ff_none;

    if ( fmflags==-1 ) {
	oldafmstate = true;
	if ( oldformatstate==ff_ttf || oldformatstate==ff_ttfsym || oldformatstate==ff_otf ||
		oldformatstate==ff_ttfdfont || oldformatstate==ff_otfdfont || oldformatstate==ff_otfciddfont ||
		oldformatstate==ff_otfcid || oldformatstate==ff_ttfmacbin || oldformatstate==ff_none )
	    oldafmstate = false;
	oldpfmstate = false;
    } else {
	oldafmstate = fmflags&1;
	oldpfmstate = (fmflags&2)?1:0;
    }

    if ( oldbitmapstate!=bf_none ) {
	BDFFont *bdf;
	int cnt;
	for ( i=0; i<2; ++i ) {
	    cnt = 0;
	    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( sizes!=NULL )
		    sizes[cnt] = bdf->pixelsize | (BDFDepth(bdf)<<16);
		++cnt;
	    }
	    if ( i==1 )
	break;
	    sizes = galloc((cnt+1)*sizeof(int32));
	}
	sizes[cnt] = 0;
    }
return( !_DoSave(sf,filename,sizes,res));
}

static void DoSave(struct gfc_data *d,unichar_t *path) {
    int err=false;
    char *temp;
    int32 *sizes=NULL;
    int iscid;
    Encoding *item=NULL;

    temp = cu_copy(path);
    oldformatstate = GGadgetGetFirstListSelectedItem(d->pstype);
    iscid = oldformatstate==ff_cid || oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont;
    if ( !iscid && (d->sf->cidmaster!=NULL || d->sf->subfontcnt>1)) {
	static int buts[] = { _STR_Yes, _STR_No, 0 };
	if ( GWidgetAskR(_STR_NotCID,buts,0,1,_STR_NotCIDOk)==1 )
return;
    }
    if ( d->sf->encoding_name>=em_base )
	for ( item=enclist; item!=NULL && item->enc_num!=d->sf->encoding_name; item=item->next );
    if ( oldformatstate<ff_ptype0 &&
	    ((d->sf->encoding_name>=em_first2byte && d->sf->encoding_name<em_base) ||
	     (d->sf->encoding_name>=em_base && (item==NULL || item->char_cnt>256))) ) {
	static int buts[3] = { _STR_Yes, _STR_Cancel, 0 };
	if ( GWidgetAskR(_STR_EncodingTooLarge,buts,0,1,_STR_TwoBEncIn1BFont)==1 )
return;
    }
    oldbitmapstate = GGadgetGetFirstListSelectedItem(d->bmptype);
    if ( oldbitmapstate!=bf_none )
	sizes = ParseBitmapSizes(d->bmpsizes,&err);

    oldafmstate = GGadgetIsChecked(d->doafm);
    oldpfmstate = GGadgetIsChecked(d->dopfm);

    err = _DoSave(d->sf,temp,sizes,-1);

    free(temp);
    d->done = !err;
    d->ret = !err;
}

static void GFD_doesnt(GIOControl *gio) {
    /* The filename the user chose doesn't exist, so everything is happy */
    struct gfc_data *d = gio->userdata;
    DoSave(d,gio->path);
    GFileChooserReplaceIO(d->gfc,NULL);
}

static void GFD_exists(GIOControl *gio) {
    /* The filename the user chose exists, ask user if s/he wants to overwrite */
    struct gfc_data *d = gio->userdata;
    unichar_t buffer[200];
    const unichar_t *rcb[3]; unichar_t rcmn[2];

    rcb[2]=NULL;
    rcb[0] = GStringGetResource( _STR_Replace, &rcmn[0]);
    rcb[1] = GStringGetResource( _STR_Cancel, &rcmn[1]);

    u_strcpy(buffer, GStringGetResource(_STR_Fileexistspre,NULL));
    u_strcat(buffer, u_GFileNameTail(gio->path));
    u_strcat(buffer, GStringGetResource(_STR_Fileexistspost,NULL));
    if ( GWidgetAsk(GStringGetResource(_STR_Fileexists,NULL),rcb,rcmn,0,1,buffer)==0 ) {
	DoSave(d,gio->path);
    }
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_SaveOk(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *ret = GGadgetGetTitle(d->gfc);
	int formatstate = GGadgetGetFirstListSelectedItem(d->pstype);

	if ( formatstate!=ff_none )	/* are we actually generating an outline font? */
	    GIOfileExists(GFileChooserReplaceIO(d->gfc,
		    GIOCreate(ret,d,GFD_exists,GFD_doesnt)));
	else
	    GFD_doesnt(GIOCreate(ret,d,GFD_exists,GFD_doesnt));	/* No point in bugging the user if we aren't doing anything */
	free(ret);
    }
return( true );
}

static int GFD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
	d->ret = false;
    }
return( true );
}

static void GFD_dircreated(GIOControl *gio) {
    struct gfc_data *d = gio->userdata;
    unichar_t *dir = u_copy(gio->path);

    GFileChooserReplaceIO(d->gfc,NULL);
    GFileChooserSetDir(d->gfc,dir);
    free(dir);
}

static void GFD_dircreatefailed(GIOControl *gio) {
    /* We couldn't create the directory */
    struct gfc_data *d = gio->userdata;
    unichar_t buffer[500];

    u_strcpy(buffer, GStringGetResource(_STR_Couldntcreatedir,NULL));
    uc_strcat(buffer,": ");
    u_strcat(buffer, u_GFileNameTail(gio->path));
    uc_strcat(buffer, ".\n");
    if ( gio->error!=NULL ) {
	u_strcat(buffer,gio->error);
	uc_strcat(buffer, "\n");
    }
    if ( gio->status[0]!='\0' )
	u_strcat(buffer,gio->status);
    GWidgetPostNotice(GStringGetResource(_STR_Couldntcreatedir,NULL),buffer);
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_NewDir(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *newdir;
	newdir = GWidgetAskStringR(_STR_Createdir,NULL,_STR_Dirname);
	if ( newdir==NULL )
return( true );
	if ( !u_GFileIsAbsolute(newdir)) {
	    unichar_t *temp = u_GFileAppendFile(GFileChooserGetDir(d->gfc),newdir,false);
	    free(newdir);
	    newdir = temp;
	}
	GIOmkDir(GFileChooserReplaceIO(d->gfc,
		GIOCreate(newdir,d,GFD_dircreated,GFD_dircreatefailed)));
	free(newdir);
    }
return( true );
}

static int GFD_Format(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *pt, *dup, *tpt, *ret;
	int format = GGadgetGetFirstListSelectedItem(d->pstype);
	int set, len, bf;
	static unichar_t nullstr[] = { 0 };
	GTextInfo **list;
	SplineFont *temp;

	set = true;
	if ( format==ff_ttf || format==ff_ttfsym || format==ff_otf ||
		format==ff_ttfdfont || format==ff_otfdfont || format==ff_otfciddfont ||
		format==ff_otfcid || format==ff_ttfmacbin || format==ff_none )
	    set = false;
	GGadgetSetChecked(d->doafm,set);
	if ( !set )				/* Don't default to generating pfms */
	    GGadgetSetChecked(d->dopfm,set);

	list = GGadgetGetList(d->bmptype,&len);
	temp = d->sf->cidmaster ? d->sf->cidmaster : d->sf;
	if ( format==ff_none ) {
	    if ( temp->bitmaps!=NULL )
		list[bf_sfnt_dfont]->disabled = false;
return( true );
	}

	ret = GGadgetGetTitle(d->gfc);
	dup = galloc((u_strlen(ret)+30)*sizeof(unichar_t));
	u_strcpy(dup,ret);
	free(ret);
	pt = u_strrchr(dup,'.');
	tpt = u_strrchr(dup,'/');
	if ( pt<tpt )
	    pt = NULL;
	if ( pt==NULL ) pt = dup+u_strlen(dup);
	if ( uc_strcmp(pt-4, ".ttf.bin" )==0 ) pt -= 4;
	if ( uc_strcmp(pt-4, ".otf.dfont" )==0 ) pt -= 4;
	uc_strcpy(pt,extensions[format]);
	GGadgetSetTitle(d->gfc,dup);
	free(dup);

	if ( d->sf->cidmaster!=NULL ) {
	    if ( format!=ff_none && format != ff_cid && format != ff_otfcid && format!=ff_otfciddfont ) {
		GGadgetSetTitle(d->bmpsizes,nullstr);
		/*GGadgetSetVisible(d->doafm,true);*/
		GGadgetSetVisible(d->dopfm,true);
	    } else {
		/*GGadgetSetVisible(d->doafm,false);*/
		GGadgetSetVisible(d->dopfm,false);
	    }
	}

	bf = GGadgetGetFirstListSelectedItem(d->bmptype);
	list[bf_sfnt_dfont]->disabled = true;
	if ( temp->bitmaps==NULL )
	    /* Don't worry about what formats are possible, they're disabled */;
	else if ( set ) {
	    /* If we're not in a ttf format (set) then we can't output ttf bitmaps */
	    if ( bf==bf_ttf_ms || bf==bf_ttf_apple ||
		    bf==bf_nfntmacbin || bf==bf_nfntdfont )
		GGadgetSelectOneListItem(d->bmptype,bf_bdf);
	    if ( format==ff_pfbmacbin )
		GGadgetSelectOneListItem(d->bmptype,bf_nfntmacbin);
	    else if ( bf==bf_nfntmacbin || bf==bf_nfntdfont )
	    list[bf_ttf_ms]->disabled = true;
	    list[bf_ttf_apple]->disabled = true;
	} else {
	    list[bf_ttf_ms]->disabled = false;
	    list[bf_ttf_apple]->disabled = false;
	    if ( bf==bf_none )
		/* Do nothing, always appropriate */;
	    else if ( format==ff_ttf || format==ff_ttfsym || format==ff_otf ||
		    format==ff_otfcid )
		GGadgetSelectOneListItem(d->bmptype,bf_ttf_ms);
	    else
		GGadgetSelectOneListItem(d->bmptype,bf_ttf_apple);
	}
#if __Mac
	{ GGadget *pulldown, *list, *tf;
	    /* The name of the postscript file is fixed and depends solely on */
	    /*  the font name. If the user tried to change it, the font would */
	    /*  not be found */
	    GFileChooserGetChildren(d->gfc,&pulldown,&list,&tf);
	    GGadgetSetVisible(tf,format!=ff_pfbmacbin);
	}
#endif
    }
return( true );
}

static int GFD_BitmapFormat(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *pt, *dup, *tpt, *ret;
	/*int format = GGadgetGetFirstListSelectedItem(d->pstype);*/
	int bf = GGadgetGetFirstListSelectedItem(d->bmptype);

	GGadgetSetEnabled(d->bmpsizes,bf!=bf_none);
	if ( bf==bf_sfnt_dfont ) {
	    ret = GGadgetGetTitle(d->gfc);
	    dup = galloc((u_strlen(ret)+30)*sizeof(unichar_t));
	    u_strcpy(dup,ret);
	    free(ret);
	    pt = u_strrchr(dup,'.');
	    tpt = u_strrchr(dup,'/');
	    if ( pt<tpt )
		pt = NULL;
	    if ( pt==NULL ) pt = dup+u_strlen(dup);
	    if ( uc_strcmp(pt-4, ".ttf.bin" )==0 ) pt -= 4;
	    if ( uc_strcmp(pt-4, ".otf.dfont" )==0 ) pt -= 4;
	    uc_strcpy(pt,".dfont");
	    GGadgetSetTitle(d->gfc,dup);
	    free(dup);
	}
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfc_data *d = GDrawGetUserData(gw);
	d->done = true;
	d->ret = false;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static unichar_t *BitmapList(SplineFont *sf) {
    BDFFont *bdf;
    int i;
    char *cret, *pt;
    unichar_t *uret;

    for ( bdf=sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i );
    pt = cret = galloc((i+1)*20);
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	if ( pt!=cret ) *pt++ = ',';
	if ( bdf->clut==NULL )
	    sprintf( pt, "%d", bdf->pixelsize );
	else
	    sprintf( pt, "%d@%d", bdf->pixelsize, BDFDepth(bdf) );
	pt += strlen(pt);
    }
    *pt = '\0';
    uret = uc_copy(cret);
    free(cret);
return( uret );
}

int FontMenuGeneratePostscript(SplineFont *sf) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14];
    GTextInfo label[14];
    struct gfc_data d;
    GGadget *pulldown, *files, *tf;
    int i, old, ofs;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, spacing;
    SplineFont *temp;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Generate,NULL);
    pos.x = pos.y = 0;
    totwid = GGadgetScale(295);
    bsbigger = 4*bs+4*14>totwid; totwid = bsbigger?4*bs+4*12:totwid;
    spacing = (totwid-4*bs-2*12)/3;
    pos.width = GDrawPointsToPixels(NULL,totwid);
    pos.height = GDrawPointsToPixels(NULL,285);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = 100*totwid/GIntGetResource(_NUM_ScaleFactor)-24; gcd[0].gd.pos.height = 182;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GFileChooserCreate;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 252-3;
    gcd[1].gd.pos.width = -1;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _STR_Save;
    label[1].text_in_resource = true;
    gcd[1].gd.mnemonic = 'S';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_SaveOk;
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = -(spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)-12; gcd[2].gd.pos.y = 252;
    gcd[2].gd.pos.width = -1;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _STR_Filter;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'F';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = -12; gcd[3].gd.pos.y = 252; gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _STR_Cancel;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'C';
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = (spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)+12; gcd[4].gd.pos.y = 252; gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) _STR_New;
    label[4].text_in_resource = true;
    label[4].image = &_GIcon_dir;
    label[4].image_precedes = false;
    gcd[4].gd.mnemonic = 'N';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = GFD_NewDir;
    gcd[4].creator = GButtonCreate;

    if ( oldafmstate==-1 ) {
	oldafmstate = true;
	if ( oldformatstate==ff_ttf || oldformatstate==ff_ttfsym || oldformatstate==ff_otf ||
		oldformatstate==ff_ttfdfont || oldformatstate==ff_otfdfont || oldformatstate==ff_otfciddfont ||
		oldformatstate==ff_otfcid || oldformatstate==ff_ttfmacbin || oldformatstate==ff_none )
	    oldafmstate = false;
    }

    gcd[5].gd.pos.x = 12; gcd[5].gd.pos.y = 214; gcd[5].gd.pos.width = 0; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | (oldafmstate ?gg_cb_on : 0 );
    label[5].text = (unichar_t *) _STR_Outputafm;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].creator = GCheckBoxCreate;

    gcd[6].gd.pos.x = 12; gcd[6].gd.pos.y = 190; gcd[6].gd.pos.width = 0; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled ;
    gcd[6].gd.u.list = formattypes;
    gcd[6].creator = GListButtonCreate;
    for ( i=0; i<sizeof(formattypes)/sizeof(formattypes[0])-1; ++i )
	formattypes[i].selected = sf->onlybitmaps;
    formattypes[ff_ptype0].disabled = sf->onlybitmaps ||
	    ( sf->encoding_name<em_jis208 && sf->encoding_name>=em_base);
    formattypes[ff_cid].disabled = sf->cidmaster==NULL;
    formattypes[ff_otfcid].disabled = sf->cidmaster==NULL;
    formattypes[ff_otfciddfont].disabled = sf->cidmaster==NULL;
    formattypes[ff_otfciddfont].disabled = true;	/* Not ready for this yet! */
    ofs = oldformatstate;
    if (( ofs==ff_ptype0 && formattypes[ff_ptype0].disabled ) ||
	    ((ofs==ff_cid || ofs==ff_otfcid || ofs==ff_otfciddfont) && formattypes[ff_cid].disabled))
	ofs = ff_pfb;
    else if ( (ofs!=ff_cid && ofs!=ff_otfcid && ofs!=ff_otfciddfont) && sf->cidmaster!=NULL )
	ofs = ff_otfcid;
    if ( sf->onlybitmaps )
	ofs = ff_none;
    for ( i=0; i<sizeof(formattypes)/sizeof(formattypes[0]); ++i )
	formattypes[i].selected = false;
    formattypes[ofs].selected = true;
    gcd[6].gd.handle_controlevent = GFD_Format;
    gcd[6].gd.label = &formattypes[ofs];

    gcd[7].gd.pos.x = 2; gcd[7].gd.pos.y = 2;
    gcd[7].gd.pos.width = pos.width-4; gcd[7].gd.pos.height = pos.height-4;
    gcd[7].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[7].creator = GGroupCreate;

    gcd[8].gd.pos.x = 155; gcd[8].gd.pos.y = 190; gcd[8].gd.pos.width = 126;
    gcd[8].gd.flags = gg_visible | gg_enabled;
    gcd[8].gd.u.list = bitmaptypes;
    gcd[8].creator = GListButtonCreate;
    for ( i=0; i<sizeof(bitmaptypes)/sizeof(bitmaptypes[0]); ++i ) {
	bitmaptypes[i].selected = false;
	bitmaptypes[i].disabled = false;
    }
    old = oldbitmapstate;
    if ( sf->onlybitmaps ) {
	old = 0;
	bitmaptypes[bf_ttf_ms].disabled = true;
	bitmaptypes[bf_ttf_apple].disabled = true;
    }
    temp = sf->cidmaster ? sf->cidmaster : sf;
    if ( temp->bitmaps==NULL ) {
	old = bf_none;
	bitmaptypes[bf_bdf].disabled = true;
	bitmaptypes[bf_ttf_ms].disabled = true;
	bitmaptypes[bf_ttf_apple].disabled = true;
	bitmaptypes[bf_sfnt_dfont].disabled = true;
	bitmaptypes[bf_nfntmacbin].disabled = true;
	bitmaptypes[bf_nfntdfont].disabled = true;
    } else if ( ofs!=ff_none )
	bitmaptypes[bf_sfnt_dfont].disabled = true;
    bitmaptypes[old].selected = true;
    gcd[8].gd.label = &bitmaptypes[old];
    gcd[8].gd.handle_controlevent = GFD_BitmapFormat;

    gcd[9].gd.pos.x = gcd[8].gd.pos.x; gcd[9].gd.pos.y = 219; gcd[9].gd.pos.width = gcd[8].gd.pos.width;
    gcd[9].gd.flags = gg_visible | gg_enabled;
    if ( old==bf_none )
	gcd[9].gd.flags &= ~gg_enabled;
    gcd[9].creator = GTextFieldCreate;
    label[9].text = BitmapList(temp);
    gcd[9].gd.label = &label[9];

    gcd[10].gd.pos.x = 12; gcd[10].gd.pos.y = 231; gcd[10].gd.pos.width = 0; gcd[10].gd.pos.height = 0;
    gcd[10].gd.flags = gg_visible | gg_enabled | (oldpfmstate ?gg_cb_on : 0 );
    label[10].text = (unichar_t *) _STR_Outputpfm;
    label[10].text_in_resource = true;
    gcd[10].gd.label = &label[10];
    gcd[10].creator = GCheckBoxCreate;

    if ( ofs==ff_otfcid || ofs==ff_cid || ofs==ff_otfciddfont) {
	/*gcd[5].gd.flags &= ~gg_visible;*/
	gcd[10].gd.flags &= ~gg_visible;
    }

    GGadgetsCreate(gw,gcd);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);
    free(label[9].text);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    {
	char *fn = sf->cidmaster==NULL? sf->fontname:sf->cidmaster->fontname;
	unichar_t *temp = galloc(sizeof(unichar_t)*(strlen(fn)+30));
	uc_strcpy(temp,fn);
	uc_strcat(temp,extensions[ofs]==NULL?".pfb":extensions[ofs]);
	GGadgetSetTitle(gcd[0].ret,temp);
	free(temp);
    }
    GFileChooserGetChildren(gcd[0].ret,&pulldown,&files,&tf);
    GWidgetIndicateFocusGadget(tf);
#if __Mac
	/* The name of the postscript file is fixed and depends solely on */
	/*  the font name. If the user tried to change it, the font would */
	/*  not be found */
	GGadgetSetVisible(tf,ofs!=ff_pfbmacbin);
#endif

    memset(&d,'\0',sizeof(d));
    d.sf = sf;
    d.gfc = gcd[0].ret;
    d.doafm = gcd[5].ret;
    d.dopfm = gcd[10].ret;
    d.pstype = gcd[6].ret;
    d.bmptype = gcd[8].ret;
    d.bmpsizes = gcd[9].ret;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return(d.ret);
}
