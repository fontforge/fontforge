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
#include <ustring.h>
#include <utype.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include "gfile.h"
#include "gresource.h"

static int enc_num = em_base;

static unichar_t tex_base_encoding[] = {
    0x0000, 0x02d9, 0xfb01, 0xfb02, 0x2044, 0x02dd, 0x0141, 0x0142,
    0x02db, 0x02da, 0x000a, 0x02d8, 0x2212, 0x000d, 0x017d, 0x017e,
    0x02c7, 0x0131, 0xf6be, 0xfb00, 0xfb03, 0xfb04, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x0060, 0x0027,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x2019,
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    0x2018, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
    0x0080, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x008e, 0x008f,
    0x0090, 0x0091, 0x0092, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x009e, 0x0178,
    0x0000, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x002d, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};
static Encoding texbase = { em_base, "TeX-Base-Encoding", 256, tex_base_encoding, NULL, NULL, 1 };
Encoding *enclist = &texbase;

static char *getPfaEditEncodings(void) {
    static char *encfile=NULL;
    char buffer[1025];

    if ( encfile!=NULL )
return( encfile );
    if ( getPfaEditDir(buffer)==NULL )
return( NULL );
    sprintf(buffer,"%s/Encodings.ps", getPfaEditDir(buffer));
    encfile = copy(buffer);
return( encfile );
}

static void EncodingFree(Encoding *item) {
    int i;

    free(item->enc_name);
    if ( item->psnames!=NULL ) for ( i=0; i<item->char_cnt; ++i )
	free(item->psnames[i]);
    free(item->psnames);
    free(item->unicode);
    free(item);
}

/* Parse a TXT file from the unicode consortium */
    /* Unicode Consortium Format A */
    /* List of lines with several fields, */
	/* first is the encoding value (in hex), second the Unicode value (in hex) */
    /* # is a comment character (to eol) */
static Encoding *ParseConsortiumEncodingFile(FILE *file) {
    char buffer[200];
    unichar_t encs[1024];
    int enc, unienc, max, i;
    Encoding *item;

    for ( i=0; i<1024; ++i )
	encs[i] = 0;
    for ( i=0; i<32; ++i )
	encs[i] = i;
    for ( i=127; i<0xa0; ++i )
	encs[i] = i;
    max = -1;

    while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	if ( buffer[0]=='0' ) {
	    if ( sscanf(buffer, "%x %x", &enc, &unienc)==2 && enc<1024 && enc>=0 ) {
		encs[enc] = unienc;
		if ( enc>max ) max = enc;
	    }
	}
    }

    if ( max==-1 )
return( NULL );

    ++max;
    if ( max<256 ) max = 256;
    item = gcalloc(1,sizeof(Encoding));
    item->char_cnt = max;
    item->unicode = galloc(max*sizeof(unichar_t));
    memcpy(item->unicode,encs,max*sizeof(unichar_t));
return( item );
}

static void DeleteEncoding(Encoding *me);

static void RemoveMultiples(Encoding *item) {
    Encoding *test;

    for ( test=enclist; test!=NULL; test = test->next ) {
	if ( strcmp(test->enc_name,item->enc_name)==0 )
    break;
    }
    if ( test!=NULL )
	DeleteEncoding(test);
}

