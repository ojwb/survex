/* > datain.h
 * Header file for code that...
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1994,1996,1997 Olly Betts
 */

/*
1994.04.18 created
1994.04.27 added jbSkipLine
1994.05.05 added ch,fh,buffer
1994.05.09 moved errorjmp here from readval.c
1994.06.18 added data_diving(); added showandskipline()
           errorjmp now calls showline or showandskipline
1994.06.20 added int argument to warning, error and fatal
1994.06.29 added skipblanks()
1994.09.08 byte -> uchar
1996.03.24 parse structure introduced
1996.11.03 added STYLE_* action codes
1997.01.23 fixed to work without new datain.c
*/

#ifdef NO_SETJMP
# define errorjmp( EN,A,JB ) fatal((EN),showline,NULL,(A))
#else
# include <setjmp.h>
# define errorjmp( EN,A,JB ) BLK( error((EN),showandskipline,NULL,(A)); longjmp((JB),1); )
#endif

typedef struct {
  FILE *fh;
#ifndef NO_SETJMP
  jmp_buf jbSkipLine;
#endif
} parse;
extern int  ch;
/*extern FILE *fh;*/
extern parse file;

#define nextch() (ch=getc(file.fh))

extern uchar buffer[];

extern void skipblanks(void);

extern void data_file( sz pth, sz fnm ); /* reads complete data file */
extern void UsingDataFile( sz fnmUsed );
extern void skipline( void );
extern void showline(char *dummy,int n);
extern void showandskipline(char *dummy, int n);

/* style functions */
#if 0
extern void data_normal( int /*action*/ );
extern void data_diving( int /*action*/ );
#else
extern void data_normal( void );
extern void data_diving( void );
#endif

#define STYLE_START    0
#define STYLE_READDATA 1
#define STYLE_END      2
