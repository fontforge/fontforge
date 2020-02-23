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

#include "autotrace.h"
#include "cvundoes.h"
#include "ffglib.h"
#include "fontforgeui.h"
#include "gfile.h"
#include "gkeysym.h"
#include "lookups.h"
#include "print.h"
#include "sftextfieldP.h"
#include "splinesaveafm.h"
#include "splineutil2.h"
#include "tottfgpos.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#if !defined(__MINGW32__)
#include <sys/wait.h>
#endif

typedef struct printffdlg {
    struct printinfo pi;
    GWindow gw;
    GWindow setup;
    GTimer *sizechanged;
    GTimer *dpichanged;
    GTimer *widthchanged;
    GTimer *resized;
    GTextInfo *scriptlangs;
    FontView *fv;
    CharView *cv;
    SplineSet *fit_to_path;
    uint8 script_unknown;
    uint8 insert_text;
    uint8 ready;
    int *done;
} PD;

static PD *printwindow;

static int lastdpi=0;
static unichar_t *old_bind_text = NULL;

/* ************************************************************************** */
/* *********************** Code for Page Setup dialog *********************** */
/* ************************************************************************** */

#define CID_lp		1001
#define CID_lpr		1002
#define	CID_ghostview	1003
#define CID_File	1004
#define CID_Other	1005
#define CID_OtherCmd	1006
#define	CID_Pagesize	1007
#define CID_CopiesLab	1008
#define CID_Copies	1009
#define CID_PrinterLab	1010
#define CID_Printer	1011
#define CID_PDFFile	1012

static void PG_SetEnabled(PD *pi) {
    int enable;

    enable = GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_lp)) ||
	    GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_lpr));

    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_CopiesLab),enable);
    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_Copies),enable);
    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_PrinterLab),enable);
    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_Printer),enable);

    GGadgetSetEnabled(GWidgetGetControl(pi->setup,CID_OtherCmd),
	    GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_Other)));
}

static int PG_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PD *pi = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret;
	int err=false;
	int copies, pgwidth, pgheight;

	copies = GetInt8(pi->setup,CID_Copies,_("_Copies:"),&err);
	if ( err )
return(true);

	if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_Other)) &&
		*_GGadgetGetTitle(GWidgetGetControl(pi->setup,CID_OtherCmd))=='\0' ) {
	    ff_post_error(_("No Command Specified"),_("No Command Specified"));
return(true);
	}

	ret = _GGadgetGetTitle(GWidgetGetControl(pi->setup,CID_Pagesize));
	if ( uc_strstr(ret,"Letter")!=NULL ) {
	    pgwidth = 612; pgheight = 792;
	} else if ( uc_strstr(ret,"Legal")!=NULL ) {
	    pgwidth = 612; pgheight = 1008;
	} else if ( uc_strstr(ret,"A4")!=NULL ) {
	    pgwidth = 595; pgheight = 842;
	} else if ( uc_strstr(ret,"A3")!=NULL ) {
	    pgwidth = 842; pgheight = 1191;
	} else if ( uc_strstr(ret,"B4")!=NULL ) {
	    pgwidth = 708; pgheight = 1000;
	} else if ( uc_strstr(ret,"B5")!=NULL ) {
	    pgwidth = 516; pgheight = 728;
	} else {
	    char *cret = cu_copy(ret), *pt;
	    float pw,ph, scale;
	    if ( sscanf(cret,"%gx%g",&pw,&ph)!=2 ) {
		IError("Bad Pagesize must be a known name or <num>x<num><units>\nWhere <units> is one of pt (points), mm, cm, in" );
return( true );
	    }
	    pt = cret+strlen(cret)-1;
	    while ( isspace(*pt) ) --pt;
	    if ( strncmp(pt-2,"in",2)==0)
		scale = 72;
	    else if ( strncmp(pt-2,"cm",2)==0 )
		scale = 72/2.54;
	    else if ( strncmp(pt-2,"mm",2)==0 )
		scale = 72/25.4;
	    else if ( strncmp(pt-2,"pt",2)==0 )
		scale = 1;
	    else {
		IError("Bad Pagesize units are unknown\nMust be one of pt (points), mm, cm, in" );
return( true );
	    }
	    pgwidth = pw*scale; pgheight = ph*scale;
	    free(cret);
	}

	ret = _GGadgetGetTitle(GWidgetGetControl(pi->setup,CID_Printer));
	if ( uc_strcmp(ret,"<default>")==0 || *ret=='\0' )
	    ret = NULL;
	pi->pi.printer = cu_copy(ret);
	pi->pi.pagewidth = pgwidth; pi->pi.pageheight = pgheight;
	pi->pi.copies = copies;

	if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_lp)))
	    pi->pi.printtype = pt_lp;
	else if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_lpr)))
	    pi->pi.printtype = pt_lpr;
	else if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_ghostview)))
	    pi->pi.printtype = pt_ghostview;
	else if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_PDFFile)))
	    pi->pi.printtype = pt_pdf;
	else if ( GGadgetIsChecked(GWidgetGetControl(pi->setup,CID_Other))) {
	    pi->pi.printtype = pt_other;
	    printcommand = cu_copy(_GGadgetGetTitle(GWidgetGetControl(pi->setup,CID_OtherCmd)));
	} else
	    pi->pi.printtype = pt_file;

	printtype = pi->pi.printtype;
	free(printlazyprinter); printlazyprinter = copy(pi->pi.printer);
	pagewidth = pgwidth; pageheight = pgheight;

	pi->pi.done = true;
	SavePrefs(true);
    }
return( true );
}

static int PG_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PD *pi = GDrawGetUserData(GGadgetGetWindow(g));
	pi->pi.done = true;
    }
return( true );
}

static int PG_RadioSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PD *pi = GDrawGetUserData(GGadgetGetWindow(g));
	PG_SetEnabled(pi);
    }
return( true );
}

static int pg_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	PD *pi = GDrawGetUserData(gw);
	pi->pi.done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/display.html", NULL);
return( true );
	}
return( false );
    }
return( true );
}

static GTextInfo *PrinterList() {
    char line[400];
    FILE *printcap;
    GTextInfo *tis=NULL;
    int cnt;
    char *bpt, *cpt;

    printcap = fopen("/etc/printcap","r");
    if ( printcap==NULL ) {
	tis = calloc(2,sizeof(GTextInfo));
	tis[0].text = uc_copy("<default>");
return( tis );
    }

    while ( 1 ) {
	cnt=1;		/* leave room for default printer */
	while ( fgets(line,sizeof(line),printcap)!=NULL ) {
	    if ( !isspace(*line) && *line!='#' ) {
		if ( tis!=NULL ) {
		    bpt = strchr(line,'|');
		    cpt = strchr(line,':');
		    if ( cpt==NULL && bpt==NULL )
			cpt = line+strlen(line)-1;
		    else if ( cpt!=NULL && bpt!=NULL && bpt<cpt )
			cpt = bpt;
		    else if ( cpt==NULL )
			cpt = bpt;
		    tis[cnt].text = uc_copyn(line,cpt-line);
		}
		++cnt;
	    }
	}
	if ( tis!=NULL ) {
	    fclose(printcap);
return( tis );
	}
	tis = calloc((cnt+1),sizeof(GTextInfo));
	tis[0].text = uc_copy("<default>");
	rewind(printcap);
    }
}

static int progexists(char *prog) {
    char buffer[1025];

return( ProgramExists(prog,buffer)!=NULL );
}

