/* > svxmacro.c
 * Function versions of various macros - possibly of use
 * for trying to make smaller executable
 * Copyright (C) 1993,1994,1996 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cavern.h"
/* NB defines macro versions */

#define fFixed(S) ((S)->name->pos->fFixed)

#ifndef data_here
extern uchar
data_here(linkfor *leg)
{
   return leg->l.reverse & FLAG_DATAHERE;
}
#endif

#ifndef reverse_leg_dirn
extern uchar
reverse_leg_dirn(linkfor *leg)
{
   return leg->l.reverse & 0x03;
}
#endif

#ifndef fixed
extern int
fixed(node *stn)
{
# if EXPLICIT_FIXED_FLAG
   return (int)fFixed(stn);
# else
   return (POS(stn, 0) != UNFIXED_VAL);
# endif
}
#endif

#ifndef fix
extern void
fix(node *stn)
{
# if EXPLICIT_FIXED_FLAG
   fFixed(stn) = (char)fTrue;
# else
   stn = stn; /* to suppress compiler warning */
   /* POS(stn, 0) = 0.0; */
# endif
}
#endif

#ifndef unfix
extern void
unfix(node *stn)
{
# if EXPLICIT_FIXED_FLAG
   fFixed(stn) = (char)fFalse;
# else
   POS(stn, 0) = UNFIXED_VAL;
# endif
}
#endif

#ifndef isEol
extern int
isEol(int c)
{
   return pcs->Translate[c] & SPECIAL_EOL;
}
#endif

#ifndef isBlank
extern int
isBlank(int c)
{
   return pcs->Translate[c] & SPECIAL_BLANK;
}
#endif

#ifndef isKeywd
extern int
isKeywd(int c)
{
   return pcs->Translate[c] & SPECIAL_KEYWORD;
}
#endif

#ifndef isComm
extern int
isComm(int c)
{
   return pcs->Translate[c] & SPECIAL_COMMENT;
}
#endif

#ifndef isOmit
extern int
isOmit(int c)
{
   return pcs->Translate[c] & SPECIAL_OMIT;
}
#endif

#ifndef isRoot
extern int
isRoot(int c)
{
   return pcs->Translate[c] & SPECIAL_ROOT;
}
#endif

#ifndef isSep
extern int
isSep(int c)
{
   return pcs->Translate[c] & SPECIAL_SEPARATOR;
}
#endif

#ifndef isNames
extern int
isNames(int c)
{
   return pcs->Translate[c] & SPECIAL_NAMES;
}
#endif

#ifndef isDecimal
extern int
isDecimal(int c)
{
   return pcs->Translate[c] & SPECIAL_DECIMAL;
}
#endif

#ifndef isMinus
extern int
isMinus(int c)
{
   return pcs->Translate[c] & SPECIAL_MINUS;
}
#endif

#ifndef isPlus
extern int
isPlus(int c)
{
   return pcs->Translate[c] & SPECIAL_PLUS;
}
#endif

#ifndef isSign
extern int
isSign(int c)
{
   return pcs->Translate[c] & (SPECIAL_PLUS|SPECIAL_MINUS);
}
#endif

#ifndef isData
extern int
isData(int c)
{
   return pcs->Translate[c] &
      (SPECIAL_OMIT | SPECIAL_ROOT | SPECIAL_SEPARATOR |
       SPECIAL_NAMES | SPECIAL_DECIMAL | SPECIAL_PLUS | SPECIAL_MINUS);
}
#endif
