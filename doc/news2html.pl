#!/usr/bin/perl -w
require 5.008;
use bytes;
use strict;

# run on a NEWS file

my $title = 'Summary of User-Visible Changes';
$title .= " in $ARGV[0]" if defined $ARGV[0];
  
print <<END;
<html><head>
<meta charset="utf-8">
<title>$title</title>
</head><body bgcolor=white text=black>
END

my $inpre = 0;
while (<STDIN>) {
    if (/^Changes in ([0-9.a-zA-Z]+)/) {
	print "</pre>\n" if $inpre;
	print "<pre id=\"", html($1), "\">";
	$inpre = 1;
    } elsif (!$inpre) {
	print "<pre>";
	$inpre = 1;
    }
    print html($_);
}

print "</pre>\n" if $inpre;
print <<END;
</body></html>
END

sub html {
    my $t = shift;
    $t =~ s/&/&amp;/g;
    $t =~ s/</&lt;/g;
    $t =~ s/>/&gt;/g;
    $t =~ s/"/&quot;/g;
    return $t;
}
