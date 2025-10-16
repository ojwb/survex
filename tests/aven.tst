#!/bin/sh
#
# Survex test suite - aven tests
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}
if [ -z "$SURVEXLIB" ] ; then
  SURVEXLIB=`cd "$srcdir/../lib" && pwd`
  export SURVEXLIB
fi

test -x "$testdir"/../src/cavern || testdir=.

: ${AVEN="$testdir"/../src/aven}

# Suppress checking for leaks on exit if we're build with lsan - we don't
# generally waste effort to free all allocations as the OS will reclaim
# memory on exit.
LSAN_OPTIONS=leak_check_at_exit=0
export LSAN_OPTIONS

vg_error=123
vg_log=vg.log
if [ -n "$VALGRIND" ] ; then
  rm -f "$vg_log"
  AVEN="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $AVEN"
fi

# This next testcase seems to hang on macos and mingw in CI so skip it.
# FIXME: Ideally this should work, and it doesn't seem very different to the
# next testcase which works.
case `uname -s` in
  Darwin) ;;
  MINGW*) ;;
  *)
    # Regression test - aven in 1.2.6 segfaulted.
    echo "SURVEXLANG=nosuch aven --help"
    if test -n "$VERBOSE"; then
      DISPLAY= SURVEXLANG=nosuch $AVEN --help > tmp.out 2>&1
      exitcode=$?
      [ $exitcode = 0 ] || cat tmp.out
      rm tmp.out
    else
      DISPLAY= SURVEXLANG=nosuch $AVEN --help > /dev/null 2>&1
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
    [ "$exitcode" = 1 ] || exit 1
    ;;
esac

# Regression test - aven in 1.2.6 segfaulted.
echo "SURVEXLANG= LANG=nosuch aven --help"
if test -n "$VERBOSE"; then
  DISPLAY= SURVEXLANG= LANG=nosuch $AVEN --help > tmp.out 2>&1
  exitcode=$?
  [ $exitcode = 0 ] || cat tmp.out
  rm tmp.out
else
  DISPLAY= SURVEXLANG= LANG=nosuch $AVEN --help > /dev/null 2>&1
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

test -n "$VERBOSE" && echo "Test passed"
exit 0
