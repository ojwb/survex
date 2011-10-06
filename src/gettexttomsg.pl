#!/usr/bin/perl -w

#  gettexttomsg.pl
#
#  Copyright (C) 2001,2002,2005,2011 Olly Betts
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

my $msgid;
open MSG, "../lib/codes.po" or die $!;
while (<MSG>) {
    next if /^\s*#/; # skip comments
   
    if (/^\s*msgid\s*"(.*)"/) {
	$msgid = $1;
    } elsif (/^\s*msgstr\s*"(.*)"/) {
	my $msgstr = $1;
	if (! defined $msgid) {
	    die "No msgid for msgstr '$msgstr'\n";
	}
	if ($msgstr !~ /^\d*$/) {
	    die "Non-numeric msgstr '$msgstr'\n";
	}
	$revmsgs{$msgid} = $msgstr;
	$msgid = undef;
    }
}

my $die = 0;

while (<>) {
    if (!/^\s*#/) {
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
	if (!$die) {
	    print STDERR "Message(s) not found in message file:\n";
	    $die = 1;
	}
	print STDERR "'$msg'\n";
    }
    return "msg(/*$msg*/$msgno)";
}
