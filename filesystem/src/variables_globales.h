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
    char* mount_dir;
    int block_size;
    int block_count;
    int retardo_acceso_bloque;
    char* log_level;
} t_filesystem_config;

extern t_log* filesystem_log;
extern t_filesystem_config* filesystem_config;

extern sem_t mutex_bitmap;
extern sem_t mutex_bloques;

#endif