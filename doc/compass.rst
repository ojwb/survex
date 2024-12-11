====================
Larry Fish's Compass
====================

Survex can read Compass survey data - it supports survey data files
and project files (``.DAT`` and ``.MAK files``), closed data files (``.CLP``),
and processed survey data (``.PLT`` and ``.PLF`` files).  Survex 1.4.6 made
significant improvements to this support so we recommend using this
version or newer if you're working with Compass data.

--------------------
Compass .MAK support
--------------------

A Compass ``.MAK`` file defines a survey project to process, and
specifies one or more ``.DAT`` files to process, along with coordinate
systems and fixed points.  You can process a ``.MAK`` file with cavern
or aven as if it were a ``.svx`` file.

Survex understands most MAK file features.  Known current
limitations and assumptions:

- Survex handles the UTM zone and datum provided the combination
  can be expressed as an EPSG code (lack of any EPSG codes for a
  datum suggests it's obsolete; lack of a code for a particular
  datum+zone combination suggests the zone is outside of the
  defined area of use of the datum). Example Compass files we've
  seen use "North American 1927" outside of where it's defined
  for use, presumably because some users fail to change the datum
  from Compass' default. To enable reading such files we return a
  PROJ4 string of the form "+proj=utm ..." for "North American
  1927" and "North American 1983" for UTM zones which don't have
  an EPSG code. Please let us know if support for additional
  cases which aren't currently supported would be useful to you.

- The ``@`` command which specifies a base location to calculate
  magnetic declinations at is handled, provided the datum and UTM
  zone are supported (see previous bullet point). The UTM
  convergence angle specified as part of this command is ignored
  as Survex knows how to calculate it.

- Link stations are ignored. These have two uses in Compass. They
  were a way to allow processing large surveys on computers from
  last century which had limited memory. Survex can easily handle
  even the largest surveys on current hardware and ignoring them
  is not a problem in this case.

  The other use is they provide a way to process surveys together
  which use the same station names for different stations. In
  this case we recommend writing a ``.svx`` file to replace the MAK
  file which wraps the ``*include`` of each DAT file in
  ``*begin survey1``/``*end survey1``, etc so that stations
  have unique qualified names. Then the link stations can be
  implemented using e.g. ``*equate survey1.XX1 survey2.XX1``.
  This example from the Compass documentation:
  ::

     #FILE1.DAT;                /no links
     #FILE2.DAT,A22,A16;
     #FILE3.DAT,A16,B14;

  would look like this:
  ::

     *begin file1
     *include FILE1.DAT
     *end file1
     *begin file2
     *include FILE2.DAT
     *end file2
     *equate file1.A22 file2.A22
     *begin file3
     *include FILE3.DAT
     *end file3
     *equate file1.A16 file3.A16
     *equate file2.B14 file3.B14

  Note that the ``.svx`` version is able to more precisely represent
  what's actually required here - in the MAK version "you must
  carry ``A16`` into ``FILE2`` even though ``FILE2`` doesn't need it for its
  own processing". If you want the exact analog of the MAK
  version you can change the ``A16`` equate to:
  ::

     *equate file1.A16 file2.A16 file3.A16

- The following commands (and any other unknown commands) are
  currently ignored: ``%`` (Convergence angle (file-level)), ``*``
  (Convergence angle (non file-level)), ``!`` (Project parameters)

--------------------
Compass .DAT support
--------------------

A Compass ``.DAT`` file contains unprocessed survey data.  You can process a
``.DAT`` file with cavern or aven as if it were a ``.svx`` file.

You can even use ``*include compassfile.dat`` or ``*include
compassproject.mak`` in a ``.svx`` file and it'll work, which allows combining
separate cave survey projects maintained in Survex and Compass.

One point to note when doing so (this tripped us up!) is that
station names in Compass are case sensitive and so Survex reads
``.DAT`` files with the equivalent of ``*case preserve``. The default
in ``.svx`` files is ``*case lower`` so this won't work
::

   *fix CE1 0 0 0
   *include datfilewhichusesCE1.dat

