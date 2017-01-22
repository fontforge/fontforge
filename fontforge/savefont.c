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

#include "savefont.h"

#include "autohint.h"
#include "cvundoes.h"
#include "dumpbdf.h"
#include "dumppfa.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "http.h"
#include "macbinary.h"
#include "namelist.h"
#include "palmfonts.h"
#include "ustring.h"
#include "gfile.h"
#include "gresource.h"
#include "utype.h"
#include "gio.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "psfont.h"

int old_sfnt_flags = ttf_flag_otmode;
int old_ps_flags = ps_flag_afm|ps_flag_round;
int old_psotb_flags = ps_flag_afm;

int oldformatstate = ff_pfb;
int oldbitmapstate = 0;
#if __Mac
const char *savefont_extensions[] = { ".pfa", ".pfb", ".res", "%s.pfb", ".pfa", ".pfb", ".pt3", ".ps",
	".cid", ".cff", ".cid.cff",
	".t42", ".t11",
	".ttf", ".ttf", ".suit", ".ttc", ".dfont", ".otf", ".otf.dfont", ".otf",
	".otf.dfont", ".svg", ".ufo", ".woff", NULL };
const char *bitmapextensions[] = { "-*.bdf", ".ttf", ".dfont", ".ttf", ".otb", ".bmap", ".dfont", ".fon", "-*.fnt", ".pdb", "-*.pt3", ".none", NULL };
#else
const char *savefont_extensions[] = { ".pfa", ".pfb", ".bin", "%s.pfb", ".pfa", ".pfb", ".pt3", ".ps",
	".cid", ".cff", ".cid.cff",
	".t42", ".t11",
	".ttf", ".ttf", ".ttf.bin", ".ttc", ".dfont", ".otf", ".otf.dfont", ".otf",
	".otf.dfont", ".svg",
	".ufo",
	".woff",
NULL };
const char *bitmapextensions[] = { "-*.bdf", ".ttf", ".dfont", ".ttf", ".otb", ".bmap.bin", ".fon", "-*.fnt", ".pdb", "-*.pt3", ".none", NULL };
#endif

static int WriteAfmFile(char *filename,SplineFont *sf, int formattype,
	EncMap *map, int flags, SplineFont *fullsf, int layer) {
    char *buf = malloc(strlen(filename)+6), *pt, *pt2;
    FILE *afm;
    int ret;
    int subtype = formattype;

    if ( (formattype==ff_mma || formattype==ff_mmb) && sf->mm!=NULL ) {
	sf = sf->mm->normal;
	subtype = ff_pfb;
    }

    strcpy(buf,filename);
    pt = strrchr(buf,'.');
    if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	pt = NULL;
    if ( pt==NULL )
	strcat(buf,".afm");
    else
	strcpy(pt,".afm");
    ff_progress_change_line1(_("Saving AFM File"));
    ff_progress_change_line2(buf);
    if ( strstr(buf,"://")==NULL )
	afm = fopen(buf,"w");
    else
	afm = tmpfile();
    if ( afm==NULL ) {
	free(buf);
return( false );
    }
    ret = AfmSplineFont(afm,sf,subtype,map,flags&ps_flag_afmwithmarks,fullsf,layer);
    if ( ret && strstr(buf,"://")!=NULL )
	ret = URLFromFile(buf,afm);
    free(buf);
    if ( fclose(afm)==-1 )
return( false );
    if ( !ret )
return( false );

    if ( (formattype==ff_mma || formattype==ff_mmb) && sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	int i;
	for ( i=0; i<mm->instance_count; ++i ) {
	    sf = mm->instances[i];
	    buf = malloc(strlen(filename)+strlen(sf->fontname)+4+1);
	    strcpy(buf,filename);
	    pt = strrchr(buf,'/');
	    if ( pt==NULL ) pt = buf;
	    else ++pt;
	    strcpy(pt,sf->fontname);
	    strcat(pt,".afm");
	    ff_progress_change_line2(buf);
	    afm = fopen(buf,"w");
	    free(buf);
	    if ( afm==NULL )
return( false );
	    ret = AfmSplineFont(afm,sf,subtype,map,flags&ps_flag_afmwithmarks,NULL,layer);
	    if ( fclose(afm)==-1 )
return( false );
	    if ( !ret )
return( false );
	}
	buf = malloc(strlen(filename)+8);

	strcpy(buf,filename);
	pt = strrchr(buf,'.');
	if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	    pt = NULL;
	if ( pt==NULL )
	    strcat(buf,".amfm");
	else
	    strcpy(pt,".amfm");
	ff_progress_change_line2(buf);
	afm = fopen(buf,"w");
	free(buf);
	if ( afm==NULL )
return( false );
	ret = AmfmSplineFont(afm,mm,formattype,map,layer);
	if ( fclose(afm)==-1 )
return( false );
    }
return( ret );
}

static int WriteTfmFile(char *filename,SplineFont *sf, EncMap *map,int layer) {
    char *buf = malloc(strlen(filename)+6), *pt, *pt2;
    FILE *tfm, *enc;
    int ret;
    int i;
    const char *encname;

    strcpy(buf,filename);
    pt = strrchr(buf,'.');
    if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	pt = NULL;
    if ( pt==NULL )
	strcat(buf,".tfm");
    else
	strcpy(pt,".tfm");
    ff_progress_change_line1(_("Saving TFM File"));
    ff_progress_change_line2(buf);
    ff_progress_next();	/* Forces a refresh */
    tfm = fopen(buf,"wb");
    if ( tfm==NULL )
return( false );
    ret = TfmSplineFont(tfm,sf,map,layer);
    if ( fclose(tfm)==-1 )
	ret = 0;

    pt = strrchr(buf,'.');
    strcpy(pt,".enc");
    enc = fopen(buf,"wb");
    free(buf);
    if ( enc==NULL )
return( false );

    encname=NULL;
    if ( sf->subfontcnt==0 && map->enc!=&custom )
	encname = EncodingName(map->enc );
    if ( encname==NULL )
	fprintf( enc, "/%s-Enc [\n", sf->fontname );
    else
	fprintf( enc, "/%s [\n", encname );
    for ( i=0; i<map->enccount && i<256; ++i ) {
	if ( map->map[i]==-1 || !SCWorthOutputting(sf->glyphs[map->map[i]]) )
	    fprintf( enc, " /.notdef" );
	else
	    fprintf( enc, " /%s", sf->glyphs[map->map[i]]->name );
	if ( (i&0xf)==0 )
	    fprintf( enc, "\t\t%% 0x%02x", i );
	putc('\n',enc);
    }
    while ( i<256 ) {
	fprintf( enc, " /.notdef" );
	if ( (i&0xf0)==0 )
	    fprintf( enc, "\t\t%% 0x%02x", i );
	putc('\n',enc);
	++i;
    }
    fprintf( enc, "] def\n" );

    if ( fclose(enc)==-1 )
	ret = 0;
return( ret );
}

