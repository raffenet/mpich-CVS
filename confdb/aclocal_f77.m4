dnl
dnl/*D
dnl PAC_PROG_F77_NAME_MANGLE - Determine how the Fortran compiler mangles
dnl names 
dnl
dnl Synopsis:
dnl PAC_PROG_F77_NAME_MANGLE([action])
dnl
dnl Output Effect:
dnl If no action is specified, one of the following names is defined:
dnl.vb
dnl If fortran names are mapped:
dnl   lower -> lower                  F77_NAME_LOWER
dnl   lower -> lower_                 F77_NAME_LOWER_USCORE
dnl   lower -> UPPER                  F77_NAME_UPPER
dnl   lower_lower -> lower__          F77_NAME_LOWER_2USCORE
dnl   mixed -> mixed                  F77_NAME_MIXED
dnl   mixed -> mixed_                 F77_NAME_MIXED_USCORE
dnl.ve
dnl If an action is specified, it is executed instead.
dnl 
dnl Notes:
dnl We assume that if lower -> lower (any underscore), upper -> upper with the
dnl same underscore behavior.  Previous versions did this by 
dnl compiling a Fortran program and running strings -a over it.  Depending on 
dnl strings is a bad idea, so instead we try compiling and linking with a 
dnl C program, since that is why we are doing this anyway.  A similar approach
dnl is used by FFTW, though without some of the cases we check (specifically, 
dnl mixed name mangling)
dnl
dnlD*/
dnl
AC_DEFUN(PAC_PROG_F77_NAME_MANGLE,[
AC_CACHE_CHECK([for Fortran 77 name mangling],
pac_cv_prog_f77_name_mangle,
[
   # Check for strange behavior of Fortran.  For example, some FreeBSD
   # systems use f2c to implement f77, and the version of f2c that they 
   # use generates TWO (!!!) trailing underscores
   # Currently, WDEF is not used but could be...
   #
   # Eventually, we want to be able to override the choices here and
   # force a particular form.  This is particularly useful in systems
   # where a Fortran compiler option is used to force a particular
   # external name format (rs6000 xlf, for example).
   rm -f conftest*
   cat > conftest.f <<EOF
       subroutine MY_name( a )
       return
       end
EOF
   # This is the ac_compile line used if LANG_FORTRAN77 is selected
   if test "X$ac_fcompile" = "X" ; then
       ac_fcompile='${F77-f77} -c $FFLAGS conftest.f 1>&AC_FD_CC'
   fi
   if AC_TRY_EVAL(ac_fcompile) && test -s conftest.o ; then
	mv conftest.o fconftestf.o
   else 
	echo "configure: failed program was:" >&AC_FD_CC
        cat conftest.f >&AC_FD_CC
   fi

   AC_LANG_SAVE
   AC_LANG_C   
   save_LIBS="$LIBS"
   LIBS="fconftestf.o $LIBS"
   AC_TRY_LINK(,my_name();,pac_cv_prog_f77_name_mangle="lower")
   if test  "X$pac_cv_prog_f77_name_mangle" = "X" ; then
     AC_TRY_LINK(,my_name_();,pac_cv_prog_f77_name_mangle="lower underscore")
   fi
   if test  "X$pac_cv_prog_f77_name_mangle" = "X" ; then
     AC_TRY_LINK(,MY_NAME();,pac_cv_prog_f77_name_mangle="upper")
   fi
   if test  "X$pac_cv_prog_f77_name_mangle" = "X" ; then
     AC_TRY_LINK(,my_name__();,
       pac_cv_prog_f77_name_mangle="lower doubleunderscore")
   fi
   if test  "X$pac_cv_prog_f77_name_mangle" = "X" ; then
     AC_TRY_LINK(,MY_name();,pac_cv_prog_f77_name_mangle="mixed")
   fi
   if test  "X$pac_cv_prog_f77_name_mangle" = "X" ; then
     AC_TRY_LINK(,MY_name_();,pac_cv_prog_f77_name_mangle="mixed underscore")
   fi
   LIBS="$save_LIBS"
   AC_LANG_RESTORE
   rm -f fconftest*
])
# Make the actual definition
pac_namecheck=`echo X$pac_cv_prog_f77_name_mangle | sed 's/ /-/g'`
ifelse($1,,[
case $pac_namecheck in
    X) AC_MSG_WARN([Cannot determine Fortran naming scheme]) ;;
    Xlower) AC_DEFINE(F77_NAME_LOWER) ;;
    Xlower-underscore) AC_DEFINE(F77_NAME_LOWER_USCORE) ;;
    Xlower-doubleunderscore) AC_DEFINE(F77_NAME_LOWER_2USCORE) ;;
    Xupper) AC_DEFINE(F77_NAME_UPPER) ;;
    Xmixed) AC_DEFINE(F77_NAME_MIXED) ;;
    Xmixed-underscore) AC_DEFINE(F77_NAME_MIXED_USCORE) ;;
    *) AC_MSG_WARN([Unknown Fortran naming scheme]) ;;
