/* Copyright (C) 2003 by George Williams */
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
#include "nomen.h"
#include <gkeysym.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>

typedef struct kernclassdlg {
    struct kernclasslistdlg *kcld;
    KernClass *new;
    KernClass *orig;
    GWindow gw;
    int beingbuilt;
    SplineChar *last1, *last2;
} KernClassDlg;

typedef struct kernclasslistdlg {
    SplineFont *sf;
    MetricsView *mv;
    KernClassDlg *kcd;
    GWindow gw;
    int isv;
} KernClassListDlg;

#define KCL_Width	200
#define KCL_Height	173
#define KC_Wid		90
#define KC_Height	294


#define CID_List	1020
#define CID_New		1021
#define CID_Delete	1022
#define CID_Edit	1023

#define CID_SLI		1000
#define CID_R2L		1001
#define CID_IgnBase	1002
#define CID_IgnLig	1003
#define CID_IgnMark	1004
#define CID_ClassCnt1	1005
#define CID_ClassCnt2	1006
#define CID_ClassIndex1	1007
#define CID_ClassIndex2	1008
#define CID_SelectClass1	1009
#define CID_SelectClass2	1010
#define CID_SetClass1	1011
#define CID_SetClass2	1012
#define CID_KernOffset	1013
#define CID_Label1	1014
#define CID_Label2	1015

static GTextInfo **KCSLIArray(SplineFont *sf,int isv) {
    int cnt;
    KernClass *kc, *head = isv ? sf->vkerns : sf->kerns;
    GTextInfo **ti;

    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt );
    ti = gcalloc(cnt+1,sizeof(GTextInfo*));
    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt ) {
	ti[cnt] = gcalloc(1,sizeof(GTextInfo));
	ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	ti[cnt]->text = ScriptLangLine(sf->script_lang[kc->sli]);
    }
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

static GTextInfo *KCSLIList(SplineFont *sf,int isv) {
    int cnt;
    KernClass *kc, *head = isv ? sf->vkerns : sf->kerns;
    GTextInfo *ti;

    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt );
    ti = gcalloc(cnt+1,sizeof(GTextInfo));
    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt )
	ti[cnt].text = ScriptLangLine(sf->script_lang[kc->sli]);
return( ti );
}

static int KC_Sli(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;
    int sli;

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	sli = GGadgetGetFirstListSelectedItem(g);
	if ( kcd->kcld->sf->script_lang!=NULL &&
		kcd->kcld->sf->script_lang[sli]!=NULL )
	    kcd->new->sli = sli;
	else
	    ScriptLangList(kcd->kcld->sf,g,kcd->new->sli);
    }
return( true );
}

static int KC_Flag(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;
    int bit;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	switch( GGadgetGetCid(g)) {
	  case CID_R2L:
	    bit = pst_r2l;
	  break;
	  case CID_IgnBase:
	    bit = pst_ignorebaseglyphs;
	  break;
	  case CID_IgnLig:
	    bit = pst_ignoreligatures;
	  break;
	  case CID_IgnMark:
	    bit = pst_ignorecombiningmarks;
	  break;
        }
	if ( GGadgetIsChecked(g))
	    kcd->new->flags |= bit;
	else
	    kcd->new->flags &= ~bit;
    }
return( true );
}

static void rebuildclass(int new,int *_old,char ***_names) {
    int old = *_old;
    char **oldnames = *_names;
    char **newnames = galloc(new*sizeof(char *));
    int i;

    for ( i=0; i<new && i<old ; ++i )
	newnames[i] = oldnames[i];
    for ( ; i<old; ++i )
	free( oldnames[i]);
    for ( ; i<new; ++i )
	newnames[i] = copy("");
    *_old = new;
    *_names = newnames;
    free( oldnames );
}

