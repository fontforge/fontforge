/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2013 Ben Martin
    Copyright 2014-2015, the FontForge Project Developers.

    This file is part of FontForge.

    FontForge is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation,either version 3 of the License, or
    (at your option) any later version.

    FontForge is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FontForge.  If not, see <http://www.gnu.org/licenses/>.

    For more details see the COPYING.gplv3 file in the root directory of this
    distribution.

*******************************************************************************
*******************************************************************************
******************************************************************************/
#include <fontforge-config.h>

#include "fvfonts.h"

#include <string.h>
#include <ctype.h>

#include "inc/ustring.h"
#include "inc/ggadget.h"
#include "inc/gwidget.h"
#include "uiinterface.h"
#include "wordlistparser.h"


static void dump_ustr( char* msg, unichar_t* u )
{
    char u8buf[1001];
    char* pt = u8buf;
    memset( u8buf, 0, 1000 );
    printf("%s\n", msg );
    unichar_t* p = u;
    unichar_t* e = u + u_strlen( u );
    for( ; p!=e; ++p )
    {
	unichar_t buf[5];
	buf[0] = *p;
	buf[1] = '\0';
	printf("walk %d %s\n", *p, u_to_c(buf));

	pt = utf8_idpb( pt, *p, 0);
    }
    printf("%s u8str:%s\n", msg, u8buf );
}


const char* Wordlist_getSCName( SplineChar* sc )
{
    /* printf("Wordlist_getSCName() sc->name:%s\n", sc->name ); */
    /* printf("Wordlist_getSCName() sc->len :%zd\n", strlen(sc->name) ); */
    /* printf("Wordlist_getSCName() is three:%d\n", strcmp( sc->name, "three" ) ); */
        
    static char ret[ 1024 ];
    int simple = false;
    /* If the glyph is unencoded, we need to keep a slash before the name because
       it doesn't correspond to the codepoint. */
    if( sc->unicodeenc != -1 ) {
    if( strlen( sc->name ) == 1 )
    {
        char ch = sc->name[0];

        if( ch >= 'a' && ch <= 'z' )
            simple = true;
        else if( ch >= '0' && ch <= '9' )
            simple = true;
        if( ch >= 'A' && ch <= 'Z' )
            simple = true;

        if( simple )
        {
            strcpy( ret, sc->name );
            return ret;
        }
    }

    if( !strcmp( sc->name, "zero" ))
        return "0";
    if( !strcmp( sc->name, "one" ))
        return "1";
    if( !strcmp( sc->name, "two" ))
        return "2";
    if( !strcmp( sc->name, "three" ))
        return "3";
    if( !strcmp( sc->name, "four" ))
        return "4";
    if( !strcmp( sc->name, "five" ))
        return "5";
    if( !strcmp( sc->name, "six" ))
        return "6";
    if( !strcmp( sc->name, "seven" ))
        return "7";
    if( !strcmp( sc->name, "eight" ))
        return "8";
    if( !strcmp( sc->name, "nine" ))
        return "9";
    }

    snprintf( ret, 1024, "/%s", sc->name );
    return ret;
}



