/*
 ============================================================================
 Name        : fileSystem.c
 Author      : EstaEsLaVencida
 Version     :
 Copyright   : Your copyright notice
 Description : Modulo de FileSystem
 ============================================================================
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "fileSystem.h"

void enviar_datos_para_lectura(int dir, int tamanio);
char* deserializar_valor(t_buffer* buffer);
void enviar_datos_para_escritura(int dir, char* valor, int tamanio);
void escribir_datos(void* datos, t_list* lista_offsets,char* nombre);
void* leer_datos(t_list* lista_offsets,char* nombre_archivo);
void* leer_dato_del_bloque(int offset, int size);
void leer_bloques_indirectos(char*nombre,t_list* lista_de_bloques, int offset_inicial, int offset_final);
uint32_t leer_acceso(int offset, int size);
t_list* obtener_lista_de_bloques(char* nombreArchivo,int offset_inicial, int size);
fcb_t* get_fcb(char* nombre);
void escribir_bitmap(t_list* list, int bit_presencia);
void limpiar_bit_en_bitmap(uint32_t id_bloque);
uint32_t obtener_primer_bloque_libre();
void setear_bit_en_bitmap(uint32_t id_bloque);
void escribir_bloques_indirectos(t_list* lista_bloques, int indice_inicial, int offset_indirecto);
void _escribir_int(uint32_t dato, int offset);

int main(int argc, char** argv) {
	//if (argc < 2) {
		//printf ("se deben especificar la ruta del archivo de pseudocodigo");
		//return EXIT_FAILURE;
	//}
	logger = iniciar_logger(FS_LOG, FS_NAME);;
	config = iniciar_config("fileSystem.config");
	lista_fcb = list_create();

	inicializar_config();
	crear_estructura_fs(contenido_superbloque);

	log_info(logger,"PATH_SUPERBLOQUE: %s", datos.path_superbloque);
	log_info(logger,"PATH_BITMAP: %s", datos.path_bitmap);
	log_info(logger,"PATH_BLOQUES: %s", datos.path_bloques);
	log_info(logger,"PATH_FCB: %s\n", datos.path_fcb);

	//prueba();

	diretorio_FCB = opendir(datos.path_fcb);

	conexion_memoria = crear_conexion(datos.ip_memoria,datos.puerto_memoria);
	enviar_mensaje("Hola te saludo desde el Fyle System",conexion_memoria);

	//void* dato1="Console";
	//log_info(logger, "El dato1 es %s", dato1);
	//escribir_dato_en_bloque(dato1,3000,80);

	//void* dato = leer_dato_del_bloque(3000,80);

	//log_info(logger, "El dato es %s", dato);

	pthread_create(&hilo_conexion_kernel,NULL,(void*) atender_kernel,NULL);

	pthread_join(hilo_conexion_kernel,NULL);

	liberar_lista_bloques();
	cerrar_bitmap();
	finalizar_fs();
	return EXIT_SUCCESS;
}

// ---------------------------------- INICIALIZAR ----------------------------------- //

void inicializar_config(){
	datos.ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	datos.puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	datos.puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	datos.path_superbloque = config_get_string_value(config,"PATH_SUPERBLOQUE");
	datos.path_bitmap = config_get_string_value(config,"PATH_BITMAP");
	datos.path_bloques = config_get_string_value(config,"PATH_BLOQUES");
	datos.path_fcb = config_get_string_value(config,"PATH_FCB");
	datos.ret_acceso_bloque = config_get_int_value(config,"RETARDO_ACCESO_BLOQUE");
}

void crear_estructura_fs(const char *contenidos[]){
	int valor = mkdir(PATH_FS, 0777);
	if(valor < 0){
		log_warning(logger, "Ya existe el directorio: (%s)", PATH_FS);
	}
	int num_contenidos = sizeof(contenido_superbloque) / sizeof(contenido_superbloque[0]);
	crear_archivo(datos.path_superbloque, contenido_superbloque, num_contenidos);
	config_superbloque = iniciar_config_test(datos.path_superbloque);
	// config_fcb = iniciar_config_test(datos.path_fcb);
	
	valor = mkdir(datos.path_fcb, 0777);
	if(valor < 0){
		log_warning(logger, "Ya existe el directorio: %s", datos.path_fcb);
	}
	inicializar_superbloque();
	crear_archivo_bloques();
	crear_archivo_bitmap();
}

void crear_archivo(const char *nombre_archivo, const char *contenidos[], int num_contenidos) {
    FILE *archivo;

    archivo = fopen(nombre_archivo, "r");
    log_info(logger, "Paso por crear archivo: %s", nombre_archivo);

    if (archivo == NULL) {
    	//log_info(logger, "Entro por NULL: %s", nombre_archivo);
        archivo = fopen(nombre_archivo, "w");
        if (archivo != NULL) {
            for (int i = 0; i < num_contenidos; i++) {
                fprintf(archivo, "%s\n", contenidos[i]);
            }
            fclose(archivo);
            log_info(logger, "Archivo creado exitosamente: %s\n", nombre_archivo);
        } else {
            log_error(logger,"No se pudo crear el archivo: %s", nombre_archivo);
        }
    } else {
        log_warning(logger, "El archivo ya existe: %s", nombre_archivo);
        fclose(archivo);
    }
}

t_config* iniciar_config_test(char * path_config) {
	t_config* config = iniciar_config(path_config);

	if(config == NULL) {
		log_error(logger, "No se pudo leer el path %s", path_config);
	}

	return config;
}

void inicializar_superbloque(){
	superbloque.block_count = config_get_int_value(config_superbloque, "BLOCK_COUNT");
	superbloque.block_size = config_get_int_value(config_superbloque, "BLOCK_SIZE");
}

void crear_archivo_bloques(){
	F_BLOCKS = fopen(datos.path_bloques,"a");

	int f_bloques;
	f_bloques = open(datos.path_bloques, O_RDWR);

	tam_fs = superbloque.block_count * superbloque.block_size;
//	set_tamanio_archivo(F_BLOCKS, tam_fs);
	ftruncate(f_bloques,tam_fs);
	memoria_file_system = mmap(NULL,tam_fs, PROT_READ | PROT_WRITE, MAP_SHARED, f_bloques, 0);

	// crear_lista_bloques(tam_fs);
}




int crear_archivo_fcb(char* nombreArchivo){
	//int resultado = -1;

	//if(buscar_fcb(nombreArchivo) != -1){
		//log_error(logger,"Ya existe un FCB con ese nombre");
		//return resultado;
	//}

	char *path_archivo_completo = concatenar_path(nombreArchivo);

	int num_contenidos = sizeof(contenido_fcb) / sizeof(contenido_fcb[0]);
	crear_archivo(path_archivo_completo, contenido_fcb, num_contenidos);
	t_config* config_nuevo_fcb = iniciar_config_test(path_archivo_completo);

	config_set_value(config_nuevo_fcb, "NOMBRE_ARCHIVO", nombreArchivo);
	config_save_in_file(config_nuevo_fcb, path_archivo_completo);

	fcb_t* nuevo_fcb = inicializar_fcb();
	nuevo_fcb->id = fcb_id++;
	nuevo_fcb->nombre_archivo = string_new();
	string_append(&nuevo_fcb->nombre_archivo,nombreArchivo);
	nuevo_fcb->ruta_archivo = string_new();
	string_append(&nuevo_fcb->ruta_archivo,path_archivo_completo);

	t_config* fcb_fisico = malloc(sizeof(t_config));
	fcb_fisico->path = string_new();
	fcb_fisico->properties = dictionary_create();
	string_append(&fcb_fisico->path,path_archivo_completo);
	char* nombre_duplicado = string_duplicate(nombreArchivo);

	list_add(lista_fcb,nuevo_fcb);
	dictionary_destroy(fcb_fisico->properties);
	free(nombre_duplicado);
	free(fcb_fisico->path);
	free(fcb_fisico);
	//free(nombreArchivo);

	return nuevo_fcb->id;
}

int buscar_fcb(char* nombre_fcb){
	int resultado = -1;
	int size = list_size(lista_fcb);

	for(int i = 0; i<size; i++){
		fcb_t* fcb = list_get(lista_fcb,i);

		if(strcmp(fcb->nombre_archivo,nombre_fcb) == 0){
			resultado = fcb->id;
			break;
		}
	}

	return resultado;
}

int buscar_fcb_id(int id){
	int resultado = -1;
	int size = list_size(lista_fcb);

	for(int i = 0; i<size; i++){
		fcb_t* fcb = list_get(lista_fcb,i);

		if(fcb->id == id){
			resultado = fcb->id;
			break;
		}
	}

	return resultado;
}





void set_tamanio_archivo(FILE* archivo, int tamanioArchivo){
	ftruncate(fileno(archivo), tamanioArchivo);
}

void crear_archivo_bitmap(){
	//F_BITMAP = fopen(datos.path_bitmap, "a");

	int tam_bits_bloque = convertir_byte_a_bit(superbloque.block_count);
	TAM_BITMAP = tam_bits_bloque;
	//set_tamanio_archivo(F_BITMAP, TAM_BITMAP);

	crear_bitmap();
}

int convertir_byte_a_bit(int block_size){
	return block_size/8;
}

void crear_bitmap(){
	int fd;

    /* Abrimos el archivo en la direccion pasada por parametro y ajustamos el tamanio del mismo */
	fd = open(datos.path_bitmap, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR);
    if(fd==EXIT_FAILURE){
	    log_error(logger, "No se pudo abrir el archivo: %s", datos.path_bitmap);
		// exit(EXIT_FAILURE);
	}

    ftruncate(fd, TAM_BITMAP);

	/* Creamos la estructura para modificar */
	datos_memoria = mmap(NULL, TAM_BITMAP*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
	if(datos_memoria==MAP_FAILED){
	    log_error(logger, "No se pudo inicializar el mmap");
	    // exit(EXIT_FAILURE);
	}

	bitmap = bitarray_create_with_mode(datos_memoria, TAM_BITMAP, LSB_FIRST);

	for(int i = 0; i<=bitmap->size; i++){
	   bitarray_clean_bit(bitmap, i);
	}

	leer_bitarray(bitmap);

	close(fd);
}


