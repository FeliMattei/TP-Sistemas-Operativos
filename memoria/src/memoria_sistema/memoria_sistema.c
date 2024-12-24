#include "memoria_sistema.h"

void inicializar_contextos() {
    contextos_procesos = dictionary_create();
    contextos_hilos = dictionary_create();
    instrucciones = dictionary_create();
}

void almacenar_contexto_proceso(int pid, int base, int limite) {
    t_contexto_proceso* contexto = malloc(sizeof(t_contexto_proceso));
    contexto->pid = pid;
    contexto->base = base;
    contexto->limite = limite;
    char key_str[12];

    sprintf(key_str, "%d", pid);
    sem_wait(&sem_mutex_contextos_procesos);
        dictionary_put(contextos_procesos, key_str, contexto);
    sem_post(&sem_mutex_contextos_procesos);
}

void almacenar_contexto_hilo(int pid, int tid) {
    t_contexto_hilo* contexto = malloc(sizeof(t_contexto_hilo));
    memset(contexto, 0, sizeof(t_contexto_hilo));  

    contexto->registros = malloc(sizeof(t_registros));
    memset(contexto->registros, 0, sizeof(t_registros));

    char key[25];

    sprintf(key, "%d,%d", pid, tid);

    sem_wait(&sem_mutex_contextos_hilos);
        dictionary_put(contextos_hilos, key, contexto);
    sem_post(&sem_mutex_contextos_hilos);
    //free(key_str);
}

void inicializar_memoria_sistema(){
    inicializar_contextos();
}

t_contexto_proceso* obtener_contexto_proceso(int pid){
    char key_str[12];
    sprintf(key_str, "%d", pid);

    sem_wait(&sem_mutex_contextos_procesos);
        t_contexto_proceso* contexto = dictionary_get(contextos_procesos, key_str);
    sem_post(&sem_mutex_contextos_procesos);

    return contexto;

}

t_contexto_hilo* obtener_contexto_hilo(int pid, int tid){

    char key[25];
    sprintf(key, "%d,%d", pid, tid);

    sem_wait(&sem_mutex_contextos_hilos);
        t_contexto_hilo* contexto = dictionary_get(contextos_hilos, key);
    sem_post(&sem_mutex_contextos_hilos);

    return contexto;

}

t_contexto_completo* obtener_contexto_completo (int pid, int tid){

    t_contexto_proceso* contexto_proceso = obtener_contexto_proceso(pid);
    t_contexto_hilo* contexto_hilo = obtener_contexto_hilo(pid, tid);

    if (contexto_proceso == NULL || contexto_hilo == NULL) {
        return NULL;
    }

    t_contexto_completo* contexto_completo = malloc(sizeof(t_contexto_completo));
    
    contexto_completo->base = contexto_proceso->base;
    contexto_completo->limite = contexto_proceso->limite;

    if (contexto_completo->registros == NULL) {
        contexto_completo->registros = malloc(sizeof(t_registros));
    }

    if (contexto_hilo->registros == NULL) {
        contexto_hilo->registros = malloc(sizeof(t_registros));
    }

    contexto_completo->registros->AX = contexto_hilo->registros->AX;
    contexto_completo->registros->BX = contexto_hilo->registros->BX;
    contexto_completo->registros->CX = contexto_hilo->registros->CX;
    contexto_completo->registros->DX = contexto_hilo->registros->DX;
    contexto_completo->registros->EX = contexto_hilo->registros->EX;
    contexto_completo->registros->FX = contexto_hilo->registros->FX;
    contexto_completo->registros->GX = contexto_hilo->registros->GX;
    contexto_completo->registros->HX = contexto_hilo->registros->HX;
    contexto_completo->registros->PC = contexto_hilo->registros->PC;

    
    return contexto_completo;

}

void actualizar_contexto_hilo (int pid, int tid, t_registros* registros){

    t_contexto_hilo* contexto_hilo = obtener_contexto_hilo(pid, tid);
    
    if (contexto_hilo == NULL) {
        log_error(memoria_log, "Error: No se encontró el contexto para el TID %d\n", tid);
        return;
    }

    memcpy(contexto_hilo->registros, registros, sizeof(t_registros));

    char key[25];
    sprintf(key, "%d,%d", pid, tid);

    sem_wait(&sem_mutex_contextos_hilos);
        dictionary_put(contextos_hilos, key, contexto_hilo);
    sem_post(&sem_mutex_contextos_hilos);

}

void almacenar_instrucciones(int pid, int tid, t_list* instrucciones_a_guardar) {
    char key[25];
    sprintf(key, "%d,%d", pid, tid);

    sem_wait(&sem_mutex_instrucciones);
        dictionary_put(instrucciones, key, instrucciones_a_guardar);
    sem_post(&sem_mutex_instrucciones);
}

t_instruccion* obtener_instruccion(int pid, int tid, int pg) {
    char key[25];
    sprintf(key, "%d,%d", pid, tid);

    sem_wait(&sem_mutex_instrucciones);
    t_list* instrucciones_hilo = dictionary_get(instrucciones, key);
    sem_post(&sem_mutex_instrucciones);
    
    if (instrucciones_hilo == NULL) {
        log_error(memoria_log, "No se encontraron instrucciones para el hilo %d del proceso %d\n", tid, pid);
        return NULL;
    }

    if (pg >= list_size(instrucciones_hilo)) {
        log_error(memoria_log, "Indice fuera de rango para el hilo %d del proceso %d\n", tid, pid);
        return NULL;
    }

    t_instruccion* instruccion_a_obtener = list_get(instrucciones_hilo, pg);

    return instruccion_a_obtener;
}

