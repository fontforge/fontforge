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
#include "gdraw.h"
#include "gkeysym.h"
#include "gresource.h"
#include "gwidget.h"
#include "ggadgetP.h"
#include "ustring.h"
#include "utype.h"
#include <math.h>

extern void (*_GDraw_InsCharHook)(GDisplay *,unichar_t);

GBox _GGadget_gtextfield_box = GBOX_EMPTY; /* Don't initialize here */
static GBox glistfield_box = GBOX_EMPTY; /* Don't initialize here */
static GBox glistfieldmenu_box = GBOX_EMPTY; /* Don't initialize here */
static GBox gnumericfield_box = GBOX_EMPTY; /* Don't initialize here */
static GBox gnumericfieldspinner_box = GBOX_EMPTY; /* Don't initialize here */
FontInstance *_gtextfield_font = NULL;
static int gtextfield_inited = false;

static GResInfo listfield_ri, listfieldmenu_ri, numericfield_ri, numericfieldspinner_ri;
static GTextInfo text_lab[] = {
    { (unichar_t *) "Disabled", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Enabled" , NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' }
};
static GTextInfo list_choices[] = {
    { (unichar_t *) "1", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "2", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "3", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};
static GGadgetCreateData text_gcd[] = {
    { GTextFieldCreate, { { 0, 0, 70, 0 }, NULL, 0, 0, 0, 0, 0, &text_lab[0], { NULL }, gg_visible, NULL, NULL }, NULL, NULL },
    { GTextFieldCreate, { { 0, 0, 70, 0 }, NULL, 0, 0, 0, 0, 0, &text_lab[1], { NULL }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *tarray[] = { GCD_Glue, &text_gcd[0], GCD_Glue, &text_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData textbox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) tarray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo gtextfield_ri = {
    &listfield_ri, &ggadget_ri,NULL, NULL,
    &_GGadget_gtextfield_box,
    &_gtextfield_font,
    &textbox,
    NULL,
    N_("Text Field"),
    N_("Text Field"),
    "GTextField",
    "Gdraw",
    false,
    omf_font|omf_padding,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GGadgetCreateData textlist_gcd[] = {
    { GListFieldCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &text_lab[0], { list_choices }, gg_visible, NULL, NULL }, NULL, NULL },
    { GListFieldCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &text_lab[1], { list_choices }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *tlarray[] = { GCD_Glue, &textlist_gcd[0], GCD_Glue, &textlist_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData textlistbox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) tlarray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo listfield_ri = {
    &listfieldmenu_ri, &gtextfield_ri,&listfieldmenu_ri, &listmark_ri,
    &glistfield_box,
    NULL,
    &textlistbox,
    NULL,
    N_("List Field"),
    N_("List Field (Combo Box)"),
    "GComboBox",
    "Gdraw",
    false,
    0,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GResInfo listfieldmenu_ri = {
    &numericfield_ri, &listfield_ri, &listmark_ri,NULL,
    &glistfieldmenu_box,
    NULL,
    &textlistbox,
    NULL,
    N_("List Field Menu"),
    N_("Box surrounding the ListMark in a list field (combobox)"),
    "GComboBoxMenu",
    "Gdraw",
    false,
    omf_padding,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GGadgetCreateData num_gcd[] = {
    { GNumericFieldCreate, { { 0, 0, 50, 0 }, NULL, 0, 0, 0, 0, 0, &list_choices[0], { NULL }, gg_visible, NULL, NULL }, NULL, NULL },
    { GNumericFieldCreate, { { 0, 0, 50, 0 }, NULL, 0, 0, 0, 0, 0, &list_choices[0], { NULL }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *narray[] = { GCD_Glue, &num_gcd[0], GCD_Glue, &num_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData numbox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) narray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo numericfield_ri = {
    &numericfieldspinner_ri, &gtextfield_ri,&numericfieldspinner_ri, NULL,
    &gnumericfield_box,
    NULL,
    &numbox,
    NULL,
    N_("Numeric Field"),
    N_("Numeric Field (Spinner)"),
    "GNumericField",
    "Gdraw",
    false,
    0,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GResInfo numericfieldspinner_ri = {
    NULL, &numericfield_ri,NULL, NULL,
    &gnumericfieldspinner_box,
    NULL,
    &numbox,
    NULL,
    N_("Numeric Field Sign"),
    N_("The box around the up/down arrows of a numeric field (spinner)"),
    "GNumericFieldSpinner",
    "Gdraw",
    false,
    omf_border_type|omf_border_width|omf_padding,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static unichar_t nullstr[] = { 0 }, nstr[] = { 'n', 0 },
	newlinestr[] = { '\n', 0 }, tabstr[] = { '\t', 0 };

static void GListFieldSelected(GGadget *g, int i);
static int GTextField_Show(GTextField *gt, int pos);
static void GTPositionGIC(GTextField *gt);

static void GCompletionDestroy(GCompletionField *gc);
static void GTextFieldComplete(GTextField *gt,int from_tab);
static int GCompletionHandleKey(GTextField *gt,GEvent *event);


static int u2utf8_index(int pos,const char *start) {
    const char *pt = start;

    while ( --pos>=0 )
	utf8_ildb(&pt);
return( pt-start );
}

static int utf82u_index(int pos, const char *start) {
    int uc = 0;
    const char *end = start+pos;

    while ( start<end ) {
	utf8_ildb(&start);
	++uc;
    }
return( uc );
}

static void GTextFieldChanged(GTextField *gt,int src) {
    GEvent e;

    e.type = et_controlevent;
    e.w = gt->g.base;
    e.u.control.subtype = et_textchanged;
    e.u.control.g = &gt->g;
    e.u.control.u.tf_changed.from_pulldown = src;
    if ( gt->g.handle_controlevent != NULL )
	(gt->g.handle_controlevent)(&gt->g,&e);
    else
	GDrawPostEvent(&e);
}

static void GTextFieldFocusChanged(GTextField *gt,int gained) {
    GEvent e;

    if ( (gt->g.box->flags & box_active_border_inner) &&
	    ( gt->g.state==gs_enabled || gt->g.state==gs_active )) {
	int state = gained?gs_active:gs_enabled;
	if ( state!=gt->g.state ) {
	    gt->g.state = state;
	    GGadgetRedraw((GGadget *) gt);
	}
    }
    e.type = et_controlevent;
    e.w = gt->g.base;
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.g = &gt->g;
    e.u.control.u.tf_focus.gained_focus = gained;
    if ( gt->g.handle_controlevent != NULL )
	(gt->g.handle_controlevent)(&gt->g,&e);
    else
	GDrawPostEvent(&e);
}

static void GTextFieldPangoRefigureLines(GTextField *gt, int start_of_change) {
    char *utf8_text, *pt, *ept;
    unichar_t *upt, *uept;
    int i, uc;
    GRect size;

    free(gt->utf8_text);
    if ( gt->lines8==NULL ) {
	gt->lines8 = malloc(gt->lmax*sizeof(int32));
	gt->lines8[0] = 0;
	gt->lines8[1] = -1;
    }

    if ( gt->password ) {
	int cnt = u_strlen(gt->text);
	utf8_text = malloc(cnt+1);
	memset(utf8_text,'*',cnt);
	utf8_text[cnt] = '\0';
    } else
	utf8_text = u2utf8_copy(gt->text);
    gt->utf8_text = utf8_text;
    GDrawLayoutInit(gt->g.base,utf8_text,-1,NULL);

    if ( !gt->multi_line ) {
	GDrawLayoutExtents(gt->g.base,&size);
	gt->xmax = size.width;
return;
    }

    if ( !gt->wrap ) {
	pt = utf8_text;
	i=0;
	while ( ( ept = strchr(pt,'\n'))!=NULL ) {
	    if ( i>=gt->lmax ) {
		gt->lines8 = realloc(gt->lines8,(gt->lmax+=10)*sizeof(int32));
		gt->lines = realloc(gt->lines,gt->lmax*sizeof(int32));
	    }
	    gt->lines8[i++] = pt-utf8_text;
	    pt = ept+1;
	}
	if ( i>=gt->lmax ) {
	    gt->lines8 = realloc(gt->lines8,(gt->lmax+=10)*sizeof(int32));
	    gt->lines = realloc(gt->lines,gt->lmax*sizeof(int32));
	}
	gt->lines8[i++] = pt-utf8_text;

	upt = gt->text;
	i = 0;
	while ( ( uept = u_strchr(upt,'\n'))!=NULL ) {
	    gt->lines[i++] = upt-gt->text;
	    upt = uept+1;
	}
	gt->lines[i++] = upt-gt->text;
    } else {
	int lcnt;
	GDrawLayoutSetWidth(gt->g.base,gt->g.inner.width);
	lcnt = GDrawLayoutLineCount(gt->g.base);
	if ( lcnt+2>=gt->lmax ) {
	    gt->lines8 = realloc(gt->lines8,(gt->lmax=lcnt+10)*sizeof(int32));
	    gt->lines = realloc(gt->lines,gt->lmax*sizeof(int32));
	}
	pt = utf8_text; uc=0;
	for ( i=0; i<lcnt; ++i ) {
	    gt->lines8[i] = GDrawLayoutLineStart(gt->g.base,i);
	    ept = utf8_text + gt->lines8[i];
	    while ( pt<ept ) {
		++uc;
		utf8_ildb((const char **) &pt);
	    }
	    gt->lines[i] = uc;
	}
	if ( i==0 ) {
	    gt->lines8[i] = strlen(utf8_text);
	    gt->lines[i] = u_strlen(gt->text);
	} else {
	    gt->lines8[i] = gt->lines8[i-1] +   strlen( utf8_text + gt->lines8[i-1]);
	    gt->lines [i] = gt->lines [i-1] + u_strlen(  gt->text + gt->lines [i-1]);
	}
    }
    if ( gt->lcnt!=i ) {
	gt->lcnt = i;
	if ( gt->vsb!=NULL )
	    GScrollBarSetBounds(&gt->vsb->g,0,gt->lcnt,
		    gt->g.inner.height<gt->fh? 1 : gt->g.inner.height/gt->fh);
	if ( gt->loff_top+gt->g.inner.height/gt->fh>gt->lcnt ) {
	    gt->loff_top = gt->lcnt-gt->g.inner.height/gt->fh;
	    if ( gt->loff_top<0 ) gt->loff_top = 0;
	    if ( gt->vsb!=NULL )
		GScrollBarSetPos(&gt->vsb->g,gt->loff_top);
	}
    }
    if ( i>=gt->lmax )
	gt->lines = realloc(gt->lines,(gt->lmax+=10)*sizeof(int32));
    gt->lines8[i] = -1;
    gt->lines[i++] = -1;

    GDrawLayoutExtents(gt->g.base,&size);
    gt->xmax = size.width;

    if ( gt->hsb!=NULL ) {
	GScrollBarSetBounds(&gt->hsb->g,0,gt->xmax,gt->g.inner.width);
    }
    GDrawLayoutSetWidth(gt->g.base,-1);
}

static void GTextFieldRefigureLines(GTextField *gt, int start_of_change) {
    GDrawSetFont(gt->g.base,gt->font);
    if ( gt->lines==NULL ) {
	gt->lines = malloc(10*sizeof(int32));
	gt->lines[0] = 0;
	gt->lines[1] = -1;
	gt->lmax = 10;
	gt->lcnt = 1;
	if ( gt->vsb!=NULL )
	    GScrollBarSetBounds(&gt->vsb->g,0,gt->lcnt,
		    gt->g.inner.height<gt->fh ? 1 : gt->g.inner.height/gt->fh);
    }

    GTextFieldPangoRefigureLines(gt,start_of_change);
return;
}

static void _GTextFieldReplace(GTextField *gt, const unichar_t *str) {
    unichar_t *old = gt->oldtext;
    unichar_t *new = malloc((u_strlen(gt->text)-(gt->sel_end-gt->sel_start) + u_strlen(str)+1)*sizeof(unichar_t));

    gt->oldtext = gt->text;
    gt->sel_oldstart = gt->sel_start;
    gt->sel_oldend = gt->sel_end;
    gt->sel_oldbase = gt->sel_base;

    u_strncpy(new,gt->text,gt->sel_start);
    u_strcpy(new+gt->sel_start,str);
    gt->sel_start = u_strlen(new);
    u_strcpy(new+gt->sel_start,gt->text+gt->sel_end);
    gt->text = new;
    gt->sel_end = gt->sel_base = gt->sel_start;
    free(old);

    GTextFieldRefigureLines(gt,gt->sel_oldstart);
}

static void GTextField_Replace(GTextField *gt, const unichar_t *str) {
    _GTextFieldReplace(gt,str);
    GTextField_Show(gt,gt->sel_start);
}

static int GTextFieldFindLine(GTextField *gt, int pos) {
    int i;
    for ( i=0; gt->lines[i+1]!=-1; ++i )
	if ( pos<gt->lines[i+1])
    break;
return( i );
}

static unichar_t *GTextFieldGetPtFromPos(GTextField *gt,int i,int xpos) {
    int ll;
    unichar_t *end;

    ll = gt->lines[i+1]==-1?-1:gt->lines[i+1]-gt->lines[i]-1;
    int index8, uc;
    if ( gt->lines8[i+1]==-1 )
	GDrawLayoutInit(gt->g.base,gt->utf8_text + gt->lines8[i],-1,NULL);
    else {
	GDrawLayoutInit(gt->g.base,gt->utf8_text + gt->lines8[i], gt->lines8[i+1]-gt->lines8[i], NULL);
    }
    index8 = GDrawLayoutXYToIndex(gt->g.base,xpos-gt->g.inner.x+gt->xoff_left,0);
    uc = utf82u_index(index8,gt->utf8_text + gt->lines8[i]);
    end = gt->text + gt->lines[i] + uc;
return( end );
}

static int GTextField_Show(GTextField *gt, int pos) {
    int i, ll, xoff, loff;
    int refresh=false;
    GListField *ge = (GListField *) gt;
    int width = gt->g.inner.width;

    if ( gt->listfield || gt->numericfield )
	width = ge->fieldrect.width - 2*(gt->g.inner.x - gt->g.r.x);

    if ( pos < 0 ) pos = 0;
    if ( pos > u_strlen(gt->text)) pos = u_strlen(gt->text);
    i = GTextFieldFindLine(gt,pos);

    loff = gt->loff_top;
    if ( gt->lcnt<gt->g.inner.height/gt->fh || loff==0 )
	loff = 0;
    if ( i<loff )
	loff = i;
    if ( i>=loff+gt->g.inner.height/gt->fh ) {
	loff = i-(gt->g.inner.height/gt->fh);
	if ( gt->g.inner.height/gt->fh>2 )
	    ++loff;
    }

    xoff = gt->xoff_left;
    if ( gt->lines[i+1]==-1 ) ll = -1; else ll = gt->lines[i+1]-gt->lines[i]-1;
    GRect size;
    if ( gt->lines8[i+1]==-1 ) ll = strlen(gt->utf8_text+gt->lines8[i]); else ll = gt->lines8[i+1]-gt->lines8[i]-1;
    GDrawLayoutInit(gt->g.base,gt->utf8_text+gt->lines8[i],ll,NULL);
    GDrawLayoutExtents(gt->g.base,&size);
    if ( size.width < width )
	    xoff = 0;
    else {
	int index8 = u2utf8_index(pos- gt->lines8[i],gt->utf8_text + gt->lines8[i]);
	GDrawLayoutIndexToPos(gt->g.base,index8,&size);
	if ( size.x + 2*size.width < width )
	    xoff = 0;
	else
	    xoff = size.x - (width - size.width)/2;
	if ( xoff<0 )
	    xoff = 0;
    }

    if ( xoff!=gt->xoff_left ) {
	gt->xoff_left = xoff;
	if ( gt->hsb!=NULL )
	    GScrollBarSetPos(&gt->hsb->g,xoff);
	refresh = true;
    }
    if ( loff!=gt->loff_top ) {
	gt->loff_top = loff;
	if ( gt->vsb!=NULL )
	    GScrollBarSetPos(&gt->vsb->g,loff);
	refresh = true;
    }
    GTPositionGIC(gt);
return( refresh );
}

static void *genunicodedata(void *_gt,int32 *len) {
    GTextField *gt = _gt;
    unichar_t *temp;
    *len = gt->sel_end-gt->sel_start + 1;
    temp = malloc((*len+2)*sizeof(unichar_t));
    temp[0] = 0xfeff;		/* KDE expects a byte order flag */
    u_strncpy(temp+1,gt->text+gt->sel_start,gt->sel_end-gt->sel_start);
    temp[*len+1] = 0;
return( temp );
}

static void *genutf8data(void *_gt,int32 *len) {
    GTextField *gt = _gt;
    unichar_t *temp =u_copyn(gt->text+gt->sel_start,gt->sel_end-gt->sel_start);
    char *ret = u2utf8_copy(temp);
    free(temp);
    *len = strlen(ret);
return( ret );
}

static void *ddgenunicodedata(void *_gt,int32 *len) {
    void *temp = genunicodedata(_gt,len);
    GTextField *gt = _gt;
    _GTextFieldReplace(gt,nullstr);
    _ggadget_redraw(&gt->g);
return( temp );
}

static void *genlocaldata(void *_gt,int32 *len) {
    GTextField *gt = _gt;
    unichar_t *temp =u_copyn(gt->text+gt->sel_start,gt->sel_end-gt->sel_start);
    char *ret = u2def_copy(temp);
    free(temp);
    *len = strlen(ret);
return( ret );
}

static void *ddgenlocaldata(void *_gt,int32 *len) {
    void *temp = genlocaldata(_gt,len);
    GTextField *gt = _gt;
    _GTextFieldReplace(gt,nullstr);
    _ggadget_redraw(&gt->g);
return( temp );
}

static void noop(void *_gt) {
}

static void GTextFieldGrabPrimarySelection(GTextField *gt) {
    int ss = gt->sel_start, se = gt->sel_end;

    GDrawGrabSelection(gt->g.base,sn_primary);
    gt->sel_start = ss; gt->sel_end = se;
    GDrawAddSelectionType(gt->g.base,sn_primary,"text/plain;charset=ISO-10646-UCS-4",gt,gt->sel_end-gt->sel_start,
	    sizeof(unichar_t),
	    genunicodedata,noop);
    GDrawAddSelectionType(gt->g.base,sn_primary,"UTF8_STRING",gt,gt->sel_end-gt->sel_start,
	    sizeof(char),
	    genutf8data,noop);
    GDrawAddSelectionType(gt->g.base,sn_primary,"text/plain;charset=UTF-8",gt,gt->sel_end-gt->sel_start,
	    sizeof(char),
	    genutf8data,noop);
    GDrawAddSelectionType(gt->g.base,sn_primary,"STRING",gt,gt->sel_end-gt->sel_start,
	    sizeof(char),
	    genlocaldata,noop);
}

static void GTextFieldGrabDDSelection(GTextField *gt) {

    GDrawGrabSelection(gt->g.base,sn_drag_and_drop);
    GDrawAddSelectionType(gt->g.base,sn_drag_and_drop,"text/plain;charset=ISO-10646-UCS-4",gt,gt->sel_end-gt->sel_start,
	    sizeof(unichar_t),
	    ddgenunicodedata,noop);
    GDrawAddSelectionType(gt->g.base,sn_drag_and_drop,"STRING",gt,gt->sel_end-gt->sel_start,sizeof(char),
	    ddgenlocaldata,noop);
}

static void GTextFieldGrabSelection(GTextField *gt, enum selnames sel ) {

    if ( gt->sel_start!=gt->sel_end ) {
	unichar_t *temp;
	char *ctemp, *ctemp2;
	int i;
	uint16 *u2temp;

	GDrawGrabSelection(gt->g.base,sel);
	temp = malloc((gt->sel_end-gt->sel_start + 2)*sizeof(unichar_t));
	temp[0] = 0xfeff;		/* KDE expects a byte order flag */
	u_strncpy(temp+1,gt->text+gt->sel_start,gt->sel_end-gt->sel_start);
	ctemp = u2utf8_copy(temp+1);
	ctemp2 = u2def_copy(temp+1);
	GDrawAddSelectionType(gt->g.base,sel,"text/plain;charset=ISO-10646-UCS-4",temp,u_strlen(temp),
		sizeof(unichar_t),
		NULL,NULL);
	u2temp = malloc((gt->sel_end-gt->sel_start + 2)*sizeof(uint16));
	for ( i=0; temp[i]!=0; ++i )
	    u2temp[i] = temp[i];
	u2temp[i] = 0;
	GDrawAddSelectionType(gt->g.base,sel,"text/plain;charset=ISO-10646-UCS-2",u2temp,u_strlen(temp),
		2,
		NULL,NULL);
	GDrawAddSelectionType(gt->g.base,sel,"UTF8_STRING",copy(ctemp),strlen(ctemp),
		sizeof(char),
		NULL,NULL);
	GDrawAddSelectionType(gt->g.base,sel,"text/plain;charset=UTF-8",ctemp,strlen(ctemp),
		sizeof(char),
		NULL,NULL);

	if ( ctemp2!=NULL && *ctemp2!='\0' /*strlen(ctemp2)==gt->sel_end-gt->sel_start*/ )
	    GDrawAddSelectionType(gt->g.base,sel,"STRING",ctemp2,strlen(ctemp2),
		    sizeof(char),
		    NULL,NULL);
	else
	    free(ctemp2);
    }
}

static int GTextFieldSelBackword(unichar_t *text,int start) {
    unichar_t ch = text[start-1];

    if ( start==0 )
	/* Can't go back */;
    else if ( isalnum(ch) || ch=='_' ) {
	int i;
	for ( i=start-1; i>=0 && ((text[i]<0x10000 && isalnum(text[i])) || text[i]=='_') ; --i );
	start = i+1;
    } else {
	int i;
	for ( i=start-1; i>=0 && !(text[i]<0x10000 && isalnum(text[i])) && text[i]!='_' ; --i );
	start = i+1;
    }
return( start );
}

static int GTextFieldSelForeword(unichar_t *text,int end) {
    unichar_t ch = text[end];

    if ( ch=='\0' )
	/* Nothing */;
    else if ( isalnum(ch) || ch=='_' ) {
	int i;
	for ( i=end; (text[i]<0x10000 && isalnum(text[i])) || text[i]=='_' ; ++i );
	end = i;
    } else {
	int i;
	for ( i=end; !(text[i]<0x10000 && isalnum(text[i])) && text[i]!='_' && text[i]!='\0' ; ++i );
	end = i;
    }
return( end );
}

static void GTextFieldSelectWord(GTextField *gt,int mid, int16 *start, int16 *end) {
    unichar_t *text;
    unichar_t ch = gt->text[mid];

    text = gt->text;
    ch = text[mid];

    if ( ch=='\0' )
	*start = *end = mid;
    else if ( (ch<0x10000 && isspace(ch)) ) {
	int i;
	for ( i=mid; text[i]<0x10000 && isspace(text[i]); ++i );
	*end = i;
	for ( i=mid-1; i>=0 && text[i]<0x10000 && isspace(text[i]) ; --i );
	*start = i+1;
    } else if ( (ch<0x10000 && isalnum(ch)) || ch=='_' ) {
	int i;
	for ( i=mid; (text[i]<0x10000 && isalnum(text[i])) || text[i]=='_' ; ++i );
	*end = i;
	for ( i=mid-1; i>=0 && ((text[i]<0x10000 && isalnum(text[i])) || text[i]=='_') ; --i );
	*start = i+1;
    } else {
	int i;
	for ( i=mid; !(text[i]<0x10000 && isalnum(text[i])) && text[i]!='_' && text[i]!='\0' ; ++i );
	*end = i;
	for ( i=mid-1; i>=0 && !(text[i]<0x10000 && isalnum(text[i])) && text[i]!='_' ; --i );
	*start = i+1;
    }
}

static void GTextFieldSelectWords(GTextField *gt,int last) {
    int16 ss, se;
    GTextFieldSelectWord(gt,gt->sel_base,&gt->sel_start,&gt->sel_end);
    if ( last!=gt->sel_base ) {
	GTextFieldSelectWord(gt,last,&ss,&se);
	if ( ss<gt->sel_start ) gt->sel_start = ss;
	if ( se>gt->sel_end ) gt->sel_end = se;
    }
}

static void GTextFieldPaste(GTextField *gt,enum selnames sel) {
    if ( GDrawSelectionHasType(gt->g.base,sel,"UTF8_STRING") ||
	    GDrawSelectionHasType(gt->g.base,sel,"text/plain;charset=UTF-8")) {
	unichar_t *temp; char *ctemp;
	int32 len;
	ctemp = GDrawRequestSelection(gt->g.base,sel,"UTF8_STRING",&len);
	if ( ctemp==NULL || len==0 )
	    ctemp = GDrawRequestSelection(gt->g.base,sel,"text/plain;charset=UTF-8",&len);
	if ( ctemp!=NULL ) {
	    temp = utf82u_copyn(ctemp,strlen(ctemp));
	    GTextField_Replace(gt,temp);
	    free(ctemp); free(temp);
	}
/* Bug in the xorg library on 64 bit machines and 32 bit transfers don't work */
/*  so avoid them, by looking for utf8 first */
    } else if ( GDrawSelectionHasType(gt->g.base,sel,"text/plain;charset=ISO-10646-UCS-4")) {
	unichar_t *temp;
	int32 len;
	temp = GDrawRequestSelection(gt->g.base,sel,"text/plain;charset=ISO-10646-UCS-4",&len);
	/* Bug! I don't handle byte reversed selections. But I don't think there should be any anyway... */
	if ( temp!=NULL )
	    GTextField_Replace(gt,temp[0]==0xfeff?temp+1:temp);
	free(temp);
    } else if ( GDrawSelectionHasType(gt->g.base,sel,"Unicode") ||
	    GDrawSelectionHasType(gt->g.base,sel,"text/plain;charset=ISO-10646-UCS-2")) {
	unichar_t *temp;
	uint16 *temp2;
	int32 len;
	temp2 = GDrawRequestSelection(gt->g.base,sel,"text/plain;charset=ISO-10646-UCS-2",&len);
	if ( temp2==NULL || len==0 )
	    temp2 = GDrawRequestSelection(gt->g.base,sel,"Unicode",&len);
	if ( temp2!=NULL ) {
	    int i;
	    temp = malloc((len/2+1)*sizeof(unichar_t));
	    for ( i=0; temp2[i]!=0; ++i )
		temp[i] = temp2[i];
	    temp[i] = 0;
	    GTextField_Replace(gt,temp[0]==0xfeff?temp+1:temp);
	    free(temp);
	}
	free(temp2);
    } else if ( GDrawSelectionHasType(gt->g.base,sel,"STRING")) {
	unichar_t *temp; char *ctemp;
	int32 len;
	ctemp = GDrawRequestSelection(gt->g.base,sel,"STRING",&len);
	if ( ctemp==NULL || len==0 )
	    ctemp = GDrawRequestSelection(gt->g.base,sel,"text/plain;charset=UTF-8",&len);
	if ( ctemp!=NULL ) {
	    temp = def2u_copy(ctemp);
	    GTextField_Replace(gt,temp);
	    free(ctemp); free(temp);
	}
    }
}

static int gtextfield_editcmd(GGadget *g,enum editor_commands cmd) {
    GTextField *gt = (GTextField *) g;

    switch ( cmd ) {
      case ec_selectall:
	gt->sel_start = 0;
	gt->sel_end = u_strlen(gt->text);
return( true );
      case ec_clear:
	GTextField_Replace(gt,nullstr);
	_ggadget_redraw(g);
return( true );
      case ec_cut:
	GTextFieldGrabSelection(gt,sn_clipboard);
	GTextField_Replace(gt,nullstr);
	_ggadget_redraw(g);
      break;
      case ec_copy:
	GTextFieldGrabSelection(gt,sn_clipboard);
return( true );
      case ec_paste:
	GTextFieldPaste(gt,sn_clipboard);
	GTextField_Show(gt,gt->sel_start);
	_ggadget_redraw(g);
      break;
      case ec_undo:
	if ( gt->oldtext!=NULL ) {
	    unichar_t *temp = gt->text;
	    int16 s;
	    gt->text = gt->oldtext; gt->oldtext = temp;
	    s = gt->sel_start; gt->sel_start = gt->sel_oldstart; gt->sel_oldstart = s;
	    s = gt->sel_end; gt->sel_end = gt->sel_oldend; gt->sel_oldend = s;
	    s = gt->sel_base; gt->sel_base = gt->sel_oldbase; gt->sel_oldbase = s;
	    GTextFieldRefigureLines(gt, 0);
	    GTextField_Show(gt,gt->sel_end);
	    _ggadget_redraw(g);
	}
      break;
      case ec_redo:		/* Hmm. not sure */ /* we don't do anything */
return( true );			/* but probably best to return success */
      case ec_backword:
        if ( gt->sel_start==gt->sel_end && gt->sel_start!=0 ) {
	    gt->sel_start = GTextFieldSelBackword(gt->text,gt->sel_start);
	}
	GTextField_Replace(gt,nullstr);
	_ggadget_redraw(g);
      break;
      case ec_deleteword:
        if ( gt->sel_start==gt->sel_end && gt->sel_start!=0 )
	    GTextFieldSelectWord(gt,gt->sel_start,&gt->sel_start,&gt->sel_end);
	GTextField_Replace(gt,nullstr);
	_ggadget_redraw(g);
      break;
      default:
return( false );
    }
    GTextFieldChanged(gt,-1);
return( true );
}

static int _gtextfield_editcmd(GGadget *g,enum editor_commands cmd) {
    if ( gtextfield_editcmd(g,cmd)) {
	_ggadget_redraw(g);
	GTPositionGIC((GTextField *) g);
return( true );
    }
return( false );
}

static int GTBackPos(GTextField *gt,int pos, int ismeta) {
    int newpos;

    if ( ismeta )
	newpos = GTextFieldSelBackword(gt->text,pos);
    else
	newpos = pos-1;
    if ( newpos==-1 ) newpos = pos;
return( newpos );
}

static int GTForePos(GTextField *gt,int pos, int ismeta) {
    int newpos=pos;

    if ( ismeta )
	newpos = GTextFieldSelForeword(gt->text,pos);
    else {
	if ( gt->text[pos]!=0 )
	    newpos = pos+1;
    }
return( newpos );
}

unichar_t *_GGadgetFileToUString(char *filename,int max) {
    FILE *file;
    int ch, ch2, ch3;
    int format=0;
    unichar_t *space, *upt, *end;

    file = fopen( filename,"r" );
    if ( file==NULL )
return( NULL );
    ch = getc(file); ch2 = getc(file); ch3 = getc(file);
    ungetc(ch3,file);
    if ( ch==0xfe && ch2==0xff )
	format = 1;		/* normal ucs2 */
    else if ( ch==0xff && ch2==0xfe )
	format = 2;		/* byte-swapped ucs2 */
    else if ( ch==0xef && ch2==0xbb && ch3==0xbf ) {
	format = 3;		/* utf8 */
	getc(file);
    } else {
	getc(file);		/* rewind probably undoes the ungetc, but let's not depend on it */
	rewind(file);
    }
    space = upt = malloc((max+1)*sizeof(unichar_t));
    end = space+max;
    if ( format==3 ) {		/* utf8 */
	while ( upt<end ) {
	    ch=getc(file);
	    if ( ch==EOF )
	break;
	    if ( ch<0x80 )
		*upt++ = ch;
	    else if ( ch<0xe0 ) {
		ch2 = getc(file);
		*upt++ = ((ch&0x1f)<<6)|(ch2&0x3f);
	    } else if ( ch<0xf0 ) {
		ch2 = getc(file); ch3 = getc(file);
		*upt++ = ((ch&0xf)<<12)|((ch2&0x3f)<<6)|(ch3&0x3f);
	    } else {
		int ch4, w;
		ch2 = getc(file); ch3 = getc(file); ch4=getc(file);
		w = ( ((ch&7)<<2) | ((ch2&0x30)>>4) ) -1;
		*upt++ = 0xd800 | (w<<6) | ((ch2&0xf)<<2) | ((ch3&0x30)>>4);
		if ( upt<end )
		    *upt++ = 0xdc00 | ((ch3&0xf)<<6) | (ch4&0x3f);
	    }
	}
    } else if ( format!=0 ) {
	while ( upt<end ) {
	    ch = getc(file); ch2 = getc(file);
	    if ( ch2==EOF )
	break;
	    if ( format==1 )
		*upt ++ = (ch<<8)|ch2;
	    else
		*upt ++ = (ch2<<8)|ch;
	}
    } else {
	char buffer[400];
	while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	    def2u_strncpy(upt,buffer,end-upt);
	    upt += u_strlen(upt);
	}
    }
    *upt = '\0';
    fclose(file);
return( space );
}

static unichar_t txt[] = { '*','.','t','x','t',  '\0' };
static unichar_t errort[] = { 'C','o','u','l','d',' ','n','o','t',' ','o','p','e','n',  '\0' };
static unichar_t error[] = { 'C','o','u','l','d',' ','n','o','t',' ','o','p','e','n',' ','%','.','1','0','0','h','s',  '\0' };

static void GTextFieldImport(GTextField *gt) {
    unichar_t *ret;
    char *cret;
    unichar_t *str;

    if ( _ggadget_use_gettext ) {
	char *temp = GWidgetOpenFile8(_("Open"),NULL,"*.txt",NULL,NULL);
	ret = utf82u_copy(temp);
	free(temp);
    } else {
	ret = GWidgetOpenFile(GStringGetResource(_STR_Open,NULL),NULL,
		txt,NULL,NULL);
    }

    if ( ret==NULL )
return;
    cret = u2def_copy(ret);
    free(ret);
    str = _GGadgetFileToUString(cret,65536);
    if ( str==NULL ) {
	if ( _ggadget_use_gettext )
	    GWidgetError8(_("Could not open file"), _("Could not open %.100s"),cret);
	else
	    GWidgetError(errort,error,cret);
	free(cret);
return;
    }
    free(cret);
    GTextField_Replace(gt,str);
    free(str);
}

static void GTextFieldSave(GTextField *gt,int utf8) {
    unichar_t *ret;
    char *cret;
    FILE *file;
    unichar_t *pt;

    if ( _ggadget_use_gettext ) {
	char *temp = GWidgetOpenFile8(_("Save"),NULL,"*.txt",NULL,NULL);
	ret = utf82u_copy(temp);
	free(temp);
    } else
	ret = GWidgetSaveAsFile(GStringGetResource(_STR_Save,NULL),NULL,
		txt,NULL,NULL);

    if ( ret==NULL )
return;
    cret = u2def_copy(ret);
    free(ret);
    file = fopen(cret,"w");
    if ( file==NULL ) {
	if ( _ggadget_use_gettext )
	    GWidgetError8(_("Could not open file"), _("Could not open %.100s"),cret);
	else
	    GWidgetError(errort,error,cret);
	free(cret);
return;
    }
    free(cret);

    if ( utf8 ) {
	putc(0xef,file);		/* Zero width something or other. Marks this as unicode, utf8 */
	putc(0xbb,file);
	putc(0xbf,file);
	for ( pt = gt->text ; *pt; ++pt ) {
	    if ( *pt<0x80 )
		putc(*pt,file);
	    else if ( *pt<0x800 ) {
		putc(0xc0 | (*pt>>6), file);
		putc(0x80 | (*pt&0x3f), file);
	    } else if ( *pt>=0xd800 && *pt<0xdc00 && pt[1]>=0xdc00 && pt[1]<0xe000 ) {
		int u = ((*pt>>6)&0xf)+1, y = ((*pt&3)<<4) | ((pt[1]>>6)&0xf);
		putc( 0xf0 | (u>>2),file );
		putc( 0x80 | ((u&3)<<4) | ((*pt>>2)&0xf),file );
		putc( 0x80 | y,file );
		putc( 0x80 | (pt[1]&0x3f),file );
	    } else {
		putc( 0xe0 | (*pt>>12),file );
		putc( 0x80 | ((*pt>>6)&0x3f),file );
		putc( 0x80 | (*pt&0x3f),file );
	    }
	}
    } else {
	putc(0xfeff>>8,file);		/* Zero width something or other. Marks this as unicode */
	putc(0xfeff&0xff,file);
	for ( pt = gt->text ; *pt; ++pt ) {
	    putc(*pt>>8,file);
	    putc(*pt&0xff,file);
	}
    }
    fclose(file);
}

#define MID_Cut		1
#define MID_Copy	2
#define MID_Paste	3

#define MID_SelectAll	4

#define MID_Save	5
#define MID_SaveUCS2	6
#define MID_Import	7

#define MID_Undo	8

static GTextField *popup_kludge;

static void GTFPopupInvoked(GWindow v, GMenuItem *mi,GEvent *e) {
    GTextField *gt;
    if ( popup_kludge==NULL )
return;
    gt = popup_kludge;
    popup_kludge = NULL;
    switch ( mi->mid ) {
      case MID_Undo:
	gtextfield_editcmd(&gt->g,ec_undo);
      break;
      case MID_Cut:
	gtextfield_editcmd(&gt->g,ec_cut);
      break;
      case MID_Copy:
	gtextfield_editcmd(&gt->g,ec_copy);
      break;
      case MID_Paste:
	gtextfield_editcmd(&gt->g,ec_paste);
      break;
      case MID_SelectAll:
	gtextfield_editcmd(&gt->g,ec_selectall);
      break;
      case MID_Save:
	GTextFieldSave(gt,true);
      break;
      case MID_SaveUCS2:
	GTextFieldSave(gt,false);
      break;
      case MID_Import:
	GTextFieldImport(gt);
      break;
    }
    _ggadget_redraw(&gt->g);
}

static GMenuItem gtf_popuplist[] = {
    { { (unichar_t *) "_Undo", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, 'Z', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Undo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) "Cu_t", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, 'X', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Cut },
    { { (unichar_t *) "_Copy", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, 'C', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Copy },
    { { (unichar_t *) "_Paste", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, 'V', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Paste },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) "_Save in UTF8", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, 'S', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Save },
    { { (unichar_t *) "Save in _UCS2", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, GTFPopupInvoked, MID_SaveUCS2 },
    { { (unichar_t *) "_Import", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, 'I', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Import },
    GMENUITEM_EMPTY
};
static int first = true;

static void GTFPopupMenu(GTextField *gt, GEvent *event) {
    int no_sel = gt->sel_start==gt->sel_end;

    if ( first ) {
	gtf_popuplist[0].ti.text = (unichar_t *) _("_Undo");
	gtf_popuplist[2].ti.text = (unichar_t *) _("Cu_t");
	gtf_popuplist[3].ti.text = (unichar_t *) _("_Copy");
	gtf_popuplist[4].ti.text = (unichar_t *) _("_Paste");
	gtf_popuplist[6].ti.text = (unichar_t *) _("_Save in UTF8");
	gtf_popuplist[7].ti.text = (unichar_t *) _("Save in _UCS2");
	gtf_popuplist[8].ti.text = (unichar_t *) _("_Import");
	first = false;
    }

    gtf_popuplist[0].ti.disabled = gt->oldtext==NULL;	/* Undo */
    gtf_popuplist[2].ti.disabled = no_sel;		/* Cut */
    gtf_popuplist[3].ti.disabled = no_sel;		/* Copy */
    gtf_popuplist[4].ti.disabled = !GDrawSelectionHasType(gt->g.base,sn_clipboard,"text/plain;charset=ISO-10646-UCS-2") &&
	    !GDrawSelectionHasType(gt->g.base,sn_clipboard,"UTF8_STRING") &&
	    !GDrawSelectionHasType(gt->g.base,sn_clipboard,"STRING");
    popup_kludge = gt;
    GMenuCreatePopupMenu(gt->g.base,event, gtf_popuplist);
}

static void GTextFieldIncrement(GTextField *gt,int amount) {
    unichar_t *end;
    double d = u_strtod(gt->text,&end);
    char buf[40];

    while ( *end==' ' ) ++end;
    if ( *end!='\0' ) {
	GDrawBeep(NULL);
return;
    }
    d = floor(d)+amount;
    sprintf(buf,"%g", d);
    free(gt->oldtext);
    gt->oldtext = gt->text;
    gt->text = uc_copy(buf);
    free(gt->utf8_text);
    gt->utf8_text = copy(buf);
    _ggadget_redraw(&gt->g);
    GTextFieldChanged(gt,-1);
}
    
static int GTextFieldDoChange(GTextField *gt, GEvent *event) {
    int ss = gt->sel_start, se = gt->sel_end;
    int pos, l, xpos, sel;
    unichar_t *upt;

    if ( ( event->u.chr.state&(GMenuMask()&~ksm_shift)) ||
	    event->u.chr.chars[0]<' ' || event->u.chr.chars[0]==0x7f ) {
	switch ( event->u.chr.keysym ) {
	  case GK_BackSpace:
	    if ( gt->sel_start==gt->sel_end ) {
		if ( gt->sel_start==0 )
return( 2 );
		--gt->sel_start;
	    }
	    GTextField_Replace(gt,nullstr);
return( true );
	  break;
	  case GK_Delete:
	    if ( gt->sel_start==gt->sel_end ) {
		if ( gt->text[gt->sel_start]==0 )
return( 2 );
		++gt->sel_end;
	    }
	    GTextField_Replace(gt,nullstr);
return( true );
	  break;
	  case GK_Left: case GK_KP_Left:
	    if ( gt->sel_start==gt->sel_end ) {
		gt->sel_start = GTBackPos(gt,gt->sel_start,event->u.chr.state&ksm_meta);
		if ( !(event->u.chr.state&ksm_shift ))
		    gt->sel_end = gt->sel_start;
	    } else if ( event->u.chr.state&ksm_shift ) {
		if ( gt->sel_end==gt->sel_base ) {
		    gt->sel_start = GTBackPos(gt,gt->sel_start,event->u.chr.state&ksm_meta);
		} else {
		    gt->sel_end = GTBackPos(gt,gt->sel_end,event->u.chr.state&ksm_meta);
		}
	    } else {
		gt->sel_end = gt->sel_base = gt->sel_start;
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_Right: case GK_KP_Right:
	    if ( gt->sel_start==gt->sel_end ) {
		gt->sel_end = GTForePos(gt,gt->sel_start,event->u.chr.state&ksm_meta);
		if ( !(event->u.chr.state&ksm_shift ))
		    gt->sel_start = gt->sel_end;
	    } else if ( event->u.chr.state&ksm_shift ) {
		if ( gt->sel_end==gt->sel_base ) {
		    gt->sel_start = GTForePos(gt,gt->sel_start,event->u.chr.state&ksm_meta);
		} else {
		    gt->sel_end = GTForePos(gt,gt->sel_end,event->u.chr.state&ksm_meta);
		}
	    } else {
		gt->sel_start = gt->sel_base = gt->sel_end;
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_Up: case GK_KP_Up:
	    if ( gt->numericfield ) {
		GTextFieldIncrement(gt,(event->u.chr.state&(ksm_shift|ksm_control))?10:1);
return( 2 );
	    }
	    if ( !gt->multi_line )
	  break;
	    if ( !( event->u.chr.state&ksm_shift ) && gt->sel_start!=gt->sel_end )
		gt->sel_end = gt->sel_base = gt->sel_start;
	    else {
		pos = gt->sel_start;
		if ( ( event->u.chr.state&ksm_shift ) && gt->sel_start==gt->sel_base )
		    pos = gt->sel_end;
		l = GTextFieldFindLine(gt,gt->sel_start);
		GRect pos_rect;
		int ll = gt->lines8[l+1]==-1 ? -1 : gt->lines8[l+1]-gt->lines8[l];
		sel = u2utf8_index(gt->sel_start-gt->lines[l],gt->utf8_text+gt->lines8[l]);
		GDrawLayoutInit(gt->g.base,gt->utf8_text+gt->lines8[l],ll,NULL);
		GDrawLayoutIndexToPos(gt->g.base,sel,&pos_rect);
		xpos = pos_rect.x;
		if ( l!=0 ) {
		    GDrawLayoutInit(gt->g.base,gt->utf8_text+gt->lines8[l-1],gt->lines8[l]-gt->lines8[l-1],NULL);
		    pos = GDrawLayoutXYToIndex(gt->g.base,xpos,0);
		    pos = utf82u_index(pos,gt->utf8_text+gt->lines8[l-1]);
		}
		if ( event->u.chr.state&ksm_shift ) {
		    if ( pos<gt->sel_base ) {
			gt->sel_start = pos;
			gt->sel_end = gt->sel_base;
		    } else {
			gt->sel_start = gt->sel_base;
			gt->sel_end = pos;
		    }
		} else {
		    gt->sel_start = gt->sel_end = gt->sel_base = pos;
		}
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_Down: case GK_KP_Down:
	    if ( gt->numericfield ) {
		GTextFieldIncrement(gt,(event->u.chr.state&(ksm_shift|ksm_control))?-10:-1);
return( 2 );
	    }
	    if ( !gt->multi_line )
	  break;
	    if ( !( event->u.chr.state&ksm_shift ) && gt->sel_start!=gt->sel_end )
		gt->sel_end = gt->sel_base = gt->sel_end;
	    else {
		pos = gt->sel_start;
		if ( ( event->u.chr.state&ksm_shift ) && gt->sel_start==gt->sel_base )
		    pos = gt->sel_end;
		l = GTextFieldFindLine(gt,gt->sel_start);
		GRect pos_rect;
		int ll = gt->lines8[l+1]==-1 ? -1 : gt->lines8[l+1]-gt->lines8[l];
		sel = u2utf8_index(gt->sel_start-gt->lines[l],gt->utf8_text+gt->lines8[l]);
		GDrawLayoutInit(gt->g.base,gt->utf8_text+gt->lines8[l],ll,NULL);
		GDrawLayoutIndexToPos(gt->g.base,sel,&pos_rect);
		xpos = pos_rect.x;
		if ( l<gt->lcnt-1 ) {
		    ll = gt->lines8[l+2]==-1 ? -1 : gt->lines8[l+2]-gt->lines8[l+1];
		    GDrawLayoutInit(gt->g.base,gt->utf8_text+gt->lines8[l+1],ll,NULL);
		    pos = GDrawLayoutXYToIndex(gt->g.base,xpos,0);
		    pos = utf82u_index(pos,gt->utf8_text+gt->lines8[l+1]);
		}
		if ( event->u.chr.state&ksm_shift ) {
		    if ( pos<gt->sel_base ) {
			gt->sel_start = pos;
			gt->sel_end = gt->sel_base;
		    } else {
			gt->sel_start = gt->sel_base;
			gt->sel_end = pos;
		    }
		} else {
		    gt->sel_start = gt->sel_end = gt->sel_base = pos;
		}
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_Home: case GK_Begin: case GK_KP_Home: case GK_KP_Begin:
	    if ( !(event->u.chr.state&ksm_shift) ) {
		gt->sel_start = gt->sel_base = gt->sel_end = 0;
	    } else {
		gt->sel_start = 0; gt->sel_end = gt->sel_base;
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  /* Move to eol. (if already at eol, move to next eol) */
	  case 'E': case 'e':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    upt = gt->text+gt->sel_base;
	    if ( *upt=='\n' )
		++upt;
	    upt = u_strchr(upt,'\n');
	    if ( upt==NULL ) upt=gt->text+u_strlen(gt->text);
	    if ( !(event->u.chr.state&ksm_shift) ) {
		gt->sel_start = gt->sel_base = gt->sel_end =upt-gt->text;
	    } else {
		gt->sel_start = gt->sel_base; gt->sel_end = upt-gt->text;
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_End: case GK_KP_End:
	    if ( !(event->u.chr.state&ksm_shift) ) {
		gt->sel_start = gt->sel_base = gt->sel_end = u_strlen(gt->text);
	    } else {
		gt->sel_start = gt->sel_base; gt->sel_end = u_strlen(gt->text);
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case 'D': case 'd':
	    if ( event->u.chr.state&ksm_control ) {	/* delete word */
		gtextfield_editcmd(&gt->g,ec_deleteword);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    }
	  break;
	  case 'W': case 'w':
	    if ( event->u.chr.state&ksm_control ) {	/* backword */
		gtextfield_editcmd(&gt->g,ec_backword);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    }
	  break;
	  case 'M': case 'm': case 'J': case 'j':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    /* fall through into return case */
	  case GK_Return: case GK_Linefeed:
	    if ( gt->accepts_returns ) {
		GTextField_Replace(gt,newlinestr);
return( true );
	    }
	  break;
	  case GK_Tab:
	    if ( gt->completionfield && ((GCompletionField *) gt)->completion!=NULL ) {
		GTextFieldComplete(gt,true);
		gt->was_completing = true;
return( 3 );
	    }
	    if ( gt->accepts_tabs ) {
		GTextField_Replace(gt,tabstr);
return( true );
	    }
	  break;
	  default:
	    if ( GMenuIsCommand(event,H_("Select All|Ctl+A")) ) {
		gtextfield_editcmd(&gt->g,ec_selectall);
return( 2 );
	    } else if ( GMenuIsCommand(event,H_("Copy|Ctl+C")) ) {
		gtextfield_editcmd(&gt->g,ec_copy);
	    } else if ( GMenuIsCommand(event,H_("Paste|Ctl+V")) ) {
		gtextfield_editcmd(&gt->g,ec_paste);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    } else if ( GMenuIsCommand(event,H_("Cut|Ctl+X")) ) {
		gtextfield_editcmd(&gt->g,ec_cut);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    } else if ( GMenuIsCommand(event,H_("Undo|Ctl+Z")) ) {
		gtextfield_editcmd(&gt->g,ec_undo);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    } else if ( GMenuIsCommand(event,H_("Save|Ctl+S")) ) {
		GTextFieldSave(gt,true);
return( 2 );
	    } else if ( GMenuIsCommand(event,H_("Import...|Ctl+Shft+I")) ) {
		GTextFieldImport(gt);
return( true );
	    } else
return( false );
	  break;
	}
    } else {
	GTextField_Replace(gt,event->u.chr.chars);
return( 4 /* Do name completion */ );
    }

    if ( gt->sel_start == gt->sel_end )
	gt->sel_base = gt->sel_start;
    if ( ss!=gt->sel_start || se!=gt->sel_end )
	GTextFieldGrabPrimarySelection(gt);
return( false );
}

static void _gt_cursor_pos(GTextField *gt, int sel_start, int *x, int *y) {
    int l, sel;

    *x = -1; *y= -1;
    GDrawSetFont(gt->g.base,gt->font);
    l = GTextFieldFindLine(gt,sel_start);
    if ( l<gt->loff_top || l>=gt->loff_top + ((gt->g.inner.height+gt->fh/2)/gt->fh))
return;
    *y = (l-gt->loff_top)*gt->fh;
    GRect pos_rect;
    int ll = gt->lines8[l+1]==-1 ? -1 : gt->lines8[l+1]-gt->lines8[l];
    sel = u2utf8_index(sel_start-gt->lines[l],gt->utf8_text+gt->lines8[l]);
    GDrawLayoutInit(gt->g.base,gt->utf8_text+gt->lines8[l],ll,NULL);
    GDrawLayoutIndexToPos(gt->g.base,sel,&pos_rect);
    *x = pos_rect.x - gt->xoff_left;
}

static void gt_cursor_pos(GTextField *gt, int *x, int *y) {
    _gt_cursor_pos(gt,gt->sel_start,x,y);
}

static void GTPositionGIC(GTextField *gt) {
    int x,y;

    if ( !gt->g.has_focus || gt->gic==NULL )
return;
    gt_cursor_pos(gt,&x,&y);
    if ( x<0 )
return;
    GDrawSetGIC(gt->g.base,gt->gic,gt->g.inner.x+x,gt->g.inner.y+y+gt->as);
}

static void gt_draw_cursor(GWindow pixmap, GTextField *gt) {
    GRect old;
    int x, y;

    if ( !gt->cursor_on || gt->sel_start != gt->sel_end )
return;
    gt_cursor_pos(gt,&x,&y);

    if ( x<0 || x>=gt->g.inner.width )
return;
    GDrawPushClip(pixmap,&gt->g.inner,&old);
    GDrawSetXORMode(pixmap);
    GDrawSetXORBase(pixmap,gt->g.box->main_background!=COLOR_DEFAULT?gt->g.box->main_background:
	    GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap)) );
    GDrawSetFont(pixmap,gt->font);
    GDrawSetLineWidth(pixmap,0);
    GDrawDrawLine(pixmap,gt->g.inner.x+x,gt->g.inner.y+y,
	    gt->g.inner.x+x,gt->g.inner.y+y+gt->fh,
	    gt->g.box->main_foreground!=COLOR_DEFAULT?gt->g.box->main_foreground:
	    GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)) );
    GDrawSetCopyMode(pixmap);
    GDrawPopClip(pixmap,&old);
}

static void GTextFieldDrawDDCursor(GTextField *gt, int pos) {
    GRect old;
    int x, y, l;

    l = GTextFieldFindLine(gt,pos);
    if ( l<gt->loff_top || l>=gt->loff_top + (gt->g.inner.height/gt->fh))
return;
    _gt_cursor_pos(gt,pos,&x,&y);
    if ( x<0 || x>=gt->g.inner.width )
return;

    GDrawPushClip(gt->g.base,&gt->g.inner,&old);
    GDrawSetXORMode(gt->g.base);
    GDrawSetXORBase(gt->g.base,gt->g.box->main_background!=COLOR_DEFAULT?gt->g.box->main_background:
	    GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(gt->g.base)) );
    GDrawSetFont(gt->g.base,gt->font);
    GDrawSetLineWidth(gt->g.base,0);
    GDrawSetDashedLine(gt->g.base,2,2,0);
    GDrawDrawLine(gt->g.base,gt->g.inner.x+x,gt->g.inner.y+y,
	    gt->g.inner.x+x,gt->g.inner.y+y+gt->fh,
	    gt->g.box->main_foreground!=COLOR_DEFAULT?gt->g.box->main_foreground:
	    GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(gt->g.base)) );
    GDrawSetCopyMode(gt->g.base);
    GDrawPopClip(gt->g.base,&old);
    GDrawSetDashedLine(gt->g.base,0,0,0);
    gt->has_dd_cursor = !gt->has_dd_cursor;
    gt->dd_cursor_pos = pos;
}

static void GTextFieldDrawLineSel(GWindow pixmap, GTextField *gt, int line ) {
    GRect selr, sofar, nextch;
    int s,e, y,llen,i,j;

    /* Caller has checked to make sure selection applies to this line */

    y = gt->g.inner.y+(line-gt->loff_top)*gt->fh;
    selr = gt->g.inner; selr.y = y; selr.height = gt->fh;
    if ( !gt->g.has_focus ) --selr.height;
    llen = gt->lines[line+1]==-1?
	    u_strlen(gt->text+gt->lines[line])+gt->lines[line]:
	    gt->lines[line+1];
    s = gt->sel_start<gt->lines[line]?gt->lines[line]:gt->sel_start;
    e = gt->sel_end>gt->lines[line+1] && gt->lines[line+1]!=-1?gt->lines[line+1]-1:
	    gt->sel_end;

    s = u2utf8_index(s-gt->lines[line],gt->utf8_text+gt->lines8[line]);
    e = u2utf8_index(e-gt->lines[line],gt->utf8_text+gt->lines8[line]);
    llen = gt->lines8[line+1]==-1? -1 : gt->lines8[line+1]-gt->lines8[line];
    GDrawLayoutInit(pixmap,gt->utf8_text+gt->lines8[line],llen,NULL);
    for ( i=s; i<e; ) {
        GDrawLayoutIndexToPos(pixmap,i,&sofar);
        for ( j=i+1; j<e; ++j ) {
	    GDrawLayoutIndexToPos(pixmap,j,&nextch);
	    if ( nextch.x != sofar.x+sofar.width )
        break;
	    sofar.width += nextch.width;
        }
        if ( sofar.width<0 ) {
	    selr.x = sofar.x+sofar.width + gt->g.inner.x - gt->xoff_left;
	    selr.width = -sofar.width;
        } else {
	    selr.x = sofar.x + gt->g.inner.x - gt->xoff_left;
	    selr.width = sofar.width;
        }
        GDrawFillRect(pixmap,&selr,gt->g.box->active_border);
        i = j;
    }
}

static void GTextFieldDrawLine(GWindow pixmap, GTextField *gt, int line, Color fg ) {
    int y = gt->g.inner.y+(line-gt->loff_top)*gt->fh;
    int ll = gt->lines[line+1]==-1 ? -1 : gt->lines[line+1]-gt->lines[line];

    ll = gt->lines8[line+1]==-1? -1 : gt->lines8[line+1]-gt->lines8[line];
    GDrawLayoutInit(pixmap,gt->utf8_text+gt->lines8[line],ll,NULL);
    GDrawLayoutDraw(pixmap,gt->g.inner.x-gt->xoff_left,y+gt->as,fg);
}

static int gtextfield_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;
    GListField *ge = (GListField *) g;
    GRect old1, old2, *r = &g->r;
    Color fg;
    int ll,i, last;
    GRect unpadded_inner;
    int pad;

    if ( g->state == gs_invisible || gt->dontdraw )
return( false );

    if ( gt->listfield || gt->numericfield ) r = &ge->fieldrect;

    GDrawPushClip(pixmap,r,&old1);

    GBoxDrawBackground(pixmap,r,g->box,
	    g->state==gs_enabled? gs_pressedactive: g->state,false);
    GBoxDrawBorder(pixmap,r,g->box,g->state,false);

    unpadded_inner = g->inner;
    pad = GDrawPointsToPixels(g->base,g->box->padding);
    unpadded_inner.x -= pad; unpadded_inner.y -= pad;
    unpadded_inner.width += 2*pad; unpadded_inner.height += 2*pad;
    GDrawPushClip(pixmap,&unpadded_inner,&old2);
    GDrawSetFont(pixmap,gt->font);

    fg = g->state==gs_disabled?g->box->disabled_foreground:
		    g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
		    g->box->main_foreground;
    ll = 0;
    if ( (last = gt->g.inner.height/gt->fh)==0 ) last = 1;
    if ( gt->sel_start != gt->sel_end ) {
	/* I used to have support for drawing on a bw display where the */
	/*  selection and the foreground color were the same (black) and */
	/*  selected text was white. No longer. */
	/* Draw the entire selection first, then the text itself */
	for ( i=gt->loff_top; i<gt->loff_top+last && gt->lines[i]!=-1; ++i ) {
	    if ( gt->sel_end>gt->lines[i] &&
		    (gt->lines[i+1]==-1 || gt->sel_start<gt->lines[i+1]))
		GTextFieldDrawLineSel(pixmap,gt,i);
	}
    }
    for ( i=gt->loff_top; i<gt->loff_top+last && gt->lines[i]!=-1; ++i )
	GTextFieldDrawLine(pixmap,gt,i,fg);

    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
    gt_draw_cursor(pixmap, gt);

    if ( gt->listfield ) {
	int marklen = GDrawPointsToPixels(pixmap,_GListMarkSize);

	GDrawPushClip(pixmap,&ge->buttonrect,&old1);

	GBoxDrawBackground(pixmap,&ge->buttonrect,&glistfieldmenu_box,
		g->state==gs_enabled? gs_pressedactive: g->state,false);
	GBoxDrawBorder(pixmap,&ge->buttonrect,&glistfieldmenu_box,g->state,false);

	GListMarkDraw(pixmap,
		ge->buttonrect.x + (ge->buttonrect.width - marklen)/2,
		g->inner.y,
		g->inner.height,
		g->state);
	GDrawPopClip(pixmap,&old1);
    } else if ( gt->numericfield ) {
	int y, w;
	int half;
	GPoint pts[5];
	int bp = GBoxBorderWidth(gt->g.base,&gnumericfieldspinner_box);
	Color fg = g->state==gs_disabled?gnumericfieldspinner_box.disabled_foreground:
			gnumericfieldspinner_box.main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
			gnumericfieldspinner_box.main_foreground;

	GBoxDrawBackground(pixmap,&ge->buttonrect,&gnumericfieldspinner_box,
		g->state==gs_enabled? gs_pressedactive: g->state,false);
	GBoxDrawBorder(pixmap,&ge->buttonrect,&gnumericfieldspinner_box,g->state,false);
	/* GDrawDrawRect(pixmap,&ge->buttonrect,fg); */

	y = ge->buttonrect.y + ge->buttonrect.height/2;
	w = ge->buttonrect.width;
	w &= ~1;
	pts[0].x = ge->buttonrect.x+3+bp;
	pts[1].x = ge->buttonrect.x+w-3-bp;
	pts[2].x = ge->buttonrect.x + w/2;
	half = pts[2].x-pts[0].x;
	GDrawDrawLine(pixmap, pts[0].x-3,y, pts[1].x+3,y, fg );
	pts[0].y = pts[1].y = y-2;
	pts[2].y = pts[1].y-half;
	pts[3] = pts[0];
	GDrawFillPoly(pixmap,pts,3,fg);
	pts[0].y = pts[1].y = y+2;
	pts[2].y = pts[1].y+half;
	pts[3] = pts[0];
	GDrawFillPoly(pixmap,pts,3,fg);
    }
return( true );
}

static int glistfield_mouse(GListField *ge, GEvent *event) {
    if ( event->type!=et_mousedown )
return( true );
    if ( ge->popup != NULL ) {
	GDrawDestroyWindow(ge->popup);
	ge->popup = NULL;
return( true );
    }
    ge->popup = GListPopupCreate(&ge->gt.g,GListFieldSelected,ge->ti);
return( true );
}

static int gnumericfield_mouse(GTextField *gt, GEvent *event) {
    GListField *ge = (GListField *) gt;
    if ( event->type==et_mousedown ) {
	gt->incr_down = event->u.mouse.y > (ge->buttonrect.y + ge->buttonrect.height/2);
	GTextFieldIncrement(gt,gt->incr_down?-1:1);
	if ( gt->numeric_scroll==NULL )
	    gt->numeric_scroll = GDrawRequestTimer(gt->g.base,200,100,NULL);
    } else if ( gt->numeric_scroll!=NULL ) {
	GDrawCancelTimer(gt->numeric_scroll);
	gt->numeric_scroll = NULL;
    }
return( true );
}

static int GTextFieldDoDrop(GTextField *gt,GEvent *event,int endpos) {

    if ( gt->has_dd_cursor )
	GTextFieldDrawDDCursor(gt,gt->dd_cursor_pos);

    if ( event->type == et_mousemove ) {
	if ( GGadgetInnerWithin(&gt->g,event->u.mouse.x,event->u.mouse.y) ) {
	    if ( endpos<gt->sel_start || endpos>=gt->sel_end )
		GTextFieldDrawDDCursor(gt,endpos);
	} else if ( !GGadgetWithin(&gt->g,event->u.mouse.x,event->u.mouse.y) ) {
	    GDrawPostDragEvent(gt->g.base,event,et_drag);
	}
    } else {
	if ( GGadgetInnerWithin(&gt->g,event->u.mouse.x,event->u.mouse.y) ) {
	    if ( endpos>=gt->sel_start && endpos<gt->sel_end ) {
		gt->sel_start = gt->sel_end = endpos;
	    } else {
		unichar_t *old=gt->oldtext, *temp;
		int pos=0;
		if ( event->u.mouse.state&ksm_control ) {
		    temp = malloc((u_strlen(gt->text)+gt->sel_end-gt->sel_start+1)*sizeof(unichar_t));
		    memcpy(temp,gt->text,endpos*sizeof(unichar_t));
		    memcpy(temp+endpos,gt->text+gt->sel_start,
			    (gt->sel_end-gt->sel_start)*sizeof(unichar_t));
		    u_strcpy(temp+endpos+gt->sel_end-gt->sel_start,gt->text+endpos);
		} else if ( endpos>=gt->sel_end ) {
		    temp = u_copy(gt->text);
		    memcpy(temp+gt->sel_start,temp+gt->sel_end,
			    (endpos-gt->sel_end)*sizeof(unichar_t));
		    memcpy(temp+endpos-(gt->sel_end-gt->sel_start),
			    gt->text+gt->sel_start,(gt->sel_end-gt->sel_start)*sizeof(unichar_t));
		    pos = endpos;
		} else /*if ( endpos<gt->sel_start )*/ {
		    temp = u_copy(gt->text);
		    memcpy(temp+endpos,gt->text+gt->sel_start,
			    (gt->sel_end-gt->sel_start)*sizeof(unichar_t));
		    memcpy(temp+endpos+gt->sel_end-gt->sel_start,gt->text+endpos,
			    (gt->sel_start-endpos)*sizeof(unichar_t));
		    pos = endpos+gt->sel_end-gt->sel_start;
		}
		gt->oldtext = gt->text;
		gt->sel_oldstart = gt->sel_start;
		gt->sel_oldend = gt->sel_end;
		gt->sel_oldbase = gt->sel_base;
		gt->sel_start = gt->sel_end = pos;
		gt->text = temp;
		free(old);
		GTextFieldRefigureLines(gt, endpos<gt->sel_oldstart?endpos:gt->sel_oldstart);
	    }
	} else if ( !GGadgetWithin(&gt->g,event->u.mouse.x,event->u.mouse.y) ) {
	    /* Don't delete the selection until someone actually accepts the drop */
	    /* Don't delete at all (copy not move) if control key is down */
	    if ( ( event->u.mouse.state&ksm_control ) )
		GTextFieldGrabSelection(gt,sn_drag_and_drop);
	    else
		GTextFieldGrabDDSelection(gt);
	    GDrawPostDragEvent(gt->g.base,event,et_drop);
	}
	gt->drag_and_drop = false;
	GDrawSetCursor(gt->g.base,gt->old_cursor);
	_ggadget_redraw(&gt->g);
    }
return( false );
}
    
static int gtextfield_mouse(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;
    GListField *ge = (GListField *) g;
    unichar_t *end=NULL, *end1, *end2;
    int i=0,ll;

    if ( gt->hidden_cursor ) {
	GDrawSetCursor(gt->g.base,gt->old_cursor);
	gt->hidden_cursor = false;
	_GWidget_ClearGrabGadget(g);
    }
    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return( false );
    if ( event->type == et_crossing )
return( false );
    if ( gt->completionfield && ((GCompletionField *) gt)->choice_popup!=NULL &&
	    event->type==et_mousedown )
	GCompletionDestroy((GCompletionField *) gt);
    if (( gt->listfield && event->u.mouse.x>=ge->buttonrect.x &&
	    event->u.mouse.x<ge->buttonrect.x+ge->buttonrect.width &&
	    event->u.mouse.y>=ge->buttonrect.y &&
	    event->u.mouse.y<ge->buttonrect.y+ge->buttonrect.height ) ||
	( gt->listfield && ge->popup!=NULL ))
return( glistfield_mouse(ge,event));
    if ( gt->numericfield && event->u.mouse.x>=ge->buttonrect.x &&
	    event->u.mouse.x<ge->buttonrect.x+ge->buttonrect.width &&
	    event->u.mouse.y>=ge->buttonrect.y &&
	    event->u.mouse.y<ge->buttonrect.y+ge->buttonrect.height )
return( gnumericfield_mouse(gt,event));
    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7)) {
	int isv = event->u.mouse.button<=5;
	if ( event->u.mouse.state&ksm_shift ) isv = !isv;
	if ( isv && gt->vsb!=NULL )
return( GGadgetDispatchEvent(&gt->vsb->g,event));
	else if ( !isv && gt->hsb!=NULL )
return( GGadgetDispatchEvent(&gt->hsb->g,event));
	else
return( true );
    }

    if ( gt->pressed==NULL && event->type == et_mousemove && g->popup_msg!=NULL &&
	    GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y))
	GGadgetPreparePopup(g->base,g->popup_msg);

    if ( event->type == et_mousedown || gt->pressed ) {
	i = (event->u.mouse.y-g->inner.y)/gt->fh + gt->loff_top;
	if ( i<0 ) i = 0;
	if ( !gt->multi_line ) i = 0;
	if ( i>=gt->lcnt )
	    end = gt->text+u_strlen(gt->text);
	else
	    end = GTextFieldGetPtFromPos(gt,i,event->u.mouse.x);
    }

    if ( event->type == et_mousedown ) {
	if ( event->u.mouse.button==3 &&
		GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	    GTFPopupMenu(gt,event);
return( true );
	}

	if ( i>=gt->lcnt )
	    end1 = end2 = end;
	else {
	    int end8;
	    ll = gt->lines8[i+1]==-1?-1:gt->lines8[i+1]-gt->lines8[i]-1;
	    GDrawLayoutInit(gt->g.base,gt->utf8_text+gt->lines8[i],ll,NULL);
	    end8 = GDrawLayoutXYToIndex(gt->g.base,event->u.mouse.x-g->inner.x+gt->xoff_left,0);
	    end1 = end2 = gt->text + gt->lines[i] + utf82u_index(end8,gt->utf8_text+gt->lines8[i]);
	}

	gt->wordsel = gt->linesel = false;
	if ( event->u.mouse.button==1 && event->u.mouse.clicks>=3 ) {
	    gt->sel_start = gt->lines[i]; gt->sel_end = gt->lines[i+1];
	    if ( gt->sel_end==-1 ) gt->sel_end = u_strlen(gt->text);
	    gt->wordsel = false; gt->linesel = true;
	} else if ( event->u.mouse.button==1 && event->u.mouse.clicks==2 ) {
	    gt->sel_start = gt->sel_end = gt->sel_base = end-gt->text;
	    gt->wordsel = true;
	    GTextFieldSelectWords(gt,gt->sel_base);
	} else if ( end1-gt->text>=gt->sel_start && end2-gt->text<gt->sel_end &&
		gt->sel_start!=gt->sel_end &&
		event->u.mouse.button==1 ) {
	    gt->drag_and_drop = true;
	    if ( !gt->hidden_cursor )
		gt->old_cursor = GDrawGetCursor(gt->g.base);
	    GDrawSetCursor(gt->g.base,ct_draganddrop);
	} else if ( /*event->u.mouse.button!=3 &&*/ !(event->u.mouse.state&ksm_shift) ) {
	    if ( event->u.mouse.button==1 )
		GTextFieldGrabPrimarySelection(gt);
	    gt->sel_start = gt->sel_end = gt->sel_base = end-gt->text;
	} else if ( end-gt->text>gt->sel_base ) {
	    gt->sel_start = gt->sel_base;
	    gt->sel_end = end-gt->text;
	} else {
	    gt->sel_start = end-gt->text;
	    gt->sel_end = gt->sel_base;
	}

	if ( gt->pressed==NULL )
	    gt->pressed = GDrawRequestTimer(gt->g.base,200,100,NULL);
	if ( gt->sel_start > u_strlen( gt->text ))	/* Ok to have selection at end, but beyond is an error */
	    fprintf( stderr, "About to crash\n" );
	_ggadget_redraw(g);
return( true );
    } else if ( gt->pressed && (event->type == et_mousemove || event->type == et_mouseup )) {
	int refresh = true;

	if ( gt->drag_and_drop ) {
	    refresh = GTextFieldDoDrop(gt,event,end-gt->text);
	} else if ( gt->linesel ) {
	    int j, e;
	    gt->sel_start = gt->lines[i]; gt->sel_end = gt->lines[i+1];
	    if ( gt->sel_end==-1 ) gt->sel_end = u_strlen(gt->text);
	    for ( j=0; gt->lines[i+1]!=-1 && gt->sel_base>=gt->lines[i+1]; ++j );
	    if ( gt->sel_start<gt->lines[i] ) gt->sel_start = gt->lines[i];
	    e = gt->lines[j+1]==-1 ? u_strlen(gt->text): gt->lines[j+1];
	    if ( e>gt->sel_end ) gt->sel_end = e;
	} else if ( gt->wordsel )
	    GTextFieldSelectWords(gt,end-gt->text);
	else if ( event->u.mouse.button!=2 ) {
	    int e = end-gt->text;
	    if ( e>gt->sel_base ) {
		gt->sel_start = gt->sel_base; gt->sel_end = e;
	    } else {
		gt->sel_start = e; gt->sel_end = gt->sel_base;
	    }
	}
	if ( event->type==et_mouseup ) {
	    GDrawCancelTimer(gt->pressed); gt->pressed = NULL;
	    if ( event->u.mouse.button==2 )
		GTextFieldPaste(gt,sn_primary);
	    if ( gt->sel_start==gt->sel_end )
		GTextField_Show(gt,gt->sel_start);
	    GTextFieldChanged(gt,-1);
	    if ( gt->sel_start<gt->sel_end && _GDraw_InsCharHook!=NULL && !gt->donthook )
		(_GDraw_InsCharHook)(GDrawGetDisplayOfWindow(gt->g.base),
			gt->text[gt->sel_start]);
	}
	if ( gt->sel_end > u_strlen( gt->text ))
	    fprintf( stderr, "About to crash\n" );
	if ( refresh )
	    _ggadget_redraw(g);
return( true );
    }
return( false );
}

static int gtextfield_key(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return( false );
    if ( gt->listfield && ((GListField *) gt)->popup!=NULL ) {
	GWindow popup = ((GListField *) gt)->popup;
	(GDrawGetEH(popup))(popup,event);
return( true );
    }

    if ( gt->completionfield && ((GCompletionField *) gt)->choice_popup!=NULL &&
	    GCompletionHandleKey(gt,event))
return( true );

    if ( event->type == et_charup )
return( false );
    if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ||
	    (event->u.chr.keysym == GK_Return && !gt->accepts_returns ) ||
	    ( event->u.chr.keysym == GK_Tab && !gt->accepts_tabs ) ||
	    event->u.chr.keysym == GK_BackTab || event->u.chr.keysym == GK_Escape )
return( false );

    if ( !gt->hidden_cursor ) {	/* hide the mouse pointer */
	if ( !gt->drag_and_drop )
	    gt->old_cursor = GDrawGetCursor(gt->g.base);
	GDrawSetCursor(g->base,ct_invisible);
	gt->hidden_cursor = true;
	_GWidget_SetGrabGadget(g);	/* so that we get the next mouse movement to turn the cursor on */
    }
    if( gt->cursor_on ) {	/* undraw the blinky text cursor if it is drawn */
	gt_draw_cursor(g->base, gt);
	gt->cursor_on = false;
    }

    switch ( GTextFieldDoChange(gt,event)) {
      case 4:
	/* We should try name completion */
	if ( gt->completionfield && ((GCompletionField *) gt)->completion!=NULL &&
		gt->was_completing && gt->sel_start == u_strlen(gt->text))
	    GTextFieldComplete(gt,false);
	else
	    GTextFieldChanged(gt,-1);
      break;
      case 3:
	/* They typed a Tab */
      break;
      case 2:
      break;
      case true:
	GTextFieldChanged(gt,-1);
      break;
      case false:
return( false );
    }
    _ggadget_redraw(g);
return( true );
}

static int gtextfield_focus(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;

    if ( g->state == gs_invisible || g->state == gs_disabled )
return( false );

    if ( gt->cursor!=NULL ) {
	GDrawCancelTimer(gt->cursor);
	gt->cursor = NULL;
	gt->cursor_on = false;
    }
    if ( gt->hidden_cursor && !event->u.focus.gained_focus ) {
	GDrawSetCursor(gt->g.base,gt->old_cursor);
	gt->hidden_cursor = false;
    }
    gt->g.has_focus = event->u.focus.gained_focus;
    if ( event->u.focus.gained_focus ) {
	gt->cursor = GDrawRequestTimer(gt->g.base,400,400,NULL);
	gt->cursor_on = true;
	if ( event->u.focus.mnemonic_focus != mf_normal )
	    GTextFieldSelect(&gt->g,0,-1);
	if ( gt->gic!=NULL )
	    GTPositionGIC(gt);
	else if ( GWidgetGetInputContext(gt->g.base)!=NULL )
	    GDrawSetGIC(gt->g.base,GWidgetGetInputContext(gt->g.base),10000,10000);
    }
    _ggadget_redraw(g);
    GTextFieldFocusChanged(gt,event->u.focus.gained_focus);
return( true );
}

static int gtextfield_timer(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return(false);
    if ( gt->cursor == event->u.timer.timer ) {
	if ( gt->cursor_on ) {
	    gt_draw_cursor(g->base, gt);
	    gt->cursor_on = false;
	} else {
	    gt->cursor_on = true;
	    gt_draw_cursor(g->base, gt);
	}
return( true );
    }
    if ( gt->numeric_scroll == event->u.timer.timer ) {
	GTextFieldIncrement(gt,gt->incr_down?-1:1);
return( true );
    }
    if ( gt->pressed == event->u.timer.timer ) {
	GEvent e;
	GDrawSetFont(g->base,gt->font);
	GDrawGetPointerPosition(g->base,&e);
	if ( (e.u.mouse.x<g->r.x && gt->xoff_left>0 ) ||
		(gt->multi_line && e.u.mouse.y<g->r.y && gt->loff_top>0 ) ||
		( e.u.mouse.x >= g->r.x + g->r.width &&
			gt->xmax-gt->xoff_left>g->inner.width ) ||
		( e.u.mouse.y >= g->r.y + g->r.height &&
			gt->lcnt-gt->loff_top > g->inner.height/gt->fh )) {
	    int l = gt->loff_top + (e.u.mouse.y-g->inner.y)/gt->fh;
	    int xpos; unichar_t *end;

	    if ( e.u.mouse.y<g->r.y && gt->loff_top>0 )
		l = --gt->loff_top;
	    else if ( e.u.mouse.y >= g->r.y + g->r.height &&
			    gt->lcnt-gt->loff_top > g->inner.height/gt->fh ) {
		++gt->loff_top;
		l = gt->loff_top + g->inner.width/gt->fh;
	    } else if ( l<gt->loff_top )
		l = gt->loff_top; 
	    else if ( l>=gt->loff_top + g->inner.height/gt->fh ) 
		l = gt->loff_top + g->inner.height/gt->fh-1;
	    if ( l>=gt->lcnt ) l = gt->lcnt-1;

	    xpos = e.u.mouse.x+gt->xoff_left;
	    if ( e.u.mouse.x<g->r.x && gt->xoff_left>0 ) {
		gt->xoff_left -= gt->nw;
		xpos = g->inner.x + gt->xoff_left;
	    } else if ( e.u.mouse.x >= g->r.x + g->r.width &&
			    gt->xmax-gt->xoff_left>g->inner.width ) {
		gt->xoff_left += gt->nw;
		xpos = g->inner.x + gt->xoff_left + g->inner.width;
	    }

	    end = GTextFieldGetPtFromPos(gt,l,xpos);
	    if ( end-gt->text > gt->sel_base ) {
		gt->sel_start = gt->sel_base;
		gt->sel_end = end-gt->text;
	    } else {
		gt->sel_start = end-gt->text;
		gt->sel_end = gt->sel_base;
	    }
	    _ggadget_redraw(g);
	    if ( gt->vsb!=NULL )
		GScrollBarSetPos(&gt->vsb->g,gt->loff_top);
	    if ( gt->hsb!=NULL )
		GScrollBarSetPos(&gt->hsb->g,gt->xoff_left);
	}
return( true );
    }
return( false );
}

static int gtextfield_sel(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;
    unichar_t *end;
    int i;

    if ( event->type == et_selclear ) {
	if ( event->u.selclear.sel==sn_primary && gt->sel_start!=gt->sel_end ) {
	    gt->sel_start = gt->sel_end = gt->sel_base;
	    _ggadget_redraw(g);
            return( true );
	}
        return( false );
    }

    if ( gt->has_dd_cursor )
	GTextFieldDrawDDCursor(gt,gt->dd_cursor_pos);
    GDrawSetFont(g->base,gt->font);
    i = (event->u.drag_drop.y-g->inner.y)/gt->fh + gt->loff_top;
    if ( !gt->multi_line ) i = 0;
    if ( i>=gt->lcnt )
	end = gt->text+u_strlen(gt->text);
    else
	end = GTextFieldGetPtFromPos(gt,i,event->u.drag_drop.x);
    if ( event->type == et_drag ) {
	GTextFieldDrawDDCursor(gt,end-gt->text);
    } else if ( event->type == et_dragout ) {
	/* this event exists simply to clear the dd cursor line. We've done */
	/*  that already */ 
    } else if ( event->type == et_drop ) {
	gt->sel_start = gt->sel_end = gt->sel_base = end-gt->text;
	GTextFieldPaste(gt,sn_drag_and_drop);
	GTextField_Show(gt,gt->sel_start);
	GTextFieldChanged(gt,-1);
	_ggadget_redraw(&gt->g);
    } else
return( false );

return( true );
}

static void gtextfield_destroy(GGadget *g) {
    GTextField *gt = (GTextField *) g;

    if ( gt==NULL )
return;
    if ( gt->listfield ) {
	GListField *glf = (GListField *) g;
	if ( glf->popup ) {
	    GDrawDestroyWindow(glf->popup);
	    GDrawSync(NULL);
	    GDrawProcessWindowEvents(glf->popup);	/* popup's destroy routine must execute before we die */
	}
	GTextInfoArrayFree(glf->ti);
    }
    if ( gt->completionfield )
	GCompletionDestroy((GCompletionField *) g);

    if ( gt->vsb!=NULL )
	(gt->vsb->g.funcs->destroy)(&gt->vsb->g);
    if ( gt->hsb!=NULL )
	(gt->hsb->g.funcs->destroy)(&gt->hsb->g);
    GDrawCancelTimer(gt->numeric_scroll);
    GDrawCancelTimer(gt->pressed);
    GDrawCancelTimer(gt->cursor);
    free(gt->lines);
    free(gt->oldtext);
    free(gt->text);
    free(gt->utf8_text);
    free(gt->lines8);
    _ggadget_destroy(g);
}

static void GTextFieldSetTitle(GGadget *g,const unichar_t *tit) {
    GTextField *gt = (GTextField *) g;
    unichar_t *old = gt->oldtext;
    if ( tit==NULL || u_strcmp(tit,gt->text)==0 )	/* If it doesn't change anything, then don't trash undoes or selection */
return;
    gt->oldtext = gt->text;
    gt->sel_oldstart = gt->sel_start; gt->sel_oldend = gt->sel_end; gt->sel_oldbase = gt->sel_base;
    gt->text = u_copy(tit);		/* tit might be oldtext, so must copy before freeing */
    free(old);
    free(gt->utf8_text);
    gt->utf8_text = u2utf8_copy(gt->text);
    gt->sel_start = gt->sel_end = gt->sel_base = u_strlen(tit);
    GTextFieldRefigureLines(gt,0);
    GTextField_Show(gt,gt->sel_start);
    _ggadget_redraw(g);
}

static const unichar_t *_GTextFieldGetTitle(GGadget *g) {
    GTextField *gt = (GTextField *) g;
return( gt->text );
}

static void GTextFieldSetFont(GGadget *g,FontInstance *new) {
    GTextField *gt = (GTextField *) g;
    gt->font = new;
    GTextFieldRefigureLines(gt,0);
}

static FontInstance *GTextFieldGetFont(GGadget *g) {
    GTextField *gt = (GTextField *) g;
return( gt->font );
}

void GTextFieldShow(GGadget *g,int pos) {
    GTextField *gt = (GTextField *) g;

    GTextField_Show(gt,pos);
    _ggadget_redraw(g);
}

void GTextFieldSelect(GGadget *g,int start, int end) {
    GTextField *gt = (GTextField *) g;

    GTextFieldGrabPrimarySelection(gt);
    if ( end<0 ) {
	end = u_strlen(gt->text);
	if ( start<0 ) start = end;
    }
    if ( start>end ) { int temp = start; start = end; end = temp; }
    if ( end>u_strlen(gt->text)) end = u_strlen(gt->text);
    if ( start>u_strlen(gt->text)) start = end;
    else if ( start<0 ) start=0;
    gt->sel_start = gt->sel_base = start;
    gt->sel_end = end;
    _ggadget_redraw(g);			/* Should be safe just to draw the textfield gadget, sbs won't have changed */
}

void GTextFieldReplace(GGadget *g,const unichar_t *txt) {
    GTextField *gt = (GTextField *) g;

    GTextField_Replace(gt,txt);
    _ggadget_redraw(g);
}

static void GListFSelectOne(GGadget *g, int32 pos) {
    GListField *gl = (GListField *) g;
    int i;

    for ( i=0; i<gl->ltot; ++i )
	gl->ti[i]->selected = false;
    if ( pos>=gl->ltot ) pos = gl->ltot-1;
    if ( pos<0 ) pos = 0;
    if ( gl->ltot>0 ) {
	gl->ti[pos]->selected = true;
	GTextFieldSetTitle(g,gl->ti[pos]->text);
    }
}

static int32 GListFIsSelected(GGadget *g, int32 pos) {
    GListField *gl = (GListField *) g;

    if ( pos>=gl->ltot )
return( false );
    if ( pos<0 )
return( false );
    if ( gl->ltot>0 )
return( gl->ti[pos]->selected );

return( false );
}

static int32 GListFGetFirst(GGadget *g) {
    int i;
    GListField *gl = (GListField *) g;

    for ( i=0; i<gl->ltot; ++i )
	if ( gl->ti[i]->selected )
return( i );

return( -1 );
}

static GTextInfo **GListFGet(GGadget *g,int32 *len) {
    GListField *gl = (GListField *) g;
    if ( len!=NULL ) *len = gl->ltot;
return( gl->ti );
}

static GTextInfo *GListFGetItem(GGadget *g,int32 pos) {
    GListField *gl = (GListField *) g;
    if ( pos<0 || pos>=gl->ltot )
return( NULL );

return(gl->ti[pos]);
}

static void GListFSet(GGadget *g,GTextInfo **ti,int32 docopy) {
    GListField *gl = (GListField *) g;

    GTextInfoArrayFree(gl->ti);
    if ( docopy || ti==NULL )
	ti = GTextInfoArrayCopy(ti);
    gl->ti = ti;
    gl->ltot = GTextInfoArrayCount(ti);
}

static void GListFClear(GGadget *g) {
    GListFSet(g,NULL,true);
}

static void gtextfield_redraw(GGadget *g) {
    GTextField *gt = (GTextField *) g;
    if ( gt->vsb!=NULL )
	_ggadget_redraw((GGadget *) (gt->vsb));
    if ( gt->hsb!=NULL )
	_ggadget_redraw((GGadget *) (gt->hsb));
    _ggadget_redraw(g);
}

static void gtextfield_move(GGadget *g, int32 x, int32 y ) {
    GTextField *gt = (GTextField *) g;
    int fxo=0, fyo=0, bxo, byo;

    if ( gt->listfield || gt->numericfield ) {
	fxo = ((GListField *) gt)->fieldrect.x - g->r.x;
	fyo = ((GListField *) gt)->fieldrect.y - g->r.y;
	bxo = ((GListField *) gt)->buttonrect.x - g->r.x;
	byo = ((GListField *) gt)->buttonrect.y - g->r.y;
    }
    if ( gt->vsb!=NULL )
	_ggadget_move((GGadget *) (gt->vsb),x+(gt->vsb->g.r.x-g->r.x),y);
    if ( gt->hsb!=NULL )
	_ggadget_move((GGadget *) (gt->hsb),x,y+(gt->hsb->g.r.y-g->r.y));
    _ggadget_move(g,x,y);
    if ( gt->listfield || gt->numericfield ) {
	((GListField *) gt)->fieldrect.x = g->r.x + fxo;
	((GListField *) gt)->fieldrect.y = g->r.y + fyo;
	((GListField *) gt)->buttonrect.x = g->r.x + bxo;
	((GListField *) gt)->buttonrect.y = g->r.y + byo;
    }
}

static void gtextfield_resize(GGadget *g, int32 width, int32 height ) {
    GTextField *gt = (GTextField *) g;
    int gtwidth=width, gtheight=height, oldheight=0;
    int fxo=0, fwo=0, fyo=0, bxo, byo;
    int l;

    if ( gt->listfield || gt->numericfield ) {
	fxo = ((GListField *) gt)->fieldrect.x - g->r.x;
	fwo = g->r.width - ((GListField *) gt)->fieldrect.width;
	fyo = ((GListField *) gt)->fieldrect.y - g->r.y;
	bxo = g->r.x+g->r.width - ((GListField *) gt)->buttonrect.x;
	byo = ((GListField *) gt)->buttonrect.y - g->r.y;
    }
    if ( gt->hsb!=NULL ) {
	oldheight = gt->hsb->g.r.y+gt->hsb->g.r.height-g->r.y;
	gtheight = height - (oldheight-g->r.height);
    }
    if ( gt->vsb!=NULL ) {
	int oldwidth = gt->vsb->g.r.x+gt->vsb->g.r.width-g->r.x;
	gtwidth = width - (oldwidth-g->r.width);
	_ggadget_move((GGadget *) (gt->vsb),gt->vsb->g.r.x+width-oldwidth,gt->vsb->g.r.y);
	_ggadget_resize((GGadget *) (gt->vsb),gt->vsb->g.r.width,gtheight);
    }
    if ( gt->hsb!=NULL ) {
	_ggadget_move((GGadget *) (gt->hsb),gt->hsb->g.r.x,gt->hsb->g.r.y+height-oldheight);
	_ggadget_resize((GGadget *) (gt->hsb),gtwidth,gt->hsb->g.r.height);
    }
    _ggadget_resize(g,gtwidth, gtheight);

    if ( gt->hsb==NULL && gt->xoff_left!=0 && !gt->multi_line &&
	    GDrawGetTextWidth(gt->g.base,gt->text,-1)<gt->g.inner.width )
	gt->xoff_left = 0;

    GTextFieldRefigureLines(gt,0);
    if ( gt->vsb!=NULL ) {
	GScrollBarSetBounds(&gt->vsb->g,0,gt->lcnt,
		gt->g.inner.height<gt->fh ? 1 : gt->g.inner.height/gt->fh);
	l = gt->loff_top;
	if ( gt->loff_top>gt->lcnt-gt->g.inner.height/gt->fh )
	    l = gt->lcnt-gt->g.inner.height/gt->fh;
	if ( l<0 ) l = 0;
	if ( l!=gt->loff_top ) {
	    gt->loff_top = l;
	    GScrollBarSetPos(&gt->vsb->g,l);
	    _ggadget_redraw(&gt->g);
	}
    }
    if ( gt->listfield || gt->numericfield) {
	((GListField *) gt)->fieldrect.x = g->r.x + fxo;
	((GListField *) gt)->fieldrect.width = g->r.width -fwo;
	((GListField *) gt)->fieldrect.y = g->r.y + fyo;
	((GListField *) gt)->buttonrect.x = g->r.x+g->r.width - bxo;
	((GListField *) gt)->buttonrect.y = g->r.y + byo;
    }
}

static GRect *gtextfield_getsize(GGadget *g, GRect *r ) {
    GTextField *gt = (GTextField *) g;
    _ggadget_getsize(g,r);
    if ( gt->vsb!=NULL )
	r->width =  gt->vsb->g.r.x+gt->vsb->g.r.width-g->r.x;
    if ( gt->hsb!=NULL )
	r->height =  gt->hsb->g.r.y+gt->hsb->g.r.height-g->r.y;
return( r );
}

static void gtextfield_setvisible(GGadget *g, int visible ) {
    GTextField *gt = (GTextField *) g;
    if ( gt->vsb!=NULL ) _ggadget_setvisible(&gt->vsb->g,visible);
    if ( gt->hsb!=NULL ) _ggadget_setvisible(&gt->hsb->g,visible);
    _ggadget_setvisible(g,visible);
}

static void gtextfield_setenabled(GGadget *g, int enabled ) {
    GTextField *gt = (GTextField *) g;
    if ( gt->vsb!=NULL ) _ggadget_setenabled(&gt->vsb->g,enabled);
    if ( gt->hsb!=NULL ) _ggadget_setenabled(&gt->hsb->g,enabled);
    _ggadget_setenabled(g,enabled);
}

static int gtextfield_vscroll(GGadget *g, GEvent *event) {
    enum sb sbt = event->u.control.u.sb.type;
    GTextField *gt = (GTextField *) (g->data);
    int loff = gt->loff_top;

    g = (GGadget *) gt;

    if ( sbt==et_sb_top )
	loff = 0;
    else if ( sbt==et_sb_bottom ) {
	loff = gt->lcnt - gt->g.inner.height/gt->fh;
    } else if ( sbt==et_sb_up ) {
	if ( gt->loff_top!=0 ) loff = gt->loff_top-1; else loff = 0;
    } else if ( sbt==et_sb_down ) {
	if ( gt->loff_top + gt->g.inner.height/gt->fh >= gt->lcnt )
	    loff = gt->lcnt - gt->g.inner.height/gt->fh;
	else
	    ++loff;
    } else if ( sbt==et_sb_uppage ) {
	int page = g->inner.height/gt->fh- (g->inner.height/gt->fh>2?1:0);
	loff = gt->loff_top - page;
	if ( loff<0 ) loff=0;
    } else if ( sbt==et_sb_downpage ) {
	int page = g->inner.height/gt->fh- (g->inner.height/gt->fh>2?1:0);
	loff = gt->loff_top + page;
	if ( loff + gt->g.inner.height/gt->fh >= gt->lcnt )
	    loff = gt->lcnt - gt->g.inner.height/gt->fh;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	loff = event->u.control.u.sb.pos;
    }
    if ( loff + gt->g.inner.height/gt->fh >= gt->lcnt )
	loff = gt->lcnt - gt->g.inner.height/gt->fh;
    if ( loff<0 ) loff = 0;
    if ( loff!=gt->loff_top ) {
	gt->loff_top = loff;
	GScrollBarSetPos(&gt->vsb->g,loff);
	_ggadget_redraw(&gt->g);
    }
return( true );
}

static int gtextfield_hscroll(GGadget *g, GEvent *event) {
    enum sb sbt = event->u.control.u.sb.type;
    GTextField *gt = (GTextField *) (g->data);
    int xoff = gt->xoff_left;

    g = (GGadget *) gt;

    if ( sbt==et_sb_top )
	xoff = 0;
    else if ( sbt==et_sb_bottom ) {
	xoff = gt->xmax - gt->g.inner.width;
	if ( xoff<0 ) xoff = 0;
    } else if ( sbt==et_sb_up ) {
	if ( gt->xoff_left>gt->nw ) xoff = gt->xoff_left-gt->nw; else xoff = 0;
    } else if ( sbt==et_sb_down ) {
	if ( gt->xoff_left + gt->nw + gt->g.inner.width >= gt->xmax )
	    xoff = gt->xmax - gt->g.inner.width;
	else
	    xoff += gt->nw;
    } else if ( sbt==et_sb_uppage ) {
	int page = (3*g->inner.width)/4;
	xoff = gt->xoff_left - page;
	if ( xoff<0 ) xoff=0;
    } else if ( sbt==et_sb_downpage ) {
	int page = (3*g->inner.width)/4;
	xoff = gt->xoff_left + page;
	if ( xoff + gt->g.inner.width >= gt->xmax )
	    xoff = gt->xmax - gt->g.inner.width;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	xoff = event->u.control.u.sb.pos;
    }
    if ( xoff + gt->g.inner.width >= gt->xmax )
	xoff = gt->xmax - gt->g.inner.width;
    if ( xoff<0 ) xoff = 0;
    if ( gt->xoff_left!=xoff ) {
	gt->xoff_left = xoff;
	GScrollBarSetPos(&gt->hsb->g,xoff);
	_ggadget_redraw(&gt->g);
    }
return( true );
}

static void GTextFieldSetDesiredSize(GGadget *g,GRect *outer,GRect *inner) {
    GTextField *gt = (GTextField *) g;

    if ( outer!=NULL ) {
	g->desired_width = outer->width;
	g->desired_height = outer->height;
    } else if ( inner!=NULL ) {
	int bp = GBoxBorderWidth(g->base,g->box);
	int extra=0;

	if ( gt->listfield ) {
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize) +
		    GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		    2*GBoxBorderWidth(gt->g.base,&_GListMark_Box) +
		    GBoxBorderWidth(gt->g.base,&glistfieldmenu_box);
	} else if ( gt->numericfield ) {
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize)/2 +
		    GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		    2*GBoxBorderWidth(gt->g.base,&gnumericfieldspinner_box);
	}
	g->desired_width = inner->width + 2*bp + extra;
	g->desired_height = inner->height + 2*bp;
	if ( gt->multi_line ) {
	    int sbadd = GDrawPointsToPixels(gt->g.base,_GScrollBar_Width) +
		    GDrawPointsToPixels(gt->g.base,1);
	    g->desired_width += sbadd;
	    if ( !gt->wrap )
		g->desired_height += sbadd;
	}
    }
}

static void GTextFieldGetDesiredSize(GGadget *g,GRect *outer,GRect *inner) {
    GTextField *gt = (GTextField *) g;
    int width=0, height;
    int extra=0;
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( gt->listfield ) {
	extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize) +
		GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		2*GBoxBorderWidth(gt->g.base,&_GListMark_Box) +
		GBoxBorderWidth(gt->g.base,&glistfieldmenu_box);
    } else if ( gt->numericfield ) {
	extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize)/2 +
		GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		2*GBoxBorderWidth(gt->g.base,&gnumericfieldspinner_box);
    }

    width = GGadgetScale(GDrawPointsToPixels(gt->g.base,80));
    height = gt->multi_line? 4*gt->fh:gt->fh;

    if ( g->desired_width>extra+2*bp ) width = g->desired_width - extra - 2*bp;
    if ( g->desired_height>2*bp ) height = g->desired_height - 2*bp;

    if ( gt->multi_line ) {
	int sbadd = GDrawPointsToPixels(gt->g.base,_GScrollBar_Width) +
		GDrawPointsToPixels(gt->g.base,1);
	width += sbadd;
	if ( !gt->wrap )
	    height += sbadd;
    }

    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = width;
	inner->height = height;
    }
    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = width + extra + 2*bp;
	outer->height = height + 2*bp;
    }
}

static int gtextfield_FillsWindow(GGadget *g) {
return( ((GTextField *) g)->multi_line && g->prev==NULL &&
	(_GWidgetGetGadgets(g->base)==g ||
	 _GWidgetGetGadgets(g->base)==(GGadget *) ((GTextField *) g)->vsb ||
	 _GWidgetGetGadgets(g->base)==(GGadget *) ((GTextField *) g)->hsb ));
}

struct gfuncs gtextfield_funcs = {
    0,
    sizeof(struct gfuncs),

    gtextfield_expose,
    gtextfield_mouse,
    gtextfield_key,
    _gtextfield_editcmd,
    gtextfield_focus,
    gtextfield_timer,
    gtextfield_sel,

    gtextfield_redraw,
    gtextfield_move,
    gtextfield_resize,
    gtextfield_setvisible,
    gtextfield_setenabled,
    gtextfield_getsize,
    _ggadget_getinnersize,

    gtextfield_destroy,

    GTextFieldSetTitle,
    _GTextFieldGetTitle,
    NULL,
    NULL,
    NULL,
    GTextFieldSetFont,
    GTextFieldGetFont,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    GTextFieldGetDesiredSize,
    GTextFieldSetDesiredSize,
    gtextfield_FillsWindow,
    NULL
};

struct gfuncs glistfield_funcs = {
    0,
    sizeof(struct gfuncs),

    gtextfield_expose,
    gtextfield_mouse,
    gtextfield_key,
    gtextfield_editcmd,
    gtextfield_focus,
    gtextfield_timer,
    gtextfield_sel,

    gtextfield_redraw,
    gtextfield_move,
    gtextfield_resize,
    gtextfield_setvisible,
    gtextfield_setenabled,
    gtextfield_getsize,
    _ggadget_getinnersize,

    gtextfield_destroy,

    GTextFieldSetTitle,
    _GTextFieldGetTitle,
    NULL,
    NULL,
    NULL,
    GTextFieldSetFont,
    GTextFieldGetFont,

    GListFClear,
    GListFSet,
    GListFGet,
    GListFGetItem,
    NULL,
    GListFSelectOne,
    GListFIsSelected,
    GListFGetFirst,
    NULL,
    NULL,
    NULL,

    GTextFieldGetDesiredSize,
    GTextFieldSetDesiredSize,
    NULL,
    NULL
};

static void GTextFieldInit() {
    FontRequest rq;

    memset(&rq,0,sizeof(rq));
    GGadgetInit();
    GDrawDecomposeFont(_ggadget_default_font,&rq);
    rq.family_name = NULL;
    rq.utf8_family_name = MONO_UI_FAMILIES;
    _gtextfield_font = GDrawInstanciateFont(NULL,&rq);
    _GGadgetCopyDefaultBox(&_GGadget_gtextfield_box);
    _GGadget_gtextfield_box.padding = 3;
    /*_GGadget_gtextfield_box.flags = box_active_border_inner;*/
    _gtextfield_font = _GGadgetInitDefaultBox("GTextField.",&_GGadget_gtextfield_box,_gtextfield_font);
    glistfield_box = _GGadget_gtextfield_box;
    _GGadgetInitDefaultBox("GComboBox.",&glistfield_box,_gtextfield_font);
    glistfieldmenu_box = glistfield_box;
    glistfieldmenu_box.padding = 1;
    _GGadgetInitDefaultBox("GComboBoxMenu.",&glistfieldmenu_box,_gtextfield_font);
    gnumericfield_box = _GGadget_gtextfield_box;
    _GGadgetInitDefaultBox("GNumericField.",&gnumericfield_box,_gtextfield_font);
    gnumericfieldspinner_box = gnumericfield_box;
    gnumericfieldspinner_box.border_type = bt_none;
    gnumericfieldspinner_box.border_width = 0;
    gnumericfieldspinner_box.padding = 0;
    _GGadgetInitDefaultBox("GNumericFieldSpinner.",&gnumericfieldspinner_box,_gtextfield_font);
    gtextfield_inited = true;
}

static void GTextFieldAddVSb(GTextField *gt) {
    GGadgetData gd;

    memset(&gd,'\0',sizeof(gd));
    gd.pos.y = gt->g.r.y; gd.pos.height = gt->g.r.height;
    gd.pos.width = GDrawPointsToPixels(gt->g.base,_GScrollBar_Width);
    gd.pos.x = gt->g.r.x+gt->g.r.width - gd.pos.width;
    gd.flags = (gt->g.state==gs_invisible?0:gg_visible)|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gd.handle_controlevent = gtextfield_vscroll;
    gt->vsb = (GScrollBar *) GScrollBarCreate(gt->g.base,&gd,gt);
    gt->vsb->g.contained = true;

    gd.pos.width += GDrawPointsToPixels(gt->g.base,1);
    gt->g.r.width -= gd.pos.width;
    gt->g.inner.width -= gd.pos.width;
}

static void GTextFieldAddHSb(GTextField *gt) {
    GGadgetData gd;

    memset(&gd,'\0',sizeof(gd));
    gd.pos.x = gt->g.r.x; gd.pos.width = gt->g.r.width;
    gd.pos.height = GDrawPointsToPixels(gt->g.base,_GScrollBar_Width);
    gd.pos.y = gt->g.r.y+gt->g.r.height - gd.pos.height;
    gd.flags = (gt->g.state==gs_invisible?0:gg_visible)|gg_enabled|gg_pos_in_pixels;
    gd.handle_controlevent = gtextfield_hscroll;
    gt->hsb = (GScrollBar *) GScrollBarCreate(gt->g.base,&gd,gt);
    gt->hsb->g.contained = true;

    gd.pos.height += GDrawPointsToPixels(gt->g.base,1);
    gt->g.r.height -= gd.pos.height;
    gt->g.inner.height -= gd.pos.height;
    if ( gt->vsb!=NULL ) {
	gt->vsb->g.r.height -= gd.pos.height;
	gt->vsb->g.inner.height -= gd.pos.height;
    }
}

static void GTextFieldFit(GTextField *gt) {
    GTextBounds bounds;
    int as=0, ds, ld, width=0;
    GRect inner, outer;
    int bp = GBoxBorderWidth(gt->g.base,gt->g.box);

    {
	FontInstance *old = GDrawSetFont(gt->g.base,gt->font);
	FontRequest rq;
	int tries;
	for ( tries = 0; tries<2; ++tries ) {
	    width = GDrawGetTextBounds(gt->g.base,gt->text, -1, &bounds);
	    GDrawWindowFontMetrics(gt->g.base,gt->font,&as, &ds, &ld);
	    if ( gt->g.r.height==0 || as+ds-3+2*bp<=gt->g.r.height || tries==1 )
	break;
	    /* Doesn't fit. Try a smaller size */
	    GDrawDecomposeFont(gt->font,&rq);
	    --rq.point_size;
	    gt->font = GDrawInstanciateFont(gt->g.base,&rq);
	}
	gt->fh = as+ds;
	gt->as = as;
	gt->nw = GDrawGetTextWidth(gt->g.base,nstr, 1);
	GDrawSetFont(gt->g.base,old);
    }

    GTextFieldGetDesiredSize(&gt->g,&outer,&inner);
    if ( gt->g.r.width==0 ) {
	int extra=0;
	if ( gt->listfield ) {
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize) +
		    2*GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		    GBoxBorderWidth(gt->g.base,&_GListMark_Box);
	} else if ( gt->numericfield ) {
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize)/2 +
		    GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		    2*GBoxBorderWidth(gt->g.base,&gnumericfieldspinner_box);
	}
	gt->g.r.width = outer.width;
	gt->g.inner.width = inner.width;
	gt->g.inner.x = gt->g.r.x + (outer.width-inner.width-extra)/2;
    } else {
	gt->g.inner.x = gt->g.r.x + bp;
	gt->g.inner.width = gt->g.r.width - 2*bp;
    }
    if ( gt->g.r.height==0 ) {
	gt->g.r.height = outer.height;
	gt->g.inner.height = inner.height;
	gt->g.inner.y = gt->g.r.y + (outer.height-gt->g.inner.height)/2;
    } else {
	gt->g.inner.y = gt->g.r.y + bp;
	gt->g.inner.height = gt->g.r.height - 2*bp;
    }

    if ( gt->multi_line ) {
	GTextFieldAddVSb(gt);
	if ( !gt->wrap )
	    GTextFieldAddHSb(gt);
    }
    if ( gt->listfield || gt->numericfield ) {
	GListField *ge = (GListField *) gt;
	int extra;
	if ( gt->listfield )
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize) +
		    GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		    2*GBoxBorderWidth(gt->g.base,&_GListMark_Box)+
		    GBoxBorderWidth(gt->g.base,&glistfieldmenu_box);
	else {
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize)/2 +
		    GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		    2*GBoxBorderWidth(gt->g.base,&gnumericfieldspinner_box);
	}
	ge->fieldrect = ge->buttonrect = gt->g.r;
	ge->fieldrect.width -= extra;
	extra -= GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip)/2;
	ge->buttonrect.x = ge->buttonrect.x+ge->buttonrect.width-extra;
	ge->buttonrect.width = extra;
	if ( gt->numericfield )
	    ++ge->fieldrect.width;
    }
}

