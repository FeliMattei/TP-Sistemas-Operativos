#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <memoria_sistema/memoria_sistema.h>
#include <memoria_usuario/memoria_usuario.h>
#include <inicializar_memoria/inicializar_memoria.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <semaphore.h>




t_log* memoria_log;
t_memoria_config* memoria_config;

int conexion_filesystem;


void* manejar_peticiones_kernel(void *memoria_fs_ptr);
void* atender_kernel(void *fd_kernel_ptr);

void* atender_cpu(void* fd_cpu);
bool asignar_memoria_particiones_dinamicas(int pid, int tamanio_proceso);
bool asignar_memoria_particiones_fijas(int pid, int tamanio_proceso);
void finalizar_proceso(int pid);
void consolidar_particiones(int index);
//void almacenar_contexto_hilo(int tid);
void finalizar_hilo(int pid, int tid);
void recibir_datos_proceso(int fd_kernel, int* pid, int* tamanio_proceso);
t_particion* obtener_particion(int pid);
void enviar_confirmacion(int fd_kernel);
void recibir_datos_finalizar_proceso(int fd_kernel, int* pid);
void recibir_datos_inicializar_hilo(int fd_kernel, int* pid, int* tid, char** filePath);
void recibir_datos_finalizar_hilo(int fd_kernel, int* pid, int* tid);
t_list* procesar_archivo(char* filePath);
void enviar_denied(int fd_kernel);
void recibir_datos_dump_memory(int conexion,int* pid,int* tid);
char* armar_nombre(int pid, int tid);

t_dictionary* contextos_procesos;
t_dictionary* contextos_hilos;
t_dictionary* instrucciones;
void* memoria_usuario;
t_list* particiones; 

sem_t sem_mutex_particiones;
sem_t sem_mutex_contextos_hilos;
sem_t sem_mutex_contextos_procesos;
sem_t sem_mutex_instrucciones;
sem_t fin_memoria;
sem_t sem_mutex_memoria_usuario;


#endif