/* netskel.h
 * SURVEX Network reduction routines
 * Copyright (C) 1994,2001,2024 Olly Betts
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

void solve_network(void);

/* Try to find a non-anonymous station which was attached to stn.
 *
 * If stn is an anonymous station, this hunts through the removed trailing
 * traverse list, and if it finds an entry, it returns the first station
 * along it (which should be non-anonymous but we don't bother to check
 * currently).
 */
node *find_non_anon_stn(node *stn);
