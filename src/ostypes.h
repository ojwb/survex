/* > ostypes.h
 * Generally useful type definitions
 * Copyright (C) 1996 Olly Betts
 */

#ifndef OSTYPES_H /* only include once */
#define OSTYPES_H

#ifdef __cplusplus
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
