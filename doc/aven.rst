aven
----

~~~~~~~~
SYNOPSIS
~~~~~~~~

   ``aven`` [--survey=\ `SURVEY`] [--print] `SURVEY_FILE`

~~~~~~~~~~~
DESCRIPTION
~~~~~~~~~~~

Aven displays processed cave surveys in a window and allows you to
manipulate the view.

If ``SURVEY_FILE`` is an unprocessed survey data format which ``cavern``
can process, then ``aven`` will run ``cavern`` on it, and if successful,
display the processed data.  If there are any warnings and errors, it will
show a log window with the output with clickable links to open the affected
file at the problematic line.

``SURVEY_FILE`` can also be processed survey data - a Survex ``.3d`` file, a
Compass ``.plt`` file or a CMAP ``.sht`` file.  It can also be a Survex
``.pos`` file or a CMP ``.una`` or ``.adj`` file, but for these only
stations are shown, not any legs (for ``.pos`` this is because the format
only records station positions).  (All Survex programs which read ``.3d``
files can also transparently handle these formats.)

On-Screen Indicators
~~~~~~~~~~~~~~~~~~~~

There is an auto-resizing scale bar along the bottom of the screen
which varies in length as you zoom in or out.  You can left-button drag
on this to zoom, and right click gives a menu to select units or hide
the scale bar (to get it back go to the Control->Indicators menu from
the menu bar).

In the lower right corner is a compass indicator showing which way is North.
You can drag this to rotate the view; if while dragging you move the mouse
outside the compass the view snaps to 45° positions: N/NE/E/SE/S/SW/W/NW.
Right click gives menu to change the view to N/S/E/W, select units, or
hide the compass (to get it back go to the Control->Indicators menu from
the menu bar).

Just to the left of the compass is clino indicator showing the angle of tilt.
You can drag this to tilt the view; if while dragging you move the mouse
outside the clino the view snaps to 90° positions: plan/elevation/inverted
plan.  Right click gives menu to change the view to plan/elevation, select
units, or hide the clino (to get it back go to the Control->Indicators menu
from the menu bar).

In the upper right is a colour key showing the correspondence between colour
and depth (by default - you can also colour by other criteria).  Right click
gives a menu to choose what to colour by, select units, or hide the colour
key (to get it back go to the Control->Indicators menu from the menu bar).

Mouse Control
~~~~~~~~~~~~~

Using the mouse to move the cave will probably feel most natural.  We suggest
you try each of these out after reading this section to get a feel for how they
work.

If you hold down the right button then the view is panned when you move the
mouse - it effectively feels like you are dragging the cave around.

If you hold down the left button, then the view is rotated as you move left or
right, and zoomed as you move up and down.  If you hold down ``Ctrl`` while
dragging with the left mouse button, then moving up and down instead tilts the
view.  Tilt goes 180 degrees from plan view through elevation view to a view
from directly below (upside down plan) - aven deliberately doesn't allow going
beyond horizontal into an inverted view.

If your mouse has a middle button then holding it down and moving the mouse up
and down tilts the cave.  Moving the mouse left and right has no effect.

And if you have a scrollwheel, this can be used to zoom in/out.

By default the mouse moves the cave, but if you press ``Ctrl-R``, then the
mouse will move the viewpoint instead (i.e. everything will go in the opposite
direction).  Apparently this feels more natural to some people.

Keyboard Control
~~~~~~~~~~~~~~~~

As with mouse controls, a little experimentation should give a better
understanding of how these work.

All keyboard shortcuts have a corresponding menu items which should show the
keyboard shortcut - this provides a way within the application to see the
keyboard shortcut for a particular action.

``Delete`` is useful if you get lost!  It resets the scale, position, and
rotation speed, so that the cave returns to the centre of the screen.  There
are also keyboard controls to use

``P`` and ``L`` select Plan and eLevation respectively.  Changing between plan
to elevation is animated to help you see where you are and how things relate.
This animation is automatically disabled on slow machines to avoid user
frustration.  You can force skipping the animation by pressing the key again
during it, so a double press will always take you there quickly.

``Space`` toggles on and off automatic rotation about a vertical axis through
the current centre point (which is moved by panning the view or by selecting
a station or survey).  ``R`` toggles the direction of auto-rotation.  The speed
of auto-rotation can be controlled by ``Z`` and ``X``.

Crosses and/or labels can be displayed at survey stations.  ``Ctrl-X`` toggles
crosses and ``Ctrl-N`` station names.  ``Ctrl-L`` toggles the display of survey
legs and ``Ctrl-F`` of surface survey legs.

``Ctrl-G`` toggles display of an auto-sizing grid.

``Ctrl-B`` toggles display of a bounding box.

``O`` toggles display of non-overlapping/all names.  For a large survey turning
on overlapping names will make update rather slow.

Holding down ``Shift`` accelerates all the following movement keys:

The cursor keys pan the survey view (like dragging with the right mouse button).

``Ctrl`` plus cursor keys rotate and tilt (like the mouse left button with
``Ctrl`` held down).  ``C``/``V`` do the same as ``Ctrl`` plus cursor
left/right, while Apostrophe ``'``, and Slash ``/`` do the same as ``Ctrl``
plus cursor up/down.

``[`` and ``]`` zoom out and in respectively.

~~~~~~~
OPTIONS
~~~~~~~

``-p``, ``--print``
   Load the specified file, open the print dialog to allow printing, then exit.
``-s``, ``--survey=``\ `SURVEY`
   Only load the sub-survey `SURVEY`.
``--help``
   display short help and exit
``--version``
   output version information and exit

.. only:: man

   ~~~~~~~~
   SEE ALSO
   ~~~~~~~~

   ``cavern``\ (1), ``diffpos``\ (1), ``dump3d``\ (1), ``extend``\ (1), ``sorterr``\ (1), ``survexport``\ (1)
