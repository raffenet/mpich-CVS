#include "mpid_nem_debug.h"
#include "mpid_nem.h"
#include "mpiimpl.h"

void
MPID_nem_dbg_dump_cell (volatile struct MPID_nem_cell *cell)
{
    MPIU_DBG_MSG_D (ALL, TERSE, "    source = %d", cell->pkt.mpich2.source);
    MPIU_DBG_MSG_D (ALL, TERSE, "    dest = %d", cell->pkt.mpich2.dest);
    MPIU_DBG_MSG_D (ALL, TERSE, "    datalen = %d", cell->pkt.mpich2.datalen);
    MPIU_DBG_MSG_D (ALL, TERSE, "    seqno = %d", cell->pkt.mpich2.seqno);
    MPIU_DBG_MSG_D (ALL, TERSE, "    type = %d", cell->pkt.mpich2.type);
}

