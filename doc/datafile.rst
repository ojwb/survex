-----------------
Survex data files
-----------------

Survey data is entered in the form of text files.  You can use any text editor
you like for this, so long as it has the capability of writing a plain text
file in UTF-8 or ASCII encoding.  The data format is very flexible; unlike some
other cave surveying software, Survex does not require survey legs to be
rearranged to suit the computer, and the ordering of instrument readings on
each line is fully specifiable.  So you can enter your data much as it appears
on the survey notes, which is important in reducing the opportunities for
transcription errors.

Also all the special characters are user-definable - for example, the
separators can be spaces and tabs, or commas (e.g. when exporting from a
spreadsheet), etc; the decimal point can changed to be a comma (as used in
continental Europe), or a slash (sometimes used for clarity in written survey
notes), or anything else you care to choose.  This flexibility means that it
should be possible to read in data from almost any sort of survey data file
without much work.

Survex places no restrictions on you in terms of the ordering of survey legs.
You can enter or process data in any order and Survex will read it all in
before determining how it is connected.  You can also use the hierarchical
naming so that you do not need to worry about using the same station name
twice.

The usual arrangement is to have one file which lists all the others that are
included (e.g., ``161.svx``).  Then ``cavern 161`` will process all your data.
To just process a section use the filename for that section, e.g.  ``cavern
dtime`` will process the dreamtime file/section of Kaninchenhöhle.  To help you
out, if the survey has no fixed points and you are using Survex's default
unspecified coordinate system, ``cavern`` will pick a station and fix it at
(0,0,0) (and print a info message to this effect).

It is up to you what data you put in which files.  You can have one file per
trip, or per area of the cave, or just one file for the whole cave if you like.
On a large survey project it makes sense to group related surveys in the same
file or directory.

Readings
========

Blank lines (i.e. lines consisting solely of ``BLANK`` characters) are ignored,
except that in interleaved data a blank line ends the current traverse.

The last line in the file need not be terminated by an end of line character.

All fields on a line must be separated by at least one ``BLANK`` character.
An ``OMIT`` character (default ``-``) indicates that a field is omitted.  If the
field is not optional, then an error is given.

Survey Station Names
====================

Survex has a powerful system for naming stations.  It uses a hierarchy of
survey names, similar to the nested folders your computer stores files in.  So
point 6 in the entrance survey of Kaninchenhöhle (cave number 161) is referred
to as: 161.entrance.6

This seems a natural way to refer to station names.  It also means that it is
very easy to include more levels, for example if you want to plot all the caves
in the area you just list them all in another file, specifying a new prefix.
So to group 3 nearby caves on the Loser Plateau you would use a file like this::

    *begin Loser
    *include 161
    *include 2YrGest
    *include 145
    *end Loser

The entrance series point mentioned above would now be referred to as:
Loser.161.entrance.6

You do not have to use this system at all, and can just give all stations
unique identifiers if you like: 1, 2, 3, 4, 5, ... 1381, 1382 or AA06, AA07,
P34, ZZ6, etc.

Station and survey names may contain any alphanumeric characters and
additionally any characters in ``NAMES`` (default ``_`` and ``-``).
Alphabetic characters may be forced to upper or lower case by using the
``*case`` command.  Station names may be any length - if you want to only treat
the first few characters as significant you can get cavern to truncate the
names using the ``*truncate`` command.

Anonymous Stations
------------------

Survex supports the concept of anonymous survey stations, that is survey
stations without a name.  Each time an anonymous station name is used it
represents a different point.  Currently three types of anonymous station are
supported, referred to by one, two or three separator characters - with the
default separator of ``.``, that means ``.``, ``..``, and ``...`` are anonymous
stations.  Their meanings are:

Single separator (``.`` by default)
   An anonymous non-wall point at the end of an implicit splay.

Double separator (``..`` by default)
   An anonymous wall point at the end of an implicit splay.

Triple separator (``...`` by default)
   An anonymous point with no implicit flags on the leg (intended for cases
   like a disto leg along a continuing passage).

You can map ``-`` to ``..`` (for compatibility with data from pocket topo)
using the command::

    *alias station - ..

Support for anonymous stations and for ``*alias station - ..`` was added in
Survex 1.2.7.

Support for anonymous stations in the ``cartesian`` data style was added in
Survex 1.4.10.

Numeric fields
==============

Measurements start with an optional ``PLUS`` or ``MINUS`` character (``+``
and ``-`` by default), and have an optional decimal point (represented by a
``DECIMAL`` character, default: ``.``) which may be embedded, leading or
trailing.  No spaces are allowed between the various elements.

All of these are valid examples: +47, 23, -22, +4.5, 1.3, -0.7, +12., 23., -34,
+.15, .4, -.05

In formal syntax that's either:

``[<MINUS>|<PLUS>] <integer part> [ <DECIMAL> [ <decimal fraction> ] ]``

or

``[<MINUS>|<PLUS>] <DECIMAL> <decimal fraction>``

Accuracy
========

Accuracy assessments may be provided or defaulted for any survey leg.  These
determine the distribution of loop closure errors over the legs in the loop.
See ``*SD`` for more information.

Cavern Commands
===============

Commands in ``.svx`` files are introduced by an asterisk (by default - this can
be changed using the ``set`` command).

The commands are documented below in a common format:

- Command Name
- Syntax
- Example
- Validity
- Description
- Caveats
- See Also

ALIAS
-----

Syntax
   ``*alias station <alias> <target>``

   ``*alias station <alias>``

Example
   ::

       *begin parsons_nose
       *alias station - ..
       1 2 12.21 073 -12
       2 -  4.33 011 +02
       2 -  1.64 180 +03
       2 3  6.77 098 -04
       *end parsons_nose

Description
   ``*alias`` allows you to map a station name which appears in the survey data
   to a different name internally.  At present, you can only create an alias of
   ``-`` to ``..``, which is intended to support the pocket topo style notation
   of ``-`` being a splay to an anonymous point on the cave wall.  You can also
   unalias ``-`` with ``*alias station -``.

   Aliases are scoped by ``*begin``/``*end`` blocks - when a ``*end`` is
   reached, the aliases in force at the corresponding ``*begin`` are restored.

   ``*alias`` was added in Survex 1.2.7.

See Also
   ``*begin``, ``*end``

BEGIN
-----

Syntax
   ``*begin <survey>``

   ``*begin``

Example
   ::

       *begin littlebit
       1 2 10.23 106 -02
       2 3  1.56 092 +10
       *end littlebit

   ::

       ; length of leg across shaft estimated
       *begin
       *sd tape 2 metres
       9 10 6.   031 -07
       *end

Description
   ``*begin`` stores the current values of the current settings such as
   instrument calibration, data format, and so on.  These stored values are
   restored after the corresponding ``*end``.  If a survey name is given, this
   is used inside the ``*begin``/``*end`` block, and the corresponding ``*end``
   should have the same survey name.  ``*begin``/``*end`` blocks may be nested
   to indefinite depth.

