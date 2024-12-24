#include "inicializar_filesystem.h"

void inicializar_filesystem(){
	sem_init(&mutex_bitmap, 0, 1);
	sem_init(&mutex_bloques, 0 , 1);
    crear_logger();
    iniciar_config();
	crear_bitmap();
	crear_bloques();
}

void crear_bitmap(){
	bitmap_memoria = malloc(block_count*block_size);
	bitmap = bitarray_create_with_mode(bitmap_memoria,block_count,LSB_FIRST);
	int fd_bitmap = open(path_bitmap,  O_RDWR , S_IRUSR | S_IWUSR);
	if(fd_bitmap == -1){
		int cero = 0;
		int fd_bitmap = open(path_bitmap,  O_CREAT | O_RDWR , S_IRUSR | S_IWUSR);
		for(int i=0;i<block_count;i++){
			write(fd_bitmap,&cero,4);
		}
	}
	close(fd_bitmap);
	iniciar_bitarray();
}

void iniciar_bitarray(){
	int file_bitmap = open(path_bitmap, O_RDWR, S_IRUSR | S_IWUSR);
	//bitmap = bitarray_create_with_mode(NULL,block_count,LSB_FIRST);
	int* aux;
	for(int i=0;i<block_count; i++){
		read(file_bitmap,aux,4);
		int valor = *(int*)aux;
		if(valor == 1){
			bitarray_set_bit(bitmap,i);
		}
	}
	close(file_bitmap);
}

void crear_bloques(){
	int fd_bloques = open(path_bloques,  O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); 
	close(fd_bloques);
}

void crear_logger() {
	filesystem_log = log_create("filesystem.log", "filesystem", 1, LOG_LEVEL_TRACE);
	
	if (filesystem_log == NULL) { 
		printf("No se pudo crear el logger");
		exit(1);
	}

}

void iniciar_config() {
	t_config* nuevo_config = config_create("/home/utnso/tp-2024-2c-Bit-Wizards/filesystem/config/filesystem.config");

	if (nuevo_config == NULL) { 
		log_error(filesystem_log, "No se pudo leer la configuracion");
		exit(1);
	}

	filesystem_config = malloc(sizeof(t_filesystem_config));    

	filesystem_config->puerto_escucha = copiar_a_memoria(config_get_string_value(nuevo_config, "PUERTO_ESCUCHA"));
	filesystem_config->mount_dir= copiar_a_memoria(config_get_string_value(nuevo_config, "MOUNT_DIR"));
	filesystem_config->block_size= config_get_int_value(nuevo_config, "BLOCK_SIZE");
	filesystem_config->block_count= config_get_int_value(nuevo_config, "BLOCK_COUNT");
	filesystem_config->retardo_acceso_bloque= config_get_int_value(nuevo_config, "RETARDO_ACCESO_BLOQUE");  
    filesystem_config->log_level = copiar_a_memoria(config_get_string_value(nuevo_config, "LOG_LEVEL"));

	config_destroy(nuevo_config);
	block_count = filesystem_config->block_count;
	block_size = filesystem_config->block_size;
	path_bitmap = malloc(strlen(filesystem_config->mount_dir)+20);
	path_bitmap = "/home/utnso/mount_dir/bitmap.dat";
	path_bloques = malloc(strlen(filesystem_config->mount_dir)+20);
	path_bloques = "/home/utnso/mount_dir/bloques.dat";	
	path_metadata = malloc(strlen(filesystem_config->mount_dir)+20);
	path_metadata = "/home/utnso/mount_dir/files/";
}