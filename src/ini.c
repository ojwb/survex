/* > ini.c
 * .ini file routines
 * Copyright (C) 1995 Olly Betts
 */

/*
pre-1995.06.23 written
1995.06.23 created ini.h and tidied for inclusion in Survex
           merged in inihier.c stuff
1995.06.26 strcmpi just calls strcmp for now
1995.10.06 added some debug code
1996.03.24 now use caseless compare as intended
1996.04.01 strcmpi -> stricmp
1998.03.21 stricmp -> strcasecmp
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ini.h"
#include "filename.h"
#include "message.h"
#include "debug.h"
#include "useful.h"

/* for testing separately from Survex */
#ifdef TEST_ALONE
/* says when two distinct strings hash to same value */
#define REPORT_COLLISION

/* !HACK!-ish - good enough for testing */
#define strcasecmp(x,y) strcmp((x),(y))

char *osstrdup(char *s) {
   char *t;
   t=malloc(strlen(s)+1);
   if (!t) {
      printf("out of memory\n");
      exit(1);
   }
   strcpy(t,s);
   return t;
}
#include <assert.h>
#define ASSERT(M) assert(M)
#endif

/* some (preferably prime) number for the hashing function */
#define HASH_PRIME 29363

static int hash_string( char *p ) {
   int hash;
   ASSERT( p!=NULL ); /* can't hash NULL */
/*   printf("HASH `%s' to ",p); */
   for( hash=0; *p ; p++ )
      hash = (hash * HASH_PRIME + tolower(*(unsigned char*)p)) & 0x7fff;
/*   printf("%d\n",hash); */
   return hash;
}

/*
void ini_write( const char *section, const char *var, const char *value ) {

   scan_for( section, var );

   static char *last_section=NULL;
   if (!last_section || strcasecmp(last_section,section)!=0) {
      if (last_section) {
	 fputc('\n',stdout);
      }
      printf("[%s]\n",section);
      last_section=section;
   }
   printf("%s=%s\n",var,value);
}
*/

char **ini_read( FILE *fh, char *section, char **vars ) {
   int n, c;
   char **vals;
   int *hash_tab, hash;
   char *p, *var, *val;
   char buf[1024];
   int fInSection;

   ASSERT( fh!=NULL );
   ASSERT( section!=NULL );
   ASSERT( vars!=NULL );

   /* count how many variables to look up */
   for( n=0 ; vars[n] ; )
      n++;
   hash_tab = malloc(n*sizeof(int));
   vals = malloc(n*sizeof(char*));
   if (!hash_tab || !vals) {
      free(hash_tab);
      return NULL;
   }

   /* calculate hashes (to save on strcmp-s) */
   for( c=0 ; vars[c] ; c++ ) {
      hash_tab[c]=hash_string(vars[c]);
      vals[c]=NULL;
   }

   ASSERT( c==n ); /* counted vars[] twice and got different answers! */

   fInSection=0;
   rewind(fh);
   for(;;) {
      if (feof(fh)) {
         /* if (fWrite) { insert_section(); insert_lines(); fDone=1; } */
         break;
      }

      /* read line and sort out terminator */
      if (!fgets(buf,1024,fh)) {
	 break;
      }
      if ( (p=strpbrk(buf,"\n\r")) != NULL )
         *p='\0';

      /* skip blank lines */
      if (buf[0]=='\0')
      	 continue;

      /* deal with a section heading */
      if (buf[0]=='[' && buf[strlen(buf)-1]==']') {
	 buf[strlen(buf)-1]='\0';
         if (fInSection) {
	    /* if (fWrite) { insert_lines(); fDone=1; } */
	    break; /* now leaving the section we wanted, so stop */
	 }
/*         printf("[%s] [%s]\n",section,buf+1);*/
	 if (!strcasecmp(section,buf+1))
            fInSection=1;
         continue;
      }

      if (!fInSection)
         continue;

      p=strchr(buf,'=');
      if (!p) {
         continue; /* non-blank line with no = sign! */
      }
      *p='\0';
      var=buf;
      val=p+1;

      /* hash the variable name and see if it's in the list passed in */
      hash=hash_string(var);
      for( c=n-1 ; c>=0 ; c-- ) {
         if (hash==hash_tab[c]) {
            if (strcasecmp(var,vars[c])==0) {
               /* if (fWrite) { replace_line(); hash_tab[c]=-1; } else */
               vals[c]=osstrdup(val);
            } else {
#ifdef REPORT_COLLISION
               printf("`%s' and `%s' both hash to %d!\n",var,vars[c],hash);
#endif
            }
         }
      }
   }

   free(hash_tab);
   return vals;
}

char **ini_read_hier( FILE *fh, char *section, char **vars ) {
   int i, j;
   char **x;
   char **vals=NULL;
   char *old_section;
   int first=1;
   int *to=NULL;

   ASSERT( fh!=NULL );
   ASSERT( section!=NULL );
   ASSERT( vars!=NULL );

   do {
      x=ini_read( fh, section, vars );
      if (!x) {
         if (!first) {
            free(vals);
            free(section);
         }
         return NULL;
      }

/*{int i; printf("[%s]\n",section);
for(i=0;vars[i];i++)printf("%d:%s\"%s\"\n",i,vars[i],x[i]?x[i]:"(null)");}*/

      if (first) {
         char **p=vars;
         int n;

         for( n=0 ; p[n] ; )
            n++;
         n++; /* include NULL */
         if ( (vars=malloc(n*sizeof(char*))) == NULL) {
            free(x);
            return NULL;
         }
         memcpy(vars,p,n*sizeof(char*));
         n--; /* to doesn't need NULL */
         if ( (to=malloc(n*sizeof(int))) == NULL ) {
            free(vars);
            free(x);
            return NULL;
         }
         while (n) {
            n--;
            to[n]=n;
         }

         vals=x;
      }

      for( i=1, j=1 ; vars[i] ; i++ ) {
/*         printf("%s: %s %d\n",vars[i],x[i]?x[i]:"(null)",to[i]);*/
         if (x[i]) {
            vals[to[i]]=x[i];
         } else {
            vars[j]=vars[i];
            to[j]=to[i];
            j++;
         }
      }
      vars[j]=NULL;

      old_section=section;
      section=x[0];

      if (!first) {
         free(x);
         free(old_section);
      }
      first=0;

   } while (section && j>1);

   free(section);
   free(vars);
   free(to);

   return vals;
}
