dump3d
------

~~~~~~~~
SYNOPSIS
~~~~~~~~

   ``dump3d`` [--survey=\ `SURVEY`] [--rewind] [--show-dates] [--legs] `INPUT_FILE`

~~~~~~~~~~~
DESCRIPTION
~~~~~~~~~~~

Dump out the entries in a processed survey data file - useful for debugging,
and also provides a textual format which is fairly easy to parse if you want
to write a simple script to pull out information from such files.

Don't be mislead by the "3d" in this tool's name - it can be used to dump a
file in any format the "img" library can read, so it works with Survex ``.3d``
and ``.pos`` files, Compass ``.plt`` and ``.plf`` files, CMAP ``.sht``,
``.adj`` and ``.una`` files.

If you're parsing the output in a script, you may want to use option ``-legs``
so you get a ``LEG`` item for each leg with from and to coordinates instead of
each traverse being a ``MOVE`` item followed by a series of ``LINE`` items.

(The ``--rewind`` option is only here to allow debugging and testing the
``img_rewind()`` function.)

~~~~~~~
OPTIONS
~~~~~~~

``-s``, ``--survey=``\ `SURVEY`
   only load the sub-survey with this prefix
``-r``, ``--rewind``
   rewind file and read it a second time
``-d``, ``--show-dates``
   show survey date information (if present)
``-l``, ``--legs``
   convert MOVE and LINE into LEG
``--help``
   display short help and exit
``--version``
   output version information and exit

.. only:: man

   ~~~~~~~~
   SEE ALSO
   ~~~~~~~~

   ``aven``\ (1), ``cavern``\ (1), ``diffpos``\ (1), ``extend``\ (1), ``sorterr``\ (1), ``survexport``\ (1)