static int PageSetup(PD *pi) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[17], boxes[5], *radarray[4][5], *txtarray[3][5], *barray[9], *hvarray[5][2];
    GTextInfo label[17];
    char buf[10], pb[30];
    int pt;
    /* Don't translate these. we compare against the text */
    static GTextInfo pagesizes[] = {
	{ (unichar_t *) "US Letter", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
	{ (unichar_t *) "US Legal", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
	{ (unichar_t *) "A3", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
	{ (unichar_t *) "A4", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
	{ (unichar_t *) "B4", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
	GTEXTINFO_EMPTY
    };

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Page Setup");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,250));
    pos.height = GDrawPointsToPixels(NULL,174);
    pi->setup = GDrawCreateTopWindow(NULL,&pos,pg_e_h,pi,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

/* program names also don't get translated */
    label[0].text = (unichar_t *) "lp";
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.mnemonic = 'l';
    gcd[0].gd.pos.x = 40; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.flags = progexists("lp")? (gg_visible | gg_enabled):gg_visible;
    gcd[0].gd.cid = CID_lp;
    gcd[0].gd.handle_controlevent = PG_RadioSet;
    gcd[0].creator = GRadioCreate;
    radarray[0][0] = GCD_HPad10; radarray[0][1] = &gcd[0];

    label[1].text = (unichar_t *) "lpr";
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.mnemonic = 'r';
    gcd[1].gd.pos.x = gcd[0].gd.pos.x; gcd[1].gd.pos.y = 18+gcd[0].gd.pos.y; 
    gcd[1].gd.flags = progexists("lpr")? (gg_visible | gg_enabled):gg_visible;
    gcd[1].gd.cid = CID_lpr;
    gcd[1].gd.handle_controlevent = PG_RadioSet;
    gcd[1].creator = GRadioCreate;
    radarray[1][0] = GCD_HPad10; radarray[1][1] = &gcd[1];

    use_gv = false;
    label[2].text = (unichar_t *) "ghostview";
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'g';
    gcd[2].gd.pos.x = gcd[0].gd.pos.x+50; gcd[2].gd.pos.y = gcd[0].gd.pos.y;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_rad_continueold;
    if ( !progexists("ghostview") ) {
	if ( progexists("gv") ) {
	    label[2].text = (unichar_t *) "gv";
	    use_gv = true;
	} else
	    gcd[2].gd.flags = gg_visible;
    }
    gcd[2].gd.cid = CID_ghostview;
    gcd[2].gd.handle_controlevent = PG_RadioSet;
    gcd[2].creator = GRadioCreate;
    radarray[0][2] = &gcd[2]; radarray[0][3] = GCD_ColSpan; radarray[0][4] =NULL;

    label[3].text = (unichar_t *) _("To _File");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'F';
    gcd[3].gd.pos.x = gcd[2].gd.pos.x; gcd[3].gd.pos.y = gcd[1].gd.pos.y; 
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_rad_continueold;
    gcd[3].gd.cid = CID_File;
    gcd[3].gd.handle_controlevent = PG_RadioSet;
    gcd[3].creator = GRadioCreate;
    radarray[1][2] = &gcd[3];

    label[4].text = (unichar_t *) _("To P_DF File");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.mnemonic = 'F';
    gcd[4].gd.pos.x = gcd[2].gd.pos.x+70; gcd[4].gd.pos.y = gcd[1].gd.pos.y; 
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_rad_continueold;
    gcd[4].gd.cid = CID_PDFFile;
    gcd[4].gd.handle_controlevent = PG_RadioSet;
    gcd[4].creator = GRadioCreate;
    radarray[1][3] = &gcd[4]; radarray[1][4] =NULL;

    label[5].text = (unichar_t *) _("_Other");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.mnemonic = 'O';
    gcd[5].gd.pos.x = gcd[1].gd.pos.x; gcd[5].gd.pos.y = 22+gcd[1].gd.pos.y; 
    gcd[5].gd.flags = gg_visible | gg_enabled| gg_rad_continueold;
    gcd[5].gd.cid = CID_Other;
    gcd[5].gd.handle_controlevent = PG_RadioSet;
    gcd[5].gd.popup_msg = _("Any other command with all its arguments.\nThe command must expect to deal with a postscript\nfile which it will find by reading its standard input.");
    gcd[5].creator = GRadioCreate;
    radarray[2][0] = GCD_HPad10; radarray[2][1] = &gcd[5];

    if ( (pt=pi->pi.printtype)==pt_unknown ) pt = pt_lp;
    if ( pt==pt_pdf ) pt = 4;		/* These two are out of order */
    else if ( pt==pt_other ) pt = 5;
    if ( !(gcd[pt].gd.flags&gg_enabled) ) pt = pt_file;		/* always enabled */
    gcd[pt].gd.flags |= gg_cb_on;

    label[6].text = (unichar_t *) (printcommand?printcommand:"");
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.mnemonic = 'O';
    gcd[6].gd.pos.x = gcd[2].gd.pos.x; gcd[6].gd.pos.y = gcd[5].gd.pos.y-4;
    gcd[6].gd.pos.width = 120;
    gcd[6].gd.flags = gg_visible | gg_enabled;
    gcd[6].gd.cid = CID_OtherCmd;
    gcd[6].creator = GTextFieldCreate;
    radarray[2][2] = &gcd[6]; radarray[2][3] = GCD_ColSpan; radarray[2][4] =NULL;
    radarray[3][0] = NULL;

    label[7].text = (unichar_t *) _("Page_Size:");
    label[7].text_is_1byte = true;
    label[7].text_in_resource = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.mnemonic = 'S';
    gcd[7].gd.pos.x = 5; gcd[7].gd.pos.y = 24+gcd[5].gd.pos.y+6; 
    gcd[7].gd.flags = gg_visible | gg_enabled;
    gcd[7].creator = GLabelCreate;
    txtarray[0][0] = &gcd[7];

    if ( pi->pi.pagewidth==595 && pi->pi.pageheight==792 )
	strcpy(pb,"US Letter");		/* Pick a name, this is the default case */
    else if ( pi->pi.pagewidth==612 && pi->pi.pageheight==792 )
	strcpy(pb,"US Letter");
    else if ( pi->pi.pagewidth==612 && pi->pi.pageheight==1008 )
	strcpy(pb,"US Legal");
    else if ( pi->pi.pagewidth==595 && pi->pi.pageheight==842 )
	strcpy(pb,"A4");
    else if ( pi->pi.pagewidth==842 && pi->pi.pageheight==1191 )
	strcpy(pb,"A3");
    else if ( pi->pi.pagewidth==708 && pi->pi.pageheight==1000 )
	strcpy(pb,"B4");
    else
	sprintf(pb,"%dx%d mm", (int) (pi->pi.pagewidth*25.4/72),(int) (pi->pi.pageheight*25.4/72));
    label[8].text = (unichar_t *) pb;
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.mnemonic = 'S';
    gcd[8].gd.pos.x = 60; gcd[8].gd.pos.y = gcd[7].gd.pos.y-6;
    gcd[8].gd.pos.width = 90;
    gcd[8].gd.flags = gg_visible | gg_enabled;
    gcd[8].gd.cid = CID_Pagesize;
    gcd[8].gd.u.list = pagesizes;
    gcd[8].creator = GListFieldCreate;
    txtarray[0][1] = &gcd[8];


    label[9].text = (unichar_t *) _("_Copies:");
    label[9].text_is_1byte = true;
    label[9].text_in_resource = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.mnemonic = 'C';
    gcd[9].gd.pos.x = 160; gcd[9].gd.pos.y = gcd[7].gd.pos.y; 
    gcd[9].gd.flags = gg_visible | gg_enabled;
    gcd[9].gd.cid = CID_CopiesLab;
    gcd[9].creator = GLabelCreate;
    txtarray[0][2] = &gcd[9];

    sprintf(buf,"%d",pi->pi.copies);
    label[10].text = (unichar_t *) buf;
    label[10].text_is_1byte = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.mnemonic = 'C';
    gcd[10].gd.pos.x = 200; gcd[10].gd.pos.y = gcd[8].gd.pos.y;
    gcd[10].gd.pos.width = 40;
    gcd[10].gd.flags = gg_visible | gg_enabled;
    gcd[10].gd.cid = CID_Copies;
    gcd[10].creator = GTextFieldCreate;
    txtarray[0][3] = &gcd[10]; txtarray[0][4] = NULL;


    label[11].text = (unichar_t *) _("_Printer:");
    label[11].text_is_1byte = true;
    label[11].text_in_resource = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.mnemonic = 'P';
    gcd[11].gd.pos.x = 5; gcd[11].gd.pos.y = 30+gcd[7].gd.pos.y+6; 
    gcd[11].gd.flags = gg_visible | gg_enabled;
    gcd[11].gd.cid = CID_PrinterLab;
    gcd[11].creator = GLabelCreate;
    txtarray[1][0] = &gcd[11];

    label[12].text = (unichar_t *) pi->pi.printer;
    label[12].text_is_1byte = true;
    if ( pi->pi.printer!=NULL )
	gcd[12].gd.label = &label[12];
    gcd[12].gd.mnemonic = 'P';
    gcd[12].gd.pos.x = 60; gcd[12].gd.pos.y = gcd[11].gd.pos.y-6;
    gcd[12].gd.pos.width = 90;
    gcd[12].gd.flags = gg_visible | gg_enabled;
    gcd[12].gd.cid = CID_Printer;
    gcd[12].gd.u.list = PrinterList();
    gcd[12].creator = GListFieldCreate;
    txtarray[1][1] = &gcd[12];
    txtarray[1][2] = GCD_ColSpan; txtarray[1][3] = GCD_Glue; txtarray[1][4] = NULL;
    txtarray[2][0] = NULL;


    gcd[13].gd.pos.x = 30-3; gcd[13].gd.pos.y = gcd[12].gd.pos.y+36;
    gcd[13].gd.pos.width = -1; gcd[13].gd.pos.height = 0;
    gcd[13].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[13].text = (unichar_t *) _("_OK");
    label[13].text_is_1byte = true;
    label[13].text_in_resource = true;
    gcd[13].gd.mnemonic = 'O';
    gcd[13].gd.label = &label[13];
    gcd[13].gd.handle_controlevent = PG_OK;
    gcd[13].creator = GButtonCreate;

    gcd[14].gd.pos.x = -30; gcd[14].gd.pos.y = gcd[13].gd.pos.y+3;
    gcd[14].gd.pos.width = -1; gcd[14].gd.pos.height = 0;
    gcd[14].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[14].text = (unichar_t *) _("_Cancel");
    label[14].text_is_1byte = true;
    label[14].text_in_resource = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.mnemonic = 'C';
    gcd[14].gd.handle_controlevent = PG_Cancel;
    gcd[14].creator = GButtonCreate;

    barray[0] = barray[2] = barray[3] = barray[4] = barray[6] = GCD_Glue; barray[7] = NULL;
    barray[1] = &gcd[13]; barray[5] = &gcd[14];

    memset(boxes,0,sizeof(boxes));

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = radarray[0];
    boxes[2].creator = GHVBoxCreate;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = txtarray[0];
    boxes[3].creator = GHVBoxCreate;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = barray;
    boxes[4].creator = GHBoxCreate;

    hvarray[0][0] = &boxes[2]; hvarray[0][1] = NULL;
    hvarray[1][0] = &boxes[3]; hvarray[1][1] = NULL;
    hvarray[2][0] = GCD_Glue; hvarray[2][1] = NULL;
    hvarray[3][0] = &boxes[4]; hvarray[3][1] = NULL;
    hvarray[4][0] = NULL;
    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(pi->setup,boxes);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GTextInfoListFree(gcd[12].gd.u.list);
    PG_SetEnabled(pi);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(pi->setup,true);
    while ( !pi->pi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(pi->setup);
    pi->pi.done = false;
return( pi->pi.printtype!=pt_unknown );
}

/* ************************************************************************** */
/* ************************* Code for Print dialog ************************** */
/* ************************************************************************** */

/* Slightly different from one in fontview */
static int FVSelCount(FontView *fv) {
    int i, cnt=0, gid;
    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1 &&
		SCWorthOutputting(fv->b.sf->glyphs[gid]))
	    ++cnt;
return( cnt);
}

    /* CIDs for Print */
#define CID_TabSet	1000
#define CID_Display	1001
#define CID_Chars	1002
#define	CID_MultiSize	1003
#define CID_PSLab	1005
#define	CID_PointSize	1006
#define CID_OK		1009
#define CID_Cancel	1010
#define CID_Setup	1010

    /* CIDs for display */
#define CID_Font	2001
#define CID_AA		2002
#define CID_SizeLab	2003
#define CID_Size	2004
#define CID_pfb		2005
#define CID_ttf		2006
#define CID_otf		2007
#define CID_nohints	2008
#define CID_bitmap	2009
#define CID_pfaedit	2010
#define CID_SampleText	2011
#define CID_ScriptLang	2022
#define CID_Features	2023
#define CID_DPI		2024
#define CID_TopBox	2025

    /* CIDs for Insert Text */
#define CID_Bind	3001
#define CID_Scale	3002
#define CID_Start	3003
#define CID_Center	3004
#define CID_End		3005
#define CID_TextWidth	3006
#define CID_YOffset	3007
#define CID_GlyphAsUnit	3008
#define CID_ActualWidth	3009

static void PRT_SetEnabled(PD *pi) {
    int enable_ps;

    enable_ps = !GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Chars));

    GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_PSLab),enable_ps);
    GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_PointSize),enable_ps);
}

static int PRT_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PD *pi = GDrawGetUserData(GGadgetGetWindow(g));
	int err = false;
	int di = pi->pi.fv!=NULL?0:pi->pi.mv!=NULL?2:1;
	char *ret;
	char *file;
	char buf[100];

	if ( pi->insert_text ) {
	    SplineSet *ss, *end;
	    int bound = GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Bind));
	    int scale = GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Scale));
	    int gunit = GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_GlyphAsUnit));
	    int align = GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Start ))? 0 :
			GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Center))? 1 : 2;
	    int width = GetInt8(pi->gw,CID_TextWidth,_("Width"),&err);
	    real offset = GetReal8(pi->gw,CID_YOffset,_("Offset"),&err);
	    CharView *cv = pi->cv;
	    LayoutInfo *sample;
	    /* int dpi  =*/ GetInt8(pi->gw,CID_DPI,_("DPI"),&err);
	    /* int size =*/ GetInt8(pi->gw,CID_Size,_("Size"),&err);
	    if ( err )
