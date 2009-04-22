make read_conf

cp read_conf_test.db2 read_conf_test.db2.test
G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind \
    --tool=memcheck --leak-check=full --leak-resolution=high \
    --num-callers=20 --show-reachable=yes \
    ./read_conf read_conf_test.db2.test > /dev/null

time ./read_conf read_conf_test.db2.test > /dev/null
rm read_conf_test.db2.test