static GTextField *_GTextFieldCreate(GTextField *gt, struct gwindow *base, GGadgetData *gd,void *data, GBox *def) {

    if ( !gtextfield_inited )
	GTextFieldInit();
    gt->g.funcs = &gtextfield_funcs;
    _GGadget_Create(&gt->g,base,gd,data,def);

    gt->g.takes_input = true; gt->g.takes_keyboard = true; gt->g.focusable = true;
    if ( gd->label!=NULL ) {
	if ( gd->label->text_is_1byte )
	    gt->text = /* def2u_*/ utf82u_copy((char *) gd->label->text);
	else if ( gd->label->text_in_resource )
	    gt->text = u_copy((unichar_t *) GStringGetResource((intpt) gd->label->text,&gt->g.mnemonic));
	else
	    gt->text = u_copy(gd->label->text);
	gt->sel_start = gt->sel_end = gt->sel_base = u_strlen(gt->text);
    }
    if ( gt->text==NULL )
	gt->text = calloc(1,sizeof(unichar_t));
    gt->font = _gtextfield_font;
    if ( gd->label!=NULL && gd->label->font!=NULL )
	gt->font = gd->label->font;
    if ( (gd->flags & gg_textarea_wrap) && gt->multi_line )
	gt->wrap = true;
    else if ( (gd->flags & gg_textarea_wrap) )	/* only used by gchardlg.c no need to make it look nice */
	gt->donthook = true;
    GTextFieldFit(gt);
    _GGadget_FinalPosition(&gt->g,base,gd);
    GTextFieldRefigureLines(gt,0);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&gt->g);
    GWidgetIndicateFocusGadget(&gt->g);
    if ( gd->flags & gg_text_xim )
	gt->gic = GWidgetCreateInputContext(base,gic_overspot|gic_orlesser);
