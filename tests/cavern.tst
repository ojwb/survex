#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${CAVERN=../src/cavern}
: ${DIFFPOS=../src/diffpos}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

: ${TESTS=${*-"singlefix singlereffix oneleg midpoint noose cross firststn\
 deltastar deltastar2 bug3 calibrate_tape nosurvey nosurvey2\
 cartesian lengthunits angleunits cmd_truncate cmd_case cmd_fix cmd_solve\
 cmd_entrance\
 beginroot revcomplist break_replace_pfx bug0 bug1 bug2 bug4 bug5\
 expobug require export export2 includecomment\
 self_loop self_eq_loop reenterwarn cmd_default cmd_prefix\
 singlefixerr singlereffixerr\
 begin_no_end end_no_begin end_no_begin_nest require_fail\
 exporterr1 exporterr2 exporterr3 exporterr4 exporterr5\
 exporterr1b exporterr2b exporterr3b exporterr6 exporterr6b\
 hanging_cpt badinc badinc2 non_existant_file\
 stnsurvey1 stnsurvey2 stnsurvey3\
 tapelessthandepth longname chinabug chinabug2 multinormal"}}

for file in $TESTS ; do
  # how many warnings to expect
  count=
  case "$file" in
  singlefix) pos=yes ; count=1;;
  singlereffix) pos=yes ; count=0 ;;
  oneleg) pos=yes ; count=0 ;;
  midpoint) pos=yes ; count=0 ;;
  noose) pos=yes ; count=0 ;;
  cross) pos=yes ; count=0 ;;
  firststn) pos=yes ; count=0 ;;
  deltastar) pos=yes ; count=0 ;;
  deltastar2) pos=yes ; count=0 ;;
  bug3) pos=yes ; count=0 ;;
  calibrate_tape) pos=yes ; count=0 ;;
  nosurvey) pos=yes ; count=0 ;;
  nosurvey2) pos=yes ; count=0 ;;
  cartesian) pos=yes ; count=0 ;;
  lengthunits) pos=yes ; count=0 ;;
  angleunits) pos=yes ; count=0 ;;
  cmd_truncate) pos=yes ; count=0 ;;
  cmd_case) pos=yes ; count=0 ;;
  cmd_fix) pos=yes ; count=1 ;;
  cmd_solve) pos=yes ; count=0 ;;
  cmd_entrance) pos=no ; count=0 ;;
  beginroot) pos=no ;;
  revcomplist) pos=no ; count=0 ;;
  break_replace_pfx) pos=no ; count=0 ;;
  bug0) pos=no ; count=0 ;;
  bug1) pos=no ; count=0 ;;
  bug2) pos=no ; count=0 ;;
  bug4) pos=no ; count=0 ;;
  bug5) pos=no ; count=0 ;;
  expobug) pos=no ; count=0 ;;
  require) pos=no ; count=0 ;;
  export) pos=no ; count=0 ;;
  export2) pos=no ; count=0 ;;
  includecomment) pos=no ; count=0 ;;
  self_loop) pos=fail ; count=0 ;;
  self_eq_loop) pos=no ; count=1 ;;
  reenterwarn) pos=no ; count=2 ;;
  cmd_default) pos=no ; count=3 ;;
  singlereffixerr) pos=no ; count=0 ;;
  cmd_prefix) pos=no ; count=1 ;;
  singlefixerr) pos=no ; count=1 ;;
  tapelessthandepth) pos=no ; count=1 ;;
  chinabug2) pos=no ; count=0 ;;
  longname) pos=no ; count=0 ;;
  chinabug) pos=fail ;;
  begin_no_end) pos=fail ;;
  end_no_begin) pos=fail ;;
  end_no_begin_nest) pos=fail ;;
  require_fail) pos=fail ;;
  exporterr1) pos=fail ;;
  exporterr2) pos=fail ;;
  exporterr3) pos=fail ;;
  exporterr4) pos=fail ;;
  exporterr5) pos=fail ;;
  exporterr1b) pos=fail ;;
  exporterr2b) pos=fail ;;
  exporterr3b) pos=fail ;;
  exporterr6) pos=fail ;;
  exporterr6b) pos=fail ;;
  hanging_cpt) pos=fail ;;
  badinc) pos=fail ;;
  badinc2) pos=fail ;;
  non_existant_file) pos=fail ;;
  stnsurvey1) pos=fail ;;
  stnsurvey2) pos=fail ;;
  stnsurvey3) pos=fail ;;
  multinormal) pos=yes ; count=0 ;;
  *) file='' ;;
  esac

  if test x"$file" != x ; then
    echo $file
    rm -f ./tmp.*
    if test -n "$VERBOSE" ; then
      if test x"$pos" = xfail ; then
        $CAVERN $srcdir/$file.svx --output=./tmp
	# success gives 0, signal (128 + <signal number>)
	test $? = 1 || exit 1
      else
        $CAVERN $srcdir/$file.svx --output=./tmp || exit 1
      fi
    else
      if test -z "$count" ; then
        if test x"$pos" = xfail ; then
	  $CAVERN $srcdir/$file.svx --output=./tmp > /dev/null
	  # success gives 0, signal (128 + <signal number>)
	  test $? = 1 || exit 1
	else
          $CAVERN $srcdir/$file.svx --output=./tmp > /dev/null || exit 1
	fi
      fi
    fi
    if test -n "$count" ; then
      warns=`$CAVERN $srcdir/$file.svx --output=./tmp | sed '$!d;$s/^Done./0 /;s/[^0-9]*\([0-9]*\).*/\1/'`
      test $? = 0 || exit 1
      test x"$warns" = x"$count" || exit 1
    fi
    case "$pos" in
    yes)
      if test -n "$VERBOSE" ; then
        $DIFFPOS ./tmp.pos $srcdir/$file.pos 0 || exit 1
      else
        $DIFFPOS ./tmp.pos $srcdir/$file.pos 0 > /dev/null || exit 1
      fi ;;
    no)
      test -f ./tmp.pos || exit 1 ;;
    fail)
      test -f ./tmp.pos && exit 1 ;;
    *)
      echo "Bad value for pos" ; exit 1 ;;
    esac
    rm -f ./tmp.*
  fi
done
exit 0
