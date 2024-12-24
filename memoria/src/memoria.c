#include "memoria.h"

algoritmos_memoria algoritmo_memoria;
algoritmos_memoria criterio_memoria;

int main(int argc, char *argv[]){

    inicializar_memoria();

    inicializar_memoria_sistema();

    inicializar_memoria_usuario();

    int memoria_fs = iniciar_servidor(memoria_config->puerto_escucha, memoria_log);

    log_debug(memoria_log, "Esperando a CPU...");
    int fd_cpu = esperar_conexion(memoria_fs);
    log_debug(memoria_log, "Se conecto con CPU");

    pthread_t hilo_cpu;

    int *socket_cliente_cpu_ptr = malloc(sizeof(int));
    *socket_cliente_cpu_ptr = fd_cpu;
    pthread_create(&hilo_cpu, NULL, atender_cpu, (void *)socket_cliente_cpu_ptr);
    pthread_detach(hilo_cpu);

    log_debug(memoria_log, "Atendiendo mensajes de CPU");    

    pthread_t peticion_kernel;
    pthread_create(&peticion_kernel, NULL, (void *)manejar_peticiones_kernel, (void *)&memoria_fs);
    pthread_detach(peticion_kernel);

    sem_wait(&fin_memoria);
}

void* manejar_peticiones_kernel(void *memoria_fs_ptr) {
    
    int memoria_fs = *(int *)memoria_fs_ptr;
    
    while(1) {
        int *fd_kernel_copia = malloc(sizeof(int));
        int fd_kernel = esperar_conexion(memoria_fs);
        *fd_kernel_copia = fd_kernel;
        log_info(memoria_log, "## Kernel Conectado - FD del socket: %d", fd_kernel);
  
        pthread_t atencion_kernel;
        pthread_create(&atencion_kernel, NULL, atender_kernel, fd_kernel_copia);
        pthread_detach(atencion_kernel);
    }
}

