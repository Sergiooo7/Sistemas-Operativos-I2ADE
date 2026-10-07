#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Variables globales para almacenar PIDs, IPC y la configuración
int x, y;
int idMemoriaX, idMemoriaY;
int *pidsX, *pidsY;
pid_t pidSuperPadre;

// Prototipos de funciones
void processArgs(int argc, char *argv[]);
void confMem(void);
void cleanMem(void);
void controlAlarma(int s);

void cadena_vertical(void);
void printInfo(int nivel);

void hoja_horizontal(void);
void ejec_horizontal(int indice);

void printSuperPadre(void);

// Validación y lectura de argumentos (x e y)
void processArgs(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <x> <y>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    x = atoi(argv[1]);
    y = atoi(argv[2]);
}

// Inicialización e integración de la memoria compartida IPC
void confMem(void) {
    idMemoriaX = shmget(IPC_PRIVATE, sizeof(int) * x, IPC_CREAT | 0666);
    if (idMemoriaX < 0) {
        perror("Error en shmget X");
        exit(EXIT_FAILURE);
    }
    pidsX = (int *)shmat(idMemoriaX, 0, 0);

    idMemoriaY = shmget(IPC_PRIVATE, sizeof(int) * y, IPC_CREAT | 0666);
    if (idMemoriaY < 0) {
        perror("Error en shmget Y");
        exit(EXIT_FAILURE);
    }
    pidsY = (int *)shmat(idMemoriaY, 0, 0);
}

// Liberación y eliminación de los segmentos IPC en el Super Padre
void cleanMem(void) {
    shmdt(pidsX);
    shmdt(pidsY);
    shmctl(idMemoriaX, IPC_RMID, NULL);
    shmctl(idMemoriaY, IPC_RMID, NULL);
}

// Manejador de señal vacío para desbloquear el pause() con SIGALRM
void controlAlarma(int s) {
    (void)s;
}

// ==== Proceso Super Padre ====
int main(int argc, char *argv[]) {
    processArgs(argc, argv);

    pidSuperPadre = getpid();
    confMem();

    // Genera la jerarquía vertical de procesos
    cadena_vertical();

    // El super padre imprime el resultado final tras la finalización de la descendencia
    printSuperPadre();

    // Liberación completa de recursos IPC
    cleanMem();

    return 0;
}

// ==== Cadena Vertical de Procesos (X niveles) ====
void cadena_vertical(void) {
    int i;
    pid_t pid;

    for (i = 1; i <= x; i++) {
        pid = fork();
        if (pid != 0) {
            // El padre de este nivel espera a su hijo directo y sale del bucle
            wait(NULL);
            break;
        } else {
            // Código ejecutado por el hijo vertical de nivel i
            pidsX[i - 1] = getpid();
            printInfo(i);
        }
    }

    if (i != 1) {
        // El último nivel vertical (i == x + 1) crea la rama de procesos Y
        if (i == x + 1) {
            hoja_horizontal();
        }
        // Desvinculación de la memoria compartida en descendientes intermedios
        shmdt(pidsX);
        shmdt(pidsY);
        exit(0);
    }
}

void printInfo(int nivel) {
    printf("Soy el proceso %d. Mis padres son: %d", getpid(), pidSuperPadre);
    for (int j = 0; j < nivel - 1; j++) {
        printf(", %d", pidsX[j]);
    }
    printf("\n");
}

// ==== Procesos Hojas Horizontales (Y procesos) ====
void hoja_horizontal(void) {
    int i;
    pid_t pid;

    for (i = 1; i <= y; i++) {
        pid = fork();
        if (pid == 0) {
            ejec_horizontal(i - 1);
        }
    }

    // El proceso de nivel X espera a que terminen sus Y hijos horizontales
    for (i = 1; i <= y; i++) {
        wait(NULL);
    }
}

void ejec_horizontal(int indice) {
    pidsY[indice] = getpid();

    signal(SIGALRM, controlAlarma);
    alarm(10);
    pause();

    // Desvinculación de memoria compartida al finalizar la hoja
    shmdt(pidsX);
    shmdt(pidsY);
    exit(0);
}

// Impresión del resumen con los PIDs de los hijos horizontales finales
void printSuperPadre(void) {
    printf("Soy el super padre %d, mis hijos finales son: ", pidSuperPadre);
    for (int i = 0; i < y; i++) {
        printf("%d", pidsY[i]);
        if (i != y - 1) {
            printf(", ");
        }
    }
    printf("\n");
}
