/* message.c
 * Fairly general purpose message and error routines
 * Copyright (C) 1993-2025 Olly Betts
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

/*#define DEBUG 1*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <locale.h>

#include "cmdline.h"
#include "filename.h"
#include "message.h"
#include "osalloc.h"
#include "filelist.h"
#include "debug.h"
#include "str.h"

#ifdef AVEN
# include "aven.h"
#endif

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <sys/types.h>
#endif

#include <sys/stat.h>

int msg_warnings = 0; /* keep track of how many warnings we've given */
int msg_errors = 0;   /* and how many (non-fatal) errors */

/* in case osmalloc() fails before appname_copy is set up */
static const char *appname_copy = "anonymous program";

/* Path to use to look for executables (used by aven to find cavern). */
static const char *exe_pth = "";

/* error code for failed osmalloc and osrealloc calls */
static void
outofmem(size_t size)
{
   /* TRANSLATORS: "%lu" is a placeholder for the number of bytes which Survex
    * was trying to allocate space for. */
   fatalerror(/*Out of memory (couldn’t find %lu bytes).*/24,
	      (unsigned long)size);
}

/* malloc with error catching if it fails. Also allows us to write special
 * versions easily eg for MS Windows.
 */
void *
osmalloc(size_t size)
{
   void *p = malloc(size);
   if (p == NULL) outofmem(size);
   return p;
}

/* realloc with error catching if it fails. */
void *
osrealloc(void *p, size_t size)
{
   /* some pre-ANSI realloc implementations don't cope with a NULL pointer */
   if (p == NULL) {
      p = malloc(size);
   } else {
      p = realloc(p, size);
   }
   if (p == NULL) outofmem(size);
   return p;
}

char *
osstrdup(const char *str)
{
   char *p;
   size_t len;
   len = strlen(str) + 1;
   p = osmalloc(len);
   memcpy(p, str, len);
   return p;
}

static int
default_charset(void)
{
   if (getenv("SURVEX_UTF8")) return CHARSET_UTF8;
#ifdef _WIN32
   switch (GetConsoleOutputCP()) {
    case 0: return CHARSET_UTF8;
    case 1252: return CHARSET_WINCP1252;
    case 1250: return CHARSET_WINCP1250;
    case 850: return CHARSET_DOSCP850;
   }
   return CHARSET_USASCII;
#else
   const char *p = getenv("LC_ALL");
   if (p == NULL || p[0] == '\0') {
      p = getenv("LC_CTYPE");
      if (p == NULL || p[0] == '\0') {
	 p = getenv("LANG");
      }
   }

   if (p) {
      const char *q = strchr(p, '.');
      if (q) p = q + 1;
   }

   if (p) {
      const char *chset = p;
      size_t name_len;

      while (*p != '\0' && *p != '@') p++;

      name_len = p - chset;

      if (name_len) {
	 bool only_digit = true;
	 size_t cnt;

	 for (cnt = 0; cnt < name_len; ++cnt)
	    if (isalpha((unsigned char)chset[cnt])) {
	       only_digit = false;
	       break;
	    }

	 if (only_digit) goto iso;

	 switch (tolower((unsigned char)chset[0])) {
	  case 'i':
	    if (tolower((unsigned char)chset[1]) == 's' &&
		tolower((unsigned char)chset[2]) == 'o') {
	       chset += 3;
	       iso:
	       if (strncmp(chset, "8859", 4) == 0) {
		  chset += 4;
		  while (chset < p && *chset && !isalnum((unsigned char)*chset))
		     chset++;
		  switch (atoi(chset)) {
		   case 1: return CHARSET_ISO_8859_1;
		   case 2: return CHARSET_ISO_8859_2;
		   case 15: return CHARSET_ISO_8859_15;
		   default: return CHARSET_USASCII;
		  }
	       }
	    }
	    break;
	  case 'u':
	    if (tolower((unsigned char)chset[1]) == 't' &&
		tolower((unsigned char)chset[2]) == 'f') {
	       chset += 3;
	       while (chset < p && *chset && !isalnum((unsigned char)*chset))
		  chset++;
	       switch (atoi(chset)) {
		case 8: return CHARSET_UTF8;
		default: return CHARSET_USASCII;
	       }
	    }
	 }
      }
   }
   return CHARSET_USASCII;
#endif
}

/* It seems that Swedish and maybe some other scandanavian languages don't
 * transliterate &auml; to ae - but it seems there may be conflicting views
 * on this...
 */
#define umlaut_to_e() 1

