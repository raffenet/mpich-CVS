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

#define PMI_SUCCESS  0
#define PMI_FAIL    -1

#define PMI_TRUE     1
#define PMI_FALSE    0

/* PMI Group functions */

/*@
PMI_Init - initialize the Process Manager Interface

Return value: success or failure
+ PMI_SUCCESS - initialization completed successfully
- PMI_FAIL - initialization failed

Notes:
Initialize PMI for this process group. The value of spawned indicates whether this process
was created by PMI_Spawn_multiple.

Module:
PMI
@*/
int PMI_Init( int *spawned );

/*@
PMI_Initialized - check if PMI has been initialized

Return value: success or failure
+ PMI_TRUE - initialize has been called
- PMI_FALSE - initialize has not been called or previously failed

Module:
PMI
@*/
int PMI_Initialized( void );

/*@
PMI_Finalize - finalize the Process Manager Interface

Return value: success or failure
+ PMI_SUCCESS - initialization completed successfully
- PMI_FAIL - initialization failed

Notes:
 Finalize PMI for this process group.

Module:
PMI
@*/
int PMI_Finalize( void );

/*@
PMI_Get_size - obtain the size of the process group

Output Parameters:
. size - pointer to an integer that receives the size of the process group

Return value: a PMI error code
+ PMI_SUCCESS - size successfully obtained
- PMI_FAIL - unable to return the size

Notes:
This function returns the size of the process group that the local process belongs to.

Module:
PMI
@*/
int PMI_Get_size( int *size );

/*@
PMI_Get_rank - obtain the rank of the local process in the process group

Output Parameters:
. rank - pointer to an integer that receives the rank in the process group

Return value: a PMI error code
+ PMI_SUCCESS - rank successfully obtained
- PMI_FAIL - unable to return the rank

Notes:
This function returns the rank of the local process in its process group.

Module:
PMI
@*/
int PMI_Get_rank( int *rank );

/*@
PMI_Get_id - obtain the id of the process group

Output Parameters:
. id_str - pointer to an array of characters that receives the id of the process group

Return value: a PMI error code
+ PMI_SUCCESS - id successfully obtained
- PMI_FAIL - unable to return the id

Notes:
This function returns a string that uniquely identifies the process group that the local
process belongs to.  The string passed in must be at least as long as the number returned
by PMI_Get_id_length_max().

Module:
PMI
@*/
int PMI_Get_id( char *id_str );

/*@
PMI_Get_id_length_max - obtain the maximum length of an id string

Return value: the maximum length of an id string

Notes:
This function returns the maximum length of a process group id string.  It must return at
least 40 so that ids can be implemented with uuids.

Module:
PMI
@*/
int PMI_Get_id_length_max( void );

/*@
PMI_Barrier - barrier across the process group

Return value: a PMI error code
+ PMI_SUCCESS - barrier successfully finished
- PMI_FAIL - barrier failed

Notes:
This function is a collective call across all processes in the local process group.  It will not
return until all the processes have called PMI_Barrier().

Module:
PMI
@*/
int PMI_Barrier( void );

/*@
PMI_Get_clique_size - obtain the number of processes on the local node

Output Parameters:
. size - pointer to an integer that receives the size of the clique

Return value: a PMI error code
+ PMI_SUCCESS - size successfully obtained
- PMI_FAIL - unable to return the clique size

Notes:
This function returns the number of processes in the local process group that are on the local node.

Module:
PMI
@*/
int PMI_Get_clique_size( int *size );

/*@
PMI_Get_clique_ranks - obtain the ranks of the local processes in the process group

Output Parameters:
. ranks - pointer to an array of integers that receive the local ranks

Return value: a PMI error code
+ PMI_SUCCESS - ranks successfully obtained
- PMI_FAIL - unable to return the ranks

Notes:
This function returns the ranks of the processes on the local node.  The array must be the size returned
by PMI_Get_clique_size().

Module:
PMI
@*/
int PMI_Get_clique_ranks( int *ranks );