See Also
   ``*end``, ``*prefix``

CARTESIAN
---------

Syntax
   ``*cartesian grid``

   ``*cartesian magnetic``

   ``*cartesian true``

   ``*cartesian grid <rotation> <units>``

   ``*cartesian magnetic <rotation> <units>``

   ``*cartesian true <rotation> <units>``

Example
   ::

       *cartesian magnetic

   ::

       *cartesian true 90 degrees

Description
   ``*cartesian`` specifies which North cartesian data is aligned to, and can
   optionally specify an extra rotation to apply.  The default is that it's
   aligned with True North.

   Notes on the different North options:

   ``GRID``
      North in the current input coordinate system (as set by e.g.  ``*cs
      UTM30``).  If no input or output coordinate system is set then this is
      the same as ``TRUE`` since in Survex's default unspecified coordinate
      system True North is the same as Grid North.
   ``MAGNETIC``
      Magnetic North.  If using automatically calculated declinations then
      this will be calculated at the ``*date`` in effect for each cartesian
      data reading.
   ``TRUE``
      True North.  If no input or output coordinate system is set then
      this is the same as ``GRID`` since in Survex's default unspecified
      coordinate system True North is the same as Grid North.

   ``*cartesian`` was added in Survex 1.4.10.  Prior to this cartesian data was
   documented as aligned with True North, but if an output coordinate system
   was specified it was actually aligned with this (which was not intended and
   doesn't really make sense since changing the output coordinate system would
   rotate cartesian data by the difference in grid convergence).

See Also
   ``*cs``, ``*data cartesian``, ``*date``, ``*declination``

CALIBRATE
---------

Syntax
   ``*calibrate <quantity list> <zero error>``

   ``*calibrate <quantity list> <zero error> <scale>``

   ``*calibrate <quantity list> <zero error> <units>``

   ``*calibrate <quantity list> <zero error> <units> <scale>``

   ``*calibrate default``

Example
   ::

       *calibrate tape +0.3

Description
   ``*calibrate`` is used to specify instrument calibrations, via a zero error
   and an optional scale factor (which defaults to 1.0 if not specified).
   Without an explicit calibration the zero error is 0.0 and the scale factor
   is 1.0.

   ``<quantity list>`` is one or more of:

      ============ ===========
      Quantity     Aliases
      ============ ===========
      LENGTH       TAPE
      BEARING      COMPASS
      GRADIENT     CLINO
      BACKLENGTH   BACKTAPE
      BACKBEARING  BACKCOMPASS
      BACKGRADIENT BACKCLINO
      COUNT        COUNTER
      LEFT          
      RIGHT         
      UP           CEILING
      DOWN         FLOOR
      DEPTH         
      EASTING      DX
      NORTHING     DY
      ALTITUDE     DZ
      DECLINATION   
      ============ ===========

   The specified calibration is applied to each quantity in the list, which is
   handy if you use the same instrument to measure several things, for example::

       *calibrate left right up down +0.1

   You need to be careful about the sign of the ZeroError.  Survex follows the
   convention used with scientific instruments - the ZeroError is what the
   instrument reads when measuring a reading which should be zero.  So for
   example, if your tape measure has the end missing, and you are using the
   30cm mark to take all measurements from, then a zero distance would be
   measured as 30cm and you would correct this with::

       *CALIBRATE tape +0.3

   If you tape was too long, starting at -20cm (it does happen!) then you can
   correct it with::

       *CALIBRATE tape -0.2

   Note: ZeroError is irrelevant for Topofil counters and depth gauges since
   pairs of readings are subtracted.

   In the first form in the synopsis above, the zero error is measured by the
   instrument itself (e.g. reading off the number where a truncated tape now
   ends) and any scale factor specified applies to it, like so (Scale defaults
   to 1.0)::

       Value = ( Reading - ZeroError ) * Scale

   In the second form above (supported since Survex 1.2.21), the zero error has
   been measured externally (e.g. measuring how much too long your tape is
   with a ruler) - the units of the zero error are explicitly specified and
   any scale factor is not applied to it::

       Value = ( Reading * Scale ) - ZeroError

   With the default scale factor of 1.0 the two forms are equivalent, though
   they still allow you to document how the zero error has been determined.

   With older Survex versions, you would specify the magnetic declination
   (difference between True North and Magnetic North) by using ``*calibrate
   declination`` to set an explicit value (with no scale factor allowed).
   Since Survex 1.2.22, it's recommended to instead use the new
   ``*declination`` command instead - see the documentation of that command for
   more details.

See Also
   ``*declination``, ``*units``

CASE
----

Syntax
   ``*case preserve``

   ``*case toupper``

   ``*case tolower``

Example
   ::

       *begin bobsbit
       ; Bob insists on using case sensitive station names
       *case preserve
       1 2   10.23 106 -02
       2 2a   1.56 092 +10
       2 2A   3.12 034 +02
       2 3    8.64 239 -01
       *end bobsbit

Description
   ``*case`` determines how the case of letters in survey names is handled.  By
   default all names are forced to lower case (which gives a case insensitive
   match), but you can tell cavern to force to upper case, or leave the case as
   is (in which case ``2a`` and ``2A`` will be regarded as different).

See Also
   ``*truncate``

COPYRIGHT
---------

Syntax
   ``*copyright <date> <text>``

Example
   ::

       *begin littlebit
       *copyright 1983 CUCC
       1 2 10.23 106 -02
       2 3  1.56 092 +10
       *end littlebit

Validity
   valid at the start of a ``*begin``/``*end`` block.

Description
   ``*copyright`` allows the copyright information to be recorded in a way that
   can be automatically collated.

See Also
   ``*begin``

CS
--

Syntax
   ``*cs <coordinate system>``

   ``*cs out <coordinate system>``

Example
   ::

       *cs UTM60S
       *fix beehive 313800 5427953 20

   ::

       ; Output in the coordinate system used in the Totes Gebirge in Austria
       *cs out custom "+proj=tmerc +lat_0=0 +lon_0=13d20 +k=1 +x_0=0 +y_0=-5200000 +ellps=bessel +towgs84=577.326,90.129,463.919,5.137,1.474,5.297,2.4232"

