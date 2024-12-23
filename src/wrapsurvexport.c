/* wrapsurvexport.c
 * Set OPENSSL_MODULES to .exe's directory and run real .exe
 *
 * Copyright (C) 2002,2010,2014,2024 Olly Betts
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

#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char **argv)
{
    const char *p = strrchr(argv[0], '\\');
    const char *e_val = argv[0];
    size_t e_len;
    size_t a_len;
    (void)argc;
    if (p) {
	e_len = p - argv[0];
	a_len = e_len + 1;
    } else {
	e_val = ".";
	e_len = 1;
	a_len = 0;
    }
    char *e = malloc(e_len + strlen("OPENSSL_MODULES=") + 1);
    if (!e) return 1;
    strcpy(e, "OPENSSL_MODULES=");
    memcpy(e + strlen("OPENSSL_MODULES="), e_val, e_len);
    e[strlen("OPENSSL_MODULES=") + e_len] = '\0';
    putenv(e);
    char *a = malloc(a_len + strlen("survexpor_.exe") + 1);
    if (!a) return 1;
    memcpy(a, argv[0], a_len);
    strcpy(a + a_len, "survexpor_.exe");
    const char *real_argv0 = argv[0];
    // Behind the scenes it appears Microsoft's _execv() actually takes the
    // argv passed and crudely glues it together into a command line string
    // with spaces in between but *WITHOUT ANY ESCAPING*, and then it gets
    // split back up into arguments at spaces, so an argument containing a
    // space gets split into two arguments.  Coupled with the default
    // installation directory path containing a space (C:\Program Files) this
    // doesn't work out well.  Words fail me.
    //
    // Apparently putting quotes around the argument is necessary.
    for (int i = 0; i < argc; ++i) {
	const char *arg = argv[i];
	if (arg[strcspn(arg, " \t\n\r\v")]) {
	    // Argument contains whitespace.
	    char *newarg = malloc(strlen(arg) + 3);
	    if (!newarg) return 1;
	    newarg[0] = '"';
	    strcpy(newarg + 1, arg);
	    strcat(newarg + 1, "\"");
	    argv[i] = newarg;
	}
    }
    _execv(a, (const char * const*)argv);
    printf("%s: %s\n", real_argv0, strerror(errno));
    return 1;
}
