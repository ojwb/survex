#!/bin/sh

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
#test -n "$*" && VERBOSE=1

test -x "$testdir"/../src/cavern || testdir=.

: ${CAVERN="$testdir"/../src/cavern}
: ${CAVERN_ORIG=cavern}
: ${DIFFPOS="$testdir"/../src/diffpos}

: ${TESTS=${*-""}}

for file in $TESTS ; do
  
  if test -n "$file" ; then
    echo "$file"
    rm -f tmp.* tmp_orig.*
    if test -n "$VERBOSE" ; then
      $CAVERN_ORIG $file --output=tmp_orig | tee tmp_orig.out || exit 1
      $CAVERN $file --output=tmp | tee tmp.out || exit 1
    else
      $CAVERN_ORIG $file --output=tmp_orig > tmp_orig.out || exit 1
      $CAVERN $file --output=tmp > tmp.out || exit 1
    fi
    warn_orig=`sed '$!d;$s/^Done.*/0/;s/[^0-9]*\([0-9]*\).*/\1/' tmp_orig.out`
    warn=`sed '$!d;$s/^Done.*/0/;s/[^0-9]*\([0-9]*\).*/\1/' tmp.out`
    if test x"$warn_orig" != x"$warn" ; then
      echo "$CAVERN_ORIG gave $warn_orig warning(s)"
      echo "$CAVERN gave $warn warning(s)"
      exit 1
    fi
    if test -n "$VERBOSE" ; then
      $DIFFPOS tmp.3d tmp_orig.3d || exit 1
    else
      $DIFFPOS tmp.3d tmp_orig.3d > /dev/null || exit 1
    fi
  fi
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