return(true);
	    free(old_bind_text);
	    old_bind_text = GGadgetGetTitle(GWidgetGetControl(pi->gw,CID_SampleText));
	    sample = LIConvertToPrint(
			&((SFTextArea *) GWidgetGetControl(pi->gw,CID_SampleText))->li,
			width, 50000, 72 );
	    ss = LIConvertToSplines(sample, 72,cv->b.layerheads[cv->b.drawmode]->order2);
	    LayoutInfo_Destroy(sample);
	    free(sample);
	    if ( bound && pi->fit_to_path )
		SplineSetBindToPath(ss,scale,gunit,align,offset,pi->fit_to_path);
	    if ( ss ) {
		SplineSet *test;
		CVPreserveState((CharViewBase *) cv);
		end = NULL;
		for ( test=ss; test!=NULL; test=test->next ) {
		    SplinePoint *sp;
		    end = test;
		    for ( sp=test->first; ; ) {
			sp->selected = true;
			if ( sp->next==NULL )
		    break;
			sp = sp->next->to;
			if ( sp==test->first )
		    break;
		    }
		}
		end->next = cv->b.layerheads[cv->b.drawmode]->splines;
		cv->b.layerheads[cv->b.drawmode]->splines = ss;
		CVCharChangedUpdate((CharViewBase *) cv);
	    }
	} else {
	    pi->pi.pt = GTabSetGetSel(GWidgetGetControl(pi->gw,CID_TabSet))==0 ? pt_fontsample :
		       GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Chars))? pt_chars:
		       GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_MultiSize))? pt_multisize:
		       pt_fontdisplay;
	    if ( pi->pi.pt==pt_fontdisplay ) {
		pi->pi.pointsize = GetInt8(pi->gw,CID_PointSize,_("_Pointsize:"),&err);
		if ( err )
return(true);
		if ( pi->pi.pointsize<1 || pi->pi.pointsize>200 ) {
		    ff_post_error(_("Invalid point size"),_("Invalid point size"));
return(true);
		}
	    }
	    if ( pi->pi.printtype==pt_unknown )
		if ( !PageSetup(pi))
return(true);

	    if ( pi->pi.printtype==pt_file || pi->pi.printtype==pt_pdf ) {
		sprintf(buf,"pr-%.90s.%s", pi->pi.mainsf->fontname,
			pi->pi.printtype==pt_file?"ps":"pdf");
		ret = gwwv_save_filename(_("Print To File..."),buf,
			pi->pi.printtype==pt_pdf?"*.pdf":"*.ps");
		if ( ret==NULL )
return(true);
		file = utf82def_copy(ret);
		free(ret);
		pi->pi.out = fopen(file,"wb");
		if ( pi->pi.out==NULL ) {
		    ff_post_error(_("Print Failed"),_("Failed to open file %s for output"), file);
		    free(file);
return(true);
		}
	    } else {
		file = NULL;
		pi->pi.out = GFileTmpfile();
		if ( pi->pi.out==NULL ) {
		    ff_post_error(_("Failed to open temporary output file"),_("Failed to open temporary output file"));
return(true);
		}
	    }

	    pdefs[di].last_cs = pi->pi.mainmap->enc;
	    pdefs[di].pt = pi->pi.pt;
	    pdefs[di].pointsize = pi->pi.pointsize;

	    if ( pi->pi.pt==pt_fontsample ) {
		pi->pi.sample = LIConvertToPrint(
			&((SFTextArea *) GWidgetGetControl(pi->gw,CID_SampleText))->li,
			(pagewidth-1*72)*printdpi/72,
			(pageheight-1*72)*printdpi/72,
			printdpi);
	    }

	    DoPrinting(&pi->pi,file);
	    free(file);
	    if ( pi->pi.pt==pt_fontsample ) {
		LayoutInfo_Destroy(pi->pi.sample);
		free(pi->pi.sample);
	    }
	}

	if ( pi->done!=NULL )
	    *(pi->done) = true;
	GDrawDestroyWindow(pi->gw);
    }
return( true );
}

static int PRT_Setup(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PD *pi = GDrawGetUserData(GGadgetGetWindow(g));
	PageSetup(pi);
    }
return( true );
}

static int PRT_RadioSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PD *pi = GDrawGetUserData(GGadgetGetWindow(g));
	PRT_SetEnabled(pi);
    }
return( true );
}

/* ************************************************************************** */
/* ************************ Code for Display dialog ************************* */
/* ************************************************************************** */

static void TextInfoDataFree(GTextInfo *ti) {
    int i;

    if ( ti==NULL )
return;
    for ( i=0; ti[i].text!=NULL || ti[i].line ; ++i )
	free(ti[i].userdata);
    GTextInfoListFree(ti);
}

static GTextInfo *FontNames(SplineFont *cur_sf, int insert_text) {
    int cnt;
    FontView *fv;
    SplineFont *sf;
    GTextInfo *ti;
    int selected = false, any_other=-1;

    for ( fv=fv_list, cnt=0; fv!=NULL; fv=(FontView *) (fv->b.next) )
	if ( (FontView *) (fv->b.nextsame)==NULL )
	    ++cnt;
    ti = calloc(cnt+1,sizeof(GTextInfo));
    for ( fv=fv_list, cnt=0; fv!=NULL; fv=(FontView *) (fv->b.next) ) {
	if ( (FontView *) (fv->b.nextsame)==NULL ) {
	    sf = fv->b.sf;
	    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	    ti[cnt].text = uc_copy(sf->fontname);
	    ti[cnt].userdata = sf;
	    /* In the print dlg, use the current font */
	    /* In the insert_text dlg, use anything other than the current */
	    if ( sf==cur_sf && !insert_text ) {
		ti[cnt].selected = true;
		selected = true;
	    } else if ( cur_sf!=sf && !selected && insert_text ) {
		if ( cur_sf->new )
		    any_other = cnt;
		else {
		    ti[cnt].selected = true;
		    selected = true;
		}
	    }
	    ++cnt;
	}
    }
    if ( !selected && any_other!=-1 )
	ti[any_other].selected = true;
    else if ( !selected )
	ti[0].selected = true;
return( ti );
}

static BDFFont *DSP_BestMatch(SplineFont *sf,int aa,int size) {
    BDFFont *bdf, *sizem=NULL;
    int a;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	if ( bdf->clut==NULL && !aa )
	    a = 4;
	else if ( bdf->clut!=NULL && aa ) {
	    if ( bdf->clut->clut_len==256 )
		a = 4;
	    else if ( bdf->clut->clut_len==16 )
		a = 3;
	    else
		a = 2;
	} else
	    a = 1;
	if ( bdf->pixelsize==size && a==4 )
return( bdf );
	if ( sizem==NULL )
	    sizem = bdf;
	else {
	    int sdnew = bdf->pixelsize-size, sdold = sizem->pixelsize-size;
	    if ( sdnew<0 ) sdnew = -sdnew;
	    if ( sdold<0 ) sdold = -sdold;
	    if ( sdnew<sdold )
		sizem = bdf;
	    else if ( sdnew==sdold ) {
		int olda;
		if ( sizem->clut==NULL && !aa )
		    olda = 4;
		else if ( sizem->clut!=NULL && aa ) {
		    if ( sizem->clut->clut_len==256 )
			olda = 4;
		    else if ( sizem->clut->clut_len==16 )
			olda = 3;
		    else
			olda = 2;
		} else
		    olda = 1;
		if ( a>olda )
		    sizem = bdf;
	    }
	}
    }
return( sizem );	
}

static BDFFont *DSP_BestMatchDlg(PD *di) {
    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
    SplineFont *sf;
    int val;
    unichar_t *end;

    if ( sel==NULL )
return( NULL );
    sf = sel->userdata;
    val = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),&end,10);
    if ( *end!='\0' || val<4 )
return( NULL );

return( DSP_BestMatch(sf,GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA)),val) );
}

static enum sftf_fonttype DSP_FontType(PD *di) {
    int type;
    type = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_pfb))? sftf_pfb :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_ttf))? sftf_ttf :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_otf))? sftf_otf :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_nohints))? sftf_nohints :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_pfaedit))? sftf_pfaedit :
	   sftf_bitmap;
return( type );
}

static void DSP_SetFont(PD *di,int doall) {
    unichar_t *end;
    int size = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),&end,10);
    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
    SplineFont *sf;
    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
    int type;
    SFTextArea *g;
    int layer;

    if ( sel==NULL || *end )
return;
    sf = sel->userdata;

    type = DSP_FontType(di);

    g = (SFTextArea *) GWidgetGetControl(di->gw,CID_SampleText);
    layer = di->fv!=NULL && di->fv->b.sf==sf ? di->fv->b.active_layer : ly_fore;
    if ( !LI_SetFontData( &g->li, doall?0:-1,-1,
	    sf,layer,type,size,aa,g->g.inner.width))
	ff_post_error(_("Bad Font"),_("Bad Font"));
}