void* atender_kernel(void *fd_kernel_ptr) {
    int fd_kernel = *(int *)fd_kernel_ptr;
    free(fd_kernel_ptr);

    op_code operacion = recibir_operacion(fd_kernel);

    int denied = 0;

    int pid, tid;

    switch(operacion){
        case INICIALIZAR_PROCESO:
        // Recibir datos del proceso
        int tamanio_proceso;
        recibir_datos_proceso(fd_kernel, &pid, &tamanio_proceso);

        switch (algoritmo_memoria){
            case DINAMICAS:
                switch (criterio_memoria){
                    case BEST:
                        sem_wait(&sem_mutex_particiones);	
                        if (!asignar_memoria_particiones_best_fit(pid, tamanio_proceso)) {
                            log_error(memoria_log, "No hay espacio suficiente para el proceso PID: %d", pid);
                            enviar_denied(fd_kernel);
                            denied = 1;
                        }
                        sem_post(&sem_mutex_particiones);	
                    break;

                    case WORST:
                     sem_wait(&sem_mutex_particiones);	
                        if (!asignar_memoria_particiones_worst_fit(pid, tamanio_proceso)) {
                            log_error(memoria_log, "No hay espacio suficiente para el proceso PID: %d", pid);
                            enviar_denied(fd_kernel);
                            denied = 1;
                        }
                    sem_post(&sem_mutex_particiones);	
                    break;

                    case FIRST:
                     sem_wait(&sem_mutex_particiones);	
                        if (!asignar_memoria_particiones_first_fit(pid, tamanio_proceso)) {
                            log_error(memoria_log, "No hay espacio suficiente para el proceso PID: %d", pid);
                            enviar_denied(fd_kernel);
                            denied = 1;
                        } 
                    sem_post(&sem_mutex_particiones);	
                    break;

                    default:
                    break;
                }
            break;

            case FIJAS:
                switch (criterio_memoria){

                    case BEST:
                     sem_wait(&sem_mutex_particiones);	
                        if (!asignar_memoria_particiones_fijas_best_fit(pid, tamanio_proceso)) {
                            log_error(memoria_log, "No hay espacio suficiente para el proceso PID: %d", pid);
                            enviar_denied(fd_kernel);
                            denied = 1;
                        }
                    sem_post(&sem_mutex_particiones);	
                    break;

                    case WORST:
                     sem_wait(&sem_mutex_particiones);	
                        if (!asignar_memoria_particiones_fijas_worst_fit(pid, tamanio_proceso)) {
                            log_error(memoria_log, "No hay espacio suficiente para el proceso PID: %d", pid);
                            enviar_denied(fd_kernel);
                            denied = 1;
                        }
                    sem_post(&sem_mutex_particiones);	
                    break;

                    case FIRST:
                     sem_wait(&sem_mutex_particiones);	
                        if (!asignar_memoria_particiones_fijas_first_fit(pid, tamanio_proceso)) {
                            log_error(memoria_log, "No hay espacio suficiente para el proceso PID: %d", pid);
                            enviar_denied(fd_kernel);
                            denied = 1;
                        } 
                    sem_post(&sem_mutex_particiones);	
                    break;

                    default:
                    break;
                }
            break;

            default:
            break;
        }

        if (!denied) {
            // Almacenar el contexto del proceso
            sem_wait(&sem_mutex_particiones);
                t_particion *particion = obtener_particion(pid);
            sem_post(&sem_mutex_particiones);

            almacenar_contexto_proceso(pid, particion->base, particion->limite);
            log_info(memoria_log, "## Proceso Creado - PID: %d - Tamaño: %d", pid, tamanio_proceso);
            //free(particion);

            // Enviar confirmación al Kernel
            enviar_confirmacion(fd_kernel);
        }
        break;

        case FINALIZAR_PROCESO:
        recibir_datos_finalizar_proceso(fd_kernel, &pid);
        sem_wait(&sem_mutex_particiones);	
            finalizar_proceso(pid);
        sem_post(&sem_mutex_particiones);	

        log_info(memoria_log, "## Proceso Destruido - PID: %d", pid);

        enviar_confirmacion(fd_kernel);
        break;

        case INICIALIZAR_HILO:

        char* filePath;
        recibir_datos_inicializar_hilo(fd_kernel, &pid, &tid, &filePath);
        almacenar_contexto_hilo(pid, tid);

        t_list* instrucciones = procesar_archivo(filePath);
        free(filePath);
        almacenar_instrucciones(pid, tid, instrucciones);

        log_info(memoria_log, "## Hilo Creado - (PID:TID) - (%d:%d)", pid, tid);

        enviar_confirmacion(fd_kernel);
        break;

        case FINALIZAR_HILO:
        
        recibir_datos_finalizar_hilo(fd_kernel, &pid, &tid);
        finalizar_hilo(pid, tid);

        log_info(memoria_log, "## Hilo Destruido - (PID:TID) - (%d:%d)", pid, tid);

        enviar_confirmacion(fd_kernel);
        break;

        case DUMPY_MEMORY:
        
        recibir_datos_dump_memory(fd_kernel,&pid,&tid);
        log_info(memoria_log, "## Memory Dump solicitado - (PID:TID) - (%d:%d)", pid, tid);
        int base,limite;
        t_particion* aux;
        sem_wait(&sem_mutex_particiones);
        for(int i=0;i<list_size(particiones);i++){
            aux = list_get(particiones,i);
            if(aux->pid == pid){
               base = aux->base;
               limite = aux->limite;     
            }
        }
        sem_post(&sem_mutex_particiones);
        int tam = limite-base;
        void* info = malloc(tam);
        sem_wait(&sem_mutex_memoria_usuario);
        memcpy(info,(char*)memoria_usuario+base,tam);
        sem_post(&sem_mutex_memoria_usuario);
        char* file_name = armar_nombre(pid,tid);
        int conexion_fs = crear_conexion(memoria_config->ip_filesystem, memoria_config->puerto_filesystem);
        t_buffer* buffer = crear_buffer();
        cargar_string_a_buffer(buffer,file_name);
        cargar_int_a_buffer(buffer,tam);
        cargar_a_buffer(buffer,info,tam);
        
        t_paquete* paquete = crear_paquete(DUMPY_MEMORY,buffer);
        enviar_paquete(paquete,conexion_fs);
        
        free(info);
        
        destruir_paquete(paquete);
        op_code confirmacion = recibir_operacion(conexion_fs);
        destruir_buffer(recibir_buffer(conexion_fs));

        if(confirmacion == OK){
            enviar_confirmacion(fd_kernel);
        }else{
            enviar_denied(fd_kernel);
        }
        close(conexion_fs);
        break;

        default:
        break;
    }
    close(fd_kernel);
}
    

