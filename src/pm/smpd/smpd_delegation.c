/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_WINDOWS_H
#include "smpd_service.h"
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <lm.h>
#endif

int smpd_register_spn(const char *dc, const char *dn, const char *dh)
{
    DWORD len;
    char err_msg[256];
    char **spns;
    HANDLE ds;
    DWORD result;
    char domain_controller[100] = "";
    char domain_name[100] = "";
    char domain_host[100] = "";
    char host[100] = "";
    int really = 0;
    char *really_env;
    PDOMAIN_CONTROLLER_INFO pInfo;

    result = DsGetDcName(NULL/*local computer*/, NULL, NULL, NULL,
	/*DS_IS_FLAT_NAME | DS_RETURN_DNS_NAME | DS_DIRECTORY_SERVICE_REQUIRED, */
	DS_DIRECTORY_SERVICE_REQUIRED | DS_KDC_REQUIRED,
	&pInfo);
    if (result == ERROR_SUCCESS)
    {
	strcpy(domain_controller, pInfo->DomainControllerName);
	strcpy(domain_name, pInfo->DomainName);
	NetApiBufferFree(pInfo);
    }

    if (dc && *dc != '\0')
    {
	strcpy(domain_controller, dc);
    }
    if (dn && *dn != '\0')
    {
	strcpy(domain_name, dn);
    }
    if (dh && *dh != '\0')
    {
	strcpy(domain_host, dh);
    }
    if (domain_host[0] == '\0')
    {
	smpd_get_hostname(host, 100);
	if (domain_name[0] != '\0')
	{
	    sprintf(domain_host, "%s\\%s", domain_name, host);
	}
	else
	{
	    strcpy(domain_host, host);
	}
    }

    printf("DsBind(%s, %s, ...)\n", domain_controller, domain_name);
    result = DsBind(
	domain_controller[0] == '\0' ? NULL : domain_controller,
	domain_name[0] == '\0' ? NULL : domain_name, &ds);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 256, NULL);
	smpd_err_printf("DsBind failed: %s\n", err_msg);
	/*ExitProcess(result);*/
	return SMPD_FAIL;
    }

    really_env = getenv("really");
    if (really_env)
	really = 1;

#if 1
    len = 1;
    /*result = DsGetSpn(DS_SPN_SERVICE, SMPD_SERVICE_NAME, SMPD_SERVICE_NAME, 0, 0, NULL, NULL, &len, &spns);*/
    result = DsGetSpn(DS_SPN_DNS_HOST, SMPD_SERVICE_NAME, NULL, SMPD_LISTENER_PORT, 0, NULL, NULL, &len, &spns);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, 256, NULL);
	smpd_err_printf("DsGetSpn failed: %s\n", err_msg);
	/*ExitProcess(result);*/
	return SMPD_FAIL;
    }
    if (really)
    {
	printf("registering: %s\n", spns[0]);
	printf("on account: %s\n", domain_host);
	result = DsWriteAccountSpn(ds, DS_SPN_ADD_SPN_OP, domain_host, 1, spns);
	if (result != ERROR_SUCCESS)
	{
	    DsFreeSpnArray(1, spns);
	    smpd_translate_win_error(result, err_msg, 256, NULL);
	    smpd_err_printf("DsWriteAccountSpn failed: %s\n", err_msg);
	    /*ExitProcess(result);*/
	    return SMPD_FAIL;
	}
    }
    else
    {
	printf("would register '%s' on %s\n", spns[0], domain_host);
    }
    DsFreeSpnArray(1, spns);
#else
    if (really)
    {
	result = DsServerRegisterSpn(DS_SPN_ADD_SPN_OP, SMPD_SERVICE_NAME, domain_host);
	if (result != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(result, err_msg, 256, NULL);
	    smpd_err_printf("DsServerRegisterSpn failed: %s\n", err_msg);
	    /*ExitProcess(result);*/
	    return SMPD_FAIL;
	}
    }
    else
    {
	printf("would register '%s' on %s\n", SMPD_SERVICE_NAME, domain_host);
    }
#endif
    result = DsUnBind(&ds);
    return SMPD_SUCCESS;
}

