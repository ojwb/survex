/* > prbitmap.h
 * Header files for
 * Bitmap routines for Survex Dot-matrix and Inkjet printer drivers
 * Copyright (C) 1994 Olly Betts
 */

extern void (*PlotDot)(long X, long Y);

extern void DrawLineDots(long x, long y, long x2, long y2);
extern void MoveTo(long x, long y);
extern void DrawTo(long x, long y);
extern void DrawCross(long x, long y);
extern void read_font(const char *pth, const char *leaf, int dpiX, int dpiY);
extern void WriteString(const char *s);