/* values <= 127 already dealt with */
static int
add_unicode(int charset, unsigned char *p, int value)
{
#ifdef DEBUG
   fprintf(stderr, "add_unicode(%d, %p, %d)\n", charset, p, value);
#endif
   if (value == 0) return 0;
   switch (charset) {
   case CHARSET_USASCII:
      if (value < 0x80) {
	 *p = value;
	 return 1;
      }
      break;
   case CHARSET_ISO_8859_1:
      if (value < 0x100) {
	 *p = value;
	 return 1;
      }
      switch (value) {
	case 0x2032:
	  /* prime -> spacing acute accent */
	  *p = '\xb4';
	  return 1;
	case 0x2033:
	  /* double prime -> 2x spacing acute accent */
	  p[1] = *p = '\xb4';
	  return 2;
      }
      break;
   case CHARSET_ISO_8859_2:
      if (value >= 0xa0) {
	 int v = 0;
	 switch (value) {
	    case 0xa0: case 0xa4: case 0xa7: case 0xa8: case 0xad: case 0xb0:
	    case 0xb4: case 0xb8: case 0xc1: case 0xc2: case 0xc4: case 0xc7:
	    case 0xc9: case 0xcb: case 0xcd: case 0xce: case 0xd3: case 0xd4:
	    case 0xd6: case 0xd7: case 0xda: case 0xdc: case 0xdd: case 0xdf:
	    case 0xe1: case 0xe2: case 0xe4: case 0xe7: case 0xe9: case 0xeb:
	    case 0xed: case 0xee: case 0xf3: case 0xf4: case 0xf6: case 0xf7:
	    case 0xfa: case 0xfc: case 0xfd:
	       v = value; break;
	    case 0x104: v = '\xa1'; break;
	    case 0x2d8: v = '\xa2'; break;
	    case 0x141: v = '\xa3'; break;
	    case 0x13d: v = '\xa5'; break;
	    case 0x15a: v = '\xa6'; break;
	    case 0x160: v = '\xa9'; break;
	    case 0x15e: v = '\xaa'; break; /* Scedil */
	    case 0x164: v = '\xab'; break;
	    case 0x179: v = '\xac'; break;
	    case 0x17d: v = '\xae'; break;
	    case 0x17b: v = '\xaf'; break;
	    case 0x105: v = '\xb1'; break;
	    case 0x2db: v = '\xb2'; break;
	    case 0x142: v = '\xb3'; break;
	    case 0x2032: v = '\xb4'; break; /* prime -> spacing acute accent */
	    case 0x13e: v = '\xb5'; break;
	    case 0x15b: v = '\xb6'; break;
	    case 0x2c7: v = '\xb7'; break;
	    case 0x161: v = '\xb9'; break;
	    case 0x15f: v = '\xba'; break; /* scedil */
	    case 0x165: v = '\xbb'; break;
	    case 0x17a: v = '\xbc'; break;
	    case 0x2dd: v = '\xbd'; break;
	    case 0x2033: v = '\xbd'; break; /* double prime -> spacing double acute accent */
	    case 0x17e: v = '\xbe'; break;
	    case 0x17c: v = '\xbf'; break;
	    case 0x154: v = '\xc0'; break;
	    case 0x102: v = '\xc3'; break;
	    case 0x139: v = '\xc5'; break;
	    case 0x106: v = '\xc6'; break;
	    case 0x10c: v = '\xc8'; break;
	    case 0x118: v = '\xca'; break;
	    case 0x11a: v = '\xcc'; break;
	    case 0x10e: v = '\xcf'; break;
	    case 0x110: v = '\xd0'; break;
	    case 0x143: v = '\xd1'; break;
	    case 0x147: v = '\xd2'; break;
	    case 0x150: v = '\xd5'; break;
	    case 0x158: v = '\xd8'; break;
	    case 0x16e: v = '\xd9'; break;
	    case 0x170: v = '\xdb'; break;
	    case 0x162: v = '\xde'; break; /* &Tcedil; */
	    case 0x155: v = '\xe0'; break;
	    case 0x103: v = '\xe3'; break;
	    case 0x13a: v = '\xe5'; break;
	    case 0x107: v = '\xe6'; break;
	    case 0x10d: v = '\xe8'; break;
	    case 0x119: v = '\xea'; break;
	    case 0x11b: v = '\xec'; break;
	    case 0x10f: v = '\xef'; break;
	    case 0x111: v = '\xf0'; break;
	    case 0x144: v = '\xf1'; break;
	    case 0x148: v = '\xf2'; break;
	    case 0x151: v = '\xf5'; break;
	    case 0x159: v = '\xf8'; break;
	    case 0x16f: v = '\xf9'; break;
	    case 0x171: v = '\xfb'; break;
	    case 0x163: v = '\xfe'; break; /* tcedil */
	    case 0x2d9: v = '\xff'; break;
	 }
	 if (v == 0) break;
	 value = v;
      }
      *p = value;
      return 1;
   case CHARSET_ISO_8859_15:
      switch (value) {
       case 0xa4: case 0xa6: case 0xb0: case 0xc4:
       case 0xd0: case 0xd4: case 0xd5: case 0xd6:
	 goto donthave;
       case 0x152: value = 0xd4; break; /* &OElig; */
       case 0x153: value = 0xd5; break; /* &oelig; */
#if 0
       case 0x0: value = 0xa4; break; /* euro */
#endif
       case 0x160: value = 0xa6; break; /* Scaron */
       case 0x161: value = 0xb0; break; /* scaron */
       case 0x17d: value = 0xc4; break; /* Zcaron */
       case 0x17e: value = 0xd0; break; /* zcaron */
#if 0
       case 0x0: value = 0xd6; break; /* Ydiersis */
#endif
      }
      if (value < 0x100) {
	 *p = value;
	 return 1;
      }
      donthave:
      break;
#ifdef _WIN32
   case CHARSET_WINCP1250:
      /* MS Windows rough equivalent to ISO-8859-2 */
      if (value >= 0x80) {
	 int v = 0;
	 /* This mapping is complete - there are 5 unused positions:
	  * 0x81 0x83 0x88 0x90 0x98 */
	 switch (value) {
	    case 0xa0: case 0xa4: case 0xa6: case 0xa7: case 0xa8: case 0xa9:
	    case 0xab: case 0xac: case 0xad: case 0xae: case 0xb0: case 0xb1:
	    case 0xb4: case 0xb5: case 0xb6: case 0xb7: case 0xb8: case 0xbb:
	    case 0xc1: case 0xc2: case 0xc4: case 0xc7: case 0xc9: case 0xcb:
	    case 0xcd: case 0xce: case 0xd3: case 0xd4: case 0xd6: case 0xd7:
	    case 0xda: case 0xdc: case 0xdd: case 0xdf: case 0xe1: case 0xe2:
	    case 0xe4: case 0xe7: case 0xe9: case 0xeb: case 0xed: case 0xee:
	    case 0xf3: case 0xf4: case 0xf6: case 0xf7: case 0xfa: case 0xfc:
	    case 0xfd:
	       v = value; break;
	    case 0x20ac: v = '\x80'; break;
	    case 0x201a: v = '\x82'; break;
	    case 0x201e: v = '\x84'; break;
	    case 0x2026: v = '\x85'; break;
	    case 0x2020: v = '\x86'; break;
	    case 0x2021: v = '\x87'; break;
	    case 0x2030: v = '\x89'; break;
	    case 0x0160: v = '\x8a'; break;
	    case 0x2039: v = '\x8b'; break;
	    case 0x015a: v = '\x8c'; break;
	    case 0x0164: v = '\x8d'; break;
	    case 0x017d: v = '\x8e'; break;
	    case 0x0179: v = '\x8f'; break;
	    case 0x2018: v = '\x91'; break;
	    case 0x2019: v = '\x92'; break;
	    case 0x201c: v = '\x93'; break;
	    case 0x201d: v = '\x94'; break;
	    case 0x2022: v = '\x95'; break;
	    case 0x2013: v = '\x96'; break;
	    case 0x2014: v = '\x97'; break;
	    case 0x2122: v = '\x99'; break;
	    case 0x0161: v = '\x9a'; break;
	    case 0x203a: v = '\x9b'; break;
	    case 0x015b: v = '\x9c'; break;
	    case 0x0165: v = '\x9d'; break;
	    case 0x017e: v = '\x9e'; break;
	    case 0x017a: v = '\x9f'; break;
	    case 0x02c7: v = '\xa1'; break;
	    case 0x02d8: v = '\xa2'; break;
	    case 0x0141: v = '\xa3'; break;
	    case 0x0104: v = '\xa5'; break;
	    case 0x015e: v = '\xaa'; break; /* Scedil */
	    case 0x017b: v = '\xaf'; break;
	    case 0x02db: v = '\xb2'; break;
	    case 0x0142: v = '\xb3'; break;
	    case 0x2032: v = '\xb4'; break; /* prime -> spacing acute accent */
	    case 0x0105: v = '\xb9'; break;
	    case 0x015f: v = '\xba'; break; /* scedil */
	    case 0x013d: v = '\xbc'; break;
	    case 0x02dd: v = '\xbd'; break;
	    case 0x2033: v = '\xbd'; break; /* double prime -> spacing double acute accent */
	    case 0x013e: v = '\xbe'; break;
	    case 0x017c: v = '\xbf'; break;
	    case 0x0154: v = '\xc0'; break;
	    case 0x0102: v = '\xc3'; break;
	    case 0x0139: v = '\xc5'; break;
	    case 0x0106: v = '\xc6'; break;
	    case 0x010c: v = '\xc8'; break;
	    case 0x0118: v = '\xca'; break;
	    case 0x011a: v = '\xcc'; break;
	    case 0x010e: v = '\xcf'; break;
	    case 0x0110: v = '\xd0'; break;
	    case 0x0143: v = '\xd1'; break;
	    case 0x0147: v = '\xd2'; break;
	    case 0x0150: v = '\xd5'; break;
	    case 0x0158: v = '\xd8'; break;
	    case 0x016e: v = '\xd9'; break;
	    case 0x0170: v = '\xdb'; break;
	    case 0x0162: v = '\xde'; break; /* &Tcedil; */
	    case 0x0155: v = '\xe0'; break;
	    case 0x0103: v = '\xe3'; break;
	    case 0x013a: v = '\xe5'; break;
	    case 0x0107: v = '\xe6'; break;
	    case 0x010d: v = '\xe8'; break;
	    case 0x0119: v = '\xea'; break;
	    case 0x011b: v = '\xec'; break;
	    case 0x010f: v = '\xef'; break;
	    case 0x0111: v = '\xf0'; break;
	    case 0x0144: v = '\xf1'; break;
	    case 0x0148: v = '\xf2'; break;
	    case 0x0151: v = '\xf5'; break;
	    case 0x0159: v = '\xf8'; break;
	    case 0x016f: v = '\xf9'; break;
	    case 0x0171: v = '\xfb'; break;
	    case 0x0163: v = '\xfe'; break; /* tcedil */
	    case 0x02d9: v = '\xff'; break;
	 }
	 if (v == 0) break;
	 value = v;
      }
      *p = value;
      return 1;
   case CHARSET_WINCP1252:
      /* MS Windows extensions to ISO-8859-1 */
      /* This mapping is complete - there are 5 unused positions:
       * 0x81 0x8d 0x8f 0x90 0x9d */
      switch (value) {
       case 0x2026: value = 0x85; break; /* hellip */
       case 0x0160: value = 0x8a; break; /* Scaron */
       case 0x0152: value = 0x8c; break; /* OElig */
       case 0x017d: value = 0x8e; break; /* Zcaron */
       case 0x2019: value = 0x92; break; /* rsquo */
       case 0x201c: value = 0x93; break; /* ldquo */
       case 0x201d: value = 0x94; break; /* rdquo */
       case 0x0161: value = 0x9a; break; /* scaron */
       case 0x0153: value = 0x9c; break; /* oelig */
       case 0x017e: value = 0x9e; break; /* zcaron */
#if 0
       /* there are a few other obscure ones we don't currently need */
       case 0x20ac: value = 0x80; break; /* euro */
       case 0x201a: value = 0x82; break; /* sbquo */
       case 0x0192: value = 0x83; break; /* fnof */
       case 0x201e: value = 0x84; break; /* bdquo */
       case 0x2020: value = 0x86; break; /* dagger */
       case 0x2021: value = 0x87; break; /* Dagger */
       case 0x02c6: value = 0x88; break; /* circ */
       case 0x2030: value = 0x89; break; /* permil */
       case 0x2039: value = 0x8b; break; /* lsaquo */
       case 0x2018: value = 0x91; break; /* lsquo */
       case 0x2022: value = 0x95; break; /* bull */
       case 0x2013: value = 0x96; break; /* ndash */
       case 0x2014: value = 0x97; break; /* mdash */
       case 0x02dc: value = 0x98; break; /* tilde */
       case 0x2122: value = 0x99; break; /* trade */
       case 0x203a: value = 0x9b; break; /* rsaquo */
       case 0x0178: value = 0x9f; break; /* Yuml */
#endif
       case 0x2032: value = 0xb4; break; /* prime -> spacing acute accent */
       case 0x2033:
	  /* double prime -> 2x spacing acute accent */
	  p[1] = *p = '\xb4';
	  return 2;
      }
      if (value < 0x100) {
	 *p = value;
	 return 1;
      }
      break;
#endif
#ifdef _WIN32
   case CHARSET_DOSCP850: {
      static const unsigned char uni2dostab[] = {
	 255, 173, 189, 156, 207, 190, 221, 245,
	 249, 184, 166, 174, 170, 240, 169, 238,
	 248, 241, 253, 252, 239, 230, 244, 250,
	 247, 251, 167, 175, 172, 171, 243, 168,
	 183, 181, 182, 199, 142, 143, 146, 128,
	 212, 144, 210, 211, 222, 214, 215, 216,
	 209, 165, 227, 224, 226, 229, 153, 158,
	 157, 235, 233, 234, 154, 237, 232, 225,
	 133, 160, 131, 198, 132, 134, 145, 135,
	 138, 130, 136, 137, 141, 161, 140, 139,
	 208, 164, 149, 162, 147, 228, 148, 246,
	 155, 151, 163, 150, 129, 236, 231, 152
      };
      if (value >= 160 && value < 256) {
	 *p = (int)uni2dostab[value - 160];
	 return 1;
      }
      switch (value) {
	case 0x2032:
	  /* prime -> spacing acute accent */
	  *p = '\xef';
	  return 1;
	case 0x2033:
	  /* double prime -> 2x spacing acute accent */
	  p[1] = *p = '\xef';
	  return 2;
      }
#if 0
      if (value == 305) { /* LATIN SMALL LETTER DOTLESS I */
	 *p = 213; /* "Modified CP850" has the Euro sign here. */
	 return 1;
      }
      if (value == 402) { /* LATIN SMALL LETTER F WITH HOOK */
	 *p = 159;
	 return 1;
      }
#endif
      break;
   }
#endif
   }
   /* Transliterate characters we can't represent */
#ifdef DEBUG
   fprintf(stderr, "transliterate “%c” 0x%x\n", value, value);
#endif
   switch (value) {
    case 160:
      *p = ' '; return 1;
    case 161 /* ¡ */:
      *p = '!'; return 1;
    case 176 /* ° */:
      *p = 'd'; p[1] = 'g'; return 2;
    case 191 /* ¿ */:
      *p = '?'; return 1;
    case 192 /* À */: case 193 /* Á */: case 194 /* Â */: case 195 /* Ã */:
      *p = 'A'; return 1;
    case 197 /* Å */:
      p[1] = *p = 'A'; return 2;
    case 196 /* Ä */: /* &Auml; */
      *p = 'A';
      if (!umlaut_to_e()) return 1;
      p[1] = 'E'; return 2;
    case 198 /* Æ */:
      *p = 'A'; p[1] = 'E'; return 2;
    case 199 /* Ç */: case 268: /* &Ccaron; */
      *p = 'C'; return 1;
    case 270: /* &Dcaron; */
      *p = 'D'; return 1;
    case 200 /* È */: case 201 /* É */: case 202 /* Ê */: case 203 /* Ë */:
      *p = 'E'; return 1;
    case 204 /* Ì */: case 205 /* Í */: case 206 /* Î */: case 207 /* Ï */:
      *p = 'I'; return 1;
    case 208 /* Ð */: case 222 /* Þ */:
      *p = 'T'; p[1] = 'H'; return 2;
    case 315: /* &Lacute; */
    case 317: /* &Lcaron; */
      *p = 'L'; return 1;
    case 209 /* Ñ */:
      *p = 'N'; return 1;
    case 210 /* Ò */: case 211 /* Ó */: case 212 /* Ô */: case 213 /* Õ */:
      *p = 'O'; return 1;
    case 214 /* Ö */: /* &Ouml; */ case 0x152: /* &OElig; */
      *p = 'O'; p[1] = 'E'; return 2;
    case 352: /* &Scaron; */
    case 0x15e: /* &Scedil; */
      *p = 'S'; return 1;
    case 0x162: /* &Tcedil; */
    case 0x164: /* &Tcaron; */
      *p = 'T'; return 1;
    case 217 /* Ù */: case 218 /* Ú */: case 219 /* Û */:
      *p = 'U'; return 1;
    case 220 /* Ü */: /* &Uuml; */
      *p = 'U'; p[1] = 'E'; return 2;
    case 221 /* Ý */:
      *p = 'Y'; return 1;
    case 381: /* &Zcaron; */
      *p = 'Z'; return 1;
    case 223 /* ß */:
      p[1] = *p = 's'; return 2;
    case 224 /* à */: case 225 /* á */: case 226 /* â */: case 227 /* ã */:
    case 259: /* &abreve; */
      *p = 'a'; return 1;
    case 228 /* ä */: /* &auml; */ case 230 /* æ */:
      *p = 'a'; p[1] = 'e'; return 2;
    case 229 /* å */:
      p[1] = *p = 'a'; return 2;
    case 231 /* ç */: case 269 /* &ccaron; */:
      *p = 'c'; return 1;
    case 271: /* &dcaron; */
      *p = 'd'; return 1;
    case 232 /* è */: case 233 /* é */: case 234 /* ê */: case 235 /* ë */:
    case 283 /* &ecaron; */:
      *p = 'e'; return 1;
    case 236 /* ì */: case 237 /* í */: case 238 /* î */: case 239 /* ï */:
      *p = 'i'; return 1;
    case 316 /* &lacute; */:
    case 318 /* &lcaron; */:
      *p = 'l'; return 1;
    case 241 /* ñ */: case 328 /* &ncaron; */:
      *p = 'n'; return 1;
    case 345: /* &rcaron; */
      *p = 'r'; return 1;
    case 353: /* &scaron; */
    case 0x15f: /* &scedil; */
      *p = 's'; return 1;
    case 357: /* &tcaron; */
    case 0x163: /* &tcedil; */
      *p = 't'; return 1;
    case 240 /* ð */: case 254 /* þ */:
      *p = 't'; p[1] = 'h'; return 2;
    case 242 /* ò */: case 243 /* ó */: case 244 /* ô */: case 245 /* õ */:
      *p = 'o'; return 1;
    case 246 /* ö */: /* &ouml; */ case 0x153: /* &oelig; */
      *p = 'o'; p[1] = 'e'; return 2;
    case 249 /* ù */: case 250 /* ú */: case 251 /* û */:
    case 367 /* &uring; */:
      *p = 'u'; return 1;
    case 252 /* ü */: /* &uuml; */
      *p = 'u'; p[1] = 'e'; return 2;
    case 253 /* ý */: case 255 /* ÿ */:
      *p = 'y'; return 1;
    case 382: /* &zcaron; */
      *p = 'z'; return 1;
    case 0x2019: /* &lsquo; */
      *p = '\''; return 1;
    case 171: /* « */ case 187: /* » */
    case 0x201c: /* &ldquo; */ case 0x201d: /* &rdquo; */
      *p = '"'; return 1;
    case 0x2026: /* &hellip; */
      *p = '.'; p[1] = '.'; p[2] = '.'; return 3;
    case 0x2032: /* prime -> apostrophe */
      *p = '\''; return 1;
    case 0x2033: /* double prime -> double quote */
      *p = '"'; return 1;
    case 0x2192: /* &rarr; */
      *p = '-'; p[1] = '>'; return 2;
    case 0x1d4d: /* gradient symbol */
      *p = 'g'; p[1] = 'r'; p[2] = 'd'; return 3;
    case 0x221e: /* infinity symbol */
      *p = 'i'; p[1] = 'n'; p[2] = 'f'; return 3;
   }
#ifdef DEBUG
   /* 169 is reported (copyright symbol), but there isn't a good <= 2 ASCII
    * character transliteration for that, so we handle that elsewhere. */
   fprintf(stderr, "failed to transliterate codepoint %d\n", value);
#endif
   return 0;
}

