/* > tex.h */

/* Beware: This file contains top-bit-set characters (160-255), so be     */
/* careful of mailers, ascii ftp, etc                                     */

/* Tables for TeX style accented chars, etc for use with Survex           */
/* Copyright (C) Olly Betts 1993-1996                                     */

/*
1993.11.20 created
1993.11.26 moved chOpenQuotes and chCloseQuotes to here too
1994.03.13 characters 128-159 translated to \xXX codes to placate compilers
1994.03.23 added caveat comment about top-bit-set characters
1994.12.03 added -DISO8859_1 to makefile to force iso-8859-1
1995.02.14 changed "char foo[]=" to "char *foo="
1996.02.10 pszTable is now an array of char *, which can be NULL
	   szSingTab can also be NULL
*/

/* NB if (as in TeX) \o and \O mean slashed-o and slashed-O, we can't
 * have \oe and \OE for linked-oe and linked-OE without cleverer code.
 * Therefore, I've changed slashed-o and slashed-O to \/o and \/O.
 */

#include "whichos.h"

#if 0
/* Archimedes RISC OS 3.1 - ISO 8859/1 (Latin 1) + extensions in 128-159 */
static char chOpenQuotes='\x94', chCloseQuotes='\x95';
static char *szSingles="lLij";
static char *szSingTab=NULL;
static char *szAccents="`'^\"~=.uvHtcCdbaAsOo/";
static char *szLetters="aeoAiuEIOUyYnNcCwWs";
static char *pszTable[]={
 "‡ËÚ¿Ï˘»Ã“Ÿ",
 "·ÈÛ¡Ì˙…Õ”⁄˝›",
 "‚ÍÙ¬Ó˚ Œ‘€\x86\x85    \x82\x81",
 "‰ÎˆƒÔ¸Àœ÷‹ˇ",
 "„ ı√    ’   Ò—",
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 "              Á",
 "               «",
 NULL,
 "™ ∫",
 "ÂÊ",
 "   ≈  ∆",
 "                  ﬂ",
 "      \x9a",
 " \x9b",
 "  ¯     ÿ"
};

#elif ((OS==RISCOS) || defined(ISO8859_1))
/* Archimedes RISC OS 2.0 - ISO 8859/1 (Latin 1) */
static char chOpenQuotes='\"', chCloseQuotes='\"';
static char *szSingles="lLij";
static char *szSingTab=NULL;
static char *szAccents="`'^\"~=.uvHtcCdbaAsOo/";
static char *szLetters="aeoAiuEIOUyYnNcCwWs";
static char *pszTable[]={
 "‡ËÚ¿Ï˘»Ã“Ÿ",
 "·ÈÛ¡Ì˙…Õ”⁄˝›",
 "‚ÍÙ¬Ó˚ Œ‘€",
 "‰ÎˆƒÔ¸Àœ÷‹ˇ",
 "„ ı√    ’   Ò—",
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 "              Á",
 "               «",
 NULL,
 "™ ∫",
 "ÂÊ",
 "   ≈  ∆",
 "                  ﬂ",
 NULL,
 NULL,
 "  ¯     ÿ"
};

#elif 0
/* MS DOS - PC-8 (code page 437?) */
static char chOpenQuotes='\"', chCloseQuotes='\"';
static char *szSingles="lLij";
static char *szSingTab=NULL;
static char *szAccents="`'^\"~=.uvHtcCdbaAsOo/";
static char *szLetters="aeoAiuEIOUyYnNcCwWs";
static char *pszTable[]={
 "\x85\x8A\x95 \x8D\x97",
 "†\x82¢ °£\x90",
 "\x83\x88\x93 \x8C\x96",
 "\x84\x89\x94\x8E\x8B\x81  \x99\x9A\x98",
 "            §•",
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 "              \x87",
 "               \x80",
 NULL,
 "¶ ß",
 "\x86\x91",
 "   \x8F  \x92",
 "                  ·",
 NULL,
 NULL,
 NULL
};

#elif 0
/* MS DOS - PC-8 Denmark/Norway */
static char chOpenQuotes='\"', chCloseQuotes='\"';
static char *szSingles="lLij";
static char *szSingTab=NULL;
static char *szAccents="`'^\"~=.uvHtcCdbaAsOo/";
static char *szLetters="aeoAiuEIOUyYnNcCwWs";
static char *pszTable[]={
 "\x85\x8A\x95 \x8D\x97",
 "†\x82¢ °£\x90     ¨",
 "\x83\x88\x93 \x8C\x96",
 "\x84\x89\x94\x8E\x8B\x81  \x99\x9A\x98",
 "© ¶™    ß   §•",
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 "              \x87",
 "               \x80",
 NULL,
 NULL,
 "\x86\x91",
 "   \x8F  \x92",
 "                  ·",
 NULL,
 NULL,
 NULL
};

#elif (OS==MSDOS)
/* MS DOS - Code page 850 */
static char chOpenQuotes='\"', chCloseQuotes='\"';
static char *szSingles="lLij";
static char *szSingTab="  ’";
static char *szAccents="`'^\"~=.uvHtcCdbaAsOo/";
static char *szLetters="aeoAiuEIOUyYnNcCwWs";
static char *pszTable[]={
 "\x85\x8A\x95∑\x8D\x97‘ „ÎÏÌ",
 "†\x82¢µ°£\x90÷‡È",
 "\x83\x88\x93∂\x8C\x96“◊‚Í",
 "\x84\x89\x94\x8E\x8B\x81”ÿ\x99\x9A\x98",
 "∆ ‰«    Â   §•",
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 "              \x87",
 "               \x80",
 NULL,
 "¶ ß",
 "\x86\x91",
 "   \x8F  \x92",
 "                  ·",
 NULL,
 NULL,
 "  \x9B     \x9D"
};
#else
/* No special chars... */
# define NO_TEX
#endif
