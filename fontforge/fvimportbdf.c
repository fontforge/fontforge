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

#include "fvimportbdf.h"

#include "bitmapchar.h"
#include "bvedit.h"
#include "cvimages.h"
#include "encoding.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "macbinary.h"
#include "mem.h"
#include "namelist.h"
#include "palmfonts.h"
#include "parsettf.h"
#include "splinefill.h"
#include "splineutil.h"
#include "splineutil2.h"
#include <gfile.h>
#include <math.h>
#include "utype.h"
#include "ustring.h"
#include <chardata.h>
#include <unistd.h>

static char *cleancopy(const char *name) {
    const char *fpt;
    char *temp, *tpt;
    char buf[200];

    fpt=name;
    /* Look for some common cases */
    /* Often bdf fonts name their glyphs things like "%" or "90". Neither is */
    /*  a good postscript name, so do something reasonable here */
    if ( !isalpha(*(unsigned char *) fpt) && fpt[0]>=' ' && /* fpt[0]<=0x7f &&*/
	    fpt[1]=='\0' )
return( copy( StdGlyphName(buf,*(unsigned char *) fpt,ui_none,(NameList *) -1)) );
    tpt = temp = malloc(strlen(name)+2);
    if ( isdigit(*fpt))
	*tpt++ = '$';
    for ( ; *fpt; ++fpt ) {
	if ( *fpt>' ' && *fpt<127 &&
		*fpt!='(' &&
		*fpt!=')' &&
		*fpt!='[' &&
		*fpt!=']' &&
		*fpt!='{' &&
		*fpt!='}' &&
		*fpt!='<' &&
		*fpt!='>' &&
		*fpt!='/' &&
		*fpt!='%' )
	    *tpt++ = *fpt;
    }
    *tpt = '\0';

    if ( *name=='\0' ) {
	char buffer[20];
	static int unique = 0;
	sprintf( buffer, "$u%d", ++unique );
	free(temp);
return( copy( buffer ));
    }

    if ( temp!=NULL )
return( temp );

return( copy(name));
}

/* The pcf code is adapted from... */
/*

Copyright 1991, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/*
 * Author:  Keith Packard, MIT X Consortium
 */

/* ************************************************************************** */
void SFDefaultAscent(SplineFont *sf) {
    if ( sf->onlybitmaps ) {
	double scaled_sum=0, cnt=0;
	int em = sf->ascent+sf->descent;
	BDFFont *b;

	for ( b=sf->bitmaps; b!=NULL; b=b->next ) {
	    scaled_sum += (double) (em*b->ascent)/b->pixelsize;
	    ++cnt;
	}
	if ( cnt!=0 )
	sf->ascent = scaled_sum/cnt;
	sf->descent = em - sf->ascent;
    }
}

/* ******************************** BDF ************************************* */

static int gettoken(FILE *bdf, char *tokbuf, int size) {
    char *pt=tokbuf, *end = tokbuf+size-2; int ch;

    while ( isspace(ch = getc(bdf)));
    while ( ch!=EOF && !isspace(ch) /* && ch!='[' && ch!=']' && ch!='{' && ch!='}' && ch!='<' && ch!='%' */ ) {
	if ( pt<end ) *pt++ = ch;
	ch = getc(bdf);
    }
    if ( pt==tokbuf && ch!=EOF )
	*pt++ = ch;
    else
	ungetc(ch,bdf);
    *pt='\0';
return( pt!=tokbuf?1:ch==EOF?-1: 0 );
}

static void ExtendSF(SplineFont *sf, EncMap *map, int enc) {
    FontViewBase *fvs;

    if ( enc>=map->enccount ) {
	int n = enc;
	if ( enc>=map->encmax )
	    map->map = realloc(map->map,(map->encmax = n+100)*sizeof(int));
	memset(map->map+map->enccount,-1,(enc-map->enccount+1)*sizeof(int));
	map->enccount = n+1;
	if ( sf->fv!=NULL ) {
	    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
		free(fvs->selected);
		fvs->selected = calloc(map->enccount,1);
	    }
	    FontViewReformatAll(sf);
	}
    }
}

static SplineChar *MakeEncChar(SplineFont *sf,EncMap *map, int enc,const char *name) {
    int uni;
    SplineChar *sc;

    ExtendSF(sf,map,enc);

    sc = SFMakeChar(sf,map,enc);
    free(sc->name);
    sc->name = cleancopy(name);

    uni = UniFromName(name,sf->uni_interp,map->enc);
    if ( uni!=-1 )
	sc->unicodeenc = uni;
return( sc );
}

static int figureProperEncoding(SplineFont *sf,EncMap *map, BDFFont *b, int enc,
	const char *name, int swidth, int swidth1, Encoding *encname) {
    int i = -1, gid;

    if ( strcmp(name,".notdef")==0 ) {
	gid = ( enc>=map->enccount || enc<0 ) ? -1 : map->map[enc];
	if ( gid==-1 || sf->glyphs[gid]==NULL || strcmp(sf->glyphs[gid]->name,name)!=0 ) {
	    SplineChar *sc;
	    if ( enc==-1 ) {
		if ( (enc = SFFindSlot(sf,map,-1,name))==-1 )
		    enc = map->enccount;
	    }
	    MakeEncChar(sf,map,enc,name);
	    sc = SFMakeChar(sf,map,enc);
	    if ( sf->onlybitmaps || !sc->widthset ) {
		sc->width = swidth;
		sc->widthset = true;
		if ( swidth1!=-1 )
		    sc->vwidth = swidth1;
	    }
	}
    } else if ( map->enc==encname ||
	    (map->enc==&custom && sf->onlybitmaps)) {
	i = enc;
	if ( i==-1 ) {
	    if ( (i = SFFindSlot(sf,map,-1,name))==-1 )
		i = map->enccount;
	}
	if ( i>=map->enccount || map->map[i]==-1 )
	    MakeEncChar(sf,map,i,name);
    } else {
	int32 uni = UniFromEnc(enc,encname);
	if ( uni==-1 )
	    uni = UniFromName(name,sf->uni_interp,map->enc);
	i = EncFromUni(uni,map->enc);
	if ( i==-1 ) {
	    i = SFFindSlot(sf,map,uni,name);
	    if ( i==-1 && sf->onlybitmaps && enc!=-1 &&
		    ((sf->bitmaps==b && b->next==NULL) || sf->bitmaps==NULL) ) {
		MakeEncChar(sf,map,enc,name);
		i = enc;
	    }
	}
    }
    if ( i==-1 || i>=map->enccount ) {
	/* try adding it to the end of the font */
	int j;
	int encmax = map->enc->char_cnt;
	for ( j=map->enccount-1; j>=encmax &&
		((gid=map->map[j])==-1 || sf->glyphs[gid]==NULL); --j );
	++j;
	if ( i<j )
	    i = j;
	MakeEncChar(sf,map,i,name);
    }

    if ( i!=-1 && i<map->enccount && ((gid=map->map[i])==-1 || sf->glyphs[gid]==NULL )) {
	SplineChar *sc = SFMakeChar(sf,map,i);
	if ( sf->onlybitmaps && ((sf->bitmaps==b && b->next==NULL) || sf->bitmaps==NULL) ) {
	    free(sc->name);
	    sc->name = cleancopy(name);
	}
    }
    if ( i!=-1 && swidth!=-1 && (gid=map->map[i])!=-1 &&
	    ((sf->onlybitmaps && ((sf->bitmaps==b && b->next==NULL) || sf->bitmaps==NULL) ) ||
	     (sf->glyphs[gid]!=NULL && sf->glyphs[gid]->layers[ly_fore].splines==NULL && sf->glyphs[gid]->layers[ly_fore].refs==NULL &&
	       !sf->glyphs[gid]->widthset)) ) {
	sf->glyphs[gid]->width = swidth;
	sf->glyphs[gid]->widthset = true;
	if ( swidth1!=-1 )
	    sf->glyphs[gid]->vwidth = swidth1;
    }
    if ( i!=-1 && (gid=map->map[i])!=-1 ) {
	if ( gid>=b->glyphcnt ) {
	    if ( gid>=b->glyphmax )
		b->glyphs = realloc(b->glyphs,(b->glyphmax = sf->glyphmax)*sizeof(BDFChar *));
	    memset(b->glyphs+b->glyphcnt,0,(gid+1-b->glyphcnt)*sizeof(BDFChar *));
	    b->glyphcnt = gid+1;
	}
    }
return( i );
}

struct metrics {
    int swidth, dwidth, swidth1, dwidth1;	/* Font wide width defaults */
    int metricsset, vertical_origin;
    int res;
};

static void AddBDFChar(FILE *bdf, SplineFont *sf, BDFFont *b,EncMap *map,int depth,
	struct metrics *defs, Encoding *encname) {
    BDFChar *bc;
    char name[40], tok[100];
    int enc=-1, width=defs->dwidth, xmin=0, xmax=0, ymin=0, ymax=0, hsz, vsz;
    int swidth= defs->swidth, swidth1=defs->swidth1;
    int vwidth = defs->dwidth1;
    int i,ch;
    uint8 *pt, *end, *eol;

    gettoken(bdf,name,sizeof(tok));
    while ( gettoken(bdf,tok,sizeof(tok))!=-1 ) {
	if ( strcmp(tok,"ENCODING")==0 ) {
	    fscanf(bdf,"%d",&enc);
	    /* Adobe says that enc is value for Adobe Standard */
	    /* But people don't use it that way. Adobe also says that if */
	    /* there is no mapping in adobe standard the -1 may be followed */
	    /* by another value, the local encoding. */
	    if ( enc==-1 ) {
		ch = getc(bdf);
		if ( ch==' ' || ch=='\t' )
		    fscanf(bdf,"%d",&enc);
		else
		    ungetc(ch,bdf);
	    }
	    if ( enc<-1 ) enc = -1;
	} else if ( strcmp(tok,"DWIDTH")==0 )
	    fscanf(bdf,"%d %*d",&width);
	else if ( strcmp(tok,"DWIDTH1")==0 )
	    fscanf(bdf,"%d %*d",&vwidth);
	else if ( strcmp(tok,"SWIDTH")==0 )
	    fscanf(bdf,"%d %*d",&swidth);
	else if ( strcmp(tok,"SWIDTH1")==0 )
	    fscanf(bdf,"%d %*d",&swidth1);
	else if ( strcmp(tok,"BBX")==0 ) {
	    fscanf(bdf,"%d %d %d %d",&hsz, &vsz, &xmin, &ymin );
	    xmax = hsz+xmin-1;
	    ymax = vsz+ymin-1;
	} else if ( strcmp(tok,"BITMAP")==0 )
    break;
    }
    if (( xmax+1==xmin && (ymax+1==ymin || ymax==ymin)) ||
	    (( xmax+1==xmin || xmax==xmin) && ymax+1==ymin ))
	/* Empty glyph */;
    else if ( xmax<xmin || ymax<ymin ) {
	LogError( _("Bad bounding box for %s.\n"), name );
return;
    }
    i = figureProperEncoding(sf,map,b,enc,name,swidth,swidth1,encname);
    if ( i!=-1 ) {
	int gid = map->map[i];
	if ( (bc=b->glyphs[gid])!=NULL ) {
	    free(bc->bitmap);
	    BDFFloatFree(bc->selection);
	} else {
	    b->glyphs[gid] = bc = chunkalloc(sizeof(BDFChar));
	    memset( bc,'\0',sizeof( BDFChar ));
	    bc->sc = sf->glyphs[gid];
	    bc->orig_pos = gid;
	}
	bc->xmin = xmin;
	bc->ymin = ymin;
	bc->xmax = xmax;
	bc->ymax = ymax;
	bc->width = width;
	bc->vwidth = vwidth;
	if ( depth==1 ) {
	    bc->bytes_per_line = ((xmax-xmin)>>3) + 1;
	    bc->byte_data = false;
	} else {
	    bc->bytes_per_line = xmax-xmin + 1;
	    bc->byte_data = true;
	}
	bc->depth = depth;
	bc->bitmap = malloc(bc->bytes_per_line*(ymax-ymin+1));

	pt = bc->bitmap; end = pt + bc->bytes_per_line*(ymax-ymin+1);
	eol = pt + bc->bytes_per_line;
	while ( pt<end ) {
	    int ch1, ch2, val;
	    while ( isspace(ch1=getc(bdf)) );
	    ch2 = getc(bdf);
	    val = 0;
	    if ( ch1>='0' && ch1<='9' ) val = (ch1-'0')<<4;
	    else if ( ch1>='a' && ch1<='f' ) val = (ch1-'a'+10)<<4;
	    else if ( ch1>='A' && ch1<='F' ) val = (ch1-'A'+10)<<4;
	    if ( ch2>='0' && ch2<='9' ) val |= (ch2-'0');
	    else if ( ch2>='a' && ch2<='f' ) val |= (ch2-'a'+10);
	    else if ( ch2>='A' && ch2<='F' ) val |= (ch2-'A'+10);
	    if ( depth==1 || depth==8 )
		*pt++ = val;
	    else if ( depth==2 ) {		/* Internal representation is unpacked, one byte per pixel */
		*pt++ = (val>>6);
		if ( pt<eol ) *pt++ = (val>>4)&3;
		if ( pt<eol ) *pt++ = (val>>2)&3;
		if ( pt<eol ) *pt++ = val&3;
	    } else if ( depth==4 ) {
		*pt++ = (val>>4);
		if ( pt<eol ) *pt++ = val&0xf;
	    } else if ( depth==16 || depth==32 ) {
		int i,j;
		*pt++ = val;
		/* I only deal with 8 bit pixels, so if they give me more */
		/*  just ignore the low order bits */
		j = depth==16?2:6;
		for ( i=0; i<j; ++j )
		    ch2 = getc(bdf);
	    }
	    if ( pt>=eol ) {
		eol += bc->bytes_per_line;
		while ( (ch1 = getc(bdf)) != '\n')
		    /* empty loop body */ ;
	    }
	    if ( ch2==EOF )
	break;
	}
    } else {
	int cnt;
	if ( depth==1 )
	    cnt = 2*(((xmax-xmin)>>3) + 1) * (ymax-ymin+1);
	else if ( depth==2 )
	    cnt = 2*(((xmax-xmin)>>2) + 1) * (ymax-ymin+1);
	else if ( depth==4 )
	    cnt = (xmax-xmin + 1) * (ymax-ymin+1);
	else if ( depth==8 )
	    cnt = 2*(xmax-xmin + 1) * (ymax-ymin+1);
	else if ( depth==16 )
	    cnt = 4*(xmax-xmin + 1) * (ymax-ymin+1);
	else
	    cnt = 8*(xmax-xmin + 1) * (ymax-ymin+1);
	for ( i=0; i<cnt; ++i ) {
	    while ( isspace(getc(bdf)) );
	    getc(bdf);
	}
    }
}

