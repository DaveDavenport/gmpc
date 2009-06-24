
#ifndef __GMPC_MENU_ITEM_RATING_H__
#define __GMPC_MENU_ITEM_RATING_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <gmpc-rating.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>

G_BEGIN_DECLS


#define GMPC_MENU_ITEM_TYPE_RATING (gmpc_menu_item_rating_get_type ())
#define GMPC_MENU_ITEM_RATING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_MENU_ITEM_TYPE_RATING, GmpcMenuItemRating))
#define GMPC_MENU_ITEM_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_MENU_ITEM_TYPE_RATING, GmpcMenuItemRatingClass))
#define GMPC_MENU_ITEM_IS_RATING(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_MENU_ITEM_TYPE_RATING))
#define GMPC_MENU_ITEM_IS_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_MENU_ITEM_TYPE_RATING))
#define GMPC_MENU_ITEM_RATING_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_MENU_ITEM_TYPE_RATING, GmpcMenuItemRatingClass))

typedef struct _GmpcMenuItemRating GmpcMenuItemRating;
typedef struct _GmpcMenuItemRatingClass GmpcMenuItemRatingClass;
typedef struct _GmpcMenuItemRatingPrivate GmpcMenuItemRatingPrivate;

struct _GmpcMenuItemRating {
	GtkMenuItem parent_instance;
	GmpcMenuItemRatingPrivate * priv;
	GtkVBox* hbox;
	GmpcRating* rating;
};

struct _GmpcMenuItemRatingClass {
	GtkMenuItemClass parent_class;
};


GType gmpc_menu_item_rating_get_type (void);
gint gmpc_menu_item_rating_get_rating (GmpcMenuItemRating* self);
GmpcMenuItemRating* gmpc_menu_item_rating_new (MpdObj* server, const mpd_Song* song);
GmpcMenuItemRating* gmpc_menu_item_rating_construct (GType object_type, MpdObj* server, const mpd_Song* song);


G_END_DECLS

#endif
