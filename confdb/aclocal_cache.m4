dnl
dnl/*D
dnl AC_CACHE_LOAD - Replacement for autoconf cache load 
dnl
dnl Notes:
dnl Caching in autoconf is broken (through version 2.13).  The problem is 
dnl that the cache is read
dnl without any check for whether it makes any sense to read it.
dnl A common problem is a build on a shared file system; connecting to 
dnl a different computer and then building within the same directory will
dnl lead to at best error messages from configure and at worse a build that
dnl is wrong but fails only at run time (e.g., wrong datatype sizes used).
dnl Later versions of autoconf do include some checks for changes in the
dnl environment that impact the choices, but still misses problems with
dnl multiple different systems.
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
dnl Bugs:
dnl This does not work with the Cygnus configure because the enable arguments
dnl are processed *after* AC_CACHE_LOAD (!).  To address this, we avoid 
dnl changing the value of enable_cache, and use real_enable_cache, duplicating
dnl the "notgiven" value.
dnl
dnl The environment variable CONFIGURE_DEBUG_CACHE, if set to yes,
dnl will cause additional data to be written out during the configure process.
dnl This can be helpful in debugging the cache file process.dnl
dnl
dnl See Also:
dnl PAC_ARG_CACHING
dnl D*/
define([AC_CACHE_LOAD],
[if test "$CONFIGURE_DEBUG_CACHE" = yes ; then 
    oldopts="$-"
    clearMinusX=no
    set -x 
    if test "$oldopts" != "$-" ; then 
        clearMinusX=yes
    fi
fi 
if test "X$cache_system" = "X" ; then
    # A default file name, just in case
    cache_system="config.system"
    if test "$cache_file" != "/dev/null" ; then
        # Get the directory for the cache file, if any
	changequote(,)
        dnl Be careful to ensure that there is no doubled slash
        cache_system=`echo $cache_file | sed -e 's%^\(.*/\)[^/]*%\1config.system%'`
	changequote([,])
        test "x$cache_system" = "x$cache_file" && cache_system="config.system"
#    else
#        We must *not* set enable_cache to no because we need to know if
#        enable_cache was not set.  
#        enable_cache=no
    fi
fi
dnl
dnl The "action-if-not-given" part of AC_ARG_ENABLE is not executed until
dnl after the AC_CACHE_LOAD is executed (!).  Thus, the value of 
dnl enable_cache if neither --enable-cache or --disable-cache is selected
dnl is null.  Just in case autoconf ever fixes this, we test both cases.
if test -z "$real_enable_cache" ; then
    real_enable_cache=$enable_cache
    if test -z "$real_enable_cache" ; then real_enable_cache="notgiven" ; fi
fi
if test "X$real_enable_cache" = "Xnotgiven" ; then
    # check for valid cache file
    if test -z "$cache_system" ; then cache_system="config.system" ; fi
    if uname -srm >/dev/null 2>&1 ; then
	dnl cleanargs=`echo "$*" | tr '"' ' '`
	cleanargs=`echo "$CC $F77 $CXX $F90" | tr '"' ' '`
        testval="`uname -srm` $cleanargs"
        if test -f "$cache_system" -a -n "$testval" ; then
	    if test "$testval" = "`cat $cache_system`" ; then
	        real_enable_cache="yes"
	    fi
        elif test ! -f "$cache_system" -a -n "$testval" ; then
	    echo "$testval" > $cache_system
	    # remove the cache file because it may not correspond to our
	    # system
	    rm -f $cache_file
	    real_enable_cache="yes"
        fi
    fi
fi
if test "X$real_enable_cache" = "Xyes" -a "$cache_file" = "/dev/null" ; then
    real_enable_cache=no
fi
if test "X$real_enable_cache" = "Xyes" ; then
  if test -r "$cache_file" ; then
    echo "loading cache $cache_file"
    if test -w "$cache_file" ; then
        # Clean the cache file (ergh)
	PAC_CACHE_CLEAN
    fi
    . $cache_file
  else
    echo "Configure in `pwd` creating cache $cache_file"
    > $cache_file
    rm -f $cache_system
    cleanargs=`echo "$CC $F77 $CXX $F90" | tr '"' ' '`
    testval="`uname -srm` $cleanargs"
    echo "$testval" > $cache_system
  fi
else
  cache_file="/dev/null"
fi
if test "$clearMinusX" = yes ; then
    set +x
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
dnl D*/
dnl Add this call to the other ARG_ENABLE calls.  Note that the values
dnl set here are redundant; the LOAD_CACHE call relies on the way autoconf
dnl initially processes ARG_ENABLE commands.
AC_DEFUN(PAC_ARG_CACHING,[
AC_ARG_ENABLE(cache,
[--enable-cache  - Turn on configure caching],
enable_cache="$enableval",enable_cache="notgiven")
])
dnl

dnl Clean the cache of extraneous quotes that AC_CACHE_SAVE may add
AC_DEFUN([PAC_CACHE_CLEAN],[
    rm -f confcache
    sed -e "s/'\\\\''//g" -e "s/'\\\\/'/" -e "s/\\\\'/'/" \
		-e "s/'\\\\''//g" $cache_file > confcache
    if cmp -s $cache_file confcache ; then
        :
    else
        if test -w $cache_file ; then
	    echo "updating cache $cache_file"
            cat confcache > $cache_file
        else
            echo "not updating unwritable cache $cache_file"
        fi
    fi	
    rm -f confcache
    if test "$DEBUG_AUTOCONF_CACHE" = "yes" ; then
        echo "Results of cleaned cache file:"
	echo "--------------------------------------------------------"
	cat $cache_file
	echo "--------------------------------------------------------"
    fi
])

dnl/*D
dnl PAC_SUBDIR_CACHE - Create a cache file before ac_output for subdirectory
dnl configures.
dnl 
dnl Synopsis:
dnl PAC_SUBDIR_CACHE
dnl
dnl Output Effects:
dnl 	
dnl Create a cache file before ac_output so that subdir configures don't
dnl make mistakes. 
dnl We can't use OUTPUT_COMMANDS to remove the cache file, because those
dnl commands are executed *before* the subdir configures.
dnl
dnl D*/
AC_DEFUN(PAC_SUBDIR_CACHE,[
if test "$cache_file" = "/dev/null" -a "X$real_enable_cache" = "Xnotgiven" ; then
    cache_file=$$conf.cache
    touch $cache_file
    dnl 
    dnl For Autoconf 2.52+, we should ensure that the environment is set
    dnl for the cache.
    ac_cv_env_CC_set=set
    ac_cv_env_CC_value=$CC
    ac_cv_env_CFLAGS_set=${CFLAGS+set}
    ac_cv_env_CFLAGS_value=$CFLAGS
    ac_cv_env_CPP_set=set
    ac_cv_env_CPP_value=$CPP
    ac_cv_env_CPPFLAGS_set=${CPPFLAGS+set}
    ac_cv_env_CPPFLAGS_value=$CPPFLAGS
    ac_cv_env_LDFLAGS_set=${LDFLAGS+set}
    ac_cv_env_LDFLAGS_value=$LDFLAGS
    ac_cv_env_LIBS_set=${LIBS+set}
    ac_cv_env_LIBS_value=$LIBS
    ac_cv_env_FC_set=${FS+set}
    ac_cv_env_FC_value=$FC
    ac_cv_env_F77_set=${F77+set}
    ac_cv_env_F77_value=$F77
    ac_cv_env_FFLAGS_set=${FFLAGS+set}
    ac_cv_env_FFLAGS_value=$FFLAGS
    ac_cv_env_CXX_set=${CXX+set}
    ac_cv_env_CXX_value=$CXX

#     dnl For Autoconf 2.57+, we also need to look at these.  This does 
#     dnl disable a sometimes useful check in autoconf, but I don't see
#     dnl a way arount it
#     ac_env_CC_set=set
#     ac_env_CC_value=$CC
#     ac_env_CFLAGS_set=set
#     ac_env_CFLAGS_value=$CFLAGS
#     ac_env_CPP_set=set
#     ac_env_CPP_value=$CPP
#     ac_env_CPPFLAGS_set=set
#     ac_env_CPPFLAGS_value=$CPPFLAGS
#     ac_env_LDFLAGS_set=set
#     ac_env_LDFLAGS_value=$LDFLAGS
#     ac_env_LIBS_set=set
#     ac_env_LIBS_value=$LIBS
#     ac_env_FC_set=set
#     ac_env_FC_value=$FC
#     ac_env_F77_set=set
#     ac_env_F77_value=$F77
#     ac_env_FFLAGS_set=set
#     ac_env_FFLAGS_value=$FFLAGS
#     ac_env_CXX_set=set
#     ac_env_CXX_value=$CXX

#	export ac_env_LDFLAGS_set
#	export ac_env_LDFLAGS_value   
    dnl other parameters are
    dnl build_alias, host_alias, target_alias

    # It turns out that A C CACHE_SAVE can't be invoked more than once
    # with data that contains blanks.  What happens is that the quotes
    # that it adds get quoted and then added again.  To avoid this,
    # we strip off the outer quotes for all cached variables
    AC_CACHE_SAVE
    PAC_CACHE_CLEAN
    ac_configure_args="$ac_configure_args -enable-cache"
fi
dnl Unconditionally export these values.  Subdir configures break otherwise
export CC
export CFLAGS
export LDFLAGS
export LIBS
export CPPFLAGS
export CPP
export FC
export F77
export CXX
export FFLAGS
export CCFLAGS
])
AC_DEFUN(PAC_SUBDIR_CACHE_CLEANUP,[
if test "$cache_file" != "/dev/null" -a "X$real_enable_cache" = "Xnotgiven" ; then
   rm -f $cache_file
fi
])