static Encoding *BDFParseEnc(char *encname, int encoff) {
    Encoding *enc;
    char buffer[200];

    enc = NULL;
    if ( strmatch(encname,"ISO10646")==0 || strmatch(encname,"ISO-10646")==0 || strmatch(encname,"ISO_10646")==0 ||
	    strmatch(encname,"Unicode")==0 )
	enc = FindOrMakeEncoding("Unicode");
    if ( enc==NULL ) {
	sprintf( buffer, "%.150s-%d", encname, encoff );
	enc = FindOrMakeEncoding(buffer);
    }
    if ( enc==NULL && strmatch(encname,"ISOLatin1Encoding")==0 )
	enc = FindOrMakeEncoding("ISO8859-1");
    if ( enc==NULL )
	enc = FindOrMakeEncoding(encname);
    if ( enc==NULL )
	enc = &custom;
return( enc );
}

static int default_ascent_descent(int *_as, int *_ds, int ascent, int descent,
	int pixelsize, int pixel_size2, int point_size, int res,
	char *filename) {

    if ( pixelsize!=-1 && pixel_size2!=-1 && ascent!=-1 && descent!=-1 ) {
	if ( pixelsize!=pixel_size2 || pixelsize!=ascent+descent ) {
	    if ( pixelsize==pixel_size2 )
		descent = pixelsize - ascent;
	    else if ( pixel_size2==ascent+descent )
		pixelsize = pixel_size2;
	    else if ( pixelsize==ascent+descent )
		;
	    else {
		pixelsize = rint( (pixelsize+pixel_size2+ascent+descent)/3.0 );
		descent = pixelsize-ascent;
	    }
	    LogError(_("Various specifications of PIXEL_SIZE do not match in %s"), filename );
	}
    } else if ( pixelsize!=-1 && ascent!=-1 && descent!=-1 ) {
	if ( pixelsize!=ascent+descent ) {
	    ascent = 8*pixelsize/10;
	    descent = pixelsize-ascent;
	}
    } else {
	if ( pixelsize==-1 )
	    pixelsize = pixel_size2;
	if ( (ascent==-1) + (descent==-1) + (pixelsize==-1)>=2 &&
		res!=-1 && point_size!=-1 )
	    pixelsize = rint( point_size*res/720.0 );
	if ( pixelsize==-1 && ascent!=-1 && descent!=-1 )
	    pixelsize = ascent+descent;
	else if ( pixelsize!=-1 ) {
	    if ( ascent==-1 && descent!=-1 )
		ascent = pixelsize - descent;
	    else if ( ascent!=-1 )
		descent = pixelsize -ascent;
	}
	if ( ascent!=-1 && descent!=-1 && pixelsize!=-1 ) {
	    if ( pixelsize!=ascent+descent )
		LogError(_("Pixel size does not match sum of Font ascent+descent in %s"), filename );
	} else if ( pixelsize!=-1 ) {
	    ascent = rint( (8*pixelsize)/10.0 );
	    descent = pixelsize - ascent;
	} else if ( ascent!=-1 ) {
	    LogError(_("Guessing pixel size based on font ascent in %s"), filename );
	    descent = ascent/4;
	    pixelsize = ascent+descent;
	} else if ( descent!=-1 ) {
	    LogError(_("Guessing pixel size based on font descent in %s"), filename );
	    ascent = 4*descent;
	    pixelsize = ascent+descent;
	} else {
	    /* No info at all. We'll ask the user later */
	}
    }
    *_as = ascent; *_ds=descent;
return( pixelsize );
}

static int slurp_header(FILE *bdf, int *_as, int *_ds, Encoding **_enc,
	char *family, char *mods, char *full, int *depth, char *foundry,
	char *fontname, char *comments, struct metrics *defs,
	int *upos, int *uwidth, BDFFont *dummy,
	char *filename) {
    int pixelsize = -1, pixel_size2= -1, point_size = -1;
    int ascent= -1, descent= -1, enc, cnt;
    char tok[100], encname[100], weight[100], italic[100], buffer[300], *buf;
    int found_copyright=0;
    int inprops=false;
    int pcnt=0, pmax=0;

    *depth = 1;
    encname[0]= '\0'; family[0] = '\0'; weight[0]='\0'; italic[0]='\0'; full[0]='\0';
    foundry[0]= '\0'; comments[0] = '\0';
    while ( gettoken(bdf,tok,sizeof(tok))!=-1 ) {
	if ( strcmp(tok,"CHARS")==0 ) {
	    cnt=0;
	    fscanf(bdf,"%d",&cnt);
	    ff_progress_change_total(cnt);
    break;
	}
	if ( strcmp(tok,"STARTPROPERTIES")==0 ) {
	    int cnt;
	    inprops = true;
	    fscanf(bdf, "%d", &cnt );
	    if ( pcnt+cnt>=pmax )
		dummy->props = realloc(dummy->props,(pmax=pcnt+cnt)*sizeof(BDFProperties));
	    /* But it isn't a property itself */
    continue;
	} else if ( strcmp(tok,"ENDPROPERTIES")==0 ) {
	    inprops = false;
    continue;
	}
	fgets(buffer,sizeof(buffer),bdf );
	buf = buffer;
	{
	    int val;
	    char *end, *eol;

	    if ( pcnt>=pmax )
		dummy->props = realloc(dummy->props,(pmax=pcnt+10)*sizeof(BDFProperties));
	    dummy->props[pcnt].name = copy(tok);
	    while ( *buf==' ' || *buf=='\t' ) ++buf;
	    for ( eol=buf+strlen(buf)-1; eol>=buf && isspace(*eol); --eol);
	    eol[1] ='\0';
	    val = strtol(buf,&end,10);
	    if ( *end=='\0' && buf<=eol ) {
		dummy->props[pcnt].u.val = val;
		dummy->props[pcnt].type = IsUnsignedBDFKey(tok)?prt_uint:prt_int;
	    } else if ( *buf=='"' ) {
		++buf;
		if ( *eol=='"' ) *eol = '\0';
		dummy->props[pcnt].u.str = copy(buf);
		dummy->props[pcnt].type = prt_string;
	    } else {
		dummy->props[pcnt].u.atom = copy(buf);
		dummy->props[pcnt].type = prt_atom;
	    }
	    if ( inprops )
		dummy->props[pcnt].type |= prt_property;
	    ++pcnt;
	}

	if ( strcmp(tok,"FONT")==0 ) {
	    if ( sscanf(buf,"-%*[^-]-%99[^-]-%99[^-]-%99[^-]-%*[^-]-", family, weight, italic )!=0 ) {
		char *pt=buf;
		int dcnt=0;
		while ( *pt=='-' && dcnt<7 ) { ++pt; ++dcnt; }
		while ( *pt!='-' && *pt!='\n' && *pt!='\0' && dcnt<7 ) {
		    while ( *pt!='-' && *pt!='\n' && *pt!='\0' ) ++pt;
		    while ( *pt=='-' && dcnt<7 ) { ++pt; ++dcnt; }
		}
		sscanf(pt,"%d", &pixelsize );
		if ( pixelsize<0 ) pixelsize = -pixelsize;	/* An extra - screwed things up once... */
		while ( *pt!='-' && *pt!='\n' && *pt!='\0' ) ++pt;
		if ( *pt=='-' ) {
		    sscanf(++pt,"%d", &point_size );
		    if ( point_size<0 ) point_size = -point_size;
		}
	    } else {
		if ( *buf!='\0' && !isdigit(*buf))
		    strcpy(family,buf);
	    }
	} else if ( strcmp(tok,"SIZE")==0 ) {
	    int junk;
	    sscanf(buf, "%d %d %d %d", &point_size, &junk, &defs->res, depth );
	    if ( pixelsize==-1 )
		pixelsize = rint( point_size*defs->res/72.0 );
	} else if ( strcmp(tok,"BITSPERPIXEL")==0 ||
		strcmp(tok,"BITS_PER_PIXEL")==0 ) {
	    sscanf(buf, "%d", depth);
	} else if ( strcmp(tok,"QUAD_WIDTH")==0 && pixelsize==-1 )
	    sscanf(buf, "%d", &pixelsize );
	    /* For Courier the quad is not an em */
	else if ( strcmp(tok,"RESOLUTION_Y")==0 )	/* y value defines pointsize */
	    sscanf(buf, "%d", &defs->res );
	else if ( strcmp(tok,"POINT_SIZE")==0 )
	    sscanf(buf, "%d", &point_size );
	else if ( strcmp(tok,"PIXEL_SIZE")==0 )
	    sscanf(buf, "%d", &pixel_size2 );
	else if ( strcmp(tok,"FONT_ASCENT")==0 )
	    sscanf(buf, "%d", &ascent );
	else if ( strcmp(tok,"FONT_DESCENT")==0 )
	    sscanf(buf, "%d", &descent );
	else if ( strcmp(tok,"UNDERLINE_POSITION")==0 )
	    sscanf(buf, "%d", upos );
	else if ( strcmp(tok,"UNDERLINE_THICKNESS")==0 )
	    sscanf(buf, "%d", uwidth );
	else if ( strcmp(tok,"SWIDTH")==0 )
	    sscanf(buf, "%d", &defs->swidth );
	else if ( strcmp(tok,"SWIDTH1")==0 )
	    sscanf(buf, "%d", &defs->swidth1 );
	else if ( strcmp(tok,"DWIDTH")==0 )
	    sscanf(buf, "%d", &defs->dwidth );
	else if ( strcmp(tok,"DWIDTH1")==0 )
	    sscanf(buf, "%d", &defs->dwidth1 );
	else if ( strcmp(tok,"METRICSSET")==0 )
	    sscanf(buf, "%d", &defs->metricsset );
	else if ( strcmp(tok,"VVECTOR")==0 )
	    sscanf(buf, "%*d %d", &defs->vertical_origin );
	/* For foundry, fontname and encname, only copy up to the buffer size */
	else if ( strcmp(tok,"FOUNDRY")==0 )
	    sscanf(buf, "%99[^\"]", foundry );
	else if ( strcmp(tok,"FONT_NAME")==0 )
	    sscanf(buf, "%99[^\"]", fontname );
	else if ( strcmp(tok,"CHARSET_REGISTRY")==0 )
	    sscanf(buf, "%99[^\"]", encname );
	else if ( strcmp(tok,"CHARSET_ENCODING")==0 ) {
	    enc = 0;
	    if ( sscanf(buf, " %d", &enc )!=1 )
		sscanf(buf, "%d", &enc );
	/* These properties should be copied up to the buffer length too */
	} else if ( strcmp(tok,"FAMILY_NAME")==0 ) {
	    strncpy(family,buf,99);
	    family[99]='\0';
	} else if ( strcmp(tok,"FULL_NAME")==0 || strcmp(tok,"FACE_NAME")==0 ) {
	    strncpy(full,buf,99);
	    full[99]='\0';
	} else if ( strcmp(tok,"WEIGHT_NAME")==0 ) {
	    strncpy(weight,buf,99);
	    weight[99]='\0';
	} else if ( strcmp(tok,"SLANT")==0 ) {
	    strncpy(italic,buf,99);
	    italic[99]='\0';
	} else if ( strcmp(tok,"COPYRIGHT")==0 ) {
		/* LS: Assume the size of the passed-in buffer is 1000, see below in
		 * COMMENT... */
	    strncpy(comments,buf,999);
	    comments[999]='\0';
	    found_copyright = true;
	} else if ( strcmp(tok,"COMMENT")==0 && !found_copyright ) {
	    char *pt = comments+strlen(comments);
	    char *eoc = comments+1000-1; /* ...here */
	    strncpy(pt,buf,eoc-pt);
	    *eoc ='\0';
	}
    }
    dummy->prop_cnt = pcnt;

    pixelsize = default_ascent_descent(_as, _ds, ascent, descent, pixelsize,
	    pixel_size2, point_size, defs->res, filename );

    *_enc = BDFParseEnc(encname,enc);

    if ( strmatch(italic,"I")==0 )
	strcpy(italic,"Italic");
    else if ( strmatch(italic,"O")==0 )
	strcpy(italic,"Oblique");
    else if ( strmatch(italic,"R")==0 )
	strcpy(italic,"");		/* Ignore roman */
    sprintf(mods,"%s%s", weight, italic );
    if ( comments[0]!='\0' && comments[strlen(comments)-1]=='\n' )
	comments[strlen(comments)-1] = '\0';

    if ( *depth!=1 && *depth!=2 && *depth!=4 && *depth!=8 && *depth!=16 && *depth!=32 )
	LogError( _("FontForge does not support this bit depth %d (must be 1,2,4,8,16,32)\n"), *depth);

return( pixelsize );
}

