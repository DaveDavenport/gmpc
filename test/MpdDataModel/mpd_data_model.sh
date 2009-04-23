make mpd_data_model

G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind \
    --tool=memcheck --leak-check=full --leak-resolution=high \
    --num-callers=20 --show-reachable=yes \
    ./mpd_data_model > /dev/null

time ./mpd_data_model > /dev/null
