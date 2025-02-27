=====================
Using the img library
=====================

The code Survex uses to read and write ``.3d`` files is provided as the
img library which allows it to be reused by other programs which are written
in C, C++ or another language which is able to call C functions.

Using this code means you can take advantage of any revisions to the 3d
file format by simply rebuilding your software with the updated
``img.c`` and ``img.h``, whereas if you write your own code to parse 3d
files then you'll have to update it for each revision to the 3d file format.

Using the img library also allows you to read old revisions of the Survex .3d
format and also other processed survey data formats: currently it supports
reading Survex ``.pos`` files, and processed survey data from `Larry Fish's
Compass <reading_compass_>`_ and from `Bob Thrun's CMAP <reading_cmap_>`_).

.. _reading_compass: compass.htm#reading-compass-plf-plt
.. _reading_cmap: cmap.htm

It also allows reading a subset of the data in a file, restricted by survey
prefix.

If not able to use the img library, parsing text output from ``dump3d``
provides many of the same benefits.  If you take this approach, you
may find the ``--legs`` option more convenient as it gives you a ``LEG``
line for each leg rather than ``MOVE`` for the first station in a traverse
then ``DRAW`` for each subsequent station.

--------------------------
Using img in your own code
--------------------------

The expected way to use img is that you copy ``src/img.c`` and ``src/img.h``
into your source tree, and periodically update them (they usually evolve fairly
slowly).

It's not currently packaged as a separate library for Debian or anywhere else
I'm aware of (there's a very low number of applications which link to it and
the effort to do that seems more usefully directed; we also don't
guarantee ABI stability, but it should be upwardly API compatible).

---
API
---

The API is documented by comments in ``src/img.h``.

See ``src/imgtest.c`` for an example program using img in "unhosted mode"
(which is how you'd use it outside of Survex).  Also ``src/dump3d.c`` shows
how to access all the different types of data returned (that's built "hosted"
but that only affects the ``#include`` lines, linking and error reporting).

If anything is unclear, please ask on the mailing list and we can clarify.
