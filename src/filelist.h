/* > filelist.h
 * Filenames, extensions, etc used by Survex system
 * Copyright (C) 1993-2000 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "whichos.h"

#if (OS==RISCOS)

# define ERRSTAT_FILE "ErrStats"
# define STATS_FILE   "Stats"
# define IMAGE_FILE   "Image3D"
# define POSLIST_FILE "PosList"
# define MESSAGE_FILE "Messages"

# define PRINT_INI "print/ini"

# define EXT_SVX_DATA "/svx" /* allows files to be read from DOS discs */
# define EXT_SVX_3D   "/3d"

#ifdef NEW3DFORMAT
# define EXT_SVX_3DX  "/3dx"
#endif

# define EXT_SVX_ERRS "/err"
# define EXT_SVX_STAT "/inf"
# define EXT_SVX_POS  "/pos"
# define EXT_SVX_MSG  "/msg"

#elif (OS==MSDOS || OS==TOS || OS==WIN32 || OS==UNIX || OS==AMIGA)

# define ERRSTAT_FILE "errstats.txt"
# define STATS_FILE   "stats.txt"
# define IMAGE_FILE   "image.3d"

#ifdef NEW3DFORMAT
# define NEW_IMAGE_FILE  "image.3dx"
#endif

# define POSLIST_FILE "poslist.txt"
# define MESSAGE_FILE "messages.txt"

# define PRINT_INI "print.ini"

# define EXT_SVX_DATA ".svx"
# define EXT_SVX_3D   ".3d"

#ifdef NEW3DFORMAT
# define EXT_SVX_3DX  ".3dx"
#endif

# define EXT_SVX_ERRS ".err"
# define EXT_SVX_STAT ".inf"
# define EXT_SVX_POS  ".pos"
# define EXT_SVX_MSG  ".msg"

#else
# error Do not know operating system 'OS'
#endif
