#ifndef CPU_H_
#define CPU_H_

#include <cpu_global.h>
#include <instrucciones.h>

void atender_kernel_interrupt(void* socket_cliente_ptr);
void atender_kernel_dispatch(void* socket_cliente_ptr);

#endif