static void DSP_ChangeFontCallback(void *context,SplineFont *sf,enum sftf_fonttype type,
	int size, int aa, uint32 script, uint32 lang, uint32 *feats) {
    PD *di = context;
    char buf[16];
    int i,j,cnt;
    uint32 *tags;
    GTextInfo **ti;

    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),aa);

    sprintf(buf,"%d",size);
    GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_Size),buf);

    {
	GTextInfo **ti;
	int i,set; int32 len;
	ti = GGadgetGetList(GWidgetGetControl(di->gw,CID_Font),&len);
	for ( i=0; i<len; ++i )
	    if ( ti[i]->userdata == sf )
	break;
	if ( i<len ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(di->gw,CID_Font),i);
	    /*GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Font),ti[i]->text);*/
	}
	set = hasFreeType() && !sf->onlybitmaps && sf->subfontcnt==0 &&
		!sf->strokedfont && !sf->multilayer;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_pfb),set);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_ttf),set);
	set = hasFreeType() && !sf->onlybitmaps &&
		!sf->strokedfont && !sf->multilayer;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_otf),set);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_nohints),hasFreeType());
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_bitmap),sf->bitmaps!=NULL);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_pfaedit),!sf->onlybitmaps);
    }

    if ( type==sftf_pfb )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfb),true);
    else if ( type==sftf_ttf )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_ttf),true);
    else if ( type==sftf_otf )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_otf),true);
    else if ( type==sftf_nohints )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_nohints),true);
    else if ( type==sftf_pfaedit )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfaedit),true);
    else
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_bitmap),true);

    if ( script==0 ) script = DEFAULT_SCRIPT;
    if ( lang  ==0 ) lang   = DEFAULT_LANG;
    buf[0] = script>>24; buf[1] = script>>16; buf[2] = script>>8; buf[3] = script;
    buf[4] = '{';
    buf[5] = lang>>24; buf[6] = lang>>16; buf[7] = lang>>8; buf[8] = lang;
    buf[9] = '}';
    buf[10] = '\0';
    GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_ScriptLang),buf);

    tags = SFFeaturesInScriptLang(sf,-2,script,lang);
    if ( tags[0]==0 ) {
	free(tags);
	tags = SFFeaturesInScriptLang(sf,-2,script,DEFAULT_LANG);
    }
    for ( cnt=0; tags[cnt]!=0; ++cnt );
    if ( feats!=NULL )
	for ( i=0; feats[i]!=0; ++i ) {
	    for ( j=0; tags[j]!=0; ++j ) {
		if ( feats[i]==tags[j] )
	    break;
	    }
	    if ( tags[j]==0 )
		++cnt;
	}
    ti = malloc((cnt+2)*sizeof(GTextInfo *));
    for ( i=0; tags[i]!=0; ++i ) {
	ti[i] = calloc( 1,sizeof(GTextInfo));
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	if ( (tags[i]>>24)<' ' || (tags[i]>>24)>0x7e )
	    sprintf( buf, "<%d,%d>", tags[i]>>16, tags[i]&0xffff );
	else {
	    buf[0] = tags[i]>>24; buf[1] = tags[i]>>16; buf[2] = tags[i]>>8; buf[3] = tags[i]; buf[4] = 0;
	}
	ti[i]->text = uc_copy(buf);
	ti[i]->userdata = (void *) (intpt) tags[i];
	if ( feats!=NULL ) {
	    for ( j=0; feats[j]!=0; ++j ) {
		if ( feats[j] == tags[i] ) {
		    ti[i]->selected = true;
	    break;
		}
	    }
	}
    }
    cnt = i;
    if ( feats!=NULL )
	for ( i=0; feats[i]!=0; ++i ) {
	    for ( j=0; tags[j]!=0; ++j ) {
		if ( feats[i]==tags[j] )
	    break;
	    }
	    if ( tags[j]==0 ) {
		ti[cnt] = calloc( 1,sizeof(GTextInfo));
		ti[cnt]->bg = COLOR_DEFAULT;
		ti[cnt]->fg = COLOR_CREATE(0x70,0x70,0x70);
		ti[cnt]->selected = true;
		buf[0] = feats[i]>>24; buf[1] = feats[i]>>16; buf[2] = feats[i]>>8; buf[3] = feats[i]; buf[4] = 0;
		ti[cnt]->text = uc_copy(buf);
		ti[cnt++]->userdata = (void *) (intpt) feats[i];
	    }
	}
    ti[cnt] = calloc(1,sizeof(GTextInfo));
    /* These will become ordered because the list widget will do that */
    GGadgetSetList(GWidgetGetControl(di->gw,CID_Features),ti,false);
    free(tags);
}

static int DSP_AAState(SplineFont *sf,BDFFont *bestbdf) {
    /* What should AntiAlias look like given the current set of bitmaps */
    int anyaa=0, anybit=0;
    BDFFont *bdf;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	if ( bdf->clut==NULL )
	    anybit = true;
	else
	    anyaa = true;
    if ( anybit && anyaa )
return( gg_visible | gg_enabled | (bestbdf!=NULL && bestbdf->clut!=NULL ? gg_cb_on : 0) );
    else if ( anybit )
return( gg_visible );
    else if ( anyaa )
return( gg_visible | gg_cb_on );

return( gg_visible );
}

static int DSP_FontChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	GTextInfo *sel = GGadgetGetListItemSelected(g);
	SplineFont *sf;
	BDFFont *best;
	int flags, pick = 0, i;
	char size[12]; unichar_t usize[12];
	uint16 cnt;

	if ( sel==NULL )
return( true );
	sf = sel->userdata;

	TextInfoDataFree(di->scriptlangs);
	di->scriptlangs = SLOfFont(sf);
	GGadgetSetList(GWidgetGetControl(di->gw,CID_ScriptLang),
		GTextInfoArrayFromList(di->scriptlangs,&cnt),false);

	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) && sf->bitmaps==NULL )
	    pick = true;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_bitmap),sf->bitmaps!=NULL);
	if ( !hasFreeType() || sf->onlybitmaps ) {
	    best = DSP_BestMatchDlg(di);
	    flags = DSP_AAState(sf,best);
	    GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),flags&gg_enabled);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),flags&gg_cb_on);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_bitmap),true);
	    for ( i=CID_pfb; i<=CID_nohints; ++i )
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),false);
	    if ( best!=NULL ) {
		sprintf( size, "%d", best->pixelsize );
		uc_strcpy(usize,size);
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
	    }
	    pick = true;
	} else if ( sf->subfontcnt!=0 ) {
	    for ( i=CID_pfb; i<CID_otf; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),false);
		if ( GGadgetIsChecked(GWidgetGetControl(di->gw,i)) )
		    pick = true;
	    }
	    GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_otf),true);
	    if ( pick )
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_otf),true);
	} else {
	    for ( i=CID_pfb; i<=CID_otf; ++i )
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),true);
	    if ( pick )
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfb),true);
	}
	if ( pick )
	    DSP_SetFont(di,false);
	else
	    SFTFSetFont(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,sf);
    }
return( true );
}

static int DSP_AAChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    int val = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),NULL,10);
	    int bestdiff = 8000, bdfdiff;
	    BDFFont *bdf, *best=NULL;
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
	    if ( sel==NULL )
return( true );
	    sf = sel->userdata;
	    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( (aa && bdf->clut) || (!aa && bdf->clut==NULL)) {
		    if ((bdfdiff = bdf->pixelsize-val)<0 ) bdfdiff = -bdfdiff;
		    if ( bdfdiff<bestdiff ) {
			best = bdf;
			bestdiff = bdfdiff;
			if ( bdfdiff==0 )
	    break;
		    }
		}
	    }
	    if ( best!=NULL ) {
		char size[12]; unichar_t usize[12];
		sprintf( size, "%d", best->pixelsize );
		uc_strcpy(usize,size);
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
	    }
	    DSP_SetFont(di,false);
	} else
	    SFTFSetAntiAlias(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,
		    GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA)));
    }
return( true );
}

static int DSP_SizeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    !e->u.control.u.tf_focus.gained_focus ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int size = GetInt8(di->gw,CID_Size,_("_Size:"),&err);
	if ( err || size<4 )
return( true );
	if ( GWidgetGetControl(di->gw,CID_SampleText)==NULL )
return( true );		/* Happens during startup */
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    BDFFont *bdf, *best=NULL;
	    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
	    if ( sel==NULL )
return( true );
	    sf = sel->userdata;
	    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( bdf->pixelsize==size ) {
		    if (( bdf->clut && aa ) || ( bdf->clut==NULL && !aa )) {
			best = bdf;
	    break;
		    }
		    best = bdf;
		}
	    }
	    if ( best==NULL ) {
		char buf[100], *pt=buf, *end=buf+sizeof(buf)-10;
		unichar_t ubuf[12];
		int lastsize = 0;
		for ( bdf=sf->bitmaps; bdf!=NULL && pt<end; bdf=bdf->next ) {
		    if ( bdf->pixelsize!=lastsize ) {
			sprintf( pt, "%d,", bdf->pixelsize );
			lastsize = bdf->pixelsize;
			pt += strlen(pt);
		    }
		}
		if ( pt!=buf )
		    pt[-1] = '\0';
		ff_post_error(_("Bad Size"),_("Requested bitmap size not available in font. Font supports %s"),buf);
		best = DSP_BestMatchDlg(di);
		if ( best!=NULL ) {
		    sprintf( buf, "%d", best->pixelsize);
		    uc_strcpy(ubuf,buf);
		    GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),ubuf);
		    size = best->pixelsize;
		}
	    }
	    if ( best==NULL )
return(true);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),best->clut!=NULL );
	    DSP_SetFont(di,false);
	} else
	    SFTFSetSize(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,size);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/* Don't change the font on each change to the text field, that might */
	/*  look rather odd. But wait until we think they may have finished */
	/*  typing. Either when they change the focus (above) or when they */
	/*  just don't do anything for a while */
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( di->sizechanged!=NULL )
	    GDrawCancelTimer(di->sizechanged);
	di->sizechanged = GDrawRequestTimer(di->gw,600,0,NULL);
    }
return( true );
}

static void DSP_SizeChangedTimer(PD *di) {
    GEvent e;

    di->sizechanged = NULL;
    memset(&e,0,sizeof(e));
    e.type = et_controlevent;
    e.u.control.g = GWidgetGetControl(di->gw,CID_Size);
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.u.tf_focus.gained_focus = false;
    DSP_SizeChanged(e.u.control.g,&e);
}


static int DSP_WidthChanged(GGadget *g, GEvent *e) {
    if ( e==NULL ||
	    (e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	     !e->u.control.u.tf_focus.gained_focus )) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int width = GetInt8(di->gw,CID_TextWidth,_("Width"),&err);
	GGadget *sample = GWidgetGetControl(di->gw,CID_SampleText);
	GRect outer;
	int bp;
	if ( err || width<20 || width>2000 )
return( true );
	if ( !di->ready )
return( true );
	bp = GBoxBorderWidth(di->gw,sample->box);
	GGadgetGetSize(sample,&outer);
	outer.width = rint(width*lastdpi/72.0)+2*bp;
	GGadgetSetDesiredSize(sample,&outer,NULL);
	GHVBoxFitWindow(GWidgetGetControl(di->gw,CID_TopBox));
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/* Don't change the font on each change to the text field, that might */
	/*  look rather odd. But wait until we think they may have finished */
	/*  typing. Either when they change the focus (above) or when they */
	/*  just don't do anything for a while */
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( di->widthchanged!=NULL )
	    GDrawCancelTimer(di->widthchanged);
	di->widthchanged = GDrawRequestTimer(di->gw,600,0,NULL);
    }
return( true );
}

static void DSP_WidthChangedTimer(PD *di) {

    di->widthchanged = NULL;
    DSP_WidthChanged(GWidgetGetControl(di->gw,CID_TextWidth),NULL);
}

static void DSP_JustResized(PD *di) {
    GGadget *sample = GWidgetGetControl(di->gw,CID_SampleText);
    GGadget *widthfield = GWidgetGetControl(di->gw,CID_TextWidth);
    char buffer[20];

    di->resized = NULL;
    if ( lastdpi!=0 && widthfield!=NULL ) {
	sprintf( buffer, "%d", (int) rint( sample->inner.width*72/lastdpi ));
	GGadgetSetTitle8(widthfield,buffer);
    }
}

static int DSP_DpiChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    !e->u.control.u.tf_focus.gained_focus ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int dpi = GetInt8(di->gw,CID_DPI,_("DPI"),&err);
	GGadget *sample = GWidgetGetControl(di->gw,CID_SampleText);
	GGadget *widthfield = GWidgetGetControl(di->gw,CID_TextWidth);
	if ( err || dpi<20 || dpi>300 )
