#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// Manejador de señal para SIGALRM
void nada(int sig) {}

// Función que crea la cadena vertical de 'x' niveles
void cadena_vertical(int x) {
    // Cadena vertical: el hijo crea los 'x - 1' niveles inferiores
    for (int j = 1; j < x; j++) {
        if (fork() != 0) {
            wait(NULL);
            exit(0);
        }
    }
    signal(SIGALRM, nada);
    alarm(10);
    pause();
    exit(0);
}

// Función que crea las 'y' columnas horizontales
void malla_horizontal(int x, int y) {
    // Bucle principal: crea las 'y' columnas horizontales
    for (int i = 0; i < y; i++) {
        if (fork() == 0) {
            cadena_vertical(x);
        }
    }
    while (wait(NULL) > 0);
}

int main(int argc, char *argv[]) {
    
    if (argc < 3) return 1;
    int x = atoi(argv[1]);
    int y = atoi(argv[2]);

    malla_horizontal(x, y);

    return 0;
}
