---------------
Getting Started
---------------

This section covers how to obtain the software, and how to unpack and
install it, and how to configure it.

Obtaining Survex
================

The latest version is available from the `Survex website
<https://survex.com/download.html>`__.  It is freely redistributable, so you are
welcome to get a copy from someone else who has already downloaded
it, and you can give copies to others.

If you want some sample data to experiment with, you can download some from the
Survex website too: https://survex.com/software/sample.tar.gz

Installing Survex
=================

The details of installation depend greatly on what platform you
are using, so there is a separate section below for each platform.

Linux
-----

Pre-built versions of Survex are available for some Linux distributions.  See
the `Survex for Linux download page
<https://survex.com/download.html?platform=linux>`__ on
our website for up-to-date information.

You'll need root access to install these prebuilt packages.  If you don't have
root access you will need to build from source (see the next section).

macOS
-----

The easiest way to install a recent release of Survex on macOS is by using the
Homebrew package manager.  If you don't already use Homebrew, you'll need to
install it first.  See the `macOS download page on the website
<https://survex.com/download.html?platform=macosx>`__ for installation
instructions.

Other versions of UNIX
----------------------

For other UNIX versions you'll need to get the source code and compile it on
your system.  Unpack the sources and read the file called INSTALL in the top
level for details about building from source.

Microsoft Windows
-----------------

This version comes packaged with an installation wizard.  Just run the
downloaded installer package and it will lead you through the installation
process.  Since Survex 1.4.9, this pre-built version requires a 64-bit
version of Microsoft Windows 7 or later.

