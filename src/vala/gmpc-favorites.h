
#ifndef __GMPC_FAVORITES_H__
#define __GMPC_FAVORITES_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libmpd/libmpdclient.h>
#include <libmpd/libmpd.h>

G_BEGIN_DECLS


#define GMPC_FAVORITES_TYPE_BUTTON (gmpc_favorites_button_get_type ())
#define GMPC_FAVORITES_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButton))
#define GMPC_FAVORITES_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButtonClass))
#define GMPC_FAVORITES_IS_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_FAVORITES_TYPE_BUTTON))
#define GMPC_FAVORITES_IS_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_FAVORITES_TYPE_BUTTON))
#define GMPC_FAVORITES_BUTTON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButtonClass))

typedef struct _GmpcFavoritesButton GmpcFavoritesButton;
typedef struct _GmpcFavoritesButtonClass GmpcFavoritesButtonClass;
typedef struct _GmpcFavoritesButtonPrivate GmpcFavoritesButtonPrivate;

struct _GmpcFavoritesButton {
	GtkEventBox parent_instance;
	GmpcFavoritesButtonPrivate * priv;
};

struct _GmpcFavoritesButtonClass {
	GtkEventBoxClass parent_class;
};


GType gmpc_favorites_button_get_type (void);
void gmpc_favorites_button_set_song (GmpcFavoritesButton* self, const mpd_Song* song);
GmpcFavoritesButton* gmpc_favorites_button_new (void);
GmpcFavoritesButton* gmpc_favorites_button_construct (GType object_type);


G_END_DECLS

#endif
