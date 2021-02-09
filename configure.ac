dnl Process this file with autoconf to produce a configure script

dnl Need autoconf 2.50 or later for AC_ARG_VAR.  2.59 has been around for
dnl long enough, that we might as well just require that.
AC_PREREQ(2.59)
AC_INIT([survex], [1.2.44], [https://trac.survex.com/])
AM_INIT_AUTOMAKE([1.5 gnu -Wall -Wportability -Werror])
RELEASE=1

AC_SUBST(RELEASE)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/aven.cc])

COPYRIGHT_MSG="Copyright (C) 1990-2021 Olly Betts"
AVEN_COPYRIGHT_MSG="Copyright (C) 1999-2003,2005,2006 Mark R. Shinwell"

AC_DEFINE_UNQUOTED(COPYRIGHT_MSG, ["$COPYRIGHT_MSG"], [Copyright Message])
AC_DEFINE_UNQUOTED(AVEN_COPYRIGHT_MSG, ["$AVEN_COPYRIGHT_MSG"],
		   [Copyright Message for Aven])

AC_SUBST(COPYRIGHT_MSG)
AC_SUBST(AVEN_COPYRIGHT_MSG)

COPYRIGHT_MSG_UTF8=`echo "$COPYRIGHT_MSG"|sed 's/(C)/©/'`
AVEN_COPYRIGHT_MSG_UTF8=`echo "$AVEN_COPYRIGHT_MSG"|sed 's/(C)/©/'`

AC_DEFINE_UNQUOTED(COPYRIGHT_MSG_UTF8, ["$COPYRIGHT_MSG_UTF8"],
		   [Copyright Message in UTF-8])
AC_DEFINE_UNQUOTED(AVEN_COPYRIGHT_MSG_UTF8, ["$AVEN_COPYRIGHT_MSG_UTF8"],
		   [Copyright Message for Aven in UTF-8])

dnl set PRETTYPACKAGE to PACKAGE with the first character capitalised
PRETTYPACKAGE=`echo "$PACKAGE"|cut -b1|tr a-z A-Z``echo "$PACKAGE"|cut -b2-`
AC_DEFINE_UNQUOTED(PRETTYPACKAGE, "$PRETTYPACKAGE",
		   [Name of package (capitalised)])
AC_SUBST(PRETTYPACKAGE)

dnl set COMMAVERSION to VERSION with the dots replaced by commas -
dnl e.g. "0,99" or "1,0,22"
COMMAVERSION=`echo "$VERSION"|tr '.' ','`
AC_DEFINE_UNQUOTED(COMMAVERSION, $COMMAVERSION,
		   [Version number of package (comma-separated)])
AC_SUBST(COMMAVERSION)

PKGDOCDIR='${prefix}/share/doc/${PACKAGE}'
AC_ARG_ENABLE(docdir,
[  --enable-docdir=DIR Set directory for installing documentation to DIR],
[case $enableval in
 yes|no)
  AC_MSG_ERROR([configure: Syntax: configure --enable-docdir=DIR]) ;;
esac
PKGDOCDIR="$enableval"
]
)
PKGDOCDIR_EXPANDED=`(
 test NONE = "$prefix" && prefix="$ac_default_prefix"
 test NONE = "$exec_prefix" && exec_prefix="$prefix"
 eval echo "$PKGDOCDIR"
)`
AC_SUBST(PKGDOCDIR)
AC_SUBST(PKGDOCDIR_EXPANDED)

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP

AC_PROG_CXX
AC_PROG_CXXCPP

dnl Probe for any options needed to enable C++11 support.
AX_CXX_COMPILE_STDCXX_11

AM_PROG_CC_C_O

AC_LANG_C

AC_C_INLINE

AC_EXEEXT
AC_OBJEXT

mswindows=no
macos=no
case $host_os in
darwin*) macos=yes ;;
*mingw*|*cygwin*|windows*) mswindows=yes ;;
esac

AC_C_BIGENDIAN

AC_ARG_VAR(STRIP, [Command for discarding symbols from object files])
AC_PATH_TOOL(STRIP, strip, [echo "not stripping "])

AC_CHECK_LIB(m, sqrt)

AC_PATH_XTRA

dnl The wxWidgets libraries we need:
wx_libs="core,gl,html"

AC_ARG_VAR(WXCONFIG, [Old name for WX_CONFIG, accepted for compatibility])
AC_ARG_VAR(WX_CONFIG, [wxWidgets configuration script to use to build Aven])
: ${WX_CONFIG="$WXCONFIG"}

AM_CONDITIONAL(WIN32, [test yes = "$mswindows"])
AM_CONDITIONAL(MACOS, [test yes = "$macos"])

