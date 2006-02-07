#include "mpid_nem.h"
#include "mpid_nem_nets.h"

void
MPID_nem_network_poll (MPID_nem_poll_dir_t in_or_out)
{
  MPID_nem_net_module_poll (in_or_out);
}

