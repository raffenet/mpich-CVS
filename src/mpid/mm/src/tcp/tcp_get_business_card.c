/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#define MAX_NUM_NICS 10

typedef struct myhostent
{
    char *h_name;
    char **h_aliases;
    int h_addrtype;
    int h_length;
    char **h_addr_list;
} myhostent;

static int GetLocalIPs(unsigned int *pIP, int max)
{
    char hostname[100], **hlist;
    myhostent *h = NULL;
    int n = 0;
    
    if (gethostname(hostname, 100) == SOCKET_ERROR)
    {
	return 0;
    }
    
    h = (struct myhostent *)gethostbyname(hostname);
    if (h == NULL)
    {
	return 0;
    }
    
    hlist = h->h_addr_list;
    while (*hlist != NULL && n<max)
    {
	pIP[n] = *(unsigned int*)(*hlist);

	/*
	unsigned int a, b, c, d;
	a = ((unsigned char *)(&pIP[n]))[0];
	b = ((unsigned char *)(&pIP[n]))[1];
	c = ((unsigned char *)(&pIP[n]))[2];
	d = ((unsigned char *)(&pIP[n]))[3];
	printf("ip: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);
	*/

	hlist++;
	n++;
    }
    return n;
}

int tcp_get_business_card(char *value, int length)
{
    unsigned int local_ip[MAX_NUM_NICS];
    unsigned int a, b, c, d;
    int num_nics, i;
    char *value_orig;
    MPIDI_STATE_DECL(MPID_STATE_TCP_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_GET_BUSINESS_CARD);

    /*snprintf(value, length, "%s:%d", TCP_Process.host, TCP_Process.port);*/

    value_orig = value;
    num_nics = GetLocalIPs(local_ip, MAX_NUM_NICS);
    for (i=0; i<num_nics; i++)
    {
	a = (unsigned char)(((unsigned char *)(&local_ip[i]))[0]);
	b = (unsigned char)(((unsigned char *)(&local_ip[i]))[1]);
	c = (unsigned char)(((unsigned char *)(&local_ip[i]))[2]);
	d = (unsigned char)(((unsigned char *)(&local_ip[i]))[3]);

	value += sprintf(value, "%u.%u.%u.%u:%d:", a, b, c, d, TCP_Process.port);
    }
    printf("tcp business card:\n%s", value_orig);fflush(stdout);

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}
