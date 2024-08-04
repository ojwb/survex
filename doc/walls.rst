======================
David McKenzie's Walls
======================

Survex 1.4.9 and later can read Walls unprocessed survey data (``.SRV``
and ``.WPJ`` files).  Walls is no longer being developed, so the focus of
support for Walls formats is primarily to help people with existing Walls data
to migrate.

We've mostly implemented this support based on Walls documentation, but
unfortunately the documentation of the SRV format sometimes incomplete or
incorrect, while the WPJ format seems to be largely undocumented.  Sadly
David is no longer around to ask, but we can at least test actual behaviour
of ``Walls32.exe`` on example data.

As of 1.4.10, some large Walls datasets can be successfully processed
(e.g. Mammoth Cave, the Thailand dataset from https://cave-registry.org.uk/,
and Big Bat Cave).  Behaviour is not identical and station positions after
loop closure will inevitably be different, but large or apparently systematic
errors are worth reporting.  An easy way to compare is to export a Shapefile
from ``Walls32.exe`` and overlay it in ``aven``.  The way to export is a bit
hidden - after processing select the `Segments` tab, make sure the whole
project is selected, and click the `Details / Rpts...` button which is towards
the upper right.  Click the `Shapefile...` button in the new dialog box, and
select what you want to output (e.g. `Vectors`).  Due to limitations in the
Shapefile format each `Shape Type` selected here exports a separate Shapefile.
To overlay a Shapefile in ``aven`` using `File->Overlay Geodata...`.

See below for a list of known limitations.  We've mostly prioritised what
to implemented based on testing with real-world datasets so commonly used
features are likely to be handled while more obscure features may not be.

- Survex reports warnings in some suspect situations which Walls
  quietly accepts.  In general this seems helpful and they do highlight
  what look like genuine problems in existing datasets, but if there are
  particular instances which are noisy and not useful, let us know.

  Some of these warnings use the same wording as errors - most of these
  probably ought to be an error except Walls quietly accepts them so we
  don't want to fail processing because of them.

  If you want a way to suppress the "unused fixed point" warning, using the
  station in a ``#NOTE`` or ``#FLAG`` command counts as a "use" so you
  can suppress these with e.g. ``#NOTE ABC123 /unused`` for each such
  fixed point.

- If an isolated LRUD (LRUD not on a leg, e.g. specified for the start or end
  station of a traverse) is missing its closing delimiter, Walls will parse
  the line as a data leg where the "to" station starts with ``*`` or ``<``,
  which will usually succeed without errors or warnings.  A real-world example
  is::

    P25     *8 5 16 3.58

  Survex parses this like Walls does, but issues a warning::

    walls.srv:1:9: warning: Parsing as "to" station but may be isolated LRUD with missing closing delimiter
     P25     *8 5 15 3.58
             ^~

  This warning will also be triggered if you have a "to" station which actually
  starts with ``*`` or ``<``, if the station name was not previously seen.
  This condition provides a simple way to suppress the warning - just add a
  dummy ``#NOTE`` command before the line of data like so::

    #note *8 ; Suppress Survex warning that this looks like broken LRUD
    P25     *8 5 16 3.58

- Walls allows hanging surveys, apparently without any complaint, and
  as a result large Walls datasets are likely to have hanging surveys.
  A hanging survey used to be an error in Survex but since 1.4.10
  a hanging survey is warned about and then ignored.

- ``#FIX`` - currently Survex does not support horizontal-only or
  vertical only fixes.  These are currently given an SD of 1000m in
  that direction instead, but that is not the same as completely
  decoupling the fix in that direction.

- Variance overrides on survey legs are mostly supported, with the following
  limitations:

  + An SD of 0 is currently treated as 1mm;
  + Floating a leg both horizontally and vertically (with ``?``) replaces it
    with a "nosurvey" leg, which is effectively the same provided both ends
    of the leg are attached to fixed points.
  + Floating a leg either horizontally or vertically (with ``?``) uses an SD of
    1000m in that direction instead of actually decoupling the connection;
  + Floating the traverse containing a leg (with ``*``) currently just floats
    that leg (so it's the same as ``?``).

- Walls ``#SEGMENT`` is apparently in practice commonly set to a set of Compass
  "shot flags", so if a ``#SEGMENT`` value consists only of letters from the
  set ``CLPSX`` with an optional leading ``/`` or ``\\`` then we map it to
  Survex flags like so (since Survex 1.4.10):

  + ``C`` is ignored
  + ``L`` is mapped to Survex's "duplicate" flag
  + ``P`` is mapped to Survex's "surface" flag
  + ``S`` is mapped to Survex's "splay" flag
  + ``X`` in Compass completely excludes the leg from processing, but it can't
    in Walls and it seems in practice it's sometimes used in Walls to flag
    data to just exclude from the surveyed length, so we treat it as an alias
    for ``L`` and map it to Survex's "duplicate" flag.

  Other values of ``#SEGMENT`` are ignored.

- Walls ``FLAG`` values seem to be arbitrary text strings.  We try to
  infer appropriate Survex station flags by checking for certain key
  words in that text (currently we map words ``ENTRANCE`` and ``FIX``
  to the corresponding Survex station flags) and otherwise ignore ``FLAG``
  values.

- ``#NOTE`` is parsed and the station name is marked as "used" (which
  suppresses the unused fixed point warning) but the note text is
  currently ignored.

