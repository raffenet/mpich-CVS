dnl
dnl/*D
dnl AC_CACHE_LOAD - Replacement for autoconf cache load 
dnl
dnl Notes:
dnl Caching in autoconf is broken.  The problem is that the cache is read
dnl without any check for whether it makes any sense to read it.
dnl A common problem is a build on a shared file system; connecting to 
dnl a different computer and then building within the same directory will
dnl lead to at best error messages from configure and at worse a build that
dnl is wrong but fails only at run time (e.g., wrong datatype sizes used).
dnl 
dnl This fixes that by requiring the user to explicitly enable caching 
dnl before the cache file will be loaded.
dnl
dnl To use this version of 'AC_CACHE_LOAD', you need to include
dnl 'aclocal_cache.m4' in your 'aclocal.m4' file.  The sowing 'aclocal.m4'
dnl file includes this file.
dnl
dnl If no --enable-cache or --disable-cache option is selected, the
dnl command causes configure to keep track of the system being configured
dnl in a config.system file; if the current system matches the value stored
dnl in that file (or there is neither a config.cache nor config.system file),
dnl configure will enable caching.  In order to ensure that the configure
dnl tests make sense, the values of CC, F77, F90, and CXX are also included 
dnl in the config.system file.
dnl
dnl See Also:
dnl PAC_ARG_CACHING
dnlD*/
define([AC_CACHE_LOAD],
[if test "X$cache_system" = "X" ; then
    if test "$cache_file" != "/dev/null" ; then
        # Get the directory for the cache file, if any
	changequote(,)
        cache_system=`echo $cache_file | sed -e 's%^\(.*/\)[^/]*%\1/config.system%'`
	changequote([,])
        test "x$cache_system" = "x$cache_file" && cache_system="config.system"
    else
        enable_cache=no
    fi
fi
dnl
dnl The "action-if-not-given" part of AC_ARG_ENABLE is not executed until
dnl after the AC_CACHE_LOAD is executed (!).  Thus, the value of 
dnl enable_cache if neither --enable-cache or --disable-cache is selected
dnl is null.  Just in case autoconf ever fixes this, we test both cases.
if test "X$enable_cache" = "Xnotgiven" -o "X$enable_cache" = "X" ; then
    # check for valid cache file
    if uname -srm >/dev/null 2>&1 ; then
	dnl cleanargs=`echo "$*" | tr '"' ' '`
	cleanargs=`echo "$CC $F77 $CXX $F90" | tr '"' ' '`
        testval="`uname -srm` $cleanargs"
        if test -f "$cache_system" -a -n "$testval" ; then
	    if test "$testval" = "`cat $cache_system`" ; then
	        enable_cache="yes"
	    fi
        elif test ! -f "$cache_system" -a -n "$testval" ; then
	    echo "$testval" > $cache_system
	    # remove the cache file because it may not correspond to our
	    # system
	    rm -f $cache_file
	    enable_cache="yes"
        fi
    fi
fi
if test "X$enable_cache" = "Xyes" -a "$cache_file" = "/dev/null" ; then
    enable_cache=no
fi
if test "X$enable_cache" = "Xyes" ; then
  if test -r "$cache_file" ; then
    echo "loading cache $cache_file"
    . $cache_file
  else
    echo "creating cache $cache_file"
    > $cache_file
    rm -f $cache_system
    cleanargs=`echo "$CC $F77 $CXX" | tr '"' ' '`
    testval="`uname -srm` $cleanargs"
    echo "$testval" > $cache_system
  fi
else
  cache_file="/dev/null"
fi
])
dnl
dnl/*D 
dnl PAC_ARG_CACHING - Enable caching of results from a configure execution
dnl
dnl Synopsis:
dnl PAC_ARG_CACHING
dnl
dnl Output Effects:
dnl Adds '--enable-cache' and '--disable-cache' to the command line arguments
dnl accepted by 'configure'.  
dnl
dnl See Also:
dnl AC_CACHE_LOAD
dnlD*/
dnl Add this call to the other ARG_ENABLE calls.  Note that the values
dnl set here are redundant; the LOAD_CACHE call relies on the way autoconf
dnl initially processes ARG_ENABLE commands.
AC_DEFUN(PAC_ARG_CACHING,[
AC_ARG_ENABLE(cache,
[--enable-cache  - Turn on configure caching],
enable_cache="$enableval",enable_cache="notgiven")
])
dnl
dnl Create a cache file before ac_output so that subdir configures don't
dnl make mistakes. 
dnl We can't use OUTPUT_COMMANDS to remove the cache file, because those
dnl commands are executed *before* the subdir configures.
AC_DEFUN(PAC_SUBDIR_CACHE,[
if test "$cache_file" = "/dev/null" -a "X$enable_cache" = "Xnotgiven" ; then
    cache_file=$$conf.cache
    touch $cache_file
    AC_CACHE_SAVE
    ac_configure_args="$ac_configure_args -enable-cache"
fi
])
AC_DEFUN(PAC_SUBDIR_CACHE_CLEANUP,[
if test "$cache_file" != "/dev/null" -a "X$enable_cache" = "Xnotgiven" ; then
   rm -f $cache_file
fi
])