Description
   ``*cs`` allows the coordinate systems used for fixed points and for
   processed survey data to be specified.

   The "input" coordinate system is set with ``*cs`` and you can change it
   between fixed points if you have some fixed points in different coordinate
   systems to others.

   The "output" coordinate system is set with ``*cs out`` and is what the
   survey data is processed in and the coordinate system used for resultant
   ``.3d`` file.  The output coordinate system must be in metres with axis
   order (East, North, Up), so for example ``*cs out long-lat`` isn't valid
   because it isn't in metres, while ``*cs out jtsk`` isn't valid because
   the axes point West and South.

   ``*cs`` was added in Survex 1.2.14, but handling of fixed points specified
   with latitude and longitude didn't work until 1.2.21. Also ``*fix`` with
   standard deviations specified also didn't work until 1.2.21.

   The currently supported coordinate systems are:

   * ``EPSG:`` followed by a positive integer code.  EPSG codes cover most
     coordinate systems in use. The website https://epsg.io/ is a useful
     resource for finding the EPSG code you want.  For example, ``EPSG:4167``
     is NZGD2000.  Supported since Survex 1.2.15.

   * ``CUSTOM`` followed by a PROJ string (like in the example above).

   * ``ESRI:`` followed by a positive integer code.  ESRI codes are used by
     ArcGIS to specify coordinate systems (in a similar way to EPSG codes)
     and PROJ supports many of them.  Supported since Survex 1.2.15.

   * ``EUR79Z30`` for UTM zone 30, EUR79 datum.  Supported since Survex
     1.2.15.

   * ``IJTSK`` for the modified version of the Czechoslovak S-JTSK system
     where the axes point East and North.  Supported since Survex 1.2.15.

   * ``IJTSK03`` for a variant of IJTSK.  Supported since Survex 1.2.15.

   * ``JTSK`` for the Czechoslovak S-JTSK system.  Its axes point West and
     South, so it's not supported as an output coordinate system.  Supported
     since Survex 1.2.16.

   * ``JTSK03`` for a variant of JTSK.  Supported since Survex 1.2.16.

   * ``LONG-LAT`` for longitude/latitude.  The WGS84 datum is assumed.
     NB ``*fix`` expects the coordinates in the order x,y,z which means
     longitude (i.e. E/W), then latitude (i.e. N/S), then altitude.
     Supported since Survex 1.2.15.

   * ``OSGB:`` followed by a two letter code for the UK Ordnance Survey
     National Grid.  The first letter should be 'H', 'N', 'O', 'S' or 'T';
     the second any letter except 'I'.  For example, ``OSGB:SD``.  Supported
     since Survex 1.2.15.

   * ``S-MERC`` for the "Web Mercator" spherical mercator projection, used by
     online map sites like OpenStreetMap, Google maps, Bing maps, etc.
     Supported since Survex 1.2.15.

   * ``UTM`` followed by a zone number (1-60), optionally followed
     by "N" or "S" specifying the hemisphere (default is North).  The WGS84
     datum is assumed.  A potential source of confusion here is the
     `Military Grid Reference System
     <https://en.wikipedia.org/wiki/Military_Grid_Reference_System>`__
     which divides each UTM zone into latitude bands represented by a
     letter suffix, so here 33S and 33N have different meanings to those
     in Survex - they are both parts of UTM zone 33, but both are in the
     Northern hemisphere (33S is around Sicily, 33N around Cameroon).
     To use such coordinates in Survex, replace suffixes "C" to "M" with "S",
     and "N" to "X" with "N".

   By default, Survex works in an unspecified coordinate system (and this was
   the only option before ``*cs`` was added).  However, it's useful for the
   coordinate system which the processed survey data is in to be specified if
   you want to use the processed data in ways which required knowing the
   coordinate system (such as exporting a list of entrances for use in a
   GPS).  You can now do this by using ``*cs out``.

   It is also useful to be able to take coordinates for fixed points in
   whatever coordinate system you receive them in and put them directly into
   Survex, rather than having to convert with an external tool.  For example,
   you may have your GPS set to show coordinates in UTM with the WGS84 datum,
   even though you want the processed data to be in some local coordinate
   system.  Someone else may provide GPS coordinates in yet another
   coordinate system.  You just need to set the appropriate coordinate system
   with ``*cs`` before each group of ``*fix`` commands in a particular
   coordinate system.

   If you're going to make use of ``*cs``, then a coordinate system must be
   specified for everything, so a coordinate system must be in effect for all
   ``*fix`` commands, and you must set the output coordinate system before
   any points are fixed.

   Also, if ``*cs`` is in use, then you can't omit the coordinates in a
   ``*fix`` command, and a fixed point won't be invented if none exists.

   If you use ``*cs out`` more than once, the second and subsequent commands
   are silently ignored - this makes it possible to combine two datasets with
   different ``*cs out`` settings without having to modify either of them.

   Something to be aware of with ``*cs`` is that altitudes are currently
   assumed to be "height above the ellipsoid", whereas GPS units typically
   give you "height above sea level", or more accurately "height above a
   particular geoid".  This is something we're looking at how best to
   address, but you shouldn't need to worry about it if your fixed points are
   in the same coordinate system as your output, or if they all use the same
   ellipsoid.  For a more detailed discussion of this, please see:
   http://expo.survex.com/handbook/survey/coord.htm

See Also
   ``*fix``

DATA
----

Syntax
   ``*data <style> <ordering>``

   ``*data``

   ``*data default

   ``*data ignore``

Example
   ::

       *data normal from to compass tape clino

   ::

       *data normal station ignoreall newline compass tape clino

