//
//  avendefs.h
//
//  Various constant definitions used in Aven.
//
//  Copyright (C) 2001, Mark R. Shinwell.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef avendefs_h
#define avendefs_h

extern const int NUM_DEPTH_COLOURS;

extern const unsigned char REDS[];
extern const unsigned char GREENS[];
extern const unsigned char BLUES[];

enum {
    aven_COMMAND_START,
    menu_FILE_OPEN,
    menu_FILE_QUIT,
    menu_ROTATION_START,
    menu_ROTATION_STOP,
    menu_ROTATION_SPEED_UP,
    menu_ROTATION_SLOW_DOWN,
    menu_ROTATION_REVERSE,
    menu_ROTATION_STEP_CCW,
    menu_ROTATION_STEP_CW,
    menu_ORIENT_MOVE_NORTH,
    menu_ORIENT_MOVE_EAST,
    menu_ORIENT_MOVE_SOUTH,
    menu_ORIENT_MOVE_WEST,
    menu_ORIENT_SHIFT_LEFT,
    menu_ORIENT_SHIFT_RIGHT,
    menu_ORIENT_SHIFT_UP,
    menu_ORIENT_SHIFT_DOWN,
    menu_ORIENT_PLAN,
    menu_ORIENT_ELEVATION,
    menu_ORIENT_HIGHER_VP,
    menu_ORIENT_LOWER_VP,
    menu_ORIENT_ZOOM_IN,
    menu_ORIENT_ZOOM_OUT,
    menu_ORIENT_DEFAULTS,
    menu_VIEW_SHOW_LEGS,
    menu_VIEW_SHOW_CROSSES,
    menu_VIEW_SHOW_NAMES,
    menu_VIEW_SHOW_SURFACE,
    menu_VIEW_SURFACE_DEPTH,
    menu_VIEW_SURFACE_DASHED,
    menu_VIEW_SHOW_OVERLAPPING_NAMES,
    menu_VIEW_COMPASS,
    menu_VIEW_CLINO,
    menu_VIEW_DEPTH_BAR,
    menu_VIEW_SCALE_BAR,
    menu_VIEW_STATUS_BAR,
    menu_CTL_REVERSE,
    menu_HELP_ABOUT,
    menu_HELP_ABOUT_CHILD,
    aven_COMMAND_END
};

#endif
