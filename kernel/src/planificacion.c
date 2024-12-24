#include <planificacion.h>

void planificar_largo_plazo(){
    while(1) {
        sem_wait(&hay_pcb_cola_new);
        t_pcb* pcb = sacar_siguiente_de_new();
        sem_post(&hay_pcb_encolado_new);    
    }
}


void planificar_corto_plazo(){
    while (1) {
	    sem_wait(&puede_entrar_a_exec);
	             sem_wait(&hay_tcb_cola_ready);
	    		
	   


		t_tcb* tcb = sacar_siguiente_de_ready();
		agregar_a_exec(tcb);

        mandar_contexto_a_cpu(tcb);
    }
}

void mandar_contexto_a_cpu(t_tcb* tcb){
	t_buffer* buffer_cpu = crear_buffer();    
    cargar_int_a_buffer(buffer_cpu, tcb->pid);
    cargar_int_a_buffer(buffer_cpu, tcb->tid);
	log_debug(kernel_logger, "Envio TID %d y PID %d a CPU", tcb->tid, tcb->pid);
	t_paquete* paquete_cpu = crear_paquete(CONTEXTO_TID_PID, buffer_cpu);
    enviar_paquete(paquete_cpu, conexion_cpu_dispatch);	
	destruir_paquete(paquete_cpu);
}

void* tcb_min_prioridad(void* a, void* b){
    t_tcb* tcb_a = (t_tcb*) a;
    t_tcb* tcb_b = (t_tcb*) b;
    return (tcb_a->prioridad <= tcb_b->prioridad) ? tcb_a : tcb_b;
}



t_tcb* sacar_siguiente_de_ready(){
	t_tcb* tcb;
	switch(ALGORITMO_PLANIFICACION){
        case FIFO:
            sem_wait(&sem_ready);
                tcb = list_remove(COLA_READY, 0);
            sem_post(&sem_ready);
            return tcb;
        break;
        
        case PRIORIDADES: 
            sem_wait(&sem_ready); // TODO: Chequear si desempata por fifo: P0: T0 0 - T1 1 | P1: T0: 0 - T1: 1
                tcb = (t_tcb*) list_get_minimum(COLA_READY, tcb_min_prioridad);
                list_remove_element(COLA_READY, tcb);
            sem_post(&sem_ready);		
            return tcb;
        break;

        case CMN:
            sem_wait(&sem_ready);
                t_tcb* tcb_aux = (t_tcb*) list_get_minimum(COLA_READY, tcb_min_prioridad);
                int valor = tcb_aux->prioridad;
                // 0 0 0 0
                //  if cola > 0 : remove [0]
                bool filtrar_por_prioridad(void* elemento){
                    t_tcb* tcb_prio = (t_tcb*) elemento; 
                    return tcb_prio->prioridad == valor;
                }

                t_list* lista_filtrada = list_filter(COLA_READY, filtrar_por_prioridad);
                tcb = list_remove(lista_filtrada, 0);
                list_destroy(lista_filtrada);
                list_remove_element(COLA_READY, tcb);
            sem_post(&sem_ready);
            return tcb;
        break; 

        default:
            return NULL;
        break;
    }
}

void agregar_a_new(t_pcb* nuevo_pcb, t_config_mem_buffer* data_buffer){
    sem_wait(&sem_new);	
        list_add(COLA_NEW, nuevo_pcb);
    sem_post(&sem_new);
    log_debug(kernel_logger, "COLA: NEW - [AGREGADO] - PCB: %d", nuevo_pcb->pid);

    thread_cheq_mem* args_cheq_mem = malloc(sizeof(thread_cheq_mem));
    args_cheq_mem->pcb = nuevo_pcb;
    args_cheq_mem->data_buffer = data_buffer;
    pthread_t thread_chequear_memoria;
    pthread_create(&thread_chequear_memoria, NULL, chequear_memoria, args_cheq_mem);
    pthread_detach(thread_chequear_memoria);
}

void agregar_a_exec(t_tcb* tcb){
    sem_wait(&sem_exec);
		list_add(COLA_EXEC, tcb);
	sem_post(&sem_exec);
    cambiar_estado_tcb(tcb, EXEC);

    if(ALGORITMO_PLANIFICACION == CMN){
        pthread_create(&hilo_quantum, NULL, (void*)esperar_rr, tcb);
        pthread_detach(hilo_quantum);
    }
}

void agregar_a_blocked(t_tcb* tcb, op_code operacion){
    sem_wait(&sem_blocked);
		list_add(COLA_BLOCKED, tcb);
	sem_post(&sem_blocked);
    cambiar_estado_tcb(tcb, BLOCK);

    switch(operacion) {
        case THREAD_JOIN:
            log_info(kernel_logger, "## (%d:%d) - Bloqueado por <PTHREAD_JOIN>", tcb->pid, tcb->tid);
        break;

        case MUTEX_LOCK:
            log_info(kernel_logger, "## (%d:%d) - Bloqueado por <MUTEX>", tcb->pid, tcb->tid);
        break;

        case DUMP_MEMORY:
            log_info(kernel_logger, "## (%d:%d) - Bloqueado por <DUMP_MEMORY>", tcb->pid, tcb->tid);
        break;

        case IO:
            log_info(kernel_logger, "## (%d:%d) - Bloqueado por <IO>", tcb->pid, tcb->tid);
        break;
    }
}

