#!/bin/sh

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

: ${CAVERN="$testdir"/../src/cavern}
: ${EXTEND="$testdir"/../src/extend}
: ${DIFFPOS="$testdir"/../src/diffpos}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

: ${TESTS=${*-"extend"}}

for file in $TESTS ; do
  echo $file
  rm -f "$testdir"/tmp.*
  if test -n "$VERBOSE" ; then
    $CAVERN $srcdir/$file.svx --output="$testdir"/tmp || exit 1
    $EXTEND "$testdir"/tmp.3d "$testdir"/tmp.x.3d || exit 1
    $DIFFPOS "$testdir"/tmp.x.3d $srcdir/${file}x.3d || exit 1
  else
    $CAVERN $srcdir/$file.svx --output="$testdir"/tmp > /dev/null || exit 1
    $EXTEND "$testdir"/tmp.3d "$testdir"/tmp.x.3d > /dev/null || exit 1
    $DIFFPOS "$testdir"/tmp.x.3d $srcdir/${file}x.3d > /dev/null || exit 1
  fi
  rm -f "$testdir"/tmp.*
done
exit 0
