#ifndef WAIT_THREAD_H
#define WAIT_THREAD_H

#ifdef WSOCK2_BEFORE_WINDOWS
#include <winsock2.h>
#endif
#include <windows.h>

void WaitForLotsOfObjects(int nHandles, HANDLE *pHandle);

#endif