static int WriteOfmFile(char *filename,SplineFont *sf, EncMap *map,int layer) {
    char *buf = malloc(strlen(filename)+6), *pt, *pt2;
    FILE *tfm, *enc;
    int ret;
    int i;
    const char *encname;
    const char *texparamnames[] = { "SLANT", "SPACE", "STRETCH", "SHRINK", "XHEIGHT", "QUAD", "EXTRASPACE", NULL };

    strcpy(buf,filename);
    pt = strrchr(buf,'.');
    if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	pt = NULL;
    if ( pt==NULL )
	strcat(buf,".ofm");
    else
	strcpy(pt,".ofm");
    ff_progress_change_line1(_("Saving OFM File"));
    ff_progress_change_line2(buf);
    ff_progress_next();	/* Forces a refresh */
    tfm = fopen(buf,"wb");
    if ( tfm==NULL )
return( false );
    ret = OfmSplineFont(tfm,sf,map,layer);
    if ( fclose(tfm)==-1 )
	ret = 0;

    pt = strrchr(buf,'.');
    strcpy(pt,".cfg");
    enc = fopen(buf,"wb");
    free(buf);
    if ( enc==NULL )
return( false );

    fprintf( enc, "VTITLE %s\n", sf->fontname );
    fprintf( enc, "FAMILY %s\n", sf->familyname );
    encname=NULL;
    if ( sf->subfontcnt==0 && map->enc!=&custom )
	encname = EncodingName(map->enc );
    fprintf( enc, "CODINGSCHEME %s\n", encname==NULL?encname:"FONT-SPECIFIC" );

    /* OfmSplineFont has already called TeXDefaultParams, so we don't have to */
    fprintf( enc, "EPSILON 0.090\n" );		/* I have no idea what this means */
    for ( i=0; texparamnames[i]!=NULL; ++i )
	fprintf( enc, "%s %g\n", texparamnames[i], sf->texdata.params[i]/(double) (1<<20) );

    for ( i=0; i<map->enccount && i<65536; ++i ) {
	if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]]) )
	    fprintf( enc, "%04X N %s\n", i, sf->glyphs[map->map[i]]->name );
    }

    if ( fclose(enc)==-1 )
	ret = 0;
return( ret );
}

#ifndef FONTFORGE_CONFIG_WRITE_PFM
static
#endif
int WritePfmFile(char *filename,SplineFont *sf, EncMap *map,int layer) {
    char *buf = malloc(strlen(filename)+6), *pt, *pt2;
    FILE *pfm;
    int ret;

    strcpy(buf,filename);
    pt = strrchr(buf,'.');
    if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	pt = NULL;
    if ( pt==NULL )
	strcat(buf,".pfm");
    else
	strcpy(pt,".pfm");
    ff_progress_change_line2(buf);
    pfm = fopen(buf,"wb");
    free(buf);
    if ( pfm==NULL )
return( false );
    ret = PfmSplineFont(pfm,sf,map,layer);
    if ( fclose(pfm)==-1 )
return( 0 );
return( ret );
}

static int WriteFontLog(char *filename,SplineFont *sf) {
    char *buf = malloc(strlen(filename)+12), *pt;
    FILE *flog;

    if ( sf->fontlog==NULL || *sf->fontlog=='\0' )
return( true );

    strcpy(buf,filename);
    pt = strrchr(buf,'/');
    if ( pt==NULL )
	strcat(buf,"FONTLOG.txt");
    else
	strcpy(pt+1,"FONTLOG.txt");
    flog = fopen(buf,"a"); // We changed this to append if the file exists.
    free(buf);
    if ( flog==NULL )
return( false );

    for ( pt=sf->fontlog; *pt; ++pt )
	putc(*pt,flog);
    if ( fclose(flog)!=0 )
return( false );

return( true );
}

static int WriteBitmaps(char *filename,SplineFont *sf, int32 *sizes,int res,
	int bf, EncMap *map) {
    char *buf = malloc(strlen(filename)+30), *pt, *pt2;
    int i;
    BDFFont *bdf;
    const char *ext;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( i=0; sizes[i]!=0; ++i );
    ff_progress_change_stages(i);
    for ( i=0; sizes[i]!=0; ++i ) {
	for ( bdf=sf->bitmaps; bdf!=NULL &&
		(bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16));
		bdf=bdf->next );
	if ( bdf==NULL ) {
	    ff_post_notice(_("Missing Bitmap"),_("Attempt to save a pixel size that has not been created (%d@%d)"),
		    sizes[i]&0xffff, sizes[i]>>16);
	    free(buf);
return( false );
	}

	if ( bf==bf_ptype3 && bdf->clut!=NULL ) {
	    ff_post_notice(_("Missing Bitmap"),_("Currently, FontForge only supports bitmap (not bytemap) type3 output") );
return( false );
	}

	strcpy(buf,filename);
	pt = strrchr(buf,'.');
	if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	    pt = NULL;
	if ( pt==NULL )
	    pt = buf+strlen(buf);
	if ( strcmp(pt-4,".otf.dfont")==0 || strcmp(pt-4,".ttf.bin")==0 ) pt-=4;
	if ( pt-2>buf && pt[-2]=='-' && pt[-1]=='*' )
	    pt -= 2;
	ext = bf==bf_bdf ? ".bdf" : bf==bf_ptype3 ? ".pt3" : ".fnt";
	if ( bdf->clut==NULL )
	    sprintf( pt, "-%d%s", bdf->pixelsize, ext );
	else
	    sprintf( pt, "-%d@%d%s", bdf->pixelsize, BDFDepth(bdf), ext );

	ff_progress_change_line2(buf);
	if ( bf==bf_bdf ) 
	    BDFFontDump(buf,bdf,map,res);
	else if ( bf==bf_ptype3 )
	    PSBitmapDump(buf,bdf,map);
	else if ( bf==bf_fnt )
	    FNTFontDump(buf,bdf,map,res);
	else
	    IError( "Unexpected font type" );
	ff_progress_next_stage();
    }
    free(buf);
return( true );
}