static SplineChar*
WordlistEscapedInputStringToRealString_readGlyphName(
    SplineFont *sf, char* in, char* in_end,
    char** updated_in, char* glyphname )
{
//    printf("WordlistEscapedInputStringToRealString_readGlyphName(top)\n");

    int startedWithBackSlash = (*in == '\\');
    if( *in != '/' && *in != '\\' )
	return 0;
    // move over the delimiter that we know we are on
    in++;
    char* startpos = in;

    // Get the largest possible 'glyphname' from the input stream.
    memset( glyphname, '\0', PATH_MAX );
    char* outname = glyphname;
    while( *in != '/' && *in != ' ' && *in != ']' && in != in_end )
    {
	*outname = *in;
	++outname;
	in++;
    }
    bool FullMatchEndsOnSpace = 0;
    char* maxpos = in;
    char* endpos = maxpos-1;
//    printf("WordlistEscapedInputStringToRealString_readGlyphName(x1) -->:%s:<--\n", glyphname);
//    printf("WordlistEscapedInputStringToRealString_readGlyphName(x2) %c %p %p\n",*in,startpos,endpos);

    int loopCounter = 0;
    int firstLookup = 1;
    for( ; endpos >= startpos; endpos--, loopCounter++ )
    {
//	printf("WordlistEscapedInputStringToRealString_readGlyphName(trim loop top) gn:%s\n", glyphname );
	SplineChar* sc = 0;
	if( startedWithBackSlash )
	{
	    if( glyphname[0] == 'u' )
		glyphname++;

	    char* endptr = 0;
	    long unicodepoint = strtoul( glyphname+1, &endptr, 16 );
	    TRACE("AAA glyphname:%s\n", glyphname+1 );
	    TRACE("AAA unicodepoint:%ld\n", unicodepoint );
	    sc = SFGetChar( sf, unicodepoint, 0 );
	    if( sc && endptr )
	    {
		char* endofglyphname = glyphname + strlen(glyphname);
		for( ; endptr < endofglyphname; endptr++ )
		    --endpos;
	    }
	    if( !sc )
	    {
		printf("WordlistEscapedInputStringToRealString_readGlyphName() no char found for backslashed unicodepoint:%ld\n", unicodepoint );
		strcpy(glyphname,"backslash");
		sc = SFGetChar( sf, -1, glyphname );
		endpos = startpos;
	    }
	}
	else
	{
	    if( firstLookup && glyphname[0] == '#' )
	    {
		char* endptr = 0;
		long unicodepoint = strtoul( glyphname+1, &endptr, 16 );
//		printf("WordlistEscapedInputStringToRealString_readGlyphName() unicodepoint:%ld\n", unicodepoint );
		sc = SFGetChar( sf, unicodepoint, 0 );
		if( sc && endptr )
		{
		    char* endofglyphname = glyphname + strlen(glyphname);
//		    printf("endptr:%p endofglyphname:%p\n", endptr, endofglyphname );
		    for( ; endptr < endofglyphname; endptr++ )
			--endpos;
		}
	    }
	    if( !sc )
	    {
//		printf("WordlistEscapedInputStringToRealString_readGlyphName(getchar) gn:%s\n", glyphname );
		sc = SFGetChar( sf, -1, glyphname );
	    }
	}

	if( sc )
	{
//	    printf("WordlistEscapedInputStringToRealString_readGlyphName(found!) gn:%s start:%p end:%p\n", glyphname, startpos, endpos );
	    if( !loopCounter && FullMatchEndsOnSpace )
	    {
		endpos++;
	    }
	    *updated_in = endpos;
	    return sc;
	}
	if( glyphname[0] != '\0' )
	    glyphname[ strlen(glyphname)-1 ] = '\0';
    }


    *updated_in = endpos;

    // printf("WordlistEscapedInputStringToRealString_readGlyphName(end) gn:%s\n", glyphname );
    return 0;
}


