/* Gnome Music Player (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Borrowed from Lee Willis <lee@leewillis.co.uk> that
 * Borrowed heavily from code by Jan Arne Petersen <jpetersen@uni-bonn.de>
 * This projects' homepage is: http://gmpcwiki.sarine.nl/
 
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

#include <config.h>
#ifdef ENABLE_MMKEYS

#include <stdio.h>
#include "mm-keys.h"
#include "eggcellrendererkeys.h"

static void mmkeys_class_init (MmKeysClass *klass);
static void mmkeys_init       (MmKeys      *object);
static void mmkeys_finalize   (GObject     *object);

static int grab_mmkey (int key_code, unsigned int mask, GdkWindow *root);

static GdkFilterReturn filter_mmkeys (GdkXEvent *xevent,
		GdkEvent *event,
		gpointer data);

#define FG_DEFAULT	None
#define FG_ERROR	"red"

/* Members of the treestore */
enum {
	MM_STORE_KEYNAME = 0,
	MM_STORE_INDEX,
	MM_STORE_KEYCODE,
	MM_STORE_MASK,
	MM_STORE_KEYVAL,
	MM_STORE_FOREGROUND,
	MM_STORE_COUNT,
};

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
	MM_SHOW_NOTIFICATION,
    MM_TOGGLE_MUTE,
	LAST_SIGNAL
};

const char *keynames[LAST_SIGNAL] = {
	N_("PlayPause"),	/** MM_PLAYPAUSE */
	N_("Next"),	 	/** MM_NEXT*/
	N_("Previous"),		/** MM_PREV */
	N_("Stop"),		/** MM_STOP */
	N_("Fast Forward"),	/** MM_FASTFORWARD */
	N_("Fast Backward"),	/** MM_FASTBACKWARD */
	N_("Repeat"),		/** MM_REPEAT */
	N_("Random"),		/** MM_RANDOM */
	N_("Raise window"),	/** MM_RAISE */
	N_("Hide window"),	/** MM_HIDE */
	N_("Toggle window"),	/** MM_TOGGLE_HIDDEN */
	N_("Volume Up"),	/** MM_VOLUME_UP */
	N_("Volume Down"),	/** MM_VOLUME_DOWN */
	N_("Show song"),	/** MM_SHOW_NOTIFICATION */
    N_("Toggle Mute"),  /** MM_TOGGLE_MUTE */
};	

static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL];
static int keycodes[LAST_SIGNAL];
static unsigned int masks[LAST_SIGNAL];
static int keyerror[LAST_SIGNAL];

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
	signals[MM_SHOW_NOTIFICATION] = 
		g_signal_new ("mm_show_notification",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,	
				G_TYPE_NONE, 1, G_TYPE_INT); 	
	signals[MM_TOGGLE_MUTE] = 
		g_signal_new ("mm_toggle_mute",
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
	gint i, j;
	int keycode = 0;
	int anyKeybindsFailed = FALSE;
	int anyDuplicatesFound = FALSE;

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
		keyerror[i] = FALSE;
		/* Detect duplicates */
		for(j=0;j<i;j++)
		{
			if (keycodes[i] != 0 &&
				keycodes[i] == keycodes[j] &&
				masks[i] == masks[j])
			{
				anyDuplicatesFound = TRUE;
				keycodes[i] = 0;
				masks[i] = 0;
				keyerror[i] = TRUE;
				cfg_set_single_value_as_int(config, "Keybindings", keynames[i], 0);
				cfg_set_single_value_as_int(config, "Keymasks", keynames[i], 0);
			}
		}
	}

	for (i = 0; i < gdk_display_get_n_screens (display); i++) {
		screen = gdk_display_get_screen (display, i);
		if (screen != NULL) {
			root = gdk_screen_get_root_window (screen);

			for (j = 0; j < LAST_SIGNAL;j++) {
				if (keycodes[j] > 0)
					if( !grab_mmkey (keycodes[j], masks[j], root) )
					{
						keyerror[j] = TRUE;
						anyKeybindsFailed = TRUE;
					}
			}

			gdk_window_add_filter (root, filter_mmkeys, object);
		}
	}

	if (anyKeybindsFailed)
	{
		GString *message = g_string_new (_("Could not grab the following multimedia keys:\n\n"));
		for (i=0;i<LAST_SIGNAL;i++)
		{
			if (keyerror[i] && keycodes[i] != 0)
			{
				gchar *rawkeysym = egg_virtual_accelerator_name (
					XKeycodeToKeysym(GDK_DISPLAY(), keycodes[i], 0),
					keycodes[i], masks[i]);
				gchar *keysym = g_markup_escape_text(rawkeysym, -1);
				q_free (rawkeysym);
				g_string_append_printf( message,
					"\t%s: %s\n",
					_(keynames[i]), keysym );
				q_free (keysym);
			}
		}
		g_string_append( message,
			_("\nEnsure that your window manager (or other applications) have not already bound this key for some other function, then restart gmpc." ));
		show_error_message (message->str, TRUE);
		g_string_free (message, TRUE);
	}

	if (anyDuplicatesFound)
	{
		show_error_message(_("Duplicate mapping(s) detected\n\n"
				"Some duplicate multimedia key mappings were detected, and disabled.  Please revisit the preferences and ensure your settings are now correct."), TRUE );
	}
}

