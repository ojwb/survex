#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${CAVERN=../src/cavern}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

# tests which should fail...
: ${TESTS="begin_no_end end_no_begin end_no_begin_nest require_fail\
 exporterr1 exporterr2 exporterr3 exporterr4 exporterr5\
 exporterr1b exporterr2b exporterr3b exporterr6 exporterr6b\
 hanging_cpt badinc badinc2 non_existant_file\
 stnsurvey1 stnsurvey2 stnsurvey3"}

for file in $TESTS ; do
  echo $file
  rm -f ./tmp.*
  $CAVERN $srcdir/$file.svx --output=./tmp > /dev/null && exit 1
  rm -f ./tmp.*
done
exit 0
