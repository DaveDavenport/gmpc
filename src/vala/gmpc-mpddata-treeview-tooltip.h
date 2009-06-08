
#ifndef __GMPC_MPDDATA_TREEVIEW_TOOLTIP_H__
#define __GMPC_MPDDATA_TREEVIEW_TOOLTIP_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <metadata.h>

G_BEGIN_DECLS


#define GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP (gmpc_mpd_data_treeview_tooltip_get_type ())
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltip))
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltipClass))
#define GMPC_MPD_DATA_TREEVIEW_IS_TOOLTIP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP))
#define GMPC_MPD_DATA_TREEVIEW_IS_TOOLTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP))
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltipClass))

typedef struct _GmpcMpdDataTreeviewTooltip GmpcMpdDataTreeviewTooltip;
typedef struct _GmpcMpdDataTreeviewTooltipClass GmpcMpdDataTreeviewTooltipClass;
typedef struct _GmpcMpdDataTreeviewTooltipPrivate GmpcMpdDataTreeviewTooltipPrivate;

struct _GmpcMpdDataTreeviewTooltip {
	GtkWindow parent_instance;
	GmpcMpdDataTreeviewTooltipPrivate * priv;
};

struct _GmpcMpdDataTreeviewTooltipClass {
	GtkWindowClass parent_class;
};


GType gmpc_mpd_data_treeview_tooltip_get_type (void);
GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_new (GtkTreeView* pw, MetaDataType type);
GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_construct (GType object_type, GtkTreeView* pw, MetaDataType type);


G_END_DECLS

#endif
