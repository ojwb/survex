#!/bin/sh

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

test -x "$testdir"/../src/cavern || testdir=.

: ${CAVERN="$testdir"/../src/cavern}
: ${EXTEND="$testdir"/../src/extend}
: ${DIFFPOS="$testdir"/../src/diffpos}

SURVEXHOME="$srcdir"/../lib
export SURVEXHOME

: ${TESTS=${*-"extend extend2names"}}

for file in $TESTS ; do
  echo $file
  rm -f tmp.*
  if test -n "$VERBOSE" ; then
    $CAVERN $srcdir/$file.svx --output=tmp || exit 1
    $EXTEND tmp.3d tmp.x.3d || exit 1
    $DIFFPOS tmp.x.3d $srcdir/${file}x.3d || exit 1
  else
    $CAVERN $srcdir/$file.svx --output=tmp > /dev/null || exit 1
    $EXTEND tmp.3d tmp.x.3d > /dev/null || exit 1
    $DIFFPOS tmp.x.3d $srcdir/${file}x.3d > /dev/null || exit 1
  fi
  rm -f tmp.*
done
exit 0
