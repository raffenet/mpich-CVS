/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidu_sock.h"

int MPIDU_Sock_init()
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_finalize()
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_get_host_description(char * host_description, int len)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_create_set(MPIDU_Sock_set_t * set)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_destroy_set(MPIDU_Sock_set_t set)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_native_to_sock(MPIDU_Sock_set_t set, MPIDU_SOCK_NATIVE_FD fd, void *user_ptr, MPIDU_Sock_t *sock_ptr)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_listen(MPIDU_Sock_set_t set, void * user_ptr, int * port, MPIDU_Sock_t * sock)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_accept(MPIDU_Sock_t listener_sock, MPIDU_Sock_set_t set, void * user_ptr, MPIDU_Sock_t * sock)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_post_connect(MPIDU_Sock_set_t set, void * user_ptr, char * host_description, int port, MPIDU_Sock_t * sock)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_set_user_ptr(MPIDU_Sock_t sock, void * user_ptr)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_post_close(MPIDU_Sock_t sock)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_post_read(MPIDU_Sock_t sock, void * buf, MPIDU_Sock_size_t minbr, MPIDU_Sock_size_t maxbr,
                         MPIDU_Sock_progress_update_func_t fn)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_post_readv(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_post_write(MPIDU_Sock_t sock, void * buf, MPIDU_Sock_size_t min, MPIDU_Sock_size_t max,
			  MPIDU_Sock_progress_update_func_t fn)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_post_writev(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_wait(MPIDU_Sock_set_t set, int timeout, MPIDU_Sock_event_t * event)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_wakeup(MPIDU_Sock_set_t set)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_read(MPIDU_Sock_t sock, void * buf, MPIDU_Sock_size_t len, MPIDU_Sock_size_t * num_read)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_readv(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_size_t * num_read)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_write(MPIDU_Sock_t sock, void * buf, MPIDU_Sock_size_t len, MPIDU_Sock_size_t * num_written)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_writev(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_size_t * num_written)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_getid(MPIDU_Sock_t sock)
{
    return MPI_SUCCESS;
}

int MPIDU_Sock_getsetid(MPIDU_Sock_set_t set)
{
    return MPI_SUCCESS;
}

/*
int test_string_functions()
{
    char val[1024], val2[1024];
    char buffer[1024];
    char recursed[1024];
    int bin_buf[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int bin_dest[10];
    int length, age, i;
    char *iter;

    iter = buffer;
    length = 1024;
    MPIU_Str_add_string_arg(&iter, &length, "name", "David Ashton");
    MPIU_Str_add_int_arg(&iter, &length, "age", 31);
    MPIU_Str_add_binary_arg(&iter, &length, "foo", bin_buf, 40);

    printf("encoded string: <%s>\n", buffer);

    MPIU_Str_get_string_arg(buffer, "name", val, 1024);
    printf("got name = '%s'\n", val);
    MPIU_Str_get_int_arg(buffer, "age", &age);
    printf("got age = %d\n", age);
    MPIU_Str_get_binary_arg(buffer, "foo", bin_dest, 40);
    printf("got binary data 'foo'\n");
    for (i=0; i<10; i++)
	printf(" bin[0] = %d\n", bin_dest[i]);

    iter = recursed;
    length = 1024;
    MPIU_Str_add_string_arg(&iter, &length, "name", "foobar");
    MPIU_Str_add_string_arg(&iter, &length, "buscard", buffer);
    MPIU_Str_add_string_arg(&iter, &length, "reason", "Because I say so");

    printf("recursive encoded string: <%s>\n", recursed);

    MPIU_Str_get_string_arg(recursed, "name", val, 1024);
    printf("got name = '%s'\n", val);
    MPIU_Str_get_string_arg(recursed, "buscard", val, 1024);
    printf("got buscard = '%s'\n", val);
    MPIU_Str_get_string_arg(val, "name", val2, 1024);
    printf("got name from buscard = '%s'\n", val2);
    MPIU_Str_get_string_arg(recursed, "reason", val, 1024);
    printf("got reason = '%s'\n", val);

    MPIU_Str_hide_string_arg(recursed, "reason");
    printf("hidden recursed string: <%s>\n", recursed);

    iter = buffer;
    iter += MPIU_Str_add_string(iter, 1024, "foo=\"");
    iter += MPIU_Str_add_string(iter, 1024, "=");
    iter += MPIU_Str_add_string(iter, 1024, "Wally World");
    printf("buffer = <%s>\n", buffer);
    iter = MPIU_Str_get_string(buffer, val, 1024, &i);
    printf("one = '%s'\n", val);
    iter = MPIU_Str_get_string(iter, val, 1024, &i);
    printf("two = '%s'\n", val);
    iter = MPIU_Str_get_string(iter, val, 1024, &i);
    printf("three = '%s'\n",val);

    return 0;
}
*/