return( gt );
}

GGadget *GTextFieldCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTextField *gt = _GTextFieldCreate(calloc(1,sizeof(GTextField)),base,gd,data,&_GGadget_gtextfield_box);

return( &gt->g );
}

GGadget *GPasswordCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTextField *gt = _GTextFieldCreate(calloc(1,sizeof(GTextField)),base,gd,data,&_GGadget_gtextfield_box);
    gt->password = true;
    GTextFieldRefigureLines(gt, 0);

return( &gt->g );
}

GGadget *GNumericFieldCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTextField *gt = calloc(1,sizeof(GNumericField));
    gt->numericfield = true;
    _GTextFieldCreate(gt,base,gd,data,&gnumericfield_box);

return( &gt->g );
}

GGadget *GTextCompletionCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTextField *gt = calloc(1,sizeof(GCompletionField));
    gt->accepts_tabs = true;
    gt->completionfield = true;
    gt->was_completing = true;
    ((GCompletionField *) gt)->completion = gd->u.completion;
    _GTextFieldCreate(gt,base,gd,data,&_GGadget_gtextfield_box);
    gt->accepts_tabs = ((GCompletionField *) gt)->completion != NULL;

return( &gt->g );
}

GGadget *GTextAreaCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTextField *gt = calloc(1,sizeof(GTextField));
    gt->multi_line = true;
    gt->accepts_returns = true;
    _GTextFieldCreate(gt,base,gd,data,&_GGadget_gtextfield_box);

