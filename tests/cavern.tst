#!/bin/sh

for file in oneleg midpoint noose cross ; do
  echo $file
  rm -f ./tmp.*
  $srcdir/../src/cavern $srcdir/$file.svx --output=./tmp > /dev/null
  $srcdir/../src/diffpos tmp.pos $file.pos 0 > /dev/null || exit 1
  rm -f ./tmp.*
done
exit 0
