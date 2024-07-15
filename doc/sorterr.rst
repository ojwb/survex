sorterr
-------

~~~~~~~~
SYNOPSIS
~~~~~~~~

   ``sorterr`` [`OPTIONS`] `ERR_FILE` [`HOW_MANY`]

~~~~~~~~~~~
DESCRIPTION
~~~~~~~~~~~

``sorterr`` re-sorts a .err file by the specified criterion (or by the error ratio
by default).  Output is sent to stdout, or if ``--replace`` (short option
``-r``) is specified the input file is replaced with the sorted version.  By
default all entries in the file are included - if a second parameter is given
then only the top ``HOW_MANY`` entries after sorting are returned.

~~~~~~~
OPTIONS
~~~~~~~

``-h``, ``--horizontal``		
   sort by horizontal error factor
``-v``, ``--vertical``		
   sort by vertical error factor
``-p``, ``--percentage``		
   sort by percentage error
``-l``, ``--per-leg``			
   sort by error per leg
``-r``, ``--replace``			
   replace .err file with re-sorted version
``--help``
   display short help and exit
``--version``
   output version information and exit

.. only:: man

   ~~~~~~~~
   SEE ALSO
   ~~~~~~~~

   ``aven``\ (1), ``cavern``\ (1), ``diffpos``\ (1), ``dump3d``\ (1), ``extend``\ (1), ``survexport``\ (1)