static void ParseEncodingFile(char *filename) {
    FILE *file;
    char *orig = filename;
    Encoding *head, *item, *prev;
    unichar_t *name;
    char buffer[10]; unichar_t ubuf[100];
    int i,ch;

    if ( filename==NULL ) filename = getPfaEditEncodings();
    file = fopen(filename,"r");
    if ( file==NULL ) {
	if ( orig!=NULL )
	    GDrawError("Couldn't open encoding file: %s", orig);
return;
    }
    ch = getc(file);
    ungetc(ch,file);
    if ( ch=='#' || ch=='0' )
	head = ParseConsortiumEncodingFile(file);
    else
	head = PSSlurpEncodings(file);
    fclose(file);
    if ( head==NULL ) {
	GWidgetErrorR(_STR_BadEncFormat,_STR_BadEncFormat );
return;
    }

    for ( i=0, prev=NULL, item=head; item!=NULL; prev = item, item=item->next, ++i ) {
	item->enc_num = ++enc_num;
	if ( item->enc_name==NULL ) {
	    if ( item==head && item->next==NULL )
		u_strcpy(ubuf,GStringGetResource(_STR_PleaseNameEnc,NULL) );
	    else {
		u_strcpy(ubuf,GStringGetResource(_STR_PleaseNameEncPre,NULL) );
		if ( i==1 )
		    u_strcat(ubuf,GStringGetResource(_STR_First,NULL) );
		else if ( i==2 )
		    u_strcat(ubuf,GStringGetResource(_STR_Second,NULL) );
		else if ( i==3 )
		    u_strcat(ubuf,GStringGetResource(_STR_Third,NULL) );
		else {
		    sprintf(buffer,"%d", i );
		    uc_strcat(ubuf,buffer);
		    u_strcat(ubuf,GStringGetResource(_STR_Th,NULL) );
		}
		u_strcat(ubuf,GStringGetResource(_STR_PleaseNameEncPost,NULL) );
	    }
	    name = GWidgetAskString(ubuf,NULL,ubuf);
	    if ( name!=NULL ) {
		item->enc_name = cu_copy(name);
		free(name);
	    } else {
		if ( prev==NULL )
		    head = item->next;
		else
		    prev->next = item->next;
		EncodingFree(item);
		item = prev;
	    }
	}
    }
    for ( item=head; item!=NULL; item=item->next )
	RemoveMultiples(item);

    if ( enclist == NULL )
	enclist = head;
    else {
	for ( item=enclist; item->next!=NULL; item=item->next );
	item->next = head;
    }
}

void LoadPfaEditEncodings(void) {
    ParseEncodingFile(NULL);
}

static void DumpPfaEditEncodings(void) {
    FILE *file;
    Encoding *item;
    int i;

    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    if ( item==NULL ) {
	unlink(getPfaEditEncodings());
return;
    }

    file = fopen( getPfaEditEncodings(), "w");
    if ( file==NULL ) {
	fprintf(stderr, "couldn't write encodings file\n" );
return;
    }

    for ( item=enclist; item!=NULL; item = item->next ) if ( !item->builtin ) {
	fprintf( file, "/%s [\n", item->enc_name );
	for ( i=0; i<item->char_cnt; ++i ) {
	    if ( item->psnames!=NULL && item->psnames[i]!=NULL )
		fprintf( file, " /%s", item->psnames[i]);
	    else if ( item->unicode[i]<' ' || (item->unicode[i]>=0x7f && item->unicode[i]<0xa0))
		fprintf( file, " /.notdef" );
	    else if ( psunicodenames[item->unicode[i]]!=NULL )
		fprintf( file, " /%s", psunicodenames[item->unicode[i]]);
	    else
		fprintf( file, " /uni%04X", item->unicode[i]);
	    if ( (i&0xf)==0 )
		fprintf( file, "\t\t%% 0x%02x\n", i );
	    else
		putc('\n',file);
	}
	fprintf( file, "] def\n\n" );
    }
    fclose(file);
}

static void DeleteEncoding(Encoding *me) {
    FontView *fv;
    Encoding *prev;
    BDFFont *bdf;

    if ( me->builtin )
return;

    if ( default_encoding == me->enc_num )
	default_encoding = e_iso8859_1;
    for ( fv = fv_list; fv!=NULL; fv = fv->next ) {
	if ( fv->sf->encoding_name==me->enc_num ) {
	    fv->sf->encoding_name = em_none;
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
		bdf->encoding_name = em_none;
	}
    }
    if ( me==enclist )
	enclist = me->next;
    else {
	for ( prev = enclist; prev!=NULL && prev->next!=me; prev=prev->next );
	if ( prev!=NULL ) prev->next = me->next;
    }
    EncodingFree(me);
    DumpPfaEditEncodings();
}

static GTextInfo *EncodingList(void) {
    GTextInfo *ti;
    int i;
    Encoding *item;

    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ++i;
    ti = gcalloc(i+1,sizeof(GTextInfo));
    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ti[i++].text = uc_copy(item->enc_name);
    if ( i!=0 )
	ti[0].selected = true;
return( ti );
}

#define CID_Encodings	1001

