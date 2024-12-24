#ifndef KERNEL_GLOBAL_H
#define KERNEL_GLOBAL_H

#include <utils/inicializadores.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <semaphore.h>
#include <pthread.h>
#include <readline/readline.h>
#include <utils/shared/shared.h>
#include <stdbool.h>

extern t_log* kernel_logger;
extern char* IP_MEMORIA;
extern char* PUERTO_MEMORIA;
extern char* IP_CPU;
extern char* PUERTO_CPU_DISPATCH;
extern char* PUERTO_CPU_INTERRUPT;
extern algoritmo ALGORITMO_PLANIFICACION;
extern int QUANTUM;
//char* LOG_LEVEL; Por el momento en desuso

extern int conexion_cpu_dispatch;
extern int conexion_cpu_interrupt;

extern t_list* COLA_NEW;
extern t_list* COLA_READY;
extern t_list* COLA_EXEC;
extern t_list* COLA_BLOCKED;
extern t_list* COLA_EXIT;

extern t_list* LISTA_PCBS;
extern t_list* LISTA_TCBS;

extern t_list* CREACIONES;

extern int IDENTIFICADOR_PID;
extern pthread_mutex_t MUTEX_PID;

extern pthread_t hilo_quantum;

extern sem_t sem_new;
extern sem_t sem_ready;
extern sem_t sem_exec;
extern sem_t sem_exit;
extern sem_t sem_blocked;
extern sem_t hay_pcb_cola_new;
extern sem_t hay_tcb_cola_ready;
extern sem_t mutex_multiprogramacion;
extern sem_t multiprogramacion_restante;
extern sem_t sem_lista_pcbs;
extern sem_t sem_lista_tcbs;
extern sem_t proceso_en_chequeo_memoria;
extern sem_t puede_entrar_a_exec;
extern sem_t agregar_hilo;
extern sem_t fin_kernel;
extern sem_t hay_pcb_encolado_new;
extern sem_t sem_creacion;


#endif