void remover_de_exec(t_pcb* pcb, op_code operacion){
    t_tcb* tcb_en_exec;
    sem_wait(&sem_exec);
        if (list_size(COLA_EXEC) > 0) {
            tcb_en_exec = list_get(COLA_EXEC, 0);
            if (tcb_en_exec->pid == pcb->pid) {
                tcb_en_exec = list_remove(COLA_EXEC, 0);
            }
        }
    sem_post(&sem_exec);
    

    switch (operacion) {
        case PROCESS_EXIT_SUCCESS:
            sem_wait(&sem_exit);
                list_add(COLA_EXIT, pcb);
            sem_post(&sem_exit);

            sem_wait(&sem_lista_tcbs);
                for (int i = 0; i < list_size(LISTA_TCBS); i++){
                    t_tcb* tcb = list_get(LISTA_TCBS, i);
                    if (tcb->pid == pcb->pid) {
                        if (tcb->estado == READY) {
                            sem_wait(&sem_ready);
                                list_remove_element(COLA_READY, tcb);
                            sem_post(&sem_ready);
                            sem_wait(&hay_tcb_cola_ready);
                        }
                        if (tcb->estado == BLOCK) {
                            sem_wait(&sem_blocked);
                                list_remove_element(COLA_BLOCKED, tcb);
                            sem_post(&sem_blocked);
                        }
                        if (tcb->estado != EXIT) {
                            sem_wait(&sem_exit);
                                list_add(COLA_EXIT, tcb);
                            sem_post(&sem_exit);
                            cambiar_estado_tcb(tcb, EXIT);
                        }
                    }
                }
            sem_post(&sem_lista_tcbs);
        break;

        case THREAD_EXIT_SUCCESS:
            sem_wait(&sem_exit);
                list_add(COLA_EXIT, tcb_en_exec);
            sem_post(&sem_exit);
            cambiar_estado_tcb(tcb_en_exec, EXIT);
        break;
    }
}

void* chequear_memoria(void* args){
    sem_wait(&hay_pcb_encolado_new);
    thread_cheq_mem* parametros = (thread_cheq_mem*)args;

    t_pcb* pcb = parametros->pcb;
    t_config_mem_buffer* data_buffer = parametros->data_buffer;

    while (1) {
        op_code respuesta;
        int conexion_memoria;
        t_buffer* buffer = crear_buffer();
        cargar_int_a_buffer(buffer, data_buffer->pid);
        cargar_int_a_buffer(buffer, data_buffer->tamanio_proceso); // TODO: Falta leerlo desde memoria cuando tengan OK
        // Es el archivo pseudo_kernel.config por ej que tiene las instrucciones de CPU
        // Hay que almacenar las funciones como hicimos en el TP del cuatri pasado.

        conexion_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

        t_paquete* paquete_memoria = crear_paquete(INICIALIZAR_PROCESO, buffer);
        enviar_paquete(paquete_memoria, conexion_memoria);
        destruir_paquete(paquete_memoria);

        respuesta = recibir_operacion(conexion_memoria);
        destruir_buffer(recibir_buffer(conexion_memoria));
        close(conexion_memoria);

        if(respuesta == OK){

            t_buffer* buffer_hilo = crear_buffer();
            cargar_int_a_buffer(buffer_hilo, data_buffer->pid);
            cargar_int_a_buffer(buffer_hilo, list_size(pcb->hilos));
            cargar_string_a_buffer(buffer_hilo, data_buffer->path);

            int conexion_memoria_2 = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
            t_paquete* paquete_path_memoria = crear_paquete(INICIALIZAR_HILO, buffer_hilo);
            enviar_paquete(paquete_path_memoria, conexion_memoria_2);
            destruir_paquete(paquete_path_memoria);

            respuesta = recibir_operacion(conexion_memoria_2);
            destruir_buffer(recibir_buffer(conexion_memoria_2));
            close(conexion_memoria_2);
crear_tcb(data_buffer->prioridad, pcb);
            sem_post(&hay_pcb_cola_new);
            free(data_buffer->path);
            free(data_buffer);
            free(parametros);
            break;
        } else if (respuesta == DENIED) {
            sem_wait(&proceso_en_chequeo_memoria);
        }
    }
}