void *atender_cpu(void *fd_cpu){
    int cpu_cliente = *(int *)fd_cpu;
    free(fd_cpu);

    t_buffer* buffer_recibido;
    t_buffer* response_buffer;
    t_paquete* response;
    int direccion_fisica;
    int pid, tid, pg;
    while (1) {
        op_code op_code = recibir_operacion(cpu_cliente);
        switch (op_code){
            case OBTENER_CONTEXTO:

                buffer_recibido = recibir_buffer(cpu_cliente);

                usleep(memoria_config->retardo_respuesta * 1000);

                pid = extraer_int_del_buffer(buffer_recibido);
                tid = extraer_int_del_buffer(buffer_recibido);

                log_debug(memoria_log, "Se solicita obtener contexto - PID: %d - TID: %d", pid, tid);
                t_contexto_completo *contexto = obtener_contexto_completo(pid, tid);
                destruir_buffer(buffer_recibido);

                if (contexto == NULL){
                    log_error(memoria_log, "No se encontro el contexto solicitado");
                    break;
                }

                response_buffer = crear_buffer();

                cargar_contexto_a_buffer(response_buffer, contexto);

                response = crear_paquete(OBTENER_CONTEXTO_OK, response_buffer);

                enviar_paquete(response, cpu_cliente);
                destruir_paquete(response);

                free(contexto->registros);
                free(contexto);

                log_info(memoria_log, "## Contexto Solicitado - (PID:TID) - (%d, %d)", pid, tid);

                break;

            case ACTUALIZAR_CONTEXTO:
                buffer_recibido = recibir_buffer(cpu_cliente);

                usleep(memoria_config->retardo_respuesta * 1000);

                pid = extraer_int_del_buffer(buffer_recibido);

                tid = extraer_int_del_buffer(buffer_recibido);

                t_registros* registros = extraer_registros_del_buffer(buffer_recibido);

                actualizar_contexto_hilo(pid, tid, registros); //Deberia hacer if si tirar error? 

                destruir_buffer(buffer_recibido);
                free(registros);

                log_info(memoria_log, "## Contexto Actualizado - (PID:TID) - (%d, %d)", pid, tid);

                response_buffer = crear_buffer();

                cargar_int_a_buffer(response_buffer, OK);

                response = crear_paquete(OK, response_buffer);
                enviar_paquete(response, cpu_cliente);
                destruir_paquete(response);
                break;
            case SOLICITAR_INSTRUCCION:
                buffer_recibido = recibir_buffer(cpu_cliente);

                usleep(memoria_config->retardo_respuesta * 1000);

                tid = extraer_int_del_buffer(buffer_recibido);
                pid = extraer_int_del_buffer(buffer_recibido);
                pg = extraer_int_del_buffer(buffer_recibido);

                destruir_buffer(buffer_recibido);

                t_instruccion* instruccion = obtener_instruccion(pid, tid, pg);
                response_buffer = crear_buffer();
                t_instruccion_a_enviar instruccion_a_enviar;
                instruccion_a_enviar.instruccion = instruccion->instruccion;
                cargar_instruccion_a_enviar_a_buffer(response_buffer, instruccion_a_enviar);

                int log_size = 100; 
                for (int i = 0; i < instruccion->tamanio_lista; i++) {
                    log_size += strlen(list_get(instruccion->parametros, i)) + 1;
                }

                char* log = malloc(log_size);
                log[0] = '\0';
                strcat(log, map_instruccion_a_string(instruccion->instruccion));
                strcat(log, " ");
                for (int i = 0; i < instruccion->tamanio_lista; i++) {
                    char* parametro = list_get(instruccion->parametros, i);
                    strcat(log, parametro);
                    cargar_string_a_buffer(response_buffer, parametro);
                    if (i < instruccion->tamanio_lista - 1) {
                        strcat(log, " "); 
                    }
                }

                log_info(memoria_log, " ## Obtener instrucción - (PID:TID) - (%d:%d) - Instrucción: %s", pid, tid, log);
                free(log);

                response = crear_paquete(SOLICITAR_INSTRUCCION_OK, response_buffer);
                enviar_paquete(response, cpu_cliente);
                destruir_paquete(response);
                break;
            case READ_MEM:
                buffer_recibido = recibir_buffer(cpu_cliente);
                usleep(memoria_config->retardo_respuesta * 1000);

                direccion_fisica = extraer_int_del_buffer(buffer_recibido);

                response_buffer = crear_buffer();

                void* valor_a_cargar_en_buffer = malloc(4);
                int valor_leido; // Variable para almacenar el valor leído y mostrarlo en el log


                sem_wait(&sem_mutex_memoria_usuario);
                memcpy(valor_a_cargar_en_buffer, ((char*)memoria_usuario)+direccion_fisica, 4);
                valor_leido = *(int*)valor_a_cargar_en_buffer; // Convertir los primeros 4 bytes a un entero

                sem_post(&sem_mutex_memoria_usuario);

                cargar_a_buffer(response_buffer, valor_a_cargar_en_buffer, 4);

                log_info(memoria_log, "## Lectura - (PID:TID) - (%d:%d) - Dir. Fisica: %d - Tamaño: 4", pid, tid, direccion_fisica);

                response = crear_paquete(OK, response_buffer);
                enviar_paquete(response, cpu_cliente);
                destruir_paquete(response);
                destruir_buffer(buffer_recibido);
                free(valor_a_cargar_en_buffer);
                break;
            case WRITE_MEM:
                buffer_recibido = recibir_buffer(cpu_cliente);
                usleep(memoria_config->retardo_respuesta * 1000);

                direccion_fisica = extraer_int_del_buffer(buffer_recibido);

                void* valor_a_escribir = extraer_de_buffer(buffer_recibido);

                response_buffer = crear_buffer();


                sem_wait(&sem_mutex_memoria_usuario);
                memcpy(((char*)memoria_usuario)+ direccion_fisica, valor_a_escribir, 4);

                sem_post(&sem_mutex_memoria_usuario);

                free(valor_a_escribir);
                cargar_int_a_buffer(response_buffer, WRITE_MEM_OK);


                log_info(memoria_log, "## Escritura - (PID:TID) - (%d:%d) - Dir. Fisica: %d - Tamaño: 4 bytes", pid, tid, direccion_fisica);
                 
                response = crear_paquete(WRITE_MEM_OK, response_buffer);
                enviar_paquete(response, cpu_cliente);
                destruir_paquete(response);

                destruir_buffer(buffer_recibido);
                break;

            default:
        
                break;
        }
    }

    return NULL;
}

