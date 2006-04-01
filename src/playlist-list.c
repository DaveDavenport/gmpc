#include <stdio.h>
#include <string.h>
#include <libmpd/debug_printf.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpd-internal.h>
#include "playlist-list.h"

/* boring declarations of local functions */

static void playlist_list_init(CustomList * pkg_tree);

static void playlist_list_class_init(CustomListClass * klass);

static void playlist_list_tree_model_init(GtkTreeModelIface * iface);

static void playlist_list_finalize(GObject * object);

static GtkTreeModelFlags playlist_list_get_flags(GtkTreeModel * tree_model);

static gint playlist_list_get_n_columns(GtkTreeModel * tree_model);

static GType playlist_list_get_column_type(GtkTreeModel * tree_model,
		gint index);

static gboolean playlist_list_get_iter(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreePath * path);

static GtkTreePath *playlist_list_get_path(GtkTreeModel * tree_model,
		GtkTreeIter * iter);

static void playlist_list_get_value(GtkTreeModel * tree_model,
		GtkTreeIter * iter,
		gint column, GValue * value);

static gboolean playlist_list_iter_next(GtkTreeModel * tree_model,
		GtkTreeIter * iter);

static gboolean playlist_list_iter_children(GtkTreeModel * tree_model,
		GtkTreeIter * iter,
		GtkTreeIter * parent);

static gboolean playlist_list_iter_has_child(GtkTreeModel * tree_model,
		GtkTreeIter * iter);

static gint playlist_list_iter_n_children(GtkTreeModel * tree_model,
		GtkTreeIter * iter);

static gboolean playlist_list_iter_nth_child(GtkTreeModel * tree_model,
		GtkTreeIter * iter,
		GtkTreeIter * parent, gint n);

static gboolean playlist_list_iter_parent(GtkTreeModel * tree_model,
		GtkTreeIter * iter,
		GtkTreeIter * child);

/* GObject stuff - nothing to worry about */
static GObjectClass *parent_class = NULL;

void playlist_list_set_current_song_pos(CustomList * cl, int new_pos)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	int song = cl->current_song_pos;
	cl->current_song_pos = new_pos;
	/* old song pos (song) is valid, make sure the row is updated so it shows unhighligted */
	if (song >= 0 && song < cl->num_rows) {
		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path, song);
		/* iter */
		if (playlist_list_get_iter(GTK_TREE_MODEL(cl), &iter, path)) {
			/* changed */
			gtk_tree_model_row_changed(GTK_TREE_MODEL(cl), path,
					&iter);
		}
		gtk_tree_path_free(path);
	}

	/* if the current position is valid: update that row */
	if (cl->current_song_pos >= 0 && cl->current_song_pos < cl->num_rows) {
		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path, cl->current_song_pos);
		/* iter */
		if (playlist_list_get_iter(GTK_TREE_MODEL(cl), &iter, path)) {
			/* changed */
			gtk_tree_model_row_changed(GTK_TREE_MODEL(cl), path,
					&iter);
		}
		gtk_tree_path_free(path);
	}
}

/* get the markup string */
gchar *playlist_list_get_markup(CustomList * cl)
{
	return cl->markup;
}

/* set the markup string */
void playlist_list_set_markup(CustomList * cl, gchar * markup)
{
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	MpdData *temp = cl->mpdata;
	if (!markup)
		return;
	if (cl->markup)
		g_free(cl->markup);
	cl->markup = g_strdup(markup);

	/* start sending signals */
	/* all rows are dirty now */
	for (temp = mpd_data_get_first(temp); temp;
			temp = mpd_data_get_next_real(temp, FALSE)) {

		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path, temp->song->pos);
		/* create an iter */
		iter.stamp = cl->stamp;
		iter.user_data = temp;
		iter.user_data2 = NULL;
		iter.user_data3 = NULL;
		/* propegate change */
		gtk_tree_model_row_changed(GTK_TREE_MODEL(cl), path, &iter);
		gtk_tree_path_free(path);
	}
}

