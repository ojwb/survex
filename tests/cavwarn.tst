#!/bin/sh

# allow us to run tests standalone more easily
: ${srcdir=.}

: ${CAVERN=../src/cavern}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

# tests which should produce warnings...
: ${TESTS="self_loop self_eq_loop reenterwarn cmd_default cmd_prefix"}

for file in $TESTS ; do
  echo $file

  # how many warnings to expect
  count=1
  case "$file" in
  reenterwarn) count=2 ;;
  cmd_default) count=3 ;;
  esac

  rm -f ./tmp.*
  
  warns=`$CAVERN $srcdir/$file.svx --output=./tmp | sed '$!d;$s/[^0-9]*\([0-9]*\).*/\1/'`
  test $? = 0 || exit 1
  test x"$warns" = x"${count:-1}" || exit 1
  rm -f ./tmp.*
done
exit 0
