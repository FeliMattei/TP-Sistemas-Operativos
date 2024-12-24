#include <kernel.h>

int main(int argc, char* argv[]) {
    kernel_logger = iniciar_logger("kernel.log", "Kernel");

    if (argc != 3) {
        log_error(kernel_logger, "No se ingresaron 2 argumentos, finalizando...");
        return EXIT_FAILURE;
    }

    t_config* kernel_config = iniciar_config("/home/utnso/tp-2024-2c-Bit-Wizards/kernel/kernel.config");

    IP_MEMORIA = copiar_a_memoria(config_get_string_value(kernel_config, "IP_MEMORIA"));
    PUERTO_MEMORIA = copiar_a_memoria(config_get_string_value(kernel_config, "PUERTO_MEMORIA"));
    IP_CPU = copiar_a_memoria(config_get_string_value(kernel_config, "IP_CPU"));
    PUERTO_CPU_DISPATCH = copiar_a_memoria(config_get_string_value(kernel_config, "PUERTO_CPU_DISPATCH"));
    PUERTO_CPU_INTERRUPT = copiar_a_memoria(config_get_string_value(kernel_config, "PUERTO_CPU_INTERRUPT"));
    char* algoritmo = copiar_a_memoria(config_get_string_value(kernel_config, "ALGORITMO_PLANIFICACION"));
    ALGORITMO_PLANIFICACION = convertir_string_a_algoritmo(algoritmo);
    QUANTUM = config_get_int_value(kernel_config, "QUANTUM");

    conexion_cpu_dispatch = crear_conexion(IP_CPU, PUERTO_CPU_DISPATCH);
    conexion_cpu_interrupt = crear_conexion(IP_CPU, PUERTO_CPU_INTERRUPT);

    config_destroy(kernel_config);

    inicializar_listas();
    inicializar_semaforos();
    inicializar_hilos();

    // La idea es: ./bin/kernel [archivo_pseudocodigo] [tamanio_proceso] [prioridad]
    t_config_mem_buffer* data_buffer = malloc(sizeof(t_config_mem_buffer));
    data_buffer->path = string_new();
    string_append(&data_buffer->path, "/home/utnso/tp-2024-2c-Bit-Wizards/memoria/scripts/");
    string_append(&data_buffer->path, argv[1]);
    data_buffer->tamanio_proceso = atoi(argv[2]);
    data_buffer->prioridad = 0;

    iniciar_proceso(data_buffer);

    sem_wait(&fin_kernel);
}

void inicializar_listas(){
    COLA_NEW = list_create();
    COLA_READY = list_create();
    COLA_EXEC = list_create();
    COLA_BLOCKED = list_create();
    COLA_EXIT = list_create();
    LISTA_PCBS = list_create();
    LISTA_TCBS = list_create();
    CREACIONES = list_create();
}

void inicializar_hilos(){
    pthread_t hilo_planificador_largo_plazo, hilo_planificador_corto_plazo, hilo_kernel_dispatch;

    pthread_create(&hilo_planificador_largo_plazo, NULL, (void *)planificar_largo_plazo, NULL);
	pthread_detach(hilo_planificador_largo_plazo);

	pthread_create(&hilo_planificador_corto_plazo, NULL, (void *)planificar_corto_plazo, NULL);
	pthread_detach(hilo_planificador_corto_plazo);

    int* socket_cliente_cpu_dispatch_ptr = malloc(sizeof(int));
    *socket_cliente_cpu_dispatch_ptr = conexion_cpu_dispatch;
    pthread_create(&hilo_kernel_dispatch, NULL, atender_cpu_dispatch, socket_cliente_cpu_dispatch_ptr);
    pthread_detach(hilo_kernel_dispatch);
}

void inicializar_semaforos(){
    sem_init(&sem_new, 0, 1);
    sem_init(&sem_ready, 0, 1);
    sem_init(&sem_exec, 0, 1);
    sem_init(&sem_blocked, 0, 1);
    sem_init(&sem_exit, 0, 1);
    sem_init(&hay_pcb_cola_new, 0, 0);
    sem_init(&hay_tcb_cola_ready, 0, 0);
	sem_init(&sem_lista_pcbs, 0, 1);
    sem_init(&sem_lista_tcbs, 0, 1);
    sem_init(&proceso_en_chequeo_memoria, 0, 0);
    sem_init(&puede_entrar_a_exec, 0 , 1);
    sem_init(&agregar_hilo, 0, 1);
    sem_init(&fin_kernel, 0, 0);
    sem_init(&hay_pcb_encolado_new, 0, 1);
    sem_init(&sem_creacion, 0, 1);
}