#if 0
/* ScpCreate

 Create a new service connection point as a child object of the 
 local server computer object.
*/
DWORD
ScpCreate(
       USHORT usPort, /* Service's default port to store in SCP. */
       LPTSTR szClass, /* Service class string to store in SCP. */
       LPTSTR szAccount, /* Logon account that must access SCP. */
       UINT ccDN, /* Length of the pszDN buffer in characters */
       TCHAR *pszDN) /* Returns distinguished name of SCP. */
{
    DWORD dwStat, dwAttr, dwLen;
    HRESULT hr;
    IDispatch *pDisp;          /* Returned dispinterface of new object. */
    IDirectoryObject *pComp;   /* Computer object; parent of SCP. */
    IADs *pIADsSCP;            /* IADs interface on new object. */

    if(!szClass || !szAccount || !pszDN || !(ccDN > 0))
    {
	hr = ERROR_INVALID_PARAMETER;
	ReportError(TEXT("Invalid parameter."), hr);
	return hr;
    }

    /* Values for SCPs keywords attribute. */
    TCHAR* KwVal[]={
	SMPD_SERVICE_GUID, /* Vendor GUID. */
	SMPD_SERVICE_GUID, /* Product GUID. */
	SMPD_PRODUCT_VENDOR, /* Vendor Name. */
	SMPD_PRODUCT, /* Product Name. */
    };

    TCHAR       szServer[MAX_PATH];
    TCHAR       szDn[MAX_PATH];
    TCHAR       szAdsPath[MAX_PATH];
    TCHAR       szPort[6];

    HKEY        hReg;
    DWORD       dwDisp;

    ADSVALUE cn,objclass,keywords[4],binding,classname,dnsname,nametype;

    /* SCP attributes to set during creation of SCP. */
    ADS_ATTR_INFO   ScpAttribs[] = 
    {
	{
	    TEXT("cn"),
		ADS_ATTR_UPDATE,
		ADSTYPE_CASE_IGNORE_STRING,
		&cn,
		1
	},
	{
	    TEXT("objectClass"),
		ADS_ATTR_UPDATE,
		ADSTYPE_CASE_IGNORE_STRING,
		&objclass,
		1
	},
	    {
		TEXT("keywords"),
		    ADS_ATTR_UPDATE,
		    ADSTYPE_CASE_IGNORE_STRING,
		    keywords,
		    4
	    },
	    {
		TEXT("serviceDnsName"),
		    ADS_ATTR_UPDATE,
		    ADSTYPE_CASE_IGNORE_STRING,
		    &dnsname,
		    1
	    },
		{
		    TEXT("serviceDnsNameType"),
			ADS_ATTR_UPDATE,
			ADSTYPE_CASE_IGNORE_STRING,
			&nametype,
			1
		},
		{
		    TEXT("serviceClassName"),
			ADS_ATTR_UPDATE,
			ADSTYPE_CASE_IGNORE_STRING,
			&classname,
			1
		},
		    {
			TEXT("serviceBindingInformation"),
			    ADS_ATTR_UPDATE,
			    ADSTYPE_CASE_IGNORE_STRING,
			    &binding,
			    1
		    },
    };

    BSTR bstrGuid = NULL;
    TCHAR pwszBindByGuidStr[1024]; 
    VARIANT var;

    /* Get the DNS name of the local computer. */
    dwLen = sizeof(szServer);
    if (!GetComputerNameEx(ComputerNameDnsFullyQualified,szServer,&dwLen))
	return GetLastError();
    _tprintf(TEXT("GetComputerNameEx: %s\n"), szServer);

    /* Enter the attribute values to be stored in the SCP. */
    keywords[0].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[1].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[2].dwType = ADSTYPE_CASE_IGNORE_STRING;
    keywords[3].dwType = ADSTYPE_CASE_IGNORE_STRING;

    keywords[0].CaseIgnoreString=KwVal[0];
    keywords[1].CaseIgnoreString=KwVal[1];
    keywords[2].CaseIgnoreString=KwVal[2];
    keywords[3].CaseIgnoreString=KwVal[3];

    cn.dwType                   = ADSTYPE_CASE_IGNORE_STRING;
    cn.CaseIgnoreString         = TEXT("SockAuthAD");
    objclass.dwType             = ADSTYPE_CASE_IGNORE_STRING;
    objclass.CaseIgnoreString   = TEXT("serviceConnectionPoint");

    dnsname.dwType              = ADSTYPE_CASE_IGNORE_STRING;
    dnsname.CaseIgnoreString    = szServer;
    classname.dwType            = ADSTYPE_CASE_IGNORE_STRING;
    classname.CaseIgnoreString  = szClass;

    _stprintf(szPort,TEXT("%d"),usPort);
    binding.dwType              = ADSTYPE_CASE_IGNORE_STRING;
    binding.CaseIgnoreString    = szPort;
    nametype.dwType             = ADSTYPE_CASE_IGNORE_STRING;
    nametype.CaseIgnoreString   = TEXT("A");

    /*
    Get the distinguished name of the computer object for the local 
    computer.
    */
    dwLen = sizeof(szDn);
    if (!GetComputerObjectName(NameFullyQualifiedDN,szDn,&dwLen))
	return GetLastError();
    _tprintf(TEXT("GetComputerObjectName: %s\n"), szDn);

    /*
    Compose the ADSpath and bind to the computer object for the local 
    computer.
    */
    _tcsncpy(szAdsPath,TEXT("LDAP://"),MAX_PATH);
    _tcsncat(szAdsPath,szDn,MAX_PATH - _tcslen(szAdsPath));
    hr = ADsGetObject(szAdsPath, IID_IDirectoryObject, (void **)&pComp);
    if (FAILED(hr)) {
	ReportError(TEXT("Failed to bind Computer Object."),hr);
	return hr;
    }

    /********************************************************************
     * Publish the SCP as a child of the computer object
     *********************************************************************/

    /* Calculate attribute count. */
    dwAttr = sizeof(ScpAttribs)/sizeof(ADS_ATTR_INFO);  

    /* Complete the action. */
    hr = pComp->CreateDSObject(TEXT("cn=SockAuthAD"),
	ScpAttribs, dwAttr, &pDisp);
    if (FAILED(hr)) {
	ReportError(TEXT("Failed to create SCP:"), hr);
	pComp -> Release();
	return hr;
    }

    pComp -> Release();

    /* Query for an IADs pointer on the SCP object. */
    hr = pDisp->QueryInterface(IID_IADs,(void **)&pIADsSCP);
    if (FAILED(hr)) {
	ReportError(TEXT("Failed to QueryInterface for IADs:"),hr);
	pDisp->Release();
	return hr;
    }
    pDisp->Release();

    /* Set ACEs on the SCP so a service can modify it. */
    hr = AllowAccessToScpProperties(
	szAccount,     /* Service account to allow access. */
	pIADsSCP);     /* IADs pointer to the SCP object. */
    if (FAILED(hr)) {
	ReportError(TEXT("Failed to set ACEs on SCP DACL:"), hr);
	return hr;
    }

    /* Get the distinguished name of the SCP. */
    VariantInit(&var); 
    hr = pIADsSCP->Get(CComBSTR("distinguishedName"), &var); 
    if (FAILED(hr)) {
	ReportError(TEXT("Failed to get distinguishedName:"), hr);
	pIADsSCP->Release();
	return hr;
    }
    _tprintf(TEXT("distinguishedName via IADs: %s\n"), var.bstrVal);

    /* Return the DN of the SCP, which is used to compose the SPN.
     The best practice is to either accept and return the buffer
     size or do this in a _try / _except block, both omitted here
     for clarity. */
    _tcsncpy(pszDN, var.bstrVal, ccDN);

    /* Retrieve the SCP objectGUID in format suitable for binding. */
    hr = pIADsSCP->get_GUID(&bstrGuid); 
    if (FAILED(hr)) {
	ReportError(TEXT("Failed to get GUID:"), hr);
	pIADsSCP->Release();
	return hr;
    }

    /* Build a string for binding to the object by GUID. */
    _tcsncpy(pwszBindByGuidStr, 
	TEXT("LDAP://<GUID="),
	1024);
    _tcsncat(pwszBindByGuidStr, 
	bstrGuid, 
	1024 -_tcslen(pwszBindByGuidStr));
    _tcsncat(pwszBindByGuidStr, 
	TEXT(">"), 
	1024 -_tcslen(pwszBindByGuidStr));
    _tprintf(TEXT("GUID binding string: %s\n"), 
	pwszBindByGuidStr);

    pIADsSCP->Release();

    /* Create a registry key under 
     HKEY_LOCAL_MACHINE\SOFTWARE\Vendor\Product.
     */
    dwStat = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
	TEXT("Software\\Fabrikam\\Auth-O-Matic"),
	0,
	NULL,
	REG_OPTION_NON_VOLATILE,
	KEY_ALL_ACCESS,
	NULL,
	&hReg,
	&dwDisp);
    if (dwStat != NO_ERROR) {
	ReportError(TEXT("RegCreateKeyEx failed:"), dwStat);
	return dwStat;
    }

    /* Cache the GUID binding string under the registry key. */
    dwStat = RegSetValueEx(hReg, TEXT("GUIDBindingString"), 0, REG_SZ,
	(const BYTE *)pwszBindByGuidStr, 
	2*(_tcslen(pwszBindByGuidStr)));
    if (dwStat != NO_ERROR) {
	ReportError(TEXT("RegSetValueEx failed:"), dwStat);
	return dwStat;
    }

    RegCloseKey(hReg);

    /* Cleanup should delete the SCP and registry key if an error occurs. */

    return dwStat;
}