t_list* procesar_archivo(char* filePath) {
   FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        log_error(memoria_log, "No se pudo abrir el archivo de instrucciones.");
        return NULL;
    }

    char linea[256];
     
    t_list* instrucciones = list_create();
    int cantidad_instrucciones = 0;
    while (fgets(linea, sizeof(linea), file) != NULL) {
        t_instruccion* instruccion = malloc(sizeof(t_instruccion));
        char* token = strtok(linea, " ");
        
        if(strcmp(token,"DUMP_MEMORY\n") == 0){
            token[strcspn(token, "\n")] = 0;
        }

        instruccion->instruccion = map_instruccion_a_enum(token);
        instruccion->parametros = list_create();
        while ((token = strtok(NULL, " ")) != NULL) {
            token[strcspn(token, "\n")] = 0;
            char* parametro = strdup(token); 
            list_add(instruccion->parametros, parametro);
        }

        list_add(instrucciones, instruccion);
        instruccion->tamanio_lista = list_size(instruccion->parametros);
        cantidad_instrucciones++;
    }
    
    fclose(file);

    return instrucciones;
}

bool asignar_memoria_particiones_dinamicas(int pid, int tamanio_proceso){
    for (int i = 0; i < list_size(particiones); i++){
        t_particion *particion = list_get(particiones, i);
        if (particion->libre && particion->limite >= tamanio_proceso){
            particion->libre = false;
            int base_original = particion->base;
            particion->limite = tamanio_proceso;

            log_info(memoria_log, "Partición asignada - PID: %d, Base: %d, Tamaño: %d", pid, base_original, tamanio_proceso);

            // Si queda espacio libre, se crea una nueva partición con la memoria sobrante
            if (particion->limite > tamanio_proceso){
                t_particion *nueva_particion = malloc(sizeof(t_particion));
                nueva_particion->base = base_original + tamanio_proceso;
                nueva_particion->limite = particion->limite - tamanio_proceso;
                nueva_particion->libre = true;
                list_add(particiones, nueva_particion);
                log_info(memoria_log, "Nueva partición libre creada - Base: %d, Tamaño: %d", nueva_particion->base, nueva_particion->limite);
            }
            return true;
        }
    }
    return false;
}

