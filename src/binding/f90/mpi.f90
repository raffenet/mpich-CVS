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
          INTEGER a,b,c,d,e
        END SUBROUTINE MPI_COMM_NULL_DELETE_FN

        SUBROUTINE MPI_COMM_DUP_FN(a,b,c,d,e,f,g)
         INTEGER a,b,c,d,e,g
         LOGICAL f
        END SUBROUTINE MPI_COMM_DUP_FN

        SUBROUTINE MPI_COMM_NULL_COPY_FN(a,b,c,d,e,f,g)
         INTEGER a,b,c,d,e,g
         LOGICAL f
        END SUBROUTINE MPI_COMM_NULL_COPY_FN

        SUBROUTINE MPI_TYPE_NULL_DELETE_FN(a,b,c,d,e)
          INTEGER a,b,c,d,e
        END SUBROUTINE MPI_TYPE_NULL_DELETE_FN

        SUBROUTINE MPI_TYPE_DUP_FN(a,b,c,d,e,f,g)
         INTEGER a,b,c,d,e,g
         LOGICAL f
        END SUBROUTINE MPI_TYPE_DUP_FN

        SUBROUTINE MPI_TYPE_NULL_COPY_FN(a,b,c,d,e,f,g)
         INTEGER a,b,c,d,e,g
         LOGICAL f
        END SUBROUTINE MPI_TYPE_NULL_COPY_FN

        SUBROUTINE MPI_WIN_NULL_DELETE_FN(a,b,c,d,e)
          INTEGER a,b,c,d,e
        END SUBROUTINE MPI_WIN_NULL_DELETE_FN

        SUBROUTINE MPI_WIN_DUP_FN(a,b,c,d,e,f,g)
         INTEGER a,b,c,d,e,g
         LOGICAL f
        END SUBROUTINE MPI_WIN_DUP_FN

        SUBROUTINE MPI_WIN_NULL_COPY_FN(a,b,c,d,e,f,g)
         INTEGER a,b,c,d,e,g
         LOGICAL f
        END SUBROUTINE MPI_WIN_NULL_COPY_FN

       END INTERFACE
       END MODULE MPI
