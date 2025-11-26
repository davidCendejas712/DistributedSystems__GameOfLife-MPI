#include "master.h"


void printWorldRow(unsigned short *world, int rowIndex, int worldWidth) {
    if (!world || rowIndex < 0 || worldWidth <= 0) {
        printf("Error: Parámetros inválidos.\n");
        return;
    }

    for (int col = 0; col < worldWidth; col++) {
        printf("%u ", world[rowIndex * worldWidth + col]);
    }
    printf("\n");
}


void invocarMaster(SDL_Window* window, SDL_Renderer* renderer, int nProc, int worldWidth, int worldHeight, int distModeStatic, int grainSize, int totalIterations, char* outputFile, int autoMode){
	//El numero de workers es el nProcesos - 1 (ya que el master no cuenta)
	int nProcesos = nProc - 1; 
	
	unsigned short *worldA;
	unsigned short *worldB;
	
	unsigned short *top;   
	unsigned short *area;
	unsigned short *bottom;
	
	unsigned short *act;
	unsigned short *sig;
	
	unsigned short *ptrRecv;   
	
	MPI_Status Stat;
	
	int grano;
	int nRestantes;
	int workerActual = 1;
	int filaActual = 0;
	int casoImpar = 0;
	
	//Para el dinamico
	int nRecibidas = 0; 
	int nEnviadas = 0; 
	
	int nFilas = 0;
	int filaInicio;
	char ch;
	
	int tableroActual = 1;
	
	int iCataclismo = ITER_CATACLYSM;  //numero de iteraciones sin cataclismo
	int random_num;		  
	int filaMedio;
	int columnaMedio;
	
	tCoordinate *aux = (tCoordinate *)malloc(sizeof(tCoordinate));
	
	//Se inicializan los mundos
	worldA = (unsigned short*) malloc (worldWidth * worldHeight * sizeof (unsigned short));
	clearWorld(worldA, worldWidth, worldHeight);
	initRandomWorld(worldA, worldWidth, worldHeight);
	
	worldB = (unsigned short*) malloc (worldWidth * worldHeight * sizeof (unsigned short));
	clearWorld(worldB, worldWidth, worldHeight);
	
	//Estos 2 punteros marcan que tablero es el actual y cual es el siguiente,
	//En el drawWorld se meteran estos    
	act = worldA;
	sig = worldB;
	
	// Se dibuja la iteracion 0
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(renderer);		
	drawWorld(worldA, worldA, renderer, 0, worldHeight - 1, worldWidth, worldHeight);
	SDL_RenderPresent(renderer);
	SDL_UpdateWindowSurface(window);

	if (!autoMode){
		printf("\nPULSA ENTER PARA COMENZAR\n");
		ch = getchar();
	}
		
	// Comienza la partida 
	for (int k = 0; k < totalIterations; k++){
		//ESTATICO, se reparte toda la carga antes de la ejecucion 
		if (distModeStatic){
			// Se calcula el numero de filas que se enviaran a cada worker
			grano = worldHeight / nProcesos;
			
			nRestantes = worldHeight;
			filaActual = 0;
			
			// ========= REPARTO DE TODA LA TAREA ============
			for (int i = 0; i < nProcesos; i++){
				//Ultimo caso en tableros impares, se le otorga una fila extra
				if (nRestantes == grano+1){    
					casoImpar = 1;
				}
				
				nFilas = grano + casoImpar;
					
				//Si esta al principio
				if (filaActual == 0){
					top = act + (worldWidth * (worldHeight - 1));  // el top es la ultima fila
					area = act + (worldWidth * filaActual);
					bottom = act + (worldWidth * (filaActual + grano + casoImpar));
				}
				//Si esta al final
				else if ((filaActual + grano + casoImpar) == worldHeight){
					bottom = act;   // el bottom es la primera fila
					top = act + (worldWidth * (filaActual - 1));
					area = act + (worldWidth * filaActual);
				}
				//	Si es una porcion intermedia
				else{
					top = act + (worldWidth * (filaActual - 1));
					area = act + (worldWidth * filaActual);
					bottom = act + (worldWidth * (filaActual + grano + casoImpar));
				}
				
				// ========= SE ENVIA LA INFO A LOS WORKERS =========
				
				// Se manda el numero de filas
				MPI_Send(&nFilas, 1, MPI_INT, workerActual, 1, MPI_COMM_WORLD);
				
				// Se manda el numero de la primera fila
				MPI_Send(&filaActual, 1, MPI_INT, workerActual, 1, MPI_COMM_WORLD);
				
				// Se mandan las filas
				MPI_Send(top, worldWidth, MPI_UNSIGNED_SHORT, workerActual, 1, MPI_COMM_WORLD);
				MPI_Send(area, worldWidth * nFilas, MPI_UNSIGNED_SHORT, workerActual, 1, MPI_COMM_WORLD);
				MPI_Send(bottom, worldWidth, MPI_UNSIGNED_SHORT, workerActual, 1, MPI_COMM_WORLD);
			
				nRestantes -= grano;
				filaActual += grano;
				workerActual++;
			}
			
			casoImpar = 0;
			workerActual = 1;
			 									
			// ========= SE RECIBE LA INFO DE LOS WORKERS  =========								
			
			for (int i = 0; i < nProcesos; i++){
				// Se recibe el nº de filas del primer worker que haya acabado
				MPI_Recv(&nFilas, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &Stat); 
				
				// Se recibe el nº de la primera fila
				MPI_Recv(&filaInicio, 1, MPI_INT, Stat.MPI_SOURCE, 1, MPI_COMM_WORLD, &Stat); 

				// Se reciben las filas de ese worker			
				ptrRecv = sig + (filaInicio * worldWidth);				
				MPI_Recv(ptrRecv, worldWidth * nFilas, MPI_UNSIGNED_SHORT, Stat.MPI_SOURCE, 1, MPI_COMM_WORLD, &Stat);
			}
		}	
		//DINAMICO, se tiene en cuenta el tamaño de grano (tamaño de datos maximo para cada worker)
		else{
			grano = grainSize;
			nRestantes = worldHeight;
			nEnviadas = 0;
			nRecibidas = 0;
			filaActual = 0;
			workerActual = 1;
			// ========= REPARTO DE LA TAREA INICIAL ============
			for (int i = 0; i < nProcesos; i++){
				nFilas = grano;
					
				//Si esta al principio
				if (filaActual == 0){
					top = act + (worldWidth * (worldHeight - 1));  // el top es la ultima fila
					area = act + (worldWidth * filaActual);
					bottom = act + (worldWidth * (filaActual + nFilas));
				}
				//Si esta al final
				else if ((filaActual + nFilas) == worldHeight){
					bottom = act;   // el bottom es la primera fila
					top = act + (worldWidth * (filaActual - 1));
					area = act + (worldWidth * filaActual);
				}
				//	Si es una porcion intermedia
				else{
					top = act + (worldWidth * (filaActual - 1));
					area = act + (worldWidth * filaActual);
					bottom = act + (worldWidth * (filaActual + nFilas));
				}
				
				// ========= SE ENVIA LA INFO A LOS WORKERS =========
				
				// Se manda el numero de filas
				MPI_Send(&nFilas, 1, MPI_INT, workerActual, 1, MPI_COMM_WORLD);
				
				// Se manda el numero de la primera fila
				MPI_Send(&filaActual, 1, MPI_INT, workerActual, 1, MPI_COMM_WORLD);
				
				// Se mandan las filas
				MPI_Send(top, worldWidth, MPI_UNSIGNED_SHORT, workerActual, 1, MPI_COMM_WORLD);
				MPI_Send(area, worldWidth * nFilas, MPI_UNSIGNED_SHORT, workerActual, 1, MPI_COMM_WORLD);
				MPI_Send(bottom, worldWidth, MPI_UNSIGNED_SHORT, workerActual, 1, MPI_COMM_WORLD);
				
				nRestantes -= nFilas;
				nEnviadas += nFilas;
				filaActual += nFilas;
				workerActual++;
			}
			
			// ========= SE RECIBE LA INFO DE LOS WORKERS  =========
			while (nRecibidas < worldHeight){
				// Se recibe el nº de filas del primer worker que haya acabado
				MPI_Recv(&nFilas, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &Stat); 
				
				// Se recibe el nº de la primera fila
				MPI_Recv(&filaInicio, 1, MPI_INT, Stat.MPI_SOURCE, 1, MPI_COMM_WORLD, &Stat); 

				// Se reciben las filas de ese worker			
				ptrRecv = sig + (filaInicio * worldWidth);				
				MPI_Recv(ptrRecv, worldWidth * nFilas, MPI_UNSIGNED_SHORT, Stat.MPI_SOURCE, 1, MPI_COMM_WORLD, &Stat);
				
				nRecibidas += nFilas;
				
				//Si aun quedan filas por enviar, se envian al primer proceso disponible
				if (nEnviadas < worldHeight){
					if (nRestantes < grano){
						nFilas = nRestantes;
					}
					else{
						nFilas = grano;
					}
					//Si esta al principio
					if (filaActual == 0){
						top = act + (worldWidth * (worldHeight - 1));  // el top es la ultima fila
						area = act + (worldWidth * filaActual);
						bottom = act + (worldWidth * (filaActual + nFilas));
					}
					//Si esta al final
					else if ((filaActual + nFilas) == worldHeight){
						bottom = act;   // el bottom es la primera fila
						top = act + (worldWidth * (filaActual - 1));
						area = act + (worldWidth * filaActual);
					}
					//	Si es una porcion intermedia
					else{
						top = act + (worldWidth * (filaActual - 1));
						area = act + (worldWidth * filaActual);
						bottom = act + (worldWidth * (filaActual + nFilas));
					}
				
					// ========= SE ENVIA LA INFO AL PRIMER WORKER LISTO =========
					
					// Se manda el numero de filas
					MPI_Send(&nFilas, 1, MPI_INT, Stat.MPI_SOURCE, 1, MPI_COMM_WORLD);
				
					// Se manda el numero de la primera fila
					MPI_Send(&filaActual, 1, MPI_INT, Stat.MPI_SOURCE, 1, MPI_COMM_WORLD);
				
					// Se mandan las filas
					MPI_Send(top, worldWidth, MPI_UNSIGNED_SHORT, Stat.MPI_SOURCE, 1, MPI_COMM_WORLD);
					MPI_Send(area, worldWidth * nFilas, MPI_UNSIGNED_SHORT, Stat.MPI_SOURCE, 1, MPI_COMM_WORLD);
					MPI_Send(bottom, worldWidth, MPI_UNSIGNED_SHORT, Stat.MPI_SOURCE, 1, MPI_COMM_WORLD);
			
					nRestantes -= nFilas;
					nEnviadas += nFilas;
					filaActual += nFilas;
				}	
			}
		}
		
		// ======= CODIGO COMUN ENTRE ESTATICO Y DINAMICO =========		 

		if (tableroActual == 1){ 
			act = worldA; sig = worldB ;
		}
		else { 
			act = worldB; sig = worldA;
		}

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(renderer);	
						
		// ========= CATACLISMO ==========
		
		if (iCataclismo == ITER_CATACLYSM){     //Cada ITER_CATACLYSM iteraciones, se comprueba si hay o no cataclismo
			random_num = rand() % 100;  // Genera un número aleatorio entre 0 y 99  
			if (random_num < PROB_CATACLYSM) {    // Si el número está en el rango [0 - PROB_CATACLYSM], ocurre cataclismo
				columnaMedio = worldWidth / 2;
				filaMedio = worldHeight / 2;
				for (int n = 0; n < worldHeight; n++){
					if (n != filaMedio){
						aux->col = columnaMedio;
						aux->row = n;
						setCellAt(aux,act,worldWidth,CELL_CATACLYSM);
						setCellAt(aux,sig,worldWidth,CELL_EMPTY);
					}
					else{
						for (int m = 0; m < worldWidth; m++){
							aux->col = m;
							aux->row = n;
							setCellAt(aux,act,worldWidth,CELL_CATACLYSM);
							setCellAt(aux,sig,worldWidth,CELL_EMPTY);
						}
					}
				}
			}
			iCataclismo = 1;
		}
		else{
			iCataclismo++;
		} 
		
		// ========= SE DIBUJA =========
		
		// Una vez recibidos toda la informacion, se pinta la partida
		drawWorld(act, sig, renderer, 0, worldHeight - 1, worldWidth, worldHeight);
			
		//Se cambian los tableros act y sig
		if (tableroActual == 1){
			act = worldB;
			sig = worldA;
			tableroActual = 2;
		}
		else{
			act = worldA;
			sig = worldB;
			tableroActual = 1;
		}
			
		//Update the surface
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
		
		if (!autoMode){
			printf("\nPULSA ENTER PARA CONTINUAR\n");
			ch = getchar();
		}
	}
	
	//Se envia END_PROCESSING a cada worker	
	nFilas = END_PROCESSING;
	for (int i = 1; i < nProc; i++){
		MPI_Send(&nFilas, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
	}
		
	// Destroy window		
	SDL_DestroyWindow(window);
	// Exiting...
	SDL_Quit();
	
	
	free(worldA);
	free(worldB);

	free(aux);
	
}
