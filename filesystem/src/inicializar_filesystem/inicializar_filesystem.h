#ifndef INICIALIZAR_FILESYSTEM_H_
#define INICIALIZAR_FILESYSTEM_H_

#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include <math.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/bitarray.h>
#include <utils/shared/shared.h>
#include <variables_globales.h>

void inicializar_filesystem();
void crear_logger();
void iniciar_config();

extern int block_count;
extern int block_size;
extern char* path_bitmap;
extern char* path_bloques;
extern char* path_metadata;
extern t_bitarray* bitmap;
extern void* bitmap_memoria;
extern t_list* punteros;

#endif