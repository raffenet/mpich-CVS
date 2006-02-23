#include "mpid_nem.h"
#include "mpid_nem_nets.h"
#include <string.h>
#include <errno.h>

#define FNAME_TEMPLATE "/tmp/SHAR_TMPXXXXXX"

int
MPID_nem_mpich2_alloc_win (void **buf, int len, MPID_nem_mpich2_win_t **win)
{
    int ret;
    int fd;
    
    *win = MPIU_Malloc (sizeof (MPID_nem_mpich2_win_t));
    if (!*win)
	goto err0_l;

    strncpy ((*win)->filename, FNAME_TEMPLATE, MPID_NEM_MAX_FNAME_LEN);
    
    fd = mkstemp ((*win)->filename);
    if (fd == -1)
	PERROR_GOTO (err1_l, "mkstmp failed");
    
    ret = ftruncate (fd, len);
    if (ret == -1)
	PERROR_GOTO (err2_l, "ftruncate failed");
    
    *buf = mmap (NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*buf == MAP_FAILED)
	PERROR_GOTO (err2_l, "mmap failed");

    do 
    {
	ret = close (fd);
    }
    while (errno == EINTR);
    if (ret == -1)
	PERROR_GOTO (err3_l, "file close failed");
    
    (*win)->proc = MPID_nem_mem_region.rank;
    (*win)->home_address = *buf;
    (*win)->len = len;
    (*win)->local_address = *buf;

    return MPID_NEM_MPICH2_SUCCESS;

 err3_l:
    ret = munmap (*buf, len);
    if (ret == -1)
	perror ("munmap failed");
 err2_l:
    do 
    {
	ret = close (fd);
    }
    while (errno == EINTR);
    if (ret == -1)
	perror ("close failed");

    ret = unlink ((*win)->filename);
    if (ret == -1)
	perror ("unlink failed");
 err1_l:
    MPIU_Free (*win);
 err0_l:
    return MPID_NEM_MPICH2_FAILURE;
}