return( true );
	if ( !di->ready )
return( true );		/* Happens during startup */
	if ( lastdpi==dpi )
return( true );
	SFTFSetDPI(sample,dpi);
	lastdpi = dpi;
	if ( widthfield!=NULL ) {
	    DSP_WidthChanged(widthfield,NULL);
	} else {
	    GGadgetRedraw(sample);
	}
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/* Don't change the font on each change to the text field, that might */
	/*  look rather odd. But wait until we think they may have finished */
	/*  typing. Either when they change the focus (above) or when they */
	/*  just don't do anything for a while */
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( di->dpichanged!=NULL )
	    GDrawCancelTimer(di->dpichanged);
	di->dpichanged = GDrawRequestTimer(di->gw,600,0,NULL);
    }
return( true );
}

static void DSP_DpiChangedTimer(PD *di) {
    GEvent e;

    di->dpichanged = NULL;
    memset(&e,0,sizeof(e));
    e.type = et_controlevent;
    e.u.control.g = GWidgetGetControl(di->gw,CID_Size);
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.u.tf_focus.gained_focus = false;
    DSP_DpiChanged(e.u.control.g,&e);
}

static int DSP_Refresh(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *sample = GWidgetGetControl(di->gw,CID_SampleText);
	GGadget *fontnames = GWidgetGetControl(di->gw,CID_Font);
	GTextInfo *sel = GGadgetGetListItemSelected(fontnames);
	GTextInfo *fn;

	SFTFRefreshFonts(sample);		/* Clear any font info and get new font maps, etc. */
	SFTFProvokeCallback(sample);		/* Refresh the feature list too */

	/* One thing we don't have to worry about is a font being removed */
	/*  if that happens we just close this window. Too hard to fix up for */
	/* But a font might be added... */
	fn = FontNames(sel!=NULL? (SplineFont *) (sel->userdata) : di->pi.mainsf, di->insert_text );
	GGadgetSetList(fontnames,GTextInfoArrayFromList(fn,NULL),false);
	GGadgetSetEnabled(fontnames,fn[1].text!=NULL);
	GTextInfoListFree(fn);
    }
return( true );
}

static int DSP_RadioSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    BDFFont *best = DSP_BestMatchDlg(di);
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    int flags;
	    char size[14]; unichar_t usize[14];

	    if ( sel!=NULL ) {
		sf = sel->userdata;
		flags = DSP_AAState(sf,best);
		GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),flags&gg_enabled);
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),flags&gg_cb_on);
		if ( best!=NULL ) {
		    sprintf( size, "%g",
			    rint(best->pixelsize*72.0/SFTFGetDPI(GWidgetGetControl(di->gw,CID_SampleText))) );
		    uc_strcpy(usize,size);
		    GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
		}
	    }
	    DSP_SetFont(di,false);
	} else {
	    /*GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),true);*/
	    SFTFSetFontType(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,
		    DSP_FontType(di));
	}
    }
return( true );
}

static int DSP_ScriptLangChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *sstr = _GGadgetGetTitle(g);
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	uint32 script, lang;

	if ( e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	    GGadgetSetTitle8(g,di->scriptlangs[e->u.control.u.tf_changed.from_pulldown].userdata );
	    sstr = _GGadgetGetTitle(g);
	} else {
	    if ( u_strlen(sstr)<4 || !isalpha(sstr[0]) || !isalnum(sstr[1]) /*|| !isalnum(sstr[2]) || !isalnum(sstr[3])*/ )
return( true );
	    if ( u_strlen(sstr)==4 )
		/* No language, we'll use default */;
	    else if ( u_strlen(sstr)!=10 || sstr[4]!='{' || sstr[9]!='}' ||
		    !isalpha(sstr[5]) || !isalpha(sstr[6]) || !isalpha(sstr[7])  )
return( true );
	}
	script = DEFAULT_SCRIPT;
	lang = DEFAULT_LANG;
	if ( u_strlen(sstr)>=4 )
	    script = (sstr[0]<<24) | (sstr[1]<<16) | (sstr[2]<<8) | sstr[3];
	if ( sstr[4]=='{' && u_strlen(sstr)>=9 )
	    lang = (sstr[5]<<24) | (sstr[6]<<16) | (sstr[7]<<8) | sstr[8];
	SFTFSetScriptLang(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,script,lang);
    }
return( true );
}

static int DSP_Menu(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonpress ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *st = GWidgetGetControl(di->gw,CID_SampleText);
	GEvent fudge;
	GPoint p;
	memset(&fudge,0,sizeof(fudge));
	fudge.type = et_mousedown;
	fudge.w = st->base;
	p.x = g->inner.x+g->inner.width/2;
	p.y = g->inner.y+g->inner.height;
	GDrawTranslateCoordinates(g->base,st->base,&p);
	fudge.u.mouse.x = p.x; fudge.u.mouse.y = p.y;
	SFTFPopupMenu((SFTextArea *) st,&fudge);
    }
return( true );
}

static int DSP_FeaturesChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	uint32 *feats;
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	int i,cnt;

	for ( i=cnt=0; i<len; ++i )
	    if ( ti[i]->selected ) ++cnt;
	feats = malloc((cnt+1)*sizeof(uint32));
	for ( i=cnt=0; i<len; ++i )
	    if ( ti[i]->selected )
		feats[cnt++] = (intpt) ti[i]->userdata;
	feats[cnt] = 0;
	/* These will be ordered because the list widget will do that */
	SFTFSetFeatures(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,feats);
    }
return( true );
}

static int PRT_Bind(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PD *pi = GDrawGetUserData(GGadgetGetWindow(g));
	int on = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_Scale),on);
	GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_Start),on);
	GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_Center),on);
	GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_End),on);
    }
return( true );
}

static int DSP_TextChanged(GGadget *g, GEvent *e) {
    if ( e==NULL ||
	    (e->type==et_controlevent && e->u.control.subtype == et_textchanged )) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *txt = _GGadgetGetTitle(g);
	const unichar_t *pt;
	SFTextArea *ta = (SFTextArea *) g;
	LayoutInfo *li = &ta->li;
	char buffer[200];

	for ( pt=txt; *pt!='\0' && ScriptFromUnicode(*pt,NULL)==DEFAULT_SCRIPT; ++pt);
	if ( *pt=='\0' ) {
	    if ( !di->script_unknown ) {
		di->script_unknown = true;
		if ( li->fontlist!=NULL ) {
		    li->fontlist->script = DEFAULT_SCRIPT;
		    li->fontlist->lang   = DEFAULT_LANG;
		}
		GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_ScriptLang),"DFLT{dflt}");
	    }
	} else if ( di->script_unknown ) {
	    uint32 script = ScriptFromUnicode(*pt,NULL);
	    struct fontlist *fl;
	    unichar_t buf[20];
	    for ( fl=li->fontlist; fl!=NULL && ta->sel_start>fl->end; fl=fl->next );
	    if ( fl!=NULL && (fl->script==DEFAULT_SCRIPT || fl->script==0 )) {
		for ( fl=li->fontlist; fl!=NULL; fl=fl->next ) {
		    if ( fl->script==DEFAULT_SCRIPT || fl->script == 0 ) {
			fl->script = script;
			fl->lang = DEFAULT_LANG;
		    }
		}
		buf[0] = (script>>24)&0xff;
		buf[1] = (script>>16)&0xff;
		buf[2] = (script>>8 )&0xff;
		buf[3] = (script    )&0xff;
		uc_strcpy(buf+4,"{dflt}");
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_ScriptLang),buf);
	    }
	    di->script_unknown = false;
	}

	if ( di->insert_text && lastdpi!=0) {
	    sprintf( buffer,_("Text Width:%4d"), (int) rint(li->xmax*72.0/lastdpi));
	    GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_ActualWidth),buffer);
	}
    }
return( true );
}

static int DSP_Done(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	PD *di = GDrawGetUserData(gw);
	if ( di->done!=NULL )
	    *(di->done) = true;
	GDrawDestroyWindow(di->gw);
    }
return( true );
}

static int dsp_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	PD *di = GDrawGetUserData(gw);
	if ( di->done!=NULL )
	    *(di->done) = true;
	GDrawDestroyWindow(di->gw);
    } else if ( event->type==et_destroy ) {
	PD *di = GDrawGetUserData(gw);
	TextInfoDataFree(di->scriptlangs);
	free(di);
	if ( di==printwindow )
	    printwindow = NULL;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/display.html", NULL);
return( true );
	} else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	    MenuExit(NULL,NULL,NULL);
    return( false );
	} else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
	    PD *di = GDrawGetUserData(gw);
	    di->pi.done = true;
	}
return( false );
    } else if ( event->type==et_timer ) {
	PD *di = GDrawGetUserData(gw);
	if ( event->u.timer.timer==di->sizechanged )
	    DSP_SizeChangedTimer(di);
	else if ( event->u.timer.timer==di->dpichanged )
	    DSP_DpiChangedTimer(di);
	else if ( event->u.timer.timer==di->widthchanged )
	    DSP_WidthChangedTimer(di);
	else if ( event->u.timer.timer==di->resized )
	    DSP_JustResized(di);
    } else if ( event->type==et_resize ) {
	PD *di = GDrawGetUserData(gw);
	if ( di->resized!=NULL )
	    GDrawCancelTimer(di->resized);
	di->resized = GDrawRequestTimer(di->gw,300,0,NULL);
    }
return( true );
}
    
