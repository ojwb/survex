/* append a string */
void s_cat(char **pstr, int *plen, char *s);

/* append a character */
void s_catchar(char **pstr, int *plen, char ch);

/* truncate string to zero length */
void s_zero(char **pstr);

void s_free(char **pstr);
