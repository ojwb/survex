dnl @synopsis AC_DEFINE_DIR(VARNAME, DIR)
dnl
dnl This macro defines (with AC_DEFINE) VARNAME to the expansion of the DIR
dnl variable, taking care of fixing up ${prefix} and such.
dnl
dnl Example:
dnl
dnl    AC_DEFINE_DIR(DATADIR, datadir)
dnl
dnl @version $Id: acinclude.m4,v 1.1 1999-09-26 22:18:15 olly Exp $
dnl @author Alexandre Oliva <oliva@dcc.unicamp.br>

AC_DEFUN(AC_DEFINE_DIR, [
	ac_expanded=`(
	    test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	    test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
	    eval echo \""[$]$2"\"
        )`
	AC_DEFINE_UNQUOTED($1, "$ac_expanded", $3)
])
