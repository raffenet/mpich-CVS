#!/bin/sh
# This is a simple version of mpigdb, which has the restriction that none of the 
# arguments to mpdrun are usable except -n <nprocs> .  In the long run, we expect to 
# replace this with a version of mpiexec that accepts all mpiexec arguments.
nprocs=$2
pgm=$3
shift  # -n
shift  # nprocs
shift  # pgm
exec mpdrun -g -n $nprocs gdb -q $pgm $*