return( &gt->g );
}

static void GListFieldSelected(GGadget *g, int i) {
    GListField *ge = (GListField *) g;

    ge->popup = NULL;
    _GWidget_ClearGrabGadget(&ge->gt.g);
    if ( i<0 || i>=ge->ltot || ge->ti[i]->text==NULL )
return;
    GTextFieldSetTitle(g,ge->ti[i]->text);
    _ggadget_redraw(g);

    GTextFieldChanged(&ge->gt,i);
}

GGadget *GSimpleListFieldCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GListField *ge = calloc(1,sizeof(GListField));

    ge->gt.listfield = true;
    if ( gd->u.list!=NULL )
	ge->ti = GTextInfoArrayFromList(gd->u.list,&ge->ltot);
    _GTextFieldCreate(&ge->gt,base,gd,data,&glistfield_box);
    ge->gt.g.funcs = &glistfield_funcs;
return( &ge->gt.g );
}

static unichar_t **GListField_NameCompletion(GGadget *t,int from_tab) {
    const unichar_t *spt; unichar_t **ret;
    GTextInfo **ti;
    int32 len;
    int i, cnt, doit, match_len;

    spt = _GGadgetGetTitle(t);
    if ( spt==NULL )
return( NULL );

    match_len = u_strlen(spt);
    ti = GGadgetGetList(t,&len);
    ret = NULL;
    for ( doit=0; doit<2; ++doit ) {
	cnt=0;
	for ( i=0; i<len; ++i ) {
	    if ( ti[i]->text && u_strncmp(ti[i]->text,spt,match_len)==0 ) {
		if ( doit )
		    ret[cnt] = u_copy(ti[i]->text);
		++cnt;
	    }
	}
	if ( doit )
	    ret[cnt] = NULL;
	else if ( cnt==0 )
return( NULL );
	else
	    ret = malloc((cnt+1)*sizeof(unichar_t *));
    }
return( ret );
}

