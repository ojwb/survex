perl -ne while (m!/\*(.*?)\*/([1-9][0-9]*)\b!g) { printf "en:%3d ", $2; $l = $1; if ($l =~ /^\s/ || $l =~ /\s$/) {print "\"$l\"\n"; } else { print "$l\n"; }}
