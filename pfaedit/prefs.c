/* Copyright (C) 2001 by George Williams */
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
#include "charset.h"
#include "gresource.h"
#include "ustring.h"

int adjustwidth = true;
int adjustlbearing = true;
int default_encoding = em_iso8859_1;
int autohint_before_rasterize = 1;
int ItalicConstrained=true;
char *BDFFoundry=NULL;
char *xuid=NULL;
char *RecentFiles[RECENT_MAX] = { NULL };
/*struct cvshows CVShows = { 1, 1, 1, 1, 1, 0, 1 };*/ /* in charview */
/* int default_fv_font_size = 24; */	/* in fontview */
/* int default_fv_antialias = false */	/* in fontview */
/* int local_encoding; */		/* in gresource.c *//* not a charset */

/* don't use mnemonics 'C' or 'O' (Cancel & OK) */
enum pref_types { pr_int, pr_bool, pr_enum, pr_encoding, pr_string };
struct enums { char *name; int value; };

static const unichar_t aws[] = { 'C','h','a','n','g','i','n','g',' ','t','h','e',' ','w','i','d','t','h',' ','o','f',' ','a',' ','c','h','a','r','a','c','t','e','r','\n','c','h','a','n','g','e','s',' ','t','h','e',' ','w','i','d','t','h','s',' ','o','f',' ','a','l','l',' ','a','c','c','e','n','t','e','d','\n','c','h','a','r','a','c','t','e','r','s',' ','b','a','s','e','d',' ','o','n',' ','i','t','.',  '\0' };
static const unichar_t als[] = { 'C','h','a','n','g','i','n','g',' ','t','h','e',' ','l','e','f','t',' ','s','i','d','e',' ','b','e','a','r','i','n','g','\n','o','f',' ','a',' ','c','h','a','r','a','c','t','e','r',' ','a','d','j','u','s','t','s',' ','t','h','e',' ','l','b','e','a','r','i','n','g','\n','o','f',' ','o','t','h','e','r',' ','r','e','f','e','r','e','n','c','e','s',' ','i','n',' ','a','l','l',' ','a','c','c','e','n','t','e','d','\n','c','h','a','r','a','c','t','e','r','s',' ','b','a','s','e','d',' ','o','n',' ','i','t','.',  '\0' };
static const unichar_t fornewfonts[] = { 'D','e','f','a','u','l','t',' ','e','n','c','o','d','i','n','g',' ','f', 'o', 'r', '\n', 'n', 'e', 'w', ' ', 'f', 'o', 'n', 't', 's',  '\0' };
static const unichar_t loc[] = { 'C','h','a','r','a','c','t','e','r',' ','s','e','t',' ','u','s','e','d',' ','b','y',' ','t','h','e',' ','l','o','c','a','l','\n','c','l','i','p','b','o','a','r','d',',',' ','f','i','l','e','s','y','s','t','e','m',',',' ','e','t','c','.',' ','(','o','n','l','y','\n','8','b','i','t',' ','c','h','a','r','s','e','t','s',' ','c','u','r','r','e','n','t','l','y',' ','s','u','p','p','o','r','t','e','d','\n','h','e','r','e',')',  '\0' };
static const unichar_t ah[] = { 'A','u','t','o','H','i','n','t',' ','b','e','f','o','r','e',' ','r','a','s','t','e','r','i','z','i','n','g',  '\0' };
static const unichar_t fn[] = { 'N','a','m','e',' ','u','s','e','d',' ','f','o','r',' ','f','o','u','n','d','r','y',' ','f','i','e','l','d',' ','i','n',' ','b','d','f','\n','f','o','n','t',' ','g','e','n','e','r','a','t','i','o','n',  '\0' };
#if 1
static const unichar_t xu[] = { 'I','f',' ','s','p','e','c','i','f','i','e','d',' ','t','h','i','s',' ','s','h','o','u','l','d',' ','b','e',' ','a',' ','s','p','a','c','e',' ','s','e','p','e','r','a','t','e','d',' ','l','i','s','t',' ','o','f',' ','i','n','t','e','g','e','r','s',' ','e','a','c','h','\n','l','e','s','s',' ','t','h','a','n',' ','1','6','7','7','7','2','1','6',' ','w','h','i','c','h',' ','u','n','i','q','u','e','l','y',' ','i','d','e','n','t','i','f','y',' ','y','o','u','r',' ','o','r','g','a','n','i','z','a','t','i','o','n','\n','P','f','a','E','d','i','t',' ','w','i','l','l',' ','g','e','n','e','r','a','t','e',' ','a',' ','r','a','n','d','o','m',' ','n','u','m','b','e','r',' ','f','o','r',' ','t','h','e',' ','f','i','n','a','l',' ','c','o','m','p','o','n','a','n','t','.',  '\0' };
#else
static const unichar_t xu[] = { 'I','f',' ','s','p','e','c','i','f','i','e','d',' ','t','h','i','s',' ','s','h','o','u','l','d',' ','b','e',' ','a',' ','s','p','a','c','e',' ','s','e','p','e','r','a','t','e','d',' ','l','i','s','t',' ','o','f',' ','i','n','t','e','g','e','r','s',' ','e','a','c','h','\n','l','e','s','s',' ','t','h','a','n',' ','1','6','7','7','7','2','1','6',' ','w','h','i','c','h',' ','u','n','i','q','u','e','l','y',' ','i','d','e','n','t','i','f','y',' ','y','o','u','r',' ','o','r','g','a','n','i','z','a','t','i','o','n','\n','P','f','a','E','d','i','t',' ','w','i','l','l',' ','h','a','s','h',' ','t','h','e',' ','f','o','n','t','n','a','m','e',' ','t','o',' ','g','e','n','e','r','a','t','e',' ','a',' ','f','i','n','a','l',' ','n','u','m','b','e','r','.',  '\0' };
#endif
static const unichar_t rulers[] = { 'D','i','s','p','l','a','y',' ','r','u','l','e','r','s',' ','i','n',' ','t','h','e',' ','O','u','t','l','i','n','e',' ','C','h','a','r','a','c','t','e','r',' ','V','i','e','w',  '\0' };
static const unichar_t sephints[] = { 'H','a','v','e',' ','s','e','p','e','r','a','t','e',' ','c','o','n','t','r','o','l','s',' ','f','o','r',' ','d','i','s','p','l','a','y',' ','h','o','r','i','z','o','n','t','a','l',' ','a','n','d',' ','v','e','r','t','i','c','a','l',' ','h','i','n','t','s','.',  '\0' };
static const unichar_t ic[] = { 'I','n',' ','t','h','e',' ','O','u','t','l','i','n','e',' ','V','i','e','w',',',' ','t','h','e',' ','S','h','i','f','t',' ','k','e','y',' ','c','o','n','s','t','r','a','i','n','s',' ','m','o','t','i','o','n',' ','t','o',' ','b','e',' ','p','a','r','a','l','l','e','l',' ','t','o',' ','t','h','e',' ','I','t','a','l','i','c','A','n','g','l','e',' ','r','a','t','h','e','r',' ','t','h','a','n',' ','t','h','e',' ','v','e','r','t','i','c','a','l','.',  '\0' };

