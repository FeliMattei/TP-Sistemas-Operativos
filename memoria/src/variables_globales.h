#ifndef VARIABLES_GLOBALES_H_
#define VARIABLES_GLOBALES_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/config.h>
#include <utils/shared/shared.h>
#include <semaphore.h>



typedef struct {
    char* puerto_escucha;
    char* ip_filesystem;
    char* puerto_filesystem;
    int tam_memoria;
    char* path_instrucciones;
    int retardo_respuesta;
    char* esquema;
    char* algoritmo_busqueda;
    t_list* particiones; //ARRAY
    char* log_level;
} t_memoria_config;


extern t_log* memoria_log;
extern t_memoria_config* memoria_config;

extern int conexion_filesystem;

extern t_dictionary* contextos_procesos;
extern t_dictionary* contextos_hilos;
extern void* memoria_usuario;
extern t_list* particiones; 
extern t_dictionary* instrucciones;
extern algoritmos_memoria algoritmo_memoria;
extern algoritmos_memoria criterio_memoria;

extern sem_t sem_mutex_particiones;
extern sem_t sem_mutex_contextos_procesos;
extern sem_t sem_mutex_contextos_hilos;
extern sem_t sem_mutex_instrucciones;
extern sem_t fin_memoria;
extern sem_t sem_mutex_memoria_usuario;


#endif