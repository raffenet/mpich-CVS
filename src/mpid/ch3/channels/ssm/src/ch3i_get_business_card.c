/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidi_ch3_impl.h"

#define MAX_NUM_NICS 16

#ifdef HAVE_WINDOWS_H

static int GetLocalIPs(int32_t *pIP, int max)
{
    char hostname[100], **hlist;
    struct hostent *h = NULL;
    int n = 0;

#ifdef HAVE_WINDOWS_H
    {
	DWORD len = 100;
	/*if (!GetComputerName(hostname, &len))*/
	if (!GetComputerNameEx(ComputerNameDnsFullyQualified, hostname, &len))
	{
	    return 0;
	}
    }
#else
    if (gethostname(hostname, 100) == SOCKET_ERROR)
    {
	return 0;
    }
#endif

    h = gethostbyname(hostname);
    if (h == NULL)
    {
	return 0;
    }
    
    hlist = h->h_addr_list;
    while (*hlist != NULL && n<max)
    {
	pIP[n] = *(int32_t*)(*hlist);

	/*{	
	unsigned int a, b, c, d;
	a = ((unsigned char *)(&pIP[n]))[0];
	b = ((unsigned char *)(&pIP[n]))[1];
	c = ((unsigned char *)(&pIP[n]))[2];
	d = ((unsigned char *)(&pIP[n]))[3];
	MPIU_DBG_PRINTF(("ip: %u.%u.%u.%u\n", a, b, c, d));
	}*/

	hlist++;
	n++;
    }
    return n;
}

#else /* HAVE_WINDOWS_H */

#define NUM_IFREQS 10
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_NET_IF_H
#ifdef __STRICT_ANSI__
/* FIXME: THIS IS WRONG.  INSTEAD, SPECIFY THE SPECIFIC FEATURE LEVEL
   NEEDED, AND THEN ONLY IF A CONFIGURE TEST SHOWS THAT IT IS REQUIRED
   THESE NEED TO BE SET FOR ALL COMPILATIONS TO AVOID HAVING DIFFERENT
   FILES COMPILED WITH DIFFERENT AND INCOMPATIBLE HEADER FILES.

   WHAT IS APPARENTLY NEEDED (SEE /usr/include/features.h ON A LINUX 
   SYSTEM) IS EITHER _BSD_SOURCE OR _SVID_SOURCE; THIS MUST BE SET
   CONSISTENTLY FOR ALL FILES COMPILED AS PART OF MPICH2
 */
#define __USE_MISC /* This must be defined to get struct ifreq defined */
#endif
#include <net/if.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

static int GetLocalIPs(int32_t *pIP, int max)
{
    int					fd;
    char *				buf_ptr;
    int					buf_len;
    int					buf_len_prev;
    char *				ptr;
    int n = 0;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
	return 0;

    /*
     * Obtain the interface information from the operating system
     *
     * Note: much of this code is borrowed from W. Richard Stevens' book
     * entitled "UNIX Network Programming", Volume 1, Second Edition.  See
     * section 16.6 for details.
     */
    buf_len = NUM_IFREQS * sizeof(struct ifreq);
    buf_len_prev = 0;

    for(;;)
    {
	struct ifconf			ifconf;
	int				rc;

	buf_ptr = (char *) MPIU_Malloc(buf_len);
	if (buf_ptr == NULL)
	    return 0;
	
	ifconf.ifc_buf = buf_ptr;
	ifconf.ifc_len = buf_len;

	rc = ioctl(fd, SIOCGIFCONF, &ifconf);
	if (rc < 0)
	{
	    if (errno != EINVAL || buf_len_prev != 0)
		return 0;
	}
        else
	{
	    if (ifconf.ifc_len == buf_len_prev)
	    {
		buf_len = ifconf.ifc_len;
		break;
	    }

	    buf_len_prev = ifconf.ifc_len;
	}
	
	MPIU_Free(buf_ptr);
	buf_len += NUM_IFREQS * sizeof(struct ifreq);
    }
	
    /*
     * Now that we've got the interface information, we need to run through
     * the interfaces and save the ip addresses
     */
    ptr = buf_ptr;

    while(ptr < buf_ptr + buf_len)
    {
	struct ifreq *			ifreq;

	ifreq = (struct ifreq *) ptr;
	
	if (ifreq->ifr_addr.sa_family == AF_INET)
	{
	    struct in_addr		addr;

	    addr = ((struct sockaddr_in *) &(ifreq->ifr_addr))->sin_addr;
/*	    
	    if ((addr.s_addr & net_mask_p->s_addr) ==
		(net_addr_p->s_addr & net_mask_p->s_addr))
	    {
		*if_addr_p = addr;
		break;
	    }
*/
	    pIP[n] = addr.s_addr;
	    n++;
	}

	/*
	 *  Increment pointer to the next ifreq; some adjustment may be
	 *  required if the address is an IPv6 address
	 */
	ptr += sizeof(struct ifreq);
	
#	if defined(AF_INET6)
	{
	    if (ifreq->ifr_addr.sa_family == AF_INET6)
	    {
		ptr += sizeof(struct sockaddr_in6) - sizeof(struct sockaddr);
	    }
	}
#	endif
    }

    MPIU_Free(buf_ptr);
    close(fd);

    return n;
}

