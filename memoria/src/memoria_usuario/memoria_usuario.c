#include "memoria_usuario.h"

void inicializar_memoria_usuario() {

    sem_init(&sem_mutex_particiones, 0, 1);
    sem_init(&sem_mutex_contextos_procesos, 0, 1);
    sem_init(&sem_mutex_contextos_hilos, 0, 1);
    sem_init(&sem_mutex_instrucciones, 0, 1);
    sem_init(&fin_memoria, 0, 0);
    sem_init(&sem_mutex_memoria_usuario, 0, 1);

    memoria_usuario = malloc(memoria_config->tam_memoria);

    if (memoria_usuario == NULL) {
        printf("Error al asignar memoria de usuario\n");
        exit(1);
    }

    particiones = list_create();

    if (strcmp(memoria_config->esquema, "FIJAS") == 0) {
        inicializar_particiones_fijas();
    } else if (strcmp(memoria_config->esquema, "DINAMICAS") == 0) {
        inicializar_particiones_dinamicas(memoria_config->tam_memoria);
    }
}

void inicializar_particiones_dinamicas(int tam_memoria) {
    t_particion* particion_inicial = malloc(sizeof(t_particion));
    particion_inicial->base = 0;
    particion_inicial->limite = tam_memoria;
    particion_inicial->libre = true;
    particion_inicial->pid = -1;

    list_add(particiones, particion_inicial);
} 

void inicializar_particiones_fijas() { 

    int offset = 0;

    for (int i = 0; i < list_size(memoria_config->particiones); i++) {
        t_particion* particion = malloc(sizeof(t_particion));
        particion->pid = -1;
        particion->base = offset;
        particion->limite = offset + list_get(memoria_config->particiones, i);
        particion->libre = true;
        offset += list_get(memoria_config->particiones, i);

        list_add(particiones, particion);
    }
}

bool asignar_memoria_particiones_fijas_first_fit(int pid, int tamanio_proceso) {
    // Recorremos las particiones para encontrar la primera que tenga suficiente espacio
    for (int i = 0; i < list_size(particiones); i++) {
        t_particion *particion = list_get(particiones, i);

        // Verificamos que la partición esté libre y tenga suficiente tamaño
        if (particion->libre && (particion->limite - particion->base) >= tamanio_proceso) {
            // Asignamos el PID y marcamos la partición como ocupada
            particion->pid = pid;
            particion->libre = false;

            // Registramos el evento en los logs
            log_info(memoria_log, "Partición fija asignada (First Fit) - PID: %d, Base: %d, Tamaño: %d", 
                     pid, particion->base, tamanio_proceso);
            return true; // Si asignamos la memoria correctamente, terminamos la función
        }
    }
    
    // Si no se encontró ninguna partición adecuada
    return false;
}


bool asignar_memoria_particiones_fijas_best_fit(int pid, int tamanio_proceso) {
    t_particion* mejor_particion = NULL;
    int mejor_tamano = INT_MAX;

    // Buscar la mejor partición para el proceso (Best Fit)
    for (int i = 0; i < list_size(particiones); i++) {
        t_particion* particion = list_get(particiones, i);
        int tamano_disponible = particion->limite - particion->base;

        // Aseguramos que la partición sea libre y tenga suficiente espacio
        if (particion->libre && tamano_disponible >= tamanio_proceso && tamano_disponible < mejor_tamano) {
            mejor_tamano = tamano_disponible;
            mejor_particion = particion;
        }
    }

    // Si encontramos una partición adecuada
    if (mejor_particion != NULL) {
        // Asignamos el PID al proceso y marcamos la partición como ocupada
        mejor_particion->pid = pid;
        mejor_particion->libre = false;

        // Registra el cambio
        log_info(memoria_log, "Partición fija asignada (Best Fit) - PID: %d, Base: %d, Tamaño: %d", 
                 pid, mejor_particion->base, tamanio_proceso);

        // Aquí no se hace ninguna modificación en el array de particiones, solo se marca la partición como ocupada
        return true;
    }

    // Si no encontramos una partición adecuada
    return false;
}




bool asignar_memoria_particiones_fijas_worst_fit(int pid, int tamanio_proceso) {
    t_particion* peor_particion = NULL;
    int peor_tamano = -1;

    // Buscar la peor partición libre
    for (int i = 0; i < list_size(particiones); i++) {
        t_particion* particion = list_get(particiones, i);
        int tamano_disponible = particion->limite - particion->base;
        
        // Verificamos que la partición sea libre y tenga suficiente espacio
        if (particion->libre && tamano_disponible >= tamanio_proceso && tamano_disponible > peor_tamano) {
            peor_tamano = tamano_disponible;
            peor_particion = particion;
        }
    }

    // Si encontramos una partición adecuada
    if (peor_particion != NULL) {
        // Asignar el PID al proceso y marcar la partición como ocupada
        peor_particion->pid = pid;
        peor_particion->libre = false;
        log_info(memoria_log, "Partición fija asignada (Worst Fit) - PID: %d, Base: %d, Tamaño: %d", pid, peor_particion->base, tamanio_proceso);

        // Fragmentación interna: No cambiamos el tamaño de la partición, solo marcamos el espacio ocupado
        return true;
    }
    
    // Si no se encuentra una partición adecuada
    return false;
}


