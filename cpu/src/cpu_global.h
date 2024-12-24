#ifndef CPU_GLOBAL_H_
#define CPU_GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/shared/shared.h>
#include <utils/inicializadores.h>

extern char* IP_MEMORIA;
extern char* PUERTO_MEMORIA;
extern char* PUERTO_ESCUCHA_INTERRUPT;
extern char* PUERTO_ESCUCHA_DISPATCH;
extern char* LOG_LEVEL;

extern t_log* cpu_logger;

extern int conexion_memoria;
extern int socket_dispatch;
extern int socket_interrupt;
extern int fd_kernel_dispatch;
extern int fd_kernel_interrupt;

extern pthread_t hilo_kernel_dispatch;
extern pthread_t hilo_kernel_interrupt;

extern t_contexto_completo* ctx_global_tid;

extern int hay_interrupcion;
extern int ya_desalojado;
extern int fin_quantum;
extern int init;

#endif