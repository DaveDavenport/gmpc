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
#include "eggcellrendererkeys.h"

static void mmkeys_class_init (MmKeysClass *klass);
static void mmkeys_init       (MmKeys      *object);
static void mmkeys_finalize   (GObject     *object);

static void grab_mmkey (int key_code, unsigned int mask, GdkWindow *root);

static GdkFilterReturn filter_mmkeys (GdkXEvent *xevent,
		GdkEvent *event,
		gpointer data);

enum {
	MM_PLAYPAUSE,
	MM_NEXT,
	MM_PREV,
	MM_STOP,
	MM_FASTFORWARD,
	MM_FASTBACKWARD,
	MM_REPEAT,
	MM_RANDOM,
	MM_RAISE,
	MM_HIDE,
	MM_TOGGLE_HIDDEN,
	MM_VOLUME_UP,
	MM_VOLUME_DOWN,
	LAST_SIGNAL
};

char *keynames[LAST_SIGNAL] = {
	"PlayPause",/** MM_PLAYPAUSE */
	"Next", 	/** MM_NEXT*/
	"Previous", /** MM_PREV */
	"Stop",		/** MM_STOP */
	"Fast Forward", /** MM_FASTFORWARD */
	"Fast Backward", /** MM_FASTBACKWARD */
	"Repeat", /** MM_REPEAT */
	"Random", /** MM_RANDOM */
	"Raise window", /** MM_RAISE */
	"Hide window", /** MM_HIDE */
	"Toggle window", /** MM_TOGGLE_HIDDEN */
	"Volume Up", /** MM_VOLUME_UP */
	"Volume Down" /** MM_VOLUME_DOWN */
};	

static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL];
static int keycodes[LAST_SIGNAL];
static unsigned int masks[LAST_SIGNAL];

static GType type = 0;

