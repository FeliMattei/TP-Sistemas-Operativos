#include <instrucciones.h>

void ciclo_de_instruccion(int pid, int tid){  
   	t_instruccion instruccion_actual;

    hay_interrupcion = 0;
    ya_desalojado = 0;
    init = 1;
    
    t_buffer* buffer = crear_buffer();
    cargar_int_a_buffer(buffer, pid);
    cargar_int_a_buffer(buffer, tid);

    t_paquete* paquete_contexto = crear_paquete(OBTENER_CONTEXTO, buffer);
    log_info(cpu_logger, "## TID: %d - Solicito Contexto Ejecución", tid);

    enviar_paquete(paquete_contexto, conexion_memoria);
    destruir_paquete(paquete_contexto);

    op_code respuesta = recibir_operacion(conexion_memoria);
    t_buffer* buffer_recibido = recibir_buffer(conexion_memoria);
    
    if (respuesta == OBTENER_CONTEXTO_OK) {
        ctx_global_tid = extraer_contexto_del_buffer(buffer_recibido);
        destruir_buffer(buffer_recibido);

        while ((ctx_global_tid != NULL && !hay_interrupcion && !ya_desalojado) || init) {
            init = 0;
		    instruccion_actual = fetch(pid, tid, ctx_global_tid->registros->PC);	
	
		    if (decode(instruccion_actual.instruccion)){     
                if(!traducir(instruccion_actual)){
                    // SEGMENTATION FAULT
                    log_info(cpu_logger, "## TID: %d - Segmentation Fault", tid);
                    log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);
                    t_buffer* buffer_contexto = crear_buffer();
                    cargar_int_a_buffer(buffer_contexto, pid);
                    cargar_int_a_buffer(buffer_contexto, tid);
                    cargar_registros_a_buffer(buffer_contexto, ctx_global_tid->registros);
                    t_paquete* paquete_contexto = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_contexto);
                    enviar_paquete(paquete_contexto, conexion_memoria);
                    destruir_paquete(paquete_contexto);

                    op_code respuesta = recibir_operacion(conexion_memoria);
                    t_buffer* buffer_recibido = recibir_buffer(conexion_memoria);
                    destruir_buffer(buffer_recibido);

                    t_buffer* buffer_exit = crear_buffer();
                    cargar_int_a_buffer(buffer_exit, pid);
                    cargar_int_a_buffer(buffer_exit, tid);
                    t_paquete* paquete_exit = crear_paquete(PROCESS_EXIT_SUCCESS, buffer_exit);
                    enviar_paquete(paquete_exit, fd_kernel_dispatch);
                    destruir_paquete(paquete_exit);

                    liberar_contexto_completo(ctx_global_tid);
                    ctx_global_tid = NULL;
                    return;
                }
            }   

            execute(instruccion_actual, ctx_global_tid, tid, pid);
            
            if(hay_interrupcion && ctx_global_tid != NULL && !ya_desalojado){
                // Enviar a memoria para que actualicen el contexto
                // TODO: TESTEAME Y ARREGLAME
                log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);
                t_buffer* buffer_desalojo_memoria = crear_buffer();
                cargar_int_a_buffer(buffer_desalojo_memoria, pid);
                cargar_int_a_buffer(buffer_desalojo_memoria, tid);
                cargar_registros_a_buffer(buffer_desalojo_memoria, ctx_global_tid->registros);
                t_paquete* paquete_desalojo_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_desalojo_memoria);
                enviar_paquete(paquete_desalojo_memoria, conexion_memoria);
                destruir_paquete(paquete_desalojo_memoria);

                op_code resp = recibir_operacion(conexion_memoria);
                destruir_buffer(recibir_buffer(conexion_memoria));

                t_buffer* buffer_desalojo_kernel = crear_buffer();
                cargar_int_a_buffer(buffer_desalojo_kernel, pid);
                cargar_int_a_buffer(buffer_desalojo_kernel, tid);
                t_paquete* paquete_desalojo_kernel = crear_paquete(HILO_DESALOJADO, buffer_desalojo_kernel);
                enviar_paquete(paquete_desalojo_kernel, fd_kernel_dispatch);
                destruir_paquete(paquete_desalojo_kernel);

                liberar_contexto_completo(ctx_global_tid);
                ctx_global_tid = NULL;     
            }
        }
        liberar_contexto_completo(ctx_global_tid);
    } else {
        destruir_buffer(buffer_recibido);
    }
}

