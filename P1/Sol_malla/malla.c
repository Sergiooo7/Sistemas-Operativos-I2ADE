#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// Manejador de señal vacío para evitar la acción por defecto del SIGALRM
void nada(int sig) {}

int main(int argc, char *argv[]) {
    // 1. Control básico de parámetros y conversión a enteros
    if (argc < 3) return 1;
    int x = atoi(argv[1]);
    int y = atoi(argv[2]);

    // 2. Bucle principal: crea las 'y' columnas horizontales
    for (int i = 0; i < y; i++) {
        
        // El proceso raíz (malla) genera un hijo por cada columna
        if (fork() == 0) {
            
            // 3. Cadena vertical: el hijo crea los 'x - 1' niveles inferiores
            for (int j = 1; j < x; j++) {
                
                // Si es el proceso padre intermedio, espera a su hijo y luego termina
                if (fork() != 0) {
                    wait(NULL);
                    exit(0);
                }
            }

            // 4. Proceso hoja (el más profundo de la columna):
            // Captura SIGALRM, programa 10 segundos para dar tiempo a 'pstree -c' y finaliza
            signal(SIGALRM, nada);
            alarm(10);
            pause();
            exit(0);
        }
    }

    // 5. El proceso raíz espera a que sus 'y' hijos terminen antes de salir
    while (wait(NULL) > 0);

    return 0;
}
