clang++ -O3 -Wall `sdl2-config --cflags` `sdl2-config --libs` -lSDL2_image -framework OpenGL src/main.cpp -o voxel