t_instruccion fetch(int pid, int tid, int pc){
    log_info(cpu_logger,"## TID: %d - FETCH - Program Counter: %d.", tid, pc);

    t_instruccion instruccion = solicitar_instruccion_a_memoria(pid, tid, pc);
    return instruccion;
} 

t_instruccion solicitar_instruccion_a_memoria(int pid, int tid, int pc) {   
    t_buffer *buffer = crear_buffer();
    cargar_int_a_buffer(buffer, tid);
    cargar_int_a_buffer(buffer, pid);
    cargar_int_a_buffer(buffer, pc);
    
    t_paquete *paquete = crear_paquete(SOLICITAR_INSTRUCCION, buffer);
    enviar_paquete(paquete, conexion_memoria);
    destruir_paquete(paquete);

    op_code op_code = recibir_operacion(conexion_memoria);
    t_buffer *buffer_recibido = recibir_buffer(conexion_memoria);

    if(op_code == SOLICITAR_INSTRUCCION_OK){
        t_instruccion_a_enviar instruccion = extraer_instruccion_a_enviar_del_buffer(buffer_recibido);
        t_instruccion nueva_instruccion = crear_instruccion_nuevamente(instruccion, buffer_recibido);
        destruir_buffer(buffer_recibido);
        //destruir_buffer(buffer);
        
        return nueva_instruccion;
    } else{
       log_error(cpu_logger, "Ocurrio un error al recibir la instruccion");
       abort();
    }
}


bool decode(instruccion instr){
    return (instr == WRITE_MEM || instr == READ_MEM);
}

