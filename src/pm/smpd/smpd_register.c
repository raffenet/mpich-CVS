/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include "smpd.h"
#include <wincrypt.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>

SMPD_BOOL smpd_setup_crypto_client()
{
    /* Ensure that the default cryptographic client is set up.*/
    HCRYPTPROV hProv;
    HCRYPTKEY hKey;
    int nError;

    smpd_enter_fn("smpd_setup_crypto_client");

    /* Attempt to acquire a handle to the default key container.*/
    if (!CryptAcquireContext(&hProv, "MPICH", MS_DEF_PROV, PROV_RSA_FULL, 0))
    {
	/* Some sort of error occured, create default key container.*/
	if (!CryptAcquireContext(&hProv, "MPICH", MS_DEF_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET))
	{
	    /* Error creating key container!*/
	    nError = GetLastError();
	    smpd_err_printf("SetupCryptoClient:CryptAcquireContext(...) failed, error: %d\n", nError);
	    smpd_exit_fn("smpd_setup_crypto_client");
	    return SMPD_FALSE;
	}
    }

    /* Attempt to get handle to signature key.*/
    if (!CryptGetUserKey(hProv, AT_SIGNATURE, &hKey))
    {
	if ((nError = GetLastError()) == NTE_NO_KEY)
	{
	    /* Create signature key pair.*/
	    if (!CryptGenKey(hProv, AT_SIGNATURE, 0, &hKey))
	    {
		/* Error during CryptGenKey!*/
		nError = GetLastError();
		CryptReleaseContext(hProv, 0);
		smpd_err_printf("SetupCryptoClient:CryptGenKey(...) failed, error: %d\n", nError);
		smpd_exit_fn("smpd_setup_crypto_client");
		return SMPD_FALSE;
	    }
	    else
	    {
		CryptDestroyKey(hKey);
	    }
	}
	else 
	{
	    /* Error during CryptGetUserKey!*/
	    CryptReleaseContext(hProv, 0);
	    smpd_err_printf("SetupCryptoClient:CryptGetUserKey(...) failed, error: %d\n", nError);
	    smpd_exit_fn("smpd_setup_crypto_client");
	    return SMPD_FALSE;
	}
    }

    /* Attempt to get handle to exchange key.*/
    if (!CryptGetUserKey(hProv,AT_KEYEXCHANGE,&hKey))
    {
	if ((nError = GetLastError()) == NTE_NO_KEY)
	{
	    /* Create key exchange key pair.*/
	    if (!CryptGenKey(hProv,AT_KEYEXCHANGE,0,&hKey))
	    {
		/* Error during CryptGenKey!*/
		nError = GetLastError();
		CryptReleaseContext(hProv, 0);
		smpd_err_printf("SetupCryptoClient:CryptGenKey(...) failed, error: %d\n", nError);
		smpd_exit_fn("smpd_setup_crypto_client");
		return SMPD_FALSE;
	    }
	    else
	    {
		CryptDestroyKey(hKey);
	    }
	}
	else
	{
	    /* Error during CryptGetUserKey!*/
	    CryptReleaseContext(hProv, 0);
	    smpd_err_printf("SetupCryptoClient:CryptGetUserKey(...) failed, error: %d\n", nError);
	    smpd_exit_fn("smpd_setup_crypto_client");
	    return SMPD_FALSE;
	}
    }

    CryptReleaseContext(hProv, 0);
    smpd_exit_fn("smpd_setup_crypto_client");
    return SMPD_TRUE;
}

