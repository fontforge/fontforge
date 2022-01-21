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

#include "fontP.h"
#include "gdraw.h"
#include "gfile.h"
#include "gresedit.h"
#include "gresource.h"
#include "gresourceP.h"
#include "ustring.h"
#include "utype.h"

#include "assert.h"
#include "math.h"

char *GResourceProgramName;
char *usercharset_names;

static int rcur, rmax=0;
static int rbase = 0, rsummit=0, rskiplen=0;	/* when restricting a search */
struct _GResource_Res *_GResource_Res;
extern GBox _ggadget_Default_Box;
extern GResFont _ggadget_default_font;

static int rcompar(const void *_r1,const void *_r2) {
    const struct _GResource_Res *r1 = _r1, *r2 = _r2;
return( strcmp(r1->res,r2->res));
}

int _GResource_FindResName(const char *name, int do_restrict) {
    int top=do_restrict?rsummit:rcur, bottom = do_restrict?rbase:0;
    int test, cmp;

    if ( rcur==0 )
return( -1 );

    for (;;) {
	if ( top==bottom )
return( -1 );
	test = (top+bottom)/2;
	cmp = strcmp(name,_GResource_Res[test].res+(do_restrict?rskiplen:0));
	if ( cmp==0 )
return( test );
	if ( test==bottom )
return( -1 );
	if ( cmp>0 )
	    bottom=test+1;
	else
	    top = test;
    }
}

static int GResourceRestrict(const char *prefix) {
    int top=rcur, bottom = 0;
    int test, cmp;
    int plen;
    int oldtest, oldtop;
    char *prestr;

    if ( prefix==NULL || *prefix=='\0' ) {
	rbase = rskiplen = 0; rsummit = rcur;
return( rcur==0?-1:0 );
    }
    if ( rcur==0 )
return( -1 );

    prestr = smprintf("%s.", prefix);
    plen = strlen(prestr);

    for (;;) {
	test = (top+bottom)/2;
	cmp = strncmp(prestr,_GResource_Res[test].res,plen);
	if ( cmp==0 )
    break;
	if ( test==bottom ) {
	    free(prestr);
	    return -1;
	}
	if ( cmp>0 ) {
	    bottom=test+1;
	    if ( bottom==top ) {
		free(prestr);
		return -1;
	    }
	} else
	    top = test;
    }
    /* at this point the resource at test begins with the prestr */
    /* we want to find the first and last resources that do */
    oldtop = top; oldtest = top = test;		/* find the first resource */
    for (;;) {
	test = (top+bottom)/2;
	cmp = strncmp(prestr,_GResource_Res[test].res,plen);
	if ( cmp<0 ) {
	    GDrawIError("Resource list out of order");
	    free(prestr);
	    return -1;
	}
	if ( test==bottom ) {
	    if ( cmp!=0 ) ++test;
    break;
	}
	if ( cmp>0 ) {
	    bottom=++test;
	    if ( bottom==top )
    break;
	} else
	    top = test;
    }
    rbase = test;

    top = oldtop; bottom = oldtest+1;		/* find the last resource */
    if ( bottom == top )
	test = top;
    else for (;;) {
	test = (top+bottom)/2;
	cmp = strncmp(prestr,_GResource_Res[test].res,plen);
	if ( cmp>0 ) {
	    GDrawIError("Resource list out of order");
	    free(prestr);
	    return -1;
	}
	if ( test==bottom ) {
	    if ( cmp==0 ) ++test;
    break;
	}
	if ( cmp==0 ) {
	    bottom=++test;
	    if ( bottom==top )
    break;
	} else
	    top = test;
    }
    rsummit = test;
    rskiplen = plen;
    free(prestr);
return( 0 );
}

