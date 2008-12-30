#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include "metadata.h"
#include "gmpc-profiles.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"

#ifndef __GMPC_PLUGIN_H__
#define __GMPC_PLUGIN_H__

#define PLUGIN_ID_MARK 1024
#define PLUGIN_ID_INTERNALL 2048
/* Usefull signal objects. */
extern MpdObj *connection;
extern config_obj *config;
extern GmpcProfiles *gmpc_profiles;

/* Plugin Type's */
/* This is a bitmask.*/
typedef enum {
	GMPC_PLUGIN_DUMMY		= 1,
	GMPC_PLUGIN_PL_BROWSER	= 2,
	GMPC_PLUGIN_NO_GUI		= 4,
	GMPC_INTERNALL			= 8,
	GMPC_DEPRECATED			= 16,
	GMPC_PLUGIN_META_DATA	= 32,
    /* Make compiler happy about GMPC_PLUGIN_PL_BROWSER|GMPC_PLUGIN_METADATA */
    GMPC_BROWSER_META       = 34
} PluginType;

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
    PL3_CAT_NUM_ITEMS,
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
    /* Padding */
    void (*padding1)                (void);
    void (*padding2)                (void);
    void (*padding3)                (void);
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
	int  (*cat_right_mouse_menu)	(GtkWidget *menu, int type, GtkWidget *tree,GdkEventButton *event);
	void (*cat_key_press)			(GtkWidget *tree, GdkEventKey *event, int selected_type) G_GNUC_DEPRECATED;
	/****** GO MENU ********/
	int  (*add_go_menu)				(GtkWidget *menu); 
	/****** Key presses (in the whole window) **/
	int  (*key_press_event)			(GtkWidget *mw, GdkEventKey *event, int type);
    /** Song list right mouse menu intergration.
     * This is only called (And allowed to be called) if the treeview is a GmpcMpdDataTreeview with songs 
     */
    int (*song_list_option_menu)    (GmpcMpdDataTreeview *tree, GtkMenu *menu);
    /* Padding */
    void (*padding1)                (void);
    void (*padding2)                (void);
    void (*padding3)                (void);
} gmpcPlBrowserPlugin;

/**
 * Metadata fetching plugin.
 * All fields required
 */
typedef struct {
    /* Set and get priority */
	int (*get_priority)				(void);
    void (*set_priority)            (int priority);
	int (*get_image)				(mpd_Song *song, MetaDataType type, char **path);
    /* Padding */
    void (*padding1)                (void);
    void (*padding2)                (void);
    void (*padding3)                (void);
} gmpcMetaDataPlugin;

/* Unique number                */
/* 16 == 0.14 release           */
/* 17 == 0.15.5 release         */
/* 18 ==       16 december      */
/* 19 == 0.16*  24 december     */
/* 20 == 0.16.5 release         */ 
/* 21 == 0.17.0 release -> adding padding so abi wont break next time */
#define PLUGIN_API_VERSION 21

/* sturcture */
typedef struct {
	/* Name of the plugin */
	const char						*name;		
	/* Version number */
	const int						version[3];	
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

    /* Padding */
    void (*padding1)                (void);
    void (*padding2)                (void);
    void (*padding3)                (void);
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
/* Tell mpd to reload the go menu */
void            pl3_update_go_menu(void);

/**
 * Plugin functions
 */
void            gmpc_plugin_init                            (gmpcPlugin *plug);
void            gmpc_plugin_destroy                         (gmpcPlugin *plug);
void            gmpc_plugin_save_yourself                   (gmpcPlugin *plug);

gboolean        gmpc_plugin_get_enabled                     (gmpcPlugin *plug);           

const char *    gmpc_plugin_get_name                        (gmpcPlugin *plug);
void            gmpc_plugin_status_changed                  (gmpcPlugin *plug, MpdObj *mi, ChangedStatusType what);
const gchar *   gmpc_plugin_get_data_path                   (gmpcPlugin *plug);
void            gmpc_plugin_mpd_connection_changed          (gmpcPlugin *plug, MpdObj *mi, int connected, gpointer data);

gboolean        gmpc_plugin_is_browser                      (gmpcPlugin *plug);
void            gmpc_plugin_browser_unselected              (gmpcPlugin *plug, GtkWidget *container);
void            gmpc_plugin_browser_selected                (gmpcPlugin *plug, GtkWidget *container);
void            gmpc_plugin_browser_add                     (gmpcPlugin *plug, GtkWidget *cat_tree);
int             gmpc_plugin_browser_cat_right_mouse_menu    (gmpcPlugin *plug, GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);
int             gmpc_plugin_browser_key_press_event         (gmpcPlugin *plug, GtkWidget *mw, GdkEventKey *event, int type);
int             gmpc_plugin_browser_add_go_menu             (gmpcPlugin *plug, GtkWidget *menu);
int             gmpc_plugin_browser_song_list_option_menu   (gmpcPlugin *plug, GmpcMpdDataTreeview *tree, GtkMenu *menu);


gboolean        gmpc_plugin_has_preferences                 (gmpcPlugin *plug);
void            gmpc_plugin_preferences_construct           (gmpcPlugin *plug,GtkWidget *wid);
void            gmpc_plugin_preferences_destroy             (gmpcPlugin *plug,GtkWidget *wid);

int             gmpc_plugin_get_type(gmpcPlugin *plug);
const int *     gmpc_plugin_get_version                     (gmpcPlugin *plug);
gboolean        gmpc_plugin_is_internal                     (gmpcPlugin *plug);

/* metadata */
gboolean        gmpc_plugin_is_metadata                     (gmpcPlugin *plug);
int             gmpc_plugin_metadata_get_priority           (gmpcPlugin *plug);
void            gmpc_plugin_metadata_set_priority           (gmpcPlugin *plug, int priority);
int             gmpc_plugin_metadata_get_image              (gmpcPlugin *plug, mpd_Song *song, MetaDataType type, char **path);
#endif
