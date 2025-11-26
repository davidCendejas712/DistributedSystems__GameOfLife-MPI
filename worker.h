#include "mpi.h"
#include "world.h"

// Enables/Disables the log messages from worker processes
#define DEBUG_WORKER 0

void printWorldRoww(unsigned short *world, int rowIndex, int worldWidth);

void invocarWorker(int worldWidth, int worldHeight);

