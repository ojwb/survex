/* portable case insensitive string compare */
/* Copyright (C) Olly Betts 1994,1999 */

#include <ctype.h>

/* What about top bit set chars? */
int strcasecmp(const char *s1, const char *s2) {
   register c1, c2;
   do {
      c1 = *s1++;
      c2 = *s2++;
   } while (c1 && toupper(c1) == toupper(c2));
   /* now calculate real difference */
   return c1 - c2;
}
