
#ifndef __GMPC_FAVORITES_H__
#define __GMPC_FAVORITES_H__

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libmpd/libmpdclient.h>

G_BEGIN_DECLS


#define GMPC_FAVORITES_TYPE_LIST (gmpc_favorites_list_get_type ())
#define GMPC_FAVORITES_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_FAVORITES_TYPE_LIST, GmpcFavoritesList))
#define GMPC_FAVORITES_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_FAVORITES_TYPE_LIST, GmpcFavoritesListClass))
#define GMPC_FAVORITES_IS_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_FAVORITES_TYPE_LIST))
#define GMPC_FAVORITES_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_FAVORITES_TYPE_LIST))
#define GMPC_FAVORITES_LIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_FAVORITES_TYPE_LIST, GmpcFavoritesListClass))

typedef struct _GmpcFavoritesList GmpcFavoritesList;
typedef struct _GmpcFavoritesListClass GmpcFavoritesListClass;
typedef struct _GmpcFavoritesListPrivate GmpcFavoritesListPrivate;

#define GMPC_FAVORITES_TYPE_BUTTON (gmpc_favorites_button_get_type ())
#define GMPC_FAVORITES_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButton))
#define GMPC_FAVORITES_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButtonClass))
#define GMPC_FAVORITES_IS_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_FAVORITES_TYPE_BUTTON))
#define GMPC_FAVORITES_IS_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_FAVORITES_TYPE_BUTTON))
#define GMPC_FAVORITES_BUTTON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButtonClass))

typedef struct _GmpcFavoritesButton GmpcFavoritesButton;
typedef struct _GmpcFavoritesButtonClass GmpcFavoritesButtonClass;
typedef struct _GmpcFavoritesButtonPrivate GmpcFavoritesButtonPrivate;

/**
     * This class is created, and stays active until the last GmpcFavoritesButton gets removed
     * POSSIBLE ISSUE: setting favorites list back to NULL seems to fail. It is no issue as 
     * I know atleast one will be active.
     */
struct _GmpcFavoritesList {
	GObject parent_instance;
	GmpcFavoritesListPrivate * priv;
};

struct _GmpcFavoritesListClass {
	GObjectClass parent_class;
};

/**
     * The actual favorite button
     */
struct _GmpcFavoritesButton {
	GtkEventBox parent_instance;
	GmpcFavoritesButtonPrivate * priv;
};

struct _GmpcFavoritesButtonClass {
	GtkEventBoxClass parent_class;
};


#define some_unique_name VERSION
extern GmpcFavoritesList* favorites;
#define use_transition TRUE
gboolean gmpc_favorites_list_is_favorite (GmpcFavoritesList* self, const char* path);
void gmpc_favorites_list_set_favorite (GmpcFavoritesList* self, const char* path, gboolean favorite);
GmpcFavoritesList* gmpc_favorites_list_construct (GType object_type);
GmpcFavoritesList* gmpc_favorites_list_new (void);
GType gmpc_favorites_list_get_type (void);
void gmpc_favorites_button_set_song (GmpcFavoritesButton* self, const mpd_Song* song);
GmpcFavoritesButton* gmpc_favorites_button_construct (GType object_type);
GmpcFavoritesButton* gmpc_favorites_button_new (void);
GType gmpc_favorites_button_get_type (void);


G_END_DECLS

#endif
