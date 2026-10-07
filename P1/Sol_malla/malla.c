#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

/* Variables globales para las dimensiones de la malla */
int x = 0;
int y = 0;

/* Prototipos de funciones */
void process_args(int argc, char *argv[]);
void manejador_alarma(int sig);
void crear_malla(void);
void crear_columna(void);
void ejecutar_hoja(void);

/* Validación y obtención de parámetros */
void process_args(int argc, char *argv[]) {
    if (argc < 3) {
        exit(EXIT_FAILURE);
    }
    x = atoi(argv[1]);
    y = atoi(argv[2]);
}

/* Manejador de señal vacío para SIGALRM */
void manejador_alarma(int sig) {

}

/* Lógica del proceso hoja (nodo inferior) */
void ejecutar_hoja(void) {
    signal(SIGALRM, manejador_alarma);
    alarm(10);
    pause();
    exit(EXIT_SUCCESS);
}

/* Creación de la cadena vertical (niveles X) */
void crear_columna(void) {
    for (int j = 1; j < x; j++) {
        if (fork() != 0) {
            wait(NULL);
            exit(EXIT_SUCCESS);
        }
    }
    ejecutar_hoja();
}

/* Creación de las columnas horizontales (Y) */
void crear_malla(void) {
    for (int i = 0; i < y; i++) {
        if (fork() == 0) {
            crear_columna();
        }
    }
    while (wait(NULL) > 0);
}

int main(int argc, char *argv[]) {
    process_args(argc, argv);
    crear_malla();
    return 0;
}
