#!/bin/sh

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

: ${CAVERN="$testdir"/../src/cavern}
: ${DIFFPOS="$testdir"/../src/diffpos}

SURVEXHOME="$testdir"/../lib
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
 tapelessthandepth longname chinabug chinabug2\
 multinormal multinormignall multidiving multicartesian multinosurv\
 multinormalbad multibug cmd_title cmd_titlebad cmd_dummy cmd_infer\
 cartes diving normal normignall nosurv"}}

for file in $TESTS ; do
  # how many warnings to expect
  warn=
  # how many errors to expect
  error=
  case "$file" in
  singlefix) pos=yes ; warn=1;;
  singlereffix) pos=yes ; warn=0 ;;
  oneleg) pos=yes ; warn=0 ;;
  midpoint) pos=yes ; warn=0 ;;
  noose) pos=yes ; warn=0 ;;
  cross) pos=yes ; warn=0 ;;
  firststn) pos=yes ; warn=0 ;;
  deltastar) pos=yes ; warn=0 ;;
  deltastar2) pos=yes ; warn=0 ;;
  bug3) pos=yes ; warn=0 ;;
  calibrate_tape) pos=yes ; warn=0 ;;
  nosurvey) pos=yes ; warn=0 ;;
  nosurvey2) pos=yes ; warn=0 ;;
  cartesian) pos=yes ; warn=0 ;;
  lengthunits) pos=yes ; warn=0 ;;
  angleunits) pos=yes ; warn=0 ;;
  cmd_truncate) pos=yes ; warn=0 ;;
  cmd_case) pos=yes ; warn=0 ;;
  cmd_fix) pos=yes ; warn=1 ;;
  cmd_solve) pos=yes ; warn=0 ;;
  cmd_entrance) pos=no ; warn=0 ;;
  beginroot) pos=no ;;
  revcomplist) pos=no ; warn=0 ;;
  break_replace_pfx) pos=no ; warn=0 ;;
  bug0) pos=no ; warn=0 ;;
  bug1) pos=no ; warn=0 ;;
  bug2) pos=no ; warn=0 ;;
  bug4) pos=no ; warn=0 ;;
  bug5) pos=no ; warn=0 ;;
  expobug) pos=no ; warn=0 ;;
  require) pos=no ; warn=0 ;;
  export) pos=no ; warn=0 ;;
  export2) pos=no ; warn=0 ;;
  includecomment) pos=no ; warn=0 ;;
  self_loop) pos=fail ; warn=0 ;;
  self_eq_loop) pos=no ; warn=1 ;;
  reenterwarn) pos=no ; warn=2 ;;
  cmd_default) pos=no ; warn=3 ;;
  singlereffixerr) pos=no ; warn=0 ;;
  cmd_prefix) pos=no ; warn=1 ;;
  singlefixerr) pos=no ; warn=1 ;;
  tapelessthandepth) pos=no ; warn=1 ;;
  chinabug2) pos=no ; warn=0 ;;
  longname) pos=no ; warn=0 ;;
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
  multinormal) pos=yes ; warn=0 ;;
  multinormignall) pos=yes ; warn=0 ;;
  multidiving) pos=yes ; warn=0 ;;
  multicartesian) pos=yes ; warn=0 ;;
  multinosurv) pos=yes ; warn=0 ;;
  multinormalbad) pos=fail ;;
  multibug) pos=no ; warn=0 ;;
  cmd_title) pos=no ; warn=0 ;;
  cmd_titlebad) pos=fail ; error=4 ;;
  cmd_dummy) pos=no ; warn=0 ;;
  cmd_infer) pos=yes ; warn=0 ;;
  cartes) pos=yes ; warn=0 ;;
  diving) pos=yes ; warn=0 ;;
  normal) pos=yes ; warn=0 ;;
  normignall) pos=yes ; warn=0 ;;
  nosurv) pos=yes ; warn=0 ;;
  *) file='' ;;
  esac

  if test x"$file" != x ; then
    echo $file
    rm -f "$testdir"/tmp.*
    if test -n "$VERBOSE" ; then
      if test x"$pos" = xfail ; then
        $CAVERN $srcdir/$file.svx --output="$testdir"/tmp
	# success gives 0, signal (128 + <signal number>)
	test $? = 1 || exit 1
      else
        $CAVERN $srcdir/$file.svx --output="$testdir"/tmp || exit 1
      fi
    else
      if test x"$pos" = xfail ; then
        $CAVERN $srcdir/$file.svx --output="$testdir"/tmp > "$testdir"/tmp.out
	# success gives 0, signal (128 + <signal number>)
	test $? = 1 || exit 1
      else
        $CAVERN $srcdir/$file.svx --output="$testdir"/tmp > "$testdir"/tmp.out || exit 1
      fi
    fi
    if test -n "$warn" ; then
      test -f "$testdir"/tmp.out || $CAVERN $srcdir/$file.svx --output="$testdir"/tmp > "$testdir"/tmp.out
      w=`sed '$!d;$s/^Done.*/0/;s/[^0-9]*\([0-9]*\).*/\1/' "$testdir"/tmp.out`
      test x"$w" = x"$warn" || exit 1
    fi
    if test -n "$error" ; then
      test -f "$testdir"/tmp.out || $CAVERN $srcdir/$file.svx --output="$testdir"/tmp > "$testdir"/tmp.out
      e=`sed '$!d;$s/^Done.*/0/;s/[^0-9]*[0-9][0-9]*[^0-9][^0-9]*\([0-9][0-9]*\).*/\1/;s/\(.*[^0-9].*\)/0/' "$testdir"/tmp.out`
      test x"$e" = x"$error" || exit 1
    fi

    case "$pos" in
    yes)
      if test -n "$VERBOSE" ; then
        $DIFFPOS "$testdir"/tmp.3d $srcdir/$file.pos || exit 1
      else
        $DIFFPOS "$testdir"/tmp.3d $srcdir/$file.pos > /dev/null || exit 1
      fi ;;
    no)
      test -f "$testdir"/tmp.3d || exit 1 ;;
    fail)
      test -f "$testdir"/tmp.3d && exit 1 ;;
    *)
      echo "Bad value for pos" ; exit 1 ;;
    esac
    rm -f "$testdir"/tmp.*
  fi
done
exit 0
