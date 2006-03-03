#include "gm_module_impl.h"
#include "gm.h"

int
MPID_nem_gm_module_test()
{
    return gm_receive_pending (port) || !MPID_nem_queue_empty (module_gm_recv_queue);
}