GGadget *GListFieldCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GListField *ge = calloc(1,sizeof(GCompletionField));

    ge->gt.listfield = true;
    if ( gd->u.list!=NULL )
	ge->ti = GTextInfoArrayFromList(gd->u.list,&ge->ltot);
    ge->gt.accepts_tabs = true;
    ge->gt.completionfield = true;
    /* ge->gt.was_completing = true; */
    ((GCompletionField *) ge)->completion = GListField_NameCompletion;
    _GTextFieldCreate(&ge->gt,base,gd,data,&_GGadget_gtextfield_box);
    ge->gt.g.funcs = &glistfield_funcs;
return( &ge->gt.g );
}

/* ************************************************************************** */
/* ***************************** text completion **************************** */
/* ************************************************************************** */

static void GCompletionDestroy(GCompletionField *gc) {
    int i;

    if ( gc->choice_popup!=NULL ) {
	GWindow cp = gc->choice_popup;
	gc->choice_popup = NULL;
	GDrawSetUserData(cp,NULL);
	GDrawDestroyWindow(cp);
    }
    if ( gc->choices!=NULL ) {
	for ( i=0; gc->choices[i]!=NULL; ++i )
	    free(gc->choices[i]);
	free(gc->choices);
	gc->choices = NULL;
    }
}

