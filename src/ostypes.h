/* > ostypes.h
 * Generally useful type definitions
 * Copyright (C) 1996 Olly Betts
 */

/*
1996.02.19 split off from useful.h
*/

#ifndef OSTYPES_H /* only include once */
#define OSTYPES_H

typedef enum { fFalse=0, fTrue=1 } BOOL;
# define bool BOOL
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned int UINT;
# define uchar UCHAR
# define ulong ULONG
# define uint UINT
typedef char* sz; /* (S)tring, (Z)ero terminated */

#endif