guint playlist_list_get_playtime(CustomList * cl)
{
	if (cl) {
		return cl->playtime;
	}
	return 0;
}

/*****************************************************************************
 *
 *  playlist_list_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/
typedef struct {
	CustomList *cl;
	MpdObj *mi;
	MpdData *data;
	MpdData *iter;
	GtkTreeView *tree;
	int total_length;
	gdouble adjustment;
}pass_data;


int playlist_list_data_fill_initial(pass_data *pd)
{
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	if(pd->data == NULL) return FALSE;
	/* start sending signals */
	path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, pd->data->song->pos);
	iter.stamp = pd->cl->stamp;
	iter.user_data = pd->data;
	iter.user_data2 = NULL;
	iter.user_data3 = NULL;
	gtk_tree_model_row_inserted(GTK_TREE_MODEL(pd->cl), path,
			&iter);
	gtk_tree_path_free(path);
	pd->cl->playtime += pd->data->song->time;
	pd->cl->num_rows++;
	pd->data = mpd_data_get_next_real(pd->data, FALSE);
	return TRUE;
}

/* I need to clean this up, atm it has a high "black voodoo" factor
 * I now determine state by the most odd things.
 * So idea might be is to add a State in the pass_data and determine what todo from there.
 *
 */
int playlist_list_data_update_data(pass_data *pd)
{
	MpdData *temp = NULL;
	MpdObj *mi = pd->mi;
	gint pos=0;
	CustomList *cl = pd->cl;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	if(pd->data == NULL) {
		if(pd->iter == NULL) pd->iter = mpd_data_get_first(cl->mpdata);
		if (cl->num_rows > mpd_playlist_get_playlist_length(mi) && cl->mpdata) 
		{
			MpdData *temp = pd->iter; 
			gint new_length = pd->total_length;
			/* nasty trick to make it go backwards, otherwise gtk thinks everything
			 * is out of sync
			 */
			for (; !mpd_data_is_last(temp);
					temp = mpd_data_get_next_real(temp, FALSE)) ;
			if (temp->song->pos >= new_length) 
			{
				int pos = temp->song->pos;
				cl->playtime -= temp->song->time;
				temp = mpd_data_delete_item(temp);
				cl->mpdata = temp;	//mpd_data_get_first(temp);
				cl->num_rows--;
				path = gtk_tree_path_new();
				gtk_tree_path_append_index(path, pos);
				gtk_tree_model_row_deleted(GTK_TREE_MODEL(cl),
						path);
				gtk_tree_path_free(path);
				pd->iter = temp;
			} else {
				temp = NULL;
			}                                                                      		
			if(temp) return TRUE;
	
		}
		return FALSE;
	}
	if(pd->iter == NULL) pd->iter = mpd_data_get_first(cl->mpdata);
	temp = pd->iter;
	pos = pd->data->song->pos;
	if (cl->num_rows > 0 && pos < cl->num_rows) {
		for (; temp->song->pos != pos;
				temp =
				mpd_data_get_next_real(temp, FALSE)) ;
		/* remove old song */
		cl->playtime -= temp->song->time;
		mpd_freeSong(temp->song);
		/* move new one in */
		temp->song = pd->data->song;
		pd->data->song = NULL;
		cl->playtime += temp->song->time;
		/* path */
		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path,
				temp->song->pos);
		/* iter */
		iter.stamp = cl->stamp;
		iter.user_data = temp;
		iter.user_data2 = NULL;
		iter.user_data3 = NULL;
		/* changed */
		gtk_tree_model_row_changed(GTK_TREE_MODEL(cl),
				path, &iter);
		gtk_tree_path_free(path);

	} else {
		/* Get the last one */
		for (; temp && !mpd_data_is_last(temp);
				temp =
				mpd_data_get_next_real(temp, FALSE)) ;
		temp = mpd_new_data_struct_append(temp);
		/* set values */
		cl->mpdata = temp;	//mpd_data_get_first(temp);
		temp->type = MPD_DATA_TYPE_SONG;
		/* move song */
		temp->song = pd->data->song;
		pd->data->song = NULL;
		/* calculate time */
		cl->playtime += temp->song->time;
		/* create path */
		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path,
				temp->song->pos);
		/* create iter */
		iter.stamp = cl->stamp;
		iter.user_data = temp;
		iter.user_data2 = NULL;
		iter.user_data3 = NULL;
		/* send signal */
		gtk_tree_model_row_inserted(GTK_TREE_MODEL(cl),
				path, &iter);
		gtk_tree_path_free(path);

		cl->num_rows++;
	}
	pd->iter = temp;
	pd->data = mpd_data_get_next(pd->data);
	if(pd->data == NULL) pd->iter = NULL;
	return TRUE;
}