// ------------------------------- FUNCIONES - BITMAP --------------------------------------- //

void escribir_bitmap(t_list* list, int bit_presencia){
 int cantElementos = list_size(list);

 switch(bit_presencia){
    case 1:
	  log_info(logger, "Log: %i", cantElementos);
	  for(int i = 0; i < cantElementos; i++){
		  char* pos = (char*)list_get(list, i);
		  int intPos = atoi(pos);
		  off_t offset = (off_t)intPos;

		  log_info(logger, "Posicion: %i", intPos);

		  bitarray_set_bit(bitmap, offset);
	  	  log_info(logger, "Acceso a Bitmap - Bloque: %i - Estado: 1", intPos);
	  }
	  msync(bitmap->bitarray, bitmap->size,MS_SYNC);
	  break;
    case 0:
	  for(int i = 0; i < cantElementos; i++){
	  	  char* pos = (char*)list_get(list, i);
	  	  int intPos = atoi(pos);
	  	  off_t offset = (off_t)intPos;

	  	  log_info(logger, "Posicion: %i", intPos);

		  bitarray_clean_bit(bitmap, offset);
	  	  log_info(logger, "Acceso a Bitmap - Bloque: %i - Estado: 0", intPos);
	  }
	  msync(bitmap->bitarray, bitmap->size,MS_SYNC);
	  break;
    default:
	  log_error(logger, "El valor del bit de presencia no es ni 0 ni 1");
	  exit(EXIT_FAILURE);
	  break;
  }
}

void cerrar_bitmap(){
	int rc = munmap(datos_memoria, TAM_BITMAP);
	if(rc == -1){
		log_info(logger, "No se pudo cerrar el F_BITMAP - Nro. Error:");
		exit(EXIT_FAILURE);
	}
	munmap(datos_memoria, TAM_BITMAP);
	bitarray_destroy(bitmap);
}

void leer_bitarray(t_bitarray* array){
	for(size_t i = 0; i<=array->size; i++){
		off_t off_i = (off_t)i;
		int bit = (int)bitarray_test_bit(array, off_i);
		log_info(logger, "Pos. %i - Datos: %i", (int)i, bit);
	}
}

void leer_pos_bitarray(t_bitarray* array, t_list* listPos){
	for(int i = 0; i < list_size(listPos); i++){
		char* pos = (char*)list_get(listPos, (int)i);
		int intPos = atoi(pos);

		off_t off_i = (off_t)intPos;

		int bit = (int)bitarray_test_bit(array, off_i);
		log_info(logger, "Pos. %i - Datos: %i", intPos, bit);
	}
}

