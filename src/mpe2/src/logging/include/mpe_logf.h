C
C  (C) 2001 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C
C
C  MPE Logging Return Codes
C
      integer MPE_LOG_OK, MPE_LOG_LOCKED_OUT,
     &        MPE_LOG_NO_MEMORY, MPE_LOG_FILE_PROB,
     &        MPE_LOG_NOT_INITIALIZED, MPE_LOG_PACK_FAIL
      parameter ( MPE_LOG_OK = 0, MPE_LOG_LOCKED_OUT = 1,
     &            MPE_LOG_NO_MEMORY = 2,  MPE_LOG_FILE_PROB = 3,
     &            MPE_LOG_NOT_INITIALIZED = 4, MPE_LOG_PACK_FAIL = 5 )
C
C  MPE Logging Function Prototypes
C
      integer  MPE_Init_log
      external MPE_Init_log
      integer  MPE_Initialized_logging
      external MPE_Initialized_logging
      integer  MPE_Describe_info_state
      external MPE_Describe_info_state
      integer  MPE_Describe_state
      external MPE_Describe_state
      integer  MPE_Describe_info_event
      external MPE_Describe_info_event
      integer  MPE_Describe_event
      external MPE_Describe_event
      integer  MPE_Log_get_event_number
      external MPE_Log_get_event_number
      integer  MPE_Start_log
      external MPE_Start_log
      integer  MPE_Log_send
      external MPE_Log_send
      integer  MPE_Log_receive
      external MPE_Log_receive
      integer  MPE_Log_pack
      external MPE_Log_pack
      integer  MPE_Log_event
      external MPE_Log_event
      integer  MPE_Log_bare_event
      external MPE_Log_bare_event
      integer  MPE_Log_info_event
      external MPE_Log_info_event
      external MPE_Log_sync_clocks
      integer  MPE_Log_sync_clocks
      integer  MPE_Stop_log
      external MPE_Stop_log
      integer  MPE_Finish_log
      external MPE_Finish_log
