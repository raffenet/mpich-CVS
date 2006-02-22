#include "gm.h"
#include "gm_module_impl.h"
#include "mpid_nem.h"
#include "gm_module.h"

#define FREE_LMT_QUEUE_ELEMENTS MPID_NEM_NUM_CELLS

gm_module_lmt_queue_head_t gm_module_lmt_queue;
gm_module_lmt_queue_t *gm_module_lmt_free_queue;

int
gm_module_lmt_init()
{
    int i;
    
    gm_module_lmt_queue.head = NULL;
    gm_module_lmt_queue.tail = NULL;

    gm_module_lmt_free_queue = NULL;
    
    for (i = 0; i < FREE_LMT_QUEUE_ELEMENTS; ++i)
    {
	gm_module_lmt_queue_t *e;
	
	e = MPIU_Malloc (sizeof (gm_module_lmt_queue_t));
	if (!e)
	    ERROR_RET (-1, "malloc failed");
	e->next = gm_module_lmt_free_queue;
	gm_module_lmt_free_queue = e;
    }
    return 0;
}

void
gm_module_lmt_finalize()
{
    gm_module_lmt_queue_t *e;

    while (gm_module_lmt_free_queue)
    {
	e = gm_module_lmt_free_queue;
	gm_module_lmt_free_queue = e->next;
	MPIU_Free (e);
    }

}


static inline int
gm_module_lmt_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *remote_vc, struct iovec *cookie)
{
    int ret = 0;
    int i, j;
    struct iovec *iov_copy;
    
    for (i = 0; i < n_iov; ++i)
    {
	ret = gm_module_register_mem (iov[i].iov_base, iov[i].iov_len);
	if (ret != 0)
	{
	    ret = -1;
	    goto error_exit;
	}
    }
    iov_copy = MPIU_Malloc (sizeof (struct iovec) * n_iov);
    if (iov_copy == 0)
    {
	ret = -1;
	goto error_exit;
    }
    MPID_NEM_MEMCPY (iov_copy, iov, sizeof (struct iovec) * n_iov);
    cookie->iov_base = iov_copy;
    cookie->iov_len = sizeof (struct iovec) * n_iov;

    return ret;
    
 error_exit:
    for (j = i-1; j <= 0; --j)
    {
	gm_module_deregister_mem (iov[j].iov_base, iov[j].iov_len);
    }

    return ret;
}

int
gm_module_lmt_send_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *dest, struct iovec *cookie)
{
    return gm_module_lmt_pre (iov, n_iov, dest, cookie);
}

int
gm_module_lmt_recv_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *src, struct iovec *cookie)
{
    return gm_module_lmt_pre (iov, n_iov, src, cookie);
}

int
gm_module_lmt_start_send (MPIDI_VC_t *dest, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr)
{
    /* We're using gets to transfer the data so, this should not be called */
    return -1;
}

int
gm_module_lmt_start_recv (MPIDI_VC_t *src_vc, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr)
{
    int ret;
    struct iovec *s_iov;
    struct iovec *r_iov;
    int s_n_iov;
    int r_n_iov;
    int s_offset;
    int r_offset;

    s_iov = s_cookie.iov_base;
    s_n_iov = s_cookie.iov_len / sizeof (struct iovec);
    r_iov = r_cookie.iov_base;
    r_n_iov = r_cookie.iov_len / sizeof (struct iovec);
    r_offset = 0;
    s_offset = 0;
    
    ret = gm_module_lmt_do_get (src_vc->ch.node_id, src_vc->ch.port_id, &r_iov, &r_n_iov, &r_offset, &s_iov, &s_n_iov, &s_offset,
				completion_ctr);
    if (ret == LMT_AGAIN)
    {
	gm_module_lmt_queue_t *e = gm_module_queue_alloc (lmt);
	if (!e)
	{
	    printf ("error: malloc failed\n");
	    return -1;
	}
	e->node_id = src_vc->ch.node_id;
	e->port_id = src_vc->ch.port_id;
	e->r_iov = r_iov;
	e->r_n_iov = r_n_iov;
	e->r_offset = r_offset;
	e->s_iov = s_iov;
	e->s_n_iov = s_n_iov;
	e->s_offset = s_offset;
	e->compl_ctr = completion_ctr;
	gm_module_queue_enqueue (lmt, e);
    }
    else if (ret == LMT_FAILURE)
    {
	printf ("error: gm_module_lmt_do_get() failed \n");
	return -1;	
    }
    return 0;
}

