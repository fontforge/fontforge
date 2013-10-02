/* -*- coding: utf-8 -*- */
/* Copyright (C) 2013 by Ben Martin */
/*
 * 
 * FontForge is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * FontForge is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FontForge.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * For more details see the COPYING file in the root directory of this
 * distribution.
 */

#ifndef _FFGLIB_H_
#define _FFGLIB_H_

#define GTimer GTimer_GTK
#define GList  GList_Glib
#include <glib.h>
#include <glib-object.h>
#undef GTimer
#undef GList

#endif
