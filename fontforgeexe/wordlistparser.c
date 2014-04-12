/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2013 Ben Martin

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

#include <string.h>
#include <ctype.h>

#include "inc/ustring.h"
#include "inc/ggadget.h"
#include "inc/gwidget.h"
#include "uiinterface.h"
#include "wordlistparser.h"
#include "gc.h"


const char* Wordlist_getSCName( SplineChar* sc )
{
    /* printf("Wordlist_getSCName() sc->name:%s\n", sc->name ); */
    /* printf("Wordlist_getSCName() sc->len :%zd\n", strlen(sc->name) ); */
    /* printf("Wordlist_getSCName() is three:%d\n", strcmp( sc->name, "three" ) ); */
        
    static char ret[ 1024 ];
    int simple = false;
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
        
    snprintf( ret, 1024, "/%s", sc->name );
    return ret;
}



static SplineChar* WordlistEscapedInputStringToRealString_readGlyphName( SplineFont *sf, char* in, char* in_end, char** updated_in, char* glyphname )
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
//    printf("WordlistEscapedInputStringToRealString_readGlyphName(top2) %c\n", *in);
    while( *in != '/' && *in != ' ' && *in != ']' && in != in_end )
    {
//	printf("WordlistEscapedInputStringToRealString_readGlyphName(add) %c\n", *in );
	*outname = *in;
	++outname;
	in++;
    }
    bool FullMatchEndsOnSpace = (*in == ' ');
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
	    sc = SFGetChar( sf, unicodepoint, 0 );
	    if( sc && endptr )
	    {
		char* endofglyphname = glyphname + strlen(glyphname);
//		printf("endptr:%p endofglyphname:%p\n", endptr, endofglyphname );
		for( ; endptr != endofglyphname; endptr++ )
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
		    for( ; endptr != endofglyphname; endptr++ )
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
//    printf("WordlistEscapedInputStringToRealString_readGlyphName(end) gn:%s\n", glyphname );
    return 0;
}


int WordlistEscapedInputStringToRealString_getFakeUnicodeAsScUnicodeEnc( SplineChar *sc, void* udata )
{
    return( sc->unicodeenc );
}


