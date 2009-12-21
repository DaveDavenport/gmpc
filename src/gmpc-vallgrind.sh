
G_SLICE=debug-blocks G_DEBUG=gc-friendly valgrind \
    --tool=memcheck --leak-check=full --leak-resolution=high \
    --num-callers=20 \
    --suppressions=../scripts/gtk.suppression \
    ./gmpc > /dev/null

