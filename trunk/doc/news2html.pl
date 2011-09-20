#!/usr/bin/perl -w
require 5.008;
use bytes;
use strict;

# run on a NEWS file

my $title = 'Summary of User-Visible Changes';
$title .= " in $ARGV[0]" if defined $ARGV[0];
  
print <<END;
<HTML><HEAD>
<TITLE>$title</TITLE>
<STYLE type="text/css"><!--
BODY, TD, CENTER, UL, OL {font-family: sans-serif;}
H1 {font-size: 16px;}
-->
</STYLE>
</HEAD><BODY BGCOLOR=white TEXT=black>

END

my $rec;

my $in_ul = 0;
my $in_li = 0;

while (<STDIN>) {
    if (s/^\*\s*//) {
	if ($in_ul == 2) {
	    if ($in_li) {
		print "</p></li>\n";
		$in_li = 0;
	    }
	    print "</ul></li>\n";
	    --$in_ul;
	}
	if (!$in_ul) {
	    print "<ul>";
	    $in_ul = 1;
	}
	if ($in_li) {
	    print "</p></li>";
	    $in_li = 0;
	}
	print "<li><p>", html($_);
	$in_li = 1;
    } elsif (s/^\s+\+\s+//) {
	if (!$in_ul) {
	    print "<ul>";
	    $in_ul = 1;
	}
	if ($in_ul != 2) {
	    if (!$in_li) {
		print "<li><p>";
	    }
	    print "<ul>";
	    $in_ul = 2;
	    $in_li = 0;
	}
	if ($in_li) {
	    print "</p></li>";
	    $in_li = 0;
	}
	print "<li><p>", html($_);
	$in_li = 1;
    } elsif (s/^[ \t]+//) {
	print html($_);
    } elsif (/^$/) {
	# do nothing
    } else {
	if ($in_li) {
	    print "</p></li>";
	    $in_li = 0;
	}
	while ($in_ul) {
	    print "</ul>";
	    --$in_ul;
	    if ($in_ul) { print "</p></li>"; }
	}
	$_ = html($_);
	s!(\(.*\))!<small>$1</small>!;
	print "<h1>$_</h1>\n";
    }
}	

if ($in_li) {
    print "</p></li>";
    $in_li = 0;
}
while ($in_ul) {
    print "</ul>";
    --$in_ul;
    if ($in_ul) { print "</p></li>"; }
}

print <<END;
</BODY>
</HTML>
END

sub html {
    my $t = shift;
    $t =~ s/&/&amp;/g;
    $t =~ s/</&lt;/g;
    $t =~ s/>/&gt;/g;
    return $t;
}
