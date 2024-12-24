#ifndef MEMORIA_USUARIO_H_
#define MEMORIA_USUARIO_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/config.h>
#include <utils/shared/shared.h>
#include <variables_globales.h>
#include <commons/collections/list.h>



void inicializar_memoria_usuario();
void inicializar_particiones_dinamicas(int tam_memoria);
void inicializar_particiones_fijas();
bool asignar_memoria_particiones_fijas_first_fit(int pid, int tamanio_proceso);
bool asignar_memoria_particiones_fijas_best_fit(int pid, int tamanio_proceso);
bool asignar_memoria_particiones_fijas_worst_fit(int pid, int tamanio_proceso);
bool asignar_memoria_particiones_first_fit(int pid, int tamanio_proceso);
bool asignar_memoria_particiones_best_fit(int pid, int tamanio_proceso);
bool asignar_memoria_particiones_worst_fit(int pid, int tamanio_proceso);



typedef struct {
    int base;
    int limite;
    bool libre;
    int pid;
} t_particion;



#endif