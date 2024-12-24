#include "inicializar_memoria.h"

void inicializar_memoria(){
    crear_logger();
    iniciar_config();
}

void crear_logger() {
	memoria_log = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_TRACE);
	
	if (memoria_log == NULL) { 
		printf("No se pudo crear el logger");
		exit(1);
	}

}

void iniciar_config() {
	t_config* nuevo_config = config_create("/home/utnso/tp-2024-2c-Bit-Wizards/memoria/config/memoria.config");

	if (nuevo_config == NULL) { 
		log_error(memoria_log, "No se pudo leer la configuracion");
		exit(1);
	}

	memoria_config = malloc(sizeof(t_memoria_config));    

	memoria_config->puerto_escucha = copiar_a_memoria(config_get_string_value(nuevo_config, "PUERTO_ESCUCHA"));
	memoria_config->ip_filesystem= copiar_a_memoria(config_get_string_value(nuevo_config, "IP_FILESYSTEM"));
	memoria_config->puerto_filesystem= copiar_a_memoria(config_get_string_value(nuevo_config, "PUERTO_FILESYSTEM"));
	memoria_config->tam_memoria= config_get_int_value(nuevo_config, "TAM_MEMORIA");
	memoria_config->path_instrucciones= copiar_a_memoria(config_get_string_value(nuevo_config, "PATH_INSTRUCCIONES"));
	memoria_config->retardo_respuesta= config_get_int_value(nuevo_config, "RETARDO_RESPUESTA");
	memoria_config->esquema = copiar_a_memoria(config_get_string_value(nuevo_config, "ESQUEMA"));
    memoria_config->algoritmo_busqueda = copiar_a_memoria(config_get_string_value(nuevo_config, "ALGORITMO_BUSQUEDA"));

    char* particiones_str = copiar_a_memoria(config_get_string_value(nuevo_config, "PARTICIONES"));
    parsear_particiones(particiones_str);
    free(particiones_str);

    memoria_config->log_level = copiar_a_memoria(config_get_string_value(nuevo_config, "LOG_LEVEL"));

    algoritmo_memoria = convertir_string_a_algoritmo_memoria(memoria_config->esquema);
    criterio_memoria = convertir_string_a_algoritmo_memoria(memoria_config->algoritmo_busqueda);; 

	config_destroy(nuevo_config);
}


void parsear_particiones(char* particiones_str) {
    // Eliminar corchetes y espacios adicionales
    char* inicio = strchr(particiones_str, '[');
    char* fin = strchr(particiones_str, ']');
    
    if (inicio != NULL) {
        inicio++;  // Avanzar para eliminar el corchete de apertura
    }
    
    if (fin != NULL) {
        *fin = '\0'; // Terminar la cadena justo antes del corchete de cierre
    }

    // Eliminar espacios adicionales al inicio y al final
    while (isspace(*inicio)) inicio++;
    char* end = inicio + strlen(inicio) - 1;
    while (isspace(*end)) end--;
    *(end + 1) = '\0';

    // Dividir la cadena de particiones
    char* token;
    char* delimitador = ",";
    int i = 0;
    token = strtok(inicio, delimitador);

    memoria_config->particiones = list_create();

    while (token != NULL) {
        list_add(memoria_config->particiones, atoi(token));
        
        token = strtok(NULL, delimitador);
    }
}










