dnl @synopsis AC_DEFINE_DIR(VARNAME, DIR [, DESCRIPTION])
dnl
dnl This macro defines (with AC_DEFINE) VARNAME to the expansion of the DIR
dnl variable, taking care of fixing up ${prefix} and such.
dnl
dnl Note that the 3 argument form is only supported with autoconf 2.13 and
dnl later (i.e. only where AC_DEFINE supports 3 arguments).
dnl
dnl Examples:
dnl
dnl    AC_DEFINE_DIR(DATADIR, datadir)
dnl
dnl    AC_DEFINE_DIR(PROG_PATH, bindir, [Location of installed binaries])
dnl
dnl @author Alexandre Oliva <oliva@lsd.ic.unicamp.br>

AC_DEFUN(AC_DEFINE_DIR, [
	ac_expanded=`(
	    test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	    test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
	    eval echo \""[$]$2"\"
        )`
	ifelse($3, ,
	  AC_DEFINE_UNQUOTED($1, "$ac_expanded"), 
	  AC_DEFINE_UNQUOTED($1, "$ac_expanded", $3))
])

dnl hacked version of AC_PROG_CXX which allows us to continue on failure
dnl hopefully autoconf will allow this sort of stuff soon so we can
dnl avoid such distateful activities
AC_DEFUN(AC_PROG_CXX_PROBE,
[AC_BEFORE([$0], [AC_PROG_CXXCPP])dnl
AC_CHECK_PROGS(CXX, $CCC m4_default([$1], [c++ g++ gpp aCC CC cxx cc++ cl]),)

if test -n "$CXX" ; then
  AC_PROG_CXX_WORKS_PROBE
  if test -n "$CXX" ; then
    AC_PROG_CXX_GNU
  
    if test $ac_cv_prog_gxx = yes; then
      GXX=yes
    else
      GXX=
    fi
  
    dnl Check whether -g works, even if CXXFLAGS is set, in case the package
    dnl plays around with CXXFLAGS (such as to build both debugging and
    dnl normal versions of a library), tasteless as that idea is.
    ac_test_CXXFLAGS="${CXXFLAGS+set}"
    ac_save_CXXFLAGS="$CXXFLAGS"
    CXXFLAGS=
    AC_PROG_CXX_G
    if test "$ac_test_CXXFLAGS" = set; then
      CXXFLAGS="$ac_save_CXXFLAGS"
    elif test $ac_cv_prog_cxx_g = yes; then
      if test "$GXX" = yes; then
        CXXFLAGS="-g -O2"
      else
        CXXFLAGS="-g"
      fi
    else
      if test "$GXX" = yes; then
        CXXFLAGS="-O2"
      else
        CXXFLAGS=
      fi
    fi
  fi
fi
])# AC_PROG_CXX_PROBE

# AC_PROG_CXX_WORKS_PROBE
# -----------------
AC_DEFUN(AC_PROG_CXX_WORKS_PROBE,
[AC_MSG_CHECKING([whether the C++ compiler ($CXX $CXXFLAGS $CPPFLAGS $LDFLAGS) works])
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILER([int main(){return(0);}],
                ac_cv_prog_cxx_works, ac_cv_prog_cxx_cross)
AC_LANG_RESTORE
AC_MSG_RESULT($ac_cv_prog_cxx_works)
if test $ac_cv_prog_cxx_works = no; then
  $CXX=
else  
  AC_MSG_CHECKING([whether the C++ compiler ($CXX $CXXFLAGS $CPPFLAGS $LDFLAGS) is a cross-compiler])
  AC_MSG_RESULT($ac_cv_prog_cxx_cross)
  cross_compiling=$ac_cv_prog_cxx_cross
fi
])# AC_PROG_CXX_WORKS_PROBE
