/* editwrap_osx.c
 * Run wish on a script with "_wrap" suffix removed from the name
 * Copyright (C) 2015 Olly Betts
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

#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <stdio.h>

int
main(int argc, char ** argv)
{
    char * script = strdup(argv[0]);
    size_t len = strlen(script);
    if (!script) return EX_OSERR;
    if (argc > 1 || len <= 5 || strcmp(script + (len - 5), "_wrap") != 0)
	return EX_USAGE;
    script[len - 5] = '\0';
    execlp("/usr/bin/wish", "svxedit", script, (char*)0);
}