void cambiar_estado_tcb(t_tcb* tcb, estado estado_nuevo){
    if (estado_nuevo != tcb->estado) {
        char* estado_anterior_string = string_new();
	    char* estado_nuevo_string = string_new();

	    estado estado_anterior = tcb->estado;
    
        tcb->estado = estado_nuevo;

        string_append(&estado_anterior_string, estado_a_string(estado_anterior));
        string_append(&estado_nuevo_string, estado_a_string(estado_nuevo));

        log_debug(kernel_logger, "PID: %d - TID: %d - Estado: (%s -> %s)", tcb->pid,tcb->tid, estado_anterior_string, estado_nuevo_string);
        	
        free(estado_anterior_string);
	    free(estado_nuevo_string);
    }
}

void agregar_tcb_a_ready(t_tcb* tcb){
    cambiar_estado_tcb(tcb, READY);
    sem_wait(&sem_ready);
        list_add(COLA_READY, tcb);
    sem_post(&sem_ready);
}

t_pcb* sacar_siguiente_de_new(){
	sem_wait(&sem_new);
	    t_pcb* pcb = list_remove(COLA_NEW, 0);
	sem_post(&sem_new);
	return pcb;
}

t_pcb* crear_pcb(int pid){
	t_pcb* nuevo_pcb = malloc(sizeof(t_pcb));

    nuevo_pcb->pid = pid;

	//nuevo_pcb->tabla_archivos = list_create(); //Comento porque no se para que sirve
	nuevo_pcb->hilos = list_create();
    pthread_mutex_init(&nuevo_pcb->sem_hilos, NULL); 
    nuevo_pcb->mutex = list_create();
    pthread_mutex_init(&nuevo_pcb->sem_mutex, NULL); 

	// agrego el nuevo proceso a la lista de pcbs
	sem_wait(&sem_lista_pcbs);
	    list_add(LISTA_PCBS, nuevo_pcb);
	sem_post(&sem_lista_pcbs);
	
    log_info(kernel_logger, "## (%d:0) Se crea el proceso - Estado: NEW", nuevo_pcb->pid);

    return nuevo_pcb;
}

void iniciar_proceso(t_config_mem_buffer* data_buffer){   
    int pid = asignar_pid_proceso();
    t_pcb* pcb = crear_pcb(pid);

    data_buffer->pid = pid;

    agregar_a_new(pcb, data_buffer);
}

void iniciar_hilo(char* path, int prioridad, int pid){
    sem_wait(&sem_lista_pcbs);
        t_pcb* pcb = buscar_pcb_por_pid(pid);
    sem_post(&sem_lista_pcbs);
  
    t_buffer* buffer_hilo = crear_buffer();
    cargar_int_a_buffer(buffer_hilo, pid);
    cargar_int_a_buffer(buffer_hilo, list_size(pcb->hilos));
    cargar_string_a_buffer(buffer_hilo, path);

    int conexion_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    t_paquete* paquete = crear_paquete(INICIALIZAR_HILO, buffer_hilo);
    enviar_paquete(paquete, conexion_memoria);
    destruir_paquete(paquete);

    free(path);

    op_code respuesta = recibir_operacion(conexion_memoria);
    destruir_buffer(recibir_buffer(conexion_memoria));
    close(conexion_memoria);

	  crear_tcb(prioridad, pcb);

}

int asignar_pid_proceso(){
    int valor_pid;

    pthread_mutex_lock(&MUTEX_PID);
        valor_pid = IDENTIFICADOR_PID;
        IDENTIFICADOR_PID++;
    pthread_mutex_unlock(&MUTEX_PID);
    return valor_pid;
}

int asignar_tid_proceso(t_pcb* pcb){
    return list_size(pcb->hilos);
}



void crear_tcb(int prioridad, t_pcb* pcb){
    t_tcb* nuevo_tcb = malloc(sizeof(t_tcb));

    int tid = asignar_tid_proceso(pcb);
    
    nuevo_tcb->pid = pcb->pid;
    nuevo_tcb->estado = READY;
    nuevo_tcb->tid = tid;
    nuevo_tcb->prioridad = prioridad;
    nuevo_tcb->blocked_by = -1;

    sem_wait(&agregar_hilo);
        list_add(pcb->hilos, tid);
    sem_post(&agregar_hilo);
    
    sem_wait(&sem_lista_tcbs);
	    list_add(LISTA_TCBS, nuevo_tcb);
	sem_post(&sem_lista_tcbs);
    log_info(kernel_logger, "## (%d:%d) Se crea el Hilo - Estado: READY", pcb->pid, nuevo_tcb->tid);

    sem_wait(&sem_ready);
        list_add(COLA_READY, nuevo_tcb);
    sem_post(&sem_ready);
    sem_post(&hay_tcb_cola_ready);
}

void esperar_rr(t_tcb* tcb){

    usleep(QUANTUM * 1000);

    log_debug(kernel_logger, "(%d:%d) - Fin de Quantum", tcb->pid, tcb->tid);

    t_buffer* buffer_interrupcion = crear_buffer();
    cargar_tcb_a_buffer(buffer_interrupcion, tcb);
    t_paquete *paquete = crear_paquete(FIN_QUANTUM, buffer_interrupcion);
    enviar_paquete(paquete, conexion_cpu_interrupt);
    destruir_paquete(paquete);

    if(ALGORITMO_PLANIFICACION == CMN){ pthread_exit(0); }
    return 0;
}

