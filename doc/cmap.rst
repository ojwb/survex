================
Bob Thrun's CMAP
================

Survex can read CMAP processed survey data, commonly known as CMAP
XYZ files.  CMAP no longer seems to be used, but we've kept the
support in place so it's there if anyone finds old data and wants to
view it.

Support was added long ago (Survex 1.0.4) but you really want to use
Survex 1.4.9 or later due to a feet to metres conversion bug in all
versions before this, which scaled all returned coordinates from CMAP
XYZ files by a factor of about 10.76.

CMAP XYZ files contain a timestamp.  CMAP was originally written for
computers where the clock was just set to localtime so it seems
likely this timestamp is in localtime, but it does not specify a
timezone.  Survex assumes it's in UTC, which is at least fairly
central in the possibilities, but may mean timestamps are off by up
to about half a day.  The timestamps in example files all use two
digit years.  It's not documented how CMAP handles years 2000 and
later, so we assume 4 digit years are correct, years < 50 are 20xx,
and other years get 1900 added to them (so year 101 is 2001).

CMAP XYZ files don't seem to contain any station flags.  There's a
single character "type" which acts as a leg flag of sorts, but it
seems the meaning is up to the user so we don't try to interpret it.
We assume all the data is underground (so all stations get the
"underground" flag).

There are two variants of CMAP XYZ files.  CMAP v16 and later produce
the "shot" variant (with extension ``.sht``), which is well supported
by Survex.

Older CMAP versions produced the "station" variant (with extension
``.adj`` for adjusted data and ``.una`` for unadjusted data).  Survex only
supports reading stations from these - the survey legs linking them
are currently ignored.
