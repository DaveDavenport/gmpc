make 

G_SLICE=debug-blocks G_DEBUG=gc-friendly valgrind \
    --tool=memcheck --leak-check=full --leak-resolution=high \
    --num-callers=20 --show-reachable=yes \
    --suppressions=../../scripts/gtk.suppression \
    ./pixbuf_cache_test > /dev/null