static int DE_Delete(GGadget *g, GEvent *e) {
    GWindow gw;
    int *done;
    GGadget *list;
    int sel,i;
    Encoding *item;

    if ( e->type==et_controlevent &&
	    (e->u.control.subtype == et_buttonactivate ||
	     e->u.control.subtype == et_listdoubleclick )) {
	gw = GGadgetGetWindow(g);
	done = GDrawGetUserData(gw);
	list = GWidgetGetControl(gw,CID_Encodings);
	sel = GGadgetGetFirstListSelectedItem(list);
	i=0;
	for ( item=enclist; item!=NULL; item=item->next ) {
	    if ( item->builtin )
		/* Do Nothing */;
	    else if ( i==sel )
	break;
	    else
		++i;
	}
	if ( item!=NULL )
	    DeleteEncoding(item);
	*done = true;
    }
return( true );
}

static int DE_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    int *done;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	done = GDrawGetUserData(gw);
	*done = true;
    }
return( true );
}

static int de_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	int *done = GDrawGetUserData(gw);
	*done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void RemoveEncoding(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[5];
    GTextInfo label[5];
    Encoding *item;
    int done = 0;

    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    if ( item==NULL )
return;

    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_RemoveEncoding,NULL);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,150);
    pos.height = GDrawPointsToPixels(NULL,110);
    gw = GDrawCreateTopWindow(NULL,&pos,de_e_h,&done,&wattrs);

    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 6;
    gcd[0].gd.pos.width = 130; gcd[0].gd.pos.height = 5*12+10;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].gd.cid = CID_Encodings;
    gcd[0].gd.u.list = EncodingList();
    gcd[0].gd.handle_controlevent = DE_Delete;
    gcd[0].creator = GListCreate;

    gcd[2].gd.pos.x = 150-GIntGetResource(_NUM_Buttonsize)-10; gcd[2].gd.pos.y = gcd[0].gd.pos.y+gcd[0].gd.pos.height+5;
    gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel ;
    label[2].text = (unichar_t *) _STR_Cancel;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'C';
    gcd[2].gd.handle_controlevent = DE_Cancel;
    gcd[2].creator = GButtonCreate;

    gcd[1].gd.pos.x = 10-3; gcd[1].gd.pos.y = gcd[2].gd.pos.y-3;
    gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default ;
    label[1].text = (unichar_t *) _STR_Delete;
    label[1].text_in_resource = true;
    gcd[1].gd.mnemonic = 'D';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = DE_Delete;
    gcd[1].creator = GButtonCreate;

    gcd[3].gd.pos.x = 2; gcd[3].gd.pos.y = 2;
    gcd[3].gd.pos.width = pos.width-4; gcd[3].gd.pos.height = pos.height-2;
    gcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[3].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GTextInfoListFree(gcd[0].gd.u.list);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

Encoding *MakeEncoding(SplineFont *sf) {
    unichar_t *name;
    int i;
    Encoding *item, *temp;

    if ( sf->encoding_name!=em_none || sf->charcnt>=1500 )
return(NULL);

    name = GWidgetAskStringR(_STR_PleaseNameEnc,NULL,_STR_PleaseNameEnc);
    if ( name==NULL )
return(NULL);
    item = gcalloc(1,sizeof(Encoding));
    item->enc_num = ++enc_num;
    item->enc_name = cu_copy(name);
    free(name);
    item->char_cnt = sf->charcnt;
    item->unicode = gcalloc(sf->charcnt,sizeof(unichar_t));
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	if ( sf->chars[i]->unicodeenc!=-1 )
	    item->unicode[i] = sf->chars[i]->unicodeenc;
	else if ( strcmp(sf->chars[i]->name,".notdef")!=0 ) {
	    if ( item->psnames==NULL )
		item->psnames = gcalloc(sf->charcnt,sizeof(unichar_t *));
	    item->psnames[i] = copy(sf->chars[i]->name);
	}
    }
    RemoveMultiples(item);

    if ( enclist == NULL )
	enclist = item;
    else {
	for ( temp=enclist; temp->next!=NULL; temp=temp->next );
	temp->next = item;
    }
    DumpPfaEditEncodings();
return( item );
}

void LoadEncodingFile(void) {
    static unichar_t filter[] = { '*','.','{','p','s',',','P','S',',','t','x','t',',','T','X','T','}',  '\0' };
    unichar_t *fn;
    char *filename;

    fn = GWidgetOpenFile(GStringGetResource(_STR_LoadEncoding,NULL), NULL, filter, NULL);
    if ( fn==NULL )
return;
    filename = cu_copy(fn);
    ParseEncodingFile(filename);
    free(fn); free(filename);
    DumpPfaEditEncodings();
}

