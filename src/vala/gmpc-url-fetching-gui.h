
#ifndef __GMPC_URL_FETCHING_GUI_H__
#define __GMPC_URL_FETCHING_GUI_H__

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

G_BEGIN_DECLS


#define GMPC_URL_FETCHING_TYPE_GUI (gmpc_url_fetching_gui_get_type ())
#define GMPC_URL_FETCHING_GUI(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_URL_FETCHING_TYPE_GUI, GmpcUrlFetchingGui))
#define GMPC_URL_FETCHING_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_URL_FETCHING_TYPE_GUI, GmpcUrlFetchingGuiClass))
#define GMPC_URL_FETCHING_IS_GUI(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_URL_FETCHING_TYPE_GUI))
#define GMPC_URL_FETCHING_IS_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_URL_FETCHING_TYPE_GUI))
#define GMPC_URL_FETCHING_GUI_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_URL_FETCHING_TYPE_GUI, GmpcUrlFetchingGuiClass))

typedef struct _GmpcUrlFetchingGui GmpcUrlFetchingGui;
typedef struct _GmpcUrlFetchingGuiClass GmpcUrlFetchingGuiClass;
typedef struct _GmpcUrlFetchingGuiPrivate GmpcUrlFetchingGuiPrivate;

struct _GmpcUrlFetchingGui {
	GObject parent_instance;
	GmpcUrlFetchingGuiPrivate * priv;
};

struct _GmpcUrlFetchingGuiClass {
	GObjectClass parent_class;
};

typedef void (*GmpcUrlFetchingGuiParseUrl) (GmpcUrlFetchingGui* gui, const char* url, void* user_data);
typedef gboolean (*GmpcUrlFetchingGuiValidateUrl) (GmpcUrlFetchingGui* gui, const char* url, void* user_data);

GType gmpc_url_fetching_gui_get_type (void);
GmpcUrlFetchingGui* gmpc_url_fetching_gui_new (GmpcUrlFetchingGuiParseUrl parse_callback, void* parse_callback_target, GmpcUrlFetchingGuiValidateUrl validate_callback, void* validate_callback_target, GDestroyNotify destroy_cb);
GmpcUrlFetchingGui* gmpc_url_fetching_gui_construct (GType object_type, GmpcUrlFetchingGuiParseUrl parse_callback, void* parse_callback_target, GmpcUrlFetchingGuiValidateUrl validate_callback, void* validate_callback_target, GDestroyNotify destroy_cb);
void gmpc_url_fetching_gui_set_processing (GmpcUrlFetchingGui* self);
void gmpc_url_fetching_gui_set_progress (GmpcUrlFetchingGui* self, double progress);
void gmpc_url_fetching_gui_set_completed (GmpcUrlFetchingGui* self);
void gmpc_url_fetching_gui_set_error (GmpcUrlFetchingGui* self, const char* error_message);


G_END_DECLS

#endif