static int KC_ClassCnt(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;
    int err, isfirst, i,j, val;
    int16 *offsets;
    KernClass *kc;

    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	isfirst = GGadgetGetCid(g)==CID_ClassCnt1;
	err = false;
	val = GetIntR(kcd->gw,isfirst?CID_ClassCnt1:CID_ClassCnt2, _STR_ClassCnt,&err)+1;
	if ( err )
return( true );
	if ( val>1001 || val<=0 ) {
	    GWidgetErrorR(_STR_OutOfRange,_STR_OutOfRange);
return( true );
	}
	kc = kcd->new;
	if ( isfirst && kc->first_cnt!=val ) {
	    offsets = gcalloc(val*kc->second_cnt,sizeof(int16));
	    for ( i=0; i<val && i<kc->first_cnt; ++i )
		memcpy(offsets+(i*kc->second_cnt),kc->offsets+(i*kc->second_cnt),kc->second_cnt*sizeof(int16));
	    free( kc->offsets ); kc->offsets = offsets;
	    rebuildclass(val,&kc->first_cnt,&kc->firsts);
	} else if ( !isfirst && kc->second_cnt!=val ) {
	    offsets = gcalloc(val*kc->first_cnt,sizeof(int16));
	    for ( i=0; i<kc->first_cnt; ++i ) for ( j=0; j<val && j<kc->second_cnt; ++j )
		offsets[i*val+j] = kc->offsets[i*kc->second_cnt+j];
	    free( kc->offsets ); kc->offsets = offsets;
	    rebuildclass(val,&kc->second_cnt,&kc->seconds);
	}
    }
return( true );
}

static void KC_SetMV(KernClassDlg *kcd,int class1_index,int class2_index) {
    char *pt, *end, ch;
    SplineChar *sc1=NULL, *sc2 = NULL, *scs[3];
    SplineFont *sf = kcd->kcld->sf;

    for ( pt = kcd->new->firsts[class1_index]; *pt; pt = end+1 ) {
	end = strchr(pt,' ');
	if ( end==NULL )
	    end = pt+strlen(pt);
	ch = *end;
	*end = '\0';
	sc1 = SFGetCharDup(sf,-1,pt);
	*end = ch;
	if ( sc1!=NULL )
    break;
	if ( ch=='\0' )		/* Couldn't find any characters in the first set */
return;
    }
    for ( pt = kcd->new->seconds[class2_index]; *pt; pt = end+1 ) {
	end = strchr(pt,' ');
	if ( end==NULL )
	    end = pt+strlen(pt);
	ch = *end;
	*end = '\0';
	sc2 = SFGetCharDup(sf,-1,pt);
	*end = ch;
	if ( sc2!=NULL )
    break;
	if ( ch=='\0' )		/* Couldn't find any characters in the second set */
return;
    }
    if ( sc1!=NULL && sc2!=NULL && (sc1!=kcd->last1 || sc2!=kcd->last2) &&
	    !kcd->kcld->isv ) {
	if ( kcd->kcld->mv==NULL )
	    kcd->kcld->mv = MetricsViewCreate(kcd->kcld->sf->fv,sc1,NULL);
	else {
	    GDrawSetVisible(kcd->kcld->mv->gw,true);
	    GDrawRaise(kcd->kcld->mv->gw);
	}
	scs[0] = sc1; scs[1] = sc2; scs[2] = NULL;
	MVSetSCs(kcd->kcld->mv,scs);
	kcd->last1 = sc1; kcd->last2 = sc2;
    }
}

static int KC_ClassIndex(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;
    const unichar_t *ret; unichar_t *end1, *end2;
    int val1, val2;
    int en1, en2;
    char buf[12];
    unichar_t ubuf[40];

    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcd->beingbuilt )
return( true );
	ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_ClassIndex1));
	val1 = u_strtol(ret,&end1,10);
	en1 = ( *end1=='\0' && val1>=0 && val1<=kcd->new->first_cnt );
	GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_SelectClass1),en1);
	if ( val1==0 ) en1 = false;
	GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_SetClass1),en1);
	if ( en1 ) {
	    uc_strncpy(ubuf,kcd->new->firsts[val1],30);
	    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_Label1),ubuf);
	}
	ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_ClassIndex2));
	val2 = u_strtol(ret,&end2,10);
	en2 = ( *end2=='\0' && val2>=0 && val2<=kcd->new->second_cnt );
	GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_SelectClass2),en2);
	if ( val2==0 ) en2 = false;
	GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_SetClass2),en2);
	GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_KernOffset),en1 && en2);
	if ( en2 ) {
	    uc_strncpy(ubuf,kcd->new->seconds[val2],30);
	    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_Label2),ubuf);
	}
	if ( en1 && en2 ) {
	    KC_SetMV(kcd,val1,val2);
	    sprintf(buf,"%d",kcd->new->offsets[val1*kcd->new->second_cnt+val2]);
	    uc_strcpy(ubuf,buf);
	    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset),ubuf);
	}
    }