void playlist_list_data_update_done(pass_data *pd)
{
	if (pd->cl->num_rows != mpd_playlist_get_playlist_length(pd->mi)) {
		debug_printf(DEBUG_ERROR, "sync went wrong %i %i\n", pd->cl->num_rows,mpd_playlist_get_playlist_length(pd->mi));
	}                                                                                                           	
	if (pd->cl->mpdata) {
		pd->cl->mpdata = mpd_data_get_first(pd->cl->mpdata);
	}                                               	
	pd->cl->playlist_id = mpd_playlist_get_playlist_id(pd->mi);
	if(pd->tree){
		GtkAdjustment *ad = NULL;
		gtk_tree_view_set_model(pd->tree, GTK_TREE_MODEL(pd->cl));
		while(gtk_events_pending()) gtk_main_iteration();
		ad = gtk_tree_view_get_vadjustment(pd->tree);
		if(pd->adjustment > -1){
			gtk_adjustment_set_value(ad,pd->adjustment);
		}


		
	}
	g_free(pd);
}

void playlist_list_data_update(CustomList * cl, MpdObj * mi,GtkTreeView *tree) {
	MpdData *data = mpd_playlist_get_changes(mi, cl->playlist_id);
	pass_data *pd = g_malloc0(sizeof(*pd));
	pd->cl = cl;
	pd->mi = mi;
	pd->data = data;
	pd->tree = tree;
	pd->iter =NULL;
	pd->adjustment = -1;
	pd->total_length =  mpd_playlist_get_playlist_length(mi);
	if(pd->tree) {
		GtkAdjustment *ad = gtk_tree_view_get_vadjustment(pd->tree);
		pd->adjustment = gtk_adjustment_get_value(ad);
		gtk_tree_view_set_model(tree, NULL);
	}
	if(cl->mpdata == NULL)
	{
		/* Don't want to remember the adjustment here, because it's an initial fill */
		pd->adjustment = -1;
		cl->mpdata = data;
		g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,(GSourceFunc)playlist_list_data_fill_initial,pd,playlist_list_data_update_done);
	}else{

		g_idle_add_full(G_PRIORITY_HIGH_IDLE,(GSourceFunc)playlist_list_data_update_data,pd,playlist_list_data_update_done);
	}

}




/* TODO: put this in an idle loop? or is this fast enough? */
/* it seems to be fast enough, for 65k items I see no slowdown */
void playlist_list_clear(CustomList * list,GtkTreeView *tree)
{
	MpdData_real *temp = (MpdData_real *) list->mpdata;
	if(temp == NULL) {
		return;
	}
	if(tree)gtk_tree_view_set_model(tree, NULL);
	/* get the last one */
	for (; !mpd_data_is_last((MpdData *) temp); temp = (MpdData_real *) mpd_data_get_next_real((MpdData *) temp, FALSE)) ;


	if (temp)
		do {
			GtkTreePath *path = gtk_tree_path_new();
			list->num_rows--;
			gtk_tree_path_append_index(path, temp->song->pos);
			gtk_tree_model_row_deleted(GTK_TREE_MODEL(list), path);
			gtk_tree_path_free(path);
			temp = temp->prev;
		} while (temp);
	if(list->mpdata) mpd_data_free(list->mpdata);
	list->mpdata = NULL;
	list->playlist_id = -1;
	list->num_rows = 0;
	list->playtime = 0;
	list->current_song_pos = -1;
	if(tree)gtk_tree_view_set_model(tree, GTK_TREE_MODEL(list));
}

