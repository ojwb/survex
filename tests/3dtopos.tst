#!/bin/sh
#
# Survex test suite - 3dtopos tests
# Copyright (C) 1999-2003,2005,2010 Olly Betts
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

: ${DIFFPOS="$testdir"/../src/diffpos}
: ${TDTOPOS="$testdir"/../src/3dtopos}

: ${TESTS=${*:-"pos.pos v0 v0b v1 v2 v3"}}

for file in $TESTS ; do
  echo $file
  case $file in
  *.pos) input="$srcdir/$file" ;;
  *) input="$srcdir/$file.3d" ;;
  esac
  rm -f tmp.pos diffpos.tmp
  $TDTOPOS "$input" tmp.pos || exit 1
  $DIFFPOS "$input" tmp.pos > diffpos.tmp
  if test -n "$VERBOSE" ; then
    cat diffpos.tmp
  fi
  test -s diffpos.tmp && exit 1
  rm -f tmp.pos diffpos.tmp
done
exit 0
