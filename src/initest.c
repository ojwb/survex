#include <stdio.h>
#include <stdlib.h>

char **ini_read( char *section, char **vars );
int ini_open( char *fnm );
void ini_close( void );

int main( int argc, char ** argv ) {
   char ** x;
   if (!ini_open( "testini" )) {
      printf("no file\n");
      exit(1);
   }

   x=ini_read( "survex", argv+1 );
   if (!x) {
      printf("no go\n");
      exit(1);
   }

   argv++;
   while (*argv) {
      printf("`%s' `%s'\n",*argv++,*x?*x:"NULL");
      x++;
   }
   return 0;
}
