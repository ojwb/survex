/* > cvrotimg.h
 * Header file for
 * Reads a .3d image file into two linked lists of blocks, suitable for use
 * by caverot.c
 * Copyright (C) 1994,1997 Olly Betts
 */

/*
1994.06.20 created
1997.06.03 filename is now const char *
*/

extern bool load_data( const char *fnmData, lid Huge **ppLegs, lid Huge **ppStns );
