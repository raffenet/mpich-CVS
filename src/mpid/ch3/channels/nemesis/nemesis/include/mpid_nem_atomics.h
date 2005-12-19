#ifndef MPID_NEM_ATOMICS_H
#define MPID_NEM_ATOMICS_H

#define HAVE_GCC_AND_PENTIUM_ASM    1
#define HAVE_GCC_ASM_AND_X86_SFENCE 1
#define HAVE_GCC_ASM_AND_X86_MFENCE 1 
#define HAVE_GCC_ASM_AND_X86_MFENCE 1

#define LOCK_PREFIX "lock ; "

/* This dummy struct and DC (i.e., Dummy Cast) macro are needed so
   that we can use a void* for swap and cas.  Note that ptr needs to
   be dereferenced in the asm block, but you can't do that if ptr is a
   void*, so we cast it to something else first. */
typedef struct { char x[100]; } MPID_NEM_DUMMY_STRUCT;
#define DC(x) ((MPID_NEM_DUMMY_STRUCT *)(x))

#define MPID_NEM_SWAP(ptr, val) ((__typeof__ (*(ptr)))MPID_NEM_SWAP_ ((void *)(ptr), (unsigned long)(val)))

static inline unsigned long MPID_NEM_SWAP_ (void *ptr, unsigned long val)
{
#ifdef i386
    __asm__ __volatile__ ("xchgl %0,%1"
			  :"=r" (val)
			  :"m" (*DC(ptr)), "0" (val)
			  :"memory");
    return val;
#elif defined(x86_64)
    __asm__ __volatile__ ("xchgq %0,%1"
			  :"=r" (val)
			  :"m" (*DC(ptr)), "0" (val)
			  :"memory");
    return val;
#else
#error No swap function defined for this architecture
#endif
}


#define MPID_NEM_CAS(ptr, oldv, newv) ((__typeof__(*(ptr)))MPID_NEM_CAS_ ((void *)(ptr), (unsigned long)(oldv), (unsigned long)(newv)))

static inline unsigned long MPID_NEM_CAS_ (void *ptr, unsigned long oldv, unsigned long newv)
{
#ifdef i386
    unsigned long prev;
    __asm__ __volatile__ (LOCK_PREFIX "cmpxchgl %1,%2"
			  : "=a" (prev)
			  : "q" (newv), "m" (*DC(ptr)), "0" (oldv)
			  : "memory");
    return prev;
#elif defined(x86_64)
    unsigned long prev;
    __asm__ __volatile__ (LOCK_PREFIX "cmpxchgq %1,%2"
			  : "=a" (prev)
			  : "q" (newv), "m" (*DC(ptr)), "0" (oldv)
			  : "memory");
    return prev;   
#else
#error No compare-and-swap function defined for this architecture
#endif
}

#define MPID_NEM_FETCH_AND_ADD(ptr, val) MPID_NEM_FETCH_AND_ADD_ ((void *)(ptr), (unsigned long)(val))

static inline unsigned long MPID_NEM_FETCH_AND_ADD_ (int *ptr, unsigned long val)
{
#ifdef i386
    __asm__ __volatile__ (LOCK_PREFIX "xaddl %0,%1"
			  : "=r" (val)
			  : "m" (*ptr), "0" (val));
    return val;
#elif defined(x86_64)
    __asm__ __volatile__ (LOCK_PREFIX "xaddq %0,%1"
			  : "=r" (val)
			  : "m" (*ptr), "0" (val));
    return val;
#else
#error No fetch-and-add function defined for this architecture
#endif
}

#define MPID_NEM_ATOMIC_ADD(val, ptr) MPID_NEM_ATOMIC_ADD_ ((val), (ptr))

