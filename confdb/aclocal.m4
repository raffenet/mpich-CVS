dnl
dnl This version of aclocal.m4 simply includes all of the individual
dnl components
builtin(include,aclocal_am.m4)
builtin(include,aclocal_bugfix.m4)
builtin(include,aclocal_cache.m4)
builtin(include,aclocal_cc.m4)
builtin(include,aclocal_cross.m4)
builtin(include,aclocal_cxx.m4)
builtin(include,aclocal_f77.m4)
builtin(include,aclocal_f90.m4)
builtin(include,aclocal_make.m4)
builtin(include,aclocal_mpi.m4)
builtin(include,aclocal_web.m4)
builtin(include,aclocal_shl.m4)
dnl builtin(include,aclocal_tcl.m4)
builtin(include,aclocal_java.m4)

dnl PAC_CONFIG_SUBDIRS_IMMEDIATE(DIR ...)
dnl Perform the configuration *now*
dnl 
dnl There is a bug in AC_OUTPUT_SUBDIRS that is tickled by this
dnl code.  There is no step to create any of the intermediate
dnl directories in the case that this is a vpath build.  
AC_DEFUN(PAC_CONFIG_SUBDIRS_IMMEDIATE,
[AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT])dnl
SAVE_subdirs="$subdirs"
subdirs="$1"
#
# Build any intermediate directories
for dir in $1 ; do
    saveIFS="$IFS"
    IFS="/"
    curdir=""
	set -x 
    for subdir in $dir ; do
	curdir="${curdir}$subdir"
	if test ! -d "$curdir" ; then mkdir "$curdir" ; fi
        curdir="${curdir}/"
    done
    set x
    IFS="$saveIFS"
done
AC_OUTPUT_SUBDIRS($1)
subdirs="$SAVE_subdirs"
])

