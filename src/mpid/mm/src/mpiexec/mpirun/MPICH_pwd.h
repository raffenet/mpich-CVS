#ifndef MPICH_PWD_H
#define MPICH_PWD_H

#ifdef WSOCK2_BEFORE_WINDOWS
#include <winsock2.h>
#endif
#include <windows.h>

BOOL SetupCryptoClient();
BOOL SavePasswordToRegistry(TCHAR *szAccount, TCHAR *szPassword, bool persistent=true);
BOOL ReadPasswordFromRegistry(TCHAR *szAccount, TCHAR *szPassword);
BOOL DeleteCurrentPasswordRegistryEntry();

#endif
