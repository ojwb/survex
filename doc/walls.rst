======================
David McKenzie's Walls
======================

Survex can read Walls unprocessed survey data files and project files (``.SRV``
and ``.WPJ`` files).  The first release with any support was Survex 1.4.9, but
the support has evolved and improved since then.  Survex 1.4.24 improved
support significantly so we strongly advise using 1.4.24 or newer, and this
chapter will assume you are.  If you really want to know when support for
something was added, see the NEWS file.

Survex 1.4.23 added support for reading Walls ``.LST`` reports as processed
survey data.  ``.LST`` is intended as a text report format, but contains enough
information to usefully read as processed survey data, and doing so means
tools such as ``diffpos`` can be used to compare the same dataset processed
by Walls and by Survex.  See the section below for details of ``.LST`` support.

David's final release was Walls "Version 2, Build 2016-11-18" in 2016.  We
do still keep an eye on compatibility with this release, but the focus is
primarily on `recent releases made by Andy Edwards
<https://github.com/wallscavesurvey/walls/releases/>`_, which are a much
better option for anyone using Walls (whether or not they are interested
in Survex).  These newer releases are particular advised if you are processing
survey data from 2010 or later as David's final release uses IGRF 12, which was
released in late 2015, is only intended to support dates up to the end of 2019,
and is only definitive up to the end of 2009 - for dates in 2010-2019 it is
predictive based on observations before this period.

---------------------------
Walls .WPJ and .SRV support
---------------------------

Survex 1.4.9 and later can read Walls unprocessed survey data (``.SRV``
and ``.WPJ`` files).  You can process a ``.WPJ`` (which is a Walls Project
file) or an individual ``.SRV`` (which contains unprocessed survey data)
with cavern or aven as if it were a ``.svx`` file.

You can even use ``*include wallsdat.srv`` or ``*include
wallsproject.wpj`` in a ``.svx`` file and it'll work, which allows combining
separate cave survey projects maintained in Survex and Walls.  (There's
similar support for Compass ``.DAT`` and ``.MAK``.)

When we started to add support, Walls was no longer being developed, so the
initial focus was to help people with existing Walls data to migrate.  Walls
development has since restarted, so ongoing interoperation is now also an aim.

If you are wanting to migrate from Walls to using Survex instead, the ability
to process mixed format datasets means you don't need to mass-convert all
your existing survey data, but can keep it in ``.SRV`` format.  Walls ``.WPJ``
format is textual but not amenable to human modification, so in this scenario
we would suggest you replace your ``.WPJ`` with a simple top-level ``.svx``
file which includes each of your ``.SRV`` files using e.g. ``*include
file1.srv``.  If you are setting options in your ``.WPJ`` you'll need to
replace those with ``#units`` at the start of each ``.SRV``.  Then going
forwards, you can either add new data as ``.SRV`` or ``.svx``, depending on
your preference.

We've mostly implemented this support based on Walls documentation, but
unfortunately the documentation of the SRV format is often incomplete and
sometimes incorrect, while the WPJ format seems to be largely undocumented.
Sadly David is no longer around to ask, but we can at least test actual
behaviour of ``Walls32.exe`` on example data.

As of 1.4.24, some large Walls datasets can be successfully processed:

- Mammoth Cave
- the Thailand dataset from https://www.cave-registry.org.uk/svn/Thailand/
- the Tatra Cave Registry from https://github.com/dlubom/Jaskiniowy-Kataster-Tatr-Zachodnich
- the Slovenian Cave Survey from https://github.com/anzeabram/SiCS
- Big Bat Cave

.. FIXME: Link Thailand to https://cave-registry.org.uk/ once that's updated.