static SplineChar*
u_WordlistEscapedInputStringToRealString_readGlyphName(
    SplineFont *sf, unichar_t* in, unichar_t* in_end,
    unichar_t** updated_in, unichar_t* glyphname )
{
    int startedWithBackSlash = (*in == '\\');
    if( *in != '/' && *in != '\\' )
	return 0;
    bool startsWithBackSlash = *in == '\\';
    // move over the delimiter that we know we are on
    in++;
    unichar_t* startpos = in;

    // Get the largest possible 'glyphname' from the input stream.
    memset( glyphname, '\0', PATH_MAX );
    unichar_t* outname = glyphname;
    while( *in != '/'
	   && ( !startsWithBackSlash || *in != '\\' )
	   && *in != ' ' && *in != ']' && in != in_end )
    {
	*outname = *in;
	++outname;
	in++;
    }
    bool FullMatchEndsOnSpace = 0;
    unichar_t* maxpos = in;
    unichar_t* endpos = maxpos-1;
    TRACE("WordlistEscapedInputStringToRealString_readGlyphName(x1) -->:%s:<--\n", u_to_c(glyphname));

    int loopCounter = 0;
    int firstLookup = 1;
    for( ; endpos >= startpos; endpos--, loopCounter++ )
    {
//	printf("WordlistEscapedInputStringToRealString_readGlyphName(trim loop top) gn:%s\n", u_to_c(glyphname) );
	SplineChar* sc = 0;
	
	if( startedWithBackSlash )
	{
	    if( glyphname[0] == 'u' )
		glyphname++;

	    unichar_t* endptr = 0;
	    long unicodepoint = u_strtoul( glyphname+1, &endptr, 16 );
	    TRACE("AAA glyphname:%s\n", u_to_c(glyphname+1) );
	    TRACE("AAA unicodepoint:%ld\n", unicodepoint );
	    sc = SFGetChar( sf, unicodepoint, 0 );
	    if( sc && endptr )
	    {
		unichar_t* endofglyphname = glyphname + u_strlen(glyphname);
		/* printf("glyphname:%p\n", glyphname ); */
		/* printf("endptr:%p endofglyphname:%p\n", endptr, endofglyphname ); */
		for( ; endptr < endofglyphname; endptr++ )
		    --endpos;
	    }
	    if( !sc )
	    {
		printf("WordlistEscapedInputStringToRealString_readGlyphName() no char found for backslashed unicodepoint:%ld\n", unicodepoint );
		uc_strcpy(glyphname,"backslash");
		sc = SFGetChar( sf, -1, u_to_c(glyphname) );
		endpos = startpos;
	    }
	}
	else
	{
	    if( uc_startswith( glyphname, "uni"))
	    {
		unichar_t* endptr = 0;
		long unicodepoint = u_strtoul( glyphname+3, &endptr, 16 );
                SplineChar* tmp = 0;
		TRACE("uni prefix, codepoint: %ld\n", unicodepoint );
		sc = SFGetChar( sf, unicodepoint, 0 );
                if (tmp = SFGetChar( sf, -1, u_to_c(glyphname) )) {
		    TRACE("have subst. char: %s\n", tmp->name );
                    sc = tmp;
                } else {
		    if( sc && endptr )
		    {
		        unichar_t* endofglyphname = glyphname + u_strlen(glyphname);
//		        printf("endptr:%p endofglyphname:%p\n", endptr, endofglyphname );
		        for( ; endptr < endofglyphname; endptr++ )
                            --endpos;
		    }
                }
	    }
	    
	    if( firstLookup && glyphname[0] == '#' )
	    {
		unichar_t* endptr = 0;
		long unicodepoint = u_strtoul( glyphname+1, &endptr, 16 );
//		printf("WordlistEscapedInputStringToRealString_readGlyphName() unicodepoint:%ld\n", unicodepoint );
		sc = SFGetChar( sf, unicodepoint, 0 );
		if( sc && endptr )
		{
		    unichar_t* endofglyphname = glyphname + u_strlen(glyphname);
//		    printf("endptr:%p endofglyphname:%p\n", endptr, endofglyphname );
		    for( ; endptr < endofglyphname; endptr++ )
			--endpos;
		}
	    }
	    if( !sc )
	    {
//		printf("WordlistEscapedInputStringToRealString_readGlyphName(getchar) gn:%s\n", glyphname );
		sc = SFGetChar( sf, -1, u_to_c(glyphname) );
	    }
	}

	if( sc )
	{
//	    printf("WordlistEscapedInputStringToRealString_readGlyphName(found!) gn:%s start:%p end:%p\n", glyphname, startpos, endpos );
	    if( !loopCounter && FullMatchEndsOnSpace )
	    {
		endpos++;
	    }
	    *updated_in = endpos;
	    return sc;
	}
	if( glyphname[0] != '\0' )
	    glyphname[ u_strlen(glyphname)-1 ] = '\0';
    }


    *updated_in = endpos;

    // printf("WordlistEscapedInputStringToRealString_readGlyphName(end) gn:%s\n", glyphname );
    return 0;
}


int WordlistEscapedInputStringToRealString_getFakeUnicodeAsScUnicodeEnc( SplineChar *sc, void* udata )
{
    return( sc->unicodeenc );
}


//
// If there is only one trailing slash, then remove it.
//
void WordlistTrimTrailingSingleSlash( unichar_t* txt )
{
    int len = u_strlen(txt);
//    printf("text changed, len :%d -1:%c -2:%c\n", len, txt[ len-1 ], txt[ len-2 ]  );
    if( len >= 1 )
    {
	if( len >= 2 && txt[ len-1 ]=='/' && txt[ len-2 ]=='/' )
	{
	    // nothing
	}
	else
	{
	    if( txt[ len-1 ]=='/' )
		txt[ len-1 ] = '\0';
	}
    }
}



