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
later, so years < 50 get 2000 added to them, years 50 to 199 inclusive
get 1900 added to them, and years >= 200 are used as is (so year 0 is 2000,
year 49 is 2049, year 50 is 1950, year 99 is 1999, year 101 is 2001, and year
125 is 2025).

CMAP XYZ files don't seem to contain any station flags.  There's a
single character "type" which acts as a leg flag of sorts, but it
seems the meaning is up to the user so we don't try to interpret it.
We assume all the data is underground (so all stations get the
"underground" flag).

There are two variants of CMAP XYZ files.  CMAP 16 and later default to
producing the "shot" variant (with extension ``.sht``), which is well
supported by Survex.  CMAP 16.1 was released in 1995 so you're probably
much more likely to encounter ``.sht`` files.

Older CMAP versions produced the "station" variant (with extension
``.adj`` for adjusted data and ``.una`` for unadjusted data).  Survex only
supports reading stations from this format - the survey legs linking them
are currently ignored.  This wasn't implemented originally because
there seemed to be a mismatch between the documentation of the format of
these files and the accompanying example files which we never managed to
resolve (the ``1st`` column is documented as *"Station number where the name
of the current station first appeared. If this is not a closure station, the
number will be the current station number."* but in the sample files it
seems to just be the current station number, even when there are loops).

Implementing reading based on what seems to be a buggy specification
really needs a broader collection of sample files.  Given this format seems
to be obsolete that seems unlikely to happen now, but if you have a
collection of old CMAP files you are keen to be able to read with Survex
then please get in touch.
