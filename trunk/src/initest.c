/* Copyright (C) Olly Betts 1999
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
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
