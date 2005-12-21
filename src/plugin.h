#include "config1.h"
#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#ifndef __GMPC_PLUGIN_H__
#define __GMPC_PLUGIN_H__

#define PLUGIN_ID_MARK 1024
#define PLUGIN_ID_INTERNALL 2048
extern MpdObj *connection;
extern config_obj *config;

/* Plugin Type's */
typedef enum {
	GMPC_PLUGIN_DUMMY	= 1,
	GMPC_PLUGIN_PL_BROWSER	= 2,
	GMPC_PLUGIN_NO_GUI	= 4,
	GMPC_INTERNALL		= 8
} PluginType;



/* usefull defines */
#define PL3_ENTRY_ALBUM 64
#define PL3_ENTRY_ARTIST 32
#define PL3_ENTRY_DIRECTORY 16
#define PL3_CUR_PLAYLIST 8
#define PL3_ENTRY_STREAM 4
#define PL3_ENTRY_PLAYLIST 2
#define PL3_ENTRY_SONG 1

/* the gtk_tree_store row's */
enum
{
	PL3_CAT_TYPE,
	PL3_CAT_TITLE,
	PL3_CAT_INT_ID,
	PL3_CAT_ICON_ID,
	PL3_CAT_PROC, /* for the lazy tree, if the dir is allready processed */
	PL3_CAT_ICON_SIZE,
	PL3_CAT_BROWSE_FORMAT,
	PL3_CAT_NROWS
} pl3_cat_store;



/* structures */
/** gmpcPrefPlugin: need to be instant apply.
 * Plugin is expected to fill the container with it preferences widgets. (gtk_container_add)
 * if destroy is called, it needs to detacht it from the container (gtk_container_remove)
 */
typedef struct {
	void (*construct)(GtkWidget *container);
	void (*destroy)(GtkWidget *container);
} gmpcPrefPlugin;

/** gmpcPlBrowserPlugin, functions needed for intergration with the playlist browser
 */
typedef struct {
	void (*add)(GtkWidget *cat_tree);
	void (*selected)(GtkWidget *container);
	void (*unselected)(GtkWidget *container);
	void (*cat_selection_changed)(GtkWidget *tree, GtkTreeIter *iter);
	void (*fill_tree)(GtkTreeView *tree,GtkTreeIter *iter);
	int (*cat_right_mouse_menu)(GtkWidget *menu, int type, GtkWidget *tree,GdkEventButton *event);
	void (*cat_key_press)(GtkWidget *tree, GdkEventKey *event, int selected_type);
	/* not yet implemented */
} gmpcPlBrowserPlugin;




/* sturcture */
typedef struct {
	char			*name;		/* Name of the plugin */
	int			version[3];	/* Version number */
	PluginType		plugin_type;	/* Type of Plugin */
	/* unique plugin id */
	int			id; /* do not fill in, is done by gmpc */
	/* path where the plugin is (only directory) can be used to get location of f.e. glade/images */
	char			*path;	/* Do not fill in, done by gmpc */
	/* Browser Plugins */
	gmpcPlBrowserPlugin	*browser;
	/* plugin with one signal for changes on Mpd */
	StatusChangedCallback	mpd_status_changed;
	/* (dis)connect signal */
	ConnectionChangedCallback mpd_connection_changed;
	/* structure to let the plugin intergrate it's preferences */
	gmpcPrefPlugin		*pref;
} gmpcPlugin;




#endif