Description
   ``<style>``
      ``NORMAL|DIVING|CARTESIAN|TOPOFIL|CYLPOLAR|NOSURVEY|PASSAGE``

   ``<ordering>``
      ordered list of instruments - which are valid depends on the style.

   In Survex 1.0.2 and later, ``TOPOFIL`` is simply a synonym for ``NORMAL``,
   left in to allow older data to be processed without modification.  Use the
   name ``NORMAL`` by preference.

   Most of the styles support two variants - interleaved and non-interleaved.
   Non-interleaved is "one line per leg", interleaved has a line for the data
   shared between two legs (e.g. ``STATION``:``FROM``/``TO``,
   ``DEPTH``:``FROMDEPTH``/``TODEPTH``, ``COUNT``:``FROMCOUNT``/``TOCOUNT``).
   Note that not all readings that can be shared have to be, for example here
   the to/from station name is shared but the depth gauge readings aren't::

       *data diving station newline fromdepth compass tape todepth

   In addition, interleaved data can have a ``DIRECTION`` reading, which can
   be ``F`` for a foresight or ``B`` for a backsight (meaning the direction of
   the leg is reversed).

   In interleaved data, a blank line (one which contains only characters
   which are set as ``BLANK``) ends the current traverse so can be used to
   handle branches in the survey, e.g.::

       *data normal station newline tape compass clino

       1
           9.34   087   -05
       2
           ; single leg up unexplored side passage
           4.30   002    +06
       3

       2
           ; and back to the main package
           6.29   093    -02
       4

   In data styles which include a ``TAPE`` reading (i.e. ``NORMAL``,
   ``DIVING``, and ``CYLPOLAR`` data styles), ``TAPE`` may be replaced by
   ``FROMCOUNT``/``TOCOUNT`` (or ``COUNT`` in interleaved data) to allow
   processing of surveys performed with a Topofil instead of a tape.

   In Survex 1.2.44 and later, you can use ``*data`` without any arguments to
   keep the currently set data style, but resetting any state.  This is useful
   when you're entering passage tubes with branches - see the
   description of the ``PASSAGE`` style below. (This feature was originally
   added in 1.2.31, but was buggy until 1.2.44 - any data up to the next
   ``*data`` gets quietly ignored.)


   DEFAULT
      Select the default data style and ordering (``NORMAL`` style, ordering:
      ``from to tape compass clino``).

   IGNORE
      Ignores survey data until another ``*data`` command sets a different
      style, or until the end of the enclosing ``*begin``...\ ``*end`` block.
      Note that commands are still processed, only survey data is ignored.
      This is useful if you have some survey data which has been superseded by
      a better survey of the same passage, but you want to keep the superseded
      data around, just not process it.  Added in Survex 1.4.11.

   NORMAL
      The usual tape/compass/clino centreline survey. For non-interleaved data
      the allowed readings are: ``FROM`` ``TO`` ``TAPE`` ``COMPASS`` ``CLINO``
      ``BACKTAPE`` ``BACKCOMPASS`` ``BACKCLINO``; for interleaved data the
      allowed readings are: ``STATION`` ``DIRECTION`` ``TAPE`` ``COMPASS``
      ``CLINO`` ``BACKTAPE`` ``BACKCOMPASS`` ``BACKCLINO``.

      ``BACKTAPE`` was added in Survex 1.2.25.

      The ``CLINO``/``BACKCLINO`` reading is not required - if it is omitted
      in the ``*data`` command then the vertical standard deviation is taken to
      be proportional to the tape measurement for all reading.  Alternatively,
      if the reading is included in the ``*data`` command then individual clino
      readings can be given as ``OMIT`` (default ``-``) and will be treated in
      this way, which allows for data where only some clino readings are
      missing.

      Examples of style ``NORMAL``:
      ::

             *data normal from to compass clino tape
             1 2 172 -03 12.61
             2 3 202  -   8.59 ; clino not recorded

      ::

             *data normal station newline direction tape compass clino
             1
               F 12.61 172 -03
             2

      ::

             *data normal from to compass clino fromcount tocount
             1 2 172 -03 11532 11873

      ::

             *data normal station count newline direction compass clino
             1 11532
               F 172 -03
             2 11873

      DIVING
         An underwater survey where the vertical information is from a diver's
         depth gauge.  This style can also be also used for an above-water
         survey where the altitude is measured with an altimeter.  ``DEPTH`` is
         defined as the altitude (Z) so increases upwards by default.  So for a
         diver's depth gauge, you'll need to use ``*CALIBRATE`` with a negative
         scale factor (e.g. ``*calibrate depth 0 -1``).

         For non-interleaved data the allowed readings are: ``FROM`` ``TO``
         ``TAPE`` ``COMPASS`` ``CLINO`` ``BACKTAPE`` ``BACKCOMPASS``
         ``BACKCLINO`` ``FROMDEPTH`` ``TODEPTH`` ``DEPTHCHANGE`` (the vertical
         can be given as readings at each station, (``FROMDEPTH``/``TODEPTH``)
         or as a change along the leg (``DEPTHCHANGE``)).

         ``BACKTAPE`` was added in Survex 1.2.25.

         For interleaved data the allowed readings are: ``STATION``
         ``DIRECTION`` ``TAPE`` ``COMPASS`` ``BACKTAPE`` ``BACKCOMPASS``
         ``DEPTH`` ``DEPTHCHANGE``.  The vertical change can be given as a
         reading at the station (``DEPTH``) or as a change along the leg
         (``DEPTHCHANGE``)::

             *data diving from to tape compass fromdepth todepth
             1 2 14.7 250 -20.7 -22.4

         ::

             *data diving station depth newline tape compass
             1 -20.7
              14.7 250
             2 -22.4

         ::

             *data diving from to tape compass depthchange
             1 2 14.7 250 -1.7

         Survex 1.2.20 and later allow an optional ``CLINO`` and/or
         ``BACKCLINO`` reading in ``DIVING`` style.  At present these extra
         readings are checked for syntactic validity, but are otherwise
         ignored.  The intention is that a future version will check them
         against the other readings to flag up likely blunders, and average
         with the slope data from the depth gauge and tape reading.

      CARTESIAN
         Cartesian data style allows you to specify the (x,y,z) changes between
         stations.  It's useful for digitising surveys where the original
         survey data has been lost and all that's available is a drawn
         up version.

         ::

             *data cartesian from to northing easting altitude
             1 2 16.1 20.4 8.7

         ::

             *data cartesian station newline northing easting altitude
             1
               16.1 20.4 8.7
             2

         By default, the North used is True North, but you can specify to use
         Magnetic or Grid North (in the input coordinate system) with the
         ``*cartesian`` command, and also specify an additional rotation to
         apply (since Survex 1.4.10).

         In Survex < 1.4.10, if ``*cs`` was used then cartesian data were
         incorrectly interpreted as relative to Grid North in the output
         coordinate system (if ``*cs`` is not used then Grid North in the
         default unspecified coordinate system is the same as True North).

      CYLPOLAR
         A ``CYLPOLAR`` style survey is very similar to a diving survey, except
         that the tape is always measured horizontally rather than along the
         slope of the leg.

         ::

             *data cylpolar from to tape compass fromdepth todepth
             1 2 9.45 311 -13.3 -19.0

         ::

             *data cylpolar station depth newline tape compass
             1 -13.3
              9.45 311
             2 -19.0

         ::

             *data cylpolar from to tape compass depthchange
             1 2 9.45 311 -5.7

      NOSURVEY
         A ``NOSURVEY`` survey doesn't have any measurements - it merely
         indicates that there is line of sight between the pairs of stations.

         ::

             *data nosurvey from to
             1 7
             5 7
             9 11

         ::

             *data nosurvey station
             1
             7
             5
         
             *data
             9
             11

      PASSAGE
         This survey style defines a 3D "tube" modelling a passage in the cave.
         The tube joins the survey stations listed in the order listed.  It's
         permitted to go between survey stations which aren't directly linked
         by the centre-line survey.  This can be useful - sometimes the
         centreline will step sideways or up/down to allow a better sight for
         the next leg and you can ignore the extra station.  You can also
         define tubes along unsurveyed passages, akin to "nosurvey" legs in the
         centreline data.

         This means that you need to split off side passages into separate
         tubes, and hence separate sections of passage data, starting with a
         new ``*data`` command with no arguments.

         Simple example of how to use this data style (note the use of
         ignoreall to allow a free-form text description to be given)::

             *data passage station left right up down ignoreall
             1  0.1 2.3 8.0 1.4  Sticking out point on left wall
             2  0.0 1.9 9.0 0.5  Point on left wall
             3  1.0 0.7 9.0 0.8  Highest point of boulder

         Each ``*data passage`` data block describes a single continuous tube -
         to break a tube or to enter a side passage you need to have a second
         block. With Survex 1.2.30 and older, you had to repeat the entire
         ``*data passage`` line to start a new tube, but in Survex 1.2.31 and
         later, you can just use ``*data`` without any arguments.

         For example here the main passage is 1-2-3 and a side passage is 2-4::

             *data passage station left right up down ignoreall
             1  0.1 2.3 8.0 1.4  Sticking out point on left wall
             2  0.0 1.9 9.0 0.5  Point on left wall opposite side passage
             3  1.0 0.7 9.0 0.8  Highest point of boulder
         
             ; If you need to be compatible with Survex 1.2.30 or earlier
             ; you need to repeat the full "*data" command here instead.
             *data
             2  0.3 0.2 9.0 0.5
             4  0.0 0.5 6.5 1.5  Fossil on left wall

   ``IGNORE`` skips a field (it may be used any number of times), and
   ``IGNOREALL`` may be used last to ignore the rest of the data line.

   ``LENGTH`` is a synonym for ``TAPE``; ``BEARING`` for ``COMPASS``;
   ``GRADIENT`` for ``CLINO``; ``COUNT`` for ``COUNTER``.

   The units of each quantity may be set with the ``*units`` command.