#if !defined _WIN32 && defined DATADIR && defined PACKAGE
/* Under Unix, we compile in the configured path */
static const char *pth_cfg_files = DATADIR "/" PACKAGE;
#else
/* On other platforms, we fall back on looking in the current directory */
static const char *pth_cfg_files = "";
#endif

static int num_msgs = 0;
static char **msg_array = NULL;

static bool msg_lang_explicit = false;
const char *msg_lang = NULL;
const char *msg_lang2 = NULL;

static char **
parse_msgs(int n, unsigned char *p, int charset_code) {
   int i;

   char **msgs = osmalloc(n * sizeof(char *));

   for (i = 0; i < n; i++) {
      unsigned char *to = p;
      int ch;
      msgs[i] = (char *)p;

      /* If we want UTF8 anyway, we just need to find the start of each
       * message */
      if (charset_code == CHARSET_UTF8) {
	 p += strlen((char *)p) + 1;
	 continue;
      }

      while ((ch = *p++) != 0) {
	 /* A byte in the range 0x80-0xbf or 0xf0-0xff isn't valid in
	  * this state, (0xf0-0xfd mean values > 0xffff) so treat as
	  * literal and try to resync so we cope better when fed
	  * non-utf-8 data.  Similarly we abandon a multibyte sequence
	  * if we hit an invalid character. */
	 if (ch >= 0xc0 && ch < 0xf0) {
	    int ch1 = *p;
	    if ((ch1 & 0xc0) != 0x80) goto resync;

	    if (ch < 0xe0) {
	       /* 2 byte sequence */
	       ch = ((ch & 0x1f) << 6) | (ch1 & 0x3f);
	       p++;
	    } else {
	       /* 3 byte sequence */
	       int ch2 = p[1];
	       if ((ch2 & 0xc0) != 0x80) goto resync;
	       ch = ((ch & 0x1f) << 12) | ((ch1 & 0x3f) << 6) | (ch2 & 0x3f);
	       p += 2;
	    }
	 }

	 resync:

	 if (ch < 127) {
	    *to++ = (char)ch;
	 } else {
	    /* We assume an N byte UTF-8 code never transliterates to more
	     * than N characters (so we can't transliterate © to (C) or
	     * ® to (R) for example) */
	    to += add_unicode(charset_code, to, ch);
	 }
      }
      *to++ = '\0';
   }
   return msgs;
}

