#ifndef MPID_MEMDEFS_H
#define MPID_MEMDEFS_H
#include <mpid_nem_copy.h>
#include <mpichconf.h>
#include <mpimem.h>

#define MALLOC(a) ({void *my_ptr___ = MPIU_Malloc(a); if (!my_ptr___) fprintf(stderr, "malloc failed %s:%d\n", __FILE__, __LINE__); my_ptr___;})
#define CALLOC(a,b)   MPIU_Calloc((a),(b))
#define FREE(a)       MPIU_Free((a)) 

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

#define MPID_NEM_MEMCPY_CROSSOVER (63*1024)
#define MPID_NEM_MEMCPY(a,b,c)  ((((c)) >= MPID_NEM_MEMCPY_CROSSOVER)	\
                                 ? amd_memcpy (a, b, c)			\
                                 : asm_memcpy (a, b, c))

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

static inline void amd64_cpy_nt (volatile void *dst, volatile void *src, size_t n)
{
    size_t n32 = (n) >> 5;
    size_t nleft = (n) & (32-1);
    
    //    printf ("\n1src = %p dst = %p n32 = %ld\n", dst, src, n32);
    if (n32)
    {
	//printf ("n32 = %ld\n", n32);
	asm volatile (".align 16  \n"
		      "1:  \n"
		      "mov (%1), %%r8  \n"
		      "mov 8(%1), %%r9  \n"
		      "add $32, %1  \n"
		      "movnti %%r8, (%2)  \n"
		      "movnti %%r9, 8(%2)  \n"
		      "add $32, %2  \n"
		      "mov -16(%1), %%r8  \n"
		      "mov -8(%1), %%r9  \n"
		      "dec %0  \n"
		      "movnti %%r8, -16(%2)  \n"
		      "movnti %%r9, -8(%2)  \n"
		      "jnz 1b  \n"
		      "sfence  \n"
		      "mfence  \n"
		      : "+a" (n32), "+S" (src), "+D" (dst)
		      : : "r8", "r9");
    }
    
    //   printf ("2src = %p dst = %p nleft = %ld\n", dst, src, nleft);
    if (nleft)
    {
	//printf ("nleft = %ld\n", nleft);
	memcpy ((void *)dst, (void *)src, nleft);
    }
}

#define MPID_NEM_MEMCPY_CROSSOVER (32*1024)
#define MPID_NEM_MEMCPY(a,b,c) (((c) >= MPID_NEM_MEMCPY_CROSSOVER) ? amd64_cpy_nt(a, b, c) : memcpy(a, b, c))
/* #define MPID_NEM_MEMCPY(a,b,c) (((c) < MPID_NEM_MEMCPY_CROSSOVER) ? memcpy(a, b, c) : amd64_cpy_nt(a, b, c)) */
/* #define MPID_NEM_MEMCPY(a,b,c) amd64_cpy_nt(a, b, c) */
/* #define MPID_NEM_MEMCPY(a,b,c) memcpy (a, b, c) */

#else
#define asm_memcpy(dst, src, n) memcpy(dst, src, n)
#define MPID_NEM_MEMCPY(a,b,c) memcpy(dst, src, n)
#endif

#endif /* MPID_MEMDEFS_H */
