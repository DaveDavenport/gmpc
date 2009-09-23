
#ifndef __GMPC_RATING_H__
#define __GMPC_RATING_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>

G_BEGIN_DECLS


#define GMPC_TYPE_RATING (gmpc_rating_get_type ())
#define GMPC_RATING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_RATING, GmpcRating))
#define GMPC_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_RATING, GmpcRatingClass))
#define GMPC_IS_RATING(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_RATING))
#define GMPC_IS_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_RATING))
#define GMPC_RATING_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_RATING, GmpcRatingClass))

typedef struct _GmpcRating GmpcRating;
typedef struct _GmpcRatingClass GmpcRatingClass;
typedef struct _GmpcRatingPrivate GmpcRatingPrivate;

struct _GmpcRating {
	GtkFrame parent_instance;
	GmpcRatingPrivate * priv;
	GtkEventBox* event_box;
};

struct _GmpcRatingClass {
	GtkFrameClass parent_class;
};


GType gmpc_rating_get_type (void);
gboolean gmpc_rating_button_press_event_callback (GmpcRating* self, GtkEventBox* wid, const GdkEventButton* event);
GmpcRating* gmpc_rating_new (MpdObj* server, const mpd_Song* song);
GmpcRating* gmpc_rating_construct (GType object_type, MpdObj* server, const mpd_Song* song);
void gmpc_rating_set_rating (GmpcRating* self, gint rating);
void gmpc_rating_update (GmpcRating* self);


G_END_DECLS

#endif