static int32 *ParseWernerSFDFile(char *wernerfilename,SplineFont *sf,int *max,
	char ***_names, EncMap *map) {
    /* one entry for each char, >=1 => that subfont, 0=>not mapped, -1 => end of char mark */
    int cnt=0, subfilecnt=0, thusfar;
    int k, warned = false;
    int r1,r2,i,modi;
    SplineFont *_sf;
    int32 *mapping;
    FILE *file;
    char buffer[200], *bpt;
    char *end, *pt;
    char *orig;
    struct remap *remap;
    char **names;
    int loop;
    static const char *pfaeditflag = "SplineFontDB:";

    file = fopen(wernerfilename,"r");
    if ( file==NULL ) {
	ff_post_error(_("No Sub Font Definition file"),_("No Sub Font Definition file"));
return( NULL );
    }

    k = 0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k++];
	if ( _sf->glyphcnt>cnt ) cnt = _sf->glyphcnt;
    } while ( k<sf->subfontcnt );

    mapping = calloc(cnt+1,sizeof(int32));
    memset(mapping,-1,(cnt+1)*sizeof(int32));
    mapping[cnt] = -2;
    *max = 0;

    while ( fgets(buffer,sizeof(buffer),file)!=NULL )
	++subfilecnt;
    names = malloc((subfilecnt+1)*sizeof(char *));

    rewind(file);
    subfilecnt = 0;
    while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	if ( strncmp(buffer,pfaeditflag,strlen(pfaeditflag))== 0 ) {
	    ff_post_error(_("Wrong type of SFD file"),_("This looks like one of FontForge's SplineFont DataBase files.\nNot one of TeX's SubFont Definition files.\nAn unfortunate confusion of extensions."));
	    free(mapping);
return( NULL );
	}
	pt=buffer+strlen(buffer)-1;
	bpt = buffer;
	if (( *pt!='\n' && *pt!='\r') || (pt>buffer && pt[-1]=='\\') ||
		(pt>buffer+1 && pt[-2]=='\\' && isspace(pt[-1])) ) {
	    bpt = copy("");
	    for (;;) {
		loop = false;
		if (( *pt!='\n' && *pt!='\r') || (pt>buffer && pt[-1]=='\\') ||
			(pt>buffer+1 && pt[-2]=='\\' && isspace(pt[-1])) )
		    loop = true;
		if ( *pt=='\n' || *pt=='\r') {
		    if ( pt[-1]=='\\' )
			pt[-1] = '\0';
		    else if ( pt[-2]=='\\' )
			pt[-2] = '\0';
		}
		bpt = realloc(bpt,strlen(bpt)+strlen(buffer)+10);
		strcat(bpt,buffer);
		if ( !loop )
	    break;
		if ( fgets(buffer,sizeof(buffer),file)==NULL )
	    break;
		pt=buffer+strlen(buffer)-1;
	    }
	}
	if ( bpt[0]=='#' || bpt[0]=='\0' || isspace(bpt[0]))
    continue;
	for ( pt = bpt; !isspace(*pt) && *pt!='\0'; ++pt );
	if ( *pt=='\0' || *pt=='\r' || *pt=='\n' )
    continue;
	names[subfilecnt] = copyn(bpt,pt-bpt);
	if ( subfilecnt>*max ) *max = subfilecnt;
	end = pt;
	thusfar = 0;
	while ( *end!='\0' ) {
	    while ( isspace(*end)) ++end;
	    if ( *end=='\0' )
	break;
	    orig = end;
	    r1 = strtoul(end,&end,0);
	    if ( orig==end )
	break;
	    while ( isspace(*end)) ++end;
	    if ( *end==':' ) {
		if ( r1>=256 || r1<0)
		    LogError( _("Bad offset: %d for subfont %s\n"), r1, names[subfilecnt]);
		else
		    thusfar = r1;
		r1 = strtoul(end+1,&end,0);
	    }
	    if ( *end=='_' || *end=='-' )
		r2 = strtoul(end+1,&end,0);
	    else
		r2 = r1;
	    for ( i=r1; i<=r2; ++i ) {
		modi = i;
		if ( map->remap!=NULL ) {
		    for ( remap = map->remap; remap->infont!=-1; ++remap ) {
			if ( i>=(int)remap->firstenc && i<=(int)remap->lastenc ) {
			    modi = i-remap->firstenc + remap->infont;
		    break;
			}
		    }
		}
		if ( modi<map->enccount )
		    modi = map->map[modi];
		else if ( sf->subfontcnt!=0 )
		    modi = modi;
		else
		    modi = -1;
		if ( modi<cnt && modi!=-1 ) {
		    if ( mapping[modi]!=-1 && !warned ) {
			if (( i==0xffff || i==0xfffe ) &&
				(map->enc->is_unicodebmp || map->enc->is_unicodefull))
			    /* Not a character anyway. just ignore it */;
			else {
			    LogError( _("Warning: Encoding %d (0x%x) is mapped to at least two locations (%s@0x%02x and %s@0x%02x)\n Only one will be used here.\n"),
				    i, i, names[subfilecnt], thusfar, names[(mapping[modi]>>8)], mapping[modi]&0xff );
			    warned = true;
			}
		    }
		    mapping[modi] = (subfilecnt<<8) | thusfar;
		}
		thusfar++;
	    }
	}
	if ( thusfar>256 )
	    LogError( _("More than 256 entries in subfont %s\n"), names[subfilecnt] );
	++subfilecnt;
	if ( bpt!=buffer )
	    free(bpt);
    }
    names[subfilecnt]=NULL;
    *_names = names;
    fclose(file);
return( mapping );
}