We encourage processing your Walls dataset with both Walls and Survex and
comparing the result.  If there are loop closures, station positions will
inevitably not be identical as Walls and Survex model errors differently.
Walls documents that the SDs are purely proportional to the leg length, and
appears to solve in X, Y and Z independently.  Survex calculates a variance
matrix for each leg based on its geometry and the instrument SDs (and this
matrix includes covariances, so doesn't assume X, Y and Z are independent).
The differences should small except in cases where there are large misclosures
due to blunders.  Please report significant or apparently systematic errors, or
anything that seems suspect.  If you can provide some data demonstrating the
problem, that's really helpful.

We suggest a couple of ways to compare the results:

- You can export a ``.LST`` report with the results of processing from Walls,
  and then compare it to the ``.3d`` file from processing the same dataset
  using::

    diffpos SURVEY.LST SURVEY.3d

  This will report stations which are only present in one file, or whose
  position differs by more than a specifiable threshold along any axis.
  The default threshold is 0.01m (about 0.4 inches).  For example, to
  report stations whose positions are different by more than 0.5m::

    diffpos SURVEY.LST SURVEY.3d 0.5

  Differences are reported as (East,North,Up) vectors in metres from the
  first file to the second.

  ``diffpos`` doesn't compare connectivity, but that's unlikely to differ
  between Survex and Walls output.

  The way to export ``.LST`` from Walls is a bit hidden - after processing
  select the `Segments` tab, make sure the whole project is selected, and click
  the `Details / Rpts...` button which is towards the upper right.  Click the
  `Coordinates...` button in the new dialog box.  If you want to include legs
  select `Connected vectors with file references`.

- A more visual way to compare is to export a Shapefile from ``Walls32.exe``
  and overlay it in ``aven``.

  The way to export from Walls is a bit hidden - after processing select the
  `Segments` tab, make sure the whole project is selected, and click the
  `Details / Rpts...` button which is towards the upper right.  Click the
  `Shapefile...` button in the new dialog box, and select what you want to
  output (e.g. `Vectors`).  Due to limitations in the Shapefile format each
  `Shape Type` selected here exports a separate Shapefile.

  To overlay a Shapefile in ``aven`` use `File->Overlay Geodata...`.

Things you should know
======================

We've mostly prioritised what to implement based on testing with real-world
datasets.  Features which are commonly used are very likely to be handled
while more obscure features may not be.  This section details differences,
limitations, and other things which we think anyone using this support should
be aware of.

Please let us know if there are things listed here (or in the next section)
which are problematic for your use to help prioritise efforts.

- Survex reports warnings in some suspect situations which Walls quietly
  accepts.  This is intended to be helpful, and we've had positive feedback
  from users that they have highlight genuine hidden problems in existing
  datasets.  Some of these warnings would ideally be errors, but we've made
  them warnings on the principle that we shouldn't fail to process a dataset
  which Walls successfully processes.  Hopefully over time more error checks
  will get added to Walls and we can upgrade our warnings to errors.

  A guiding principle is that warnings need to be actionable, and in particular
  it should be possible to keep a dataset free from warnings (if users expect
  there to be warnings, it's all too easy to miss when a change introduces a
  new warning, but it's often easiest to address a new warning right away while
  everything is fresh in your mind).  The fix is obvious in some cases, but
  see the section discussing specific warnings below if you encounter a warning
  and are unsure what to do about it.

- The ``LRUD=`` option is parsed and checked for validity, but LRUD data is
  currently ignored.  Work is in progress on this.

- Not all combinations of ``TAPE=``, ``ORDER=`` and use of instrument and/or
  target heights are supported.  You will get a warning if you use an
  unsupported combination.

  + ``TAPE=IT`` (Walls default) is fully supported.   ``TAPE=IT`` with
    ``ORDER=DA`` or ``ORDER=AD`` is marked as style "cylpolar" in the
    ``.3d`` file (which is what Survex calls a "flat-tape" style survey).

  + ``TAPE=SS`` is supported except for legs which have **both** a clino
    (and/or backclino) reading **and** instrument and/or target heights.
    The Walls manual says to enter underwater data using ``TAPE=SS`` and
    ``ORDER=DA``/``ORDER=AD``, so any ``TAPE=SS`` legs without a clino are
    marked as style "diving" in the ``.3d`` file.

  + ``TAPE=IS`` and ``TAPE=ST`` are not currently supported.

  Uses of unsupported combinations are warned about, and handled by ignoring
  the heights, so the instrument and target are effectively assumed to be on
  their respective stations.

  None of the unsupported combinations seem to be used in practice (apart from
  some uses of ``TAPE=SS`` with both clino and heights in the Mammoth dataset,
  which turned out to all be erroneous on further investigation).

- Walls only runs on Microsoft Windows, where filenames are case-insensitive
  and you may find the case of filenames in the ``.PRJ`` file doesn't match
  the actual case of the filename on disk.  If processing a Walls dataset on a
  platform such as Linux where filenames are case-sensitive this can cause
  problems.

  This same issue can affect people using different platforms and collaborating
  on native Survex datasets too, and the same workaround is used when opening
  ``.SRV`` files: if the file isn't found ``cavern`` will try a few variations
  of the case for the leafname.  First it will try all lower case, then all
  lower case except with the first character of the leafname in upper case, and
  finally all upper case.

  Walls always seems to write ``.PRJ`` files with the names of ``.SRV`` files
  in upper case, so if the filename on disk has mixed case (aside from the
  initial character) then this workaround won't help.

  A useful trick is to put the data in a ZIP archive and then unzip it on
  Linux using ``unzip -L`` which will unpack all the filenames in lower case.
  Then ``cavern`` should successfully process it thanks to the "try lowercase"
  part of the workaround.

- ``#FIX`` - at present Survex does not support horizontal-only or
  vertical only fixes.  These are currently given an SD of 1000m in
  that direction instead, but that is not the same as completely
  decoupling the fix in that direction.

- Variance overrides on survey legs are mostly supported, with the following
  limitations:

  + An SD of 0 is currently treated as 1mm (approximately 0.04 inches).
  + Floating a leg horizontally, vertically, or both (with ``?``) uses an SD of
    1000m in the floated direction(s) instead of using an actually infinite
    SD.
  + Floating the traverse containing a leg (with ``*``) currently just floats
    that leg (so it's the same as ``?``).

- ``UV=``, ``UVH=`` and ``UVV=`` are supported with the following limitations:

  + A factor of ``0`` is currently replaced by ``0.000001``.
  + A factor of more than ``9999`` is replaced by ``9999``.

- Walls supports up to 3 prefix levels, which we map into Survex's hierarchical
  prefixes.  One difference is that Walls allows an empty prefix but Survex
  does not.  Leading empty prefix levels are handled by just omitting them
  (as Walls does when showing prefixed station names) so this mismatch can only
  affect projects which use more than one prefix level - in practice only very
  large projects seem to do do.

  To handle this case, Survex replaces such empty prefix levels with a single
  space - a Walls prefix can't contain a space, so this can't collide.  For
  example, ``PEP::1`` in Walls becomes ``PEP: :1`` in Survex; it can even
  happen twice in the same name - e.g. ``X:::1`` becomes ``X: : :1``.

  If you are processing output from Survex then simply stripping any spaces
  from station names should undo this and make them match Walls (unless you
  have stations with empty names - in that case you'll need to first check for
  ``:empty name`` at the end of the name and replace with ``:`` if present,
  and then strip any remaining spaces, so e.g. ``X: : :empty name`` becomes
  ``X:::``).

- Walls ``#SEGMENT`` directives often contain a set of Compass "shot flags".
  The likely origin of this is that Walls creates such ``#SEGMENT`` directives
  when you import data from Compass.  If a ``#SEGMENT`` value consists only of
  letters from the set ``CLPSX`` with an optional leading ``/`` or ``\\`` then
  we map it to Survex leg flags like so:

  + ``C`` in Compass causes legs it is set on to not be affected by loop
    closure, but setting Compass flags in the Walls ``#SEGMENT`` is just a
    user-level convention so Walls won't do anything in response to ``C``, so
    Survex ignores it too.
  + ``L`` is mapped to Survex's "duplicate" flag
  + ``P`` is mapped to Survex's "surface" flag
  + ``S`` is mapped to Survex's "splay" flag
  + ``X`` in Compass completely excludes the leg from processing, but setting
    Compass flags in the Walls ``#SEGMENT`` is just a user-level convention
    so Walls won't do anything in response to ``X``.  It seems in practice that
    ``X`` is sometimes used in Walls datasets to flag data to just exclude from
    the surveyed length, so we treat it as an alias for ``L`` and map it to
    Survex's "duplicate" flag.

  Other values of ``#SEGMENT`` are ignored, as is ``#SEGMENT`` on a ``#FIX``
  directive line (since mapped Compass flags apply to legs and not to a fixed
  point).

- Walls ``FLAG`` values are arbitrary text strings.  We try to infer
  appropriate Survex station flags by checking for certain key words in that
  text and otherwise ignore ``FLAG`` values.

  Currently we map the word ``ENTRANCE`` (in any case) to the corresponding
  Survex station flag.

- If a station is used with an explicit Walls prefix (e.g. ``PEP:A123`` or
  ``:ZZ9ZA``) then it will will be flagged as "exported" in the ``.3d`` file,
  which can then be used by programs reading that file.  For example, ``aven``
  has an option to show blue blobs on stations flagged "exported" (and they
  will get their name shown in preference when station names overlap and we
  pick which to show).

- The inheritance of settings in WPJ files is not documented and so has been
  reversed engineered.  Please report if you find any differences.

Known differences and limitations which are unlikely to matter in practice
==========================================================================

- ``TYPEAB=`` and ``TYPEVB=`` are implemented by mapping the backsight
  tolerances to compass and clino SDs such that only mismatches above the
  tolerances are warned about.  The default compass and clino SDs are set to
  match the default Walls tolerance of 5°.  Survex takes these SDs into
  account when closing loops, so a reading with a higher backsight tolerance
  will be trusted less, which seems appropriate.

  Less helpful is that this means that the default compass and clino SDs for
  Walls data are higher than the defaults for native data, so if you close a
  loop containing both data from Walls and data from ``.svx`` (or Compass
  ``.dat``), the Walls compass and clino readings will be trusted less.

  The ``X`` flag (which means only use foresights but still check backsights)
  is currently ignored (with a warning) so foresights and backsights are always
  averaged.  We are not aware of any uses of ``X`` in real-world data.

- Walls uses ``:`` as a separator between prefix levels and between any prefix
  and the station name.  Survex takes into account which
  characters are allowed in Walls station names when picking the separator to
  use in the ``.3d`` file - for a pure Walls dataset, it will pick ``:``, so
  Walls station names shown by ``aven`` and other tools should match the names
  as shown in Walls.  If you have a dataset which mixes data in Walls format
  with Survex and/or Compass data, then it's possible ``:`` is used in a Survex
  or Compass station name - if so a different separator will be chosen.

- ``#SYMBOL`` directives are quietly ignored.  In Walls they specify a symbol
  shape and colour to use for a named flag, which doesn't usefully map into
  Survex where we avoid mixing presentation information with the raw data.

  The Walls manual notes *Since it's now easy to accomplished [sic] this
  interactively via the Flag and Marker Symbols dialog, you may choose not to
  use #Symbol directives at all*.  They aren't used in any of the Walls
  datasets we've seen.

- We don't currently support all the datum names which Walls does because we
  haven't managed to find an EPSG code for any UTM zones in some of these
  datums.  This probably means they're not actually in current use.

- ``#NOTE`` is parsed and the station name is marked as "used" (which
  suppresses the unused fixed point warning) but the note text is
  currently ignored (as in native ``.svx`` data, since the ``.3d``
  format doesn't currently contain station notes).

- The Walls documentation specifies that the ``SAVE`` and ``RESTORE`` options
  should be processed before other options, and experimentation shows that
  this applies to ``RESET`` as well.  Andy Edwards reports some other options
  are processed out-of-order (which is undocumented).  Currently Survex just
  processes all options in the order specified, which makes no difference to
  any real-world data we've seen.

- The ``GRID=`` option currently gives an "Ignoring unsupported Walls option"
  warning, and is skipped.  If your Walls data specifies a UTM zone then Survex
  will automatically correct for grid convergence.

- Walls seems to allow ``\\`` in place of ``/`` in some places (e.g.
  ``#FLAG``).  We aim to support this too, but it doesn't seem to be documented
  so may not currently be supported in the correct places.

- Walls prefixes are mapped to Survex prefixes, with a corner case we don't
  currently attempt to address.  If you have stations with full names ``:::X1``
  and ``::X1:A1``, then omitting leading empty prefixes (as Walls normally does
  and which Survex follows) these becomes ``X1`` and ``X1:A1``.  This means
  that ``X1`` names both a station and a survey in Survex, which is not
  allowed (Survex naming works like your computer's filing system in this
  regard - you can't have a folder and a file with the same full name).

  Survex tools won't explicitly error if they see such data, but for example
  aven's survey tree view can't correctly show a name as both.

  We haven't yet found a good solution to this.  It's probably very rare in
  practice, so renaming needs to be restricted to cases where there's actually
  a collision.

- Walls allows a station with an explicit prefix to have an empty name,
  e.g. ``PEP:``.  The Walls documentation doesn't mention this, though it
  also doesn't explicitly say the name can't be empty.  This quirk seems
  unlikely to be intentionally used and Survex doesn't allow an empty station
  name, so we issue a warning and use the name ``empty name``, so ``PEP:`` in
  Walls becomes ``PEP:empty name`` in Survex.  (Since ``empty name`` contains
  a space it can't collide with a real Walls station name which can't contain
  a space.)

- The two UPS zones for the polar regions (specified as UTM zone
  values of -61 and 61 in Walls) are supported with datum WGS84, but
  we do not have any real data to test this support with.

- Walls allows quoting of tokens in ``#UNITS``, for example

  ::

    #units "typevb" = "n"

  Survex handles this, but this feature does not seem to be documented so there
  may well be cases which are not handled the same.  Please report any you find.

- The Walls documentation mentions a ``NOTE=`` option, but doesn't
  document what it does, and testing with Walls32.exe it doesn't
  seem to actually be supported!

- Walls explicitly documents that `Unprefixed names [...] must not contain any
  colons, semicolons, commas, pound signs (#), or embedded tabs or spaces.` but
  it actually allows ``#`` in station names (though it can't be used as the
  first character of the from station name as that will be interpreted as a
  directive.

- Walls gives an error if an unprefixed station name is more than 8 characters
  long but Survex does not enforce this restriction.

- Walls documents `The total length of the three prefix components combined,
  including any embedded colon separators, is 127 characters` but Survex does
  not enforce any limit.

Warnings and what to do about them
==================================

Survex reports warnings in some suspect situations which Walls
quietly accepts.  In general this seems helpful and they do highlight
what look like genuine problems in existing datasets, but if there are
particular instances which are noisy and not useful, let us know.

Some of these warnings use the same wording as errors - most of these
probably ought to be an error except Walls quietly accepts them so we
don't want to fail processing because of them.

In some cases a warning can trigger when there's not a problem.  It's
good practice to keep a dataset free from warnings so that a new warning
doesn't get overlooked.  Here are suggestions for suppressing particular
warnings (which also work in Walls):

- Survex warns about Walls features that it doesn't currently implement.
  In general it's hard to fix these by changing your data (though in some
  cases you might not be intending to use the feature - for example, the
  Mammoth dataset had some surveys using ``TAPE=SS`` with clinos as well as
  heights, but on closer inspection all of these were incorrect use).
  In this case the warning is "actionable" by telling us.

- Survex warns about ``#FIX`` directives where the station is not ever
  used anywhere, for example::

    unusedfix.srv:3: warning: Unused fixed point “unused”

  This is intended to help catch a typo in the station name in the ``#FIX``
  directive, which would otherwise go unnoticed if the survey station which was
  meant to be fixed is connected to another fixed station.

  However it's also reasonable to record fixes for stations which don't
  have survey connected to them.  For example, they may be permanent
  survey benchmarks, or entrances to caves that haven't been surveyed yet.

  Assuming the station name is not mistyped, you can suppress the warning
  by adding a dummy "use" - ``#NOTE``, ``#FLAG`` or isolated LRUD all
  "use" a station.

  For example, here ``#NOTE`` is used to suppress the warning:

  ::

    #NOTE ABC123 /unused
    #FIX ABC123 123456 987654 1234

  Or you could use ``#FLAG`` instead, which has the bonus of allowing
  highlighting of such stations in Walls.  If it's an unsurveyed entrance
  we suggest adding a flag containing the word `entrance` (which will also
  result in Survex's "entrance" flag being set for the station), for
  example:

  ::

    #FLAG ABC123 /unexplored entrance
    #FIX ABC123 123456 987654 1234

  Otherwise you could just flag it as "unused":

  ::

    #FLAG ABC123 /unused fix
    #FIX ABC123 123456 987654 1234

  However, using ``#FLAG`` might be undesirable if you have a default flag
  which you expect to be applied to these stations.

  Isolated LRUD is another way (a dummy facing direction is needed to
  prevent Walls from warning):

  ::

    #FIX ABC123 123456 987654 1234
    ABC123 <--,--,--,--,0> ; Suppress Survex unused fix warning

- Walls doesn't complain about dangling surveys, which tends to result in
  larger Walls datasets having some hanging surveys.  A hanging survey used to
  be an error in Survex but since 1.4.10 a hanging survey is warned about and
  then ignored (for all supported survey data formats).

  This warning may indicate a missing (or typo-ed) connecting leg or ``#FIX``,
  in which case adding adding/fixing that is the solution.

  However, you might also want to add data you've received, but where the
  connection is currently missing but expected soon.  In this situation, we
  suggest adding an estimated connection, or marking the data not to be
  processed (either in the project tree, or by commenting out part of a survey
  with ``#[`` and ``#]``), or just tolerating the warning for a short time.

- Walls processes a missing ``.srv`` file without complaint (but does report an
  error if it exists but isn't readable, for example due to file permissions).

  There is an indication in the Walls UI (the icon is a blank page instead of a
  page with writing), but we've seen surveys accidentally not included in a
  real world dataset because the path wasn't set - this feature meant this was
  missed until the dataset was processed with Survex.

  Survex warns about missing files (and errors for unreadable files):

  ::

    wallsmissingsrv.wpj:7:7: warning: Couldn't open file "WX_MISS.srv"
     .NAME WX_MISS
           ^~~~~~~

  If the file should exist, check that ``Name:`` is spelled correctly, and that
  ``Path:`` is set suitably in the item's properties.

  If the file is a place-holder to be filled in later, we suggest either
  creating an empty file, or changing its ``Type`` to ``Other`` in the item's
  properties (and then back to ``Survey file`` when you later actually create
  it).

- If an isolated LRUD (LRUD not on a leg, e.g. specified for the start or end
  station of a traverse) is missing its closing delimiter, Walls will parse
  the line as a data leg where the "to" station starts with ``*`` or ``<``,
  which will usually succeed without errors or warnings.  A real-world example
  is::

    P25     *8 5 15 3.58

  Survex parses this like Walls does, but issues a warning::

    walls.srv:1:9: warning: Parsing as "to" station but may be isolated LRUD with missing closing delimiter
     P25     *8 5 15 3.58
             ^~

  This warning will also be triggered if you have a "to" station which actually
  starts with ``*`` or ``<``, if the station name was not previously seen.
  This condition provides a simple way to suppress the warning - just add a
  dummy ``#NOTE`` directive before the line of data like so::

    #note *8 - ; Suppress Survex warning that this looks like broken LRUD
    P25     *8 5 15 3.58

  An alternative way to suppress this warning is to put LRUD with all readings
  omitted on the leg)::

    P25     *8 5 15 3.58   <--,--,--,-->

  Walls parsing of cases where the LRUD has the same delimiter type as appears
  in the station name is quirky, so we recommend using the other delimiter
  (which also seems clearer), so for ``<`` at the start of the to station name
  use::

    P25     <8 5 15 3.58   *--,--,--,--*

  Walls 2.2.3 added a warning for this situation.

- In Europe a comma is customarily used for the decimal point.  Walls ``.svy``
  format treats a comma (``,``) as separating fields and allows optional
  instrument and target heights on each survey leg which can result in the data
  being interpreted in a different way to what the user intended.

  A real world example::

    GB1        GB2        5,00    0       30

  Walls treats this as equivalent to::

    GB1        GB2        5 00    0       30

  So Walls quietly parses this as a leg with length ``5``, compass ``00``,
  clino ``0`` and (optional field) instrument height ``30``.

  Survex warns about such cases, unless there's a decimal point in the
  component on either side of the comma.  If you have data where this warning
  fires but the comma is meant to be separate readings, you can workaround by
  changing the comma to a space, or adding a ``.0`` to one of the readings
  adjacent to the comma.   Please also report such cases and we'll try to
  adjust the warning conditions to avoid them.

- A few Walls directives and options are not supported by Survex and give
  warnings such as "Unknown command" or "Ignoring unsupported Walls option".
  These are all obscure features which we've not seen any real-world uses
  of, so we'd suggest first checking you are actually intending to use the
  warned about feature.  If you are, please let us know, ideally with some
  sample data showing how you're using it.

- Walls ignores junk after the numeric argument in ``TYPEAB=``, ``TYPEVB=``,
  ``UV=``, ``UVH=``, and ``UVV=``.  Survex warns and skips the junk.  To
  resolve the warning, work out what was intended and fix the option value.

- Options ``D=``, ``S=``, ``A=``, ``AB=``, ``V=``, ``VB=``, ``TYPEAB=`` and
  ``TYPEVB=`` take a string argument which can have one or several specified
  values.  Walls documents that these values can be abbreviated to just their
  first letter, but in fact it only checks the first letter, so for example
  ``D=FEAT`` is quietly accepted.  Survex warns when the value isn't one of
  those documented or a single letter abbreviation or ``DEG`` for ``DEGREES``
  (since that's commonly used), then handles as Walls does.  Fix this warning
  by correcting the value.

- The option ``CASE=`` is documented as `CASE = Upper / Lower /
  Mixed`.  Only the first character of the value is actually checked (as with
  several other options) but an additional quirk here is that any non-empty
  value seems to be allowed and unexpected initial characters are treated as
  ``Mixed``).  Survex warns if the value isn't one of ``UPPER``, ``LOWER``,
  ``MIXED``, ``U``, ``L`` or ``M``, then handles it like Walls seems to.
  To resolve this warning, determine what the intended setting was (or if
  you're happy to assume that the actual behaviour is OK you can replace
  a value starting ``U`` with ``UPPER``, a value starting ``L`` with
  ``LOWER``, and any other value with ``MIXED``).

- Walls doesn't issue an error for some directive lines which seem like they
  are invalid (at least there's no documented meaning), for example all of
  these are quietly accepted by Walls and apparently just ignored:

  ::

    #
    #<L,R,U,D>
    #<,>
    #*,>
    #!,"
    #THANK,YOU

  Survex issues "Unknown command" as a warning instead of an error for these.
  The complete pattern for what is accepted by Walls is hard to reliably
  discern (it seems to be either empty directives or those with a comma
  somewhere on the line after the invalid directive with no intervening
  whitespace) so it's possible there are cases which are still errors should
  also be warnings - please report any you come across.

  To fix these warnings, fix the bad directive.  It may be intended to be
  a comment since ``#`` is often used as a comment character in other
  contexts - if so, replace ``#`` with ``;`` (Walls actual comment character).

------------------
Walls .LST support
------------------

All Survex programs which can read a ``.3d`` file can transparently read a
Walls ``.LST`` file instead.  This is intended as a report format, but we
can usefully treat it as a processed survey data format.

In detail:

- Survey legs are handled (so for example, you can view a Walls-processed
  centreline in ``aven`` by exporting it as a ``.LST`` file).  This requires
  that *Connected vectors with file references* was selected in Walls when
  creating the ``.LST`` file.
- All stations are assumed to be underground and so flagged as ``UNDERGROUND``.
- Fixed stations are flagged as ``FIXED``.
- Georeferencing is handled for the same datum and UTM zone combinations that
  are handled in Walls ``.WPJ`` files.  Other combinations in a ``.LST`` file
  are quietly ignored, and the file loaded as if it wasn't georeferenced.
  Please report unsupported combinations you would find useful.
- If *Include only vectors or stations in current view frame* was selected
  legs which cross to outside the exported area are flagged as ``DUPLICATE``
  so they can be easily distinguished.
- The ``.LST`` format uses ``<REF>`` as a fake station at ``(0,0,0)`` which
  fixed points are connected to, but ``<REF>`` is actually a valid station
  name in Walls.  It's unlikely this name would be chosen in real-world data,
  but if it is, has no prefix, and is at coordinates ``(0,0,0))`` then it
  will be assumed to be this fake station.
