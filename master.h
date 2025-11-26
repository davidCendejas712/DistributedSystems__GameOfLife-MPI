#include "graph.h"
#include "mpi.h"

// Enables/Disables the log messages from the master process
#define DEBUG_MASTER 0

// Probability that a cataclysm may occur [0-100] :(
#define PROB_CATACLYSM 100

// Number of iterations between two possible cataclysms
#define ITER_CATACLYSM 5

void printWorldRow(unsigned short *world, int rowIndex, int worldWidth);

void invocarMaster(SDL_Window* window, SDL_Renderer* renderer, int nProc, int worldWidth, int worldHeight, int distModeStatic, int grainSize, int totalIterations, char* outputFile, int autoMode);