SMPD_BOOL smpd_delete_current_password_registry_entry()
{
    int nError;
    HKEY hRegKey = NULL;
    DWORD dwNumValues;

    smpd_enter_fn("smpd_delete_current_password_registry_entry");

    if (RegOpenKeyEx(HKEY_CURRENT_USER, MPICH_REGISTRY_KEY, 0, KEY_ALL_ACCESS, &hRegKey) != ERROR_SUCCESS)
    {
	nError = GetLastError();
	smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegOpenKeyEx(...) failed, error: %d\n", nError);
	smpd_exit_fn("smpd_delete_current_password_registry_entry");
	return SMPD_FALSE;
    }

    if (RegDeleteValue(hRegKey, TEXT("smpdPassword")) != ERROR_SUCCESS)
    {
	nError = GetLastError();
	smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegDeleteValue(...) failed, error: %d\n", nError);
	RegCloseKey(hRegKey);
	smpd_exit_fn("smpd_delete_current_password_registry_entry");
	return SMPD_FALSE;
    }

    if (RegDeleteValue(hRegKey, TEXT("smpdAccount")) != ERROR_SUCCESS)
    {
	nError = GetLastError();
	smpd_err_printf("DeleteCurrentPasswordRegistryEntry:RegDeleteValue(...) failed, error: %d\n", nError);
	RegCloseKey(hRegKey);
	smpd_exit_fn("smpd_delete_current_password_registry_entry");
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

    smpd_exit_fn("smpd_delete_current_password_registry_entry");
    return SMPD_TRUE;
}

SMPD_BOOL smpd_save_password_to_registry(const char *szAccount, const char *szPassword, SMPD_BOOL persistent)
{
    int nError;
    SMPD_BOOL bResult = SMPD_TRUE;
    TCHAR szKey[256];
    HKEY hRegKey = NULL;
    HCRYPTPROV hProv = (HCRYPTPROV)NULL;
    HCRYPTKEY hKey = (HCRYPTKEY)NULL;
    HCRYPTHASH hHash = (HCRYPTHASH)NULL;
    DWORD dwLength;
    BYTE *pbBuffer;
    TCHAR szLocalPassword[] = _T("MMPzI6C@HaA0NiL*I%Ll");

    smpd_enter_fn("smpd_save_password_to_registry");

    _tcscpy(szKey, MPICH_REGISTRY_KEY);

    if (persistent)
    {
	if (RegCreateKeyEx(HKEY_CURRENT_USER, szKey,
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
	    smpd_exit_fn("smpd_save_password_to_registry");
	    return SMPD_FALSE;
	}
    }
    else
    {
	RegDeleteKey(HKEY_CURRENT_USER, szKey);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, szKey,
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
	    smpd_exit_fn("smpd_save_password_to_registry");
	    return SMPD_FALSE;
	}
    }

    /* Store the account name*/
    if (RegSetValueEx(
	hRegKey, _T("smpdAccount"), 0, REG_SZ, 
	(BYTE*)szAccount, 
	(DWORD)sizeof(TCHAR)*(_tcslen(szAccount)+1)
	)!=ERROR_SUCCESS)
    {
	nError = GetLastError();
	smpd_err_printf("SavePasswordToRegistry:RegSetValueEx(...) failed, error: %d\n", nError);
	RegCloseKey(hRegKey);
	smpd_exit_fn("smpd_save_password_to_registry");
	return SMPD_FALSE;
    }

    /* Get handle to user default provider.*/
    if (CryptAcquireContext(&hProv, "MPICH", NULL, PROV_RSA_FULL, 0))
    {
	/* Create hash object.*/
	if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
	{
	    /* Hash password string.*/
	    dwLength = (DWORD)sizeof(TCHAR)*_tcslen(szLocalPassword);
	    if (CryptHashData(hHash, (BYTE *)szLocalPassword, dwLength, 0))
	    {
		/* Create block cipher session key based on hash of the password.*/
		if (CryptDeriveKey(hProv, CALG_RC4, hHash, CRYPT_EXPORTABLE, &hKey))
		{
		    /* Determine number of bytes to encrypt at a time.*/
		    dwLength = (DWORD)sizeof(TCHAR)*(_tcslen(szPassword)+1);

		    /* Allocate memory.*/
		    pbBuffer = (BYTE *)malloc(dwLength);
		    if (pbBuffer != NULL)
		    {
			memcpy(pbBuffer, szPassword, dwLength);
			/* Encrypt data*/
			if (CryptEncrypt(hKey, 0, TRUE, 0, pbBuffer, &dwLength, dwLength)) 
			{
			    /* Write data to registry.*/
			    /* Add the password.*/
			    if (RegSetValueEx(hRegKey, _T("smpdPassword"), 0, REG_BINARY, pbBuffer, dwLength)!=ERROR_SUCCESS)
			    {
				nError = GetLastError();
				smpd_err_printf("SavePasswordToRegistry:RegSetValueEx(...) failed, error: %d\n", nError);
				bResult = SMPD_FALSE;
			    }
			    RegCloseKey(hRegKey);
			}	
			else
			{
			    nError = GetLastError();
			    smpd_err_printf("SavePasswordToRegistry:CryptEncrypt(...) failed, error: %d\n", nError);
			    bResult = SMPD_FALSE;
			}
			/* Free memory.*/
			free(pbBuffer);
		    }
		    else
		    {
			nError = GetLastError();
			smpd_err_printf("SavePasswordToRegistry:malloc(...) failed, error: %d\n", nError);
			bResult = SMPD_FALSE;
		    }
		    CryptDestroyKey(hKey);  /* Release provider handle.*/
		}
		else
		{
		    /* Error during CryptDeriveKey!*/
		    nError = GetLastError();
		    smpd_err_printf("SavePasswordToRegistry:CryptDeriveKey(...) failed, error: %d\n", nError);
		    bResult = SMPD_FALSE;
		}
	    }
	    else
	    {
		/* Error during CryptHashData!*/
		nError = GetLastError();
		smpd_err_printf("SavePasswordToRegistry:CryptHashData(...) failed, error: %d\n", nError);
		bResult = SMPD_FALSE;
	    }
	    CryptDestroyHash(hHash); /* Destroy session key.*/
	}
	else
	{
	    /* Error during CryptCreateHash!*/
	    nError = GetLastError();
	    smpd_err_printf("SavePasswordToRegistry:CryptCreateHash(...) failed, error: %d\n", nError);
	    bResult = SMPD_FALSE;
	}
	CryptReleaseContext(hProv, 0);
    }

    smpd_exit_fn("smpd_save_password_to_registry");
    return bResult;
}

/*
The following function reads the password from the registry and decrypts it. 
Note that the szPassword parameter should be already allocated with a minimum 
size of 32 characters (64 bytes if using UNICODE). 
The account buffer must be able to hold 100 characters.
*/
SMPD_BOOL smpd_read_password_from_registry(char *szAccount, char *szPassword) 
{
    int nError;
    SMPD_BOOL bResult = SMPD_TRUE;
    TCHAR szKey[256];
    HKEY hRegKey = NULL;
    HCRYPTPROV hProv = (HCRYPTPROV)NULL;
    HCRYPTKEY hKey = (HCRYPTKEY)NULL;
    HCRYPTHASH hHash = (HCRYPTHASH)NULL;
    DWORD dwType;
    /* has to be the same used to encrypt!*/
    TCHAR szLocalPassword[] = _T("MMPzI6C@HaA0NiL*I%Ll");

    smpd_enter_fn("smpd_read_password_from_registry");

    _tcscpy(szKey, MPICH_REGISTRY_KEY);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_QUERY_VALUE, &hRegKey) == ERROR_SUCCESS) 
    {
	DWORD dwLength = 100;
	*szAccount = TEXT('\0');
	if (RegQueryValueEx(
	    hRegKey, 
	    _T("smpdAccount"), NULL, 
	    NULL, 
	    (BYTE*)szAccount, 
	    &dwLength)!=ERROR_SUCCESS)
	{
	    nError = GetLastError();
	    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
	    RegCloseKey(hRegKey);
	    smpd_exit_fn("smpd_read_password_from_registry");
	    return SMPD_FALSE;
	}
	if (_tcslen(szAccount) < 1)
	{
	    smpd_exit_fn("smpd_read_password_from_registry");
	    return SMPD_FALSE;
	}

	/* Get handle to user default provider.*/
	if (CryptAcquireContext(&hProv, "MPICH", NULL, PROV_RSA_FULL, 0))
	{
	    /* Create hash object.*/
	    if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
	    {
		/* Hash password string.*/
		dwLength = (DWORD)sizeof(TCHAR)*_tcslen(szLocalPassword);
		if (CryptHashData(hHash, (BYTE *)szLocalPassword, dwLength, 0))
		{
		    /* Create block cipher session key based on hash of the password.*/
		    if (CryptDeriveKey(hProv, CALG_RC4, hHash, CRYPT_EXPORTABLE, &hKey))
		    {
			/* the password is less than 32 characters*/
			dwLength = 32*sizeof(TCHAR);
			dwType = REG_BINARY;
			if (RegQueryValueEx(hRegKey, _T("smpdPassword"), NULL, &dwType, (BYTE*)szPassword, &dwLength)==ERROR_SUCCESS)
			{
			    if (!CryptDecrypt(hKey, 0, TRUE, 0, (BYTE *)szPassword, &dwLength))
			    {
				nError = GetLastError();
				/*smpd_err_printf("ReadPasswordFromRegistry:CryptDecrypt(...) failed, error: %d\n", nError);*/
				bResult = SMPD_FALSE;
			    }
			}
			else
			{
			    nError = GetLastError();
			    /*smpd_err_printf("ReadPasswordFromRegistry:RegQueryValueEx(...) failed, error: %d\n", nError);*/
			    bResult = SMPD_FALSE;
			}
			CryptDestroyKey(hKey);  /* Release provider handle.*/
		    }
		    else
		    {
			/* Error during CryptDeriveKey!*/
			nError = GetLastError();
			/*smpd_err_printf("ReadPasswordFromRegistry:CryptDeriveKey(...) failed, error: %d\n", nError);*/
			bResult = SMPD_FALSE;
		    }
		}
		else
		{
		    /* Error during CryptHashData!*/
		    nError = GetLastError();
		    /*smpd_err_printf("ReadPasswordFromRegistry:CryptHashData(...) failed, error: %d\n", nError);*/
		    bResult = SMPD_FALSE;
		}
		CryptDestroyHash(hHash); /* Destroy session key.*/
	    }
	    else
	    {
		/* Error during CryptCreateHash!*/
		nError = GetLastError();
		/*smpd_err_printf("ReadPasswordFromRegistry:CryptCreateHash(...) failed, error: %d\n", nError);*/
		bResult = SMPD_FALSE;
	    }
	    CryptReleaseContext(hProv, 0);
	}
	RegCloseKey(hRegKey);
    }	
    else
    {
	nError = GetLastError();
	/*smpd_err_printf("ReadPasswordFromRegistry:RegOpenKeyEx(...) failed, error: %d\n", nError);*/
	bResult = SMPD_FALSE;
    }

    smpd_exit_fn("smpd_read_password_from_registry");
    return bResult;
}

