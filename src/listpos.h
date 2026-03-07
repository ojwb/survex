/* listpos.h
 * SURVEX Cave surveying software: stuff to do with stn position output
 * Copyright (C) 1994,2001,2012,2025 Olly Betts
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

/* Check but don't print, for getting the checks we do during the scan. */
void check_node_stats(void);

/* Scan the prefix tree and issue warnings for any stations with SFLAGS_FIXED
 * set but SFLAGS_USED not.
 */
void check_for_unused_fixed_points(void);
