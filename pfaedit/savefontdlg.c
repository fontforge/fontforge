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
#include <stdlib.h>
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

static char *extensions[] = { ".pfa", ".pfb", ".ps", ".ps", ".ttf", ".ttf", ".otf", NULL };
static GTextInfo formattypes[] = {
    { (unichar_t *) "PS Type 1 (Ascii)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Type 1 (Binary)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Type 3", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Type 0", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "True Type", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "True Type (Symbol)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Open Type (PS)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) _STR_Nooutlinefont, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }
};
static GTextInfo bitmaptypes[] = {
    { (unichar_t *) "BDF", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "GDF", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) _STR_Nobitmapfonts, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }
};

static int oldafmstate = true, oldpfmstate = false;
static int oldformatstate = ff_pfb;
static int oldbitmapstate = 0;

static double *ParseBitmapSizes(GGadget *g,int *err) {
    const unichar_t *val = _GGadgetGetTitle(g), *pt; unichar_t *end, *end2;
    int i;
    double *sizes;

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
    sizes = galloc((i+1)*sizeof(double));

    for ( i=0, pt = val; *pt!='\0' ; ) {
	sizes[i]=u_strtod(pt,&end);
	if ( sizes[i]>0 ) ++i;
	if ( *end!=' ' && *end!=',' && *end!='\0' ) {
	    free(sizes);
	    Protest("Pixel List");
	    *err = true;
return( NULL );
	}
	while ( *end==' ' || *end==',' ) ++end;
	pt = end;
    }
    sizes[i] = 0;
return( sizes );
}

static int WriteAfmFile(char *filename,SplineFont *sf, int type0) {
    char *buf = malloc(strlen(filename)+6), *pt, *pt2;
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
    ret = AfmSplineFont(afm,sf,type0);
    if ( fclose(afm)==-1 )
return( 0 );
return( ret );
}