static GType mmkeys_get_type (void)
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
	signals[MM_FASTFORWARD] = 
		g_signal_new ("mm_fastforward",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);
	signals[MM_FASTBACKWARD]= 
		g_signal_new ("mm_fastbackward",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);
	signals[MM_REPEAT]= 
		g_signal_new ("mm_repeat",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);
	signals[MM_RANDOM]= 
		g_signal_new ("mm_random",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,	
				G_TYPE_NONE, 1, G_TYPE_INT); 	
	signals[MM_RAISE]= 
		g_signal_new ("mm_raise",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,	
				G_TYPE_NONE, 1, G_TYPE_INT); 	
	signals[MM_HIDE]= 
		g_signal_new ("mm_hide",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,	
				G_TYPE_NONE, 1, G_TYPE_INT); 	
	signals[MM_TOGGLE_HIDDEN]= 
		g_signal_new ("mm_toggle_hidden",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,	
				G_TYPE_NONE, 1, G_TYPE_INT); 	
	signals[MM_VOLUME_UP] = 
		g_signal_new ("mm_volume_up",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,	
				G_TYPE_NONE, 1, G_TYPE_INT); 	

	signals[MM_VOLUME_DOWN] = 
		g_signal_new ("mm_volume_down",
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
	masks[MM_PLAYPAUSE] = cfg_get_single_value_as_int_with_default(config, "Keymasks", keynames[MM_PLAYPAUSE], 0);
	/**
	 * Previous
	 */
	keycode = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPrev);
	keycodes[MM_PREV] = cfg_get_single_value_as_int_with_default(config, "Keybindings", keynames[MM_PREV], keycode);
	masks[MM_PREV] = cfg_get_single_value_as_int_with_default(config, "Keymasks", keynames[MM_PREV], 0);
	/**
	 * Next 
	 */
	keycode = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioNext);
	keycodes[MM_NEXT] = cfg_get_single_value_as_int_with_default(config, "Keybindings", keynames[MM_NEXT], keycode);
	masks[MM_NEXT] = cfg_get_single_value_as_int_with_default(config, "Keymasks", keynames[MM_NEXT], 0);
	/**
	 * Stop
	 */
	keycode = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioStop);
	keycodes[MM_STOP] = cfg_get_single_value_as_int_with_default(config, "Keybindings", keynames[MM_STOP], keycode);
	masks[MM_STOP] = cfg_get_single_value_as_int_with_default(config, "Keymasks", keynames[MM_STOP], 0);

	for(i=0;i<LAST_SIGNAL;i++)
	{
		keycodes[i] = cfg_get_single_value_as_int_with_default(config, "Keybindings", keynames[i], 0);
		masks[i] = cfg_get_single_value_as_int_with_default(config, "Keymasks", keynames[i], 0);
	}


	
	for (i = 0; i < gdk_display_get_n_screens (display); i++) {
		screen = gdk_display_get_screen (display, i);
		if (screen != NULL) {
			root = gdk_screen_get_root_window (screen);

			for (j = 0; j < LAST_SIGNAL;j++) {
				if (keycodes[j] > 0)
				{
					grab_mmkey (keycodes[j], masks[j], root);
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


static void grab_mmkey (int key_code, unsigned int mask, GdkWindow *root)
{
	gdk_error_trap_push ();

	XGrabKey (GDK_DISPLAY (), key_code,
			mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod5Mask | mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			LockMask | mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | Mod5Mask | mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | LockMask | mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod5Mask | LockMask | mask,
			GDK_WINDOW_XID (root), True,
			GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | Mod5Mask | LockMask | mask,
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
	unsigned int keystate;

	xev = (XEvent *) xevent;
	if (xev->type != KeyPress) {
		return GDK_FILTER_CONTINUE;
	}

	key = (XKeyEvent *) xevent;
	keystate = key->state & ~(Mod2Mask | Mod5Mask | LockMask);
	for(i=0; i < LAST_SIGNAL;i++)
	{
		if(keycodes[i] == key->keycode && masks[i] == keystate )
		{
			g_signal_emit (data, signals[i], 0, 0);
			debug_printf(DEBUG_INFO, "%s pressed", keynames[i]);
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


static void ungrab_mmkey (int key_code, int mask, GdkWindow *root)
{
	gdk_error_trap_push ();

	XUngrabKey (GDK_DISPLAY (), key_code,
			mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod5Mask | mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			LockMask | mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | Mod5Mask | mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | LockMask | mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod5Mask | LockMask | mask,
			GDK_WINDOW_XID (root));
	XUngrabKey (GDK_DISPLAY (), key_code,
			Mod2Mask | Mod5Mask | LockMask | mask,
			GDK_WINDOW_XID (root));

	gdk_flush ();
	if (gdk_error_trap_pop ()) {
		fprintf (stderr, "Error grabbing key %d, %p\n", key_code, root);
	}
}



void grab_key(int key, int keycode, unsigned int mask)
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
				ungrab_mmkey (keycodes[key], masks[key], root);
			}
		}
	}
	keycodes[key] = 0;
	masks[key] = 0;
	if(keycode >0)
	{
		keycodes[key] = keycode;
		masks[key] = mask;

		for (i = 0; i < gdk_display_get_n_screens (display); i++) {
			screen = gdk_display_get_screen (display, i);
			if (screen != NULL) {
				root = gdk_screen_get_root_window (screen);
				grab_mmkey (keycodes[key], masks[key], root);
			}
		}
	}
	cfg_set_single_value_as_int(config,"Keybindings", keynames[key], keycodes[key]); 
	cfg_set_single_value_as_int(config,"Keymasks", keynames[key], masks[key]); 
}

/*****
 * Multimedia key plugin for config panel 
 */
void mmkeys_pref_destroy(GtkWidget *container);
void mmkeys_pref_construct(GtkWidget *container);
GladeXML *mmkeys_pref_xml = NULL;

gmpcPrefPlugin mmkeys_gpp = {
	mmkeys_pref_construct,
	mmkeys_pref_destroy
};

gmpcPlugin mmkeys_plug = {
	"Multimedia Keys",
	{1,1,1},
	GMPC_INTERNALL,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&mmkeys_gpp,
	NULL,
	NULL,
	NULL
};

static void accel_cleared_callback(GtkCellRendererText *cell, const char *path_string, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	int key;

	gtk_tree_model_get_iter (model, &iter, path);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			2, 0,
			3,0,
			4,0,
			-1);
	gtk_tree_path_free (path);
	gtk_tree_model_get(model, &iter, 1, &key, -1);
	grab_key(key, 0, 0);
	cfg_set_single_value_as_int(config,"Keybindings", keynames[key], keycodes[key]); 
	cfg_set_single_value_as_int(config,"Keymasks", keynames[key], masks[key]); 
}

	static void
accel_edited_callback (GtkCellRendererText *cell,
		const char          *path_string,
		guint                keyval,
		GdkModifierType      mask,
		guint                hardware_keycode,
		gpointer             data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	int key;

	gtk_tree_model_get_iter (model, &iter, path);

	debug_printf(DEBUG_INFO, "EggCellRenderKeys grabbed %d %u (%s)", mask, hardware_keycode, egg_virtual_accelerator_name(keyval,hardware_keycode,mask) );
	if(hardware_keycode == 22)
	{
		hardware_keycode = 0;
	}	                            

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			2, hardware_keycode,
			3, mask,
			4, keyval,
			-1);
	gtk_tree_path_free (path);
	gtk_tree_model_get(model, &iter, 1, &key, -1);
	grab_key(key, hardware_keycode, mask);

}








void mmkeys_pref_destroy(GtkWidget *container)
{
	if(mmkeys_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(mmkeys_pref_xml, "mmkeys-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(mmkeys_pref_xml);
		mmkeys_pref_xml = NULL;
	}
}

void mmkeys_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	mmkeys_pref_xml = glade_xml_new(path, "mmkeys-vbox",NULL);
	g_free(path);
	if(mmkeys_pref_xml)
	{
		int i=0;
		GtkWidget *vbox = glade_xml_get_widget(mmkeys_pref_xml, "mmkeys-vbox");
		GtkTreeViewColumn *column = NULL;
		GtkListStore *store = gtk_list_store_new(5, G_TYPE_STRING,G_TYPE_INT, G_TYPE_UINT,G_TYPE_UINT,G_TYPE_UINT);
		GtkCellRenderer *rend =gtk_cell_renderer_text_new();

		gtk_tree_view_set_model(GTK_TREE_VIEW (glade_xml_get_widget(mmkeys_pref_xml, "mmkeys-tree")), GTK_TREE_MODEL(store));

		column = gtk_tree_view_column_new();
		gtk_tree_view_column_pack_start(column, rend, TRUE);
		gtk_tree_view_column_add_attribute(column, rend, "text", 0);
		gtk_tree_view_column_set_title(column, _("Action"));
		gtk_tree_view_append_column(GTK_TREE_VIEW (glade_xml_get_widget(mmkeys_pref_xml, "mmkeys-tree")), column);

		rend =  egg_cell_renderer_keys_new ();
		column = gtk_tree_view_column_new ();

		/*		g_object_set (G_OBJECT (rend), "accel_mode", EGG_CELL_RENDERER_KEYS_MODE_X);*/
		egg_cell_renderer_keys_set_accel_mode(EGG_CELL_RENDERER_KEYS(rend), EGG_CELL_RENDERER_KEYS_MODE_GTK);
		g_object_set (G_OBJECT (rend), "editable", TRUE, NULL);

		g_signal_connect (G_OBJECT (rend),
				"accel_edited",
				G_CALLBACK (accel_edited_callback),
				store);                            		

		g_signal_connect (G_OBJECT (rend),
				"accel_cleared",
				G_CALLBACK (accel_cleared_callback),
				store);                            				
		gtk_tree_view_column_pack_start (column, rend,
				TRUE);
		gtk_tree_view_column_set_title(column, _("Shortcut"));
		gtk_tree_view_column_set_attributes (column, rend,
				"keycode", 2,
				"accel_mask", 3,
				"accel_key",4,
				NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (glade_xml_get_widget(mmkeys_pref_xml, "mmkeys-tree")), column);

		gtk_container_add(GTK_CONTAINER(container),vbox);
		for(i=0;i< LAST_SIGNAL;i++)
		{
			GtkTreeIter iter;
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
				0, keynames[i],
				1, i,
				2, keycodes[i],
				3, masks[i],
				4, XKeycodeToKeysym(GDK_DISPLAY(), keycodes[i], 0), 
				-1);
		}
	}

}



#endif
