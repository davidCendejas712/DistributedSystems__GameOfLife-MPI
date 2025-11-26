#include "worker.h"

void printWorldRoww(unsigned short *world, int rowIndex, int worldWidth) {
    if (!world || rowIndex < 0 || worldWidth <= 0) {
        printf("Error: Parámetros inválidos.\n");
        return;
    }

    for (int col = 0; col < worldWidth; col++) {
        printf("%u ", world[rowIndex * worldWidth + col]);
    }
    printf("\n");
}

void invocarWorker(int worldWidth, int worldHeight){
	MPI_Status Stat;
	int nFilas = 0;
	int filaInicio = 0;
	
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	unsigned short *top;   
	unsigned short *area;
	unsigned short *bottom;
	
	unsigned short *newArea;
	
	tCoordinate *actual = (tCoordinate *)malloc(sizeof(tCoordinate));
	tCoordinate *aux = (tCoordinate *)malloc(sizeof(tCoordinate));
	tCoordinate *auxAbajo = (tCoordinate *)malloc(sizeof(tCoordinate));
	tCoordinate *auxArriba = (tCoordinate *)malloc(sizeof(tCoordinate));
	tCoordinate *aux2 = (tCoordinate *)malloc(sizeof(tCoordinate));
	
	unsigned short int tipoActual;
	
	int k = 0; //sirve para indicar cual es la primera iteracion
	
	do {
		int nVivas = 0;
		
		//Recibe el numero de filas
		MPI_Recv(&nFilas, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD, &Stat);
	
		if (nFilas != END_PROCESSING){
			if (k == 0){
				//En la primera iteracion, se inicializan los buffers
				top = (unsigned short*) malloc(worldWidth * sizeof(unsigned short));
				area = (unsigned short*) malloc(worldWidth * nFilas * sizeof(unsigned short));
				bottom = (unsigned short*) malloc(worldWidth * sizeof(unsigned short));
	
				newArea = (unsigned short*) malloc(worldWidth * nFilas * sizeof(unsigned short));
				clearWorld(newArea, worldWidth, nFilas);
				k = 0;
			}
	
			// Se recibe el nº de la primera fila (solo para devolverselo al master)
			MPI_Recv(&filaInicio, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD, &Stat); 
	
			//Recibe los vectores top, area y bottom
			MPI_Recv(top, worldWidth, MPI_UNSIGNED_SHORT, MASTER, 1, MPI_COMM_WORLD, &Stat);	
			MPI_Recv(area, worldWidth * nFilas, MPI_UNSIGNED_SHORT, MASTER, 1, MPI_COMM_WORLD, &Stat);	
			MPI_Recv(bottom, worldWidth, MPI_UNSIGNED_SHORT, MASTER, 1, MPI_COMM_WORLD, &Stat);
	
			//calculo
			for (int j = 0; j < nFilas; j++){
				for (int i = 0; i < worldWidth; i++){
					actual->col = i; actual->row = j;
					tipoActual = getCellAtWorld (actual, area, worldWidth);
					
					//ARRIBA
					if (j == 0){   //caso limite, se usa top
						//ARRIBA
						aux->col = actual->col;
						aux->row = 0;
						if(getCellAtWorld (aux, top, worldWidth) == CELL_LIVE){
							nVivas++;
						}
						auxArriba = aux;
			
						//ARRIBA IZQ
						getCellLeft(auxArriba, worldWidth, aux2);  	
						if(getCellAtWorld (aux2, top, worldWidth) == CELL_LIVE){
							nVivas++;  
						}
						//ARRIBA DCH
						getCellRight(auxArriba, worldWidth, aux2); 	
						if(getCellAtWorld (aux2, top, worldWidth) == CELL_LIVE) {
							nVivas++; 
						}	
					}
					else{
						//ARRIBA
						getCellUp(actual, aux);				
						if(getCellAtWorld (aux, area, worldWidth) == CELL_LIVE){
							nVivas++;
						}
						auxArriba = aux;
				
						//ARRIBA IZQ
						getCellLeft(auxArriba, worldWidth, aux2);  	
						if(getCellAtWorld (aux2, area, worldWidth) == CELL_LIVE){
							nVivas++;  
						}
						//ARRIBA DCH
						getCellRight(auxArriba, worldWidth, aux2); 	
						if(getCellAtWorld (aux2, area, worldWidth) == CELL_LIVE) {
							nVivas++; 
						}		
					}
				 			
					//IZQ
					getCellLeft(actual, worldWidth, aux);   	
					if(getCellAtWorld (aux, area, worldWidth) == CELL_LIVE) {
						nVivas++;
					}
				
					//DCH
					getCellRight(actual, worldWidth, aux);  	
					if(getCellAtWorld (aux, area, worldWidth) == CELL_LIVE) {
						nVivas++;
					}
				
					//ABAJO
					if (j == nFilas - 1){    //caso limite, se usa el bottom
						//ABAJO
						aux->col = actual->col;
						aux->row = 0;
						if(getCellAtWorld (aux, bottom, worldWidth) == CELL_LIVE){
							nVivas++;
						}
						auxAbajo = aux;
				
						//ABAJO IZQ
						getCellLeft(auxAbajo, worldWidth, aux2);  	
						if(getCellAtWorld (aux2, bottom, worldWidth) == CELL_LIVE){
							nVivas++;
						}
						//ABAJO DCH
						getCellRight(auxAbajo, worldWidth, aux2); 	
						if(getCellAtWorld (aux2, bottom, worldWidth) == CELL_LIVE){
							nVivas++;	
						}
					}
					else{
						//ABAJO
						getCellDown(actual, aux);  
						if(getCellAtWorld (aux, area, worldWidth) == CELL_LIVE){
							nVivas++;
						}
						auxAbajo = aux;
						
						//ABAJO IZQ
						getCellLeft(auxAbajo, worldWidth, aux2);  	
						if(getCellAtWorld (aux2, area, worldWidth) == CELL_LIVE){
							nVivas++;
						}
						//ABAJO DCH
						getCellRight(auxAbajo, worldWidth, aux2); 
						if(getCellAtWorld (aux2, area, worldWidth) == CELL_LIVE){
							nVivas++;
						}
					}
				
					//Computo "vacio" para hacer mas obvia la diferencia de carga entre procesos
					if (nVivas == 0) {
						calculateLonelyCell();
					}
					
					if (tipoActual == CELL_EMPTY){
						//Si está vacía y hay exactamente 3 vivas, nace
						if (nVivas == 3){    
							setCellAt (actual, newArea, worldWidth, CELL_LIVE);
						}
						//Sino, se mantiene vacia
						else{                
							setCellAt (actual, newArea, worldWidth, CELL_EMPTY);
						}
					}
					else if (tipoActual == CELL_LIVE){
						//Si está viva y hay 2 o 3 vivas, se mantiene vivo
						if (nVivas == 3 || nVivas == 2){   
							setCellAt (actual, newArea, worldWidth, CELL_LIVE);
						}
						//Sino, se muere
						else{                
							setCellAt (actual, newArea, worldWidth, CELL_EMPTY);	
						}
					}
					nVivas = 0;
				}
			}
			// Se manda el numero de filas
			MPI_Send(&nFilas, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);

			// Se manda el numero de la primera fila
			MPI_Send(&filaInicio, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);
			
			//Se manda la info
			MPI_Send(newArea, worldWidth * nFilas, MPI_UNSIGNED_SHORT, MASTER, 1, MPI_COMM_WORLD);
		}
	}
	while (nFilas != END_PROCESSING);
	
	printf("Finalizando worker %u\n", rank);
	
	free(top);
	free(area);
	free(bottom);
	free(newArea);
	free(actual);
	free(aux);
	//free(auxArriba);
	//free(auxAbajo);
	free(aux2);
	
}
