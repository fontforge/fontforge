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
#include <ustring.h>
#include <chardata.h>
#include <utype.h>
#include "psfont.h"

int PSDictFindEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( i );

return( -1 );
}

char *PSDictHasEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( NULL );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( dict->values[i] );

return( NULL );
}

int PSDictRemoveEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( false );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next )
return( false );
    free( dict->keys[i]);
    free( dict->values[i] );
    --dict->next;
    while ( i<dict->next ) {
	dict->keys[i] = dict->keys[i+1];
	dict->values[i] = dict->values[i+1];
	++i;
    }

return( true );
}

int PSDictChangeEntry(struct psdict *dict, char *key, char *newval) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next ) {
	if ( dict->next>=dict->cnt ) {
	    dict->cnt += 10;
	    dict->keys = grealloc(dict->keys,dict->cnt*sizeof(char *));
	    dict->values = grealloc(dict->values,dict->cnt*sizeof(char *));
	}
	dict->keys[dict->next] = copy(key);
	dict->values[dict->next] = NULL;
	++dict->next;
    }
    free(dict->values[i]);
    dict->values[i] = copy(newval);
return( i );
}

enum { pt_number, pt_boolean, pt_array, pt_code };
static struct { const char *name; short type, arr_size, present; } KnownPrivates[] = {
    { "BlueValues", pt_array, 14 },
    { "OtherBlues", pt_array, 10 },
    { "BlueFuzz", pt_number },
    { "FamilyBlues", pt_array, 14 },
    { "FamilyOtherBlues", pt_array, 10 },
    { "BlueScale", pt_number },
    { "BlueShift", pt_number },
    { "StdHW", pt_array, 1 },
    { "StdVW", pt_array, 1 },
    { "StemSnapH", pt_array, 12 },
    { "StemSnapV", pt_array, 12 },
    { "ForceBold", pt_boolean },
    { "LanguageGroup", pt_number },
    { "RndStemUp", pt_number },
    { "UniqueID", pt_number },
    { "lenIV", pt_number },
    { "ExpansionFactor", pt_number },
    { "Erode", pt_code },
/* I am deliberately not including Subrs and OtherSubrs */
/* The first could not be entered (because it's a set of binary strings) */
/* And the second has special meaning to us and must be handled with care */
    { NULL }
};

struct ask_data {
    int ret;
    int done;
};

static int Ask_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    struct ask_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int Ask_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    struct ask_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = d->ret = true;
    }
return( true );
}

static int ask_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct ask_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static char *AskKey(SplineFont *sf) {
    int i,j, cnt=0;
    GTextInfo *ti;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct ask_data d;
    static unichar_t title[] = { 'P','r','i','v', 'a', 't','e',' ','K','e','y',  '\0' };
    char *ret;

    if ( sf->private==NULL )
	for ( i=0; KnownPrivates[i].name!=NULL; ++i ) {
	    KnownPrivates[i].present = 0;
	    ++cnt;
	}
    else {
	for ( i=0; KnownPrivates[i].name!=NULL; ++i ) {
	    for ( j=0; j<sf->private->next; ++j )
		if ( strcmp(KnownPrivates[i].name,sf->private->keys[j])==0 )
	    break;
	    if ( !(KnownPrivates[i].present = (j<sf->private->next)) )
		++cnt;
	}
    }
    if ( cnt==0 )
	ti = NULL;
    else {
	ti = gcalloc(cnt+1,sizeof(GTextInfo));
	for ( i=cnt=0; KnownPrivates[i].name!=NULL; ++i )
	    if ( !KnownPrivates[i].present )
		ti[cnt++].text = uc_copy(KnownPrivates[i].name);
    }

    memset(&d,'\0',sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,150);
    pos.height = GDrawPointsToPixels(NULL,90);
    gw = GDrawCreateTopWindow(NULL,&pos,ask_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) "Key (in Private dictionary)";
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 6;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = 18; gcd[1].gd.pos.width = 130;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    gcd[1].creator = GTextFieldCreate;
    if ( ti!=NULL ) {
	gcd[1].gd.u.list = ti;
	gcd[1].creator = GListFieldCreate;
    }

    gcd[2].gd.pos.x = 20-3; gcd[2].gd.pos.y = 90-35-3;
    gcd[2].gd.pos.width = 55; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[2].text = (unichar_t *) "OK";
    label[2].text_is_1byte = true;
    gcd[2].gd.mnemonic = 'O';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = Ask_OK;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = 150-55-20; gcd[3].gd.pos.y = 90-35;
    gcd[3].gd.pos.width = 55; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) "Cancel";
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'C';
    gcd[3].gd.handle_controlevent = Ask_Cancel;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 2; gcd[4].gd.pos.y = 2;
    gcd[4].gd.pos.width = pos.width-4; gcd[4].gd.pos.height = pos.height-2;
    gcd[4].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[4].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    ret = NULL;
    if ( d.ret )
	ret = cu_copy(_GGadgetGetTitle(gcd[1].ret));
    GTextInfoListFree(ti);
    GDrawDestroyWindow(gw);
