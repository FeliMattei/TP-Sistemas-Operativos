#include "cpu.h"

int main(int argc, char* argv[]) {

    cpu_logger = iniciar_logger("cpu.log", "CPU");

    t_config* cpu_config = iniciar_config("/home/utnso/tp-2024-2c-Bit-Wizards/cpu/config/cpu.config");

    IP_MEMORIA = copiar_a_memoria(config_get_string_value(cpu_config, "IP_MEMORIA"));
	PUERTO_MEMORIA= copiar_a_memoria(config_get_string_value(cpu_config, "PUERTO_MEMORIA"));
	PUERTO_ESCUCHA_DISPATCH= copiar_a_memoria(config_get_string_value(cpu_config, "PUERTO_ESCUCHA_DISPATCH"));
	PUERTO_ESCUCHA_INTERRUPT= copiar_a_memoria(config_get_string_value(cpu_config, "PUERTO_ESCUCHA_INTERRUPT")); 
	LOG_LEVEL = copiar_a_memoria(config_get_string_value(cpu_config, "LOG_LEVEL"));

    config_destroy(cpu_config);

    socket_dispatch = iniciar_servidor(PUERTO_ESCUCHA_DISPATCH, cpu_logger);
	socket_interrupt = iniciar_servidor(PUERTO_ESCUCHA_INTERRUPT, cpu_logger);

    conexion_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	log_debug(cpu_logger, "Se conecto con Memoria");

    log_debug(cpu_logger, "Esperando a Kernel en el Puerto de Dispatch...");
    log_debug(cpu_logger, "Esperando a Kernel en el Puerto de Interrupt...");
    fd_kernel_dispatch = esperar_conexion(socket_dispatch);
	log_debug(cpu_logger, "Se conecto con Kernel en el Puerto de Dispatch");
    
	fd_kernel_interrupt = esperar_conexion(socket_interrupt);
	log_debug(cpu_logger, "Se conecto con Kernel en el Puerto de Interrupt");

    int* socket_cliente_kernel_dispatch_ptr = malloc(sizeof(int));
    *socket_cliente_kernel_dispatch_ptr = fd_kernel_dispatch;
    pthread_create(&hilo_kernel_dispatch, NULL, atender_kernel_dispatch, socket_cliente_kernel_dispatch_ptr);
    pthread_detach(hilo_kernel_dispatch);



    int* socket_cliente_kernel_interrupt_ptr = malloc(sizeof(int));
    *socket_cliente_kernel_interrupt_ptr = fd_kernel_interrupt;
    pthread_create(&hilo_kernel_interrupt, NULL, atender_kernel_interrupt, socket_cliente_kernel_interrupt_ptr);
    pthread_join(hilo_kernel_interrupt, NULL);

}

void atender_kernel_dispatch(void* socket_cliente_ptr) {
    int* ptr = (int*)socket_cliente_ptr;
    int cliente_kd = *ptr;
    
    while (1){          
        op_code operacion = recibir_operacion(cliente_kd);  
        log_debug(cpu_logger,"[D] Recibi operacion [%d]",operacion); 
        t_buffer* buffer = recibir_buffer(cliente_kd);

        switch(operacion) {
            case CONTEXTO_TID_PID:
                int pid = extraer_int_del_buffer(buffer);
                int tid = extraer_int_del_buffer(buffer);
                log_info(cpu_logger, "Recibi contexto TID %d y PID %d", tid, pid);
                ciclo_de_instruccion(pid, tid);
            break;

            default:
                log_error(cpu_logger, "[D] Operacion invalida");
            break;
	    }
        destruir_buffer(buffer);
    } 
    log_info(cpu_logger,"Fin de hilo de dispatch");
}


void atender_kernel_interrupt(void* socket_cliente_ptr) {
    int* ptr = (int*)socket_cliente_ptr;
    int cliente_ki = *ptr;

    while (1){
        op_code operacion = recibir_operacion(cliente_ki);    
        log_info(cpu_logger,"## Llega interrupción al puerto Interrupt");
        t_buffer* buffer = recibir_buffer(cliente_ki);
        
        switch(operacion) {
            // TODO: Falta realizar lógica de INTERRUPCION cuando se implementen lo/s algoritmo/s que lo requieran en Kernel
            case INTERRUPCION:
                log_debug(cpu_logger, "[I] Interrupcion generica");                     
                hay_interrupcion = 1;
                break;

            case FIN_QUANTUM:
                log_debug(cpu_logger, "[I] Fin de Quantum");
                hay_interrupcion = 1; // esto por ahi deberia ser fin_quantum si queremos atajarlo antes del execute en el ciclo de instruccion
                break;    
            default:
        
                log_error(cpu_logger, "[I] No se reconoce la operacion");
            break;
        }    
        destruir_buffer(buffer);
    } 

}
