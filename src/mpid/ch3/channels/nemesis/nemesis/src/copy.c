
#include <mpid_nem_copy.h>
#include <stdio.h>
#include <string.h>


static inline void * my_memcpy(void * to, const void * from, size_t n)
{
  int d0, d1, d2;
  asm volatile(
        "rep ; movsl     \n"
        "testb $2,%b4    \n"
        "je 1f           \n"
        "movsw           \n"
        "1: testb $1,%b4 \n"
        "   je 2f        \n"
        "   movsb        \n"
        "2:              \n"
        : "=&c" (d0), "=&D" (d1), "=&S" (d2)
        :"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
        : "memory");
  return (to);
}

void mmx_copy (void *dest, const void *src, int nblock);


void *amd_memcpy (void *to, const void *from, size_t length)
{
    char *dest;
    const char *src;
    int len;

    if (length < 16)
    {
	my_memcpy (to, from, length);
    }
    else
    {
	dest = to;
	src  = from;

	/* make dest ptr divisible by 16 */
	if ((int) dest % 16)
	{
	    len = 16 - ((int) dest % 16);
	    my_memcpy (dest, src, len); 
	    dest += len;
	    src  += len;
	    length -= len;
	}

	/* Copy main chunk */

	len = length - (length % 128);

	if (len > 0)
	{
	    mmx_copy (dest, src, len);
	    dest += len;
	    src  += len;
	    length -= len;
	}

	/* Copy whats not divisibly by 8K */

	my_memcpy (dest, src, length);
    }
    return to;
}

