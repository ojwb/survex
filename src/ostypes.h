/* > ostypes.h
 * Generally useful type definitions
 * Copyright (C) 1996 Olly Betts
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

#ifndef OSTYPES_H /* only include once */
#define OSTYPES_H

#ifndef __cplusplus
typedef enum { fFalse=0, fTrue=1 } BOOL;
# define bool BOOL
#else
#define fFalse false
#define fTrue true
#endif
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned int UINT;
# define uchar UCHAR
# define ulong ULONG
# define uint UINT

#endif