/* ************************************************************************** */
/* ****************************** CID Encodings ***************************** */
/* ************************************************************************** */
struct cidmap {
    char *registry, *ordering;
    int supplement, maxsupple;
    int cidmax;			/* Max cid found in the charset */
    int namemax;		/* Max cid with useful info */
    unichar_t *unicode;
    char **name;
    struct cidmap *next;
};

static struct cidmap *cidmaps = NULL;

int CID2NameEnc(struct cidmap *map,int cid, char *buffer, int len) {
    int enc = -1;

#ifdef _NO_SNPRINTF
    if ( map==NULL )
	sprintf(buffer,"cid_%d", cid);
    else if ( cid<map->namemax && map->name[cid]!=NULL )
	strncpy(buffer,map->name[cid],len);
    else if ( cid==0 || (cid<map->namemax && map->unicode[cid]!=0 )) {
	enc = map->unicode[cid];
	if ( psunicodenames[enc]!=NULL )
	    strncpy(buffer,psunicodenames[enc],len);
	else
	    sprintf(buffer,"uni%04X", enc);
    } else
	sprintf(buffer,"%s_%d", map->ordering, cid);
#else
    if ( map==NULL )
	snprintf(buffer,len,"cid_%d", cid);
    else if ( cid<map->namemax && map->name[cid]!=NULL )
	strncpy(buffer,map->name[cid],len);
    else if ( cid==0 || (cid<map->namemax && map->unicode[cid]!=0 )) {
	enc = map->unicode[cid];
	if ( psunicodenames[enc]!=NULL )
	    strncpy(buffer,psunicodenames[enc],len);
	else
	    snprintf(buffer,len,"uni%04X", enc);
    } else
	snprintf(buffer,len,"%s_%d", map->ordering, cid);
#endif
return( enc );
}

int NameEnc2CID(struct cidmap *map,int enc, char *name) {
    int i;

    if ( map==NULL )
return( -1 );
    if ( enc!=-1 ) {
	for ( i=0; i<map->namemax; ++i )
	    if ( map->unicode[i]==enc )
return( i );
    } else {
	for ( i=0; i<map->namemax; ++i )
	    if ( map->name[i]!=NULL && strcmp(map->name[i],name)==0 )
return( i );
    }
return( -1 );
}

int MaxCID(struct cidmap *map) {
return( map->cidmax );
}

static char *SearchDirForCidMap(char *dir,char *registry,char *ordering,
	int supplement,char **maybefile) {
    char maybe[FILENAME_MAX+1];
    struct dirent *ent;
    DIR *d;
    int len, rlen = strlen(registry), olen=strlen(ordering);
    char *pt, *end, *ret;
    int test, best = -1;

    if ( dir==NULL )
return( NULL );

    if ( *maybefile!=NULL ) {
	char *pt = strrchr(*maybefile,'.');
	while ( pt>*maybefile && isdigit(pt[-1]))
	    --pt;
	best = strtol(pt,NULL,10);
    }

    d = opendir(dir);
    if ( d==NULL )
return( NULL );
    while ( (ent = readdir(d))!=NULL ) {
	if ( (len = strlen(ent->d_name))<8 )
    continue;
	if ( strcmp(ent->d_name+len-7,".cidmap")!=0 )
    continue;
	if ( strncmp(ent->d_name,registry,rlen)!=0 || ent->d_name[rlen]!='-' )
    continue;
	pt = ent->d_name+rlen+1;
	if ( strncmp(pt,ordering,olen)!=0 || pt[olen]!='-' )
    continue;
	pt += olen+1;
	if ( !isdigit(*pt))
    continue;
	test = strtol(pt,&end,10);
	if ( *end!='.' )
    continue;
	if ( test>=supplement ) {
	    ret = galloc(strlen(dir)+1+len+1);
	    strcpy(ret,dir);
	    strcat(ret,"/");
	    strcat(ret,ent->d_name);
	    closedir(d);
return( ret );
	} else if ( test>best ) {
	    best = test;
	    strcpy(maybe,ent->d_name);
	}
    }
    closedir(d);
    if ( best>-1 ) {
	ret = galloc(strlen(dir)+1+strlen(maybe)+1);
	strcpy(ret,dir);
	strcat(ret,"/");
	strcat(ret,maybe);
	*maybefile = ret;
    }
return( NULL );
}