esac
],[$1])
])
dnl
dnl/*D
dnl PAC_PROG_F77_CHECK_SIZEOF - Determine the size in bytes of a Fortran
dnl type
dnl
dnl Synopsis:
dnl PAC_PROG_F77_CHECK_SIZEOF(type,[cross-size])
dnl
dnl Output Effect:
dnl Sets SIZEOF_F77_uctype to the size if bytes of type.
dnl If type is unknown, the size is set to 0.
dnl If cross-compiling, the value cross-size is used (it may be a variable)
dnl For example 'PAC_PROG_F77_CHECK_SIZEOF(real)' defines
dnl 'SIZEOF_F77_REAL' to 4 on most systems.  The variable 
dnl 'pac_cv_sizeof_f77_<type>' (e.g., 'pac_cv_sizeof_f77_real') is also set to
dnl the size of the type. 
dnl If the corresponding variable is already set, that value is used.
dnl If the name has an '*' in it (e.g., 'integer*4'), the defined name 
dnl replaces that with an underscore (e.g., 'SIZEOF_F77_INTEGER_4').
dnl
dnl Notes:
dnl If the 'cross-size' argument is not given, 'autoconf' will issue an error
dnl message.
dnl
dnlD*/
AC_DEFUN(PAC_PROG_F77_CHECK_SIZEOF,[
changequote(<<, >>)dnl
dnl The name to #define.
define(<<PAC_TYPE_NAME>>, translit(sizeof_f77_$1, [a-z *], [A-Z__]))dnl
dnl The cache variable name.
define(<<PAC_CV_NAME>>, translit(pac_cv_f77_sizeof_$1, [ *], [__]))dnl
changequote([, ])dnl
AC_CACHE_CHECK([for size of Fortran type $1],PAC_CV_NAME,[
AC_REQUIRE([PAC_PROG_F77_NAME_MANGLE])
/bin/rm -f conftest*
cat <<EOF > conftest.f
      subroutine isize( )
      $1 i(2)
      call cisize( i(1), i(2) )
      end
EOF
if AC_TRY_EVAL(ac_fcompile) && test -s conftest.o ; then
    mv conftest.o conftestf.o
else 
    echo "configure: failed program was:" >&AC_FD_CC
    cat conftest.f >&AC_FD_CC
fi
AC_LANG_SAVE
AC_LANG_C
save_LIBS="$LIBS"
LIBS="conftestf.o $LIBS"
AC_TRY_RUN([#include <stdio.h>
#ifdef F77_NAME_UPPER
#define cisize_ CISIZE
#define isize_ ISIZE
#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
#define cisize_ cisize
#define isize_ isize
#endif
static int isize_val;
void cisize_(char *i1p, char *i2p)
{ 
   isize_val = (int)(i2p - i1p);
}
main()
{
    FILE *f = fopen("conftestval", "w");
    if (!f) exit(1);
    isize_();
    fprintf(f,"%d\n", isize_val );
    exit(0);
}],PAC_CV_NAME=`cat conftestval`, PAC_CV_NAME=0, 
ifelse([$2],,,AC_CV_NAME=$2))
LIBS="$save_LIBS"
AC_LANG_RESTORE
])
AC_DEFINE_UNQUOTED(PAC_TYPE_NAME,$PAC_CV_NAME)
undefine([PAC_TYPE_NAME])
undefine([PAC_CV_NAME])
])
dnl
dnl/*D
dnl PAC_PROG_F77_EXCLAIM_COMMENTS
dnl
dnl Synopsis:
dnl PAC_PROG_F77_EXCLAIM_COMMENTS([action-if-true],[action-if-false])
dnl
dnl Notes:
dnl Check whether '!' may be used to begin comments in Fortran.
dnl
dnl This macro requires a version of autoconf `after` 2.13; the 'acgeneral.m4'
dnl file contains an error in the handling of Fortran programs in 
dnl 'AC_TRY_COMPILE' (fixed in our local version).
dnl
dnlD*/
AC_DEFUN(PAC_PROG_F77_EXCLAIM_COMMENTS,[
AC_CACHE_CHECK([whether Fortran accepts ! for comments],
pac_cv_prog_f77_exclaim_comments,[
AC_LANG_SAVE
AC_LANG_FORTRAN77
AC_TRY_COMPILE(,[
!      This is a comment
],pac_cv_prog_f77_exclaim_comments="yes",
pac_cv_prog_f77_exclaim_comments="no")
AC_LANG_RESTORE
])
if test "$pac_cv_prog_f77_exclaim_comments" = "yes" ; then
    ifelse($1,,:,$1)
else
    ifelse($2,,:,$2)
fi
])dnl
dnl
dnl/*D
dnl PAC_F77_CHECK_COMPILER_OPTION - Check that a compiler option is accepted
dnl without warning messages
dnl
dnl Synopsis:
dnl PAC_F77_CHECK_COMPILER_OPTION(optionname,action-if-ok,action-if-fail)
dnl
dnl Output Effects:
dnl
dnl If no actions are specified, a working value is added to 'FOPTIONS'
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
AC_DEFUN(PAC_F77_CHECK_COMPILER_OPTION,[
AC_MSG_CHECKING([that Fortran 77 compiler accepts option $1])
save_FFLAGS="$FFLAGS"
FFLAGS="$1 $FFLAGS"
rm -f conftest.out
cat >conftest2.f <<EOF
        subroutine try()
        end
EOF
cat >conftest.f <<EOF
        program main
        end
EOF
if ${F77-f77} $save_FFLAGS -o conftest conftest.f >conftest.bas 2>&1 ; then
   if ${F77-f77} $FFLAGS -o conftest conftest.f >conftest.out 2>&1 ; then
      if diff -b conftest.out conftest.bas >/dev/null 2>&1 ; then
         AC_MSG_RESULT(yes)
         AC_MSG_CHECKING([that routines compiled with $1 can be linked with ones compiled  without $1])       
         /bin/rm -f conftest.out
         /bin/rm -f conftest.bas
         if ${F77-f77} -c $save_FFLAGS conftest2.f >conftest2.out 2>&1 ; then
            if ${F77-f77} $FFLAGS -o conftest conftest2.o conftest.f >conftest.bas 2>&1 ; then
               if ${F77-f77} $FFLAGS -o conftest conftest2.o conftest.f >conftest.out 2>&1 ; then
                  if diff -b conftest.out conftest.bas >/dev/null 2>&1 ; then
	             AC_MSG_RESULT(yes)	  
		     FFLAGS="$save_FFLAGS"
                     ifelse($2,,FOPTIONS="$FOPTIONS $1",$2)
                  elif test -s conftest.out ; then
	             cat conftest.out >&AC_FD_CC
	             AC_MSG_RESULT(no)
                     FFLAGS="$save_FFLAGS"
	             $3
                  else
                     AC_MSG_RESULT(no)
                     FFLAGS="$save_FFLAGS"
	             $3
                  fi  
               else
	          if test -s conftest.out ; then
	             cat conftest.out >&AC_FD_CC
	          fi
                  AC_MSG_RESULT(no)
                  FFLAGS="$save_FFLAGS"
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
            FFLAGS="$save_FFLAGS"
	    $3
         fi
      else
         cat conftest.out >&AC_FD_CC
         AC_MSG_RESULT(no)
         $3
         FFLAGS="$save_FFLAGS"         
      fi
   else
      AC_MSG_RESULT(no)
      $3
      if test -s conftest.out ; then cat conftest.out >&AC_FD_CC ; fi    
      FFLAGS="$save_FFLAGS"
   fi
else
    # Could not compile without the option!
    echo "configure: Could not compile program" >&AC_FD_CC
    cat conftest.f >&AC_FD_CC
    cat conftest.bas >&AC_FD_CC
    AC_MSG_RESULT(no)
fi
rm -f conftest*
])
dnl
dnl/*D
dnl PAC_PROG_F77_CMDARGS - Determine how to access the command line from
dnl Fortran 77
dnl
dnl Output Effects:
dnl  The following variables are set:
dnl.vb
dnl    F77_GETARG   - Statement to get an argument i into string s
dnl    F77_IARGC    - Routine to return the number of arguments
dnl    FXX_MODULE   - Module command when using Fortran 90 compiler
dnl    F77_GETARGDECL - Declaration of routine used for F77_GETARG
dnl.ve
dnl If 'F77_GETARG' has a value, then that value and the values for these
dnl other symbols will be used instead.  If no approach is found, all of these
dnl variables will have empty values.
dnl If no other approach works and a file 'f77argdef' is in the directory, 
dnl that file will be sourced for the values of the above four variables.
dnl
dnl 'AC_SUBST' is called for all four variables.
dnl
dnl f77argdef
dnlD*/
AC_DEFUN(PAC_PROG_F77_CMDARGS,[
found_cached="yes"
AC_MSG_CHECKING([for routines to access the command line from Fortran 77])
AC_CACHE_VAL(pac_cv_prog_f77_cmdarg,
[
    AC_MSG_RESULT([searching...])
    found_cached="no"
    # Grumph.  Here are a bunch of different approaches
    # We have several axes the check:
    # Library to link with (none, -lU77 (HPUX), -lg2c (LINUX f77))
    # The first line is "<space><newline>, the space is important
trial_LIBS=" 
-lU77
-lg2c"
    # Discard libs that are not availble:
    save_IFS="$IFS"
    IFS="
"
    save_trial_LIBS="$trial_LIBS"
    trial_LIBS=""
    cat > conftest.f <<EOF
        program main
        end
EOF
    ac_fcompilelink_test='${F77-f77} -o conftest $FFLAGS conftest.f $libs $LIBS 1>&AC_FD_CC'
    for libs in $save_trial_LIBS ; do
	if test "$libs" = " " ; then
	    lib_ok="yes"
        else
	    AC_MSG_CHECKING([whether Fortran 77 links with $libs])
	    if AC_TRY_EVAL(ac_fcompilelink_test) && test -x conftest ; then
		AC_MSG_RESULT([yes])
	        lib_ok="yes"
	    else
		AC_MSG_RESULT([no])
	        lib_ok="no"
	    fi
	fi
	if test "$lib_ok" = "yes" ; then
	    trial_LIBS="$trial_LIBS
$libs"
        fi
    done

    # Options to use when compiling and linking
    # The first line is "<space><newline>, the space is important
trial_FLAGS=" 
-f
-N109
-YEXT_NAMES=LCS
+U77"
    # Discard options that are not available:
    save_IFS="$IFS"
    IFS="
"
    save_trial_FLAGS="$trial_FLAGS"
    trial_FLAGS=""
    for flag in $save_trial_FLAGS ; do
	if test "$flag" = " " ; then
	    opt_ok="yes"
        else
            PAC_F77_CHECK_COMPILER_OPTION($flag,opt_ok=yes,opt_ok=no)
        fi
	if test "$opt_ok" = "yes" ; then
	    if test "$flag" = " " ; then fflag="" ; else fflag="$flag" ; fi
	    
	    # discard options that don't allow mixed-case name matching
	    cat > conftest.f <<EOF
        program main
        call aB()
        end
        subroutine Ab()
        end
EOF
	    if test -n "$fflag" ; then flagval="with $fflag" ; else flagval="" ; fi
	    AC_MSG_CHECKING([that Fortran 77 routine names are case-insensitive $flagval])
	    dnl we can use double quotes here because all is already
            dnl evaluated
            ac_fcompilelink_test="${F77-f77} -o conftest $fflag $FFLAGS
conftest.f $LIBS 1>&AC_FD_CC"
	    if AC_TRY_EVAL(ac_fcompilelink_test) && test -x conftest ; then
	        AC_MSG_RESULT(yes)
	    else
	        AC_MSG_RESULT(no)
	        opt_ok="no"
            fi
        fi
        if test "$opt_ok" = "yes" ; then
	    trial_FLAGS="$trial_FLAGS
$flag"
        fi
    done
    IFS="$save_IFS"
    # Name of routines.  Since these are in groups, we use a case statement
    # and loop until the end (accomplished by reaching the end of the
    # case statement
    trial=0
    while test -z "$pac_cv_prog_f77_cmdarg" ; do
        case $trial in 
	0) # User-specified values, if any
	   if test -z "$F77_GETARG" -o -z "$F77_IARGC" ; then 
	       trial=`expr $trial + 1`
	       continue 
           fi
           MSG="Using environment values of F77_GETARG etc."
	   ;;
	1) # Standard practice, uppercase (some compilers are case-sensitive)
	   FXX_MODULE=""
	   F77_GETARGDECL="external GETARG"
	   F77_GETARG="call GETARG(i,s)"
	   F77_IARGC="IARGC()"
	   MSG="GETARG and IARGC"
	   ;;
	2) # Standard practice, lowercase
	   FXX_MODULE=""
           F77_GETARGDECL="external getarg"
	   F77_GETARG="call getarg(i,s)"
	   F77_IARGC="iargc()"
	   MSG="getarg and iargc"
	   ;;
	3) # Posix alternative
	   FXX_MODULE=""
	   F77_GETARGDECL="external pxfgetarg"
	   F77_GETARG="call pxfgetarg(i,s,l,ier)"
	   F77_IARGC="ipxfiargc()"
	   MSG="pxfgetarg and ipxfiargc"
	   ;;
	4) # Nag f90_unix_env module
	   FXX_MODULE="        use f90_unix_env"
	   F77_GETARGDECL=""
	   F77_GETARG="call getarg(i,s)"
	   F77_IARGC="iargc()"
	   MSG="f90_unix_env module"
	   ;;
        5) # Nag f90_unix module
	   FXX_MODULE="        use f90_unix"
	   F77_GETARGDECL=""
	   F77_GETARG="call getarg(i,s)"
	   F77_IARGC="iargc()"
	   MSG="f90_unix module"
	   ;;
	6) # user spec in a file
	   if test -s f77argdef ; then
		. ./f77argdef
	       MSG="Using definitions in the file f77argdef"
	   else
	        trial=`expr $trial + 1`
		continue
	   fi
	   ;;
        *) # exit from while loop
	   FXX_MODULE=""
	   F77_GETARGDECL=""
	   F77_GETARG=""
	   F77_IARGC=""
           break
	   ;;
	esac
	# Create the program
        cat > conftest.f <<EOF
        program main