void inicializar_hilo(){
    pthread_t hilo_proceso;

    pthread_create(&hilo_proceso, NULL, (void *)ejecutar_hilo, NULL);
	pthread_detach(hilo_proceso);
}

void ejecutar_hilo(){
    //ejecutar o configurar tcb, ni idea
}


void atender_cpu_dispatch(void* socket_cliente_ptr) {
    int cliente_kd = *(int*)socket_cliente_ptr;
    //free(socket_cliente_ptr);    	
    while (1) {
        op_code operacion = recibir_operacion(cliente_kd);
        t_buffer* buffer = recibir_buffer(cliente_kd);	
        int pid;
        int tid;
        t_tcb* tcb;
        int conexion_memoria;

        switch(operacion) {
            case HILO_DESALOJADO:
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);  
                
                sem_wait(&sem_lista_tcbs);
                    tcb = buscar_tcb_por_pid(tid, pid);
                sem_post(&sem_lista_tcbs);

                log_info(kernel_logger, "## (%d:%d) - Desalojado por fin de Quantum", tcb->pid, tcb->tid);


                sem_wait(&sem_exec);
                    list_remove_element(COLA_EXEC,tcb);
                sem_post(&sem_exec);

                agregar_tcb_a_ready(tcb);
                sem_post(&hay_tcb_cola_ready);
                sem_post(&puede_entrar_a_exec);
            break;

            case PROCESS_CREATE:
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: PROCESS_CREATE", pid, tid);
                t_config_mem_buffer* buffer_nuevo_proceso = malloc(sizeof(t_config_mem_buffer));
                buffer_nuevo_proceso->path = string_new();
                string_append(&buffer_nuevo_proceso->path, "/home/utnso/tp-2024-2c-Bit-Wizards/memoria/scripts/");
                string_append(&buffer_nuevo_proceso->path, extraer_string_del_buffer(buffer));
                buffer_nuevo_proceso->tamanio_proceso = atoi(extraer_string_del_buffer(buffer));
                buffer_nuevo_proceso->prioridad = atoi(extraer_string_del_buffer(buffer));
                iniciar_proceso(buffer_nuevo_proceso);
            break;
            case THREAD_CREATE:
                char* path = string_new();
                string_append(&path, "/home/utnso/tp-2024-2c-Bit-Wizards/memoria/scripts/");
                string_append(&path, extraer_string_del_buffer(buffer));
                int prioridad = atoi(extraer_string_del_buffer(buffer));
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: THREAD_CREATE", pid, tid);
                iniciar_hilo(path, prioridad, pid);
            break;
            case PROCESS_EXIT_SUCCESS:
                if(ALGORITMO_PLANIFICACION == CMN){ pthread_cancel(hilo_quantum); }
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: PROCESS_EXIT", pid, tid);
                sem_wait(&sem_lista_pcbs);
                    t_pcb* pcb = buscar_pcb_por_pid(pid);
                sem_post(&sem_lista_pcbs);
                manejar_fin_proceso(pcb);
                sem_post(&puede_entrar_a_exec);
                //destruir_buffer(buffer); TODO: Probar si con esto anda igual, capaz que se pierde info
            break;
            case THREAD_EXIT_SUCCESS:
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: THREAD_EXIT", pid, tid);
                sem_wait(&sem_lista_tcbs);
                    tcb = buscar_tcb_por_pid(tid, pid);
                sem_post(&sem_lista_tcbs);
                manejar_fin_hilo(tcb, 0);
                //destruir_buffer(buffer); TODO: Probar si con esto anda igual, capaz que se pierde info
            break;
            case THREAD_JOIN:
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: THREAD_JOIN", pid, tid);
                int tid_join = atoi(extraer_string_del_buffer(buffer));

                sem_wait(&sem_lista_tcbs);
                    tcb = buscar_tcb_por_pid(tid, pid);
                sem_post(&sem_lista_tcbs);

                sem_wait(&sem_lista_tcbs);
                    t_tcb* tcb_joined = buscar_tcb_por_pid(tid_join, pid);
                sem_post(&sem_lista_tcbs);

                t_buffer* r_buffer_join = crear_buffer();
                t_paquete* p_cpu;

                if(tcb_joined != NULL ){  
                    if(tcb_joined->estado != EXIT){
                        if(ALGORITMO_PLANIFICACION == CMN){ pthread_cancel(hilo_quantum); }

                        cargar_int_a_buffer(r_buffer_join, OK);
                        p_cpu = crear_paquete(OK, r_buffer_join);
                        enviar_paquete(p_cpu, conexion_cpu_dispatch);
                        destruir_paquete(p_cpu);

                        sem_wait(&sem_exec);
                            list_remove_element(COLA_EXEC, tcb);
                        sem_post(&sem_exec);

                        agregar_a_blocked(tcb, THREAD_JOIN);
                        tcb->blocked_by = tcb_joined->tid;

                        sem_post(&puede_entrar_a_exec);
                    } else {
                        cargar_int_a_buffer(r_buffer_join, NO_OK);
                        p_cpu = crear_paquete(NO_OK, r_buffer_join);
                        enviar_paquete(p_cpu, conexion_cpu_dispatch);
                        destruir_paquete(p_cpu);
                    }
                } else {
                    cargar_int_a_buffer(r_buffer_join, NO_OK);
                    p_cpu = crear_paquete(NO_OK, r_buffer_join);
                    enviar_paquete(p_cpu, conexion_cpu_dispatch);
                    destruir_paquete(p_cpu);
                }
                
            break;

            case THREAD_CANCEL:
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                int tid_cancel = atoi(extraer_string_del_buffer(buffer));
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: THREAD_CANCEL", pid, tid);
                sem_wait(&sem_lista_tcbs);
                    tcb = buscar_tcb_por_pid(tid_cancel, pid);
                sem_post(&sem_lista_tcbs);
                if (tcb != NULL) {
                    if (tcb->estado != EXIT) {
                        log_debug(kernel_logger,"[D] Finalizando correctamente el hilo %d del proceso %d",tcb->tid, pid);
                        manejar_fin_hilo(tcb, 1);
                    } else { 
                        log_debug(kernel_logger,"[D] El hilo %d del proceso %d ya se encuentra en estado EXIT",tcb->tid, pid);
                    }
                } else {
                    log_debug(kernel_logger,"[D] El hilo %d del proceso %d no se pudo cancelar porque no existe",tid_cancel, pid);
                }
            break;

            case MUTEX_CREATE:
                char* id_mutex = extraer_string_del_buffer(buffer);
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: MUTEX_CREATE", pid, tid);

                if (existe_mutex(id_mutex, pid)) {
                    log_debug(kernel_logger,"[D] Ya existe un Mutex con el ID: %s", id_mutex);
                } else {
                    t_mutex* nuevo_mutex = malloc(sizeof(t_mutex));

                    sem_wait(&sem_lista_pcbs);
                        t_pcb* pcb = buscar_pcb_por_pid(pid);
                    sem_post(&sem_lista_pcbs);

                    nuevo_mutex->id = id_mutex;

                    pthread_mutex_lock(&pcb->sem_mutex);
                        list_add(pcb->mutex, nuevo_mutex);
                    pthread_mutex_unlock(&pcb->sem_mutex);
                    nuevo_mutex->tid = -1;
                    nuevo_mutex->lista_bloqueados = list_create();
                    pthread_mutex_init(&nuevo_mutex->sem_lista_bloqueados, NULL); 

                    log_debug(kernel_logger, "Se crea un Mutex - ID: %s", id_mutex);
                }
            break;

            case MUTEX_LOCK:
                char* id_mutex_2 = extraer_string_del_buffer(buffer);
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: MUTEX_LOCK", pid, tid);

                t_buffer* response_buffer = crear_buffer();
                t_paquete* paquete_cpu;
                t_mutex* mutex;

                if (existe_mutex(id_mutex_2, pid)) {
                    mutex = buscar_mutex(id_mutex_2, pid);
                    if (esta_tomado_mutex(id_mutex_2, pid)) {
                        if(ALGORITMO_PLANIFICACION == CMN){pthread_cancel(hilo_quantum);}

                        sem_wait(&sem_lista_tcbs);
                            tcb = buscar_tcb_por_pid(tid, pid);
                        sem_post(&sem_lista_tcbs);

                        pthread_mutex_lock(&mutex->sem_lista_bloqueados);
                            list_add(mutex->lista_bloqueados, tcb);
                        pthread_mutex_unlock(&mutex->sem_lista_bloqueados);

                        sem_wait(&sem_exec);
                            list_remove_element(COLA_EXEC, tcb);
                        sem_post(&sem_exec);

                        agregar_a_blocked(tcb, MUTEX_LOCK);

                        cargar_int_a_buffer(response_buffer, MUTEX_BLOQUEADO);
                        paquete_cpu = crear_paquete(MUTEX_BLOQUEADO, response_buffer);
			                enviar_paquete(paquete_cpu, conexion_cpu_dispatch);
                destruir_paquete(paquete_cpu);
			 sem_post(&puede_entrar_a_exec);
                    } else {
                        mutex->tid = tid;
                        cargar_int_a_buffer(response_buffer, MUTEX_ASIGNADO);
                        paquete_cpu = crear_paquete(MUTEX_ASIGNADO, response_buffer);
			                    enviar_paquete(paquete_cpu, conexion_cpu_dispatch);
                destruir_paquete(paquete_cpu);
                    }
                } else {
                    log_debug(kernel_logger,"[D] No existe un Mutex con el ID: %s", id_mutex_2);
                    cargar_int_a_buffer(response_buffer, MUTEX_INEXISTENTE);
                    paquete_cpu = crear_paquete(MUTEX_INEXISTENTE, response_buffer);
			                enviar_paquete(paquete_cpu, conexion_cpu_dispatch);
                destruir_paquete(paquete_cpu);
                }
            break;

            case MUTEX_UNLOCK:
                char* id_mutex_3 = extraer_string_del_buffer(buffer);
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: MUTEX_UNLOCK", pid, tid);

                t_buffer* response_buffer_2 = crear_buffer();
                t_paquete* paquete_cpu_2;
                t_mutex* mutex_2;

                if (existe_mutex(id_mutex_3, pid)) {
                    mutex_2 = buscar_mutex(id_mutex_3, pid);
                    if (mutex_2->tid == tid) {
                        t_tcb* tcb_desbloqueado = NULL;

                        pthread_mutex_lock(&mutex_2->sem_lista_bloqueados);
                            if(list_size(mutex_2->lista_bloqueados) > 0) {
                                tcb_desbloqueado = list_remove(mutex_2->lista_bloqueados, 0);
                                mutex_2->tid = tcb_desbloqueado->tid;

                                sem_wait(&sem_blocked);
                                    list_remove_element(COLA_BLOCKED, tcb_desbloqueado);
                                sem_post(&sem_blocked);

                                agregar_tcb_a_ready(tcb_desbloqueado);
                                sem_post(&hay_tcb_cola_ready);
                            } else {
                                mutex_2->tid = -1;
                            }
                        pthread_mutex_unlock(&mutex_2->sem_lista_bloqueados);

                        cargar_int_a_buffer(response_buffer_2, OK);
                        paquete_cpu = crear_paquete(OK, response_buffer_2);
                    } else {
                        log_debug(kernel_logger,"[D] El (%d:%d) no tiene asignado el Mutex con el ID: %s", pid, tid, id_mutex_3);
                        cargar_int_a_buffer(response_buffer_2, MUTEX_SIN_ASIGNACION);
                        paquete_cpu = crear_paquete(MUTEX_SIN_ASIGNACION, response_buffer_2);
                    }
                } else {
                    log_debug(kernel_logger,"[D] No existe un Mutex con el ID: %s", id_mutex_3);
                    cargar_int_a_buffer(response_buffer_2, MUTEX_INEXISTENTE);
                    paquete_cpu = crear_paquete(MUTEX_INEXISTENTE, response_buffer_2);
                }

                enviar_paquete(paquete_cpu, conexion_cpu_dispatch);
                destruir_paquete(paquete_cpu);
            break;

            case DUMP_MEMORY:
                if(ALGORITMO_PLANIFICACION == CMN){ pthread_cancel(hilo_quantum); }
                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: DUMP_MEMORY", pid, tid);

                sem_wait(&sem_lista_tcbs);
                    tcb = buscar_tcb_por_pid(tid, pid);
                sem_post(&sem_lista_tcbs);

                sem_wait(&sem_exec);
                    list_remove_element(COLA_EXEC,tcb);
                sem_post(&sem_exec);

                agregar_a_blocked(tcb, DUMP_MEMORY);

                sem_post(&puede_entrar_a_exec);

                pthread_t dm;
                pthread_create(&dm, NULL, (void *)esperar_respuesta_dump_memory, tcb);
                pthread_detach(dm);
                
            break;

            case IO:
                if(ALGORITMO_PLANIFICACION == CMN){ pthread_cancel(hilo_quantum); }

                pid = extraer_int_del_buffer(buffer);
                tid = extraer_int_del_buffer(buffer);
                log_info(kernel_logger, "## (%d:%d) - Solicitó syscall: IO", pid, tid);
                char* tiempo = extraer_string_del_buffer(buffer);    

                sem_wait(&sem_lista_tcbs);
                    tcb = buscar_tcb_por_pid(tid, pid);
                sem_post(&sem_lista_tcbs);

                sem_wait(&sem_exec);
                    list_remove_element(COLA_EXEC,tcb);
                sem_post(&sem_exec);

                agregar_a_blocked(tcb, IO);                

                sem_post(&puede_entrar_a_exec);
                
                thread_io* args_io = malloc(sizeof(thread_io));
                args_io->tcb = tcb;
                args_io->tiempo = tiempo;
                pthread_t io;
                pthread_create(&io, NULL, (void*)dormir_hilo, args_io);
                pthread_detach(io);
            break;

            default:
                log_error(kernel_logger, "[D] No se reconoce la operacion");
            break;
        }
        destruir_buffer(buffer);
    }
}

