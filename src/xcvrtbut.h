/* xcvrtbut.h
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

#include <X11/Xlib.h>

extern int button_width, button_height;

typedef void (*button_process)(GC);

void button_init(Display *display, Window window,
		 unsigned long fg, unsigned long bg, GC gc);

void button_create(const char *txt, const char *txt2, int *pstate,
		   button_process fn);

void button_draw(void);

void button_handleevent(XEvent *e);
