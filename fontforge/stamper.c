#include <stdio.h>
#include <time.h>

static const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
	"Aug", "Sep", "Oct", "Nov", "Dec", NULL };
int main() {
    time_t now;
    struct tm *tm;

    time(&now);
    tm = localtime(&now);
    printf( "#include <time.h>\n\n" );
    printf( "const time_t source_modtime = %ld;\n", now);
    printf( "const char *source_modtime_str = \"%02d:%02d %d-%s-%d\";\n",
	    tm->tm_hour, tm->tm_min,
	    tm->tm_mday, months[tm->tm_mon], tm->tm_year+1900 );
    printf( "const char *source_version_str = \"%04d%02d%02d\";\n",
	    tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday );
return( 0 );
}