void esperar_respuesta_dump_memory(t_tcb* tcb){
    t_buffer* buffer_dump_mem = crear_buffer();
    cargar_int_a_buffer(buffer_dump_mem, tcb->pid);
    cargar_int_a_buffer(buffer_dump_mem, tcb->tid);

    int conexion_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    t_paquete* paquete_dump_memory = crear_paquete(DUMPY_MEMORY, buffer_dump_mem);
    enviar_paquete(paquete_dump_memory, conexion_memoria);
    destruir_paquete(paquete_dump_memory);

    op_code respuesta_mem = recibir_operacion(conexion_memoria);
    destruir_buffer(recibir_buffer(conexion_memoria));

    if(respuesta_mem == DENIED){
        log_debug(kernel_logger,"[D] Finalizando proceso por Dump Memory %d", tcb->pid);

        t_pcb* pcb;
        sem_wait(&sem_lista_pcbs);
            pcb = buscar_pcb_por_pid(tcb->pid);
        sem_post(&sem_lista_pcbs);

        manejar_fin_proceso(pcb);
    }else{
        sem_wait(&sem_blocked);
            list_remove_element(COLA_BLOCKED, tcb);
        sem_post(&sem_blocked);

        agregar_tcb_a_ready(tcb);
        sem_post(&hay_tcb_cola_ready);
    }
}


