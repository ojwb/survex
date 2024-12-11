extend
------

~~~~~~~~
SYNOPSIS
~~~~~~~~

   ``extend`` [--survey=\ `SURVEY`] [--specfile=\ `ESPEC_FILE`] [--show-breaks] `INPUT_FILE` [`OUTPUT_3D_FILE`]

~~~~~~~~~~~
DESCRIPTION
~~~~~~~~~~~

``INPUT_FILE`` can be a Survex ``.3d`` file, a Compass ``.plt`` file or a
CMAP ``.sht`` file (all Survex programs which read ``.3d`` files can also
transparently handle these formats).

If no ``--specfile`` option (or short option ``-p``) is given, extend starts
with the highest station marked as an entrance which has at least one
underground survey leg attached to it.  If there are no such stations, the
highest deadend station in the survey (or the highest station if there are no
deadends) is used.  Extend puts the first station on the left, then folds each
leg out individually to the right, breaking loops arbitrarily (usually at
junctions).

If the output filename is not specified, extend bases the output filename on
the input filename, but replacing the extension with ``_extend.3d``. For
example, ``extend deep_pit.3d`` produces an extended elevation called
``deep_pit_extend.3d``.

The ``--survey=``\ `SURVEY` option (short option ``-s``) restricts processing to
the survey `SURVEY` including any sub-surveys.

If you pass ``--show-breaks`` (short option ``-b``) then a leg flagged as
"surface survey" will be added between each point at which a loop has been
broken - this can be very useful for visualising the result in aven.

This approach suffices for simple caves or sections of cave, but for
more complicated situations human intervention is required.  More
complex sections of cave can be handled with a specfile giving
directions to switch the direction of extension between left and
right, to explicitly specify the start station, or to break the
extension at particular stations or legs.

The specfile is in a format similar to cavern's data format:
::

   ; This is a comment

   ; start the elevation at station entrance.a
   *start entrance.a  ;this is a comment after a command

   ; start extending leftwards from station half-way-down.5
   *eleft half-way-down.5

   ; change direction of extension at further-down.8
   *eswap further-down.8

   ; extend right from further-down.junction, but only for
   ; the leg joining it to very-deep.1, other legs continuing
   ; as before
   *eright further-down.junction  very-deep.1

   ; break the survey at station side-loop.4
   *break side-loop.4

   ; break survey at station side-loop.junction but only
   ; for leg going to complex-loop.2
   *break side-loop.junction complex-loop.2

This approach requires some trial and error, but gives useful results
for many caves.  The most complex systems would benefit from an
interactive interface to select and view the breaks and switches of
direction.

.. only:: man

   ~~~~~~~~
   SEE ALSO
   ~~~~~~~~

   ``aven``\ (1), ``cavern``\ (1), ``diffpos``\ (1), ``dump3d``\ (1), ``sorterr``\ (1), ``survexport``\ (1)
