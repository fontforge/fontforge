#include <stdio.h>
#include <time.h>

int main() {
    time_t now;
    struct tm *tm;

    time(&now);
    tm = localtime(&now);
    printf( "#include <time.h>\n\n" );
    printf( "const time_t source_modtime = %ld;\n", now);
    printf( "const char *source_modtime_str = \"%d:%02d %d-%d-%d\";\n",
	    tm->tm_hour, tm->tm_min,
	    tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900 );
    printf( "const char *source_version_str = \"%02d%02d%02d\";\n",
	    tm->tm_year-100, tm->tm_mon+1, tm->tm_mday );
return( 0 );
}
