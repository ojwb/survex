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
file is produced):

``.3d``
   This is a binary file format containing the adjusted survey data and
   associated meta data.
``.err``
   This is a text file which contains statistics about each traverse in the
   survey which is part of a loop.  It includes various statistics for each
   traverse, such as the percentage error per leg.  This information is now
   also present in the ``.3d`` file so you can view the survey coloured by
   these errors, but the ``.err`` file can still be useful as you can sort
   it using ``sorterr`` to get a ranked list of the sections of survey with
   the worst misclosure errors.

.. FIXME: Explain what the statistics in .err mean - or in sorterr page?

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