/* ******************************** GF (TeX) ******************************** */
enum gf_cmd { gf_paint_0=0, gf_paint_1=1, gf_paint_63=63,
	gf_paint1b=64, gf_paint2b, gf_paint3b,	/* followed by 1,2,or3 bytes saying how far to paint */
	gf_boc=67, /* 4bytes of character code, 4bytes of -1, min_col 4, min_row 4, max_col 4, max_row 4 */
	    /* set row=max_row, col=min_col, paint=white */
	gf_boc1=68, /* 1 byte quantities, and no -1 field, cc, width, max_col, height, max_row */
	gf_eoc=69,
	gf_skip0=70,	/* start next row (don't need to complete current) as white*/
	gf_skip1=71, gf_skip2=72, gf_skip3=73, /* followed by 1,2,3 bytes saying number rows to skip (leave white) */
	gf_newrow_0=74,	/* Same as skip, start drawing with black */
	gf_newrow_1=75, gf_newrow_164=238,
	gf_xxx1=239, /* followed by a 1 byte count, and then that many bytes of ignored data */
	gf_xxx2, gf_xxx3, gf_xxx4,
	gf_yyy=243, /* followed by 4 bytes of numeric data */
	gf_no_op=244,
	gf_char_loc=245, /* 1?byte encoding, 4byte dx, 4byte dy (<<16), 4byte advance width/design size <<24, 4byte offset to character's boc */
	gf_char_loc0=246, /* 1?byte encoding, 1byte dx, 4byte advance width/design size <<24, 4byte offset to character's boc */
	gf_pre=247, /* one byte containing gf_version_number, one byte count, that many bytes data */
	gf_post=248,	/* start of post amble */
	gf_post_post=249,	/* End of postamble, 4byte offset to start of preamble, 1byte gf_version_number, 4 or more bytes of 0xdf */
	/* rest 250..255 are undefined */
	gf_version_number=131 };
/* The postamble command is followed by 9 4byte quantities */
/*  the first is an offset to a list of xxx commands which I'm ignoring */
/*  the second is the design size */
/*  the third is a check sum */
/*  the fourth is horizontal pixels per point<<16 */
/*  the fifth is vertical pixels per point<<16 */
/*  the sixth..ninth give the font bounding box */
/* Then a bunch of char_locs (with back pointers to the characters and advance width info) */
/* Then a post_post with a back pointer to start of postamble + gf_version */
/* Then 4-7 bytes of 0xdf */

static BDFChar *SFGrowTo(SplineFont *sf,BDFFont *b, int cc, EncMap *map) {
    char buf[20];
    int gid;
    BDFChar *bc;

    if ( cc >= map->enccount ) {
	if ( cc>=map->encmax ) {
	    int new = ((map->enccount+256)>>8)<<8;
	    if ( new<cc+1 ) new = cc+1;
	    map->map = realloc(map->map,new*sizeof(int));
	    map->encmax = new;
	}
	memset(map->map+map->enccount,-1,(cc+1-map->enccount)*sizeof(int));
	map->enccount = cc+1;
    }
    if ( (gid = map->map[cc])==-1 || sf->glyphs[gid]==NULL )
	gid = SFMakeChar(sf,map,cc)->orig_pos;
    if ( sf->onlybitmaps && ((sf->bitmaps==b && b->next==NULL) || sf->bitmaps==NULL) ) {
	free(sf->glyphs[gid]->name);
	sprintf( buf, "enc-%d", cc);
	sf->glyphs[gid]->name = cleancopy( buf );
	sf->glyphs[gid]->unicodeenc = -1;
    }
    if ( b->glyphcnt<sf->glyphcnt ) {
	if ( b->glyphmax<sf->glyphcnt )
	    b->glyphs = realloc(b->glyphs,(b->glyphmax = sf->glyphmax)*sizeof(BDFChar *));
	memset(b->glyphs+b->glyphcnt,0,(sf->glyphcnt-b->glyphcnt)*sizeof(BDFChar *));
	b->glyphcnt = sf->glyphcnt;
    }
    if ( (bc=b->glyphs[gid])!=NULL ) {
	free(bc->bitmap);
	BDFFloatFree(bc->selection);
    } else {
	b->glyphs[gid] = bc = chunkalloc(sizeof(BDFChar));
	memset( bc,'\0',sizeof( BDFChar ));
	bc->sc = sf->glyphs[gid];
	bc->orig_pos = gid;
    }
return(bc);
}

static void gf_skip_noops(FILE *gf,char *char_name) {
    uint8 cmd;
    int32 val;
    int i;
    char buffer[257];

    if ( char_name ) *char_name = '\0';

    while ( 1 ) {
	cmd = getc(gf);
	switch( cmd ) {
	  case gf_no_op:		/* One byte no-op */
	  break;
	  case gf_yyy:			/* followed by a 4 byte value */
	    getc(gf); getc(gf); getc(gf); getc(gf);
	  break;
	  case gf_xxx1:
	    val = getc(gf);
	    for ( i=0; i<val; ++i ) buffer[i] = getc(gf);
	    buffer[i] = 0;
	    if (strncmp(buffer,"title",5)==0 && char_name!=NULL ) {
		char *pt = buffer+6, *to = char_name;
		while ( *pt ) {
		    if ( *pt=='(' ) {
			while ( *pt!=')' && *pt!='\0' ) ++pt;
		    } else if ( *pt==' ' || *pt==')' ) {
			if ( to==char_name || to[-1]!='-' )
			    *to++ = '-';
			++pt;
		    } else
			*to++ = *pt++;
		}
		if ( to!=char_name && to[-1]=='-' )
		    --to;
		*to = '\0';
	    }
	  break;
	  case gf_xxx2:
	    val = getc(gf);
	    val = (val<<8) | getc(gf);
	    for ( i=0; i<val; ++i ) getc(gf);
	  break;
	  case gf_xxx3:
	    val = getc(gf);
	    val = (val<<8) | getc(gf);
	    val = (val<<8) | getc(gf);
	    for ( i=0; i<val; ++i ) getc(gf);
	  break;
	  case gf_xxx4:
	    val = getlong(gf);
	    for ( i=0; i<val; ++i ) getc(gf);
	  break;
	  default:
	    ungetc(cmd,gf);
return;
	}
    }
}

static int gf_postamble(FILE *gf, int *_as, int *_ds, Encoding **_enc, char *family,
	char *mods, char *full, char *filename) {
    int pixelsize=-1;
    int ch;
    int design_size, pixels_per_point;
    double size;
    char *pt, *fpt;
    int32 pos, off;

    fseek(gf,-4,SEEK_END);
    pos = ftell(gf);
    ch = getc(gf);
    if ( ch!=0xdf )
return( -2 );
    while ( ch==0xdf ) {
	--pos;
	fseek(gf,-2,SEEK_CUR);
	ch = getc(gf);
    }
    if ( ch!=gf_version_number )
return( -2 );
    pos -= 4;
    fseek(gf,pos,SEEK_SET);
    off = getlong(gf);
    fseek(gf,off,SEEK_SET);
    ch = getc(gf);
    if ( ch!=gf_post )
return( -2 );
    /* offset to comments */ getlong(gf);
    design_size = getlong(gf);
    /* checksum = */ getlong(gf);
    pixels_per_point = getlong(gf);
    /* vert pixels per point = */ getlong(gf);
    /* min_col = */ getlong(gf);
    /* min_row = */ getlong(gf);
    /* max_col = */ getlong(gf);
    /* max_row = */ getlong(gf);

    size = (pixels_per_point / (double) (0x10000)) * (design_size / (double) (0x100000));
    pixelsize = size+.5;
    *_enc = &custom;
    *_as = *_ds = -1;
    *mods = '\0';
    pt = strrchr(filename, '/');
    if ( pt==NULL ) pt = filename; else ++pt;
    for ( fpt=family; isalpha(*pt);)
	*fpt++ = *pt++;
    *fpt = '\0';
    strcpy(full,family);

return( pixelsize );
}