static inline int
gm_module_lmt_post (struct iovec cookie)
{
    int ret = 0;
    int i;
    struct iovec *iov;
    int n_iov;

    iov = cookie.iov_base;
    n_iov = cookie.iov_len / sizeof (struct iovec);
    
    for (i = 0; i < n_iov; ++i)
    {
	gm_module_deregister_mem (iov[i].iov_base, iov[i].iov_len);
    }
    
    MPIU_Free (iov);

    return ret;
}

int
gm_module_lmt_send_post (struct iovec cookie)
{
    return gm_module_lmt_post (cookie);
}

int
gm_module_lmt_recv_post (struct iovec cookie)
{
    return gm_module_lmt_post (cookie);
}

static void
get_callback (struct gm_port *p, void *completion_ctr, gm_status_t status)
{
    if (status != GM_SUCCESS)
    {
	gm_perror ("Get error", status);
    }

    ++num_send_tokens;

    MPID_NEM_ATOMIC_DEC ((int *)completion_ctr);
}


int
gm_module_lmt_do_get (int node_id, int port_id, struct iovec **r_iov, int *r_n_iov, int *r_offset, struct iovec **s_iov, int *s_n_iov,
		      int *s_offset, int *compl_ctr)
{
    int s_i, r_i;
    char *s_buf;
    char *r_buf;
    int s_len;
    int r_len;
    int len;

    s_i = 0;
    r_i = 0;
    s_buf = (char *)((*s_iov)[s_i].iov_base) + *s_offset;
    r_buf = (char *)((*r_iov)[r_i].iov_base) + *r_offset;
    s_len = (*s_iov)[s_i].iov_len;
    r_len = (*r_iov)[r_i].iov_len;
    
    while (1)
    {
	if (num_recv_tokens == 0)
	{
	    *s_offset = s_buf - (char *)((*s_iov)[s_i].iov_base);
	    *r_offset = r_buf - (char *)((*r_iov)[r_i].iov_base);
	    *s_iov += s_i;
	    *r_iov += r_i;
	    *s_n_iov -= s_i;
	    *r_n_iov -= r_i;
	    return LMT_AGAIN;
	}

	if (s_len < r_len)
	    len = s_len;
	else
	    len = r_len;
	if (len > 0)
	{
	    MPID_NEM_ATOMIC_INC (compl_ctr);
	    gm_get (port, (long)s_buf, r_buf, len, GM_LOW_PRIORITY, node_id, port_id, get_callback, compl_ctr);
	    
	    --num_send_tokens;
	    
	    s_len -= len;
	    r_len -= len;
	    s_buf += len;
	    r_buf += len;
	}
	
	if (s_len == 0)
	{
	    ++s_i;
	    if (s_i == *s_n_iov)
		break;
	    s_buf = (*s_iov)[s_i].iov_base;
	    s_len = (*s_iov)[s_i].iov_len;
	}
	if (r_len == 0)
	{
	    ++r_i;
	    if (r_i == *r_n_iov)
		break;
	    r_buf = (*r_iov)[r_i].iov_base;
	    r_len = (*r_iov)[r_i].iov_len;
	}   
    }

    if (s_i != *s_n_iov || r_i != *r_n_iov)
    {
	printf ("error: iov mismatch in gm_module_lmt_start_recv\n");
	return LMT_FAILURE;
    }
    return LMT_COMPLETE;
}

