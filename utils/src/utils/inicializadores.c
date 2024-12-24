#include "inicializadores.h"

t_log* iniciar_logger(char* nombre_archivo, char* modulo) {
    t_log* nuevo_logger;
    nuevo_logger = log_create(nombre_archivo, modulo, 1, LOG_LEVEL_TRACE);

    log_debug(nuevo_logger, "Iniciado el log de %s", modulo);

    if (nuevo_logger == NULL) {
        log_error(nuevo_logger, "No se pudo crear el log de %s", modulo);
        exit (EXIT_FAILURE);
    }

    return nuevo_logger;
}

t_config* iniciar_config(char* nombre_archivo) {
	t_config* nuevo_config = config_create(nombre_archivo);

	if(nuevo_config == NULL) {
		perror("No se pudo leer el archivo de configuracion");
		exit(EXIT_FAILURE);
	}

	return nuevo_config;
}