#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include "metadata.h"
#include "gmpc-profiles.h"

#ifndef __GMPC_PLUGIN_H__
#define __GMPC_PLUGIN_H__

#define PLUGIN_ID_MARK 1024
#define PLUGIN_ID_INTERNALL 2048
extern MpdObj *connection;
extern config_obj *config;
extern GmpcProfiles *gmpc_profiles;

/* Plugin Type's */
typedef enum {
	GMPC_PLUGIN_DUMMY		= 1,
	GMPC_PLUGIN_PL_BROWSER	= 2,
	GMPC_PLUGIN_NO_GUI		= 4,
	GMPC_INTERNALL			= 8,
	GMPC_DEPRECATED			= 16,
	GMPC_PLUGIN_META_DATA	= 32
} PluginType;

/* usefull defines */
#define PL3_ENTRY_DIR_UP 128
#define PL3_ENTRY_ALBUM 64
#define PL3_ENTRY_ARTIST 32
#define PL3_ENTRY_DIRECTORY 16
#define PL3_CUR_PLAYLIST 8
#define PL3_ENTRY_STREAM 4
#define PL3_ENTRY_PLAYLIST 2
#define PL3_ENTRY_SONG 1

/* the gtk_tree_store row's */
typedef enum
{
	PL3_CAT_TYPE, /** Plugin id */
	PL3_CAT_TITLE, /** title that is showed */
	PL3_CAT_INT_ID, /* id */
	PL3_CAT_ICON_ID, /* icon id */
	PL3_CAT_PROC, /* for the lazy tree, if the dir is allready processed */
	PL3_CAT_ICON_SIZE, /** icon size */
	PL3_CAT_BROWSE_FORMAT, /** string, added for tag browser */
	PL3_CAT_ORDER, /* int for sorting the list */
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
	/**
	 * Adding to the left side tree */
	void (*add)						(GtkWidget *cat_tree);
	/**
	 * If selected,  you need to fill the right screen */
	void (*selected)				(GtkWidget *container);
	/** 
	 * if unselected, you need to remove youself from the container
	 */
	void (*unselected)				(GtkWidget *container);
	/**
	 * if selection changed, but still the same plugin 
	 */
	void (*cat_selection_changed)	(GtkWidget *tree, GtkTreeIter *iter);
	/**
	 * if the user expands the tree.
	 */
	void (*cat_row_expanded)		(GtkWidget *tree,GtkTreeIter *iter);
	int  (*cat_right_mouse_menu)	(GtkWidget *menu, int type, GtkWidget *tree,GdkEventButton *event);
	void (*cat_key_press)			(GtkWidget *tree, GdkEventKey *event, int selected_type);
	/****** GO MENU ********/
	int  (*add_go_menu)				(GtkWidget *menu); 
	/****** Key presses (in the whole window) **/
	int  (*key_press_event)			(GtkWidget *mw, GdkEventKey *event, int type);
} gmpcPlBrowserPlugin;

typedef struct {
	int (*get_priority)				(void);
	int (*get_image)				(mpd_Song *song, MetaDataType type, char **path);
} gmpcMetaDataPlugin;

/* Unique number */
/* 16 == 0.14 release */
#define PLUGIN_API_VERSION 17

/* sturcture */
typedef struct {
	/* Name of the plugin */
	char							*name;		
	/* Version number */
	int								version[3];	
	/* Type of Plugin */
	PluginType						plugin_type;	
	/* unique plugin id */
	/* do not fill in, is done by gmpc */
	int								id; 

	/* path where the plugin is (only directory) can be used to get location of f.e. glade/images 
	 * Don't use this anymore use: gmpc_get.*_path */
	/* Do not fill in, done by gmpc */
	char							*path;	
	/* function gets called on startup */
	void							(*init)(void);
	/* Plugin should destroy all it's widgets and free all allocated space */
	void                    		(*destroy)(void);
	/* Browser Plugins */
	gmpcPlBrowserPlugin				*browser;
	/* plugin with one signal for changes on Mpd */
	StatusChangedCallback			mpd_status_changed;
	/* (dis)connect signal */
	ConnectionChangedCallback 		mpd_connection_changed;
	/* structure to let the plugin intergrate it's preferences */
	gmpcPrefPlugin					*pref;
	/** Meta data */
	gmpcMetaDataPlugin 				*metadata;

	/** Plugin control functions
	 */
	int	 							(*get_enabled)(void);
	void							(*set_enabled)(int enable);
	/* Function that is called when the plugin is going to be destroyed, 
	 * This is the place if you want to save settings
	 */
	void							(*save_yourself)(void);
} gmpcPlugin;

/** plugin functions */
gmpcPlugin * 	plugin_get_from_id(int id);
GtkListStore *	playlist3_get_category_tree_store(void);
GtkTreeView *	playlist3_get_category_tree_view(void);
/**
 * Helper functions to get path to gmpc directory and metadata directory
 */

gchar * 		gmpc_get_covers_path(const gchar *filename);
gchar * 		gmpc_get_user_path(const gchar *filename);
void 			playlist3_insert_browser(GtkTreeIter *iter, gint position);
#endif