static inline void MPID_NEM_ATOMIC_ADD_ (int val, int *ptr)
{
#ifdef i386
    __asm__ __volatile__ (LOCK_PREFIX "addl %1,%0"
			  :"=m" (*ptr)
			  :"ir" (val), "m" (*ptr));
    return;
#elif defined(x86_64)
    __asm__ __volatile__ (LOCK_PREFIX "addq %1,%0"
			  :"=m" (*ptr)
			  :"ir" (val), "m" (*ptr));
    return;
#else
#error No fetch-and-add function defined for this architecture
#endif
}

#define MPID_NEM_ATOMIC_INC(ptr) MPID_NEM_ATOMIC_INC_ (ptr)

static inline void MPID_NEM_ATOMIC_INC_ (int *ptr)
{
#ifdef i386
    __asm__ __volatile__ (LOCK_PREFIX "incl %0"
			  :"=m" (*ptr)
			  :"m" (*ptr));
    return;
#elif defined(x86_64)
    __asm__ __volatile__ (LOCK_PREFIX "incq %0"
			  :"=m" (*ptr)
			  :"m" (*ptr));
    return;
#else
#error No fetch-and-add function defined for this architecture
#endif
}

#define MPID_NEM_ATOMIC_DEC(ptr) MPID_NEM_ATOMIC_DEC_ (ptr)

static inline void MPID_NEM_ATOMIC_DEC_ (int *ptr)
{
#ifdef i386
    __asm__ __volatile__ (LOCK_PREFIX "decl %0"
			  :"=m" (*ptr)
			  :"m" (*ptr));
    return;
#elif defined(x86_64)
    __asm__ __volatile__ (LOCK_PREFIX "decq %0"
			  :"=m" (*ptr)
			  :"m" (*ptr));
    return;
#else
#error No fetch-and-add function defined for this architecture
#endif
}

#undef MPID_NEM_DUMMY_STRUCT

#if REMOVE_THIS


struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((struct __xchg_dummy *)(x))

static inline unsigned long __xchg (unsigned long x, volatile void * ptr, int size)
{
#ifdef i386
    switch (size)
    {
    case 1:
	__asm__ __volatile__("xchgb %b0,%1"
			     :"=q" (x)
			     :"m" (*__xg(ptr)), "0" (x)
			     :"memory");
	break;
    case 2:
	__asm__ __volatile__("xchgw %w0,%1"
			     :"=r" (x)
			     :"m" (*__xg(ptr)), "0" (x)
			     :"memory");
	break;
    case 4:
	__asm__ __volatile__("xchgl %0,%1"
			     :"=r" (x)
			     :"m" (*__xg(ptr)), "0" (x)
			     :"memory");
	break;
    default:
	assert (0);
    }
    return x;
#elif defined(x86_64)

    switch (size) 
    {
    case 1:
	__asm__ __volatile__ ("xchgb %b0,%1"
			      :"=q" (x)
			      :"m" (*__xg (ptr)), "0" (x)
			      :"memory");
	break;
    case 2:
	__asm__ __volatile__ ("xchgw %w0,%1"
			      :"=r" (x)
			      :"m" (*__xg (ptr)), "0" (x)
			      :"memory");
	break;
    case 4:
	__asm__ __volatile__ ("xchgl %k0,%1"
			      :"=r" (x)
			      :"m" (*__xg (ptr)), "0" (x)
			      :"memory");
	break;
    case 8:
	__asm__ __volatile__ ("xchgq %0,%1"
			      :"=r" (x)
			      :"m" (*__xg (ptr)), "0" (x)
			      :"memory");
	break;
    }
    return x;
#else
#error No swap function defined for this arch
#endif
}

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */

