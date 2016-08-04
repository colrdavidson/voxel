clang -O2 -Wall `sdl2-config --cflags` `sdl2-config --libs` -framework OpenGL src/main.c -o voxel
