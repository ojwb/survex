#!/usr/bin/perl -w
require 5.008;
use bytes;
use strict;
use POSIX;
use Locale::PO;

my $pot_creation_date = strftime "%Y-%m-%d %H:%M:%S +0000", gmtime();

use integer;

my (%msgs, @uses);
while (<ARGV>) {
    while (m!/\*(.*?)\*/(\d+)\b!g) {
	my ($msg, $msgno) = ($1, $2);
	if (exists $msgs{$msgno}) {
	    if ($msgs{$msgno} ne $msg) {
		print STDERR "Mismatch for message number $msgno:\n";
		print STDERR "$msgs{$msgno}\n$msg\n";
	    }
	} else {
	    $msgs{$msgno} = $msg;
	}
	push @{$uses[$msgno]}, "$ARGV:$.";
    }
} continue {
    # Reset $. for each input file.
    close ARGV if eof;
}

print << "END";
# Survex translation template.
# Copyright (C) YEAR COPYRIGHT HOLDERS
# This file is distributed under the same licence as Survex.
#
msgid ""
msgstr ""
"Project-Id-Version: survex\\n"
"Report-Msgid-Bugs-To: olly\@survex.com\\n"
"POT-Creation-Date: $pot_creation_date\\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI:SE +ZONE\\n"
"Language-Team: LANGUAGE <LL\@li.org>\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=utf-8\\n"
"Content-Transfer-Encoding: 8bit\\n"
END

my $num_list = Locale::PO->load_file_asarray("po_codes");
my $first = 1;
foreach my $po_entry (@{$num_list}) {
    my $msgno = $po_entry->dequote($po_entry->msgstr);
    if ($first) {
	$first = 0;
	next if ($po_entry->msgid eq '""');
    }
    my $msg;
    if (exists $msgs{$msgno}) {
	$msg = $msgs{$msgno};
	delete $msgs{$msgno};
    } else {
	print STDERR "Message number $msgno is in po_codes but not found in source - preserving\n" unless $po_entry->obsolete;
	$msg = $po_entry->dequote($po_entry->msgid);
    }
    if (defined $po_entry->automatic) {
	my $automatic = "\n" . $po_entry->automatic;
	$automatic =~ s/\n/\n#. /g;
	while ($automatic =~ s/\n#. \n/\n#.\n/g) { }
	print $automatic;
    }
    if ($msgno =~ /^\d+$/) {
	for (@{$uses[$msgno]}) {
	    print "\n#: ", $_;
	}
    }
    print "\n#, c-format" if $msg =~ /\%[a-z0-9]/;
    if ($msg =~ s/(?:^|[^\\])"/\\"/g) {
	print STDERR "Escaping unescaped \" in message number $msgno\n";
    }
    print "\n";
    print "#~ " if $po_entry->obsolete;
    print "msgid \"$msg\"\n";
    print "#~ " if $po_entry->obsolete;
    print "msgstr \"$msgno\"\n";
}

for my $msgno (sort keys %msgs) {
    next if ($msgno == 0 || $msgno >= 1000);
    print STDERR "New message number $msgno\n";
    for (@{$uses[$msgno]}) {
	print "\n#: ", $_;
    }
    my $msg = $msgs{$msgno};
    print "\n#, c-format" if $msg =~ /\%[a-z0-9]/;
    if ($msg =~ s/(?:^|[^\\])"/\\"/g) {
	print STDERR "Escaping unescaped \" in message number $msgno\n";
    }
    print "\nmsgid \"$msg\"\n";
    print "msgstr \"$msgno\"\n";
}