struct enums fvsize_enums[] = { NULL };

static struct prefs_list {
    char *name;
    enum pref_types type;
    void *val;
    char mn;
    struct enums *enums;
    unsigned int dontdisplay: 1;
    const unichar_t *popup;
} prefs_list[] = {
	{ "AutoWidthSync", pr_bool, &adjustwidth, '\0', NULL, 0, aws },
	{ "AutoLBearingSync", pr_bool, &adjustlbearing, '\0', NULL, 0, als },
	{ "DefaultFVSize", pr_enum, &default_fv_font_size, 'S', fvsize_enums, 1 },
	{ "AntiAlias", pr_bool, &default_fv_antialias, '\0', NULL, 1 },
	{ "AutoHint", pr_bool, &autohint_before_rasterize, 'A', NULL, 0, ah },
	{ "LocalCharset", pr_encoding, &local_encoding, 'L', NULL, 0, loc },
	{ "NewCharset", pr_encoding, &default_encoding, 'N', NULL, 0, fornewfonts },
	{ "ShowRulers", pr_bool, &CVShows.showrulers, '\0', NULL, 0, rulers },
	{ "FoundryName", pr_string, &BDFFoundry, '\0', NULL, 0, fn },
	{ "XUID-Base", pr_string, &xuid, '\0', NULL, 0, xu },
	{ "SeperateHintControls", pr_bool, &seperate_hint_controls, '\0', NULL, 1, sephints },
	{ "PageWidth", pr_int, &pagewidth, '\0', NULL, 1 },
	{ "PageHeight", pr_int, &pageheight, '\0', NULL, 1 },
	{ "PrintType", pr_int, &printtype, '\0', NULL, 1 },
	{ "ItalicConstrained", pr_bool, &ItalicConstrained, '\0', NULL, 0, ic },
	{ NULL }
};

