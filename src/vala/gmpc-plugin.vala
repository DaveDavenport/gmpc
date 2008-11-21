using GLib;
using Gtk;


namespace Gmpc {
    public interface Plugin {
        public abstract string get_name ();
        public abstract int[3] get_version ();
      
        /*  */
        public abstract void save_yourself ();

        /* Get/set enabled */
        public abstract bool get_enabled ();
        public abstract void set_enabled (bool state);
    }

    public abstract class PluginBase : Plugin { 
        public abstract string get_name ();
        public abstract int[3] get_version ();
        
        public abstract void save_yourself ();

        public abstract bool get_enabled ();
        public abstract void set_enabled (bool state);
    }
    public interface MetaData : Plugin {
       public abstract int get_data ();
       /* Set get priority */
       public abstract int get_priority ();
       public abstract void set_priority (int priority);
    }
    public interface Browser : Plugin {
        /* Function is called by gmpc, the plugin should then insert itself in the left tree  */
        public abstract  void add (Gtk.Widget *category_tree);
        /* This gets called, the plugin should add it view in container */
        public abstract void  selected (GtkWidget *container);
        /* Plugin should remove itself from container */
        public abstract void  unselected (GtkWidget *container);

    }
    public interface Preferences : Plugin {
        public abstract Gtk.Widget pref_construct ();
        public abstract Gtk.Widget pref_destroy ();

    }
    public interface SongList : Plugin {
        public abstract int song_list (Gtk.Widget *tree, Gtk.Menu *menu);

    }

}
