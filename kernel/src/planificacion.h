#ifndef PLANIFICACION_H
#define PLANIFICACION_H

#include <kernel_global.h>
#include <utils/shared/shared.h>

void planificar_largo_plazo();
void planificar_corto_plazo();
t_pcb* crear_pcb(int pid);
void inicializar_hilo();
void ejecutar_hilo();
void crear_tcb(int prioridad, t_pcb* pcb);
t_pcb* sacar_siguiente_de_new();
t_tcb* sacar_siguiente_de_ready();
int asignar_pid_proceso();
void agregar_a_new(t_pcb* nuevo_pcb, t_config_mem_buffer* data_buffer);
void agregar_a_exec(t_tcb* tcb);
void agregar_a_blocked(t_tcb* tcb, op_code operacion);
void remover_de_exec(t_pcb* pcb, op_code operacion);
void* chequear_memoria(void* args);
int mensaje_a_consola(char *mensaje);
void iniciar_proceso(t_config_mem_buffer* data_buffer);
int asignar_tid_proceso(t_pcb* pcb);
int asignar_pid_proceso();
void cambiar_estado_tcb(t_tcb* tcb, estado estado_nuevo);
void mandar_contexto_a_cpu(t_tcb* tcb);
void atender_cpu_dispatch(void* socket_cliente_ptr);
t_pcb* buscar_pcb_por_pid(int pid);
void manejar_fin_proceso(t_pcb* pcb);
void iniciar_hilo(char* path, int prioridad, int pid);
t_tcb* buscar_tcb_por_pid(int tid, int pid);
void manejar_fin_hilo(t_tcb* tcb, int es_otro_tid);
t_tcb* buscar_hilo_bloqueado_por(t_tcb* tcb);
void esperar_rr(t_tcb* tcb);
bool existe_mutex(char* id, int pid);
bool esta_tomado_mutex(char* id, int pid);
t_mutex* buscar_mutex(char* id, int pid);
void liberar_mutex(int pid, int tid);
bool esta_blocked(int tid, int pid);
void buscar_y_remover_de_cola(t_tcb* tcb);
void dormir_hilo(void* args);
void esperar_respuesta_dump_memory(t_tcb* tcb);

#endif