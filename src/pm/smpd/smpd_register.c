/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include "smpd.h"
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>

#undef FCNAME
#define FCNAME "smpd_delete_current_password_registry_entry"
SMPD_BOOL smpd_delete_current_password_registry_entry()
{
    int nError;
    HKEY hRegKey = NULL;
    DWORD dwNumValues;

    smpd_enter_fn(FCNAME);

    nError = RegOpenKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY, 0, KEY_ALL_ACCESS, &hRegKey);
    if (nError != ERROR_SUCCESS && nError != ERROR_PATH_NOT_FOUND && nError != ERROR_FILE_NOT_FOUND)
    {
	smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegOpenKeyEx(...) failed, error: %d\n", nError);
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    nError = RegDeleteValue(hRegKey, "smpdPassword");
    if (nError != ERROR_SUCCESS && nError != ERROR_FILE_NOT_FOUND)
    {
	smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegDeleteValue(password) failed, error: %d\n", nError);
	RegCloseKey(hRegKey);
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    nError = RegDeleteValue(hRegKey, "smpdAccount");
    if (nError != ERROR_SUCCESS && nError != ERROR_FILE_NOT_FOUND)
    {
	smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegDeleteValue(account) failed, error: %d\n", nError);
	RegCloseKey(hRegKey);
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    if (RegQueryInfoKey(hRegKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwNumValues, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
	RegCloseKey(hRegKey);
	RegDeleteKey(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY);
    }
    else
    {
	RegCloseKey(hRegKey);
	if (dwNumValues == 0)
	    RegDeleteKey(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY);
    }

    smpd_exit_fn(FCNAME);
    return SMPD_TRUE;
}

#undef FCNAME
#define FCNAME "smpd_save_password_to_registry"
SMPD_BOOL smpd_save_password_to_registry(const char *szAccount, const char *szPassword, SMPD_BOOL persistent)
{
    int nError;
    SMPD_BOOL bResult = SMPD_TRUE;
    HKEY hRegKey = NULL;
    DATA_BLOB password_blob, blob;

    smpd_enter_fn(FCNAME);

    if (persistent)
    {
	if (RegCreateKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY,
	    0, 
	    NULL, 
	    REG_OPTION_NON_VOLATILE,
	    KEY_ALL_ACCESS, 
	    NULL,
	    &hRegKey, 
	    NULL) != ERROR_SUCCESS) 
	{
	    nError = GetLastError();
	    smpd_err_printf("SavePasswordToRegistry:RegCreateKeyEx(...) failed, error: %d\n", nError);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
    }
    else
    {
	RegDeleteKey(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY,
	    0, 
	    NULL, 
	    REG_OPTION_VOLATILE,
	    KEY_ALL_ACCESS, 
	    NULL,
	    &hRegKey, 
	    NULL) != ERROR_SUCCESS) 
	{
	    nError = GetLastError();
	    smpd_err_printf("SavePasswordToRegistry:RegDeleteKey(...) failed, error: %d\n", nError);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
    }

    /* Store the account name*/
    if (RegSetValueEx(hRegKey, "smpdAccount", 0, REG_SZ, (BYTE*)szAccount, (DWORD)strlen(szAccount)+1) != ERROR_SUCCESS)
    {
	nError = GetLastError();
	smpd_err_printf("SavePasswordToRegistry:RegSetValueEx(...) failed, error: %d\n", nError);
	RegCloseKey(hRegKey);
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    password_blob.cbData = (DWORD)strlen(szPassword)+1; /* store the NULL termination */
    password_blob.pbData = (BYTE*)szPassword;
    if (CryptProtectData(&password_blob, L"MPICH2 User Credentials", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blob))
    {
	/* Write data to registry.*/
	if (RegSetValueEx(hRegKey, "smpdPassword", 0, REG_BINARY, blob.pbData, blob.cbData) != ERROR_SUCCESS)
	{
	    nError = GetLastError();
	    smpd_err_printf("SavePasswordToRegistry:RegSetValueEx(...) failed, error: %d\n", nError);
	    bResult = SMPD_FALSE;
	}
	LocalFree(blob.pbData);
    }
    else
    {
	nError = GetLastError();
	smpd_err_printf("SavePasswordToRegistry:CryptProtectData(...) failed, error: %d\n", nError);
	bResult = SMPD_FALSE;
    }

    RegCloseKey(hRegKey);

    smpd_exit_fn(FCNAME);
    return bResult;
}

#undef FCNAME
#define FCNAME "smpd_read_password_from_registry"
SMPD_BOOL smpd_read_password_from_registry(char *szAccount, char *szPassword) 
{
    int nError;
    SMPD_BOOL bResult = SMPD_TRUE;
    HKEY hRegKey = NULL;
    DWORD dwType;
    DATA_BLOB password_blob, blob;

    smpd_enter_fn(FCNAME);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY, 0, KEY_QUERY_VALUE, &hRegKey) == ERROR_SUCCESS) 
    {
	DWORD dwLength = SMPD_MAX_PASSWORD_LENGTH;
	*szAccount = '\0';
	if (RegQueryValueEx(hRegKey, "smpdAccount", NULL, NULL, (BYTE*)szAccount, &dwLength) != ERROR_SUCCESS)
	{
	    nError = GetLastError();
	    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
	    RegCloseKey(hRegKey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
	if (strlen(szAccount) < 1)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}

	dwType = REG_BINARY;
	if (RegQueryValueEx(hRegKey, "smpdPassword", NULL, &dwType, NULL, &dwLength) == ERROR_SUCCESS)
	{
	    blob.cbData = dwLength;
	    blob.pbData = MPIU_Malloc(dwLength);
	    if (RegQueryValueEx(hRegKey, "smpdPassword", NULL, &dwType, (BYTE*)blob.pbData, &dwLength) == ERROR_SUCCESS)
	    {
		if (CryptUnprotectData(&blob, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &password_blob))
		{
		    strcpy(szPassword, (const char *)(password_blob.pbData));
		    LocalFree(password_blob.pbData);
		}
		else
		{
		    nError = GetLastError();
		    /*smpd_err_printf("ReadPasswordFromRegistry:CryptUnprotectData(...) failed, error: %d\n", nError);*/
		    bResult = SMPD_FALSE;
		}
	    }
	    else
	    {
		nError = GetLastError();
		/*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
		bResult = SMPD_FALSE;
	    }
	    MPIU_Free(blob.pbData);
	}
	else
	{
	    nError = GetLastError();
	    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
	    bResult = SMPD_FALSE;
	}
	RegCloseKey(hRegKey);
    }
    else
    {
	nError = GetLastError();
	/*smpd_err_printf("ReadPasswordFromRegistry:RegOpenKeyEx(...) failed, error: %d\n", nError);*/
	bResult = SMPD_FALSE;
    }

    smpd_exit_fn(FCNAME);
    return bResult;
}

#undef FCNAME
#define FCNAME "smpd_cache_password"
int smpd_cache_password(const char *account, const char *password)
{
    int nError;
    char szEncodedPassword[SMPD_MAX_PASSWORD_LENGTH*2+1];
    HKEY hRegKey = NULL;
    int num_bytes;
    DWORD len;

    smpd_enter_fn(FCNAME);

    if (smpd_option_on("nocache"))
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    RegDeleteKey(HKEY_CURRENT_USER, SMPD_REGISTRY_CACHE_KEY);
    if (RegCreateKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_CACHE_KEY,
	0, 
	NULL, 
	REG_OPTION_VOLATILE,
	KEY_ALL_ACCESS, 
	NULL,
	&hRegKey, 
	NULL) != ERROR_SUCCESS) 
    {
	nError = GetLastError();
	/*smpd_err_printf("CachePassword:RegDeleteKey(...) failed, error: %d\n", nError);*/
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* Store the account name*/
    len = (DWORD)strlen(account)+1;
    nError = RegSetValueEx(hRegKey, "smpda", 0, REG_SZ, (BYTE*)account, len);
    if (nError != ERROR_SUCCESS)
    {
	/*smpd_err_printf("CachePassword:RegSetValueEx(%s) failed, error: %d\n", g_pszAccount, nError);*/
	RegCloseKey(hRegKey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* encode the password*/
    smpd_encode_buffer(szEncodedPassword, SMPD_MAX_PASSWORD_LENGTH*2, password, (int)strlen(password)+1, &num_bytes);
    szEncodedPassword[num_bytes*2] = '\0';
    /*smpd_dbg_printf("szEncodedPassword = '%s'\n", szEncodedPassword);*/

    /* Store the encoded password*/
    nError = RegSetValueEx(hRegKey, "smpdp", 0, REG_SZ, (BYTE*)szEncodedPassword, num_bytes*2);
    if (nError != ERROR_SUCCESS)
    {
	/*smpd_err_printf("CachePassword:RegSetValueEx(...) failed, error: %d\n", nError);*/
	RegCloseKey(hRegKey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    RegCloseKey(hRegKey);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_cached_password"
SMPD_BOOL smpd_get_cached_password(char *account, char *password)
{
    int nError;
    char szAccount[SMPD_MAX_ACCOUNT_LENGTH];
    char szPassword[SMPD_MAX_PASSWORD_LENGTH*2];
    HKEY hRegKey = NULL;
    DWORD dwLength;
    int num_bytes;

    smpd_enter_fn(FCNAME);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, SMPD_REGISTRY_CACHE_KEY, 0, KEY_QUERY_VALUE, &hRegKey) == ERROR_SUCCESS) 
    {
	*szAccount = '\0';
	dwLength = SMPD_MAX_ACCOUNT_LENGTH;
	if ((nError = RegQueryValueEx(
	    hRegKey, 
	    "smpda", NULL, 
	    NULL, 
	    (BYTE*)szAccount, 
	    &dwLength))!=ERROR_SUCCESS)
	{
	    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
	    RegCloseKey(hRegKey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}
	if (strlen(szAccount) < 1)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}

	*szPassword = '\0';
	dwLength = SMPD_MAX_PASSWORD_LENGTH*2;
	if ((nError = RegQueryValueEx(
	    hRegKey, 
	    "smpdp", NULL, 
	    NULL, 
	    (BYTE*)szPassword, 
	    &dwLength))!=ERROR_SUCCESS)
	{
	    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
	    RegCloseKey(hRegKey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FALSE;
	}

	RegCloseKey(hRegKey);

	strcpy(account, szAccount);
	smpd_decode_buffer(szPassword, password, SMPD_MAX_PASSWORD_LENGTH, &num_bytes);
	smpd_exit_fn(FCNAME);
	return SMPD_TRUE;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "smpd_delete_cached_password"
int smpd_delete_cached_password()
{
    smpd_enter_fn(FCNAME);
    RegDeleteKey(HKEY_CURRENT_USER, SMPD_REGISTRY_CACHE_KEY);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
