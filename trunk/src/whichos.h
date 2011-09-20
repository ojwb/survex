/* whichos.h  Detect which OS we're compiling for.
 * Copyright (C) 2005 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef SURVEX_WHICHOS_H
#define SURVEX_WHICHOS_H

/* Attempt to auto-detect OS. */
#if (defined(unix) || defined(UNIX))
# define OS_UNIX 1
#elif defined(__GNUC__) && defined(__APPLE_CC__)
/* MacOS X is Unix for most purposes. */
# define OS_UNIX 1
# define OS_UNIX_MACOSX 1
#endif

#if !OS_UNIX
# if defined WIN32 || defined _WIN32 || defined __WIN32 || defined __WIN32__
#  define OS_WIN32 1
# endif
#endif

#ifndef OS_UNIX
# define OS_UNIX 0
#endif

#ifndef OS_UNIX_MACOSX
# define OS_UNIX_MACOSX 0
#endif

#ifndef OS_WIN32
# define OS_WIN32 0
#endif

/* Check that we detected an OS! */
# if !(OS_UNIX+OS_WIN32)
#  error Failed to detect which os to compile for
# endif

#endif
