/* readval.c
 * Routines to read a prefix or number from the current input file
 * Copyright (C) 1991-2024 Olly Betts
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

#include <config.h>

#include <limits.h>
#include <stddef.h> /* for offsetof */

#include "cavern.h"
#include "commands.h" /* For match_tok(), etc */
#include "date.h"
#include "debug.h"
#include "filename.h"
#include "message.h"
#include "readval.h"
#include "datain.h"
#include "netbits.h"
#include "osalloc.h"
#include "str.h"

int root_depr_count = 0;

static prefix *
new_anon_station(void)
{
    prefix *name = osnew(prefix);
    name->pos = NULL;
    name->ident.p = NULL;
    name->stn = NULL;
    name->up = pcs->Prefix;
    name->down = NULL;
    name->filename = file.filename;
    name->line = file.line;
    name->min_export = name->max_export = 0;
    name->sflags = BIT(SFLAGS_ANON);
    /* Keep linked list of anon stations for node stats. */
    name->right = anon_list;
    anon_list = name;
    return name;
}

static char *id = NULL;
static size_t id_len = 0;

/* if prefix is omitted: if PFX_OPT set return NULL, otherwise use longjmp */
extern prefix *
read_prefix(unsigned pfx_flags)
{
   bool f_optional = !!(pfx_flags & PFX_OPT);
   bool fSurvey = !!(pfx_flags & PFX_SURVEY);
   bool fSuspectTypo = !!(pfx_flags & PFX_SUSPECT_TYPO);
   prefix *back_ptr, *ptr;
   size_t i;
   bool fNew;
   bool fImplicitPrefix = true;
   int depth = -1;
   filepos here;
   filepos fp_firstsep;

   skipblanks();
   get_pos(&here);
#ifndef NO_DEPRECATED
   if (isRoot(ch)) {
      if (!(pfx_flags & PFX_ALLOW_ROOT)) {
	 compile_diagnostic(DIAG_ERR|DIAG_COL, /*ROOT is deprecated*/25);
	 longjmp(jbSkipLine, 1);
      }
      if (root_depr_count < 5) {
	 compile_diagnostic(DIAG_WARN|DIAG_COL, /*ROOT is deprecated*/25);
	 if (++root_depr_count == 5)
	    compile_diagnostic(DIAG_INFO, /*Further uses of this deprecated feature will not be reported*/95);
      }
      nextch();
      ptr = root;
      if (!isNames(ch)) {
	 if (!isSep(ch)) return ptr;
	 /* Allow optional SEPARATOR after ROOT */
	 get_pos(&fp_firstsep);
	 nextch();
      }
      fImplicitPrefix = false;
#else
   if (0) {
#endif
   } else {
      if ((pfx_flags & PFX_ANON) &&
	  (isSep(ch) || (pcs->dash_for_anon_wall_station && ch == '-'))) {
	 int first_ch = ch;
	 nextch();
	 if (isBlank(ch) || isComm(ch) || isEol(ch)) {
	    if (!isSep(first_ch))
	       goto anon_wall_station;
	    /* A single separator alone ('.' by default) is an anonymous
	     * station which is on a point inside the passage and implies
	     * the leg to it is a splay.
	     */
	    if (TSTBIT(pcs->flags, FLAGS_ANON_ONE_END)) {
	       set_pos(&here);
	       compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Can't have a leg between two anonymous stations*/3);
	       longjmp(jbSkipLine, 1);
	    }
	    pcs->flags |= BIT(FLAGS_ANON_ONE_END) | BIT(FLAGS_IMPLICIT_SPLAY);
	    return new_anon_station();
	 }
	 if (isSep(first_ch) && ch == first_ch) {
	    nextch();
	    if (isBlank(ch) || isComm(ch) || isEol(ch)) {
	       /* A double separator ('..' by default) is an anonymous station
		* which is on the wall and implies the leg to it is a splay.
		*/
	       prefix * pfx;
anon_wall_station:
	       if (TSTBIT(pcs->flags, FLAGS_ANON_ONE_END)) {
		  set_pos(&here);
		  compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Can't have a leg between two anonymous stations*/3);
		  longjmp(jbSkipLine, 1);
	       }
	       pcs->flags |= BIT(FLAGS_ANON_ONE_END) | BIT(FLAGS_IMPLICIT_SPLAY);
	       pfx = new_anon_station();
	       pfx->sflags |= BIT(SFLAGS_WALL);
	       return pfx;
	    }
	    if (ch == first_ch) {
	       nextch();
	       if (isBlank(ch) || isComm(ch) || isEol(ch)) {
		  /* A triple separator ('...' by default) is an anonymous
		   * station, but otherwise not handled specially (e.g. for
		   * a single leg down an unexplored side passage to a station
		   * which isn't refindable).
		   */
		  if (TSTBIT(pcs->flags, FLAGS_ANON_ONE_END)) {
		     set_pos(&here);
		     compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Can't have a leg between two anonymous stations*/3);
		     longjmp(jbSkipLine, 1);
		  }
		  pcs->flags |= BIT(FLAGS_ANON_ONE_END);
		  return new_anon_station();
	       }
	    }
	 }
	 set_pos(&here);
      }
      ptr = pcs->Prefix;
   }

   i = 0;
   do {
      fNew = false;
      if (id == NULL) {
	 /* Allocate buffer the first time */
	 id_len = 256;
	 id = osmalloc(id_len);
      }
      /* i==0 iff this is the first pass */
      if (i) {
	 i = 0;
	 nextch();
      }
      while (isNames(ch)) {
	 if (i < pcs->Truncate) {
	    /* truncate name */
	    id[i++] = (pcs->Case == LOWER ? tolower(ch) :
		       (pcs->Case == OFF ? ch : toupper(ch)));
	    if (i >= id_len) {
	       id_len *= 2;
	       id = osrealloc(id, id_len);
	    }
	 }
	 nextch();
      }
      if (isSep(ch)) {
	 fImplicitPrefix = false;
	 get_pos(&fp_firstsep);
      }
      if (i == 0) {
	 if (!f_optional) {
	    if (isEol(ch)) {
	       if (fSurvey) {
		  compile_diagnostic(DIAG_ERR|DIAG_COL, /*Expecting survey name*/89);
	       } else {
		  compile_diagnostic(DIAG_ERR|DIAG_COL, /*Expecting station name*/28);
	       }
	    } else {
	       /* TRANSLATORS: Here "station" is a survey station, not a train station. */
	       compile_diagnostic(DIAG_ERR|DIAG_COL, /*Character “%c” not allowed in station name (use *SET NAMES to set allowed characters)*/7, ch);
	    }
	    longjmp(jbSkipLine, 1);
	 }
	 return (prefix *)NULL;
      }

      id[i++] = '\0';

      back_ptr = ptr;
      ptr = ptr->down;
      if (ptr == NULL) {
	 /* Special case first time around at each level */
	 ptr = osnew(prefix);
	 ptr->sflags = BIT(SFLAGS_SURVEY);
	 if (i <= sizeof(ptr->ident.i)) {
	     memcpy(ptr->ident.i, id, i);
	     ptr->sflags |= BIT(SFLAGS_IDENT_INLINE);
	 } else {
	     char *new_id = osmalloc(i);
	     memcpy(new_id, id, i);
	     ptr->ident.p = new_id;
	 }
	 ptr->right = ptr->down = NULL;
	 ptr->pos = NULL;
	 ptr->stn = NULL;
	 ptr->up = back_ptr;
	 ptr->filename = file.filename;
	 ptr->line = file.line;
	 ptr->min_export = ptr->max_export = 0;
	 if (fSuspectTypo && !fImplicitPrefix)
	    ptr->sflags |= BIT(SFLAGS_SUSPECTTYPO);
	 back_ptr->down = ptr;
	 fNew = true;
      } else {
	 /* Use caching to speed up adding an increasing sequence to a
	  * large survey */
	 static prefix *cached_survey = NULL, *cached_station = NULL;
	 prefix *ptrPrev = NULL;
	 int cmp = 1; /* result of strcmp ( -ve for <, 0 for =, +ve for > ) */
	 if (cached_survey == back_ptr) {
	    cmp = strcmp(prefix_ident(cached_station), id);
	    if (cmp <= 0) ptr = cached_station;
	 }
	 while (ptr && (cmp = strcmp(prefix_ident(ptr), id)) < 0) {
	    ptrPrev = ptr;
	    ptr = ptr->right;
	 }
	 if (cmp) {
	    /* ie we got to one that was higher, or the end */
	    prefix *newptr = osnew(prefix);
	    newptr->sflags = BIT(SFLAGS_SURVEY);
	    if (strlen(id) < sizeof(newptr->ident.i)) {
		memcpy(newptr->ident.i, id, i);
		newptr->sflags |= BIT(SFLAGS_IDENT_INLINE);
	    } else {
		char *new_id = osmalloc(i);
		memcpy(new_id, id, i);
		newptr->ident.p = new_id;
	    }
	    if (ptrPrev == NULL)
	       back_ptr->down = newptr;
	    else
	       ptrPrev->right = newptr;
	    newptr->right = ptr;
	    newptr->down = NULL;
	    newptr->pos = NULL;
	    newptr->stn = NULL;
	    newptr->up = back_ptr;
	    newptr->filename = file.filename;
	    newptr->line = file.line;
	    newptr->min_export = newptr->max_export = 0;
	    if (fSuspectTypo && !fImplicitPrefix)
	       newptr->sflags |= BIT(SFLAGS_SUSPECTTYPO);
	    ptr = newptr;
	    fNew = true;
	 }
	 cached_survey = back_ptr;
	 cached_station = ptr;
      }
      depth++;
      f_optional = false; /* disallow after first level */
      if (isSep(ch)) {
	 get_pos(&fp_firstsep);
	 if (!TSTBIT(ptr->sflags, SFLAGS_SURVEY)) {
	    /* TRANSLATORS: Here "station" is a survey station, not a train station.
	     *
	     * Here "survey" is a "cave map" rather than list of questions - it should be
	     * translated to the terminology that cavers using the language would use.
	     */
	    compile_diagnostic(DIAG_ERR|DIAG_FROM(here), /*“%s” can’t be both a station and a survey*/27,
			       sprint_prefix(ptr));
	 }
      }
   } while (isSep(ch));

   /* don't warn about a station that is referred to twice */
   if (!fNew) ptr->sflags &= ~BIT(SFLAGS_SUSPECTTYPO);

   if (fNew) {
      /* fNew means SFLAGS_SURVEY is currently set */
      SVX_ASSERT(TSTBIT(ptr->sflags, SFLAGS_SURVEY));
      if (!fSurvey) {
	 ptr->sflags &= ~BIT(SFLAGS_SURVEY);
	 if (TSTBIT(pcs->infer, INFER_EXPORTS)) ptr->min_export = USHRT_MAX;
      }
   } else {
      /* check that the same name isn't being used for a survey and station */
      if (fSurvey ^ TSTBIT(ptr->sflags, SFLAGS_SURVEY)) {
	 /* TRANSLATORS: Here "station" is a survey station, not a train station.
	  *
	  * Here "survey" is a "cave map" rather than list of questions - it should be
	  * translated to the terminology that cavers using the language would use.
	  */
	 compile_diagnostic(DIAG_ERR|DIAG_FROM(here), /*“%s” can’t be both a station and a survey*/27,
			    sprint_prefix(ptr));
      }
      if (!fSurvey && TSTBIT(pcs->infer, INFER_EXPORTS)) ptr->min_export = USHRT_MAX;
   }

   /* check the export level */
#if 0
   printf("R min %d max %d depth %d pfx %s\n",
	  ptr->min_export, ptr->max_export, depth, sprint_prefix(ptr));
#endif
   if (ptr->min_export == 0 || ptr->min_export == USHRT_MAX) {
      if (depth > ptr->max_export) ptr->max_export = depth;
   } else if (ptr->max_export < depth) {
      prefix *survey = ptr;
      char *s;
      const char *p;
      int level;
      for (level = ptr->max_export + 1; level; level--) {
	 survey = survey->up;
	 SVX_ASSERT(survey);
      }
      s = osstrdup(sprint_prefix(survey));
      p = sprint_prefix(ptr);
      if (survey->filename) {
	 compile_diagnostic_pfx(DIAG_ERR, survey,
				/*Station “%s” not exported from survey “%s”*/26,
				p, s);
      } else {
	 compile_diagnostic(DIAG_ERR, /*Station “%s” not exported from survey “%s”*/26, p, s);
      }
      osfree(s);
#if 0
      printf(" *** pfx %s warning not exported enough depth %d "
	     "ptr->max_export %d\n", sprint_prefix(ptr),
	     depth, ptr->max_export);
#endif
   }
   if (!fImplicitPrefix && (pfx_flags & PFX_WARN_SEPARATOR)) {
      filepos fp_tmp;
      get_pos(&fp_tmp);
      set_pos(&fp_firstsep);
      compile_diagnostic(DIAG_WARN|DIAG_COL, /*Separator in survey name*/392);
      set_pos(&fp_tmp);
   }
   return ptr;
}

char *
read_walls_prefix(void)
{
    string name = S_INIT;
    skipblanks();
    if (!isNames(ch))
	return NULL;
    do {
	s_appendch(&name, ch);
	nextch();
    } while (isNames(ch));
    return s_steal(&name);
}

prefix *
read_walls_station(char * const walls_prefix[3], bool anon_allowed, bool *p_new)
{
    if (p_new) *p_new = false;
//    bool f_optional = false; //!!(pfx_flags & PFX_OPT);
//    bool fSuspectTypo = false; //!!(pfx_flags & PFX_SUSPECT_TYPO);
//    prefix *back_ptr, *ptr;
    string component = S_INIT;
//    size_t i;
//    bool fNew;
//    bool fImplicitPrefix = true;
//    int depth = -1;
//    filepos fp_firstsep;

    filepos fp;
    get_pos(&fp);

    skipblanks();
    if (anon_allowed && ch == '-') {
	// - or -- is an anonymous wall point in a shot, but in #Fix they seem
	// to just be treated as ordinary station names.
	// FIXME: Issue warning for such a useless station?
	//
	// Not yet checked, but you can presumably use - and -- as a prefix
	// (FIXME check this).
	nextch();
	int dashes = 1;
	if (ch == '-') {
	    ++dashes;
	    nextch();
	}
	if (!isNames(ch) && ch != ':') {
	    // An anonymous station implies the leg it is on is a splay.
	    if (TSTBIT(pcs->flags, FLAGS_ANON_ONE_END)) {
		set_pos(&fp);
		// Walls also rejects this case.
		compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Can't have a leg between two anonymous stations*/3);
		longjmp(jbSkipLine, 1);
	    }
	    pcs->flags |= BIT(FLAGS_ANON_ONE_END) | BIT(FLAGS_IMPLICIT_SPLAY);
	    prefix *pfx = new_anon_station();
	    pfx->sflags |= BIT(SFLAGS_WALL);
	    // An anonymous station is always new.
	    if (p_new) *p_new = true;
	    return pfx;
	}
	s_appendn(&component, dashes, '-');
    }

    char *w_prefix[3] = { NULL, NULL, NULL };
    int explicit_prefix_levels = 0;
    while (true) {
	while (isNames(ch)) {
	    s_appendch(&component, ch);
	    nextch();
	}
	//printf("component = '%s'\n", s_str(&component));
	if (ch == ':') {
	    nextch();

	    if (++explicit_prefix_levels > 3) {
		// FIXME Make this a proper error
		printf("too many prefix levels\n");
		s_free(&component);
		for (int i = 0; i < 3; ++i) osfree(w_prefix[i]);
		longjmp(jbSkipLine, 1);
	    }

	    if (!s_empty(&component)) {
		// printf("w_prefix[%d] = '%s'\n", explicit_prefix_levels - 1, s_str(&component));
		w_prefix[explicit_prefix_levels - 1] = s_steal(&component);
	    }

	    continue;
	}

	// printf("explicit_prefix_levels=%d %s:%s:%s\n", explicit_prefix_levels, w_prefix[0], w_prefix[1], w_prefix[2]);

	// component is the station name itself.
	if (s_empty(&component)) {
	    if (explicit_prefix_levels == 0) {
		compile_diagnostic(DIAG_ERR|DIAG_COL, /*Expecting station name*/28);
		s_free(&component);
		for (int i = 0; i < 3; ++i) osfree(w_prefix[i]);
		longjmp(jbSkipLine, 1);
	    }
	    // Walls allows an empty station name if there's an explicit prefix.
	    // This seems unlikely to be intended, so warn about it.
	    compile_diagnostic(DIAG_WARN|DIAG_COL, /*Expecting station name*/28);
	    // Use a name with a space in so it can't collide with a real
	    // Walls station name.
	    s_append(&component, "empty name");
	}
	int len = s_len(&component);
	char *p = s_steal(&component);
	// Apply case treatment.
	switch (pcs->Case) {
	  case LOWER:
	    for (int i = 0; i < len; ++i)
		p[i] = tolower((unsigned char)p[i]);
	    break;
	  case UPPER:
	    for (int i = 0; i < len; ++i)
		p[i] = toupper((unsigned char)p[i]);
	    break;
	  case OFF:
	    // Avoid unhandled enum warning.
	    break;
	}

	prefix *ptr = root;
	for (int i = 0; i < 4; ++i) {
	    char *name;
	    int sflag = BIT(SFLAGS_SURVEY);
	    if (i == 3) {
		name = p;
		sflag = 0;
	    } else {
		if (i < 3 - explicit_prefix_levels) {
		    name = walls_prefix[i];
		    // printf("using walls_prefix[%d] = '%s'\n", 2 - i, name);
		} else {
		    name = w_prefix[i - (3 - explicit_prefix_levels)]; // FIXME: Could steal wprefix[i].
		    // printf("using w_prefix[%d] = '%s'\n", i - (3 - explicit_prefix_levels), name);
		}

		if (name == NULL) {
		    // FIXME: This means :X::Y is treated as the same as
		    // ::X:Y but is that right?  Walls docs don't really
		    // say.  Need to test (and is they're different then
		    // probably use a character not valid in Walls station
		    // names for the empty prefix level (e.g. space or
		    // `#`).
		    //
		    // Also, does Walls allow :::X as a station and
		    // ::X:Y which would mean X is a station and survey?
		    // If so, we probably want to keep every empty level.
		    continue;
		}
	    }
	    prefix *back_ptr = ptr;
	    ptr = ptr->down;
	    if (ptr == NULL) {
		/* Special case first time around at each level */
		/* No need to check if we're at the station level - if the
		 * prefix is new the station must be. */
		if (p_new) *p_new = true;
		ptr = osnew(prefix);
		ptr->sflags = sflag;
		if (strlen(name) < sizeof(ptr->ident.i)) {
		    strcpy(ptr->ident.i, name);
		    ptr->sflags |= BIT(SFLAGS_IDENT_INLINE);
		    if (i >= 3) osfree(name);
		} else {
		    ptr->ident.p = (i < 3 ? osstrdup(name) : name);
		}
		name = NULL;
		ptr->right = ptr->down = NULL;
		ptr->pos = NULL;
		ptr->stn = NULL;
		ptr->up = back_ptr;
		ptr->filename = file.filename; // FIXME: Or location of #Prefix, etc for it?
		ptr->line = file.line; // FIXME: Or location of #Prefix, etc for it?
		ptr->min_export = ptr->max_export = 0;
		back_ptr->down = ptr;
	    } else {
		/* Use caching to speed up adding an increasing sequence to a
		 * large survey */
		static prefix *cached_survey = NULL, *cached_station = NULL;
		prefix *ptrPrev = NULL;
		int cmp = 1; /* result of strcmp ( -ve for <, 0 for =, +ve for > ) */
		if (cached_survey == back_ptr) {
		    cmp = strcmp(prefix_ident(cached_station), name);
		    if (cmp <= 0) ptr = cached_station;
		}
		while (ptr && (cmp = strcmp(prefix_ident(ptr), name))<0) {
		    ptrPrev = ptr;
		    ptr = ptr->right;
		}
		if (cmp) {
		    /* ie we got to one that was higher, or the end */
		    if (p_new) *p_new = true;
		    prefix *newptr = osnew(prefix);
		    newptr->sflags = sflag;
		    if (strlen(name) < sizeof(newptr->ident.i)) {
			strcpy(newptr->ident.i, name);
			newptr->sflags |= BIT(SFLAGS_IDENT_INLINE);
			if (i >= 3) osfree(name);
		    } else {
			newptr->ident.p = (i < 3 ? osstrdup(name) : name);
		    }
		    name = NULL;
		    if (ptrPrev == NULL)
			back_ptr->down = newptr;
		    else
			ptrPrev->right = newptr;
		    newptr->right = ptr;
		    newptr->down = NULL;
		    newptr->pos = NULL;
		    newptr->stn = NULL;
		    newptr->up = back_ptr;
		    newptr->filename = file.filename; // FIXME
		    newptr->line = file.line;
		    newptr->min_export = newptr->max_export = 0;
		    ptr = newptr;
		} else {
		    ptr->sflags |= sflag;
		}
		if (!TSTBIT(ptr->sflags, SFLAGS_SURVEY)) {
		    ptr->min_export = USHRT_MAX;
		}
		cached_survey = back_ptr;
		cached_station = ptr;
	    }
	    if (name == p) osfree(p);
	}

	// Do the equivalent of "*infer exports" for Walls stations with an
	// explicit prefix.
	if (ptr->min_export == 0 || ptr->min_export == USHRT_MAX) {
	    if (explicit_prefix_levels > ptr->max_export)
		ptr->max_export = explicit_prefix_levels;
	}

	for (int i = 0; i < 3; ++i) osfree(w_prefix[i]);

	return ptr;
    }
}

/* if numeric expr is omitted: if f_optional return HUGE_REAL, else longjmp */
real
read_number(bool f_optional, bool f_unsigned)
{
   bool fPositive = true, fDigits = false;
   real n = (real)0.0;
   filepos fp;
   int ch_old;

   get_pos(&fp);
   ch_old = ch;
   if (!f_unsigned) {
      fPositive = !isMinus(ch);
      if (isSign(ch)) nextch();
   }

   while (isdigit(ch)) {
      n = n * (real)10.0 + (char)(ch - '0');
      nextch();
      fDigits = true;
   }

   if (isDecimal(ch)) {
      real mult = (real)1.0;
      nextch();
      while (isdigit(ch)) {
	 mult *= (real).1;
	 n += (char)(ch - '0') * mult;
	 fDigits = true;
	 nextch();
      }
   }

   /* !'fRead' => !fDigits so fDigits => 'fRead' */
   if (fDigits) return (fPositive ? n : -n);

   /* didn't read a valid number.  If it's optional, reset filepos & return */
   set_pos(&fp);
   if (f_optional) {
      return HUGE_REAL;
   }

   if (isOmit(ch_old)) {
      compile_diagnostic(DIAG_ERR|DIAG_COL, /*Field may not be omitted*/8);
   } else {
      compile_diagnostic_token_show(DIAG_ERR, /*Expecting numeric field, found “%s”*/9);
   }
   longjmp(jbSkipLine, 1);
}

real
read_quadrant(bool f_optional)
{
   enum {
      POINT_N = 0,
      POINT_E = 1,
      POINT_S = 2,
      POINT_W = 3,
      POINT_NONE = -1
   };
   static const sztok pointtab[] = {
	{"E", POINT_E },
	{"N", POINT_N },
	{"S", POINT_S },
	{"W", POINT_W },
	{NULL, POINT_NONE }
   };
   static const sztok pointewtab[] = {
	{"E", POINT_E },
	{"W", POINT_W },
	{NULL, POINT_NONE }
   };
   if (f_optional && isOmit(ch)) {
      return HUGE_REAL;
   }
   const int quad = 90;
   filepos fp;
   get_pos(&fp);
   get_token_legacy_no_blanks();
   int first_point = match_tok(pointtab, TABSIZE(pointtab));
   if (first_point == POINT_NONE) {
      set_pos(&fp);
      if (isOmit(ch)) {
	 compile_diagnostic(DIAG_ERR|DIAG_COL, /*Field may not be omitted*/8);
      }
      compile_diagnostic_token_show(DIAG_ERR, /*Expecting quadrant bearing, found “%s”*/483);
      longjmp(jbSkipLine, 1);
   }
   real r = read_number(true, true);
   if (r == HUGE_REAL) {
      if (isSign(ch) || isDecimal(ch)) {
	 /* Give better errors for S-0E, N+10W, N.E, etc. */
	 set_pos(&fp);
	 compile_diagnostic_token_show(DIAG_ERR, /*Expecting quadrant bearing, found “%s”*/483);
	 longjmp(jbSkipLine, 1);
      }
      /* N, S, E or W. */
      return first_point * quad;
   }
   if (first_point == POINT_E || first_point == POINT_W) {
      set_pos(&fp);
      compile_diagnostic_token_show(DIAG_ERR, /*Expecting quadrant bearing, found “%s”*/483);
      longjmp(jbSkipLine, 1);
   }

   get_token_legacy_no_blanks();
   int second_point = match_tok(pointewtab, TABSIZE(pointewtab));
   if (second_point == POINT_NONE) {
      set_pos(&fp);
      compile_diagnostic_token_show(DIAG_ERR, /*Expecting quadrant bearing, found “%s”*/483);
      longjmp(jbSkipLine, 1);
   }

   if (r > quad) {
      set_pos(&fp);
      compile_diagnostic_token_show(DIAG_ERR, /*Suspicious compass reading*/59);
      longjmp(jbSkipLine, 1);
   }

   if (first_point == POINT_N) {
      if (second_point == POINT_W) {
	 r = quad * 4 - r;
      }
   } else {
      if (second_point == POINT_W) {
	 r += quad * 2;
      } else {
	 r = quad * 2 - r;
      }
   }
   return r;
}

extern real
read_numeric(bool f_optional)
{
   skipblanks();
   return read_number(f_optional, false);
}

extern real
read_numeric_multi(bool f_optional, bool f_quadrants, int *p_n_readings)
{
   size_t n_readings = 0;
   real tot = (real)0.0;

   skipblanks();
   if (!isOpen(ch)) {
      real r = 0;
      if (!f_quadrants) {
	  r = read_number(f_optional, false);
      } else {
	  r = read_quadrant(f_optional);
	  if (r != HUGE_REAL)
	      do_legacy_token_warning();
      }
      if (p_n_readings) *p_n_readings = (r == HUGE_REAL ? 0 : 1);
      return r;
   }
   nextch();

   skipblanks();
   do {
      if (!f_quadrants) {
	 tot += read_number(false, false);
      } else {
	 tot += read_quadrant(false);
	 do_legacy_token_warning();
      }
      ++n_readings;
      skipblanks();
   } while (!isClose(ch));
   nextch();

   if (p_n_readings) *p_n_readings = n_readings;
   /* FIXME: special averaging for bearings ... */
   /* And for percentage gradient */
   return tot / n_readings;
}

/* read numeric expr or omit (return HUGE_REAL); else longjmp */
extern real
read_bearing_multi_or_omit(bool f_quadrants, int *p_n_readings)
{
   real v;
   v = read_numeric_multi(true, f_quadrants, p_n_readings);
   if (v == HUGE_REAL) {
      if (!isOmit(ch)) {
	 compile_diagnostic_token_show(DIAG_ERR, /*Expecting numeric field, found “%s”*/9);
	 longjmp(jbSkipLine, 1);
      }
      nextch();
   }
   return v;
}

/* Don't skip blanks, variable error code */
unsigned int
read_uint_raw(int errmsg, const filepos *fp)
{
   unsigned int n = 0;
   if (!isdigit(ch)) {
      if (fp) set_pos(fp);
      compile_diagnostic_token_show(DIAG_ERR, errmsg);
      longjmp(jbSkipLine, 1);
   }
   while (isdigit(ch)) {
      n = n * 10 + (char)(ch - '0');
      nextch();
   }
   return n;
}

extern unsigned int
read_uint(void)
{
   skipblanks();
   return read_uint_raw(/*Expecting numeric field, found “%s”*/9, NULL);
}

extern int
read_int(int min_val, int max_val)
{
    skipblanks();
    unsigned n = 0;
    filepos fp;

    get_pos(&fp);
    bool negated = isMinus(ch);
    unsigned limit;
    if (negated) {
	limit = (unsigned)(min_val == INT_MIN ? INT_MIN : -min_val);
    } else {
	limit = (unsigned)max_val;
    }
    if (isSign(ch)) nextch();

    if (!isdigit(ch)) {
bad_value:
	set_pos(&fp);
	/* TRANSLATORS: The first %d will be replaced by the (inclusive) lower
	 * bound and the second by the (inclusive) upper bound, for example:
	 * Expecting integer in range -60 to 60
	 */
	compile_diagnostic(DIAG_ERR|DIAG_NUM, /*Expecting integer in range %d to %d*/489);
	longjmp(jbSkipLine, 1);
    }

    while (isdigit(ch)) {
	unsigned old_n = n;
	n = n * 10 + (char)(ch - '0');
	if (n > limit || n < old_n) {
	    goto bad_value;
	}
	nextch();
    }
    if (isDecimal(ch)) goto bad_value;

    if (negated) {
	if (n > (unsigned)INT_MAX) {
	    // Avoid unportable casting.
	    return INT_MIN;
	}
	return -(int)n;
    }
    return (int)n;
}

extern void
read_string(string *pstr)
{
   s_clear(pstr);

   skipblanks();
   if (ch == '\"') {
      /* String quoted in "" */
      nextch();
      while (1) {
	 if (isEol(ch)) {
	    compile_diagnostic(DIAG_ERR|DIAG_COL, /*Missing \"*/69);
	    longjmp(jbSkipLine, 1);
	 }

	 if (ch == '\"') break;

	 s_appendch(pstr, ch);
	 nextch();
      }
      nextch();
   } else {
      /* Unquoted string */
      while (1) {
	 if (isEol(ch) || isComm(ch)) {
	    if (s_empty(pstr)) {
	       compile_diagnostic(DIAG_ERR|DIAG_COL, /*Expecting string field*/121);
	       longjmp(jbSkipLine, 1);
	    }
	    return;
	 }

	 if (isBlank(ch)) break;

	 s_appendch(pstr, ch);
	 nextch();
      }
   }
}

extern void
read_walls_srv_date(int *py, int *pm, int *pd)
{
    skipblanks();

    filepos fp_date;
    get_pos(&fp_date);
    unsigned y = read_uint_raw(/*Expecting date, found “%s”*/198, &fp_date);
    int separator = -2;
    if (ch == '-' || ch == '/') {
	separator = ch;
	nextch();
    }
    filepos fp_month;
    get_pos(&fp_month);
    unsigned m = read_uint_raw(/*Expecting date, found “%s”*/198, &fp_date);
    if (ch == separator) {
	nextch();
    }
    filepos fp_day;
    get_pos(&fp_day);
    unsigned d = read_uint_raw(/*Expecting date, found “%s”*/198, &fp_date);

    filepos fp_year;
    if (y < 100) {
	// Walls recommends ISO 8601 date format (yyyy-mm-dd and seemingly the
	// non-standard variant yyyy/mm/dd), but also accepts "some date formats
	// common in the U.S. (mm/dd/yy, mm-dd-yyyy, etc.)"
	unsigned tmp = y;
	y = d;
	fp_year = fp_day;
	d = m;
	fp_day = fp_month;
	m = tmp;
	fp_month = fp_date;

	if (y < 100) {
	    // FIXME: Are all 2 digit years 19xx?
	    y += 1900;

	    filepos fp_save;
	    get_pos(&fp_save);
	    set_pos(&fp_year);
	    /* TRANSLATORS: %d will be replaced by the assumed year, e.g. 1918 */
	    compile_diagnostic(DIAG_WARN|DIAG_UINT, /*Assuming 2 digit year is %d*/76, y);
	    set_pos(&fp_save);
	}
    } else {
	if (y < 1900 || y > 2078) {
	    set_pos(&fp_date);
	    compile_diagnostic(DIAG_WARN|DIAG_UINT, /*Invalid year (< 1900 or > 2078)*/58);
	    longjmp(jbSkipLine, 1);
	}
	fp_year = fp_date;
    }

    if (m < 1 || m > 12) {
	set_pos(&fp_month);
	compile_diagnostic(DIAG_WARN|DIAG_UINT, /*Invalid month*/86);
	longjmp(jbSkipLine, 1);
    }

    if (d < 1 || d > (unsigned)last_day(y, m)) {
	set_pos(&fp_day);
	/* TRANSLATORS: e.g. 31st of April, or 32nd of any month */
	compile_diagnostic(DIAG_WARN|DIAG_UINT, /*Invalid day of the month*/87);
	longjmp(jbSkipLine, 1);
    }

    if (py) *py = y;
    if (pm) *pm = m;
    if (pd) *pd = d;
}
