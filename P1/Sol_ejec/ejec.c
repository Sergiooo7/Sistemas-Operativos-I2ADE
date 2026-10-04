#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

/// Variables globales para almacenar los PID de la jerarquía
pid_t arb, a, b, x, y, z;

// Función ejecutada por el proceso A tras recibir la señal SIGUSR1 enviada por Z
void ejecuta_pstree(){
	char arb_cad[100];
	sprintf(arb_cad, "%d", arb);
	
	if(fork() == 0){
		// Reemplaza la imagen del hijo con el comando pstree mostrando el árbol desde arb
		execlp("pstree", "pstree", "-c", arb_cad, NULL);
	}
	else{
		// A espera a que el proceso del comando pstree finalice
		wait(NULL); 
		// Notifica a arb que el comando terminó para iniciar la cadena de destrucción
		kill(arb, SIGUSR2);	
	}
}

// Manejadores de señal para la cadena de destrucción
void destroy_ejec(){
	// ejec le indica a A que continúe con la destrucción
	kill(a, SIGUSR2);
}

void destroy_A(){
	// A le indica a B que inicie la destrucción de sus hijos
	kill(b, SIGUSR2);
}

// Manejador genérico para señales que solo desbloquean un pause()
void nada(){
}

int main(int argc, char *argv[]){
	int tiempo;
	
	if(argc != 2){
		printf("Usage: ./ejec tiempo\n");
		exit(1);
	}
	else{
		// Conversión del tiempo recibido como parámetro a entero
		tiempo = atoi(argv[1]);	
		arb = getpid();
		printf("Soy el proceso ejec: mi pid es %d\n", arb);
		
		a = fork();	// Creación del proceso A
		if(a != 0){
			// ==== CÓDIGO DEL PROCESO EJEC ====
			signal(SIGUSR2, destroy_ejec);
			pause(); // Espera a que A termine pstree y mande orden de destrucción
			
			wait(NULL); // Espera a que A muera completamente
			printf("Soy ejec (%d) y muero\n", arb); 
			exit(0);
		}
		else{
			// ==== CÓDIGO DEL PROCESO A ====
			a = getpid();
			printf("Soy el proceso A: mi pid es %d. Mi padre es %d\n", a, arb);	
			
			b = fork(); // Creación del proceso B
			if(b != 0){
				// =============== CONTINUACIÓN DEL PROCESO A ============================
				signal(SIGUSR1, ejecuta_pstree);
				signal(SIGUSR2, destroy_A);
				
				pause(); // Espera la llegada de SIGUSR1 enviada por Z para ejecutar pstree
				wait(NULL); // Espera a que el proceso B muera
				printf("Soy A (%d) y muero\n", a);
				exit(0);
			}
			else{
				// ==== CÓDIGO DEL PROCESO B ====
				b = getpid();
				printf("Soy el proceso B: mi pid es %d. Mi padre es %d. Mi abuelo es %d\n", b, a, arb);
				
				x = fork(); // Creación del proceso X
				if(x == 0){
					// ==== CÓDIGO DEL PROCESO X ====
					printf("Soy el proceso X: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", getpid(), b, a, arb);
					signal(SIGUSR2, nada);	
					pause(); // Espera la señal de B para morir
					printf("Soy X (%d) y muero\n", getpid());
					exit(0);
				}
				
				y = fork(); // Creación del proceso Y
				if(y == 0){
					// ==== CÓDIGO DEL PROCESO Y ====
					printf("Soy el proceso Y: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", getpid(), b, a, arb);
					signal(SIGUSR2, nada);
					pause(); // Espera la señal de B para morir
					printf("Soy Y (%d) y muero\n", getpid());
					exit(0);
				}
				
				z = fork(); // Creación del proceso Z
				if(z == 0){
					// ==== CÓDIGO DEL PROCESO Z ====
					printf("Soy el proceso Z: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", getpid(), b, a, arb);
					
					signal(SIGALRM, nada); // Prepara la captura de SIGALRM
					alarm(tiempo);	// Programación del temporizador
					pause(); // Se duerme hasta cumplir los segundos
					
					kill(a, SIGUSR1); // Notifica a A que debe ejecutar pstree
					
					signal(SIGUSR2, nada);
					pause(); // Espera orden final de B para morir
					printf("Soy Z (%d) y muero\n", getpid());
					exit(0);
				}
				
				// ==== CONTINUACIÓN DEL PROCESO B ====
				signal(SIGUSR2, nada);
				pause(); // Espera a que A le autorice a destruir a sus hijos
				
				// Destrucción ordenada en secuencia inversa (Z -> Y -> X)
				kill(z, SIGUSR2); // z se despierta y muere
				wait(NULL); 
				
				kill(y, SIGUSR2); // y se despierta y muere
				wait(NULL);
				
				kill(x, SIGUSR2); // x se despierta y muere
				wait(NULL);
				
				printf("Soy B (%d) y muero\n", b);
				exit(0); 
			}
		}
	}
	return 0;
}