return( ret );
}

#define CID_PrivateEntries	1001
#define	CID_PrivateValues	1002
#define	CID_Add			1003
#define CID_Guess		1004
#define CID_Remove		1005

struct pi_data {
    SplineFont *sf;
    GWindow gw;
    int done;
    int old_sel;
};

static GTextInfo *PI_ListSet(SplineFont *sf) {
#if 1
    GTextInfo *ti = gcalloc((sf->private==NULL?0:sf->private->next)+1,sizeof( GTextInfo ));
#else
    GTextInfo *ti = gcalloc((sf->private==NULL?0:sf->private->next)+(sf->subrs==NULL?0:1)+1,sizeof( GTextInfo ));
#endif
    int i=0;

    if ( sf->private!=NULL ) {
	for ( i=0; i<sf->private->next; ++i ) {
	    ti[i].text = uc_copy(sf->private->keys[i]);
	}
    }
#if 0
    if ( sf->subrs!=NULL ) {
	ti[i++].text = uc_copy("Subrs");
    }
#endif
    if ( i!=0 )
	ti[0].selected = true;
return( ti );
}

static GTextInfo **PI_ListArray(SplineFont *sf) {
#if 1
    GTextInfo **ti = gcalloc((sf->private==NULL?0:sf->private->next)+1,sizeof( GTextInfo *));
#else
    GTextInfo **ti = gcalloc((sf->private==NULL?0:sf->private->next)+(sf->subrs==NULL?0:1)+1,sizeof( GTextInfo *));
#endif
    int i=0;

    if ( sf->private!=NULL ) {
	for ( i=0; i<sf->private->next; ++i ) {
	    ti[i] = gcalloc(1,sizeof(GTextInfo));
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	    ti[i]->text = uc_copy(sf->private->keys[i]);
	}
    }
#if 0
    if ( sf->subrs!=NULL ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	ti[i++]->text = uc_copy("Subrs");
    }
#endif
    ti[i] = gcalloc(1,sizeof(GTextInfo));
    if ( i!=0 )
	ti[0]->selected = true;
return( ti );
}

static int PIFinishFormer(struct pi_data *d) {
    static const unichar_t arrayquest[] = { 'E','x','p','e','c','t','e','d',' ','a','r','r','a','y','\n','P','r','o','c','e','d','e',' ','a','n','y','w','a','y','?',  '\0' };
    static const unichar_t numberquest[] = { 'E','x','p','e','c','t','e','d',' ','n','u','m','b','e','r','\n','P','r','o','c','e','d','e',' ','a','n','y','w','a','y','?',  '\0' };
    static const unichar_t boolquest[] = { 'E','x','p','e','c','t','e','d',' ','b','o','o','l','e','a','n','\n','P','r','o','c','e','d','e',' ','a','n','y','w','a','y','?',  '\0' };
    static const unichar_t codequest[] = { 'E','x','p','e','c','t','e','d',' ','c','o','d','e','\n','P','r','o','c','e','d','e',' ','a','n','y','w','a','y','?',  '\0' };
    static unichar_t title[] = { 'B','a','d',' ','t','y','p','e',  '\0' };
    static unichar_t ok[] = { 'O','K',  '\0' };
    static unichar_t cancel[] = { 'C','a','n','c','e','l',  '\0' };
    static unichar_t *buts[] = { ok, cancel, NULL };
    static unichar_t mn[] = { 'O', 'C', '\0' };
    unichar_t *end;

    if ( d->sf->private!=NULL && d->old_sel>=0 && d->old_sel!=d->sf->private->next ) {
	const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_PrivateValues));
	const unichar_t *pt = val;
	int i;

	/* does the type appear reasonable? */
	while ( isspace(*pt)) ++pt;
	for ( i=0; KnownPrivates[i].name!=NULL; ++i )
	    if ( strcmp(KnownPrivates[i].name,d->sf->private->keys[d->old_sel])==0 )
	break;
	if ( KnownPrivates[i].name!=NULL ) {
	    if ( KnownPrivates[i].type==pt_array ) {
		if ( *pt!='[' && GWidgetAsk(title,arrayquest,buts,mn,0,1)==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_boolean ) {
		if ( uc_strcmp(pt,"true")!=0 && uc_strcmp(pt,"false")!=0 &&
			GWidgetAsk(title,boolquest,buts,mn,0,1)==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_code ) {
		if ( *pt!='{' && GWidgetAsk(title,codequest,buts,mn,0,1)==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_number ) {
		u_strtod(pt,&end);
		while ( isspace(*end)) ++end;
		if ( *end!='\0' && GWidgetAsk(title,numberquest,buts,mn,0,1)==1 )
return( false );
	    }
	}

	/* Ok then set it */
	free(d->sf->private->values[d->old_sel]);
	d->sf->private->values[d->old_sel] = cu_copy(val);
	d->old_sel = -1;
    }
return( true );
}

static void ProcessListSel(struct pi_data *d) {
    GGadget *list = GWidgetGetControl(d->gw,CID_PrivateEntries);
    int sel = GGadgetGetFirstListSelectedItem(list);
    unichar_t *temp;
    static const unichar_t nullstr[] = { 0 };
    SplineFont *sf = d->sf;

    if ( d->old_sel==sel )
return;

    if ( !PIFinishFormer(d)) {
	/*GGadgetSelectListItem(list,sel,false);*/
	GGadgetSelectListItem(list,d->old_sel,true);
return;
    }
    if ( sel==-1 ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),false);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),nullstr);