void leer_bitmap(){
	size_t ret;
	void *buffer = malloc(TAM_BITMAP);
	int fd;

	fd = open(datos.path_bitmap, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR);
	if (!fd){
        log_error(logger, "No se pudo abrir el archivo");
    }

	ret = read(fd, buffer, TAM_BITMAP);

	for(size_t i = 0; i < ret; i++){
		//Manipulo bytes individuales del buffer
		char* bytePtrBuffer = (char*)buffer;
		//Calculo la direccion de memoria que quiero
		char* ptrObtenido = bytePtrBuffer + i;
	    log_info(logger, "Pos. %i - Datos: %d", (int)i, *((int*)ptrObtenido));
	}

	free(buffer);
	close(fd);
}

// ---------------------------------- FUNCIONES - ARCHIVOS - GENERAL -------------------------- //

void leer_archivo(FILE* archivo, size_t tamArchivo, char* path){
	size_t ret;
	char* buffer = malloc((int)tamArchivo);
	int dif;
    int fd;

	fd = open(path, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR);
	if (!fd){
        log_error(logger, "No se pudo abrir el archivo");
    }
	
	// Obtenemos la cantidad de valores leidos con ret y los almacenamos en el buffer
    ret = read(fd, buffer, tamArchivo);

    //Lectura del archivo cada 4 valores hexadecimales 
	for(size_t i = 0; i < ret; i+=4){
	    log_info(logger, "Lec. %i - Dato: %02X %02X %02X %02X", (int)i , buffer[i], buffer[i+1], buffer[i+2], buffer[i+3]);
		dif = (int)ret - i;
	}
    
	//Leo valores hexadecimales restantes del archivo si me quedasen 
	if(dif != 0){
	    for(size_t i = dif; i < ret; i++){
	        log_info(logger, "Lec. %i - Dato: %02X", (int)i , buffer[i]);	
	    }
	}

	free(buffer);
	close(fd);
}

void* leer_dato_del_bloque(int offset, int size){
	void* dato = malloc(size);
	//log_info(logger, "Paso por leer");
	memcpy(dato,memoria_file_system + offset, size);
	msync(memoria_file_system,tam_fs,MS_SYNC);
	usleep(datos.ret_acceso_bloque*1000);

	return dato;
}

void* leer_datos(t_list* lista_offsets,char* nombre_archivo){
	int cant_bloques = list_size(lista_offsets);
	int offset = 0;
	offset_fcb_t* bloque_inicial = list_get(lista_offsets,0);

	void* dato = leer_dato_del_bloque(bloque_inicial->offset,bloque_inicial->tamanio);

	void* datos = malloc(bloque_inicial->tamanio);

	memcpy(datos,dato,bloque_inicial->tamanio);
	offset += bloque_inicial->tamanio;

	free(dato);

	for(int i = 1; i < cant_bloques; i++){
		offset_fcb_t* bloque = list_get(lista_offsets,i);

		log_info(logger,"Acceso Bloque - Archivo: %s - Bloque File System: %d",nombre_archivo, bloque->id_bloque);
		void* dato2 = leer_dato_del_bloque(bloque->offset,bloque->tamanio);

		datos = realloc(datos,offset + bloque->tamanio);
		memcpy(datos+offset,dato,bloque->tamanio);
		offset += bloque->tamanio;
		free(dato2);
	}

	return datos;
}

uint32_t leer_fcb_por_key(char* nombreArchivo,char* key){
	char* path_archivo_completo = concatenar_path(nombreArchivo);
	t_config* config_fcb = iniciar_config_test(path_archivo_completo);

	//if(key!="TAMANIO_ARCHIVO" || key!="PUNTERO_DIRECTO" || key!="PUNTERO_INDIRECTO"){
		//Si se le pasa un parametro invalido...
		//log_warning(logger, "En leer_fcb_por_key se ingerso la key %s", key);
	//}
	//Parametro valido
	return (uint32_t) config_get_int_value(config_fcb, key);
}

void escribir_dato_en_bloque(void* dato, int offset, int size){
//	memoria_file_system = mmap(NULL,tam_fs, PROT_READ | PROT_WRITE, MAP_SHARED, f_bloques, 0);
	memcpy(memoria_file_system + offset,dato, size);
	msync(memoria_file_system,tam_fs,MS_SYNC);
	usleep(datos.ret_acceso_bloque*1000);
}

//--------------------------- FUNCIONES - ARCHIVO DE BLOQUES ----------------------------- //
// void crear_lista_bloques(){
// 	//int tamanioBloque = ceil(tamanioArchivo/superbloque.block_size);
// 	lista_bloques = list_create();

// 	for(int i = 0; i <= superbloque.block_count; i++){
// 		/*
// 		t_block* bloque = malloc(sizeof(t_block));
// 		bloque->pos=i;
// 		bloque->fid=fid_generator;

// 		fid_generator += 1;

// 		list_add(lista_bloques, bloque);
// 		*/
// 	}
// }

void liberar_lista_bloques(){
	for(int i = 0; i <= superbloque.block_count; i++){
		t_block* bloque = list_get(lista_bloques, i);
		free(bloque);
	}
}

// ---------------------------------------------------------------------------------------- //
/*
void prueba(){
	t_list* pos_bloques = malloc(sizeof(int)*4);//Posiblemente sea este el problema que lea mal los datos cuando escribe
	list_add(pos_bloques, "0");
	list_add(pos_bloques, "1");
	list_add(pos_bloques, "2");
	list_add(pos_bloques, "3");
	escribir_bitmap(pos_bloques, 1);

	leer_bitarray(bitmap);

	leer_pos_bitarray(bitmap, pos_bloques);

	int tamanio = tamanio_maximo_real_archivo();

	log_info(logger, "Tamanio Maximo Real: %i", tamanio);

	int count = cant_bloques_disponibles_bitmap();

	log_info(logger, "Cant. de BLoques Disponibles: %i", count);

	free(pos_bloques);
}
*/
/*
int* transf_list_a_ptr(t_list* lista){
	int* ptr = malloc(list_size(lista)*sizeof(int));
	for(int i = 0; i<list_size(lista);i++){
		ptr[i] = list_get(lista, i);
	}

	return ptr;
}
*/