static int SaveSubFont(SplineFont *sf,char *newname,
	int32 *mapping, int subfont, char **names,int layer) {
    SplineFont temp;
    SplineChar *chars[256], **newchars;
    SplineFont *_sf;
    int k, i, used, base, extras;
    char *filename;
    char *spt, *pt, buf[8];
    RefChar *ref;
    int err = 0;
    enum fontformat subtype = strstr(newname,".pfa")!=NULL ? ff_pfa : ff_pfb ;
    EncMap encmap;
    int32 _mapping[256], _backmap[256];

    memset(&encmap,0,sizeof(encmap));
    encmap.enccount = encmap.encmax = encmap.backmax = 256;
    encmap.map = _mapping; encmap.backmap = _backmap;
    memset(_mapping,-1,sizeof(_mapping));
    memset(_backmap,-1,sizeof(_backmap));
    encmap.enc = &custom;

    temp = *sf;
    temp.glyphcnt = 0;
    temp.glyphmax = 256;
    temp.glyphs = chars;
    temp.bitmaps = NULL;
    temp.subfonts = NULL;
    temp.subfontcnt = 0;
    temp.uniqueid = 0;
    memset(chars,0,sizeof(chars));
    temp.glyphnames = NULL;
    used = 0;
    for ( i=0; mapping[i]!=-2; ++i ) if ( (mapping[i]>>8)==subfont ) {
	k = 0;
	do {
	    _sf = sf->subfontcnt==0 ? sf : sf->subfonts[k++];
	    if ( i<_sf->glyphcnt && _sf->glyphs[i]!=NULL )
	break;
	} while ( k<sf->subfontcnt );
	if ( temp.glyphcnt<256 ) {
	    if ( i<_sf->glyphcnt ) {
		if ( _sf->glyphs[i]!=NULL ) {
		    _sf->glyphs[i]->parent = &temp;
		    _sf->glyphs[i]->orig_pos = temp.glyphcnt;
		    chars[temp.glyphcnt] = _sf->glyphs[i];
		    _mapping[mapping[i]&0xff] = temp.glyphcnt;
		    _backmap[temp.glyphcnt] = mapping[i]&0xff;
		    ++temp.glyphcnt;
		    ++used;
		}
	    }
	}
    }
    if ( used==0 )
return( 0 );

    /* check for any references to things outside this subfont and add them */
    /*  as unencoded chars */
    /* We could just replace with splines, I suppose but that would make */
    /*  korean fonts huge */
    for (;;) {
	extras = 0;
	for ( i=0; i<temp.glyphcnt; ++i ) if ( temp.glyphs[i]!=NULL ) {
	    for ( ref=temp.glyphs[i]->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
		if ( ref->sc->parent!=&temp )
		    ++extras;
	}
	if ( extras == 0 )
    break;
	newchars = calloc(temp.glyphcnt+extras,sizeof(SplineChar *));
	memcpy(newchars,temp.glyphs,temp.glyphcnt*sizeof(SplineChar *));
	if ( temp.glyphs!=chars ) free(temp.glyphs );
	base = temp.glyphcnt;
	temp.glyphs = newchars;
	extras = 0;
	for ( i=0; i<base; ++i ) if ( temp.glyphs[i]!=NULL ) {
	    for ( ref=temp.glyphs[i]->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
		if ( ref->sc->parent!=&temp ) {
		    temp.glyphs[base+extras] = ref->sc;
		    ref->sc->parent = &temp;
		    ref->sc->orig_pos = base+extras++;
		}
	}
	temp.glyphcnt += extras;	/* this might be a slightly different value from that found before if some references get reused. N'importe */
	temp.glyphmax = temp.glyphcnt;
    }

    filename = malloc(strlen(newname)+strlen(names[subfont])+10);
    strcpy(filename,newname);
    pt = strrchr(filename,'.');
    spt = strrchr(filename,'/');
    if ( spt==NULL ) spt = filename; else ++spt;
    if ( pt>spt )
	*pt = '\0';
    pt = strstr(spt,"%d");
    if ( pt==NULL )
	pt = strstr(spt,"%s");
    if ( pt==NULL )
	strcat(filename,names[subfont]);
    else {
	int len = strlen(names[subfont]);
	int l;
	if ( len>2 ) {
	    for ( l=strlen(pt); l>=2 ; --l )
		pt[l+len-2] = pt[l];
	} else if ( len<2 )
	    strcpy(pt+len,pt+2);
	memcpy(pt,names[subfont],len);
    }
    temp.fontname = copy(spt);
    temp.fullname = malloc(strlen(temp.fullname)+strlen(names[subfont])+3);
    strcpy(temp.fullname,sf->fullname);
    strcat(temp.fullname," ");
    strcat(temp.fullname,names[subfont]);
    strcat(spt,subtype==ff_pfb ? ".pfb" : ".pfa" );
    ff_progress_change_line2(filename);

    if ( sf->xuid!=NULL ) {
	sprintf( buf, "%d", subfont );
	temp.xuid = malloc(strlen(sf->xuid)+strlen(buf)+5);
	strcpy(temp.xuid,sf->xuid);
	pt = temp.xuid + strlen( temp.xuid )-1;
	while ( pt>temp.xuid && *pt==' ' ) --pt;
	if ( *pt==']' ) --pt;
	*pt = ' ';
	strcpy(pt+1,buf);
	strcat(pt,"]");
    }

    err = !WritePSFont(filename,&temp,subtype,old_ps_flags,&encmap,sf,layer);
    if ( err )
	ff_post_error(_("Save Failed"),_("Save Failed"));
    if ( !err && (old_ps_flags&ps_flag_afm) && ff_progress_next_stage()) {
	if ( !WriteAfmFile(filename,&temp,oldformatstate,&encmap,old_ps_flags,sf,layer)) {
	    ff_post_error(_("Afm Save Failed"),_("Afm Save Failed"));
	    err = true;
	}
    }
    if ( !err && (old_ps_flags&ps_flag_tfm) ) {
	if ( !WriteTfmFile(filename,&temp,&encmap,layer)) {
	    ff_post_error(_("Tfm Save Failed"),_("Tfm Save Failed"));
	    err = true;
	}
    }
    /* ??? Bitmaps */
    if ( !ff_progress_next_stage())
	err = -1;

    if ( temp.glyphs!=chars )
	free(temp.glyphs);
    GlyphHashFree( &temp );
    free( temp.xuid );
    free( temp.fontname );
    free( temp.fullname );
    free( filename );

    /* SaveSubFont messes up the parent and orig_pos fields. Fix 'em up */
    /* Do this after every save, else afm,tfm files might produce extraneous kerns */
    k = 0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k++];
	for ( i=0; i<_sf->glyphcnt; ++i ) if ( _sf->glyphs[i]!=NULL ) {
	    _sf->glyphs[i]->parent = _sf;
	    _sf->glyphs[i]->orig_pos = i;
	}
    } while ( k<sf->subfontcnt );

return( err );
}