void execute(t_instruccion instruccion, t_contexto_completo* contexto_tid, int tid, int pid){
    t_buffer* buffer_request = crear_buffer();
    t_buffer* buffer_request_2 = crear_buffer();
    t_paquete* paquete_kernel;
    t_paquete* paquete_memoria;
    op_code respuesta;

    switch (instruccion.instruccion){
        case SET: // SET (Registro, Valor)
            char* registro_set = (char*)list_get(instruccion.parametros, 0);
            char* valor_inst = (char*)list_get(instruccion.parametros, 1);
            int valor = atoi(valor_inst);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <SET> - <%s %d>", tid, registro_set, valor);
            asignar_valor_a_registro(contexto_tid, registro_set, valor);
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);
            list_destroy_and_destroy_elements(instruccion.parametros, free);

            destruir_buffer(buffer_request);
            destruir_buffer(buffer_request_2);
        break;
    
        case SUM: // SUM( Registro Destino, Registro Origen)
            char* registro_destino_sum = (char*) list_get(instruccion.parametros, 0);
            char* registro_origen_sum = (char*) list_get(instruccion.parametros, 1);

            int valor_origen_sum = *(uint32_t*) obtener_puntero_al_registro(contexto_tid, registro_origen_sum);    
            int valor_destino_sum = *(uint32_t*) obtener_puntero_al_registro(contexto_tid, registro_destino_sum);    

            log_info(cpu_logger, "## TID: %d - Ejecutando: <SUM> - <%s %s>", tid, registro_destino_sum, registro_origen_sum);
            asignar_valor_a_registro(contexto_tid, registro_destino_sum, valor_destino_sum + valor_origen_sum);
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);
            list_destroy_and_destroy_elements(instruccion.parametros, free);

            destruir_buffer(buffer_request);
            destruir_buffer(buffer_request_2);
        break;

        case SUB: // SUB (Registro Destino, Registro Origen)
            char* registro_destino_sub = (char*) list_get(instruccion.parametros, 0);
            char* registro_origen_sub = (char*) list_get(instruccion.parametros, 1);            
            
            int valor_destino_sub = *(uint32_t*) obtener_puntero_al_registro(contexto_tid, registro_destino_sub);    
            int valor_origen_sub = *(uint32_t*) obtener_puntero_al_registro(contexto_tid, registro_origen_sub);
            
            log_info(cpu_logger, "## TID: %d - Ejecutando: <SUB> - <%s %s>", tid, registro_destino_sub, registro_origen_sub);
            asignar_valor_a_registro(contexto_tid, registro_destino_sub, valor_destino_sub - valor_origen_sub);
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);
            list_destroy_and_destroy_elements(instruccion.parametros, free);

            destruir_buffer(buffer_request);
            destruir_buffer(buffer_request_2);
        break;

        case JNZ: // JNZ (Registro, Instrucción)
            char* registro_jnz = (char*) list_get(instruccion.parametros, 0);
            char* numero_aux = (char*) list_get(instruccion.parametros, 1); 
            int numero_instruccion = atoi(numero_aux); 

            int valor_registro = *(uint32_t*) obtener_puntero_al_registro(contexto_tid, registro_jnz);    
            
            log_info(cpu_logger, "## TID: %d - Ejecutando: <JNZ> - <%s %d>", tid, registro_jnz, numero_instruccion);
            if (valor_registro != 0) {
                contexto_tid->registros->PC = numero_instruccion;
            } else {
                asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);
            }
            list_destroy_and_destroy_elements(instruccion.parametros, free);


            destruir_buffer(buffer_request);
            destruir_buffer(buffer_request_2);
        break;

        case PROCESS_CREATE:
            char* archivo_instrucciones = (char*)list_get(instruccion.parametros, 0);
            char* char_tamanio = (char*)list_get(instruccion.parametros, 1);
            char* prioridad_tid = (char*)list_get(instruccion.parametros, 2);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <PROCESS_CREATE> - <%s %d %d>", tid, archivo_instrucciones, atoi(char_tamanio), atoi(prioridad_tid));

            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            cargar_string_a_buffer(buffer_request, archivo_instrucciones);
            cargar_string_a_buffer(buffer_request, char_tamanio);
            cargar_string_a_buffer(buffer_request, prioridad_tid);
            paquete_kernel = crear_paquete(PROCESS_CREATE, buffer_request);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);

            destruir_buffer(buffer_request_2);

            list_destroy_and_destroy_elements(instruccion.parametros, free);
    
        break;

        case PROCESS_EXIT:
            log_info(cpu_logger, "## TID: %d - Ejecutando: <PROCESS_EXIT>", tid);
            log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);
            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            cargar_registros_a_buffer(buffer_request, contexto_tid->registros);
            paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request);
            enviar_paquete(paquete_memoria, conexion_memoria);
            destruir_paquete(paquete_memoria);
            
            respuesta = recibir_operacion(conexion_memoria);
            destruir_buffer(recibir_buffer(conexion_memoria));

            cargar_int_a_buffer(buffer_request_2, pid);
            cargar_int_a_buffer(buffer_request_2, tid);
            paquete_kernel = crear_paquete(PROCESS_EXIT_SUCCESS, buffer_request_2);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);

            list_destroy_and_destroy_elements(instruccion.parametros, free);


            liberar_contexto_completo(ctx_global_tid);
            ctx_global_tid = NULL;
        break;  

        case THREAD_CREATE:
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);
            char* archivo_instrucciones_t = (char*)list_get(instruccion.parametros, 0);
            char* prioridad_tid_t = (char*)list_get(instruccion.parametros, 1);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <THREAD_CREATE> - <%s %d>", tid, archivo_instrucciones_t, atoi(prioridad_tid_t));

            cargar_string_a_buffer(buffer_request_2, archivo_instrucciones_t);
            cargar_string_a_buffer(buffer_request_2, prioridad_tid_t);
            cargar_int_a_buffer(buffer_request_2, pid);
            cargar_int_a_buffer(buffer_request_2, tid);

            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            cargar_registros_a_buffer(buffer_request, contexto_tid->registros);
            paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request);
            enviar_paquete(paquete_memoria, conexion_memoria);
            destruir_paquete(paquete_memoria);

            respuesta = recibir_operacion(conexion_memoria);
            destruir_buffer(recibir_buffer(conexion_memoria));

            paquete_kernel = crear_paquete(THREAD_CREATE, buffer_request_2);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);


            list_destroy_and_destroy_elements(instruccion.parametros, free);
        break;

        case THREAD_JOIN:
            char* tid_join = (char*)list_get(instruccion.parametros, 0);

            log_info(cpu_logger, "## TID: %d - Ejecutando: <THREAD_JOIN> - <%d>", tid, atoi(tid_join));

            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);

            log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);
            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            cargar_registros_a_buffer(buffer_request, contexto_tid->registros);
            paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request);
            enviar_paquete(paquete_memoria, conexion_memoria);
            destruir_paquete(paquete_memoria);

            respuesta = recibir_operacion(conexion_memoria);
            destruir_buffer(recibir_buffer(conexion_memoria));            
            
            cargar_int_a_buffer(buffer_request_2, pid);
            cargar_int_a_buffer(buffer_request_2, tid);
            cargar_string_a_buffer(buffer_request_2, tid_join);
            paquete_kernel = crear_paquete(THREAD_JOIN, buffer_request_2);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);
            
            op_code respuesta_t_join = recibir_operacion(fd_kernel_dispatch);
            destruir_buffer(recibir_buffer(fd_kernel_dispatch));

            list_destroy_and_destroy_elements(instruccion.parametros, free);


            if(respuesta_t_join == OK){
                liberar_contexto_completo(ctx_global_tid);
                ctx_global_tid = NULL;
            }

        break;

        case THREAD_CANCEL:
            char* tid_cancel = (char*)list_get(instruccion.parametros, 0);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <THREAD_CANCEL> - <%d>", tid, atoi(tid_cancel));
            
            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            cargar_string_a_buffer(buffer_request, tid_cancel);

            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);

            log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);
            cargar_int_a_buffer(buffer_request_2, pid);
            cargar_int_a_buffer(buffer_request_2, tid);
            cargar_registros_a_buffer(buffer_request_2, contexto_tid->registros);
            paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request_2);
            enviar_paquete(paquete_memoria, conexion_memoria);
            destruir_paquete(paquete_memoria);

            respuesta = recibir_operacion(conexion_memoria);
            destruir_buffer(recibir_buffer(conexion_memoria));            
            
            paquete_kernel = crear_paquete(THREAD_CANCEL, buffer_request);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);


            list_destroy_and_destroy_elements(instruccion.parametros, free);
        break;

        case THREAD_EXIT:
            log_info(cpu_logger, "## TID: %d - Ejecutando: <THREAD_EXIT>", tid);

            log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);
            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            cargar_registros_a_buffer(buffer_request, contexto_tid->registros);
            paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request);
            enviar_paquete(paquete_memoria, conexion_memoria);
            destruir_paquete(paquete_memoria);

            respuesta = recibir_operacion(conexion_memoria);
            destruir_buffer(recibir_buffer(conexion_memoria));            

            cargar_int_a_buffer(buffer_request_2, pid);
            cargar_int_a_buffer(buffer_request_2, tid);
            paquete_kernel = crear_paquete(THREAD_EXIT_SUCCESS, buffer_request_2);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);

            list_destroy_and_destroy_elements(instruccion.parametros, free);


            liberar_contexto_completo(ctx_global_tid);
            ctx_global_tid = NULL;
        break;

        case MUTEX_CREATE:
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);

            char* id = (char*)list_get(instruccion.parametros, 0);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <MUTEX_CREATE> - <%s>", tid, id);

            cargar_string_a_buffer(buffer_request, id);
            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            paquete_kernel = crear_paquete(MUTEX_CREATE, buffer_request);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);

            destruir_buffer(buffer_request_2); 

            list_destroy_and_destroy_elements(instruccion.parametros, free);
        break;

        case MUTEX_LOCK:
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);

            char* id_mutex = (char*)list_get(instruccion.parametros, 0);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <MUTEX_LOCK> - <%s>", tid, id_mutex);

            cargar_string_a_buffer(buffer_request, id_mutex);
            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            paquete_kernel = crear_paquete(MUTEX_LOCK, buffer_request);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);

            op_code respuesta_kernel = recibir_operacion(fd_kernel_dispatch);
            destruir_buffer(recibir_buffer(fd_kernel_dispatch));

            if (respuesta_kernel == MUTEX_BLOQUEADO) {
                log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);
                t_buffer* buffer_request_2 = crear_buffer();
                cargar_int_a_buffer(buffer_request_2, pid);
                cargar_int_a_buffer(buffer_request_2, tid);
                cargar_registros_a_buffer(buffer_request_2, ctx_global_tid->registros);
                paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request_2);
                enviar_paquete(paquete_memoria, conexion_memoria);
                destruir_paquete(paquete_memoria);

                respuesta = recibir_operacion(conexion_memoria);
                destruir_buffer(recibir_buffer(conexion_memoria));
                ya_desalojado = 1;
            }

            if (respuesta_kernel == MUTEX_INEXISTENTE) {
                log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);

                cargar_int_a_buffer(buffer_request_2, pid);
                cargar_int_a_buffer(buffer_request_2, tid);
                cargar_registros_a_buffer(buffer_request_2, contexto_tid->registros);
                paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request_2);
                enviar_paquete(paquete_memoria, conexion_memoria);
                destruir_paquete(paquete_memoria);

                respuesta = recibir_operacion(conexion_memoria);
                destruir_buffer(recibir_buffer(conexion_memoria));            

                t_buffer* buffer_exit_thread_2 = crear_buffer();
                cargar_int_a_buffer(buffer_exit_thread_2, pid);
                cargar_int_a_buffer(buffer_exit_thread_2, tid);
                t_paquete* paquete_exit_thread_2 = crear_paquete(THREAD_EXIT_SUCCESS, buffer_exit_thread_2);
                enviar_paquete(paquete_exit_thread_2, fd_kernel_dispatch);

                destruir_paquete(paquete_exit_thread_2);
                liberar_contexto_completo(ctx_global_tid);
                ctx_global_tid = NULL;
            }

            list_destroy_and_destroy_elements(instruccion.parametros, free);

        break;

        case MUTEX_UNLOCK:
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);

            char* id_mutex_2 = (char*)list_get(instruccion.parametros, 0);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <MUTEX_UNLOCK> - <%s>", tid, id_mutex_2);

            cargar_string_a_buffer(buffer_request, id_mutex_2);
            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);

            paquete_kernel = crear_paquete(MUTEX_UNLOCK, buffer_request);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);

            op_code respuesta_kernel_2 = recibir_operacion(fd_kernel_dispatch);
            destruir_buffer(recibir_buffer(fd_kernel_dispatch));

            if (respuesta_kernel_2 == MUTEX_INEXISTENTE) {
                log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);

                cargar_int_a_buffer(buffer_request_2, pid);
                cargar_int_a_buffer(buffer_request_2, tid);
                cargar_registros_a_buffer(buffer_request_2, contexto_tid->registros);
                paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request_2);
                enviar_paquete(paquete_memoria, conexion_memoria);
                destruir_paquete(paquete_memoria);

                respuesta = recibir_operacion(conexion_memoria);
                destruir_buffer(recibir_buffer(conexion_memoria));            

                t_buffer* buffer_exit_thread_2 = crear_buffer();
                cargar_int_a_buffer(buffer_exit_thread_2, pid);
                cargar_int_a_buffer(buffer_exit_thread_2, tid);
                t_paquete* paquete_exit_thread_2 = crear_paquete(THREAD_EXIT_SUCCESS, buffer_exit_thread_2);
                enviar_paquete(paquete_exit_thread_2, fd_kernel_dispatch);

                destruir_paquete(paquete_exit_thread_2);
                liberar_contexto_completo(ctx_global_tid);
                ctx_global_tid = NULL;
            }

            list_destroy_and_destroy_elements(instruccion.parametros, free);

        break;

        case READ_MEM:
            char* registro_datos_r = (char*) list_get(instruccion.parametros, 0);
            int registro_direccion_r = (int) list_get(instruccion.parametros, 1);
            char* dir_aux = (char*) list_get(instruccion.parametros, 2);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <READ_MEM> - <%s %s>", tid, registro_datos_r, dir_aux);
            log_info(cpu_logger, "## TID: %d - Acción: LEER - Direccion Fisica: %d", tid, registro_direccion_r);
            
            cargar_int_a_buffer(buffer_request, registro_direccion_r);
            paquete_memoria = crear_paquete(READ_MEM, buffer_request);
            enviar_paquete(paquete_memoria, conexion_memoria);
            destruir_paquete(paquete_memoria);

            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);

            respuesta = recibir_operacion(conexion_memoria);
            t_buffer* response_r_mem = recibir_buffer(conexion_memoria);
            int valor_extraido = *(int*)extraer_de_buffer(response_r_mem);
            log_debug(cpu_logger, "TID: %d - Se leyo el valor: %d", tid, valor_extraido);
            destruir_buffer(response_r_mem);

            asignar_valor_a_registro(ctx_global_tid, registro_datos_r, valor_extraido);
            
            list_remove(instruccion.parametros, 1);
            list_destroy_and_destroy_elements(instruccion.parametros, free);

            destruir_buffer(buffer_request_2);
        break;

        case WRITE_MEM:
            char* registro_datos_w = (char*) list_get(instruccion.parametros, 0);
            int dir_fisica = list_get(instruccion.parametros, 1);
            char* dir_aux_r = (char*) list_get(instruccion.parametros, 2);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <WRITE_MEM> - <%s %s>", tid, dir_aux_r, registro_datos_w);
            log_info(cpu_logger, "## TID: %d - Acción: ESCRIBIR - Direccion Fisica: %d", tid, dir_fisica);

            void* valor_datos = (void*) obtener_puntero_al_registro(contexto_tid, registro_datos_w);
            
            cargar_int_a_buffer(buffer_request, dir_fisica);
            cargar_a_buffer(buffer_request, valor_datos, 4);

            paquete_memoria = crear_paquete(WRITE_MEM, buffer_request);
            enviar_paquete(paquete_memoria, conexion_memoria);
            destruir_paquete(paquete_memoria);

            respuesta = recibir_operacion(conexion_memoria);
            t_buffer* response_w_mem = recibir_buffer(conexion_memoria);

            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);

            if(respuesta == WRITE_MEM_OK){
                log_debug(cpu_logger, "TID: %d - Escribio Memoria - Direccion Fisica: %d - Valor: %d", tid, dir_fisica, *(int*)valor_datos);
            } else {
                log_debug(cpu_logger, "TID: %d - Error al escribir en Memoria", tid);
                hay_interrupcion = 1;
            }

            list_remove(instruccion.parametros, 1);
            list_destroy_and_destroy_elements(instruccion.parametros, free);

            destruir_buffer(response_w_mem);
            destruir_buffer(buffer_request_2);
        break;

        case DUMP_MEMORY:
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <DUMP_MEMORY>", tid);
            log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);
            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            cargar_registros_a_buffer(buffer_request, contexto_tid->registros);
            paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request);
            enviar_paquete(paquete_memoria, conexion_memoria);
            destruir_paquete(paquete_memoria);

            respuesta = recibir_operacion(conexion_memoria);
            destruir_buffer(recibir_buffer(conexion_memoria));            

            cargar_int_a_buffer(buffer_request_2, pid);
            cargar_int_a_buffer(buffer_request_2, tid);
            paquete_kernel = crear_paquete(DUMP_MEMORY, buffer_request_2);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);
            
            liberar_contexto_completo(ctx_global_tid);
            ctx_global_tid = NULL;

            list_destroy_and_destroy_elements(instruccion.parametros, free);
        break;

        case LOG:
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);
            char* registro_log = (char*) list_get(instruccion.parametros, 0);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <LOG> - <%s>", tid, registro_log);
            int valor_log = obtener_valor_de_registro(contexto_tid, registro_log);
            log_info(cpu_logger, "Log %s: %d", registro_log, valor_log);
            
            destruir_buffer(buffer_request_2);
            destruir_buffer(buffer_request);

            list_destroy_and_destroy_elements(instruccion.parametros, free);
        break;

        case IO:
            asignar_valor_a_registro(contexto_tid, "PC", contexto_tid->registros->PC + 1);
            log_info(cpu_logger, "## TID: %d - Actualizo Contexto Ejecución", tid);
            char* tiempo = (char*) list_get(instruccion.parametros, 0);
            log_info(cpu_logger, "## TID: %d - Ejecutando: <IO> - <%d>", tid, atoi(tiempo));
            cargar_int_a_buffer(buffer_request, pid);
            cargar_int_a_buffer(buffer_request, tid);
            cargar_registros_a_buffer(buffer_request, contexto_tid->registros);
            paquete_memoria = crear_paquete(ACTUALIZAR_CONTEXTO, buffer_request);
            enviar_paquete(paquete_memoria, conexion_memoria);
            destruir_paquete(paquete_memoria);

            respuesta = recibir_operacion(conexion_memoria);
            destruir_buffer(recibir_buffer(conexion_memoria));            

            cargar_int_a_buffer(buffer_request_2, pid);
            cargar_int_a_buffer(buffer_request_2, tid);
            cargar_string_a_buffer(buffer_request_2, tiempo);
            paquete_kernel = crear_paquete(IO, buffer_request_2);
            enviar_paquete(paquete_kernel, fd_kernel_dispatch);
            destruir_paquete(paquete_kernel);

            list_destroy_and_destroy_elements(instruccion.parametros, free);       

            liberar_contexto_completo(ctx_global_tid);
            ctx_global_tid = NULL;

        break; 

        default:
        break;
    }
}

