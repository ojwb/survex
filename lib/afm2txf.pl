#!/usr/bin/perl -w

# afm2txf.pl 0.2
#
# Generates .txf font textures from Type 1 fonts
# Requirements: Ghostscript, ImageMagick
#
# Usage:
#       afm2txf.pl [-o OUTPUT.txf] whatever.afm
#
# Changelog:
#       0.2 (06/28/2002): Generate fonts with proper padding
#       0.1 (06/28/2002): Initial version
#
# Copyright (C) 2002 Andrew James Ross
# Copyright (C) 2010,2011 Olly Betts
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

use strict;

my $output;
if (scalar @ARGV >= 1) {
    my $arg = $ARGV[0];
    if ($arg =~ s/^-o//) {
	shift;
	if ($arg eq '') {
	    if (scalar @ARGV == 0) {
		die "-o needs an argument\n";
	    }
	    $output = shift;
	} else {
	    $output = $arg;
	}
    }
}

my $METRICS = shift or die; # AFM file

# Texture size
my $TEXSIZ = 256;

# Padding around each character, for mipmap separation
my $PADDING = 4;

# Antialiasing multiplier.  Should be 16 for production work.  As low
# as 4 works well for testing.
my $DOWNSAMPLE = 16;

# The printable ISO-8859-1 characters (and space and hard space) and their
# postscript glyph names.  We use names because not all postscript
# fonts are encoded using ASCII.  AFM metrics generated by ttf2afm, in
# fact, don't have any numerical character IDs at all.  In principle,
# this mechanism will work for any 8 bit font encoding, you just have
# to do the legwork of figuring out the name to ID mapping.
my %REVCHARS = (32=>'space', 33=>'exclam', 34=>'quotedbl',
	     35=>'numbersign', 36=>'dollar', 37=>'percent',
	     38=>'ampersand', 39=>'quotesingle', 40=>'parenleft',
	     41=>'parenright', 42=>'asterisk', 43=>'plus',
	     44=>'comma', 45=>'hyphen', 46=>'period', 47=>'slash',
	     48=>'zero', 49=>'one', 50=>'two', 51=>'three',
	     52=>'four', 53=>'five', 54=>'six', 55=>'seven',
	     56=>'eight', 57=>'nine', 58=>'colon', 59=>'semicolon',
	     60=>'less', 61=>'equal', 62=>'greater', 63=>'question',
	     64=>'at', 65=>'A', 66=>'B', 67=>'C', 68=>'D', 69=>'E',
	     70=>'F', 71=>'G', 72=>'H', 73=>'I', 74=>'J', 75=>'K',
	     76=>'L', 77=>'M', 78=>'N', 79=>'O', 80=>'P', 81=>'Q',
	     82=>'R', 83=>'S', 84=>'T', 85=>'U', 86=>'V', 87=>'W',
	     88=>'X', 89=>'Y', 90=>'Z', 91=>'bracketleft',
	     92=>'backslash', 93=>'bracketright', 94=>'asciicircum',
	     95=>'underscore', 96=>'quoteleft', 97=>'a', 98=>'b', 99=>'c',
	     100=>'d', 101=>'e', 102=>'f', 103=>'g', 104=>'h',
	     105=>'i', 106=>'j', 107=>'k', 108=>'l', 109=>'m',
	     110=>'n', 111=>'o', 112=>'p', 113=>'q', 114=>'r',
	     115=>'s', 116=>'t', 117=>'u', 118=>'v', 119=>'w',
	     120=>'x', 121=>'y', 122=>'z', 123=>'braceleft',
	     124=>'bar', 125=>'braceright', 126=>'asciitilde',
	     160=>'space', 161=>'exclamdown', 162=>'cent', 163=>'sterling',
	     164=>'currency', 165=>'yen', 166=>'brokenbar', 167=>'section',
	     168=>'dieresis', 169=>'copyright', 170=>'ordfeminine', 171=>'guillemotleft',
	     172=>'logicalnot', 173=>'hyphen', 174=>'registered', 175=>'macron',
	     176=>'degree', 177=>'plusminus', 178=>'twosuperior', 179=>'threesuperior',
	     180=>'acute', 181=>'mu', 182=>'paragraph', 183=>'bullet',
	     184=>'cedilla', 185=>'onesuperior', 186=>'ordmasculine', 187=>'guillemotright',
	     188=>'onequarter', 189=>'onehalf', 190=>'threequarters', 191=>'questiondown',
	     192=>'Agrave', 193=>'Aacute', 194=>'Acircumflex', 195=>'Atilde',
	     196=>'Adieresis', 197=>'Aring', 198=>'AE', 199=>'Ccedilla',
	     200=>'Egrave', 201=>'Eacute', 202=>'Ecircumflex', 203=>'Edieresis',
	     204=>'Igrave', 205=>'Iacute', 206=>'Icircumflex', 207=>'Idieresis',
	     208=>'Eth', 209=>'Ntilde', 210=>'Ograve', 211=>'Oacute',
	     212=>'Ocircumflex', 213=>'Otilde', 214=>'Odieresis', 215=>'multiply',
	     216=>'Oslash', 217=>'Ugrave', 218=>'Uacute', 219=>'Ucircumflex',
	     220=>'Udieresis', 221=>'Yacute', 222=>'Thorn', 223=>'germandbls',
	     224=>'agrave', 225=>'aacute', 226=>'acircumflex', 227=>'atilde',
	     228=>'adieresis', 229=>'aring', 230=>'ae', 231=>'ccedilla',
	     232=>'egrave', 233=>'eacute', 234=>'ecircumflex', 235=>'edieresis',
	     236=>'igrave', 237=>'iacute', 238=>'icircumflex', 239=>'idieresis',
	     240=>'eth', 241=>'ntilde', 242=>'ograve', 243=>'oacute',
	     244=>'ocircumflex', 245=>'otilde', 246=>'odieresis', 247=>'divide',
	     248=>'oslash', 249=>'ugrave', 250=>'uacute', 251=>'ucircumflex',
	     252=>'udieresis', 253=>'yacute', 254=>'thorn', 255=>'ydieresis'
	     );

