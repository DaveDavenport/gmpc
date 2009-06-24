G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 src/gmpc 