void dormir_hilo(void* args){
    thread_io* parametros = (thread_io*)args;
    t_tcb* tcb = parametros->tcb;
    int tiempo_sleep = atoi(parametros->tiempo);

    usleep(tiempo_sleep * 1000);

    sem_wait(&sem_blocked);
        list_remove_element(COLA_BLOCKED,tcb);
    sem_post(&sem_blocked);  
    log_info(kernel_logger, "## (%d:%d) finalizó IO y pasa a READY", tcb->pid, tcb->tid);

    agregar_tcb_a_ready(tcb);
    sem_post(&hay_tcb_cola_ready);
    free(parametros);
}

void manejar_fin_hilo(t_tcb* tcb, int es_otro_tid) {
    int conexion_memoria;
    conexion_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

    if(!es_otro_tid) {
        sem_wait(&sem_exec);
            if (list_size(COLA_EXEC) > 0) {
                list_remove_element(COLA_EXEC, tcb);
            }
	    sem_post(&sem_exec);
        if(ALGORITMO_PLANIFICACION == CMN){ pthread_cancel(hilo_quantum); }
    } else {
        buscar_y_remover_de_cola(tcb); 
    }

    sem_wait(&sem_exit);
        list_add(COLA_EXIT, tcb);
    sem_post(&sem_exit);
    cambiar_estado_tcb(tcb, EXIT);

    liberar_mutex(tcb->pid, tcb->tid);

    t_buffer* buffer = crear_buffer();
    cargar_int_a_buffer(buffer, tcb->pid);
    cargar_int_a_buffer(buffer, tcb->tid);

    t_paquete* paquete_memoria = crear_paquete(FINALIZAR_HILO, buffer);
    enviar_paquete(paquete_memoria, conexion_memoria);
    destruir_paquete(paquete_memoria);

    op_code respuesta = recibir_operacion(conexion_memoria);
    destruir_buffer(recibir_buffer(conexion_memoria));
    close(conexion_memoria);
    log_info(kernel_logger,"## (%d:%d) Finaliza el hilo", tcb->pid, tcb->tid);
    
    sem_wait(&sem_blocked);
        for (int i = 0; i<= list_size(COLA_BLOCKED); i++) {
            t_tcb* tcb_blocked = buscar_hilo_bloqueado_por(tcb);

            if (tcb_blocked != NULL) {
                list_remove_element(COLA_BLOCKED, tcb_blocked);

                tcb_blocked->blocked_by = -1;

                agregar_tcb_a_ready(tcb_blocked);
                sem_post(&hay_tcb_cola_ready);
            }
        }
    sem_post(&sem_blocked);

    if(!es_otro_tid) {
        sem_post(&puede_entrar_a_exec);
    }
}