GType playlist_list_get_type(void)
{
	static GType playlist_list_type = 0;

	if (playlist_list_type) {
		return playlist_list_type;
	}

	/* Some boilerplate type registration stuff */
	if (1) {
		static const GTypeInfo playlist_list_info = {
			sizeof(CustomListClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) playlist_list_class_init,
			NULL,	/* class finalize */
			NULL,	/* class_data */
			sizeof(CustomList),
			0,	/* n_preallocs */
			(GInstanceInitFunc) playlist_list_init
		};

		playlist_list_type =
			g_type_register_static(G_TYPE_OBJECT, "CustomList",
					&playlist_list_info, (GTypeFlags) 0);
	}

	/* Here we register our GtkTreeModel interface with the type system */
	if (1) {
		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) playlist_list_tree_model_init,
			NULL,
			NULL
		};

		g_type_add_interface_static(playlist_list_type,
				GTK_TYPE_TREE_MODEL,
				&tree_model_info);
	}

	return playlist_list_type;
}

/*****************************************************************************
 *
 *  playlist_list_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/

static void playlist_list_class_init(CustomListClass * klass)
{
	GObjectClass *object_class;

	parent_class = (GObjectClass *) g_type_class_peek_parent(klass);
	object_class = (GObjectClass *) klass;

	object_class->finalize = playlist_list_finalize;
}

/*****************************************************************************
 *
 *  playlist_list_tree_model_init: init callback for the interface registration
 *                               in playlist_list_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/

static void playlist_list_tree_model_init(GtkTreeModelIface * iface)
{
	iface->get_flags = playlist_list_get_flags;
	iface->get_n_columns = playlist_list_get_n_columns;
	iface->get_column_type = playlist_list_get_column_type;
	iface->get_iter = playlist_list_get_iter;
	iface->get_path = playlist_list_get_path;
	iface->get_value = playlist_list_get_value;
	iface->iter_next = playlist_list_iter_next;
	iface->iter_children = playlist_list_iter_children;
	iface->iter_has_child = playlist_list_iter_has_child;
	iface->iter_n_children = playlist_list_iter_n_children;
	iface->iter_nth_child = playlist_list_iter_nth_child;
	iface->iter_parent = playlist_list_iter_parent;
}

/*****************************************************************************
 *
 *  playlist_list_init: this is called everytime a new custom list object
 *                    instance is created (we do that in playlist_list_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/

static void playlist_list_init(CustomList * playlist_list)
{
	playlist_list->n_columns = PLAYLIST_LIST_N_COLUMNS;

	playlist_list->column_types[PLAYLIST_LIST_COL_MPDSONG] = G_TYPE_POINTER;
	playlist_list->column_types[PLAYLIST_LIST_COL_MARKUP] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_PLAYING] = G_TYPE_BOOLEAN;
	playlist_list->column_types[PLAYLIST_LIST_COL_PLAYING_FONT_WEIGHT] = G_TYPE_INT;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_FILE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_ARTIST] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_ALBUM] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_TITLE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_TITLEFILE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_TRACK] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_GENRE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_NAME] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_COMPOSER] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_DATE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_LENGTH] = G_TYPE_INT;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_POS] = G_TYPE_INT;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_ID] = G_TYPE_INT;
	playlist_list->column_types[PLAYLIST_LIST_COL_ICON_ID] = G_TYPE_STRING;

	playlist_list->num_rows = 0;
	playlist_list->mpdata = NULL;
	playlist_list->playlist_id = -1;
	playlist_list->current_song_pos = -1;

	playlist_list->playtime = 0;
	/* default markup */
	playlist_list->markup = g_strdup("[%title%[ - %artist%]]|%shortfile%");

	playlist_list->stamp = g_random_int();	/* Random int to check whether an iter belongs to our model */

}

