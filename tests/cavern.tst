#!/bin/sh
#
# Survex test suite - cavern tests
# Copyright (C) 1999-2004,2005,2006,2010,2012,2013,2014 Olly Betts
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

test -x "$testdir"/../src/cavern || testdir=.

# Make testdir absolute, so we can cd before running cavern to get a consistent
# path in diagnostic messages.
testdir=`(cd "$testdir" && pwd)`

: ${CAVERN="$testdir"/../src/cavern}
: ${DIFFPOS="$testdir"/../src/diffpos}
: ${CAD3D="$testdir"/../src/cad3d}

: ${TESTS=${*:-"singlefix singlereffix oneleg midpoint noose cross firststn\
 deltastar deltastar2 bug3 calibrate_tape nosurvey2 cartesian cartesian2\
 lengthunits angleunits cmd_alias cmd_truncate cmd_case cmd_fix cmd_solve\
 cmd_entrance cmd_entrance_bad cmd_sd cmd_sd_bad cmd_fix_bad cmd_set\
 cmd_set_bad beginroot revcomplist break_replace_pfx bug0 bug1 bug2 bug4 bug5\
 expobug require export export2 includecomment\
 self_loop self_eq_loop reenterwarn cmd_default cmd_prefix cmd_prefix_bad\
 singlefixerr singlereffixerr\
 begin_no_end end_no_begin end_no_begin_nest require_fail\
 exporterr1 exporterr2 exporterr3 exporterr4 exporterr5\
 exporterr1b exporterr2b exporterr3b exporterr6 exporterr6b\
 hanging_cpt badinc badinc2 badinc3 badinc4 non_existant_file ONELEG\
 stnsurvey1 stnsurvey2 stnsurvey3\
 tapelessthandepth longname chinabug chinabug2\
 multinormal multinormignall multidiving multicylpolar multicartesian\
 multinosurv multinormalbad multibug\
 cmd_title cmd_titlebad cmd_dummy cmd_infer cmd_date cmd_datebad cmd_datebad2\
 cartes diving cylpolar normal normal_bad normignall nosurv cmd_flags\
 bad_cmd_flags plumb unusedstation exportnakedbegin oldestyle bugdz\
 baddatacylpolar badnewline badquantities imgoffbyone infereqtopofil 3sdfixbug\
 omitclino back notentranceorexport inferunknown inferexports bad_units_factor\
 bad_units_qlist\
 percent_gradient dotinsurvey leandroclino lowsd revdir gettokennullderef\
 nosurveyhanging cmd_solve_nothing cmd_solve_nothing_implicit\
 lech level 2fixbug dot17 3dcorner\
 declination.dat ignore.dat backread.dat nomeasure.dat\
 surfequate passage hanging_lrud equatenosuchstn surveytypo\
 skipafterbadomit passagebad badreadingdotplus badcalibrate calibrate_clino\
 badunits badbegin anonstn anonstnbad anonstnrev doubleinc reenterlots\
 cs csbad omitfixaroundsolve\
"}}

LC_ALL=C
export LC_ALL
SURVEXLANG=en
export SURVEXLANG

vg_error=123
vg_log=$testdir/vg.log
if [ -n "$VALGRIND" ] ; then
  rm -f "$vg_log"
  CAVERN="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $CAVERN"
  DIFFPOS="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $DIFFPOS"
  CAD3D="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $CAD3D"
fi

