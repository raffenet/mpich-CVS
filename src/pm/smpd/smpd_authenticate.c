/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#include "crypt.h"

int smpd_getpid()
{
#ifdef HAVE_WINDOWS_H
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

int smpd_authenticate(sock_set_t set, sock_t sock, int type)
{
    int ret_val;
    char phrase[SMPD_PASSPHRASE_MAX_LENGTH];

    if (smpd_get_smpd_data("phrase", phrase, SMPD_PASSPHRASE_MAX_LENGTH) != SMPD_SUCCESS)
    {
	smpd_dbg_printf("failed to get the phrase\n");
	return SMPD_FAIL;
    }

    switch (type)
    {
    case SMPD_SERVER_AUTHENTICATION:
	smpd_dbg_printf("authenticating from server\n");
	ret_val = smpd_server_authenticate(set, sock, SMPD_DEFAULT_PASSPHRASE);
	break;
    case SMPD_CLIENT_AUTHENTICATION:
	/*smpd_dbg_printf("authenticating from client\n");*/
	ret_val = smpd_client_authenticate(set, sock, SMPD_DEFAULT_PASSPHRASE);
	break;
    default:
	smpd_err_printf("invalid authentication type: %d\n", type);
	ret_val = SMPD_FAIL;
	break;
    }
    return ret_val;
}

static int smpd_gen_authentication_strings(char *phrase, char *append, char *crypted)
{
    int stamp;
    char *crypted_internal;
    char phrase_internal[SMPD_PASSPHRASE_MAX_LENGTH+1];

    stamp = rand();

    snprintf(phrase_internal, SMPD_PASSPHRASE_MAX_LENGTH, "%s%d", phrase, stamp);
    snprintf(append, SMPD_AUTHENTICATION_STR_LEN, "%d", stamp);

    crypted_internal = crypt(phrase_internal, SMPD_SALT_VALUE);
    if (strlen(crypted_internal) > SMPD_PASSPHRASE_MAX_LENGTH)
    {
	smpd_err_printf("internal crypted string too long: %d > %d\n", strlen(crypted_internal), SMPD_PASSPHRASE_MAX_LENGTH);
	return SMPD_FAIL;
    }
    strcpy(crypted, crypted_internal);

    return SMPD_SUCCESS;
}

int smpd_server_authenticate(sock_set_t set, sock_t sock, char *passphrase)
{
    int ret_val;
    char pszStr[SMPD_AUTHENTICATION_STR_LEN], pszCrypt[SMPD_AUTHENTICATION_STR_LEN];

    /* generate the challenge string and the encrypted result*/
    if (smpd_gen_authentication_strings(passphrase, pszStr, pszCrypt) != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_server_authenticate: failed to generate the authentication strings\n");
	return SMPD_FAIL;
    }
    /*
    smpd_dbg_printf("gen_authentication_strings:\n passphrase - %s\n pszStr - %s\n pszCrypt - %s\n",
	passphrase, pszStr, pszCrypt);
    */

    /* write the challenge string*/
    /*smpd_dbg_printf("writing challenge string: %s\n", pszStr);*/
    if (smpd_write_string(set, sock, pszStr) != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_server_authenticate: Writing challenge string failed\n");
	return SMPD_FAIL;
    }

    /* read the response*/
    if (smpd_read_string(set, sock, pszStr, SMPD_AUTHENTICATION_STR_LEN) != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_server_authenticate: Reading challenge response failed\n");
	return SMPD_FAIL;
    }
    /*smpd_dbg_printf("read challenge-response string: %s\n", pszStr);*/

    /*smpd_dbg_printf("read response, comparing with crypted string:\n%s\n%s\n", pszStr, pszCrypt);*/
    /* compare the response with the encrypted result and write success or failure*/
    if (strcmp(pszStr, pszCrypt) == 0)
	ret_val = smpd_write_string(set, sock, SMPD_AUTHENTICATION_ACCEPTED_STR);
    else
	ret_val = smpd_write_string(set, sock, SMPD_AUTHENTICATION_REJECTED_STR);
    if (ret_val != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_server_authenticate: Writing authentication result failed\n");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("successful server authentication\n");
    return SMPD_SUCCESS;
}

int smpd_client_authenticate(sock_set_t set, sock_t sock, char *passphrase)
{
    char phrase[SMPD_PASSPHRASE_MAX_LENGTH];
    char *result;
    char pszStr[SMPD_AUTHENTICATION_STR_LEN];

    strcpy(phrase, passphrase);

    /* read the challenge string*/
    if (smpd_read_string(set, sock, pszStr, SMPD_AUTHENTICATION_STR_LEN) != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_client_authenticate: Reading challenge string failed\n");
	return SMPD_FAIL;
    }
    /*smpd_dbg_printf("challenge string read: %s\n", pszStr);*/

    /* crypt the passphrase + the challenge*/
    if (strlen(phrase) + strlen(pszStr) > SMPD_PASSPHRASE_MAX_LENGTH)
    {
	smpd_err_printf("smpd_client_authenticate: unable to process passphrase.\n");
	return SMPD_FAIL;
    }
    strcat(phrase, pszStr);

    /*smpd_dbg_printf("crypting: %s\n", phrase);*/
    result = crypt(phrase, SMPD_SALT_VALUE);
    strcpy(pszStr, result);

    /*smpd_dbg_printf("writing response: %s\n", pszStr);*/
    /* write the response*/
    if (smpd_write_string(set, sock, pszStr) != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_client_authenticate: WriteString of the encrypted response string failed\n");
	return SMPD_FAIL;
    }

    /* read the result*/
    if (smpd_read_string(set, sock, pszStr, SMPD_AUTHENTICATION_STR_LEN) != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_client_authenticate: reading authentication result failed\n");
	return SMPD_FAIL;
    }
    if (strcmp(pszStr, SMPD_AUTHENTICATION_ACCEPTED_STR))
    {
	smpd_err_printf("smpd_client_authenticate: server returned - %s\n", pszStr);
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}