/* PMI Keymap functions */
/*@
PMI_KVS_Get_my_name - obtain the name of the keyval space the local process group has access to

Output Parameters:
. kvsname - a string that receives the keyval space name

Return value: a PMI error code
+ PMI_SUCCESS - kvsname successfully obtained
- PMI_FAIL - unable to return the kvsname

Notes:
This function returns the name of the keyval space that this process and all other processes in the
process group have access to.  kvsname must be at least as long as the value returned by
PMI_KVS_Get_name_length_max().

Module:
PMI
@*/
int PMI_KVS_Get_my_name( char *kvsname );

/*@
PMI_KVS_Get_name_length_max - obtain the length necessary to store a kvsname

Return value: maximum length required to hold a keyval space name

Notes:
This function returns the string length required to store a keyval space name.

Module:
PMI
@*/
int PMI_KVS_Get_name_length_max( void );

/*@
PMI_KVS_Get_key_length_max - obtain the length necessary to store a key

Return value: maximum length required to hold a key string.

Notes:
This function returns the string length required to store a key.

Module:
PMI
@*/
int PMI_KVS_Get_key_length_max( void );

/*@
PMI_KVS_Get_value_length_max - obtain the length necessary to store a value

Return value: maximum length required to hold a keyval space value

Notes:
This function returns the string length required to store a value from a keyval space.

Module:
PMI
@*/
int PMI_KVS_Get_value_length_max( void );

/*@
PMI_KVS_Create - create a new keyval space

Output Parameters:
. kvsname - a string that receives the keyval space name

Return value: a PMI error code
+ PMI_SUCCESS - keyval space successfully created
- PMI_FAIL - unable to create a new keyval space

Notes:
This function creates a new keyval space.  Everyone in the same process group can access this
keyval space by the name returned by this function.  The function is not collective.  Only one
process calls this function.  kvsname must be at least as long as the value returned by
PMI_KVS_Get_name_length_max().

Module:
PMI
@*/
int PMI_KVS_Create( char *kvsname );            /* make a new one, get name */

/*@
PMI_KVS_Destroy - destroy keyval space

Input Parameters:
. kvsname - keyval space name

Return value: a PMI error code
+ PMI_SUCCESS - keyval space successfully destroyed
- PMI_FAIL - unable to destroy the keyval space

Notes:
This function destroys a keyval space created by PMI_KVS_Create().

Module:
PMI
@*/
int PMI_KVS_Destroy( const char *kvsname );

/*@
PMI_KVS_Put - put a key/value pair in a keyval space

Input Parameters:
+ kvsname - keyval space name
. key - key
- value - value

Return value: a PMI error code
+ PMI_SUCCESS - keyval pair successfully put in keyval space
- PMI_FAIL - put failed

Notes:
This function puts the key/value pair in the specified keyval space.  The value is not visible
to other processes until PMI_KVS_Commit is called.  The function may complete locally.  After 
PMI_KVS_Commit() is called, the value may be retrieved by calling PMI_KVS_Get().  All keys put
to a keyval space must be unique to the keyval space.  You may not put more than once with the
same key.

Module:
PMI
@*/
int PMI_KVS_Put( const char *kvsname, const char *key, const char *value);

/*@
PMI_KVS_Commit - commit all previous puts to the keyval space

Input Parameters:
. kvsname - keyval space name

Return value: a PMI error code
+ PMI_SUCCESS - commit succeeded
- PMI_FAIL - commit failed

Notes:
This function commits all previous puts since the last PMI_KVS_Commit() into the specified keyval space.
It is a process local operation.

Module:
PMI
@*/
int PMI_KVS_Commit( const char *kvsname );