/* This is the name of the default language, which can be set like so:
 * ./configure --enable-defaultlang=fr
 */
#ifdef DEFAULTLANG
/* No point extracting these errors as they won't get used if file opens */
# include "../lib/defaultlang.h"
#else
#define N_DONTEXTRACTMSGS 5
static unsigned char dontextractmsgs[] =
   "Can't open message file \"%s\" using path \"%s\"\0"/*1000*/
   "Problem with message file \"%s\"\0"/*1001*/
   "I don't understand this message file version\0"/*1002*/
   "Message file truncated?\0"/*1003*/
   "Out of memory (couldn't find %lu bytes).\0"/*1004*/;
#endif

static char **dontextract = NULL;

static void
parse_msg_file(int charset_code)
{
   FILE *fh;
   unsigned char header[20];
   int i;
   unsigned len;
   unsigned char *p;
   char *fnm, *s;
   int n;

#ifdef DEBUG
   fprintf(stderr, "parse_msg_file(%d)\n", charset_code);
#endif

   /* sort out messages we need to print if we can't open the message file */
   dontextract = parse_msgs(N_DONTEXTRACTMSGS, dontextractmsgs, charset_code);

   fnm = osstrdup(msg_lang);
   /* trim off charset from stuff like "de_DE.iso8859_1" */
   s = strchr(fnm, '.');
   if (s) *s = '\0';
   /* trim off any "@<something>" modifier. */
   s = strchr(fnm, '@');
   if (s) *s = '\0';

   fh = fopenWithPthAndExt(pth_cfg_files, fnm, EXT_SVX_MSG, "rb", NULL);

   if (!fh && strlen(fnm) > 3 && fnm[2] == '_') {
      /* e.g. if 'en_GB' is unknown, see if we know 'en' */
      fnm[2] = '\0';
      fh = fopenWithPthAndExt(pth_cfg_files, fnm, EXT_SVX_MSG, "rb", NULL);
      if (!fh) fnm[2] = '_'; /* for error reporting */
   }

   if (!fh && !msg_lang_explicit) {
      /* If msg_lang wasn't specified using environment variable SURVEXLANG,
       * then default to 'en' if we don't find messages for language msg_lang.
       */
      if (fnm[0] && fnm[1]) {
	 strcpy(fnm, "en");
      } else {
	 free(fnm);
	 fnm = osstrdup("en");
      }
      fh = fopenWithPthAndExt(pth_cfg_files, fnm, EXT_SVX_MSG, "rb", NULL);
   }

   if (!fh) {
      fatalerror(/*Can't open message file “%s” using path “%s”*/1000,
		 fnm, pth_cfg_files);
   }

   if (FREAD(header, 1, 20, fh) < 20 ||
       memcmp(header, "Svx\nMsg\r\n\xfe\xff", 12) != 0) {
      fatalerror(/*Problem with message file “%s”*/1001, fnm);
   }

   if (header[12] != 0)
      fatalerror(/*I don't understand this message file version*/1002);

   n = (header[14] << 8) | header[15];

   len = 0;
   for (i = 16; i < 20; i++) len = (len << 8) | header[i];

   p = osmalloc(len);
   if (FREAD(p, 1, len, fh) < len)
      fatalerror(/*Message file truncated?*/1003);

   fclose(fh);

#ifdef DEBUG
   fprintf(stderr, "fnm = “%s”, n = %d, len = %d\n", fnm, n, len);
#endif
   free(fnm);

   msg_array = parse_msgs(n, p, charset_code);
   num_msgs = n;
}

