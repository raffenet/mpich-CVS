#include <stdio.h>

int main()
{
    setvbuf(stdout,NULL,_IOLBF,0);  
    printf( "first line\n" );
    sleep(1);
    printf( "second line\n" );
    sleep(1); 
    printf( "last line\n" ); 
    fflush(stdout);
    return 0;
}