static int WritePfmFile(char *filename,SplineFont *sf, int type0) {
    char *buf = malloc(strlen(filename)+6), *pt, *pt2;
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

static int WriteBitmaps(char *filename,SplineFont *sf, double *sizes, int do_grey) {
    char *buf = malloc(strlen(filename)+20), *pt, *pt2;
    int i;
    BDFFont *bdf;
    unichar_t *temp;
    char buffer[100];

    for ( i=0; sizes[i]!=0; ++i );
    GProgressChangeStages(i);
    for ( i=0; sizes[i]!=0; ++i ) {
	buffer[0] = '\0';
	if ( do_grey ) {
	    bdf = SplineFontAntiAlias(sf,(int) sizes[i],4);
	    if ( bdf==NULL )
		sprintf(buffer,"Couldn't generate an anti-aliased font at the requested size (%d)", (int) sizes[i]);
	} else {
	    for ( bdf=sf->bitmaps; bdf!=NULL && bdf->pixelsize!=sizes[i]; bdf=bdf->next );
	    if ( bdf==NULL )
		sprintf(buffer,"Attempt to save a pixel size that has not been created (%d)", (int) sizes[i]);
	}
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
	sprintf( pt, "-%d.%s", bdf->pixelsize, bdf->clut==NULL?"bdf":"gdf" );
	GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
	BDFFontDump(buf,bdf,EncodingName(sf->encoding_name));
	if ( do_grey )
	    BDFFontFree(bdf);
	GProgressNextStage();
    }
    free(buf);
return( true );
}

static void DoSave(struct gfc_data *d,unichar_t *path) {
    int err=false;
    char *temp;
    double *sizes;
    static unichar_t save[] = { 'S','a','v','i','n','g',' ','f','o','n','t',  '\0' };
    static unichar_t saveps[] = { 'S','a','v','i','n','g',' ','P','o','s','t','s','c','r','i','p','t',' ','F','o','n','t', '\0' };
    static unichar_t savettf[] = { 'S','a','v','i','n','g',' ','T','r','u','e','T','y','p','e',' ','F','o','n','t', '\0' };
    static unichar_t saveotf[] = { 'S','a','v','i','n','g',' ','O','p','e','n','T','y','p','e',' ','F','o','n','t', '\0' };
    static unichar_t saveafm[] = { 'S','a','v','i','n','g',' ','A','F','M',' ','F','i','l','e', '\0' };
    static unichar_t savepfm[] = { 'S','a','v','i','n','g',' ','P','F','M',' ','F','i','l','e', '\0' };
    static unichar_t savebdf[] = { 'S','a','v','i','n','g',' ','B','i','t','m','a','p',' ','F','o','n','t','(','s',')', '\0' };

    temp = cu_copy(path);
    oldformatstate = GGadgetGetFirstListSelectedItem(d->pstype);
    GProgressStartIndicator(10,save,oldformatstate==ff_ttf || oldformatstate==ff_ttfsym?
		savettf:oldformatstate==ff_otf?saveotf:saveps,
	    path,d->sf->charcnt,1);
    GProgressEnableStop(false);
    if ( oldformatstate!=ff_none ) {
	if ( (oldformatstate!=ff_ttf && oldformatstate!=ff_ttfsym && oldformatstate!=ff_otf &&
		    !WritePSFont(temp,d->sf,oldformatstate)) ||
		((oldformatstate==ff_ttf || oldformatstate==ff_ttfsym || oldformatstate==ff_otf) &&
		    !WriteTTFFont(temp,d->sf,oldformatstate)) ) {
	    GWidgetErrorR(_STR_Savefailedtitle,_STR_Savefailedtitle);
	    err = true;
	}
    }
    oldafmstate = GGadgetIsChecked(d->doafm);
    oldpfmstate = GGadgetIsChecked(d->dopfm);
    if ( !err && oldafmstate ) {
	GProgressChangeLine1(saveafm);
	GProgressIncrementBy(-d->sf->charcnt);
	if ( !WriteAfmFile(temp,d->sf,oldformatstate==ff_ptype0)) {
	    GWidgetErrorR(_STR_Afmfailedtitle,_STR_Afmfailedtitle);
	    err = true;
	}
    }
    if ( !err && oldpfmstate ) {
	GProgressChangeLine1(savepfm);
	GProgressIncrementBy(-d->sf->charcnt);
	if ( !WritePfmFile(temp,d->sf,oldformatstate==ff_ptype0)) {
	    GWidgetErrorR(_STR_Pfmfailedtitle,_STR_Pfmfailedtitle);
	    err = true;
	}
    }
    oldbitmapstate = GGadgetGetFirstListSelectedItem(d->bmptype);
    if ( oldbitmapstate!=2 && !err ) {
	sizes = ParseBitmapSizes(d->bmpsizes,&err);
	GProgressChangeLine1(savebdf);
	GProgressIncrementBy(-d->sf->charcnt);
	if ( !WriteBitmaps(temp,d->sf,sizes,oldbitmapstate))
	    err = true;
	free( sizes );
    }
    if ( !err ) {
	/*free(d->sf->filename);*/
	/*d->sf->filename = copy(temp);*/
	/*SplineFontSetUnChanged(d->sf);*/
    }
    GProgressEndIndicator();
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
	int set;

	set = true;
	if ( format==ff_ttf || format==ff_ttfsym || format==ff_otf || format==ff_none )
	    set = false;
	GGadgetSetChecked(d->doafm,set);
	if ( !set )				/* Don't default to generating pfms */
	    GGadgetSetChecked(d->dopfm,set);

	if ( format==ff_none )
return( true );

	ret = GGadgetGetTitle(d->gfc);
	dup = galloc((u_strlen(ret)+5)*sizeof(unichar_t));
	u_strcpy(dup,ret);
	free(ret);
	pt = u_strrchr(dup,'.');
	tpt = u_strrchr(dup,'/');
	if ( pt<tpt )
	    pt = NULL;
	if ( pt==NULL ) pt = dup+u_strlen(dup);
	uc_strcpy(pt,extensions[format]);
	GGadgetSetTitle(d->gfc,dup);
	free(dup);
    }
return( true );
}