void finalizar_fs(){
	log_destroy(logger);
	config_destroy(config);
	close(server_fd);
	close(conexion_memoria);
	fclose(F_BITMAP);
	fclose(F_BLOCKS);
	// fclose(F_FCB);
	closedir(diretorio_FCB);
}

// ----------------------------------- ADICIONALES ----------------------------------- //

char* concatenar_path(char* nombre_archivo){
	char *unaPalabra = string_new();
	string_append(&unaPalabra, datos.path_fcb);
	string_append(&unaPalabra, "/");
	string_append(&unaPalabra, nombre_archivo);
	string_append(&unaPalabra, ".dat");

    return unaPalabra;
}

void limpiar_bitarray(t_bitarray* bitarray){
 	for(int i = 0; i < bitarray_get_max_bit(bitarray); i++){
 		bitarray_clean_bit(bitarray, i);
 	}
}

int cantidad_punteros_por_bloques(int block_size){
	return block_size/TAMANIO_DE_PUNTERO; 
}

fcb_t* inicializar_fcb(){
	fcb_t* new_fcb = malloc(sizeof(fcb_t));

	new_fcb->nombre_archivo = malloc(sizeof(char) * 2);
	memcpy(new_fcb->nombre_archivo,"0",sizeof(char) * 2);
	new_fcb->ruta_archivo = malloc(sizeof(char) * 2);
	memcpy(new_fcb->nombre_archivo,"0",sizeof(char) * 2);
	new_fcb->id = 0;
	new_fcb->puntero_archivo = 0;
	new_fcb->puntero_directo = 0;
	new_fcb->puntero_indirecto = 0;
	new_fcb->tamanio_archivo = 0;

	return new_fcb;
}
//Obtiene el fcb asociado al pcb
//fcb_t* obtener_fcb(t_pcb* pcb){
//	int i=1;
//	while(i<list_size(lista_fcb)){
//		t_fcb* fcb = list_get(lista_fcb,i);
//		if(strcmp(fcb->nombre_archivo,pcb->arch_a_abrir)==1){
//			return fcb;
//		}
//		i++;
//	}
//	if(buscar_archivo_fcb(pcb->arch_a_abrir)==0){
//		log_info(logger, "No se encontro el archivo: %s", pcb->arch_a_abrir);
//	    exit(EXIT_FAILURE);
//	}else{
//		t_fcb* fcb = crear_fcb(pcb);
//		actualizar_lista_fcb(fcb);
//		return fcb;
//	}
//}

//void actualizar_lista_fcb(fcb_t* fcb){
//	log_info(logger, "Se actualiza la lista de fcbs");
//	if(buscar_fcb(fcb->nombre_archivo)==0){
//	    list_add(lista_fcb,fcb);
//	}else{
//		list_remove(lista_fcb, fcb);
//	}
//}


// ------------------------- MODIFICAR TAMANIO ---------------------- //

void agrandar_archivo(char* nombre_archivo, int tamanio_pedido, fcb_t* fcb){

	//char *path_archivo_completo = concatenar_path(nombre_archivo);
	//t_config* config_fcb = iniciar_config_test(path_archivo_completo);

    //int tamanio_archivo = config_get_int_value(config_fcb,"TAMANIO_ARCHIVO");
	t_list *lista_total_de_bloques = obtener_lista_total_de_bloques(nombre_archivo,fcb->id);
	int size_inicial = list_size(lista_total_de_bloques);
	int tamanio_archivo = fcb->tamanio_archivo;

    int diferencia = tamanio_pedido - tamanio_archivo;

	int cant_bloques_a_asignar = ceil((double) diferencia / superbloque.block_size);
	int cant_bloques_actual = ceil((double) tamanio_archivo/superbloque.block_size);

	for(int i = cant_bloques_actual; i<cant_bloques_a_asignar; i++){
		int id_bloque = obtener_primer_bloque_libre();
		offset_fcb_t *bloque = malloc(sizeof(offset_fcb_t));

		if (i == 0){
			size_inicial++;
			modificar_fcb(fcb, PUNTERO_DIRECTO, id_bloque);
			setear_bit_en_bitmap(id_bloque);
			bloque->id_bloque = id_bloque;
			list_add(lista_total_de_bloques, bloque);
			continue;
		}

		if (i == 1){
			size_inicial++;
			cant_bloques_a_asignar++;
			modificar_fcb(fcb, PUNTERO_INDIRECTO, id_bloque);
			setear_bit_en_bitmap(id_bloque);
			bloque->id_bloque = id_bloque;
			list_add(lista_total_de_bloques, bloque);
			continue;
		}

		bloque->id_bloque = id_bloque;
		setear_bit_en_bitmap(id_bloque);
		list_add(lista_total_de_bloques, bloque);
	}

	int size_final = list_size(lista_total_de_bloques);

	if(size_final > 2){
		uint32_t offset_indirecto = fcb->puntero_indirecto * superbloque.block_size;

		log_info(logger,"Acceso a Bloque - Archivo: %s - Bloque File System: %d",nombre_archivo,fcb->puntero_indirecto);
		escribir_bloques_indirectos(lista_total_de_bloques, size_inicial, offset_indirecto);
	}

	list_destroy_and_destroy_elements(lista_total_de_bloques,free);

	modificar_fcb(fcb, TAMANIO_ARCHIVO, tamanio_pedido);

}

void _escribir_int(uint32_t dato, int offset){
	memcpy(memoria_file_system + offset, &dato, 4);
	msync(memoria_file_system,tam_fs,MS_SYNC);
}

void escribir_bloques_indirectos(t_list* lista_bloques, int indice_inicial, int offset_indirecto){
	int size = list_size(lista_bloques);

	if(size < 2){
		log_error(logger,"No puede haber menos de 2 bloques para escribir indirectos");
		return;
	}

	for (int i = indice_inicial; i < size; i++)
	{
		offset_fcb_t *bloque = list_get(lista_bloques, i);

		int offset = ((i - 2) * sizeof(uint32_t)) + offset_indirecto;

		_escribir_int(bloque->id_bloque, offset);
	}

	usleep(datos.ret_acceso_bloque*1000);
}

void setear_bit_en_bitmap(uint32_t id_bloque){
	//off_t offset = (off_t)id_bloque;
	bitarray_set_bit(bitmap, id_bloque);
	msync(bitmap->bitarray,bitmap->size,MS_SYNC);
	log_info(logger,"Acceso a Bitmap - Bloque: %d - Estado: 1", id_bloque);
}