static inline unsigned long __cmpxchg (volatile void *ptr, unsigned long old, unsigned long new, int size)
{
#ifdef i386
    unsigned long prev;
    switch (size)
    {
    case 1:
	__asm__ __volatile__(LOCK_PREFIX "cmpxchgb %b1,%2"
			     : "=a"(prev)
			     : "q"(new), "m"(*__xg(ptr)), "0"(old)
			     : "memory");
	return prev;
    case 2:
	__asm__ __volatile__(LOCK_PREFIX "cmpxchgw %w1,%2"
			     : "=a"(prev)
			     : "q"(new), "m"(*__xg(ptr)), "0"(old)
			     : "memory");
	return prev;
    case 4:
	__asm__ __volatile__(LOCK_PREFIX "cmpxchgl %1,%2"
			     : "=a"(prev)
			     : "q"(new), "m"(*__xg(ptr)), "0"(old)
			     : "memory");
	return prev;
    default:
	assert (0);
    }
    return old;
#elif defined(x86_64)
    unsigned long prev;
    switch (size) 
    {
    case 1:
	__asm__ __volatile__ (LOCK_PREFIX "cmpxchgb %b1,%2"
			      : "=a"(prev)
			      : "q"(new), "m"(*__xg (ptr)), "0"(old)
			      : "memory");
	return prev;
    case 2:
	__asm__ __volatile__ (LOCK_PREFIX "cmpxchgw %w1,%2"
			      : "=a"(prev)
			      : "q"(new), "m"(*__xg (ptr)), "0"(old)
			      : "memory");
	return prev;
    case 4:
	__asm__ __volatile__ (LOCK_PREFIX "cmpxchgl %k1,%2"
			      : "=a"(prev)
			      : "q"(new), "m"(*__xg (ptr)), "0"(old)
			      : "memory");
	return prev;
    case 8:
	__asm__ __volatile__ (LOCK_PREFIX "cmpxchgq %1,%2"
			      : "=a"(prev)
			      : "q"(new), "m"(*__xg (ptr)), "0"(old)
			      : "memory");
	return prev;   
    }
    return old;
#else
#error No cmpswap function defined for this arch
#endif	
}



#define SWAP(ptr, val) ((__typeof__(*(ptr)))__xchg ((unsigned long)(val), (ptr), sizeof(*(ptr))))
#define CAS(ptr, oldv, newv) ((__typeof__(*(ptr)))__cmpxchg ((ptr), (unsigned long)(oldv), (unsigned long)(newv), sizeof(*(ptr))))

static inline unsigned long __xadd (volatile void *ptr, unsigned long val, int size)
{
#ifdef i386
    switch (size)
    {
    case 1:
	__asm__ __volatile__(LOCK_PREFIX "xaddb %b0,%1"
			     : "=r"(val)
			     : "m"(*__xg(ptr)), "0"(val)
			     : "memory");
	return val;
    case 2:
	__asm__ __volatile__(LOCK_PREFIX "xaddw %w0,%1"
			     : "=r"(val)
			     : "m"(*__xg(ptr)), "0"(val)
			     : "memory");
	return val;
    case 4:
	__asm__ __volatile__(LOCK_PREFIX "xaddl %0,%1"
			     : "=r"(val)
			     : "m"(*__xg(ptr)), "0"(val));
	return val;
    default:
	assert (0);
    }
    return val;
#elif defined(x86_64)
    switch (size)
    {
    case 1:
	__asm__ __volatile__ (LOCK_PREFIX "xaddb %b0,%1"
			      : "=r"(val)
			      : "m"(*__xg (ptr)), "0"(val)
			      : "memory");
	return val;
    case 2:
	__asm__ __volatile__ (LOCK_PREFIX "xaddw %w0,%1"
			      : "=r"(val)
			      : "m"(*__xg (ptr)), "0"(val)
			      : "memory");
	return val;
    case 4:
	__asm__ __volatile__ (LOCK_PREFIX "xaddl %k0,%1"
			      : "=r"(val)
			      : "m"(*__xg (ptr)), "0"(val));
	return val;
    case 8:
	__asm__ __volatile__ (LOCK_PREFIX "xaddq %0,%1"
			      : "=r"(val)
			      : "m"(*__xg (ptr)), "0"(val));
	return val;
    default:
	assert (0);   
    }
    return val;
#else
#error No fetchandadd function defined for this arch
#endif
	
}
#define FETCH_AND_ADD(ptr, val) ((__typeof__(*(ptr)))__xadd ((ptr), (unsigned long)(val), sizeof(*(ptr))))