/*@
PMI_KVS_Get - get a key/value pair from a keyval space

Input Parameters:
+ kvsname - keyval space name
- key - key

Output Parameters:
. value - value

Return value: a PMI error code
+ PMI_SUCCESS - get succeeded
- PMI_FAIL - get failed

Notes:
This function gets the value of the specified key in the keyval space.

Module:
PMI
@*/
int PMI_KVS_Get( const char *kvsname, const char *key, char *value);

/*@
PMI_KVS_Iter_first - initialize the iterator and get the first value

Input Parameters:
. kvsname - keyval space name

Output Parameters:
+ key - key
- value - value

Return value: a PMI error code
+ PMI_SUCCESS - keyval pair successfully retrieved from the keyval space
- PMI_FAIL - failed to initialize the iterator and get the first keyval pair

Notes:
This function initializes the iterator for the specified keyval space and retrieves the first key/val pair.
The end of the keyval space is specified by returning an empty key string.  key and val must be at least as
long as the values returned by PMI_KVS_Get_key_length_max() and PMI_KVS_Get_value_length_max().

Module:
PMI
@*/
int PMI_KVS_Iter_first(const char *kvsname, char *key, char *val);  /* loop through the */

/*@
PMI_KVS_Iter_next - get the next keyval pair from the keyval space

Input Parameters:
. kvsname - keyval space name

Output Parameters:
+ key - key
- value - value

Return value: a PMI error code
+ PMI_SUCCESS - keyval pair successfully retrieved from the keyval space
- PMI_FAIL - failed to get the next keyval pair

Notes:
This function retrieves the next keyval pair from the specified keyval space.  PMI_KVS_Iter_first must have
been previously called.  The end of the keyval space is specified by returning an empty key string.
key and val must be at least as long as the values returned by PMI_KVS_Get_key_length_max() and
PMI_KVS_Get_value_length_max().

Module:
PMI
@*/
int PMI_KVS_Iter_next(const char *kvsname, char *key, char *val);

/* PMI Process Creation functions */

/*S
PMI_keyval_t - keyval structure used by PMI_Spawn_mulitiple

Fields:
+ key - name of the key
- val - value of the key

Module:
PMI
S*/
typedef struct PMI_keyval_t
{
    char * key;
    char * val;
} PMI_keyval_t;

/*@
PMI_Spawn_multiple - spawn a new set of processes

Input Parameters:
+ count - count of commands
. cmds - array of command strings
. argvs - array of argv arrays for each command string
. maxprocs - array of maximum processes to spawn for each command string
. info_keyval_sizes - array of sizes of each keyval vector
. info_keyval_vectors - array of keyval vector arrays
. preput_keyval_size - size of preput keyval vector
- preput_keyval_vector - array of keyvals to be pre-put in the spawned keyval space

Output Parameters:
+ errors - array of errors for each command
- same_domain - PMI same domain flag

Return value: a PMI error code
+ PMI_SUCCESS - spawn successful
- PMI_FAIL - spawn failed

Notes:
This function spawns a new process group.  The same_domain flag indicates if the spawned group
is in the same PMI domain as the calling process.  Processes in the same PMI domain can access
each others keyval spaces directly.

Module:
PMI
@*/
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

/*@
PMI_Args_to_info - create keyval structures from command line arguments

Input Parameters:
+ argcp - pointer to argc
- argvp - pointer to argv

Output Parameters:
+ keyvalp - array of keyvals
- size - size of the allocated array

Return value: a PMI error code
+ PMI_SUCCESS - success
- PMI_FAIL - fail

Notes:
This function removes PMI specific arguments from the command line and creates the corresponding
PMI_keyval_t structures for them.  It returns an array and size to the caller that can then be passed to
PMI_Spawn_multiple.  The array can be freed by free().

Module:
PMI
@*/
int PMI_Args_to_keyval(int *argcp, char ***argvp, PMI_keyval_t *keyvalp, int *size);

#if defined(__cplusplus)
}
#endif

#endif