static char *getPfaEditPrefs(void) {
    static char *prefs=NULL;
    char buffer[1025];

    if ( prefs!=NULL )
return( prefs );
    if ( getPfaEditDir(buffer)==NULL )
return( NULL );
    sprintf(buffer,"%s/prefs", getPfaEditDir(buffer));
    prefs = copy(buffer);
return( prefs );
}

void LoadPrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    char line[1100];
    int i, ri=0;
    char *pt;

    LoadPfaEditEncodings();

    if ( prefs==NULL )
return;
    if ( (p=fopen(prefs,"r"))==NULL )
return;
    while ( fgets(line,sizeof(line),p)!=NULL ) {
	if ( *line=='#' )
    continue;
	pt = strchr(line,':');
	if ( pt==NULL )
    continue;
	for ( i=0; prefs_list[i].name!=NULL; ++i )
	    if ( strncmp(line,prefs_list[i].name,pt-line)==0 )
	break;
	for ( ++pt; *pt=='\t'; ++pt );
	if ( line[strlen(line)-1]=='\n' )
	    line[strlen(line)-1] = '\0';
	if ( line[strlen(line)-1]=='\r' )
	    line[strlen(line)-1] = '\0';
	if ( prefs_list[i].name==NULL ) {
	    if ( strncmp(line,"Recent:",strlen("Recent:"))==0 && ri<RECENT_MAX )
		RecentFiles[ri++] = copy(pt);
    continue;
	}
	switch ( prefs_list[i].type ) {
	  case pr_encoding:
	    if ( sscanf( pt, "%d", prefs_list[i].val )!=1 ) {
		Encoding *item;
		for ( item = enclist; item!=NULL && strcmp(item->enc_name,pt)!=0; item = item->next );
		if ( item==NULL )
		    *((int *) (prefs_list[i].val)) = em_iso8859_1;
		else
		    *((int *) (prefs_list[i].val)) = item->enc_num;
	    }
	  break;
	  case pr_bool: case pr_int:
	    sscanf( pt, "%d", prefs_list[i].val );
	  break;
	  case pr_string:
	    if ( *pt=='\0' ) pt=NULL;
	    *((char **) (prefs_list[i].val)) = copy(pt);
	  break;
	}
    }
    fclose(p);
}

void SavePrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    int i, val;

    if ( prefs==NULL )
return;
    if ( (p=fopen(prefs,"w"))==NULL )
return;

    for ( i=0; prefs_list[i].name!=NULL; ++i ) {
	switch ( prefs_list[i].type ) {
	  case pr_encoding:
	    val = *(int *) (prefs_list[i].val);
	    if ( val<em_base )
		fprintf( p, "%s:\t%d\n", prefs_list[i].name, val );
	    else {
		Encoding *item;
		for ( item = enclist; item!=NULL && item->enc_num!=val; item=item->next );
		fprintf( p, "%s:\t%s\n", prefs_list[i].name, item==NULL?"-1":item->enc_name );
	    }
	  break;
	  case pr_bool: case pr_int:
	    fprintf( p, "%s:\t%d\n", prefs_list[i].name, *(int *) (prefs_list[i].val) );
	  break;
	  case pr_string:
	    if ( *(char **) (prefs_list[i].val)!=NULL )
		fprintf( p, "%s:\t%s\n", prefs_list[i].name, *(char **) (prefs_list[i].val) );
	  break;
	}
    }

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i )
	fprintf( p, "Recent:\t%s\n", RecentFiles[i]);

    fclose(p);
}

struct pref_data {
    int done;
};

static int Prefs_Ok(GGadget *g, GEvent *e) {
    int i;
    int err=0, enc;
    struct pref_data *p;
    GWindow gw;
    const unichar_t *ret;
    int lc=-1;
    GTextInfo *ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	p = GDrawGetUserData(gw);
	for ( i=0; prefs_list[i].name!=NULL; ++i ) {
	    /* before assigning values, check for any errors */
	    /* if any errors, then NO values should be assigned, in case they cancel */
	    if ( prefs_list[i].dontdisplay )
	continue;
	    if ( prefs_list[i].type==pr_int ) {
		GetInt(gw,1000+i,prefs_list[i].name,&err);
	    } else if ( prefs_list[i].val == &local_encoding ) {
		enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,1000+i));
		lc = (int) (encodingtypes[enc].userdata);
	    }
	}
	if ( err )
