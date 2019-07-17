#include "gkeysym.h"

#include <stdio.h>

struct { char *name; int ks; } keysyms[] = {

	"BackSpace", GK_BackSpace,
	"Tab", 	GK_Tab,
	"Linefeed", GK_Linefeed	,
	"Clear", GK_Clear,	
	"Return", GK_Return,	
	"Pause", GK_Pause,	
	"Scroll_Lock", GK_Scroll_Lock	,
	"Sys_Req", GK_Sys_Req	,
	"Escape", GK_Escape,	
	"Delete", GK_Delete,	
                                                
	"Home", 	GK_Home, 
	"Left", 	GK_Left, 
	"Up", 	GK_Up, 
	"Right", GK_Right,	
	"Down", 	GK_Down, 
	"Prior", GK_Prior,	
	"Page_Up", GK_Page_Up,	
	"Next", 	GK_Next, 
	"Page_Down", GK_Page_Down,	
	"End", 	GK_End, 
	"Begin", GK_Begin,	
                                      
	"KP_Home", GK_KP_Home	,
	"KP_Left", GK_KP_Left	,
	"KP_Up", GK_KP_Up	,
	"KP_Right", GK_KP_Right	,
	"KP_Down", GK_KP_Down	,
	"KP_Prior", GK_KP_Prior	,
	"KP_Page_Up", GK_KP_Page_Up	,
	"KP_Next", GK_KP_Next	,
	"KP_Page_Down", GK_KP_Page_Down	,
	"KP_End", GK_KP_End	,
	"KP_Begin", GK_KP_Begin	,
  
	"F1", 	GK_F1, 
	"F2", 	GK_F2, 
	"F3", 	GK_F3, 
	"F4", 	GK_F4, 
	"F5", 	GK_F5, 
	"F6", 	GK_F6, 
	"F7", 	GK_F7, 
	"F8", 	GK_F8, 
	"F9", 	GK_F9, 
	"F10", 	GK_F10, 
	"F11", 	GK_F11, 
	"L1", 	GK_L1, 
	"F12", 	GK_F12, 
	"L2", 	GK_L2, 
	"F13", 	GK_F13, 
	"L3", 	GK_L3, 
	"F14", 	GK_F14, 
	"L4", 	GK_L4, 
	"F15", 	GK_F15, 
	"L5", 	GK_L5, 
	"F16", 	GK_F16, 
	"L6", 	GK_L6, 
	"F17", 	GK_F17, 
	"L7", 	GK_L7, 
	"F18", 	GK_F18, 
	"L8", 	GK_L8, 
	"F19", 	GK_F19, 
	"L9", 	GK_L9, 
	"F20", 	GK_F20, 
	"L10", 	GK_L10, 
	"F21", 	GK_F21, 
	"R1", 	GK_R1, 
	"F22", 	GK_F22, 
	"R2", 	GK_R2, 
	"F23", 	GK_F23, 
	"R3", 	GK_R3, 
	"F24", 	GK_F24, 
	"R4", 	GK_R4, 
	"F25", 	GK_F25, 
	"R5", 	GK_R5, 
	"F26", 	GK_F26, 
	"R6", 	GK_R6, 
	"F27", 	GK_F27, 
	"R7", 	GK_R7, 
	"F28", 	GK_F28, 
	"R8", 	GK_R8, 
	"F29", 	GK_F29, 
	"R9", 	GK_R9, 
	"F30", 	GK_F30, 
	"R10", 	GK_R10, 
	"F31", 	GK_F31, 
	"R11", 	GK_R11, 
	"F32", 	GK_F32, 
	"R12", 	GK_R12, 
	"F33", 	GK_F33, 
	"R13", 	GK_R13, 
	"F34", 	GK_F34, 
	"R14", 	GK_R14, 
	"F35", 	GK_F35, 
	"R15", 	GK_R15, 
                            
	"Shift_L", GK_Shift_L	,
	"Shift_R", GK_Shift_R	,
	"Control_L", GK_Control_L,
	"Control_R", GK_Control_R,
	"Caps_Lock", GK_Caps_Lock,
	"Shift_Lock", GK_Shift_Lock	,
                                                
	"Meta_L", GK_Meta_L	,
	"Meta_R", GK_Meta_R	,
	"Alt_L", GK_Alt_L	,
	"Alt_R", GK_Alt_R	,
	"Super_L", GK_Super_L	,
	"Super_R", GK_Super_R	,
	"Hyper_L", GK_Hyper_L	,
	"Hyper_R", GK_Hyper_R,
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