void GResourceSetProg(const char *prog) {
    const char *pt;

    if ( prog!=NULL ) {
	if ( GResourceProgramName!=NULL && strcmp(prog,GResourceProgramName)==0 )
return;
	free(GResourceProgramName);
	if (( pt=strrchr(prog,'/'))!=NULL )
	    ++pt;
	else
	    pt = prog;
	GResourceProgramName = copy(pt);
    } else if ( GResourceProgramName==NULL )
	GResourceProgramName = copy("gdraw");
    else
return;
}

void GResourceAddResourceString(const char *string,const char *prog) {
    char *ept;
    const char *pt, *next;
    int cnt, plen;
    struct _GResource_Res temp;
    int i,j,k, off;

    GResourceSetProg(prog);
    plen = strlen(GResourceProgramName);

    if ( string==NULL )
return;

    cnt = 0;
    pt = string;
    while ( *pt!='\0' ) {
	next = strchr(pt,'\n');
	if ( next==NULL ) next = pt+strlen(pt);
	else ++next;
	if ( strncmp(pt,"Gdraw.",6)==0 ) ++cnt;
	else if ( strncmp(pt,GResourceProgramName,plen)==0 && pt[plen]=='.' ) ++cnt;
	else if ( strncmp(pt,"*",1)==0 ) ++cnt;
	pt = next;
    }
    if ( cnt==0 )
return;

    if ( rcur+cnt>=rmax ) {
	if ( cnt<10 ) cnt = 10;
	if ( rmax==0 )
	    _GResource_Res = malloc(cnt*sizeof(struct _GResource_Res));
	else
	    _GResource_Res = realloc(_GResource_Res,(rcur+cnt)*sizeof(struct _GResource_Res));
	rmax += cnt;
    }

    pt = string;
    while ( *pt!='\0' ) {
	next = strchr(pt,'\n');
	if ( next==NULL ) next = pt+strlen(pt);
	if ( strncmp(pt,"Gdraw.",6)==0 || strncmp(pt,"*",1)==0 ||
		(strncmp(pt,GResourceProgramName,plen)==0 && pt[plen]=='.' )) {
	    temp.generic = false;
	    if ( strncmp(pt,"Gdraw.",6)==0 ) {
		temp.generic = true;
		off = 6;
	    } else if ( strncmp(pt,"*",1)==0 ) {
		temp.generic = true;
		off = 1;
	    } else
		off = plen+1;
	    ept = strchr(pt+off,':');
	    if ( ept==NULL )
	goto bad;
	    temp.res = copyn(pt+off,ept-(pt+off));
	    pt = ept+1;
	    while ( isspace( *pt ) && pt<next ) ++pt;
	    temp.val = copyn(pt,next-pt);
	    temp.new = true;
	    _GResource_Res[rcur++] = temp;
	}
	bad:
	if ( *next ) ++next;
	pt = next;
    }

    if ( rcur!=0 )
	qsort(_GResource_Res,rcur,sizeof(struct _GResource_Res),rcompar);

    for ( i=j=0; i<rcur; ) {
	if ( i!=j )
	    _GResource_Res[j] = _GResource_Res[i];
	for ( k=i+1; k<rcur && strcmp(_GResource_Res[j].res,_GResource_Res[k].res)==0; ++k ) {
	    if (( !_GResource_Res[k].generic && (_GResource_Res[i].generic || _GResource_Res[i+1].new)) ||
		    (_GResource_Res[k].generic && _GResource_Res[i].generic && _GResource_Res[i+1].new)) {
		free(_GResource_Res[j].res); free(_GResource_Res[j].val);
		_GResource_Res[i].res=NULL;
		_GResource_Res[j] = _GResource_Res[k];
	    } else {
		free(_GResource_Res[k].res); free(_GResource_Res[k].val);
		_GResource_Res[k].res=NULL;
	    }
	}
	i = k; ++j;
    }
    rcur = rsummit = j;
    for ( i=0; i<j; ++i )
	_GResource_Res[i].new = false;
}

