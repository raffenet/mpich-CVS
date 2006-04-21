#ifndef MPID_NEM_ATOMICS_H
#define MPID_NEM_ATOMICS_H

#include <mpichconf.h>

static inline void *MPID_NEM_SWAP (volatile void *ptr, void *val)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    asm volatile ("xchgl %0,%1"
		  :"=r" (val)
		  :"m" (*(void **)ptr), "0" (val)
		  :"memory");
    return val;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    asm volatile ("xchgq %0,%1"
		  :"=r" (val)
		  :"m" (*(void **)ptr), "0" (val)
		  :"memory");
    return val;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    asm volatile ("xchg8 %0=[%1],%2" : "=r" (val)
                  : "r" (ptr), "0" (val)
                  : "memory");
    return val;
#else
#error No swap function defined for this architecture
#endif
}


static inline void *MPID_NEM_CAS (volatile void *ptr, void *oldv, void *newv)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    void *prev;
    asm volatile ("lock ; cmpxchgl %1,%2"
		  : "=a" (prev)
		  : "q" (newv), "m" (*(void **)ptr), "0" (oldv)
		  : "memory");
    return prev;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    void *prev;
    asm volatile ("lock ; cmpxchgq %1,%2"
		  : "=a" (prev)
		  : "q" (newv), "m" (*(void **)ptr), "0" (oldv)
		  : "memory");
    return prev;   
#elif defined(HAVE_GCC_AND_IA64_ASM)
    void *prev;
    asm volatile ("mov ar.ccv=%1;;"
                  "cmpxchg8.rel %0=[%2],%3,ar.ccv"
                  : "=r"(prev)
                  : "rO"(oldv), "r"(ptr), "r"(newv)
                  : "memory");
    return prev;   
#else
#error No compare-and-swap function defined for this architecture
#endif
}

static inline int MPID_NEM_FETCH_AND_ADD (volatile int *ptr, int val)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    asm volatile ("lock ; xaddl %0,%1"
		  : "=r" (val)
		  : "m" (*ptr), "0" (val));
    return val;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    asm volatile ("lock ; xadd %0,%1" /* xadd not xaddq: let the assembler choose which one to use */
		  : "=r" (val)
		  : "m" (*ptr), "0" (val));
    return val;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int new;
    int prev;
    int old;
    
    do
    {
        old = *ptr;
        new = old + val;
        asm volatile ("mov ar.ccv=%1;;"
                      "cmpxchg4.rel %0=[%2],%3,ar.ccv"
                      : "=r"(prev)
                      : "rO"(old), "r"(ptr), "r"(new)
                      : "memory");
    }
    while (prev != old);
    return new;
#else
    /* default implementation */
    int new;
    int prev;
    int old;
    
    MPIU_Assert (sizeof(int) == sizeof(void *));
    do
    {
        old = *ptr;
        new = old + val;
        prev = (int)MPID_NEM_CAS (ptr, (void *)old, (void *)new)
    }
    while (prev != old);
    return new;
#endif
}

static inline void MPID_NEM_ATOMIC_ADD (int *ptr, int val)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    asm volatile ("lock ; addl %1,%0"
		  :"=m" (*ptr)
		  :"ir" (val), "m" (*ptr));
    return;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    asm volatile ("lock ; addq %1,%0"
		  :"=m" (*ptr)
		  :"ir" (val), "m" (*ptr));
    return;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    MPIU_Assertp (0);; /* implement atomic add */
#else
#error No fetch-and-add function defined for this architecture
#endif
}

static inline int MPID_NEM_FETCH_AND_INC (volatile int *ptr)
{
#ifdef HAVE_GCC_AND_IA64_ASM
    int val;
    asm volatile ("fetchadd4.rel %0=[%1],%2"
                  : "=r"(val) : "r"(ptr), "i" (1)
                  : "memory");
    return val;
#else
    /* default implementation */
    return MPID_NEM_FETCH_AND_ADD (ptr, 1);
#endif
}

static inline void MPID_NEM_ATOMIC_INC (int *ptr)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    asm volatile ("lock ; incl %0"
		  :"=m" (*ptr)
		  :"m" (*ptr));
    return;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    asm volatile ("lock ; incq %0"
		  :"=m" (*ptr)
		  :"m" (*ptr));
    return;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int val;
    asm volatile ("fetchadd4.rel %0=[%1],%2"
                  : "=r"(val) : "r"(ptr), "i" (1)
                  : "memory");
    return;
#else
#error No fetch-and-add function defined for this architecture
#endif
}

static inline void MPID_NEM_ATOMIC_DEC (int *ptr)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    asm volatile ("lock ; decl %0"
		  :"=m" (*ptr)
		  :"m" (*ptr));
    return;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    asm volatile ("lock ; decq %0"
		  :"=m" (*ptr)
		  :"m" (*ptr));
    return;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int val;
    asm volatile ("fetchadd4.rel %0=[%1],%2"
                  : "=r"(val) : "r"(ptr), "i" (-1)
                  : "memory");
    return;
#else
#error No fetch-and-add function defined for this architecture
#endif
}


#ifdef HAVE_GCC_AND_PENTIUM_ASM

#ifdef HAVE_GCC_ASM_AND_X86_SFENCE
#define MPID_NEM_WRITE_FENCE() asm volatile  ( "sfence" ::: "memory" )
#else /* HAVE_GCC_ASM_AND_X86_SFENCE */
#define MPID_NEM_WRITE_FENCE()
#endif /* HAVE_GCC_ASM_AND_X86_SFENCE */

#ifdef HAVE_GCC_ASM_AND_X86_LFENCE
/*
  #define MPID_NEM_READ_FENCE() asm volatile  ( ".byte 0x0f, 0xae, 0xe8" ::: "memory" ) */
#define MPID_NEM_READ_FENCE() asm volatile  ( "lfence" ::: "memory" )
#else /* HAVE_GCC_ASM_AND_X86_LFENCE */
#define MPID_NEM_READ_FENCE()
#endif /* HAVE_GCC_ASM_AND_X86_LFENCE */


#ifdef HAVE_GCC_ASM_AND_X86_MFENCE
/*
  #define MPID_NEM_READ_WRITE_FENCE() asm volatile  ( ".byte 0x0f, 0xae, 0xf0" ::: "memory" )
*/
#define MPID_NEM_READ_WRITE_FENCE() asm volatile  ( "mfence" ::: "memory" )
#else /* HAVE_GCC_ASM_AND_X86_MFENCE */
#define MPID_NEM_READ_WRITE_FENCE()
#endif /* HAVE_GCC_ASM_AND_X86_MFENCE */

#elif defined(HAVE_MASM_AND_X86)
#define MPID_NEM_WRITE_FENCE()
#define MPID_NEM_READ_FENCE() __asm { __asm _emit 0x0f __asm _emit 0xae __asm _emit 0xe8 }
#define MPID_NEM_READ_WRITE_FENCE()

#elif defined(HAVE_GCC_AND_IA64_ASM)
#define MPID_NEM_WRITE_FENCE() //asm volatile  ("mf" ::: "memory" )
#define MPID_NEM_READ_FENCE() //asm volatile  ("mf" ::: "memory" )
#define MPID_NEM_READ_WRITE_FENCE() //asm volatile  ("mf" ::: "memory" )

#else
#define MPID_NEM_WRITE_FENCE()
#define MPID_NEM_READ_FENCE()
#define MPID_NEM_READ_WRITE_FENCE()
#endif /* HAVE_GCC_AND_PENTIUM_ASM */


#endif /* MPID_NEM_ATOMICS_H */
