/* > filelist.h
 * Filenames, extensions, etc used by Survex system
 * Copyright (C) 1993,1994,1996 Olly Betts
 */

/*
1993.05.19 (W) created
1993.05.27 stripped semi-colons from after #defines
1993.05.28 removed duplicate #define for OS==AMIGA
1993.06.04 added () to #if
           moved FNM_SEP_XXX here from error.c
1993.07.01 corrected readme.txt to survex.doc (for addresses)
1993.07.14 moved DOOMDAY_H here from doomday.c
1993.07.19 moved FNM_SEP_xxx to osdepend.h (was common.h)
1993.08.15 DOS -> MSDOS
1993.08.16 added STATS_FILE for sending survey statistics to
           added more EXT_SVX_* for output files
1993.10.20 (W) added Optionchars
1993.10.24 changed Optionchars to SWITCH_SYMBOLS and moved to osdepend.h
1993.11.05 print*, caverot and survex all use the same error message file now
1993.11.21 errlist.txt -> messages.txt
1993.11.28 added DM_CFG, PCL_CFG, PS_CFG and HPGL_CFG
1994.04.10 added TOS
1994.06.20 README_FILE is no longer used
1996.02.27 PRINT_INI added
*/

/* *_CFG now unused */

#include "whichos.h"

#if   (OS==RISCOS)

# define ERRSTAT_FILE "ErrStats"
# define STATS_FILE   "Stats"
# define IMAGE_FILE   "Image3D"
# define POSLIST_FILE "PosList"
# define MESSAGE_FILE "Messages"

# define PCL_CFG  "pclcfg"
# define DM_CFG   "dmcfg"
# define PS_CFG   "pscfg"
# define HPGL_CFG "hpglcfg"
# define PRINT_INI "print/ini"

# define EXT_SVX_DATA "/svx" /* allows files to be read from DOS discs */
# define EXT_SVX_3D   "/3d"
# define EXT_SVX_ERRS "/err"
# define EXT_SVX_STAT "/inf"
# define EXT_SVX_POS  "/pos"

#elif (OS==MSDOS || OS==TOS || OS==WIN32)

# define ERRSTAT_FILE "errstats.txt"
# define STATS_FILE   "stats.txt"
# define IMAGE_FILE   "image.3d"
# define POSLIST_FILE "poslist.txt"
# define MESSAGE_FILE "messages.txt"

# define PCL_CFG  "pcl.cfg"
# define DM_CFG   "dm.cfg"
# define PS_CFG   "ps.cfg"
# define HPGL_CFG "hpgl.cfg"
# define PRINT_INI "print.ini"

# define EXT_SVX_DATA ".svx"
# define EXT_SVX_3D   ".3d"
# define EXT_SVX_ERRS ".err"
# define EXT_SVX_STAT ".inf"
# define EXT_SVX_POS  ".pos"

#elif (OS==UNIX)

# define ERRSTAT_FILE "errstats.txt"
# define STATS_FILE   "stats.txt"
# define IMAGE_FILE   "image.3d"
# define POSLIST_FILE "poslist.txt"
# define MESSAGE_FILE "messages.txt"

# define PCL_CFG  "pcl.cfg"
# define DM_CFG   "dm.cfg"
# define PS_CFG   "ps.cfg"
# define HPGL_CFG "hpgl.cfg"
# define PRINT_INI "print.ini"

# define EXT_SVX_DATA ".svx"
# define EXT_SVX_3D   ".3d"
# define EXT_SVX_ERRS ".err"
# define EXT_SVX_STAT ".inf"
# define EXT_SVX_POS  ".pos"

#elif (OS==AMIGA)

# define ERRSTAT_FILE "errstats.txt"
# define STATS_FILE   "stats.txt"
# define IMAGE_FILE   "image.3d"
# define POSLIST_FILE "poslist.txt"
# define MESSAGE_FILE "messages.txt"

# define PCL_CFG  "pcl.cfg"
# define DM_CFG   "dm.cfg"
# define PS_CFG   "ps.cfg"
# define HPGL_CFG "hpgl.cfg"
# define PRINT_INI "print.ini"

# define EXT_SVX_DATA ".svx"
# define EXT_SVX_3D   ".3d"
# define EXT_SVX_ERRS ".err"
# define EXT_SVX_STAT ".inf"
# define EXT_SVX_POS  ".pos"

#else
# error Do not know operating system 'OS'
#endif
