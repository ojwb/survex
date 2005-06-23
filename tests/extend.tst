#!/bin/sh
#
# Survex test suite - extend tests
# Copyright (C) 1999-2003 Olly Betts
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

test -x "$testdir"/../src/cavern || testdir=.

: ${CAVERN="$testdir"/../src/cavern}
: ${EXTEND="$testdir"/../src/extend}
: ${DIFFPOS="$testdir"/../src/diffpos}

: ${TESTS=${*-"extend extend2names eswap"}} # also eswap-break
# JPNP: eswap-break currently fails due to bug in diffpos

for file in $TESTS ; do
  echo $file
  EXTEND_ARGS=""
  test -f "$file.espec" && EXTEND_ARGS="--specfile $file.espec"
  rm -f tmp.*
  if test -n "$VERBOSE" ; then
    $CAVERN "$srcdir/$file.svx" --output=tmp || exit 1
    $EXTEND $EXTEND_ARGS tmp.3d tmp.x.3d || exit 1
    $DIFFPOS tmp.x.3d "$srcdir/${file}x.3d" || exit 1
  else
    $CAVERN "$srcdir/$file.svx" --output=tmp > /dev/null || exit 1
    $EXTEND $EXTEND_ARGS tmp.3d tmp.x.3d > /dev/null || exit 1
    $DIFFPOS tmp.x.3d "$srcdir/${file}x.3d" > /dev/null || exit 1
  fi
  rm -f tmp.*
done
exit 0
