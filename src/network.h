/* network.h
 * SURVEX Network reduction routines
 * Copyright (C) 1994,2001,2026 Olly Betts
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

void remove_subnets(void);
void replace_subnets(void);

/* Try to find a non-invented station in the same component as stn.
 *
 * This hunts through the stacked delta-star transforms to find the
 * one which created stn.  If it finds one it returns a real station
 * from amongst the 3 that were removed (assuming there is one).
 */
node *find_non_invented_stn(node *stn);