for file in $TESTS ; do
  # how many warnings to expect
  warn=
  # how many errors to expect
  error=
  case $file in
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
  nosurvey2) pos=yes ; warn=0 ;;
  cartesian) pos=yes ; warn=0 ;;
  cartesian2) pos=yes ; warn=0 ;;
  lengthunits) pos=yes ; warn=0 ;;
  angleunits) pos=yes ; warn=0 ;;
  cmd_alias) pos=yes ;;
  cmd_truncate) pos=yes ; warn=0 ;;
  cmd_case) pos=yes ; warn=0 ;;
  cmd_fix) pos=yes ; warn=1 ;;
  cmd_fix_bad) pos=fail ; error=10 ;;
  cmd_solve) pos=yes ; warn=0 ;;
  cmd_entrance) pos=no ; warn=0 ;;
  cmd_entrance_bad) pos=fail ; warn=0 ; error=1 ;;
  cmd_sd) pos=no ; warn=0 ;;
  cmd_sd_bad) pos=fail ; error=7 ;;
  cmd_set) pos=no ; warn=0 ;;
  cmd_set_bad) pos=fail ; error=8 ;;
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
  self_loop) pos=fail ; warn=0 ; error=1 ;;
  self_eq_loop) pos=no ; warn=1 ;;
  reenterwarn) pos=no ; warn=2 ;;
  cmd_default) pos=no ; warn=3 ;;
  singlereffixerr) pos=no ; warn=0 ;;
  cmd_prefix) pos=no ; warn=1 ;;
  cmd_prefix_bad) pos=fail ; warn=1 ; error=1 ;;
  singlefixerr) pos=no ; warn=1 ;;
  tapelessthandepth) pos=no ; warn=1 ;;
  chinabug2) pos=no ; warn=0 ;;
  longname) pos=no ; warn=0 ;;
  chinabug) pos=fail ;; # We exit before the error count.
  begin_no_end) pos=fail ; error=1 ;;
  end_no_begin) pos=fail ; error=1 ;;
  end_no_begin_nest) pos=fail ; error=2 ;;
  require_fail) pos=fail ; error=1 ;;
  exporterr1) pos=fail ; error=1 ;;
  exporterr2) pos=fail ; error=1 ;;
  exporterr3) pos=fail ; error=1 ;;
  exporterr4) pos=fail ; error=1 ;;
  exporterr5) pos=fail ; error=1 ;;
  exporterr1b) pos=fail ; error=1 ;;
  exporterr2b) pos=fail ; error=1 ;;
  exporterr3b) pos=fail ; error=1 ;;
  exporterr6) pos=fail ; error=1 ;;
  exporterr6b) pos=fail ; error=1 ;;
  hanging_cpt) pos=fail ;; # We exit before the error count.
  badinc) pos=fail ;; # We exit before the error count.
  badinc2) pos=fail ;; # We exit before the error count.
  badinc3) pos=fail ;; # We exit before the error count.
  badinc4) pos=fail ; error=3 ;;
  non_existant_file) pos=fail ;; # We exit before the error count.
  ONELEG) pos=fail ;; # We exit before the error count.
  stnsurvey1) pos=fail ; error=1 ;;
  stnsurvey2) pos=fail ; error=2 ;;
  stnsurvey3) pos=fail ;; # We exit before the error count.
  multinormal) pos=yes ; warn=0 ;;
  multinormignall) pos=yes ; warn=0 ;;
  multidiving) pos=yes ; warn=0 ;;
  multicylpolar) pos=yes ; warn=0 ;;
  multicartesian) pos=yes ; warn=0 ;;
  multinosurv) pos=yes ; warn=0 ;;
  multinormalbad) pos=fail ; error=1 ;;
  multibug) pos=no ; warn=0 ;;
  cmd_title) pos=no ; warn=0 ;;
  cmd_titlebad) pos=fail ; error=4 ;;
  cmd_dummy) pos=no ; warn=0 ;;
  cmd_infer) pos=yes ; warn=0 ;;
  cmd_date) pos=no ; warn=4 ;;
  cmd_datebad) pos=no ; warn=37 ;;
  cmd_datebad2) pos=fail ; warn=20 ; error=2 ;;
  cartes) pos=yes ; warn=0 ;;
  diving) pos=yes ; warn=0 ;;
  cylpolar) pos=yes ; warn=0 ;;
  normal) pos=yes ; warn=0 ;;
  normal_bad) pos=fail ; warn=0 ; error=3 ;;
  normignall) pos=yes ; warn=0 ;;
  nosurv) pos=yes ; warn=0 ;;
  cmd_flags) pos=no ; warn=0 ;;
  bad_cmd_flags) pos=fail ; error=19 ;;
  plumb) pos=yes ; warn=0 ;;
  unusedstation) pos=no ; warn=2 ;;
  oldestyle) pos=no ; warn=1 ;;
  exportnakedbegin) pos=fail ; error=1 ;;
  bugdz) pos=yes ; warn=0 ;;
  baddatacylpolar) pos=fail ; error=1 ;;
  badnewline) pos=fail ; error=2 ;;
  badquantities) pos=fail ; error=11 ;;
  imgoffbyone) pos=yes ;; # don't actually care about coords, just the names
  infereqtopofil) pos=yes ; warn=0 ;;
  3sdfixbug) pos=yes ; warn=0 ;;
  omitclino) pos=yes ; warn=0 ;;
  back) pos=yes; warn=0 ;;
  notentranceorexport) pos=fail; warn=0 ; error=1 ;;
  inferunknown) pos=fail; error=1 ;;
  inferexports) pos=no; warn=0 ;;
  bad_units_factor) pos=fail; error=6 ;;
  bad_units_qlist) pos=fail; error=3 ;;
  percent_gradient) pos=yes; warn=0 ;;
  dotinsurvey) pos=no; warn=2 ;;
  leandroclino) pos=yes; warn=0 ;;
  lowsd) pos=no; warn=0 ;;
  revdir) pos=yes; warn=0 ;;
  gettokennullderef) pos=fail ;; # We exit before the error count.
  lech) pos=no; warn=0 ;;
  level) pos=yes; warn=0 ;;
  2fixbug) pos=no; warn=0 ;;
  declination.dat|ignore.dat|backread.dat|nomeasure.dat) pos=yes; warn=0 ;;
  dot17) pos=yes; warn=0 ;;
  3dcorner) pos=yes; warn=0 ;;
  surfequate) pos=dxf; warn=0 ;;
  nosurveyhanging) pos=fail ;; # We exit before the error count.
  cmd_solve_hanging) pos=fail ; error=1 ;;
  cmd_solve_nothing*) pos=no; warn=0 ;;
  passage) pos=no; warn=0 ;;
  hanging_lrud) pos=fail; error=1 ;;
  equatenosuchstn) pos=fail; error=1 ;;
  surveytypo) pos=fail ;; # Actually, 2 errors, but we exit before the error count.
  skipafterbadomit) pos=fail; error=1 ;;
  passagebad) pos=fail; error=3 ;;
  badreadingdotplus) pos=fail ; error=3 ;;
  badcalibrate) pos=fail; error=3 ;;
  calibrate_clino) pos=yes; warn=8 ;;
  badunits) pos=fail; error=12 ;;
  badbegin) pos=fail; error=2 ;;
  anonstn) pos=yes; warn=0; error=0 ;;
  anonstnbad) pos=fail; warn=1; error=6 ;;
  anonstnrev) pos=yes; warn=0; error=0 ;;
  doubleinc) pos=no; warn=3; error=0 ;;
  reenterlots) pos=no; warn=11; error=0 ;;
  cs) pos=no; warn=0; error=0 ;;
  csbad) pos=fail; warn=0; error=17 ;;
  omitfixaroundsolve) pos=fail; warn=2; error=1 ;;
  *) echo "Warning: don't know how to run test '$file' - skipping it"
     file='' ;;
  esac

  if test -n "$file" ; then
    echo "$file"
    case $file in
    *.*)
      input="./$file"
      posfile="$srcdir"/`echo "$file"|sed 's/\.[^.]*$/.pos/'`
      dxffile="$srcdir"/`echo "$file"|sed 's/\.[^.]*$/.dxf/'` ;;
    *)
      input="./$file.svx"
      posfile="$srcdir/$file.pos"
      dxffile="$srcdir/$file.dxf" ;;
    esac
    rm -f tmp.*
    pwd=`pwd`
    cd "$srcdir"
    srcdir=. $CAVERN "$input" --output="$pwd/tmp" > "$pwd/tmp.out"
    exitcode=$?
    cd "$pwd"
    test -n "$VERBOSE" && cat tmp.out
    if [ -n "$VALGRIND" ] ; then
      if [ $exitcode = "$vg_error" ] ; then
	cat "$vg_log"
	rm "$vg_log"
	exit 1
      fi
      rm "$vg_log"
    fi
    if test fail = "$pos" ; then
      # success gives 0, signal (128 + <signal number>)
      test $exitcode = 1 || exit 1
    else
      test $exitcode = 0 || exit 1
    fi
    if test -n "$warn" ; then
      w=`sed '$!d;$s/^Done.*/0/;s/[^0-9]*\([0-9]*\).*/\1/' tmp.out`
      test x"$w" = x"$warn" || exit 1
    fi
    if test -n "$error" ; then
      e=`sed '$!d;$s/^Done.*/0/;s/[^0-9]*[0-9][0-9]*[^0-9][^0-9]*\([0-9][0-9]*\).*/\1/;s/\(.*[^0-9].*\)/0/' tmp.out`
      test x"$e" = x"$error" || exit 1
    fi
    nan=`sed 's/.*\<[Nn]a[Nn]m\?\>.*/x/p;d' tmp.out`
    if test -n "$nan" ; then
      exit 1
    fi

    case $pos in
    yes)
      if test -n "$VERBOSE" ; then
	$DIFFPOS "$posfile" tmp.3d
	exitcode=$?
      else
	$DIFFPOS "$posfile" tmp.3d > /dev/null
	exitcode=$?
      fi
      if [ -n "$VALGRIND" ] ; then
	if [ $exitcode = "$vg_error" ] ; then
	  cat "$vg_log"
	  rm "$vg_log"
	  exit 1
	fi
	rm "$vg_log"
      fi
      [ "$exitcode" = 0 ] || exit 1
      ;;
    dxf)
      if test -n "$VERBOSE" ; then
	$CAD3D tmp.3d tmp.dxf
	exitcode=$?
      else
	$CAD3D tmp.3d tmp.dxf > /dev/null
	exitcode=$?
      fi
      if [ -n "$VALGRIND" ] ; then
	if [ $exitcode = "$vg_error" ] ; then
	  cat "$vg_log"
	  rm "$vg_log"
	  exit 1
	fi
	rm "$vg_log"
      fi
      [ "$exitcode" = 0 ] || exit 1
      if test -n "$VERBOSE" ; then
	diff "$dxffile" tmp.dxf || exit 1
      else
	cmp -s "$dxffile" tmp.dxf || exit 1
      fi ;;
    no)
      test -f tmp.3d || exit 1 ;;
    fail)
      test -f tmp.3d && exit 1
      # Check that last line doesn't contains "Bug in program detected"
      case `tail -n 1 tmp.out` in
      *"Bug in program detected"*) exit 1 ;;
      esac ;;
    *)
      echo "Bad value for pos: '$pos'" ; exit 1 ;;
    esac

    out=$srcdir/$file.out
    if test -f "$out" ; then
      # Check output is as expected.
      if test -n "$VERBOSE" ; then
	sed '1,/^Copyright/d;/^\(CPU t\|T\)ime used  *[0-9][0-9.]*s$/d;s!.*/src/\(cavern: \)!\1!' tmp.out|diff "$out" - || exit 1
      else
	sed '1,/^Copyright/d;/^\(CPU t\|T\)ime used  *[0-9][0-9.]*s$/d;s!.*/src/\(cavern: \)!\1!' tmp.out|cmp -s "$out" - || exit 1
      fi
    fi
    rm -f tmp.*
  fi
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
