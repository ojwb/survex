#!/bin/sh

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

test -x "$testdir"/../src/cavern || testdir=.

: ${DIFFPOS="$testdir"/../src/diffpos}
: ${TDTOPOS="$testdir"/../src/3dtopos}

SURVEXHOME="$srcdir"/../lib
export SURVEXHOME

: ${TESTS=${*-"pos v0 v1 v2 v3"}}

for file in $TESTS ; do
  echo $file
  if test x"$file" = "xpos" ; then
    file="$file".pos
  else
    file="$file".3d
  fi
  rm -f tmp.pos diffpos.tmp
  $TDTOPOS $srcdir/$file tmp.pos || exit 1
  $DIFFPOS $srcdir/$file tmp.pos > diffpos.tmp
  if test -n "$VERBOSE" ; then
    cat diffpos.tmp
  fi
  test -s diffpos.tmp && exit 1
  rm -f tmp.pos diffpos.tmp
done
exit 0
