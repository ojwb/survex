#!/bin/sh
#
# Survex test suite - cavern tests
# Copyright (C) 1999-2004,2005,2006,2010,2012,2013,2014,2015 Olly Betts
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
 hanging_cpt badinc badinc2 badinc3 badinc4 nonexistent_file ONELEG\
 stnsurvey1 stnsurvey2\
 tapelessthandepth longname chinabug chinabug2\
 multinormal multinormignall multidiving multicylpolar multicartesian\
 multinosurv multinormalbad multibug\
 cmd_title cmd_titlebad cmd_dummy cmd_infer cmd_date cmd_datebad cmd_datebad2\
 cartes diving cylpolar normal normal_bad normignall nosurv cmd_flags\
 bad_cmd_flags plumb unusedstation exportnakedbegin oldestyle bugdz\
 baddatacylpolar badnewline badquantities imgoffbyone infereqtopofil 3sdfixbug\
 omitclino back back2\
 notentranceorexport inferunknown inferexports bad_units_factor\
 bad_units_qlist\
 percent_gradient dotinsurvey leandroclino lowsd revdir gettokennullderef\
 nosurveyhanging cmd_solve_nothing cmd_solve_nothing_implicit\
 cmd_calibrate cmd_declination cmd_declination_auto\
 lech level 2fixbug dot17 3dcorner\
 declination.dat ignore.dat backread.dat nomeasure.dat\
 surfequate passage hanging_lrud equatenosuchstn surveytypo\
 skipafterbadomit passagebad badreadingdotplus badcalibrate calibrate_clino\
 badunits badbegin anonstn anonstnbad anonstnrev doubleinc reenterlots\
 cs csbad csbadsdfix cslonglat omitfixaroundsolve repeatreading\
"}}

# Test file stnsurvey3.svx missing: pos=fail # We exit before the error count.

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
  case $file in
    nonexistent_file*|ONELEG)
      # ONELEG tests that we don't apply special handling to command line
      # arguments, only those in *include.
      realfile= ;;
    *.*) realfile=$file ;;
    *) realfile=$file.svx ;;
  esac

  if [ -n "$realfile" ] && [ ! -r "$realfile" ] ; then
    echo "Warning: don't know how to run test '$file' - skipping it"
    continue
  fi

  echo "$file"

  # how many warnings to expect
  warn=
  # how many errors to expect
  error=

  case $file in
    *.dat)
      # .dat files can't start with a comment.  All the current .dat tests
      # have the same settings.
      pos=yes
      warn=0
      ;;
    nonexistent_file*|ONELEG)
      # We exit before the error count.
      pos=fail
      ;;
    *)
      read header < "$realfile"
      set dummy $header
      while shift && [ -n "$1" ]: ; do
	case $1 in
	  pos=*) pos=`expr "$1" : 'pos=\(.*\)'` ;;
	  warn=*) warn=`expr "$1" : 'warn=\(.*\)'` ;;
	  error=*) error=`expr "$1" : 'error=\(.*\)'` ;;
	esac
      done
      ;;
  esac

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
    w=`sed '$!d;s/^There were \([0-9]*\).*/\1/;s/^[^0-9].*$/0/' tmp.out`
    if test x"$w" != x"$warn" ; then
      test -n "$VERBOSE" && echo "Got $w warnings, expected $warn"
      exit 1
    fi
  fi
  if test -n "$error" ; then
    e=`sed '$!d;s/^There were .* and \([0-9][0-9]*\).*/\1/;s/^[^0-9].*$/0/' tmp.out`
    if test x"$e" != x"$error" ; then
      test -n "$VERBOSE" && echo "Got $e errors, expected $error"
      exit 1
    fi
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
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