uint32_t obtener_primer_bloque_libre()
{
	for (uint32_t i = 0; i < bitarray_get_max_bit(bitmap); i++)
	{
		if (bitarray_test_bit(bitmap, i) == 0)
		{
			return i;
		}
	}
	return -1;
}

int cant_bloques_disponibles_bitmap(){
	int count = -1;
	int j = bitarray_get_max_bit(bitmap);

	for(int i = 0; i < j; i++){
		if(bitarray_test_bit(bitmap, i)==0){
		  count += 1;
		}
	}

	return count;
}

void asignar_bloque_a_archivo(char* nombreArchivo, t_config* config_fcb){

}
//----------------------------------- AUXILIARES -----------------------------------//

FILE* obtener_archivo(char* nombreArchivo){

	char *path_archivo_completo = concatenar_path(nombreArchivo);

	log_info(logger, "Entro en obtener_archivo");
	FILE* archivo = fopen(path_archivo_completo, "r");
	if(archivo==NULL){
		log_warning(logger, "Tenes un archivo en NULL");
	}
	return archivo;
}


//fcb_t* crear_fcb(t_pcb* pcb){
//	fcb_t* fcb = malloc(sizeof(t_fcb*));
//
//	char *path_archivo_completo = concatenar_path(pcb->arch_a_abrir);
//
////	char* pf = strcat(PATH_FCB, pcb->arch_a_abrir);
////	char* path_archivo = strcat(pf, ".dat");
//
//	if(buscar_archivo_fcb(pcb->arch_a_abrir)==0){
//	   crear_archivo_fcb(pcb->arch_a_abrir);
//	}
//
//	FILE* archivo = obtener_archivo(pcb->arch_a_abrir);
//
//	fcb->archivo=archivo;
//	fcb->path_archivo=path_archivo_completo;
//	fcb->nombre_archivo=pcb->arch_a_abrir;
//	fcb->tamanio_archivo=pcb->dat_tamanio;
//    //set_tamanio_archivo(fcb->archivo, pcb->dat_tamanio);
//    //log_info(logger, "la cosa va por aca");
//	fcb->puntero_directo=0;
//	fcb->puntero_indirecto=0;
//
//    list_add(lista_fcb, fcb);
//
//    //log_info(logger, "Se agregó el fcb a la lista de fcbs");
//
//	return fcb;
//}
// 

int buscar_archivo_fcb(char* nombre_archivo){
    //DIR* dir = opendir(datos.path_fcb);

    if (diretorio_FCB == NULL) {
        log_error(logger, "No se pudo abrir el directorio: %s", datos.path_fcb);
        return 0;
    }

    FILE* archivo = fopen(concatenar_path(nombre_archivo), "r+");

    if(archivo!=NULL){
    	fcb_t* fcb = malloc(sizeof(fcb_t));
    	fcb->id = fcb_id++;
    	fcb->nombre_archivo = nombre_archivo;
    	fcb->ruta_archivo = concatenar_path(nombre_archivo);
    	log_info(logger,"Se encontro el archivo: %s",nombre_archivo);
    	fcb->puntero_directo = leer_fcb_por_key(nombre_archivo,"PUNTERO_DIRECTO");
    	fcb->tamanio_archivo = leer_fcb_por_key(nombre_archivo,"TAMANIO_ARCHIVO");
    	fcb->puntero_indirecto = leer_fcb_por_key(nombre_archivo,"PUNTERO_INDIRECTO");
    	fcb->puntero_archivo = 0;

    	list_add(lista_fcb,fcb);

    	return 1;
    } else {
    	log_info(logger,"No se encontro el archivo: %s",nombre_archivo);
        return 0;
    }


}

//Obtiene el puntero de la lista de fcbs del fcb buscado
//int buscar_fcb(char* nombre_archivo){
//	int i=1;
//	while(i<list_size(lista_fcb)){
//		t_fcb* fcb = list_get(lista_fcb,i);
//		if(strcmp(fcb->nombre_archivo,nombre_archivo)==1){
//			return i;
//		}
//		i++;
//	}
//	return 0;
//}

int tamanio_maximo_real_archivo(){
	return ((superbloque.block_size/4) + 1)* superbloque.block_size;;
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}

t_list* armar_lista_offsets(char* nombreArchivo, int id_fcb, int tam_a_leer, int p_seek){
	t_list* lista_offsets = list_create();

	int bloque_apuntado = ceil((double)(p_seek + 1) / superbloque.block_size);
	int nro_bloque = 0;
	t_list* lista_bloques = obtener_lista_de_bloques(nombreArchivo,p_seek,tam_a_leer);
	int cant_bloques = list_size(lista_bloques);

	while(nro_bloque < cant_bloques){
		offset_fcb_t* nuevo_offset = malloc(sizeof(offset_fcb_t));
		offset_fcb_t* bloque = list_get(lista_bloques, nro_bloque);

		nuevo_offset->offset = bloque->id_bloque * superbloque.block_size;
		nuevo_offset->id_bloque = bloque->id_bloque;
		nuevo_offset->tamanio = superbloque.block_size;

		if(tam_a_leer < superbloque.block_size){
			nuevo_offset->offset = nuevo_offset->offset + (p_seek - ((bloque_apuntado - 1) * superbloque.block_size));
			nuevo_offset->tamanio = tam_a_leer;
		} else if (nro_bloque + 1 == cant_bloques){
			nuevo_offset->tamanio = (p_seek + tam_a_leer) - ((bloque_apuntado - 1) * superbloque.block_size);
		} else if(nro_bloque == 0){
			nuevo_offset->offset = nuevo_offset->offset + (p_seek - ((bloque_apuntado - 1) * superbloque.block_size));
			nuevo_offset->tamanio = (bloque_apuntado * superbloque.block_size) - p_seek;
		}

		nro_bloque++;
		bloque_apuntado = ceil(((double)(p_seek + 1) + (superbloque.block_size * nro_bloque)) / superbloque.block_size);

		list_add(lista_offsets,nuevo_offset);
	}

	list_destroy_and_destroy_elements(lista_bloques,free);

	return lista_offsets;
}

