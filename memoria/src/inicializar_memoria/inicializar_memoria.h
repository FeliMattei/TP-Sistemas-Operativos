#ifndef INICIALIZAR_MEMORIA_H_
#define INICIALIZAR_MEMORIA_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/config.h>
#include <utils/shared/shared.h>
#include <variables_globales.h>
#include <ctype.h>
#include <string.h> // Para strlen, strtok, etc.

void inicializar_memoria();
void crear_logger();
void iniciar_config();
void parsear_particiones(char* particiones_str);


#endif