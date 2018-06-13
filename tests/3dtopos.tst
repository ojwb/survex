#!/bin/sh
#
# Survex test suite - 3d to pos tests
# Copyright (C) 1999-2003,2005,2010,2012,2018 Olly Betts
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

: ${DIFFPOS="$testdir"/../src/diffpos}
: ${CAD3D="$testdir"/../src/cad3d}

: ${TESTS=${*:-"pos.pos v0 v0b v1 v2 v3"}}

vg_error=123
vg_log=vg.log
if [ -n "$VALGRIND" ] ; then
  rm -f "$vg_log"
  CAD3D="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $CAD3D"
  DIFFPOS="$VALGRIND --log-file=$vg_log --error-exitcode=$vg_error $DIFFPOS"
fi

for file in $TESTS ; do
  echo $file
  case $file in
  *.pos) input="$srcdir/$file" ;;
  *) input="$srcdir/$file.3d" ;;
  esac
  rm -f tmp.pos diffpos.tmp
  $CAD3D "$input" tmp.pos
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
  $DIFFPOS "$input" tmp.pos > diffpos.tmp
  exitcode=$?
  if test -n "$VERBOSE" ; then
    cat diffpos.tmp
  fi
  if [ -n "$VALGRIND" ] ; then
    if [ $exitcode = "$vg_error" ] ; then
      cat "$vg_log"
      rm "$vg_log"
      exit 1
    fi
    rm "$vg_log"
  fi
  test -s diffpos.tmp && exit 1
  rm -f tmp.pos diffpos.tmp
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