/* ttf2tfm supports multiple sfd files. I do not. */
static int WriteMultiplePSFont(SplineFont *sf,char *newname,int32 *sizes,
	char *wernerfilename,EncMap *map, int layer) {
    int err=0, tofree=false, max, filecnt;
    int32 *mapping;
    char *path;
    int i;
    char **names;
    char *pt;

    pt = strrchr(newname,'.');
    if ( pt==NULL ||
	    (strcmp(pt,".pfa")!=0 && strcmp(pt,".pfb")!=0 && strcmp(pt,".mult")!=0)) {
	ff_post_error(_("Bad Extension"),_("You must specify a standard type1 extension (.pfb or .pfa)"));
return( 0 );
    }
    if ( wernerfilename==NULL )
return( 0 );
    mapping = ParseWernerSFDFile(wernerfilename,sf,&max,&names,map);
    if ( tofree ) free(wernerfilename);
    if ( mapping==NULL )
return( 1 );

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;

    filecnt = 1;
    if ( (old_ps_flags&ps_flag_afm) )
	filecnt = 2;
    path = def2utf8_copy(newname);
    ff_progress_start_indicator(10,_("Saving font"),
	    _("Saving Multiple PostScript Fonts"),
	    path,256,(max+1)*filecnt );
    free(path);

    for ( i=0; i<=max && !err; ++i )
	err = SaveSubFont(sf,newname,mapping,i,names,layer);

    free(mapping);
    for ( i=0; names[i]!=NULL; ++i ) free(names[i]);
    free(names);
    free( sizes );
    ff_progress_end_indicator();
    if ( !err )
	SavePrefs(true);
return( err );
}

int CheckIfTransparent(SplineFont *sf) {
    /* Type3 doesn't support translucent fills */
    int i,j;
    char *buts[3];
    buts[0] = _("_Yes");
    buts[1] = _("_Cancel");
    buts[2] = NULL;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];
	for ( j=ly_fore; j<sc->layer_cnt; ++j ) {
	    if ( sc->layers[j].fill_brush.opacity!=1 || sc->layers[j].stroke_pen.brush.opacity!=1 ) {
		if ( ff_ask(_("Bad Drawing Operation"),(const char **) buts,0,1,_("This font contains at least one translucent layer, but type3 does not support that (anything translucent or transparent is treated as opaque). Do you want to proceed anyway?"))==1 )
return( true );

return( false );
	    }
	}
    }
return( false );
}

int _DoSave(SplineFont *sf,char *newname,int32 *sizes,int res,
	EncMap *map, char *subfontdefinition,int layer) {
    char *path;
    int err=false;
    int iscid = oldformatstate==ff_cid || oldformatstate==ff_cffcid ||
	    oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont;
    int flags = 0;
    int tmpstore = 0;

    if ( oldformatstate == ff_multiple )
return( WriteMultiplePSFont(sf,newname,sizes,subfontdefinition,map,layer));

    if ( oldformatstate<=ff_cffcid )
	flags = old_ps_flags;
    else if ( oldformatstate<=ff_ttfdfont )
	flags = old_sfnt_flags;
    else if ( oldformatstate!=ff_none )
	flags = old_sfnt_flags;
    else
	flags = old_sfnt_flags&~(ttf_flag_ofm);
    if ( oldformatstate<=ff_cffcid && oldbitmapstate==bf_otb )
	flags = old_psotb_flags;

    path = def2utf8_copy(newname);
    ff_progress_start_indicator(10,_("Saving font"),
		oldformatstate==ff_ttf || oldformatstate==ff_ttfsym ||
		     oldformatstate==ff_ttfmacbin ?_("Saving TrueType Font") :
		 oldformatstate==ff_otf || oldformatstate==ff_otfdfont ?_("Saving OpenType Font"):
		 oldformatstate==ff_cid || oldformatstate==ff_cffcid ||
		  oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont ?_("Saving CID keyed font") :
		  oldformatstate==ff_mma || oldformatstate==ff_mmb ?_("Saving multi-master font") :
		  oldformatstate==ff_svg ?_("Saving SVG font") :
		  oldformatstate==ff_ufo ?_("Saving Unified Font Object") :
		 _("Saving PostScript Font"),
	    path,sf->glyphcnt,1);
    free(path);
    if ( oldformatstate!=ff_none ) {
	int oerr = 0;
	int bmap = oldbitmapstate;
	if ( bmap==bf_otb ) bmap = bf_none;
	if ( strstr(newname,"://")!=NULL ) {
	    if ( oldformatstate==ff_pfbmacbin || oldformatstate==ff_ttfmacbin ) {
		ff_post_error(_("Mac Resource Not Remote"),_("You may not save a mac resource file to a remote location"));
		oerr = true;
	    } else if ( oldformatstate==ff_ufo ) {
		ff_post_error(_("Directory Not Remote"),_("You may not save ufo directory to a remote location"));
		oerr = true;
	    }
	}
	if ( !oerr ) switch ( oldformatstate ) {
	  case ff_mma: case ff_mmb:
	    sf = sf->mm->instances[0];
	  case ff_pfa: case ff_pfb: case ff_ptype3: case ff_ptype0:
	  case ff_cid:
	  case ff_type42: case ff_type42cid:
	    if ( sf->multilayer && CheckIfTransparent(sf))
return( true );
	    oerr = !WritePSFont(newname,sf,oldformatstate,flags,map,NULL,layer);
	  break;
	  case ff_ttf: case ff_ttfsym: case ff_otf: case ff_otfcid:
	  case ff_cff: case ff_cffcid:
	    oerr = !WriteTTFFont(newname,sf,oldformatstate,sizes,bmap,
		flags,map,layer);
	  break;
	  case ff_woff:
	    oerr = !WriteWOFFFont(newname,sf,oldformatstate,sizes,bmap,
		flags,map,layer);
	  break;
	  case ff_pfbmacbin:
	    oerr = !WriteMacPSFont(newname,sf,oldformatstate,flags,map,layer);
	  break;
	  case ff_ttfmacbin: case ff_ttfdfont: case ff_otfdfont: case ff_otfciddfont:
	    oerr = !WriteMacTTFFont(newname,sf,oldformatstate,sizes,
		    bmap,flags,map,layer);
	  break;
	  case ff_svg:
	    oerr = !WriteSVGFont(newname,sf,oldformatstate,flags,map,layer);
	  break;
	  case ff_ufo:
	    tmpstore = sf->preferred_kerning; // We toggle this flag in order to force native kerning output.
	    if (flags & ttf_native_kern) sf->preferred_kerning = 1; // 1 flags native kerning.
	    sf->preferred_kerning |= 4; // 4 flags old-style naming for the starting name in UFONameKerningClasses.
	    oerr = !WriteUFOFont(newname,sf,oldformatstate,flags,map,layer);
	    if (flags & ttf_native_kern) sf->preferred_kerning = tmpstore;
	  break;
	  default:
	  break;
	}
	if ( oerr ) {
	    ff_post_error(_("Save Failed"),_("Save Failed"));
	    err = true;
	}
    }
    if ( !err && (flags&ps_flag_tfm) ) {
	if ( !WriteTfmFile(newname,sf,map,layer)) {
	    ff_post_error(_("Tfm Save Failed"),_("Tfm Save Failed"));
	    err = true;
	}
    }
    if ( !err && (flags&ttf_flag_ofm) ) {
	if ( !WriteOfmFile(newname,sf,map,layer)) {
	    ff_post_error(_("Ofm Save Failed"),_("Ofm Save Failed"));
	    err = true;
	}
    }
    if ( !err && (flags&ps_flag_afm) ) {
	ff_progress_increment(-sf->glyphcnt);
	if ( !WriteAfmFile(newname,sf,oldformatstate,map,flags,NULL,layer)) {
	    ff_post_error(_("Afm Save Failed"),_("Afm Save Failed"));
	    err = true;
	}
    }
    if ( !err && (flags&ps_flag_outputfontlog) ) {
	if ( !WriteFontLog(newname,sf)) {
	    ff_post_error(_("FontLog Save Failed"),_("FontLog Save Failed"));
	    err = true;
	}
    }
    if ( !err && (flags&ps_flag_pfm) && !iscid ) {
	ff_progress_change_line1(_("Saving PFM File"));
	ff_progress_increment(-sf->glyphcnt);
	if ( !WritePfmFile(newname,sf,map,layer)) {
	    ff_post_error(_("Pfm Save Failed"),_("Pfm Save Failed"));
	    err = true;
	}
    }
    if ( oldbitmapstate==bf_otb || oldbitmapstate==bf_sfnt_ms ) {
	char *temp = newname;
	if ( newname[strlen(newname)-1]=='.' ) {
	    temp = malloc(strlen(newname)+8);
	    strcpy(temp,newname);
	    strcat(temp,oldbitmapstate==bf_otb ? "otb" : "ttf" );
	}
	if ( !WriteTTFFont(temp,sf,ff_none,sizes,oldbitmapstate,flags,map,layer) )
	    err = true;
	if ( temp!=newname )
	    free(temp);
    } else if ( oldbitmapstate==bf_sfnt_dfont ) {
	char *temp = newname;
	if ( newname[strlen(newname)-1]=='.' ) {
	    temp = malloc(strlen(newname)+8);
	    strcpy(temp,newname);
	    strcat(temp,"dfont");
	}
	if ( !WriteMacTTFFont(temp,sf,ff_none,sizes,oldbitmapstate,flags,map,layer) )
	    err = true;
	if ( temp!=newname )
	    free(temp);
    } else if ( (oldbitmapstate==bf_bdf || oldbitmapstate==bf_fnt ||
	    oldbitmapstate==bf_ptype3 ) && !err ) {
	ff_progress_change_line1(_("Saving Bitmap Font(s)"));
	ff_progress_increment(-sf->glyphcnt);
	if ( !WriteBitmaps(newname,sf,sizes,res,oldbitmapstate,map))
	    err = true;
    } else if ( oldbitmapstate==bf_fon && !err ) {
	if ( !FONFontDump(newname,sf,sizes,res,map))
	    err = true;
    } else if ( oldbitmapstate==bf_palm && !err ) {
	if ( !WritePalmBitmaps(newname,sf,sizes,map))
	    err = true;
    } else if ( (oldbitmapstate==bf_nfntmacbin /*|| oldbitmapstate==bf_nfntdfont*/) &&
	    !err ) {
	if ( !WriteMacBitmaps(newname,sf,sizes,false/*oldbitmapstate==bf_nfntdfont*/,map))
	    err = true;
    }
    free( sizes );
    ff_progress_end_indicator();
    if ( !err )
	SavePrefs(true);
return( err );
}

