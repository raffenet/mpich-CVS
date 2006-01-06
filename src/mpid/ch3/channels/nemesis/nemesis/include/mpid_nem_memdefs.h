#ifndef MPID_MEMDEFS_H
#define MPID_MEMDEFS_H
#include <mpid_nem_copy.h>
#include <mpichconf.h>

#define MALLOC(a)     malloc((a))
#define CALLOC(a,b)   calloc((unsigned)(a),(unsigned)(b))
#define FREE(a)       free((a)) 

#if defined(HAVE_GCC_AND_PENTIUM_ASM)
#define asm_memcpy(dst, src, n) ({					\
    const char *p = (char *)src;					\
    char *q = (char *)dst;						\
    size_t nl = (size_t)(n) >> 2;					\
    asm volatile ("cld ; rep ; movsl ; movl %3,%0 ; rep ; movsb"	\
 	                  : "+c" (nl), "+S" (p), "+D" (q)		\
	                  : "r" (n & 3));				\
    (void *)dst;							\
})
#elif defined(HAVE_GCC_AND_X86_64_ASM)
#define asm_memcpy(dst, src, n) ({					\
    const char *p = (char *)src;					\
    char *q = (char *)dst;						\
    size_t nq = n >> 3;							\
    asm volatile ("cld ; rep ; movsq ; movl %3,%%ecx ; rep ; movsb"	\
	                  : "+c" (nq), "+S" (p), "+D" (q)		\
	                  : "r" ((uint32_t)(n & 7)));			\
  (void *)dst;								\
})
#else
#define asm_memcpy(dst, src, n) memcpy(dst, src, n)
#endif



#ifdef HAVE_GCC_AND_PENTIUM_ASM
#define MPID_NEM_MEMCPY_CROSSOVER (63*1024)
#define MPID_NEM_MEMCPY(a,b,c)  ((((c)) >= MPID_NEM_MEMCPY_CROSSOVER) 	\
                                 ? amd_memcpy ((void *)a, b, c)			\
                                 : asm_memcpy ((void *)a, b, c))
#else /* HAVE_GCC_AND_PENTIUM_ASM */
#define MPID_NEM_MEMCPY(a,b,c)    memcpy ((void *)a, b, c)
#endif /* HAVE_GCC_AND_PENTIUM_ASM */


#endif /* MPID_MEMDEFS_H */