/************************************************************/
/************************************************************/
/************************************************************/

static int WordListLineSz = 1024;

int WordListLine_countSelected( WordListLine wll )
{
    int ret = 0;
    for( ; wll->sc; wll++ ) {
	ret += wll->isSelected;
    }
    return ret;
}

WordListLine WordListLine_end( WordListLine wll )
{
    for( ; wll->sc; wll++ ) {
    }
    return wll;
}

int WordListLine_size( WordListLine wll )
{
    int ret = 0;
    for( ; wll->sc; wll++ ) {
	++ret;
    }
    return ret;
}


WordListLine WordlistEscapedInputStringToParsedDataComplex(
    SplineFont* sf,
    const unichar_t* input_const,
    WordlistEscapedInputStringToRealString_getFakeUnicodeOfScFunc getUnicodeFunc,
    void* udata )
{
    unichar_t* input = u_copy( input_const );
    WordListChar* ret = calloc( WordListLineSz, sizeof(WordListChar));
    WordListChar* out = ret;
    unichar_t* in     = input;
    unichar_t* in_end = input + u_strlen(input);
    // trim comment and beyond from input
    {
	unichar_t* p = input;
	while( p && p < in_end  )
	{
	    p = u_strchr( p, '#' );
	    if( p > input && *(p-1) == '/' )
	    {
		p++;
		continue;
	    }
	    if( p )
		*p = '\0';
	    break;
	}
    }
    in_end = input + u_strlen(input);

    int addingGlyphsToSelected = 0;
    int currentGlyphIndex = -1;
    for ( ; in < in_end; in++ )
    {
	unichar_t ch = *in;
	TRACE("in:%p end:%p got char %d %c\n", in, in_end, ch, ch );
	if( ch == '[' )
	{
	    addingGlyphsToSelected = 1;
	    continue;
	}
	if( ch == ']' )
	{
	    addingGlyphsToSelected = 0;
	    continue;
	}
	int isSelected = addingGlyphsToSelected;
	currentGlyphIndex++;

	if( ch == '/' || ch == '\\' )
	{
	    // start of a glyph name
	    unichar_t glyphname[ PATH_MAX+1 ];
	    unichar_t* updated_in = 0;
	    SplineChar* sc = u_WordlistEscapedInputStringToRealString_readGlyphName( sf, in, in_end, &updated_in, glyphname );
	    if( sc )
	    {
		in = updated_in;
		int n = getUnicodeFunc( sc, udata );
		if( n == -1 )
		{
		    /*
		     * Okay, this probably means we've got an unencoded glyph (generally
		     * used for OpenType substitutions).
		     * Redeem the value from the SplineFont datamap instead of fetching from
		     * the Unicode identifier.
		     */
		    n = sf->map->backmap[sc->orig_pos];

		    /*
		     * Unencoded glyphs have special mappings in the SplineFont that
		     * start from 65536 (values beyond Unicode, 65535 being the reserved
		     * "frontier" value).
		     */
		    if ( (sf->map->enc->is_unicodebmp || sf->map->enc->is_unicodefull) && n < 65536 ) {
		        TRACE("ToRealString: backmapped position does not match Unicode encoding\n");
		        TRACE("orig_pos: %d, backmap: %d, attached unicode enc: %d\n", sc->orig_pos, n, sc->unicodeenc );
		        TRACE("ToRealString: INVALID CHAR POSITION, name: %s\n", sc->name );
		    }
		}

		out->sc = sc;
		out->isSelected = isSelected;
		out->currentGlyphIndex = currentGlyphIndex;
                out->n = n;
		out++;
		/* out = utf8_idpb( out, n, 0 ); */
		/* if( !out ) */
		/*     printf("ToRealString error on out\n"); */
		continue;
	    }
	}

	/* If we reach this point, we're looking based on codepoint. */
	SplineChar* sc = SFGetOrMakeChar( sf, (int)ch, 0 );
	out->sc = sc;
	out->isSelected = isSelected;
	out->currentGlyphIndex = currentGlyphIndex;
	out++;
    }

    free(input);
    return(ret);
}

