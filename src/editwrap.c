/* editwrap.c
 * Run svxedit.tcl from the same directory as this program
 * Copyright (C) 2002,2010,2014 Olly Betts
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <windows.h>

int APIENTRY
WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
   DWORD len = 256;
   wchar_t *buf = NULL, *p;
   PWSTR cmd_line;
   (void)hInst; /* suppress compiler warning */
   (void)hPrevInst; /* suppress compiler warning */
   (void)lpCmdLine; /* suppress compiler warning */
   while (1) {
       DWORD got;
       buf = realloc(buf, len * 2);
       if (!buf) exit(1);
       got = GetModuleFileNameW(NULL, buf, len);
       if (got + 12 < len) break;
       len += len;
   }
   /* Strange Win32 nastiness - strip prefix "\\?\" if present */
   if (wcsncmp(buf, L"\\\\?\\", 4) == 0) buf += 4;
   p = wcsrchr(buf, L'\\');
   if (p) ++p; else p = buf;
   wcscpy(p, L"svxedit.tcl");
   cmd_line = GetCommandLineW();
   /* ShellExecute returns an HINSTANCE for some wacko reason - the docs say
    * the only valid operation is to convert it to an int.  Marvellous. */
   if ((int)ShellExecuteW(NULL, NULL, buf, cmd_line, NULL, nCmdShow) <= 32) {
       ShellExecuteW(NULL, NULL, L"notepad", cmd_line, NULL, nCmdShow);
   }
   return 0; 
}
