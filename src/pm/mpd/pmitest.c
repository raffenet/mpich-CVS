/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int n, pmi_rank, pmi_fd;
    char msg_to_send[1024];
    char msg_recvd[1024];
    char kvsname[80];

    pmi_rank = atoi(getenv("PMI_RANK"));
    pmi_fd   = atoi(getenv("PMI_FD"));
    printf("%d: pmi_rank=%d pmi_fd=%d\n",getpid(),pmi_rank,pmi_fd);  fflush(stdout);

    if (argc > 1)
    {
	printf("%d: I must have been spawned with arg1=:%s:\n",getpid(),argv[1]);
	fflush(stdout);
	exit(0);
    }

    strcpy(msg_to_send,"cmd=init\n");
    write(pmi_fd,msg_to_send,strlen(msg_to_send));
    printf("sent init\n"); fflush(stdout);

    strcpy(msg_to_send,"cmd=get_maxes\n");
    write(pmi_fd,msg_to_send,strlen(msg_to_send));
    printf("sent get_maxes\n"); fflush(stdout);
    n = read(pmi_fd,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);  fflush(stdout);

    strcpy(msg_to_send,"cmd=get_my_kvsname\n");
    write(pmi_fd,msg_to_send,strlen(msg_to_send));
    printf("sent get_my_kvsname\n"); fflush(stdout);
    n = read(pmi_fd,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd); fflush(stdout);
    strcpy(kvsname, &msg_recvd[23]); 
    kvsname[strlen(kvsname)-1] = '\0';
    printf("kvsname = :%s:\n", kvsname ); fflush(stdout);

    /* must recv barrier_out before doing gets from the kvs */
    strcpy(msg_to_send,"cmd=barrier_in\n");
    write(pmi_fd,msg_to_send,strlen(msg_to_send));
    printf("sent barrier_in\n"); fflush(stdout);
    n = read(pmi_fd,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd); fflush(stdout);

    sprintf(msg_to_send,"cmd=get_universe_size\n");
    write(pmi_fd,msg_to_send,strlen(msg_to_send));
    printf("sent get for universe size\n"); fflush(stdout);
    n = read(pmi_fd,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd); fflush(stdout);
	
    sprintf(msg_to_send,"cmd=get kvsname=%s key=MPI_APPNUM\n", kvsname);
    write(pmi_fd,msg_to_send,strlen(msg_to_send));
    printf("sent get for appnum (likely 0)\n"); fflush(stdout);
    n = read(pmi_fd,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd); fflush(stdout);
	
    /* interface may have changed
    sprintf(msg_to_send,"cmd=spawn nprocs=%d execname=%s arg=%s\n",
	    1,"/home/rbutler/mpd2/pmitest","spawned");
    write(pmi_fd,msg_to_send,strlen(msg_to_send));
    printf("sent spawn\n"); fflush(stdout);
    */

    /**********
    if (pmi_rank == 0)
	exit(-1);
    printf("GOING INTO INFINITE LOOP\n");
    while (1)
	;
    **********/

    strcpy(msg_to_send,"cmd=finalize\n");
    write(pmi_fd,msg_to_send,strlen(msg_to_send));
}