static inline void __atomic_add (int i, int *ptr, int size)
{
#ifdef i386
    switch (size)
    {
    case 1:
	__asm__ __volatile__(LOCK_PREFIX "addb %b1,%0"
			     :"=m" (*__xg(ptr))
			     :"ir" (i), "m" (*__xg(ptr)));
	return;
    case 2:
	__asm__ __volatile__(LOCK_PREFIX "addw %w1,%0"
			     :"=m" (*__xg(ptr))
			     :"ir" (i), "m" (*__xg(ptr)));
	return;
    case 4:
	__asm__ __volatile__(LOCK_PREFIX "addl %1,%0"
			     :"=m" (*__xg(ptr))
			     :"ir" (i), "m" (*__xg(ptr)));
	return;
    default:
	assert (0);
    }
#elif defined(x86_64)
    switch (size)
    {
    case 1:
	__asm__ __volatile__ (LOCK_PREFIX "addb %b1,%0"
			      :"=m" (*__xg (ptr))
			      :"ir" (i), "m" (*__xg (ptr)));
	return;
    case 2:
	__asm__ __volatile__ (LOCK_PREFIX "addw %w1,%0"
			      :"=m" (*__xg (ptr))
			      :"ir" (i), "m" (*__xg (ptr)));
	return;
    case 4:
	__asm__ __volatile__ (LOCK_PREFIX "addl %k1,%0"
			      :"=m" (*__xg (ptr))
			      :"ir" (i), "m" (*__xg (ptr)));
	return;
    case 8:
	__asm__ __volatile__ (LOCK_PREFIX "addq %1,%0"
			      :"=m" (*__xg (ptr))
			      :"ir" (i), "m" (*__xg (ptr)));
	return;
    default:
	assert (0);
    }
#else
#error No atomic add function defined for this arch
#endif
}

static inline void __atomic_inc (int *ptr, int size)
{
#ifdef i386
    switch (size)
    {
    case 1:
	__asm__ __volatile__(LOCK_PREFIX "incb %b0"
			     :"=m" (*__xg(ptr))
			     :"m" (*__xg(ptr)));
	return;
    case 2:
	__asm__ __volatile__(LOCK_PREFIX "incw %w0"
			     :"=m" (*__xg(ptr))
			     :"m" (*__xg(ptr)));
	return;
    case 4:
	__asm__ __volatile__(LOCK_PREFIX "incl %0"
			     :"=m" (*__xg(ptr))
			     :"m" (*__xg(ptr)));
	return;
    default:
	assert (0);
    }
#elif defined(x86_64)
    switch (size)
    {
    case 1:
	__asm__ __volatile__ (LOCK_PREFIX "incb %b0"
			      :"=m" (*__xg (ptr))
			      :"m" (*__xg (ptr)));
	return;
    case 2:
	__asm__ __volatile__ (LOCK_PREFIX "incw %w0"
			      :"=m" (*__xg (ptr))
			      :"m" (*__xg (ptr)));
	return;
    case 4:
	__asm__ __volatile__ (LOCK_PREFIX "incl %k0"
			      :"=m" (*__xg (ptr))
			      :"m" (*__xg (ptr)));
	return;
    case 8:
	__asm__ __volatile__ (LOCK_PREFIX "incq %0"
			      :"=m" (*__xg (ptr))
			      :"m" (*__xg (ptr)));
	return;
    default:
	assert (0);
    }
#else
#error No atomic add function defined for this arch
#endif
}

