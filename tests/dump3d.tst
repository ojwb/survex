#!/bin/sh
#
# Survex test suite - dump3d tests
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

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

test -x "$testdir"/../src/cavern || testdir=.

: ${DUMP3D="$testdir"/../src/dump3d}

: ${TESTS=${*:-"cmapstn.adj cmap.sht multisection.plt multisurvey.plt pre1970.plt"}}

# Suppress checking for leaks on exit if we're build with lsan - we don't
# generally waste effort to free all allocations as the OS will reclaim
# memory on exit.
LSAN_OPTIONS=leak_check_at_exit=0
export LSAN_OPTIONS

vg_error=123
vg_log=vg.log
if [ -n "$VALGRIND" ] ; then
  rm -f "$vg_log"
  DUMP3D="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $DUMP3D"
fi

for file in $TESTS ; do
  echo $file
  case $file in
  *.*)
    input="$srcdir/$file"
    expect=$srcdir/`echo "$file"|sed 's/\.[^.]*$//'`.dump
    ;;
  *)
    input="$srcdir/$file.3d"
    expect="$srcdir/$file.dump"
    ;;
  esac
  rm -f tmp.diff tmp.dump
  $DUMP3D --show-dates --legs "$input" > tmp.dump
  exitcode=$?
  if [ -n "$VALGRIND" ] ; then
    if [ $exitcode = "$vg_error" ] ; then
      cat "$vg_log"
      rm "$vg_log"
      exit 1
    fi
    rm "$vg_log"
  fi
  test $exitcode = 0 || exit 1
  diff tmp.dump "$expect" > tmp.diff
  exitcode=$?
  if test -n "$VERBOSE" ; then
    cat tmp.diff
  fi
  if [ -n "$VALGRIND" ] ; then
    if [ $exitcode = "$vg_error" ] ; then
      cat "$vg_log"
      rm "$vg_log"
      exit 1
    fi
    rm "$vg_log"
  fi
  test $exitcode = 0 || exit 1
  rm -f tmp.diff tmp.dump
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
