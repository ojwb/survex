/* rotplot.h */

/* Copyright (C) Olly Betts 1994 */

/*
1994.06.21 created rotplot.h
1994.09.11 added draw_scale_bar()
*/

extern void set_view(float sc, float theta, float z);
extern void draw_view_legs( lid Huge *plid );
extern void draw_view_stns( lid Huge *plid );
extern void draw_view_labs( lid Huge *plid );
extern void draw_scale_bar( void );
