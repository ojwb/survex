======================
David McKenzie's Walls
======================

Survex 1.4.9 and later can read Walls unprocessed survey data (``.SRV``
and ``.WPJ`` files).  The support is currently somewhat experimental, as
the documentation of the SRV format is incomplete and incorrect in
places, while the WPJ format seems to be largely undocumented, and
David is sadly no longer around to ask.

The current status is that most ``.SRV`` files should process
individually (but see details below of features which are not
handled), while there are still some known issues which cause
problems with some ``.WPJ`` files.

Walls is no longer being developed, so the focus of support for Walls
formats is primarily to help people with existing Walls data to
migrate.  If you are in this situation and the incomplete WPJ support
is a problem, you should be able to write a ``.svx`` file to replace your
WPJ file - you can use ``*include somedata.srv`` to include a Walls
``.srv`` from a ``.svx`` file.

- Walls allows hanging surveys, whereas these are currently treated
  as an error by Survex.  This is probably the current top priority
  to address.

- Survex reports warnings in some suspect situations which Walls
  quietly accepts.  In general this seems helpful, but if there are
  particular instances which are noisy and not useful, let us know.

- ``#FIX`` - currently Survex does not support horizontal-only or
  vertical only fixes.  These are currently given an SD of 1000m in
  that direction instead, but that is not the same as completely
  decoupling the fix in that direction.

- ``#FIX`` - degree:minute:second fixes (e.g. ``W97:43:52.5``) are not
  currently supported.

- Walls ``FLAG`` values seems to be arbitrary text strings.  We try to
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
  (unless the argument is zero), and is skipped.

- The ``RECT=`` option currently gives an "Unknown command" warning, and
  is skipped (this option specifies how much to orient cartesian
  style data relative to true North, not to be confused with the
  unrelated ``RECT`` option without a value which is supported).

- Walls documents allowing a "maximum of eight characters" in
  unprefixed names - we don't bother trying to enforce this
  restriction, but this should not make a difference in valid data.

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
  long, but Survex allows longer names in Walls data.

- Walls documents `The total length of the three prefix components combined,
  including any embedded colon separators, is 127 characters` but Survex does
  not enforce any limit.

- In the option ``UNITS=`` the documentation says `CASE = Upper / Lower /
  Mixed` but it seems actually any string is allowed and if it starts
  with a letter other than ``U`` or ``L`` then it's treated as ``Mixed``.
  Since Survex 1.4.10.

- Walls explicitly documents that `Unprefixed names can have a maximum of eight
  characters and must not contain any colons, semicolons, commas, pound signs
  (#), or embedded tabs or spaces.` but it actually allows ``#`` in station
  names (though it can't be used as the first character of the from station
  name as that will be interpreted as a command.  Since Survex 1.4.10.

- Walls ignores junk after the numeric argument in ``TYPEAB=``, ``TYPEVB=``,
  ``UV=``, ``UVH=``, and ``UVV=``.  Survex warns and skips the junk.  Since
  Survex 1.4.10.

- Walls allows the clino reading to be completely omitted with ``ORDER=DAV``
  and ``ORDER=ADV`` (this seems to be undocumented).  Since Survex 1.4.10.

- If a station is used with an explicit Walls prefix (e.g. ``PEP:A123``)
  then it will will be flagged as "exported" in the ``.3d`` file.  This
  is currently applied even if the explicit prefix is empty (e.g. ``:A123``).
  Since Survex 1.4.10.

If you find some Walls data which Survex doesn't handle or handles
incorrectly, and it is not already noted above, please let us know.
If you can provide some data demonstrating the problem, that's really
helpful.  It's also useful to know if there are things listed above
that are problematic to help prioritise efforts.
