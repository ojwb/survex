#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${CAVERN=../src/cavern}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

# tests which should produce warnings...
# expect one warning unless ":<number>" is appended
TESTS="self_loop self_eq_loop reenterwarn:2"

for t in $TESTS ; do
  file=`echo $t|cut -f 1 -d :`
  count=`echo $t|sed 's/[^:]*:\?//'`
  echo $file
  rm -f ./tmp.*
  
  warns=`$CAVERN $srcdir/$file.svx --output=./tmp | sed '$!d;$s/[^0-9]*\([0-9]*\).*/\1/'`
  test $? = 0 || exit 1
  test x"$warns" = x"${count:-1}" || exit 1
  rm -f ./tmp.*
done
exit 0
