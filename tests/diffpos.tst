#!/bin/sh

# allow us to run tests standalone more easily
if test "x$srcdir" = "x" ; then srcdir=. ; fi

for file in delatend addatend ; do
  echo $file
  rm -f diffpos.tmp
  ../src/diffpos $srcdir/${file}a.pos $srcdir/${file}b.pos > diffpos.tmp
  cmp diffpos.tmp $srcdir/${file}.out > /dev/null || exit 1
  rm -f diffpos.tmp
done
exit 0
