make mtc_test 

G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind \
    --tool=memcheck --leak-check=full --leak-resolution=high \
    --num-callers=20 --show-reachable=yes \
    ./mtc_test > /dev/null

time ./mtc_test > /dev/null