return( true );
}

static int KC_KernOffset(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;
    const unichar_t *ret; unichar_t *end1, *end2;
    int val1, val2, val3, err=false;
    int en1, en2;

    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcd->beingbuilt )
return( true );
	ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_ClassIndex1));
	val1 = u_strtol(ret,&end1,10);
	en1 = ( *end1=='\0' && val1>0 && val1<=kcd->new->first_cnt );
	ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_ClassIndex2));
	val2 = u_strtol(ret,&end2,10);
	en2 = ( *end2=='\0' && val2>0 && val2<=kcd->new->second_cnt );
	val3 = GetIntR(kcd->gw,CID_KernOffset,_STR_KernOffset,&err);
	if ( en1 && en2 && !err) {
	    if ( kcd->new->offsets[val1*kcd->new->second_cnt+val2]!=val3 ) {
		kcd->new->offsets[val1*kcd->new->second_cnt+val2]=val3;
		kcd->kcld->sf->changed = true;
		if ( kcd->kcld->mv )
		    MVRefreshAll(kcd->kcld->mv);
	    }
	}
    }
return( true );
}

static void SelectNames(FontView *fv,char *namelist) {
    char *pt, *end, ch;
    SplineFont *sf = fv->sf;
    SplineChar *sc;

    for ( pt = namelist; *pt; pt = end+1 ) {
	end = strchr(pt,' ');
	if ( end==NULL )
	    end = pt+strlen(pt);
	ch = *end;
	*end = '\0';
	sc = SFGetCharDup(sf,-1,pt);
	if ( sc!=NULL )
	    sf->fv->selected[sc->enc] = true;
	*end = ch;
	if ( ch=='\0' )
    break;
    }
}

static int KC_Select(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;
    int issecond;
    const unichar_t *ret; unichar_t *end;
    int val, top, found = -1, i;
    char **classnames;
    SplineFont *sf;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	sf = kcd->kcld->sf;
	GDrawRaise(sf->fv->gw);
	GDrawSetVisible(sf->fv->gw,true);
	issecond = GGadgetGetCid(g)==CID_SelectClass2;
	ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_ClassIndex1+issecond));
	val = u_strtol(ret,&end,10);
	if ( issecond ) {
	    top = kcd->new->second_cnt;
	    classnames = kcd->new->seconds;
	} else {
	    top = kcd->new->first_cnt;
	    classnames = kcd->new->firsts;
	}
	if ( *end=='\0' && val>=0 && val<=top ) {
	    memset(sf->fv->selected,0,sf->charcnt);
	    if ( val!=0 )
		SelectNames(sf->fv,classnames[val]);
	    else {
		for ( val=1; val<top; ++val )
		    SelectNames(sf->fv,classnames[val]);
		for ( i=0; i<sf->charcnt; ++i ) {
		    if ( sf->fv->selected[i] )
			sf->fv->selected[i] = false;
		    else if ( sf->chars[i]!=NULL )
			sf->fv->selected[i] = true;
		}
	    }
	    for ( found=0; found<sf->charcnt; ++found )
		if ( sf->fv->selected[found] )
	    break;
	    if ( found!=sf->charcnt )
		FVScrollToChar(sf->fv,found);
	    GDrawRequestExpose(sf->fv->v,NULL,false);
	}
    }
return( true );
}

static int ClassesFindName(char **classnames,char *name,int top,int dontmatch) {
    int i;
    char *pt, *end, ch;

    for ( i=1; i<top; ++i ) if ( i!=dontmatch ) {
	for ( pt = classnames[i]; *pt; pt = end+1 ) {
	    end = strchr(pt,' ');
	    if ( end==NULL )
		end = pt+strlen(pt);
	    ch = *end;
	    *end = '\0';
	    if ( strcmp(pt,name)==0 ) {
		*end=ch;
return( i );
	    }
	    *end = ch;
	    if ( ch=='\0' )
	break;
	}
    }
return( 0 );
}

