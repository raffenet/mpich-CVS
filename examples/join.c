#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    char str[10];
    int err=0, rank, nprocs;
    MPI_Comm intercomm;
    int listenfd, connfd, clilen, port, namelen, len;
    struct sockaddr_in cliaddr, servaddr;
    struct hostent *h;
    char hostname[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs != 2) {
        printf("Run this program with 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    if (rank == 1) {
        /* server */
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd < 0) {
            printf("server cannot open socket\n");
            MPI_Abort(MPI_COMM_WORLD,1);
        }
        
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = 0;

        err = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
        if (err < 0) {
            printf("bind failed\n");
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        len = sizeof(servaddr);
        err = getsockname(listenfd, (struct sockaddr *) &servaddr, &len);
        if (err < 0) {
            printf("getsockname failed\n");
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        port = ntohs(servaddr.sin_port);
        MPI_Get_processor_name(hostname, &namelen);

        MPI_Send(hostname, namelen+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&port, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);

        err = listen(listenfd, 5);
        if (err < 0) {
            printf("listen failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        clilen = sizeof(cliaddr);

        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
        if (connfd < 0) {
            printf("accept failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    else {
        /* client */

        MPI_Recv(hostname, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 1, 0, 
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&port, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        h = gethostbyname(hostname);
        if (h == NULL) {
            printf("gethostbyname failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        servaddr.sin_family = h->h_addrtype;
        memcpy((char *) &servaddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
        servaddr.sin_port = htons(port);

        /* create socket */
        connfd = socket(AF_INET, SOCK_STREAM, 0);
        if (connfd < 0) {
            printf("client cannot open socket\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        /* connect to server */
        err = connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
        if (err < 0) {
            printf("client cannot connect\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    err = MPI_Comm_join(connfd, &intercomm);
    if (err) printf("Error in MPI_Comm_join\n");

    if (rank == 1) {
        err = MPI_Recv(str, 3, MPI_CHAR, 0, 0, intercomm, MPI_STATUS_IGNORE);
        printf("Server received from client: %s\n", str);
        fflush(stdout);

        err = MPI_Send("bye", 4, MPI_CHAR, 0, 0, intercomm); 
    }
    else{
        err = MPI_Send("hi", 3, MPI_CHAR, 0, 0, intercomm);
        
        err = MPI_Recv(str, 4, MPI_CHAR, 0, 0, intercomm, MPI_STATUS_IGNORE);
        printf("Client received from server: %s\n", str);
        fflush(stdout);
    }

    MPI_Finalize();

    return 0;
}
