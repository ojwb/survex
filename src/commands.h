/* > commands.h
 * Header file for code for directives
 * Copyright (C) 1994,1995,1996 Olly Betts
 */

/*
1994.04.16 created
1994.04.18 added data() proto and tidied
1994.06.27 added BEGIN and END
1994.08.13 msc barfs if we have a fn named end() - now begin_block/end_block
1994.11.12 added get_token() and match_tok() prototypes
1995.06.06 added size arg to match_tok() ; added TABSIZE() macro
1996.03.24 added set_chars()
*/

extern void handle_command(const char *fnmUsed, const char *fnm);

extern void get_token(int en);

typedef struct { char *sz; int tok; } sztok;
extern int match_tok(sztok *tab, int tab_size);

#define TABSIZE(T) ((sizeof(T)/sizeof(sztok))-1)