because ``CE1`` in the ``*fix`` is actually interpreted as ``ce1``.  The
solution is to turn on preserving of case while you fix the point
like so:
::

   *begin
   *case preserve
   *fix CE1 0 0 0
   *end
   *include datfilewhichusesCE1.dat

If you want to be able to refer to the fixed point from Survex data too then
you can add in a ``*equate`` to a lower-case name to achieve that:
::

   *begin
   *case preserve
   *fix CE1 0 0 0
   *equate CE1 ce1
   *end
   *include datfilewhichusesCE1.dat
   *include svxfilewhichusesce1.svx

Or if you're just wanting to link a Compass survey to a Survex one, you can use
a ``*equate`` with ``*case preserve on``:
::

   *begin
   *case preserve
   *equate CE1 ce1
   *end
   *include datfilewhichusesCE1.dat
   *include svxfilewhichusesce1.svx

Survex understands most DAT file features.  Known current limitations and
assumptions:

- The cave name, survey short name, survey comment and survey
  team information are currently ignored (because this
  information isn't currently saved in the ``.3d`` file even for ``.svx``
  files).
- Survey date January 1st 1901 is treated as "no date specified",
  since this is the date Compass stores in this situation, and it
  seems very unlikely to occur in real data.
- Passage dimensions are currently ignored.
- Shot flag ``C`` is currently ignored.
- Shot flag ``L`` is mapped to Survex's "duplicate" leg flag.
- Shot flag ``P`` is mapped to Survex's "surface" leg flag. The
  Compass documentation describes shot flag ``P`` as "Exclude this
  shot from plotting", but the suggested use for it is for
  surface data, and shots flagged ``P`` "[do] not support passage
  modeling". Even if it's actually being used for a different
  purpose, Survex programs don't show surface legs by default so
  the end effect is at least to not plot as intended.
- Shot flag ``S`` is mapped to Survex's "splay" leg flag.
- Surveys which indicate a depth gauge was used for azimuth
  readings are marked as ``STYLE_DIVING`` in the ``.3d`` file.
- Compass seems to quietly ignore a shot with the same "from" and "to"
  station. This seems likely to be a mistake in the data so Survex 1.4.12
  and later warn about this in a Compass DAT file (in native Survex data
  this is treated as an error, which is how older Survex versions treat
  it in Compass DAT files).

--------------------
Compass .CLP support
--------------------

A Compass .CLP file contains raw survey data after adjusting for
loop closure. The actual format is otherwise identical to a
Compass ``.DAT`` file, and Survex 1.4.6 and later support processing a
.CLP file with cavern or aven as if it were a ``.svx`` file (the extra
support is to recognise the ``.CLP`` extension, and to not apply the
instrument corrections a second time).

You can even use ``*include compassfile.clp`` in a ``.svx`` file
and it'll work, which allows combining separate cave survey
projects maintained in Survex and Compass.

Usually it is preferable to process the survey data without loop
closure adjustments (i.e. ``.DAT``) so that when new data is added
errors get distributed appropriately across old and new data, but
it might be useful to use the ``.CLP`` file if you want to keep
existing stations at the same adjusted positions, for example to
be able to draw extensions on an existing drawn-up survey which
was processed with Compass.

Another possible reason to use the data from a ``.CLP`` file is if
that's all you have because the original ``.DAT`` file has been lost!

-------------------------
Compass .PLF/.PLT support
-------------------------

A Compass ``.PLT`` file contains processed survey data.  The extension
``.PLF`` is also used for "special feature files" which have
essentially the same format.

Survex supports both reading and writing these files, each of which
are documented in separate sections below.

Reading Compass .PLF/.PLT
=========================

You can load these files with ``aven`` as if they were .3d files, and
similarly for other Survex tools which expect a .3d file such as
``survexport``, ``extend``, ``diffpos``, ``3dtopos`` and ``dump3d``.
(This support is actually provided by Survex's img library, so other
programs which use this library should also be able to read Compass
``.PLT`` files without much extra work.)

Survex understands most PLT file features.  Known current
limitations and assumptions:

