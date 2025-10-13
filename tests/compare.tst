#!/bin/sh
#
# Survex test suite - compare 2 versions of cavern on a dataset
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}
if [ -z "$SURVEXLIB" ] ; then
  SURVEXLIB=`cd "$srcdir/../lib" && pwd`
  export SURVEXLIB
fi

test -x "$testdir"/../src/cavern || testdir=.

: ${CAVERN="$testdir"/../src/cavern}
: ${CAVERN_ORIG=cavern}
: ${DIFFPOS="$testdir"/../src/diffpos}

: ${TESTS=${*-""}}

# Suppress checking for leaks on exit if we're build with lsan - we don't
# generally waste effort to free all allocations as the OS will reclaim
# memory on exit.
LSAN_OPTIONS=leak_check_at_exit=0
export LSAN_OPTIONS

vg_error=123
vg_log=vg.log
if [ -n "$VALGRIND" ] ; then
  rm -f "$vg_log"
  CAVERN="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $CAVERN"
  DIFFPOS="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $DIFFPOS"
fi

case $TESTS in
  ""|*--help*)
    echo "Usage: $0 SVX_FILE..."
    echo ""
    echo "Process each SVX_FILE in turn with two versions of cavern and compare."
    echo ""
    echo "Cavern versions specified by environment variables:"
    echo ""
    echo "  CAVERN_ORIG (default: $CAVERN_ORIG)"
    echo "  CAVERN (default: $CAVERN)"
    if [ -z "$TESTS" ] ; then
      exit 1
    else
      exit 0
    fi
    ;;
esac

for file in $TESTS ; do
  if test -n "$file" ; then
    echo "$file"
    rm -f tmp.* tmp_orig.*
    if test -n "$VERBOSE" ; then
      { $CAVERN_ORIG "$file" --output=tmp_orig ; exitcode_orig=$? ; } | tee tmp_orig.out
      { $CAVERN "$file" --output=tmp ; exitcode=$? ; } | tee tmp.out
    else
      $CAVERN_ORIG "$file" --output=tmp_orig > tmp_orig.out
      exitcode_orig=$?
      $CAVERN "$file" --output=tmp > tmp.out
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
    diag_orig=`sed 's/^There were \([0-9]* warning(s) and [0-9]* error(s)\).*/\1/p;d' tmp_orig.out`
    diag=`sed 's/^There were \([0-9]* warning(s) and [0-9]* error(s)\).*/\1/p;d' tmp.out`
    if test x"$diag_orig" != x"$diag" ; then
      echo "  $diag_orig from $CAVERN_ORIG"
      echo "  $diag from $CAVERN"
      exit 1
    fi
    [ "$exitcode_orig:$exitcode" = "0:0" ] || exit 1
    if test -n "$VERBOSE" ; then
      $DIFFPOS tmp.3d tmp_orig.3d
      exitcode=$?
    else
      $DIFFPOS tmp.3d tmp_orig.3d > /dev/null
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
  fi
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
