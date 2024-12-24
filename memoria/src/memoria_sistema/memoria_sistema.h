#ifndef MEMORIA_SISTEMA_H_
#define MEMORIA_SISTEMA_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/config.h>
#include <utils/shared/shared.h>
#include <variables_globales.h>

typedef struct {
    int pid;
    int base;
    int limite;
} t_contexto_proceso;

typedef struct {
    int tid;
    t_registros* registros;
} t_contexto_hilo;

void inicializar_contextos();
void almacenar_contexto_proceso(int pid, int base, int limite);
void almacenar_contexto_hilo(int pid, int tid);
void inicializar_memoria_sistema();
t_contexto_proceso* obtener_contexto_proceso(int pid);
t_contexto_hilo* obtener_contexto_hilo(int pid, int tid);
t_contexto_completo* obtener_contexto_completo (int pid, int tid);
void actualizar_contexto_hilo(int pid, int tid, t_registros* registros);
void almacenar_instrucciones (int pid, int tid, t_list* instrucciones);
t_instruccion* obtener_instruccion (int pid, int tid, int pg);


#endif