bool asignar_memoria_particiones_first_fit(int pid, int tamanio_proceso) {
    for (int i = 0; i < list_size(particiones); i++) {
        t_particion *particion = list_get(particiones, i);

        if (particion->libre && (particion->limite - particion->base) >= tamanio_proceso) {
            int base_original = particion->base;
            int limite_original = particion->limite;

            t_particion* particion_ocupada = malloc(sizeof(t_particion));
            particion_ocupada->pid = pid;
            particion_ocupada->base = base_original;
            particion_ocupada->limite = base_original + tamanio_proceso;
            particion_ocupada->libre = false;
            log_info(memoria_log, "Partición asignada (First Fit) - PID: %d, Base: %d, Tamaño: %d", pid, base_original, tamanio_proceso);

            particion->base = particion_ocupada->limite;
            if (particion->base == limite_original) {
                list_remove_and_destroy_element(particiones, i, free);
            } else {
                log_info(memoria_log, "Nueva partición libre creada - Base: %d, Limite: %d", particion->base, particion->limite);
            }

            list_add_in_index(particiones, i, particion_ocupada);

            return true;
        }
    }
    return false;
}

bool asignar_memoria_particiones_best_fit(int pid, int tamanio_proceso) {
    t_particion* mejor_particion = NULL;
    int mejor_tamano = INT_MAX;
    int mejor_index = -1;

    for (int i = 0; i < list_size(particiones); i++) {
        t_particion* particion = list_get(particiones, i);
        int tamano_disponible = particion->limite - particion->base;

        if (particion->libre && tamano_disponible >= tamanio_proceso && tamano_disponible < mejor_tamano) {
            mejor_tamano = tamano_disponible;
            mejor_particion = particion;
            mejor_index = i;
        }
    }

    if (mejor_particion != NULL) {
        int base_original = mejor_particion->base;
        int limite_original = mejor_particion->limite;

        t_particion* particion_ocupada = malloc(sizeof(t_particion));
        particion_ocupada->pid = pid;
        particion_ocupada->base = base_original;
        particion_ocupada->limite = base_original + tamanio_proceso;
        particion_ocupada->libre = false;

        log_info(memoria_log, "Partición asignada (Best Fit) - PID: %d, Base: %d, Tamaño: %d", pid, base_original, tamanio_proceso);

        mejor_particion->base = particion_ocupada->limite; // Nuevo inicio de la partición libre

        if (mejor_particion->base == limite_original) {
            list_remove_and_destroy_element(particiones, mejor_index, free);
        } else {
            log_info(memoria_log, "Nueva partición libre creada (Best Fit) - Base: %d, Tamaño: %d",
                     mejor_particion->base, mejor_particion->limite - mejor_particion->base);
        }

        list_add_in_index(particiones, mejor_index, particion_ocupada);

        return true;
    }

    return false;
}

bool asignar_memoria_particiones_worst_fit(int pid, int tamanio_proceso) {
    t_particion* peor_particion = NULL;
    int peor_tamano = -1;
    int peor_index = -1;
    for (int i = 0; i < list_size(particiones); i++) {
        t_particion* particion = list_get(particiones, i);
        int tamano_disponible = particion->limite - particion->base;
        if (particion->libre && tamano_disponible >= tamanio_proceso && tamano_disponible > peor_tamano) {
            peor_tamano = tamano_disponible;
            peor_particion = particion;
            peor_index = i;
        }
    }
    if (peor_particion != NULL) {
        peor_particion->libre = false;
        int base_original = peor_particion->base;
        peor_particion->limite = base_original + tamanio_proceso;
        log_info(memoria_log, "Partición asignada (Worst Fit) - PID: %d, Base: %d, Tamaño: %d", pid, base_original, tamanio_proceso);
        if (peor_tamano > tamanio_proceso) {
            t_particion* nueva_particion = malloc(sizeof(t_particion));
            nueva_particion->base = base_original + tamanio_proceso;
            nueva_particion->limite = base_original + peor_tamano;
            nueva_particion->libre = true;
            list_add_in_index(particiones, peor_index + 1, nueva_particion);
            log_info(memoria_log, "Nueva partición libre creada (Worst Fit) - Base: %d, Tamaño: %d", nueva_particion->base, nueva_particion->limite - nueva_particion->base);
        }
        return true;
    }
    return false;
}

bool dir_fisica_valida(int direccion_fisica){

 for (int i = 0; i < list_size(particiones); i++) {
    t_particion* particion = list_get(particiones, i);
        
    if (direccion_fisica >= particion->base && direccion_fisica < particion->limite && direccion_fisica >= 0) {
        return true;
    }

 }

 return false;
}