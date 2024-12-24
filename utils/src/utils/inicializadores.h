#ifndef INICIALIZADORES_H
#define INICIALIZADORES_H

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>

t_log* iniciar_logger(char* nombre_archivo, char* modulo);
t_config* iniciar_config(char* nombre_archivo);

#endif