static int GTextFieldSetTitleRmDotDotDot(GGadget *g,unichar_t *tit) {
    unichar_t *pt = uc_strstr(tit," ...");
    if ( pt!=NULL )
	*pt = '\0';
    GTextFieldSetTitle(g,tit);
    if ( pt!=NULL )
	*pt = ' ';
return( pt!=NULL );
}

static int popup_eh(GWindow popup,GEvent *event) {
    GGadget *owner = GDrawGetUserData(popup);
    GTextField *gt = (GTextField *) owner;
    GCompletionField *gc = (GCompletionField *) owner;
    GRect old1, r;
    Color fg;
    int i, bp;

    if ( owner==NULL )		/* dying */
return( true );

    bp = GBoxBorderWidth(owner->base,owner->box);
    if ( event->type == et_expose ) {
	GDrawPushClip(popup,&event->u.expose.rect,&old1);
	GDrawSetFont(popup,gt->font);
	GBoxDrawBackground(popup,&event->u.expose.rect,owner->box,
		owner->state,false);
	GDrawGetSize(popup,&r);
	r.x = r.y = 0;
	GBoxDrawBorder(popup,&r,owner->box,owner->state,false);
	r.x += bp; r.width -= 2*bp;
	fg = owner->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(popup)):
		owner->box->main_foreground;
	for ( i=0; gc->choices[i]!=NULL; ++i ) {
	    if ( i==gc->selected ) {
		r.y = i*gt->fh+bp;
		r.height = gt->fh;
		GDrawFillRect(popup,&r,owner->box->active_border);
	    }
	    GDrawDrawText(popup,bp,i*gt->fh+gt->as+bp,gc->choices[i],-1,fg);
	}
	GDrawPopClip(popup,&old1);
    } else if ( event->type == et_mouseup ) {
	gc->selected = (event->u.mouse.y-bp)/gt->fh;
	if ( gc->selected>=0 && gc->selected<gc->ctot ) {
	    int tryagain = GTextFieldSetTitleRmDotDotDot(owner,gc->choices[gc->selected]);
	    GTextFieldChanged(gt,-1);
	    GCompletionDestroy(gc);
	    if ( tryagain )
		GTextFieldComplete(gt,false);
	} else {
	    gc->selected = -1;
	    GDrawRequestExpose(popup,NULL,false);
	}
    } else if ( event->type == et_char ) {
return( gtextfield_key(owner,event));
    }