void buscar_y_remover_de_cola(t_tcb* tcb){
    if (tcb->estado == READY){
        sem_wait(&sem_ready);
            list_remove_element(COLA_READY,tcb);
        sem_post(&sem_ready);
        sem_wait(&hay_tcb_cola_ready);
    }

    if (tcb->estado == EXEC){
        sem_wait(&sem_exec);
            list_remove_element(COLA_EXEC,tcb);
        sem_post(&sem_exec);
    }

    if (tcb->estado == BLOCK){
        sem_wait(&sem_blocked);
            list_remove_element(COLA_BLOCKED, tcb);
        sem_post(&sem_blocked);
    }
}


void manejar_fin_proceso(t_pcb* pcb) {
    int conexion_memoria;
    conexion_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

    remover_de_exec(pcb, PROCESS_EXIT_SUCCESS);
    list_destroy_and_destroy_elements(pcb->mutex, free);
    list_destroy(pcb->hilos);

    t_buffer* buffer = crear_buffer();
    cargar_int_a_buffer(buffer, pcb->pid);
    t_paquete* paquete_memoria = crear_paquete(FINALIZAR_PROCESO, buffer);
    enviar_paquete(paquete_memoria, conexion_memoria);
    destruir_paquete(paquete_memoria);

    op_code respuesta = recibir_operacion(conexion_memoria);
    destruir_buffer(recibir_buffer(conexion_memoria));
    close(conexion_memoria);
    log_info(kernel_logger,"## Finaliza el proceso %d", pcb->pid);

    sem_post(&proceso_en_chequeo_memoria);
}