/*****************************************************************************
 *
 *  playlist_list_finalize: this is called just before a custom list is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

static void playlist_list_finalize(GObject * object)
{
	CustomList *cl = PLAYLIST_LIST(object);
	debug_printf(DEBUG_INFO, "Finalize playlist-backend\n");
	/* free all records and free all memory used by the list */
	if(cl->mpdata)mpd_data_free(cl->mpdata);
	if (cl->markup)
		g_free(cl->markup);

	/* must chain up - finalize parent */
	(*parent_class->finalize) (object);
}

/*****************************************************************************
 *
 *  playlist_list_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics. In our case,
 *                         we have a list model (instead of a tree), and each
 *                         tree iter is valid as long as the row in question
 *                         exists, as it only contains a pointer to our struct.
 *
 *****************************************************************************/

static GtkTreeModelFlags playlist_list_get_flags(GtkTreeModel * tree_model)
{
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), (GtkTreeModelFlags) 0);

	return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

/*****************************************************************************
 *
 *  playlist_list_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/

static gint playlist_list_get_n_columns(GtkTreeModel * tree_model)
{
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), 0);

	return PLAYLIST_LIST(tree_model)->n_columns;
}

/*****************************************************************************
 *
 *  playlist_list_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/

	static GType
playlist_list_get_column_type(GtkTreeModel * tree_model, gint index)
{
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), G_TYPE_INVALID);
	g_return_val_if_fail(index < PLAYLIST_LIST(tree_model)->n_columns
			&& index >= 0, G_TYPE_INVALID);

	return PLAYLIST_LIST(tree_model)->column_types[index];
}

/*****************************************************************************
 *
 *  playlist_list_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure (the content of the iter
 *                        fields will only be used internally by our model).
 *                        We simply store a pointer to our CustomRecord
 *                        structure that represents that row in the tree iter.
 *
 *****************************************************************************/

static gboolean playlist_list_get_iter(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreePath * path)
{
	CustomList *playlist_list = PLAYLIST_LIST(tree_model);
	MpdData *data;
	gint *indices, n, depth;

	g_assert(playlist_list != NULL);
	g_assert(path != NULL);

	indices = gtk_tree_path_get_indices(path);
	depth = gtk_tree_path_get_depth(path);

	/* we do not allow children */
	g_assert(depth == 1);	/* depth 1 = top level; a list only has top level nodes and no children */

	n = indices[0];		/* the n-th top level row */

	/* Check if valid */
	if (n >= playlist_list->num_rows || n < 0)
		return FALSE;

	data = playlist_list->mpdata;	/*mpd_data_get_first(playlist_list->mpdata); */
	g_assert(data != NULL);

	if (n < data->song->pos)
		data = mpd_data_get_first(data);
	for (; data != NULL && data->song->pos != n;
			data = mpd_data_get_next_real(data, FALSE)) ;

	g_assert(data->song->pos == n);

	playlist_list->mpdata = data;

	/* We simply store a pointer to our custom record in the iter */
	iter->stamp = playlist_list->stamp;
	iter->user_data = data;
	iter->user_data2 = NULL;	/* unused */
	iter->user_data3 = NULL;	/* unused */

	return TRUE;
}

