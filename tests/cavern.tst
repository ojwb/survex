#!/bin/sh
#
# Survex test suite - cavern tests
# Copyright (C) 1999-2024 Olly Betts
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

test -x "$testdir"/../src/cavern || testdir=.

# Make testdir absolute, so we can cd before running cavern to get a consistent
# path in diagnostic messages.
testdir=`cd "$testdir" && pwd`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}
if [ -z "$SURVEXLIB" ] ; then
  SURVEXLIB=`cd "$srcdir/../lib" && pwd`
  export SURVEXLIB
fi

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

case `uname -a` in
  MINGW*)
    DIFF='diff --strip-trailing-cr'
    QUIET_DIFF='diff -q --strip-trailing-cr'
    ;;
  *)
    DIFF=diff
    # Use cmp when we can as a small optimisation.
    QUIET_DIFF='cmp -s'
    ;;
esac

: ${CAVERN="$testdir"/../src/cavern}
: ${DIFFPOS="$testdir"/../src/diffpos}
: ${DUMP3D="$testdir"/../src/dump3d}
: ${SURVEXPORT="$testdir"/../src/survexport}

: ${TESTS=${*:-"singlefix singlereffix oneleg midpoint noose cross firststn\
 deltastar deltastar2 bug3 calibrate_tape nosurvey2 cartesian cartesian2\
 lengthunits angleunits cmd_alias cmd_alias_bad cmd_truncate cmd_truncate_bad\
 cmd_case cmd_case_bad cmd_fix\
 cmd_solve cmd_entrance cmd_entrance_bad cmd_sd cmd_sd_bad cmd_fix_bad cmd_set\
 cmd_set_bad cmd_set_dot_in_name\
 beginroot revcomplist break_replace_pfx bug0 bug1 bug2 bug4 bug5\
 expobug require export export2 includecomment\
 self_loop self_eq_loop reenterwarn cmd_default cmd_default_bad\
 cmd_prefix cmd_prefix_bad\
 cmd_begin_bad cmd_equate_bad cmd_export_bad\
 singlefixerr singlereffixerr\
 begin_no_end end_no_begin end_no_begin_nest require_fail\
 exporterr1 exporterr2 exporterr3 exporterr4 exporterr5\
 exporterr1b exporterr2b exporterr3b exporterr6 exporterr6b\
 hanging_cpt badinc badinc2 badinc3 badinc4 badinc5.mak nonexistent_file ONELEG\
 stnsurvey1 stnsurvey2\
 tapelessthandepth longname chinabug chinabug2\
 multinormal multinormignall multidiving multicylpolar multicartesian\
 multinosurv multinormalbad multibug\
 cmd_title cmd_titlebad cmd_dummy cmd_infer cmd_date cmd_datebad cmd_datebad2\
 cartes diving cylpolar normal normal_bad normignall nosurv cmd_flags\
 bad_cmd_flags plumb unusedstation exportnakedbegin oldestyle bugdz\
 baddatacylpolar badnewline badquantities imgoffbyone infereqtopofil 3sdfixbug\
 omitclino back back2 bad_back\
 notentranceorexport inferunknown inferexports bad_units_factor\
 bad_units_qlist\
 percent_gradient dotinsurvey leandroclino lowsd revdir gettokennullderef\
 nosurveyhanging cmd_solve_nothing cmd_solve_nothing_implicit\
 cmd_cartesian cmd_cartesian_bad\
 cmd_calibrate cmd_declination cmd_declination_auto cmd_declination_auto_bad\
 cmd_declination_conv cmd_declination_conv_proj_bug\
 lech level 2fixbug dot17 3dcorner\
 unconnected-bug\
 backread.dat corrections.dat depthguage.dat flags.dat karstcompat.dat\
 lrud.dat nomeasure.dat noteam.dat\
 badmak.mak\
 fixfeet.mak utm.mak\
 clptest.dat clptest.clp\
 walls.srv\
 badopts.srv\
 wallsbaddatum.wpj\
 wallsdecl.wpj\
 passage hanging_lrud equatenosuchstn surveytypo\
 skipafterbadomit passagebad badreadingdotplus badcalibrate calibrate_clino\
 badunits badbegin anonstn anonstnbad anonstnrev doubleinc reenterlots\
 cs csbad csbadsdfix csfeet cslonglat omitfixaroundsolve repeatreading\
 mixedeols utf8bom nonewlineateof suspectreadings cmd_data_default\
 cmd_data_ignore\
 quadrant_bearing bad_quadrant_bearing\
 samename tabinhighlight legacytokens\
 3dexport \
 dxffullcoords dxfsurfequate\
 gpxexport hpglexport jsonexport kmlexport pltexport svgexport\
"}}

