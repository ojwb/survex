/* portable case insensitive string compare */
/* Copyright (C) Olly Betts 1994 */

#include <ctype.h>

/* What about top bit set chars? */
int strcasecmp( const char *s1, const char *s2 ) {
   register r, c1, c2;
   do {
      c1 = *s1++;
      c2 = *s2++;
      r = toupper(c1) - toupper(c2);
   } while (!r && (c1));
   return c1 - c2;
}
