#include <stdio.h>
#include <time.h>

int main() {
    time_t now;
    struct tm *tm;

    time(&now);
    tm = localtime(&now);
    printf( "#include <time.h>\n\n" );
    printf( "const time_t link_time = %ld;\n", now);
    printf( "const char *link_time_str = \"%d:%02d %d-%d-%d\";\n",
	    tm->tm_hour, tm->tm_min,
	    tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900 );
return( 0 );
}