const char *
msg_cfgpth(void)
{
   return pth_cfg_files;
}

const char *
msg_exepth(void)
{
   return exe_pth;
}

const char *
msg_appname(void)
{
   return appname_copy;
}

void
(msg_init)(char * const *argv)
{
   char *p;
   SVX_ASSERT(argv);

   /* Point to argv[0] itself so we report a more helpful error if the
    * code to work out the clean appname generates a signal */
   appname_copy = argv[0];
#ifndef _WIN32
   /* use name as-is on Unix - programs run from path get name as supplied */
   appname_copy = osstrdup(argv[0]);
#else
   // Use the lower-cased leafname on Microsoft Windows.
   p = leaf_from_fnm(argv[0]);
   appname_copy = p;
   while (*p) {
      *p = tolower((unsigned char)*p);
      ++p;
   }
   /* Remove .exe extension if present. */
   if (p - appname_copy > 4 && memcmp(p - 4, ".exe", 4) == 0) {
       p[-4] = '\0';
   }
#endif

   /* shortcut --version so you can check the version number even when the
    * correct message file can't be found... */
   if (argv[1] && strcmp(argv[1], "--version") == 0) {
      cmdline_version();
      exit(0);
   }
   char *pth = getenv("SURVEXLIB");
   if (pth && pth[0]) {
      pth_cfg_files = osstrdup(pth);
   } else if (argv[0]) {
      exe_pth = path_from_fnm(argv[0]);
#if !defined _WIN32 && defined DATADIR && defined PACKAGE
      if (exe_pth[0]) {
	 struct stat buf;
#ifdef __APPLE__
# ifndef AVEN
	 /* On macOS the programs may be installed anywhere, with the
	  * share directory and the binaries in the same directory. */
	 p = use_path(exe_pth, "share/survex/en.msg");
	 if (stat(p, &buf) == 0 && S_ISREG(buf.st_mode)) {
	    pth_cfg_files = use_path(exe_pth, "share/survex");
	    goto macos_got_msg;
	 }
	 free(p);
# endif
	 /* In the diskimage package, this case is used for aven, and for
	  * the hardlinked copies of cavern and extend alongside the aven
	  * binary, which are the ones which aven runs.
	  */
	 p = use_path(exe_pth, "../Resources/en.msg");
	 if (stat(p, &buf) == 0 && S_ISREG(buf.st_mode)) {
	    pth_cfg_files = use_path(exe_pth, "../Resources");
	    goto macos_got_msg;
	 }
	 free(p);
#endif
	 /* If we're run with an explicit path, check if "../lib/en.msg"
	  * from the program's path exists, and if so look there for
	  * support files - this allows us to test binaries in the build
	  * tree easily. */
	 p = use_path(exe_pth, "../lib/en.msg");
	 if (stat(p, &buf) == 0) {
#ifdef S_ISREG
	    /* POSIX way */
	    if (S_ISREG(buf.st_mode)) {
	       pth_cfg_files = use_path(exe_pth, "../lib");
	    }
#else
	    /* BSD way */
	    if ((buf.st_mode & S_IFMT) == S_IFREG) {
	       pth_cfg_files = use_path(exe_pth, "../lib");
	    }
#endif
	 }
#ifdef __APPLE__
macos_got_msg:
#endif
	 free(p);
      }
#elif defined _WIN32
      DWORD len = 256;
      char *buf = NULL, *modname;
      while (1) {
	  DWORD got;
	  buf = osrealloc(buf, len);
	  got = GetModuleFileName(NULL, buf, len);
	  if (got < len) break;
	  len += len;
      }
      modname = buf;
      /* Strange Win32 nastiness - strip prefix "\\?\" if present */
      if (strncmp(modname, "\\\\?\\", 4) == 0) modname += 4;
      pth_cfg_files = path_from_fnm(modname);
      free(buf);
#else
      /* Get the path to the support files from argv[0] */
      pth_cfg_files = exe_path;
#endif
   }

   msg_lang = getenv("SURVEXLANG");
#ifdef DEBUG
   fprintf(stderr, "msg_lang = %p (= \"%s\")\n", msg_lang, msg_lang?msg_lang:"(null)");
#endif

   msg_lang_explicit = true;
   if (!msg_lang || !*msg_lang) {
      msg_lang_explicit = false;
      msg_lang = getenv("LC_ALL");
   }
   if (!msg_lang || !*msg_lang) {
      msg_lang = getenv("LC_MESSAGES");
      if (!msg_lang || !*msg_lang) {
	 msg_lang = getenv("LANG");
	 /* Something (AutoCAD?) on Microsoft Windows sets LANG to a number. */
	 if (msg_lang && !isalpha(msg_lang[0])) msg_lang = NULL;
      }
      if (!msg_lang || !*msg_lang) {
#ifdef _WIN32
	 LCID locid;
#endif
#ifdef DEFAULTLANG
	 msg_lang = STRING(DEFAULTLANG);
#else
	 msg_lang = "en";
#endif
#ifdef _WIN32
	 /* GetUserDefaultUILanguage() requires Microsoft Windows 2000 or
	  * newer, but we don't support anything earlier than Vista.
	  */
	 locid = GetUserDefaultUILanguage();
	 if (locid) {
	    WORD langid = LANGIDFROMLCID(locid);
	    switch (PRIMARYLANGID(langid)) {
	     case LANG_BULGARIAN:
	       msg_lang = "bg";
	       break;
/* older mingw compilers don't seem to supply this value */
#ifndef LANG_CATALAN
# define LANG_CATALAN 0x03
#endif
	     case LANG_CATALAN:
	       msg_lang = "ca";
	       break;
	     case LANG_CHINESE:
	       msg_lang = "zh_CN";
	       break;
	     case LANG_ENGLISH:
	       if (SUBLANGID(langid) == SUBLANG_ENGLISH_US)
		  msg_lang = "en_US";
	       else
		  msg_lang = "en";
	       break;
	     case LANG_FRENCH:
	       msg_lang = "fr";
	       break;
	     case LANG_GERMAN:
	       switch (SUBLANGID(langid)) {
		case SUBLANG_GERMAN_SWISS:
		  msg_lang = "de_CH";
		  break;
		default:
		  msg_lang = "de";
	       }
	       break;
	     case LANG_GREEK:
	       msg_lang = "el";
	       break;
	     case LANG_HUNGARIAN:
	       msg_lang = "hu";
	       break;
	     case LANG_INDONESIAN:
	       msg_lang = "id";
	       break;
	     case LANG_ITALIAN:
	       msg_lang = "it";
	       break;
	     case LANG_POLISH:
	       msg_lang = "pl";
	       break;
	     case LANG_PORTUGUESE:
	       if (SUBLANGID(langid) == SUBLANG_PORTUGUESE_BRAZILIAN)
		  msg_lang = "pt_BR";
	       else
		  msg_lang = "pt";
	       break;
	     case LANG_ROMANIAN:
	       msg_lang = "ro";
	       break;
	     case LANG_RUSSIAN:
	       msg_lang = "ru";
	       break;
	     case LANG_SLOVAK:
	       msg_lang = "sk";
	       break;
	     case LANG_SPANISH:
	       msg_lang = "es";
	       break;
	    }
	 }
#endif
      }
   }
#ifdef DEBUG
   fprintf(stderr, "msg_lang = %p (= \"%s\")\n", msg_lang, msg_lang?msg_lang:"(null)");
#endif

   /* On Mandrake LANG defaults to C */
   if (strcmp(msg_lang, "C") == 0) msg_lang = "en";

   { /* If msg_lang has a country code, snip it out to give msg_lang2. */
      size_t b = 0;
      while (isalpha((unsigned char)msg_lang[b])) {
	 ++b;
      }
      if (msg_lang[b] == '_') {
	 char * tmp;
	 size_t e = b + 1;
	 while (isalpha((unsigned char)msg_lang[e])) {
	    ++e;
	 }
	 tmp = osstrdup(msg_lang);
	 memmove(tmp + b, tmp + e, strlen(tmp + e) + 1);
	 msg_lang2 = tmp;
      }
   }

#ifdef LC_MESSAGES
   /* try to setlocale() appropriately too */
   if (!setlocale(LC_MESSAGES, msg_lang)) {
      if (msg_lang2) {
	 (void)setlocale(LC_MESSAGES, msg_lang2);
      }
   }
#endif

   select_charset(default_charset());
}

