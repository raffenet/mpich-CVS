#ifndef MPID_MEMDEFS_H
#define MPID_MEMDEFS_H
#include <mpid_nem_copy.h>

#define MALLOC(a)     malloc((a))
#define CALLOC(a,b)   calloc((unsigned)(a),(unsigned)(b))
#define FREE(a)       free((a)) 

#if 1 /* USE A MACRO */
#if defined(__i386__)
#define asm_memcpy(dst, src, n) ({						\
    const char *p = (char *)src;						\
    char *q = (char *)dst;							\
    size_t nl = (size_t)(n) >> 2;						\
    asm volatile ("cld ; rep ; movsl ; movl %3,%0 ; rep ; movsb"	\
 	                  : "+c" (nl), "+S" (p), "+D" (q)			\
	                  : "r" (n & 3));					\
    (void *)dst;								\
})
#elif defined(__x86_64__)
#define asm_memcpy(dst, src, n) ({						\
    const char *p = (char *)src;						\
    char *q = (char *)dst;							\
    size_t nq = n >> 3;								\
    asm volatile ("cld ; rep ; movsq ; movl %3,%%ecx ; rep ; movsb"	\
	                  : "+c" (nq), "+S" (p), "+D" (q)			\
	                  : "r" ((uint32_t)(n & 7)));				\
  (void *)dst;									\
})
#else
#define asm_memcpy(dst, src, n) ({		\
    const char *p = (char *)src;		\
    char *q = (char *)dst;			\
    size_t _n = (size_t)(n);			\
    while ( (_n)-- )				\
    {						\
        *q++ = *p++;				\
    }						\
    (void *)dst;				\
})
#endif

#else /* USE A MACRO */
static void *
asm_memcpy(void *dst, const void *src, size_t n)
{
    const char *p = src;
    char *q = dst;
#if defined(__i386__)
    size_t nl = n >> 2;
    asm volatile ("cld ; rep ; movsl ; movl %3,%0 ; rep ; movsb"
			  : "+c" (nl), "+S" (p), "+D" (q)
			  : "r" (n & 3));
#elif defined(__x86_64__)
    size_t nq = n >> 3;
    asm volatile ("cld ; rep ; movsq ; movl %3,%%ecx ; rep ; movsb"
			  : "+c" (nq), "+S" (p), "+D" (q)
			  : "r" ((uint32_t)(n & 7)));
#else
    while ( n-- ) {
	*q++ = *p++;
    }
#endif

    return dst;
}
#endif /* USE A MACRO */


#ifdef i386
/*#define MPID_NEM_MEMCPY(a,b,c)    asm_memcpy((a), (b), (unsigned)(c)) */
#define MPID_NEM_MEMCPY_CROSSOVER (63*1024)
#define MPID_NEM_MEMCPY(a,b,c)  (((c)) >= MPID_NEM_MEMCPY_CROSSOVER) ? amd_memcpy (a, b, c) : asm_memcpy (a, b, c)
#else /* i386 */
#define MPID_NEM_MEMCPY(a,b,c)    memcpy (a, b, c)
#endif /* i386 */


#endif /* MPID_MEMDEFS_H */