- Survey date January 1st 1901 is treated as "no date specified",
  since this is the date Compass stores in this situation, and it
  seems very unlikely to occur in real data.

- Passage dimensions are translated to passage tubes, but Survex
  may interpret them differently from Compass.

- Shot flag ``C`` is currently ignored.

- Shot flag ``L`` is mapped to Survex's "duplicate" leg flag.

- Shot flag ``P`` and plot command ``d`` are mapped to Survex's "surface"
  leg flag. The Compass documentation describes shot flag ``P`` as
  "Exclude this shot from plotting", but the suggested use for it
  is for surface data, and shots flagged ``P`` "[do] not support
  passage modeling". Even if it's actually being used for a
  different purpose, Survex programs don't show surface legs by
  default so the end effect is at least to not plot as intended.
  Stations are flagged as surface and/or underground based on
  whether they are at the ends of legs flagged surface or
  non-surface (a station at the boundary can be flagged as both).

- Shot flag ``S`` is mapped to Survex's "splay" leg flag. A station
  at the far end of a shot flagged ``S`` gets the "station on wall"
  flag set since the Compass PLT format specification says: "The
  shot is a "splay" shot, which is a shot from a station to the
  wall to define the passage shape."

- Stations with "distance from entrance" of zero are flagged as
  entrances.

- Stations which are present in multiple surveys are flagged as
  exported (like when ``*infer exports on`` is in effect in ``.svx``
  files).

- Stations listed as fixed points are flagged as fixed points.

- If a PLT file only uses one datum and UTM zone combination and
  it is supported (the same combinations are supported as for MAK
  files) then they are converted to the appropriate EPSG code or
  PROJ4 string and this is reported as the coordinate system.
  Please let us know if support for additional cases which aren't
  currently supported would be useful to you. Files with multiple
  datums could be handled too, but we'd need to convert
  coordinates to a common coordinate system in the img library,
  which would need it to depend on PROJ. Please let us know if
  support for mixed datums would be useful to you.

Exporting Compass .PLT
======================

Survex can also create PLT files via ``aven``'s File->Export feature, and also
from the command line via ``survexport --plt``.

This export was originally added to allow importing data from Survex into
Carto.  The principal author of Carto has sadly died and it seems Carto is no
longer actively developed, but we've left this support in place in case it is
useful - the generated files can be used with Compass itself for example,
though they are currently rather crudely structured.  Here are some notes on
this support:

- The whole Survex survey tree is exported as a single survey.

- Compass station names can't contain spaces, so any spaces (and also ASCII
  control characters) are in station names are replaced by ``%`` follow by two
  lowercase hex digits giving the byte value (like the escaping used in URLs).
  ``%`` itself is also escaped as ``%25``.

- The full Survex station name include survey prefixes is used - no attempt is
  currently made to shorten station names to fit within the 12 character limit
  documented for the Compass PLT format.  If you export a single survey the
  names should be short enough, but exporting the whole of a complex survey
  project will likely give names longer than 12 characters.

- Anonymous stations are given a name ``%:`` followed by a number starting from
  one and incrementing for each anonymous station (Compass doesn't allow empty
  station names, and these invented names can't collide with actual station
  names).  Since Survex 1.4.10 (1.4.6 implemented support for exporting
  anonymous stations to PLT, but with names which typically exceeded the
  documented 12 character limit of the format).

- Passage data is not included in the export (each exported leg has dummy LRUD
  readings of all ``-9`` which is needed to avoid a bug in some versions of
  Compass which don't cope with legs without LRUD).

- Survex's "surface" leg flag is mapped to Compass shot flag ``P``.
  The Compass documentation describes shot flag ``P`` as "Exclude this shot
  from plotting", but the suggested use for it is for surface data, and shots
  flagged ``P`` "[do] not support passage modeling".  Since Survex 1.4.10.

- Survex's "splay" leg flag is mapped to Compass shot flag ``S``.  Since Survex
  1.4.10.

- Survex's "duplicate" leg flag is mapped to Compass shot flag ``L``.  Since
  Survex 1.4.10.

- The Datum and UTM zone information is not currently set in exported PLT
  files.