my %CHARS;
while (my ($k, $v) = each %REVCHARS) {
    $CHARS{$v} = $k unless exists $CHARS{$v};
}

my %metrics = ();
my %positions = ();

#
# Parse the font metrics.  This is a 5 element array.  All numbers are
# expressed as a fraction of the line spacing.
# 0:    nominal width (distance to the next character)
# 1, 2: Coordinates of the lower left corner of the bounding box,
#       relative to the nominal position.
# 3, 4: Size of the bounding box
#
print STDERR "Reading font metrics...\n";
my $FONT;
open METRICS, '<', $METRICS or die $!;
my $m;
while (defined($m = <METRICS>)) {
    if ($m =~ /^FontName (\S*)/) { $FONT = $1; next; }
    next unless $m =~ /^C /;
    chomp $m;

    die "No name: $m" if $m !~ /N\s+([^\s]+)\s+;/;
    my $name = $1;

    die "No box: $m"
	if $m !~ /B\s+([-0-9]+)\s+([-0-9]+)\s+([-0-9]+)\s+([-0-9]+)\s+;/;
    my ($left, $bottom, $right, $top) = ($1/1000, $2/1000, $3/1000, $4/1000);

    die "No width: $m" if $m !~ /WX\s+([-0-9]+)/;
    my $nomwid = $1/1000; # nominal, not physical width!

    # The coordinates of the corner relative to the character
    # "position"
    my ($x, $y) = (-$left, -$bottom);
    my ($w, $h) = ($right-$left, $top-$bottom);

    $metrics{$name} = [$nomwid, $x, $y, $w, $h];
}
close METRICS;

die "No FontName found in metrics" if not defined $FONT;

# Sanitise $FONT.
$FONT =~ s!/!_!g;

#
# Find the height of the tallest character, and print some warnings
#
my $maxhgt = 0;
foreach my $c (keys %CHARS) {
    if(!defined $metrics{$c}) {
	print STDERR "% WARNING: no metrics for char $c.  Skipping.\n";
	next;
    }
    if($metrics{$c}->[4] > $maxhgt) { $maxhgt = $metrics{$c}->[4]; }
}
if($maxhgt == 0) {
    print STDERR "No usable characters found.  Bailing out.\n";
    exit 1;
}

#
# Do the layout.  Keep increasing the row count until the characters
# just fit.  This isn't terribly elegant, but it's simple.
#
print STDERR "Laying out";
my $rows = 1;
my $PS;
my $LINEHGT;
while(!defined ($PS = genPostscript($rows))) { $rows++; }
print STDERR " ($rows rows)\n";

#
# Call ghostscript to render
#
print STDERR "Rendering Postscript...\n";
my $res = $TEXSIZ * $DOWNSAMPLE;
my $pid = open PS, "|gs -r$res -g${res}x${res} -sDEVICE=ppm -sOutputFile=\Q$FONT\E.ppm > /dev/null";
die "Couldn't spawn ghostscript interpreter" if !defined $pid;
foreach (@$PS) {
    print PS "$_\n";
}
close PS;
waitpid($pid, 0);

#
# Downsample with ImageMagick
#
print STDERR "Antialiasing image...\n";
system("mogrify -geometry ${TEXSIZ}x${TEXSIZ} \Q$FONT\E.ppm") == 0
    or die "Couldn't rescale $FONT.ppm";

#
# Generate the .txf file
#
print STDERR "Generating textured font file...\n";

# Prune undefined glyphs
foreach my $c (keys %metrics) {
    delete $metrics{$c} if !defined $CHARS{$c};
}