void GResourceAddResourceFile(const char *filename,const char *prog,int warn) {
    FILE *file;
    char buffer[1000];

    file = fopen(filename,"r");
    if ( file==NULL ) {
	if ( warn )
	    fprintf( stderr, "Failed to open resource file: %s\n", filename );
return;
    }
    while ( fgets(buffer,sizeof(buffer),file)!=NULL )
	GResourceAddResourceString(buffer,prog);
    fclose(file);
}

static void _GResourceFindFont(const char *resourcename, GResFont *font, int is_value);
static void _GResourceFindImage(const char *fname, GResImage *img);

void GResourceFind( GResStruct *info,const char *prefix) {
    int pos;

    if ( GResourceRestrict(prefix)== -1 ) {
	rbase = rskiplen = 0; rsummit = rcur;
	// return; // Even if no resources are loaded we still need to init
	// the default images and fonts
    }
    while ( info->resname!=NULL ) {
	pos = _GResource_FindResName(info->resname, true);
	info->found = (pos!=-1);
	if ( pos==-1 && info->type!=rt_font && info->type!=rt_image ) // rt_font and rt_image may need default initialization
	    /* Do Nothing */;
	else if ( info->type == rt_string ) {
	    if ( info->cvt!=NULL )
		*(void **) (info->val) = (info->cvt)( _GResource_Res[pos].val, *(void **) (info->val) );
	    else
		*(char **) (info->val) = copy( _GResource_Res[pos].val );
	} else if ( info->type == rt_font ) {
	    _GResourceFindFont(info->found ? _GResource_Res[pos].val : NULL, (GResFont *) info->val, true);
	} else if ( info->type == rt_image ) {
	    _GResourceFindImage(info->found ? _GResource_Res[pos].val : NULL, (GResImage *) info->val);
	} else if ( info->type == rt_color ) {
	    Color temp = _GImage_ColourFName(_GResource_Res[pos].val );
	    if ( temp==-1 ) {
		fprintf( stderr, "Can't convert %s to a Color for resource: %s\n",
			_GResource_Res[pos].val, info->resname );
		info->found = false;
	    } else
		*(Color *) (info->val) = temp;
	} else if ( info->type == rt_int ) {
	    char *end;
	    int val = strtol(_GResource_Res[pos].val,&end,0);
	    if ( *end!='\0' ) {
		fprintf( stderr, "Can't convert %s to an int for resource: %s\n",
			_GResource_Res[pos].val, info->resname );
		info->found = false;
	    } else
		*(int *) (info->val) = val;
	} else if ( info->type == rt_bool ) {
	    int val = -1;
	    if ( strmatch(_GResource_Res[pos].val,"true")==0 ||
		    strmatch(_GResource_Res[pos].val,"on")==0 || strcmp(_GResource_Res[pos].val,"1")==0 )
		val = 1;
	    else if ( strmatch(_GResource_Res[pos].val,"false")==0 ||
		    strmatch(_GResource_Res[pos].val,"off")==0 || strcmp(_GResource_Res[pos].val,"0")==0 )
		val = 0;
	    if ( val==-1 ) {
		fprintf( stderr, "Can't convert %s to a boolean for resource: %s\n",
			_GResource_Res[pos].val, info->resname );
		info->found = false;
	    } else
		*(int *) (info->val) = val;
	} else if ( info->type == rt_double ) {
	    char *end;
	    double val = strtod(_GResource_Res[pos].val,&end);
	    if ( *end=='.' || *end==',' ) {
		*end = (*end==',')?'.':',';
		val = strtod(_GResource_Res[pos].val,&end);
	    }
	    if ( *end!='\0' ) {
		fprintf( stderr, "Can't convert %s to a double for resource: %s\n",
			_GResource_Res[pos].val, info->resname );
		info->found = false;
	    } else
		*(double *) (info->val) = val;
	} else {
	    fprintf( stderr, "Invalid resource type for: %s\n", info->resname );
	    info->found = false;
	}
	++info;
    }
    rbase = rskiplen = 0; rsummit = rcur;
}

