#include <string.h>

#include "osalloc.h"

/* append a string */
void
s_cat(char **pstr, int *plen, char *s)
{
   int new_len = strlen(s) + 1; /* extra 1 for nul */
   int len = 0;

   if (*pstr) {
      len = strlen(*pstr);
      new_len += len;
   }

   if (!*pstr || new_len > *plen) {
      *plen = (new_len + 32) & ~3;
      *pstr = osrealloc(*pstr, *plen);
   }
   
   strcpy(*pstr + len, s);
}

/* append a character */
void
s_catchar(char **pstr, int *plen, char ch)
{
   int new_len = 2; /* extra 1 for nul */
   int len = 0;

   if (*pstr) {
      len = strlen(*pstr);
      new_len += len;
   }

   if (!*pstr || new_len > *plen) {
      *plen = (new_len + 32) & ~3;
      *pstr = osrealloc(*pstr, *plen);
   }
   
   (*pstr)[len] = ch;
   (*pstr)[len + 1] = '\0';
}

/* truncate string to zero length */
void
s_zero(char **pstr)
{
   if (*pstr) **pstr = '\0';
}

void
s_free(char **pstr)
{
   if (*pstr) {
      osfree(*pstr);
      *pstr = NULL;
   }
}
