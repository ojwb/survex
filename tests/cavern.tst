#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${CAVERN=../src/cavern}
: ${DIFFPOS=../src/diffpos}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

TESTS="oneleg midpoint noose cross firststn break_replace_pfx deltastar\
 deltastar2 bug0 bug1 bug2 bug3 bug4 calibrate_tape expobug nosurvey nosurvey2\
 cartesian require beginroot"

for file in $TESTS ; do
  echo $file
  rm -f ./tmp.*
  echo $CAVERN $srcdir/$file.svx --output=./tmp  || exit 1
  $CAVERN $srcdir/$file.svx --output=./tmp  || exit 1
  echo $DIFFPOS ./tmp.pos $srcdir/$file.pos 0  || exit 1
  $DIFFPOS ./tmp.pos $srcdir/$file.pos 0  || exit 1
  rm -f ./tmp.*
done
exit 0
