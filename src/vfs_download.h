#include <libgnomevfs/gnome-vfs.h>

typedef struct _dl_info
{
	gchar *link;
	void * (* function)(gchar *buffer, gpointer data);
	gpointer data;
	GString *file;
	GtkWidget *dialog;
	GtkWidget *prog_bar;
	GnomeVFSAsyncHandle *handle;
}dl_info;


void get_file(dl_info *di);
void load_genres(gchar *buffer);
void load_streams(gchar *buffer);
