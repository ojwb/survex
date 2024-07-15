=================
``.svx`` Cookbook
=================

Here is some example Survex data (a very small cave numbered
1623/163):
::

    2 1 26.60 222  17.5
    2 3 10.85 014   7
    2 4  7.89 254 -11
    4 5  2.98  - DOWN
    5 6  9.29 271 -28.5

You can vary the data ordering. The default is:

    from-station to-station tape compass clino

This data demonstrates a number of useful features of Survex:

Legs can be measured either way round, which allows the use of
techniques like "leap-frogging" (which is where legs alternate
forwards and backwards).

Also notice that there is a spur in the survey (2 to 3). You do not
need to specify this specially.

Survex places few restrictions on station naming (see "Survey Station
Names" in the previous section), so you can number the stations as
they were in the original survey notes. Although not apparent from
this example, there is no requirement for each leg to connect to an
existing station. Survex can accept data in any order, and will check
for connectedness once all the data has been read in.

Each survey is also likely to have other information associated with
it, such as instrument calibrations, etc. This has been omitted from
this example to keep things simple.

Most caves will take more than just one survey trip to map. Commonly
the numbering in each survey will begin at 1, so we need to be able
to tell apart stations with the same number in different surveys.

To accomplish this, Survex has a very flexible system of hierarchical
prefixes. All you need do is give each survey a unique name or
number, and enter the data like so:
::

    *begin 163
    *export 1
    2 1 26.60 222  17.5
    2 3 10.85 014   7
    2 4  7.89 254 -11
    4 5  2.98  - DOWN
    5 6  9.29 271 -28.5
    *end 163

Survex will name the stations by attaching the current prefix. In
this case, the stations will be named 163.1, 163.2, etc.

We have a convention with the CUCC Austria data that the entrance
survey station of a cave is named P<cave number>, P163 in this case.
We can accomplish this like so:
::

    *equate P163 163.1
    *entrance P163
    *begin 163
    *export 1
    2 1 26.60 222  17.5
    2 3 10.85 014   7
    2 4  7.89 254 -11
    4 5  2.98  - DOWN
    5 6  9.29 271 -28.5
    *end 163

---------------------
Join surveys together
---------------------

Once you have more than one survey you need to specify how they
link together.  To do this use ``*export`` to make the stations to be
joined accessible in the enclosing survey, then ``*equate`` in the
enclosing survey to join them together.  For example:
::

   ; CUCC convention is that p<number> is the entrance station
   ; for cave <number>.
   *entrance p157
   *equate p157 157.0

   *begin 157

   *export 157.0 ; tag bolt

   *begin entpitch
   *team Notes Olly Betts
   *team Insts Jenny Black
   *date 2012.08.05
   *data normal from to tape compass clino
   *export 0 5

   1	0	3.226	202	-05
   1	2	1.505	080	-13
   1	3	2.605	030	+25
   3	4	8.53	335	+80
   4	5	7.499	060	+24

   *end entpitch

   *equate entpitch.5 pt2.1

   *begin pt2
   *team Notes Olly Betts
   *team Insts Jenny Black
   *date 2012.08.29
   *export 1

   1	2	5.361	054	-54
   2	3	4.271	190	+00
   2	4	4.634	138	+04

   *end pt2

   *end 157

-------------------
Surface survey data
-------------------

Say you have 2 underground surveys and 2 surface ones with a single
fixed reference point.  You want to mark the surface surveys so that
their length isn't included in length statistics, and so that Aven
knows to display them differently.  To do this you mark surface
data with the "surface" flag - this is set with ``*flags surface``
like so:
::

    ; fixed reference point
    *fix fix_a 12345 56789 1234

    ; surface data (enclosed in *begin ... *end to stop the *flags command
    ; from "leaking" out)
    *begin
    *flags surface
    *include surface1
    *include surface2
    *end

    ; underground data
    *include cave1
    *include cave2

You might also have a single survey which starts on the surface and heads into
a cave.  This can be easily handled too - here's an example which goes in one
entrance, through the cave, and out of another entrance: ::

    *begin BtoC
    *title "161b to 161c"
    *date 1990.08.06 ; trip 1990-161c-3 in 1990 logbook

    *begin
    *flags surface
    02    01      3.09   249    -08.5
    02    03      4.13   252.5  -26
    *end

    04    03      6.00   020    +37
    04    05      3.07   329    -31
    06    05      2.67   203    -40.5
    06    07      2.20   014    +04
    07    08      2.98   032    +04
    08    09      2.73   063.5  +21
    09    10     12.35   059    +15

    *begin
    *flags surface
    11    10      4.20   221.5  -11.5
    11    12      5.05   215    +03.5
    11    13      6.14   205    +12.5
    13    14     15.40   221    -14
    *end

    *end BtoC

Note that to avoid needless complication, Survex regards each leg
as being either "surface" or "not surface" - if a leg spans the
boundary you'll have to call it one or the other.  It's good
surveying practice to deliberately put a station at the
surface/underground interface (typically the highest closed
contour or drip line) so this generally isn't an onerous
restriction.

-------------
Reading order
-------------

The ``*data`` command is used to specify the order in which the readings are
given.

------
Plumbs
------

Plumbed legs can be specified by using ``UP`` or ``DOWN`` in place of the
clino reading and a dash (or a different specified ``OMIT``
character) in place of the compass reading.  This distinguishes
them from legs measured with a compass and clino.  Here's an
example:
::

    1 2 21.54 - UP
    3 2 7.36 017 +17
    3 4 1.62 091 +08
    5 4 10.38 - DOWN

``U``/``D`` or ``+V``/``-V`` may be used instead of ``UP``/``DOWN``; the check
is not case sensitive.

If you prefer to use ``-90`` and ``+90`` as "magic" clino readings which are
instead treated as plumbs and don't get clino corrections applied, etc, then
see ``*infer plumbs on``.

------------------------
Legs across static water
------------------------

Legs surveyed across the surface of a static body of water where no clino
reading is taken (since the surface of the water can be assumed to be flat) can
be indicated by using ``LEVEL`` in place of a clino reading.  This prevents the
clino correction being applied.  (Note that unlike with plumbed readings, there
isn't a way to treat a clino reading of ``0`` as meaning ``LEVEL`` because
``0`` is a completely reasonable actual clino reading.)

Here's an example:
::

    1 2 11.37 190 -12
    3 2  7.36 017 LEVEL
    3 4  1.62 091 LEVEL

--------------------
Specify a BCRA grade
--------------------

The ``*sd`` command can be used to specify the standard deviations of
the various measurements (tape, compass, clino, etc).  Examples
files are supplied which define BCRA Grade 3 and BCRA Grade 5
using a number of ``*sd`` commands.  You can use these by simply
including them at the relevant point, as follows:
::

    *begin somewhere
    ; This survey is only grade 3
    *include grade3
    2 1 26.60 222  17.5
    2 3 10.85 014   7
    ; etc
    *end somewhere

The default values for the standard deviations are those for BCRA grade 5.
Note that it is good practice to keep the ``*include grade3`` within ``*begin``
and ``*end`` commands otherwise it will apply to following survey data, which
may not be what you intended.

---------------------------
Override accuracy for a leg
---------------------------

For example, suppose the tape on the plumbed leg in this survey is
suspected of being less accurate than the rest of the survey
because the length was obtained by measuring the length of the
rope used to rig the pitch.  We can set a higher sd for this one
measurement and use a ``*begin``/``*end`` block to make sure this
setting only applies to the one leg:
::

    2 1 26.60 222  17.5
    2 3 10.85 014   7
    2 4  7.89 254 -11
    *begin
    ; tape measurement was taken from the rope length
    *sd tape 0.5 metres
    4 5  34.50 - DOWN
    *end
    5 6  9.29 271 -28.5

-----------------
Repeated Readings
-----------------

If your survey data contains multiple versions of each leg (for
example, pockettopo produces such data), then provided these are
adjacent to one another, Survex 1.2.17 and later will automatically
average these and treat them as a single leg.

------------------
Radiolocation Data
------------------

This is done by using the ``*sd`` command to specify the appropriate
errors for the radiolocation "survey leg" so that the loop
closure algorithm knows how to distribute errors if it forms part
of a loop.

The best approach for a radiolocation where the underground
station is vertically below the surface station is to represent it
as a plumbed leg, giving suitable SDs for the length and plumb
angle.  The horizontal positioning of this is generally quite
accurate, but the vertical positioning may be much less well
known.  E.g. we have a radiolocation of about 50m depth ±20m and
horizontal accuracy of ±8m.  Over 50m the ±8m is equivalent to
an angle of 9 degrees, so that is the expected plumb error.  20m is
the expected error in the length.  To get the equivalent SD we
assume that 99.74% of readings will be within 3 standard
deviations of the error value.  Thus we divide the expected errors
by 3 to get the SD we should specify:
::

    *begin
    *sd length 6.67 metres
    *sd plumb 3 degrees
    surface underground 50 - down
    *end

We wrap the radiolocation leg in a ``*begin``/``*end`` block to make
sure that the special \*sd settings only apply to this one leg.

For more information on the expected errors from radiolocations
see Compass Points Issue 10, available online at
http://www.chaos.org.uk/survex/cp/CP10/CPoint10.htm

-----------
Diving Data
-----------

Surveys made underwater using a diver's depth gauge can be
processed - use the ``*data`` command with the ``diving`` style
to specify that the data is of this type.

---------------
Theodolite data
---------------

Theodolite data with turned angles is not yet explicitly catered for, so for
now you will need to convert it into equivalent legs in another style -
``normal`` or ``cylpolar`` styles are likely to be the best choices.

If there is no vertical info in your theodolite data then you
should use the ``cylpolar`` style and use ``*sd`` command to specify very
low accuracy (high SD) in the depth so that the points will move
in the vertical plane as required if the end points are fixed or
the survey is part of a loop.
