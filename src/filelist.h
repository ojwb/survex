/* > filelist.h
 * Filenames, extensions, etc used by Survex system
 * Copyright (C) 1993,1994,1996 Olly Betts
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
# define EXT_SVX_ERRS "/err"
# define EXT_SVX_STAT "/inf"
# define EXT_SVX_POS  "/pos"
# define EXT_SVX_MSG  "/msg"

#elif (OS==MSDOS || OS==TOS || OS==WIN32 || OS==UNIX || OS==AMIGA)

# define ERRSTAT_FILE "errstats.txt"
# define STATS_FILE   "stats.txt"
# define IMAGE_FILE   "image.3d"
# define POSLIST_FILE "poslist.txt"
# define MESSAGE_FILE "messages.txt"

# define PRINT_INI "print.ini"

# define EXT_SVX_DATA ".svx"
# define EXT_SVX_3D   ".3d"
# define EXT_SVX_ERRS ".err"
# define EXT_SVX_STAT ".inf"
# define EXT_SVX_POS  ".pos"
# define EXT_SVX_MSG  ".msg"

#else
# error Do not know operating system 'OS'
#endif
