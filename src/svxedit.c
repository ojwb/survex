/* svxedit.c
 * Run svxedit.tcl from the same directory as this program
 * Copyright (C) 2002 Olly Betts
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>

int
main(int /* argc */, char **argv)
{
   DWORD len = 256;
   char *buf = NULL, *p;
   while (1) {
       DWORD got;
       buf = osrealloc(buf, len);
       got = GetModuleFileName(NULL, buf, len);
       if (got + 12 < len) break;
       len += len;
   }
   /* Strange Win32 nastiness - strip prefix "\\?\" if present */
   if (strncmp(buf, "\\\\?\\", 4) == 0) buf += 4;
   p = strrchr(buf, '\\');
   if (p) ++p; else p = buf;
   strcpy(p, "svxedit.tcl");
   ShellExecute(NULL, NULL, buf, argv[1], NULL, SW_SHOWNORMAL);
   return 0; 
}