static int gf_char(FILE *gf, SplineFont *sf, BDFFont *b,EncMap *map) {
    int32 pos, to;
    int ch, enc, dx, aw;
    int min_c, max_c, min_r, max_r, w;
    int r,c, col,cnt,i;
    int gid;
    BDFChar *bc;
    char charname[256];

    /* We assume we are positioned within the postamble. we read one char_loc */
    /*  then read the char it points to, then return to postamble */
    ch = getc(gf);
    if ( ch== gf_char_loc ) {
	enc = getc(gf);
	dx = getlong(gf)>>16;
	/* dy */ (void)(getlong(gf)>>16);
	aw = (getlong(gf)*(sf->ascent+sf->descent))>>20;
	to = getlong(gf);
    } else if ( ch==gf_char_loc0 ) {
	enc = getc(gf);
	dx = getc(gf);
	aw = (getlong(gf)*(sf->ascent+sf->descent))>>20;
	to = getlong(gf);
    } else
return( false );
    pos = ftell(gf);
    fseek(gf,to,SEEK_SET);

    gf_skip_noops(gf,charname);
    ch = getc(gf);
    if ( ch==gf_boc ) {
	/* encoding = */ getlong(gf);
	/* backpointer = */ getlong(gf);
	min_c = getlong(gf);
	max_c = getlong(gf);
	min_r = getlong(gf);
	max_r = getlong(gf);
    } else if ( ch==gf_boc1 ) {
	/* encoding = */ getc(gf);
	w = getc(gf);
	max_c = getc(gf);
	min_c = max_c-w+1;
	w = getc(gf);
	max_r = getc(gf);
	min_r = max_r-w+1;
    } else
return( false );

    bc = SFGrowTo(sf,b,enc,map);
    gid = map->map[enc];
    if ( charname[0]!='\0' && sf->onlybitmaps && (sf->bitmaps==NULL ||
	    (sf->bitmaps==b && b->next==NULL )) ) {
	free(sf->glyphs[gid]->name);
	sf->glyphs[gid]->name = cleancopy(charname);
	sf->glyphs[gid]->unicodeenc = -1;
    }
    bc->xmin = min_c;
    bc->xmax = max_c>min_c? max_c : min_c;
    bc->ymin = min_r;
    bc->ymax = max_r>min_r? max_r : min_r;
    bc->width = dx;
    bc->vwidth = b->pixelsize;
    bc->bytes_per_line = ((bc->xmax-bc->xmin+8)>>3);
    bc->bitmap = calloc(bc->bytes_per_line*(bc->ymax-bc->ymin+1),1);

    if ( sf->glyphs[gid]->layers[ly_fore].splines==NULL && sf->glyphs[gid]->layers[ly_fore].refs==NULL &&
	    !sf->glyphs[gid]->widthset ) {
	sf->glyphs[gid]->width = aw;
	sf->glyphs[gid]->widthset = true;
    }

    for ( r=min_r, c=min_c, col=0; r<=max_r; ) {
	gf_skip_noops(gf,NULL);
	ch = getc(gf);
	if ( ch==gf_eoc )
    break;
	if ( ch>=gf_paint_0 && ch<=gf_paint3b ) {
	    if ( ch>=gf_paint_0 && ch<=gf_paint_63 )
		cnt = ch-gf_paint_0;
	    else if ( ch==gf_paint1b )
		cnt = getc(gf);
	    else if ( ch==gf_paint2b )
		cnt = getushort(gf);
	    else
		cnt = get3byte(gf);
	    if ( col ) {
		for ( i=0; i<cnt && c<=max_c; ++i ) {
		    bc->bitmap[(r-min_r)*bc->bytes_per_line+((c-min_c)>>3)]
			    |= (0x80>>((c-min_c)&7));
		    ++c;
		    /*if ( c>max_c ) { c-=max_c-min_c; ++r; }*/
		}
	    } else {
		c+=cnt;
		/*while ( c>max_c ) { c-=max_c-min_c; ++r; }*/
	    }
	    col = !col;
	} else if ( ch>=gf_newrow_0 && ch<=gf_newrow_164 ) {
	    ++r;
	    c = min_c + ch-gf_newrow_0;
	    col = 1;
	} else if ( ch>=gf_skip0 && ch<=gf_skip3 ) {
	    col = 0;
	    c = min_c;
	    if ( ch==gf_skip0 )
		++r;
	    else if ( ch==gf_skip1 )
		r += getc(gf)+1;
	    else if ( ch==gf_skip2 )
		r += getushort(gf)+1;
	    else
		r += get3byte(gf)+1;
	} else if ( ch==EOF ) {
	    LogError( _("Unexpected EOF in gf\n") );
    break;
	} else
	    LogError( _("Uninterpreted code in gf: %d\n"), ch);
    }
    fseek(gf,pos,SEEK_SET);
return( true );
}

/* ******************************** PK (TeX) ******************************** */

enum pk_cmd { pk_rrr1=240, pk_rrr2, pk_rrr3, pk_rrr4, pk_yyy, pk_post, pk_no_op,
	pk_pre, pk_version_number=89 };
static void pk_skip_noops(FILE *pk) {
    uint8 cmd;
    int32 val;
    int i;

    while ( 1 ) {
	cmd = getc(pk);
	switch( cmd ) {
	  case pk_no_op:		/* One byte no-op */
	  break;
	  case pk_post:			/* Signals start of noop section */
	  break;
	  case pk_yyy:			/* followed by a 4 byte value */
	    getc(pk); getc(pk); getc(pk); getc(pk);
	  break;
	  case pk_rrr1:
	    val = getc(pk);
	    for ( i=0; i<val; ++i ) getc(pk);
	  break;
	  case pk_rrr2:
	    val = getc(pk);
	    val = (val<<8) | getc(pk);
	    for ( i=0; i<val; ++i ) getc(pk);
	  break;
	  case pk_rrr3:
	    val = getc(pk);
	    val = (val<<8) | getc(pk);
	    val = (val<<8) | getc(pk);
	    for ( i=0; i<val; ++i ) getc(pk);
	  break;
	  case pk_rrr4:
	    val = getc(pk);
	    val = (val<<8) | getc(pk);
	    val = (val<<8) | getc(pk);
	    val = (val<<8) | getc(pk);
	    for ( i=0; i<val; ++i ) getc(pk);
	  break;
	  default:
	    ungetc(cmd,pk);
return;
	}
    }
}

static int pk_header(FILE *pk, int *_as, int *_ds, Encoding **_enc, char *family,
	char *mods, char *full, char *filename) {
    int pixelsize=-1;
    int ch,i;
    int design_size, pixels_per_point;
    double size;
    char *pt, *fpt;

    pk_skip_noops(pk);
    ch = getc(pk);
    if ( ch!=pk_pre )
return( -2 );
    ch = getc(pk);
    if ( ch!=pk_version_number )
return( -2 );
    ch = getc(pk);
    for ( i=0; i<ch; ++i ) getc(pk);		/* Skip comment. Perhaps that should be the family? */
    design_size = getlong(pk);
    /* checksum = */ getlong(pk);
    pixels_per_point = getlong(pk);
    /* vert pixels per point = */ getlong(pk);

    size = (pixels_per_point / 65536.0) * (design_size / (double) (0x100000));
    pixelsize = size+.5;
    *_enc = &custom;
    *_as = *_ds = -1;
    *mods = '\0';
    pt = strrchr(filename, '/');
    if ( pt==NULL ) pt = filename; else ++pt;
    for ( fpt=family; isalpha(*pt);)
	*fpt++ = *pt++;
    *fpt = '\0';
    strcpy(full,family);

return( pixelsize );
}

struct pkstate {
    int byte, hold;
    int rpt;
    int dyn_f;
    int cc;		/* for debug purposes */
};

static int pkgetcount(FILE *pk, struct pkstate *st) {
    int i,j;
#define getnibble(pk,st) (st->hold==1?(st->hold=0,(st->byte&0xf)):(st->hold=1,(((st->byte=getc(pk))>>4))) )

    while ( 1 ) {
	i = getnibble(pk,st);
	if ( i==0 ) {
	    j=0;
	    while ( i==0 ) { ++j; i=getnibble(pk,st); }
	    while ( j>0 ) { --j; i = (i<<4) + getnibble(pk,st); }
return( i-15 + (13-st->dyn_f)*16 + st->dyn_f );
	} else if ( i<=st->dyn_f ) {
return( i );
	} else if ( i<14 ) {
return( (i-st->dyn_f-1)*16 + getnibble(pk,st) + st->dyn_f + 1 );
	} else {
	    if ( st->rpt!=0 )
		LogError( _("Duplicate repeat row count in char %d of pk file\n"), st->cc );
	    if ( i==15 ) st->rpt = 1;
	    else st->rpt = pkgetcount(pk,st);
 /*printf( "[%d]", st->rpt );*/
	}
    }
}

static int pk_char(FILE *pk, SplineFont *sf, BDFFont *b, EncMap *map) {
    int flag = getc(pk);
    int black;
    int pl, cc, tfm, w, h, hoff, voff, dm, dx;
    int i, ch, j,r,c,cnt;
    int gid;
    BDFChar *bc;
    struct pkstate st;
    int32 char_end;

    memset(&st,'\0', sizeof(st));

    /* flag byte */
    st.dyn_f = (flag>>4);
    if ( st.dyn_f==15 ) {
	ungetc(flag,pk);
return( 0 );
    }
    black = flag&8 ? 1 : 0;

    if ( (flag&7)==7 ) {		/* long preamble, 4 byte sizes */
	pl = getlong(pk);
	cc = getlong(pk);
	char_end = ftell(pk) + pl;
	tfm = getlong(pk);
	dx = getlong(pk)>>16;
        /* dy */ (void)(getlong(pk)>>16);
	w = getlong(pk);
	h = getlong(pk);
	hoff = getlong(pk);
	voff = getlong(pk);
    } else if ( flag & 4 ) {		/* extended preamble, 2 byte sizes */
	pl = getushort(pk) + ((flag&3)<<16);
	cc = getc(pk);
	char_end = ftell(pk) + pl;
	tfm = get3byte(pk);
	dm = getushort(pk);
	dx = dm;
	w = getushort(pk);
	h = getushort(pk);
	hoff = (short) getushort(pk);
	voff = (short) getushort(pk);
    } else {				/* short, 1 byte sizes */
	pl = getc(pk) + ((flag&3)<<8);
	cc = getc(pk);
	char_end = ftell(pk) + pl;
	tfm = get3byte(pk);
	dm = getc(pk);
	dx = dm;
	w = getc(pk);
	h = getc(pk);
	hoff = (signed char) getc(pk);
	voff = (signed char) getc(pk);
    }
    st.cc = cc;			/* We can give better errors with this in st */
    /* hoff is -xmin, voff is ymax */
    /* w,h is the width,height of the bounding box */
    /* dx is the advance width? */
    /* cc is the character code */
    /* tfm is the advance width as a fraction of an em/2^20 so multiply by 1000/2^20 to get postscript values */
    /* dy is the advance height for vertical text? */

    bc = SFGrowTo(sf,b,cc,map);
    gid = map->map[cc];

    bc->xmin = -hoff;
    bc->ymax = voff;
    bc->xmax = w-1-hoff;
    bc->ymin = voff-h+1;
    bc->width = dx;
    bc->vwidth = b->pixelsize;
    bc->bytes_per_line = ((w+7)>>3);
    bc->bitmap = calloc(bc->bytes_per_line*h,1);

    if ( sf->glyphs[gid]->layers[ly_fore].splines==NULL && sf->glyphs[gid]->layers[ly_fore].refs==NULL &&
	    !sf->glyphs[gid]->widthset ) {
	sf->glyphs[gid]->width = (sf->ascent+sf->descent)*(double) tfm/(0x100000);
	sf->glyphs[gid]->widthset = true;
    }

    if ( w==0 && h==0 )
	/* Nothing */;
    else if ( st.dyn_f==14 ) {
	/* We've got raster data in the file */
	for ( i=0; i<((w*h+7)>>3); ++i ) {
	    ch = getc(pk);
	    for ( j=0; j<8; ++j ) {
		r = ((i<<3)+j)/w;
		c = ((i<<3)+j)%w;
		if ( r<h && (ch&(1<<(7-j))) )
		    bc->bitmap[r*bc->bytes_per_line+(c>>3)] |= (1<<(7-(c&7)));
	    }
	    if ( ftell(pk)>char_end )
	break;
	}
    } else {
	/* We've got run-length encoded data */
	r = c = 0;
	while ( r<h ) {
	    cnt = pkgetcount(pk,&st);
  /* if ( black ) printf( "%d", cnt ); else printf( "(%d)", cnt );*/
	    if ( c+cnt>=w && st.rpt!=0 ) {
		if ( black ) {
		    while ( c<w ) {
			bc->bitmap[r*bc->bytes_per_line+(c>>3)] |= (1<<(7-(c&7)));
			--cnt;
			++c;
		    }
		} else
		    cnt -= (w-c);
		for ( i=0; i<st.rpt && r+i+1<h; ++i )
		    memcpy(bc->bitmap+(r+i+1)*bc->bytes_per_line,
			    bc->bitmap+r*bc->bytes_per_line,
			    bc->bytes_per_line );
		r += st.rpt+1;
		c = 0;
		st.rpt = 0;
	    }
	    while ( cnt>0 && r<h ) {
		while ( c<w && cnt>0) {
		    if ( black )
			bc->bitmap[r*bc->bytes_per_line+(c>>3)] |= (1<<(7-(c&7)));
		    --cnt;
		    ++c;
		}
		if ( c==w ) {
		    c = 0;
		    ++r;
		}
	    }
	    black = !black;
	    if ( ftell(pk)>char_end )
	break;
	}
    }
    if ( ftell(pk)!=char_end ) {
	LogError(_("The character, %d, was not read properly (or pk file is in bad format)\n At %ld should be %d, off by %ld\n"), cc, ftell(pk), char_end, ftell(pk)-char_end );
	fseek(pk,char_end,SEEK_SET);
    }
 /* printf( "\n" ); */
return( 1 );
}

/* ****************************** PCF *************************************** */

struct toc {
    int type;
    int format;
    int size;		/* in 32bit words */
    int offset;
};

#define PCF_FILE_VERSION	(('p'<<24)|('c'<<16)|('f'<<8)|1)

    /* table types */