return( true );
}

static void GCompletionCreatePopup(GCompletionField *gc) {
    int width, maxw, i;
    GWindowAttrs pattrs;
    GWindow base = gc->gl.gt.g.base;
    GDisplay *disp = GDrawGetDisplayOfWindow(base);
    GWindow root = GDrawGetRoot(disp);
    int bp = GBoxBorderWidth(base,gc->gl.gt.g.box);
    GRect pos, screen;
    GPoint pt;

    GDrawSetFont(base,gc->gl.gt.font);

    maxw = 0;
    for ( i=0; i<gc->ctot; ++i ) {
	width = GDrawGetTextWidth(base,gc->choices[i],-1);
	if ( width > maxw ) maxw = width;
    }
    maxw += 2*bp;
    pos.width = maxw; pos.height = gc->gl.gt.fh*gc->ctot+2*bp;
    if ( pos.width < gc->gl.gt.g.r.width )
	pos.width = gc->gl.gt.g.r.width;

    pattrs.mask = wam_events|wam_nodecor|wam_positioned|wam_cursor|
	    wam_transient|wam_verytransient/*|wam_bordwidth|wam_bordcol*/;
    pattrs.event_masks = -1;
    pattrs.nodecoration = true;
    pattrs.positioned = true;
    pattrs.cursor = ct_pointer;
    pattrs.transient = GWidgetGetTopWidget(base);
    pattrs.border_width = 1;
    pattrs.border_color = gc->gl.gt.g.box->main_foreground;

    GDrawGetSize(root,&screen);
    pt.x = gc->gl.gt.g.r.x;
    pt.y = gc->gl.gt.g.r.y + gc->gl.gt.g.r.height;
    GDrawTranslateCoordinates(base,root,&pt);
    if (  pt.y+pos.height > screen.height ) {
	if ( pt.y-gc->gl.gt.g.r.height-pos.height>=0 ) {
	    /* Is there more room above the widget ?? */
	    pt.y -= gc->gl.gt.g.r.height;
	    pt.y -= pos.height;
	} else if ( pt.x + gc->gl.gt.g.r.width + maxw <= screen.width ) {
	    pt.x += gc->gl.gt.g.r.width;
	    pt.y = 0;
	} else
	    pt.x = pt.y = 0;
    }
    pos.x = pt.x; pos.y = pt.y;

    gc->choice_popup = GWidgetCreateTopWindow(disp,&pos,popup_eh,gc,&pattrs);
    GDrawSetGIC(gc->choice_popup,GWidgetCreateInputContext(gc->choice_popup,gic_overspot|gic_orlesser),
	    gc->gl.gt.g.inner.x,gc->gl.gt.g.inner.y+gc->gl.gt.as);
    GDrawSetVisible(gc->choice_popup,true);
    /* Don't grab this one. User should be free to ignore it */
}