WordListLine WordlistEscapedInputStringToParsedData(
    SplineFont* sf,
    unichar_t* input_const )
{
    WordListLine ret = WordlistEscapedInputStringToParsedDataComplex(
	sf, input_const, 
	WordlistEscapedInputStringToRealString_getFakeUnicodeAsScUnicodeEnc, 0 );
    return ret;
}




/************************************************************/
/************************************************************/
/************************************************************/

static bool WordlistLoadFileToGTextInfo_IsLineBreak( char ch )
{
    return ch == '\n' || ch == '\r';
}
static bool WordlistLoadFileToGTextInfo_isLineAllWhiteSpace( char* buffer )
{
    char* p = buffer;
    for( ; *p; ++p )
    {
	if( !isspace( *p ))
	    return false;
    }

    return true;
}



GTextInfo** WordlistLoadFileToGTextInfoBasic( int words_max )
{
    return WordlistLoadFileToGTextInfo( -1, words_max );
}


GTextInfo** WordlistLoadFileToGTextInfo( int type, int words_max )
{
    GTextInfo **words = 0;
    int cnt;
    char buffer[PATH_MAX];
    char *filename, *temp;

    filename = gwwv_open_filename(type==-1 ? "File of Kerning Words":"File of glyphname lists",NULL,"*.txt",NULL);
    if ( !filename )
    {
	return 0;
    }
    temp = utf82def_copy(filename);
    GIOChannel* file = g_io_channel_new_file( temp, "r", 0 );
    free(temp);
    if ( !file )
    {
	ff_post_error("Could not open", "Could not open %s", filename );
	return 0;
    }
    free(filename);

    words = malloc( words_max * sizeof(GTextInfo *));

    cnt = 0;
    if ( type==-1 )
    {
	// Kerning words
	while( true )
	{
	    gsize len = 0;
	    gchar* buffer = 0;
	    GIOStatus status = g_io_channel_read_line( file, &buffer, &len, 0, 0 );

//	    printf("getline status:%d \n", status );
	    if( status != G_IO_STATUS_NORMAL )
		break;

	    chomp(buffer);
	    if ( buffer[0]=='\0'
		 || WordlistLoadFileToGTextInfo_IsLineBreak(buffer[0])
		 || WordlistLoadFileToGTextInfo_isLineAllWhiteSpace( buffer ))
	    {
		free(buffer);
		continue;
	    }

	    words[cnt] = calloc(1,sizeof(GTextInfo));
	    words[cnt]->fg = words[cnt]->bg = COLOR_DEFAULT;
	    words[cnt]->text = (unichar_t *) utf82def_copy( buffer );
	    words[cnt++]->text_is_1byte = true;
	    free(buffer);
	    if( cnt >= words_max )
		break;
	}
    }
    else
    {
	// glyphname lists
	strcpy(buffer,"​");		/* Zero width space: 0x200b, I use as a flag */
	gsize bytes_read = 0;
	while( G_IO_STATUS_NORMAL == g_io_channel_read_chars( file,
							      buffer+3,
							      sizeof(buffer)-3,
							      &bytes_read,
							      0 ))
	{
	    if ( buffer[3]=='\n' || buffer[3]=='#' )
		continue;
	    if ( cnt>1000-3 )
		break;
	    if ( buffer[strlen(buffer)-1]=='\n' )
		buffer[strlen(buffer)-1] = '\0';
	    words[cnt] = calloc(1,sizeof(GTextInfo));
	    words[cnt]->fg = words[cnt]->bg = COLOR_DEFAULT;
	    words[cnt]->text = (unichar_t *) copy( buffer );
	    words[cnt++]->text_is_1byte = true;
	}
    }

    g_io_channel_shutdown( file, 1, 0 );
    g_io_channel_unref( file );
    if ( cnt!=0 )
    {
	words[cnt] = calloc(1,sizeof(GTextInfo));
	words[cnt]->fg = words[cnt]->bg = COLOR_DEFAULT;
	words[cnt++]->line = true;
	words[cnt] = calloc(1,sizeof(GTextInfo));
	words[cnt]->fg = words[cnt]->bg = COLOR_DEFAULT;
	words[cnt]->text = (unichar_t *) copy( _("Load Word List...") );
	words[cnt]->text_is_1byte = true;
	words[cnt++]->userdata = (void *) -1;
	words[cnt] = calloc(1,sizeof(GTextInfo));
	words[cnt]->fg = words[cnt]->bg = COLOR_DEFAULT;
	words[cnt]->text = (unichar_t *) copy( _("Load Glyph Name List...") );
	words[cnt]->text_is_1byte = true;
	words[cnt++]->userdata = (void *) -2;
	words[cnt] = calloc(1,sizeof(GTextInfo));
    }
    else
    {
	free(words);
	words = 0;
    }
    return words;
}


