#!/bin/sh
#
# Survex test suite - diffpos tests
# Copyright (C) 1999-2003,2010 Olly Betts
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

: ${TESTS=${*:-"delatend addatend"}}

for file in $TESTS ; do
  echo $file
  rm -f diffpos.tmp
  $DIFFPOS "$srcdir/${file}a.pos" "$srcdir/${file}b.pos" > diffpos.tmp
  if test -n "$VERBOSE" ; then
    cat diffpos.tmp
    cmp diffpos.tmp "$srcdir/${file}.out" || exit 1
  else
    cmp diffpos.tmp "$srcdir/${file}.out" > /dev/null || exit 1
  fi
  rm -f diffpos.tmp
done

for args in '' '--survey survey' '--survey survey.xyzzy' '--survey xyzzy' ; do
  echo "diffpos $args"
  rm -f diffpos.tmp
  $DIFFPOS $args "$srcdir/v0.3d" "$srcdir/v0.3d" > diffpos.tmp
  if test -n "$VERBOSE" ; then
    cat diffpos.tmp
  fi
  cmp diffpos.tmp /dev/null > /dev/null || exit 1
  rm -f diffpos.tmp
done
  
exit 0