DWORD SpnCompose(
    TCHAR ***pspn,          /* Output: an array of SPNs */
    unsigned long *pulSpn,  /* Output: the number of SPNs returned */
    TCHAR *pszDNofSCP,      /* Input: DN of the service's SCP */
    TCHAR* pszServiceClass) /* Input: the name of the service class */
{
    DWORD   dwStatus;

    dwStatus = DsGetSpn(
	DS_SPN_SERVICE, /* Type of SPN to create (enumerated type) */
	pszServiceClass, /* Service class - a name in this case */
	pszDNofSCP, /* Service name - DN of the service SCP */
	0, /* Default: omit port component of SPN */
	0, /* Number of entries in hostnames and ports arrays */
	NULL, /* Array of hostnames. Default is local computer */
	NULL, /* Array of ports. Default omits port component */
	pulSpn, /* Receives number of SPNs returned in array */
	pspn /* Receives array of SPN(s) */
	);

    return dwStatus;
}

/***************************************************************************

    SpnRegister()

    Register or unregister the SPNs under the service's account.

    If the service runs in LocalSystem account, pszServiceAcctDN is the 
    distinguished name of the local computer account.

    Parameters:

    pszServiceAcctDN - Contains the distinguished name of the logon 
    account for this instance of the service.

    pspn - Contains an array of SPNs to register.

    ulSpn - Contains the number of SPNs in the array.

    Operation - Contains one of the DS_SPN_WRITE_OP values that determines 
    the type of operation to perform on the SPNs.

***************************************************************************/