#ifndef AVEN
/* Return message if messages available, else a fallback value. */
static const char *
msg_opt(int en, const char * fallback)
{
   /* NB can't use SVX_ASSERT here! */
   if (!msg_array || en <= 0 || en >= num_msgs) {
      return fallback;
   }

   return msg_array[en];
}
#endif

const char *
msg(int en)
{
   /* NB can't use SVX_ASSERT here! */
   if (dontextract && en >= 1000 && en < 1000 + N_DONTEXTRACTMSGS)
      return dontextract[en - 1000];
   if (!msg_array) {
      if (en != 1)  {
	 fprintf(STDERR, "Message %d requested before fully initialised\n", en);
	 return "Message requested before fully initialised\n";
      }
      /* this should be the only other message which can be requested before
       * the message file is opened and read... */
      if (!dontextract) return "Out of memory (couldn't find %lu bytes).";
      return dontextract[(/*Out of memory (couldn't find %lu bytes).*/1004)
			 - 1000];
   }

   if (en < 0 || en >= num_msgs) {
      fprintf(STDERR, "Message %d out of range\n", en);
      return "Message out of range\n";
   }

   if (en == 0) {
      const char *p = msg_array[0];
      if (!*p) p = "(C)";
      return p;
   }

   return msg_array[en];
}

