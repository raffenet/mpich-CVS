#include "mpid_nem.h"
#include "mpid_nem_nets.h"

void
MPID_nem_network_poll (int in_or_out)
{
  net_module_poll(in_or_out);
}