static void ClassesRemoveName(char **classnames,char *name,int top,int fromhere) {
    char *pt, *end, ch;
    int cmp;

    if ( fromhere==-1 )
	fromhere = ClassesFindName(classnames,name,top,-1);
    if ( fromhere==-1 )
return;
    for ( pt = classnames[fromhere]; *pt; pt = end+1 ) {
	end = strchr(pt,' ');
	if ( end==NULL )
	    end = pt+strlen(pt);
	ch = *end;
	*end = '\0';
	cmp = strcmp(pt,name);
	*end = ch;
	if ( cmp==0 ) {
	    char *new = galloc(strlen(classnames[fromhere])-strlen(name)+1);
	    int pos = pt-classnames[fromhere];
	    if ( pos!=0 ) {
		strncpy(new,classnames[fromhere],pos-1);
		strcpy(new+pos-1,end);
	    } else if ( ch!='\0' )
		strcpy(new,end+1);
	    else
		*new = '\0';
	    free( classnames[fromhere] );
	    classnames[fromhere] = new;
return;
	}
	if ( ch=='\0' )
return;
    }
}

static int KC_Set(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;
    int issecond;
    const unichar_t *ret; unichar_t *end;
    int val, i, j, top, pos;
    char **classnames;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	issecond = GGadgetGetCid(g)==CID_SetClass2;
	ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_ClassIndex1+issecond));
	val = u_strtol(ret,&end,10);
	if ( issecond ) {
	    top = kcd->new->second_cnt;
	    classnames = kcd->new->seconds;
	} else {
	    top = kcd->new->first_cnt;
	    classnames = kcd->new->firsts;
	}
	if ( *end=='\0' && val>0 && val<=top ) {
	    SplineFont *sf = kcd->kcld->sf;
	    FontView *fv = sf->fv;
	    char *names;
	    int len;
	    for ( j=0; j<2 ; ++j ) {
		for ( i=len=0; i<sf->charcnt; ++i ) if ( fv->selected[i] ) {
		    SFMakeChar(sf,i);
		    if ( j==0 && (pos = ClassesFindName(classnames,sf->chars[i]->name,top,val))!=0 ) {
			static int buts[] = { _STR_FromThis, _STR_FromOld, _STR_Cancel, 0 };
			int ans = GWidgetAskR(_STR_AlreadyUsed,buts,0,2,_STR_AlreadyInClass, sf->chars[i]->name);
			if ( ans==2 )	/* Cancel */
return( true );
			else if ( ans==0 ) {
			    fv->selected[i]=false;
			    GDrawRequestExpose(fv->v,NULL,false);
		continue;
			} else
			    ClassesRemoveName(classnames,sf->chars[i]->name,top,pos);
		    }
		    if ( j ) {
			strcpy(names+len,sf->chars[i]->name);
			strcat(names+len," ");
		    }
		    len += strlen(sf->chars[i]->name)+1;
		}
		if ( j )
		    names[len-1] = '\0';
		else
		    names = galloc(len+2);
	    }
	    free(classnames[val]);
	    classnames[val] = names;
	    sf->changed = true;
	}
    }
return( true );
}

static int KC_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GDrawDestroyWindow(GGadgetGetWindow(g));
    }
return( true );
}

static void KC_DoCancel(KernClassDlg *kcd) {
    KernClass temp;
    KernClassListDlg *kcld = kcd->kcld;

    if ( kcd->orig==NULL ) {
	if ( kcld->isv )
	    kcld->sf->vkerns = kcd->new->next;
	else
	    kcld->sf->kerns = kcd->new->next;
	kcd->new->next = NULL;
	kcd->orig = kcd->new;
	kcd->new = NULL;
    } else {
	temp = *kcd->orig;
	*kcd->orig = *kcd->new;
	*kcd->new = temp;
	kcd->new->next = kcd->orig->next;
	kcd->orig->next = NULL;
    }
    if ( kcd->kcld->mv!=NULL )
	MVRefreshAll(kcd->kcld->mv);
    GDrawDestroyWindow(kcd->gw);
}

static int KC_Cancel(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	KC_DoCancel(kcd);
    }
return( true );
}

static int kcd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	KernClassDlg *kcd = GDrawGetUserData(gw);
	KC_DoCancel(kcd);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("metricsview.html#kernclass");
return( true );
	}