return( true );

	if ( lc!=-1 ) {
	    if ( lc>=em_base || lc==em_none || lc==em_adobestandard )
		lc = em_iso8859_1;
	    local_encoding = lc+3;		/* must be done early, else strings don't convert */
	}

	for ( i=0; prefs_list[i].name!=NULL; ++i ) {
	    if ( prefs_list[i].dontdisplay )
	continue;
	    switch( prefs_list[i].type ) {
	      case pr_int:
	        *((int *) (prefs_list[i].val)) = GetInt(gw,1000+i,prefs_list[i].name,&err);
	      break;
	      case pr_bool:
	        *((int *) (prefs_list[i].val)) = GGadgetIsChecked(GWidgetGetControl(gw,1000+i));
	      break;
	      case pr_encoding:
		if ( prefs_list[i].val==&local_encoding )
	continue;
		enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,1000+i));
		ti = GGadgetGetListItem(GWidgetGetControl(gw,1000+i),enc);
		*((int *) (prefs_list[i].val)) = (int) (ti->userdata);
	      break;
	      case pr_string:
	        ret = _GGadgetGetTitle(GWidgetGetControl(gw,1000+i));
		free( *((char **) (prefs_list[i].val)) );
		*((char **) (prefs_list[i].val)) = NULL;
		if ( ret!=NULL && *ret!='\0' )
		    *((char **) (prefs_list[i].val)) = u2def_copy(ret);
	      break;
	    }
	}
	p->done = true;
	SavePrefs();
    }
return( true );
}

