/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_getpid()
{
#ifdef HAVE_WINDOWS_H
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

int smpd_gen_authentication_strings(char *phrase, char *append, char *crypted)
{
    int stamp;
    char hash[SMPD_PASSPHRASE_MAX_LENGTH];
    char phrase_internal[SMPD_PASSPHRASE_MAX_LENGTH+1];

    smpd_enter_fn("smpd_gen_authentication_strings");

    stamp = rand();

    snprintf(phrase_internal, SMPD_PASSPHRASE_MAX_LENGTH, "%s%d", phrase, stamp);
    snprintf(append, SMPD_AUTHENTICATION_STR_LEN, "%d", stamp);

    smpd_hash(phrase_internal, (int)strlen(phrase_internal), hash, SMPD_PASSPHRASE_MAX_LENGTH);
    if (strlen(hash) > SMPD_PASSPHRASE_MAX_LENGTH)
    {
	smpd_err_printf("internal crypted string too long: %d > %d\n", strlen(hash), SMPD_PASSPHRASE_MAX_LENGTH);
	smpd_exit_fn("smpd_gen_authentication_strings");
	return SMPD_FAIL;
    }
    strcpy(crypted, hash);

    smpd_exit_fn("smpd_gen_authentication_strings");
    return SMPD_SUCCESS;
}