static struct cidmap *MakeDummyMap(char *registry,char *ordering,int supplement) {
    struct cidmap *ret = galloc(sizeof(struct cidmap));

    ret->registry = copy(registry);
    ret->ordering = copy(ordering);
    ret->supplement = ret->maxsupple = supplement;
    ret->cidmax = ret->namemax = 0;
    ret->unicode = NULL; ret->name = NULL;
    ret->next = cidmaps;
    cidmaps = ret;
return( ret );
}

static struct cidmap *LoadMapFromFile(char *file,char *registry,char *ordering,
	int supplement) {
    struct cidmap *ret = galloc(sizeof(struct cidmap));
    char *pt = strrchr(file,'.');
    FILE *f;
    int cid1, cid2, uni, cnt, i;
    char name[100];

    while ( pt>file && isdigit(pt[-1]))
	--pt;
    ret->supplement = ret->maxsupple = strtol(pt,NULL,10);
    if ( supplement>ret->maxsupple )
	ret->maxsupple = supplement;
    ret->registry = copy(registry);
    ret->ordering = copy(ordering);
    ret->next = cidmaps;
    cidmaps = ret;

    f = fopen( file,"r" );
    if ( f==NULL ) {
	fprintf( stderr, "Couldn't open %s\n", file );
	ret->cidmax = ret->namemax = 0;
	ret->unicode = NULL; ret->name = NULL;
    } else {
	fscanf( f, "%d %d", &ret->cidmax, &ret->namemax );
	ret->unicode = gcalloc(ret->namemax+1,sizeof(unichar_t));
	ret->name = gcalloc(ret->namemax+1,sizeof(char *));
	while ( 1 ) {
	    cnt=fscanf( f, "%d..%d %x", &cid1, &cid2, &uni );
	    if ( cnt<=0 )
	break;
	    if ( cid1>ret->namemax )
	continue;
	    if ( cnt==3 ) {
		if ( cid2>ret->namemax ) cid2 = ret->namemax;
		for ( i=cid1; i<=cid2; ++i )
		    ret->unicode[i] = uni++;
	    } else if ( cnt==1 ) {
		if ( fscanf(f,"%x", &uni )==1 )
		    ret->unicode[cid1] = uni;
		else if ( fscanf(f," /%s", name )==1 )
		    ret->name[cid1] = copy(name);
	    }
	}
	fclose(f);
    }
    free(file);
return( ret );
}

struct cidmap *FindCidMap(char *registry,char *ordering,int supplement) {
    struct cidmap *map, *maybe=NULL;
    char *file, *maybefile=NULL;
    int maybe_sup = -1;
    static int buts[] = { _STR_UseIt, _STR_Search, 0 };
    static int buts2[] = { _STR_UseIt, _STR_GiveUp, 0 };
    unichar_t ubuf[100]; char buf[100];

    for ( map = cidmaps; map!=NULL; map = map->next ) {
	if ( strcmp(map->registry,registry)==0 && strcmp(map->ordering,ordering)==0 ) {
	    if ( supplement<=map->supplement )
return( map );
	    else if ( maybe==NULL || maybe->supplement<map->supplement )
		maybe = map;
	}
    }
    if ( maybe!=NULL && supplement<=maybe->maxsupple )
return( maybe );	/* User has said it's ok to use maybe at this supplement level */

    file = SearchDirForCidMap(".",registry,ordering,supplement,&maybefile);
    if ( file==NULL )
	file = SearchDirForCidMap(GResourceProgramDir,registry,ordering,supplement,&maybefile);
#ifdef SHAREDIR
    if ( file==NULL )
	file = SearchDirForCidMap(SHAREDIR,registry,ordering,supplement,&maybefile);
#endif
    if ( file==NULL )
	file = SearchDirForCidMap(getPfaEditShareDir(),registry,ordering,supplement,&maybefile);
    if ( file==NULL )
	file = SearchDirForCidMap("/usr/share/pfaedit",registry,ordering,supplement,&maybefile);