if test -n "$WX_CONFIG" ; then
  dnl WX_CONFIG specified - sanity check the value
  dnl don't check for --ldflags - older wx-config didn't do that
  if (exec >&5 2>&5;$WX_CONFIG --libs --cflags --cxxflags "$wx_libs";exit $?) then
    :
  else
    AC_MSG_ERROR(['$WX_CONFIG --libs --cflags --cxxflags "$wx_libs"' does not work, bailing out])
  fi
else
  if test yes = "$macos" ; then
    wxdef=__WXMAC__
  elif test yes = "$mswindows" ; then
    wxdef=__WXMSW__
  else
    wxdef=__WXGTK__
  fi
  dnl See if wx-config exists and is for the correct version.
  dnl Fedora install wx3's wx-config as wx-config-3.0, so look for that in
  dnl preference.
  dnl Arch Linux uses wx-config-gtk3.
  AC_PATH_PROGS(WX_CONFIG, [wx-config-3.0 wx-config-gtk3 wx-config])
  if test -n "$WX_CONFIG" ; then
    if (exec >&5 2>&5;$WX_CONFIG --cflags "$wx_libs"|grep -e -D"$wxdef";exit $?) then
      :
    else
      AC_MSG_ERROR([wxWidgets not for the right toolkit.  Run ./configure WX_CONFIG=/path/to/wx-config])
    fi
  else
    AC_MSG_ERROR([wxWidgets not found.  Run ./configure WX_CONFIG=/path/to/wx-config])
  fi
fi

dnl Unless the user has explicitly specified a --unicode setting, prefer
dnl --unicode=yes if we can get it.
case $WX_CONFIG in
  *--unicode=*) ;;
  *)
    if (exec >&5 2>&5;$WX_CONFIG --unicode=yes --version "$wx_libs";exit $?) ; then
      WX_CONFIG="$WX_CONFIG --unicode=yes"
    elif (exec >&5 2>&5;$WX_CONFIG --unicode=no --version "$wx_libs";exit $?) ; then
      WX_CONFIG="$WX_CONFIG --unicode=no"
    fi ;;
esac