char *GResourceFindString(const char *name) {
    int pos;

    pos = _GResource_FindResName(name, false);
    if ( pos==-1 )
return( NULL );
    else
return( copy(_GResource_Res[pos].val));
}

int GResourceFindBool(const char *name, int def) {
    int pos;
    int val = -1;

    pos = _GResource_FindResName(name, false);
    if ( pos==-1 )
return( def );

    if ( strmatch(_GResource_Res[pos].val,"true")==0 ||
	    strmatch(_GResource_Res[pos].val,"on")==0 || strcmp(_GResource_Res[pos].val,"1")==0 )
	val = 1;
    else if ( strmatch(_GResource_Res[pos].val,"false")==0 ||
	    strmatch(_GResource_Res[pos].val,"off")==0 || strcmp(_GResource_Res[pos].val,"0")==0 )
	val = 0;

    if ( val==-1 )
return( def );

return( val );
}

long GResourceFindInt(const char *name, long def) {
    int pos;
    char *end;
    long ret;

    pos = _GResource_FindResName(name, false);
    if ( pos==-1 )
return( def );

    ret = strtol(_GResource_Res[pos].val,&end,10);
    if ( *end!='\0' )
return( def );

return( ret );
}

Color GResourceFindColor(const char *name, Color def) {
    int pos;
    Color ret;

    pos = _GResource_FindResName(name, false);
    if ( pos==-1 )
return( def );

    ret = _GImage_ColourFName(_GResource_Res[pos].val );
    if ( ret==-1 )
return( def );

return( ret );
}

static int match(char **list, char *val) {
    int i;

    for ( i=0; list[i]!=NULL; ++i )
	if ( strmatch(val,list[i])==0 )
return( i );

return( -1 );
}

static void *border_type_cvt(char *val, void *def) {
    static char *types[] = { "none", "box", "raised", "lowered", "engraved",
	    "embossed", "double", NULL };
    int ret = match(types,val);
    if ( ret== -1 )
return( def );
return( (void *) (intptr_t) ret );
}

static void *border_shape_cvt(char *val, void *def) {
    static char *shapes[] = { "rect", "roundrect", "elipse", "diamond", NULL };
    int ret = match(shapes,val);
    if ( ret== -1 )
return( def );
return( (void *) (intptr_t) ret );
}

void _GGadgetCopyDefaultBox(GBox *box) {
    *box = _ggadget_Default_Box;
}

