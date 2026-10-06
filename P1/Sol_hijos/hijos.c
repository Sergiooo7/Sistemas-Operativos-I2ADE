#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Manejador de señal vacío utilizado para capturar SIGALRM y desbloquear el pause()
void controlAlarm(){}

int main(int argc, char *argv[]){
	int i, x, y, shmidX, shmidY;
	int *pidX, *pidY; // Punteros a los vectores de memoria compartida para los PIDs
	pid_t pid, pidHijos;
	
	if(argc != 3){
		printf("error en argumentos\n");
	}
	else{
		pidHijos = getpid();
		x = atoi(argv[1]);
		y = atoi(argv[2]);
		
		// PROCESO CLAVE 1: Creación e integración de memoria compartida
		shmidX = shmget(IPC_PRIVATE, sizeof(int) * x, IPC_CREAT | 0666);
		pidX = (int *) shmat (shmidX, 0, 0);
		
		// Vector que almacena PIDs de los procesos 'y'.
		shmidY = shmget(IPC_PRIVATE, sizeof(int) * y, IPC_CREAT | 0666);
		pidY = (int *) shmat (shmidY, 0, 0);

		// PROCESO CLAVE 2: Creación de la rama vertical de procesos (X niveles)
		for(i = 1; i <= x; i++){
			pid = fork();
			if(pid != 0){
				wait(NULL); 
				break;
			}
			else{
				// El nuevo proceso almacena su PID en el vector compartido
				pidX[i - 1] = getpid();
				// Muestra por pantalla su PID y los PIDs de sus antecesores
				printf("Soy el proceso %d. Mis padres son: ", getpid());
				printf("%d", pidHijos); 
				
				for(int j = 0; j < i - 1; j++){
					printf(", %d", pidX[j]);
				}
				printf("\n");	
			}
		}
		if(i == 1){ 
			// PROCESO CLAVE 4: Impresión del resultado final y liberación de recursos en el Super Padre
			printf("Soy el super padre %d, mis hijos finales son: ", getpid());
			for(i = 0; i < y; i++){
				printf("%d" ,pidY[i]);
				if(i != y - 1){
					printf(", ");
				}
			}
			printf("\n");
			// Desvinculación y eliminación de los segmentos IPC de memoria compartida
			shmdt(pidX);
			shmdt(pidY);
			shmctl(shmidX, IPC_RMID, NULL);
			shmctl(shmidY, IPC_RMID, NULL);
		}
		else{
			// PROCESO CLAVE 3: El último hijo vertical genera los Y hijos horizontales
			if(i == x + 1){
				for(i = 1; i <= y; i++){
					pid = fork();
					if(pid == 0){
						pidY[i-1] = getpid();
						signal(SIGALRM, controlAlarm);
						alarm(10);
						pause();
						// Limpieza de memoria y salida explícita del hijo horizontal
						shmdt(pidX);
						shmdt(pidY);
						exit(0);
					}
				}
				if(i == y + 1){	// El nodo vertical inferior espera a todos sus hijos
					for(i = 1; i <= y; i++){
						wait(NULL);
					}
				}
			}
			// Desvinculación de la memoria compartida en los procesos intermedios
			shmdt(pidX);
			shmdt(pidY);
		}
	}
	return 0;
}



