/* > out.h
 * Header file for output stuff
 * Copyright (C) Olly Betts 2000
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

#define out_current_action(SZ) if (fQuiet) NOP; else printf("\n%s...\n", (SZ))
/* unfortunately we'll have to remember to double bracket ... */
#define out_printf(X) do{printf X;putchar('\n');}while(0)
#define out_puts(SZ) puts((SZ))
#define out_set_percentage(P) printf("%d%%\r", (int)(P))
#define out_set_fnm(SZ) if (!fPercent) NOP; else printf("%s:\n", (SZ))
