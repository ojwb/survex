#!/usr/bin/perl -w
require 5.004;
use strict;

use integer;

my @ent = ();
for (128..255) {
   $ent[$_] = "#$_";
}

open ENT, "named-entities.txt" or die $!;
while (<ENT>) {
   my ($e, $v) = /^(\w+),(\d+)/;
   $ent[$v] = $e;
}
close ENT;

while (<>) {
   if (/^\s*#/) {
      print;
      next;
   }
   
   my ($pre, $msg) = /^([-\w,]+:\s*\d+\s+)(.*)/;

   $msg =~ s/([\x80-\xff])/"&".$ent[ord($1)].";"/ge;

   $msg =~ s/'\%s'/`%s'/g;
   $msg =~ s/^([^`']* )\%s'/$1`%s'/g;

   print "$pre$msg\n";
}