    if ( file==NULL && (maybe!=NULL || maybefile!=NULL)) {
	if ( maybefile!=NULL ) {
	    char *pt = strrchr(maybefile,'.');
	    while ( pt>maybefile && isdigit(pt[-1]))
		--pt;
	    maybe_sup = strtol(pt,NULL,10);
	    if ( maybe!=NULL && maybe->supplement >= maybe_sup ) {
		free(maybefile); maybefile = NULL;
		maybe_sup = maybe->supplement;
	    } else
		maybe = NULL;
	}
	if ( maybe!=NULL )
	    maybe_sup = maybe->supplement;
	if ( GWidgetAskR(_STR_UseCidMap,buts,0,1,_STR_SearchForCIDMap,
		registry,ordering,supplement,maybe_sup)==0 ) {
	    if ( maybe!=NULL ) {
		maybe->maxsupple = supplement;
return( maybe );
	    } else {
		file = maybefile;
		maybefile = NULL;
	    }
	}
    }

    if ( file==NULL ) {
	unichar_t *uret;
#ifdef _NO_SNPRINTF
	sprintf(buf,"%s-%s-*.cidmap", registry, ordering );
#else
	snprintf(buf,sizeof(buf),"%s-%s-*.cidmap", registry, ordering );
#endif
	uc_strcpy(ubuf,buf);
	uret = GWidgetOpenFile(GStringGetResource(_STR_FindCharset,NULL),NULL,ubuf,NULL);
	if ( uret==NULL ) {
	    if ( GWidgetAskR(_STR_UseCidMap,buts2,0,1,_STR_AreYouSureCharset)==0 ) {
		if ( maybe!=NULL ) {
		    maybe->maxsupple = supplement;
return( maybe );
		} else {
		    file = maybefile;
		    maybefile = NULL;
		}
	    }
	} else {
	    file = u2def_copy(uret);
	    free(uret);
	}
    }

    free(maybefile);
    if ( file!=NULL )
return( LoadMapFromFile(file,registry,ordering,supplement));

return( MakeDummyMap(registry,ordering,supplement));
}

void SFEncodeToMap(SplineFont *sf,struct cidmap *map) {
    SplineChar **chars, *sc;
    int i,max=0;
    RefChar *refs, *rnext, *rprev;
    SplineSet *new, *spl;

    for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL ) {
	sc->enc = NameEnc2CID(map,sc->unicodeenc,sc->name);
	if ( sc->enc>max ) max = sc->enc;
    }

    /* Remove references to characters which aren't in the new map (if any) */
    /* Don't need to fix up dependencies, because we throw the char away */
    for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL ) {
	for ( rprev = NULL, refs=sc->refs; refs!=NULL; refs=rnext ) {
	    rnext = refs->next;
	    if ( refs->sc->enc==-1 ) {
		new = refs->splines;
		if ( new!=NULL ) {
		    for ( spl = new; spl->next!=NULL; spl = spl->next );
		    spl->next = sc->splines;
		    sc->splines = new;
		}
		refs->splines=NULL;
		RefCharFree(refs);
		if ( rprev==NULL )
		    sc->refs = rnext;
		else
		    rprev->next = rnext;
	    } else
		rprev = refs;
	}
    }

    chars = gcalloc(max+1,sizeof(SplineChar *));
    for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL ) {
	if ( sc->enc==-1 )
	    SplineCharFree(sc);
	else
	    chars[sc->enc] = sc;
    }

    free(sf->chars);
    sf->charcnt = max;
    sf->chars = chars;
}

struct block {
    int cur, tot;
    char **maps;
    char **dirs;
};

static void AddToBlock(struct block *block,char *mapname, char *dir) {
    int i, val, j;
    int len = strlen(mapname);

    if ( mapname[len-7]=='.' ) len -= 7;
    for ( i=0; i<block->cur; ++i ) {
	if ( (val=strncmp(block->maps[i],mapname,len))==0 )
return;		/* Duplicate */
	else if ( val>0 )
    break;
    }
    if ( block->tot==0 ) {
	block->tot = 10;
	block->maps = galloc(10*sizeof(char *));
	block->dirs = galloc(10*sizeof(char *));
    } else if ( block->cur>=block->tot ) {
	block->tot += 10;
	block->maps = grealloc(block->maps,block->tot*sizeof(char *));
	block->dirs = grealloc(block->dirs,block->tot*sizeof(char *));
    }
    for ( j=block->cur; j>=i; --j ) {
	block->maps[j+1] = block->maps[j];
	block->dirs[j+1] = block->dirs[j];
    }
    block->maps[i] = copyn(mapname,len);
    block->dirs[i] = dir;
    ++block->cur;
}