#define PCF_PROPERTIES		    (1<<0)
#define PCF_ACCELERATORS	    (1<<1)
#define PCF_METRICS		    (1<<2)
#define PCF_BITMAPS		    (1<<3)
#define PCF_INK_METRICS		    (1<<4)
#define	PCF_BDF_ENCODINGS	    (1<<5)
#define PCF_SWIDTHS		    (1<<6)
#define PCF_GLYPH_NAMES		    (1<<7)
#define PCF_BDF_ACCELERATORS	    (1<<8)

    /* formats */
#define PCF_DEFAULT_FORMAT	0x00000000
#define PCF_INKBOUNDS		0x00000200
#define PCF_ACCEL_W_INKBOUNDS	0x00000100
#define PCF_COMPRESSED_METRICS	0x00000100

#define PCF_GLYPH_PAD_MASK	(3<<0)
#define PCF_BYTE_MASK		(1<<2)
#define PCF_BIT_MASK		(1<<3)
#define PCF_SCAN_UNIT_MASK	(3<<4)

#define	PCF_FORMAT_MASK		0xffffff00
#define PCF_FORMAT_MATCH(a,b) (((a)&PCF_FORMAT_MASK) == ((b)&PCF_FORMAT_MASK))

#define MSBFirst	1
#define LSBFirst	0

#define PCF_BYTE_ORDER(f)	(((f) & PCF_BYTE_MASK)?MSBFirst:LSBFirst)
#define PCF_BIT_ORDER(f)	(((f) & PCF_BIT_MASK)?MSBFirst:LSBFirst)
#define PCF_GLYPH_PAD_INDEX(f)	((f) & PCF_GLYPH_PAD_MASK)
#define PCF_GLYPH_PAD(f)	(1<<PCF_GLYPH_PAD_INDEX(f))
#define PCF_SCAN_UNIT_INDEX(f)	(((f) & PCF_SCAN_UNIT_MASK) >> 4)
#define PCF_SCAN_UNIT(f)	(1<<PCF_SCAN_UNIT_INDEX(f))

#define GLYPHPADOPTIONS	4

struct pcfmetrics {
    short lsb;
    short rsb;
    short width;
    short ascent;
    short descent;
    short attrs;
};

struct pcfaccel {
    unsigned int noOverlap:1;
    unsigned int constantMetrics:1;
    unsigned int terminalFont:1;
    unsigned int constantWidth:1;
    unsigned int inkInside:1;
    unsigned int inkMetrics:1;
    unsigned int drawDirection:1;
    int ascent;
    int descent;
    int maxOverlap;
    struct pcfmetrics minbounds;
    struct pcfmetrics maxbounds;
    struct pcfmetrics ink_minbounds;
    struct pcfmetrics ink_maxbounds;
};
/* End declarations */

/* Code based on Keith Packard's pcfread.c in the X11 distribution */

static int getint32(FILE *file) {
    int val = getc(file);
    val |= (getc(file)<<8);
    val |= (getc(file)<<16);
    val |= (getc(file)<<24);
return( val );
}

static int getformint32(FILE *file,int format) {
    int val;
    if ( format&PCF_BYTE_MASK ) {
	val = getc(file);
	val = (val<<8) | getc(file);
	val = (val<<8) | getc(file);
	val = (val<<8) | getc(file);
    } else {
	val = getc(file);
	val |= (getc(file)<<8);
	val |= (getc(file)<<16);
	val |= (getc(file)<<24);
    }
return( val );
}

static int getformint16(FILE *file,int format) {
    int val;
    if ( format&PCF_BYTE_MASK ) {
	val = getc(file);
	val = (val<<8) | getc(file);
    } else {
	val = getc(file);
	val |= (getc(file)<<8);
    }
return( val );
}

static struct toc *pcfReadTOC(FILE *file) {
    int cnt, i;
    struct toc *toc;

    if ( getint32(file)!=PCF_FILE_VERSION )
return( NULL );
    cnt = getint32(file);
    toc = calloc(cnt+1,sizeof(struct toc));
    for ( i=0; i<cnt; ++i ) {
	toc[i].type = getint32(file);
	toc[i].format = getint32(file);
	toc[i].size = getint32(file);
	toc[i].offset = getint32(file);
    }

return( toc );
}

static int pcfSeekToType(FILE *file, struct toc *toc, int type) {
    int i;

    for ( i=0; toc[i].type!=0 && toc[i].type!=type; ++i );
    if ( toc[i].type==0 )
return( false );
    fseek(file,toc[i].offset,SEEK_SET);
return( true );
}

static void pcfGetMetrics(FILE *file,int compressed,int format,struct pcfmetrics *metric) {
    if ( compressed ) {
	metric->lsb = getc(file)-0x80;
	metric->rsb = getc(file)-0x80;
	metric->width = getc(file)-0x80;
	metric->ascent = getc(file)-0x80;
	metric->descent = getc(file)-0x80;
	metric->attrs = 0;
    } else {
	metric->lsb = getformint16(file,format);
	metric->rsb = getformint16(file,format);
	metric->width = getformint16(file,format);
	metric->ascent = getformint16(file,format);
	metric->descent = getformint16(file,format);
	metric->attrs = getformint16(file,format);
    }
}

static int pcfGetAccel(FILE *file, struct toc *toc,int which, struct pcfaccel *accel) {
    int format;

    if ( !pcfSeekToType(file,toc,which))
return(false);
    format = getint32(file);
    if ( (format&PCF_FORMAT_MASK)!=PCF_DEFAULT_FORMAT &&
	    (format&PCF_FORMAT_MASK)!=PCF_ACCEL_W_INKBOUNDS )
return(false);
    accel->noOverlap = getc(file);
    accel->constantMetrics = getc(file);
    accel->terminalFont = getc(file);
    accel->constantWidth = getc(file);
    accel->inkInside = getc(file);
    accel->inkMetrics = getc(file);
    accel->drawDirection = getc(file);
    /* padding = */ getc(file);
    accel->ascent = getformint32(file,format);
    accel->descent = getformint32(file,format);
    accel->maxOverlap = getformint32(file,format);
    pcfGetMetrics(file,false,format,&accel->minbounds);
    pcfGetMetrics(file,false,format,&accel->maxbounds);
    if ( (format&PCF_FORMAT_MASK)==PCF_ACCEL_W_INKBOUNDS ) {
	pcfGetMetrics(file,false,format,&accel->ink_minbounds);
	pcfGetMetrics(file,false,format,&accel->ink_maxbounds);
    } else {
	accel->ink_minbounds = accel->minbounds;
	accel->ink_maxbounds = accel->maxbounds;
    }
return( true );
}

static int pcf_properties(FILE *file,struct toc *toc, int *_as, int *_ds,
	Encoding **_enc, char *family, char *mods, char *full, BDFFont *dummy,
	char *filename) {
    int pixelsize = -1, point_size = -1, res = -1;
    int ascent= -1, descent= -1, enc=0;
    char encname[101], weight[101], italic[101];
    int cnt, i, format, strl, dash_cnt;
    struct props { int name_offset; int isStr; int val; char *name; char *value; } *props;
    char *strs, *pt;

    family[0] = '\0'; full[0] = '\0';
    encname[0] = encname[100] = '\0';
    weight[0] = weight[100] = '\0';
    italic[0] = italic[100] = '\0';
    if ( !pcfSeekToType(file,toc,PCF_PROPERTIES))
return(-2);
    format = getint32(file);
    if ( (format&PCF_FORMAT_MASK)!=PCF_DEFAULT_FORMAT )
return(-2);
    cnt = getformint32(file,format);
    props = malloc(cnt*sizeof(struct props));
    for ( i=0; i<cnt; ++i ) {
	props[i].name_offset = getformint32(file,format);
	props[i].isStr = getc(file);
	props[i].val = getformint32(file,format);
    }
    if ( cnt&3 )
	fseek(file,4-(cnt&3),SEEK_CUR);
    strl = getformint32(file,format);
    strs = malloc(strl+1);
    strs[strl]=0;
    fread(strs,1,strl,file);
    for ( i=0; i<cnt; ++i ) {
	props[i].name = strs+props[i].name_offset;
	if ( props[i].isStr )
	    props[i].value = strs+props[i].val;
	else
	    props[i].value = NULL;
    }

    /* the properties here are almost exactly the same as the bdf ones */
    /* except that FONT is a pcf property, and SIZE etc. aren't mentioned */

    dummy->prop_cnt = cnt;
    dummy->props = malloc(cnt*sizeof(BDFProperties));

    for ( i=0; i<cnt; ++i ) {
	dummy->props[i].name = copy(props[i].name);
	if ( props[i].isStr ) {
	    dummy->props[i].u.str = copy(props[i].value);
	    dummy->props[i].type  = prt_string | prt_property;
	    if ( strcmp(props[i].name,"FAMILY_NAME")==0 )
		strcpy(family,props[i].value);
	    else if ( strcmp(props[i].name,"WEIGHT_NAME")==0 )
		strncpy(weight,props[i].value,sizeof(weight)-1);
	    else if ( strcmp(props[i].name,"FULL_NAME")==0 )
		strcpy(full,props[i].value);
	    else if ( strcmp(props[i].name,"SLANT")==0 )
		strncpy(italic,props[i].value,sizeof(italic)-1);
	    else if ( strcmp(props[i].name,"CHARSET_REGISTRY")==0 )
		strncpy(encname,props[i].value,sizeof(encname)-1);
	    else if ( strcmp(props[i].name,"CHARSET_ENCODING")==0 )
		enc = strtol(props[i].value,NULL,10);
	    else if ( strcmp(props[i].name,"FONT")==0 ) {
		dummy->props[i].type = prt_atom;
		if ( sscanf(props[i].value,"-%*[^-]-%[^-]-%[^-]-%[^-]-%*[^-]-",
			family, weight, italic )!=0 ) {
		    for ( pt = props[i].value, dash_cnt=0; *pt && dash_cnt<7; ++pt )
			if ( *pt=='-' ) ++dash_cnt;
		    if ( dash_cnt==7 && isdigit(*pt) )
			pixelsize = strtol(pt,NULL,10);
		}
	    }
	} else {
	    dummy->props[i].u.val = props[i].val;
	    dummy->props[i].type  = (IsUnsignedBDFKey(dummy->props[i].name)?prt_uint:prt_int) | prt_property;
	    if ( strcmp(props[i].name,"PIXEL_SIZE")==0 ||
		    ( pixelsize==-1 && strcmp(props[i].name,"QUAD_WIDTH")==0 ))
		pixelsize = props[i].val;
	    else if ( strcmp(props[i].name,"FONT_ASCENT")==0 )
		ascent = props[i].val;
	    else if ( strcmp(props[i].name,"FONT_DESCENT")==0 )
		descent = props[i].val;
	    else if ( strcmp(props[i].name,"POINT_SIZE")==0 )
		point_size = props[i].val;
	    else if ( strcmp(props[i].name,"RESOLUTION_Y")==0 )
		res = props[i].val;
	    else if ( strcmp(props[i].name,"CHARSET_ENCODING")==0 )
		enc = props[i].val;
	}
    }
    if ( ascent==-1 || descent==-1 ) {
	struct pcfaccel accel;
	if ( pcfGetAccel(file,toc,PCF_BDF_ACCELERATORS,&accel) ||
		pcfGetAccel(file,toc,PCF_ACCELERATORS,&accel)) {
	    ascent = accel.ascent;
	    descent = accel.descent;
	    if ( pixelsize==-1 )
		pixelsize = ascent+descent;
	}
    }
    pixelsize = default_ascent_descent(_as, _ds, ascent, descent, pixelsize,
	    -1, point_size, res, filename );

    *_enc = BDFParseEnc(encname,enc);

    if ( strmatch(italic,"I")==0 )
	strcpy(italic,"Italic");
    else if ( strmatch(italic,"O")==0 )
	strcpy(italic,"Oblique");
    else if ( strmatch(italic,"R")==0 )
	strcpy(italic,"");		/* Ignore roman */
    sprintf(mods,"%s%s", weight, italic );
    if ( full[0]=='\0' ) {
	if ( *mods )
	    sprintf(full,"%s-%s", family, mods );
	else
	    strcpy(full,family);
    }

    free(strs);
    free(props);

return( pixelsize );
}

static struct pcfmetrics *pcfGetMetricsTable(FILE *file, struct toc *toc,int which, int *metrics_cnt) {
    int format, cnt, i;
    struct pcfmetrics *metrics;

