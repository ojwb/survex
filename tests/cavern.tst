#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${CAVERN=../src/cavern}
: ${DIFFPOS=../src/diffpos}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

TESTS="singlefix oneleg midpoint noose cross firststn deltastar\
 deltastar2 bug3 calibrate_tape nosurvey nosurvey2\
 cartesian"

NO_POS_TESTS="beginroot revcomplist break_replace_pfx bug0 bug1 bug2 bug4 bug5\
 expobug require export includecomment"

for file in $TESTS ; do
  echo $file
  rm -f ./tmp.*
  $CAVERN $srcdir/$file.svx --output=./tmp > /dev/null || exit 1
  $DIFFPOS ./tmp.pos $srcdir/$file.pos 0 > /dev/null || exit 1
  rm -f ./tmp.*
done

for file in $NO_POS_TESTS ; do
  echo $file
  rm -f ./tmp.*
  $CAVERN $srcdir/$file.svx --output=./tmp > /dev/null || exit 1
  test -f ./tmp.pos || exit 1
  rm -f ./tmp.*
done
exit 0
