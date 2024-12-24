#include "filesystem.h"

t_bitarray* bitmap;
void* bitmap_memoria;
t_list* punteros;
int block_count;
int block_size;
char* path_bitmap;
char* path_bloques;
char* path_metadata;
sem_t mutex_bitmap;
sem_t mutex_bloques;

int main(int argc, char* argv[]) {


    inicializar_filesystem();

    int filesystem_fs = iniciar_servidor(filesystem_config->puerto_escucha, filesystem_log);

     while (1) {
        int *memoria_socket_copia = malloc(sizeof(int));
        int memoria_socket = esperar_conexion(filesystem_fs);
        *memoria_socket_copia = memoria_socket;

        if (memoria_socket < 0) {
            log_error(filesystem_log, "Error al aceptar conexión");
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, atender_conexion, memoria_socket_copia) < 0) {
            log_error(filesystem_log, "Error al crear el hilo");
            close(memoria_socket);
        } else {
            pthread_detach(thread_id);
        }
    }

    close(filesystem_fs);
    return 0;
}

void* atender_conexion(void* cliente_socket) {
    int conexion = *(int*)cliente_socket;
    free(cliente_socket);

    log_debug(filesystem_log, "Se conecto una instancia de Memoria");
    
    op_code operacion = recibir_operacion(conexion);
    t_buffer* buffer = recibir_buffer(conexion);
    char* file_name = obtener_file_name(buffer);
    int tam = obtener_tamanio(buffer);
    void* info = obtener_info(buffer);
   
    if(operacion == DUMPY_MEMORY){
        punteros = list_create();
        if(espacio_libre()>tam){
            sem_wait(&mutex_bloques);
            sem_wait(&mutex_bitmap);
            log_info(filesystem_log,"## Archivo creado: %s - Tamaño: %d",file_name,tam);
            int bloque_punteros = reservar_bitmap(tam,file_name);
            crear_metadata(file_name,tam,bloque_punteros);
            escribir_bloques(info,tam,file_name,bloque_punteros);
            //list_destroy(punteros);
            sem_post(&mutex_bloques);
            sem_post(&mutex_bitmap);
            log_info(filesystem_log,"## Fin de solicitud - Archivo: %s",file_name);
            enviar_confirmacion(conexion);
        }else{
            log_error(filesystem_log, "Espacio insuficiente");
            enviar_denied(conexion);
        }
    }
    else{
        log_error(filesystem_log, "Se recibio un codigo de operacion incorrecto");
    }
    //free(info);
    close(conexion);
    destruir_buffer(buffer);
    pthread_exit(NULL);
}

int reservar_bitmap(int tam, char* file_name){
    int fd_bitmap = open(path_bitmap,  O_RDWR , S_IRUSR | S_IWUSR);
    int uno = 1;
    int bloque_punteros = siguiente_bloque_libre();
    log_info(filesystem_log,"## Bloque asignado: %d - Archivo: %s - Bloques Libres: %d",siguiente_bloque_libre(),file_name,(espacio_libre()/block_size)-1);
    bitarray_set_bit(bitmap,bloque_punteros);
    lseek(fd_bitmap,bloque_punteros*4,SEEK_SET);
    write(fd_bitmap,&uno,4);
    int cantidad_reservar = ceil(tam/block_size);
    for(int i = 0; i<cantidad_reservar; i++){
        list_add(punteros,siguiente_bloque_libre());
        log_info(filesystem_log,"## Bloque asignado: %d - Archivo: %s - Bloques Libres: %d",siguiente_bloque_libre(),file_name,(espacio_libre()/block_size)-1);
        lseek(fd_bitmap,siguiente_bloque_libre()*4,SEEK_SET);
        bitarray_set_bit(bitmap,siguiente_bloque_libre());
        write(fd_bitmap,&uno,4);
    }
    close(fd_bitmap);
    escribir_bloque(punteros,block_size,bloque_punteros);
    return bloque_punteros;
}

void crear_metadata(char* nombre, int tam, int puntero){
    char* nombre_completo = malloc((strlen(path_metadata)+1)  +  (strlen(nombre)+1) );
    nombre_completo[0] = '\0';
    strcat(nombre_completo,path_metadata);
    strcat(nombre_completo,nombre);
    int metadata = open(nombre_completo,  O_CREAT , S_IRUSR | S_IWUSR);
    metadata = open(nombre_completo, O_RDWR , S_IRUSR | S_IWUSR);
    write(metadata,&tam,sizeof(tam));
    write(metadata,&puntero,sizeof(puntero));
    close(metadata);
}

void escribir_bloque(void* info,int tam,int puntero){
	int bloques = open(path_bloques,  O_RDWR , S_IRUSR | S_IWUSR);
    lseek(bloques,puntero*block_size,SEEK_SET);
    for(int i=0; i<list_size(punteros); i++){
        int index = list_get(punteros,i);
        write(bloques,&index,4);
    }
    close(bloques);
}

void escribir_bloques(void* info,int tam,char* file_name,int bloque_puntero){
	int bloques = open(path_bloques,  O_RDWR , S_IRUSR | S_IWUSR);
    int puntero;
    log_info(filesystem_log,"## Acceso Bloque - Archivo: %s - Tipo Bloque: ÍNDICE - Bloque File System %d",file_name,bloque_puntero);
    usleep(filesystem_config->retardo_acceso_bloque*1000);
    for(int i=0; i<list_size(punteros); i++){
        puntero = list_get(punteros,i);
        lseek(bloques,puntero*block_size,SEEK_SET);
        if(tam>block_size){
            write(bloques,info,block_size);
        }else{
            write(bloques,info,tam);
        }
        log_info(filesystem_log,"## Acceso Bloque - Archivo: %s - Tipo Bloque: DATOS - Bloque File System %d",file_name,puntero);
        usleep(filesystem_config->retardo_acceso_bloque*1000);
        info += block_size;
        tam -= block_size;
    }
    close(bloques);
}

int siguiente_bloque_libre(){
    for(int i=0; i<block_count; i++){
        if(!bitarray_test_bit(bitmap,i)){
            return i;
        }
    }
}

int espacio_libre(){
    int bloques_libres = 0;
    for(int i=0; i<block_count ;i++){
        if(!bitarray_test_bit(bitmap,i)){
            bloques_libres += 1;
        }
    }
    int espacio_libre = bloques_libres * block_size;
    return espacio_libre;
}

char* obtener_file_name(t_buffer* buffer){
    return extraer_string_del_buffer(buffer);
}

int obtener_tamanio(t_buffer* buffer){
    return extraer_int_del_buffer(buffer);
}

void* obtener_info(t_buffer* buffer){
    return extraer_de_buffer(buffer);
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