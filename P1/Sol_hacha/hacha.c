#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

// Variables globales para la configuración del troceado
char *nombreOrigen;
int tamTrozo;
int numTrozos;

// Prototipos de funciones
void processArgs(int argc, char *argv[]);
int calcularTrozos(const char *nombre, int tam);
void divFich(void);

void ejecPadre(int fd_origen, int tubo[2], char *buffer);
void ejecHijo(const char *nombre, int indice, int tubo[2], char *buffer);
void genFrag(char *outNombre, const char *base, int indice);

// Validación y lectura de argumentos (fichero y tamaño de trozo)
void processArgs(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error. Uso: %s fichero tam_trozo\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    nombreOrigen = argv[1];
    tamTrozo = atoi(argv[2]);
}

// Obtiene el tamaño del fichero original y calcula el número total de trozos
int calcularTrozos(const char *nombre, int tam) {
    struct stat propFich;
    if (stat(nombre, &propFich) < 0) {
        perror("Error. No existe el fichero");
        exit(EXIT_FAILURE);
    }

    int trozos = propFich.st_size / tam;
    if (propFich.st_size % tam != 0) {
        trozos++;
    }
    return trozos;
}

// ==== Proceso Principal (Coordinador) ====
int main(int argc, char *argv[]) {
    processArgs(argc, argv);

    numTrozos = calcularTrozos(nombreOrigen, tamTrozo);

    divFich();

    return 0;
}

// ==== Bucle de División y Gestión de Procesos ====
void divFich(void) {
    int fd_origen = open(nombreOrigen, O_RDONLY);
    if (fd_origen < 0) {
        perror("Error al abrir el fichero de origen");
        exit(EXIT_FAILURE);
    }

    char *buffer = (char *)malloc(sizeof(char) * tamTrozo);
    if (buffer == NULL) {
        perror("Error de asignación de memoria");
        close(fd_origen);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numTrozos; i++) {
        int tubo[2];
        if (pipe(tubo) < 0) {
            perror("Error al crear la tubería");
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Error al crear proceso hijo");
            break;
        }

        if (pid != 0) {
            // Lógica del Proceso Padre
            ejecPadre(fd_origen, tubo, buffer);
        } else {
            // Lógica del Proceso Hijo (el hijo se encarga de cerrar fd_origen y salir)
            close(fd_origen);
            ejecHijo(nombreOrigen, i, tubo, buffer);
        }
    }

    close(fd_origen);
    free(buffer);

    // Espera a que finalicen todos los procesos hijos
    for (int i = 0; i < numTrozos; i++) {
        wait(NULL);
    }
}

// ==== Proceso Padre: Lectura de origen y envío por la tubería ====
void ejecPadre(int fd_origen, int tubo[2], char *buffer) {
    int numLeidos = read(fd_origen, buffer, tamTrozo);
    if (numLeidos > 0) {
        write(tubo[1], buffer, numLeidos);
    }
    close(tubo[0]);
    close(tubo[1]);
}

// ==== Proceso Hijo: Recepción por tubería y escritura del fragmento ====
void ejecHijo(const char *nombre, int indice, int tubo[2], char *buffer) {
    char nombreFichero[50];
    genFrag(nombreFichero, nombre, indice);

    int fd_destino = creat(nombreFichero, 0666);
    if (fd_destino < 0) {
        perror("Error al crear fragmento destino");
        close(tubo[0]);
        close(tubo[1]);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    int numLeidos = read(tubo[0], buffer, tamTrozo);
    if (numLeidos > 0) {
        write(fd_destino, buffer, numLeidos);
    }

    close(tubo[0]);
    close(tubo[1]);
    close(fd_destino);
    free(buffer);
    exit(EXIT_SUCCESS);
}

// Formatea el nombre de salida del fragmento (.h00, .h01, ... o .h10)
void genFrag(char *outNombre, const char *base, int indice) {
    if (indice < 10) {
        sprintf(outNombre, "%s.h0%d", base, indice);
    } else {
        sprintf(outNombre, "%s.h%d", base, indice);
    }
}
