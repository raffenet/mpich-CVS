dnl
dnl C++ macros
dnl
dnl Many compilers generate directories that hold information while compiling
dnl and using templates.  This definition attempts to find their names (so
dnl that they can be cleaned later).
dnl
dnl This was inspired by similar code in the MPI-C++ distribution from
dnl the University of Notre Dame.
dnl PAC_PROG_CXX_TEMPLATE_DIR(dirname)
dnl sets dirname to the name of the template directory, or to empty
dnl if it can not be determined.
AC_DEFUN(PAC_PROG_CXX_TEMPLATE_DIR,[
AC_CACHE_CHECK([for C++ template repository directory name],
pac_cv_cxx_template_dir,[
mkdir conftest_dir
cd conftest_dir
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
cat > conftest_1.h <<EOF
template <class T>
class foo {
public:
   foo( T in ) : data( in ) { in.member(1); };
   void member( int i );
private:
   T data;
};
class bar {
public:
   bar( int i ) { data = i; };
   void member( int i ) { data = data + i; };
private:
   int data;
};
EOF
cat > conftest_1.${ac_ext} <<EOF
#include "conftest_1.h"

void my_function( void )
{
   foo<bar> v1(6);
   foo< foo<bar> > v2(v1);
}
EOF
cat > conftest.${ac_ext} <<EOF
#include "conftest_1.h"

void my_function( void );

template <class T>
void foo<T>::member(int i)
{
 i++;
}
int main( int argc, char *argv[] )
{
    foo<bar> v1(6);
    foo< foo<bar> > v2(v1);
    my_function();
    return 0;
}
EOF
#
ac_compile_special='${CXX-g++} -c $CXXFLAGS $CPPFLAGS conftest_1.$ac_ext 1>&AC_FD_CC'
if AC_TRY_EVAL(ac_compile_special) ; then
    if AC_TRY_EVAL(ac_compile) ; then
	# Look for a new directory
	for file in `ls` ; do
	    if test -d "$file" -a "$file" != "." -a "$file" != ".." ; then
	         pac_cv_cxx_template_dir="$pac_cv_cxx_template_dir $file"
	    fi
        done
	pac_cv_cxx_template_dir_name="$pac_cv_cxx_template_dir"
	if test -z "$pac_cv_cxx_template_dir" ; then
	    pac_cv_cxx_template_dir="could not determine"
	fi
    else
        echo "configure: failed program was:" >&AC_FD_CC
        cat conftest.$ac_ext >&AC_FD_CC
	pac_cv_cxx_template_dir="could not determine"
    fi
else
    echo "configure: failed program was:" >&AC_FD_CC
    cat conftest_1.$ac_ext >&AC_FD_CC
    pac_cv_cxx_template_dir="templates not supported"
fi
cd ..
#rm -rf conftest_dir
AC_LANG_RESTORE
])
$1="$pac_cv_cxx_template_dir_name"
])
#
# This is a replacement that checks that FAILURES are signaled as well
# (later configure macros look for the .o file, not just success from the
# compiler, but they should not HAVE to
#
AC_DEFUN(PAC_PROG_CXX_WORKS,
[AC_PROG_CXX_WORKS
AC_MSG_CHECKING([whether the C++ compiler sets its return status correctly])
AC_LANG_SAVE
AC_LANG_CXX
AC_TRY_COMPILE(,[int a = bzzzt;],notbroken=no,notbroken=yes)
AC_MSG_RESULT($notbroken)
if test "$notbroken" = "no" ; then
    AC_MSG_ERROR([installation or configuration problem: C++ compiler does not
correctly set error code when a fatal error occurs])
fi
])
