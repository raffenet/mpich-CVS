dnl
dnl This is a replacement for AC_PROG_CC that does not prefer gcc and
dnl that does not mess with CFLAGS.  See acspecific.m4 for the original defn.
dnl
dnl/*D
dnl PAC_PROG_CC - Find a working C compiler
dnl
dnl Synopsis:
dnl PAC_PROG_CC
dnl
dnl Output Effect:
dnl   Sets the variable CC if it is not already set
dnl
dnl Notes:
dnl   Unlike AC_PROG_CC, this does not prefer gcc and does not set CFLAGS.
dnl   It does check that the compiler can compile a simple C program.
dnl   It also sets the variable GCC to yes if the compiler is gcc.  It does
dnl   not yet check for some special options needed in particular for 
dnl   parallel computers, such as -Tcray-t3e, or special options to get
dnl   full ANSI/ISO C, such as -Aa for HP.
dnl
dnlD*/
AC_DEFUN(PAC_PROG_CC,[
AC_PROVIDE([AC_PROG_CC])
AC_CHECK_PROGS(CC, cc gcc xlC xlc pgcc )
test -z "$CC" && AC_MSG_ERROR([no acceptable cc found in \$PATH])
AC_PROG_CC_WORKS
AC_PROG_CC_GNU
if test $ac_cv_prog_gcc = yes; then
  GCC=yes
else
  GCC=
fi
])
dnl
dnl/*D
dnl PAC_C_CHECK_COMPILER_OPTION - Check that a compiler option is accepted
dnl without warning messages
dnl
dnl Synopsis:
dnl PAC_C_CHECK_COMPILER_OPTION(optionname,action-if-ok,action-if-fail)
dnl
dnl Output Effects:
dnl
dnl If no actions are specified, a working value is added to 'COPTIONS'
dnl
dnl Notes:
dnl This is now careful to check that the output is different, since 
dnl some compilers are noisy.
dnl 
dnl We are extra careful to prototype the functions in case compiler options
dnl that complain about poor code are in effect.
dnl
dnl Because this is a long script, we have ensured that you can pass a 
dnl variable containing the option name as the first argument.
dnlD*/
AC_DEFUN(PAC_C_CHECK_COMPILER_OPTION,[
AC_MSG_CHECKING([that C compiler accepts option $1])
save_CFLAGS="$CFLAGS"
CFLAGS="$1 $CFLAGS"
rm -f conftest.out
echo 'int try(void);int try(void){return 0;}' > conftest2.c
echo 'int main(void);int main(void){return 0;}' > conftest.c
if ${CC-cc} $save_CFLAGS $CPPFLAGS -o conftest conftest.c >conftest.bas 2>&1 ; then
   if ${CC-cc} $CFLAGS $CPPFLAGS -o conftest conftest.c >conftest.out 2>&1 ; then
      if diff -b conftest.out conftest.bas >/dev/null 2>&1 ; then
         AC_MSG_RESULT(yes)
         AC_MSG_CHECKING([that routines compiled with $1 can be linked with ones compiled  without $1])       
         /bin/rm -f conftest.out
         /bin/rm -f conftest.bas
         if ${CC-cc} -c $save_CFLAGS $CPPFLAGS conftest2.c >conftest2.out 2>&1 ; then
            if ${CC-cc} $CFLAGS $CPPFLAGS -o conftest conftest2.o conftest.c >conftest.bas 2>&1 ; then
               if ${CC-cc} $CFLAGS $CPPFLAGS -o conftest conftest2.o conftest.c >conftest.out 2>&1 ; then
                  if diff -b conftest.out conftest.bas >/dev/null 2>&1 ; then
	             AC_MSG_RESULT(yes)	  
		     CFLAGS="$save_CFLAGS"
                     ifelse($2,,COPTIONS="$COPTIONS $1",$2)
                  elif test -s conftest.out ; then
	             cat conftest.out >&AC_FD_CC
	             AC_MSG_RESULT(no)
                     CFLAGS="$save_CFLAGS"
	             $3
                  else
                     AC_MSG_RESULT(no)
                     CFLAGS="$save_CFLAGS"
	             $3
                  fi  
               else
	          if test -s conftest.out ; then
	             cat conftest.out >&AC_FD_CC
	          fi
                  AC_MSG_RESULT(no)
                  CFLAGS="$save_CFLAGS"
                  $3
               fi
	    else
               # Could not link with the option!
               AC_MSG_RESULT(no)
            fi
         else
            if test -s conftest2.out ; then
               cat conftest.out >&AC_FD_CC
            fi
	    AC_MSG_RESULT(no)
            CFLAGS="$save_CFLAGS"
	    $3
         fi
      else
         cat conftest.out >&AC_FD_CC
         AC_MSG_RESULT(no)
         $3
         CFLAGS="$save_CFLAGS"         
      fi
   else
      AC_MSG_RESULT(no)
      $3
      if test -s conftest.out ; then cat conftest.out >&AC_FD_CC ; fi    
      CFLAGS="$save_CFLAGS"
   fi
else
    # Could not compile without the option!
    AC_MSG_RESULT(no)
fi
rm -f conftest*
])
dnl
dnl/*D
dnl PAC_C_OPTIMIZATION - Determine C options for producing optimized code
dnl
dnl Synopsis
dnl PAC_C_OPTIMIZATION([action if found])
dnl
dnl Output Effect:
dnl Adds options to 'COPTIONS' if no other action is specified
dnl 
dnl Notes:
dnl This is a temporary standin for compiler optimization.
dnl It should try to match known systems to known compilers (checking, of
dnl course), and then falling back to some common defaults.
dnl Note that many compilers will complain about -g and aggressive
dnl optimization.  
dnlD*/
AC_DEFUN(PAC_C_OPTIMIZATION,[
    for copt in "-O4 -Ofast" "-Ofast" "-fast" "-O3" "-xO3" "-O" ; do
        PAC_C_CHECK_COMPILER_OPTION($copt,found_opt=yes,found_opt=no)
        if test $found_opt = "yes" ; then
	    ifelse($1,,COPTIONS="$COPTIONS $copt",$1)
	    break
        fi
    done
])
dnl
dnl/*D
dnl PAC_C_DEPENDS - Determine how to use the C compiler to generate 
dnl dependency information
dnl
dnl Synopsis:
dnl PAC_C_DEPENDS
dnl
dnl Output Effects:
dnl Sets the following shell variables and call AC_SUBST for them:
dnl+ C_DEPEND_OPT - Compiler options needed to create dependencies
dnl. C_DEPEND_OUT - Shell redirection for dependency file (may be empty)
dnl- C_DEPEND_MV - Command to move created dependency file
dnl Also creates a Depends file in the top directory (!).
dnl
dnl Notes:
dnl A typical Make rule that exploits this macro is
dnl.vb
dnl #
dnl # Dependency processing
dnl .SUFFIXES: .dep
dnl DEP_SOURCES = ${SOURCES:.c=.dep}
dnl Depends: ${DEP_SOURCES}
dnl         @-rm -f Depends
dnl         cat *.dep >Depends
dnl .c.dep:
dnl         @${C_COMPILE} @C_DEPEND_OPT@ $< @C_DEPEND_OUT@
dnl         @@C_DEPEND_MV@
dnl
dnl depends-clean:
dnl         @-rm -f *.dep ${srcdir}/*.dep Depends ${srcdir}/Depends
dnl         @-touch Depends
dnl.ve
dnl
dnl For each file 'foo.c', this creates a file 'foo.dep' and creates a file
dnl 'Depends' that contains all of the '*.dep' files.
dnl 
dnlD*/
dnl 
dnl Eventually, we can add an option to the C_DEPEND_MV to strip system
dnl includes, such as /usr/xxxx and /opt/xxxx
dnl
AC_DEFUN(PAC_C_DEPENDS,[
AC_SUBST(C_DEPEND_OPT)
AC_SUBST(C_DEPEND_OUT)
AC_SUBST(C_DEPEND_MV)
rm -f conftest*
cat >conftest.c <<EOF
    #include "confdefs.h"
    int f(void) { return 0; }
EOF
dnl -xM1 is Solaris C compiler (no /usr/include files)
dnl -MM is gcc (no /usr/include files)
dnl -MMD is gcc to .d
dnl .u is xlC (AIX) output
for copt in "-xM1" "-c -xM1" "-xM" "-c -xM" "-MM" "-M" "-c -M"; do
    AC_MSG_CHECKING([whether $copt option generates dependencies])
    rm -f conftest.o conftest.u conftest.d conftest.err
    dnl also need to check that error output is empty
    if $CC $CFLAGS $copt conftest.c >conftest.out 2>conftest.err && \
	test ! -s conftest.err ; then
        dnl Check for dependency info in conftest.out
        if test -s conftest.u ; then 
	    C_DEPEND_OUT=""
	    C_DEPEND_MV='mv $[*].u $[*].dep'
            pac_dep_file=conftest.u 
        elif test -s conftest.d ; then
	    C_DEPEND_OUT=""
	    C_DEPEND_MV='mv $[*].d $[*].dep'
            pac_dep_file=conftest.d 
        else
	    C_DEPEND_OUT='>$[*].dep'
	    C_DEPEND_MV=:
            pac_dep_file=conftest.out
        fi
        if grep 'confdefs.h' $pac_dep_file >/dev/null 2>&1 ; then
            AC_MSG_RESULT(yes)
	    C_DEPEND_OPT="$copt"
	    AC_MSG_CHECKING([whether .o file created with dependency file])
	    if test -s conftest.o ; then
	        AC_MSG_RESULT(yes)
	    else
                AC_MSG_RESULT(no)
            fi
	    break
        else
	    AC_MSG_RESULT(no)
        fi
    else
	cat conftest.out >&AC_FD_CC
	AC_MSG_RESULT(no)
    fi
    copt=""
done
dnl if test "X$copt" = "X" ; then
dnl fi
# Ensure that there is a Depends file
touch Depends
rm -f conftest*
])
dnl
dnl/*D 
dnl PAC_C_PROTOTYPES - Check that the compiler accepts ANSI prototypes.  
dnl
dnl Synopsis:
dnl PAC_C_PROTOTYPES([action if true],[action if false])
dnl
dnlD*/
AC_DEFUN(PAC_C_PROTOTYPES,[
AC_CACHE_CHECK([if $CC supports function prototypes],
pac_cv_c_prototypes,[
AC_TRY_COMPILE([int f(double a){return 0;}],[return 0];,
pac_cv_c_prototypes="yes",pac_cv_c_prototypes="no")])
if test "$pac_cv_c_prototypes" = "yes" ; then
    ifelse([$1],,:,[$1])
else
    ifelse([$2],,:,[$2])
fi
])dnl
dnl
dnl/*D
dnl PAC_FUNC_SEMCTL - Check for semctl and its argument types
dnl
dnl Synopsis:
dnl PAC_FUNC_SEMCTL
dnl
dnl Output Effects:
dnl Sets 'HAVE_SEMCTL' if semctl is available.
dnl Sets 'HAVE_UNION_SEMUN' if 'union semun' is available.
dnl Sets 'SEMCTL_NEEDS_SEMUN' if a 'union semun' type must be passed as the
dnl fourth argument to 'semctl'.
dnlD*/ 
dnl Check for semctl and arguments
AC_DEFUN(PAC_FUNC_SEMCTL,[
AC_CHECK_FUNC(semctl)
if test "$ac_cv_func_semctl" = "yes" ; then
    AC_CACHE_CHECK([for union semun],
    pac_cv_type_union_semun,[
    AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>],[union semun arg;arg.val=0;],
    pac_cv_type_union_semun="yes",pac_cv_type_union_semun="no")])
    if test "$pac_cv_type_union_semun" = "yes" ; then
        AC_DEFINE(HAVE_UNION_SEMUN)
        #
        # See if we can use an int in semctl or if we need the union
        AC_CACHE_CHECK([whether semctl needs union semun],
        pac_cv_func_semctl_needs_semun,[
        AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>],[
int arg = 0; semctl( 1, 1, SETVAL, arg );],
        pac_cv_func_semctl_needs_semun="yes",
        pac_cv_func_semctl_needs_semun="no")
        ])
        if test "$pac_cv_func_semctl_needs_semun" = "yes" ; then
            AC_DEFINE(SEMCTL_NEEDS_SEMUN)
        fi
    fi
