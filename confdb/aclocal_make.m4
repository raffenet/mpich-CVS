dnl
dnl We need routines to check that make works.  Possible problems with
dnl make include
dnl
dnl It is really gnumake, and contrary to the documentation on gnumake,
dnl it insists on screaming everytime a directory is changed.  The fix
dnl is to add the argument --no-print-directory to the make
dnl
dnl It is really BSD 4.4 make, and can't handle 'include'.  For some
dnl systems, this can be fatal; there is no fix (other than removing this
dnl alleged make).
dnl
dnl It is the OSF V3 make, and can't handle a comment in a block of target
dnl code.  There is no acceptable fix.
dnl
dnl
dnl
dnl This assumes that "MAKE" holds the name of the make program.  If it
dnl determines that it is an improperly built gnumake, it adds
dnl --no-print-directorytries to the symbol MAKE.
AC_DEFUN(PAC_PROG_MAKE_ECHOS_DIR,[
AC_CACHE_CHECK([whether make echos directory changes],
pac_cv_prog_make_echos_dir,
[
/bin/rm -f conftest
cat > conftest <<.
SHELL=/bin/sh
ALL:
	@(dir=`pwd` ; cd .. ; \$(MAKE) -f \$\$dir/conftest SUB)
SUB:
	@echo "success"
.
str=`$MAKE -f conftest 2>&1`
if test "$str" != "success" ; then
    str=`$MAKE --no-print-directory -f conftest 2>&1`
    if test "$str" = "success" ; then
        MAKE="$MAKE --no-print-directory"
	pac_cv_prog_make_echos_dir="yes using --no-print-directory"
    else
	pac_cv_prog_make_echos_dir="no"
    fi
else
    pac_cv_prog_make_echos_dir="no"
fi
/bin/rm -f conftest
str=""
])
])dnl
dnl
dnl This make does not support "include filename"
dnl (some versions of BSD 4.4 required #include instead of include)
dnl PAC_PROG_MAKE_INCLUDE([true text])
dnl
AC_DEFUN(PAC_PROG_MAKE_INCLUDE,[
AC_CACHE_CHECK([whether make supports include],pac_cv_prog_make_include,[
/bin/rm -f conftest
cat > conftest <<.
ALL:
	@echo "success"
.
cat > conftest1 <<.
include conftest
.
str=`$MAKE -f conftest1 2>&1`
/bin/rm -f conftest conftest1
if test "$str" != "success" ; then
    pac_cv_prog_make_include="no"
    ifelse([$1],,[$1])
else
    pac_cv_prog_make_include="yes"
fi
str=""
])])dnl
dnl
dnl PAC_PROG_MAKE_ALLOWS_COMMENTS([true text])
dnl (some versions of OSF V3 make do not all comments in action commands)
dnl
AC_DEFUN(PAC_PROG_MAKE_ALLOWS_COMMENTS,[
AC_CACHE_CHECK([whether make allows comments in actions],
pac_cv_prog_make_allows_comments,[
/bin/rm -f conftest
cat > conftest <<.
SHELL=/bin/sh
ALL:
	@# This is a valid comment!
	@echo "success"
.
str=`$MAKE -f conftest 2>&1`
/bin/rm -f conftest 
if test "$str" != "success" ; then
    pac_cv_prog_make_allows_comments="no"
    AC_MSG_WARN([Your make does not allow comments in target code.])
    AC_MSG_WARN([Using this make may cause problems when building programs.])
    AC_MSG_WARN([You should consider using gnumake instead.])
    ifelse([$1],,[$1])
else
    pac_cv_prog_make_allows_comments="yes"
fi
str=""
])
])dnl
dnl
dnl Look for a style of VPATH.  Known forms are
dnl VPATH = .:dir
dnl .PATH: . dir
dnl
dnl Defines VPATH or .PATH with . $(srcdir)
dnl Requires that vpath work with implicit targets
dnl NEED TO DO: Check that $< works on explicit targets.
dnl
AC_DEFUN(PAC_PROG_MAKE_VPATH,[
AC_SUBST(VPATH)
AC_CACHE_CHECK([for virtual path format],
pac_prog_make_vpath,[
rm -rf conftest*
mkdir conftestdir
cat >conftestdir/a.c <<EOF
A sample file
EOF
cat > conftest <<EOF
all: a.o
VPATH=.:conftestdir
.c.o:
	@echo \$<
EOF
ac_out=`$MAKE -f conftest 2>&1 | grep 'conftestdir/a.c'`
if test -n "$ac_out" ; then 
    pac_prog_make_vpath="VPATH"
    VPATH='VPATH=.:$(srcdir)'
else
    rm -f conftest
    cat > conftest <<EOF
all: a.o
.PATH: . conftestdir
.c.o:
	@echo \$<
EOF
    ac_out=`$MAKE -f conftest 2>&1 | grep 'conftestdir/a.c'`
    if test -n "$ac_out" ; then 
        pac_prog_make_vpath=".PATH"
        VPATH='.PATH: . $(srcdir)'
    else
	pac_prog_make_vpath="neither VPATH nor .PATH works"
    fi
fi
rm -rf conftest*
])
])dnl
dnl
dnl PAC_PROG_MAKE_SET_CFLAGS([action if true],[action if false])
dnl
AC_DEFUN(PAC_PROG_MAKE_SET_CFLAGS,[
AC_CACHE_CHECK([whether make sets CFLAGS],
pac_cv_prog_make_set_cflags,[
/bin/rm -f conftest
cat > conftest <<.
SHELL=/bin/sh
ALL:
	@echo X[\$]{CFLAGS}X
.
pac_str=`$MAKE -f conftest 2>&1`
/bin/rm -f conftest 
if test "$pac_str" = "XX" ; then
    pac_cv_prog_make_set_cflags="no"
    ifelse([$2],,[$2])
else
    pac_cv_prog_make_set_cflags="yes"
    ifelse([$1],,[$1])
fi
])
])dnl
dnl
dnl
dnl PAC_PROG_MAKE checks for the varieties of MAKE, including support for 
dnl VPATH
dnl
dnl Also sets SET_CFLAGS if make defines CFLAGS for you.
dnl
AC_DEFUN(PAC_PROG_MAKE,[
if test "X$MAKE" = "X" ; then
    AC_CHECK_PROGS(MAKE,make gnumake nmake pmake smake)
fi
PAC_PROG_MAKE_ECHOS_DIR
PAC_PROG_MAKE_INCLUDE
PAC_PROG_MAKE_ALLOWS_COMMENTS
PAC_PROG_MAKE_VPATH
AC_SUBST(SET_CFLAGS)
PAC_PROG_MAKE_SET_CFLAGS([SET_CFLAGS='CFLAGS='])
AC_PROG_MAKE_SET
])
