#!/bin/sh

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
test -n "$*" && VERBOSE=1

: ${DIFFPOS="$testdir"/../src/diffpos}

SURVEXHOME=$srcdir/../lib
export SURVEXHOME

: ${TESTS=${*-"delatend addatend"}}

for file in $TESTS ; do
  echo $file
  rm -f "$testdir"/diffpos.tmp
  $DIFFPOS $srcdir/${file}a.pos $srcdir/${file}b.pos > "$testdir"/diffpos.tmp
  if test -n "$VERBOSE" ; then
    cat "$testdir"/diffpos.tmp
    cmp "$testdir"/diffpos.tmp $srcdir/${file}.out || exit 1
  else
    cmp "$testdir"/diffpos.tmp $srcdir/${file}.out > /dev/null || exit 1
  fi
  rm -f "$testdir"/diffpos.tmp
done
exit 0