See Also
   ``*units``

DATE
----

Syntax
   ``*date <date>``

   ``*date <date1>-<date2>``

Example
   ::

       *date 2001

   ::

       *date 2000.10

   ::

       *date 1987.07.27

   ::

       *date 1985.08.12-1985.08.13

Validity
   valid at the start of a ``*begin``/``*end`` block.

Description
   ``*date`` specifies the date that the survey was done.  A range of dates can
   be specified (useful for overnight or multi-day surveying trips).

   Dates must be in the order year then month then day, the day or month and day
   can be omitted.  The separator between components must be ``.``.

   Dates with just a year (e.g. ``2001``) are treated as being in the middle of
   that year.  Dates with a month and year (e.g. ``2000.10``) are treated as
   being in the middle of that month.

See Also
   ``*begin``, ``*instrument``, ``*team``

DECLINATION
-----------

Syntax
   ``*declination auto <x> <y> <z>``

   ``*declination <declination> <units>``

Description
   The ``*declination`` command is the modern way to specify magnetic
   declinations in Survex.  Magnetic declination is the difference between
   Magnetic North and True North.  It varies over time as the Earth's magnetic
   field moves, and also with location.  Compass bearings are measured relative
   to Magnetic North - adding the magnetic declination gives bearings relative
   to True North.

   Prior to 1.2.22, ``*calibrate declination`` was used instead.  If you use a
   mixture of ``*calibrate declination`` and ``*declination``, they interact in
   the natural way - whichever was set most recently is used for each compass
   reading (taking into account survey scope).  We don't generally recommend
   mixing the two, but it's useful to understand how they interact if you want
   to combine datasets using the old and new commands, and perhaps if you have
   a large existing dataset and want to migrate it without having to change
   everything at once.

   Note that the value specified uses the conventional sign for magnetic
   declination, unlike the old ``*calibrate declination`` which needed a value
   with the opposite sign (because ``*calibrate`` specifies a zero error), so
   take care when updating old data, or if you're used to the semantics of
   ``*calibrate declination``.

   If you have specified the output coordinate system (using ``*cs out``) then
   you can use ``*declination auto`` (and we recommend that you do).  This is
   supported since Survex 1.2.21 and automatically calculates magnetic
   declinations based on the IGRF (International Geomagnetic Reference
   Field) model.  A revised version of the IGRF model is usually issued every 5
   years, and calculates values using a model based on observations for years
   before it is issued, and on predictions for 5 years after it is issued.
   Survex 1.2.43 updated to using version 13 in early 2020.

   The IGRF model takes a date and a location as inputs.  Survex uses the
   specified date of the survey, and uses the "x y z" coordinates specified in
   the ``*declination auto`` command as the location in the current input
   coordinate system (as set by ``*cs``).  Most users can just specify a single
   representative location somewhere in the area of the cave.  If you're not
   sure what to use pick some coordinates roughly in the middle of the bounding
   box of the cave - it doesn't need to be a fixed point or a known refindable
   location, though it can be if you prefer.

   Survex 1.2.27 and later also automatically correct for grid convergence (the
   difference between Grid North and True North) when ``*declination auto`` is
   in use, based on the same specified representative location.

   You might wonder why Survex needs a representative location instead of
   calculating the magnetic declination and grid convergence for the actual
   position of each survey station.  The reason is that we need to adjust the
   compass bearings before we can solve the network to find survey station
   locations.  Both magnetic declination and grid convergence don't generally
   vary significantly over the area of a typical cave system - if you are
   mapping a very large cave system, or caves over a wide area, or are working
   close to a magnetic pole or where the output coordinate system is rather
   distorted, then you can specify ``*declination auto`` several times with
   different representative locations for different areas of the cave system -
   the one currently in effect is used for each survey leg.

   For each ``*declination auto`` command cavern will (since Survex 1.4.2)
   report the range of calculated declination values and the dates at which the
   ends of the range were obtained, and also the grid convergence (which
   doesn't vary with time).  This appears in the log - if you processed the
   data with aven you can view this by using "File->View Log".  It looks like
   this::

       1623.svx:20: info: Declination: -0.4° @ 1977-07-02 / 3.8° @ 2018-07-21, grid convergence: -0.9°
        *declination auto 36670.37 83317.43 1903.97

   Generally it's best to specify a suitable output coordinate system, and use
   ``*declination auto`` so Survex corrects for magnetic declination and grid
   convergence for you.  Then Aven knows how to translate coordinates to
   allow export to formats such as GPX and KML, and to overlay terrain data
   and other geodata.

   If you don't specify an output coordinate system, but fix one or more points
   then Survex works implicitly in the coordinate system your fixed points were
   specified in.  This mode of operation is provided for compatibility with
   datasets from before support for explicit coordinate systems was added to
   Survex - it's much better to specify the output coordinate system as above.
   But if you have a survey of a cave which isn't connected to any known fixed
   points then you'll need to handle it this way, either fixing an entrance to
   some arbitrary coordinates (probably (0,0,0)) or letting Survex pick a
   station as the origin. If the survey was all done in a short enough period
   of time that the magnetic declination won't have changed significantly, you
   can just ignore it and Grid North in the implicit coordinate system will be
   Magnetic North at the time of the survey.  If you want to correct
   for magnetic declination, you can't use ``*declination auto`` because the
   IGRF model needs the real world coordinates, but you can specify literal
   declination values for each survey using ``*declination <declination>
   <units>``.  Then Grid North in the implicit coordinate system is True North.

See Also
   ``*calibrate``

DEFAULT
-------

Syntax
   ``*default calibrate``

   ``*default data``

   ``*default units``

Description
   ``*default`` restores defaults for given settings.  This command is
   deprecated - you should instead use: ``*calibrate default``, ``*data
   default``, ``*units default``.

See Also
   ``*calibrate``, ``*data``, ``*units``

END
---

Syntax
   ``*end <survey>``

   ``*end``

Validity
   valid for closing a block started by ``*begin`` in the same file.

Description
   Closes a block started by ``*begin``.

See Also
   ``*begin``

ENTRANCE
--------

Syntax
   ``*entrance <station>``

Example
   ::

       *entrance P163

Description
   ``*entrance`` marks a station as an entrance.  This information is
   used by aven to allow entrances to be highlighted.

EQUATE
------

Syntax
   ``*equate <station> <station>...``

Example
   ::

       *equate chosspot.1 triassic.27

Description
   ``*equate`` specifies that the station names in the list refer to the same
   physical survey station.  An error is given if there is only one station
   listed.

See Also
   ``*infer equates``

EXPORT
------

Syntax
   ``*export <station>...``

Example
   ::

       *export 1 6 17

Validity
   valid at the start of a ``*begin``/``*end`` block.

Description
   ``*export`` marks the stations named as referable to from the enclosing
   survey.  To be able to refer to a station from a survey several levels
   above, it must be exported from each enclosing survey.

See Also
   ``*begin``, ``*infer exports``

FIX
---

Syntax
   ``*fix <station> [reference] <x> <y> <z>``

   ``*fix <station> [reference] <x> <y> <z> <std err>``

   ``*fix <station> [reference] <x> <y> <z> <horizontal std err> <vertical std err>``

   ``*fix <station> [reference] <x> <y> <z> <x std err> <y std err> <z std err>``

   ``*fix <station> [reference] <x> <y> <z> <x std err> <y std err> <z std err> <cov(x,y)> <cov(y,z)> <cov(z,x)>``

   ``*fix <station>``

Example
   ::

       *fix entrance.0 32768 86723 1760

   ::

       *fix KT114_96 reference 36670.37 83317.43 1903.97

Description
   ``*fix`` fixes the position of <station> at the given coordinates.  If you
   haven't specified the coordinate system with ``*cs``, you can omit the
   position and it will default to (0,0,0) which provides an easy way to
   specify a point to arbitrarily fix rather than rely on ``cavern`` picking
   one (which has the downsides of the choice potentially changing when more
   survey data is added, and of triggering an "info" message).

   The standard errors default to zero (fix station exactly).  ``cavern`` will
   give an error if you attempt to fix the same survey station twice at
   different coordinates, or a warning if you fix it twice with matching
   coordinates.

   You can also specify just one standard error (in which case it is assumed
   equal in X, Y, and Z) or two (in which case the first is taken as the
   standard error in X and Y, and the second as the standard error in Z).

   If you have covariances for the fix, you can also specify these - the order
   is cov(x,y) cov(y,z) cov(z,x).

   If you've specified a coordinate system (see ``*cs``) then that determines
   the meaning of X, Y and Z (if you want to specify the units for altitude,
   note that using a PROJ string containing ``+vunits`` allows this - e.g.
   ``+vunits=us-ft`` for US survey feet).  If you don't specify a coordinate
   system, then the coordinates must be in metres.  The standard deviations
   must always be in metres (and the covariances in metres squared).

   You can fix as many stations as you like - just use a ``*fix`` command for
   each one.  Cavern will check that all stations are connected to at least one
   fixed point so that co-ordinates can be calculated for all stations.  If
   there is unconnected survey data then you'll get a warning (since Survex
   1.4.10; in earlier versions this was an error) and only the connected data
   is processed.

   By default cavern will warn about stations which have been ``*fix``-ed but
   are not used otherwise, as this might be due to a typo in the station name.
   Uses in survey data and (since 1.4.9) ``*entrance`` count for these
   purposes.  This warning is unhelpful if you want to include a standard file
   of benchmarks, some of which won't be used.  In this sort of situation,
   specify ``reference`` after the station name in the ``*fix`` command to
   suppress this warning for a particular station.  It's OK to use
   ``reference`` on a station which is used.

   Since Survex 1.4.10 it's an error to specify ``reference`` without
   coordinates (e.g. ``*fix a reference``) as this usage doesn't really make
   sense.

   .. note:: X is Easting, Y is Northing, and Z is altitude.  This convention
      was chosen since on a map, the horizontal (X) axis is usually East, and
      the vertical axis (Y) North.  The choice of altitude (rather than depth)
      for Z is taken from surface maps, and makes for less confusion when
      dealing with cave systems with more than one entrance.  It also gives a
      right-handed set of axes.

