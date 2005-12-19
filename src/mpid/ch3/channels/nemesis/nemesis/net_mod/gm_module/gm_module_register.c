#include "gm_module_impl.h"
#include "gm.h"

int
gm_module_register_mem (void *p, int len)
{
    if (gm_register_memory (port, p, len) == GM_SUCCESS)
	return 0;
    else
	return -1;
}

int
gm_module_deregister_mem (void *p, int len)
{
    if (gm_deregister_memory (port, p, len) == GM_SUCCESS)
	return 0;
    else
	return -1;
}