#if 0
    } else if ( sf->private==NULL || sel==sf->private->next ) {
	/* Subrs entry */
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),true);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),false);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( "<Subroutine array,\n not human readable>" ));
	free( temp );
#endif
    } else {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),true);
	if ( strcmp(sf->private->keys[sel],"BlueValues")==0 ||
		strcmp(sf->private->keys[sel],"OtherBlues")==0 ||
		strcmp(sf->private->keys[sel],"StdHW")==0 ||
		strcmp(sf->private->keys[sel],"StemSnapH")==0 ||
		strcmp(sf->private->keys[sel],"StdVW")==0 ||
		strcmp(sf->private->keys[sel],"StemSnapV")==0 )
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),true);
	else
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),true);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( sf->private->values[sel]));
	free( temp );
    }
    d->old_sel = sel;
}

static int PI_Done(GGadget *g, GEvent *e) {
    GWindow gw;
    struct pi_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	if ( PIFinishFormer(d))
	    d->done = true;
    }
return( true );
}

static int PI_Add(GGadget *g, GEvent *e) {
    GWindow gw;
    struct pi_data *d;
    GGadget *list;
    int i;
    char *newkey;
    GTextInfo **ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	if ( !PIFinishFormer(d))
return(true);
	newkey = AskKey(d->sf);
	if ( newkey==NULL )
return( true );
	if ( d->sf->private==NULL ) {
	    d->sf->private = gcalloc(1,sizeof(struct psdict));
	    d->sf->private->cnt = 10;
	    d->sf->private->keys = gcalloc(10,sizeof(char *));
	    d->sf->private->values = gcalloc(10,sizeof(char *));
	}
	if (( i = PSDictFindEntry(d->sf->private,newkey))==-1 )
	    i = PSDictChangeEntry(d->sf->private,newkey,"");
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	ti = PI_ListArray(d->sf);
	if ( i>0 ) {
	    ti[0]->selected = false;
	    ti[i]->selected = true;
	}
	GGadgetSetList(list,ti,false);
	d->old_sel = -1;
	ProcessListSel(d);
	free(newkey);
    }
return( true );
}

static void arraystring(char *buffer,double *array,int cnt) {
    int i, ei;

    for ( ei=cnt; ei>1 && array[ei]==0; --ei );
    *buffer++ = '[';
    for ( i=0; i<ei; ++i ) {
	sprintf(buffer, "%d ", (int) array[i]);
	buffer += strlen(buffer);
    }
    if ( buffer[-1] ==' ' ) --buffer;
    *buffer++ = ']'; *buffer='\0';
}

static void SnapSet(SplineFont *sf,double stemsnap[12], double snapcnt[12],
	char *name1, char *name2 ) {
    int i, mi;
    char buffer[211];

    mi = -1;
    for ( i=0; stemsnap[i]!=0 && i<12; ++i )
	if ( mi==-1 ) mi = i;
	else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
    if ( mi==-1 )
return;
    sprintf( buffer, "[%d]", (int) stemsnap[mi]);
    PSDictChangeEntry(sf->private,name1,buffer);
    arraystring(buffer,stemsnap,12);
    PSDictChangeEntry(sf->private,name2,buffer);
}

