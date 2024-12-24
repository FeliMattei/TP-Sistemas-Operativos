#include <cpu_global.h>

char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* PUERTO_ESCUCHA_INTERRUPT;
char* PUERTO_ESCUCHA_DISPATCH;
char* LOG_LEVEL;

t_log* cpu_logger;

int conexion_memoria;
int socket_dispatch;
int socket_interrupt;
int fd_kernel_dispatch;
int fd_kernel_interrupt;

pthread_t hilo_kernel_dispatch;
pthread_t hilo_kernel_interrupt;

t_contexto_completo* ctx_global_tid;

int hay_interrupcion;
int ya_desalojado;
int fin_quantum;
int init;