void
v_report(int severity, const char *fnm, int line, int col, int en, va_list ap)
{
#ifdef AVEN
   (void)col;
   aven_v_report(severity, fnm, line, en, ap);
#else
   const char * level;
   if (fnm) {
      fputs(fnm, STDERR);
      if (line) fprintf(STDERR, ":%d", line);
      if (col > 0) fprintf(STDERR, ":%d", col);
   } else {
      fputs(appname_copy, STDERR);
   }
   fputs(": ", STDERR);

   switch (severity) {
    case DIAG_INFO:
      /* TRANSLATORS: Indicates a informational message e.g.:
       * "spoon.svx:12: info: Declination: [...]" */
      level = msg_opt(/*info*/485, "info");
      break;
    case DIAG_WARN:
      /* TRANSLATORS: Indicates a warning message e.g.:
       * "spoon.svx:12: warning: *prefix is deprecated" */
      level = msg_opt(/*warning*/106, "warning");
      break;
    default:
      /* TRANSLATORS: Indicates an error message e.g.:
       * "spoon.svx:13:4: error: Field may not be omitted" */
      level = msg_opt(/*error*/93, "error");
      break;
   }
   fputs(level, STDERR);
   fputs(": ", STDERR);

#ifdef HAVE__VSPRINTF_P
   // Microsoft's vfprintf() doesn't support positional argument specifiers,
   // so we need to use the Microsoft-specific _vfprintf_p().
   _vfprintf_p(STDERR, msg(en), ap);
#else
   vfprintf(STDERR, msg(en), ap);
#endif
   fputnl(STDERR);
#endif

   switch (severity) {
    case DIAG_WARN:
      msg_warnings++;
      break;
    case DIAG_ERR:
      msg_errors++;
      if (msg_errors == 50)
	 fatalerror_in_file(fnm, 0, /*Too many errors - giving up*/19);
      break;
    case DIAG_FATAL:
      exit(EXIT_FAILURE);
   }
}