/*****************************************************************************
 *
 *  playlist_list_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/

static GtkTreePath *playlist_list_get_path(GtkTreeModel * tree_model,
		GtkTreeIter * iter)
{
	GtkTreePath *path;
	CustomList *playlist_list;
	MpdData *data;

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), NULL);
	g_return_val_if_fail(iter != NULL, NULL);
	g_return_val_if_fail(iter->user_data != NULL, NULL);

	playlist_list = PLAYLIST_LIST(tree_model);

	data = (MpdData *) iter->user_data;

	path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, data->song->pos);

	return path;
}

/*****************************************************************************
 *
 *  playlist_list_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/

	static void
playlist_list_get_value(GtkTreeModel * tree_model,
		GtkTreeIter * iter, gint column, GValue * value)
{
	MpdData *data = NULL;

	g_return_if_fail(CUSTOM_IS_LIST(tree_model));
	g_return_if_fail(iter != NULL);
	g_return_if_fail(column < PLAYLIST_LIST(tree_model)->n_columns);

	g_value_init(value, PLAYLIST_LIST(tree_model)->column_types[column]);

	data = (MpdData *) iter->user_data;

	g_return_if_fail(data != NULL);

	switch (column) {
		case PLAYLIST_LIST_COL_MPDSONG:
			g_value_set_pointer(value, data->song);
			break;
		case PLAYLIST_LIST_COL_PLAYING:
			if (data->song->pos ==
					PLAYLIST_LIST(tree_model)->current_song_pos) {
				g_value_set_boolean(value, TRUE);
			} else {
				g_value_set_boolean(value, FALSE);
			}
			break;
		case PLAYLIST_LIST_COL_PLAYING_FONT_WEIGHT:
			if (data->song->pos ==
					PLAYLIST_LIST(tree_model)->current_song_pos) {
				g_value_set_int(value, PANGO_WEIGHT_ULTRABOLD);
			} else {
				g_value_set_int(value, PANGO_WEIGHT_NORMAL);
			}
			break;
		case PLAYLIST_LIST_COL_MARKUP:
			{
				/* we want to go cache this stuff */
				gchar buffer[1024];
				mpd_song_markup(buffer, 1024,
						PLAYLIST_LIST(tree_model)->markup,
						data->song);
				g_value_set_string(value, buffer);
				break;
			}
		case PLAYLIST_LIST_COL_SONG_FILE:
			g_value_set_string(value, data->song->file);
			break;
		case PLAYLIST_LIST_COL_SONG_ARTIST:
			g_value_set_string(value, data->song->artist);
			break;
		case PLAYLIST_LIST_COL_SONG_ALBUM:
			g_value_set_string(value, data->song->album);
			break;
		case PLAYLIST_LIST_COL_SONG_TITLE:
			g_value_set_string(value, data->song->title);
			break;
		case PLAYLIST_LIST_COL_SONG_TITLEFILE:
			if(data->song->title == NULL) {
				gchar *path = g_path_get_basename(data->song->file);
				g_value_set_string(value, path);
				g_free(path);

			}else{
				g_value_set_string(value, data->song->title);
			}
			break;                                       			
		case PLAYLIST_LIST_COL_SONG_GENRE:
			g_value_set_string(value, data->song->genre);
			break;
		case PLAYLIST_LIST_COL_SONG_TRACK:
			g_value_set_string(value, data->song->track);
			break;
		case PLAYLIST_LIST_COL_SONG_NAME:
			g_value_set_string(value, data->song->name);
			break;
		case PLAYLIST_LIST_COL_SONG_COMPOSER:
			g_value_set_string(value, data->song->composer);
			break;
		case PLAYLIST_LIST_COL_SONG_DATE:
			g_value_set_string(value, data->song->date);
			break;
		case PLAYLIST_LIST_COL_SONG_LENGTH:
			g_value_set_int(value, data->song->time);
			break;
		case PLAYLIST_LIST_COL_SONG_POS:
			g_value_set_int(value, data->song->pos);
			break;
		case PLAYLIST_LIST_COL_SONG_ID:
			g_value_set_int(value, data->song->id);
			break;

		case PLAYLIST_LIST_COL_ICON_ID:
			if (data->song->pos ==
					PLAYLIST_LIST(tree_model)->current_song_pos) {
				g_value_set_string(value, "gtk-media-play");
			} else if (strstr(data->song->file, "://")) {
				g_value_set_string(value, "media-stream");
			} else {
				g_value_set_string(value, "media-audiofile");
			}
			break;

	}
}

