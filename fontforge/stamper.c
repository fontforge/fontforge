/*
 * This program is used to create a 'release date' time stamp when it is time for the
 * next fontforge release build. See fontforge/GNUmakefile.in
 */
#include <gutils.h>
#include <stdio.h>
#include <time.h>

static const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
	"Aug", "Sep", "Oct", "Nov", "Dec", NULL };

int main( int argc, char **argv ) {
    time_t now = GetTime();
    struct tm *tm = gmtime(&now);

    /* Let the user or developer know that this resulting output is generated rather than edited */
    printf( "/* This file was generated using stamper.c to create the next version release.          */\n" );
    printf( "/* If you need to update this to the next release version, see fontforge/GNUmakefile.in */\n\n" );
    if ( argc==1 ) {
	/* Generate a *.c type output if there is some variable on the command line */
	printf( "#include <fontforge-config.h>\n" );
	printf( "#include <time.h>\n\n" );
	printf( "const time_t source_modtime    = %ldL;\t/* Seconds since 1970 (standard unix time) */\n", now);
	printf( "const char *source_modtime_str = \"%02d:%02d GMT %d-%s-%d\";\n",
		tm->tm_hour, tm->tm_min,
		tm->tm_mday, months[tm->tm_mon], tm->tm_year+1900 );
	printf( "const char *source_version_str = \"%04d%02d%02d\";\t/* Year, month, day */\n",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday );
    } else {
	/* Generate a #define type output if there is no argument on the command line */
	printf( "#define LibFF_ModTime\t\t%ldL\t/* Seconds since 1970 (standard unix time) */\n", now);
	printf( "#define LibFF_ModTime_Str\t\"%02d:%02d GMT %d-%s-%d\"\n",
		tm->tm_hour, tm->tm_min,
		tm->tm_mday, months[tm->tm_mon], tm->tm_year+1900 );
	printf( "#define LibFF_VersionDate\t%04d%02d%02d\t/* Year, month, day */\n",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday );
    }

return( 0 );
}
