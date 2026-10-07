#include <stdio.h> 
#include <sys/wait.h>
#include <sys/stat.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <string.h>
#include <fcntl.h>

void divFich(char *nombre, int tam);

int main(int argc, char *argv[]){
	if(argc != 3){	
		printf("Error. Uso: %s fichero tam_trozo\n", argv[0]);
	}
	else{
		divFich(argv[1], atoi(argv[2])); 
	}
	return 0;
}

// Divide un fichero en partes mediante tuberías (pipes) y procesos hijos
void divFich(char nombre[], int tam){
	int i;	
	int tubo[2]; // tubo[0] = lectura, tubo[1] = escritura
	char nombreFichero[50];
	int fd_origen, fd_destino; 
	struct stat propFich; // Propiedades del fichero
	int numTrozos, // Número total de trozos a generar 
		numLeidos; // Bytes leídos de la tubería
 	char *lectura;		
		
	lectura = (char *) malloc(sizeof(char) * tam); // Búfer de memoria para la lectura
	fd_origen = open(nombre, O_RDONLY); // PROCESO PADRE: Abre el fichero original en modo lectura
	if(fd_origen < 0){
		printf("Error. No existe el fichero\n");
	}
	else{
		// PROCESO CLAVE 1: Obtención del tamaño del fichero y cálculo de fragmentos
		stat(nombre, &propFich);	
		numTrozos = propFich.st_size/tam;	// el numero de trozos a leer = total del fichero / total del trozo
		if(propFich.st_size % tam != 0){
			numTrozos++; // si el tamaño del fichero no es multiplo del tamaño del trozo, añade un trozo adicional para el residuo final
		}
		// PROCESO CLAVE 2: Bucle de creación de tuberías y bifurcación de procesos
		for(i = 0; i < numTrozos; i++){
			pipe(tubo); // crea una tuberia distinta para la comunicación con cada hijo.
			if(fork() != 0){
				// ==== CÓDIGO DEL PROCESO PADRE ====
				numLeidos = read(fd_origen, lectura, tam); // Lee del fichero original
				write(tubo[1], lectura, numLeidos);	// Escribe los bytes en la tubería

				// Cierre de descriptores de tubería en el padre tras usarlos
				close(tubo[0]);
				close(tubo[1]);
			}
			else{ 
				// ==== CÓDIGO DEL PROCESO PADRE ====
				close(fd_origen);

				if(i < 10){ 
					sprintf(nombreFichero, "%s.h0%d", nombre, i);
				}
				else{
					sprintf(nombreFichero, "%s.h%d", nombre, i);
				}
				fd_destino = creat(nombreFichero, 0666);

				// PROCESO CLAVE 3: Lectura desde la tubería y escritura del fragmento
				numLeidos = read(tubo[0], lectura, tam);				
				write(fd_destino, lectura, numLeidos);	
				
				// Limpieza de recursos en el hijo
				close(tubo[0]);
				close(tubo[1]);
				close(fd_destino);
				exit(0);
			}
		}
		// PROCESO CLAVE 4: Espera y finalización en el proceso padre
		if(i == numTrozos){
			free(lectura);
			for(int i = 0; i < numTrozos; i++){
				wait(NULL);
			}
		}
	}
}

