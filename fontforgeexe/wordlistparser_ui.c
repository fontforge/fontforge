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

#include "wordlistparser_ui.h"

#include "gwidget.h"
#include "ffglib.h"
#include "uiinterface.h"
#include "ustring.h"
#include "utype.h"

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
	int32_t len;
	GGadgetGetList(g,&len);
	/* We subtract 3 because: There are two lines saying "load * list" */
	/*  and then a line with a rule on it which we don't want access to */
	if ( cidx+offset >=0 && cidx+offset<len-3 )
	{
	    cidx += offset;
	    *idx = cidx;
	    GGadgetSelectOneListItem( g, cidx );
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