static inline void __atomic_dec (int *ptr, int size)
{
#ifdef i386
    switch (size)
    {
    case 1:
	__asm__ __volatile__(LOCK_PREFIX "decb %b0"
			     :"=m" (*__xg(ptr))
			     :"m" (*__xg(ptr)));
	return;
    case 2:
	__asm__ __volatile__(LOCK_PREFIX "decw %w0"
			     :"=m" (*__xg(ptr))
			     :"m" (*__xg(ptr)));
	return;
    case 4:
	__asm__ __volatile__(LOCK_PREFIX "decl %0"
			     :"=m" (*__xg(ptr))
			     :"m" (*__xg(ptr)));
	return;
    default:
	assert (0);
    }
#elif defined(x86_64)
    switch (size)
    {
    case 1:
	__asm__ __volatile__ (LOCK_PREFIX "decb %b0"
			      :"=m" (*__xg (ptr))
			      :"m" (*__xg (ptr)));
	return;
    case 2:
	__asm__ __volatile__ (LOCK_PREFIX "decw %w0"
			      :"=m" (*__xg (ptr))
			      :"m" (*__xg (ptr)));
	return;
    case 4:
	__asm__ __volatile__ (LOCK_PREFIX "decl %k0"
			      :"=m" (*__xg (ptr))
			      :"m" (*__xg (ptr)));
	return;
    case 8:
	__asm__ __volatile__ (LOCK_PREFIX "decq %0"
			      :"=m" (*__xg (ptr))
			      :"m" (*__xg (ptr)));
	return;
    default:
	assert (0);
    }
#else
#error No atomic add function defined for this arch
#endif
}

#define ATOMIC_ADD(i, ptr) (__atomic_add ((i), (ptr), sizeof(*ptr)))
#define ATOMIC_INC(ptr) (__atomic_inc ((ptr), sizeof(*ptr)))
#define ATOMIC_DEC(ptr) (__atomic_dec ((ptr), sizeof(*ptr)))

#endif //REMOVE_THIS

#ifdef HAVE_GCC_AND_PENTIUM_ASM

#ifdef HAVE_GCC_ASM_AND_X86_SFENCE
#define MPID_NEM_WRITE_BARRIER() __asm__ __volatile__  ( "sfence" ::: "memory" )
#else // HAVE_GCC_ASM_AND_X86_SFENCE
#define MPID_NEM_WRITE_BARRIER()
#endif // HAVE_GCC_ASM_AND_X86_SFENCE

#ifdef HAVE_GCC_ASM_AND_X86_LFENCE
/*
  #define MPID_NEM_READ_BARRIER() __asm__ __volatile__  ( ".byte 0x0f, 0xae, 0xe8" ::: "memory" ) */
#define MPID_NEM_READ_BARRIER() __asm__ __volatile__  ( "lfence" ::: "memory" )
#else // HAVE_GCC_ASM_AND_X86_LFENCE
#define MPID_NEM_READ_BARRIER()
#endif // HAVE_GCC_ASM_AND_X86_LFENCE


#ifdef HAVE_GCC_ASM_AND_X86_MFENCE
/*
  #define MPID_NEM_READ_WRITE_BARRIER() __asm__ __volatile__  ( ".byte 0x0f, 0xae, 0xf0" ::: "memory" )
*/
#define MPID_NEM_READ_WRITE_BARRIER() __asm__ __volatile__  ( "mfence" ::: "memory" )
#else // HAVE_GCC_ASM_AND_X86_MFENCE
#define MPID_NEM_READ_WRITE_BARRIER()
#endif // HAVE_GCC_ASM_AND_X86_MFENCE

#elif defined(HAVE_MASM_AND_X86)
#define MPID_NEM_WRITE_BARRIER()
#define MPID_NEM_READ_BARRIER() __asm { __asm _emit 0x0f __asm _emit 0xae __asm _emit 0xe8 }
#define MPID_NEM_READ_WRITE_BARRIER()

#else
#define MPID_NEM_WRITE_BARRIER()
#define MPID_NEM_READ_BARRIER()
#define MPID_NEM_READ_WRITE_BARRIER()
#endif // HAVE_GCC_AND_PENTIUM_ASM


#endif // MPID_NEM_ATOMICS_H