bool asignar_memoria_particiones_fijas(int pid, int tamanio_proceso) {
    if (strcmp(memoria_config->algoritmo_busqueda, "FIRST") == 0) {
        return asignar_memoria_particiones_fijas_first_fit(pid, tamanio_proceso);
    } else if (strcmp(memoria_config->algoritmo_busqueda, "BEST") == 0) {
        return asignar_memoria_particiones_fijas_best_fit(pid, tamanio_proceso);
    } else if (strcmp(memoria_config->algoritmo_busqueda, "WORST") == 0) {
        return asignar_memoria_particiones_fijas_worst_fit(pid, tamanio_proceso);
    }
    log_error(memoria_log, "Algoritmo de búsqueda desconocido para particiones fijas: %s", memoria_config->algoritmo_busqueda);
    return false;
}

void finalizar_proceso(int pid){
    for (int i = 0; i < list_size(particiones); i++){
        t_particion *particion = list_get(particiones, i);
        if (particion->pid == pid){
            particion->libre = true;
            log_info(memoria_log, "Proceso finalizado - PID: %d, Partición liberada: Base: %d, Límite: %d", pid, particion->base, particion->limite);
            if(strcmp(memoria_config->esquema, "DINAMICAS") == 0 ){
                consolidar_particiones(i);
            }
            break;
        }
    }
}

void consolidar_particiones(int index){
    t_particion *particion_actual = list_get(particiones, index);

    // Consolidar con la siguiente partición si está libre
    if (index + 1 < list_size(particiones)){
        t_particion *particion_siguiente = list_get(particiones, index + 1);
        if (particion_siguiente->libre){
            particion_actual->limite = particion_siguiente->limite;
            list_remove(particiones, index + 1);
            log_info(memoria_log, "Particiones consolidadas en el índice %d", index);
        }
    }

    // Consolidar con la partición anterior si está libre
    if (index > 0){
        t_particion *particion_anterior = list_get(particiones, index - 1);
        if (particion_anterior->libre){
            particion_anterior->limite = particion_actual->limite;
            list_remove(particiones, index);
            log_info(memoria_log, "Particiones consolidadas en el índice %d", index - 1);
        }
    }
}

