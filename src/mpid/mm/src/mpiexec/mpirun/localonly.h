#ifndef LOCALONLY_H
#define LOCALONLY_H

#include "MPIRun.h"

void RunLocal(bool bDoSMP);
bool ReadMPDRegistry(char *name, char *value, DWORD *length = NULL);

#endif