static int ucmp(const void *_s1, const void *_s2) {
return( u_strcmp(*(const unichar_t **)_s1,*(const unichar_t **)_s2));
}

#define MAXLINES	30		/* Maximum # entries allowed in popup window */
#define MAXBRACKETS	30		/* Maximum # chars allowed in [] pairs */

static void GTextFieldComplete(GTextField *gt,int from_tab) {
    GCompletionField *gc = (GCompletionField *) gt;
    unichar_t **ret;
    int i, len, orig_len;
    unichar_t *pt1, *pt2, ch;
    /* If not from_tab, then the textfield has already been changed and we */
    /* must mark it as such (but don't mark twice) */

    ret = (gc->completion)(&gt->g,from_tab);
    if ( ret==NULL || ret[0]==NULL ) {
	if ( from_tab )
	    GDrawBeep(NULL);
	else
	    GTextFieldChanged(gt,-1);
	free(ret);
    } else {
	orig_len = u_strlen(gt->text);
	len = u_strlen(ret[0]);
	for ( i=1; ret[i]!=NULL; ++i ) {
	    for ( pt1=ret[0], pt2=ret[i]; *pt1==*pt2 && pt1-ret[0]<len ; ++pt1, ++pt2 );
	    len = pt1-ret[0];
	}
	if ( orig_len!=len ) {
	    ch = ret[0][len]; ret[0][len] = '\0';
	    GTextFieldSetTitle(&gt->g,ret[0]);
	    ret[0][len] = ch;
	    if ( !from_tab )
		GTextFieldSelect(&gt->g,orig_len,len);
	    GTextFieldChanged(gt,-1);
	} else if ( !from_tab )
	    GTextFieldChanged(gt,-1);
	if ( ret[1]!=NULL ) {
	    gc->choices = ret;
	    gc->selected = -1;
	    if ( from_tab ) GDrawBeep(NULL);
	    qsort(ret,i,sizeof(unichar_t *),ucmp);
	    gc->ctot = i;
	    if ( i>=MAXLINES ) {
		/* Try to shrink the list by just showing initial stubs of the */
		/*  names with multiple entries with a common next character */
		/* So if we have matched against "a" and we have "abc", "abd" "acc" */
		/*  the show "ab..." and "acc" */
		unichar_t **ret2=NULL, last_ch = -1;
		int cnt, doit, type2=false;
		for ( doit=0; doit<2; ++doit ) {
		    for ( i=cnt=0; ret[i]!=NULL; ++i ) {
			if ( last_ch!=ret[i][len] ) {
			    if ( doit && type2 ) {
				int c2 = cnt/MAXBRACKETS, c3 = cnt%MAXBRACKETS;
				if ( ret[i][len]=='\0' )
		    continue;
				if ( c3==0 ) {
				    ret2[c2] = calloc((len+MAXBRACKETS+2+4+1),sizeof(unichar_t));
				    memcpy(ret2[c2],ret[i],len*sizeof(unichar_t));
				    ret2[c2][len] = '[';
				}
				ret2[c2][len+1+c3] = ret[i][len];
				uc_strcpy(ret2[c2]+len+2+c3,"] ...");
			    } else if ( doit ) {
				ret2[cnt] = malloc((u_strlen(ret[i])+5)*sizeof(unichar_t));
				u_strcpy(ret2[cnt],ret[i]);
			    }
			    ++cnt;
			    last_ch = ret[i][len];
			} else if ( doit && !type2 ) {
			    int j;
			    for ( j=len+1; ret[i][j]!='\0' && ret[i][j] == ret2[cnt-1][j]; ++j );
			    uc_strcpy(ret2[cnt-1]+j," ...");
			}
		    }
		    if ( cnt>=MAXLINES*MAXBRACKETS )
		break;
		    if ( cnt>=MAXLINES && !doit ) {
			type2 = (cnt+MAXBRACKETS-1)/MAXBRACKETS;
			ret2 = malloc((type2+1)*sizeof(unichar_t *));
		    } else if ( !doit )
			ret2 = malloc((cnt+1)*sizeof(unichar_t *));
		    else {
			if ( type2 )
			    cnt = type2;
			ret2[cnt] = NULL;
		    }
		}
		if ( ret2!=NULL ) {
		    for ( i=0; ret[i]!=NULL; ++i )
			free(ret[i]);
		    free(ret);
		    ret = gc->choices = ret2;
		    i = gc->ctot = cnt;
		}
	    }
	    if ( gc->ctot>=MAXLINES ) {
		/* Too many choices. Don't popup a list of them */
		gc->choices = NULL;
		for ( i=0; ret[i]!=NULL; ++i )
		    free(ret[i]);
		free(ret);
	    } else {
		gc->ctot = i;
		GCompletionCreatePopup(gc);
	    }
	} else {
	    free(ret[1]);
	    free(ret);
	}
    }
}

static int GCompletionHandleKey(GTextField *gt,GEvent *event) {
    GCompletionField *gc = (GCompletionField *) gt;
    int dir = 0;

    if ( gc->choice_popup==NULL || event->type == et_charup )
return( false );

    if ( event->u.chr.keysym == GK_Up || event->u.chr.keysym == GK_KP_Up )
	dir = -1;
    else if ( event->u.chr.keysym == GK_Down || event->u.chr.keysym == GK_KP_Down )
	dir = 1;

    if ( dir==0 || event->u.chr.chars[0]!='\0' ) {
	/* For normal characters we destroy the popup window and pretend it */
	/*  wasn't there */
	GCompletionDestroy(gc);
	if ( event->u.chr.keysym == GK_Escape )
	    gt->was_completing = false;
return( event->u.chr.keysym == GK_Escape ||	/* Eat an escape, other chars will be processed further */
	    event->u.chr.keysym == GK_Return );
    }

    if (( gc->selected==-1 && dir==-1 ) || ( gc->selected==gc->ctot-1 && dir==1 ))
return( true );
    gc->selected += dir;
    if ( gc->selected!=-1 )
	GTextFieldSetTitleRmDotDotDot(&gt->g,gc->choices[gc->selected]);
    GTextFieldChanged(gt,-1);
    GDrawRequestExpose(gc->choice_popup,NULL,false);
return( true );
}

void GCompletionFieldSetCompletion(GGadget *g,GTextCompletionHandler completion) {
    ((GCompletionField *) g)->completion = completion;
    ((GTextField *) g)->accepts_tabs = ((GCompletionField *) g)->completion != NULL;
}

void GCompletionFieldSetCompletionMode(GGadget *g,int enabled) {
    ((GTextField *) g)->was_completing = enabled;
}

GResInfo *_GTextFieldRIHead(void) {

    if ( !gtextfield_inited )
	GTextFieldInit();
return( &gtextfield_ri );
}