void PrepareUnlinkRmOvrlp(SplineFont *sf,const char *filename,int layer) {
    int gid;
    SplineChar *sc;
    RefChar *ref, *refnext;
    int old_nwui = no_windowing_ui, old_maxundoes = maxundoes;

#if !defined(_NO_PYTHON)
    PyFF_CallDictFunc(sf->python_temporary,"generateFontPostHook","fs",sf->fv,filename);
#endif

    if ( maxundoes==0 ) maxundoes = 1;		/* Force undoes */

    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL && sc->unlink_rm_ovrlp_save_undo ) {
	if ( autohint_before_generate && sc!=NULL &&
		sc->changedsincelasthinted && !sc->manualhints ) {
	    no_windowing_ui = true;
	    SplineCharAutoHint(sc,layer,NULL);	/* Do this now, else we get an unwanted undo on the stack from hinting */
	}
	no_windowing_ui = false;
	SCPreserveLayer(sc,layer,false);
	no_windowing_ui = true;			/* Clustering wants to create an undo that I don't need */
	for ( ref= sc->layers[layer].refs; ref!=NULL; ref=refnext ) {
	    refnext = ref->next;
	    SCRefToSplines(sc,ref,layer);
	}
#if 0
	// We await testing on the necessity of this operation.
	SCRoundToCluster(sc,layer,false,.03,.12);
#endif // 0
	sc->layers[layer].splines = SplineSetRemoveOverlap(sc,sc->layers[layer].splines,over_remove);
	no_windowing_ui = false;
	if ( !sc->manualhints )
	    sc->changedsincelasthinted = false;
    }
    no_windowing_ui = old_nwui;
    maxundoes = old_maxundoes;
}

void RestoreUnlinkRmOvrlp(SplineFont *sf,const char *filename,int layer) {
    int gid;
    SplineChar *sc;

    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL && sc->unlink_rm_ovrlp_save_undo ) {
	SCDoUndo(sc,layer);
	if ( !sc->manualhints )
	    sc->changedsincelasthinted = false;
    }
#if !defined(_NO_PYTHON)
    PyFF_CallDictFunc(sf->python_temporary,"generateFontPostHook","fs",sf->fv,filename);
#endif
}

static int32 *AllBitmapSizes(SplineFont *sf) {
    int32 *sizes=NULL;
    BDFFont *bdf;
    int i,cnt;

    for ( i=0; i<2; ++i ) {
	cnt = 0;
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( sizes!=NULL )
		sizes[cnt] = bdf->pixelsize | (BDFDepth(bdf)<<16);
	    ++cnt;
	}
	if ( i==1 )
    break;
	sizes = malloc((cnt+1)*sizeof(int32));
    }
    sizes[cnt] = 0;
return( sizes );
}