DWORD SpnRegister(TCHAR *pszServiceAcctDN,
                  TCHAR **pspn,
                  unsigned long ulSpn,
                  DS_SPN_WRITE_OP Operation)
{
    DWORD dwStatus;
    HANDLE hDs;
    TCHAR szSamName[512];
    DWORD dwSize = sizeof(szSamName);
    PDOMAIN_CONTROLLER_INFO pDcInfo;

    /* Bind to a domain controller. */
    /* Get the domain for the current user. */
    GetComputerNameEx(samcompatible, buf, size);
    if(GetUserNameEx(NameSamCompatible, szSamName, &dwSize))
    {
        TCHAR *pWhack = _tcschr(szSamName, '\\');
        if(pWhack)
        {
            *pWhack = '\0';
        }
    } 
    else 
    {
        return GetLastError();
    }
     
    /* Get the name of a domain controller in that domain. */
    dwStatus = DsGetDcName(NULL,
        szSamName,
        NULL,
        NULL,
        DS_IS_FLAT_NAME |
            DS_RETURN_DNS_NAME |
            DS_DIRECTORY_SERVICE_REQUIRED,
        &pDcInfo);
    if(dwStatus != 0) 
    {
        return dwStatus;
    }
     
    /* Bind to the domain controller. */
    dwStatus = DsBind(pDcInfo->DomainControllerName, NULL, &hDs);
     
    /* Free the DOMAIN_CONTROLLER_INFO buffer. */
    NetApiBufferFree(pDcInfo);
    if(dwStatus != 0) 
    {
        return dwStatus;
    }
     
    /* Write the SPNs to the service account or computer account. */
    dwStatus = DsWriteAccountSpn(
            hDs,                    /* Handle to the directory. */
            Operation,              /* Add or remove SPN from account's existing SPNs. */
            pszServiceAcctDN,       /* DN of service account or computer account. */
            ulSpn,                  /* Number of SPNs to add. */
            (const TCHAR **)pspn);  /* Array of SPNs. */

    /* Unbind the DS in any case. */
    DsUnBind(&hDs);
     
    return dwStatus;
}

SMPD_BOOL SetupScp()
{
    DWORD dwStatus;

    /* Create the service's Service Connection Point (SCP). */
    dwStatus = ScpCreate(
	SMPD_LISTENER_PORT,
        SMPD_SERVICE_NAME,
        szServiceAccountSAM,  /* SAM name of logon account for ACE */
        szDNofSCP             /* Buffer returns the DN of the SCP */
        );
    if (dwStatus != 0)
    {
	smpd_err_printf("ScpCreate failed: %d\n", dwStatus);
	return SMPD_FALSE;
    }

    /* Compose and register a service principal name for this service. */
    /* If a local account of the format ".\username", skip the SPN. */
    if ( szServiceAccountSAM[0] == '.' )
    {
	smpd_dbg_printf("Do not register SPN for a local account.\n");
	return SMPD_TRUE;
    }

    dwStatus = SpnCompose(
	&pspn,            /* Receives pointer to the SPN array. */
        &ulSpn,           /* Receives number of SPNs returned. */
        szDNofSCP,        /* Input: DN of the SCP. */
        szServiceClass);  /* Input: the service's class string. */

    if (dwStatus == NO_ERROR)
    {
	dwStatus = SpnRegister(
        szServiceAccountDN,  /* Account on which SPNs are registered. */
        pspn,                /* Array of SPNs to register. */
        ulSpn,               /* Number of SPNs in array. */
        DS_SPN_ADD_SPN_OP);  /* Operation code: Add SPNs. */
    }

    if (dwStatus != NO_ERROR)
    {
	_tprintf(TEXT("Failed to compose SPN: Error was %X\n"), dwStatus);
	DeleteService(schService);
	ScpDelete(szDNofSCP, szServiceClass, szServiceAccountDN);
	goto cleanup;
    }
}

