#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

: ${DIFFPOS=../src/diffpos}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

: ${TESTS=${*-"delatend addatend"}}

for file in $TESTS ; do
  echo $file
  rm -f diffpos.tmp
  $DIFFPOS $srcdir/${file}a.pos $srcdir/${file}b.pos > diffpos.tmp
  test -n "$VERBOSE" && cat diffpos.tmp
  cmp diffpos.tmp $srcdir/${file}.out > /dev/null || exit 1
  rm -f diffpos.tmp
done
exit 0
