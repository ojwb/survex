#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${DIFFPOS=../src/diffpos}

: ${TESTS=$*}
: ${TESTS="delatend addatend"}

for file in $TESTS ; do
  echo $file
  rm -f diffpos.tmp
  $DIFFPOS $srcdir/${file}a.pos $srcdir/${file}b.pos > diffpos.tmp
  cmp diffpos.tmp $srcdir/${file}.out > /dev/null || exit 1
  rm -f diffpos.tmp
done
exit 0