int
MPID_nem_mpich2_free_win (MPID_nem_mpich2_win_t *win)
{
    int ret;

    MPIU_Assert (win->proc == MPID_nem_mem_region.rank);
    
    ret = munmap (win->home_address, win->len);
    if (ret == -1)
	PERROR_RET (MPID_NEM_MPICH2_FAILURE, "munmap failed");

    ret = unlink (win->filename);
    if (ret == -1)
	PERROR_RET (MPID_NEM_MPICH2_FAILURE, "unlink failed");

    MPIU_Free (win);

    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_attach_win (void **buf, MPID_nem_mpich2_win_t *remote_win)
{
    int ret;
    int fd;
    
    if (remote_win->proc == MPID_nem_mem_region.rank)
    {
	*buf = remote_win->home_address;
    }
    else
    {
	fd = open (remote_win->filename, O_RDWR);
	if (fd == -1)
	    PERROR_GOTO (err0_l, "open failed");
	
	*buf = mmap (NULL, remote_win->len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (*buf == MAP_FAILED)
	    PERROR_GOTO (err1_l, "mmap failed");

	do
	{
	    ret = close (fd);
	}
	while (errno == EINTR);
	if (ret == -1)
	    PERROR_GOTO (err2_l, "file close failed");
    }
    
    remote_win->local_address = *buf;

    return MPID_NEM_MPICH2_SUCCESS;

 err2_l:
    ret = munmap (*buf, remote_win->len);
    if (ret == -1)
	perror ("munmap failed");
 err1_l:
    do 
    {
	ret = close (fd);
    }
    while (errno == EINTR);
    if (ret == -1)
	perror ("file close failed");
 err0_l:
    return MPID_NEM_MPICH2_FAILURE;
    
}

int
MPID_nem_mpich2_detach_win (MPID_nem_mpich2_win_t *remote_win)
{
    int ret;

    ret = munmap (remote_win->local_address, remote_win->len);
    if (ret == -1)
	PERROR_RET (MPID_NEM_MPICH2_FAILURE, "munmap failed");
    
    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_win_put (void *s_buf, void *d_buf, int len, MPID_nem_mpich2_win_t *remote_win)
{
    char *_d_buf = d_buf;

    _d_buf += (char *)remote_win->local_address - (char *)remote_win->home_address;

    if (_d_buf < (char *)remote_win->local_address || _d_buf + len > (char *)remote_win->local_address + remote_win->len)
	ERROR_RET (MPID_NEM_MPICH2_FAILURE, "win_put out of bounds");

    MPID_NEM_MEMCPY (_d_buf, s_buf, len);
    /*MPID_NEM_MEMCPY (s_buf, _d_buf, len); */
    
    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_win_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich2_win_t *remote_win)
{
    size_t diff;
    int len;
    
    diff = (char *)remote_win->local_address - (char *)remote_win->home_address;

    if ((char *)(*d_iov)->iov_base + diff < (char *)remote_win->local_address ||
	(char *)(*d_iov)->iov_base + diff + (*d_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
	ERROR_RET (MPID_NEM_MPICH2_FAILURE, "win_putv out of bounds");
    while (*s_niov && *d_niov)
    {
	if ((*s_iov)->iov_len > (*d_iov)->iov_len)
	{
	    len = (*d_iov)->iov_len;	    
	    MPID_NEM_MEMCPY ((*d_iov)->iov_base + diff, (*s_iov)->iov_base, len);

	    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
	    (*s_iov)->iov_len =- len;	    

	    ++(*d_iov);
	    --(*d_niov);
	    if ((char *)(*d_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*d_iov)->iov_base + diff + (*d_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
		ERROR_RET (MPID_NEM_MPICH2_FAILURE, "win_putv out of bounds");
	}
	else if ((*s_iov)->iov_len > (*d_iov)->iov_len)
	{
	    len = (*s_iov)->iov_len;	    
	    MPID_NEM_MEMCPY ((*d_iov)->iov_base + diff, (*s_iov)->iov_base, len);

	    ++(*s_iov);
	    --(*s_niov);
	    
	    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
	    (*d_iov)->iov_len =- len;	    
	}
	else
	{
	    len = (*s_iov)->iov_len;	    
	    MPID_NEM_MEMCPY ((*d_iov)->iov_base + diff, (*s_iov)->iov_base, len);

	    ++(*s_iov);
	    --(*s_niov);
	    
	    ++(*d_iov);
	    --(*d_niov);
	    if ((char *)(*d_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*d_iov)->iov_base + diff + (*d_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
		ERROR_RET (MPID_NEM_MPICH2_FAILURE, "win_putv out of bounds");
	}
    }
    
    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_win_get (void *s_buf, void *d_buf, int len, MPID_nem_mpich2_win_t *remote_win)
{
    char *_s_buf = s_buf;
    
    _s_buf += (char *)remote_win->local_address - (char *)remote_win->home_address;

    if (_s_buf < (char *)remote_win->local_address || _s_buf + len > (char *)remote_win->local_address + remote_win->len)
	ERROR_RET (MPID_NEM_MPICH2_FAILURE, "win_get out of bounds");

    MPID_NEM_MEMCPY (d_buf, _s_buf, len);
    
    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_win_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich2_win_t *remote_win)
{
    size_t diff;
    int len;
    
    diff = (char *)remote_win->local_address - (char *)remote_win->home_address;

    if ((char *)(*s_iov)->iov_base + diff < (char *)remote_win->local_address ||
	(char *)(*s_iov)->iov_base + diff + (*s_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
	ERROR_RET (MPID_NEM_MPICH2_FAILURE, "win_getv out of bounds");
    while (*s_niov && *d_niov)
    {
	if ((*d_iov)->iov_len > (*s_iov)->iov_len)
	{
	    len = (*s_iov)->iov_len;	    
	    MPID_NEM_MEMCPY ((*d_iov)->iov_base + diff, (*s_iov)->iov_base, len);

	    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
	    (*d_iov)->iov_len =- len;	    

	    ++(*s_iov);
	    --(*s_niov);
	    if ((char *)(*s_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*s_iov)->iov_base + diff + (*s_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
		ERROR_RET (MPID_NEM_MPICH2_FAILURE, "win_getv out of bounds");
	}
	else if ((*d_iov)->iov_len > (*s_iov)->iov_len)
	{
	    len = (*d_iov)->iov_len;	    
	    MPID_NEM_MEMCPY ((*d_iov)->iov_base + diff, (*s_iov)->iov_base, len);

	    ++(*d_iov);
	    --(*d_niov);
	    
	    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
	    (*s_iov)->iov_len =- len;	    
	}
	else
	{
	    len = (*d_iov)->iov_len;	    
	    MPID_NEM_MEMCPY ((*d_iov)->iov_base + diff, (*s_iov)->iov_base, len);

	    ++(*d_iov);
	    --(*d_niov);
	    
	    ++(*s_iov);
	    --(*s_niov);
	    if ((char *)(*s_iov)->iov_base + diff < (char *)remote_win->local_address ||
		(char *)(*s_iov)->iov_base + diff + (*s_iov)->iov_len > (char *)remote_win->local_address + remote_win->len)
		ERROR_RET (MPID_NEM_MPICH2_FAILURE, "win_getv out of bounds");
	}
    }
    
    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_serialize_win (void *buf, int buf_len, MPID_nem_mpich2_win_t *win, int *len)
{
    if (buf_len < sizeof (MPID_nem_mpich2_win_t))
	goto err_l;
    if (win->proc != MPID_nem_mem_region.rank)
	goto err_l;

    MPID_NEM_MEMCPY (buf, win, sizeof (MPID_nem_mpich2_win_t));
    *len = sizeof (MPID_nem_mpich2_win_t);
    
    return MPID_NEM_MPICH2_SUCCESS;
 err_l:
    return MPID_NEM_MPICH2_FAILURE;
}

int
MPID_nem_mpich2_deserialize_win (void *buf, int buf_len, MPID_nem_mpich2_win_t **win)
{
    if (buf_len < sizeof (MPID_nem_mpich2_win_t))
	goto err_l;

    *win = MPIU_Malloc (sizeof (MPID_nem_mpich2_win_t));
    if (!*win)
	goto err_l;

    MPID_NEM_MEMCPY (*win, buf, sizeof (MPID_nem_mpich2_win_t));
    (*win)->local_address = 0;
    
    return MPID_NEM_MPICH2_SUCCESS;
 err_l:
    return MPID_NEM_MPICH2_FAILURE;
}

int
MPID_nem_mpich2_register_memory (void *buf, int len)
{
    if (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
    {
      //return gm_module_register_mem (buf, len);
    }
    
    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_deregister_memory (void *buf, int len)
{
    if (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
    {
      //return gm_module_deregister_mem (buf, len);
    }

    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_put (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr)
{
    if (MPID_NEM_IS_LOCAL (proc))
    {
	ERROR_RET (MPID_NEM_MPICH2_FAILURE, "not implemented");
    }
    else 
    {
      /*
	switch (MPID_NEM_NET_MODULE)
	{
	case MPID_NEM_GM_MODULE:
	    MPID_NEM_ATOMIC_INC (completion_ctr);
	    return gm_module_put (d_buf, s_buf, len, proc, completion_ctr);
	    break;
	default:
	    ERROR_RET (MPID_NEM_MPICH2_FAILURE, "not implemented");
	    break;
	}
      */
    }
    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		   int *completion_ctr)
{
    int len;
    
    if (MPID_NEM_IS_LOCAL (proc))
    {
	ERROR_RET (MPID_NEM_MPICH2_FAILURE, "not implemented");
    }
    else 
    {
      /*
	switch (MPID_NEM_NET_MODULE)
	{
	case MPID_NEM_GM_MODULE:
	    while (*s_niov && *d_niov)
	    {
		if ((*d_iov)->iov_len > (*s_iov)->iov_len)
		{
		    len = (*s_iov)->iov_len;
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    gm_module_put ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
		    (*d_iov)->iov_len =- len;	    

		    ++(*s_iov);
		    --(*s_niov);
		}
		else if ((*d_iov)->iov_len > (*s_iov)->iov_len)
		{
		    len = (*d_iov)->iov_len;	    
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    gm_module_put ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    ++(*d_iov);
		    --(*d_niov);
		    
		    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
		    (*s_iov)->iov_len =- len;	    
		}
		else
		{
		    len = (*d_iov)->iov_len;	    
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    gm_module_put ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    ++(*d_iov);
		    --(*d_niov);
		    
		    ++(*s_iov);
		    --(*s_niov);
		}
	    }
	    break;
	default:
	    ERROR_RET (MPID_NEM_MPICH2_FAILURE, "not implemented");
	    break;
	}
      */
    }
    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_get (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr)
{
    if (MPID_NEM_IS_LOCAL (proc))
    {
	ERROR_RET (MPID_NEM_MPICH2_FAILURE, "not implemented");
    }
    else 
    {
      /*
      switch (MPID_NEM_NET_MODULE)
	{
	case MPID_NEM_GM_MODULE:
	    MPID_NEM_ATOMIC_INC (completion_ctr);
	    return gm_module_get (d_buf, s_buf, len, proc, completion_ctr);
	    break;
	default:
	    ERROR_RET (MPID_NEM_MPICH2_FAILURE, "not implemented");
	    break;
	}
      */
    }
    return MPID_NEM_MPICH2_SUCCESS;
}

int
MPID_nem_mpich2_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		   int *completion_ctr)
{
    int len;

    if (MPID_NEM_IS_LOCAL (proc))
    {
	ERROR_RET (MPID_NEM_MPICH2_FAILURE, "not implemented");
    }
    else 
    {
      /*
	switch (MPID_NEM_NET_MODULE)
	{
	case MPID_NEM_GM_MODULE:
	    while (*s_niov && *d_niov)
	    {
		if ((*d_iov)->iov_len > (*s_iov)->iov_len)
		{
		    len = (*s_iov)->iov_len;
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    gm_module_get ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    (*d_iov)->iov_base = (char *)(*d_iov)->iov_base + len;
		    (*d_iov)->iov_len =- len;	    

		    ++(*s_iov);
		    --(*s_niov);
		}
		else if ((*d_iov)->iov_len > (*s_iov)->iov_len)
		{
		    len = (*d_iov)->iov_len;	    
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    gm_module_get ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    ++(*d_iov);
		    --(*d_niov);
		    
		    (*s_iov)->iov_base = (char *)(*s_iov)->iov_base + len;
		    (*s_iov)->iov_len =- len;	    
		}
		else
		{
		    len = (*d_iov)->iov_len;	    
		    MPID_NEM_ATOMIC_INC (completion_ctr);
		    gm_module_get ((*d_iov)->iov_base, (*s_iov)->iov_base, len, proc, completion_ctr);

		    ++(*d_iov);
		    --(*d_niov);
		    
		    ++(*s_iov);
		    --(*s_niov);
		}
	    }
	    break;
	default:
	    ERROR_RET (MPID_NEM_MPICH2_FAILURE, "not implemented");
	    break;
	}
      */
    }
    return MPID_NEM_MPICH2_SUCCESS;
}