#endif /* HAVE_WINDOWS_H */

#if 0
int MPIDI_CH3I_Get_business_card(char *value, int length)
{
    int32_t local_ip[MAX_NUM_NICS];
    unsigned int a, b, c, d;
    int num_nics, i;
    char *value_orig;
    struct hostent *h;
    int port;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_GET_BUSINESS_CARD);

    port = MPIDI_CH3I_Listener_get_port();

    /*snprintf(value, length, "%s:%d", host, port);*/

    value_orig = value;
    num_nics = GetLocalIPs(local_ip, MAX_NUM_NICS);
    for (i=0; i<num_nics; i++)
    {
	a = (unsigned char)(((unsigned char *)(&local_ip[i]))[0]);
	b = (unsigned char)(((unsigned char *)(&local_ip[i]))[1]);
	c = (unsigned char)(((unsigned char *)(&local_ip[i]))[2]);
	d = (unsigned char)(((unsigned char *)(&local_ip[i]))[3]);

	if (a != 127)
	{
	    h = gethostbyaddr((const char *)&local_ip[i], sizeof(int), AF_INET);
	    if (h && h->h_name != NULL)
		value += sprintf(value, "%s:%u.%u.%u.%u:%d:", 
				 h->h_name, 
				 a, b, c, d,
				 port);
	    else
		value += sprintf(value, "%u.%u.%u.%u:%u.%u.%u.%u:%d:", 
				 a, b, c, d, 
				 a, b, c, d,
				 port);
	}
    }
    /*MPIU_DBG_PRINTF(("Business card:\n<%s>\n", value_orig));*/

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Get_business_card(char *value, int length)
{
    int mpi_errno;

    mpi_errno = MPIDI_CH3U_Get_business_card_sock(&value, &length);
    if (mpi_errno != MPI_SUCCESS)
    {
        return mpi_errno;
    }

    return MPIDI_CH3U_Get_business_card_sshm(&value, &length);
            
    /* brad : now this is accomplished through upcalls.  prior, this wasn't used anyway but
     *           accomplished within ch3_init.c itself.  now that code is common and uses
     *           the business card methods
     */
    /* brad : eventually, we want to add bizcard functionality to ssm (full cache only in
     *         sock initially)
     */
#if 0    
    int port;
    char host_description[256];
    char host[100];

    mpi_errno = MPIDU_Sock_get_host_description(host_description, 256);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	return mpi_errno;
    }

    port = MPIDI_CH3I_Listener_get_port();

#ifdef HAVE_WINDOWS_H
    {
	DWORD len = 100;
	/*GetComputerName(host, &len);*/
	GetComputerNameEx(ComputerNameDnsFullyQualified, host, &len);
    }
#else
    gethostname(host, 100);
#endif

    mpi_errno = MPIU_Str_add_string_arg(&value, &length, MPIDI_CH3I_HOST_KEY, host);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    mpi_errno = MPIU_Str_add_int_arg(&value, &length, MPIDI_CH3I_PORT_KEY, port);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    mpi_errno = MPIU_Str_add_string_arg(&value, &length, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    return MPI_SUCCESS;
#endif
}