    if ( !pcfSeekToType(file,toc,which))
return(NULL);
    format = getint32(file);
    if ( (format&PCF_FORMAT_MASK)!=PCF_DEFAULT_FORMAT &&
	    (format&PCF_FORMAT_MASK)!=PCF_COMPRESSED_METRICS )
return(NULL);
    if ( (format&PCF_FORMAT_MASK)==PCF_COMPRESSED_METRICS ) {
	cnt = getformint16(file,format);
	metrics = malloc(cnt*sizeof(struct pcfmetrics));
	for ( i=0; i<cnt; ++i )
	    pcfGetMetrics(file,true,format,&metrics[i]);
    } else {
	cnt = getformint32(file,format);
	metrics = malloc(cnt*sizeof(struct pcfmetrics));
	for ( i=0; i<cnt; ++i )
	    pcfGetMetrics(file,false,format,&metrics[i]);
    }
    *metrics_cnt = cnt;
return( metrics );
}

static uint8 bitinvert[] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static void BitOrderInvert(uint8 *bitmap,int sizebitmaps) {
    int i;

    for ( i=0; i<sizebitmaps; ++i )
	bitmap[i] = bitinvert[bitmap[i]];
}

static void TwoByteSwap(uint8 *bitmap,int sizebitmaps) {
    int i, t;

    for ( i=0; i<sizebitmaps-1; i+=2 ) {
	t = bitmap[i];
	bitmap[i] = bitmap[i+1];
	bitmap[i+1] = t;
    }
}

static void FourByteSwap(uint8 *bitmap,int sizebitmaps) {
    int i, t;

    for ( i=0; i<sizebitmaps-1; i+=2 ) {
	t = bitmap[i];
	bitmap[i] = bitmap[i+3];
	bitmap[i+3] = t;
	t = bitmap[i+1];
	bitmap[i+1] = bitmap[i+2];
	bitmap[i+2] = t;
    }
}

static int PcfReadBitmaps(FILE *file,struct toc *toc,BDFFont *b) {
    int format, cnt, i, sizebitmaps, j;
    int *offsets;
    uint8 *bitmap;
    int bitmapSizes[GLYPHPADOPTIONS];

    if ( !pcfSeekToType(file,toc,PCF_BITMAPS))
return(false);
    format = getint32(file);
    if ( (format&PCF_FORMAT_MASK)!=PCF_DEFAULT_FORMAT )
return(false);

    cnt = getformint32(file,format);
    if ( cnt!=b->glyphcnt )
return( false );
    offsets = malloc(cnt*sizeof(int));
    for ( i=0; i<cnt; ++i )
	offsets[i] = getformint32(file,format);
    for ( i=0; i<GLYPHPADOPTIONS; ++i )
	bitmapSizes[i] = getformint32(file, format);
    sizebitmaps = bitmapSizes[PCF_GLYPH_PAD_INDEX(format)];
    bitmap = malloc(sizebitmaps==0 ? 1 : sizebitmaps );
    fread(bitmap,1,sizebitmaps,file);
    if (PCF_BIT_ORDER(format) != MSBFirst )
	BitOrderInvert(bitmap,sizebitmaps);
    if ( PCF_SCAN_UNIT(format)==1 )
	/* Nothing to do */;
    else if ( PCF_BYTE_ORDER(format)==MSBFirst )
	/* Nothing to do */;
    else if ( PCF_SCAN_UNIT(format)==2 )
	TwoByteSwap(bitmap, sizebitmaps);
    else if ( PCF_SCAN_UNIT(format)==2 )
	FourByteSwap(bitmap, sizebitmaps);
    if ( PCF_GLYPH_PAD(format)==1 ) {
	for ( i=0; i<cnt; ++i ) {
	    BDFChar *bc = b->glyphs[i];
	    if ( i<cnt-1 && offsets[i+1]-offsets[i]!=bc->bytes_per_line * (bc->ymax-bc->ymin+1))
		IError("Bad PCF glyph bitmap size");
	    memcpy(bc->bitmap,bitmap+offsets[i],
		    bc->bytes_per_line * (bc->ymax-bc->ymin+1));
	    ff_progress_next();
	}
    } else {
	int pad = PCF_GLYPH_PAD(format);
	for ( i=0; i<cnt; ++i ) {
	    BDFChar *bc = b->glyphs[i];
	    int bpl = ((bc->bytes_per_line+pad-1)/pad)*pad;
	    for ( j=bc->ymin; j<=bc->ymax; ++j )
		memcpy(bc->bitmap+(j-bc->ymin)*bc->bytes_per_line,
			bitmap+offsets[i]+(j-bc->ymin)*bpl,
			bc->bytes_per_line);
	    ff_progress_next();
	}
    }
    free(bitmap);
    free(offsets);
return( true );
}

static void PcfReadEncodingsNames(FILE *file,struct toc *toc,SplineFont *sf,
	EncMap *map, BDFFont *b, Encoding *encname) {
    int format, cnt, i, stringsize;
    int *offsets=NULL;
    char *string=NULL;
    int *encs;

    encs = malloc(b->glyphcnt*sizeof(int));
    memset(encs,-1,b->glyphcnt*sizeof(int));

    if ( pcfSeekToType(file,toc,PCF_GLYPH_NAMES) &&
	    ((format = getint32(file))&PCF_FORMAT_MASK)==PCF_DEFAULT_FORMAT &&
	    (cnt = getformint32(file,format))==b->glyphcnt ) {
	offsets = malloc(cnt*sizeof(int));
	for ( i=0; i<cnt; ++i )
	    offsets[i] = getformint32(file,format);
	stringsize = getformint32(file,format);
	string = malloc(stringsize);
	fread(string,1,stringsize,file);
    }
    if ( pcfSeekToType(file,toc,PCF_BDF_ENCODINGS) &&
	    ((format = getint32(file))&PCF_FORMAT_MASK)==PCF_DEFAULT_FORMAT ) {
	int min2, max2, min1, max1, tot, glyph;
	min2 = getformint16(file,format);
	max2 = getformint16(file,format);
	min1 = getformint16(file,format);
	max1 = getformint16(file,format);
	/* def */(void)getformint16(file,format);
	tot = (max2-min2+1)*(max1-min1+1);
	for ( i=0; i<tot; ++i ) {
	    glyph = getformint16(file,format);
	    if ( glyph!=0xffff && glyph<b->glyphcnt ) {
		encs[glyph] = (i/(max2-min2+1) + min2)*256 +
					(i%(max2-min2+1) + min1);
	    }
	}
    }
    cnt = b->glyphcnt;
    for ( i=0; i<cnt; ++i ) {
	const char *name;
	if ( string!=NULL ) name = string+offsets[i];
	else name = ".notdef";
	encs[i] = figureProperEncoding(sf,map,b,encs[i],name,-1,-1,encname);
	if ( encs[i]!=-1 ) {
	    int gid = map->map[encs[i]];
	    if ( gid!=-1 ) {
		b->glyphs[i]->sc = sf->glyphs[gid];
		b->glyphs[i]->orig_pos = gid;
	    }
	}
    }
    free(string); free(offsets); free(encs);
}

static int PcfReadSWidths(FILE *file,struct toc *toc,BDFFont *b) {
    int format, cnt, i;

    if ( !pcfSeekToType(file,toc,PCF_SWIDTHS))
return(false);
    format = getint32(file);
    if ( (format&PCF_FORMAT_MASK)!=PCF_DEFAULT_FORMAT )
return(false);

    cnt = getformint32(file,format);
    if ( cnt>b->glyphcnt )
return( false );
    for ( i=0; i<cnt; ++i ) {
	int swidth = getformint32(file,format);
	if ( b->glyphs[i]->sc!=NULL ) {
	    b->glyphs[i]->sc->width = swidth;
	    b->glyphs[i]->sc->widthset = true;
	}
    }
return( true );
}

static int PcfParse(FILE *file,struct toc *toc,SplineFont *sf,EncMap *map, BDFFont *b,
	Encoding *encname) {
    int metrics_cnt;
    struct pcfmetrics *metrics = pcfGetMetricsTable(file,toc,PCF_METRICS,&metrics_cnt);
    int mcnt = metrics_cnt;
    BDFChar **new, **mult;
    int i, multcnt;

    if ( metrics==NULL )
return( false );
    b->glyphcnt = b->glyphmax = mcnt;
    free(b->glyphs);
    b->glyphs = calloc(mcnt,sizeof(BDFChar *));
    for ( i=0; i<mcnt; ++i ) {
	BDFChar *bc = b->glyphs[i] = chunkalloc(sizeof(BDFChar));
	memset( bc,'\0',sizeof( BDFChar ));
	bc->xmin = metrics[i].lsb;
	bc->xmax = metrics[i].rsb-1;
	if ( metrics[i].rsb==0 ) bc->xmax = 0;
	bc->ymin = -metrics[i].descent;
	bc->ymax = metrics[i].ascent-1;
	if ( bc->ymax<bc->ymin || bc->xmax<bc->xmin ) {
	    bc->ymax = bc->ymin-1;
	    bc->xmax = bc->xmin-1;
	}
	/*if ( metrics[i].ascent==0 ) bc->ymax = 0;*/ /*??*/
	bc->width = metrics[i].width;
	bc->vwidth = b->pixelsize;	/* pcf doesn't support vmetrics */
	bc->bytes_per_line = ((bc->xmax-bc->xmin)>>3) + 1;
	bc->bitmap = malloc(bc->bytes_per_line*(bc->ymax-bc->ymin+1));
	bc->orig_pos = -1;
    }
    free(metrics);

    if ( !PcfReadBitmaps(file,toc,b))
return( false );
    PcfReadEncodingsNames(file,toc,sf,map,b,encname);
    if ( sf->onlybitmaps )
	PcfReadSWidths(file,toc,b);
    new = calloc(sf->glyphcnt,sizeof(BDFChar *));
    mult = calloc(mcnt+1,sizeof(BDFChar *)); multcnt=0;
    for ( i=0; i<mcnt; ++i ) {
	BDFChar *bc = b->glyphs[i];
	if ( bc->orig_pos==-1 || bc->orig_pos>=sf->glyphcnt )
	    BDFCharFree(bc);
	else if ( new[bc->orig_pos]==NULL )
	    new[bc->orig_pos] = bc;
	else
	    mult[multcnt++] = bc;
    }
    if ( multcnt!=0 ) {
	for ( multcnt=0; mult[multcnt]!=NULL; multcnt++ ) {
	    for ( i=0; i<sf->glyphcnt; ++i )
		if ( new[i]==NULL ) {
		    new[i] = mult[multcnt];
	    break;
		}
	}
    }
    free( b->glyphs );
    free( mult );
    b->glyphs = new;
    b->glyphcnt = b->glyphmax = sf->glyphcnt;
return( true );
}

/* ************************* End Bitmap Formats ***************************** */

static int askusersize(char *filename) {
    char *pt;
    int guess;
    char *ret, *end;
    char def[10];

    for ( pt=filename; *pt && !isdigit(*pt); ++pt );
    guess = strtol(pt,NULL,10);
    if ( guess!=0 )
	sprintf(def,"%d",guess);
    else
	*def = '\0';
  retry:
    ret = ff_ask_string(_("Pixel size:"),def,_("What is the pixel size of the font in this file?"));
    if ( ret==NULL )
	guess = -1;
    else {
	guess = strtol(ret,&end,10);
	free(ret);
	if ( guess<=0 || *end!='\0' ) {
	    ff_post_error(_("Bad Number"),_("Bad Number"));
  goto retry;
	}
    }
return( guess );
}

static int alreadyexists(int pixelsize) {
    int ret;
    char *buts[3];
    buts[0] = _("_OK");
    buts[1] = _("_Cancel");
    buts[2] = NULL;

    ret = ff_ask(_("Duplicate pixelsize"),(const char **) buts,0,1,
	_("The font database already contains a bitmap\012font with this pixelsize (%d)\012Do you want to overwrite it?"),
	pixelsize);

return( ret==0 );
}

