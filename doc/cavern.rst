cavern
------

~~~~~~~~
SYNOPSIS
~~~~~~~~

   ``cavern`` [`OPTIONS`] `SURVEY_DATA_FILE`...

~~~~~~~~~~~
DESCRIPTION
~~~~~~~~~~~

``cavern`` is the Survex data processing engine.

``cavern`` is a command line tool, but if you're not a fan of working from the
command line you can open unprocessed survey data files with ``aven`` and it
will run ``cavern`` for you, and if successful, display the processed data.  If
there are any warnings and errors, ``aven`` will show a log window with the
output with clickable links to open the affected file at the problematic line.

If multiple survey data files are listed on the command line, they
are processed in order from left to right.  Settings are reset to
their defaults before processing each file.

Each `SURVEY_DATA_FILE` must be unprocessed survey data in a format
which Survex supports, either native format (``.svx``) or Compass format
(``.mak``, ``.dat`` or ``.clp``), or Walls format (``.wpj`` or ``.srv``).

Support for Compass ``.clp`` was added in Survex 1.4.6; support for
Walls was added in Survex 1.4.9.

~~~~~~~
OPTIONS
~~~~~~~

``-o``, ``--output=``\ `OUTPUT`
   Sets location for output files.

``-q``, ``--quiet``
   Only show a brief summary (``--quiet --quiet`` or ``-qq`` will display
   warnings and errors only).

``-s``, ``--no-auxiliary-files``
   do not create .err file.

``-w``, ``--warnings-are-errors``
   turn warnings into errors.

``--log``
   Send screen output to a .log file.

``-v``, ``--3d-version=``\ `3D_VERSION`
   Specify the 3d file format version to output.  By default the
   latest version is written, but you can override this to produce
   a 3d file which can be read by software which doesn't
   understand the latest 3d file format version.  Note that any
   information which the specified format version didn't support
   will be omitted.

``--help``
   display short help and exit

``--version``
   output version information and exit

~~~~~~
OUTPUT
~~~~~~

If there were no errors during processing, cavern produces two
output files, with the extensions ``.3d`` and ``.err`` (unless
``--no-auxiliary-files`` is specified in which case only the ``.3d``
file is produced).

These two files are always created with their respective extensions.  By
default they are created in the current directory, with the same base filename
as the first `SURVEY_DATA_FILE` listed on the command line.

E.g. if you process the data file ``entrance.svx`` with the command
``cavern entrance`` or ``cavern entrance.svx`` then the files ``entrance.3d``
and ``entrance.err`` will be created.

You can change the directory and/or base filename using the ``--output``
command line option.  If you specify a directory then output files will
go there instead of the current directory, but still use the basename
of the first `SURVEY_DATA_FILE`.  If you specify a filename which is not a
directory (note that it doesn't need to actually exist as a file) then the
directory this file is in is used, and also the basename of the filename
is used instead of the basename of the first `SURVEY_DATA_FILE`.

Details of the output files:

``.3d``
   This is a binary file format containing the adjusted survey data and
   associated meta data.
``.err``
   This is a text file which contains statistics about each traverse in the
   survey which is part of a loop.  It includes various statistics for each
   traverse:

   Original length
      This is the measured length of the traverse (for a "normal" or "diving"
      survey this is the sum of the tape readings after applying calibration
      corrections).
   Number of legs
      The number of survey legs in the traverse
   Moved
      How much one end of the traverse moved by relative to the other after
      loop closure
   Moved per leg
      `Moved` / `Number of legs`
   Percentage error
      (`Moved` / `Original length`) as a percentage.  This seems to be a
      popular measure of how good or bad a misclosure is, but it's a
      problematic one because a longer traverse will naturally tend to
      have a lower percentage error so you can't just compare values
      between traverses.  We recommend using the `E`, `H` and `V` values
      instead.
   Error (`E`)
      This isn't labelled in the `.err` file but is the value on a line by
      itself.  In ``aven`` it's the value used by `Colour by Error`.  It
      is `Moved` divided by the standard deviation for the traverse based on
      the standard errors specified for the instruments.  This tells us how
      plausible it is that the misclosure is just due to random errors.  It
      is a number of standard deviations, so the `68-95-99.7 rule
      <https://en.wikipedia.org/wiki/68%E2%80%9395%E2%80%9399.7_rule>`__
      applies - e.g. approximately 99.7% of traverses should have a value of
      3.0 or less (assuming the specified instrument standard deviations are
      realistic).
    Horizontal Error (`H`)
      This is like `E` but only considers the horizontal component.  In
      ``aven`` it's the value used by `Colour by Horizontal Error`.  You
      can identify suspect traverses by looking at `E` and then compare
      `H` and `V` to see what sort of blunder might explain the misclosure.
      For example, if `H` is small but `V` is large it could be a clino reading
      or plumb with an incorrect sign, or a tape blunder on a plumbed leg; if
      `H` is large but `V` is small it could be a compass blunder, or a tape
      blunder of a nearly-flat leg.
    Vertical Error (`V`)
      This is like `E` but only considers the vertical component.  In
      ``aven`` it's the value used by `Colour by Vertical Error`.

   This information is now also present in the ``.3d`` file so you can view the
   survey coloured by these errors, but the ``.err`` file can
   still be useful as you can sort it using ``sorterr`` to get a ranked list of
   the sections of survey with the worst misclosure errors.

Cavern also reports a range of statistics at the end of a successful
run:

- The highest and lowest stations and the height difference between
  them
- The East-West and North-South ranges, and the Northernmost,
  Southernmost, Easternmost, and Westernmost stations.
- The total length of the survey (before and after adjustment).  This
  total excludes survey legs flagged as ``SURFACE``, ``DUPLICATE``, or
  ``SPLAY``.
- The number of stations and legs. Note that a ``*equate`` is counted
  as a leg in this statistic.
- The number of each size of node in the network (where size is number of
  connections to a station) i.e. a one node is the end of a dead-end traverse,
  a two-node is a typical station in the middle of a traverse, a three-node is
  a T-junction etc.
- How long the processing took and how much CPU time was used.

If you successfully processed your data by loading it into ``aven`` then you
can see this log output by using ``File->Show Log`` (also available as an icon
in the toolbar).

Error Messages
~~~~~~~~~~~~~~

There are many different error messages that you can get when processing
data.  Along with the error message, a location is reported.  For an error
like "file not found" this only reports the filename, but usually it will
give the filename and line number of the offending line, and in many cases also
an offset or span within the line.  

The format of the location data follows that used by the GCC compiler
so if your text editor can parse errors from GCC then you should be able to set
it to allow you to jump to the file and line of each error.

One common cause of errors and warnings are typing mistakes.  Another is
your survey data not being all attached to fixed points (which is a warning
since Survex 1.4.10, but was an error prior to this; in this situation, Survex
will list at least one station in each piece of survey data which is not
connected).

We try to make error and warning messages self-explanatory, but welcome
feedback on cases where you get a message which seems unclear.

Generally you want to look at the first reported error first as there
can be a cascade effect where one error triggers another.  Cavern will stop
after more than 50 errors.  This usually indicates something like the incorrect
data order being specified and deluging the user with error messages in such
cases usually makes the actual problem less clear.

.. only:: man

   ~~~~~~~~
   SEE ALSO
   ~~~~~~~~

   ``aven``\ (1), ``diffpos``\ (1), ``dump3d``\ (1), ``extend``\ (1), ``sorterr``\ (1), ``survexport``\ (1)
