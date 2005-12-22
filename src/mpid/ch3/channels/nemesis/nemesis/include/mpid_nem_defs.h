#ifndef MPID_NEM_DEFS_H
#define MPID_NEM_DEFS_H

#include "mpid_nem_datatypes.h"
#include "mpidi_ch3i_nemesis_conf.h"

#define MPID_NEM_RET_OK       1
#define MPID_NEM_RET_NG      -1
#define MPID_NEM_KEY          632236
#define MPID_NEM_ANY_SOURCE  -1
#define MPID_NEM_IN           1
#define MPID_NEM_OUT          0 
#define MPID_NEM_MAX_FNAME_LEN 256

/* Network module defines */
#define MPID_NEM_NO_MODULE    0
#define MPID_NEM_GM_MODULE    1
#define MPID_NEM_TCP_MODULE   2

//#define MPID_NEM_NET_MODULE   MPID_NEM_NO_MODULE

#define MPID_NEM_POLL_IN      0
#define MPID_NEM_POLL_OUT     1

#define MPID_NEM_ASYMM_NULL_VAL    64
typedef unsigned int MPID_nem_addr_t;
extern  char *MPID_nem_asymm_base_addr;
extern  char *MPID_nem_asymm_null_var;

#define MPID_NEM_ASYMM_NULL ((void *)0x0)
//#define MPID_NEM_ASYMM_NULL MPID_nem_asymm_null_var

#ifndef MPID_NEM_SYMMETRIC_QUEUES
#define MPID_NEM_REL_TO_ABS( ptr ) (( __typeof__((ptr)))(((char *)(ptr)) + ((MPID_nem_addr_t)(MPID_nem_asymm_base_addr))))
#define MPID_NEM_ABS_TO_REL( ptr ) (( __typeof__((ptr)))(((char *)(ptr)) - ((MPID_nem_addr_t)(MPID_nem_asymm_base_addr))))
#define MPID_NEM_SET_REL_NULL( ptr ) (((char *)(ptr)) == NULL ) ? (( __typeof__((ptr)))( MPID_NEM_ASYMM_NULL)) : MPID_NEM_ABS_TO_REL( ptr )
#define MPID_NEM_SET_ABS_NULL( ptr ) (((char *)(ptr)) == ((char *)(MPID_NEM_ASYMM_NULL))) ? (( __typeof__((ptr))) 0 ) : MPID_NEM_REL_TO_ABS( ptr )
#else //MPID_NEM_SYMMETRIC_QUEUES
#define MPID_NEM_REL_TO_ABS( ptr )   (ptr)
#define MPID_NEM_ABS_TO_REL( ptr )   (ptr)
#define MPID_NEM_SET_REL_NULL( ptr ) (ptr)
#define MPID_NEM_SET_ABS_NULL( ptr ) (ptr)
#endif //MPID_NEM_SYMMETRIC_QUEUES

typedef struct MPID_nem_barrier
{
    volatile int val;
    volatile int wait;
}
MPID_nem_barrier_t;

typedef struct MPID_nem_seg
{
  /* Sizes */
  int   max_size ;
  int   size_left;
  /* Pointers */
  char *base_addr;
  char *current_addr;
  char *max_addr;
  /* Misc */
  char  file_name[MPID_NEM_MAX_FNAME_LEN];
  int   base_descs; 
  int   symmetrical;
} MPID_nem_seg_t, *MPID_nem_seg_ptr_t;

typedef struct MPID_nem_seg_info
{
    int   size;
    char *addr; 
} MPID_nem_seg_info_t, *MPID_nem_seg_info_ptr_t; 

#define MPID_NEM_NON_LOCAL -1
#define MPID_NEM_IS_LOCAL(grank) (MPID_nem_mem_region.local_ranks[grank] != MPID_NEM_NON_LOCAL)
#define MPID_NEM_LOCAL_RANK(grank) (MPID_nem_mem_region.local_ranks[grank])

typedef struct MPID_nem_mem_region
{
    MPID_nem_seg_t          memory;
    MPID_nem_seg_info_t        *seg;
    int                num_seg;
    int                map_lock;
    pid_t             *pid;
    int                rank;
    int                num_local;
    int                num_procs;
    int               *local_procs; /* local_procs[lrank] gives the global rank of proc with local rank lrank */
    int                local_rank;    
    int               *local_ranks; /* local_ranks[grank] gives the local rank of proc with global rank grank or MPID_NEM_NON_LOCAL */
    int                ext_procs;  /* Number of non-local processes */
    int               *ext_ranks;  /* Ranks of non-local processes */ 
    MPID_nem_fbox_arrays_t         mailboxes;
    MPID_nem_cell_ptr_t       Elements;
    MPID_nem_queue_ptr_t    *FreeQ;
    MPID_nem_queue_ptr_t    *RecvQ;
    MPID_nem_cell_ptr_t       net_elements;
    MPID_nem_queue_ptr_t     net_free_queue;
    MPID_nem_queue_ptr_t     net_recv_queue;
    MPID_nem_barrier_t *barrier;
    struct MPID_nem_mem_region *next;
} MPID_nem_mem_region_t, *MPID_nem_mem_region_ptr_t;

#define MEM_REGION_IN_HEAP 0
#if MEM_REGION_IN_HEAP
#define MPID_nem_mem_region (*MPID_nem_mem_region_ptr)
extern MPID_nem_mem_region_t *MPID_nem_mem_region_ptr;
#else
extern MPID_nem_mem_region_t MPID_nem_mem_region;
#endif

#endif /* MPID_NEM_DEFS_H */
