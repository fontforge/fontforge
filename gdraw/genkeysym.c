#include <stdio.h>
#include "../inc/gkeysym.h"

struct { char *name; int ks; } keysyms[] = {

	"BackSpace", XK_BackSpace,
	"Tab", 	XK_Tab,
	"Linefeed", XK_Linefeed	,
	"Clear", XK_Clear,	
	"Return", XK_Return,	
	"Pause", XK_Pause,	
	"Scroll_Lock", XK_Scroll_Lock	,
	"Sys_Req", XK_Sys_Req	,
	"Escape", XK_Escape,	
	"Delete", XK_Delete,	
                                                
	"Home", 	XK_Home, 
	"Left", 	XK_Left, 
	"Up", 	XK_Up, 
	"Right", XK_Right,	
	"Down", 	XK_Down, 
	"Prior", XK_Prior,	
	"Page_Up", XK_Page_Up,	
	"Next", 	XK_Next, 
	"Page_Down", XK_Page_Down,	
	"End", 	XK_End, 
	"Begin", XK_Begin,	
                                      
	"KP_Home", XK_KP_Home	,
	"KP_Left", XK_KP_Left	,
	"KP_Up", XK_KP_Up	,
	"KP_Right", XK_KP_Right	,
	"KP_Down", XK_KP_Down	,
	"KP_Prior", XK_KP_Prior	,
	"KP_Page_Up", XK_KP_Page_Up	,
	"KP_Next", XK_KP_Next	,
	"KP_Page_Down", XK_KP_Page_Down	,
	"KP_End", XK_KP_End	,
	"KP_Begin", XK_KP_Begin	,
  
	"F1", 	XK_F1, 
	"F2", 	XK_F2, 
	"F3", 	XK_F3, 
	"F4", 	XK_F4, 
	"F5", 	XK_F5, 
	"F6", 	XK_F6, 
	"F7", 	XK_F7, 
	"F8", 	XK_F8, 
	"F9", 	XK_F9, 
	"F10", 	XK_F10, 
	"F11", 	XK_F11, 
	"L1", 	XK_L1, 
	"F12", 	XK_F12, 
	"L2", 	XK_L2, 
	"F13", 	XK_F13, 
	"L3", 	XK_L3, 
	"F14", 	XK_F14, 
	"L4", 	XK_L4, 
	"F15", 	XK_F15, 
	"L5", 	XK_L5, 
	"F16", 	XK_F16, 
	"L6", 	XK_L6, 
	"F17", 	XK_F17, 
	"L7", 	XK_L7, 
	"F18", 	XK_F18, 
	"L8", 	XK_L8, 
	"F19", 	XK_F19, 
	"L9", 	XK_L9, 
	"F20", 	XK_F20, 
	"L10", 	XK_L10, 
	"F21", 	XK_F21, 
	"R1", 	XK_R1, 
	"F22", 	XK_F22, 
	"R2", 	XK_R2, 
	"F23", 	XK_F23, 
	"R3", 	XK_R3, 
	"F24", 	XK_F24, 
	"R4", 	XK_R4, 
	"F25", 	XK_F25, 
	"R5", 	XK_R5, 
	"F26", 	XK_F26, 
	"R6", 	XK_R6, 
	"F27", 	XK_F27, 
	"R7", 	XK_R7, 
	"F28", 	XK_F28, 
	"R8", 	XK_R8, 
	"F29", 	XK_F29, 
	"R9", 	XK_R9, 
	"F30", 	XK_F30, 
	"R10", 	XK_R10, 
	"F31", 	XK_F31, 
	"R11", 	XK_R11, 
	"F32", 	XK_F32, 
	"R12", 	XK_R12, 
	"F33", 	XK_F33, 
	"R13", 	XK_R13, 
	"F34", 	XK_F34, 
	"R14", 	XK_R14, 
	"F35", 	XK_F35, 
	"R15", 	XK_R15, 
                            
	"Shift_L", XK_Shift_L	,
	"Shift_R", XK_Shift_R	,
	"Control_L", XK_Control_L,
	"Control_R", XK_Control_R,
	"Caps_Lock", XK_Caps_Lock,
	"Shift_Lock", XK_Shift_Lock	,
                                                
	"Meta_L", XK_Meta_L	,
	"Meta_R", XK_Meta_R	,
	"Alt_L", XK_Alt_L	,
	"Alt_R", XK_Alt_R	,
	"Super_L", XK_Super_L	,
	"Super_R", XK_Super_R	,
	"Hyper_L", XK_Hyper_L	,
	"Hyper_R", XK_Hyper_R,
	NULL
};

int main() {
    int i, j;

    printf( "#include <stdio.h>\n" );
    printf( "#include <gdraw.h>\n\n" );
    for ( i=0; keysyms[i].name!=NULL; ++i ) {
	printf( "static unichar_t %s[] = { ", keysyms[i].name );
	for ( j=0; keysyms[i].name[j]!='\0'; ++j )
	    printf( "'%c', ", keysyms[i].name[j] );
	printf( "'\\0' };\n" );
    }
    printf("\n");

    printf( "unichar_t *GDrawKeysyms[] = { \n" );
    for ( i=0xff00; i<=0xffff; ++i ) {
	for ( j=0; keysyms[j].name!=NULL; ++j )
	    if ( keysyms[j].ks==i )
	break;
	if ( keysyms[j].name!=NULL )
	    printf( "\t%s,\n", keysyms[j].name );
	else
	    printf( "\tNULL,\n" );
    }
    printf( "\tNULL\n};\n" );
return( 0 );
}
