#include <stdio.h>
#include <time.h>

static const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
	"Aug", "Sep", "Oct", "Nov", "Dec", NULL };
int main( int argc, char **argv ) {
    time_t now;
    struct tm *tm;

    time(&now);
    tm = gmtime(&now);
    if ( argc==1 ) {
	printf( "#include <time.h>\n\n" );
	printf( "const time_t source_modtime = %ldL;\n", now);
	printf( "const char *source_modtime_str = \"%02d:%02d GMT %d-%s-%d\";\n",
		tm->tm_hour, tm->tm_min,
		tm->tm_mday, months[tm->tm_mon], tm->tm_year+1900 );
	printf( "const char *source_version_str = \"%04d%02d%02d\";\n",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday );
    } else {
	printf( "#define LibFF_ModTime\t\t%ldL\t/* Seconds since 1970 (standard unix time) */\n", now);
	printf( "#define LibFF_ModTime_Str\t\"%02d:%02d GMT %d-%s-%d\"\n",
		tm->tm_hour, tm->tm_min,
		tm->tm_mday, months[tm->tm_mon], tm->tm_year+1900 );
	printf( "#define LibFF_VersionDate\t%04d%02d%02d\t/* Year, month, day */\n",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday );
    }

return( 0 );
}
