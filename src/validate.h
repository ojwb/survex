/* > validate.h
 *
 * Header file for validate.c
 *   Copyright (C) 1994,1996 Olly Betts
 */

/*
1994.06.20 created
1994.08.14 survex.exe specific stuff moved here from debug.h
1996.04.04 NOP introduced
*/

#include "debug.h"
#include "survex.h"

extern bool (validate)( void );
extern void (dump_node)(node *stn);
extern void (dump_entire_network)( void );
extern void (dump_network)( void );

#if (VALIDATE==0)
# define validate() NOP
# define dump_node(S) NOP
#endif

#if (DUMP_NETWORK==0)
# define dump_network() NOP
# define dump_entire_network() NOP
#endif
