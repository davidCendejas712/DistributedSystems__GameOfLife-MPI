mpicc -o lifeGame *.c -I/usr/local/include/SDL2 -L/usr/local/lib/ -lSDL2
mpiexec -hostfile machines -np 3 lifeGame 100 100 2000 auto resul static
