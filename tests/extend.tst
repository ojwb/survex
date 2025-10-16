#!/bin/sh
#
# Survex test suite - extend tests
# Copyright (C) 1999-2025 Olly Betts
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

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

test -x "$testdir"/../src/cavern || testdir=.

: ${CAVERN="$testdir"/../src/cavern}
: ${EXTEND="$testdir"/../src/extend}
: ${DIFFPOS="$testdir"/../src/diffpos}

: ${TESTS=${*:-"extend extend2names eswap eswap-break"}}

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
  EXTEND="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $EXTEND"
  DIFFPOS="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $DIFFPOS"
fi

for file in $TESTS ; do
  echo $file
  EXTEND_ARGS=""
  test -f "$srcdir/$file.espec" && EXTEND_ARGS="--specfile $srcdir/$file.espec"
  rm -f tmp.*
  if test -n "$VERBOSE" ; then
    $CAVERN "$srcdir/$file.svx" --output=tmp > tmp.stdout
    exitcode=$?
    [ $exitcode = 0 ] || cat tmp.stdout
    rm tmp.stdout
  else
    $CAVERN "$srcdir/$file.svx" --output=tmp > /dev/null
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
    $EXTEND $EXTEND_ARGS tmp.3d tmp.x.3d > tmp.stdout
    exitcode=$?
    [ $exitcode = 0 ] || cat tmp.stdout
    rm tmp.stdout
  else
    $EXTEND $EXTEND_ARGS tmp.3d tmp.x.3d > /dev/null
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
    $DIFFPOS tmp.x.3d "$srcdir/${file}x.3d" > tmp.stdout
    exitcode=$?
    [ $exitcode = 0 ] || cat tmp.stdout
    rm tmp.stdout
  else
    $DIFFPOS tmp.x.3d "$srcdir/${file}x.3d" > /dev/null
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
  rm -f tmp.*
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
