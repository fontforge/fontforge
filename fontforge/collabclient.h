/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2013 Ben Martin
    
    This file is part of FontForge.

    FontForge is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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

#ifndef _ALREADY_INCLUDED_FF_COLLAB_CLIENT_H
#define _ALREADY_INCLUDED_FF_COLLAB_CLIENT_H

#include "baseviews.h"
#include "views.h"


extern void* collabclient_new( char* address, int port );
extern void  collabclient_free( void** ccvp );

extern void collabclient_sessionStart( void* ccvp, FontView *fv );
extern void collabclient_sessionJoin( void* ccvp, FontView *fv );

extern void collabclient_sendRedo( CharViewBase *cv );


#endif