- We don't currently support all the datum names which Walls does
  because we haven't managed to find an EPSG code for any UTM zones
  in some of these datums.  This probably means they're not actually
  in current use.

- We currently assume all two digit years are 19xx (Walls documents
  it 'also accepts "some date formats common in the U.S. (``mm/dd/yy``,
  ``mm-dd-yyyy``, etc.)' but doesn't say how it interprets ``yy``.

- The documentation specifies that the ``SAVE`` and ``RESTORE`` options
  should be processed before other options.  Currently Survex just
  processes all options in the order specified, which makes no
  difference to any real-world data we've seen.  We need to test with
  Walls32.exe to determine exactly how this works (and if ``RESET`` is
  also special).

- LRUD data is currently ignored.

- The ``TAPE=`` option is currently quietly skipped, and tape
  measurements are assumed to be station to station.

- In ``TYPEAB=`` and ``TYPEVB=``, the threshold is ignored, as is the ``X``
  meaning to only use foresights (but still check backsights).
  Survex uses a threshold based on the specified instrument SDs, and
  averages foresights and backsights.

- ``UV=``, ``UVH=`` and ``UVV=`` are all quietly skipped.

- The ``GRID=`` option currently gives an "Unknown command" warning, and
  is skipped.  If your Walls data specifies a UTM zone then Survex
  will automatically correct for grid convergence.

- The ``INCH=`` option currently gives an "Unknown command" warning
  (unless the argument is zero, since Survex 1.4.10), and is skipped.

- Walls seems to allow ``\\`` in place of ``/`` in some places (e.g.
  ``#FLAG``).  We aim to support this too, but it doesn't seem to be documented
  so may not currently be supported in the correct places.

- The inheritance of settings in WPJ files is probably not correctly
  implemented currently.

- The Walls documentation mentions a ``NOTE=`` option, but doesn't
  document what it does, and testing with Walls32.exe it doesn't
  seem to actually be supported!

- The two UPS zones for the polar regions (specified as UTM zone
  values of -61 and 61 in Walls) are supported with datum WGS84, but
  we do not have any real data to test this support with.

- Walls gives an error if an unprefixed station name is more than 8 characters
  long but Survex does not enforce this restriction.

- Walls documents `The total length of the three prefix components combined,
  including any embedded colon separators, is 127 characters` but Survex does
  not enforce any limit.

- In the option ``UNITS=`` the documentation says `CASE = Upper / Lower /
  Mixed` but it seems actually any string is allowed and if it starts
  with a letter other than ``U`` or ``L`` then it's treated as ``Mixed``.
  Since Survex 1.4.10.

- Walls explicitly documents that `Unprefixed names [...] must not contain any
  colons, semicolons, commas, pound signs (#), or embedded tabs or spaces.` but
  it actually allows ``#`` in station names (though it can't be used as the
  first character of the from station name as that will be interpreted as a
  command.  Since Survex 1.4.10.

- Walls ignores junk after the numeric argument in ``TYPEAB=``, ``TYPEVB=``,
  ``UV=``, ``UVH=``, and ``UVV=``.  Survex warns and skips the junk.  Since
  Survex 1.4.10.

- Walls allows the clino reading to be completely omitted with ``ORDER=DAV``
  and ``ORDER=ADV`` on a "wall shot" (leg to or from an anonymous station).
  Supported since Survex 1.4.10.

- If a station is used with an explicit Walls prefix (e.g. ``PEP:A123``)
  then it will will be flagged as "exported" in the ``.3d`` file.  This
  is currently applied even if the explicit prefix is empty (e.g. ``:A123``).
  Since Survex 1.4.10.

- Walls allows a station with an explicit prefix to have an empty name,
  e.g. ``PEP:``.  The Walls documentation doesn't mention this, though it
  also doesn't explicitly say the name can't be empty.  This quirk seems
  unlikely to be intentionally used and Survex doesn't allow an empty station
  name, so we issue a warning and use the name ``empty name`` (which has a
  space in, so can't collide with a real Walls station name which can't contain
  a space) - so ``PEP:`` in Walls becomes ``PEP.empty name`` in Survex.
  Since Survex 1.4.10.

If you find some Walls data which Survex doesn't handle or handles
incorrectly, and it is not already noted above, please let us know.
If you can provide some data demonstrating the problem, that's really
helpful.  It's also useful to know if there are things listed above
that are problematic to help prioritise efforts.