FLAGS
-----
Syntax
   ``*flags <flags>``

Example
   ::

       *flags duplicate not surface

Description
   ``*flags`` updates the currently set flags.  Flags not mentioned retain
   their previous state.  Valid flags are ``duplicate``, ``splay``, and
   ``surface``, and a flag may be preceded with ``not`` to turn it off.

   Survey legs marked ``surface`` are hidden from plots by default, and not
   included in cave survey length calculations.

   Survey legs marked as ``duplicate`` or ``splay`` are also not included in
   cave survey length calculations; legs marked ``splay`` are ignored by the
   extend program.  ``duplicate`` is intended for the case when if you have two
   different surveys along the same section of passage (for example to tie two
   surveys into a permanent survey station); ``splay`` is intended for cases
   such as radial legs in a large chamber, or to walls and other features with
   a disto-x or similar device.

See Also
   ``*begin``

INCLUDE
-------

Syntax
   ``*include <filename>``

Example
   ::

       *include mission

   ::

       *include "the pits"

Description
   ``*include`` processes ``<filename>`` as if it were inserted at this place
   in the current file - i.e. the current settings are carried into the included
   file and any alterations to settings in the included file will be carried
   back again.  There's one exception to this for historical reasons, which is
   that the survey prefix is restored upon return to the original file.  Since
   ``*begin`` and ``*end`` nesting cannot cross files, this can only make a
   difference if you use the deprecated ``*prefix`` command.

   If the filename contains spaces, it must be enclosed in double quotes.

   An included file which does not have a complete path is resolved relative to
   the directory which the parent file is in (just as relative HTML links do).

   The included file can be any filetype which cavern can process, so you can
   ``*include compassdata.mak``, ``*include compassdata.dat``, ``*include
   wallsdata.wpj`` or ``*include wallsdata.srv`` to allow processing
   mixed-format datasets.

   If the filename as specified is not found, cavern will try adding a ``.svx``
   extension, and will also try translating ``\`` to ``/``.