return( false );
    } else if ( event->type == et_destroy ) {
	KernClassDlg *kcd = GDrawGetUserData(gw);
	kcd->kcld->kcd = NULL;
	GGadgetSetList(GWidgetGetControl(kcd->kcld->gw,CID_List),
		KCSLIArray(kcd->kcld->sf,kcd->kcld->isv),false);
	GGadgetSetEnabled(GWidgetGetControl(kcd->kcld->gw,CID_New),true);
	KernClassListFree(kcd->orig);
	free(kcd);
    }
return( true );
}

static void KernClassD(KernClassListDlg *kcld,KernClass *kc) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[30];
    GTextInfo label[30];
    KernClassDlg *kcd;
    int i,j, kc_width;
    char buf[12];
    unichar_t cc1[12], cc2[12];

    if ( kcld->kcd!=NULL ) {
	GDrawSetVisible(kcld->kcd->gw,true);
	GDrawRaise(kcld->kcd->gw);
return;
    }
    kcd = gcalloc(1,sizeof(KernClassDlg));
    kcd->kcld = kcld;
    kcld->kcd = kcd;
    kcd->orig = KernClassCopy(kc);
    if ( kc==NULL ) {
	kc = chunkalloc(sizeof(KernClass));
	kc->first_cnt = kc->second_cnt = 1;
	kc->firsts = gcalloc(1,sizeof(char *));
	kc->seconds = gcalloc(1,sizeof(char *));
	kc->offsets = gcalloc(1,sizeof(int16));
	if ( kcld->isv ) {
	    kc->next = kcld->sf->vkerns;
	    kcld->sf->vkerns = kc;
	} else {
	    kc->next = kcld->sf->kerns;
	    kcld->sf->kerns = kc;
	}
	kcld->sf->changed = true;
    }
    kcd->new = kc;
    kcd->beingbuilt = true;

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource( _STR_KernClass,NULL );
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,KC_Wid+10))+
	    GDrawPointsToPixels(NULL,2*GIntGetResource(_NUM_Buttonsize));
    pos.height = GDrawPointsToPixels(NULL,KC_Height);
    kcd->gw = GDrawCreateTopWindow(NULL,&pos,kcd_e_h,kcd,&wattrs);

    kc_width = GDrawPixelsToPoints(NULL,pos.width*100/GGadgetScale(100));

    i = 0;
    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 5;
    gcd[i].gd.pos.width = kc_width-20;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.u.list = SFLangList(kcld->sf,true,(SplineChar *) -1);
    for ( j=0; gcd[i].gd.u.list[j].text!=NULL; ++j )
	gcd[i].gd.u.list[j].selected = false;
    gcd[i].gd.u.list[kc->sli].selected = true;
    gcd[i].gd.cid = CID_SLI;
    gcd[i].gd.handle_controlevent = KC_Sli;
    gcd[i++].creator = GListButtonCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+28;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc->flags&pst_r2l?gg_cb_on:0);
    label[i].text = (unichar_t *) _STR_RightToLeft;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_R2L;
    gcd[i].gd.handle_controlevent = KC_Flag;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc->flags&pst_ignorebaseglyphs?gg_cb_on:0);
    label[i].text = (unichar_t *) _STR_IgnoreBaseGlyphs;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_IgnBase;
    gcd[i].gd.handle_controlevent = KC_Flag;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc->flags&pst_ignoreligatures?gg_cb_on:0);
    label[i].text = (unichar_t *) _STR_IgnoreLigatures;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_IgnLig;
    gcd[i].gd.handle_controlevent = KC_Flag;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc->flags&pst_ignorecombiningmarks?gg_cb_on:0);
    label[i].text = (unichar_t *) _STR_IgnoreCombiningMarks;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_IgnMark;
    gcd[i].gd.handle_controlevent = KC_Flag;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = KC_Wid-15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+17;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    label[i].text = (unichar_t *) _STR_FirstChar;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = KC_Wid-5+GIntGetResource(_NUM_Buttonsize); gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    label[i].text = (unichar_t *) _STR_SecondChar;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = gcd[i-1].gd.pos.x-5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.height = KC_Height-40-gcd[i].gd.pos.y;
    gcd[i].gd.flags = /*gg_visible | gg_enabled |*/ gg_line_vert;
    gcd[i++].creator = GLineCreate;

    gcd[i].gd.pos.x = gcd[i-4].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _STR_ClassCnt;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i++].creator = GLabelCreate;

    sprintf( buf,"%d", kc->first_cnt-1); uc_strcpy(cc1,buf);
    gcd[i].gd.pos.x = gcd[i-4].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-3;
    gcd[i].gd.pos.width = GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = cc1;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_ClassCnt1;
    gcd[i].gd.handle_controlevent = KC_ClassCnt;
    gcd[i++].creator = GTextFieldCreate;

    sprintf( buf,"%d", kc->second_cnt-1); uc_strcpy(cc2,buf);
    gcd[i].gd.pos.x = gcd[i-4].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.width = gcd[i-1].gd.pos.width;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = cc2;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_ClassCnt2;
    gcd[i].gd.handle_controlevent = KC_ClassCnt;
    gcd[i++].creator = GTextFieldCreate;

    gcd[i].gd.pos.x = gcd[i-3].gd.pos.x+5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26;
    gcd[i].gd.pos.width = kc_width-gcd[i-3].gd.pos.x-10;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i++].creator = GLineCreate;

    gcd[i].gd.pos.x = gcd[i-4].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+7;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _STR_ClassIndex;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = gcd[i-4].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-3;
    gcd[i].gd.pos.width = gcd[i-4].gd.pos.width;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) "0";
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_ClassIndex1;
    gcd[i].gd.handle_controlevent = KC_ClassIndex;
    gcd[i++].creator = GTextFieldCreate;

    gcd[i].gd.pos.x = gcd[i-4].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.width = gcd[i-4].gd.pos.width;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) "0";
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_ClassIndex2;
    gcd[i].gd.handle_controlevent = KC_ClassIndex;
    gcd[i++].creator = GTextFieldCreate;

    gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26;
    gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.cid = CID_Label1;
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.cid = CID_Label2;
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = gcd[i-5].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _STR_KernOffset;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = (gcd[i-5].gd.pos.x+gcd[i-2].gd.pos.x)/2; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-3;
    gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
    gcd[i].gd.flags = gg_visible;
    gcd[i].gd.cid = CID_KernOffset;
    gcd[i].gd.handle_controlevent = KC_KernOffset;
    gcd[i++].creator = GTextFieldCreate;

    gcd[i].gd.pos.x = gcd[i-7].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+27;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _STR_SelectClass;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = gcd[i-7].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _STR_Select_nom;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_SelectClass1;
    gcd[i].gd.popup_msg = GStringGetResource(_STR_SelectFromClassPopup,NULL);
    gcd[i].gd.handle_controlevent = KC_Select;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = gcd[i-7].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _STR_Select_nom;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_SelectClass2;
    gcd[i].gd.popup_msg = GStringGetResource(_STR_SelectFromClassPopup,NULL);
    gcd[i].gd.handle_controlevent = KC_Select;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = gcd[i-3].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+30;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _STR_FromSelection;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = gcd[i-3].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible ;
    label[i].text = (unichar_t *) _STR_Set;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_SetClass1;
    gcd[i].gd.popup_msg = GStringGetResource(_STR_SetFromSelectionPopup,NULL);
    gcd[i].gd.handle_controlevent = KC_Set;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = gcd[i-3].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible ;
    label[i].text = (unichar_t *) _STR_Set;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_SetClass2;
    gcd[i].gd.popup_msg = GStringGetResource(_STR_SetFromSelectionPopup,NULL);
    gcd[i].gd.handle_controlevent = KC_Set;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26;
    gcd[i].gd.pos.width = kc_width-20;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i++].creator = GLineCreate;

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+6;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _STR_OK;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = KC_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = KC_Cancel;
    gcd[i++].creator = GButtonCreate;

    GGadgetsCreate(kcd->gw,gcd);
    GDrawSetVisible(kcd->gw,true);
    kcd->beingbuilt = false;

    GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Delete),false);
    GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Edit),false);
    GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_New),false);
}

