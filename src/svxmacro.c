/* > svxmacro.c
 * Function versions of various macros - possibly of user
 * for trying to make smaller executable
 * Copyright (C) 1993,1994,1996 Olly Betts
 */

/*
1993.05.02 split from "cavein.h"
1993.06.04 FALSE -> fFalse
1993.07.12 slight mod to unfixed_2_node()
1993.07.16 changed shape() to use a shifted constant rather than a table
1993.08.09 altered data structure (OLD_NODES compiles previous version)
1993.08.10 added EXPLICIT_FIXED_FLAG option to make data smaller & faster
1993.08.11 NEW_NODES & POS(stn,d)
           added stn=stn; to fix() to suppress warning from BC
1993.08.12 fettled
           changed to only define functions if macros *not* defined
           recoded ident_cpy
           deleted some dead code
1993.08.13 added fFixed macro to simplify code
           fettled
1993.08.15 removed ident_cpy (now defined in readval.c using memcpy)
1993.11.08 only NEW_NODES code left
1993.11.10 added isSign() and isData()
1993.12.09 type link -> linkfor
1994.04.18 removed var; fettled
1994.04.27 actually removed var!
1994.06.26 cs -> pcs
1994.09.08 byte -> uchar
1996.03.23 Translate now a pointer to one elt in
1996.04.15 SPECIAL_POINT -> SPECIAL_DECIMAL
*/

#include "cavein.h"
/* NB defines macro versions */

# define fFixed(S) ((S)->name->pos->fFixed)

#ifndef data_here
extern uchar (data_here)( linkfor *leg )
 { return leg->l.reverse & 0x80; }
#endif

#ifndef reverse_leg
extern uchar (reverse_leg)( linkfor *leg )
 { return leg->l.reverse & 0x03; }
#endif

#ifndef fixed
extern int (fixed)( node *stn )
# if EXPLICIT_FIXED_FLAG
 { return (int)fFixed(stn); }
# else
 { return (POS(stn,0)!=UNFIXED_VAL); }
# endif
#endif

#ifndef fix
extern void (fix)( node *stn )
# if EXPLICIT_FIXED_FLAG
 { fFixed(stn)=(char)fTrue; }
# else
 { stn=stn; /* POS(stn,0)=0.0; */ } /* to suppress warning */
# endif
#endif

#ifndef unfix
extern void (unfix)( node *stn )
# if EXPLICIT_FIXED_FLAG
 { fFixed(stn)=(char)fFalse; }
# else
 { POS(stn,0)=UNFIXED_VAL; }
# endif
#endif

#ifndef shape
extern int (shape)( node *stn )
 { return (stn->leg[0]!=NULL)+(stn->leg[1]!=NULL)+(stn->leg[2]!=NULL); }
#endif

#ifndef unfixed_2_node
extern bool (unfixed_2_node)( node *stn )
 { return ( (shape(stn)==2) && !fixed(stn) ); }
#endif

#ifndef remove_leg_from_station
extern void (remove_leg_from_station)( uchar leg , node *stn )
 { stn->leg[leg]=NULL; }
#endif

#ifndef isEol
extern int (isEol)(int c)
 { return pcs->Translate[c] & SPECIAL_EOL; }
#endif

#ifndef isBlank
extern int (isBlank)(int c)
 { return pcs->Translate[c] & SPECIAL_BLANK; }
#endif

#ifndef isKeywd
extern int (isKeywd)(int c)
 { return pcs->Translate[c] & SPECIAL_KEYWORD; }
#endif

#ifndef isComm
extern int (isComm)(int c)
 { return pcs->Translate[c] & SPECIAL_COMMENT; }
#endif

#ifndef isOmit
extern int (isOmit)(int c)
 { return pcs->Translate[c] & SPECIAL_OMIT; }
#endif

#ifndef isRoot
extern int (isRoot)(int c)
 { return pcs->Translate[c] & SPECIAL_ROOT; }
#endif

#ifndef isSep
extern int (isSep)(int c)
 { return pcs->Translate[c] & SPECIAL_SEPARATOR; }
#endif

#ifndef isNames
extern int (isNames)(int c)
 { return pcs->Translate[c] & SPECIAL_NAMES; }
#endif

#ifndef isDecimal
extern int (isDecimal)(int c)
 { return pcs->Translate[c] & SPECIAL_DECIMAL; }
#endif

#ifndef isMinus
extern int (isMinus)(int c)
 { return pcs->Translate[c] & SPECIAL_MINUS; }
#endif

#ifndef isPlus
extern int (isPlus)(int c)
 { return pcs->Translate[c] & SPECIAL_PLUS; }
#endif

#ifndef isSign
extern int (isSign)(int c)
 { return pcs->Translate[c] & (SPECIAL_PLUS|SPECIAL_MINUS); }
#endif

#ifndef isData
extern int (isData)(int c)
 { return pcs->Translate[c] & (SPECIAL_OMIT|SPECIAL_ROOT|SPECIAL_SEPARATOR|
  SPECIAL_NAMES|SPECIAL_DECIMAL|SPECIAL_PLUS|SPECIAL_MINUS); }
#endif
