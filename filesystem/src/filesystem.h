#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <variables_globales.h>
#include <inicializar_filesystem/inicializar_filesystem.h>

t_log* filesystem_log;
t_filesystem_config* filesystem_config;

void* atender_conexion(void* socket_desc);
int reservar_bitmap(int tam,char* file_name);
void crear_metadata(char* nombre, int tam, int puntero);
void escribir_bloque(void* info,int tam,int puntero);
void escribir_bloques(void* info,int tam,char* file_name,int bloque_puntero);
int siguiente_bloque_libre();
int espacio_libre();
char* obtener_file_name(t_buffer* buffer);
int obtener_tamanio(t_buffer* buffer);
void* obtener_info(t_buffer* buffer);
void enviar_denied(int fd_kernel);
void enviar_confirmacion(int fd_kernel);

#endif