dnl
dnl/*D 
dnl PAC_LIB_MPI - Check for MPI library
dnl
dnl Synopsis:
dnl PAC_LIB_MPI([action if found],[action if not found])
dnl
dnl Output Effect:
dnl
dnl Notes:
dnl Currently, only checks for lib mpi and mpi.h.  Later, we will add
dnl MPI_Pcontrol prototype (const int or not?).  
dnl
dnl If PAC_ARG_MPICH_BUILDING is included, this will work correctly 
dnl when MPICH is being built.
dnl
dnl Prerequisites:
dnl autoconf version 2.13 (for AC_SEARCH_LIBS)
dnlD*/
dnl Other tests to add:
dnl Version of MPI
dnl MPI-2 I/O?
dnl MPI-2 Spawn?
dnl MPI-2 RMA?
dnl PAC_LIB_MPI([found text],[not found text])
AC_DEFUN(PAC_LIB_MPI,[
AC_PREREQ(2.13)
if test "X$pac_lib_mpi_is_building" != "Xyes" ; then
  # Use CC if TESTCC is defined
  if test "X$pac_save_level" != "X" ; then
     pac_save_TESTCC="${TESTCC}"
     pac_save_TESTCPP="${TESTCPP}"
     CC="$pac_save_CC"
     if test "X$pac_save_CPP" != "X" ; then
         CPP="$pac_save_CPP"
     fi
  fi
  AC_SEARCH_LIBS(MPI_Init,mpi mpich)
  if test "$ac_cv_search_MPI_Init" = "no" ; then
    ifelse($2,,
    AC_MSG_ERROR([Could not find MPI library]),[$2])
  fi
  AC_CHECK_HEADER(mpi.h,pac_have_mpi_h="yes",pac_have_mpi_h="no")
  if test $pac_have_mpi_h = "no" ; then
    ifelse($2,,
    AC_MSG_ERROR([Could not find mpi.h include file]),[$2])
  fi
  if test "X$pac_save_level" != "X" ; then
     CC="$pac_save_TESTCC"
     CPP="$pac_save_TESTCPP"
  fi
fi
ifelse($1,,,[$1])
])
dnl
dnl
dnl/*D
dnl PAC_ARG_MPICH_BUILDING - Add configure command-line argument to indicated
dnl that MPICH is being built
dnl
dnl Output Effect:
dnl Adds the command-line switch '--with-mpichbuilding' that may be used to
dnl indicate that MPICH is building.  This allows a configure to work-around
dnl the fact that during a build of MPICH, certain commands, particularly the
dnl compilation commands such as 'mpicc', are not yet functional.
dnl
dnl See Also:
dnl PAC_LIB_MPI
dnlD*/
AC_DEFUN(PAC_ARG_MPICH_BUILDING,[
AC_ARG_WITH(mpichbuilding,
[--with-mpichbuilding - Assume that MPICH is being built],
pac_lib_mpi_is_building=$withval,pac_lib_mpi_is_building="no")
])
dnl
dnl
dnl This should also set MPIRUN.
dnl
dnl/*D
dnl PAC_ARG_MPI_TYPES - Add command-line switches for different MPI 
dnl environments
dnl
dnl Synopsis:
dnl PAC_ARG_MPI_TYPES
dnl
dnl Output Effects:
dnl Adds the following command line options to configure
dnl+ \-\-with\-mpich[=path] - MPICH.  'path' is the location of MPICH commands
dnl. \-\-with\-ibmmpi - IBM MPI
dnl- \-\-with\-sgimpi - SGI MPI
dnl
dnl Sets 'CC', 'F77', 'TESTCC', and 'TESTF77'.
dnl
dnl See also:
dnl PAC_LANG_PUSH_COMPILERS, PAC_LIB_MPI
dnlD*/
AC_DEFUN(PAC_ARG_MPI_TYPES,[
AC_ARG_WITH(mpich,
[--with-mpich=path  - Assume that we are building with MPICH],[
save_PATH="$PATH"
if test "$withval" != "yes" -a "$withval" != "no" ; then 
    PATH=$withval:${PATH}
fi
dnl 
dnl This isn't correct.  It should try to get the underlying compiler
dnl from the mpicc and mpif77 scripts or mpireconfig
AC_PATH_PROG(MPICC,mpicc)
TESTCC=${CC-cc}
CC="$MPICC"
AC_PATH_PROG(MPIF77,mpif77)
TESTF77=${F77-f77}
F77="$MPIF77"
PATH="$save_PATH"
])
AC_ARG_WITH(ibmmpi,
[--with-ibmmpi    - Use the IBM SP implementation of MPI],
TESTCC=${CC-xlC}; TESTF77=${F77-xlf}; CC=mpcc; F77=mpxlf)
AC_ARG_WITH(sgimpi,
[--with-sgimpi    - Use the SGI implementation of MPI],
TESTCC=${CC:=cc}; TESTF77=${F77:=f77})
])
