/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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

#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include "metadata.h"
#include "gmpc-profiles.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#define PLUGIN_ID_MARK 1024
#define PLUGIN_ID_INTERNALL 2048
/* Usefull signal objects. */
extern MpdObj *connection;
extern config_obj *config;
extern GmpcProfiles *gmpc_profiles;


typedef struct _gmpcPluginParent gmpcPluginParent;
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
	PL3_CAT_PROC_DEPRECATED, /* for the lazy tree, if the dir is allready processed */
	PL3_CAT_ICON_SIZE_DEPRECATED, /** icon size */
	PL3_CAT_BROWSE_FORMAT_DEPRECATED, /** string, added for tag browser */
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
    MpdData * (*integrate_search)   (const int search_field,const gchar *search_query,GError **error);
    gboolean  (*integrate_search_field_supported) (const int search_field);
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
    gint    (*tool_menu_integration)    (GtkMenu *menu); 
    void (*padding2)                (void);
    void (*padding3)                (void);
} gmpcPlugin;




/**
 * Playlist function 
 */

/**
 * Get the treeview and tree_store of the category list on the left.
 */
GtkListStore *	playlist3_get_category_tree_store(void);
GtkTreeView *	playlist3_get_category_tree_view(void);

/**
 * Get this GtkWindow of the playlist.
 * Use this to set parent and so
 */
GtkWidget * playlist3_get_window(void);
/**
 * indicates if the window is hidden to tray
 */
gboolean    playlist3_window_is_hidden(void);

/** plugin functions */
gmpcPluginParent * 	plugin_get_from_id(int id);


/**
 * Helper functions to get path to gmpc directory and metadata directory
 */

char  *         gmpc_get_full_glade_path(const char *filename);
gchar * 		gmpc_get_covers_path(const gchar *filename);
gchar * 		gmpc_get_user_path(const gchar *filename);
void 			playlist3_insert_browser(GtkTreeIter *iter, gint position);


void pl3_option_menu_activate(void);
/* Tell mpd to reload the go menu */
void            pl3_update_go_menu(void);


/**
 * Plugin functions
 */

int             gmpc_plugin_get_id                          (gmpcPluginParent *plug);
void            gmpc_plugin_init                            (gmpcPluginParent *plug);
void            gmpc_plugin_destroy                         (gmpcPluginParent *plug);
void            gmpc_plugin_save_yourself                   (gmpcPluginParent *plug);

gboolean        gmpc_plugin_get_enabled                     (gmpcPluginParent *plug);           
void            gmpc_plugin_set_enabled                     (gmpcPluginParent *plug, gboolean enabled);

const char *    gmpc_plugin_get_name                        (gmpcPluginParent *plug);
void            gmpc_plugin_status_changed                  (gmpcPluginParent *plug, MpdObj *mi, ChangedStatusType what);

gint            gmpc_plugin_tool_menu_integration           (gmpcPluginParent  *plug, GtkMenu *menu);
/* Used by plugins themself */
gchar *         gmpc_plugin_get_data_path                   (gmpcPlugin *plug);
void            gmpc_plugin_mpd_connection_changed          (gmpcPluginParent *plug, MpdObj *mi, int connected, gpointer data);

gboolean        gmpc_plugin_is_browser                      (gmpcPluginParent *plug);
void            gmpc_plugin_browser_unselected              (gmpcPluginParent *plug, GtkWidget *container);
void            gmpc_plugin_browser_selected                (gmpcPluginParent *plug, GtkWidget *container);
void            gmpc_plugin_browser_add                     (gmpcPluginParent *plug, GtkWidget *cat_tree);
int             gmpc_plugin_browser_cat_right_mouse_menu    (gmpcPluginParent *plug, GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);
int             gmpc_plugin_browser_key_press_event         (gmpcPluginParent *plug, GtkWidget *mw, GdkEventKey *event, int type);
int             gmpc_plugin_browser_add_go_menu             (gmpcPluginParent *plug, GtkWidget *menu);
int             gmpc_plugin_browser_song_list_option_menu   (gmpcPluginParent *plug, GmpcMpdDataTreeview *tree, GtkMenu *menu);
gboolean        gmpc_plugin_browser_has_integrate_search    (gmpcPluginParent *plug);
MpdData *       gmpc_plugin_browser_integrate_search        (gmpcPluginParent *plug, const int search_field, const gchar * query, GError **error);
gboolean        gmpc_plugin_browser_integrate_search_field_supported        (gmpcPluginParent *plug, const int search_field);

gboolean        gmpc_plugin_has_preferences                 (gmpcPluginParent *plug);
void            gmpc_plugin_preferences_construct           (gmpcPluginParent *plug,GtkWidget *wid);
void            gmpc_plugin_preferences_destroy             (gmpcPluginParent *plug,GtkWidget *wid);

int             gmpc_plugin_get_type(gmpcPluginParent *plug);
const int *     gmpc_plugin_get_version                     (gmpcPluginParent *plug);
gboolean        gmpc_plugin_is_internal                     (gmpcPluginParent *plug);

/* metadata */
gboolean        gmpc_plugin_is_metadata                     (gmpcPluginParent *plug);
int             gmpc_plugin_metadata_get_priority           (gmpcPluginParent *plug);
void            gmpc_plugin_metadata_set_priority           (gmpcPluginParent *plug, int priority);
int             gmpc_plugin_metadata_get_image              (gmpcPluginParent *plug, mpd_Song *song, MetaDataType type, char **path);

/**
 * Update parts of the gui
 */
void pl3_tool_menu_update(void);

#endif