void _GGadgetInitDefaultBox(const char *class, GBox *box) {
    GResStruct boxtypes[] = {
	{ "Box.BorderType", rt_string, NULL, border_type_cvt, 0 },
	{ "Box.BorderShape", rt_string, NULL, border_shape_cvt, 0 },
	{ "Box.BorderWidth", rt_int, NULL, NULL, 0 },
	{ "Box.Padding", rt_int, NULL, NULL, 0 },
	{ "Box.Radius", rt_int, NULL, NULL, 0 },
	{ "Box.BorderInner", rt_bool, NULL, NULL, 0 },
	{ "Box.BorderOuter", rt_bool, NULL, NULL, 0 },
	{ "Box.ActiveInner", rt_bool, NULL, NULL, 0 },
	{ "Box.DoDepressedBackground", rt_bool, NULL, NULL, 0 },
	{ "Box.DrawDefault", rt_bool, NULL, NULL, 0 },
	{ "Box.BorderBrightest", rt_color, NULL, NULL, 0 },
	{ "Box.BorderBrighter", rt_color, NULL, NULL, 0 },
	{ "Box.BorderDarkest", rt_color, NULL, NULL, 0 },
	{ "Box.BorderDarker", rt_color, NULL, NULL, 0 },
	{ "Box.NormalBackground", rt_color, NULL, NULL, 0 },
	{ "Box.NormalForeground", rt_color, NULL, NULL, 0 },
	{ "Box.DisabledBackground", rt_color, NULL, NULL, 0 },
	{ "Box.DisabledForeground", rt_color, NULL, NULL, 0 },
	{ "Box.ActiveBorder", rt_color, NULL, NULL, 0 },
	{ "Box.PressedBackground", rt_color, NULL, NULL, 0 },
	{ "Box.BorderLeft", rt_color, NULL, NULL, 0 },
	{ "Box.BorderTop", rt_color, NULL, NULL, 0 },
	{ "Box.BorderRight", rt_color, NULL, NULL, 0 },
	{ "Box.BorderBottom", rt_color, NULL, NULL, 0 },
	{ "Box.GradientBG", rt_bool, NULL, NULL, 0 },
	{ "Box.GradientStartCol", rt_color, NULL, NULL, 0 },
	{ "Box.ShadowOuter", rt_bool, NULL, NULL, 0 },
	{ "Box.BorderInnerCol", rt_color, NULL, NULL, 0 },
	{ "Box.BorderOuterCol", rt_color, NULL, NULL, 0 },
	GRESSTRUCT_EMPTY
    };
    intptr_t bt, bs;
    int bw, pad, rr, inner, outer, active, depressed, def, grad, shadow;
    char *classstr = NULL;

    bt = box->border_type;
    bs = box->border_shape;
    bw = box->border_width;
    pad = box->padding;
    rr = box->rr_radius;
    inner = box->flags & box_foreground_border_inner;
    outer = box->flags & box_foreground_border_outer;
    active = box->flags & box_active_border_inner;
    depressed = box->flags & box_do_depressed_background;
    def = box->flags & box_draw_default;
    grad = box->flags & box_gradient_bg;
    shadow = box->flags & box_foreground_shadow_outer;

    boxtypes[0].val = &bt;
    boxtypes[1].val = &bs;
    boxtypes[2].val = &bw;
    boxtypes[3].val = &pad;
    boxtypes[4].val = &rr;
    boxtypes[5].val = &inner;
    boxtypes[6].val = &outer;
    boxtypes[7].val = &active;
    boxtypes[8].val = &depressed;
    boxtypes[9].val = &def;
    boxtypes[10].val = &box->border_brightest;
    boxtypes[11].val = &box->border_brighter;
    boxtypes[12].val = &box->border_darkest;
    boxtypes[13].val = &box->border_darker;
    boxtypes[14].val = &box->main_background;
    boxtypes[15].val = &box->main_foreground;
    boxtypes[16].val = &box->disabled_background;
    boxtypes[17].val = &box->disabled_foreground;
    boxtypes[18].val = &box->active_border;
    boxtypes[19].val = &box->depressed_background;
    boxtypes[20].val = &box->border_brightest;
    boxtypes[21].val = &box->border_brighter;
    boxtypes[22].val = &box->border_darkest;
    boxtypes[23].val = &box->border_darker;
    boxtypes[24].val = &grad;
    boxtypes[25].val = &box->gradient_bg_end;
    boxtypes[26].val = &shadow;
    boxtypes[27].val = &box->border_inner;
    boxtypes[28].val = &box->border_outer;

    if ( class!=NULL )
	classstr = smprintf("%s.", class);

    /* for a plain box, default to all borders being the same. they must change*/
    /*  explicitly */
    if ( bt==bt_box || bt==bt_double )
	box->border_brightest = box->border_brighter = box->border_darker = box->border_darkest;
    GResourceFind( boxtypes, class);
    free(classstr);

    box->border_type = bt;
    box->border_shape = bs;
    box->border_width = bw;
    box->padding = pad;
    box->rr_radius = rr;
    box->flags=0;
    if ( inner )
	box->flags |= box_foreground_border_inner;
    if ( outer )
	box->flags |= box_foreground_border_outer;
    if ( active )
	box->flags |= box_active_border_inner;
    if ( depressed )
	box->flags |= box_do_depressed_background;
    if ( def )
	box->flags |= box_draw_default;
    if ( grad )
	box->flags |= box_gradient_bg;
    if ( shadow )
	box->flags |= box_foreground_shadow_outer;
}