static int Prefs_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct pref_data *p = GDrawGetUserData(GGadgetGetWindow(g));
	p->done = true;
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct pref_data *p = GDrawGetUserData(gw);
	p->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void DoPrefs(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData *gcd;
    GTextInfo *label, **list;
    struct pref_data p;
    static unichar_t title[] = { 'P','r','e','f','e','r','e','n','c','e','s',  '\0' };
    int i, gc, j, line, llen;
    char buf[20];

    for ( i=line=gc=0; prefs_list[i].name!=NULL; ++i ) {
	if ( prefs_list[i].dontdisplay )
    continue;
	gc += 2;
	if ( prefs_list[i].type==pr_bool ) ++gc;
	++line;
    }

    memset(&p,'\0',sizeof(p));
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,260);
    pos.height = GDrawPointsToPixels(NULL,line*30+45);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);

    gcd = gcalloc(gc+4,sizeof(GGadgetCreateData));
    label = gcalloc(gc+4,sizeof(GTextInfo));

    gc = 0;
    gcd[gc].gd.pos.x = gcd[gc].gd.pos.y = 2;
    gcd[gc].gd.pos.width = pos.width-4; gcd[gc].gd.pos.height = pos.height-2;
    gcd[gc].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[gc++].creator = GGroupCreate;

    for ( i=line=0; prefs_list[i].name!=NULL; ++i ) {
	if ( prefs_list[i].dontdisplay )
    continue;
	label[gc].text = (unichar_t *) prefs_list[i].name;
	label[gc].text_is_1byte = true;
	gcd[gc].gd.label = &label[gc];
	gcd[gc].gd.mnemonic = prefs_list[i].mn;
	gcd[gc].gd.popup_msg = prefs_list[i].popup;
	gcd[gc].gd.pos.x = 8;
	gcd[gc].gd.pos.y = 8+30*line + 6;
	gcd[gc].gd.flags = gg_visible | gg_enabled;
	gcd[gc++].creator = GLabelCreate;

	label[gc].text_is_1byte = true;
	gcd[gc].gd.label = &label[gc];
	gcd[gc].gd.mnemonic = prefs_list[i].mn;
	gcd[gc].gd.popup_msg = prefs_list[i].popup;
	gcd[gc].gd.pos.x = 110;
	gcd[gc].gd.pos.y = 8+30*line;
	gcd[gc].gd.flags = gg_visible | gg_enabled;
	gcd[gc].gd.cid = 1000+i;
	switch ( prefs_list[i].type ) {
	  case pr_bool:
	    label[gc].text = (unichar_t *) "On";
	    gcd[gc++].creator = GRadioCreate;
	    gcd[gc] = gcd[gc-1];
	    gcd[gc].gd.pos.x += 50;
	    gcd[gc].gd.cid = 0;
	    gcd[gc].gd.label = &label[gc];
	    label[gc].text = (unichar_t *) "Off";
	    label[gc].text_is_1byte = true;
	    if ( *((int *) prefs_list[i].val))
		gcd[gc-1].gd.flags |= gg_cb_on;
	    else
		gcd[gc].gd.flags |= gg_cb_on;
	    ++gc;
	  break;
	  case pr_int:
	    sprintf(buf,"%d", *((int *) prefs_list[i].val));
	    label[gc].text = (unichar_t *) copy( buf );
	    gcd[gc++].creator = GTextFieldCreate;
	  break;
	  case pr_encoding:
	    if ( prefs_list[i].val==&local_encoding ) {
		gcd[gc].gd.u.list = encodingtypes;
		gcd[gc].gd.label = EncodingTypesFindEnc(encodingtypes,
			*(int *) prefs_list[i].val - 3);
	    } else {
		gcd[gc].gd.u.list = GetEncodingTypes();
		gcd[gc].gd.label = EncodingTypesFindEnc(gcd[gc].gd.u.list,
			*(int *) prefs_list[i].val);
		gcd[gc].gd.pos.width = 143;
	    }
	    if ( gcd[gc].gd.label==NULL ) gcd[gc].gd.label = &encodingtypes[0];
	    gcd[gc++].creator = GListButtonCreate;
	  break;
	  case pr_string:
	    if ( *((char **) prefs_list[i].val)!=NULL )
		label[gc].text = def2u_copy( *((char **) prefs_list[i].val));
	    else if ( ((char **) prefs_list[i].val)==&BDFFoundry )
		label[gc].text = def2u_copy( "PfaEdit" );
	    else
		label[gc].text = def2u_copy( "" );
	    label[gc].text_is_1byte = false;
	    gcd[gc++].creator = GTextFieldCreate;
	  break;
	}
	++line;
    }

    gcd[gc].gd.pos.x = 30-3; gcd[gc].gd.pos.y = 8+line*30+5-3;
    gcd[gc].gd.pos.width = 55; gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[gc].text = (unichar_t *) "OK";
    label[gc].text_is_1byte = true;
    gcd[gc].gd.mnemonic = 'O';
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.handle_controlevent = Prefs_Ok;
    gcd[gc++].creator = GButtonCreate;

    gcd[gc].gd.pos.x = 250-55-30; gcd[gc].gd.pos.y = gcd[gc-1].gd.pos.y+3;
    gcd[gc].gd.pos.width = 55; gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[gc].text = (unichar_t *) "Cancel";
    label[gc].text_is_1byte = true;
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.mnemonic = 'C';
    gcd[gc].gd.handle_controlevent = Prefs_Cancel;
    gcd[gc++].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);

    for ( gc=1,i=0; prefs_list[i].name!=NULL; ++i ) {
	if ( prefs_list[i].dontdisplay )
    continue;
	switch ( prefs_list[i].type ) {
	  case pr_bool:
	    ++gc;
	  break;
	  case pr_encoding: {
	    GGadget *g = gcd[gc+1].ret;
	    list = GGadgetGetList(g,&llen);
	    for ( j=0; j<llen ; ++j ) {
		if ( list[j]->text!=NULL &&
			(void *) ( *((int *) prefs_list[i].val) - (prefs_list[i].val==&local_encoding?3:0)) == list[j]->userdata )
		    list[j]->selected = true;
		else
		    list[j]->selected = false;
		if ( prefs_list[i].val == &local_encoding &&
			( (int) (list[j]->userdata) >= em_first2byte ||
			  (int) (list[j]->userdata) == em_none ||
			  (int) (list[j]->userdata) == em_base ||
			  (int) (list[j]->userdata) == em_adobestandard ))
		    list[j]->disabled = true;
	    }
	    if ( gcd[gc+1].gd.u.list!=encodingtypes )
		GTextInfoListFree(gcd[gc+1].gd.u.list);
	  } break;
	  case pr_string: case pr_int:
	    free(label[gc+1].text);
	  break;
	}
	gc += 2;
    }

    free(gcd);
    free(label);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !p.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

void RecentFilesRemember(char *filename) {
    int i;

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i )
	if ( strcmp(RecentFiles[i],filename)==0 )
    break;

    if ( i<RECENT_MAX && RecentFiles[i]!=NULL ) {
	if ( i!=0 ) {
	    filename = RecentFiles[i];
	    RecentFiles[i] = RecentFiles[0];
	    RecentFiles[0] = filename;
	}
    } else {
	if ( RecentFiles[RECENT_MAX-1]!=NULL )
	    free( RecentFiles[RECENT_MAX-1]);
	for ( i=RECENT_MAX-1; i>0; --i )
	    RecentFiles[i] = RecentFiles[i-1];
	RecentFiles[0] = copy(filename);
    }
    SavePrefs();
}

