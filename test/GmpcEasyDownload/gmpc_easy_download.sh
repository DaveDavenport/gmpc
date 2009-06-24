make gmpc_easy_download

G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind \
    --tool=memcheck --leak-check=full --leak-resolution=high \
    --num-callers=20 --show-reachable=yes \
    ./gmpc_easy_download > /dev/null

time ./gmpc_easy_download > /dev/null
