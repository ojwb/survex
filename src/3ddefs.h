/* 3ddefs.h
 * Symbolic constants for .3dx files
 * Copyright (C) 2000 Phil Underwood
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

/* Below are the "magic" numbers for Phil Underwood's ".3dx" file format
 * as used by Chasm */

#define STATION_3D 0x02
#define STATLINK_3D 0x08
#define STATNAMELINK_3D 0x10
#define TITLE_3D 0x20
#define PREF_COLOUR_3D 0x22
#define DATE_3D 0x24
#define DRAWINGS_3D 0x28
#define INSTRUMENTS_3D 0x2A
#define TAPE_3D 0x2C
#define SOURCE_3D 0x30
#define BASE_SOURCE_3D 0x32
#define BASE_FILE_3D 0x34
#define LEG_3D 0x82
#define BRANCH_3D 0x84
#define INVISIBLE_3D 0xe0
#define END_3D 0xfe
#define COMMIN_3D 0x00
#define COMMAX_3D 0x40
