/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
 * Borrowed from Lee Willis <lee@leewillis.co.uk> that
 * Borrowed heavily from code by Jan Arne Petersen <jpetersen@uni-bonn.de>
 * This projects' homepage is: http://gmpc.wikia.com/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <stdio.h>
#include <gtk/gtktogglebutton.h>
#include "main.h"

#ifndef __MM_KEYS_H
#define __MM_KEYS_H

#define TYPE_MMKEYS            (mmkeys_get_type ())
#define MMKEYS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MMKEYS, MmKeys))
#define MMKEYS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MMKEYS, MmKeysClass))
#define IS_MMKEYS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MMKEYS))
#define IS_MMKEYS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MMKEYS))
#define MMKEYS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MMKEYS, MmKeysClass))

typedef struct _MmKeys      MmKeys;
typedef struct _MmKeysClass MmKeysClass;

struct _MmKeys
{
		GObject parent;
};

struct _MmKeysClass
{
		GObjectClass parent_class;
};

MmKeys *mmkeys_new      (void);

void grab_key(int key, int keycode, unsigned int mask);
extern gmpcPlugin mmkeys_plug;
#endif /* __MM_KEYS_H */


