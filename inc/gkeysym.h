#ifndef _GKEYSYM_H_
#define _GKEYSYM_H_

#include "fontforge-config.h"

#ifndef X_DISPLAY_MISSING
#include <X11/keysym.h>
/* Based on the X keysymdef file */
/***********************************************************
Copyright 1987, 1994, 1998  The Open Group

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


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#define GK_Special		0xfe00	/* keysyms above this value aren't unicode */
#define GK_TypeAhead		0x0000	/* no single keysym, unicode value in event string */

#define GK_BackSpace		XK_BackSpace
#define GK_Tab			XK_Tab
#ifndef XK_ISO_Left_Tab
# define GK_BackTab		XK_Tab
#else
# define GK_BackTab		XK_ISO_Left_Tab
#endif
#define GK_Linefeed		XK_Linefeed	
#define GK_Clear		XK_Clear	
#define GK_Return		XK_Return	
#define GK_Pause		XK_Pause	
#define GK_Scroll_Lock		XK_Scroll_Lock	
#define GK_Sys_Req		XK_Sys_Req	
#define GK_Escape		XK_Escape	
#define GK_Delete		XK_Delete	
                                                
#define GK_Home			XK_Home		
#define GK_Left			XK_Left		
#define GK_Up			XK_Up		
#define GK_Right		XK_Right	
#define GK_Down			XK_Down		
#define GK_Prior		XK_Prior	
#define GK_Page_Up		XK_Page_Up	
#define GK_Next			XK_Next		
#define GK_Page_Down		XK_Page_Down	
#define GK_End			XK_End		
#define GK_Begin		XK_Begin	

#define GK_Menu			XK_Menu
#define GK_Help			XK_Help

#define GK_KP_Enter		XK_KP_Enter
#define GK_KP_Home		XK_KP_Home	
#define GK_KP_Left		XK_KP_Left	
#define GK_KP_Up		XK_KP_Up	
#define GK_KP_Right		XK_KP_Right	
#define GK_KP_Down		XK_KP_Down	
#define GK_KP_Prior		XK_KP_Prior	
#define GK_KP_Page_Up		XK_KP_Page_Up	
#define GK_KP_Next		XK_KP_Next	
#define GK_KP_Page_Down		XK_KP_Page_Down	
#define GK_KP_End		XK_KP_End	
#define GK_KP_Begin		XK_KP_Begin	

#define GK_F1			XK_F1		
#define GK_F2			XK_F2		
#define GK_F3			XK_F3		
#define GK_F4			XK_F4		
#define GK_F5			XK_F5		
#define GK_F6			XK_F6		
#define GK_F7			XK_F7		
#define GK_F8			XK_F8		
#define GK_F9			XK_F9		
#define GK_F10			XK_F10		
#define GK_F11			XK_F11		
#define GK_L1			XK_L1		
#define GK_F12			XK_F12		
#define GK_L2			XK_L2		
#define GK_F13			XK_F13		
#define GK_L3			XK_L3		
#define GK_F14			XK_F14		
#define GK_L4			XK_L4		
#define GK_F15			XK_F15		
#define GK_L5			XK_L5		
#define GK_F16			XK_F16		
#define GK_L6			XK_L6		
#define GK_F17			XK_F17		
#define GK_L7			XK_L7		
#define GK_F18			XK_F18		
#define GK_L8			XK_L8		
#define GK_F19			XK_F19		
#define GK_L9			XK_L9		
#define GK_F20			XK_F20		
#define GK_L10			XK_L10		
#define GK_F21			XK_F21		
#define GK_R1			XK_R1		
#define GK_F22			XK_F22		
#define GK_R2			XK_R2		
#define GK_F23			XK_F23		
#define GK_R3			XK_R3		
#define GK_F24			XK_F24		
#define GK_R4			XK_R4		
#define GK_F25			XK_F25		
#define GK_R5			XK_R5		
#define GK_F26			XK_F26		
#define GK_R6			XK_R6		
#define GK_F27			XK_F27		
#define GK_R7			XK_R7		
#define GK_F28			XK_F28		
#define GK_R8			XK_R8		
#define GK_F29			XK_F29		
#define GK_R9			XK_R9		
#define GK_F30			XK_F30		
#define GK_R10			XK_R10		
#define GK_F31			XK_F31		
#define GK_R11			XK_R11		
#define GK_F32			XK_F32		
#define GK_R12			XK_R12		
#define GK_F33			XK_F33		
#define GK_R13			XK_R13		
#define GK_F34			XK_F34		
#define GK_R14			XK_R14		
#define GK_F35			XK_F35		
#define GK_R15			XK_R15		
                                                
