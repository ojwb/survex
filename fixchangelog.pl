#!/usr/bin/perl -w
require 5.8;
use bytes;
use strict;

my $tag = $ARGV[0];
while (1) {
    my $x = '';
    my $sym_names = 0;
    my $ok = 0;
    while (<STDIN>) {
	$x .= $_;
	if ($sym_names) {
	    if (/^\s+([^:]*): /) {
		$ok = 1 if ($1 eq $tag);
	    } else {
		$sym_names = 0;
	    }
	}
	if (/^symbolic names:$/) {
	    $sym_names = 1; 
	}
	
	last if ($_ eq ('='x 77)."\n");
    }
    $_ = <STDIN>;
    $x .= $_ if defined $_;
    print $x if $ok;
    last if !defined $_;
}
