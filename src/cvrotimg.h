/* > cvrotimg.h
 * Header file for
 * Reads a .3d image file into two linked lists of blocks, suitable for use
 * by caverot.c
 * Copyright (C) 1994,1997 Olly Betts
 */

extern bool load_data(const char *fnmData, lid Huge **ppLegs, lid Huge **ppStns);