t_list* obtener_lista_de_bloques(char* nombreArchivo,int offset_inicial, int size){
	t_list *lista_de_bloques = list_create();

	int tamanio_archivo = (int) leer_fcb_por_key(nombreArchivo, "TAMANIO_ARCHIVO");

	if (tamanio_archivo == 0){
		return lista_de_bloques;
	}

	if(size + offset_inicial > tamanio_archivo) {
		log_error(logger,"El tamanio a leer es mayor al tamanio del archivo seleccionado");
		return lista_de_bloques;
	}

	int cant_bloques = ceil(tamanio_archivo / superbloque.block_size);

	if(size == 0) size = cant_bloques * superbloque.block_size;
	int offset_fcb = offset_inicial;

	if (offset_inicial < superbloque.block_size){
		offset_fcb_t *bloque_directo = malloc(sizeof(offset_fcb_t));
		bloque_directo->id_bloque = leer_fcb_por_key(nombreArchivo, "PUNTERO_DIRECTO");
		bloque_directo->offset = bloque_directo->id_bloque * superbloque.block_size;
		cant_bloques--;
		offset_fcb += superbloque.block_size - offset_inicial;

		list_add(lista_de_bloques, bloque_directo);
	}

	if (cant_bloques >= 1)
	{
		leer_bloques_indirectos(nombreArchivo,lista_de_bloques,offset_fcb,size + offset_inicial);
	}

	return lista_de_bloques;
}

void leer_bloques_indirectos(char* nombre,t_list* lista_de_bloques, int offset_inicial, int offset_final){
	int offset_fcb = offset_inicial;
	uint32_t bloque_indirecto = leer_fcb_por_key(nombre,"PUNTERO_INDIRECTO");
	int offset = floor(((double)offset_fcb / superbloque.block_size) - 1) * sizeof(uint32_t);

	while(offset_fcb < offset_final){
		offset_fcb_t *bloque = malloc(sizeof(offset_fcb_t));
		uint32_t dato = leer_acceso((bloque_indirecto * superbloque.block_size) + offset, sizeof(uint32_t));
		memcpy(&bloque->id_bloque, &dato , sizeof(uint32_t));
		bloque->offset = bloque->id_bloque * superbloque.block_size;
		list_add(lista_de_bloques, bloque);
		offset += sizeof(uint32_t);
		offset_fcb += superbloque.block_size;
	}

	usleep(datos.ret_acceso_bloque*1000);
}

uint32_t leer_acceso(int offset, int size){
	uint32_t dato = 0;
	memcpy(&dato,memoria_file_system + offset, size);
	msync(memoria_file_system,tam_fs,MS_SYNC);
	usleep(datos.ret_acceso_bloque*1000);
	return dato;
}

int cantidad_de_bloques(int tamanio){
	int cant_bloques = ceil(tamanio / superbloque.block_size);
	return cant_bloques;
}

void escribir_datos(void* datos, t_list* lista_offsets,char* nombre){
	int cant_bloques = list_size(lista_offsets);
	int offset = 0;

	for(int i = 0; i<cant_bloques; i++){
		offset_fcb_t* bloque = list_get(lista_offsets,i);
		log_info(logger,"Acceso Bloque - Archivo: %s - Bloque File System: %d",nombre, bloque->id_bloque);
		escribir_dato_en_bloque(datos + offset, bloque->offset, bloque->tamanio);
		offset += bloque->tamanio;
	}
}
//----------------------------------- KERNEL -----------------------------------//

