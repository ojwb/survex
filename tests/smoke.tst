#!/bin/sh
set -e

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

test -x "$testdir"/../src/cavern || testdir=.

: ${CAVERN="$testdir"/../src/cavern}
: ${DIFFPOS="$testdir"/../src/diffpos}

PROGS="cad3d cavern printdm printps printpcl printhpgl diffpos extend sorterr\
 3dtopos"
test -r "$testdir"/../src/xcaverot && PROGS="$PROGS xcaverot"
test -r "$testdir"/../src/aven && PROGS="$PROGS aven"
# FIXME: aten and xcaverot where 

for p in ${PROGS}; do
   echo $p
   if test -n "$VERBOSE"; then
      "$testdir"/../src/$p --version
      "$testdir"/../src/$p --help
   else
      "$testdir"/../src/$p --version > /dev/null
      "$testdir"/../src/$p --help > /dev/null
   fi
done
exit 0
