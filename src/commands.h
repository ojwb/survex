/* > commands.h
 * Header file for code for directives
 * Copyright (C) 1994,1995,1996 Olly Betts
 */

extern void handle_command(void);
extern void default_all(settings *s);

extern void get_token(void);

typedef struct { const char *sz; int tok; } sztok;
extern int match_tok(const sztok *tab, int tab_size);

#define TABSIZE(T) ((sizeof(T) / sizeof(sztok)) - 1)