#define GK_Shift_L		XK_Shift_L	
#define GK_Shift_R		XK_Shift_R	
#define GK_Control_L		XK_Control_L	
#define GK_Control_R		XK_Control_R	
#define GK_Caps_Lock		XK_Caps_Lock	
#define GK_Shift_Lock		XK_Shift_Lock	
                                                
#define GK_Meta_L		XK_Meta_L	
#define GK_Meta_R		XK_Meta_R	
#define GK_Alt_L		XK_Alt_L	
#define GK_Alt_R		XK_Alt_R	
#define GK_Super_L		XK_Super_L	
#define GK_Super_R		XK_Super_R	
#define GK_Hyper_L		XK_Hyper_L	
#define GK_Hyper_R		XK_Hyper_R

#define GK_Mode_switch  XK_Mode_switch

#else		/* No X */

#define GK_Special		0xfe00	/* keysyms above this value aren't unicode */
#define GK_TypeAhead		0x0000	/* no single keysym, unicode value in event string */

#define GK_BackSpace		0xff01
#define GK_Tab			0xff02
#define GK_BackTab		0xff03
#define GK_Linefeed		0xff04
#define GK_Clear		0xff05
#define GK_Return		0xff06
#define GK_Pause		0xff07
#define GK_Scroll_Lock		0xff08
#define GK_Sys_Req		0xff09
#define GK_Escape		0xff0a
#define GK_Delete		0xff0b
                                                
#define GK_Home			0xff0c		
#define GK_Left			0xff0d		
#define GK_Up			0xff0e		
#define GK_Right		0xff0f	
#define GK_Down			0xff10		
#define GK_Prior		0xff11	
#define GK_Page_Up		0xff12	
#define GK_Next			0xff13		
#define GK_Page_Down		0xff14	
#define GK_End			0xff15		
#define GK_Begin		0xff16	

#define GK_Menu			0xff17
#define GK_Help			0xff18

#define GK_KP_Enter		0xFF8D
#define GK_KP_Home		0xff19	
#define GK_KP_Left		0xff1a	
#define GK_KP_Up		0xff1b	
#define GK_KP_Right		0xff1c	
#define GK_KP_Down		0xff1d	
#define GK_KP_Prior		0xff1e	
#define GK_KP_Page_Up		0xff1f	
#define GK_KP_Next		0xff20	
#define GK_KP_Page_Down		0xff21	
#define GK_KP_End		0xff22	
#define GK_KP_Begin		0xff23	
                                                
#define GK_F1			0		
#define GK_F2			0		
#define GK_F3			0		
#define GK_F4			0		
#define GK_F5			0		
#define GK_F6			0		
#define GK_F7			0		
#define GK_F8			0		
#define GK_F9			0		
#define GK_F10			0		
#define GK_F11			0		
#define GK_L1			0		
#define GK_F12			0		
#define GK_L2			0		
#define GK_F13			0		
#define GK_L3			0		
#define GK_F14			0		
#define GK_L4			0		
#define GK_F15			0		
#define GK_L5			0		
#define GK_F16			0		
#define GK_L6			0		
#define GK_F17			0		
#define GK_L7			0		
#define GK_F18			0		
#define GK_L8			0		
#define GK_F19			0		
#define GK_L9			0		
#define GK_F20			0		
#define GK_L10			0		
#define GK_F21			0		
#define GK_R1			0		
#define GK_F22			0		
#define GK_R2			0		
#define GK_F23			0		
#define GK_R3			0		
#define GK_F24			0		
#define GK_R4			0		
#define GK_F25			0		
#define GK_R5			0		
#define GK_F26			0		
#define GK_R6			0		
#define GK_F27			0		
#define GK_R7			0		
#define GK_F28			0		
#define GK_R8			0		
#define GK_F29			0		
#define GK_R9			0		
#define GK_F30			0		
#define GK_R10			0		
#define GK_F31			0		
#define GK_R11			0		
#define GK_F32			0		
#define GK_R12			0		
#define GK_F33			0		
#define GK_R13			0		
#define GK_F34			0		
#define GK_R14			0		
#define GK_F35			0		
#define GK_R15			0		
                                                
#define GK_Shift_L		0	
#define GK_Shift_R		0	
#define GK_Control_L		0	
#define GK_Control_R		0	
#define GK_Caps_Lock		0	
#define GK_Shift_Lock		0	
                                                
#define GK_Meta_L		0	
#define GK_Meta_R		0	
#define GK_Alt_L		0	
#define GK_Alt_R		0	
#define GK_Super_L		0	
#define GK_Super_R		0	
#define GK_Hyper_L		0	
#define GK_Hyper_R		0

#define GK_Mode_switch 0xff7e
#endif	/* No X */

#endif

int GKeysymIsModifier(uint16 keysym);
