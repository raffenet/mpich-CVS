#include "mpe_logging_conf.h"
#include "clog.h"

/*
    Replace CLOGByteSwapDouble() and CLOGByteSwapInt() by the more general
    and more efficient CLOG_byteswap().
    (The measurement was done by ts_byteswap.c on Linux P4 2.4Ghz with
     gcc 2.96 or winXP P4 1.4Ghz with gcc 3.3.1.  A 15%-70% improvement
     depends on optimization level.)

    input/output  bytes    - pointer to the buffer
    input         elem_sz  - e.g. sizeof(short), sizeof(int) or sizeof(double)
    input         Nelem    - number of elements, each is of size elem_sz
 */
#if ! defined(WORDS_BIGENDIAN)
void CLOG_byteswap( void  *bytes,
                    int    elem_sz,
                    int    Nelem )
{
    char *bptr;
    char  btmp;
    int end_ii;
    int ii, jj;
                                                                                
    bptr = (char *) bytes;
    for ( jj = 0; jj < Nelem; jj++ ) {
         for ( ii = 0; ii < elem_sz/2; ii++ ) {
             end_ii          = elem_sz - 1 - ii;
             btmp            = bptr[ ii ];
             bptr[ ii ]      = bptr[ end_ii ];
             bptr[ end_ii ]  = btmp;
         }
         bptr += elem_sz;
    }
}
#else
void CLOG_byteswap( void *bytes, int elem_sz, int Nelem ){}
#endif