Survex 1.4.8 and later support installing for all users (which requires
administrator rights) or just for the current user (which doesn't).  If
installed for just the current user, other user accounts won't see the file
associations, menu entries, desktop icons, etc for Survex.

Note that if you have an existing installation the installer will see it and
try to upgrade it, and if that installation was done with administrator rights
(which any installation of 1.4.7 or earlier will be) you'll also need
administrator rights to upgrade.  To change to a non-admin installation you
need to first uninstall the existing admin install (which will need admin
rights) then install the new version.

The survey viewer that's part of Survex is called aven, and uses OpenGL for 3d
rendering.

If you find that 3D rendering is sometimes very slow (e.g. one user reported
very slow performance when running full screen, while running in a window was
fine) then try installing the OpenGL driver supplied by the manufacturer of
your graphics card rather than the driver Microsoft supply.

The installer creates a Survex group in the Programs sub-menu of the Start menu
containing the following items:

-  Aven

-  Documentation

-  Uninstall Survex

   Icons are installed for ``.svx``, ``.3d``, ``.err``, and ``.pos`` files, and
   also for Compass Plot files (``.plt`` and ``.plf``) (which Survex can read).
   Double-clicking on a ``.svx`` file loads it for editing.  To process it to
   produce a ``.3d`` file, right click and choose "Process" from the menu -
   this runs aven to process the ``.svx`` file and automatically load the
   resultant ``.3d`` file.  All the Survex file types can be right clicked on
   to give a menu of possible actions.

   ``.svx``
      Process
         Process file with aven to produce ``.3d`` file (and ``.err`` file)

   ``.3d``
      Open
         Load file into Aven

      Print
         Print the file via Aven

      Extend
         Produce extended elevation

      Convert to DXF
         This entry used to be provided to allow converting to a DXF file
         (suitable for importing into many CAD packages) but this functionality
         is now available from inside Aven with the ability to control what is
         exported, and this entry was dropped in 1.2.35.

      Convert for hand plotting
         This entry used to be provided to allow converting to a ``.pos`` file
         listing all the stations and their coordinates, but this functionality
         is now available from inside Aven with the ability to control what is
         exported, and this entry was dropped in 1.2.35.

   ``.err``
      Open
         Load file into Notepad

      Sort by Error
         Sort ``.err`` file by the error in each traverse

      Sort by Horizontal Error
         Sort ``.err`` file by the horizontal error in each traverse

      Sort by Vertical Error
         Sort ``.err`` file by the vertical error in each traverse

      Sort by Percentage Error
         Sort ``.err`` file by the percentage error in each traverse

      Sort by Error per Leg
         Sort ``.err`` file by the error per leg in each traverse

Configuration
=============

Selecting Your Preferred Language
---------------------------------

Survex has extensive internationalisation capabilities.  The language used for
messages from Survex and most of the libraries it uses can be changed.  By
default this is automatically picked up from the language the operating system
is set to use (from "Regional Settings" in Control Panel on Microsoft Windows,
from the LANG environment variable on UNIX).  If no setting is found, or Survex
hasn't been translated into the requested language, UK English is used.

However you may want to override the language manually - for example if Survex
isn't available in your native language you'll want to choose the supported
language you understand best.

To do this, you set the ``SURVEXLANG`` environment variable.  Here's a list of
the codes currently supported:

========= =====================
Code      Language
========= =====================
``en``    International English
``en_US`` US English
``bg``    Bulgarian
``ca``    Catalan
``cs``    Czech
``de``    German
``de_CH`` Swiss German
``el``    Greek
``es``    Spanish
``fr``    French
``hu``    Hungarian
``id``    Indonesian
``it``    Italian
``pl``    Polish
``pt``    Portuguese
``pt_BR`` Brazillian Portuguese
``ro``    Romanian
``ru``    Russian
``sk``    Slovak
``zh_CN`` Chinese (Simplified)
========= =====================

Here are examples of how to set this environment variable to give messages in
French (language code ``fr``):

Microsoft Windows
   For MS Windows proceed as follows (this description was
   written from MS Windows 2000, but it should be fairly
   similar in other versions): Open the Start Menu, navigate
   to the Settings sub-menu, and open Control Panel. Open
   System (picture of a computer) and click on the Advanced
   tab. Choose ``Environmental Variables``, and create a new
   one: name ``SURVEXLANG``, value ``fr``.  Click ``OK`` and the new
   value should be effective immediately.

UNIX - sh/bash
   ``SURVEXLANG=fr ; export SURVEXLANG``

UNIX - csh/tcsh
   ``setenv SURVEXLANG fr``

If Survex isn't available in your language, you could help out by providing a
translation.  The initial translation is likely to be about a day's work; after
that translations for new or changed messages are occasionally required.
Contact us for details if you're interested.

Using Survex
============

Most common tasks can now be accomplished through ``aven`` - processing survey
data, viewing the processed data, printing, exporting to other formats, and
producing simple extended elevations.  

A few tasks still require you to use the command line; some functionality
which is available via ``aven`` is also available from the command line, which
allows it to be scripted.

.. FIXME the remainder of this section seems rather redundant with the
.. cmdline section that follows.

The command line programs that come with Survex are:

``cavern``
   Processes survey data.  Since Survex 1.2.3 you can process ``.svx``
   files by opening them with ``aven``, so you don't need to use
   ``cavern`` from the command line if you don't want to, but it's still
   available for users who prefer to work from the command line and for
   use in scripts.

``diffpos``
   Compares the positions of stations in two processed survey data files
   (``.3d``, ``.pos``, ``.plt``, etc).

``dump3d``
   Dumps out a list of the items in a processed survey data file (``.3d``,
   ``.plt``, etc).  ``dump3d`` was originally written for debugging, but can
   also be useful if you want to access processed survey data from a script.

``extend``
   Produces extended elevations - this is probably the most useful of these
   command line tools.  Since Survex 1.2.27 you can produce simple extended
   elevations from ``aven`` using the "Extended Elevation" function.  However
   the command line tool allows you to specify a spec file to control how the
   survey is extended, which you can't currently do via ``aven``.

``sorterr``
   Reorders a .err file by a specified field.

``survexport``
   Provides access to ``aven``'s "Export" functionality from the command line,
   which can be useful in scripts.  Added in Survex 1.2.35.