static int KCL_New(GGadget *g, GEvent *e) {
    KernClassListDlg *kcld;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcld->kcd==NULL )
	    KernClassD(kcld,NULL);
    }
return( true );
}

static int KCL_Delete(GGadget *g, GEvent *e) {
    int len, i,j;
    GTextInfo **old, **new;
    GGadget *list;
    KernClassListDlg *kcld;
    KernClass *p, *kc, *n;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcld->kcd==NULL ) {
	    list = GWidgetGetControl(kcld->gw,CID_List);
	    old = GGadgetGetList(list,&len);
	    new = gcalloc(len+1,sizeof(GTextInfo *));
	    p = NULL; kc = kcld->isv ? kcld->sf->vkerns : kcld->sf->kerns;
	    for ( i=j=0; i<len; ++i, kc = n ) {
		n = kc->next;
		if ( !old[i]->selected ) {
		    new[j] = galloc(sizeof(GTextInfo));
		    *new[j] = *old[i];
		    new[j]->text = u_copy(new[j]->text);
		    ++j;
		    p = kc;
		} else {
		    if ( p!=NULL )
			p->next = n;
		    else if ( kcld->isv )
			kcld->sf->vkerns = n;
		    else
			kcld->sf->kerns = n;
		    kc->next = NULL;
		    KernClassListFree(kc);
		}
	    }
	    new[j] = gcalloc(1,sizeof(GTextInfo));
	    GGadgetSetList(list,new,false);
	}
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Delete),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Edit),false);
    }
