
@BOTTOM@

/* Define WINDOWS specific features */
/*
 *    Windows' open() opens an ASCII file by default, add Windows specific
 *       flag O_BINARY to open()'s argument
 *       */
#ifdef HAVE_WINDOWS_H
#define OPEN( a, b, c )    open( a, b | O_BINARY, c )
#else
#ifdef _LARGEFILE64_SOURCE
#define OPEN( a, b, c )    open( a, b | O_LARGEFILE, c )
#else
#define OPEN( a, b, c )    open( a, b, c )
#endif
#endif
