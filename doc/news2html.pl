#!/usr/bin/perl -w
use strict;

# run on a NEWS file

my $title = 'Summary of User-Visible Changes';
$title .= "in ARGV[1]" if defined $ARGV[1];
  
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
    } elsif (s/^[ \t]+//) {
	print html($_);
    } elsif (/^$/) {
	# do nothing
    } else {
	if ($in_li) {
	    print "</p></li>";
	    $in_li = 0;
	}
	if ($in_ul) {
	    print "</ul>";
	    $in_ul = 0;
	}
	print "<h1>", html($_), "</h1>\n";
    }
}	

if ($in_li) {
    print "</p></li>";
    $in_li = 0;
}
if ($in_ul) {
    print "</ul>";
    $in_ul = 0;
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