return( true );
}

static int KCL_Edit(GGadget *g, GEvent *e) {
    int sel, i;
    KernClassListDlg *kcld;
    KernClass *kc;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	sel = GGadgetGetFirstListSelectedItem(GWidgetGetControl(GGadgetGetWindow(g),CID_List));
	if ( sel==-1 || kcld->kcd!=NULL )
return( true );
	for ( kc=kcld->isv?kcld->sf->vkerns:kcld->sf->kerns, i=0; i<sel; kc=kc->next, ++i );
	KernClassD(kcld,kc);
    }
return( true );
}

static int KCL_Done(GGadget *g, GEvent *e) {
    KernClassListDlg *kcld;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcld->kcd != NULL )
	    KC_DoCancel(kcld->kcd);
	GDrawDestroyWindow(kcld->gw);
    }
return( true );
}

static int KCL_SelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	KernClassListDlg *kcld = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Delete),sel!=-1 && kcld->kcd==NULL);
	GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Edit),sel!=-1 && kcld->kcd==NULL);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	KernClassListDlg *kcld = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcld->kcd==NULL ) {
	    e->u.control.subtype = et_buttonactivate;
	    e->u.control.g = GWidgetGetControl(kcld->gw,CID_Edit);
	    KCL_Edit(e->u.control.g,e);
	} else {
	    GDrawSetVisible(kcld->kcd->gw,true);
	    GDrawRaise(kcld->kcd->gw);
	}
    }
return( true );
}

static int kcl_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	KernClassListDlg *kcld = GDrawGetUserData(gw);
	if ( kcld->kcd != NULL )
	    KC_DoCancel(kcld->kcd);
	GDrawDestroyWindow(kcld->gw);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("metricsview.html#kernclass");
return( true );
	}
return( false );
    } else if ( event->type == et_destroy ) {
	KernClassListDlg *kcld = GDrawGetUserData(gw);
	if ( kcld->isv )
	    kcld->sf->vkcld = NULL;
	else
	    kcld->sf->kcld = NULL;
	free(kcld);
    }
return( true );
}

