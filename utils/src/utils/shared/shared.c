#include "shared.h"

int crear_conexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;
}

int iniciar_servidor(char* puerto, t_log* logger){
    int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, puerto, &hints, &servinfo);

    socket_servidor = socket(servinfo->ai_family,
                         servinfo->ai_socktype,
                         servinfo->ai_protocol);
						 
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_debug(logger, "Se inicio correctamente, esperando cliente en el puerto %s", puerto);

	return socket_servidor;
}

int esperar_conexion(int socket_servidor) {
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

char* copiar_a_memoria(char* palabra){
    size_t longitud = strlen(palabra);
    char* tmp = malloc(sizeof(char) * (longitud + 1));
    memcpy(tmp, palabra, longitud);
    tmp[longitud] = '\0';
    return tmp;
}

t_paquete* crear_paquete(op_code cod_op, t_buffer* buffer){
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->cod_operacion = cod_op;
    paquete->buffer = buffer;
    return paquete;
}
/*
void enviar_paquete(t_paquete* paquete,int socket_cliente)
{	void* a_enviar = serializar_paquete(paquete);
	int bytes = paquete->buffer->size + 2*sizeof(int);
	

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}*/

void enviar_paquete(t_paquete* paquete, int socket_cliente) {
    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void* a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
}
/*
void* serializar_paquete(t_paquete* paquete){
	int size = paquete->buffer->size + 2*sizeof(int);
	void* coso = malloc(size);
	int desplazamiento = 0;

	memcpy(coso + desplazamiento, &(paquete->cod_operacion),sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(coso + desplazamiento, &(paquete->buffer->size),sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(coso + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return coso;
}*/

void* serializar_paquete(t_paquete* paquete, int bytes) {
    void* magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(paquete->cod_operacion), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    desplazamiento += paquete->buffer->size;

    return magic;
}

void destruir_paquete(t_paquete* paquete){
    destruir_buffer(paquete->buffer);
    free(paquete);
}

t_buffer* crear_buffer(){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = 0;
	buffer->stream = NULL;
	return buffer;
}

void destruir_buffer(t_buffer* buffer){
    if(buffer->stream != NULL){
        free(buffer->stream);
     }    
    free(buffer);
}

void* extraer_de_buffer(t_buffer* buffer){
    if(buffer->size == 0){
       printf("\n Error al extraer contenido del buffer VACIO\n");
       exit(EXIT_FAILURE);
    }
    if(buffer->size < 0){
        printf("\n Error al extraer contenido del buffer por tamanio negativo\n");
        exit(EXIT_FAILURE);
    }

    int size_buffer;
    memcpy(&size_buffer, buffer->stream, sizeof(int));
    void* valor = malloc(size_buffer);
    memcpy(valor, buffer->stream + sizeof(int), size_buffer);

    int new_size = buffer->size - sizeof(int) - size_buffer;
    if(new_size == 0){
        buffer->size = 0;
        free(buffer->stream);
        buffer->stream = NULL;
        return valor;
    }
    if(new_size < 0){
        printf("\n Error al extraer contenido del buffer por tamanio negativo\n");
        exit(EXIT_FAILURE);
    }
    void* nuevo_stream = malloc(new_size);
    memcpy(nuevo_stream, buffer->stream + sizeof(int) + size_buffer, new_size);
    free(buffer->stream);
    buffer->size = new_size;
    buffer->stream = nuevo_stream;

    return valor;
}

void cargar_a_buffer(t_buffer* buffer, void* valor, int tamanio){
    if(buffer -> size == 0){
        buffer->stream = malloc(sizeof(int) + tamanio);
        memcpy(buffer->stream, &tamanio, sizeof(int));
        memcpy(buffer->stream + sizeof(int), valor, tamanio);
    }
    else{
        buffer->stream = realloc(buffer->stream, buffer->size + sizeof(int) + tamanio);
        memcpy(buffer->stream + buffer->size, &tamanio, sizeof(int));
        memcpy(buffer->stream + buffer->size + sizeof(int), valor, tamanio);
    }
    buffer->size += sizeof(int);
    buffer->size += tamanio;
}

t_buffer* recibir_buffer(int conexion){
    t_buffer* buffer = malloc(sizeof(t_buffer));

    if(recv(conexion, &(buffer->size), sizeof(int), MSG_WAITALL) > 0){
        buffer -> stream = malloc(buffer->size);
        if(recv(conexion, buffer->stream, buffer->size, MSG_WAITALL) > 0){
            return buffer;
        }else {
            printf("Error al recibir el buffer\n");
            exit(EXIT_FAILURE);
        }
    }
    else{
        printf("Error al recibir el tamanio del buffer\n");
        exit(EXIT_FAILURE);
    }
    return buffer;
}

int extraer_int_del_buffer(t_buffer* buffer){
    int* entero = extraer_de_buffer(buffer);
    int valor_int = *entero;
    free(entero);
    return valor_int;
}

void cargar_int_a_buffer(t_buffer* buffer, int valor){
    cargar_a_buffer(buffer, &valor, sizeof(int));
}

void cargar_uint32_a_buffer(t_buffer* buffer, uint32_t valor){
    cargar_a_buffer(buffer, &valor, sizeof(uint32_t));
}

uint32_t extraer_uint32_del_buffer(t_buffer* buffer){
    uint32_t* entero = extraer_de_buffer(buffer);
    uint32_t valor_int = *entero;
    free(entero);
    return valor_int;
}

void cargar_contexto_a_buffer(t_buffer* buffer, t_contexto_completo* contexto) {
    cargar_int_a_buffer(buffer, contexto->base);
    cargar_int_a_buffer(buffer, contexto->limite);
    
    cargar_uint32_a_buffer(buffer, contexto->registros->AX);
    cargar_uint32_a_buffer(buffer, contexto->registros->BX);
    cargar_uint32_a_buffer(buffer, contexto->registros->CX);
    cargar_uint32_a_buffer(buffer, contexto->registros->DX);
    cargar_uint32_a_buffer(buffer, contexto->registros->EX);
    cargar_uint32_a_buffer(buffer, contexto->registros->FX);
    cargar_uint32_a_buffer(buffer, contexto->registros->GX);
    cargar_uint32_a_buffer(buffer, contexto->registros->HX);
    cargar_uint32_a_buffer(buffer, contexto->registros->PC);
}

t_contexto_completo* extraer_contexto_del_buffer(t_buffer* buffer) {
    t_contexto_completo* contexto = malloc(sizeof(t_contexto_completo));
    contexto->base = extraer_int_del_buffer(buffer);
    contexto->limite = extraer_int_del_buffer(buffer);

    contexto->registros = malloc(sizeof(t_registros));

    contexto->registros->AX = extraer_uint32_del_buffer(buffer);
    contexto->registros->BX = extraer_uint32_del_buffer(buffer);
    contexto->registros->CX = extraer_uint32_del_buffer(buffer);
    contexto->registros->DX = extraer_uint32_del_buffer(buffer);
    contexto->registros->EX = extraer_uint32_del_buffer(buffer);
    contexto->registros->FX = extraer_uint32_del_buffer(buffer);
    contexto->registros->GX = extraer_uint32_del_buffer(buffer);
    contexto->registros->HX = extraer_uint32_del_buffer(buffer);
    contexto->registros->PC = extraer_uint32_del_buffer(buffer);

    return contexto;
}

void cargar_registros_a_buffer(t_buffer* buffer, t_registros* registros){

    cargar_uint32_a_buffer(buffer, registros->AX);
    cargar_uint32_a_buffer(buffer, registros->BX);
    cargar_uint32_a_buffer(buffer, registros->CX);
    cargar_uint32_a_buffer(buffer, registros->DX);
    cargar_uint32_a_buffer(buffer, registros->EX);
    cargar_uint32_a_buffer(buffer, registros->FX);
    cargar_uint32_a_buffer(buffer, registros->GX);
    cargar_uint32_a_buffer(buffer, registros->HX);
    cargar_uint32_a_buffer(buffer, registros->PC);   
}

t_registros* extraer_registros_del_buffer(t_buffer* buffer){
    t_registros* registros = malloc(sizeof(t_registros));
    registros->AX = extraer_uint32_del_buffer(buffer);
    registros->BX = extraer_uint32_del_buffer(buffer);
    registros->CX = extraer_uint32_del_buffer(buffer);
    registros->DX = extraer_uint32_del_buffer(buffer);
    registros->EX = extraer_uint32_del_buffer(buffer);
    registros->FX = extraer_uint32_del_buffer(buffer);
    registros->GX = extraer_uint32_del_buffer(buffer);
    registros->HX = extraer_uint32_del_buffer(buffer);
    registros->PC = extraer_uint32_del_buffer(buffer);
 
    return registros;
}

void cargar_string_a_buffer(t_buffer* buffer, char* valor){
    cargar_a_buffer(buffer, valor, strlen(valor) + 1);
}

/*
char extraer_string_del_buffer(t_buffer* buffer){
    char* string = extraer_de_buffer(buffer);
    char valor_string = *string;
    free(string);
    return valor_string;
}*/

char* extraer_string_del_buffer(t_buffer* buffer){
    char* string = extraer_de_buffer(buffer);
    return string;
}

// Por el momento no se usa porque se manda TID y PID
void cargar_pcb_a_buffer(t_buffer* buffer, t_pcb* pcb){
    cargar_a_buffer(buffer, pcb, sizeof(t_pcb));
}

// Por el momento no se usa porque se manda TID y PID
void cargar_tcb_a_buffer(t_buffer* buffer, t_tcb* tcb){
    cargar_a_buffer(buffer, tcb, sizeof(t_tcb));
}

t_tcb* extraer_tcb_del_buffer(t_buffer* buffer){
    t_tcb* tcb = extraer_de_buffer(buffer);
    return tcb;
}

char* estado_a_string(estado estado){
    if (estado == NEW) return "NEW";
    if (estado == READY) return "READY";
    if (estado == EXEC) return "EXEC";
    if (estado == BLOCK) return "BLOCK";
    if (estado == EXIT) return "EXIT";
}

algoritmo convertir_string_a_algoritmo(char* algoritmo){
    if (strcmp(algoritmo, "FIFO") == 0) return FIFO;
    if (strcmp(algoritmo, "PRIORIDADES") == 0) return PRIORIDADES;
    if (strcmp(algoritmo, "CMN") == 0) return CMN;
}

algoritmos_memoria convertir_string_a_algoritmo_memoria(char* algoritmo){
    if (strcmp(algoritmo, "DINAMICAS") == 0) return DINAMICAS;
    if (strcmp(algoritmo, "FIJAS") == 0) return FIJAS;
    if (strcmp(algoritmo, "FIRST") == 0) return FIRST;
    if (strcmp(algoritmo, "BEST") == 0) return BEST;
    if (strcmp(algoritmo, "WORST") == 0) return WORST;
}

t_instruccion_a_enviar extraer_instruccion_a_enviar_del_buffer(t_buffer* buffer) {
    t_instruccion_a_enviar* instruccion_a_enviar;
    instruccion_a_enviar = extraer_de_buffer(buffer);
    t_instruccion_a_enviar valor_instruccion = *instruccion_a_enviar;
    free(instruccion_a_enviar);
    return valor_instruccion;    
}

void cargar_instruccion_a_buffer(t_buffer* buffer, t_instruccion* instruccion) {
    int buffer_size = sizeof(op_code) + sizeof(int) +  (instruccion->tamanio_lista * sizeof(char*)) + instruccion->tamanio_lista*1;
    cargar_a_buffer(buffer, instruccion, buffer_size);
}

t_instruccion extraer_instruccion_del_buffer(t_buffer* buffer){
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    instruccion = extraer_de_buffer(buffer);
    t_instruccion valor_instruccion = *instruccion;
    free(instruccion);
    return valor_instruccion;
}

void cargar_instruccion_a_enviar_a_buffer(t_buffer* buffer, t_instruccion_a_enviar instruccion) {     
    cargar_a_buffer(buffer, &instruccion, sizeof(t_instruccion_a_enviar));
}

int map_instruccion_a_enum(char* instruccion){
    if(strcmp(instruccion, "SET") == 0){
        return SET ;
    } else if(strcmp(instruccion, "READ_MEM") == 0){
        return READ_MEM ;
    } else if(strcmp(instruccion, "WRITE_MEM") == 0){
        return WRITE_MEM;
    } else if(strcmp(instruccion, "SUM") == 0){
        return SUM ;
    } else if(strcmp(instruccion, "SUB") == 0){
        return SUB ;
    } else if(strcmp(instruccion, "JNZ") == 0){
        return JNZ ;
    } else if(strcmp(instruccion, "LOG") == 0){
        return LOG ;
    } else if(strcmp(instruccion, "DUMP_MEMORY") == 0){
        return DUMP_MEMORY ;
    } else if(strcmp(instruccion, "IO") == 0){
        return IO ;
    } else if(strcmp(instruccion, "PROCESS_CREATE") == 0){
        return PROCESS_CREATE ;
    } else if(strcmp(instruccion, "THREAD_CREATE") == 0){
        return THREAD_CREATE ;
    } else if(strcmp(instruccion, "THREAD_JOIN") == 0){
        return THREAD_JOIN ;
    } else if(strcmp(instruccion, "THREAD_CANCEL") == 0){
        return THREAD_CANCEL ;
    } else if(strcmp(instruccion, "MUTEX_CREATE") == 0){
        return MUTEX_CREATE  ;
    } else if(strcmp(instruccion, "MUTEX_LOCK") == 0){
        return MUTEX_LOCK  ;
    } else if(strcmp(instruccion, "MUTEX_UNLOCK") == 0){
        return MUTEX_UNLOCK ;
    } else if(strcmp(instruccion, "THREAD_EXIT") == 0){
        return THREAD_EXIT ;
    } else if(strcmp(instruccion, "PROCESS_EXIT") == 0){
        return PROCESS_EXIT ;
    } else {
        return -1;
    }
}

char* map_instruccion_a_string(instruccion instr) {
    switch (instr) {
        case SET: return "SET";
        case READ_MEM: return "READ_MEM";
        case WRITE_MEM: return "WRITE_MEM";
        case SUM: return "SUM";
        case SUB: return "SUB";
        case JNZ: return "JNZ";
        case LOG: return "LOG";
        case DUMP_MEMORY: return "DUMP_MEMORY";
        case IO: return "IO";
        case PROCESS_CREATE: return "PROCESS_CREATE";
        case THREAD_CREATE: return "THREAD_CREATE";
        case THREAD_JOIN: return "THREAD_JOIN";
        case THREAD_CANCEL: return "THREAD_CANCEL";
        case MUTEX_CREATE: return "MUTEX_CREATE";
        case MUTEX_LOCK: return "MUTEX_LOCK";
        case MUTEX_UNLOCK: return "MUTEX_UNLOCK";
        case THREAD_EXIT: return "THREAD_EXIT";
        case PROCESS_EXIT: return "PROCESS_EXIT";
        default: return "INSTRUCCION_INVALIDA";
    }
}