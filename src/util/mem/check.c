#include <stdio.h>

extern int MPIU_Snprintf( char *, size_t, const char *, ... );

int main( int argc, char *argv[] )
{
    char buf[1000];
    int n;

    n = MPIU_Snprintf( buf, 100, "This is a test\n" );
    printf( "%d:%s", n, buf );

    n = MPIU_Snprintf( buf, 100, "This is a test %d\n", 3000000 );
    printf( "%d:%s", n, buf );

    n = MPIU_Snprintf( buf, 100, "This %s %% %d\n", "is a test for ", -3000 );
    printf( "%d:%s", n, buf );

    return 0;
}