t_instruccion crear_instruccion_nuevamente(t_instruccion_a_enviar instruccion_a_enviar, t_buffer *buffer_recibido){
    t_instruccion instruccion;
    instruccion.instruccion = instruccion_a_enviar.instruccion;
    instruccion.parametros = list_create();

    switch(instruccion.instruccion){
        case LOG:
        case IO:
        case THREAD_JOIN:
        case THREAD_CANCEL:
        case MUTEX_CREATE:
        case MUTEX_LOCK:
        case MUTEX_UNLOCK:
            list_add(instruccion.parametros, extraer_string_del_buffer(buffer_recibido));
            break;
        case SET:
        case SUM:
        case SUB:
        case JNZ:
        case READ_MEM:
        case WRITE_MEM:
        case THREAD_CREATE:
            list_add(instruccion.parametros, extraer_string_del_buffer(buffer_recibido));
            list_add(instruccion.parametros, extraer_string_del_buffer(buffer_recibido));
            break;
        case PROCESS_CREATE:
            list_add(instruccion.parametros, extraer_string_del_buffer(buffer_recibido));
            list_add(instruccion.parametros, extraer_string_del_buffer(buffer_recibido));
            list_add(instruccion.parametros, extraer_string_del_buffer(buffer_recibido));
            break;
        case THREAD_EXIT:
        case PROCESS_EXIT:
        case DUMP_MEMORY:
            break;
    }

    return instruccion;
}