static void BDFForceEnc(SplineFont *sf, EncMap *map) {
/* jisx0208, jisx0212, ISO10646, ISO8859 */
    int i;
    BDFFont *bdf = sf->bitmaps;
    static struct bdf_2_ff_enc { const char *bdf, *ff; } bdf_2_ff_enc[] = {
	/* A map between bdf encoding names and my internal ones */
	{ "iso10646", "unicode" },
	{ "unicode", "unicode" },
	{ "jisx0208", "jis208" },
	{ "jisx0212", "jis212" },
	{ "jisx0201", "jis201" },
	{ NULL, NULL }
    };
    char *fn, *pt;
    Encoding *enc;

    for ( i=0; i<bdf->prop_cnt; ++i )
	if ( strcmp("FONT",bdf->props[i].name)==0 )
    break;
    if ( i==bdf->prop_cnt || (bdf->props[i].type&~prt_property)==prt_int ||
	    (bdf->props[i].type&~prt_property)==prt_uint )
return;
    fn = bdf->props[i].u.str;
    for ( pt = fn+strlen(fn)-1; pt>fn && *pt!='-'; --pt );
    for ( --pt; pt>fn && *pt!='-'; --pt );
    enc = NULL;
    if ( pt>fn )
	enc = FindOrMakeEncoding(pt+1);
    if ( enc==NULL ) {
	for ( i=0; bdf_2_ff_enc[i].bdf!=NULL; ++i )
	    if ( strstrmatch(fn,bdf_2_ff_enc[i].bdf)!=NULL ) {
		enc = FindOrMakeEncoding(bdf_2_ff_enc[i].ff);
	break;
	    }
    }
    if ( enc!=NULL )
	SFForceEncoding(sf,map,enc);
}

