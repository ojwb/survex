#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${CAVERN=../src/cavern}
: ${DIFFPOS=../src/diffpos}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

TESTS="oneleg midpoint noose cross firststn break_replace_pfx deltastar\
 deltastar2 bug0 bug1 bug2 bug3 calibrate_tape expobug nosurvey nosurvey2\
 cartesian require"

for file in $TESTS ; do
  echo $file
  rm -f ./tmp.*
  $CAVERN $srcdir/$file.svx --output=./tmp > /dev/null || exit 1
  $DIFFPOS ./tmp.pos $srcdir/$file.pos 0 > /dev/null || exit 1
  rm -f ./tmp.*
done
exit 0