int smpd_cache_password(const char *account, const char *password)
{
    int nError;
    char szEncodedPassword[SMPD_MAX_PASSWORD_LENGTH*2+1];
    HKEY hRegKey = NULL;
    int num_bytes;
    DWORD len;

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
	return SMPD_FAIL;
    }

    /* Store the account name*/
    len = (DWORD)strlen(account)+1;
    if ((nError = RegSetValueEx(
	hRegKey, "smpda", 0, REG_SZ, 
	(BYTE*)account, 
	len
	))!=ERROR_SUCCESS)
    {
	/*smpd_err_printf("CachePassword:RegSetValueEx(%s) failed, error: %d\n", g_pszAccount, nError);*/
	RegCloseKey(hRegKey);
	return SMPD_FAIL;
    }

    /* encode the password*/
    smpd_encode_buffer(szEncodedPassword, SMPD_MAX_PASSWORD_LENGTH*2, password, (int)strlen(password)+1, &num_bytes);
    szEncodedPassword[num_bytes*2] = '\0';
    /*smpd_dbg_printf("szEncodedPassword = '%s'\n", szEncodedPassword);*/

    /* Store the encoded password*/
    if ((nError = RegSetValueEx(
	hRegKey, "smpdp", 0, REG_SZ, 
	(BYTE*)szEncodedPassword, 
	num_bytes*2
	))!=ERROR_SUCCESS)
    {
	/*smpd_err_printf("CachePassword:RegSetValueEx(...) failed, error: %d\n", nError);*/
	RegCloseKey(hRegKey);
	return SMPD_FAIL;
    }

    RegCloseKey(hRegKey);
    return SMPD_SUCCESS;
}

SMPD_BOOL smpd_get_cached_password(char *account, char *password)
{
    int nError;
    char szAccount[SMPD_MAX_ACCOUNT_LENGTH];
    char szPassword[SMPD_MAX_PASSWORD_LENGTH*2];
    HKEY hRegKey = NULL;
    DWORD dwLength;
    int num_bytes;

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
	    return SMPD_FALSE;
	}
	if (strlen(szAccount) < 1)
	    return SMPD_FALSE;

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
	    return SMPD_FALSE;
	}

	RegCloseKey(hRegKey);

	strcpy(account, szAccount);
	smpd_decode_buffer(szPassword, password, SMPD_MAX_PASSWORD_LENGTH, &num_bytes);
	return SMPD_TRUE;
    }

    return SMPD_FALSE;
}

int smpd_delete_cached_password()
{
    RegDeleteKey(HKEY_CURRENT_USER, SMPD_REGISTRY_CACHE_KEY);
    return SMPD_SUCCESS;
}
