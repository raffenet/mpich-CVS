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

/*
   nt_memcpy (dst, src, len)
   This performs a memcopy using non-temporal stores.  It's optimized
   for ia_32 machines.

   The general idea is to prefetch a block of the source data into the
   cache, then read the data from the source buffer into 64-bit mmx
   registers so that the data can be written to the destination buffer
   using non-temporal move instructions.

   This is done in three steps:  copy 8K or larger chunks, copy (8K,
   128B] chunks, and copy the rest.

   In the first step, the main loop prefetches an 8K chunk, by reading
   one element from each cacheline.  Then we copy that 8K chunk, 64
   bytes at a time (8bytes per mmx reg * 8 mmx regs) using
   non-temporal stores.  Rinse and repeat.

   The second step is essentially the same as the first, except that
   the amount of data to be copied in that step is less than 8K, so we
   prefetch all of the data.  These two steps could have been combined
   but I think I saved some time by simplifying the main loop in step
   one by not checking if we have to prefetch less than 8K.

   The last step just copies whatever's left.
   
 */
static inline void *nt_memcpy (volatile void *dst, volatile void *src, size_t len)
{
    void *orig_dst = (void *)dst;
    int n;

    /* copy in 8K chunks */
    n = len & (-8*1024);
    if (n)
    {

	asm volatile (".set PREFETCHBLOCK, 1024\n" /* prefetch PREFETCHBLOCK number of 8-byte words */
		      "lea (%%esi, %%ecx, 8), %%esi\n"
		      "lea (%%edi, %%ecx, 8), %%edi\n"
		  
		      "neg %%ecx\n"
		      "emms\n"
		  
		      "1:\n" /* main loop */

		      /* eax is the prefetch loop iteration counter */
		      "mov $PREFETCHBLOCK/16, %%eax\n"  /* only need to touch one element per cacheline, and we're doing two at once  */

		      /* prefetch 2 cachelines at a time (128 bytes) */
		      "2:\n" /* prefetch loop */
		      "mov (%%esi, %%ecx, 8), %%ebx\n"
		      "mov 64(%%esi, %%ecx, 8), %%ebx\n"
		      "add $16, %%ecx\n" 
		  
		      "dec %%eax\n"
		      "jnz 2b\n"
		      "sub $PREFETCHBLOCK, %%ecx\n"

		      /* eax is the copy loop iteration counter */
		      "mov $PREFETCHBLOCK/8, %%eax\n"

		      /* copy data 64 bytes at a time */
		      "3:\n" /* copy loop */
		      "movq (%%esi, %%ecx, 8), %%mm0\n"
		      "movq 8(%%esi, %%ecx, 8), %%mm1\n"
		      "movq 16(%%esi, %%ecx, 8), %%mm2\n"
		      "movq 24(%%esi, %%ecx, 8), %%mm3\n"
		      "movq 32(%%esi, %%ecx, 8), %%mm4\n"
		      "movq 40(%%esi, %%ecx, 8), %%mm5\n"
		      "movq 48(%%esi, %%ecx, 8), %%mm6\n"
		      "movq 56(%%esi, %%ecx, 8), %%mm7\n"
		  
		      "movntq %%mm0, (%%edi, %%ecx, 8)\n"
		      "movntq %%mm1, 8(%%edi, %%ecx, 8)\n"
		      "movntq %%mm2, 16(%%edi, %%ecx, 8)\n"
		      "movntq %%mm3, 24(%%edi, %%ecx, 8)\n"
		      "movntq %%mm4, 32(%%edi, %%ecx, 8)\n"
		      "movntq %%mm5, 40(%%edi, %%ecx, 8)\n"
		      "movntq %%mm6, 48(%%edi, %%ecx, 8)\n"
		      "movntq %%mm7, 56(%%edi, %%ecx, 8)\n"

		      "add $8, %%ecx\n"
		      "dec %%eax\n"
		      "jnz 3b\n"

		      "or %%ecx, %%ecx\n"
		      "jnz 1b\n"

		      "sfence\n"
		      "emms\n"
		      : 
		      : "D" (dst), "S" (src), "c" (n >> 3)
		      : "eax", "ebx");

	src = (char *)src + n;
	dst = (char *)dst + n;
    }
    
    /* copy in 128byte chunks */
    n = len & (8*1024 - 1) & -128;
    if (n)
    {
	asm volatile ("lea (%%esi, %%ecx, 8), %%esi\n"
		      "lea (%%edi, %%ecx, 8), %%edi\n"

		      "push %%ecx\n"        /* save n */
			  
		      "mov %%ecx, %%eax\n" /* prefetch loopctr = n/128 */
		      "shr $4, %%eax\n" 
		  
		      "neg %%ecx\n"
		      "emms\n"

		      /* prefetch all data to be copied 2 cachelines at a time (128 bytes)*/
		      "1:\n" /* prefetch loop */
		      "mov (%%esi, %%ecx, 8), %%ebx\n"
		      "mov 64(%%esi, %%ecx, 8), %%ebx\n"
		      "add $16, %%ecx\n"
		  
		      "dec %%eax\n"
		      "jnz 1b\n"

		      "pop %%ecx\n" /* restore n */

		      "mov %%ecx, %%eax\n" /* write loopctr = n/64 */
		      "shr $3, %%eax\n"
		      "neg %%ecx\n"

		      /* copy data 64 bytes at a time */
		      "2:\n" /* copy loop */
		      "movq (%%esi, %%ecx, 8), %%mm0\n"
		      "movq 8(%%esi, %%ecx, 8), %%mm1\n"
		      "movq 16(%%esi, %%ecx, 8), %%mm2\n"
		      "movq 24(%%esi, %%ecx, 8), %%mm3\n"
		      "movq 32(%%esi, %%ecx, 8), %%mm4\n"
		      "movq 40(%%esi, %%ecx, 8), %%mm5\n"
		      "movq 48(%%esi, %%ecx, 8), %%mm6\n"
		      "movq 56(%%esi, %%ecx, 8), %%mm7\n"
		  
		      "movntq %%mm0, (%%edi, %%ecx, 8)\n"
		      "movntq %%mm1, 8(%%edi, %%ecx, 8)\n"
		      "movntq %%mm2, 16(%%edi, %%ecx, 8)\n"
		      "movntq %%mm3, 24(%%edi, %%ecx, 8)\n"
		      "movntq %%mm4, 32(%%edi, %%ecx, 8)\n"
		      "movntq %%mm5, 40(%%edi, %%ecx, 8)\n"
		      "movntq %%mm6, 48(%%edi, %%ecx, 8)\n"
		      "movntq %%mm7, 56(%%edi, %%ecx, 8)\n"

		      "add $8, %%ecx\n"
		      "dec %%eax\n"
		      "jnz 2b\n"

		      "sfence\n"
		      "emms\n"
		      : 
		      : "D" (dst), "S" (src), "c" (n >> 3)
		      : "eax", "ebx");
	src = (char *)src + n;
	dst = (char *)dst + n;
    }
    
    /* copy leftover */
    n = len & (128 - 1);
    if (n)
	asm_memcpy (dst, src, n);
    
    return orig_dst;
}
#define MPID_NEM_MEMCPY_CROSSOVER (63*1024)
#define MPID_NEM_MEMCPY(a,b,c)  ((((c)) >= MPID_NEM_MEMCPY_CROSSOVER)	\
                                 ? nt_memcpy (a, b, c)			\
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