/* font name may be something like:
	bold italic extended 12pt courier
	400 10pt small-caps
    family name comes at the end, size must have "pt" or "px" after it
*/
static int _GResToFontRequest(const char *resname, FontRequest *rq, GHashTable *ht,
                              int name_is_value, int strict) {
    static char *styles[] = { "normal", "italic", "oblique", "small-caps",
	    "bold", "light", "extended", "condensed", NULL };
    char *val, *pt, *end, ch, *relname=NULL, *e;
    int ret, top_level = false, percent = 100, adjust = 0;
    int found_rel = false, found_style = false, found_weight = false, err = false;
    FontRequest rel_rq;

    if ( ht==NULL ) {
	ht = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
	top_level = true;
    }

    memset(rq,0,sizeof(*rq));
    memset(&rel_rq,0,sizeof(rel_rq));
    rq->weight = 400;

    if ( name_is_value )
	val = copy(resname);
    else {
	val = GResourceFindString(resname);
    }

    if ( val==NULL ) {
	if ( top_level )
	    GDrawError("Missing font resource reference to '%s': cannot resolve", resname);
	err = true;
    }

    for ( pt=val; !err && *pt && *pt!='"'; ) {
	for ( end=pt; *end!=' ' && *end!='\0'; ++end );
	ch = *end;
	*end = '\0';
	ret = match(styles,pt);
	if ( ret==-1 && isdigit(*pt)) {
	    ret = strtol(pt,&e,10);
	    if ( strmatch(e,"pt")==0 )
		rq->point_size = ret;
	    else if ( strmatch(e,"px")==0 )
		rq->point_size = -ret;
	    else if ( strmatch(e,"%")==0 ) {
		percent = ret;
		if ( percent<0 )
		    percent = -percent;
	    } else if ( *e=='\0' ) {
		found_weight = true;
		rq->weight = ret;
	    } else {
		*end = ch;
		break;
	    }
	} else if ( ret==-1 && (*pt=='+' || *pt=='-') ) {
	    adjust = strtol(pt,&e,10);
	} else if ( ret==-1 && *pt=='^' ) {
	    relname = copy(pt+1);
	    if ( g_hash_table_contains(ht, relname) ) {
		GDrawError("Circular font resource reference to '%s': cannot resolve", relname);
		if ( strict ) {
		    err = true;
		    break;
		} else
		    continue;
	    }
	    g_hash_table_add(ht, copy(relname));
	    if ( !_GResToFontRequest(relname, &rel_rq, ht, false, strict) ) {
		err = true;
		break;
	    }
	    found_rel = true;
	} else if ( ret==-1 ) {
	    *end = ch;
	    break;
	} else if ( ret==0 ) {
	    found_weight = true;
	    rq->weight = 400;
	} else if ( ret==1 || ret==2 ) {
	    found_style = true;
	    rq->style |= fs_italic;
	} else if ( ret==3 ) {
	    found_style = true;
	    rq->style |= fs_smallcaps;
	} else if ( ret==4 ) {
	    found_weight = true;
	    rq->weight = 700;
	} else if ( ret==5 ) {
	    found_weight = true;
	    rq->weight = 300;
	} else if ( ret==6 ) {
	    found_style = true;
	    rq->style |= fs_extended;
	} else {
	    found_style = true;
	    rq->style |= fs_condensed;
	}
	*end = ch;
	pt = end;
	while ( *pt==' ' ) ++pt;
    }

    if ( !err ) {
	if ( *pt!='\0' )
	    rq->utf8_family_name = copy(pt);
	if ( found_rel && rq->utf8_family_name==NULL && rel_rq.utf8_family_name!=NULL )
	    rq->utf8_family_name = copy(rel_rq.utf8_family_name);
	if ( found_rel && !found_weight )
	    rq->weight = rel_rq.weight;
	if ( found_rel && !found_style )
	    rq->style = rel_rq.style;
	if ( rq->point_size==0 ) {
	    if ( found_rel ) {
		if ( rel_rq.point_size==0 ) {
		    GDrawError("Point size not set for '%s': cannot calculate relative point size", relname);
		    if ( strict )
			err = true;
		} else {
		    float tmp = (float)percent * (float)rel_rq.point_size / 100.0;
		    rq->point_size = (int) ((tmp - floor(tmp) > 0.5) ? ceil(tmp) : floor(tmp));
		    if ( rq->point_size < 0 )
			rq->point_size -= adjust;
		    else
			rq->point_size += adjust;
		}
	    } else if ( strict ) {
		err = true;
	    }
	}
    }

    if ( top_level )
	g_hash_table_destroy(ht);

    free((char *)rel_rq.utf8_family_name);
    free(val);
    free(relname);

    if ( err ) {
	free((char *)rq->utf8_family_name);
	rq->utf8_family_name = NULL;
	return false;
    }

    return true;
}

