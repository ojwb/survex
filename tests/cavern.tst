#!/bin/sh

# allow us to run tests standalone more easily
if test "x$srcdir" = "x" ; then srcdir=. ; fi

TESTS="oneleg midpoint noose cross break_replace_pfx deltastar deltastar2\
 bug0 bug1 bug2 bug3"

for file in $TESTS ; do
  echo $file
  rm -f ./tmp.*
  SURVEXHOME=$srcdir/../lib ../src/cavern $srcdir/$file.svx --output=./tmp > /dev/null || exit 1
  SURVEXHOME=$srcdir/../lib ../src/diffpos ./tmp.pos $srcdir/$file.pos 0 > /dev/null || exit 1
  rm -f ./tmp.*
done
exit 0
