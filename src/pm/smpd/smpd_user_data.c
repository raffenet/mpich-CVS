/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_set_user_data(char *key, char *value)
{
    return SMPD_FAIL;
}

int smpd_set_smpd_data(char *key, char *value)
{
    return SMPD_FAIL;
}

int smpd_get_user_data_default(char *key, char *value, int value_len)
{
    return SMPD_FAIL;
}

int smpd_get_user_data(char *key, char *value, int value_len)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;

    result = RegOpenKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_KEY,
	0, 
	KEY_READ,
	&tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to open the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d\n", result);
	return SMPD_FAIL;
    }

    len = value_len;
    result = RegQueryValueEx(tkey, key, 0, NULL, (unsigned char *)value, &len);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to read the smpd registry key '%s', error %d\n", key, result);
	RegCloseKey(tkey);
	return SMPD_FAIL;
    }

    RegCloseKey(tkey);
    return SMPD_SUCCESS;
#else
#endif
}

int smpd_get_smpd_data_default(char *key, char *value, int value_len)
{
    if (strcmp(key, "phrase") == 0)
    {
	/*smpd_dbg_printf("returning default phrase: %s\n", SMPD_DEFAULT_PASSPHRASE);*/
	strncpy(value, SMPD_DEFAULT_PASSPHRASE, value_len);
	value[value_len-1] = '\0';
    }
    else if (strcmp(key, "foo") == 0)
    {
	strncpy(value, "bar", value_len);
	value[value_len-1] = '\0';
    }
    else
    {
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}

int smpd_get_smpd_data(char *key, char *value, int value_len)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY,
	0, 
	KEY_READ,
	&tkey);
    if (result != ERROR_SUCCESS)
    {
	if (smpd_get_smpd_data_default(key, value, value_len) != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to get the data for the key '%s'\n", key);
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }

    len = value_len;
    result = RegQueryValueEx(tkey, key, 0, NULL, (unsigned char *)value, &len);
    if (result != ERROR_SUCCESS)
    {
	RegCloseKey(tkey);
	if (smpd_get_smpd_data_default(key, value, value_len) != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to get the data for the key '%s'\n", key);
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }

    RegCloseKey(tkey);
    return SMPD_SUCCESS;
#else
    return SMPD_FAIL;
#endif
}
