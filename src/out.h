/* Copyright (C) Olly Betts
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define out_current_action(SZ) printf("\n%s...\n", (SZ))
/* unfortunately we'll have to remember to double bracket ... */
#define out_printf(X) do{printf X;putchar('\n');}while(0)
#define out_puts(SZ) puts((SZ))
#define out_set_percentage(P) printf("%d%%\r", (int)(P))
#define out_set_fnm(SZ) if (!fPercent) NOP; else printf("%s:\n", (SZ))
