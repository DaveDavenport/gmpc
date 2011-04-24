make 

G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind \
    --tool=memcheck --leak-check=full --leak-resolution=high \
    --num-callers=20 --show-reachable=yes \
    --suppressions=../../scripts/gtk.suppression \
    ./async_image_test > /dev/null

