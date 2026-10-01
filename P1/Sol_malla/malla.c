#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;

    int x = atoi(argv[1]);
    int y = atoi(argv[2]);

    // 1. El proceso raíz 'malla' crea 'y' columnas
    for (int i = 0; i < y; i++) {
        if (fork() == 0) { 
            
            // 2. Se crean (x - 1) niveles más (el proceso actual ya es el nivel 1)
            for (int j = 0; j < x - 1; j++) {
                if (fork() > 0) break; // El padre se detiene en su nivel
            }
            
            pause(); // Mantiene vivo a cada proceso de la malla
            exit(0);
        }
    }

    pause(); // Mantiene vivo al proceso raíz 'malla'
    return 0;
}
