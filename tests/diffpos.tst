#!/bin/sh

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

test -x "$testdir"/../src/cavern || testdir=.

: ${DIFFPOS="$testdir"/../src/diffpos}

SURVEXHOME="$srcdir"/../lib
export SURVEXHOME

: ${TESTS=${*-"delatend addatend"}}

for file in $TESTS ; do
  echo $file
  rm -f diffpos.tmp
  $DIFFPOS $srcdir/${file}a.pos $srcdir/${file}b.pos > diffpos.tmp
  if test -n "$VERBOSE" ; then
    cat diffpos.tmp
    cmp diffpos.tmp $srcdir/${file}.out || exit 1
  else
    cmp diffpos.tmp $srcdir/${file}.out > /dev/null || exit 1
  fi
  rm -f diffpos.tmp
done

for args in '' '--survey survey' '--survey survey.xyzzy' '--survey xyzzy' ; do
  echo "diffpos $args"
  rm -f diffpos.tmp
  $DIFFPOS $args $srcdir/v0.3d $srcdir/v0.3d > diffpos.tmp
  if test -n "$VERBOSE" ; then
    cat diffpos.tmp
  fi
  cmp diffpos.tmp /dev/null > /dev/null || exit 1
  rm -f diffpos.tmp
done
  
exit 0