.. comment to workaround vim .rst highlighting bug ``

   To help users wanting to take a dataset from a platform where filenames are
   case-insensitive and process it on a platform where filenames are
   case-sensitive, if the file isn't found cavern will try a few variations of
   the case.  First it will try all lower case (in Survex 1.4.5 and older this
   was the only case variant tried), then all lower case except with the first
   character of the leafname upper case, and finally all upper case.  These
   different variants are only tried if the case as given doesn't match so
   there's no overhead in the normal situation.

   One specific trick this enables which is worth noting is that if you're
   running Survex on a system with case-sensitive filenames (which Linux and
   other Unix-like systems typically are) and someone sends you a dataset in a
   ZIP archive with mismatched filename case, you can unzip it using ``unzip
   -L`` to unpack all the filenames in lower case and ``cavern`` should
   successfully process it.

   The depth to which you can nest include files may be limited by the
   operating system you use.  Usually the limit on modern platforms is
   high (e.g. the default is 1024 files per process on Linux) but if you want
   to be able to process your dataset with Survex on any supported platform, it
   would be prudent not to go overboard with deeply nested include files.

INFER
-----

Syntax
   ``*infer plumbs on``

   ``*infer plumbs off``

   ``*infer equates on``

   ``*infer equates off``

   ``*infer exports on``

   ``*infer exports off``

Description
   ``*infer plumbs on`` tells cavern to interpret gradients of ±90 degrees
   as UP/DOWN (so it will not apply the clino correction to them).  This is
   useful when you have data which uses this convention for plumbed legs.

   ``*infer equates on`` tells cavern to interpret a leg with a tape reading of
   zero as a ``*equate`` which this prevents tape corrections being applied to
   them.  This is useful when you have data which uses this convention for
   equating stations.

   ``*infer exports on`` is necessary when you have a dataset which is partly
   annotated with ``*export``.  It tells cavern not to complain about missing
   ``*export`` commands in the parts of the dataset it is enabled for.  Also
   stations which were used to join surveys are marked as exported in the 3d
   file.

INSTRUMENT
----------

Syntax
   ``*instrument <instrument> <identifier>``

Example
   ::

       *instrument compass "CUCC 2"
       *instrument clino "CUCC 2"
       *instrument tape "CUCC Fisco Ranger open reel"

Validity
   valid at the start of a ``*begin``/``*end`` block.

Description
   ``*instrument`` specifies the particular instruments used to perform a
   survey.

See Also
   ``*begin``, ``*date``, ``*team``

PREFIX
------

Syntax
   ``*prefix <survey>``

Example
   ::

       *prefix flapjack

Description
   ``*prefix`` sets the current survey.

Caveats
   ``*prefix`` is deprecated - you should use ``*begin`` and ``*end`` instead.

See Also
   ``*begin``, ``*end``

REF
---

Syntax
   ``*ref <string>``

Example
   ::

       *ref "survey folder 2007#12"

Validity
   valid at the start of a ``*begin``/``*end`` block.

Description
   ``*ref`` allows you to specify a reference.  If the reference contains
   spaces, you must enclose it in double quotes.  Survex doesn't try to
   interpret the reference in any way, so it's up to you how you use it - for
   example it could specify where the original survey notes can be found.

   ``*ref`` was added in Survex 1.2.23.

See Also
   ``*begin``, ``*date``, ``*instrument``, ``*team``

REQUIRE
-------

Syntax
   ``*require <version>``

Example
   ::

       *require 0.98

Description
   ``*require`` checks that the version of cavern in use is at least
   ``<version>`` and stops with an error if not.

   If your dataset requires a feature introduced in a particular version, you
   can add a ``*require`` command and users will know what version they need to
   upgrade to, rather than getting an error message and having to guess what
   the real problem is.

SD
--

Syntax
   ``*sd <quantity list> <standard deviation> <units>``

Example
   ::

       *sd tape 0.15 metres

   ::

       *sd compass backcompass clino backclino 0.5 degrees

Description
   ``*sd`` specifies the standard deviation of a measurement.

   ``<quantity>`` is one of (each group gives alternative names
   for the same quantity):

   - TAPE, LENGTH
   - BACKTAPE, BACKLENGTH (added in Survex 1.2.25)
   - COMPASS, BEARING
   - BACKCOMPASS, BACKBEARING
   - CLINO, GRADIENT
   - BACKCLINO, BACKGRADIENT
   - COUNTER, COUNT
   - DEPTH
   - DECLINATION
   - DX, EASTING
   - DY, NORTHING
   - DZ, ALTITUDE
   - LEFT
   - RIGHT
   - UP, CEILING
   - DOWN, FLOOR
   - LEVEL
   - PLUMB
   - POSITION

   ``<standard deviation>`` is a positive real number, e.g. ``0.05``.

   ``<units>`` specifies the units, which must be appropriate for the
   quantity or quantities in the list (e.g. ``metres`` for distance,
   ``degrees`` for an angle).  See ``*units`` below for full list of valid
   units.

   To utilise this command fully you need to understand what
   a *standard deviation* is.  It gives a value to the
   'spread' of the errors in a measurement.  Assuming that
   these are normally distributed we can say that 95.44% of
   the actual lengths will fall within two standard
   deviations of the measured length, e.g. a tape SD of 0.25
   metres means that the actual length of a tape measurement
   is within ±0.5 metres of the recorded value 95.44%
   of the time.  So if the measurement is 7.34m then the
   actual length is very likely to be between 6.84m and
   7.84m.  This example corresponds to BCRA grade 3.  Note
   that this is just one interpretation of the BCRA
   standard, taking the permitted error values as 2SD 95.44%
   confidence limits.  If you want to take the readings as
   being some other limit (e.g. 1SD = 68.26%) then you will
   need to change the BCRA3 and BCRA5 files accordingly.
   This issue is explored in more detail in various
   surveying articles.

See Also
   ``*units``

SET
---

Syntax
   ``*set <item> <character list>``

Example
   ::

       *set blank x09x20
       *set decimal ,

   This example sets the decimal separator to be a comma.  Note that here we
   need to eliminate comma from being a blank before setting it as a decimal -
   otherwise the comma in ``*set decimal ,`` is parsed as a blank, and cavern
   sets decimal to not have any characters representing it.

   Here's an example of how to allow additional characters in station names:
   ::

       *set names ?+_

   After this ``?``, ``+`` and ``_`` will be allowed in names (in addition to
   characters A-Z, a-z and 0-9 which are always allowed).  ``*set`` replaces
   the previous setting so while ``_`` and ``-`` are both allowed by default,
   ``-`` no longer is after this command because it isn't in the list.

Description
   ``*set`` sets the specified ``<item>`` to the character or characters given
   in ``<character list>``.

   The characters specified in ``<character list>`` may not be alphanumeric
   since these characters are used for station names and for readings.

   In ``<character list>``, ``x`` followed by two hex digits means the
   character with that hex value, e.g. ``x20`` is a space.

   The complete list of items that can be set, the defaults (in brackets), and
   the meaning of the item, is:

   BLANK (``x09x20,``)
      Separates fields
   COMMENT (``;``)
      The rest of the current line is a comment
   DECIMAL (``.``)
      Decimal point character
   EOL (``x0Ax0D``)
      End of line character
   KEYWORD (``*``)
      Introduces keywords
   MINUS (``-``)
      Indicates negative number
   NAMES (``_-``)
      Non-alphanumeric chars permitted in station names (letters and numbers
      are always permitted).
   OMIT (``-``)
      Contents of field omitted (e.g. in plumbed legs)
   PLUS (``+``)
      Indicates positive number
   ROOT (``\``)
      Prefix in force at start of current file (use of ``ROOT`` is deprecated)
   SEPARATOR (``.``)
      Level separator in prefix hierarchy