void ShowKernClasses(SplineFont *sf,MetricsView *mv,int isv) {
    KernClassListDlg *kcld;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7];
    GTextInfo label[7];

    if ( sf->kcld && !isv ) {
	GDrawSetVisible(sf->kcld->gw,true);
	GDrawRaise(sf->kcld->gw);
return;
    } else if ( sf->vkcld && isv ) {
	GDrawSetVisible(sf->vkcld->gw,true);
	GDrawRaise(sf->vkcld->gw);
return;
    }

    kcld = gcalloc(1,sizeof(KernClassListDlg));
    kcld->sf = sf;
    kcld->mv = mv;
    kcld->isv = isv;
    if ( isv )
	sf->vkcld = kcld;
    else
	sf->kcld = kcld;

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource( isv?_STR_VKernByClasses:_STR_KernByClasses,NULL );
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,KCL_Width));
    pos.height = GDrawPointsToPixels(NULL,KCL_Height);
    kcld->gw = GDrawCreateTopWindow(NULL,&pos,kcl_e_h,kcld,&wattrs);

    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
    gcd[0].gd.pos.width = KCL_Width-28; gcd[0].gd.pos.height = 7*12+10;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    gcd[0].gd.cid = CID_List;
    gcd[0].gd.u.list = KCSLIList(sf,isv);
    gcd[0].gd.handle_controlevent = KCL_SelChanged;
    gcd[0].creator = GListCreate;

    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = gcd[0].gd.pos.y+gcd[0].gd.pos.height+4;
    gcd[1].gd.pos.width = -1;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    label[1].text = (unichar_t *) _STR_NewDDD;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.cid = CID_New;
    gcd[1].gd.handle_controlevent = KCL_New;
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); gcd[2].gd.pos.y = gcd[1].gd.pos.y;
    gcd[2].gd.pos.width = -1;
    gcd[2].gd.flags = gg_visible;
    label[2].text = (unichar_t *) _STR_Delete;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.cid = CID_Delete;
    gcd[2].gd.handle_controlevent = KCL_Delete;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = -10; gcd[3].gd.pos.y = gcd[1].gd.pos.y;
    gcd[3].gd.pos.width = -1;
    gcd[3].gd.flags = gg_visible;
    label[3].text = (unichar_t *) _STR_EditDDD;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.cid = CID_Edit;
    gcd[3].gd.handle_controlevent = KCL_Edit;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 10; gcd[4].gd.pos.y = gcd[1].gd.pos.y+28;
    gcd[4].gd.pos.width = KCL_Width-20;
    gcd[4].gd.flags = gg_visible;
    gcd[4].creator = GLineCreate;

    gcd[5].gd.pos.x = (KCL_Width-GIntGetResource(_NUM_Buttonsize))/2; gcd[5].gd.pos.y = gcd[4].gd.pos.y+7;
    gcd[5].gd.pos.width = -1;
    gcd[5].gd.flags = gg_visible|gg_enabled|gg_but_default|gg_but_cancel;
    label[5].text = (unichar_t *) _STR_Done;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = KCL_Done;
    gcd[5].creator = GButtonCreate;

    GGadgetsCreate(kcld->gw,gcd);
    GDrawSetVisible(kcld->gw,true);
}

void KCLD_End(KernClassListDlg *kcld) {
    if ( kcld==NULL )
return;
    if ( kcld->kcd != NULL )
	KC_DoCancel(kcld->kcd);
    GDrawDestroyWindow(kcld->gw);
}

void KCLD_MvDetach(KernClassListDlg *kcld,MetricsView *mv) {
    if ( kcld==NULL )
return;
    if ( kcld->mv == mv )
	kcld->mv = NULL;
}

int KernClassContains(KernClass *kc, char *name1, char *name2, int ordered ) {
    int infirst=0, insecond=0, scpos1, kwpos1, scpos2, kwpos2;
    int i;

    for ( i=1; i<kc->first_cnt; ++i ) {
	if ( PSTContains(kc->firsts[i],name1) ) {
	    scpos1 = i;
	    if ( ++infirst>=3 )		/* The name occurs twice??? */
    break;
	} else if ( PSTContains(kc->firsts[i],name2) ) {
	    kwpos1 = i;
	    if ( (infirst+=2)>=3 )
    break;
	}
    }
    if ( infirst==0 || infirst>3 )
return( 0 );
    for ( i=1; i<kc->second_cnt; ++i ) {
	if ( PSTContains(kc->seconds[i],name1) ) {
	    scpos2 = i;
	    if ( ++insecond>=3 )
    break;
	} else if ( PSTContains(kc->seconds[i],name2) ) {
	    kwpos2 = i;
	    if ( (insecond+=2)>=3 )
    break;
	}
    }
    if ( insecond==0 || insecond>3 )
return( 0 );
    if ( (infirst&1) && (insecond&2) ) {
	if ( kc->offsets[scpos1*kc->second_cnt+kwpos2]!=0 )
return( kc->offsets[scpos1*kc->second_cnt+kwpos2] );
    }
    if ( !ordered ) {
	if ( (infirst&2) && (insecond&1) ) {
	    if ( kc->offsets[kwpos1*kc->second_cnt+scpos2]!=0 )
return( kc->offsets[kwpos1*kc->second_cnt+scpos2] );
	}
    }
return( 0 );
}
