#ifndef _playlist_list_h_included_
#define _playlist_list_h_included_

#include <gtk/gtk.h>

/* Some boilerplate GObject defines. 'klass' is used
 *   instead of 'class', because 'class' is a C++ keyword */

#define CUSTOM_TYPE_LIST            (playlist_list_get_type ())
#define PLAYLIST_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CUSTOM_TYPE_LIST, CustomList))
#define PLAYLIST_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CUSTOM_TYPE_LIST, CustomListClass))
#define CUSTOM_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_LIST))
#define CUSTOM_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_TYPE_LIST))
#define PLAYLIST_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_LIST, CustomListClass))

/* The data columns that we export via the tree model interface */

enum
{
	PLAYLIST_LIST_COL_MPDSONG = 0, 	/* get the mpd_Song */
	PLAYLIST_LIST_COL_MARKUP, 	/* a string that has markup */
	PLAYLIST_LIST_COL_PLAYING,	/* Shows if this song is the current song */
	PLAYLIST_LIST_COL_PLAYING_FONT_WEIGHT,	/* Shows if this song is the current song */
	PLAYLIST_LIST_COL_SONG_FILE,	/* internal field of mpd_Song ->file */
	PLAYLIST_LIST_COL_SONG_ARTIST,	/* artist name */
	PLAYLIST_LIST_COL_SONG_ALBUM,	/* album name */
	PLAYLIST_LIST_COL_SONG_TITLE,	/* song title */
	PLAYLIST_LIST_COL_SONG_GENRE,	/* song genre */
	PLAYLIST_LIST_COL_SONG_TRACK,	/* song track */
	PLAYLIST_LIST_COL_SONG_NAME,	/* stream name */
	PLAYLIST_LIST_COL_SONG_COMPOSER,
	PLAYLIST_LIST_COL_SONG_DATE,
	PLAYLIST_LIST_COL_SONG_LENGTH,
	PLAYLIST_LIST_COL_SONG_POS,
	PLAYLIST_LIST_COL_SONG_ID,
	PLAYLIST_LIST_COL_ICON_ID,
	PLAYLIST_LIST_N_COLUMNS,
} ;


typedef struct _CustomList       CustomList;
typedef struct _CustomListClass  CustomListClass;







/* CustomList: this structure contains everything we need for our
 *             model implementation. You can add extra fields to
 *             this structure, e.g. hashtables to quickly lookup
 *             rows or whatever else you might need, but it is
 *             crucial that 'parent' is the first member of the
 *             structure.                                          */

struct _CustomList
{
	GObject         parent;      /* this MUST be the first member */

	guint           num_rows;    /* number of rows that we have   */
	guint		playlist_id;
	gint		current_song_pos;
	MpdData *mpdata;
	char *markup;
	gint            n_columns;
	guint 		playtime;

	GType           column_types[PLAYLIST_LIST_N_COLUMNS];

	gint            stamp;       /* Random integer to check whether an iter belongs to our model */
};



/* CustomListClass: more boilerplate GObject stuff */

struct _CustomListClass
{
	GObjectClass parent_class;
};


GType             playlist_list_get_type (void);

CustomList       *playlist_list_new (void);

void playlist_list_set_current_song_pos(CustomList *cl, int new_pos);
gchar *playlist_list_get_markup(CustomList *cl);
void playlist_list_set_markup(CustomList *cl, gchar *markup);
void playlist_list_clear(CustomList *list);
void playlist_list_data_update(CustomList *cl ,MpdObj *mi,GtkTreeView *tree,GtkWidget *pb);
guint playlist_list_get_playtime(CustomList *cl);
#endif /* _playlist_list_h_included_ */
