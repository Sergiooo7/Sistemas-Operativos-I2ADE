#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

// Variables globales para almacenar PIDs y la configuración 
pid_t g_pid_ejec;
pid_t g_pid_a;
pid_t g_pid_b;
pid_t g_pid_x;
pid_t g_pid_y;
pid_t g_pid_z;

int g_seconds;

// Prototipos de funciones 
void parse_args(int argc, char *argv[]);
void handler_start_destruction(int s);

void create_A_process(void);
void run_process_A(void);
void handler_A_exec_task(int s);
void handler_A_destroy_and_propagate(int s);

void create_B_process(void);
void run_process_B(void);
void handler_B_exec_task(int s);
void handler_B_destroy_and_propagate(int s);

void create_X_process(void);
void run_process_X(void);
void handler_X_destroy_leaf(int s);

void create_Y_process(void);
void run_process_Y(void);
void handler_Y_destroy_leaf(int s);

void create_Z_process(void);
void run_process_Z(void);
void handler_Z_alarm(int s);
void handler_Z_destroy_leaf(int s);

// Validación y lectura del único argumento (segundos) 
void parse_args(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <segundos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    g_seconds = atoi(argv[1]);
}

// ==== Proceso super-padre (ejec) ====
int main(int argc, char *argv[]) {
    parse_args(argc, argv);

    g_pid_ejec = getpid();
    printf("Soy el proceso ejec: mi pid es %d\n", g_pid_ejec);

    // Registra el manejador para iniciar la secuencia de apagado 
    signal(SIGUSR2, handler_start_destruction);

    // Crea la rama de procesos y espera a su finalización 
    create_A_process();

    printf("Soy ejec (%d) y muero\n", g_pid_ejec);
    return 0;
}

void handler_start_destruction(int s) {
    (void)s;
    kill(g_pid_a, SIGUSR2);
}

// ==== Proceso A ====
void create_A_process(void) {
    switch (g_pid_a = fork()) {
        case 0:
            run_process_A();
            exit(0);
        default:
            // El padre espera a que el hijo termine antes de salir 
            wait(NULL);
    }
}

void run_process_A(void) {
    g_pid_a = getpid();
    printf("Soy el proceso A: mi pid es %d. Mi padre es %d\n", g_pid_a, g_pid_ejec);

    signal(SIGUSR1, handler_A_exec_task);
    signal(SIGUSR2, handler_A_destroy_and_propagate);

    create_B_process();
}

// Al recibir SIGUSR1, A crea un hijo auxiliar para ejecutar 'pstree' 
void handler_A_exec_task(int s) {
    pid_t pid;
    (void)s;

    pid = fork();
    if (pid == 0) {
        execlp("pstree", "pstree", (char *)NULL);
        exit(EXIT_FAILURE);
    }
    
    // Espera la ejecución del comando y notifica a ejec para iniciar el apagado 
    wait(NULL);
    kill(g_pid_ejec, SIGUSR2);
}

void handler_A_destroy_and_propagate(int s) {
    (void)s;
    kill(g_pid_b, SIGUSR2);
    wait(NULL);

    printf("Soy A (%d) y muero\n", g_pid_a);
    exit(0);
}

// ==== Proceso B ==== 
void create_B_process(void) {
    switch (g_pid_b = fork()) {
        case 0:
            run_process_B();
            exit(0);
        default:
            wait(NULL);
    }
}

void run_process_B(void) {
    g_pid_b = getpid();
    printf("Soy el proceso B: mi pid es %d. Mi padre es %d. Mi abuelo es %d\n",
           g_pid_b, g_pid_a, g_pid_ejec);

    signal(SIGUSR2, handler_B_destroy_and_propagate);

    create_X_process();
    create_Y_process();
    create_Z_process();

    while (1) {
        pause();
    }
}

// Propagación ordenada de la destrucción hacia las hojas Z, Y, X 
void handler_B_destroy_and_propagate(int s) {
    (void)s;

    kill(g_pid_z, SIGUSR2);
    wait(NULL);

    kill(g_pid_y, SIGUSR2);
    wait(NULL);

    kill(g_pid_x, SIGUSR2);
    wait(NULL);

    printf("Soy B (%d) y muero\n", g_pid_b);
    exit(0);
}

// ==== Proceso X ==== 
void create_X_process(void) {
    switch (g_pid_x = fork()) {
        case 0:
            run_process_X();
            exit(0);
    }
}

void run_process_X(void) {
    g_pid_x = getpid();
    printf("Soy el proceso X: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n",
           g_pid_x, g_pid_b, g_pid_a, g_pid_ejec);

    signal(SIGUSR2, handler_X_destroy_leaf);

    while (1) {
        pause();
    }
}

void handler_X_destroy_leaf(int s) {
    (void)s;
    printf("Soy X (%d) y muero\n", g_pid_x);
    exit(0);
}

// ==== Proceso Y ====
void create_Y_process(void) {
    switch (g_pid_y = fork()) {
        case 0:
            run_process_Y();
            exit(0);
    }
}

void run_process_Y(void) {
    g_pid_y = getpid();
    printf("Soy el proceso Y: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n",
           g_pid_y, g_pid_b, g_pid_a, g_pid_ejec);

    signal(SIGUSR2, handler_Y_destroy_leaf);

    while (1) {
        pause();
    }
}

void handler_Y_destroy_leaf(int s) {
    (void)s;
    printf("Soy Y (%d) y muero\n", g_pid_y);
    exit(0);
}

// ==== Proceso Z ==== 
void create_Z_process(void) {
    switch (g_pid_z = fork()) {
        case 0:
            run_process_Z();
            exit(0);
    }
}

void run_process_Z(void) {
    g_pid_z = getpid();
    printf("Soy el proceso Z: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n",
           g_pid_z, g_pid_b, g_pid_a, g_pid_ejec);

    signal(SIGALRM, handler_Z_alarm);
    signal(SIGUSR2, handler_Z_destroy_leaf);

    // Programa la alarma para enviar la señal transcurridos los segundos indicados 
    alarm(g_seconds);

    while (1) {
        pause();
    }
}

// Envía SIGUSR1 directamente al proceso A al expirar el tiempo 
void handler_Z_alarm(int s) {
    (void)s;
    kill(g_pid_a, SIGUSR1);
}

void handler_Z_destroy_leaf(int s) {
    (void)s;
    printf("Soy Z (%d) y muero\n", g_pid_z);
    exit(0);
}
