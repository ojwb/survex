#!/usr/bin/perl -i
require 5.008;
use strict;
use warnings;
use POSIX;

my $pot_creation_date = strftime "%Y-%m-%d %H:%M:%S +0000", gmtime();

while (<>) {
    s/^("PO-Revision-Date:).*(\\n")/$1 $pot_creation_date$2/;
    print;
}
