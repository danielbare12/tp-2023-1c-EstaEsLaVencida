/*
 * fileSystem.h
 *
 *  Created on: Apr 7, 2023
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <pthread.h>
#include <utils.h>
#include <sockets.h>
#include <compartido.h>
#include <serializacion.h>
#include <utils.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
//------------------ FUNC-DEFINIDAS ------------------//
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
//----------------------------------------------------//

#define FS_LOG "filesySystem.log"
#define FS_CONFIG "fileSystem.config"
#define FS_NAME "File_System"
#define PATH_FS "/home/utnso/fs" //cambiar las direcciones (/home/utnso/tp-2023-1c-EstaEsLaVencida/files)
//#define PATH_FCB "/home/utnso/fs/fcb" //cambiar las direcciones (/home/utnso/tp-2023-1c-EstaEsLaVencida/files/FCB)
#define TAMANIO_DE_PUNTERO 4

const char *contenido_superbloque[] = {
	"BLOCK_SIZE=64",
	"BLOCK_COUNT=1024"
};

const char *contenido_fcb[] = {
	"NOMBRE_ARCHIVO=",
	"TAMANIO_ARCHIVO=0",
	"PUNTERO_DIRECTO=0",
	"PUNTERO_INDIRECTO=0"
};

char* str1 = "/";
//char* path_bitmap = "/home/utnso/fs/BITMAPA.dat";
DIR* diretorio_FCB;
void* memoria_file_system;
int tam_fs;
int f_bloques;

typedef struct{
	char* ip_memoria;
	char* puerto_memoria;
	char* puerto_escucha;
	char* path_superbloque;
	char* path_bitmap;
	char* path_bloques;
	char* path_fcb;
	int ret_acceso_bloque;
}datos_config;

typedef struct{
   int block_size;
   int block_count;
}datos_superbloque;

// ------------------------ESTRUCTURAS------------------------ //
typedef struct{
	uint32_t id;
	char* nombre_archivo;
	char* ruta_archivo;
	uint32_t tamanio_archivo;
	uint32_t puntero_directo;
	uint32_t puntero_indirecto;
	uint32_t puntero_archivo;
} fcb_t;

typedef struct{
	t_list* lista_fcb;
} fcb_list_t;

typedef struct{
	uint32_t id_bloque;
	uint32_t offset;
	uint32_t tamanio;
} offset_fcb_t;

typedef struct{
	int fid; //Identificador del bloque
	void* dato;
	int pos;
}t_block;

void* datos_memoria;

// ------------------------ARCHIVOS------------------------ //

FILE* F_BITMAP;
FILE* F_BLOCKS;
FILE* F_FCB;

// ------------------------VARIABLES GLOBALES------------------------ //
t_bitarray* bitmap;
size_t TAM_BITMAP;
t_list* lista_fcb;
t_list* lista_bloques;

char* PATH_FCB;

int fid_generator = 0;
char* nombreArchivo;



// ------------------------CONEXIONES------------------------ //
t_log* logger;

t_config* config;
t_config* config_superbloque;
//TODO: Ver si lo sacamos o dejamos
t_config* config_bitmap; 
t_config* config_bloques;
t_config* config_fcb;
datos_config datos;
datos_superbloque superbloque;
int server_fd;
int conexion_memoria;
pthread_t hilo_conexion_kernel;
pthread_t hilo_conexion_memoria;

// ------------------------FUNCIONES------------------------ //
void* atender_kernel(void);
void enviar_respuesta_kernel(int*, op_code);

void crear_archivo(const char*, const char *contenidos[], int); //solo crea el archivo de superbloque? 
int crear_archivo_fcb(char*);
//fcb_t* crear_fcb(t_pcb* pcb);
void crear_archivo_bloques();

int buscar_fcb(char*);
int buscar_archivo_fcb(char*);
//fcb_t* obtener_fcb(t_pcb*);
FILE* obtener_archivo(char*);
//void actualizar_lista_fcb(fcb_t*);
void agrandar_archivo(char*, int,fcb_t*);
//void achicar_archivo(fcb_t*, int);
int convertir_byte_a_bit(int);


int fcb_id;
//fcb_list_t* lista_global_fcb;


void escribir_bitmap(t_list*, int);
void leer_bitmap();
void leer_bitarray(t_bitarray*);
void crear_bitmap();
void crear_archivo_bitmap();
void cerrar_bitmap();

void liberar_lista_bloques();


void finalizar_fs();
void inicializar_superbloque();
void inicializar_config();
void crear_estructura_fs(const char *contenidos[]);
t_config* iniciar_config_test(char*);
void iterator(char*);
void set_tamanio_archivo(FILE*, int);
int tamanio_maximo_real_archivo();
//int tamanio_maximo_real_archivo(t_fcb*);
int cant_bloques_disponibles_bitmap();
char* concatenar_path(char*);
void asignar_bloque_a_archivo(char* , t_config*);
void* leer_dato_del_bloque(int, int);
void escribir_dato_en_bloque(void*, int, int);
uint32_t leer_fcb_por_key(char*,char*);

fcb_t* inicializar_fcb();
int buscar_fcb_id(int );
int buscar_fcb(char*);
t_list* armar_lista_offsets(char*, int, int, int);
t_list* obtener_lista_de_bloques(char*,int, int);

typedef enum
{
	TAMANIO_ARCHIVO,
	PUNTERO_INDIRECTO,
	PUNTERO_DIRECTO
} fcb_enum;

fcb_t* get_fcb_id(char*);
uint32_t valor_fcb(int, fcb_enum);
int modificar_fcb(fcb_t*, fcb_enum, uint32_t);
t_list* obtener_lista_total_de_bloques(char*, int);
void achicar_archivo(char*, int, fcb_t* fcb);
#endif /* FILESYSTEM_H_ */
