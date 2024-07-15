---------------
Survex Programs
---------------

This section describes command-line use of Survex.  Our aim is to make all
functionality available without needing to use the command line (though we
aren't quite there currently - the main thing lacking is more complex use of
``extend``).  We also aim to give access from the command line to any
functionality which it is useful to be able to use from scripts, so that
users who do like to use the command line can take full advantage of it.

Standard Options
================

All Survex programs support to the following command line options:

``--help``
   display option summary and exit

``--version``
   output version information and exit

Tools which take a processed survey data file to read also provide a way to
specify the prefix of a sub-survey to restrict reading to:

``-s``, ``--survey=``\ `SURVEY`
   Only load the sub-survey `SURVEY`.

Short and Long Options
======================

Options have two forms: short (a dash followed by a single letter e.g. ``cavern
-q``) and long (two dashes followed by one or more words e.g. ``cavern
--quiet``).  The long form is generally easier to remember, while the short
form is quicker to type.  Options are often available in both forms, but more
obscure or potentially dangerous options may only have a long form.

.. note:: Command line options are case sensitive, so ``-B`` and ``-b``
   are different (this didn't used to be the case before Survex 0.90).  Case
   sensitivity doubles the number of available short options (and is also the
   norm on UNIX-like platforms).

Filenames on the Command Line
=============================

Filenames with spaces can be processed (provided your operating system supports
them - UNIX does, and so do modern versions of Microsoft Windows).  You need to
enclose the filename in quotes like so::

   cavern "Spider Cave"

A file specified on the command line of any of the Survex suite of programs
will be looked for as specified.  If it is not found, then the file is looked
for with the appropriate extension appended, so the command above will look
first for ``Spider Cave``, then for ``Spider Cave.svx``.

Command Reference
=================

.. include:: aven.rst
.. include:: cavern.rst
.. include:: diffpos.rst
.. include:: dump3d.rst
.. include:: extend.rst
.. include:: sorterr.rst
.. include:: survexport.rst

.. Dummy non-use of survex.rst to prevent warnings (it's only used to generate the
   survex.7 man page).

.. only:: somethingthatshouldneverbedefined

    .. include:: survex.rst
