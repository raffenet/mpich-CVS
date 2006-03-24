! -*- Mode:F90; -*-
!     (C) 2004 by Argonne National Laboratory.
!     See COPYRIGHT in top-level directory.
!
       MODULE MPI
       IMPLICIT NONE
       INCLUDE 'mpifnoext.h'
       INTERFACE
        SUBROUTINE MPI_INIT(IERROR)
        INTEGER IERROR
        END SUBROUTINE MPI_INIT
        
        SUBROUTINE MPI_COMM_NULL_DELETE_FN(a,b,c,d,e)
          USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND
          INTEGER a,b,e
          INTEGER (KIND=MPI_ADDRESS_KIND) c, d
        END SUBROUTINE MPI_COMM_NULL_DELETE_FN

        SUBROUTINE MPI_COMM_DUP_FN(a,b,c,d,e,f,g)
          USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND
          INTEGER a,b,g
          INTEGER (KIND=MPI_ADDRESS_KIND) c,d,e
          LOGICAL f
        END SUBROUTINE MPI_COMM_DUP_FN

        SUBROUTINE MPI_COMM_NULL_COPY_FN(a,b,c,d,e,f,g)
          USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND
          INTEGER a,b,g
          INTEGER (KIND=MPI_ADDRESS_KIND) c,d,e
          LOGICAL f
        END SUBROUTINE MPI_COMM_NULL_COPY_FN

        SUBROUTINE MPI_TYPE_NULL_DELETE_FN(a,b,c,d,e)
          USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND
          INTEGER a,b,e
          INTEGER (KIND=MPI_ADDRESS_KIND) c, d
        END SUBROUTINE MPI_TYPE_NULL_DELETE_FN

        SUBROUTINE MPI_TYPE_DUP_FN(a,b,c,d,e,f,g)
          USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND
          INTEGER a,b,g
          INTEGER (KIND=MPI_ADDRESS_KIND) c,d,e
          LOGICAL f
        END SUBROUTINE MPI_TYPE_DUP_FN

        SUBROUTINE MPI_TYPE_NULL_COPY_FN(a,b,c,d,e,f,g)
          USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND
          INTEGER a,b,g
          INTEGER (KIND=MPI_ADDRESS_KIND) c,d,e
          LOGICAL f
        END SUBROUTINE MPI_TYPE_NULL_COPY_FN

        SUBROUTINE MPI_WIN_NULL_DELETE_FN(a,b,c,d,e)
          USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND
          INTEGER a,b,e
          INTEGER (KIND=MPI_ADDRESS_KIND) c, d
        END SUBROUTINE MPI_WIN_NULL_DELETE_FN

        SUBROUTINE MPI_WIN_DUP_FN(a,b,c,d,e,f,g)
          USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND
          INTEGER a,b,g
          INTEGER (KIND=MPI_ADDRESS_KIND) c,d,e
          LOGICAL f
        END SUBROUTINE MPI_WIN_DUP_FN

        SUBROUTINE MPI_WIN_NULL_COPY_FN(a,b,c,d,e,f,g)
          USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND
          INTEGER a,b,g
          INTEGER (KIND=MPI_ADDRESS_KIND) c,d,e
          LOGICAL f
        END SUBROUTINE MPI_WIN_NULL_COPY_FN

       END INTERFACE
       END MODULE MPI