/*****************************************************************************
 *
 *  playlist_list_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_next(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	CustomList *playlist_list;
	MpdData *data = NULL;
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

	if (iter == NULL || iter->user_data == NULL)
		return FALSE;

	playlist_list = PLAYLIST_LIST(tree_model);

	data = (MpdData *) iter->user_data;

	/* Is this the last record in the list? */
	if (mpd_data_is_last(data))
		return FALSE;
	data = mpd_data_get_next(data);

	iter->stamp = playlist_list->stamp;
	iter->user_data = data;

	return TRUE;
}

/*****************************************************************************
 *
 *  playlist_list_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_children(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreeIter * parent)
{
	CustomList *playlist_list;

	g_return_val_if_fail(parent == NULL
			|| parent->user_data != NULL, FALSE);

	/* this is a list, nodes have no children */
	if (parent)
		return FALSE;

	/* parent == NULL is a special case; we need to return the first top-level row */

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

	playlist_list = PLAYLIST_LIST(tree_model);

	/* No rows => no first row */
	if (playlist_list->num_rows == 0)
		return FALSE;

	/* Set iter to first item in list */
	iter->stamp = playlist_list->stamp;
	iter->user_data = mpd_data_get_first((MpdData *) playlist_list->mpdata);

	return TRUE;
}

/*****************************************************************************
 *
 *  playlist_list_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *                              We only have a list and thus no children.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_has_child(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	return FALSE;
}

/*****************************************************************************
 *
 *  playlist_list_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/

	static gint
playlist_list_iter_n_children(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	CustomList *playlist_list;

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), -1);
	g_return_val_if_fail(iter == NULL || iter->user_data != NULL, FALSE);

	playlist_list = PLAYLIST_LIST(tree_model);

	/* special case: if iter == NULL, return number of top-level rows */
	if (!iter)
		return playlist_list->num_rows;

	return 0;		/* otherwise, this is easy again for a list */
}

/*****************************************************************************
 *
 *  playlist_list_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_nth_child(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
	CustomList *playlist_list;
	MpdData *data;

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

	playlist_list = PLAYLIST_LIST(tree_model);

	/* a list has only top-level rows */
	if (parent)
		return FALSE;

	/* special case: if parent == NULL, set iter to n-th top-level row */

	if (n >= playlist_list->num_rows)
		return FALSE;

	/*: if n > data->song->pos then up, else get the first item */
	data = (MpdData *) playlist_list->mpdata;
	if (data->song->pos > n)
		data = mpd_data_get_first(data);
	for (; data->song->pos != n &&
			!mpd_data_is_last(data); data = mpd_data_get_next(data)) ;

	g_assert(data != NULL);
	g_assert(data->song->pos == n);
	/* to improve lineair searches? */
	playlist_list->mpdata = data;
	iter->stamp = playlist_list->stamp;
	iter->user_data = data;

	return TRUE;
}

/*****************************************************************************
 *
 *  playlist_list_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_parent(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreeIter * child)
{
	return FALSE;
}

/*****************************************************************************
 *
 *  playlist_list_new:  This is what you use in your own code to create a
 *                    new custom list tree model for you to use.
 *
 *****************************************************************************/

CustomList *playlist_list_new(void)
{
	CustomList *newcustomlist;

	newcustomlist = (CustomList *) g_object_new(CUSTOM_TYPE_LIST, NULL);

	g_assert(newcustomlist != NULL);

	return newcustomlist;
}
