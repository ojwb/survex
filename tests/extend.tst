#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${CAVERN=../src/cavern}
: ${EXTEND=../src/extend}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

TESTS="extend"

for file in $TESTS ; do
  echo $file
  rm -f ./tmp.*
  $CAVERN $srcdir/$file.svx --output=./tmp > /dev/null || exit 1
  $EXTEND ./tmp.3d ./tmp.x3d > /dev/null || exit 1
  sed '1,4d' < ./tmp.x3d | cmp - ${file}x.3d > /dev/null || exit 1
  rm -f ./tmp.*
done
exit 0
