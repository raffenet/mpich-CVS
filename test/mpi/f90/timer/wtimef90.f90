        program main
          use mpi
          double precision time1

          time1 = mpi_wtime()
          time1 = mpi_wtick()
          time1 = pmpi_wtime()
          time1 = pmpi_wtick()

          print *, ' No Errors'
        end