int ResStrToFontRequest(const char *resstr, FontRequest *rq) {
    return _GResToFontRequest(resstr, rq, NULL, true, true);
}

static void _GResourceFindFont(const char *resourcename, GResFont *font, int is_value) {
    FontRequest rq;
    FontInstance *fi;
    char *rstr;
    memset(&rq, 0, sizeof(rq));
    if ( is_value )
	rstr = copy(resourcename);
    else
	rstr = GResourceFindString(resourcename);

    if ( rstr!=NULL && ( font->rstr==NULL || strcmp(rstr, font->rstr)!=0 ) ) {
	if ( ResStrToFontRequest(rstr, &rq) ) {
	    fi = GDrawInstanciateFont(NULL, &rq);
	    if ( fi!=NULL ) {
		// The original resource string is likely a constant so just accept
		// leaking in the rare double-set cases.
		font->rstr = rstr;
		font->fi = fi;
	    } else {
		TRACE("Could not find font corresponding to resource %s '%s', using default\n", resourcename, rstr);
	    }
	} else {
	    //TRACE("Could not convert font resource %s '%s', using default\n", resourcename, rstr);
	}
    }

    if ( rstr!=font->rstr )
	free(rstr);

    if ( font->fi==NULL && font->rstr!=NULL ) {
#ifndef NDEBUG
	int r =
#endif
	ResStrToFontRequest(font->rstr, &rq);
	assert( r );
        font->fi = GDrawInstanciateFont(NULL, &rq);
	free((char *) rq.utf8_family_name);
	if ( font->fi==NULL ) {
	    TRACE("Could not find font corresponding to default '%s', using system default\n", font->rstr);
	    font->fi = _ggadget_default_font.fi;
	}
    }
}

void GResourceFindFont(const char *resourcename, const char *elemname, GResFont *font) {
    char *rstr;
    if ( resourcename!=NULL )
	rstr = smprintf("%s.%s", resourcename, elemname);
    else
	rstr = (char *) elemname;
    _GResourceFindFont(rstr, font, false);
    if ( resourcename!=NULL )
	free(rstr);
}

static void _GResourceFindImage(const char *fname, GResImage *img) {
    GImageCacheBucket *t = NULL;

    if ( fname!=NULL ) {
	t = _GGadgetImageCache(fname, false);
	if ( t==NULL || t->image==NULL ) {
	    TRACE("Could not find image corresponding to '%s', using default\n", fname);
	} else {
	    img->bucket = t;
	}
    }

    if ( GResImageGetImage(img)==NULL && img->ini_name!=NULL ) {
	img->bucket = _GGadgetImageCache(img->ini_name, true);
	if ( GResImageGetImage(img)==NULL ) {
	    TRACE("Could not find image corresponding to default name '%s'\n", img->ini_name);
	}
    }
}

GImage *GResImageGetImage(GResImage *ri) {
    if ( ri==NULL || ri->bucket==NULL )
	return NULL;

    return ri->bucket->image;
}