static void _PrintFFDlg(FontView *fv,SplineChar *sc,MetricsView *mv,
	int isprint,CharView *cv,SplineSet *fit_to_path) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20], boxes[15], mgcd[5], pgcd[8], vbox, tgcd[14],
	    *harray[8], *farray[8], *barray[4],
	    *barray2[8], *varray[11], *varray2[9], *harray2[5],
	    *varray3[4][4], *ptarray[4], *varray4[4], *varray5[4][2],
	    *regenarray[9], *varray6[3], *tarray[18], *alarray[6],
	    *patharray[5], *oarray[4];
    GTextInfo label[20], mlabel[5], plabel[8], tlabel[14];
    GTabInfo aspects[3];
    int dpi;
    char buf[12], dpibuf[12], sizebuf[12], widthbuf[12], pathlen[32];
    SplineFont *sf = fv!=NULL ? fv->b.sf : sc!=NULL ? sc->parent : mv->fv->b.sf;
    int hasfreetype = hasFreeType();
    BDFFont *bestbdf=DSP_BestMatch(sf,true,12);
    unichar_t *temp;
    int cnt;
    int fromwindow = fv!=NULL?0:sc!=NULL?1:2;
    PD *active;
    int done = false;
    int width = 300;
    extern int _GScrollBar_Width;

    if ( printwindow!=NULL && isprint ) {
	GDrawSetVisible(printwindow->gw,true);
	GDrawRaise(printwindow->gw);
return;
    }

    if ( sf->cidmaster )
	sf = sf->cidmaster;

    active = calloc(1,sizeof(PD));
    if ( isprint )
	printwindow = active;
    active->fv = fv;

    if ( mv!=NULL ) {
	PI_Init(&active->pi,(FontViewBase *) mv->fv,sc);
	active->pi.mv = mv;
    } else
	PI_Init(&active->pi,(FontViewBase *) fv,sc);
    active->cv = cv;
    active->fit_to_path = fit_to_path;
    active->insert_text = !isprint;
    active->done = isprint ? NULL : &done;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = isprint ? _("Print") : _("Insert Text Outlines");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,410));
    pos.height = GDrawPointsToPixels(NULL,330);
    active->gw = GDrawCreateTopWindow(NULL,&pos,dsp_e_h,active,&wattrs);

    memset(&vbox,0,sizeof(vbox));
    memset(&label,0,sizeof(label));
    memset(&mlabel,0,sizeof(mlabel));
    memset(&plabel,0,sizeof(plabel));
    memset(&tlabel,0,sizeof(tlabel));
    memset(&gcd,0,sizeof(gcd));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&pgcd,0,sizeof(pgcd));
    memset(&tgcd,0,sizeof(tgcd));
    memset(&boxes,0,sizeof(boxes));

    label[0].text = (unichar_t *) sf->fontname;
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.popup_msg = _("Select some text, then use this list to change the\nfont in which those characters are displayed.");
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.pos.width = 150;
    gcd[0].gd.cid = CID_Font;
    gcd[0].gd.u.list = FontNames(sf,active->insert_text);
    gcd[0].gd.flags = (fv_list==NULL || gcd[0].gd.u.list[1].text==NULL ) ?
	    (gg_visible):
	    (gg_visible | gg_enabled);
    gcd[0].gd.handle_controlevent = DSP_FontChanged;
    gcd[0].creator = GListButtonCreate;
    varray[0] = &gcd[0]; varray[1] = NULL;

    label[2].text = (unichar_t *) _("_Size:");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 210; gcd[2].gd.pos.y = gcd[0].gd.pos.y+6; 
    gcd[2].gd.flags = gg_visible | gg_enabled;
    if ( isprint )
	gcd[2].gd.popup_msg = _("Select some text, this specifies the point\nsize of those characters");
    else
	gcd[2].gd.popup_msg = _("Select some text, this specifies the vertical\nsize of those characters in em-units");
    gcd[2].gd.cid = CID_SizeLab;
    gcd[2].creator = GLabelCreate;
    harray[0] = &gcd[2];

    if ( bestbdf !=NULL && ( !hasfreetype || sf->onlybitmaps ))
	sprintf( buf, "%d", bestbdf->pixelsize );
    else
	strcpy(buf,"12");
    label[3].text = (unichar_t *) buf;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 240; gcd[3].gd.pos.y = gcd[0].gd.pos.y+3; 
    gcd[3].gd.pos.width = 40;
    gcd[3].gd.flags = gg_visible | gg_enabled;
    gcd[3].gd.cid = CID_Size;
    gcd[3].gd.handle_controlevent = DSP_SizeChanged;
    gcd[3].gd.popup_msg = _("Select some text, this specifies the pixel\nsize of those characters");
    gcd[3].creator = GNumericFieldCreate;
    harray[1] = &gcd[3]; harray[2] = GCD_HPad10;

    label[1].text = (unichar_t *) _("_AA");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 170; gcd[1].gd.pos.y = gcd[0].gd.pos.y+3; 
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    if ( sf->bitmaps!=NULL && ( !hasfreetype || sf->onlybitmaps ))
	gcd[1].gd.flags = DSP_AAState(sf,bestbdf);
    if ( !isprint )
	gcd[1].gd.flags = gg_enabled | gg_cb_on;
    gcd[1].gd.popup_msg = _("Select some text, this controls whether those characters will be\nAntiAlias (greymap) characters, or bitmap characters");
    gcd[1].gd.handle_controlevent = DSP_AAChange;
    gcd[1].gd.cid = CID_AA;
    gcd[1].creator = GCheckBoxCreate;
    harray[3] = &gcd[1]; harray[4] = GCD_Glue; harray[5] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;
    varray[2] = &boxes[2]; varray[3] = NULL;

    label[4].text = (unichar_t *) "pfb";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = gcd[0].gd.pos.x; gcd[4].gd.pos.y = 24+gcd[3].gd.pos.y; 
    gcd[4].gd.flags = gg_visible | gg_enabled;
    gcd[4].gd.cid = CID_pfb;
    gcd[4].gd.handle_controlevent = DSP_RadioSet;
    gcd[4].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[4].creator = GRadioCreate;
    if ( sf->subfontcnt!=0 || !hasfreetype || sf->onlybitmaps || sf->strokedfont || sf->multilayer ) gcd[4].gd.flags = gg_visible;
    farray[0] = &gcd[4];

    label[5].text = (unichar_t *) "ttf";
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = 46; gcd[5].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[5].gd.flags = gg_visible | gg_enabled;
    gcd[5].gd.cid = CID_ttf;
    gcd[5].gd.handle_controlevent = DSP_RadioSet;
    gcd[5].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[5].creator = GRadioCreate;
    if ( sf->subfontcnt!=0 || !hasfreetype || sf->onlybitmaps || sf->strokedfont || sf->multilayer ) gcd[5].gd.flags = gg_visible;
    else if ( sf->layers[ly_fore].order2 ) gcd[5].gd.flags |= gg_cb_on;
    farray[1] = &gcd[5];

    label[6].text = (unichar_t *) "otf";
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 114; gcd[6].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[6].gd.flags = gg_visible | gg_enabled;
    gcd[6].gd.cid = CID_otf;
    gcd[6].gd.handle_controlevent = DSP_RadioSet;
    gcd[6].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[6].creator = GRadioCreate;
    if ( !hasfreetype || sf->onlybitmaps || sf->strokedfont || sf->multilayer ) gcd[6].gd.flags = gg_visible;
    else if ( sf->subfontcnt!=0 || !sf->layers[ly_fore].order2 ) gcd[6].gd.flags |= gg_cb_on;
    farray[2] = &gcd[6];

    label[7].text = (unichar_t *) _("nohints");
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = 114; gcd[7].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[7].gd.flags = gg_visible | gg_enabled;
    gcd[7].gd.cid = CID_nohints;
    gcd[7].gd.handle_controlevent = DSP_RadioSet;
    gcd[7].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[7].creator = GRadioCreate;
    if ( !hasfreetype || sf->onlybitmaps ) gcd[7].gd.flags = gg_visible;
    else if ( sf->strokedfont || sf->multilayer ) gcd[7].gd.flags |= gg_cb_on;
    farray[3] = &gcd[7];
    

    label[8].text = (unichar_t *) "bitmap";
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = 148; gcd[8].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[8].gd.flags = gg_visible | gg_enabled;
    gcd[8].gd.cid = CID_bitmap;
    gcd[8].gd.handle_controlevent = DSP_RadioSet;
    gcd[8].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[8].creator = GRadioCreate;
    if ( sf->bitmaps==NULL ) gcd[8].gd.flags = gg_visible;
    else if ( sf->onlybitmaps ) gcd[8].gd.flags |= gg_cb_on;
    farray[4] = &gcd[8];

    label[9].text = (unichar_t *) "FontForge";
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = 200; gcd[9].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[9].gd.flags = gg_visible | gg_enabled;
    gcd[9].gd.cid = CID_pfaedit;
    gcd[9].gd.handle_controlevent = DSP_RadioSet;
    gcd[9].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[9].creator = GRadioCreate;
    if ( sf->onlybitmaps ) gcd[9].gd.flags = gg_visible;
    if ( !hasfreetype && sf->bitmaps==NULL ) gcd[9].gd.flags |= gg_cb_on;
    else if ( sf->onlybitmaps ) gcd[9].gd.flags = gg_visible;
    farray[5] = &gcd[9]; farray[6] = GCD_Glue; farray[7] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = farray;
    boxes[3].creator = GHBoxCreate;
    varray[4] = &boxes[3]; varray[5] = NULL;
    if ( !isprint ) {
	int k;
	for ( k=4; k<=9; ++k )
	    gcd[k].gd.flags &= ~gg_visible;
	boxes[3].gd.flags = gg_enabled;
    }

    label[10].text = (unichar_t *) "DFLT{dflt}";
    label[10].text_is_1byte = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.popup_msg = _("Select some text, then use this list to specify\nthe current script & language.");
    gcd[10].gd.pos.x = 12; gcd[10].gd.pos.y = 6; 
    gcd[10].gd.pos.width = 150;
    gcd[10].gd.flags = gg_visible | gg_enabled;
    gcd[10].gd.cid = CID_ScriptLang;
    gcd[10].gd.u.list = active->scriptlangs = SLOfFont(sf);
    gcd[10].gd.handle_controlevent = DSP_ScriptLangChanged;
    gcd[10].creator = GListFieldCreate;
    varray[6] = GCD_Glue; varray[7] = NULL;
    varray[8] = &gcd[10]; varray[9] = NULL; varray[10] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = varray;
    boxes[4].creator = GHVBoxCreate;
    harray2[1] = &boxes[4];

    gcd[11].gd.popup_msg = _("Select some text, then use this list to specify\nactive features.");
    gcd[11].gd.pos.width = 50;
    gcd[11].gd.flags = gg_visible | gg_enabled| gg_list_alphabetic | gg_list_multiplesel;
    gcd[11].gd.cid = CID_Features;
    gcd[11].gd.handle_controlevent = DSP_FeaturesChanged;
    gcd[11].creator = GListCreate;
    harray2[0] = &gcd[11];

    label[12].image = &GIcon_menudelta;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.popup_msg = _("Menu");
    gcd[12].gd.flags = gg_visible | gg_enabled;
    gcd[12].gd.handle_controlevent = DSP_Menu;
    gcd[12].creator = GButtonCreate;

    varray6[0] = GCD_Glue; varray6[1] = &gcd[12]; varray6[2] = NULL;
    vbox.gd.flags = gg_enabled|gg_visible;
    vbox.gd.u.boxelements = varray6;
    vbox.creator = GVBoxCreate;

    harray2[2] = &vbox; harray2[3] = GCD_Glue; harray2[4] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = harray2;
    boxes[5].creator = GHBoxCreate;
    varray2[0] = &boxes[5]; varray2[1] = NULL;


    gcd[13].gd.pos.x = 5; gcd[13].gd.pos.y = 20+gcd[13].gd.pos.y; 
    gcd[13].gd.pos.width = 400; gcd[13].gd.pos.height = 236; 
    gcd[13].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_text_xim;
    gcd[13].gd.handle_controlevent = DSP_TextChanged;
    gcd[13].gd.cid = CID_SampleText;
    gcd[13].creator = SFTextAreaCreate;
    varray2[2] = &gcd[13]; varray2[3] = NULL;

    gcd[14].gd.flags = gg_visible | gg_enabled ;
    gcd[14].creator = GLineCreate;
    varray2[4] = &gcd[14]; varray2[5] = NULL;

    label[15].text = (unichar_t *) _("DPI:");
    label[15].text_is_1byte = true;
    gcd[15].gd.label = &label[15];
    gcd[15].gd.flags = gg_visible | gg_enabled;
    gcd[15].gd.popup_msg = _("Specifies screen dots per inch");
    gcd[15].creator = GLabelCreate;

    if ( lastdpi==0 )
	lastdpi = GDrawPointsToPixels(NULL,72);
    dpi = lastdpi;
    sprintf( dpibuf, "%d", dpi );
    label[16].text = (unichar_t *) dpibuf;
    label[16].text_is_1byte = true;
    gcd[16].gd.label = &label[16];
    gcd[16].gd.pos.width = 50;
    gcd[16].gd.flags = gg_visible | gg_enabled;
    gcd[16].gd.popup_msg = _("Specifies screen dots per inch");
    gcd[16].gd.cid = CID_DPI;
    gcd[16].gd.handle_controlevent = DSP_DpiChanged;
    gcd[16].creator = GNumericFieldCreate;

    regenarray[0] = &gcd[15]; regenarray[1] = &gcd[16]; regenarray[2] = GCD_Glue;
    if ( isprint ) {
	gcd[17].gd.flags = gg_visible | gg_enabled;
	gcd[17].gd.popup_msg = _("FontForge does not update this window when a change is made to the font.\nIf a font has changed press the button to force an update");
	label[17].text = (unichar_t *) _("_Refresh");
	label[17].text_is_1byte = true;
	label[17].text_in_resource = true;
	gcd[17].gd.label = &label[17];
	gcd[17].gd.handle_controlevent = DSP_Refresh;
	gcd[17].creator = GButtonCreate;
	regenarray[3] = &gcd[17]; regenarray[4] = NULL;
    } else {
	label[17].text = (unichar_t *) _("Text Width:    0");
	label[17].text_is_1byte = true;
	gcd[17].gd.label = &label[17];
	gcd[17].gd.flags = gg_visible | gg_enabled;
	gcd[17].gd.cid = CID_ActualWidth;
	gcd[17].creator = GLabelCreate;

	label[18].text = (unichar_t *) _("Wrap Pos:");
	label[18].text_is_1byte = true;
	gcd[18].gd.label = &label[18];
	gcd[18].gd.flags = gg_visible | gg_enabled;
	gcd[18].gd.popup_msg = _("The text will wrap to a new line after this many em-units");
	gcd[18].creator = GLabelCreate;

	sprintf( widthbuf, "%d", width );
	label[19].text = (unichar_t *) widthbuf;
	label[19].text_is_1byte = true;
	gcd[19].gd.label = &label[19];
	gcd[19].gd.pos.width = 50;
	gcd[19].gd.flags = gg_visible | gg_enabled;
	gcd[19].gd.popup_msg = _("The text will wrap to a new line after this many em-units");
	gcd[19].gd.cid = CID_TextWidth;
	gcd[19].gd.handle_controlevent = DSP_WidthChanged;
	gcd[19].creator = GNumericFieldCreate;
	regenarray[3] = &gcd[17]; regenarray[4] = GCD_Glue;
	regenarray[5] = &gcd[18]; regenarray[6] = &gcd[19]; regenarray[7] = NULL;
	gcd[13].gd.pos.width = GDrawPixelsToPoints(NULL,width*dpi/72) + _GScrollBar_Width+4;
    }

    boxes[12].gd.flags = gg_enabled|gg_visible;
    boxes[12].gd.u.boxelements = regenarray;
    boxes[12].creator = GHBoxCreate;
    varray2[6] = &boxes[12]; varray2[7] = NULL; varray2[8] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = varray2;
    boxes[6].creator = GHVBoxCreate;

    if ( isprint ) {
	memset(aspects,0,sizeof(aspects));
	aspects[0].text = (unichar_t *) _("Display");
	aspects[0].text_is_1byte = true;
	aspects[0].gcd = &boxes[6];

	plabel[0].text = (unichar_t *) _("_Full Font Display");
	plabel[0].text_is_1byte = true;
	plabel[0].text_in_resource = true;
	pgcd[0].gd.label = &plabel[0];
	pgcd[0].gd.pos.x = 12; pgcd[0].gd.pos.y = 6; 
	pgcd[0].gd.flags = gg_visible | gg_enabled;
	pgcd[0].gd.cid = CID_Display;
	pgcd[0].gd.handle_controlevent = PRT_RadioSet;
	pgcd[0].gd.popup_msg = _("Displays all the glyphs in the font on a rectangular grid at the given point size");
	pgcd[0].creator = GRadioCreate;
	varray3[0][0] = GCD_HPad10; varray3[0][1] = &pgcd[0]; varray3[0][2] = GCD_Glue; varray3[0][3] = NULL;

	cnt = 1;
	if ( fv!=NULL )
	    cnt = FVSelCount(fv);
	else if ( mv!=NULL )
	    cnt = mv->glyphcnt;
	plabel[1].text = (unichar_t *) (cnt==1?_("Full Pa_ge Glyph"):_("Full Pa_ge Glyphs"));
	plabel[1].text_is_1byte = true;
	plabel[1].text_in_resource = true;
	pgcd[1].gd.label = &plabel[1];
	pgcd[1].gd.flags = (cnt==0 ? (gg_visible): (gg_visible | gg_enabled));
	pgcd[1].gd.cid = CID_Chars;
	pgcd[1].gd.handle_controlevent = PRT_RadioSet;
	pgcd[1].gd.popup_msg = _("Displays all the selected characters, each on its own page, at an extremely large point size");
	pgcd[1].creator = GRadioCreate;
	varray3[1][0] = GCD_HPad10; varray3[1][1] = &pgcd[1]; varray3[1][2] = GCD_Glue; varray3[1][3] = NULL;

	plabel[2].text = (unichar_t *) (cnt==1?_("_Multi Size Glyph"):_("_Multi Size Glyphs"));
	plabel[2].text_is_1byte = true;
	plabel[2].text_in_resource = true;
	pgcd[2].gd.label = &plabel[2];
	pgcd[2].gd.flags = pgcd[1].gd.flags;
	pgcd[2].gd.cid = CID_MultiSize;
	pgcd[2].gd.handle_controlevent = PRT_RadioSet;
	pgcd[2].gd.popup_msg = _("Displays all the selected characters, at several different point sizes");
	pgcd[2].creator = GRadioCreate;

	if ( pdefs[fromwindow].pt==pt_chars && cnt==0 )
	    pdefs[fromwindow].pt = pt_fontdisplay;
	if ( pdefs[fromwindow].pt == pt_fontsample )
	    pgcd[pt_fontdisplay].gd.flags |= gg_cb_on;
	else
	    pgcd[pdefs[fromwindow].pt].gd.flags |= gg_cb_on;

	varray3[2][0] = GCD_HPad10; varray3[2][1] = &pgcd[2]; varray3[2][2] = GCD_Glue; varray3[2][3] = NULL;
	varray3[3][0] = NULL;

	boxes[13].gd.flags = gg_enabled|gg_visible;
	boxes[13].gd.u.boxelements = varray3[0];
	boxes[13].creator = GHVBoxCreate;

	plabel[3].text = (unichar_t *) _("_Pointsize:");
	plabel[3].text_is_1byte = true;
	plabel[3].text_in_resource = true;
	pgcd[3].gd.label = &plabel[3];
	pgcd[3].gd.flags = gg_visible | gg_enabled;
	pgcd[3].gd.cid = CID_PSLab;
	pgcd[3].creator = GLabelCreate;
	ptarray[0] = &pgcd[3];

	sprintf(sizebuf,"%d",active->pi.pointsize);
	plabel[4].text = (unichar_t *) sizebuf;
	plabel[4].text_is_1byte = true;
	pgcd[4].gd.label = &plabel[4];
	pgcd[4].gd.pos.width = 60;
	pgcd[4].gd.flags = gg_visible | gg_enabled;
	pgcd[4].gd.cid = CID_PointSize;
	pgcd[4].creator = GNumericFieldCreate;
	ptarray[1] = &pgcd[4]; ptarray[2] = GCD_Glue; ptarray[3] = NULL;

	boxes[8].gd.flags = gg_enabled|gg_visible;
	boxes[8].gd.u.boxelements = ptarray;
	boxes[8].creator = GHBoxCreate;

	varray4[0] = &boxes[13]; varray4[1] = &boxes[8]; varray4[2] = GCD_Glue; varray4[3] = NULL;
	boxes[9].gd.flags = gg_enabled|gg_visible;
	boxes[9].gd.u.boxelements = varray4;
	boxes[9].creator = GVBoxCreate;

	aspects[1].text = (unichar_t *) _("Print");
	aspects[1].text_is_1byte = true;
	aspects[1].gcd = &boxes[9];

	mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
	mgcd[0].gd.u.tabs = aspects;
	mgcd[0].gd.flags = gg_visible | gg_enabled;
	mgcd[0].gd.cid = CID_TabSet;
	mgcd[0].creator = GTabSetCreate;

	mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
	mgcd[1].gd.flags = gg_visible | gg_enabled ;
	mlabel[1].text = (unichar_t *) _("S_etup");
	mlabel[1].text_is_1byte = true;
	mlabel[1].text_in_resource = true;
	mgcd[1].gd.label = &mlabel[1];
	mgcd[1].gd.handle_controlevent = PRT_Setup;
	mgcd[1].creator = GButtonCreate;
	barray[0] = GCD_Glue; barray[1] = &mgcd[1]; barray[2] = GCD_Glue; barray[3] = NULL;

	boxes[14].gd.flags = gg_enabled|gg_visible;
	boxes[14].gd.u.boxelements = barray;
	boxes[14].creator = GHBoxCreate;

	mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
	mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
	mlabel[2].text = (unichar_t *) _("_Print");
	mlabel[2].text_is_1byte = true;
	mlabel[2].text_in_resource = true;
	mgcd[2].gd.mnemonic = 'O';
	mgcd[2].gd.label = &mlabel[2];
	mgcd[2].gd.handle_controlevent = PRT_OK;
	mgcd[2].gd.cid = CID_OK;
	mgcd[2].creator = GButtonCreate;

	mgcd[3].gd.pos.width = -1; mgcd[3].gd.pos.height = 0;
	mgcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	mlabel[3].text = (unichar_t *) _("_Done");
	mlabel[3].text_is_1byte = true;
	mlabel[3].text_in_resource = true;
	mgcd[3].gd.label = &mlabel[3];
	mgcd[3].gd.cid = CID_Cancel;
	mgcd[3].gd.handle_controlevent = DSP_Done;
	mgcd[3].creator = GButtonCreate;
	barray2[0] = GCD_Glue; barray2[1] = &mgcd[2]; barray2[2] = GCD_Glue;
	barray2[3] = GCD_Glue; barray2[4] = &mgcd[3]; barray2[5] = GCD_Glue; barray2[6] = NULL;

	boxes[11].gd.flags = gg_enabled|gg_visible;
	boxes[11].gd.u.boxelements = barray2;
	boxes[11].creator = GHBoxCreate;

	varray5[0][0] = &mgcd[0]; varray5[0][1] = NULL;
	varray5[1][0] = &boxes[14]; varray5[1][1] = NULL;
	varray5[2][0] = &boxes[11]; varray5[2][1] = NULL;
	varray5[3][0] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray5[0];
	boxes[0].gd.cid = CID_TopBox;
	boxes[0].creator = GHVGroupCreate;
    } else {
	tarray[0] = &boxes[6]; tarray[1] = NULL;

	tgcd[0].gd.flags = fit_to_path==NULL ? gg_visible : (gg_visible | gg_enabled | gg_cb_on);
	tlabel[0].text = (unichar_t *) _("Bind to Path");
	tlabel[0].text_is_1byte = true;
	tlabel[0].text_in_resource = true;
	tgcd[0].gd.label = &tlabel[0];
	tgcd[0].gd.handle_controlevent = PRT_Bind;
	tgcd[0].gd.cid = CID_Bind;
	tgcd[0].creator = GCheckBoxCreate;
	patharray[0] = &tgcd[0]; patharray[1] = GCD_Glue;

	if ( fit_to_path==NULL ) {
	    tgcd[1].gd.flags = 0;
	    strcpy(pathlen,"0");
	} else {
	    tgcd[1].gd.flags = gg_visible | gg_enabled;
	    sprintf(pathlen, _("Path Length: %g"), PathLength(fit_to_path));
	}
	tlabel[1].text = (unichar_t *) pathlen;
	tlabel[1].text_is_1byte = true;
	tlabel[1].text_in_resource = true;
	tgcd[1].gd.label = &tlabel[1];
	tgcd[1].creator = GLabelCreate;
	patharray[2] = &tgcd[1]; patharray[3] = NULL;

	boxes[9].gd.flags = gg_enabled|gg_visible;
	boxes[9].gd.u.boxelements = patharray;
	boxes[9].creator = GHBoxCreate;
	tarray[2] = &boxes[9]; tarray[3] = NULL;

	tgcd[2].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled | gg_cb_on);
	tlabel[2].text = (unichar_t *) _("Scale so text width matches path length");
	tlabel[2].text_is_1byte = true;
	tlabel[2].text_in_resource = true;
	tgcd[2].gd.label = &tlabel[2];
	tgcd[2].gd.cid = CID_Scale;
	tgcd[2].creator = GCheckBoxCreate;
	tarray[4] = &tgcd[2]; tarray[5] = NULL;

	tgcd[3].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[3].text = (unichar_t *) _("Rotate each glyph as a unit");
	tlabel[3].text_is_1byte = true;
	tlabel[3].text_in_resource = true;
	tgcd[3].gd.label = &tlabel[3];
	tgcd[3].gd.cid = CID_GlyphAsUnit;
	tgcd[3].creator = GCheckBoxCreate;
	tarray[6] = &tgcd[3]; tarray[7] = NULL;

	tgcd[4].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[4].text = (unichar_t *) _("Align:");
	tlabel[4].text_is_1byte = true;
	tlabel[4].text_in_resource = true;
	tgcd[4].gd.label = &tlabel[4];
	tgcd[4].creator = GLabelCreate;
	alarray[0] = &tgcd[4];

	tgcd[5].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled | gg_cb_on);
	tlabel[5].text = (unichar_t *) _("At Start");
	tlabel[5].text_is_1byte = true;
	tlabel[5].text_in_resource = true;
	tgcd[5].gd.label = &tlabel[5];
	tgcd[5].gd.cid = CID_Start;
	tgcd[5].creator = GRadioCreate;
	alarray[1] = &tgcd[5];

	tgcd[6].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[6].text = (unichar_t *) _("Centered");
	tlabel[6].text_is_1byte = true;
	tlabel[6].text_in_resource = true;
	tgcd[6].gd.label = &tlabel[6];
	tgcd[6].gd.cid = CID_Center;
	tgcd[6].creator = GRadioCreate;
	alarray[2] = &tgcd[6];

	tgcd[7].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[7].text = (unichar_t *) _("At End");
	tlabel[7].text_is_1byte = true;
	tlabel[7].text_in_resource = true;
	tgcd[7].gd.label = &tlabel[7];
	tgcd[7].gd.cid = CID_End;
	tgcd[7].creator = GRadioCreate;
	alarray[3] = &tgcd[7];
	alarray[4] = GCD_Glue; alarray[5] = NULL;

	boxes[8].gd.flags = gg_enabled|gg_visible;
	boxes[8].gd.u.boxelements = alarray;
	boxes[8].creator = GHBoxCreate;
	tarray[8] = &boxes[8]; tarray[9] = NULL;

	tgcd[8].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[8].text = (unichar_t *) _("Offset text from path by:");
	tlabel[8].text_is_1byte = true;
	tlabel[8].text_in_resource = true;
	tgcd[8].gd.label = &tlabel[8];
	tgcd[8].creator = GLabelCreate;
	oarray[0] = &tgcd[8];

	tgcd[9].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tgcd[9].gd.pos.width = 60;
	tlabel[9].text = (unichar_t *) "0";
	tlabel[9].text_is_1byte = true;
	tlabel[9].text_in_resource = true;
	tgcd[9].gd.label = &tlabel[9];
	tgcd[9].gd.cid = CID_YOffset;
	tgcd[9].creator = GNumericFieldCreate;
	oarray[1] = &tgcd[9]; oarray[2] = GCD_Glue; oarray[3] = NULL;

	boxes[13].gd.flags = gg_enabled|gg_visible;
	boxes[13].gd.u.boxelements = oarray;
	boxes[13].creator = GHBoxCreate;
	tarray[10] = &boxes[13]; tarray[11] = NULL;

	tgcd[10].gd.flags = gg_visible | gg_enabled ;
	tgcd[10].creator = GLineCreate;
	tarray[12] = &tgcd[10]; tarray[13] = NULL;

	tgcd[11].gd.pos.width = -1; tgcd[11].gd.pos.height = 0;
	tgcd[11].gd.flags = gg_visible | gg_enabled | gg_but_default;
	tlabel[11].text = (unichar_t *) _("_Insert");
	tlabel[11].text_is_1byte = true;
	tlabel[11].text_in_resource = true;
	tgcd[11].gd.mnemonic = 'O';
	tgcd[11].gd.label = &tlabel[11];
	tgcd[11].gd.handle_controlevent = PRT_OK;
	tgcd[11].gd.cid = CID_OK;
	tgcd[11].creator = GButtonCreate;

	tgcd[12].gd.pos.width = -1; tgcd[12].gd.pos.height = 0;
	tgcd[12].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	tlabel[12].text = (unichar_t *) _("_Cancel");
	tlabel[12].text_is_1byte = true;
	tlabel[12].text_in_resource = true;
	tgcd[12].gd.label = &tlabel[12];
	tgcd[12].gd.cid = CID_Cancel;
	tgcd[12].gd.handle_controlevent = DSP_Done;
	tgcd[12].creator = GButtonCreate;
	barray2[0] = GCD_Glue; barray2[1] = &tgcd[11]; barray2[2] = GCD_Glue;
	barray2[3] = GCD_Glue; barray2[4] = &tgcd[12]; barray2[5] = GCD_Glue; barray2[6] = NULL;

	boxes[11].gd.flags = gg_enabled|gg_visible;
	boxes[11].gd.u.boxelements = barray2;
	boxes[11].creator = GHBoxCreate;
	tarray[14] = &boxes[11]; tarray[15] = NULL; tarray[16] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = tarray;
	boxes[0].gd.cid = CID_TopBox;
	boxes[0].creator = GHVGroupCreate;
    }

    GGadgetsCreate(active->gw,boxes);
    GListSetSBAlwaysVisible(gcd[11].ret,true);
    GListSetPopupCallback(gcd[11].ret,MV_FriendlyFeatures);

    GTextInfoListFree(gcd[0].gd.u.list);
    DSP_SetFont(active,true);
    if ( isprint ) {
	SFTFSetDPI(gcd[13].ret,dpi);
	temp = PrtBuildDef(sf,&((SFTextArea *) gcd[13].ret)->li,
		(void (*)(void *, int, uint32, uint32))LayoutInfoInitLangSys);
	GGadgetSetTitle(gcd[13].ret, temp);
	free(temp);
    } else {
	active->script_unknown = true;
	if ( old_bind_text ) {
	    SFTextAreaReplace(gcd[13].ret,old_bind_text);
	    DSP_TextChanged(gcd[13].ret,NULL);
	}
    }
    SFTFRegisterCallback(gcd[13].ret,active,DSP_ChangeFontCallback);

    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableRow(boxes[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
    GHVBoxSetExpandableRow(boxes[6].ret,1);
    GHVBoxSetExpandableCol(boxes[12].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[11].ret,gb_expandglue);
    GHVBoxSetExpandableRow(vbox.ret,gb_expandglue);
    if ( isprint ) {
	GHVBoxSetExpandableCol(boxes[8].ret,gb_expandglue);
	GHVBoxSetExpandableRow(boxes[9].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[13].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[14].ret,gb_expandglue);
    } else {
	GHVBoxSetExpandableCol(boxes[8].ret,gb_expandglue);
	GHVBoxSetExpandableRow(boxes[9].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[13].ret,gb_expandglue);
    }
    GHVBoxFitWindow(boxes[0].ret);

    SFTextAreaSelect(gcd[13].ret,0,0);
    SFTextAreaShow(gcd[13].ret,0);
    SFTFProvokeCallback(gcd[13].ret);
    GWidgetIndicateFocusGadget(gcd[13].ret);

    GDrawSetVisible(active->gw,true);
    active->ready = true;
    if ( !isprint ) {
	while ( !done )
	    GDrawProcessOneEvent(NULL);
    }
}

void PrintFFDlg(FontView *fv,SplineChar *sc,MetricsView *mv) {
    _PrintFFDlg(fv,sc,mv,true,NULL,NULL);
}

static SplineSet *OnePathSelected(CharView *cv) {
    RefChar *rf;
    SplineSet *ss, *sel=NULL;
    SplinePoint *sp;

    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->selected ) {
		if ( sel==NULL )
		    sel = ss;
		else if ( sel!=ss )
return( NULL );
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    if ( sel==NULL )
return( NULL );

    for ( rf = cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL; rf=rf->next ) {
	if ( rf->selected )
return( NULL );
    }
return( sel );
}

void InsertTextDlg(CharView *cv) {
    _PrintFFDlg(NULL,cv->b.sc,NULL,false,cv,OnePathSelected(cv));
}

void PrintWindowClose(void) {
    if ( printwindow!=NULL )
	GDrawDestroyWindow(printwindow->gw);
}
