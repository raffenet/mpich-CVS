/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_set_user_data(char *key, char *value)
{
    smpd_enter_fn("smpd_set_user_data");
    smpd_exit_fn("smpd_set_user_data");
    return SMPD_FAIL;
}

int smpd_set_smpd_data(char *key, char *value)
{
    smpd_enter_fn("smpd_set_smpd_data");
    smpd_exit_fn("smpd_set_smpd_data");
    return SMPD_FAIL;
}

int smpd_get_user_data_default(char *key, char *value, int value_len)
{
    smpd_enter_fn("smpd_get_user_data_default");
    smpd_exit_fn("smpd_get_user_data_default");
    return SMPD_FAIL;
}

int smpd_get_user_data(char *key, char *value, int value_len)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;

    smpd_enter_fn("smpd_get_user_data");

    result = RegOpenKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_KEY,
	0, 
	KEY_READ,
	&tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to open the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d\n", result);
	smpd_exit_fn("smpd_get_user_data");
	return SMPD_FAIL;
    }

    len = value_len;
    result = RegQueryValueEx(tkey, key, 0, NULL, (unsigned char *)value, &len);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to read the smpd registry key '%s', error %d\n", key, result);
	RegCloseKey(tkey);
	smpd_exit_fn("smpd_get_user_data");
	return SMPD_FAIL;
    }

    RegCloseKey(tkey);
    smpd_exit_fn("smpd_get_user_data");
    return SMPD_SUCCESS;
#else
    int result;
    smpd_enter_fn("smpd_get_user_data");
    result = smpd_get_user_data_default(key, value, value_len);
    smpd_exit_fn("smpd_get_user_data");
    return result;
#endif
}

int smpd_get_smpd_data_default(char *key, char *value, int value_len)
{
    smpd_enter_fn("smpd_get_smpd_data_default");
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
	smpd_exit_fn("smpd_get_smpd_data_default");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_get_smpd_data_default");
    return SMPD_SUCCESS;
}

int smpd_get_smpd_data(char *key, char *value, int value_len)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;

    smpd_enter_fn("smpd_get_smpd_data");

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY,
	0, 
	KEY_READ,
	&tkey);
    if (result != ERROR_SUCCESS)
    {
	if (smpd_get_smpd_data_default(key, value, value_len) != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to get the data for the key '%s'\n", key);
	    smpd_exit_fn("smpd_get_smpd_data");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_get_smpd_data");
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
	    smpd_exit_fn("smpd_get_smpd_data");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_get_smpd_data");
	return SMPD_SUCCESS;
    }

    RegCloseKey(tkey);
    smpd_exit_fn("smpd_get_smpd_data");
    return SMPD_SUCCESS;
#else
    int result;
    smpd_enter_fn("smpd_get_smpd_data");
    result = smpd_get_smpd_data_default(key, value, value_len);
    smpd_exit_fn("smpd_get_smpd_data");
    return result;
#endif
}