static int PI_Guess(GGadget *g, GEvent *e) {
    GWindow gw;
    struct pi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    static const unichar_t bluequest[] = { 'T','h','i','s',' ','w','i','l','l',' ','c','h','a','n','g','e',' ','b','o','t','h',' ','B','l','u','e','V','a','l','u','e','s',' ','a','n','d',' ','O','t','h','e','r','B','l','u','e','s','.','\n','D','o',' ','y','o','u',' ','w','a','n','t',' ','t','o',' ','c','o','n','t','i','n','u','e','?',  '\0' };
    static const unichar_t hquest[] = { 'T','h','i','s',' ','w','i','l','l',' ','c','h','a','n','g','e',' ','b','o','t','h',' ','S','t','d','H','W',' ','a','n','d',' ','S','t','e','m','S','n','a','p','H','.','\n','D','o',' ','y','o','u',' ','w','a','n','t',' ','t','o',' ','c','o','n','t','i','n','u','e','?',  '\0' };
    static const unichar_t vquest[] = { 'T','h','i','s',' ','w','i','l','l',' ','c','h','a','n','g','e',' ','b','o','t','h',' ','S','t','d','V','W',' ','a','n','d',' ','S','t','e','m','S','n','a','p','V','.','\n','D','o',' ','y','o','u',' ','w','a','n','t',' ','t','o',' ','c','o','n','t','i','n','u','e','?',  '\0' };
    static unichar_t title[] = { 'G','u','e','s','s','.','.','.',  '\0' };
    static unichar_t ok[] = { 'O','K',  '\0' };
    static unichar_t cancel[] = { 'C','a','n','c','e','l',  '\0' };
    static unichar_t *buts[] = { ok, cancel, NULL };
    static unichar_t mn[] = { 'O', 'C', '\0' };
    double bluevalues[14], otherblues[10];
    double snapcnt[12];
    double stemsnap[12];
    char buffer[211];
    unichar_t *temp;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	sf = d->sf;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	if ( strcmp(sf->private->keys[sel],"BlueValues")==0 ||
		strcmp(sf->private->keys[sel],"OtherBlues")==0 ) {
	    if ( GWidgetAsk(title,bluequest,buts,mn,0,1)==1 )
return( true );
	    FindBlues(sf,bluevalues,otherblues);
	    arraystring(buffer,bluevalues,14);
	    PSDictChangeEntry(sf->private,"BlueValues",buffer);
	    arraystring(buffer,otherblues,10);
	    PSDictChangeEntry(sf->private,"OtherBlues",buffer);
	} else if ( strcmp(sf->private->keys[sel],"StdHW")==0 ||
		strcmp(sf->private->keys[sel],"StemSnapH")==0 ) {
	    if ( GWidgetAsk(title,hquest,buts,mn,0,1)==1 )
return( true );
	    FindHStems(sf,stemsnap,snapcnt);
	    SnapSet(sf,stemsnap,snapcnt,"StdHW","StemSnapH");
	} else if ( strcmp(sf->private->keys[sel],"StdVW")==0 ||
		strcmp(sf->private->keys[sel],"StemSnapV")==0 ) {
	    if ( GWidgetAsk(title,vquest,buts,mn,0,1)==1 )
return( true );
	    FindHStems(sf,stemsnap,snapcnt);
	    SnapSet(sf,stemsnap,snapcnt,"StdVW","StemSnapV");
	}
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( sf->private->values[sel]));
	free( temp );
    }
return( true );
}

