#!/usr/bin/perl -w
require 5.008;
use bytes;
use strict;
use POSIX;
use Locale::PO;

sub pot_creation_date {
    return strftime "%Y-%m-%d %H:%M:%S +0000", gmtime();
}

use integer;

my (%msgs, @uses, %comment, %loc);
my $translator_comment;
while (<ARGV>) {
    if (m!(/[/*])\s*(TRANSLATORS:.*?)\s*\z!) {
	my ($comment_type, $comment) = ($1, $2);
	if ($comment_type eq '/*') {
	    while ($comment !~ s!\s*\*/\z!! && defined($_ = <ARGV>)) {
		if (m!^\s*\*?\s*(.*?)\s*\z!) {
		    # */ on a line by itself results in '/' for $1.
		    last if $1 eq '/';
		    $comment .= "\n$1";
		}
	    }
	} else {
	    # // comment - see if there are further // comments immediately
	    # following.
	    while (defined($_ = <ARGV>) && m!//\s*(.*?)\s*\z!) {
		$comment .= "\n$1";
	    }
	}
	$comment =~ s/\n+$//;
	if (defined $translator_comment) {
	    print STDERR "$ARGV:$.: Ignored TRANSLATORS comment: $translator_comment\n";
	}
	$translator_comment = $comment;
	last if !defined $_;
    }

    while (m!/\*(.*?)\*/(\d+)\b!g) {
	my ($msg, $msgno) = ($1, $2);
	# Handle there being a comment before the comment with the message in.
	$msg =~ s!.*/\*!!;
	if (exists $msgs{$msgno}) {
	    if ($msgs{$msgno} ne $msg) {
		print STDERR "$ARGV:$.: Mismatch for message number $msgno:\n";
		print STDERR "$msgs{$msgno}\n$msg\n";
	    }
	} else {
	    $msgs{$msgno} = $msg;
	}
	if (defined $translator_comment) {
	    if (exists $comment{$msgno} && $comment{$msgno} ne $translator_comment) {
		print STDERR "Different TRANSLATOR comments for message #$msgno\n";
		print STDERR "${$uses[$msgno]}[0]: $comment{$msgno}\n";
		print STDERR "$ARGV:$.: $translator_comment\n";
	    } else {
		$comment{$msgno} = $translator_comment;
	    }
	    undef $translator_comment;
	}
	push @{$uses[$msgno]}, "$ARGV:$.";
    }
} continue {
    # Reset $. for each input file.
    close ARGV if eof;
}

my $num_list = Locale::PO->load_file_asarray("survex.pot");
my $first = 1;
foreach my $po_entry (@{$num_list}) {
    my $msgno = '';
    my $ref = $po_entry->reference;
    if (defined $ref && $ref =~ /^n:(\d+)$/m) {
	$msgno = $1;
    }
    if ($first) {
	$first = 0;
	if ($po_entry->msgid eq '""') {
	    chomp(my $header = $po_entry->dump);
	    print $header;
	    next;
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
"POT-Creation-Date: ${\(pot_creation_date)}\\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI:SE +ZONE\\n"
"Language-Team: LANGUAGE <LL\@li.org>\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=utf-8\\n"
"Content-Transfer-Encoding: 8bit\\n"
END
    }
    my $msg;
    if (exists $msgs{$msgno}) {
	$msg = $msgs{$msgno};
	delete $msgs{$msgno};
    } else {
	print STDERR "../lib/survex.pot:", $po_entry->loaded_line_number, ": Message number $msgno is in survex.pot but not found in source - preserving\n" unless $po_entry->obsolete;
	$msg = $po_entry->dequote($po_entry->msgid);
    }
    if (exists $comment{$msgno}) {
	my $new = $comment{$msgno};
	my $old = $po_entry->automatic;
	$po_entry->automatic($new);
	if (defined $old) {
	    $old =~ s/\s+/ /g;
	    $new =~ s/\s+/ /g;
	    if ($old ne $new) {
		print STDERR "Comment for message #$msgno changed:\n";
		print STDERR "../lib/survex.pot:", $po_entry->loaded_line_number, ": [$old]\n";
		print STDERR "${$uses[$msgno]}[0]: [$new]\n";
	    }
	}
    }
    if (defined $po_entry->automatic) {
	if (!exists $comment{$msgno}) {
	    my $fake_err = ": Comment for message #$msgno not in source code\n";
	    if ($msgno ne '' && exists($uses[$msgno])) {
		print STDERR join($fake_err, "../lib/survex.pot:".$po_entry->loaded_line_number, @{$uses[$msgno]}), $fake_err if exists($uses[$msgno]);
		my $x = $po_entry->automatic;
		$x =~ s/\n/\n     * /g;
		print STDERR "    /* $x */\n";
	    } else {
		# Currently unused message.
		# print STDERR $fake_err;
		# my $x = $po_entry->automatic;
		# $x =~ s/\n/\n     * /g;
		# print STDERR "    /* $x */\n";
	    }
	}
	my $automatic = "\n" . $po_entry->automatic;
	$automatic =~ s/\n/\n#. /g;
	while ($automatic =~ s/\n#. \n/\n#.\n/g) { }
	print $automatic;
    }
    if ($msgno =~ /^\d+$/) {
	for (@{$uses[$msgno]}) {
	    print "\n#: ", $_;
	}
	print "\n#: n:$msgno";
    }
    print "\n#, c-format" if $msg =~ /\%[a-z0-9.]/;
    if ($msg =~ s/(?:^|[^\\])"/\\"/g) {
	print STDERR "Escaping unescaped \" in message number $msgno\n";
    }
    print "\n";
    print "#~ " if $po_entry->obsolete;
    print "msgid \"$msg\"\n";
    print "#~ " if $po_entry->obsolete;
    print "msgstr \"\"\n";
}

for my $msgno (sort keys %msgs) {
    next if ($msgno == 0 || $msgno >= 1000);
    print STDERR "New message number $msgno\n";
    for (@{$uses[$msgno]}) {
	print "\n#: ", $_;
    }
    my $msg = $msgs{$msgno};
    print "\n#: n:$msgno";
    print "\n#, c-format" if $msg =~ /\%[a-z0-9.]/;
    if ($msg =~ s/(?:^|[^\\])"/\\"/g) {
	print STDERR "Escaping unescaped \" in message number $msgno\n";
    }
    print "\nmsgid \"$msg\"\n";
    print "msgstr \"\"\n";
}