MmKeys * mmkeys_new (void)
{
	return MMKEYS (g_object_new (TYPE_MMKEYS, NULL));
}


static int grab_mmkey (int key_code, unsigned int mask, GdkWindow *root)
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
		debug_printf (DEBUG_INFO, "Error grabbing key %d+%d, %p\n", key_code, mask, root);
		return FALSE;
	}
	return TRUE;
}

static GdkFilterReturn filter_mmkeys (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	unsigned int i;
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
		if(keycodes[i] == (int)(key->keycode) && masks[i] == keystate )
		{
			g_signal_emit (data, signals[i], 0, 0);
			debug_printf(DEBUG_INFO, "%s pressed", keynames[i]);
			return GDK_FILTER_REMOVE;
		}
	}

    return GDK_FILTER_CONTINUE;
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
		debug_printf (DEBUG_INFO, "Error ungrabbing key %d+%d, %p\n", key_code, mask, root);
	}
}



void grab_key(int key, int keycode, unsigned int mask)
{
	GdkDisplay *display;
	GdkScreen *screen;
	GdkWindow *root;
	gint i;
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
	keyerror[key] = FALSE;
	if(keycode >0)
	{
		keycodes[key] = keycode;
		masks[key] = mask;

		for (i = 0; i < gdk_display_get_n_screens (display); i++) {
			screen = gdk_display_get_screen (display, i);
			if (screen != NULL) {
				root = gdk_screen_get_root_window (screen);
				if (!grab_mmkey (keycodes[key], masks[key], root))
				{
					/* Grab failed */
					keyerror[key] = TRUE;
				}
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
	.construct      = mmkeys_pref_construct,
	.destroy        = mmkeys_pref_destroy
};

gmpcPlugin mmkeys_plug = 
 {
	. name          = N_("Multimedia Keys"),
	.version        = {1,1,1},
	.plugin_type    = GMPC_INTERNALL,
	.pref           = &mmkeys_gpp    /* preferences */
};

static void accel_cleared_callback(GtkCellRendererText *cell, const char *path_string, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	int key;

	gtk_tree_model_get_iter (model, &iter, path);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			MM_STORE_KEYCODE,	0,
			MM_STORE_MASK,		0,
			MM_STORE_KEYVAL,	0, 
			MM_STORE_FOREGROUND,	FG_DEFAULT,
			-1);
	gtk_tree_path_free (path);
	gtk_tree_model_get(model, &iter, 1, &key, -1);
	grab_key(key, 0, 0);
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
	int i;

	gtk_tree_model_get_iter (model, &iter, path);

	debug_printf(DEBUG_INFO, "EggCellRenderKeys grabbed %d %u", mask, hardware_keycode );
	if(hardware_keycode == 22)
	{
		hardware_keycode = 0;
	}	                            

	gtk_tree_model_get(model, &iter, 1, &key, -1);

	/* Check for duplicates */
	for (i=0;i<LAST_SIGNAL;i++) {
		if (i == key)
			continue;
		if (hardware_keycode != 0 &&
			keycodes[i] == (int)hardware_keycode &&
			masks[i] == mask)
		{
			gchar *message;
			gchar *rawkeysym = egg_virtual_accelerator_name (
				XKeycodeToKeysym(GDK_DISPLAY(), hardware_keycode, 0),
				hardware_keycode, mask);
			gchar *keysym = g_markup_escape_text(rawkeysym, -1);
			q_free (rawkeysym);
			message = g_strdup_printf( _("<b>Duplicate mapping detected</b>\n\n"
					"%s is already mapped to %s"),
					keysym, _(keynames[i]) );
			q_free (keysym);
			show_error_message (message, TRUE);
			q_free (message);
			
			/* Clear the duplicate entry */
			accel_cleared_callback(cell, path_string, data);
			return;
		}
	}

	grab_key(key, hardware_keycode, mask);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			MM_STORE_KEYCODE,	hardware_keycode,
			MM_STORE_MASK,		mask,
			MM_STORE_KEYVAL,	keyval, 
			MM_STORE_FOREGROUND,	keyerror[key] ? FG_ERROR : FG_DEFAULT,
			-1);
	gtk_tree_path_free (path);
	if( keyerror[key] )
	{
		gchar *message;
		gchar *rawkeysym = egg_virtual_accelerator_name (
			XKeycodeToKeysym(GDK_DISPLAY(), keycodes[key], 0),
			keycodes[key], masks[key]);
		gchar *keysym = g_markup_escape_text(rawkeysym, -1);
		q_free (rawkeysym);
		message = g_strdup_printf(
			_("Could not grab multimedia key:\n\n"
			"\t%s: %s\n\n"
			"Ensure that your window manager (or other applications) have not already bound this key for some other function, then restart gmpc."),
			_(keynames[key]), keysym );
		q_free (keysym);
		show_error_message (message, TRUE);
		q_free (message);
	}
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
	q_free(path);
	if(mmkeys_pref_xml)
	{
		int i=0;
		GtkWidget *vbox = glade_xml_get_widget(mmkeys_pref_xml, "mmkeys-vbox");
		GtkTreeViewColumn *column = NULL;
		GtkListStore *store = gtk_list_store_new(MM_STORE_COUNT,
			G_TYPE_STRING,	/* MM_STORE_KEYNAME	*/
			G_TYPE_INT,	/* MM_STORE_INDEX	*/
			G_TYPE_UINT,	/* MM_STORE_KEYCODE	*/
			G_TYPE_UINT,	/* MM_STORE_MASK	*/
			G_TYPE_UINT,	/* MM_STORE_KEYVAL	*/
			G_TYPE_STRING	/* MM_STORE_FOREGROUND	*/
			);
		GtkCellRenderer *rend =gtk_cell_renderer_text_new();
		gtk_tree_view_set_model(GTK_TREE_VIEW (glade_xml_get_widget(mmkeys_pref_xml, "mmkeys-tree")), GTK_TREE_MODEL(store));

		column = gtk_tree_view_column_new();
		gtk_tree_view_column_pack_start(column, rend, TRUE);
		gtk_tree_view_column_add_attribute(column, rend,
			"text", MM_STORE_KEYNAME);
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
				"keycode",	MM_STORE_KEYCODE,
				"accel_mask",	MM_STORE_MASK,
				"accel_key",	MM_STORE_KEYVAL,
				"foreground",	MM_STORE_FOREGROUND,
				NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (glade_xml_get_widget(mmkeys_pref_xml, "mmkeys-tree")), column);

		gtk_container_add(GTK_CONTAINER(container),vbox);
		for(i=0;i< LAST_SIGNAL;i++)
		{
			GtkTreeIter iter;
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
				MM_STORE_KEYNAME,	_(keynames[i]),
				MM_STORE_INDEX,		i,
				MM_STORE_KEYCODE,	keycodes[i],
				MM_STORE_MASK,		masks[i],
				MM_STORE_KEYVAL,	XKeycodeToKeysym(GDK_DISPLAY(), keycodes[i], 0), 
				MM_STORE_FOREGROUND,	keyerror[i] ? FG_ERROR : FG_DEFAULT,
				-1);
		}
	}

}



#endif
