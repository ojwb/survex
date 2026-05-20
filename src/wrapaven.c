/* wrapaven.c
 * Set OPENSSL_MODULES to .exe's directory and run real .exe
 *
 * Copyright (C) 2002,2010,2014,2024,2025,2026 Olly Betts
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
 * along with this program; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <windows.h>

int
main(int argc, char **argv)
{
   DWORD len = 256;
   wchar_t *buf = NULL;
   (void)argc; /* suppress compiler warning */
   (void)argv; /* suppress compiler warning */
   while (1) {
       DWORD got;
       buf = realloc(buf, len * sizeof(wchar_t));
       if (!buf) return 1;
       got = GetModuleFileNameW(NULL, buf, len);
       if (got < len) {
	   /* Strange Microsoft nastiness - skip prefix "\\?\" if present. */
	   if (wcsncmp(buf, L"\\\\?\\", 4) == 0) {
	       buf += 4;
	       got -= 4;
	   }
	   wchar_t *p = wcsrchr(buf, L'\\');
	   const wchar_t *e_val = buf;
	   size_t e_len;
	   if (p) {
	       e_len = p - buf;
	   } else {
	       e_val = L".";
	       e_len = 1;
	   }
	   wchar_t *e = malloc((e_len + strlen("OPENSSL_MODULES=") + 1) * sizeof(wchar_t));
	   if (!e) return 1;
	   wcscpy(e, L"OPENSSL_MODULES=");
	   memcpy(e + strlen("OPENSSL_MODULES="), e_val, e_len * sizeof(wchar_t));
	   e[strlen("OPENSSL_MODULES=") + e_len] = L'\0';
	   _wputenv(e);
	   buf[got - 5] = L'_';
	   break;
       }
       len += len;
   }

   PROCESS_INFORMATION process_info = {};
   STARTUPINFOW startup_info;
   GetStartupInfoW(&startup_info);
   if (CreateProcessW(buf, GetCommandLineW(), NULL, NULL, TRUE, 0,
		      NULL, NULL, &startup_info, &process_info) == 0) {
       return 1;
   }
   return 0; 
}