void Wordlist_MoveByOffset( GGadget* g, int* idx, int offset )
{
    int cidx = *idx;
    if ( cidx!=-1 )
    {
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	/* We subtract 3 because: There are two lines saying "load * list" */
	/*  and then a line with a rule on it which we don't want access to */
	if ( cidx+offset >=0 && cidx+offset<len-3 )
	{
	    cidx += offset;
	    *idx = cidx;
	    GGadgetSelectOneListItem( g, cidx );
	    ti = NULL;
	}
	Wordlist_touch( g );
    }
}

void Wordlist_touch( GGadget* g )
{
    // Force any extra chars to be setup and drawn
    GEvent e;
    e.type=et_controlevent;
    e.u.control.subtype = et_textchanged;
    e.u.control.u.tf_changed.from_pulldown = GGadgetGetFirstListSelectedItem(g);
    GGadgetDispatchEvent( g, &e );
}


void WordlistLoadToGTextInfo( GGadget* g, int* idx  )
{
    int words_max = 1024*128;
    GTextInfo** words = WordlistLoadFileToGTextInfoBasic( words_max );
    if( !words )
    {
	GGadgetSetTitle8(g,"");
	return;
    }

    if( words[0] )
    {
	GGadgetSetList(g,words,true);
	GGadgetSetTitle8(g,(char *) (words[0]->text));
	GTextInfoArrayFree(words);
	*idx = 0;
	GGadgetSelectOneListItem( g, *idx );
	Wordlist_touch( g );
	return;
    }
    return;
}


static GArray* Wordlist_selectedToBitmapArray( GArray* a )
{
    GArray* ret = g_array_new( 1, 1, sizeof(gint) );
    ret = g_array_set_size( ret, PATH_MAX+1 );
    
    int i = 0;
    for (i = 0; i < a->len; i++)
    {
        int v = g_array_index (a, gint, i);
        int one = 1;
        g_array_insert_val( ret, v, one );
    }
    return ret;
}


unichar_t* Wordlist_selectionClear( SplineFont* sf, EncMap *map, unichar_t* txtu )
{
    static unichar_t ret[ PATH_MAX ];
    int limit = PATH_MAX;
    memset( ret, 0, sizeof(unichar_t) * PATH_MAX );

    unichar_t *dst = ret;
    const unichar_t *src_end = 0;
    const unichar_t *src = 0;
    src_end=txtu+u_strlen(txtu);
    for ( src=txtu; src < src_end; ++src )
    {
	if( *src != '[' && *src != ']' )
	{
	    *dst = *src;
	    dst++;
	}
    }
    
    return ret;
}

unichar_t* Wordlist_selectionAdd( SplineFont* sf, EncMap *map, unichar_t* txtu, int offset )
{
    int i = 0;
    static unichar_t ret[ PATH_MAX ];
    memset( ret, 0, sizeof(unichar_t) * PATH_MAX );
 
    WordlistTrimTrailingSingleSlash( txtu );
    WordListLine wll = WordlistEscapedInputStringToParsedData( sf, txtu );

    for( i = 0; wll->sc; wll++, i++ )
    {
	SplineChar* sc = wll->sc;
        int element_selected = wll->isSelected;
    
	if( i == offset )
	    element_selected = 1;
	
        if( element_selected )
        {
            int pos = map->backmap[ sc->orig_pos ];
            TRACE("pos1:%d\n", pos );
            TRACE("map:%d\n", map->map[ pos ] );
            int gid = pos < 0 || pos >= map->enccount ? -2 : map->map[pos];
            if( gid == -2 )
                continue;
            if( gid==-1 || !sf->glyphs[gid] ) 
                sc = SFMakeChar( sf, map, pos );
            else
                sc = sf->glyphs[gid];
        }
        
        
        if( element_selected )
            uc_strcat( ret, "[" );

        /* uc_strcat( ret, "/" ); */
        /* uc_strcat( ret, scarray[i]->name ); */
        uc_strcat( ret, Wordlist_getSCName( sc ));

        if( element_selected )
            uc_strcat( ret, "]" );
    }
    
    return ret;
}