$FXX_MODULE
        integer i
        character*20 s

        $F77_GETARGDECL
        $F77_GETARG
        i=$F77_IARGC
        end
EOF
    #
    # Now, try to find some way to compile and link that program, looping 
    # over the possibilities of options and libraries
        save_IFS="$IFS"
        IFS="
"
        for libs in $trial_LIBS ; do
            if test -n "$pac_cv_prog_f77_cmdarg" ; then break ; fi
	    if test "$libs" = " " ; then libs="" ; fi
            for flags in $trial_FLAGS ; do
	        if test "$flags" = " " ; then flags="" ; fi
                AC_MSG_CHECKING([if ${F77-f77} $flags $libs works with $MSG])
		IFS="$save_IFS"
		dnl We need this here because we've fiddled with IFS
	        ac_fcompilelink_test="${F77-f77} -o conftest $FFLAGS $flags conftest.f $libs $LIBS 1>&AC_FD_CC"
                if AC_TRY_EVAL(ac_fcompilelink_test) && test -x conftest ; then
	            AC_MSG_RESULT([yes])
		    pac_cv_prog_f77_cmdarg="$MSG"
		    break
	        else
                    AC_MSG_RESULT([no])
		    echo "configure: failed program was:" >&AC_FD_CC
                    cat conftest.f >&AC_FD_CC
	        fi
		IFS="
"
            done
        done
        IFS="$save_IFS"   
	rm -f conftest.*
        trial=`expr $trial + 1`   
    done
pac_cv_F77_GETARGDECL="$F77_GETARGDECL"
pac_cv_F77_IARGC="$F77_IARGC"
pac_cv_F77_GETARG="$F77_GETARG"
pac_cv_FXX_MODULE="$FXX_MODULE"
])
if test "$found_cached" = "yes" ; then 
    AC_MSG_RESULT([$pac_cv_prog_f77_cmdarg])
elif test -z "$pac_cv_F77_IARGC" ; then
    AC_MSG_WARN([Could not find a way to access the command line from Fortran 77])
fi
# Set the variable values based on pac_cv_prog_xxx
F77_GETARGDECL="$pac_cv_F77_GETARGDECL"
F77_IARGC="$pac_cv_F77_IARGC"
F77_GETARG="$pac_cv_F77_GETARG"
FXX_MODULE="$pac_cv_FXX_MODULE"
AC_SUBST(F77_GETARGDECL)
AC_SUBST(F77_IARGC)
AC_SUBST(F77_GETARG)
AC_SUBST(FXX_MODULE)
])