void SFSetFontName(SplineFont *sf, char *family, char *mods,char *fullname) {
    char *full;
    char *n;
    char *pt, *tpt;

    n = malloc(strlen(family)+strlen(mods)+2);
    strcpy(n,family); strcat(n," "); strcat(n,mods);
    if ( fullname==NULL || *fullname == '\0' )
	full = copy(n);
    else
	full = copy(fullname);
    free(sf->fullname); sf->fullname = full;

    for ( pt=tpt=n; *pt; ) {
	if ( !isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';

    /* In the URW world fontnames aren't just a simple concatenation of */
    /*  family name and modifiers, so neither the family name nor the modifiers */
    /*  changed, then don't change the font name */
    if ( strcmp(family,sf->familyname)==0 && strcmp(n,sf->fontname)==0 )
	/* Don't change the fontname */
	/* or anything else */
	free(n);
    else {
	free(sf->fontname); sf->fontname = n;
	free(sf->familyname); sf->familyname = copy(family);
	free(sf->weight); sf->weight = NULL;
	if ( strstrmatch(mods,"extralight")!=NULL || strstrmatch(mods,"extra-light")!=NULL )
	    sf->weight = copy("ExtraLight");
	else if ( strstrmatch(mods,"demilight")!=NULL || strstrmatch(mods,"demi-light")!=NULL )
	    sf->weight = copy("DemiLight");
	else if ( strstrmatch(mods,"demibold")!=NULL || strstrmatch(mods,"demi-bold")!=NULL )
	    sf->weight = copy("DemiBold");
	else if ( strstrmatch(mods,"semibold")!=NULL || strstrmatch(mods,"semi-bold")!=NULL )
	    sf->weight = copy("SemiBold");
	else if ( strstrmatch(mods,"demiblack")!=NULL || strstrmatch(mods,"demi-black")!=NULL )
	    sf->weight = copy("DemiBlack");
	else if ( strstrmatch(mods,"extrabold")!=NULL || strstrmatch(mods,"extra-bold")!=NULL )
	    sf->weight = copy("ExtraBold");
	else if ( strstrmatch(mods,"extrablack")!=NULL || strstrmatch(mods,"extra-black")!=NULL )
	    sf->weight = copy("ExtraBlack");
	else if ( strstrmatch(mods,"book")!=NULL )
	    sf->weight = copy("Book");
	else if ( strstrmatch(mods,"regular")!=NULL )
	    sf->weight = copy("Regular");
	else if ( strstrmatch(mods,"roman")!=NULL )
	    sf->weight = copy("Roman");
	else if ( strstrmatch(mods,"normal")!=NULL )
	    sf->weight = copy("Normal");
	else if ( strstrmatch(mods,"demi")!=NULL )
	    sf->weight = copy("Demi");
	else if ( strstrmatch(mods,"medium")!=NULL )
	    sf->weight = copy("Medium");
	else if ( strstrmatch(mods,"bold")!=NULL )
	    sf->weight = copy("Bold");
	else if ( strstrmatch(mods,"heavy")!=NULL )
	    sf->weight = copy("Heavy");
	else if ( strstrmatch(mods,"black")!=NULL )
	    sf->weight = copy("Black");
	else if ( strstrmatch(mods,"Nord")!=NULL )
	    sf->weight = copy("Nord");
/* Sigh. URW uses 4 letter abreviations... */
	else if ( strstrmatch(mods,"Regu")!=NULL )
	    sf->weight = copy("Regular");
	else if ( strstrmatch(mods,"Medi")!=NULL )
	    sf->weight = copy("Medium");
	else if ( strstrmatch(mods,"blac")!=NULL )
	    sf->weight = copy("Black");
	else
	    sf->weight = copy("Medium");
    }

    FVSetTitles(sf);
}

static BDFFont *SFImportBDF(SplineFont *sf, char *filename,int ispk, int toback,
	EncMap *map) {
    FILE *bdf;
    char tok[100];
    int pixelsize, ascent, descent;
    Encoding *enc;
    BDFFont *b;
    char family[100], mods[200], full[300], foundry[100], comments[1000], fontname[300];
    struct toc *toc=NULL;
    int depth=1;
    struct metrics defs;
    unsigned upos= (int) 0x80000000, uwidth = (int) 0x80000000;
    BDFFont dummy;
    int ch;

    memset(&dummy,0,sizeof(dummy));

    defs.swidth = defs.swidth1 = -1; defs.dwidth=defs.dwidth1=0;
    defs.metricsset = 0; defs.vertical_origin = 0;
    defs.res = -1;
    foundry[0] = '\0';
    fontname[0] = '\0';
    comments[0] = '\0';

    if ( ispk==1 && strcmp(filename+strlen(filename)-2,"gf")==0 )
	ispk = 3;
    bdf = fopen(filename,"rb");
    if ( bdf==NULL ) {
	ff_post_error(_("Couldn't open file"), _("Couldn't open file %.200s"), filename );
return( NULL );
    }
    if ( ispk==1 ) {
	pixelsize = pk_header(bdf,&ascent,&descent,&enc,family,mods,full, filename);
	if ( pixelsize==-2 ) {
	    fclose(bdf);
	    ff_post_error(_("Not a pk file"), _("Not a (metafont) pk file %.200s"), filename );
return( NULL );
	}
    } else if ( ispk==3 ) {		/* gf */
	pixelsize = gf_postamble(bdf,&ascent,&descent,&enc,family,mods,full, filename);
	if ( pixelsize==-2 ) {
	    fclose(bdf);
	    ff_post_error(_("Not a gf file"), _("Not a (metafont) gf file %.200s"), filename );
return( NULL );
	}
    } else if ( ispk==2 ) {		/* pcf */
	if (( toc = pcfReadTOC(bdf))== NULL ) {
	    fclose(bdf);
	    ff_post_error(_("Not a pcf file"), _("Not an X11 pcf file %.200s"), filename );
return( NULL );
	}
	pixelsize = pcf_properties(bdf,toc,&ascent,&descent,&enc,family,mods,full,&dummy, filename);
	if ( pixelsize==-2 ) {
	    fclose(bdf); free(toc);
	    ff_post_error(_("Not a pcf file"), _("Not an X11 pcf file %.200s"), filename );
return( NULL );
	}
    } else {
	if ( gettoken(bdf,tok,sizeof(tok))==-1 || strcmp(tok,"STARTFONT")!=0 ) {
	    fclose(bdf);
	    ff_post_error(_("Not a bdf file"), _("Not a bdf file %.200s"), filename );
return( NULL );
	}
	while ( (ch=getc(bdf))!='\n' && ch!='\r' && ch!=EOF );
	pixelsize = slurp_header(bdf,&ascent,&descent,&enc,family,mods,full,
		&depth,foundry,fontname,comments,&defs,&upos,&uwidth,&dummy,filename);
	if ( defs.dwidth == 0 ) defs.dwidth = pixelsize;
	if ( defs.dwidth1 == 0 ) defs.dwidth1 = pixelsize;
    }
    if ( pixelsize==-1 )
	pixelsize = askusersize(filename);
    if ( pixelsize==-1 ) {
	fclose(bdf); free(toc);
return( NULL );
    }
    if ( /* !toback && */ sf->bitmaps==NULL && sf->onlybitmaps ) {
	/* Loading first bitmap into onlybitmap font sets the name and encoding */
	SFSetFontName(sf,family,mods,full);
	if ( fontname[0]!='\0' ) {
	    free(sf->fontname);
	    sf->fontname = copy(fontname);
	}
	map->enc = enc;
	if ( defs.metricsset!=0 ) {
	    sf->hasvmetrics = true;
	    /*sf->vertical_origin = defs.vertical_origin==0?sf->ascent:defs.vertical_origin;*/
	}
	sf->display_size = pixelsize;
	if ( comments[0]!='\0' ) {
	    free(sf->copyright);
	    sf->copyright = copy(comments);
	}
	if ( upos!=0x80000000 )
	    sf->upos = upos;
	if ( uwidth!=0x80000000 )
	    sf->upos = uwidth;
    }

    b = NULL;
    if ( !toback )
	for ( b=sf->bitmaps; b!=NULL && (b->pixelsize!=pixelsize || BDFDepth(b)!=depth); b=b->next );
    if ( b!=NULL ) {
	if ( !alreadyexists(pixelsize)) {
	    fclose(bdf); free(toc);
	    BDFPropsFree(&dummy);
return( (BDFFont *) -1 );
	}
    }
    if ( b==NULL ) {
	if ( ascent==-1 && descent==-1 )
	    ascent = rint(pixelsize*sf->ascent/(real) (sf->ascent+sf->descent));
	if ( ascent==-1 && descent!=-1 )
	    ascent = pixelsize - descent;
	else if ( ascent!=-1 )
	    descent = pixelsize -ascent;
	b = calloc(1,sizeof(BDFFont));
	b->sf = sf;
	b->glyphcnt = b->glyphmax = sf->glyphcnt;
	b->pixelsize = pixelsize;
	b->glyphs = calloc(sf->glyphcnt,sizeof(BDFChar *));
	b->ascent = ascent;
	b->descent = pixelsize-b->ascent;
	b->res = defs.res;
	if ( depth!=1 )
	    BDFClut(b,(1<<(depth/2)));
	if ( !toback ) {
	    b->next = sf->bitmaps;
	    sf->bitmaps = b;
	    SFOrderBitmapList(sf);
	}
    }
    BDFPropsFree(b);
    b->prop_cnt = dummy.prop_cnt;
    b->props = dummy.props;
    free(b->foundry);
    b->foundry = ( foundry[0]=='\0' ) ? NULL : copy(foundry);
    if ( ispk==1 ) {
	while ( pk_char(bdf,sf,b,map));
    } else if ( ispk==3 ) {
	while ( gf_char(bdf,sf,b,map));
    } else if ( ispk==2 ) {
	if ( !PcfParse(bdf,toc,sf,map,b,enc) ) {
	    ff_post_error(_("Not a pcf file"), _("Not an X11 pcf file %.200s"), filename );
	}
    } else {
	while ( gettoken(bdf,tok,sizeof(tok))!=-1 ) {
	    if ( strcmp(tok,"COMMENT")==0 ) {
		int ch;
		while ( (ch=getc(bdf))!=EOF && ch!='\n' && ch!='\r' );
		if ( ch=='\r' ) {
		    ch = getc(bdf);
		    if ( ch!='\n' )
			ungetc(ch,bdf);
		}
	    } else if ( strcmp(tok,"STARTCHAR")==0 ) {
		AddBDFChar(bdf,sf,b,map,depth,&defs,enc);
		ff_progress_next();
	    }
	}
    }
    fclose(bdf); free(toc);
    sf->changed = true;
    if ( sf->bitmaps!=NULL && sf->bitmaps->next==NULL && dummy.prop_cnt>0 &&
	    map->enc == &custom )
	BDFForceEnc(sf,map);
return( b );
}

static BDFFont *_SFImportBDF(SplineFont *sf, char *filename,int ispk, int toback, EncMap *map) {
    int i;
    char *pt, *temp=NULL;
    char buf[1500];
    BDFFont *ret;

    pt = strrchr(filename,'.');
    i = -1;
    if ( pt!=NULL ) for ( i=0; compressors[i].ext!=NULL; ++i )
	if ( strcmp(compressors[i].ext,pt+1)==0 )
    break;
    if ( i==-1 || compressors[i].ext==NULL ) i=-1;
    else {
	sprintf( buf, "%s %s", compressors[i].decomp, filename );
	if ( system(buf)==0 )
	    *pt='\0';
	else {
	    /* Assume no write access to file */
	    char *dir = getenv("TMPDIR");
	    if ( dir==NULL ) dir = P_tmpdir;
	    temp = malloc(strlen(dir)+strlen(GFileNameTail(filename))+2);
	    strcpy(temp,dir);
	    strcat(temp,"/");
	    strcat(temp,GFileNameTail(filename));
	    *strrchr(temp,'.') = '\0';
	    sprintf( buf, "%s -c %s > %s", compressors[i].decomp, filename, temp );
	    if ( system(buf)==0 )
		filename = temp;
	    else {
		free(temp);
		ff_post_error(_("Decompress Failed!"),_("Decompress Failed!"));
return( NULL );
	    }
	}
    }
    ret = SFImportBDF(sf, filename,ispk, toback, map);
    if ( temp!=NULL ) {
	unlink(temp);
	free(temp);
    } else if ( i!=-1 ) {
	sprintf( buf, "%s %s", compressors[i].recomp, filename );
	system(buf);
    }
return( ret );
}

static void SFSetupBitmap(SplineFont *sf,BDFFont *strike,EncMap *map) {
    int i;
    SplineChar *sc;

    strike->sf = sf;
    if ( strike->glyphcnt>sf->glyphcnt )
	ExtendSF(sf,map,strike->glyphcnt);
    for ( i=0; i<strike->glyphcnt; ++i ) if ( strike->glyphs[i]!=NULL ) {
	if ( i>=sf->glyphcnt || sf->glyphs[i]==NULL ) {
	    int enc=-1;
	    if ( strike->glyphs[i]->sc->unicodeenc!=-1 )
		enc = EncFromUni(strike->glyphs[i]->sc->unicodeenc,map->enc);
	    if ( enc==-1 )
		enc = EncFromName(strike->glyphs[i]->sc->name,sf->uni_interp,map->enc);
	    if ( enc==-1 )
		enc = map->enccount;
	    sc = MakeEncChar(sf,map,enc,strike->glyphs[i]->sc->name);
	    strike->glyphs[i]->sc = sc;
	} else
	    strike->glyphs[i]->sc = sf->glyphs[i];
	sc = strike->glyphs[i]->sc;
	if ( sc!=NULL && !sc->widthset ) {
	    sc->widthset = true;
	    sc->width = strike->glyphs[i]->width*(sf->ascent+sf->descent)/strike->pixelsize;
	}
    }
}

static void SFMergeBitmaps(SplineFont *sf,BDFFont *strikes,EncMap *map) {
    BDFFont *b, *prev, *snext;

    while ( strikes ) {
	snext = strikes->next;
	strikes->next = NULL;
	for ( prev=NULL,b=sf->bitmaps; b!=NULL &&
		(b->pixelsize!=strikes->pixelsize || BDFDepth(b)!=BDFDepth(strikes));
		b=b->next );
	if ( b==NULL ) {
	    strikes->next = sf->bitmaps;
	    sf->bitmaps = strikes;
	    SFSetupBitmap(sf,strikes,map);
	} else if ( !alreadyexists(strikes->pixelsize)) {
	    BDFFontFree(strikes);
	} else {
	    strikes->next = b->next;
	    if ( prev==NULL )
		sf->bitmaps = strikes;
	    else
		prev->next = strikes;
	    BDFFontFree(b);
	    SFSetupBitmap(sf,strikes,map);
	}
	strikes = snext;
    }
    SFOrderBitmapList(sf);
    SFDefaultAscent(sf);
}

static void SFAddToBackground(SplineFont *sf,BDFFont *bdf);

int FVImportBDF(FontViewBase *fv, char *filename, int ispk, int toback) {
    BDFFont *b, *anyb=NULL;
    char buf[300];
    char *eod, *fpt, *file, *full;
    int fcnt, any = 0;
    int oldenccnt = fv->map->enccount;

    eod = strrchr(filename,'/');
    *eod = '\0';
    fcnt = 1;
    fpt = eod+1;
    while (( fpt=strstr(fpt,"; "))!=NULL )
	{ ++fcnt; fpt += 2; }

    sprintf(buf, _("Loading font from %.100s"), filename);
    ff_progress_start_indicator(10,_("Loading..."),buf,_("Reading Glyphs"),0,fcnt);
    ff_progress_enable_stop(false);

    file = eod+1;
    do {
	fpt = strstr(file,"; ");
	if ( fpt!=NULL ) *fpt = '\0';
	full = malloc(strlen(filename)+1+strlen(file)+1);
	strcpy(full,filename); strcat(full,"/"); strcat(full,file);
	sprintf(buf, _("Loading font from %.100s"), filename);
	ff_progress_change_line1(buf);
	b = _SFImportBDF(fv->sf,full,ispk,toback, fv->map);
	free(full);
	if ( fpt!=NULL ) ff_progress_next_stage();
	if ( b!=NULL ) {
	    anyb = b;
	    any = true;
	    FVRefreshAll(fv->sf);
	}
	file = fpt+2;
    } while ( fpt!=NULL );
    ff_progress_end_indicator();
    if ( oldenccnt != fv->map->enccount ) {
	FontViewBase *fvs;
	for ( fvs=fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	    free(fvs->selected);
	    fvs->selected = calloc(fvs->map->enccount,sizeof(char));
	}
	FontViewReformatAll(fv->sf);
    }
    if ( anyb==NULL ) {
	ff_post_error( _("No Bitmap Font"), _("Could not find a bitmap font in %s"), filename );
    } else if ( toback )
	SFAddToBackground(fv->sf,anyb);
return( any );
}

/* sf and bdf are assumed to have the same gids */
static void SFAddToBackground(SplineFont *sf,BDFFont *bdf) {
    struct _GImage *base;
    GClut *clut;
    GImage *img;
    int i;
    SplineChar *sc; BDFChar *bdfc;
    real scale = (sf->ascent+sf->descent)/(double) (bdf->ascent+bdf->descent);
    real yoff = sf->ascent-bdf->ascent*scale;

    for ( i=0; i<sf->glyphcnt && i<bdf->glyphcnt; ++i ) {
	if ( bdf->glyphs[i]!=NULL ) {
	    if ( (sc = sf->glyphs[i])==NULL ) {
		sc = sf->glyphs[i] = SplineCharCreate(2);
		sc->name = copy(bdf->glyphs[i]->sc->name);
		sc->orig_pos = i;
		sc->unicodeenc = bdf->glyphs[i]->sc->unicodeenc;
	    }
	    bdfc = bdf->glyphs[i];

	    base = calloc(1,sizeof(struct _GImage));
	    base->image_type = it_mono;
	    base->data = bdfc->bitmap;
	    base->bytes_per_line = bdfc->bytes_per_line;
	    base->width = bdfc->xmax-bdfc->xmin+1;
	    base->height = bdfc->ymax-bdfc->ymin+1;
	    bdfc->bitmap = NULL;

	    clut = calloc(1,sizeof(GClut));
	    clut->clut_len = 2;
	    clut->clut[0] = default_background;
	    clut->clut[1] = 0x808080;
	    clut->trans_index = 0;
	    base->trans = 0;
	    base->clut = clut;

	    img = calloc(1,sizeof(GImage));
	    img->u.image = base;

	    SCInsertImage(sc,img,scale,yoff+(bdfc->ymax+1)*scale,bdfc->xmin*scale,ly_back);
	}
    }
    BDFFontFree(bdf);
}

int FVImportMult(FontViewBase *fv, char *filename, int toback, int bf) {
    SplineFont *strikeholder, *sf = fv->sf;
    BDFFont *strikes;
    char buf[300];

    snprintf(buf, sizeof(buf), _("Loading font from %.100s"), filename);
    ff_progress_start_indicator(10,_("Loading..."),buf,_("Reading Glyphs"),0,2);
    ff_progress_enable_stop(false);

    if ( bf == bf_ttf )
	strikeholder = SFReadTTF(filename,toback?ttf_onlyonestrike|ttf_onlystrikes:ttf_onlystrikes,0);
    else if ( bf == bf_fon )
	strikeholder = SFReadWinFON(filename,toback);
    else if ( bf == bf_palm )
	strikeholder = SFReadPalmPdb(filename);
    else
	strikeholder = SFReadMacBinary(filename,toback?ttf_onlyonestrike|ttf_onlystrikes:ttf_onlystrikes,0);

    if ( strikeholder==NULL || (strikes = strikeholder->bitmaps)==NULL ) {
	SplineFontFree(strikeholder);
	ff_progress_end_indicator();
return( false );
    }
    SFMatchGlyphs(strikeholder,sf,false);
    if ( toback )
	SFAddToBackground(sf,strikes);
    else
	SFMergeBitmaps(sf,strikes,fv->map);

    strikeholder->bitmaps =NULL;
    SplineFontFree(strikeholder);
    ff_progress_end_indicator();
return( true );
}

SplineFont *SFFromBDF(char *filename,int ispk,int toback) {
    SplineFont *sf = SplineFontBlank(256);
    EncMap *map = EncMapNew(256,256,&custom);
    BDFFont *bdf;

    sf->onlybitmaps = true;
    bdf = SFImportBDF(sf,filename,ispk,toback, map);
    sf->map = map;
    if ( bdf==(BDFFont *) -1 )
	/* Do Nothing: User cancelled a load of a duplicate pixelsize */;
    else if ( toback && bdf!=NULL )
	SFAddToBackground(sf,bdf);
    else
	sf->changed = false;
    SFDefaultAscent(sf);
return( sf );
}

void SFCheckPSBitmap(SplineFont *sf) {
    /* Check to see if this type3 font is actually a bitmap font in disguise */
    /*  (and make sure all bitmaps are the same size */
    /* If so, create a bitmap version of the font too */
    int i,j;
    SplineChar *sc;
    ImageList *il=NULL;
    double scale = 0;
    BDFFont *bdf;
    BDFChar *bdfc;
    struct _GImage *base;

    if ( !sf->multilayer )
return;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	if ( sc->layer_cnt!=2 )
return;
	if ( sc->layers[ly_fore].splines!=NULL || sc->layers[ly_fore].refs!=NULL )
return;
	if ( (il = sc->layers[ly_fore].images)!=NULL ) {
	    base = il->image->list_len==0 ? il->image->u.image : il->image->u.images[0];
	    if ( il->next!=NULL )
return;
	    if ( base->image_type!=it_mono )
return;
	    if ( !RealNear(il->xscale,il->yscale) )
return;
	    if ( scale == 0 )
		scale = il->xscale;
	    else if ( !RealNear(il->xscale,scale))
return;
	}
    }
    if ( il==NULL || scale <= 0 )
return;			/* No images */

    /* Every glyph is either:
    	A single image
	empty (space)
      and all images have the same scale
    */

    bdf = chunkalloc(sizeof(BDFFont));
    bdf->sf = sf;
    sf->bitmaps = bdf;
    bdf->pixelsize = (sf->ascent+sf->descent)/scale;
    bdf->ascent = rint(sf->ascent/scale);
    bdf->descent = bdf->pixelsize - bdf->ascent;
    bdf->res = -1;
    bdf->glyphcnt = bdf->glyphmax = sf->glyphcnt;
    bdf->glyphs = calloc(sf->glyphcnt,sizeof(BDFChar *));

    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	bdf->glyphs[i] = bdfc = chunkalloc(sizeof(BDFChar));
	memset( bdfc,'\0',sizeof( BDFChar ));
	bdfc->sc = sc;
	bdfc->orig_pos = i;
	bdfc->depth = 1;
	bdfc->width = rint(sc->width/scale);
	bdfc->vwidth = rint(sc->vwidth/scale);
	if ( (il = sc->layers[ly_fore].images)==NULL )
	    bdfc->bitmap = malloc(1);
	else {
	    base = il->image->list_len==0 ? il->image->u.image : il->image->u.images[0];
	    bdfc->xmin = rint(il->xoff/scale);
	    bdfc->ymax = rint(il->yoff/scale);
	    bdfc->xmax = bdfc->xmin + base->width -1;
	    bdfc->ymin = bdfc->ymax - base->height +1;
	    bdfc->bytes_per_line = base->bytes_per_line;
	    bdfc->bitmap = malloc(bdfc->bytes_per_line*base->height);
	    memcpy(bdfc->bitmap,base->data,bdfc->bytes_per_line*base->height);
	    for ( j=0; j<bdfc->bytes_per_line*base->height; ++j )
		bdfc->bitmap[j] ^= 0xff;
	}
    }
}
