/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

SMPD_BOOL smpd_option_on(const char *option)
{
    char val[SMPD_MAX_VALUE_LENGTH];

    if (smpd_get_smpd_data(option, val, SMPD_MAX_VALUE_LENGTH) == SMPD_SUCCESS)
    {
	if (strcmp(val, "yes") == 0 || strcmp(val, "1") == 0)
	    return SMPD_TRUE;
    }
    return SMPD_FALSE;
}

int smpd_delete_user_data(const char *key)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD result;

    smpd_enter_fn("smpd_delete_user_data");

    if (key == NULL)
	return SMPD_FAIL;

    result = RegCreateKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_KEY,
	0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to open the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d\n", result);
	smpd_exit_fn("smpd_delete_user_data");
	return SMPD_FAIL;
    }

    result = RegDeleteValue(tkey, key);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to delete the user smpd registry value '%s', error %d\n", key, result);
	RegCloseKey(tkey);
	smpd_exit_fn("smpd_delete_user_data");
	return SMPD_FAIL;
    }

    RegCloseKey(tkey);
    smpd_exit_fn("smpd_delete_user_data");
    return SMPD_SUCCESS;
#else
    smpd_enter_fn("smpd_delete_user_data");
    smpd_exit_fn("smpd_delete_user_data");
    return SMPD_FAIL;
#endif
}

int smpd_delete_smpd_data(const char *key)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD result;

    smpd_enter_fn("smpd_delete_smpd_data");

    if (key == NULL)
	return SMPD_FAIL;

    result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY,
	0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to open the HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY " registry key, error %d\n", result);
	smpd_exit_fn("smpd_delete_smpd_data");
	return SMPD_FAIL;
    }

    result = RegDeleteValue(tkey, key);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to delete the smpd registry value '%s', error %d\n", key, result);
	RegCloseKey(tkey);
	smpd_exit_fn("smpd_delete_smpd_data");
	return SMPD_FAIL;
    }

    RegCloseKey(tkey);
    smpd_exit_fn("smpd_delete_smpd_data");
    return SMPD_SUCCESS;
#else
    smpd_enter_fn("smpd_delete_smpd_data");
    smpd_exit_fn("smpd_delete_smpd_data");
    return SMPD_FAIL;
#endif
}

int smpd_set_user_data(const char *key, const char *value)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;

    smpd_enter_fn("smpd_set_user_data");

    if (key == NULL || value == NULL)
	return SMPD_FAIL;

    result = RegCreateKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_KEY,
	0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to open the HKEY_CURRENT_USER\\" SMPD_REGISTRY_KEY " registry key, error %d\n", result);
	smpd_exit_fn("smpd_set_user_data");
	return SMPD_FAIL;
    }

    len = (DWORD)(strlen(value)+1);
    result = RegSetValueEx(tkey, key, 0, REG_SZ, value, len);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to write the user smpd registry value '%s:%s', error %d\n", key, value, result);
	RegCloseKey(tkey);
	smpd_exit_fn("smpd_set_user_data");
	return SMPD_FAIL;
    }

    RegCloseKey(tkey);
    smpd_exit_fn("smpd_set_user_data");
    return SMPD_SUCCESS;
#else
    smpd_enter_fn("smpd_set_user_data");
    smpd_exit_fn("smpd_set_user_data");
    return SMPD_FAIL;
#endif
}

int smpd_set_smpd_data(const char *key, const char *value)
{
#ifdef HAVE_WINDOWS_H
    HKEY tkey;
    DWORD len, result;

    smpd_enter_fn("smpd_set_user_data");

    if (key == NULL || value == NULL)
	return SMPD_FAIL;

    result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY,
	0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to open the HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY " registry key, error %d\n", result);
	smpd_exit_fn("smpd_set_user_data");
	return SMPD_FAIL;
    }

    len = (DWORD)(strlen(value)+1);
    result = RegSetValueEx(tkey, key, 0, REG_SZ, value, len);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to write the smpd registry value '%s:%s', error %d\n", key, value, result);
	RegCloseKey(tkey);
	smpd_exit_fn("smpd_set_user_data");
	return SMPD_FAIL;
    }

    RegCloseKey(tkey);
    smpd_exit_fn("smpd_set_user_data");
    return SMPD_SUCCESS;
