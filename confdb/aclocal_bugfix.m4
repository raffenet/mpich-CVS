dnl
dnl Fixes to bugs in AC_xxx macros
dnl 
dnl (AC_TRY_COMPILE is missing a newline after the end in the Fortran
dnl branch; that has been fixed in-place)
dnl
dnl (AC_PROG_CC makes many dubious assumptions.  One is that -O is safe
dnl with -g, even with gcc.  This isn't true; gcc will eliminate dead code
dnl when -O is used, even if you added code explicitly for debugging 
dnl purposes.  -O shouldn't do dead code elimination when -g is selected, 
dnl unless a specific option is selected.  Unfortunately, there is no
dnl documented option to turn off dead code elimination.
dnl
dnl
dnl AC_CONFIG_AUX_DIRS only checks for install-sh, but assumes other
dnl values are present.  Also doesn't provide a way to override the
dnl sources of the various configure scripts.  PAC_CONFIG_AUX_DIRS
dnl overcomes this 
dnl Internal subroutine.
dnl Search for the configuration auxiliary files in directory list $1.
dnl We look only for install-sh, so users of AC_PROG_INSTALL
dnl do not automatically need to distribute the other auxiliary files.
dnl AC_CONFIG_AUX_DIRS(DIR ...)
dnl Also note that since AC_CONFIG_AUX_DIR_DEFAULT calls this, there
dnl isn't a easy way to fix it other than replacing it completely.
dnl This fix applies to 2.13
dnl/*D
dnl AC_CONFIG_AUX_DIRS - Find the directory containing auxillery scripts
dnl for configure
dnl
dnl Synopsis:
dnl AC_CONFIG_AUX_DIRS( [ directories to search ] )
dnl
dnl Output Effect:
dnl Sets 'ac_config_guess' to location of 'config.guess', 'ac_config_sub'
dnl to location of 'config.sub', 'ac_install_sh' to the location of
dnl 'install-sh' or 'install.sh', and 'ac_configure' to the location of a
dnl Cygnus-style 'configure'.  Only 'install-sh' is guaranteed to exist,
dnl since the other scripts are needed only by some special macros.
dnl
dnl The environment variable 'CONFIG_AUX_DIR', if set, overrides the
dnl directories listed.  This is an extension to the 'autoconf' version of
dnl this macro. 
dnlD*/
undefine([AC_CONFIG_AUX_DIRS])
AC_DEFUN(AC_CONFIG_AUX_DIRS,
[if test -f $CONFIG_AUX_DIR/install-sh ; then ac_aux_dir=$CONFIG_AUX_DIR 
else
ac_aux_dir=
for ac_dir in $1; do
  if test -f $ac_dir/install-sh; then
    ac_aux_dir=$ac_dir
    ac_install_sh="$ac_aux_dir/install-sh -c"
    break
  elif test -f $ac_dir/install.sh; then
    ac_aux_dir=$ac_dir
    ac_install_sh="$ac_aux_dir/install.sh -c"
    break
  fi
done
fi
if test -z "$ac_aux_dir"; then
  AC_MSG_ERROR([can not find install-sh or install.sh in $1])
fi
ac_config_guess=$ac_aux_dir/config.guess
ac_config_sub=$ac_aux_dir/config.sub
ac_configure=$ac_aux_dir/configure # This should be Cygnus configure.
AC_PROVIDE([AC_CONFIG_AUX_DIR_DEFAULT])dnl
])

