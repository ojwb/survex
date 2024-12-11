diffpos
-------

~~~~~~~~
SYNOPSIS
~~~~~~~~

   ``diffpos`` `FILE1` `FILE2` [`THRESHOLD`]

~~~~~~~~~~~
DESCRIPTION
~~~~~~~~~~~

Diffpos reports stations which are in one file but not the other, and
also stations which have moved by more than a specified threshold
distance in X, Y, or Z.  `THRESHOLD` is a distance in metres and
defaults to 0.01m if not specified.

Note that the input files can be any format the "img" library can read (and
can be different formats), so it works with Survex ``.3d`` and ``.pos`` files,
Compass ``.plt`` and ``.plf`` files, CMAP ``.sht``, ``.adj`` and ``.una``
files.

~~~~~~~
OPTIONS
~~~~~~~

``--help``
   display short help and exit
``--version``
   output version information and exit

.. only:: man

   ~~~~~~~~
   SEE ALSO
   ~~~~~~~~

   ``aven``\ (1), ``cavern``\ (1), ``dump3d``\ (1), ``extend``\ (1), ``sorterr``\ (1), ``survexport``\ (1)
