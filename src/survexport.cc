/* survexport.cc
 * Convert a processed survey data file to another format.
 */

/* Copyright (C) 1994-2004,2008,2010,2011,2013,2014,2018,2020 Olly Betts
 * Copyright (C) 2004 John Pybus (SVG Output code)
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "export.h"
#include "mainfrm.h"

#include "cmdline.h"
#include "filename.h"
#include "img_hosted.h"
#include "message.h"
#include "str.h"
#include "useful.h"

#include <iostream>
#include <string>

using namespace std;

int
main(int argc, char **argv)
{
   double pan = 0;
   double tilt = -90.0;
   export_format format = FMT_MAX_PLUS_ONE_;
   int show_mask = 0;
   const char *survey = NULL;
   double grid = 0.0; /* grid spacing (or 0 for no grid) */
   double text_height = DEFAULT_TEXT_HEIGHT; /* for station labels */
   double marker_size = DEFAULT_MARKER_SIZE; /* for station markers */
   double scale = 500.0;
   SurveyFilter* filter = NULL;

   {
       /* Default to .pos output if installed as 3dtopos. */
       char* progname = baseleaf_from_fnm(argv[0]);
       if (strcasecmp(progname, "3dtopos") == 0) {
	   format = FMT_POS;
       }
       osfree(progname);
   }

   const int OPT_FMT_BASE = 20000;
   enum {
       OPT_SCALE = 0x100, OPT_BEARING, OPT_TILT, OPT_PLAN, OPT_ELEV,
       OPT_LEGS, OPT_SURF, OPT_SPLAYS, OPT_CROSSES, OPT_LABELS, OPT_ENTS,
       OPT_FIXES, OPT_EXPORTS, OPT_XSECT, OPT_WALLS, OPT_PASG,
       OPT_CENTRED, OPT_FULL_COORDS, OPT_CLAMP_TO_GROUND, OPT_DEFAULTS
   };
   static const struct option long_opts[] = {
	/* const char *name; int has_arg (0 no_argument, 1 required, 2 options_*); int *flag; int val */
	{"survey", required_argument, 0, 's'},
	{"scale", required_argument, 0, OPT_SCALE},
	{"bearing", required_argument, 0, OPT_BEARING},
	{"tilt", required_argument, 0, OPT_TILT},
	{"plan", no_argument, 0, OPT_PLAN},
	{"elevation", no_argument, 0, OPT_ELEV},
	{"legs", no_argument, 0, OPT_LEGS},
	{"surface-legs", no_argument, 0, OPT_SURF},
	{"splays", no_argument, 0, OPT_SPLAYS},
	{"crosses", no_argument, 0, OPT_CROSSES},
	{"station-names", no_argument, 0, OPT_LABELS},
	{"entrances", no_argument, 0, OPT_ENTS},
	{"fixes", no_argument, 0, OPT_FIXES},
	{"exports", no_argument, 0, OPT_EXPORTS},
	{"cross-sections", no_argument, 0, OPT_XSECT},
	{"walls", no_argument, 0, OPT_WALLS},
	{"passages", no_argument, 0, OPT_PASG},
	{"origin-in-centre", no_argument, 0, OPT_CENTRED},
	{"full-coordinates", no_argument, 0, OPT_FULL_COORDS},
	{"clamp-to-ground", no_argument, 0, OPT_CLAMP_TO_GROUND},
	{"defaults", no_argument, 0, OPT_DEFAULTS},
	{"grid", optional_argument, 0, 'g'},
	{"text-height", required_argument, 0, 't'},
	{"marker-size", required_argument, 0, 'm'},
	{"csv", no_argument, 0, OPT_FMT_BASE + FMT_CSV},
	{"dxf", no_argument, 0, OPT_FMT_BASE + FMT_DXF},
	{"eps", no_argument, 0, OPT_FMT_BASE + FMT_EPS},
	{"gpx", no_argument, 0, OPT_FMT_BASE + FMT_GPX},
	{"hpgl", no_argument, 0, OPT_FMT_BASE + FMT_HPGL},
	{"json", no_argument, 0, OPT_FMT_BASE + FMT_JSON},
	{"kml", no_argument, 0, OPT_FMT_BASE + FMT_KML},
	{"plt", no_argument, 0, OPT_FMT_BASE + FMT_PLT},
	{"skencil", no_argument, 0, OPT_FMT_BASE + FMT_SK},
	{"pos", no_argument, 0, OPT_FMT_BASE + FMT_POS},
	{"svg", no_argument, 0, OPT_FMT_BASE + FMT_SVG},
	{"help", no_argument, 0, HLP_HELP},
	{"version", no_argument, 0, HLP_VERSION},
	// US spelling:
	{"origin-in-center", no_argument, 0, OPT_CENTRED},
	// Abbreviation:
	{"full-coords", no_argument, 0, OPT_FULL_COORDS},
	{0,0,0,0}
   };

