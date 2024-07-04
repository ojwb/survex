#!/bin/sh
#
# Survex test suite - smoke tests
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

# Ensure that --version and --help work without an X display.
DISPLAY=
export DISPLAY

PROGS="cavern diffpos extend sorterr survexport aven"

# Suppress checking for leaks on exit if we're build with lsan - we don't
# generally waste effort to free all allocations as the OS will reclaim
# memory on exit.
LSAN_OPTIONS=leak_check_at_exit=0
export LSAN_OPTIONS

vgrun=
vg_error=123
vg_log=vg.log
if [ -n "$VALGRIND" ] ; then
  rm -f "$vg_log"
  vgrun="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error"
fi

for p in ${PROGS}; do
  echo $p
  for o in version help ; do
    if test -n "$VERBOSE"; then
      $vgrun "$testdir/../src/$p" --$o
      exitcode=$?
    else
      $vgrun "$testdir/../src/$p" --$o > /dev/null
      exitcode=$?
    fi 2> stderr.log
    if [ -s stderr.log ] ; then
      echo "$p --$o produced output on stderr:"
      cat stderr.log
      rm stderr.log
      exit 1
    fi
    rm stderr.log
    if [ -n "$VALGRIND" ] ; then
      if [ $exitcode = "$vg_error" ] ; then
	cat "$vg_log"
	rm "$vg_log"
	exit 1
      fi
      rm "$vg_log"
    fi
    [ "$exitcode" = 0 ] || exit 1
  done
done

test -n "$VERBOSE" && echo "Test passed"
exit 0