unichar_t* Wordlist_advanceSelectedCharsBy( SplineFont* sf, EncMap *map, unichar_t* txtu, int offset )
{
    unichar_t original_data[ PATH_MAX ];
    static unichar_t ret[ PATH_MAX ];
    int i = 0;

    u_strcpy( original_data, txtu );
    TRACE("Wordlist_advanceSelectedCharsBy(1) %s\n", u_to_c( txtu ));
    WordlistTrimTrailingSingleSlash( txtu );
    WordListLine wll = WordlistEscapedInputStringToParsedData( sf, txtu );

    int selectedCount = WordListLine_countSelected( wll );
    if( !selectedCount )
	wll->isSelected = 1;
    
    memset( ret, 0, sizeof(unichar_t) * PATH_MAX );
    for( i = 0; wll->sc; wll++, i++ )
    {
	SplineChar* sc = wll->sc;
        int element_selected = wll->isSelected;

        if( element_selected )
        {
            int pos = map->backmap[ sc->orig_pos ];
            pos += offset;
            int gid = pos < 0 || pos >= map->enccount ? -2 : map->map[pos];
            if( gid == -2 )
	    {
		// we can't find a glyph at the desired position.
		// so instead of dropping it we just do not perform the operation
		// on this char.
		pos -= offset;
		gid = pos < 0 || pos >= map->enccount ? -2 : map->map[pos];
		if( gid == -2 )
		{
		    // we can't go back manually!
		    u_strcpy( ret, original_data );
		    return ret;
		}
	    }
	    if( gid==-1 || !sf->glyphs[gid] ) 
                sc = SFMakeChar( sf, map, pos );
            else
                sc = sf->glyphs[gid];
        }
        
        
        if( element_selected )
            uc_strcat( ret, "[" );

        /* uc_strcat( ret, "/" ); */
        /* uc_strcat( ret, scarray[i]->name ); */
        uc_strcat( ret, Wordlist_getSCName( sc ));

        if( element_selected )
            uc_strcat( ret, "]" );
    }

    return ret;
}



/**
 * haveSelection is set to true iff there is 1+ selections i txtu
 */
static unichar_t* Wordlist_selectionStringOnly( unichar_t* txtu, int* haveSelection )
{
    static unichar_t ret[ PATH_MAX ];
    int limit = PATH_MAX;
    memset( ret, 0, sizeof(unichar_t) * PATH_MAX );
    *haveSelection = 0;
    
    int inSelection = 0;
    unichar_t *dst = ret;
    const unichar_t *src_end = 0;
    const unichar_t *src = 0;
    src_end=txtu+u_strlen(txtu);
    for ( src=txtu; src < src_end; ++src )
    {
	if( *src == '[' )
	{
	    inSelection = 1;
	    *haveSelection = 1;
	    continue;
	}
	if( *src == ']' )
	{
	    inSelection = 0;
	    continue;
	}

	if( inSelection )
	{
	    *dst = *src;
	    dst++;
	}
    }
    
    return ret;
}


bool Wordlist_selectionsEqual( unichar_t* s1, unichar_t* s2 )
{
    static unichar_t s1stripped[ PATH_MAX ];
    static unichar_t s2stripped[ PATH_MAX ];
    int s1HasSelection = 0;
    int s2HasSelection = 0;

    u_strcpy( s1stripped, Wordlist_selectionStringOnly( s1, &s1HasSelection ));
    u_strcpy( s2stripped, Wordlist_selectionStringOnly( s2, &s2HasSelection ));

    if( s1HasSelection && !s2HasSelection )
	return false;
    if( !s1HasSelection && s2HasSelection )
	return false;

    return !u_strcmp( s1stripped, s2stripped );
}


unichar_t* WordListLine_toustr( WordListLine wll )
{
    unichar_t* ret = calloc( WordListLine_size(wll)+1, sizeof(unichar_t));
    unichar_t* p = ret;
    for( ; wll->sc; wll++, p++ ) {
	*p = wll->sc->unicodeenc;
        if (*p == -1) *p = wll->n;
    }
    return ret;
}





