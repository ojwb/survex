/* xcvrtbut.c
 * Copyright (C) 1993-2001 Bill Purvis, Olly Betts, John Pybus, Mark Shinwell,
 * Leandro Dybal Bertoni, Andy Holtsbery, et al
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Original license on code Copyright 1993 Bill Purvis:
 *
 * Bill Purvis DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * Bill Purvis BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xcvrtbut.h"

static Display *mydisplay;
static Window mywindow;

static unsigned int button_count = 0;
static unsigned long button_fg, button_bg;
static GC button_gc, button_invgc;
static int button_created = 0;

int button_width, button_height;

struct button_info {
   const char *txt;
   const char *txt2;
   Window window;
   button_process process;
   int *pstate;
};

static struct button_info buttons[32];

void
button_init(Display *display, Window window,
	    unsigned long fg, unsigned long bg, GC gc)
{
   static XFontStruct *f;

   mydisplay = display;
   mywindow = window;

   button_count = 0;
   button_fg = fg;
   button_bg = bg;

   f = XQueryFont(display, XGContextFromGC(gc));
   if (f) {
      button_height = f->max_bounds.ascent + 4;
   } else {
      button_height = 16;
   }

   button_gc = gc;
   button_invgc = XCreateGC(display, mywindow, 0, 0);
   XCopyGC(display, button_gc, GCFont, button_invgc);

   XSetForeground(display, button_invgc, bg);
   XSetBackground(display, button_invgc, fg);
}

void
button_create(const char *txt, const char *txt2, int *pstate,
	      button_process fn)
{
   if (button_count >= (sizeof(buttons) / sizeof(buttons[0]))) {
      printf("more buttons than space was reserved for\n");
      exit(1);
   }
   buttons[button_count].txt = txt;
   buttons[button_count].txt2 = txt2;
   buttons[button_count].process = fn;
   buttons[button_count].pstate = pstate;
   button_count++;
}

static void
button_flip(Window button, GC normalgc, GC inversegc, const char *string)
{
   int len = strlen(string);
   int width;
   int hoffset;
   XFontStruct * fs;

   fs = XQueryFont(mydisplay, XGContextFromGC(normalgc));
   width = XTextWidth(fs, string, len);
   XFreeFontInfo(NULL, fs, 1);
   hoffset = (button_width - width) / 2;
   if (hoffset < 0) hoffset = 0;

   XClearWindow(mydisplay, button);
   XFillRectangle(mydisplay, button, normalgc, 0, 0, button_width,
		  button_height);
   XDrawImageString(mydisplay, button, inversegc, hoffset, button_height - 3,
		    string, len);
   XFlush(mydisplay);
}

void
button_draw(void)
{
   unsigned int i;
   if (!button_created) {
      XFontStruct * fs = XQueryFont(mydisplay, XGContextFromGC(button_gc));
      button_width = 16;

      for (i = 0; i < button_count; i++) {
	 const char *s = buttons[i].txt;
	 int w = XTextWidth(fs, s, strlen(s));
	 if (w > button_width) button_width = w;
	 s = buttons[i].txt2;
	 if (s) {
	    w = XTextWidth(fs, s, strlen(s));
	    if (w > button_width) button_width = w;
	 }
      }

      XFreeFontInfo(NULL, fs, 1);

      button_width += 4;

      for (i = 0; i < button_count; i++) {
	 Window b;
	 b = XCreateSimpleWindow(mydisplay, mywindow, button_width * i, 0,
				 button_width, button_height,
				 1, button_fg, button_bg);
	 XSelectInput(mydisplay, b,
		      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
	 XMapRaised(mydisplay, b);
	 buttons[i].window = b;
      }

      button_created = 1;
   }

   for (i = 0; i < button_count; i++) {
      const char *s = buttons[i].txt;
      if (buttons[i].txt2 && buttons[i].pstate && *buttons[i].pstate)
	 s = buttons[i].txt2;
      button_flip(buttons[i].window, button_invgc, button_gc, s);
   }
}

void
button_handleevent(XEvent *e)
{
   unsigned int i;
   Window b = e->xbutton.window;

   for (i = 0; i < button_count; i++) {
      if (buttons[i].window == b) {
	 const char *s = buttons[i].txt;
	 if (buttons[i].txt2 && buttons[i].pstate && *buttons[i].pstate)
	    s = buttons[i].txt2;
	 button_flip(b, button_gc, button_invgc, s);

	 if (buttons[i].process)
	    buttons[i].process(button_gc);
	 else if (buttons[i].pstate)
	    *buttons[i].pstate = !*buttons[i].pstate;

	 s = buttons[i].txt;
	 if (buttons[i].txt2 && buttons[i].pstate && *buttons[i].pstate)
	    s = buttons[i].txt2;
	 button_flip(b, button_invgc, button_gc, s);
	 return;
      }
   }
}