unichar_t* WordlistEscapedInputStringToRealString(
    SplineFont* sf,
    unichar_t* input_const,
    GArray** selected_out,
    WordlistEscapedInputStringToRealString_getFakeUnicodeOfScFunc getUnicodeFunc,
    void* udata )
{
    char* input = u2utf8_copy(input_const);

    // truncate insanely long lines rather than crash
    if( strlen(input) > PATH_MAX )
	input[PATH_MAX] = '\0';

//    printf("MVEscapedInputStringToRealString(top) input:%s\n", input );
    int  buffer_sz = PATH_MAX;
    char buffer[PATH_MAX+1];
    memset( buffer, '\0', buffer_sz );
    char *out = buffer;
    char* in = input;
    char* in_end = input + strlen(input);
    // trim comment and beyond from input
    {
	char* p = input;
	while( p && p < in_end  )
	{
	    p = strchr( p, '#' );
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
    in_end = input + strlen(input);

//    printf("MVEscapedInputStringToRealString() in:%p in_end:%p\n", in, in_end );

    GArray* selected = g_array_new( 1, 1, sizeof(int));
    *selected_out = selected;
    int addingGlyphsToSelected = 0;
    int currentGlyphIndex = -1;
    for ( ; in != in_end; in++ )
    {
	char ch = *in;
//	printf("got ch:%c buf:%s\n", ch, buffer );

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
	currentGlyphIndex++;
	if( addingGlyphsToSelected )
	{
	    int selectGlyph = currentGlyphIndex;
	    g_array_append_val( selected, selectGlyph );
	}

	if( ch == '/' || ch == '\\' )
	{
	    // start of a glyph name
	    char glyphname[ PATH_MAX+1 ];
	    char* updated_in = 0;
	    SplineChar* sc = WordlistEscapedInputStringToRealString_readGlyphName( sf, in, in_end, &updated_in, glyphname );
	    if( sc )
	    {
		TRACE("ToRealString have an sc!... in:%p updated_in:%p\n", in, updated_in );
		in = updated_in;
		int n = getUnicodeFunc( sc, udata );
		TRACE("ToRealString orig_pos:%d\n", sc->orig_pos );
		if( n == -1 )
		{
		    printf("ToRealString NO UNICODE, orig_pos:%d\n", sc->orig_pos );
		    printf("ToRealString NO UNICODE, name:%s\n", sc->name );
		}
		
//		printf("ToRealString have an sc!... n:%d\n", n );
//		printf("sc->unic:%d\n",sc->unicodeenc);

		TRACE("calling utf8_idpb buffer:%s out:%s ch:%d\n", buffer, out, n );
		
		out = utf8_idpb( out, n, 0);
		if( !out )
		    printf("ToRealString error on out\n");
		continue;
	    }
	}

	*out++ = ch;
    }

    unichar_t* ret = (unichar_t *) utf82u_copy( buffer );
    free(input);
    return(ret);
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


unichar_t* WordlistEscapedInputStringToRealStringBasic(
    SplineFont* sf,
    unichar_t* input_const,
    GArray** selected_out )
{
    unichar_t* ret = WordlistEscapedInputStringToRealString(
	sf, input_const, selected_out,
	WordlistEscapedInputStringToRealString_getFakeUnicodeAsScUnicodeEnc, 0 );
    return ret;
}


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
    int limit = PATH_MAX;
    SplineChar* scarray[ PATH_MAX + 1 ];
    GArray* selected = 0;
    memset( ret, 0, sizeof(unichar_t) * PATH_MAX );
    memset( scarray, 0, sizeof(SplineChar*) * limit+1 );
 
    WordlistTrimTrailingSingleSlash( txtu );
    txtu = WordlistEscapedInputStringToRealStringBasic( sf, txtu, &selected );
    GArray* bv = Wordlist_selectedToBitmapArray( selected );
    g_array_unref( selected );
    selected = 0;


    const unichar_t *pt, *ept, *tpt;
    pt = txtu;
    ept=txtu+u_strlen(txtu);
    for ( tpt=pt; tpt<ept; ++tpt )
    {
        int ch = *tpt;
        if( tpt == pt )
        {
            // your own char at the leading of the text
            SplineChar* sc = SFGetOrMakeCharFromUnicodeBasic( sf, ch );
            scarray[i] = sc;
            i++;
            continue;
        }
        scarray[i] = SFGetOrMakeCharFromUnicodeBasic( sf, ch );

        i++;
        if( i >= limit )
            break;
    }
    
    memset( ret, 0, sizeof(unichar_t) * PATH_MAX );
    for( i = 0; scarray[i]; i++ )
    {
        int element_selected = g_array_index (bv, gint, i);
	if( i == offset )
	    element_selected = 1;
	
        if( element_selected )
        {
            int pos = map->backmap[ scarray[i]->orig_pos ];
            TRACE("pos1:%d\n", pos );
            TRACE("map:%d\n", map->map[ pos ] );
            int gid = pos < 0 || pos >= map->enccount ? -2 : map->map[pos];
            if( gid == -2 )
                continue;
            if( gid==-1 || !sf->glyphs[gid] ) 
                scarray[i] = SFMakeChar( sf, map, pos );
            else
                scarray[i] = sf->glyphs[gid];
        }
        
        
        if( element_selected )
            uc_strcat( ret, "[" );

        /* uc_strcat( ret, "/" ); */
        /* uc_strcat( ret, scarray[i]->name ); */
        uc_strcat( ret, Wordlist_getSCName( scarray[i] ));

        if( element_selected )
            uc_strcat( ret, "]" );
    }
    
    return ret;
}


unichar_t* Wordlist_advanceSelectedCharsBy( SplineFont* sf, EncMap *map, unichar_t* txtu, int offset )
{
    unichar_t original_data[ PATH_MAX ];
    static unichar_t ret[ PATH_MAX ];
    int limit = PATH_MAX;
    SplineChar* scarray[ PATH_MAX + 1 ];
    GArray* selected = 0;
    int i = 0;

    u_strcpy( original_data, txtu );
    TRACE("Wordlist_advanceSelectedCharsBy(1) %s\n", u_to_c( txtu ));
    WordlistTrimTrailingSingleSlash( txtu );
    txtu = WordlistEscapedInputStringToRealStringBasic( sf, txtu, &selected );
    TRACE("Wordlist_advanceSelectedCharsBy(2) %s\n", u_to_c( txtu ));

    GArray* bv = Wordlist_selectedToBitmapArray( selected );
    TRACE("selected->len:%d\n", selected->len );
    if( !selected->len )
    {
        int one = 1;
        g_array_insert_val( bv, 0, one );
    }
    g_array_unref( selected );
    selected = 0;
    
    memset( scarray, 0, sizeof(SplineChar*) * limit+1 );
    const unichar_t *pt, *ept, *tpt;
    pt = txtu;
    ept=txtu+u_strlen(txtu);
    for ( tpt=pt; tpt<ept; ++tpt )
    {
        int ch = *tpt;
        if( tpt == pt )
        {
            // your own char at the leading of the text
            SplineChar* sc = SFGetOrMakeCharFromUnicodeBasic( sf, ch );
            scarray[i] = sc;
            i++;
            continue;
        }
		
        scarray[i] = SFGetOrMakeCharFromUnicodeBasic( sf, ch );

        
        i++;
        if( i >= limit )
            break;
    }

    

    memset( ret, 0, sizeof(unichar_t) * PATH_MAX );
    for( i = 0; scarray[i]; i++ )
    {
        int element_selected = g_array_index (bv, gint, i);

        if( element_selected )
        {
            int pos = map->backmap[ scarray[i]->orig_pos ];
            TRACE("pos1:%d\n", pos );
            pos += offset;
            TRACE("pos2:%d\n", pos );
            TRACE("map:%d\n", map->map[ pos ] );
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
		    printf("no glyph!\n");
		    u_strcpy( ret, original_data );
		    return ret;
		}
	    }
	    if( gid==-1 || !sf->glyphs[gid] ) 
                scarray[i] = SFMakeChar( sf, map, pos );
            else
                scarray[i] = sf->glyphs[gid];
        }
        
        
        if( element_selected )
            uc_strcat( ret, "[" );

        /* uc_strcat( ret, "/" ); */
        /* uc_strcat( ret, scarray[i]->name ); */
        uc_strcat( ret, Wordlist_getSCName( scarray[i] ));

        if( element_selected )
            uc_strcat( ret, "]" );
    }

    TRACE("Wordlist_advanceSelectedCharsBy(e) %s\n", u_to_c( ret ));
    return ret;
}










