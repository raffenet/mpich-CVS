#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int n, pmi_rank, pmi_fd;
    char msg_to_send[1024];
    char msg_recvd[1024];

    pmi_rank = atoi(getenv("PMI_RANK"));
    pmi_fd   = atoi(getenv("PMI_FD"));
    printf("pmi_rank=%d pmi_fd=%d\n",pmi_rank,pmi_fd);
    strcpy(msg_to_send,"cmd=get_maxes\n");
    write(pmi_fd,msg_to_send,strlen(msg_to_send));
    n = read(pmi_fd,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("recvd msg=:%s:\n",msg_recvd);

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
