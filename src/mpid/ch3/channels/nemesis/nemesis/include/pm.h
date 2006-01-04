#ifndef PM_H
#define PM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mpid_nem_memdefs.h"
#include <pmi.h>

#define MAX_HOSTNAME_LEN 256
extern char MPID_nem_hostname[MAX_HOSTNAME_LEN];

extern char *pmi_kvs_name;
extern char *pmi_key;
extern char *pmi_val;
extern int pmi_key_max_sz;
extern int pmi_val_max_sz;

int pm_init (int *size, int *rank);
void pm_finalize(void);


#endif
