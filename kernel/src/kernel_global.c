#include <kernel_global.h>

t_log* kernel_logger;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* IP_CPU;
char* PUERTO_CPU_DISPATCH;
char* PUERTO_CPU_INTERRUPT;
algoritmo ALGORITMO_PLANIFICACION;
int QUANTUM;
//char* LOG_LEVEL; Por el momento en desuso

int conexion_cpu_dispatch;
int conexion_cpu_interrupt;

t_list* COLA_NEW;
t_list* COLA_READY;
t_list* COLA_EXEC;
t_list* COLA_BLOCKED;
t_list* COLA_EXIT;

t_list* LISTA_PCBS;
t_list* LISTA_TCBS;

t_list* CREACIONES;

int IDENTIFICADOR_PID;
pthread_mutex_t MUTEX_PID;

pthread_t hilo_quantum;

sem_t sem_new;
sem_t sem_ready;
sem_t sem_exec;
sem_t sem_exit;
sem_t sem_blocked;
sem_t hay_pcb_cola_new;
sem_t hay_tcb_cola_ready;
sem_t mutex_multiprogramacion;
sem_t multiprogramacion_restante;
sem_t sem_lista_pcbs;
sem_t sem_lista_tcbs;
sem_t proceso_en_chequeo_memoria;
sem_t puede_entrar_a_exec;
sem_t agregar_hilo;
sem_t fin_kernel;
sem_t hay_pcb_encolado_new;
sem_t sem_creacion;