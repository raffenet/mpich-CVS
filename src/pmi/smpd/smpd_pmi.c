/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ipmi.h"

/* global variables */
static ipmi_functions_t fn =
{
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

int PMI_Init(int *spawned)
{
    char *dll_name;
#ifdef HAVE_WINDOWS_H
    HMODULE hModule;
#else
#define LoadLibrary(a) dlopen(a, RTLD_LAZY /* or RTLD_NOW */)
#define GetProcAddress dlsym
    void *hModule;
#endif

    if (fn.PMI_Init != NULL)
    {
	/* Init cannot be called more than once? */
	return PMI_FAIL;
    }

    dll_name = getenv("PMI_DLL_NAME");
    if (dll_name)
    {
	hModule = LoadLibrary(dll_name);
	if (hModule != NULL)
	{
	    fn.PMI_Init = (int (*)(int *))GetProcAddress(hModule, "PMI_Init");
	    if (fn.PMI_Init == NULL)
	    {
		return PMI_FAIL;
	    }
	    fn.PMI_Initialized = (int (*)(void))GetProcAddress(hModule, "PMI_Initialized");
	    fn.PMI_Finalize = (int (*)(void))GetProcAddress(hModule, "PMI_Finalize");
	    fn.PMI_Get_size = (int (*)(int *))GetProcAddress(hModule, "PMI_Get_size");
	    fn.PMI_Get_rank = (int (*)(int *))GetProcAddress(hModule, "PMI_Get_rank");
	    fn.PMI_Get_id = (int (*)(char *))GetProcAddress(hModule, "PMI_Get_id");
	    fn.PMI_Get_id_length_max = (int (*)(void))GetProcAddress(hModule, "PMI_Get_id_length_max");
	    fn.PMI_Barrier = (int (*)(void))GetProcAddress(hModule, "PMI_Barrier");
	    fn.PMI_Get_clique_size = (int (*)(int *))GetProcAddress(hModule, "PMI_Get_clique_size");
	    fn.PMI_Get_clique_ranks = (int (*)(int *))GetProcAddress(hModule, "PMI_Get_clique_ranks");
	    fn.PMI_KVS_Get_my_name = (int (*)(char *))GetProcAddress(hModule, "PMI_KVS_Get_my_name");
	    fn.PMI_KVS_Get_name_length_max = (int (*)(void))GetProcAddress(hModule, "PMI_KVS_Get_name_length_max");
	    fn.PMI_KVS_Get_key_length_max = (int (*)(void))GetProcAddress(hModule, "PMI_KVS_Get_key_length_max");
	    fn.PMI_KVS_Get_value_length_max = (int (*)(void))GetProcAddress(hModule, "PMI_KVS_Get_value_length_max");
	    fn.PMI_KVS_Create = (int (*)(char *))GetProcAddress(hModule, "PMI_KVS_Create");
	    fn.PMI_KVS_Destroy = (int (*)(const char *))GetProcAddress(hModule, "PMI_KVS_Destroy");
	    fn.PMI_KVS_Put = (int (*)(const char *, const char *, const char *))GetProcAddress(hModule, "PMI_KVS_Put");
	    fn.PMI_KVS_Commit = (int (*)(const char *))GetProcAddress(hModule, "PMI_KVS_Commit");
	    fn.PMI_KVS_Get = (int (*)(const char *, const char *, char *))GetProcAddress(hModule, "PMI_KVS_Get");
	    fn.PMI_KVS_Iter_first = (int (*)(const char *, char *, char *))GetProcAddress(hModule, "PMI_KVS_Iter_first");
	    fn.PMI_KVS_Iter_next = (int (*)(const char *, char *, char *))GetProcAddress(hModule, "PMI_KVS_Iter_next");
	    fn.PMI_Spawn_multiple = (int (*)(int, const char **, const char ***, const int *, const int *, const PMI_keyval_t **, int, const PMI_keyval_t *, int *))GetProcAddress(hModule, "PMI_Spawn_multiple");
	    fn.PMI_Args_to_keyval = (int (*)(int *, char ***, PMI_keyval_t *, int *))GetProcAddress(hModule, "PMI_Args_to_keyval");
	    return fn.PMI_Init(spawned);
	}
    }

    fn.PMI_Init = iPMI_Init;
    fn.PMI_Initialized = iPMI_Initialized;
    fn.PMI_Finalize = iPMI_Finalize;
    fn.PMI_Get_size = iPMI_Get_size;
    fn.PMI_Get_rank = iPMI_Get_rank;
    fn.PMI_Get_id = iPMI_Get_id;
    fn.PMI_Get_id_length_max = iPMI_Get_id_length_max;
    fn.PMI_Barrier = iPMI_Barrier;
    fn.PMI_Get_clique_size = iPMI_Get_clique_size;
    fn.PMI_Get_clique_ranks = iPMI_Get_clique_ranks;
    fn.PMI_KVS_Get_my_name = iPMI_KVS_Get_my_name;
    fn.PMI_KVS_Get_name_length_max = iPMI_KVS_Get_name_length_max;
    fn.PMI_KVS_Get_key_length_max = iPMI_KVS_Get_key_length_max;
    fn.PMI_KVS_Get_value_length_max = iPMI_KVS_Get_value_length_max;
    fn.PMI_KVS_Create = iPMI_KVS_Create;
    fn.PMI_KVS_Destroy = iPMI_KVS_Destroy;
    fn.PMI_KVS_Put = iPMI_KVS_Put;
    fn.PMI_KVS_Commit = iPMI_KVS_Commit;
    fn.PMI_KVS_Get = iPMI_KVS_Get;
    fn.PMI_KVS_Iter_first = iPMI_KVS_Iter_first;
    fn.PMI_KVS_Iter_next = iPMI_KVS_Iter_next;
    fn.PMI_Spawn_multiple = iPMI_Spawn_multiple;
    fn.PMI_Args_to_keyval = iPMI_Args_to_keyval;
    return fn.PMI_Init(spawned);
}

int PMI_Finalize()
{
    if (fn.PMI_Finalize == NULL)
	return PMI_FAIL;
    return fn.PMI_Finalize();
}

int PMI_Get_size(int *size)
{
    if (fn.PMI_Get_size == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_size(size);
}

int PMI_Get_rank(int *rank)
{
    if (fn.PMI_Get_rank == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_rank(rank);
}

int PMI_Get_clique_size( int *size )
{
    if (fn.PMI_Get_clique_size == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_clique_size(size);
}

int PMI_Get_clique_ranks( int *ranks )
{
    if (fn.PMI_Get_clique_ranks == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_clique_ranks(ranks);
}

int PMI_Get_id( char *id_str )
{
    if (fn.PMI_Get_id == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_id(id_str);
}

int PMI_Get_id_length_max()
{
    if (fn.PMI_Get_id_length_max == NULL)
	return PMI_FAIL;
    return fn.PMI_Get_id_length_max();
}

int PMI_Barrier()
{
    if (fn.PMI_Barrier == NULL)
	return PMI_FAIL;
    return fn.PMI_Barrier();
}

int PMI_KVS_Get_my_name(char *kvsname)
{
    if (fn.PMI_KVS_Get_my_name == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get_my_name(kvsname);
}

int PMI_KVS_Get_name_length_max()
{
    if (fn.PMI_KVS_Get_name_length_max == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get_name_length_max();
}

int PMI_KVS_Get_key_length_max()
{
    if (fn.PMI_KVS_Get_key_length_max == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get_key_length_max();
}

int PMI_KVS_Get_value_length_max()
{
    if (fn.PMI_KVS_Get_value_length_max == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get_value_length_max();
}

int PMI_KVS_Create(char * kvsname)
{
    if (fn.PMI_KVS_Create == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Create(kvsname);
}

int PMI_KVS_Destroy(const char * kvsname)
{
    if (fn.PMI_KVS_Destroy == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Destroy(kvsname);
}

int PMI_KVS_Put(const char *kvsname, const char *key, const char *value)
{
    if (fn.PMI_KVS_Put == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Put(kvsname, key, value);
}

int PMI_KVS_Commit(const char *kvsname)
{
    if (fn.PMI_KVS_Commit == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Commit(kvsname);
}

int PMI_KVS_Get(const char *kvsname, const char *key, char *value)
{
    if (fn.PMI_KVS_Get == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Get(kvsname, key, value);
}

int PMI_KVS_Iter_first(const char *kvsname, char *key, char *value)
{
    if (fn.PMI_KVS_Iter_first == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Iter_first(kvsname, key, value);
}

int PMI_KVS_Iter_next(const char *kvsname, char *key, char *value)
{
    if (fn.PMI_KVS_Iter_next == NULL)
	return PMI_FAIL;
    return fn.PMI_KVS_Iter_next(kvsname, key, value);
}

int PMI_Spawn_multiple(int count,
                       const char ** cmds,
                       const char *** argvs,
                       const int * maxprocs,
                       const int * info_keyval_sizes,
                       const PMI_keyval_t ** info_keyval_vectors,
                       int preput_keyval_size,
                       const PMI_keyval_t * preput_keyval_vector,
                       int * errors)
{
    if (fn.PMI_Spawn_multiple == NULL)
	return PMI_FAIL;
    return fn.PMI_Spawn_multiple(count, cmds, argvs, maxprocs,
	info_keyval_sizes, info_keyval_vectors,
	preput_keyval_size, preput_keyval_vector,
	errors);
}

int PMI_Args_to_keyval(int *argcp, char ***argvp, PMI_keyval_t *keyvalp, int *size)
{
    if (fn.PMI_Args_to_keyval == NULL)
	return PMI_FAIL;
    return fn.PMI_Args_to_keyval(argcp, argvp, keyvalp, size);
}
