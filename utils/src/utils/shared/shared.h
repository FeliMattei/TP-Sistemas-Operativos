#ifndef SHARED_H_
#define SHARED_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include<unistd.h>
#include<assert.h>
#include <stdio.h>
#include <limits.h>

typedef enum{
    PAQUETE,
    OK,
    DENIED,
    ERROR,
    INTERRUPCION,
    PROCESO_CREATE,
    PROCESO_EXIT,
    INICIALIZAR_PROCESO,
    FINALIZAR_PROCESO,
    INICIALIZAR_HILO,
    FINALIZAR_HILO,
    CONTEXTO_TID_PID,
    SOLICITAR_INSTRUCCION,
    SOLICITAR_INSTRUCCION_OK,
    OBTENER_CONTEXTO,
    OBTENER_CONTEXTO_OK,
    ACTUALIZAR_CONTEXTO,
    ACTUALIZAR_CONTEXTO_OK,
    HILO_DESALOJADO,
    PROCESS_EXIT_SUCCESS,
    THREAD_EXIT_SUCCESS,
    SEGMENTATION_FAULT,
    WRITE_MEM_OK,
    FIN_QUANTUM,
    MUTEX_INEXISTENTE,
    MUTEX_ASIGNADO,
    MUTEX_BLOQUEADO,
    MUTEX_SIN_ASIGNACION,
    NO_OK,
    DUMPY_MEMORY
}op_code;

typedef struct{
    int size;
    void* stream;
}t_buffer;

typedef struct{
    op_code cod_operacion;
    t_buffer* buffer;
}t_paquete;
typedef struct {
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
    uint32_t EX;
    uint32_t FX;
    uint32_t GX;
    uint32_t HX;
    uint32_t PC;
} t_registros;


typedef struct {
    int base;
    int limite;
    t_registros* registros;
} t_contexto_completo;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCK,
	EXIT
} estado;

typedef struct{
	int pid;	
	t_list* hilos;
    pthread_mutex_t sem_hilos;
    t_list* mutex;
    pthread_mutex_t sem_mutex;
} t_pcb;

typedef struct{
    int pid;
	int tid;	
    estado estado;
	int prioridad;
    int blocked_by;
} t_tcb;

typedef struct {
    char* id;
    int tid;
    t_list* lista_bloqueados;
    pthread_mutex_t sem_lista_bloqueados;
} t_mutex;

typedef struct {
    char* path;
    int tamanio_proceso;
    int prioridad;
    int pid;
} t_config_mem_buffer;

typedef struct {
    t_pcb* pcb;
    t_config_mem_buffer* data_buffer;
} thread_cheq_mem;

typedef struct {
    t_tcb* tcb;
    char* tiempo;
} thread_io;

typedef enum{
    SET,
    READ_MEM,
    WRITE_MEM,
    SUM,
    SUB,
    JNZ,
    LOG,
    DUMP_MEMORY,
    IO,
    THREAD_CREATE,
    THREAD_JOIN,
    THREAD_CANCEL,
    MUTEX_CREATE,
    MUTEX_LOCK,
    MUTEX_UNLOCK,
    THREAD_EXIT,
    PROCESS_CREATE,
    PROCESS_EXIT
} instruccion;

typedef enum{
    DINAMICAS,
    FIJAS,
    BEST,
    FIRST,
    WORST
} algoritmos_memoria;

typedef struct {
	instruccion instruccion;	
} t_instruccion_a_enviar;

typedef struct {
	instruccion instruccion;
	t_list* parametros;
    int tamanio_lista;
} t_instruccion;

typedef enum {
    FIFO,
    PRIORIDADES,
    CMN
} algoritmo;

algoritmo convertir_string_a_algoritmo(char* algoritmo);
int iniciar_servidor(char* puerto, t_log* logger);
int esperar_conexion(int socket_servidor);
int crear_conexion(char *ip, char* puerto);
int recibir_operacion(int socket_cliente);
char* copiar_a_memoria(char* palabra);
t_paquete* crear_paquete(op_code cod_op, t_buffer* buffer);
void enviar_paquete(t_paquete* paquete,int socket_cliente);
void* serializar_paquete(t_paquete* paquete, int bytes);
void destruir_paquete(t_paquete* paquete);
t_buffer* crear_buffer();
void destruir_buffer(t_buffer* buffer);
void* extraer_de_buffer(t_buffer* buffer);
void cargar_a_buffer(t_buffer* buffer, void* valor, int tamanio);
t_buffer* recibir_buffer(int conexion);
int extraer_int_del_buffer(t_buffer* buffer);
void cargar_int_a_buffer(t_buffer* buffer, int valor);
void cargar_uint32_a_buffer(t_buffer* buffer, uint32_t valor);
uint32_t extraer_uint32_del_buffer(t_buffer* buffer);
void cargar_contexto_a_buffer(t_buffer* buffer, t_contexto_completo* contexto);
t_contexto_completo* extraer_contexto_del_buffer(t_buffer* buffer);
void cargar_registros_a_buffer(t_buffer* buffer, t_registros* registros);
t_registros* extraer_registros_del_buffer(t_buffer* buffer);
char* extraer_string_del_buffer(t_buffer* buffer);
void cargar_string_a_buffer(t_buffer* buffer, char* valor);
char* estado_a_string(estado estado);
void cargar_pcb_a_buffer(t_buffer* buffer, t_pcb* pcb);
void cargar_tcb_a_buffer(t_buffer* buffer, t_tcb* tcb);
t_instruccion_a_enviar extraer_instruccion_a_enviar_del_buffer(t_buffer* buffer);
void cargar_instruccion_a_enviar_a_buffer(t_buffer* buffer, t_instruccion_a_enviar instruccion);
void cargar_instruccion_a_buffer(t_buffer* buffer, t_instruccion* instruccion);
char* map_instruccion_a_string(instruccion instr);
int map_instruccion_a_enum(char* instruccion);
t_instruccion extraer_instruccion_del_buffer(t_buffer* buffer);
t_tcb* extraer_tcb_del_buffer(t_buffer* buffer);

#endif
