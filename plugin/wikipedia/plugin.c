#include <stdio.h>
#include <string.h>
#include <gtkmozembed.h>
#include "../../src/plugin.h"
GtkWidget *wp_pref_vbox = NULL;
void wp_add(GtkWidget *cat_tree);
void wp_selected(GtkWidget *container);
void wp_unselected(GtkWidget *container);
void wp_changed(GtkWidget *tree, GtkTreeIter *iter);

GtkWidget *moz = NULL;
GtkWidget *wp_vbox = NULL;
GtkWidget *pgb = NULL;

void wp_construct(GtkWidget *container);
void wp_destroy(GtkWidget *container);

gmpcPrefPlugin wp_gpp = {
	wp_construct,
	wp_destroy
};

/* Needed plugin_wp stuff */
gmpcPlBrowserPlugin wp_gbp = {
	wp_add,
	wp_selected,
	wp_unselected,
	wp_changed
};

gmpcPlugin plugin_wp = {
	"wikipedia plugin",
	{0,0,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	&wp_gbp,
	NULL,
	&wp_gpp
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
	wp_vbox = gtk_vbox_new(FALSE, 6);
	moz = gtk_moz_embed_new();
	if(moz == NULL)
	{
		printf("Failed to create mozilla object\n");
	}
	gtk_container_add(GTK_CONTAINER(sw), moz);
	gtk_box_pack_start_defaults(GTK_BOX(wp_vbox), sw);
	pgb = gtk_progress_bar_new();
/*	gtk_box_pack_start(GTK_BOX(wp_vbox), pgb, FALSE, TRUE, 0);*/
	gtk_widget_show_all(wp_vbox);
	g_object_ref(G_OBJECT(wp_vbox));
	g_signal_connect(moz, "progress", G_CALLBACK(wp_progress), NULL);
}

void wp_add(GtkWidget *cat_tree)
{
	GtkTreeStore *pl3_tree = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));	
	GtkTreeIter iter;
	if(!cfg_get_single_value_as_int_with_default(config, "wp-plugin", "enable", 0)) return;
	printf("adding plugin_wp: %i '%s'\n", plugin_wp.id, plugin_wp.name);
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

	if(wp_vbox== NULL)
	{
		wp_init();
	}

	gtk_container_add(GTK_CONTAINER(container), wp_vbox);
	gtk_widget_show_all(wp_vbox);
	while (gtk_events_pending ())
		gtk_main_iteration ();

}

void wp_unselected(GtkWidget *container)
{
	gtk_widget_hide(moz);
	gtk_container_remove(GTK_CONTAINER(container),wp_vbox);
}

void wp_enable_toggle(GtkWidget *wid)
{
	int kk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid));
	cfg_set_single_value_as_int(config, "wp-plugin", "enable", kk);
	pl3_reinitialize_tree();
}
void wp_destroy(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), wp_pref_vbox);
}

void wp_construct(GtkWidget *container)
{
	GtkWidget *enable_cg = gtk_check_button_new_with_mnemonic("_Enable WikiPedia");
	GtkWidget *label = NULL;
	wp_pref_vbox = gtk_vbox_new(FALSE,6);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_cg), 	
			cfg_get_single_value_as_int_with_default(config, "wp-plugin", "enable", 0));
	label = gtk_label_new("WikiPedia Plugin");
	gtk_label_set_markup(GTK_LABEL(label),"<span size=\"large\"><b>WikiPedia Plugin</b></span>");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	
	g_signal_connect(G_OBJECT(enable_cg), "toggled", G_CALLBACK(wp_enable_toggle), NULL);
	gtk_box_pack_start(GTK_BOX(wp_pref_vbox),label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(wp_pref_vbox), enable_cg, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(container), wp_pref_vbox);
	gtk_widget_show_all(container);
}