#else
    smpd_enter_fn("smpd_set_smpd_data");
    smpd_exit_fn("smpd_set_smpd_data");
    return SMPD_FAIL;
#endif
}

int smpd_get_user_data_default(const char *key, char *value, int value_len)
{
    smpd_enter_fn("smpd_get_user_data_default");
    smpd_exit_fn("smpd_get_user_data_default");
    return SMPD_FAIL;
}

int smpd_get_user_data(const char *key, char *value, int value_len)
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
	smpd_dbg_printf("Unable to read the smpd registry key '%s', error %d\n", key, result);
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

int smpd_get_smpd_data_default(const char *key, char *value, int value_len)
{
    smpd_enter_fn("smpd_get_smpd_data_default");
#ifdef HAVE_WINDOWS_H
    /* A default passphrase is only available for Windows */
    if (strcmp(key, "phrase") == 0)
    {
	strncpy(value, SMPD_DEFAULT_PASSPHRASE, value_len);
	value[value_len-1] = '\0';
	smpd_exit_fn("smpd_get_smpd_data_default");
	return SMPD_SUCCESS;
    }
#endif
    if (strcmp(key, "log") == 0)
    {
	strncpy(value, "no", value_len);
	value[value_len-1] = '\0';
    }
    else if (strcmp(key, "prepend_rank") == 0)
    {
	strncpy(value, "yes", value_len);
	value[value_len-1] = '\0';
    }
    else if (strcmp(key, "trace") == 0)
    {
	strncpy(value, "yes", value_len);
	value[value_len-1] = '\0';
    }
    else if (strcmp(key, "noprompt") == 0)
    {
	strncpy(value, "no", value_len);
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

int smpd_get_smpd_data(const char *key, char *value, int value_len)
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
	    smpd_dbg_printf("Unable to get the data for the key '%s'\n", key);
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
    char smpd_filename[SMPD_MAX_FILENAME] = "";
    char *homedir;
    FILE *fin;
    char line[SMPD_PASSPHRASE_MAX_LENGTH+3];
    struct stat s;

    smpd_enter_fn("smpd_get_smpd_data");
    if (strcmp(key, "phrase") == 0)
    {
	homedir = getenv("HOME");
	strcpy(smpd_filename, homedir);
	if (smpd_filename[strlen(smpd_filename)-1] != '/')
	    strcat(smpd_filename, "/.smpd");
	else
	    strcat(smpd_filename, ".smpd");

	if (stat(smpd_filename, &s) == 0)
	{
	    if (s.st_mode & 00077)
	    {
		printf("smpd passphrase file, %s, cannot be readable by anyone other than the current user, please set the permissions accordingly (0600).\n", smpd_filename);
	    }
	    else
	    {
		fin = fopen(smpd_filename, "r");
		if (fin != NULL)
		{
		    fgets(line, SMPD_PASSPHRASE_MAX_LENGTH+2, fin);
		    line[SMPD_PASSPHRASE_MAX_LENGTH] = '\0';
		    if (strlen(line) > 0)
		    {
			while (strlen(line) > 0 && (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n'))
			    line[strlen(line)-1] = '\0';
			if (strlen(line) > 0)
			{
			    strcpy(smpd_process.passphrase, line);
			    fclose(fin);
			    smpd_exit_fn("smpd_get_smpd_data");
			    return SMPD_SUCCESS;
			}
		    }
		    fclose(fin);
		}
	    }
	}
    }
    result = smpd_get_smpd_data_default(key, value, value_len);
    smpd_exit_fn("smpd_get_smpd_data");
    return result;
#endif
}
