/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_H
#define PMI_H

/* prototypes for the PMI interface in MPICH2 */

#if defined(__cplusplus)
extern "C" {
#endif

/* PMI Group functions */
int PMI_Init( int *spawned );           /* initialize PMI for this process group
                                           The value of spawned indicates whether this process
                                           was created by PMI_Spawn_multiple. */
int PMI_Initialized( void );            /* Return true if PMI has been initialized */
int PMI_Finalize( void );               /* finalize PMI for this process group */
int PMI_Get_size( int *size );          /* get size of process group */
int PMI_Get_rank( int *rank );          /* get rank in process group */
int PMI_Get_id( char *id_str );         /* get a string to uniquely identify the process group */
int PMI_Get_id_length_max( void );      /* get the maximum length the id string can be. Must return >= 40 */
int PMI_Barrier( void );                /* barrier across processes in process group */
int PMI_Get_clique_size( int *size );   /* get the number of processes on my node */
int PMI_Get_clique_ranks( int *ranks ); /* get the ranks on my node */

/* PMI Keymap functions */
int PMI_KVS_Get_my_name( char *kvsname );       /* get name of keyval space */
int PMI_KVS_Get_name_length_max( void );        /* needed to communicate keyval space */
int PMI_KVS_Get_key_length_max( void );         /* contents to a foreign domain */
int PMI_KVS_Get_value_length_max( void );
int PMI_KVS_Create( char *kvsname );            /* make a new one, get name */
int PMI_KVS_Destroy( const char *kvsname );     /* finish with one */
int PMI_KVS_Put( const char *kvsname, const char *key,
                const char *value);             /* put data */
int PMI_KVS_Commit( const char *kvsname );      /* block until all pending put
                                                   operations from this process
                                                   are complete.  This is a process
                                                   local operation. */
int PMI_KVS_Get( const char *kvsname,
                 const char *key, char *value); /* get value associated with key */
int PMI_KVS_Iter_first(const char *kvsname, char *key, char *val);  /* loop through the */
int PMI_KVS_Iter_next(const char *kvsname, char *key, char *val);   /* pairs in the kvs */

/* PMI Process Creation functions */

typedef struct PMI_keyval_t
{
    char * key;
    char * val;
}
PMI_keyval_t;

int PMI_Spawn_multiple(int count,
                       const char ** cmds,
                       const char *** argvs,
                       const int * maxprocs,
                       const int * info_keyval_sizes,
                       const PMI_keyval_t ** info_keyval_vectors,
                       int preput_keyval_size,
                       const PMI_keyval_t * preput_keyval_vector,
                       int * errors,
                       int * same_domain);

/* parse PMI implementation specific values into an info object that can then be passed to 
   PMI_Spawn_multiple.  Remove PMI implementation specific arguments from argc and argv */
int PMI_Args_to_info(int *argcp, char ***argvp, void *infop);

#if defined(__cplusplus)
}
#endif

#endif