void asignar_valor_a_registro(t_contexto_completo* contexto, char* registro, int valor) {
    uint32_t valor_32 = (uint32_t)valor; 

    if (strcmp(registro, "AX") == 0) {
        contexto->registros->AX = valor_32;
    } else if (strcmp(registro, "BX") == 0) {
        contexto->registros->BX = valor_32;
    } else if (strcmp(registro, "CX") == 0) {
        contexto->registros->CX = valor_32;
    } else if (strcmp(registro, "DX") == 0) {
        contexto->registros->DX = valor_32;
    } else if (strcmp(registro, "EX") == 0) {
        contexto->registros->EX = valor_32;
    } else if (strcmp(registro, "FX") == 0) {
        contexto->registros->FX = valor_32;
    } else if (strcmp(registro, "GX") == 0) {
        contexto->registros->GX = valor_32;
    } else if (strcmp(registro, "HX") == 0) {
        contexto->registros->HX = valor_32;
    } else if (strcmp(registro, "PC") == 0) {
        contexto->registros->PC = valor_32;
    } else {
        log_debug(cpu_logger, "Registro desconocido: %s\n", registro);
    }
}

void* obtener_puntero_al_registro(t_contexto_completo* contexto, char* registro) {
    if (strcmp(registro, "AX") == 0) {
        return &(contexto->registros->AX);
    } else if (strcmp(registro, "BX") == 0) {
        return &(contexto->registros->BX);
    } else if (strcmp(registro, "CX") == 0) {
        return &(contexto->registros->CX);
    } else if (strcmp(registro, "DX") == 0) {
        return &(contexto->registros->DX);
    } else if (strcmp(registro, "EX") == 0) {
        return &(contexto->registros->EX);
    } else if (strcmp(registro, "FX") == 0) {
        return &(contexto->registros->FX);
    } else if (strcmp(registro, "GX") == 0) {
        return &(contexto->registros->GX);
    } else if (strcmp(registro, "HX") == 0) {
        return &(contexto->registros->HX);
    } else if (strcmp(registro, "PC") == 0) {
        return &(contexto->registros->PC);
    } else {
        log_debug(cpu_logger, "Registro desconocido: %s\n", registro);
        return -1; // Error: registro desconocido
    }
}