DWORD ScpUpdate(USHORT usPort)
{
    DWORD   dwStat, dwType, dwLen;
    BOOL    bUpdate=FALSE;

    HKEY    hReg;

    TCHAR   szAdsPath[MAX_PATH];
    TCHAR   szServer[MAX_PATH];
    TCHAR   szPort[8];
    TCHAR   *pszAttrs[]={
	{TEXT("serviceDNSName")},
	{TEXT("serviceBindingInformation")},
    };

    HRESULT             hr;
    IDirectoryObject    *pObj;
    DWORD               dwAttrs;
    int                 i;

    PADS_ATTR_INFO  pAttribs;
    ADSVALUE        dnsname,binding;

    ADS_ATTR_INFO   Attribs[]={
	{TEXT("serviceDnsName"),ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,&dnsname,1},
	{TEXT("serviceBindingInformation"),ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,&binding,1},
    };

    /* Open the service registry key. */
    dwStat = RegOpenKeyEx(
	HKEY_LOCAL_MACHINE,
	TEXT("Software\\Microsoft\\Windows 2000 Auth-O-Matic"),
	0,
	KEY_QUERY_VALUE,
	&hReg);
    if (dwStat != NO_ERROR) 
    {
	ReportServiceError("RegOpenKeyEx failed", dwStat);
	return dwStat;
    }

    /* Get the GUID binding string used to bind to the service SCP. */
    dwLen = sizeof(szAdsPath);
    dwStat = RegQueryValueEx(hReg, TEXT("GUIDBindingString"), 0, &dwType, 
	(LPBYTE)szAdsPath, &dwLen);
    if (dwStat != NO_ERROR) {
	ReportServiceError("RegQueryValueEx failed", dwStat);
	return dwStat;
    }

    RegCloseKey(hReg);

    /* Bind to the SCP. */
    hr = ADsGetObject(szAdsPath, IID_IDirectoryObject, (void **)&pObj);
    if (FAILED(hr)) 
    {
	char szMsg1[1024];
	sprintf(szMsg1, 
	    "ADsGetObject failed to bind to GUID (bind string: %S): ", 
	    szAdsPath);
	ReportServiceError(szMsg1, hr);
	if(pObj)
	{
	    pObj->Release();
	}
	return dwStat;
    }

    /* Retrieve attributes from the SCP. */
    hr = pObj->GetObjectAttributes(pszAttrs, 2, &pAttribs, &dwAttrs);
    if (FAILED(hr)) {
	ReportServiceError("GetObjectAttributes failed", hr);
	pObj->Release();
	return hr;
    }

    /* Get the current port and DNS name of the host server. */
    _stprintf(szPort,TEXT("%d"),usPort);
    dwLen = sizeof(szServer);
    if (!GetComputerNameEx(ComputerNameDnsFullyQualified,szServer,&dwLen)) 
    {
	pObj->Release();
	return GetLastError();
    }

    /* Compare the current DNS name and port to the values retrieved from
      the SCP. Update the SCP only if nothing has changed. */
    for (i=0; i<(LONG)dwAttrs; i++) 
    {
	if ((_tcscmp(TEXT("serviceDNSName"),pAttribs[i].pszAttrName)==0) &&
	    (pAttribs[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING))
	{
	    if (_tcscmp(szServer,pAttribs[i].pADsValues->CaseIgnoreString) != 0)
	    {
		ReportServiceError("serviceDNSName being updated", 0);
		bUpdate = TRUE;
	    }
	    else
		ReportServiceError("serviceDNSName okay", 0);

	}

	if ((_tcscmp(TEXT("serviceBindingInformation"),pAttribs[i].pszAttrName)==0) &&
	    (pAttribs[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING))
	{
	    if (_tcscmp(szPort,pAttribs[i].pADsValues->CaseIgnoreString) != 0)
	    {
		ReportServiceError("serviceBindingInformation being updated", 0);
		bUpdate = TRUE;
	    }
	    else
		ReportServiceError("serviceBindingInformation okay", 0);
	}
    }

    FreeADsMem(pAttribs);

    /* The binding data or server name have changed, 
      so update the SCP values. */
    if (bUpdate)
    {
	dnsname.dwType              = ADSTYPE_CASE_IGNORE_STRING;
	dnsname.CaseIgnoreString    = szServer;
	binding.dwType              = ADSTYPE_CASE_IGNORE_STRING;
	binding.CaseIgnoreString    = szPort;
	hr = pObj->SetObjectAttributes(Attribs, 2, &dwAttrs);
	if (FAILED(hr)) 
	{
	    ReportServiceError("ScpUpdate: Failed to set SCP values.", hr);
	    pObj->Release();
	    return hr;
	}
    }

    pObj->Release();

    return dwStat;
}

/****************************************************************************
 
   ScpLocate()
 
   All strings returned by ScpLocate must be freed by the caller using 
   FreeADsStr after it is finished using them.
 
 *****************************************************************************/

DWORD ScpLocate (
		 LPWSTR *ppszDN,                  /* Returns distinguished name of SCP. */
		 LPWSTR *ppszServiceDNSName,      /* Returns service DNS name. */
		 LPWSTR *ppszServiceDNSNameType,  /* Returns type of DNS name. */
		 LPWSTR *ppszClass,               /* Returns name of service class. */
		 USHORT *pusPort)                 /* Returns service port. */
{
    HRESULT hr;
    IDirectoryObject *pSCP = NULL;
    ADS_ATTR_INFO *pPropEntries = NULL;
    IDirectorySearch *pSearch = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;

    /* Get an IDirectorySearch pointer for the Global Catalog.  */
    hr = GetGCSearch(&pSearch);
    if (FAILED(hr)) 
    {
	fprintf(stderr,"GetGC failed 0x%x",hr);
	goto Cleanup;
    }

    /* Set up a deep search.
      Thousands of objects are not expected in this example, therefore
      query for 1000 rows per page.*/
    ADS_SEARCHPREF_INFO SearchPref[2];
    DWORD dwPref = sizeof(SearchPref)/sizeof(ADS_SEARCHPREF_INFO);
    SearchPref[0].dwSearchPref =    ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPref[0].vValue.dwType =   ADSTYPE_INTEGER;
    SearchPref[0].vValue.Integer =  ADS_SCOPE_SUBTREE;

    SearchPref[1].dwSearchPref =    ADS_SEARCHPREF_PAGESIZE;
    SearchPref[1].vValue.dwType =   ADSTYPE_INTEGER;
    SearchPref[1].vValue.Integer =  1000;

    hr = pSearch->SetSearchPreference(SearchPref, dwPref);
    fprintf (stderr, "SetSearchPreference: 0x%x\n", hr);
    if (FAILED(hr))
    {
	fprintf (stderr, "Failed to set search prefs: hr:0x%x\n", hr);
	goto Cleanup;
    } 

    /* Execute the search. From the GC get the distinguished name 
      of the SCP. Use the DN to bind to the SCP and get the other 
      properties. */
    LPWSTR rgszDN[] = {L"distinguishedName"};

    /* Search for a match of the product GUID. */
    hr = pSearch->ExecuteSearch(    L"keywords=A762885A-AA44-11d2-81F1-00C04FB9624E",
	rgszDN,
	1,
	&hSearch);

    fprintf (stderr, "ExecuteSearch: 0x%x\n", hr);

    if (FAILED(hr)) 
    {
	fprintf (stderr, "ExecuteSearch failed: hr:0x%x\n", hr);
	goto Cleanup;
    } 

    /* Loop through the results. Each row should be an instance of the 
      service identified by the product GUID.
      Add logic to select from multiple service instances. */
    hr = pSearch->GetNextRow(hSearch);
    if (SUCCEEDED(hr) && hr !=S_ADS_NOMORE_ROWS) 
    {
	ADS_SEARCH_COLUMN Col;

	hr = pSearch->GetColumn(hSearch, L"distinguishedName", &Col);
	*ppszDN = AllocADsStr(Col.pADsValues->CaseIgnoreString);
	pSearch->FreeColumn(&Col);
	hr = pSearch->GetNextRow(hSearch);
    }

    /* Bind to the DN to get the other properties. */
    LPWSTR lpszLDAPPrefix = L"LDAP://";
    DWORD dwSCPPathLength = wcslen(lpszLDAPPrefix) + wcslen(*ppszDN) + 1;
    LPWSTR pwszSCPPath = new WCHAR[dwSCPPathLength];
    if(pwszSCPPath)
    {
	wcscpy(pwszSCPPath, lpszLDAPPrefix);
	wcscat(pwszSCPPath, *ppszDN);
    }       
    else
    {
	fprintf(stderr,"Failed to allocate a buffer");
	goto Cleanup;
    }               

    hr = ADsGetObject(  pwszSCPPath,
	IID_IDirectoryObject,
	(void**)&pSCP);

    /* Free the string buffer */
    delete pwszSCPPath;

    if (SUCCEEDED(hr)) 
    {
	/* Properties to retrieve from the SCP object. */
	LPWSTR rgszAttribs[]=
	{
	    {L"serviceClassName"},
	    {L"serviceDNSName"},
	    {L"serviceDNSNameType"},
	    {L"serviceBindingInformation"}
	};

	DWORD dwAttrs = sizeof(rgszAttribs)/sizeof(LPWSTR);
	DWORD dwNumAttrGot;
	hr = pSCP->GetObjectAttributes( rgszAttribs,
	    dwAttrs,
	    &pPropEntries,
	    &dwNumAttrGot);
	if(FAILED(hr)) 
	{
	    fprintf (stderr, "GetObjectAttributes Failed. hr:0x%x\n", hr);
	    goto Cleanup;
	}

	/* Loop through the entries returned by GetObjectAttributes 
	  and save the values in the appropriate buffers.  */
	for (int i = 0; i < (LONG)dwAttrs; i++) 
	{
	    if ((wcscmp(L"serviceDNSName", pPropEntries[i].pszAttrName)==0) &&
		(pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)) 
	    {
		*ppszServiceDNSName = AllocADsStr(pPropEntries[i].pADsValues->CaseIgnoreString);
	    }

	    if ((wcscmp(L"serviceDNSNameType", pPropEntries[i].pszAttrName)==0) &&
		(pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)) 
	    {
		*ppszServiceDNSNameType = AllocADsStr(pPropEntries[i].pADsValues->CaseIgnoreString);
	    }

	    if ((wcscmp(L"serviceClassName", pPropEntries[i].pszAttrName)==0) &&
		(pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)) 
	    {
		*ppszClass = AllocADsStr(pPropEntries[i].pADsValues->CaseIgnoreString);
	    }

	    if ((wcscmp(L"serviceBindingInformation", pPropEntries[i].pszAttrName)==0) &&
		(pPropEntries[i].dwADsType == ADSTYPE_CASE_IGNORE_STRING)) 
	    {
		*pusPort=(USHORT)_wtoi(pPropEntries[i].pADsValues->CaseIgnoreString);
	    }
	}
    }

Cleanup:
    if (pSCP) 
    {
	pSCP->Release();
	pSCP = NULL;
    }

    if (pPropEntries)
    {
	FreeADsMem(pPropEntries);
	pPropEntries = NULL;
    }

    if (pSearch)
    {
	if (hSearch)
	{
	    pSearch->CloseSearchHandle(hSearch);
	    hSearch = NULL;
	}

	pSearch->Release();
	pSearch = NULL;
    }

    return hr;
}

/****************************************************************************
 
   GetGCSearch()
 
   Retrieves an IDirectorySearch pointer for a Global Catalog (GC)
 
 *****************************************************************************/

HRESULT GetGCSearch(IDirectorySearch **ppDS)
{
    HRESULT hr;
    IEnumVARIANT *pEnum = NULL;
    IADsContainer *pCont = NULL;
    IDispatch *pDisp = NULL;
    VARIANT var;
    ULONG lFetch;

    *ppDS = NULL;

    /* Bind to the GC: namespace container object. The true GC DN 
      is a single immediate child of the GC: namespace, which must 
      be obtained using enumeration. */
    hr = ADsOpenObject( L"GC:",
	NULL,
	NULL,
	ADS_SECURE_AUTHENTICATION, /* Use Secure Authentication. */
	IID_IADsContainer,
	(void**)&pCont);
    if (FAILED(hr)) 
    {
	_tprintf(TEXT("ADsOpenObject failed: 0x%x\n"), hr);
	goto cleanup;
    } 

    /* Get an enumeration interface for the GC container.  */
    hr = ADsBuildEnumerator(pCont, &pEnum);
    if (FAILED(hr)) 
    {
	_tprintf(TEXT("ADsBuildEnumerator failed: 0x%x\n"), hr);
	goto cleanup;
    } 

    /* Now enumerate. There is only one child of the GC: object. */
    hr = ADsEnumerateNext(pEnum, 1, &var, &lFetch);
    if (FAILED(hr)) 
    {
	_tprintf(TEXT("ADsEnumerateNext failed: 0x%x\n"), hr);
	goto cleanup;
    } 

    if ((hr == S_OK) && (lFetch == 1))
    {
	pDisp = V_DISPATCH(&var);
	hr = pDisp->QueryInterface( IID_IDirectorySearch, (void**)ppDS); 
    }

cleanup:
    if (pEnum)
    {
	ADsFreeEnumerator(pEnum);
	pEnum = NULL;
    }

    if (pCont)
    {
	pCont->Release();
	pCont = NULL;
    }

    if (pDisp)
    {
	pDisp->Release();
	pDisp = NULL;
    }

    return hr;
}

#include <atlbase.h>

/*******************************
 
   AllowAccessToScpProperties()
 
 ********************************/

HRESULT AllowAccessToScpProperties(
				   LPWSTR wszAccountSAM,   /* Service account to allow access. */
				   IADs *pSCPObject)       /* IADs pointer to the SCP object. */
{
    HRESULT hr = E_FAIL;
    IADsAccessControlList *pACL = NULL;
    IADsSecurityDescriptor *pSD = NULL;
    IDispatch *pDisp = NULL;
    IADsAccessControlEntry *pACE1 = NULL;
    IADsAccessControlEntry *pACE2 = NULL;
    IDispatch *pDispACE = NULL;
    long lFlags = 0L;
    CComBSTR sbstrTrustee;
    CComBSTR sbstrSecurityDescriptor = L"nTSecurityDescriptor";
    VARIANT varSD;

    if(NULL == pSCPObject)
    {
	return E_INVALIDARG;
    }

    VariantInit(&varSD);

    /*
    If no service account is specified, service runs under 
    LocalSystem. Allow access to the computer account of the 
    service's host.
    */
    if (wszAccountSAM) 
    {
	sbstrTrustee = wszAccountSAM;
    }
    else
    {
	LPWSTR pwszComputerName;
	DWORD dwLen;

	/* Get the size required for the SAM account name. */
	dwLen = 0;
	GetComputerObjectNameW(NameSamCompatible, 
	    NULL, &dwLen);

	pwszComputerName = new WCHAR[dwLen + 1];
	if(NULL == pwszComputerName)
	{
	    hr = E_OUTOFMEMORY;
	    goto cleanup;
	}

	/*
	Get the SAM account name of the computer object for 
	the server.
	*/
	if(!GetComputerObjectNameW(NameSamCompatible,
	    pwszComputerName, &dwLen))
	{
	    delete pwszComputerName;

	    hr = HRESULT_FROM_WIN32(GetLastError());
	    goto cleanup;
	}

	sbstrTrustee = pwszComputerName;
	wprintf(L"GetComputerObjectName: %s\n", pwszComputerName);
	delete pwszComputerName;
    } 

    /* Get the nTSecurityDescriptor. */
    hr = pSCPObject->Get(sbstrSecurityDescriptor, &varSD);
    if (FAILED(hr) || (varSD.vt != VT_DISPATCH)) 
    {
	_tprintf(TEXT("Get nTSecurityDescriptor failed: 0x%x\n"), hr);
	goto cleanup;
    } 

    /*
    Use the V_DISPATCH macro to get the IDispatch pointer from 
    VARIANT structure and QueryInterface for an IADsSecurityDescriptor 
    pointer.
    */
    hr = V_DISPATCH( &varSD )->QueryInterface(
	IID_IADsSecurityDescriptor,
	(void**)&pSD);
    if (FAILED(hr)) 
    {
	_tprintf(
	    TEXT("Cannot get IADsSecurityDescriptor: 0x%x\n"), 
	    hr);
	goto cleanup;
    } 

    /*
    Get an IADsAccessControlList pointer to the security 
    descriptor's DACL.
    */
    hr = pSD->get_DiscretionaryAcl(&pDisp);
    if (SUCCEEDED(hr))
    {
	hr = pDisp->QueryInterface(
	    IID_IADsAccessControlList,
	    (void**)&pACL);
    }
    if (FAILED(hr)) 
    {
	_tprintf(TEXT("Cannot get DACL: 0x%x\n"), hr);
	goto cleanup;
    } 

    /* Create the COM object for the first ACE. */
    hr = CoCreateInstance(CLSID_AccessControlEntry,
	NULL,
	CLSCTX_INPROC_SERVER,
	IID_IADsAccessControlEntry,
	(void **)&pACE1);

    /* Create the COM object for the second ACE. */
    if (SUCCEEDED(hr))
    {
	hr = CoCreateInstance(CLSID_AccessControlEntry,
	    NULL,
	    CLSCTX_INPROC_SERVER,
	    IID_IADsAccessControlEntry,
	    (void **)&pACE2);
    }
    if (FAILED(hr)) 
    {
	_tprintf(TEXT("Cannot create ACEs: 0x%x\n"), hr);
	goto cleanup;
    } 

    /* Set the properties of the two ACEs. */

    /* Allow read and write access to the property. */
    hr = pACE1->put_AccessMask(
	ADS_RIGHT_DS_READ_PROP | ADS_RIGHT_DS_WRITE_PROP);
    hr = pACE2->put_AccessMask( 
	ADS_RIGHT_DS_READ_PROP | ADS_RIGHT_DS_WRITE_PROP);

    /* Set the trustee, which is either the service account or the 
      host computer account. */
    hr = pACE1->put_Trustee( sbstrTrustee );
    hr = pACE2->put_Trustee( sbstrTrustee );

    /* Set the ACE type. */
    hr = pACE1->put_AceType( ADS_ACETYPE_ACCESS_ALLOWED_OBJECT );
    hr = pACE2->put_AceType( ADS_ACETYPE_ACCESS_ALLOWED_OBJECT );

    /* Set AceFlags to zero because ACE is not inheritable. */
    hr = pACE1->put_AceFlags( 0 );
    hr = pACE2->put_AceFlags( 0 );

    /* Set Flags to indicate an ACE that protects a specified object. */
    hr = pACE1->put_Flags( ADS_FLAG_OBJECT_TYPE_PRESENT );
    hr = pACE2->put_Flags( ADS_FLAG_OBJECT_TYPE_PRESENT );

    /* Set ObjectType to the schemaIDGUID of the attribute.
      serviceDNSName */
    hr = pACE1->put_ObjectType( 
	L"{28630eb8-41d5-11d1-a9c1-0000f80367c1}"); 
    /* serviceBindingInformation */
    hr = pACE2->put_ObjectType( 
	L"{b7b1311c-b82e-11d0-afee-0000f80367c1}"); 

    /*
    Add the ACEs to the DACL. Need an IDispatch pointer for 
    each ACE to pass to the AddAce method.
    */
    hr = pACE1->QueryInterface(IID_IDispatch,(void**)&pDispACE);
    if (SUCCEEDED(hr))
    {
	hr = pACL->AddAce(pDispACE);
    }
    if (FAILED(hr)) 
    {
	_tprintf(TEXT("Cannot add first ACE: 0x%x\n"), hr);
	goto cleanup;
    }
    else 
    {
	if (pDispACE)
	    pDispACE->Release();

	pDispACE = NULL;
    }

    /* Repeat for the second ACE. */
    hr = pACE2->QueryInterface(IID_IDispatch, (void**)&pDispACE);
    if (SUCCEEDED(hr))
    {
	hr = pACL->AddAce(pDispACE);
    }
    if (FAILED(hr)) 
    {
	_tprintf(TEXT("Cannot add second ACE: 0x%x\n"), hr);
	goto cleanup;
    }

    /* Write the modified DACL back to the security descriptor. */
    hr = pSD->put_DiscretionaryAcl(pDisp);
    if (SUCCEEDED(hr))
    {
	/*
	Write the ntSecurityDescriptor property to the 
	property cache.
	*/
	hr = pSCPObject->Put(sbstrSecurityDescriptor, varSD);
	if (SUCCEEDED(hr))
	{
	    /* SetInfo updates the SCP object in the directory. */
	    hr = pSCPObject->SetInfo();
	}
    }

cleanup:
    if (pDispACE)
	pDispACE->Release();

    if (pACE1)
	pACE1->Release();

    if (pACE2)
	pACE2->Release();

    if (pACL)
	pACL->Release();

    if (pDisp)
	pDisp->Release();

    if (pSD)
	pSD->Release();

    VariantClear(&varSD);

    return hr;
}

#endif
