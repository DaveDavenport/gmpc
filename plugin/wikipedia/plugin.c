#include <stdio.h>
#include <string.h>
#include <gtkmozembed.h>
#include "../../src/plugin.h"

void wp_add(GtkWidget *cat_tree);
void wp_selected(GtkWidget *container);
void wp_unselected(GtkWidget *container);
void wp_changed(GtkWidget *tree, GtkTreeIter *iter);

GtkWidget *moz = NULL;
GtkWidget *vbox = NULL;
GtkWidget *pgb = NULL;

/* Needed plugin_wp stuff */
gmpcPlBrowserPlugin gbp = {
	wp_add,
	wp_selected,
	wp_unselected,
	wp_changed
};

gmpcMpdSignals gms = {
	NULL,
	NULL,
	NULL,
	NULL
};


gmpcPlugin plugin_wp = {
	"wikipedia plugin_wp",
	{0,0,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	&gbp,
	&gms,
	NULL
};
void wp_changed(GtkWidget *tree, GtkTreeIter *iter){
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		if(song->artist != NULL){
			int i;
			gchar *url = g_strdup_printf("http://wikipedia.com/w/index.php?useskin=chick&title=%s", song->artist);
			for(i=0;i< strlen(url);i++){
				if(url[i] == ' ') url[i] = '_';
			}
			printf("%s\n", url);
			gtk_moz_embed_load_url(GTK_MOZ_EMBED(moz),url);
			g_free(url);
		}
		else{
			gtk_moz_embed_load_url(GTK_MOZ_EMBED(moz), "http://musicpd.org/");
		}
	}
}

void wp_progress(GtkMozEmbed *mozem, int cur, int max)
{
/*	float progress = cur/(float)max;
	printf("%i %i\n", cur,max);
	progress = (progress<0)? 0:(progress>1)? 1:progress;
	if(max == -1)
	{
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pgb));
	}
	else
	{
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pgb),progress); 
	}
*/
}
void wp_init()
{
	GtkWidget *sw =gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(sw), GTK_SHADOW_ETCHED_IN);
	vbox = gtk_vbox_new(FALSE, 6);
	moz = gtk_moz_embed_new();
	if(moz == NULL)
	{
		printf("Failed to create mozilla object\n");
	}
	gtk_container_add(GTK_CONTAINER(sw), moz);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), sw);
	pgb = gtk_progress_bar_new();
/*	gtk_box_pack_start(GTK_BOX(vbox), pgb, FALSE, TRUE, 0);*/
	gtk_widget_show_all(vbox);
	g_object_ref(G_OBJECT(vbox));
	g_signal_connect(moz, "progress", G_CALLBACK(wp_progress), NULL);
}

void wp_add(GtkWidget *cat_tree)
{
	GtkTreeStore *pl3_tree = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));	
	GtkTreeIter iter;
	printf("adding plugin_wp: %i '%s'\n", (&plugin_wp)->id, plugin_wp.name);
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, plugin_wp.id,
			PL3_CAT_TITLE, "Wikipedia Lookup",
			PL3_CAT_INT_ID, "/",
			PL3_CAT_ICON_ID, "gtk-info",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
}

void wp_selected(GtkWidget *container)
{
	if(vbox== NULL)
	{
		wp_init();
	}

	gtk_container_add(GTK_CONTAINER(container), vbox);
	gtk_widget_show_all(vbox);
	while (gtk_events_pending ())
		gtk_main_iteration ();

}

void wp_unselected(GtkWidget *container)
{
	gtk_widget_hide(moz);
	gtk_container_remove(GTK_CONTAINER(container),vbox);
}















