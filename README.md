Game of Life (MPI)

## Project Summary

This project implements a parallel version of **Conway’s Game of Life** using **MPI** (master/worker model) and displays the simulation using **SDL2**. The code distributes portions of the world among worker processes, which compute the next generation, while the master process coordinates the work, collects results, and renders the display.

Video [here](https://youtu.be/kzehSRYD4RQ)

---

## Repository Structure

* `lifeGame.c` — Main program (argument parsing, master/worker startup).
* `master.c` / `master.h` — Master process logic (row distribution, result collection, rendering, image saving).
* `worker.c` / `worker.h` — Worker logic (receiving rows, computing next generation, sending results).
* `world.c` / `world.h` — World utilities (cell access, coordinate handling, initialization).
* `graph.c` / `graph.h` — Drawing functions with SDL2 and BMP saving.
* `types.h` — Types, constants, and macros (e.g., `SEED`, initial percentages, `END_PROCESSING`).
* `machines` — Example hostfile for `mpiexec`.
* `compile*.sh` — Compilation scripts (`compileVM.sh`, `compileMacOS.sh`, `compileLab.sh`).
* `nombres.txt` — Auxiliary file (not critical to execution).

---

## Requirements

* MPI (e.g., `mpicc`, `mpiexec` — Open MPI or MPICH).
* SDL2 (headers and library for rendering and image saving).
* Unix-like system (scripts are designed for Linux/macOS).

Compilation example (already included in the scripts):

```bash
# Linux/VM (adjust SDL2 paths as needed)
./compileVM.sh
# Or compile manually:
mpicc -o lifeGame *.c -I/usr/local/include/SDL2 -L/usr/local/lib/ -lSDL2
```

---

## Usage

```
mpiexec -np <N> lifeGame worldWidth worldHeight iterations [step|auto] outputImage [static|dynamic grainSize]
```

* `worldWidth` `worldHeight`: world dimensions (cells).
* `iterations`: number of simulation iterations.
* `step` or `auto`: execution mode (`step` waits for interaction; `auto` advances automatically).
* `outputImage`: prefix/name for output BMP images.
* `static` or `dynamic grainSize`: work distribution method.

  * `static`: fixed-size portions assigned to each worker.
  * `dynamic`: the master sends portions of size `grainSize` dynamically to balance load.

Example (included in `compileVM.sh`):

```bash
mpiexec -hostfile machines -np 3 lifeGame 100 100 2000 auto resul static
```

---

## Origin of the Game

The **Game of Life** is a cellular automaton created in 1970 by mathematician **John Horton Conway**. On a grid of cells, each cell lives, dies, or is born based on the number of living neighbors:

1. A live cell with fewer than 2 live neighbors dies (underpopulation).
2. A live cell with 2 or 3 neighbors survives.
3. A live cell with more than 3 neighbors dies (overpopulation).
4. A dead cell with exactly 3 neighbors becomes alive.

Despite its simple rules, the game produces complex emergent behavior and has been studied extensively in computing, automata theory, and complex systems.

---

## MPI Technology — Overview and Why It Is Used Here

**MPI (Message Passing Interface)** is a standard for distributed-memory parallel programming. It enables independent processes (possibly on different machines) to exchange messages and collaborate on a parallel task.

Core MPI features used:

* `MPI_Init`, `MPI_Finalize` — start/stop MPI environment.
* `MPI_Comm_size`, `MPI_Comm_rank` — number of processes and ID of each process.
* `MPI_Send`, `MPI_Recv` — point-to-point communication.
* `MPI_Wtime` — performance timing.
* `MPI_Abort` — terminate execution across all processes.

Why MPI fits this project:

* The Game of Life is naturally parallelizable by splitting the grid into rows or blocks.
* Each process only needs its assigned portion plus adjacent border rows.
* MPI allows scaling across clusters or multi-core machines.

---

## Design and Implementation (Your Approach)

### 1) Master/Worker Model

* **Master** initializes the world, distributes work (row blocks), receives results, merges them into the next generation, renders the world using SDL2, and saves images.
* **Workers** receive a block of rows plus the `top` and `bottom` neighbor rows, compute the next generation for their block, and send it back.

### 2) Work Partitioning and Load Balancing

Two execution modes:

* **Static**: fixed number of rows per worker.
* **Dynamic**: the master assigns work in chunks of `grainSize` on demand, improving load balancing.

`grainSize` determines the rows per assigned block.

### 3) Communication

* Messages use a single tag (value `1`).
* MPI types: `MPI_INT` for sizes/offsets, `MPI_UNSIGNED_SHORT` for cell blocks.
* End-of-work signal uses `END_PROCESSING` = `-1`.

### 4) World Borders

The world behaves like a **torus** (wrap-around edges).

* Master constructs appropriate `top` and `bottom` rows.
* Workers then compute neighbors correctly even at boundaries.

### 5) Extra Computational Load

A special function `calculateLonelyCell()` injects extra work (matrix multiplications) when a cell has no neighbors:

* Used to simulate uneven loads.
* Helps demonstrate the benefit of dynamic scheduling.

### 6) Additional Features

* Global parameters in `types.h`: `SEED`, `INITIAL_CELLS_PERCENTAGE`, `PROB_CATACLYSM`, `ITER_CATACLYSM`.
* SDL-based rendering and BMP saving in `graph.c`.

---

## Relevant Defaults

* `PROB_CATACLYSM`: 100
* `ITER_CATACLYSM`: iteration interval for catastrophic events.
* `SEED`: 123
* `INITIAL_CELLS_PERCENTAGE`: 30%

---

## Example Runs

1. Local execution with 4 processes:

```bash
mpiexec -np 4 ./lifeGame 200 150 1000 auto result static
```

2. Dynamic scheduling with `grainSize = 4`:

```bash
mpiexec -np 6 ./lifeGame 400 300 2000 auto result dynamic 4
```

---

## Possible Improvements

* Use `MPI_Isend` / `MPI_Irecv` for overlapped communication/computation.
* Use `MPI_Scatterv` / `MPI_Gatherv` for regular block distribution.
* Collect detailed timing statistics for scalability analysis.
* Add option to disable extra computational load.
* Export per-worker performance metrics.

---



*Generated based on the analysis of the provided source code (`.c` and `.h` files).*