int traducir (t_instruccion instruccion){
    int dir_logica;
    int dir_fisica;
    char* dir_aux;
    if(instruccion.instruccion == WRITE_MEM){
        dir_aux = list_get(instruccion.parametros, 0);
        dir_logica = obtener_valor_de_registro(ctx_global_tid, (char*)list_remove(instruccion.parametros,0));
    } else {
        dir_aux = list_get(instruccion.parametros, 1);
        dir_logica = obtener_valor_de_registro(ctx_global_tid, (char*)list_remove(instruccion.parametros,1));
    }
    
    if(dir_logica > ctx_global_tid->limite){
        return 0;
    }

    dir_fisica = ctx_global_tid->base + dir_logica;
    
    list_add(instruccion.parametros, dir_fisica);
    list_add(instruccion.parametros, dir_aux);

    return 1;
}

uint32_t obtener_valor_de_registro(t_contexto_completo* contexto, char* registro) {
    if (strcmp(registro, "AX") == 0) {
        return contexto->registros->AX;
    } else if (strcmp(registro, "BX") == 0) {
        return contexto->registros->BX;
    } else if (strcmp(registro, "CX") == 0) {
        return contexto->registros->CX;
    } else if (strcmp(registro, "DX") == 0) {
        return contexto->registros->DX;
    } else if (strcmp(registro, "EX") == 0) {
        return contexto->registros->EX;
    } else if (strcmp(registro, "FX") == 0) {
        return contexto->registros->FX;
    } else if (strcmp(registro, "GX") == 0) {
        return contexto->registros->GX;
    } else if (strcmp(registro, "HX") == 0) {
        return contexto->registros->HX;
    } else if (strcmp(registro, "PC") == 0) {
        return contexto->registros->PC;
    } else {
        log_debug(cpu_logger, "Registro desconocido: %s\n", registro);
        return -1; // Error: registro desconocido
    }
}

void liberar_contexto_completo(t_contexto_completo* contexto) {
    if (contexto != NULL) {
        if (contexto->registros != NULL) {
            free(contexto->registros);
        }
        free(contexto);
    }
}