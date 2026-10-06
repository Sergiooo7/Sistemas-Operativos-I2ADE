#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

// Variables globales para almacenar PIDs y la configuración
pid_t pidEjec;
pid_t pidA;
pid_t pidB;
pid_t pidX;
pid_t pidY;
pid_t pidZ;

int secs;

// Prototipos de funciones
void processArgs(int argc, char *argv[]);
void handler_start_destruction(int s);

void make_processA(void);
void exec_processA(void);
void handler_A_execTask(int s);
void handler_A_destroy_propagate(int s);

void make_processB(void);
void exec_processB(void);
void handler_B_execTask(int s);
void handler_B_destroy_propagate(int s);

void make_processX(void);
void exec_processX(void);
void handler_X_destroyLeaf(int s);

void make_processY(void);
void exec_processY(void);
void handler_Y_destroyLeaf(int s);

void make_processZ(void);
void exec_processZ(void);
void handler_Z_alarm(int s);
void handler_Z_destroyLeaf(int s);

// Validación y lectura del único argumento (segundos)
void processArgs(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <segundos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    secs = atoi(argv[1]); // Conversión de parámetro CLI a entero
}

// ==== Proceso super-padre (ejec) ====
int main(int argc, char *argv[]) {
    processArgs(argc, argv);

    pidEjec = getpid(); // Obtener PID del proceso raíz
    printf("Soy el proceso ejec: mi pid es %d\n", pidEjec);

    // Registra el manejador para iniciar la secuencia de apagado
    signal(SIGUSR2, handler_start_destruction); // Manejador de apagado global

    // Crea la rama de procesos y espera a su finalización
    make_processA(); // Inicio del árbol de procesos

    printf("Soy ejec (%d) y muero\n", pidEjec);
    return 0;
}

void handler_start_destruction(int s) {
    (void)s;
    kill(pidA, SIGUSR2); // Notificar orden de destrucción al subárbol A
}

// ==== Proceso A ====
void make_processA(void) {
    switch (pidA = fork()) { // Creación del proceso A
        case 0:
            exec_processA();
            exit(0);
        default:
            // El padre espera a que el hijo termine antes de salir
            wait(NULL); // Espera activa hasta que el proceso A finalice
    }
}

void exec_processA(void) {
    pidA = getpid();
    printf("Soy el proceso A: mi pid es %d. Mi padre es %d\n", pidA, pidEjec);

    signal(SIGUSR1, handler_A_execTask); // Manejador de orden de tarea pstree
    signal(SIGUSR2, handler_A_destroy_propagate); // Manejador de destrucción

    make_processB();
}

// Al recibir SIGUSR1, A crea un hijo auxiliar para ejecutar 'pstree'
void handler_A_execTask(int s) {
    pid_t pid;
    (void)s;

    pid = fork(); // Subproceso auxiliar para el comando externo
    if (pid == 0) {
        execlp("pstree", "pstree", (char *)NULL); // Reemplazar imagen con pstree
        exit(EXIT_FAILURE);
    }
   
    // Espera la ejecución del comando y notifica a ejec para iniciar el apagado
    wait(NULL); // Bloqueo hasta la finalización de pstree
    kill(pidEjec, SIGUSR2); // Informar al proceso raíz para iniciar el cierre
}

void handler_A_destroy_propagate(int s) {
    (void)s;
    kill(pidB, SIGUSR2); // Enviar señal de apagado al subárbol B
    wait(NULL); // Aguardar la muerte de B

    printf("Soy A (%d) y muero\n", pidA);
    exit(0);
}

// ==== Proceso B ====
void make_processB(void) {
    switch (pidB = fork()) { // Creación del nodo intermedio B
        case 0:
            exec_processB();
            exit(0);
        default:
            wait(NULL);
    }
}

void exec_processB(void) {
    pidB = getpid();
    printf("Soy el proceso B: mi pid es %d. Mi padre es %d. Mi abuelo es %d\n",
           pidB, pidA, pidEjec);

    signal(SIGUSR2, handler_B_destroy_propagate);

    make_processX();
    make_processY();
    make_processZ();

    while (1) {
        pause(); // Suspensión en bucle a la espera de señales
    }
}

// Propagación ordenada de la destrucción hacia las hojas Z, Y, X
void handler_B_destroy_propagate(int s) {
    (void)s;

    kill(pidZ, SIGUSR2); // Apagado de la hoja Z
    wait(NULL);

    kill(pidY, SIGUSR2); // Apagado de la hoja Y
    wait(NULL);

    kill(pidX, SIGUSR2); // Apagado de la hoja X
    wait(NULL);

    printf("Soy B (%d) y muero\n", pidB);
    exit(0);
}

// ==== Proceso X ====
void make_processX(void) {
    switch (pidX = fork()) { // Creación del proceso hoja X
        case 0:
            exec_processX();
            exit(0);
    }
}

void exec_processX(void) {
    pidX = getpid();
    printf("Soy el proceso X: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n",
           pidX, pidB, pidA, pidEjec);

    signal(SIGUSR2, handler_X_destroyLeaf);

    while (1) {
        pause();
    }
}

void handler_X_destroyLeaf(int s) {
    (void)s;
    printf("Soy X (%d) y muero\n", pidX);
    exit(0);
}

// ==== Proceso Y ====
void make_processY(void) {
    switch (pidY = fork()) { // Creación del proceso hoja Y
        case 0:
            exec_processY();
            exit(0);
    }
}

void exec_processY(void) {
    pidY = getpid();
    printf("Soy el proceso Y: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n",
           pidY, pidB, pidA, pidEjec);

    signal(SIGUSR2, handler_Y_destroyLeaf);

    while (1) {
        pause();
    }
}

void handler_Y_destroyLeaf(int s) {
    (void)s;
    printf("Soy Y (%d) y muero\n", pidY);
    exit(0);
}

// ==== Proceso Z ====
void make_processZ(void) {
    switch (pidZ = fork()) { // Creación del proceso hoja Z
        case 0:
            exec_processZ();
            exit(0);
    }
}

void exec_processZ(void) {
    pidZ = getpid();
    printf("Soy el proceso Z: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n",
           pidZ, pidB, pidA, pidEjec);

    signal(SIGALRM, handler_Z_alarm);
    signal(SIGUSR2, handler_Z_destroyLeaf);

    // Programa la alarma para enviar la señal transcurridos los segundos indicados
    alarm(secs); // Temporizador en segundos antes de disparar SIGALRM

    while (1) {
        pause();
    }
}

// Envía SIGUSR1 directamente al proceso A al expirar el tiempo
void handler_Z_alarm(int s) {
    (void)s;
    kill(pidA, SIGUSR1); // Alerta por señal a A tras expiración del temporizador
}

void handler_Z_destroyLeaf(int s) {
    (void)s;
    printf("Soy Z (%d) y muero\n", pidZ);
    exit(0);
}