WX_LIBS=`$WX_CONFIG --libs "$wx_libs"`
dnl Needed for nvidia drivers on linux (for some setups anyway).
AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryVersion, [WX_LIBS="$WX_LIBS -lXxf86vm"], [], [$WX_LIBS])
dnl We use functions from libGLU so always link -lGLU explicitly if it's
dnl present.
AC_CHECK_LIB([GLU], [gluProject], [WX_LIBS="$WX_LIBS -lGLU"], [], [$WX_LIBS])
dnl We use functions from libGL so always link -lGL explicitly if it's
dnl present.
AC_CHECK_LIB([GL], [glPushMatrix], [WX_LIBS="$WX_LIBS -lGL"], [], [$WX_LIBS])
AC_SUBST(WX_LIBS)
dnl macOS has OpenGL/gl.h.
AC_CHECK_HEADERS([GL/gl.h OpenGL/gl.h], [], [], [ ])
dnl The Debian mingw-w64 packages lack GL/glext.h; macOS has OpenGL/glext.h.
AC_CHECK_HEADERS([GL/glext.h OpenGL/glext.h], [], [], [#include <GL/gl.h>])

WX_CFLAGS=`$WX_CONFIG --cflags "$wx_libs"`
AC_SUBST(WX_CFLAGS)
WX_CXXFLAGS=`$WX_CONFIG --cxxflags "$wx_libs"`
AC_SUBST(WX_CXXFLAGS)

AC_LANG_C
save_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS $WX_CFLAGS"
AC_CHECK_SIZEOF([wxChar], [], [
  #include <wx/defs.h>
  #include <wx/chartype.h>
])
CFLAGS=$save_CFLAGS

dnl Check for libav (FIXME: does our code still work with ffmpeg?)
PKG_CHECK_MODULES([LIBAV], [libavcodec libavformat libswscale libavutil], [
  AC_DEFINE([WITH_LIBAV], [1], [Use libav for movie export])
  save_CXXFLAGS=$CXXFLAGS
  save_LIBS=$LIBS
  CFLAGS="$CFLAGS $LIBAV_CFLAGS"
  LIBS="$LIBS $LIBAV_LIBS"
  AC_CHECK_FUNCS([av_guess_format avio_open avio_close avformat_write_header avcodec_encode_video2 avcodec_free_frame avcodec_open2 avformat_new_stream av_frame_alloc av_frame_free])
  AC_CHECK_DECLS([AVMEDIA_TYPE_VIDEO], [], [], [#include <libavcodec/avcodec.h>])
  AC_CHECK_DECLS([AV_CODEC_ID_NONE], [], [], [#include <libavcodec/avcodec.h>])
  AC_CHECK_DECLS([AV_PIX_FMT_RGB24], [], [], [#include <libavutil/pixfmt.h>])
  AC_CHECK_DECLS([AV_PIX_FMT_YUV420P], [], [], [#include <libavutil/pixfmt.h>])
  CXXFLAGS=$save_CXXFLAGS
  LIBS=$save_LIBS
], [
  dnl Build without movie export feature.
])
AC_SUBST([LIBAV_LIBS])
AC_SUBST([LIBAV_CFLAGS])

dnl Check for PROJ4
PKG_CHECK_MODULES([PROJ], [proj], [
], [
  dnl pkg-config support in proj seems quite new, so probe directly if not
  dnl found.
  AC_CHECK_LIB([proj], [pj_init_plus], [
      PROJ_LIBS=-lproj
      PROJ_CFLAGS=
    ], [
      AC_MSG_ERROR([PROJ4 required for coordinate transformations])
    ])
])
AC_SUBST([PROJ_LIBS])
AC_SUBST([PROJ_CFLAGS])
dnl Check for new-style PROJ header.
AC_CHECK_HEADERS([proj.h])

dnl Checks for header files.
AC_HEADER_STDC
dnl don't use AC_CHECK_FUNCS for setjmp - mingw #define-s it to _setjmp
AC_CHECK_HEADERS(limits.h string.h setjmp.h sys/select.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_SIGNAL

dnl check if CLOCKS_PER_SEC is defined by time.h (as ANSI says it should be)
dnl if it isn't, assume it is 1000000, as it is on SunOS 4
AC_TRY_COMPILE([#include <time.h>],[
#ifndef CLOCKS_PER_SEC
choke me
#endif
], , AC_DEFINE(CLOCKS_PER_SEC, 1000000, [For old systems which don't define it themselves]))

dnl Check for 16-bit and 32-bit integer types.
AC_TYPE_INT16_T
AC_TYPE_INT32_T

dnl Checks for library functions.
AC_FUNC_STRFTIME
if test yes != "$ac_cv_func_strftime"; then
  AC_LIBOBJ(strftime)
fi
AC_FUNC_VPRINTF
dnl Check if there's a declaration of lround().  Below we also check for
dnl lround() with AC_CHECK_FUNCS, which just checks if it can be linked
dnl against.  Some platforms are missing a prototype for lround(), so
dnl both checks are useful.
AC_CHECK_DECLS([lround], [], [], [[#include <math.h>]])
dnl check for strcasecmp here as well as below - this one is to get
dnl HAVE_STRCASECMP defined if appropriate (for img.c)
AC_CHECK_FUNCS([popen getpwuid lround strcasecmp hypot mmap])

dnl difftime seems to be an ANSI invention
AC_CHECK_FUNCS(difftime)

dnl Much faster than using getc()/putc(), at least on Linux.
AC_CHECK_FUNCS([getc_unlocked putc_unlocked])

AC_CHECK_FUNCS([setenv unsetenv])

dnl try to find a case-insensitive compare

strcasecmp=no
dnl The order of these tests is important.
AC_CHECK_FUNC(strcasecmp, [strcasecmp=strcasecmp])
if test no = "$strcasecmp"; then
  AC_CHECK_FUNC(stricmp, [strcasecmp=stricmp])
fi
if test no = "$strcasecmp"; then
  AC_CHECK_FUNC(strcmpi, [strcasecmp=strcmpi])
fi

AC_MSG_CHECKING([how to compare strings ignoring case])
case $strcasecmp in
no)
  AC_LIBOBJ(strcasecmp)
  AC_MSG_RESULT([using own implementation of strcasecmp]) ;;
strcasecmp)
  AC_MSG_RESULT([strcasecmp]) ;;
*)
  AC_DEFINE_UNQUOTED(strcasecmp, "$strcasecmp",
    [strcasecmp define for systems which call it something else])
  AC_MSG_RESULT([$strcasecmp]) ;;
esac

AC_MSG_CHECKING(for sgmltools v2 or v3)
if test : != "$SGMLTOOLS" ; then
  : ${SGMLTOOLS=sgmltools}
  case `(exec 2>&5 ; $SGMLTOOLS --version)` in
  *" version 3"*)
    AC_MSG_RESULT([$SGMLTOOLS]) ;;
  *" version 2"*)
    AC_MSG_RESULT([$SGMLTOOLS]) ;;
  *)
    SGMLTOOLS="$MISSING sgmltools"
    AC_MSG_RESULT(not found) ;;
  esac
else
  AC_MSG_RESULT([$SGMLTOOLS])
fi
AC_SUBST(SGMLTOOLS)
AC_ARG_VAR(SGMLTOOLS, [sgmltools command])

AC_MSG_CHECKING(for docbook2man)
if test : != "$DOCBOOK2MAN" ; then
  : ${DOCBOOK2MAN=docbook2man}
  $DOCBOOK2MAN 2> /dev/null
  if test $? = 1 ; then
    AC_MSG_RESULT([$DOCBOOK2MAN])
  else
    DOCBOOK2MAN="$MISSING docbook2man"
    AC_MSG_RESULT([not found])
  fi
else
  AC_MSG_RESULT([$DOCBOOK2MAN])
fi
AC_SUBST(DOCBOOK2MAN)
AC_ARG_VAR(DOCBOOK2MAN, [docbook2man command])

dnl extra warning flags for building with GCC
if test yes = "$GCC"; then
  if test yes = "$cross_compiling"; then
dnl    AM_CFLAGS="$AM_CFLAGS -Werror -Wall -Wunused -Wpointer-arith\
dnl      -Wwrite-strings -Wcast-align"
dnl -Wsign-compare causes a warning with the mingw FD_SET macro, so we have
dnl to disable it for mingw builds.
    AM_CFLAGS="$AM_CFLAGS -Wall -Wunused -Wpointer-arith\
      -Wwrite-strings -Wcast-align"
    AM_CXXFLAGS="$AM_CXXFLAGS -Wall -Wunused -Wpointer-arith\
      -Wwrite-strings -Wcast-align -Wno-sign-compare"
  else
    AM_CFLAGS="$AM_CFLAGS -Wall -W -Wunused -Wshadow -Wpointer-arith\
      -Wmissing-prototypes -Wwrite-strings -Wredundant-decls -Wnested-externs\
      -Wcast-align"
    AM_CXXFLAGS="$AM_CXXFLAGS -Wall -W -Wunused -Wshadow -Wpointer-arith\
      -Wwrite-strings -Wcast-align"
  fi
  dnl too many complaints from headers, etc: -Wconversion
fi
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)

dnl See if large file support is available.  Survex is unlikely to need to
dnl process files > 2GB in size, but LFS is also needed for stat() to work
dnl on filing systems which return 64 bit inode values, such as CIFS mounts.
AC_SYS_LARGEFILE

AC_ARG_ENABLE(profiling,
[  --enable-profiling Build binaries to generate profiling information],
[case $enableval in
  yes) AM_CXXFLAGS="$AM_CXXFLAGS -pg"
       AM_CFLAGS="$AM_CFLAGS -pg"
       AC_MSG_RESULT(building binaries to generate profiling information);;
  no) ;;
  *) AC_MSG_ERROR(bad value $enableval for --enable-profiling) ;;
esac])

HTMLFILES="AUTHORS.htm HACKING.htm NEWS.htm OLDNEWS.htm TODO.htm index.htm 3dformat.htm 3dformat-old.htm"
AC_SUBST(HTMLFILES)

EXTRA_TEXT="AUTHORS COPYING NEWS TODO ChangeLog"
AC_SUBST(EXTRA_TEXT)

AC_SUBST_FILE(DESC)
DESC=desc.txt
AC_SUBST_FILE(AVENDESC)
AVENDESC=desc-aven.txt

dnl Don't define DATADIR if building for MS Windows - it won't be used, and
dnl can conflict with the DATADIR typedef in objidl.h
if test no = "$mswindows"; then
  AC_DEFINE_DIR(DATADIR, datadir, [Location of platform independent support files])
fi

AH_BOTTOM(
[/* Use getc_unlocked() and putc_unlocked() where available, since they are
 * faster, and we don't multithread file accesses.
 */

#ifdef HAVE_GETC_UNLOCKED
# define GETC(F) getc_unlocked(F)
#else
# define GETC(F) getc(F)
#endif
#ifdef HAVE_PUTC_UNLOCKED
# define PUTC(C, F) putc_unlocked(C, F)
#else
# define PUTC(C, F) putc(C, F)
#endif
])

AC_CONFIG_FILES([
 Makefile src/Makefile doc/Makefile lib/Makefile lib/icons/Makefile
 lib/images/Makefile
 tests/Makefile vim/Makefile survex.iss doc/index.htm doc/survex.7
 survex.spec lib/Info.plist
 ])
AC_OUTPUT
dnl FIXME update msvc makefile for 1.2 branch
dnl src/msvc/config.h