static int PI_Delete(GGadget *g, GEvent *e) {
    GWindow gw;
    struct pi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    GTextInfo **ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	sf = d->sf;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
#if 0
	if ( sf->private==NULL || sel == sf->private->next ||
		strcmp(sf->private->keys[sel],"OtherSubrs")==0 ) {
	    static const unichar_t quest[] = { 'D','e','l','e','t','i','n','g',' ','t','h','i','s',' ','e','n','t','r','y',' ','w','i','l','l',' ','l','o','s','e',' ','a','l','l',' ','h','i','n','t',' ','s','u','b','s','t','i','t','u','t','i','o','n',' ','a','n','d',' ','f','l','e','x',' ','h','i','n','t',' ','i','n','f','o','r','m','a','t','i','o','n','\n','D','o',' ','y','o','u',' ','s','t','i','l','l',' ','w','a','n','t',' ','t','o',' ','d','e','l','e','t','e',' ','i','t','?',  '\0' };
	    static unichar_t title[] = { 'D','e','l','e','t','e',' ','H','i','n','t','s','?',  '\0' };
	    static unichar_t ok[] = { 'O','K',  '\0' };
	    static unichar_t cancel[] = { 'C','a','n','c','e','l',  '\0' };
	    static unichar_t *buts[] = { ok, cancel, NULL };
	    static unichar_t mn[] = { 'O', 'C', '\0' };
	    if ( GWidgetAsk(title,quest,buts,mn,0,1)==1 )
return ( true );
	    PSDictRemoveEntry(sf->private, "OtherSubrs");
	    PSCharsFree(sf->subrs);
	    sf->subrs = NULL;
	    for ( i=0; i<sf->charcnt; ++i ) {
		if ( sf->chars[i]!=NULL && sf->chars[i]->origtype1!=NULL ) {
		    free( sf->chars[i]->origtype1 );
		    sf->chars[i]->origtype1 = NULL;
		    sf->chars[i]->origlen = 0;
		}
	    }
	} else
#endif
	    PSDictRemoveEntry(sf->private, sf->private->keys[sel]);
	sf->changed = true;
	ti = PI_ListArray(sf);
	--sel;
	if ( sf->private!=NULL && sel>=sf->private->next )
	    sel = sf->private->next-1;
	if ( sel>0 ) {
	    ti[0]->selected = false;
	    ti[sel]->selected = true;
	}
	GGadgetSetList(list,ti,false);
	d->old_sel = -2;
	ProcessListSel(d);
    }
return( true );
}

static int PI_ListSel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	ProcessListSel(GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int pi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct pi_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void SFPrivateInfo(SplineFont *sf) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct pi_data d;
    static unichar_t title[] = { 'P','r','i','v', 'a', 't','e',' ','I','n','f','o','r','m','a','t','i','o','n',  '\0' };

    memset(&d,'\0',sizeof(d));
    d.sf = sf;
    d.old_sel = -2;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,220);
    pos.height = GDrawPointsToPixels(NULL,275);
    d.gw = gw = GDrawCreateTopWindow(NULL,&pos,pi_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 6;
    gcd[0].gd.pos.width = 200; gcd[0].gd.pos.height = 8*12+10;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].gd.cid = CID_PrivateEntries;
    gcd[0].gd.u.list = PI_ListSet(sf);
    gcd[0].gd.handle_controlevent = PI_ListSel;
    gcd[0].creator = GListCreate;

    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = gcd[0].gd.pos.y+gcd[0].gd.pos.height+10;
    gcd[1].gd.pos.width = gcd[0].gd.pos.width; gcd[1].gd.pos.height = 6*12+10;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    gcd[1].gd.cid = CID_PrivateValues;
    gcd[1].creator = GTextAreaCreate;

    gcd[2].gd.pos.x = 20; gcd[2].gd.pos.y = 275-35-30;
    gcd[2].gd.pos.width = 55; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled ;
    label[2].text = (unichar_t *) "Add";
    label[2].text_is_1byte = true;
    gcd[2].gd.mnemonic = 'A';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = PI_Add;
    gcd[2].gd.cid = CID_Add;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = (220-55)/2; gcd[3].gd.pos.y = 275-35-30;
    gcd[3].gd.pos.width = 55; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible ;
    label[3].text = (unichar_t *) "Guess";
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'G';
    gcd[3].gd.handle_controlevent = PI_Guess;
    gcd[3].gd.cid = CID_Guess;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 220-55-20; gcd[4].gd.pos.y = 275-35-30;
    gcd[4].gd.pos.width = 55; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled ;
    label[4].text = (unichar_t *) "Remove";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.mnemonic = 'R';
    gcd[4].gd.handle_controlevent = PI_Delete;
    gcd[4].gd.cid = CID_Remove;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = (220-55-6)/2; gcd[5].gd.pos.y = 275-35-3;
    gcd[5].gd.pos.width = 55+6; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_but_cancel;
    label[5].text = (unichar_t *) "Done";
    label[5].text_is_1byte = true;
    gcd[5].gd.mnemonic = 'D';
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = PI_Done;
    gcd[5].creator = GButtonCreate;

    gcd[6].gd.pos.x = 2; gcd[6].gd.pos.y = 2;
    gcd[6].gd.pos.width = pos.width-4; gcd[6].gd.pos.height = pos.height-2;
    gcd[6].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[6].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GTextInfoListFree(gcd[0].gd.u.list);

    ProcessListSel(&d);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
