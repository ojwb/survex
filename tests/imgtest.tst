#!/bin/sh
#
# Survex test suite - test using img library non-hosted
# Copyright (C) 2020,2021 Olly Betts
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

# Make testdir absolute, so we can cd before running cavern to get a consistent
# path in diagnostic messages.
testdir=`(cd "$testdir" && pwd)`

: ${CAVERN="$testdir"/../src/cavern}
: ${IMGTEST="$testdir"/../src/imgtest}

: ${TESTS=${*:-"simple survey"}}

vg_error=123
vg_log=$testdir/vg.log
if [ -n "$VALGRIND" ] ; then
  rm -f "$vg_log"
  CAVERN="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $CAVERN"
  IMGTEST="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $IMGTEST"
fi

for test in $TESTS ; do
  echo $test
  file=imgtest_$test
  rm -f "$file.3d" "$file.err" cavern.tmp imgtest.tmp
  pwd=`pwd`
  cd "$srcdir"
  srcdir=. $CAVERN "$file.svx" --output="$pwd/$file" > "$pwd/cavern.tmp" 2>&1
  exitcode=$?
  cd "$pwd"
  test -n "$VERBOSE" && cat cavern.tmp
  if [ -n "$VALGRIND" ] ; then
    if [ $exitcode = "$vg_error" ] ; then
      cat "$vg_log"
      rm "$vg_log"
      exit 1
    fi
    rm "$vg_log"
  fi
  test $exitcode = 0 || exit 1

  args=
  case $test in
    survey)
	args=svy ;;
  esac

  $IMGTEST "$file.3d" $args > imgtest.tmp 2>&1
  exitcode=$?
  if test -n "$VERBOSE" ; then
    cat imgtest.tmp
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

  rm -f "$file.3d" "$file.err" cavern.tmp imgtest.tmp
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
