#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${CAVERN=../src/cavern}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

# tests which should fail...
TESTS="begin_no_end end_no_begin end_no_begin_nest require_fail\
 exporterr1 exporterr2 exporterr3 exporterr4 exporterr5"

for file in $TESTS ; do
  echo $file
  rm -f ./tmp.*
  $CAVERN $srcdir/$file.svx --output=./tmp > /dev/null && exit 1
  rm -f ./tmp.*
done
exit 0
