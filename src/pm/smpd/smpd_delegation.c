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

#undef FCNAME
#define FCNAME "smpd_register_spn"
int smpd_register_spn(const char *dc, const char *dn, const char *dh)
{
    DWORD len;
    char err_msg[256];
    char **spns;
    HANDLE ds;
    DWORD result;
    char domain_controller[SMPD_MAX_HOST_LENGTH] = "";
    char domain_name[SMPD_MAX_HOST_LENGTH] = "";
    char domain_host[SMPD_MAX_HOST_LENGTH] = "";
    char host[SMPD_MAX_HOST_LENGTH] = "";
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
	smpd_get_hostname(host, SMPD_MAX_HOST_LENGTH);
	if (domain_name[0] != '\0')
	{
	    sprintf(domain_host, "%s\\%s", domain_name, host);
	}
	else
	{
	    strcpy(domain_host, host);
	}
    }

    printf("DsBind(%s, %s, ...)\n", domain_controller[0] == '\0' ? NULL : domain_controller, domain_name[0] == '\0' ? NULL : domain_name);
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
	len = SMPD_MAX_HOST_LENGTH;
	GetComputerObjectName(NameFullyQualifiedDN, domain_host, &len);
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

#undef FCNAME
#define FCNAME "smpd_lookup_spn"
int smpd_lookup_spn(char *target, int length, const char * host, int port)
{
    int result;
    char err_msg[256];
    ULONG len = length/*SMPD_MAX_NAME_LENGTH*/;
    char *env;

    env = getenv("MPICH_SPN");
    if (env)
    {
	if (strlen(env) > 1)
	{
	    strncpy(target, env, SMPD_MAX_NAME_LENGTH);
	}
	else
	{
	    switch (env[0])
	    {
	    case 'p':
		GetUserNameEx(NameUserPrincipal, target, &len);
		break;
	    case 'd':
		GetUserNameEx(NameDnsDomain, target, &len);
		break;
	    case 'n':
		GetUserNameEx(NameSamCompatible, target, &len);
		break;
	    case 'x':
		*target = '\0';
		break;
	    default:
		GetUserName(target, &len);
		break;
	    }
	}
    }
    else
    {
	/*result = DsMakeSpn(SMPD_SERVICE_NAME, SMPD_SERVICE_NAME, NULL, 0, NULL, &len, target);*/
	result = DsMakeSpn(SMPD_SERVICE_NAME, NULL, host, (USHORT)port, NULL, &len, target);
	if (result != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(result, err_msg, 255, NULL);
	    smpd_err_printf("DsMakeSpn failed: %s\n", err_msg);
	    return SMPD_FAIL;
	}
	/*
	char **spns;
	result = DsGetSpn(DS_SPN_DNS_HOST, SMPD_SERVICE_NAME, NULL, port, 1, &host, NULL, &len, &spns);
	if (result != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(result, err_msg, 255, NULL);
	    smpd_err_printf("DsGetSpn failed: %s\n", err_msg);
	    return SMPD_FAIL;
	}
	MPIU_Strncpy(target, spns[0], SMPD_MAX_NAME_LENGTH);
	DsFreeSpnArray(1, spns);
	*/
	/*MPIU_Snprintf(target, SMPD_MAX_NAME_LENGTH, "%s/%s:%d", SMPD_SERVICE_NAME, host, port);*/
	/*GetUserNameEx(NameUserPrincipal, target, &len);*/
	/*GetUserNameEx(NameDnsDomain, target, &len);*/
	/*GetUserNameEx(NameSamCompatible, target, &len);*/
    }
    return SMPD_SUCCESS;
}
