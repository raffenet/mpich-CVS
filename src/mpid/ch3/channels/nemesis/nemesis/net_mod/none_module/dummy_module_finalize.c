#include "dummy_module.h"

int
dummy_module_finalize ( void )
{
  return dummy_module_ckpt_shutdown ();    
}

int
dummy_module_ckpt_shutdown ( void )
{
  return 0 ;
}

