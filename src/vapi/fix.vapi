using Gtk;

namespace FixGtk
{
	[CCode (cheader_filename = "gtk/gtk.h",cname="GtkBuildable",cprefix="gtk_", type_id="GTK_TYPE_BUILDABLE")]
	public interface Buildable {
		public virtual void add_child (Gtk.Builder builder, GLib.Object child, string? type);
		public virtual unowned GLib.Object construct_child (Gtk.Builder builder, string name);
		public virtual void custom_finished (Gtk.Builder builder, GLib.Object? child, string tagname, void* data);
		public virtual void custom_tag_end (Gtk.Builder builder, GLib.Object? child, string tagname, out void* data);
		public virtual bool custom_tag_start (Gtk.Builder builder, GLib.Object? child, string tagname, out GLib.MarkupParser parser, out void* data);
		public virtual unowned GLib.Object get_internal_child (Gtk.Builder builder, string childname);
		public virtual unowned string get_name ();
		public virtual void parser_finished (Gtk.Builder builder);
		public virtual void set_buildable_property (Gtk.Builder builder, string name, GLib.Value value);
		public virtual void set_name (string name);
	}
}