t_pcb* buscar_pcb_por_pid(int pid) {
    for(int i = 0; i < list_size(LISTA_PCBS); i++) {
        t_pcb* pcb = list_get(LISTA_PCBS, i);
        if (pcb->pid == pid) {
            return pcb;
        }
    }
}

t_tcb* buscar_tcb_por_pid(int tid, int pid) {
    for(int i = 0; i < list_size(LISTA_TCBS); i++) {
        t_tcb* tcb = list_get(LISTA_TCBS, i);
        if (tcb->pid == pid && tcb->tid == tid) {
            return tcb;
        }
    }
    return NULL;
}

t_tcb* buscar_hilo_bloqueado_por(t_tcb* tcb) {
    for(int i = 0; i < list_size(COLA_BLOCKED); i++) {
        t_tcb* tcb_aux = list_get(COLA_BLOCKED, i);
        if (tcb->pid == tcb_aux->pid && tcb_aux->blocked_by == tcb->tid) {
            return tcb_aux;
        }
    }
    return NULL;
}

bool existe_mutex(char* id, int pid) {
    sem_wait(&sem_lista_pcbs);
        t_pcb* pcb = buscar_pcb_por_pid(pid);
    sem_post(&sem_lista_pcbs);

    pthread_mutex_lock(&pcb->sem_mutex);
    for(int i = 0; i < list_size(pcb->mutex); i++) {
        t_mutex* mutex = list_get(pcb->mutex, i);
        if (strcmp(mutex->id, id) == 0) {
            pthread_mutex_unlock(&pcb->sem_mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&pcb->sem_mutex);
    return false;
}

bool esta_tomado_mutex(char* id, int pid) {
    sem_wait(&sem_lista_pcbs);
        t_pcb* pcb = buscar_pcb_por_pid(pid);
    sem_post(&sem_lista_pcbs);

    pthread_mutex_lock(&pcb->sem_mutex);
    for(int i = 0; i < list_size(pcb->mutex); i++) {
        t_mutex* mutex = list_get(pcb->mutex, i);
        if (strcmp(mutex->id, id) == 0) {
            pthread_mutex_unlock(&pcb->sem_mutex);
            return mutex->tid != -1;
        }
    }
    pthread_mutex_unlock(&pcb->sem_mutex);
    return false;
}

t_mutex* buscar_mutex(char* id, int pid) {
    sem_wait(&sem_lista_pcbs);
        t_pcb* pcb = buscar_pcb_por_pid(pid);
    sem_post(&sem_lista_pcbs);

    pthread_mutex_lock(&pcb->sem_mutex);
    for(int i = 0; i < list_size(pcb->mutex); i++) {
        t_mutex* mutex = list_get(pcb->mutex, i);
        if (strcmp(mutex->id, id) == 0) {
            pthread_mutex_unlock(&pcb->sem_mutex);
            return mutex;
        }
    }
    pthread_mutex_unlock(&pcb->sem_mutex);
    return NULL;
}

void liberar_mutex(int pid, int tid) {
    sem_wait(&sem_lista_pcbs);
        t_pcb* pcb = buscar_pcb_por_pid(pid);
    sem_post(&sem_lista_pcbs);

    pthread_mutex_lock(&pcb->sem_mutex);
    for(int i = 0; i < list_size(pcb->mutex); i++) {
        t_mutex* mutex = list_get(pcb->mutex, i);
        if (mutex->tid == tid) {
            mutex->tid = -1;
            pthread_mutex_lock(&mutex->sem_lista_bloqueados);
                for(int j = 0; j < list_size(mutex->lista_bloqueados); j++) {
                    t_tcb* tcb_desbloqueado = list_remove(mutex->lista_bloqueados, j);
                    sem_wait(&sem_blocked);
                        list_remove_element(COLA_BLOCKED, tcb_desbloqueado);
                    sem_post(&sem_blocked);
                    agregar_tcb_a_ready(tcb_desbloqueado);
                    sem_post(&hay_tcb_cola_ready);
                }
            pthread_mutex_unlock(&mutex->sem_lista_bloqueados);
        }
    }
    pthread_mutex_unlock(&pcb->sem_mutex);
}

bool esta_blocked(int tid, int pid) {
    sem_wait(&sem_blocked);
        for (int i = 0; i < list_size(COLA_BLOCKED); i++) {
            t_tcb* tcb_aux = list_get(COLA_BLOCKED, i);
            if (tcb_aux->tid == tid && tcb_aux->pid == pid) {
                return true;
            }
        }
    sem_post(&sem_blocked);
    return false;
}
