#!/bin/sh

# allow us to run tests standalone more easily
if test "x$srcdir" = "x" ; then srcdir=. ; fi

# tests which should fail...
TESTS="begin_no_end end_no_begin end_no_begin_nest"

for file in $TESTS ; do
  echo $file
  rm -f ./tmp.*
  SURVEXHOME=$srcdir/../lib ../src/cavern $srcdir/$file.svx --output=./tmp > /dev/null && exit 1
  rm -f ./tmp.*
done
exit 0