void
diag(int severity, int en, ...)
{
   va_list ap;
   va_start(ap, en);
   v_report(severity, NULL, 0, 0, en, ap);
   va_end(ap);
}

void
diag_in_file(int severity, const char *fnm, int line, int en, ...)
{
   va_list ap;
   va_start(ap, en);
   v_report(severity, fnm, line, 0, en, ap);
   va_end(ap);
}

/* Code to support switching character set at runtime (e.g. for a printer
 * driver to support different character sets on screen and on the printer)
 */
typedef struct charset_li {
   struct charset_li *next;
   int code;
   char **msg_array;
} charset_li;

static charset_li *charset_head = NULL;

static int charset = CHARSET_BAD;

int
select_charset(int charset_code)
{
   int old_charset = charset;
   charset_li *p;

#ifdef DEBUG
   fprintf(stderr, "select_charset(%d), old charset = %d\n", charset_code,
	   charset);
#endif

   if (charset == charset_code) return charset;

   charset = charset_code;

   /* check if we've already parsed messages for new charset */
   for (p = charset_head; p; p = p->next) {
#ifdef DEBUG
      printf("%p: code %d msg_array %p\n", p, p->code, p->msg_array);
#endif
      if (p->code == charset) {
	 msg_array = p->msg_array;
	 return old_charset;
      }
   }

   /* nope, got to reparse message file */
   parse_msg_file(charset_code);

   /* add to list */
   p = osnew(charset_li);
   p->code = charset;
   p->msg_array = msg_array;
   p->next = charset_head;
   charset_head = p;

   return old_charset;
}