#define short_opts "s:g::t:m:"

   static struct help_msg help[] = {
	/*			<-- */
	{HLP_ENCODELONG(0),   /*only load the sub-survey with this prefix*/199, 0},
	/* TRANSLATORS: These example input values should not be translated. */
	{HLP_ENCODELONG(1),   /*scale (50, 0.02, 1:50 and 2:100 all mean 1:50)*/217, 0},
	/* TRANSLATORS: These example input values should not be translated. */
	{HLP_ENCODELONG(2),   /*bearing (90, 90d, 100g all mean 90°)*/460, 0},
	/* TRANSLATORS: These example input values should not be translated. */
	{HLP_ENCODELONG(3),   /*tilt (45, 45d, 50g, 100% all mean 45°)*/461, 0},
	/* TRANSLATORS: Don't translate example command line option --tilt=-90 */
	{HLP_ENCODELONG(4),   /*plan view (equivalent to --tilt=-90)*/462, 0},
	/* TRANSLATORS: Don't translate example command line option --tilt=0 */
	{HLP_ENCODELONG(5),   /*elevation view (equivalent to --tilt=0)*/463, 0},
	{HLP_ENCODELONG(6),   /*underground survey legs*/476, 0},
	{HLP_ENCODELONG(7),   /*surface survey legs*/464, 0},
	{HLP_ENCODELONG(8),   /*splay legs*/465, 0},
	{HLP_ENCODELONG(9),   /*station markers*/474, 0},
	{HLP_ENCODELONG(10),  /*station labels*/475, 0},
	{HLP_ENCODELONG(11),  /*entrances*/466, 0},
	{HLP_ENCODELONG(12),  /*fixed points*/467, 0},
	{HLP_ENCODELONG(13),  /*exported stations*/468, 0},
	{HLP_ENCODELONG(14),  /*cross-sections*/469, 0},
	{HLP_ENCODELONG(15),  /*walls*/470, 0},
	{HLP_ENCODELONG(16),  /*passages*/471, 0},
	{HLP_ENCODELONG(17),  /*origin in centre*/472, 0},
	{HLP_ENCODELONG(18),  /*full coordinates*/473, 0},
	{HLP_ENCODELONG(19),  /*clamp to ground*/478, 0},
	{HLP_ENCODELONG(20),  /*include items exported by default*/155, 0},
	{HLP_ENCODELONG(21),  /*generate grid (default %sm)*/148, STRING(DEFAULT_GRID_SPACING)},
	{HLP_ENCODELONG(22),  /*station labels text height (default %s)*/149, STRING(DEFAULT_TEXT_HEIGHT)},
	{HLP_ENCODELONG(23),  /*station marker size (default %s)*/152, STRING(DEFAULT_MARKER_SIZE)},
	{HLP_ENCODELONG(24),  /*produce CSV output*/102, 0},
	{HLP_ENCODELONG(25),  /*produce DXF output*/156, 0},
	{HLP_ENCODELONG(26),  /*produce EPS output*/454, 0},
	{HLP_ENCODELONG(27),  /*produce GPX output*/455, 0},
	{HLP_ENCODELONG(28),  /*produce HPGL output*/456, 0},
	{HLP_ENCODELONG(29),  /*produce JSON output*/457, 0},
	{HLP_ENCODELONG(30),  /*produce KML output*/458, 0},
	/* TRANSLATORS: "Compass" and "Carto" are the names of software packages,
	 * so should not be translated. */
	{HLP_ENCODELONG(31),  /*produce Compass PLT output for Carto*/159, 0},
	/* TRANSLATORS: "Skencil" is the name of a software package, so should not be
	 * translated. */
	{HLP_ENCODELONG(32),  /*produce Skencil output*/158, 0},
	{HLP_ENCODELONG(33),  /*produce Survex POS output*/459, 0},
	{HLP_ENCODELONG(34),  /*produce SVG output*/160, 0},
	{0, 0, 0}
   };

   msg_init(argv);

   string optmap[sizeof(show_mask) * CHAR_BIT];

   int long_index;
   bool always_include_defaults = false;
   cmdline_init(argc, argv, short_opts, long_opts, &long_index, help, 1, 2);
   while (1) {
      long_index = -1;
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      int bit = 0;
      switch (opt) {
       case OPT_LEGS:
	 bit = LEGS;
	 break;
       case OPT_SURF:
	 bit = SURF;
	 break;
       case OPT_SPLAYS:
	 bit = SPLAYS;
	 break;
       case OPT_CROSSES:
	 bit = STNS;
	 break;
       case OPT_LABELS:
	 bit = LABELS;
	 break;
       case OPT_ENTS:
	 bit = ENTS;
	 break;
       case OPT_FIXES:
	 bit = FIXES;
	 break;
       case OPT_EXPORTS:
	 bit = EXPORTS;
	 break;
       case OPT_XSECT:
	 bit = XSECT;
	 break;
       case OPT_WALLS:
	 bit = WALLS;
	 break;
       case OPT_PASG:
	 bit = PASG;
	 break;
       case OPT_CENTRED:
	 bit = CENTRED;
	 break;
       case OPT_FULL_COORDS:
	 bit = FULL_COORDS;
	 break;
       case OPT_CLAMP_TO_GROUND:
	 bit = CLAMP_TO_GROUND;
	 break;
       case OPT_DEFAULTS:
	 always_include_defaults = true;
	 break;
       case 'g': /* Grid */
	 if (optarg) {
	    grid = cmdline_double_arg();
	 } else {
	    grid = (double)DEFAULT_GRID_SPACING;
	 }
	 bit = GRID;
	 break;
       case OPT_SCALE: {
	 char* colon = strchr(optarg, ':');
	 if (!colon) {
	     /* --scale=1000 => 1:1000 => scale = 1000 */
	     scale = cmdline_double_arg();
	     if (scale < 1.0) {
		 /* --scale=0.001 => 1:1000 => scale = 1000 */
		 scale = 1.0 / scale;
	     }
	 } else if (colon - optarg == 1 && optarg[0] == '1') {
	     /* --scale=1:1000 => 1:1000 => scale = 1000 */
	     optarg += 2;
	     scale = cmdline_double_arg();
	     optarg -= 2;
	 } else {
	     /* --scale=2:1000 => 1:500 => scale = 500 */
	     *colon = '\0';
	     scale = cmdline_double_arg();
	     optarg = colon + 1;
	     scale = cmdline_double_arg() / scale;
	     *colon = ':';
	 }
	 bit = SCALE;
	 break;
       }
       case OPT_BEARING: {
	 int units = 0;
	 size_t len = strlen(optarg);
	 if (len > 0) {
	     char ch = optarg[len - 1];
	     switch (ch) {
		 case 'd':
		 case 'g':
		     units = ch;
		     optarg[len - 1] = '\0';
		     break;
	     }
	     pan = cmdline_double_arg();
	     optarg[len - 1] = ch;
	 } else {
	     pan = cmdline_double_arg();
	 }
	 if (units == 'g') {
	     pan *= 0.9;
	 }
	 bit = ORIENTABLE;
	 break;
       }
       case OPT_TILT: {
	 int units = 0;
	 size_t len = strlen(optarg);
	 if (len > 0) {
	     char ch = optarg[len - 1];
	     switch (ch) {
		 case '%':
		 case 'd':
		 case 'g':
		     units = ch;
		     optarg[len - 1] = '\0';
		     break;
	     }
	     tilt = cmdline_double_arg();
	     optarg[len - 1] = ch;
	 } else {
	     tilt = cmdline_double_arg();
	 }
	 if (units == 'g') {
	     tilt *= 0.9;
	 } else if (units == '%') {
	     tilt = deg(atan(tilt * 0.01));
	 }
	 bit = ORIENTABLE;
	 break;
       }
       case OPT_PLAN:
	 tilt = -90.0;
	 bit = ORIENTABLE;
	 break;
       case OPT_ELEV:
	 tilt = 0.0;
	 bit = ORIENTABLE;
	 break;
       case 't': /* Text height */
	 text_height = cmdline_double_arg();
	 bit = TEXT_HEIGHT;
	 break;
       case 'm': /* Marker size */
	 marker_size = cmdline_double_arg();
	 bit = MARKER_SIZE;
	 break;
       case 's':
	 if (survey) {
	     if (!filter) {
		 filter = new SurveyFilter();
		 filter->add(survey);
	     }
	     filter->add(optarg);
	 } else {
	     survey = optarg;
	 }
	 break;
       default:
	 if (opt >= OPT_FMT_BASE && opt < OPT_FMT_BASE + FMT_MAX_PLUS_ONE_) {
	     format = export_format(opt - OPT_FMT_BASE);
	 }
      }
      if (bit) {
	  show_mask |= bit;
	  int i = 0;
	  while (((bit >> i) & 1) == 0) ++i;

	  if (!optmap[i].empty()) optmap[i] += ' ';

	  // Reconstruct what the command line option was.
	  if (long_index < 0) {
	      optmap[i] += '-';
	      optmap[i] += char(opt);
	      if (optarg) {
		  if (optarg == argv[optind - 1]) {
		      optmap[i] += ' ';
		  }
		  optmap[i] += optarg;
	      }
	  } else {
	      optmap[i] += "--";
	      optmap[i] += long_opts[long_index].name;
	      if (optarg) {
		  if (optarg == argv[optind - 1]) {
		      optmap[i] += ' ';
		  } else {
		      optmap[i] += '=';
		  }
		  optmap[i] += optarg;
	      }
	  }
      }
   }

   // A single --survey is handled by img at load-time.  Multiple --survey are
   // handled via a SurveyFilter at export time.
   if (filter) survey = NULL;

   const char* fnm_in = argv[optind++];
   const char* fnm_out = argv[optind];
   if (fnm_out) {
      if (format == FMT_MAX_PLUS_ONE_) {
	 // Select format based on extension.
	 size_t len = strlen(fnm_out);
	 for (size_t i = 0; i < FMT_MAX_PLUS_ONE_; ++i) {
	    const auto& info = export_format_info[i];
	    size_t l = strlen(info.extension);
	    if (len > l + 1 &&
		strcasecmp(fnm_out + len - l, info.extension) == 0) {
	       format = export_format(i);
	       break;
	    }
	 }
	 if (format == FMT_MAX_PLUS_ONE_) {
	    fatalerror(/*Export format not specified and not known from output file extension*/252);
	 }
      }
   } else {
      if (format == FMT_MAX_PLUS_ONE_) {
	 fatalerror(/*Export format not specified*/253);
      }
      char *baseleaf = baseleaf_from_fnm(fnm_in);
      /* note : memory allocated by fnm_out gets leaked in this case... */
      fnm_out = add_ext(baseleaf, export_format_info[format].extension);
      osfree(baseleaf);
   }

   const auto& format_info_mask = export_format_info[format].mask;
   unsigned not_allowed = show_mask &~ format_info_mask;
   if (not_allowed) {
       printf("warning: The following options are not supported for this export format and will be ignored:\n");
       int i = 0;
       int bit = 1;
       while (not_allowed) {
	   if (not_allowed & bit) {
	       // E.g. --walls maps to two bits in show_mask, but the options
	       // are only put on the least significant in such cases.
	       if (!optmap[i].empty())
		   printf("%s\n", optmap[i].c_str());
	       not_allowed &= ~bit;
	   }
	   ++i;
	   bit <<= 1;
       }
       show_mask &= format_info_mask;
   }

   if (always_include_defaults || show_mask == 0) {
       show_mask |= export_format_info[format].defaults;
   }

   if (!(format_info_mask & ORIENTABLE)) {
       pan = 0.0;
       tilt = -90.0;
   }

   Model model;
   int err = model.Load(fnm_in, survey);
   if (err) fatalerror(err, fnm_in);
   if (filter) filter->SetSeparator(model.GetSeparator());

   try {
       if (!Export(fnm_out, model.GetSurveyTitle(),
		   model.GetDateString(),
		   model, filter,
		   pan, tilt, show_mask, format,
		   grid, text_height, marker_size,
		   scale)) {
	  fatalerror(/*Couldn’t write file “%s”*/402, fnm_out);
       }
   } catch (const wxString & m) {
       wxString r = msg_appname();
       r += ": ";
       r += wmsg(/*error*/93);
       r += ": ";
       r += m;
       wcerr << r.c_str() << '\n';
   }

   return 0;
}