static int GFD_Bitmap(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int bitmapstate = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(d->bmpsizes,bitmapstate!=2);
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
    pt = cret = galloc((i+1)*10);
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	if ( pt!=cret ) *pt++ = ',';
	sprintf( pt, "%d", bdf->pixelsize );
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
    static unichar_t title[] = { 'G','e','n','e','r','a','t','e',' ','p','o','s','t','s','c','r','i','p','t',' ','f','o','n','t', '\0' };
    int i, old;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, spacing;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    pos.x = pos.y = 0;
    bsbigger = 4*bs+4*14>295; totwid = bsbigger?4*bs+4*12:295;
    spacing = (totwid-4*bs-2*12)/3;
    pos.width = GDrawPointsToPixels(NULL,totwid);
    pos.height = GDrawPointsToPixels(NULL,285);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = totwid-24; gcd[0].gd.pos.height = 182;
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

    gcd[2].gd.pos.x = (totwid-spacing)/2-bs; gcd[2].gd.pos.y = 252;
    gcd[2].gd.pos.width = -1;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _STR_Filter;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'F';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = totwid-bs-12; gcd[3].gd.pos.y = 252; gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _STR_Cancel;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'C';
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = (totwid+spacing)/2; gcd[4].gd.pos.y = 252; gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) _STR_New;
    label[4].text_in_resource = true;
    label[4].image = &_GIcon_dir;
    label[4].image_precedes = false;
    gcd[4].gd.mnemonic = 'N';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = GFD_NewDir;
    gcd[4].creator = GButtonCreate;

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
    old = oldformatstate;
    if ( old==ff_ptype0 && formattypes[ff_ptype0].disabled )
	old = ff_pfb;
    if ( sf->onlybitmaps )
	old = ff_none;
    for ( i=0; i<sizeof(formattypes)/sizeof(formattypes[0]); ++i )
	formattypes[i].selected = false;
    formattypes[old].selected = true;
    gcd[6].gd.handle_controlevent = GFD_Format;
    gcd[6].gd.label = &formattypes[old];

    gcd[7].gd.pos.x = 2; gcd[7].gd.pos.y = 2;
    gcd[7].gd.pos.width = pos.width-4; gcd[7].gd.pos.height = pos.height-4;
    gcd[7].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[7].creator = GGroupCreate;

    gcd[8].gd.pos.x = 150; gcd[8].gd.pos.y = 190; gcd[8].gd.pos.width = 126;
    gcd[8].gd.flags = gg_visible | gg_enabled;
    gcd[8].gd.u.list = bitmaptypes;
    gcd[8].creator = GListButtonCreate;
    for ( i=0; i<sizeof(bitmaptypes)/sizeof(bitmaptypes[0]); ++i )
	bitmaptypes[i].selected = false;
    old = oldbitmapstate;
    if ( sf->onlybitmaps ) {
	old = 0;
	bitmaptypes[1].disabled = true;
    } else
	bitmaptypes[1].disabled = false;
    if ( sf->bitmaps==NULL ) {
	old = 2;
	bitmaptypes[0].disabled = true;
    } else
	bitmaptypes[0].disabled = false;
    bitmaptypes[old].selected = true;
    gcd[8].gd.label = &bitmaptypes[old];
    gcd[8].gd.handle_controlevent = GFD_Bitmap;

    gcd[9].gd.pos.x = 150; gcd[9].gd.pos.y = 219; gcd[9].gd.pos.width = 126;
    gcd[9].gd.flags = gg_visible | gg_enabled;
    if ( oldbitmapstate==2 )
	gcd[9].gd.flags &= ~gg_enabled;
    gcd[9].gd.u.list = bitmaptypes;
    gcd[9].creator = GTextFieldCreate;
    label[9].text = BitmapList(sf);
    gcd[9].gd.label = &label[9];

    gcd[10].gd.pos.x = 12; gcd[10].gd.pos.y = 231; gcd[10].gd.pos.width = 0; gcd[10].gd.pos.height = 0;
    gcd[10].gd.flags = gg_visible | gg_enabled | (oldpfmstate ?gg_cb_on : 0 );
    label[10].text = (unichar_t *) _STR_Outputpfm;
    label[10].text_in_resource = true;
    gcd[10].gd.label = &label[10];
    gcd[10].creator = GCheckBoxCreate;

    GGadgetsCreate(gw,gcd);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);
    free(label[9].text);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    {
	unichar_t *temp = galloc(sizeof(unichar_t)*(strlen(sf->fontname)+8));
	uc_strcpy(temp,sf->fontname);
	uc_strcat(temp,extensions[oldformatstate]==NULL?".pfb":extensions[oldformatstate]);
	GGadgetSetTitle(gcd[0].ret,temp);
	free(temp);
    }
    GFileChooserGetChildren(gcd[0].ret,&pulldown,&files,&tf);
    GWidgetIndicateFocusGadget(tf);

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