/*void almacenar_contexto_hilo(int tid)
{
    t_contexto_hilo *contexto = malloc(sizeof(t_contexto_hilo));
    memset(contexto, 0, sizeof(t_contexto_hilo));
    dictionary_put(contextos_hilos, &tid, contexto);
}*/

void finalizar_hilo(int pid, int tid){
    char key[25];
    sprintf(key, "%d,%d", pid, tid);
    
    sem_wait(&sem_mutex_contextos_hilos);
        t_contexto_hilo *contexto = dictionary_remove(contextos_hilos, key);
    sem_post(&sem_mutex_contextos_hilos);

    if (contexto != NULL){
        free(contexto);
        log_info(memoria_log, "Hilo finalizado - TID: %d", tid);
    }
    else{
        log_error(memoria_log, "No se encontró el contexto del hilo con TID: %d", tid);
    }
}

void recibir_datos_proceso(int fd_kernel, int* pid, int* tamanio_proceso) {
    t_buffer* buffer = recibir_buffer(fd_kernel);
    *pid = extraer_int_del_buffer(buffer);       
    *tamanio_proceso = extraer_int_del_buffer(buffer);
    destruir_buffer(buffer);   
}

void recibir_datos_dump_memory(int conexion,int* pid, int* tid){
    t_buffer* buffer = recibir_buffer(conexion);  
    *pid = extraer_int_del_buffer(buffer);         
    *tid = extraer_int_del_buffer(buffer);
    destruir_buffer(buffer);
}

t_particion* obtener_particion(int pid) {
    for (int i = 0; i < list_size(particiones); i++) {
        t_particion* particion = list_get(particiones, i); 
        if (particion->pid == pid) {                      
            return particion;                              
        }
    }
    return NULL; 
}

void recibir_datos_inicializar_hilo(int fd_kernel, int* pid, int* tid, char** filePath) {
    t_buffer* buffer = recibir_buffer(fd_kernel);  
    *pid = extraer_int_del_buffer(buffer);         
    *tid = extraer_int_del_buffer(buffer);
    *filePath = extraer_string_del_buffer(buffer);   
    destruir_buffer(buffer);                       
}

void enviar_denied(int fd_kernel) {
    t_buffer* buffer_sin_uso = crear_buffer();
    cargar_int_a_buffer(buffer_sin_uso, 0);
    t_paquete* paquete = crear_paquete(DENIED, buffer_sin_uso); 
    enviar_paquete(paquete, fd_kernel);             
    destruir_paquete(paquete);           
}

void enviar_confirmacion(int fd_kernel) {
    t_buffer* buffer_sin_uso = crear_buffer();
    cargar_int_a_buffer(buffer_sin_uso, 1);
    t_paquete* paquete = crear_paquete(OK, buffer_sin_uso);  
    enviar_paquete(paquete, fd_kernel);            
    destruir_paquete(paquete);                     
}

void recibir_datos_finalizar_proceso(int fd_kernel, int* pid) {
    t_buffer* buffer = recibir_buffer(fd_kernel);  
    *pid = extraer_int_del_buffer(buffer);         
    destruir_buffer(buffer);                       
}

void recibir_datos_finalizar_hilo(int fd_kernel, int* pid, int* tid) {
    t_buffer* buffer = recibir_buffer(fd_kernel); 
    *pid = extraer_int_del_buffer(buffer); 
    *tid = extraer_int_del_buffer(buffer);         
    destruir_buffer(buffer);                       
}

char* armar_nombre(int pid, int tid){
    char* tiempo = temporal_get_string_time("%H:%M:%S:%MS");
    char* key = malloc(strlen(tiempo)+10);
    sprintf(key, "%d-%d-", pid, tid);
    strcat(key,tiempo);
    strcat(key,".dmp");

    return key;
}
