#include "param.h"
#include <stdio.h>

int main( int argc, char *argv[] )
{
    int val;

    MPIU_Param_init( 0, 0, "sample.prm" );

    MPIU_Param_bcast( );
    
    MPIU_Param_get_int( "SOCKBUFSIZE", 65536, &val );

    printf( "Socket size is %d\n", val );

    MPIU_Param_finalize( );
}