fi
])
dnl
dnl/*D
dnl PAC_C_VOLATILE - Check if C supports volatile
dnl
dnl Synopsis:
dnl PAC_C_VOLATILE
dnl
dnl Output Effect:
dnl Defines 'HAS_VOLATILE' if 'volatile int a;' can be compiled.
dnl
dnlD*/
AC_DEFUN(PAC_C_VOLATILE,[
AC_CACHE_CHECK([for volatile],
pac_cv_c_volatile,[
AC_TRY_COMPILE(,[volatile int a;],pac_cv_c_volatile="yes",
pac_cv_c_volatile="no")])
if test "$pac_cv_c_volatile" = "yes" ; then
    AC_DEFINE(HAS_VOLATILE)
fi
])dnl
dnl
dnl/*D
dnl PAC_C_CPP_CONCAT - Check whether the C compiler accepts ISO CPP string
dnl   concatenation
dnl
dnl Synopsis:
dnl PAC_C_CPP_CONCAT([true-action],[false-action])
dnl
dnl Output Effects:
dnl Invokes the true or false action
dnl
dnlD*/
AC_DEFUN(PAC_C_CPP_CONCAT,[
pac_pound="#"
AC_CACHE_CHECK([that the compiler $CC accepts $ac_pound$ac_pound for concatenation in cpp],
pac_cv_c_cpp_concat,[
AC_TRY_COMPILE([
#define concat(a,b) a##b],[int concat(a,b);return ab;],
pac_cv_cpp_concat="yes",pac_cv_cpp_concat="no")])
if test $pac_cv_c_cpp_concat = "yes" ; then
    ifelse([$1],,:,[$1])
else
    ifelse([$2],,:,[$2])
fi
])dnl
dnl
dnl/*D
dnl PAC_FUNC_GETTIMEOFDAY - Check whether gettimeofday takes 1 or 2 arguments
dnl
dnl Synopsis
dnl  PAC_IS_GETTIMEOFDAY_OK(ok_action,failure_action)
dnl
dnl Notes:
dnl One version of Solaris accepted only one argument.
dnl
dnlD*/
AC_DEFUN(PAC_FUNC_GETTIMEOFDAY,[
AC_CACHE_CHECK([whether gettimeofday takes 2 arguments],
pac_cv_func_gettimeofday,[
AC_TRY_COMPILE([#include <sys/time.h>],[struct timeval tp;
gettimeofday(&tp,(void*)0);return 0;],pac_cv_func_gettimeofday="yes",
pac_cv_func_gettimeofday="no")
])
if test "$pac_cv_func_gettimeofday" = "yes" ; then
     ifelse($1,,:,$1)
else
     ifelse($2,,:,$2)
fi
])
dnl
dnl/*D
dnl PAC_C_RESTRICT - Check if C supports restrict
dnl
dnl Synopsis:
dnl PAC_C_RESTRICT
dnl
dnl Output Effect:
dnl Defines 'restrict' if some version of restrict is supported; otherwise
dnl defines 'restrict' as empty.  This allows you to include 'restrict' in 
dnl declarations in the same way that 'AC_C_CONST' allows you to use 'const'
dnl in declarations even when the C compiler does not support 'const'
dnl
dnlD*/
AC_DEFUN(PAC_C_RESTRICT,[
AC_CACHE_CHECK([for restrict],
pac_cv_c_restrict,[
AC_TRY_COMPILE(,[int * restrict a;],pac_cv_c_restrict="restrict",
pac_cv_c_restrict="no")
if test "$pac_cv_c_restrict" = "no" ; then
   AC_TRY_COMPILE(,[int * _Restrict a;],pac_cv_c_restrict="_Restrict",
   pac_cv_c_restrict="no")
fi
if test "$pac_cv_c_restrict" = "no" ; then
   AC_TRY_COMPILE(,[int * __restrict a;],pac_cv_c_restrict="__restrict",
   pac_cv_c_restrict="no")
fi
])
if test "$pac_cv_c_restrict" = "no" ; then
  AC_DEFINE(restrict, )
elif test "$pac_cv_c_restrict" != "restrict" ; then
  AC_DEFINE_UNQUOTED(restrict,$pac_cv_c_restrict)
fi
])dnl