sub round { sprintf "%.0f", $_[0] }
$output = "$FONT.txf" unless defined $output;
open TXF, '>', $output or die;
print TXF pack "V", 0x667874ff;
print TXF pack "V", 0x12345678;
print TXF pack "V", 0;
print TXF pack "V", $TEXSIZ;
print TXF pack "V", $TEXSIZ;
print TXF pack "V", round($TEXSIZ * $LINEHGT);
print TXF pack "V", 0;
my @chars = sort keys %REVCHARS;
print TXF pack "V", scalar @chars;
foreach my $c (@chars) {
    my $name = $REVCHARS{$c};
    my $m = $metrics{$name};
    my $p = $positions{$name};
    my $step = round($m->[0] * $LINEHGT * $TEXSIZ);

    # Pad the bounding box, to handle areas that outside.  This can
    # happen due to thick lines in the font path, or be an artifact of
    # the downsampling.
    my ($w, $h, $xoff, $yoff, $x, $y);
    if ($name eq 'space') {
	$w = $h = $xoff = $yoff = $x = $y = 0;
    } else {
	$w = round($m->[3] * $LINEHGT * $TEXSIZ + 2*$PADDING);
	$h = round($m->[4] * $LINEHGT * $TEXSIZ + 2*$PADDING);
	$xoff = -round($m->[1] * $LINEHGT * $TEXSIZ) - $PADDING;
	$yoff = -round($m->[2] * $LINEHGT * $TEXSIZ) - $PADDING;
	$x = round($p->[0] * $TEXSIZ - $PADDING);
	$y = round($p->[1] * $TEXSIZ - $PADDING);
    }
    print TXF pack "v", $c;
    print TXF pack "C", $w;
    print TXF pack "C", $h;
    print TXF pack "c", $xoff;
    print TXF pack "c", $yoff;
    print TXF pack "C", $step;
    print TXF pack "C", 0;
    print TXF pack "v", $x;
    print TXF pack "v", $y;
}

# Read in the .ppm file, dump the duplicate color values (ghostscript
# won't generate pgm's) and write to the end of the .txf.  Remember to
# swap the order of the rows; OpenGL textures are bottom-up.
open PPM, "$FONT.ppm" or die;
my $pixel;
foreach my $r (1 .. $TEXSIZ) {
    seek PPM, -3*$r*$TEXSIZ, 2 or die;
    foreach (1 .. $TEXSIZ) {
	read PPM, $pixel, 3 or die;
	print TXF substr($pixel, 0, 1);
    }
}
close PPM;
close TXF;

# Clean up
unlink("$FONT.ppm");

########################################################################
########################################################################
########################################################################

# Put the digits first to help ensure they are all on the same line in the
# texture and will exactly align vertically when rendered - a slight
# discrepancy here is particularly visible in the colour key legends and
# compass bearing.
sub render_order {
    my $a = $CHARS{$a};
    my $b = $CHARS{$b};
    my $a_dig = chr($a) =~ /\d/;
    my $b_dig = chr($b) =~ /\d/;
    if ($a_dig ^ $b_dig) {
	return $a_dig ? -1 : 1;
    }
    return $a <=> $b;
}

sub genPostscript {
    my $rows = shift;
    my $rowhgt = 1/$rows;

    my @PS = ();

    # The canonical "point size" number, in texture space
    $LINEHGT = ($rowhgt - 2*$PADDING/$TEXSIZ) / $maxhgt;

    # Get to where we want.  Draw the whole thing in a 1 inch square at
    # the bottom left of the "page".
    push @PS, "72 72 scale";

    # Fill the square with black
    push @PS, "0 setgray";
    push @PS, "-1 -1 moveto";
    push @PS, "-1 1 lineto 1 1 lineto 1 -1 lineto";
    push @PS, "closepath";
    push @PS, "fill";

    # Draw in white
    push @PS, "1 setgray";

    # Generate our PUSH @PS, font
    push @PS, "/$FONT findfont $LINEHGT scalefont setfont";

    my $x = $PADDING/$TEXSIZ;
    my $y = 1 - $rowhgt + $PADDING/$TEXSIZ;
    my @chars = sort render_order (keys %CHARS);
    foreach my $c (@chars) {
	next if $c eq 'space';
	my $m = $metrics{$c};
	next if !defined $m;

	# No space?
	my $w = $m->[3]*$LINEHGT;
	if($x + $w + $PADDING/$TEXSIZ > 1) {
	    $x = $PADDING/$TEXSIZ;
	    $y -= $rowhgt;
	    return undef if $y < 0;
	}

	# Record where in the texture the box ended up
	$positions{$c} = [$x, $y];

	my $vx = $x + $m->[1]*$LINEHGT;
	my $vy = $y + $m->[2]*$LINEHGT;

	push @PS, "$vx $vy moveto";
	push @PS, "/$c glyphshow";

	# Next box...
	$x += $w + 2*$PADDING/$TEXSIZ;
    }

    push @PS, "showpage";

    return \@PS;
}