int GenerateScript(SplineFont *sf,char *filename,const char *bitmaptype, int fmflags,
	int res, char *subfontdefinition, struct sflist *sfs,EncMap *map,
	NameList *rename_to,int layer) {
    int i;
    static const char *bitmaps[] = {"bdf", "ttf", "dfont", "ttf", "otb", "bin", "fon", "fnt", "pdb", "pt3", NULL };
    int32 *sizes=NULL;
    char *end = filename+strlen(filename);
    struct sflist *sfi;
    char *freeme = NULL;
    int ret;
    struct sflist *sfl;
    char **former;

    if ( sf->bitmaps==NULL ) i = bf_none;
    else if ( strmatch(bitmaptype,"otf")==0 ) i = bf_ttf;
    else if ( strmatch(bitmaptype,"ms")==0 ) i = bf_ttf;
    else if ( strmatch(bitmaptype,"apple")==0 ) i = bf_ttf;
    else if ( strmatch(bitmaptype,"sbit")==0 ) i = bf_sfnt_dfont;
    else if ( strmatch(bitmaptype,"nfnt")==0 ) i = bf_nfntmacbin;
    else if ( strmatch(bitmaptype,"ps")==0 ) i = bf_ptype3;
    else for ( i=0; bitmaps[i]!=NULL; ++i ) {
	if ( strmatch(bitmaptype,bitmaps[i])==0 )
    break;
    }
    oldbitmapstate = i;

    for ( i=0; savefont_extensions[i]!=NULL; ++i ) {
	if ( strlen( savefont_extensions[i])>0 &&
             end-filename>=(ptrdiff_t)strlen(savefont_extensions[i]) &&
		strmatch(end-strlen(savefont_extensions[i]),savefont_extensions[i])==0 )
    break;
    }
    if ( end-filename>8 && strmatch(end-strlen(".ttf.bin"),".ttf.bin")==0 )
	i = ff_ttfmacbin;
    else if ( end-filename>5 && strmatch(end-strlen(".suit"),".suit")==0 )	/* Different extensions for Mac/non Mac, support both always */
	i = ff_ttfmacbin;
    else if ( end-filename>4 && strmatch(end-strlen(".bin"),".bin")==0 )
	i = ff_pfbmacbin;
    else if ( end-filename>4 && strmatch(end-strlen(".res"),".res")==0 )
	i = ff_pfbmacbin;
    else if ( end-filename>8 && strmatch(end-strlen(".sym.ttf"),".sym.ttf")==0 )
	i = ff_ttfsym;
    else if ( end-filename>8 && strmatch(end-strlen(".cid.cff"),".cid.cff")==0 )
	i = ff_cffcid;
    else if ( end-filename>8 && strmatch(end-strlen(".cid.t42"),".cid.t42")==0 )
	i = ff_type42cid;
    else if ( end-filename>7 && strmatch(end-strlen(".mm.pfa"),".mm.pfa")==0 )
	i = ff_mma;
    else if ( end-filename>7 && strmatch(end-strlen(".mm.pfb"),".mm.pfb")==0 )
	i = ff_mmb;
    else if ( end-filename>7 && strmatch(end-strlen(".mult"),".mult")==0 )
	i = ff_multiple;
    else if (( i==ff_pfa || i==ff_pfb ) && strstr(filename,"%s")!=NULL )
	i = ff_multiple;
    if ( savefont_extensions[i]==NULL ) {
	for ( i=0; bitmaps[i]!=NULL; ++i ) {
	    if ( end-filename>(ptrdiff_t)strlen(bitmaps[i]) &&
		    strmatch(end-strlen(bitmaps[i]),bitmaps[i])==0 )
	break;
	}
	if ( *filename=='\0' || end[-1]=='.' )
	    i = ff_none;
	else if ( bitmaps[i]==NULL )
	    i = ff_pfb;
	else {
	    oldbitmapstate = i;
	    i = ff_none;
	}
    }
    if ( i==ff_ttfdfont && strmatch(end-strlen(".otf.dfont"),".otf.dfont")==0 )
	i = ff_otfdfont;
    if ( sf->cidmaster!=NULL ) {
	if ( i==ff_otf ) i = ff_otfcid;
	else if ( i==ff_otfdfont ) i = ff_otfciddfont;
    }
    if ( (i==ff_none || sf->onlybitmaps) && oldbitmapstate==bf_ttf )
	oldbitmapstate = bf_sfnt_ms;
    oldformatstate = i;

    if ( oldformatstate==ff_none && end[-1]=='.' &&
	    (oldbitmapstate==bf_ttf || oldbitmapstate==bf_sfnt_dfont || oldbitmapstate==bf_otb)) {
	freeme = malloc(strlen(filename)+8);
	strcpy(freeme,filename);
	if ( strmatch(bitmaptype,"otf")==0 )
	    strcat(freeme,"otf");
	else if ( oldbitmapstate==bf_otb )
	    strcat(freeme,"otb");
	else if ( oldbitmapstate==bf_sfnt_dfont )
	    strcat(freeme,"dfont");
	else
	    strcat(freeme,"ttf");
	filename = freeme;
    } else if ( sf->onlybitmaps && sf->bitmaps!=NULL &&
	    (oldformatstate==ff_ttf || oldformatstate==ff_otf) &&
	    (oldbitmapstate == bf_none || oldbitmapstate==bf_ttf ||
	     oldbitmapstate==bf_sfnt_dfont || oldbitmapstate==bf_otb)) {
	if ( oldbitmapstate==ff_ttf )
	    oldbitmapstate = bf_ttf;
	oldformatstate = ff_none;
    }

    if ( oldbitmapstate==bf_sfnt_dfont )
	oldformatstate = ff_none;

    if ( fmflags==-1 ) {
	/* Default to what we did last time */
    } else {
	if ( oldformatstate==ff_ttf && (fmflags&0x2000))
	    oldformatstate = ff_ttfsym;
	if ( oldformatstate<=ff_cffcid ) {
	    old_ps_flags = 0;
	    if ( fmflags&1 ) old_ps_flags |= ps_flag_afm;
	    if ( fmflags&2 ) old_ps_flags |= ps_flag_pfm;
	    if ( fmflags&0x10000 ) old_ps_flags |= ps_flag_tfm;
	    if ( fmflags&0x20000 ) old_ps_flags |= ps_flag_nohintsubs;
	    if ( fmflags&0x40000 ) old_ps_flags |= ps_flag_noflex;
	    if ( fmflags&0x80000 ) old_ps_flags |= ps_flag_nohints;
	    if ( fmflags&0x100000 ) old_ps_flags |= ps_flag_restrict256;
	    if ( fmflags&0x200000 ) old_ps_flags |= ps_flag_round;
	    if ( fmflags&0x400000 ) old_ps_flags |= ps_flag_afmwithmarks;
	    if ( i==bf_otb ) {
		old_sfnt_flags = 0;
		switch ( fmflags&0x90 ) {
		  case 0x80:
		    old_sfnt_flags |= ttf_flag_applemode|ttf_flag_otmode;
		  break;
		  case 0x90:
		    /* Neither */;
		  break;
		  case 0x10:
		    old_sfnt_flags |= ttf_flag_applemode;
		  break;
		  case 0x00:
		    old_sfnt_flags |= ttf_flag_otmode;
		  break;
		  default:
		  break;
		}
		if ( fmflags&4 ) old_sfnt_flags |= ttf_flag_shortps;
		if ( fmflags&0x20 ) old_sfnt_flags |= ttf_flag_pfed_comments;
		if ( fmflags&0x40 ) old_sfnt_flags |= ttf_flag_pfed_colors;
		if ( fmflags&0x200 ) old_sfnt_flags |= ttf_flag_TeXtable;
		if ( fmflags&0x400 ) old_sfnt_flags |= ttf_flag_ofm;
		if ( (fmflags&0x800) && !(old_sfnt_flags&ttf_flag_applemode) )
		    old_sfnt_flags |= ttf_flag_oldkern;
		if ( fmflags&0x2000 ) old_sfnt_flags |= ttf_flag_symbol;
		if ( fmflags&0x4000 ) old_sfnt_flags |= ttf_flag_dummyDSIG;
		if ( fmflags&0x800000 ) old_sfnt_flags |= ttf_flag_pfed_lookupnames;
		if ( fmflags&0x1000000 ) old_sfnt_flags |= ttf_flag_pfed_guides;
		if ( fmflags&0x2000000 ) old_sfnt_flags |= ttf_flag_pfed_layers;
		if ( fmflags&0x4000000 ) old_sfnt_flags |= ttf_flag_oldkernmappedonly;
	    }
	} else {
	    old_sfnt_flags = 0;
		/* Applicable postscript flags */
	    if ( fmflags&1 ) old_sfnt_flags |= ps_flag_afm;
	    if ( fmflags&2 ) old_sfnt_flags |= ps_flag_pfm;
	    if ( fmflags&0x20000 ) old_sfnt_flags |= ps_flag_nohintsubs;
	    if ( fmflags&0x40000 ) old_sfnt_flags |= ps_flag_noflex;
	    if ( fmflags&0x80000 ) old_sfnt_flags |= ps_flag_nohints;
	    if ( fmflags&0x200000 ) old_sfnt_flags |= ps_flag_round;
	    if ( fmflags&0x400000 ) old_sfnt_flags |= ps_flag_afmwithmarks;
		/* Applicable truetype flags */
	    switch ( fmflags&0x90 ) {
	      case 0x80:
		old_sfnt_flags |= ttf_flag_applemode|ttf_flag_otmode;
	      break;
	      case 0x90:
		/* Neither */;
	      break;
	      case 0x10:
		old_sfnt_flags |= ttf_flag_applemode;
	      break;
	      case 0x00:
		old_sfnt_flags |= ttf_flag_otmode;
	      break;
              default:
	      break;
	    }
	    if ( fmflags&4 ) old_sfnt_flags |= ttf_flag_shortps;
	    if ( fmflags&8 ) old_sfnt_flags |= ttf_flag_nohints;
	    if ( fmflags&0x20 ) old_sfnt_flags |= ttf_flag_pfed_comments;
	    if ( fmflags&0x40 ) old_sfnt_flags |= ttf_flag_pfed_colors;
	    if ( fmflags&0x100 ) old_sfnt_flags |= ttf_flag_glyphmap;
	    if ( fmflags&0x200 ) old_sfnt_flags |= ttf_flag_TeXtable;
	    if ( fmflags&0x400 ) old_sfnt_flags |= ttf_flag_ofm;
	    if ( (fmflags&0x800) && !(old_sfnt_flags&ttf_flag_applemode) )
		old_sfnt_flags |= ttf_flag_oldkern;
	    if ( fmflags&0x2000 ) old_sfnt_flags |= ttf_flag_symbol;
	    if ( fmflags&0x4000 ) old_sfnt_flags |= ttf_flag_dummyDSIG;
	    if ( fmflags&0x800000 ) old_sfnt_flags |= ttf_flag_pfed_lookupnames;
	    if ( fmflags&0x1000000 ) old_sfnt_flags |= ttf_flag_pfed_guides;
	    if ( fmflags&0x2000000 ) old_sfnt_flags |= ttf_flag_pfed_layers;
	    if ( fmflags&0x4000000 ) old_sfnt_flags |= ttf_flag_oldkernmappedonly;
	}
    }

    if ( oldbitmapstate!=bf_none ) {
	if ( sfs!=NULL ) {
	    for ( sfi=sfs; sfi!=NULL; sfi=sfi->next )
		sfi->sizes = AllBitmapSizes(sfi->sf);
	} else
	    sizes = AllBitmapSizes(sf);
    }

    former = NULL;
    if ( sfs!=NULL ) {
	for ( sfl=sfs; sfl!=NULL; sfl=sfl->next ) {
	    PrepareUnlinkRmOvrlp(sfl->sf,filename,layer);
	    if ( rename_to!=NULL )
		sfl->former_names = SFTemporaryRenameGlyphsToNamelist(sfl->sf,rename_to);
	}
    } else {
	PrepareUnlinkRmOvrlp(sf,filename,layer);
	if ( rename_to!=NULL )
	    former = SFTemporaryRenameGlyphsToNamelist(sf,rename_to);
    }

    if ( sfs!=NULL ) {
	int flags = 0;
	if ( oldformatstate<=ff_cffcid )
	    flags = old_ps_flags;
	else
	    flags = old_sfnt_flags;
	ret = WriteMacFamily(filename,sfs,oldformatstate,oldbitmapstate,flags,layer);
    } else {
	ret = !_DoSave(sf,filename,sizes,res,map,subfontdefinition,layer);
    }
    free(freeme);

    if ( sfs!=NULL ) {
	for ( sfl=sfs; sfl!=NULL; sfl=sfl->next ) {
	    RestoreUnlinkRmOvrlp(sfl->sf,filename,layer);
	    if ( rename_to!=NULL )
		SFTemporaryRestoreGlyphNames(sfl->sf,sfl->former_names);
	}
    } else {
	RestoreUnlinkRmOvrlp(sf,filename,layer);
	if ( rename_to!=NULL )
	    SFTemporaryRestoreGlyphNames(sf,former);
    }

    if ( oldbitmapstate!=bf_none ) {
	if ( sfs!=NULL ) {
	    for ( sfi=sfs; sfi!=NULL; sfi=sfi->next )
		free(sfi->sizes);
	}
    }
return( ret );
}
