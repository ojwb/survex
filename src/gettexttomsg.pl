#!/usr/bin/perl -w

#  gettexttomsg.pl
#
#  Copyright (C) 2001,2002,2005,2011,2012,2014,2015 Olly Betts
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

require 5.008;
use strict;

my %revmsgs = ();

my $msgno;
open MSG, "../lib/survex.pot" or die $!;
while (<MSG>) {
    if (/^#: n:(\d+)$/) {
	$msgno = $1;
    } elsif (defined $msgno && /^msgid\s*"(.*)"/) {
	$revmsgs{$1} = $msgno;
	$msgno = undef;
    }
}
close MSG;

my $die = 0;

my $suppress_argc_argv_unused_warnings = 0;
my @conditionals;
my %macro;
# Helper so we can just eval preprocessor expressions.
sub pp_defined { exists $macro{$_[0]} // 0 }

sub pp_eval {
    local $_ = shift;
    # defined is a built-in function in Perl.
    s/\bdefined\b/pp_defined/g;
    no strict 'subs';
    no warnings;
    return eval($_);
}

my $active = 1;
while (<>) {
    if (m!^#\s*(\w+)\s*(.*?)\s*(?:/[/*].*)?$!) {
	my ($directive, $cond) = ($1, $2);
	if ($directive eq 'endif') {
	    if (@conditionals == 0) {
		die "$ARGV:$.: Unmatched #endif\n";
	    }
	    $active = pop @conditionals;
	} elsif ($directive eq 'else') {
	    if (@conditionals == 0) {
		die "$ARGV:$.: Unmatched #else\n";
	    }
	    $active = (!$active) && $conditionals[-1];
	} elsif ($directive eq 'ifdef') {
	    push @conditionals, $active;
	    $active &&= pp_defined($cond);
	} elsif ($directive eq 'ifndef') {
	    push @conditionals, $active;
	    $active &&= !pp_defined($cond);
	} elsif ($directive eq 'if') {
	    push @conditionals, $active;
	    $active &&= pp_eval($cond);
	} elsif ($active) {
	    if ($directive eq 'define') {
		$cond =~ /^(\w+)\s*(.*)/;
		$macro{$1} = $2;
		no warnings;
		eval "sub $1 { q($2) }";
	    } elsif ($directive eq 'undef') {
		$cond =~ /^(\w+)/;
		no warnings;
		eval "sub $1 { 0 }";
	    }
	}
	print;
	next;
    }

    if (!$active) {
	print;
	next;
    }

    if ($suppress_argc_argv_unused_warnings && /^{/) {
	$suppress_argc_argv_unused_warnings = 0;
	print "$_  (void)argc;\n  (void)argv;\n";
	next;
    }

    if (/^_getopt_initialize\b/) {
	$suppress_argc_argv_unused_warnings = 1;
    } elsif (!/^\s*#/) {
	while (/\\\n$/) {
	    $_ .= <>;
	}
	# very crude - doesn't know about comments, etc
	s!\b_\("(.*?)"\)!replacement($1)!gse;
    } elsif (/\s*#\s*define\s+_\(/) {
	$_ = "#include \"message.h\"\n";
    }
    print;
}

if ($die) {
    die "Not all messages found!\n";
}

sub replacement {
    my $msg = shift;
    $msg =~ s/\\\n//g;
    my $msgno = "";
    if (exists $revmsgs{$msg}) {
	$msgno = $revmsgs{$msg};
    } else {
	my $tmp = $msg;
	$tmp =~ s/`(.*?)'/“$1”/g;
	$tmp =~ s/(\w)'(\w)/$1’$2/g;
	if (exists $revmsgs{$tmp}) {
	    $msg = $tmp;
	    $msgno = $revmsgs{$msg};
	} else {
	    if (!$die) {
		print STDERR "Message(s) not found in message file:\n";
		$die = 1;
	    }
	    print STDERR "'$msg'\n";
	}
    }
    return "msg(/*$msg*/$msgno)";
}
