#ifndef BSOCKETIMPL_H
#define BSOCKETIMPL_H

#include "bsocket.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "mpidimpl.h"
#include "blockallocator.h"

#ifdef HAVE_WINDOWS_SOCKET
#define close closesocket
#define read(a,b,c) recv(a,b,c,0)
#define write(a,b,c) send(a,b,c,0)
#endif

#ifdef __cplusplus
}
#endif

#endif
