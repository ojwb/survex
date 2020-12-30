/* imgtest.c */
/* Test img in unhosted mode */
/* Copyright (C) 2014,2020 Olly Betts
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include "img.h"

int
main(int argc, char **argv)
{
    char *fnm;
    char *survey;
    img *pimg;
    unsigned long c_stations = 0;
    unsigned long c_legs = 0;

    if (argc < 2 || argc > 3) {
	fprintf(stderr, "Syntax: %s 3DFILE [SURVEY]\n", argv[0]);
	return 1;
    }

    fnm = argv[1];
    survey = argv[2];

    if (survey) {
	pimg = img_open_survey(fnm, survey);
    } else {
	pimg = img_open(fnm);
    }
    if (!pimg) {
	fprintf(stderr, "%s: Failed to open '%s' (error code %d)\n",
		argv[0], fnm, (int)img_error());
	return 1;
    }

    printf("Title: \"%s\"\n", pimg->title);
    printf("Date: \"%s\"\n", pimg->datestamp);
    printf("Format-Version: %d\n", pimg->version);
    printf("Extended-Elevation: %s\n",
	   pimg->is_extended_elevation ? "yes" : "no");
    while (1) {
	img_point pt;
	int code = img_read_item(pimg, &pt);
	if (code == img_STOP) break;
	switch (code) {
	    case img_LINE:
		c_legs++;
		break;
	    case img_LABEL:
		c_stations++;
		break;
	    case img_BAD:
		img_close(pimg);
		fprintf(stderr, "%s: img_read_item failed (error code %d)\n",
			argv[0], (int)img_error());
		return 1;
	}
    }

    printf("Stations: %lu\nLegs: %lu\n", c_stations, c_legs);

    img_close(pimg);

    return 0;
}