static void FindMapsInDir(struct block *block,char *dir) {
    struct dirent *ent;
    DIR *d;
    int len;
    char *pt, *pt2;

    /* format of cidmap filename "?*-?*-[0-9]*.cidmap" */
    d = opendir(dir);
    if ( d==NULL )
return;
    while ( (ent = readdir(d))!=NULL ) {
	if ( (len = strlen(ent->d_name))<8 )
    continue;
	if ( strcmp(ent->d_name+len-7,".cidmap")!=0 )
    continue;
	pt = strchr(ent->d_name, '-');
	if ( pt==NULL || pt==ent->d_name )
    continue;
	pt2 = strchr(pt+1, '-' );
	if ( pt2==NULL || pt2==pt+1 || !isdigit(pt2[1]))
    continue;
	AddToBlock(block,ent->d_name,dir);
    }
}

struct cidmap *AskUserForCIDMap(SplineFont *sf) {
    struct block block;
    struct cidmap *map = NULL;
    char buffer[200];
    const unichar_t **choices;
    int i,ret;
    static unichar_t cidwild[] = { '?','*','-','?','*','-','[','0','-','9',']','*','.','c','i','d','m','a','p',  '\0' };
    char *filename=NULL;
    char *reg, *ord, *pt;
    int supplement;

    memset(&block,'\0',sizeof(block));
    for ( map = cidmaps; map!=NULL; map = map->next ) {
	sprintf(buffer,"%s-%s-%d", map->registry, map->ordering, map->supplement);
	AddToBlock(&block,buffer,NULL);
    }
    FindMapsInDir(&block,".");
    FindMapsInDir(&block,GResourceProgramDir);
#ifdef SHAREDIR
    FindMapsInDir(&block,SHAREDIR);
#endif
    FindMapsInDir(&block,getPfaEditShareDir());
    FindMapsInDir(&block,"/usr/share/pfaedit");

    choices = gcalloc(block.cur+2,sizeof(unichar_t *));
    choices[0] = u_copy(GStringGetResource(_STR_Browse,NULL));
    for ( i=0; i<block.cur; ++i )
	choices[i+1] = uc_copy(block.maps[i]);
    ret = GWidgetChoicesR(_STR_FindCharset,choices,i+1,0,_STR_SelectCIDOrdering);
    for ( i=0; i<=block.cur; ++i )
	free( (unichar_t *) choices[i] );
    free(choices);
    if ( ret==0 ) {
	unichar_t *uret = GWidgetOpenFile(GStringGetResource(_STR_FindCharset,NULL),NULL,cidwild,NULL);
	if ( uret==NULL )
	    ret = -1;
	else {
	    filename = u2def_copy(uret);
	    free(uret);
	}
    }
    if ( ret!=-1 ) {
	if ( filename!=NULL )
	    /* Do nothing for now */;
	else if ( block.dirs[ret-1]!=NULL ) {
	    filename = galloc(strlen(block.dirs[ret-1])+strlen(block.maps[ret-1])+3+8);
	    strcpy(filename,block.dirs[ret-1]);
	    strcat(filename,"/");
	    strcat(filename,block.maps[ret-1]);
	    strcat(filename,".cidmap");
	}
	if ( ret!=0 )
	    reg = block.maps[ret-1];
	else {
	    reg = strrchr(filename,'/');
	    if ( reg==NULL ) reg = filename;
	    else ++reg;
	    reg = copy(reg);
	}
	pt = strchr(reg,'-');
	if ( pt==NULL )
	    ret = -1;
	else {
	    *pt = '\0';
	    ord = pt+1;
	    pt = strchr(ord,'-');
	    if ( pt==NULL )
		ret = -1;
	    else {
		*pt = '\0';
		supplement = strtol(pt+1,NULL,10);
	    }
	}
	if ( ret == -1 )
	    /* No map */;
	else if ( filename==NULL )
	    map = FindCidMap(reg,ord,supplement);
	else
	    map = LoadMapFromFile(filename,reg,ord,supplement);
	if ( reg!=block.maps[ret-1] )
	    free(reg);
	/*free(filename);*/	/* Freed by loadmap */
    }
    for ( i=0; i<block.cur; ++i )
	free( block.maps[i]);
    free(block.maps);
    free(block.dirs);
    if ( map!=NULL ) {
	sf->cidregistry = copy(map->registry);
	sf->ordering = copy(map->ordering);
	sf->supplement = map->supplement;
    }
return( map );
}
