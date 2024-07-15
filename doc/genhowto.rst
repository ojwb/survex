==================
General: How do I?
==================

-------------------
Create a new survey
-------------------

You create a text file containing the relevant survey data, using a text
editor, and save it with a suitable name with a ``.svx`` extension.  The easiest
way is to look at some of the example data and use that as a template.  Nearly
all surveys will need a bit of basic info as well as the survey data itself:
e.g.  the date (``*date``), comments about where, what cave, a name for the
survey (using ``*begin`` and ``*end``), instrument error corrections, etc. Here
is a typical survey file:

All the lines starting with ``;`` are comments, which are ignored by Survex.
You can also see the use of ``DOWN`` for plumbs, and ``*calibrate tape`` for
dealing with a tape length error (in this case the end of the tape had fallen
off so measurements were made from the 20cm point).
::

   *equate chaos.1 triassic.pt3.8
   *equate chaos.2 triassic.pt3.9

   *begin chaos
   *title "Bottomless Pit of Eternal Chaos to Redemption pitch"
   *date 1996.07.11
   *team "Nick Proctor" compass clino tape
   *team "Anthony Day" notes pictures tape
   *instrument compass "CUCC 2"
   *instrument clino "CUCC 2"
   ;Calibration: Cairn-Rock 071 072 071,  -22 -22 -22
   ;       Rock-Cairn 252 251 252,  +21 +21 +21
   ;Calibration at 161d entrance from cairn nr entrance to
   ;prominent rock edge lower down. This is different from
   ;calibration used for thighs survey of 5 July 1996

   *export 1 2

   ;Tape is 20cm too short
   *calibrate tape +0.2

   1 2 9.48 208 +08
   2 3 9.30 179 -23
   3 4 2.17 057 +09
   5 4 10.13 263 +78
   5 6 2.10 171 -73
   7 6 7.93 291 +75
   *begin
   *calibrate tape 0
   8 7 35.64 262 +86 ;true length measured for this leg
   *end
   8 9 24.90 - DOWN
   10 9 8.61 031 -43
   10 11 2.53 008 -34
   11 12 2.70 286 -20
   13 12 5.36 135 +23
   14 13 1.52 119 -12
   15 14 2.00 036 +13
   16 15 2.10 103 +12
   17 16 1.40 068 -07
   17 18 1.53 285 -42
   19 18 5.20 057 -36
   19 20 2.41 161 -67
   20 21 27.47 - DOWN
   21 22 9.30 192 -29
   *end chaos

-------------------
Organise my surveys
-------------------

This is actually a large subject.  There are many ways you can
organise your data using Survex.  Take a look at the example
dataset for some ideas of ways to go about it.

Fixed Points (Control Points)
=============================

The ``*fix`` command is used to specify fixed points (also know as
control points).  See the description of this command in the
"Cavern Commands" section of this manual.

More than one survey per trip
=============================

Suppose you have two separate bits of surveying which were done
on the same trip.  So the calibration details, etc. are the same
for both, but you want to give a different survey name to the
two sections.  This is easily achieved like so:
::


    *begin
    *calibrate compass 1.0
    *calibrate clino 0.5
    *begin altroute
    ; first survey
    *end altroute
    *begin faraway
    ; second survey
    *end faraway
    *end

----------------------
Add surface topography
----------------------

Survex 1.2.18 added support for loading terrain data and rendering
it as a transparent surface.  Currently the main documentation for
this is maintained as a `wiki
page <https://trac.survex.com/wiki/TerrainData>`__ as this allows
us to update it between releases.

This supports loading data in the HGT format that NASA offers SRTM
data in.  The SRTM data provides terrain data on a 1 arc-second
grid (approximately 30m) for most of the world.

--------------
Overlay a grid
--------------

Aven is able to display a grid, but this functionality isn't
currently available in printouts.  You can achieve a similar effect
for now by creating a ``.svx`` file where the survey legs form a
grid.

If you want to do this, we suggest fixing points at the end of
each grid line and using the ``NOSURVEY`` data style to addd
effectively elastic legs between these fixed points.  This is
simpler to generate than generating fake tape/compass/clino legs
and is very fast for cavern to process.
Some tips for doing this

Here's a small example of a 500mx500m grid with lines 100m apart:
::

   *fix 0W 000 000 0
   *fix 1W 000 100 0
   *fix 2W 000 200 0
   *fix 3W 000 300 0
   *fix 4W 000 400 0
   *fix 5W 000 500 0

   *fix 0E 500 000 0
   *fix 1E 500 100 0
   *fix 2E 500 200 0
   *fix 3E 500 300 0
   *fix 4E 500 400 0
   *fix 5E 500 500 0

   *fix 0S 000 000 0
   *fix 1S 100 000 0
   *fix 2S 200 000 0
   *fix 3S 300 000 0
   *fix 4S 400 000 0
   *fix 5S 500 000 0

   *fix 0N 000 500 0
   *fix 1N 100 500 0
   *fix 2N 200 500 0
   *fix 3N 300 500 0
   *fix 4N 400 500 0
   *fix 5N 500 500 0

   *data nosurvey from to

   0W 0E
   1W 1E
   2W 2E
   3W 3E
   4W 4E
   5W 5E

   0S 0N
   1S 1N
   2S 2N
   3S 3N
   4S 4N
   5S 5N

-------------------------------
Import data from other programs
-------------------------------

Survex supports a number of features to help with importing
existing data.

Unprocessed survey data in Compass or Walls format can be processed
directly, and mixed datasets are support to aid combining data from
different projects, and also migrating data into Survex from another
format.

Processed survey data in Compass or CMAP formats can be viewed.

For data in formats without explicit support, you may be able to read the data
by renaming the file to have a ``.svx`` extension and adding a few Survex
commands at the start of the file to set things up so Survex can read the data
which follows.  For example, you can specify the ordering of items on a
line using ``*data`` (see Survex Keywords above), and you can specify
the characters used to mean different things using ``*set`` (see
Survex Keywords above).

The ``ignore`` and ``ignoreall`` items in the ``*data`` command are often
particularly useful, e.g. if you have a dataset with LRUD info or
comments on the ends of lines.

-------------------------------
Changing Meanings of Characters
-------------------------------

See the ``*set`` command documentation for details, and some examples.

-----------------------------------------------------
See errors and warnings that have gone off the screen
-----------------------------------------------------

When you run Survex it will process the specified survey data
files in order, reporting any warnings and errors. If there are no
errors, the output files are written and various statistics about
the survey are displayed. If there are a lot of warnings or
errors, they can scroll off the screen and it's not always
possible to scroll back to read them.

The easiest way to see all the text is to use **cavern --log** to
redirect output to a ``.log`` file, which you can then inspect
with a text editor.

----------------------------
Create an Extended Elevation
----------------------------

You can create a simple extended elevation from ``aven``, using the
``File->Extended Elevation...`` menu option.  This takes the currently
loaded survey file and "flattens" it.

Behind the scenes this runs the ``extend`` command-line program.  This program
offers more powerful features, such as being able to control which way legs are
folded and where loops are broken, but currently these features are only
accessible from the command line (the intention is to allow them to be used
from ``aven`` in the future).