void* atender_kernel(void){
	server_fd = iniciar_servidor(logger,datos.puerto_escucha);
	log_info(logger, "Fyle System listo para recibir al kernel");
	t_list* lista;
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	int *cliente_fd = esperar_cliente(logger,server_fd);
	t_pcb* pcb;
	t_buffer* buffer;
	uint32_t puntero;
	char* nombreArchivo;
	int id_fcb;
	int tamanio;
	t_list* lista_de_bloques;

	fcb_t* fcb;
	//PATH_FCB = strcat(datos.path_fcb, "/");

	while(1){
		int cod_op = recibir_operacion(*cliente_fd);
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(logger,*cliente_fd);
				break;
			case PAQUETE:
				lista = recibir_paquete(*cliente_fd);
				log_info(logger, "Me llegaron los siguientes valores:\n");
				list_iterate(lista, (void*) iterator);
				break;
			case ABRIR_ARCHIVO:
				buffer=desempaquetar(paquete,*cliente_fd);
				pcb = deserializar_pcb(buffer);
				
				log_info(logger, "Abrir Archivo: %s", pcb->arch_a_abrir);
				//log_info(logger,"El id es %d", pcb->pid);
				//uint32_t punteroDirecto = leer_fcb_por_key(pcb->arch_a_abrir,"PUNTERO_DIRECTO");
				//log_info(logger,"El puntero es : %d",punteroDirecto);
				//sleep(100);

				if(buscar_archivo_fcb(pcb->arch_a_abrir)){//buscar_archivo_fcb(pcb->arch_a_abrir)
					//log_info(logger,"PID: %d - F_OPEN: %s",nueva_instruccion->pid,nueva_instruccion->param1);
					log_info(logger,"Abrir Archivo: %s", pcb->arch_a_abrir);
					//enviar_respuesta_kernel(cliente_fd, EXISTE_ARCHIVO);
					enviar_pcb_a(pcb,*cliente_fd,EXISTE_ARCHIVO);
				}
				else {
					//log_info(logger,"PID: %d - F_OPEN: %s - NO EXISTE",nueva_instruccion->pid,nueva_instruccion->param1);
					//enviar_respuesta_kernel(cliente_fd, CREAR_ARCHIVO);
					enviar_pcb_a(pcb,*cliente_fd,CREAR_ARCHIVO);
				}
				break;
			case CREAR_ARCHIVO:
				buffer=desempaquetar(paquete,*cliente_fd);
				pcb = deserializar_pcb(buffer);

				log_info(logger, "Crear Archivo: %s", pcb->arch_a_abrir);
				//log_info(logger, "El id es %d", pcb->pid);

				if(crear_archivo_fcb(pcb->arch_a_abrir) != -1){
					//log_info(logger,"PID: %d - F_CREATE: %s",nueva_instruccion->pid,nueva_instruccion->param1);
					log_info(logger,"Crear Archivo: %s", pcb->arch_a_abrir);
				}

				//enviar_respuesta_kernel(cliente_fd, ARCHIVO_CREADO);
				enviar_pcb_a(pcb,*cliente_fd,ARCHIVO_CREADO);

				break;
			case MODIFICAR_TAMANIO:
				buffer=desempaquetar(paquete,*cliente_fd);
				pcb = deserializar_pcb(buffer);

				log_info(logger, "Truncar Archivo: %s - Tamaño: %i", pcb->arch_a_abrir, pcb->dat_tamanio);

				//char *path_archivo_completo = concatenar_path(pcb->arch_a_abrir);
				//t_config* config_fcb = iniciar_config_test(path_archivo_completo);
			  //  int tamanio_archivo = config_get_int_value(config_fcb,"TAMANIO_ARCHIVO");

				fcb = get_fcb(pcb->arch_a_abrir);


				//if(buscar_archivo_fcb(pcb->arch_a_abrir)!=0){

					if(fcb->tamanio_archivo < pcb->dat_tamanio){
						//Si el tamanio del archivo es menor al solicitado...

						if(pcb->dat_tamanio > tamanio_maximo_real_archivo()){
							log_warning(logger, "Tamanio pedido: %i es mayor que Tamanio Max: %i", pcb->dat_tamanio, tamanio_maximo_real_archivo());
						}

						log_info(logger, "Agrandar archivo: %s", pcb->arch_a_abrir);
						agrandar_archivo(pcb->arch_a_abrir, pcb->dat_tamanio,fcb);

					}else if(fcb->tamanio_archivo > pcb->dat_tamanio){
						//Si el tamanio del archivo es mayor al solicitado...

						log_info(logger, "Achica archivo: %s", pcb->arch_a_abrir);
						achicar_archivo(pcb->arch_a_abrir, pcb->dat_tamanio,fcb);

					}else{
						log_info(logger, "Tiene el mismo tamanio: %s", fcb->nombre_archivo);
					}

				//}

		       // enviar_respuesta_kernel(cliente_fd, OK);
				enviar_pcb_a(pcb,*cliente_fd,DESBLOQUEADO);

				break;
			case LEER_ARCHIVO:
				buffer=desempaquetar(paquete,*cliente_fd);
				pcb = deserializar_pcb(buffer);

				log_info(logger, "paso por LEER_ARCHIVO");

				puntero = (uint32_t) pcb->posicion;
				nombreArchivo = pcb->arch_a_abrir;
				id_fcb = buscar_fcb(pcb->arch_a_abrir);
				tamanio = pcb->cant_bytes;

				lista_de_bloques = armar_lista_offsets(nombreArchivo,id_fcb, tamanio, puntero);

				void* datos = leer_datos(lista_de_bloques,pcb->arch_a_abrir);

				enviar_datos_para_escritura(pcb->df_fs, datos, pcb->cant_bytes);
				int result;
				recv(conexion_memoria, &result, sizeof(int), MSG_WAITALL);

				if(result == 0){
					log_info(logger,"Algo salio bien");

				} else {
					log_info(logger,"El programa sigue");
				}

				enviar_pcb_a(pcb,*cliente_fd,DESBLOQUEADO);

				list_destroy_and_destroy_elements(lista_de_bloques,free);
				//char* memoria = "80";
				//char* tam = "64";
				//log_info(logger,"Leer Archivo: %s - Puntero: %d - Memoria: %s - Tamaño: %s",pcb->arch_a_abrir, puntero, memoria, tam);
//				realizar_f_read(nueva_instruccion);

				break;
			case ESCRIBIR_ARCHIVO:
				buffer=desempaquetar(paquete,*cliente_fd);
				pcb = deserializar_pcb(buffer);
				enviar_datos_para_lectura(pcb->df_fs,pcb->cant_bytes);
				void* valor = malloc(pcb->cant_bytes);
				recv(conexion_memoria, valor, pcb->cant_bytes, MSG_WAITALL);
				log_info(logger, "paso por ESCRIBIR_ARCHIVO");
				//log_info(logger,"Se ingreso el valor %s",valor);

				//void* datos = "DATO";
				nombreArchivo = pcb->arch_a_abrir;
				id_fcb = buscar_fcb(pcb->arch_a_abrir);
				tamanio = pcb->cant_bytes;
				puntero = (uint32_t) pcb->posicion;

				lista_de_bloques = armar_lista_offsets(pcb->arch_a_abrir,id_fcb, tamanio, puntero);

				escribir_datos(valor, lista_de_bloques,pcb->arch_a_abrir);

				enviar_pcb_a(pcb,*cliente_fd,DESBLOQUEADO);

				list_destroy_and_destroy_elements(lista_de_bloques,free);
				//free(datos);

				break;
			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				log_destroy(logger);
				config_destroy(config);
				close(*cliente_fd);
				close(server_fd);
				//return EXIT_FAILURE;
				exit(1);
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
		}
	}
}

void enviar_respuesta_kernel(int *cliente_fd, op_code msj){
	send(*cliente_fd,&msj,sizeof(op_code),0);
}

//----------------------------------- MEMORIA -----------------------------------//

void enviar_datos_para_lectura(int dir, int tamanio){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = ACCEDER_PARA_LECTURA;
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int offset = 0;
	buffer->size = sizeof(int)*2;
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &dir, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &tamanio, sizeof(int));
	offset += sizeof(int);

	paquete->buffer = buffer;

	enviar_paquete(paquete,conexion_memoria);

	//free(paquete->buffer->stream);
	//free(paquete->buffer);
	//free(paquete);
}

char* deserializar_valor(t_buffer* buffer){
	char* valor;

	int offset = 0;

	memcpy(&valor,buffer->stream + offset,sizeof(char*));
	offset += sizeof(char*);

	return valor;
}

void enviar_datos_para_escritura(int dir, char* valor, int tamanio){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = ACCEDER_PARA_ESCRITURA;
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int offset = 0;
	buffer->size = sizeof(int)*3 + tamanio;
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &dir, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &tamanio, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, valor, tamanio);
	offset += tamanio;

	log_info(logger,"La df es %i con tamanio %i se envio %s",dir,tamanio,valor);
	if(offset != buffer->size){
		log_warning(logger,"No son iguales offset %i buffer %i",offset,buffer->size);
	}
	paquete->buffer = buffer;

	enviar_paquete(paquete,conexion_memoria);
}

