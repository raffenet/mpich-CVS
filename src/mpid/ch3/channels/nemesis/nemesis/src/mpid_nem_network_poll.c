#include "mpid_nem.h"
#include "gm_module.h"
#include "tcp_module.h"

/*static int count = 0; */

void
MPID_nem_network_poll (int in_or_out)
{
    switch (MPID_NEM_NET_MODULE)
    {
    case MPID_NEM_GM_MODULE:
        if (in_or_out)
            gm_module_send_poll();
        else
            gm_module_recv_poll();
        break;
    case MPID_NEM_TCP_MODULE:
            tcp_module_poll (in_or_out);
        break;
    default:
        break;
    }
}

/* void */
/* MPID_nem_rel_network_poll( int in_or_out ) */
/* { */
/*     switch (MPID_NEM_NET_MODULE) */
/*     { */
/*     case MPID_NEM_GM_MODULE: */
/*         if (in_or_out) */
/*             gm_module_send_poll(); */
/*         else */
/*             gm_module_recv_poll(); */
/*         break; */
/*     case MPID_NEM_TCP_MODULE: */
/*             tcp_module_poll ( in_or_out ); */
/*         break; */
/*     default: */
/*         break; */
/*     } */
/* } */

