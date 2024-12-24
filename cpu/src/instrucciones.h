#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include <cpu_global.h>


t_instruccion crear_instruccion_nuevamente(t_instruccion_a_enviar instruccion_a_enviar, t_buffer *buffer_recibido);
void* obtener_puntero_al_registro(t_contexto_completo* contexto, char* registro);
t_instruccion fetch(int pid, int tid, int pc);
void execute(t_instruccion instruccion, t_contexto_completo* contexto_tid, int tid, int pid);
t_instruccion solicitar_instruccion_a_memoria(int pid, int tid, int pc);
uint32_t obtener_valor_de_registro(t_contexto_completo* contexto, char* registro);
bool decode(instruccion instr);
int traducir (t_instruccion instruccion);
void liberar_contexto_completo(t_contexto_completo* contexto);


#endif