fcb_t* get_fcb(char* nombre){
	int id = buscar_fcb(nombre); 
	fcb_t* resultado;
	int size = list_size(lista_fcb);

	for(int i = 0; i<size; i++){
		fcb_t* fcb = list_get(lista_fcb,i);

		if(fcb->id == id){
			resultado = fcb;
			break;
		}
	}

	return resultado;
}

fcb_t* get_fcb_por_nombre(char* nombre){
	//int id = buscar_fcb(nombre);
	fcb_t* resultado;
	int size = list_size(lista_fcb);

	for(int i = 0; i<size; i++){
		fcb_t* fcb = list_get(lista_fcb,i);

		if(strcmp(fcb->nombre_archivo,nombre)){
			resultado = fcb;
			break;
		}
	}

	return resultado;
}

/*
uint32_t valor_fcb(int id_fcb, fcb_enum llave){
	fcb_t* fcb = get_fcb(id_fcb);
	uint32_t valor = 0;

	switch(llave){
		case TAMANIO_ARCHIVO:
			valor = fcb->tamanio_archivo;
			break;
		case PUNTERO_DIRECTO:
			valor = fcb->puntero_directo;
			break;
		case PUNTERO_INDIRECTO:
			valor = fcb->puntero_indirecto;
			break;
		default:
			break;
	}

	return valor;
}*/

int modificar_fcb(fcb_t* fcb,fcb_enum llave, uint32_t valor){
	//fcb_t* fcb = get_fcb(id);
	int resultado = 1;
	t_config* fcb_fisico = config_create(fcb->ruta_archivo);
	char* valor_string = string_itoa(valor);

	switch(llave){
		case TAMANIO_ARCHIVO:
			fcb->tamanio_archivo = valor;
			config_set_value(fcb_fisico, "TAMANIO_ARCHIVO", valor_string);
			break;
		case PUNTERO_DIRECTO:
			fcb->puntero_directo = valor;
			config_set_value(fcb_fisico, "PUNTERO_DIRECTO", valor_string);
			break;
		case PUNTERO_INDIRECTO:
			fcb->puntero_indirecto = valor;
			config_set_value(fcb_fisico, "PUNTERO_INDIRECTO", valor_string);
			break;
		default:
			resultado = -1;
			break;
	}

	config_save(fcb_fisico);
	config_destroy(fcb_fisico);
	free(valor_string);

	for(int i = 0; i < list_size(lista_fcb);i++){
		fcb_t* fcb2 = list_get(lista_fcb,i);

		if(strcmp(fcb2->nombre_archivo,fcb->nombre_archivo) == 0){
			list_replace(lista_fcb,i,fcb);
			break;
		}
	}

	return resultado;
}

t_list* obtener_lista_total_de_bloques(char* nombre_archivo, int id_fcb){
	t_list *lista_de_bloques = list_create();
	int tamanio_archivo = leer_fcb_por_key(nombre_archivo, "TAMANIO_ARCHIVO");

	if (tamanio_archivo == 0){
	 	return lista_de_bloques;
	}

	int offset_fcb = 0;
	int cant_bloques_fcb = ceil((double)tamanio_archivo / superbloque.block_size);
	int size_final = cant_bloques_fcb * superbloque.block_size;

	offset_fcb_t *bloque_directo = malloc(sizeof(offset_fcb_t));
	bloque_directo->id_bloque = leer_fcb_por_key(nombre_archivo, "PUNTERO_DIRECTO");
	bloque_directo->offset = bloque_directo->id_bloque * superbloque.block_size;
	cant_bloques_fcb--;
	offset_fcb += superbloque.block_size;

	list_add(lista_de_bloques, bloque_directo);

	if (cant_bloques_fcb >= 1)
	{
		offset_fcb_t *bloque_indirecto = malloc(sizeof(offset_fcb_t));
		bloque_indirecto->id_bloque = leer_fcb_por_key(nombre_archivo, "PUNTERO_DIRECTO");
		bloque_indirecto->offset = bloque_indirecto->id_bloque * superbloque.block_size;

		list_add(lista_de_bloques, bloque_indirecto);

		leer_bloques_indirectos(nombre_archivo,lista_de_bloques,offset_fcb,size_final);
	}

	return lista_de_bloques;
}

void achicar_archivo(char* nombre_archivo, int nuevo_tamanio, fcb_t* fcb)
{
	//int id_fcb = buscar_fcb(nombre_archivo);
	t_list *lista_de_bloques = obtener_lista_total_de_bloques(nombre_archivo, fcb->id);
	//int tamanio_archivo = leer_fcb_por_key(nombre_archivo, "TAMANIO_ARCHIVO");
	int tamanio_archivo = fcb->tamanio_archivo;
	int cant_bloques_a_desasignar = ceil(((double)(tamanio_archivo - nuevo_tamanio) / superbloque.block_size));

	int i = 0;
	int puntero_indirecto = 0;
	if(nuevo_tamanio <= superbloque.block_size && tamanio_archivo > superbloque.block_size){
		cant_bloques_a_desasignar++;
		//puntero_indirecto = valor_fcb(id_fcb, PUNTERO_INDIRECTO);
		//puntero_indirecto = leer_fcb_por_key(nombre_archivo,"PUNTERO_INDIRECTO");
		puntero_indirecto = fcb->puntero_indirecto;
	}

	while (i < cant_bloques_a_desasignar)
	{
		int last_element_index = (lista_de_bloques->elements_count - 1);
		offset_fcb_t *bloque = list_remove(lista_de_bloques, last_element_index);
		if(bloque->id_bloque == puntero_indirecto) modificar_fcb(fcb, PUNTERO_INDIRECTO, 0);
		//t_list* lista = list_create();
		//list_add(lista,bloque)
		//escribir_bitmap(bloque, 0); //limpiar_bit_en_bitmap(bloque->id_bloque);
		limpiar_bit_en_bitmap(bloque->id_bloque);
		i++;
	}

	list_destroy_and_destroy_elements(lista_de_bloques,free);

	modificar_fcb(fcb, TAMANIO_ARCHIVO, nuevo_tamanio);
}

void limpiar_bit_en_bitmap(uint32_t id_bloque) // Setea el bit en 0 para el bloque indicado
{
	bitarray_clean_bit(bitmap, id_bloque);
	msync(bitmap->bitarray,bitmap->size,MS_SYNC);
	log_info(logger,"Acceso a Bitmap - Bloque: %d - Estado: 0", id_bloque);
}
