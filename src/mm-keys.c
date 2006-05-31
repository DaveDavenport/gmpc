/*
 * Copyright (C) 2004-2006 Qball Cow <Qball@qballcow.nl>
 * Borrowed from Lee Willis <lee@leewillis.co.uk> that
 * Borrowed heavily from code by Jan Arne Petersen <jpetersen@uni-bonn.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <config.h>
#ifdef ENABLE_MMKEYS

#include <stdio.h>
#include "mm-keys.h"
#include "main.h"

static void mmkeys_class_init (MmKeysClass *klass);
static void mmkeys_init       (MmKeys      *object);
static void mmkeys_finalize   (GObject     *object);

static void grab_mmkey (int key_code, GdkWindow *root);

static GdkFilterReturn filter_mmkeys (GdkXEvent *xevent,
		GdkEvent *event,
		gpointer data);

enum {
	MM_PLAYPAUSE,
	MM_NEXT,
	MM_PREV,
	MM_STOP,
	LAST_SIGNAL
};

char *keynames[LAST_SIGNAL] = {
	"PlayPause",/** MM_PLAYPAUSE */
	"Next", 	/** MM_NEXT*/
	"Previous", /** MM_PREV */
	"Stop" 		/** MM_STOP */
};

static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL];
static int keycodes[LAST_SIGNAL];

static GType type = 0;

GType mmkeys_get_type (void)
{
	if (!type) {
		static const GTypeInfo info = {
			sizeof (MmKeysClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) mmkeys_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MmKeys),
			0,
			(GInstanceInitFunc) mmkeys_init,
		};

		type = g_type_register_static (G_TYPE_OBJECT, "MmKeys",&info, 0);
	}

	return type;
}

static void mmkeys_class_init (MmKeysClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	object_class->finalize = mmkeys_finalize;

	signals[MM_PLAYPAUSE] =
		g_signal_new ("mm_playpause",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);
	signals[MM_PREV] =
		g_signal_new ("mm_prev",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);
	signals[MM_NEXT] =
		g_signal_new ("mm_next",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);
	signals[MM_STOP] =
		g_signal_new ("mm_stop",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);
}

static void mmkeys_finalize (GObject *object)
{
	parent_class->finalize (G_OBJECT(object));
}

static void mmkeys_init (MmKeys *object)
{
	GdkDisplay *display;
	GdkScreen *screen;
	GdkWindow *root;
	guint i, j;
	int keycode = 0;

	display = gdk_display_get_default ();

	/** Play Pause */
	keycode = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPlay);
	keycodes[MM_PLAYPAUSE] = cfg_get_single_value_as_int_with_default(config, "Keybindings", keynames[MM_PLAYPAUSE], keycode);
	/**
	 * Previous
	 */
	keycode = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPrev);
	keycodes[MM_PREV] = cfg_get_single_value_as_int_with_default(config, "Keybindings", keynames[MM_PREV], keycode);
	/**
	 * Next 
	 */
	keycode = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioNext);
	keycodes[MM_NEXT] = cfg_get_single_value_as_int_with_default(config, "Keybindings", keynames[MM_NEXT], keycode);
	/**
	 * Stop
	 */
	keycode = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioStop);
	keycodes[MM_STOP] = cfg_get_single_value_as_int_with_default(config, "Keybindings", keynames[MM_STOP], keycode);

	for (i = 0; i < gdk_display_get_n_screens (display); i++) {
		screen = gdk_display_get_screen (display, i);
		if (screen != NULL) {
			root = gdk_screen_get_root_window (screen);

			for (j = 0; j < LAST_SIGNAL;j++) {
				if (keycodes[j] > 0)
				{
					printf("grab: %i\n", keycodes[j]);
					grab_mmkey (keycodes[j], root);
				}
			}

			gdk_window_add_filter (root, filter_mmkeys, object);
		}
	}
}

MmKeys * mmkeys_new (void)
{
	return MMKEYS (g_object_new (TYPE_MMKEYS, NULL));
}


static void grab_mmkey (int key_code, GdkWindow *root)
{
	gdk_error_trap_push ();

	XGrabKey (GDK_DISPLAY (), key_code,
			0,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod5Mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			LockMask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | Mod5Mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | LockMask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod5Mask | LockMask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | Mod5Mask | LockMask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);

	gdk_flush ();
	if (gdk_error_trap_pop ()) {
		fprintf (stderr, "Error grabbing key %d, %p\n", key_code, root);
	}
}

static GdkFilterReturn filter_mmkeys (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	int i;
	XEvent *xev;
	XKeyEvent *key;

	xev = (XEvent *) xevent;
	if (xev->type != KeyPress) {
		return GDK_FILTER_CONTINUE;
	}

	key = (XKeyEvent *) xevent;
	printf("%i\n", key->keycode);
	for(i=0; i < LAST_SIGNAL;i++)
	{
		if(keycodes[i] == key->keycode)
		{
			g_signal_emit (data, signals[i], 0, 0);
			return GDK_FILTER_REMOVE;
		}
	}
	/*
	if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPlay) == key->keycode) {
		g_signal_emit (data, signals[MM_PLAYPAUSE], 0, 0);
		return GDK_FILTER_REMOVE;
	} else if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPause) == key->keycode) {
		g_signal_emit (data, signals[MM_PLAYPAUSE], 0, 0);
		return GDK_FILTER_REMOVE;
	} else if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPrev) == key->keycode) {
		g_signal_emit (data, signals[MM_PREV], 0, 0);
		return GDK_FILTER_REMOVE;
	} else if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioNext) == key->keycode) {
		g_signal_emit (data, signals[MM_NEXT], 0, 0);
		return GDK_FILTER_REMOVE;
	} else if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioStop) == key->keycode) {
		g_signal_emit (data, signals[MM_STOP], 0, 0);
		return GDK_FILTER_REMOVE;
	} else {
	*/
		return GDK_FILTER_CONTINUE;
	/*}*/
}


static void ungrab_mmkey (int key_code, GdkWindow *root)
{
	gdk_error_trap_push ();

	XUngrabKey (GDK_DISPLAY (), key_code,
			0,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod5Mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			LockMask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | Mod5Mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | LockMask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod5Mask | LockMask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | Mod5Mask | LockMask,
			GDK_WINDOW_XID (root));

	gdk_flush ();
	if (gdk_error_trap_pop ()) {
		fprintf (stderr, "Error grabbing key %d, %p\n", key_code, root);
	}
}



void grab_key(int key, int keycode)
{
	GdkDisplay *display;
	GdkScreen *screen;
	GdkWindow *root;
	guint i;
	display = gdk_display_get_default ();

	/* remove old key */
	if(keycodes[key] > 0)
	{
		for (i = 0; i < gdk_display_get_n_screens (display); i++) {
			screen = gdk_display_get_screen (display, i);
			if (screen != NULL) {
				root = gdk_screen_get_root_window (screen);
				ungrab_mmkey (keycodes[key], root);
			}
		}
	}
	keycodes[key] = 0;
	if(keycode >0)
	{
		keycodes[key] = keycode;
		cfg_set_single_value_as_int(config,"Keybindings", keynames[key], keycodes[key]); 
		for (i = 0; i < gdk_display_get_n_screens (display); i++) {
			screen = gdk_display_get_screen (display, i);
			if (screen != NULL) {
				root = gdk_screen_get_root_window (screen);
				grab_mmkey (keycodes[key], root);
			}
		}
	}
}

/*****
 * Multimedia key plugin for config panel 
 */




































#endif