SOLVE
-----

Syntax
   ``*solve``

Example
   ::

       *include 1997data
       *solve
       *include 1998data

Description
   ``*solve`` distributes misclosures around any loops in the survey (in the
   same way that happens implicitly after reading all the data), then fixes
   the positions of all existing stations.

   This command is intended for situations where you have some new surveys
   adding extensions to an already drawn-up survey which you wish to avoid
   completely redrawing.  You can read in the old data, use ``*solve`` to
   fix it, and then read in the new data.  Then old stations will be in the
   same positions as they are in the existing drawn up survey, even if new
   loops have been formed by the extensions.

TEAM
----

Syntax
   ``*team <person> [<role>...]``

Example
   ::

       *team "Nick Proctor" compass clino tape
       *team "Anthony Day" notes pictures tape

Validity
   valid at the start of a ``*begin``/``*end`` block.

Description
   ``*team`` specifies the people involved in a survey and optionally what role
   or roles they filled during that trip. Unless the person is only identified
   by one name you need to put double quotes around their name.

See Also
   ``*begin``, ``*date``, ``*instrument``

TITLE
-----

Syntax
   ``*title <title>``

Example
   ::

       *title Dreamtime

   ::

       *title "Mission Impossible"

Description
   ``*title`` allows you to set a descriptive title for a survey.  If the title
   contains spaces, you need to enclose it in double quotes ("").  If there is
   no ``*title`` command, the title defaults to the survey name given in the
   ``*begin`` command.

TRUNCATE
--------

Syntax
   ``*truncate <length>``

   ``*truncate off``

Description
   Station names may be of any length in Survex, but some other (mostly older)
   cave surveying software only regards the first few characters of a name as
   significant (e.g. "entran" and "entrance" might be treated as the same).
   To facilitate using data imported from such a package, Survex allows you to
   truncate names to whatever length you want (but by default truncation is
   off).

   Figures for the number of characters which are significant in various
   software packages: Compass currently has a limit of 12, CMAP has a limit of
   6, Smaps 4 had a limit of 8, Surveyor87/8 used 8. Survex itself used 8 per
   prefix level up to version 0.41, and 12 per prefix level up to 0.73 (more
   recent versions removed this rather archaic restriction).

See Also
   ``*case``

UNITS
-----

Syntax
   ``*units <quantity list> [<factor>] <unit>``

   ``*units default``

Example
   ::

       *units tape metres

   ::

       *units compass backcompass clino backclino grads

   ::

       *units dx dy dz 1000 metres ; data given as kilometres

   ::

       *units left right up down feet

Description
   ``*units`` changes the current units of all the quantities listed to
   [<factor>] <unit>.  Note that quantities can be expressed either as the
   instrument (e.g. ``COMPASS``) or the measurement (e.g. ``BEARING``).

   ``<quantity>`` is one of the following (grouped entries are
   just alternative names for the same thing):

   - ``TAPE``/``LENGTH``
   - ``BACKTAPE``/``BACKLENGTH`` (added in Survex 1.2.25)
   - ``COMPASS``/``BEARING``
   - ``BACKCOMPASS``/``BACKBEARING``
   - ``CLINO``/``GRADIENT``
   - ``BACKCLINO``/``BACKGRADIENT``
   - ``COUNTER``/``COUNT``
   - ``DEPTH``
   - ``DECLINATION``
   - ``DX``/``EASTING``
   - ``DY``/``NORTHING``
   - ``DZ``/``ALTITUDE``
   - ``LEFT``
   - ``RIGHT``
   - ``UP``/``CEILING``
   - ``DOWN``/``FLOOR``

   ``<factor>`` allows you to easy specify situations such as
   measuring distance with a diving line knotted every 10cm
   (``*units distance 0.1 metres``).  If ``<factor>`` is omitted it
   defaults to ``1.0``.  If specified, it must be non-zero.

   Valid units for listed quantities are:

   ``TAPE``/``LENGTH``, ``BACKTAPE``/``BACKLENGTH``, ``COUNTER``/``COUNT``, ``DEPTH``,
   ``DX``/``EASTING``, ``DY``/``NORTHING``, ``DZ``/``ALTITUDE`` in
   ``YARDS``\|\ ``FEET``\|\ ``METRIC``\|\ ``METRES``\|\ ``METERS`` (default: ``METRES``)

   ``CLINO``/``GRADIENT``, ``BACKCLINO``/``BACKGRADIENT`` in
   ``DEGS``\|\ ``DEGREES``\|\ ``GRADS``\|\ ``MINUTES``\|\ ``PERCENT``\|\ ``PERCENTAGE`` (default:
   ``DEGREES``)

   ``COMPASS``/``BEARING``, ``BACKCOMPASS``/``BACKBEARING``, ``DECLINATION`` in
   ``DEGS``\|\ ``DEGREES``\|\ ``GRADS``\|\ ``MINUTES``\|\ ``QUADS``\|\ ``QUADRANTS`` (default:
   ``DEGREES``)

   (360 degrees = 400 grads)

   ``QUADRANTS`` are a style of bearing used predominantly in
   land survey, and occasionally in survey with handheld
   instruments.  All bearings are N or S, a numeric from 0 to
   90, followed by E or W.  For example S34E to refer to 146
   degrees, or 34 degrees in the SE quadrant. In this
   format, exact cardinal directions may be simply
   alphabetic. E.g. N is equivalent to N0E and E is
   equivalent to N90E. This unit was added in Survex 1.2.44.

   Survex has long supported ``MILS`` as an alias for ``GRADS``.
   However, this seems to be a bogus definition of a "mil"
   which is unique to Survex (except that Therion has since
   copied it) - there are several different definitions of a
   "mil" but they vary from 6000 to 6400 in a full circle,
   not 400. Because of this we deprecated ``MILS`` in Survex
   1.2.38 - you can still process data which uses them but
   you'll now get a warning, and we recommend you update
   your data.

   For example, if your data uses
   ::

       *units compass mils

   then you need to determine what the intended units
   actually are.  If there are 400 in a full circle, then
   instead use this (which will work with older Survex
   versions too):
   ::

       *units compass grads

   If the units are actually mils, you can specify that in
   terms of degrees.  For example, there are 6400 NATO mils
   in a full circle, so one NATO mil is 360/6400 degrees, and
   360/6400=0.05625 so you can use this (which also works
   with older Survex versions):
   ::

       ; Compass readings are NATO mils (6400 = 360 degrees)
       *units compass 0.05625 degrees

See Also
   ``*calibrate``
