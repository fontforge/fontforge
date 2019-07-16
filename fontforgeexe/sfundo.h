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

#ifndef FONTFORGE_SFUNDO_H
#define FONTFORGE_SFUNDO_H

#include "splinefont.h"

void SFUndoFreeAssociated( struct sfundoes *undo );
void SFUndoFree( struct sfundoes *undo );
SFUndoes* SFUndoCreateSFD( enum sfundotype t, char* staticmsg, char* sfdfrag );

/**
 * Remove undo from the font level undoes on splinefont 'sf' and
 * completely free the given undo from memory.
 */
void SFUndoRemoveAndFree( SplineFont *sf, struct sfundoes *undo );


char* SFUndoToString( SFUndoes* undo );
SFUndoes* SFUndoFromString( char* str );

void SFUndoPerform( SFUndoes* undo, SplineFont* sf );
SFUndoes* SFUndoCreateRedo( SFUndoes* undo, SplineFont* sf );

void SFUndoPushFront( struct sfundoes ** undoes, SFUndoes* undo );



#endif /* FONTFORGE_SFUNDO_H */