# Test file stnsurvey3.svx missing: pos=fail # We exit before the error count.

LC_ALL=C
export LC_ALL
SURVEXLANG=en
export SURVEXLANG

# Suppress checking for leaks on exit if we're build with lsan - we don't
# generally waste effort to free all allocations as the OS will reclaim
# memory on exit.
LSAN_OPTIONS=leak_check_at_exit=0
export LSAN_OPTIONS

# Allow datestamps in 3d files (we normalise the expected output for GPX
# etc) to allow for the datestamp not being fixed, but under SOURCE_DATE_EPOCH
# the datestamp is omitted entirely which would break those testcases.
unset SOURCE_DATE_EPOCH

vg_error=123
vg_log=$testdir/vg.log
if [ -n "$VALGRIND" ] ; then
  rm -f "$vg_log"
  CAVERN="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $CAVERN"
  DIFFPOS="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $DIFFPOS"
  DUMP3D="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $DUMP3D"
  SURVEXPORT="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $SURVEXPORT"
fi

for file in $TESTS ; do
  case $file in
    nonexistent_file*|ONELEG)
      # ONELEG tests that we don't apply special handling to command line
      # arguments, only those in *include.
      realfile= ;;
    *.*) realfile=$srcdir/$file ;;
    *) realfile=$srcdir/$file.svx ;;
  esac

  if [ x"$file" = xONELEG ] && [ -f "ONELEG.SVX" ] ; then
    echo "Case insensitive filing system - skipping ONELEG testcase"
    continue
  fi

  if [ -n "$realfile" ] && [ ! -r "$realfile" ] ; then
    echo "Don't know how to run test '$file'"
    exit 1
  fi

  echo "$file"

  # how many warnings to expect (or empty not to check)
  warn=

  # how many errors to expect (or empty not to check)
  error=

  # One of:
  # yes : diffpos 3D file output with <testcase_name>.pos
  # no : Check that a 3D file is produced, but not positions in it
  # fail : Check that a 3D file is NOT produced
  # 3d : Convert to 3D with survexport, compare dump3d to <testcase_name>.dump
  # dxf : Convert to DXF with survexport and compare with <testcase_name>.dxf
  # gpx : Convert to GPX with survexport and compare with <testcase_name>.gpx
  # json : Convert to JSON with survexport and compare with <testcase_name>.json
  # kml : Convert to KML with survexport and compare with <testcase_name>.kml
  # plt : Convert to PLT with survexport and compare with <testcase_name>.plt
  # svg : Convert to SVG with survexport and compare with <testcase_name>.svg
  pos=

  case $file in
    backread.dat|clptest.dat|clptest.clp|depthguage.dat|flags.dat|karstcompat.dat)
      pos=dump
      warn=0
      ;;
    *.dat)
      # .dat files can't start with a comment.  All the other .dat tests
      # have the same settings.
      pos=yes
      warn=0
      ;;
    nonexistent_file*|ONELEG)
      # These testcase files don't exist (or for ONELEG exist with a different
      # case).  They all have the same settings.
      pos=fail
      ;;
    wallsbaddatum.wpj)
      # .wpj files can't start with a comment.
      pos=fail
      warn=0
      err=1
      ;;
    *.wpj)
      # .wpj files can't start with a comment.
      pos=dump
      warn=0
      ;;
    *)
      survexportopts=
      read header < "$realfile"
      set dummy $header
      while shift && [ -n "$1" ] ; do
	case $1 in
	  pos=*) pos=`expr "$1" : 'pos=\(.*\)'` ;;
	  warn=*) warn=`expr "$1" : 'warn=\(.*\)'` ;;
	  error=*) error=`expr "$1" : 'error=\(.*\)'` ;;
	  survexportopt=*)
	    survexportopts="$survexportopts "`expr "$1" : 'survexportopt=\(.*\)'`
	    ;;
	esac
      done
      ;;
  esac

  basefile=$srcdir/$file
  case $file in
  *.*)
    input="./$file"
    basefile=`echo "$basefile"|sed 's/\.[^.]*$//'` ;;
  *)
    input="./$file.svx" ;;
  esac
  outfile=$basefile.out
  outfile2=$basefile.altout
  posfile=$basefile.pos
  rm -f tmp.*
  pwd=`pwd`
  cd "$srcdir"
  srcdir=. SOURCE_DATE_EPOCH=1 $CAVERN "$input" --output="$pwd/tmp" > "$pwd/tmp.out"
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
    w=`sed '$!d;s/^There were \([0-9]*\).*/\1/p;d' tmp.out`
    if test x"${w:-0}" != x"$warn" ; then
      test -n "$VERBOSE" && echo "Got $w warnings, expected $warn"
      exit 1
    fi
  fi
  if test -n "$error" ; then
    e=`sed '$!d;s/^There were .* and \([0-9][0-9]*\).*/\1/p;d' tmp.out`
    if test x"${e:-0}" != x"$error" ; then
      test -n "$VERBOSE" && echo "Got $e errors, expected $error"
      exit 1
    fi
  fi
  # Fail if nan, NaN, etc in output (which might be followed by m for metres or
  # s for seconds).
  if egrep -q '(^|[^A-Za-z0-9])nan[ms]?($|[^A-Za-z0-9])' tmp.out ; then
    echo "Not-a-number appears in output"
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
  dump)
    expectedfile=$basefile.dump
    tmpfile=tmp.dump
    $DUMP3D --show-dates --legs tmp.3d > "$tmpfile"
    exitcode=$?
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
      $DIFF "$expectedfile" "$tmpfile" || exit 1
    else
      $QUIET_DIFF "$expectedfile" "$tmpfile" || exit 1
    fi
    ;;
  dxf|gpx|hpgl|json|kml|plt|svg)
    # $pos gives us the file extension here.
    expectedfile=$basefile.$pos
    tmpfile=tmp.$pos
    if test -n "$VERBOSE" ; then
      $SURVEXPORT --defaults$survexportopts tmp.3d "$tmpfile"
      exitcode=$?
    else
      $SURVEXPORT --defaults$survexportopts tmp.3d "$tmpfile" > /dev/null
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

    # Normalise exported file if required.
    case $pos in
      dxf)
	# On x86 excess precision can result in -0.00 for some coordinates.
	sed 's/-0\.00\>/ 0.00/g' < "$tmpfile" > tmp.tmp
	mv tmp.tmp "$tmpfile"
	;;
      json)
	# On x86 excess precision can result in -0.00 for some coordinates.
	sed 's/-0\.00\>/0.00/g' < "$tmpfile" > tmp.tmp
	mv tmp.tmp "$tmpfile"
	;;
      gpx)
	sed 's,<time>[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]Z</time>,<time>REDACTED</time>,;s,survex [0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*,survex REDACTED,' < "$tmpfile" > tmp.tmp
	mv tmp.tmp "$tmpfile"
	;;
    esac

    if test -n "$VERBOSE" ; then
      $DIFF "$expectedfile" "$tmpfile" || exit 1
    else
      $QUIET_DIFF "$expectedfile" "$tmpfile" || exit 1
    fi
    ;;
  3d)
    expectedfile=$basefile.dump
    tmpfile=tmp.dump
    if test -n "$VERBOSE" ; then
      SOURCE_DATE_EPOCH=1 $SURVEXPORT --defaults$survexportopts tmp.3d "$tmpfile.3d"
      exitcode=$?
    else
      SOURCE_DATE_EPOCH=1 $SURVEXPORT --defaults$survexportopts tmp.3d "$tmpfile.3d" > /dev/null
      exitcode=$?
    fi
    $DUMP3D --show-dates --legs "$tmpfile.3d" > "$tmpfile"
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
      $DIFF "$expectedfile" "$tmpfile" || exit 1
    else
      $QUIET_DIFF "$expectedfile" "$tmpfile" || exit 1
    fi
    ;;
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

  if test -f "$outfile" ; then
    # Version and time used info from output, working around Apple's stone-age
    # sed.
    sed '1,/^Copyright/d;/^\(CPU \)*[Tt]ime used  *[0-9][0-9.]*s$/d;s!.*/src/\(cavern: \)!\1!' tmp.out > tmp.out2
    mv tmp.out2 tmp.out
    # Check output is as expected.
    if $QUIET_DIFF "$outfile" tmp.out ; then
      : # Matches.
    elif [ -f "$outfile2" ] && $QUIET_DIFF "$outfile2" tmp.out ; then
      : # Matches alternative output (e.g. due to older PROJ).
    else
      test -z "$VERBOSE" || $DIFF "$outfile" tmp.out
      exit 1
    fi
  fi
  rm -f tmp.*
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
