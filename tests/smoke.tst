#!/bin/sh
#
# Survex test suite - smoke tests
# Copyright (C) 1999-2003,2005,2011 Olly Betts
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
set -e

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

test -x "$testdir"/../src/cavern || testdir=.

: ${CAVERN="$testdir"/../src/cavern}
: ${DIFFPOS="$testdir"/../src/diffpos}

# Ensure that --version and --help work without an X display.
DISPLAY=
export DISPLAY

PROGS="cad3d cavern diffpos extend sorterr 3dtopos aven"

for p in ${PROGS}; do
   echo $p
   if test -n "$VERBOSE"; then
      "$testdir/../src/$p" --version
      "$testdir/../src/$p" --help
   else
      "$testdir/../src/$p" --version > /dev/null
      "$testdir/../src/$p" --help > /dev/null
   fi
done
exit 0
