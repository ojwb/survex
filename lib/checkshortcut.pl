#!/usr/bin/perl -w
use strict;
my %menu = ();
open I, "<../src/mainfrm.cc" or die $!;
while (<I>) {
    /(\w+)->Append\(.*\(\/\*.*?\*\/(\d+)/ && push @{$menu{$1}}, $2;
}
close I;

for my $lang (@ARGV) {
    print "Lang $lang:\n";
    open L, "<$lang.msg" or die $!;
    my $buf;
    read L, $buf, 20;
    read L, $buf, 999999;
    close L;
    my @msg = split /\0/, $buf;
    for my $menu (sort keys %menu) {
	my %sc = ();
	my $bad = 0;
	for (@{$menu{$menu}}) {
            my ($acc) = ($msg[$_] =~ /\@(.)/);
	    $acc = lc $acc;
	    if (exists $sc{$acc}) {
		print "Menu $menu : '$msg[$sc{$acc}]' and '$msg[$_]' clash\n";
		$bad = 1;
	    } else {
		$sc{$acc} = $_;
	    }
	}
	if ($bad) {
	    print "Unused letters: ", grep {!exists $sc{$_}} ('a' .. 'z'), "\n";
	}
    }
    print "\n";
}

