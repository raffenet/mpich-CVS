#ifndef MPIDIMPL_H
#define MPIDIMPL_H

#include "mpiimpl.h"

#include "bsocket.h"

/* key used by spawners and spawnees to get the port by which they can connect to each other */
#define MPICH_PARENT_PORT_KEY     "MPI_Parent_port"
/* key used to tell comm_accept that it doesn't need to transfer pmi databases */
#define MPICH_PMI_SAME_DOMAIN_KEY "PMI_SAME_DOMAIN"
/* key used to inform spawned processes that their parent is mpiexec and not another mpi application */
#define MPICH_EXEC_IS_PARENT_KEY  "MPIEXECSpawned"

typedef struct OpenPortNode {
    char port_name[MPI_MAX_PORT_NAME];
    int bfd;
    struct OpenPortNode *next;
} OpenPortNode_t;

typedef struct {
    MPID_Thread_lock_t lock;
    char              pmi_dbname[100];
    MPID_Comm         *comm_parent;
    OpenPortNode_t    *port_list;
} MPID_PerProcess_t;
extern MPID_PerProcess_t MPID_Process;

int MM_Open_port(MPID_Info *, char *);
int MM_Close_port(char *);
int MM_Accept(MPID_Info *, char *);
int MM_Connect(MPID_Info *, char *);
int MM_Send(int, char *, int);
int MM_Recv(int, char *, int);
int MM_Close(int);

#endif
