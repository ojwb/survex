#!/usr/bin/perl -w
require 5.8;
use bytes;
use strict;

# run on the output of:
# cvs2cl --separate-header --no-wrap
# or vanilla cvs2cl it seems...

my $title = 'ChangeLog';
$title = "$ARGV[0] $title" if defined $ARGV[0];
  
print <<END;
<HTML><HEAD>
<TITLE>$title</TITLE>
<STYLE type="text/css"><!--
BODY, TD, CENTER, UL, OL {font-family: sans-serif;}
-->
</STYLE>
</HEAD><BODY BGCOLOR=white TEXT=black>

<dl>
END

my $rec;

while (<STDIN>) {
    if (/^\d/) {
	print_entry($rec) if defined($rec);
	$rec = '';
    }
    $rec .= $_;
}
print_entry($rec) if defined($rec);

print <<END;
</dl>

</BODY>
</HTML>
END

sub print_entry {
    my ($hdr, $rec) = split /\n/, shift, 2;
    $hdr = html($hdr);
    $hdr =~ s/  /&nbsp;&nbsp;/;
    $hdr =~ s/  +/ /g;
    print "<dt>", $hdr, "</dt>\n";
    $rec =~ s/^\s*\*\s*//;
    $rec = html($rec);
    $rec =~ s/\s*\n\n\s*/<p>/g;
    $rec =~ s/^\s*/<p>/;
    $rec =~ s/\s*$/<p>/;
    $rec =~ s/^\s+//mg;
    $rec =~ s/\s+$//mg;
    $rec =~ s/  +/ /g;
    print "<dd>", $rec, "</dd>\n";
}

sub html {
    my $t = shift;
    $t =~ s/&/&amp;/g;
    $t =~ s/</&lt;/g;
    $t =~ s/>/&gt;/g;
    return $t;
}
