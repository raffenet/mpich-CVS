/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SMPD_DATABASE_H
#define SMPD_DATABASE_H

#define SMPD_DBS_SUCCESS          0
#define SMPD_DBS_FAIL            -1
#define SMPD_MAX_DBS_NAME_LEN     256
#define SMPD_MAX_DBS_KEY_LEN      256
#define SMPD_MAX_DBS_VALUE_LEN    1024

#if defined(__cplusplus)
extern "C" {
#endif

int smpd_dbs_init();
int smpd_dbs_finalize();
int smpd_dbs_create(char *name);
int smpd_dbs_create_name_in(char *name);
int smpd_dbs_destroy(char *name);
int smpd_dbs_get(char *name, char *key, char *value);
int smpd_dbs_put(char *name, char *key, char *value);
int smpd_dbs_delete(char *name, char *key);
int smpd_dbs_first(char *name, char *key, char *value);
int smpd_dbs_next(char *name, char *key, char *value);
int smpd_dbs_firstdb(char *name);
int smpd_dbs_nextdb(char *name);

#if defined(__cplusplus)
}
#endif

#endif
