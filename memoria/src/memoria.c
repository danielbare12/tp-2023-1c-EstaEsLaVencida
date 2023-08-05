/*
 ============================================================================
 Name        : memoria.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include "memoria.h"

typedef struct{
	int base;
	int tamanio;
}hueco_libre;

typedef struct{
	hueco_libre* hueco;
	int indice;
}t_hueco;

typedef struct{
	int df;
	int tamanio;
	char* dato;
}t_df;
typedef struct{
	int pid;
	t_list* segments;
}tabla_de_proceso;
bool ordenar_tamanios(void* data1,void* data2);
void mostrar_tabla_huecos_libres();
void asignar_hueco_segmento_0(int tamanio);
op_asignacion devolver_metodo_asignacion(char* asignacion);
void mostrar_tablas_de_segmentos();
void atender_modulos(void* data);
t_df* deserializar_df(t_buffer* buffer);
void* memoria_usuario;


t_list* tabla_huecos_libres;
t_list* tabla_general;

sem_t mutex_modulos;

t_hueco buscar_por_first(int seg_tam);
t_hueco buscar_por_best(int seg_tam);
t_hueco buscar_por_worst(int seg_tam);
t_list* crear_segmento(int pid,int seg_id,int seg_tam);

seg_aux deserializar_segmento_auxiliar(t_buffer* buffer);
seg_aux deserializar_segmento_a_eliminar(t_buffer* buffer);
void eliminar_segmento(int pid, int seg_id);

void enviar_tabla_actualizada(t_list* resultado,int cliente_fd);
t_paquete* crear_paquete_tabla_actualizada(t_list* lista,op_code codigo);
t_buffer* serializar_tabla_actualizada(t_list* lista);
char* leer_valor_de_memoria(int df,int tamanio);
void escribir_valor_de_memoria(char* valor,int df, int tamanio);

//----------------------------------------------------------------

int suma_de_huecos=0;

typedef struct{
	int pid;
	segmento*segm;
}segm_y_pid;

bool ordenar_x_base(void* data1,void* data2);
void compactar_tabla_general();
void compactar_tabla_huecos_libres();
//void compactar_memoria_usuario();
void mostrar_tabla_aux(segm_y_pid* value);
void combinar_huecos_libres();
t_list* buscar_lista_segmentos(int pid);
void eliminar_proceso(int pid);
void enviar_tabla_general(int conexion);
t_paquete* crear_paquete_tabla_general();
t_buffer* serializar_tabla_general();
int hay_espacio(int tamanio_segmento);

int pid;
int main(int argc, char** argv) {
	if (argc < 2) {
		printf ("se deben especificar la ruta del archivo de pseudocodigo");
		return EXIT_FAILURE;
	}
	pthread_t hilo_atender_modulos;
	sem_init(&mutex_modulos,0,1);
	tabla_huecos_libres = list_create();
	tabla_general = list_create();
	pid = 0;
	logger = iniciar_logger(MEMORIA_LOG, MEMORIA_NAME);
	config = iniciar_config(argv[1]);
	//config = iniciar_config(MEMORIA_CONFIG);

	iniciar_config_memoria();
	//Aqui le reservo la memoria al espacio de usuario contiguo
	memoria_usuario = malloc(datos.tam_memoria);
	//Le asigno el primer hueco libre que seria la memroia completa
	hueco_libre* hueco = malloc(sizeof(hueco_libre));

	hueco->base = 0;
	hueco->tamanio = datos.tam_memoria;

	list_add(tabla_huecos_libres,hueco);
	//Reservo el primer segmento que sera compartido por todos los procesos
	asignar_hueco_segmento_0(datos.tam_segmento);

	mostrar_tabla_huecos_libres();

	int server_fd = iniciar_servidor(logger,datos.puerto_escucha);

	log_info(logger, "Memoria listo para recibir a los modulos");

	cliente_fd = malloc(sizeof(int));

	while(1){
		//Recibe todos los modulos que solicita bajo demanda
		cliente_fd = esperar_cliente(logger,server_fd);
		pthread_create(&hilo_atender_modulos,NULL,(void*) atender_modulos,cliente_fd);
		pthread_detach(hilo_atender_modulos);
	}
	
	finalizar_memoria();
}

void atender_modulos(void* data){
	int cliente_fd = *((int*)data);
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	t_buffer* buffer;
	t_list* lista;
	t_list* resultado;
	seg_aux datos_aux;
	int df;
	int tamanio;
	int offset = 0;
	char* valor = string_new();
	while(1){

		int cod_op = recibir_operacion(cliente_fd);
		sem_wait(&mutex_modulos);
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(logger,cliente_fd);
				break;
			case PAQUETE:
				lista = recibir_paquete(cliente_fd);
				log_info(logger, "Me llegaron los siguientes valores:\n");
				list_iterate(lista, (void*) iterator);
				break;
			case INICIALIZAR_ESTRUCTURA:

				pid++;
				/*Cuando le llega un proceso se le envia nomas el tamanio del segmentoa
				  ya que se da por sentado que todos van a empezar con ese segmento
				  asi que solo se le envia el tamanio del segmento 0 para simplificar
				*/

				log_info(logger,"Creación de Proceso PID: <%d>",pid);
				segmento * seg = malloc(sizeof(segmento));
				seg->id = 0;
				seg->direccion_base = 0;
				seg->tamanio = datos.tam_segmento;

				//Creo la tabla de segmentos de los procesos
				tabla_de_proceso* tab = malloc(sizeof(tabla_de_proceso));
				tab->pid = pid;
				tab->segments = list_create();

				list_add(tab->segments,seg);
				list_add(tabla_general,tab);
				//Le envio el tamanio del proceso
				send(cliente_fd,&datos.tam_segmento,sizeof(int),0);
				mostrar_tablas_de_segmentos();
				break;
			case CREAR_SEGMENTO:
				buffer = desempaquetar(paquete,cliente_fd);

				//Recive del kernel el segmento a crear
				datos_aux = deserializar_segmento_auxiliar(buffer);

				if(hay_espacio(datos_aux.segmento.tamanio) == 0){
					/*log_info(logger,"Crear segmento con pid: %i id: %i con base: %i con tamanio: %i ",
							datos_aux.pid,datos_aux.segmento.id,datos_aux.segmento.tamanio);*/
					//En esta funcion se crea el segmento
					log_info(logger,"PID: <%d> - Crear Segmento: <%d> - Base: <%d> - TAMAÑO: <%d>",datos_aux.pid,datos_aux.segmento.id,datos_aux.segmento.direccion_base,datos_aux.segmento.tamanio);
					resultado = crear_segmento(datos_aux.pid, datos_aux.segmento.id,datos_aux.segmento.tamanio);

					if(list_size(resultado) == 0){
						op_code codigo = REALIZAR_COMPACTACION;

						log_info(logger,"Solicitud de Compactación");

						send(cliente_fd,&codigo,sizeof(op_code),0);
					} else {
						enviar_tabla_actualizada(resultado,cliente_fd);
					}
				} else {
					op_code codigo = SIN_MEMORIA;
					send(cliente_fd,&codigo,sizeof(op_code),0);
				}



				break;
			case ELIMINAR_SEGMENTO:
				buffer = desempaquetar(paquete, cliente_fd);
				datos_aux = deserializar_segmento_auxiliar(buffer);

				log_info(logger,"PID: <%d> - Eliminar Segmento: <%d> - Base: <%d> - TAMAÑO: <%d>",
						datos_aux.pid, datos_aux.segmento.id,datos_aux.segmento.direccion_base,datos_aux.segmento.tamanio);
				eliminar_segmento(datos_aux.pid,datos_aux.segmento.id);
				resultado = buscar_lista_segmentos(datos_aux.pid);
				enviar_tabla_actualizada(resultado,cliente_fd);

				break;
			case FINALIZAR:
				buffer = desempaquetar(paquete, cliente_fd);
				int pid;
				memcpy(&pid,buffer->stream,sizeof(int));

				log_info(logger,"Eliminación de Proceso PID: <%d>",pid);

				eliminar_proceso(pid);
				break;

			case REALIZAR_COMPACTACION:
				log_info(logger,"Empezando a compactar");
				compactar_tabla_general();
				compactar_tabla_huecos_libres();
				//sleep(5);
				mostrar_tabla_huecos_libres();
				mostrar_tablas_de_segmentos();
				usleep(datos.ret_compactacion*1000);
				//op_code codigo = COMPACTAR;
				//send(cliente_fd,&codigo,sizeof(op_code),0);
				enviar_tabla_general(cliente_fd);
				break;
			case ACCEDER_PARA_LECTURA:
				buffer = desempaquetar(paquete, cliente_fd);
				offset = 0;

				memcpy(&df, buffer->stream + offset, sizeof(int));
				offset += sizeof(int);
				memcpy(&tamanio, buffer->stream + offset, sizeof(int));
				offset += sizeof(int);

				//Leo en memoria elvalor del df que pide
				valor = leer_valor_de_memoria(df,tamanio);
				log_info(logger,"Acción: <LEER> - Dirección física: <%d> - Tamaño: <%d> - Origen: <CPU/FS>",df,tamanio);

				send(cliente_fd, valor, tamanio, 0);
				//Ejemplo de lectura
				break;
			case ACCEDER_PARA_ESCRITURA:
				buffer = desempaquetar(paquete, cliente_fd);

				t_df* df = deserializar_df(buffer);

				//log_info(logger,"El df es %i, con tamanio %i, el valor es %s",df->df,df->tamanio,df->dato); //TODO: memoryLeak
				log_info(logger,"Acción: <ESCRIBIR> - Dirección física: <%d> - Tamaño: <%d> - Origen: <CPU/FS>",df->df,df->tamanio);
				//Le agrego en memoria el valor del string
				escribir_valor_de_memoria(df->dato,df->df,df->tamanio);
				int a = 0;
				send(cliente_fd, &a, sizeof(int), 0);
				break;
			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				log_destroy(logger);
				config_destroy(config);
				close(cliente_fd);
				close(server_fd);
				free(data);
				//return EXIT_FAILURE;
				exit(1);
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
		}
		sem_post(&mutex_modulos);
	}

}

void iniciar_config_memoria(){
	datos.puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	datos.tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
	datos.tam_segmento = config_get_int_value(config, "TAM_SEGMENTO_0");
	datos.cant_segmentos = config_get_int_value(config, "CANT_SEGMENTOS");
	datos.ret_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
	datos.ret_compactacion = config_get_int_value(config, "RETARDO_COMPACTACION");
	datos.algoritmo = devolver_metodo_asignacion(config_get_string_value(config, "ALGORITMO_ASIGNACION"));
}

void finalizar_memoria(){
	log_destroy(logger);
	config_destroy(config);
	close(server_fd);
	free(cliente_fd);
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}

void mostrar_tabla_huecos_libres(){
	log_info(logger,"ESTA ES LA TABLA DE HUECOS LIBRES:");
	log_info(logger,"___________________________________");
	log_info(logger,"BASE        |    TAMANIO     ");
	for(int i = 0; i < list_size(tabla_huecos_libres);i++){
		hueco_libre* hueco = list_get(tabla_huecos_libres,i);
		log_info(logger,"%i          |    %i          ",hueco->base,hueco->tamanio);
	}
	log_info(logger,"___________________________________");
}

void mostrar_tablas_de_segmentos(){
	log_info(logger,"ESTA ES LA TABLA DE SEGMENTOS:");
	for(int i = 0; i < list_size(tabla_general);i++){
		tabla_de_proceso* pro = list_get(tabla_general,i);
		log_info(logger,"___________________________________");
		log_info(logger,"PROCESO %i",pro->pid);
		for(int j = 0; j < list_size(pro->segments);j++){
			segmento* s = list_get(pro->segments,j);
			log_info(logger,"___________________________________");
			log_info(logger,"Segmento: %i | Base: %i | Tamanio: %i",s->id,s->direccion_base,s->tamanio);
		}
	}
	log_info(logger,"___________________________________");
}

op_asignacion devolver_metodo_asignacion(char* asignacion){
	op_asignacion cod;

	if(strcmp(asignacion,"FIRST")==0){
		cod = FIRST;
	} else if(strcmp(asignacion,"BEST")==0){
		cod = BEST;
	} else if(strcmp(asignacion,"WORST")==0){
		cod = WORST;
	} else {
		log_warning(logger,"Codigo invalido");
	}

	return cod;
}

void asignar_hueco_segmento_0(int tamanio){
	segmento * seg = malloc(sizeof(segmento));
	for(int i = 0; i < list_size(tabla_huecos_libres);i++){
		hueco_libre* hueco_asignado = list_get(tabla_huecos_libres,i);
		if(hueco_asignado->tamanio > tamanio){
			seg->id = 0;
			seg->direccion_base = 0;
			seg->tamanio = tamanio;

			hueco_asignado->base += tamanio;
			hueco_asignado->tamanio -= tamanio;
			list_replace(tabla_huecos_libres,i,hueco_asignado);
		}
	}
}

t_list* crear_segmento(int pid,int seg_id,int seg_tam){

	t_hueco hueco_i;
	t_list* tabla_a_enviar;
	//Busco el hueco por el aggoritmo elegido
	switch(datos.algoritmo){
		case FIRST:
			hueco_i = buscar_por_first(seg_tam);
			break;
		case BEST:
			hueco_i = buscar_por_best(seg_tam);
			break;
		case WORST:
			hueco_i = buscar_por_worst(seg_tam);
			break;
	}
	//Busco la tabla por el pid y se lo asigno

	if(hueco_i.indice < 0){
		tabla_a_enviar = list_create();
	} else {
		for(int i = 0; i < list_size(tabla_general);i++){
			tabla_de_proceso* proc = list_get(tabla_general,i);
			if(proc->pid == pid){
				segmento* seg= malloc(sizeof(segmento));
				seg->id = seg_id;
				seg->tamanio = seg_tam;
				seg->direccion_base = hueco_i.hueco->base;

				hueco_i.hueco->base += seg_tam;
				hueco_i.hueco->tamanio -= seg_tam;

				list_replace(tabla_huecos_libres,hueco_i.indice,hueco_i.hueco);

				list_add(proc->segments,seg);
				//En esta variable guardo la tabla de segmetno que le voy a enviar al kernel
				//Y actualizar el constexto de ejecucion
				tabla_a_enviar = proc->segments;

			}
		}
	}


	//Ordeno la tabla de huecos para mas comodidad
	list_sort(tabla_huecos_libres,ordenar_tamanios);
	mostrar_tabla_huecos_libres();
	mostrar_tablas_de_segmentos();


	return tabla_a_enviar;
}

bool ordenar_tamanios(void* data1,void* data2){
	hueco_libre* hueco1 = ((hueco_libre*) data1);
	hueco_libre* hueco2 = ((hueco_libre*) data2);

	if(hueco1->base < hueco2->base){
		return true;

	} else {
		return false;
	}
}

t_hueco buscar_por_first(int seg_tam){
	t_hueco hueco_i;
	hueco_i.indice=-1;
	for(int i = 0; i<list_size(tabla_huecos_libres);i++){
		hueco_i.hueco = list_get(tabla_huecos_libres,i);

		if(hueco_i.hueco->tamanio >= seg_tam){
			hueco_i.indice = i;
			break;
		}

		suma_de_huecos+=hueco_i.hueco->tamanio;
	}

	return hueco_i;
}

t_hueco buscar_por_best(int seg_tam){


	t_hueco hueco_i;
	for(int i = 0; i<list_size(tabla_huecos_libres);i++){
		hueco_i.hueco = list_get(tabla_huecos_libres,i);

		if(hueco_i.hueco->tamanio == seg_tam){
			hueco_i.indice = i;
			break;
		} else if(hueco_i.hueco->tamanio > seg_tam){
			hueco_i.indice = i;
			for(int j = i+1; j<list_size(tabla_huecos_libres);j++){
				hueco_libre* hueco_j = list_get(tabla_huecos_libres,j);
				if(hueco_j->tamanio < hueco_i.hueco->tamanio && hueco_j->tamanio >= seg_tam){
					hueco_i.hueco = hueco_j;
					hueco_i.indice = j;
					break;
				}
			}
			break;
		}
	}

	return hueco_i;
}

t_hueco buscar_por_worst(int seg_tam){


	t_hueco hueco_i;
	for(int i = 0; i<list_size(tabla_huecos_libres);i++){
		hueco_i.hueco = list_get(tabla_huecos_libres,i);


		if(hueco_i.hueco->tamanio >= seg_tam){
			hueco_i.indice = i;
			for(int j = i+1; j<list_size(tabla_huecos_libres);j++){
				hueco_libre* hueco_j = list_get(tabla_huecos_libres,j);
				if(hueco_j->tamanio > hueco_i.hueco->tamanio){
					hueco_i.hueco = hueco_j;
					hueco_i.indice = j;
					break;
				}
			}
			break;
		}
	}

	return hueco_i;
}

void eliminar_segmento(int pid, int seg_id){

	//busco la tabla del proceso
	for(int i = 0; i < list_size(tabla_general);i++){
		tabla_de_proceso* proc = list_get(tabla_general,i);
		if(proc->pid == pid){
			//Ahora busco es la tabla de segmentos del proceso el segmento a eliminar
			for(int j = 0; j < list_size(proc->segments); j++){
				segmento* seg = list_get(proc->segments,j);
				if(seg->id == seg_id){
					hueco_libre* hueco = malloc(sizeof(hueco_libre));
					hueco->base = seg->direccion_base;
					hueco->tamanio = seg->tamanio;
					list_add(tabla_huecos_libres,hueco);
					list_remove(proc->segments,j);
					//Guardo la tabla de segmentos que despues le tengo que enviar al pcb para actualizar

					break;

				}
			}

			break;

		}
	}
	list_sort(tabla_huecos_libres,ordenar_tamanios);
	combinar_huecos_libres();
	mostrar_tabla_huecos_libres();
	mostrar_tablas_de_segmentos();
}

t_list* buscar_lista_segmentos(int pid){
	t_list* tabla_a_enviar;
	//busco la tabla del proceso
	for(int i = 0; i < list_size(tabla_general);i++){
		tabla_de_proceso* proc = list_get(tabla_general,i);
		if(proc->pid == pid){
			//Ahora busco es la tabla de segmentos del proceso el segmento a eliminar
			//Guardo la tabla de segmentos que despues le tengo que enviar al pcb para actualizar
			tabla_a_enviar = proc->segments;
			break;

		}
	}

	return tabla_a_enviar;
}

seg_aux deserializar_segmento_auxiliar(t_buffer* buffer){
	seg_aux segmento_auxiliar;
	int pid, seg_id, seg_tam;
	int offset = 0;

	memcpy(&pid, buffer->stream + offset, sizeof(int));
	offset+=sizeof(int);
	memcpy(&seg_id,buffer->stream + offset, sizeof(int));
	offset+=sizeof(int);
	memcpy(&seg_tam, buffer->stream + offset, sizeof(int));//TODO:memoryLeak
	offset+=sizeof(int);

	segmento_auxiliar.pid=pid;
	segmento_auxiliar.segmento.id=seg_id;
	segmento_auxiliar.segmento.tamanio=seg_tam;

	return segmento_auxiliar;
}

seg_aux deserializar_segmento_a_eliminar(t_buffer* buffer){
	seg_aux segmento_auxiliar;
	int pid, seg_id;
	int offset = 0;

	memcpy(&pid, buffer->stream + offset, sizeof(int));
	offset+=sizeof(int);
	memcpy(&seg_id,buffer->stream + offset, sizeof(int));
	offset+=sizeof(int);

	segmento_auxiliar.pid=pid;
	segmento_auxiliar.segmento.id=seg_id;
	segmento_auxiliar.segmento.tamanio=0;

	return segmento_auxiliar;
}

char* leer_valor_de_memoria(int df,int tamanio){
	char* valor = malloc(tamanio);//TODO: esta bien el tamanioa  reservar memoria?
	//Copio el valor de la df a la variable valor
	memcpy(valor,memoria_usuario+df,tamanio);
	//memcpy(valor+tamanio,&final,sizeof(char));
	//valor[tamanio] = '\0';

	usleep(datos.ret_memoria*1000);
	return valor;
	//free(valor); //TODO: memoryLeak
}

void escribir_valor_de_memoria(char* valor,int df, int tamanio){
	log_info(logger,"Se escribio el valor %s", valor);
	memcpy(memoria_usuario+df,valor,tamanio);
	usleep(datos.ret_memoria*1000);
}

void enviar_tabla_actualizada(t_list* resultado,int cliente_fd){
	t_paquete* paquete = crear_paquete_tabla_actualizada(resultado,CREACION_EXITOSA);

	enviar_paquete_a(paquete,cliente_fd);
}

t_paquete* crear_paquete_tabla_actualizada(t_list* lista,op_code codigo){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo;
	paquete->buffer = serializar_tabla_actualizada(lista);

	return paquete;
}

t_buffer* serializar_tabla_actualizada(t_list* lista){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int offset = 0;
	int tamanio_tabla = list_size(lista);
	buffer->size = sizeof(int)+sizeof(segmento)*tamanio_tabla;
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &(tamanio_tabla), sizeof(int));
	offset += sizeof(int);

	for(int i = 0; i < tamanio_tabla; i++) {
		segmento* seg = list_get(lista,i);

		memcpy(buffer->stream + offset, &seg->id, sizeof(int));
		offset += sizeof(int);

		memcpy(buffer->stream + offset, &seg->direccion_base, sizeof(int));
		offset += sizeof(int);

		memcpy(buffer->stream + offset, &seg->tamanio, sizeof(int));
		offset += sizeof(int);
	}

	return buffer;
}


t_df* deserializar_df(t_buffer* buffer){
	int offset = 0;
	t_df* df = malloc(sizeof(t_df));
	//log_info(logger,"el buffer tiene %s",buffer->stream);
	memcpy(&df->df, buffer->stream+offset, sizeof(int));
	offset += sizeof(int);
	memcpy(&df->tamanio, buffer->stream+offset, sizeof(int));
	offset += sizeof(int);
	df->dato = malloc(df->tamanio+1);
	memcpy(df->dato, buffer->stream+offset, df->tamanio);
	offset += df->tamanio;

	return df;
}


bool ordenar_x_base(void* data1,void* data2){
	segm_y_pid* seg1 = ((segm_y_pid*) data1);
	segm_y_pid* seg2 = ((segm_y_pid*) data2);

	if(seg1->segm->direccion_base < seg2->segm->direccion_base){
		return true;

	} else {
		return false;
	}
}

void compactar_tabla_general(){
	//segmento * unseg = malloc(sizeof(segmento));
	t_list*tabla_aux=list_create();//tabla con todo junto de tabla general (tabla de segmentosypid)
	tabla_de_proceso*tabla; //pid del proceso y lista de segmentos

	//int seg0=1;//para que ponga el segmento 0 una sola vez

	for(int i=0;i<list_size(tabla_general);i++){
		tabla=list_get(tabla_general,i);//obtengo una tabla de proceso

		for(int j=0;j<list_size(tabla->segments);j++){ //cada segmento lo pongo con su pid y lo guardo en tabla_aux
			segm_y_pid*segmento=malloc(sizeof(segm_y_pid*));
			segmento->segm=list_get(tabla->segments,j);
			segmento->pid=tabla->pid;
			list_add(tabla_aux,segmento);
		}
	}

	list_sort(tabla_aux,ordenar_x_base);// ordeno la lista tabla_aux (de segmentosypid) x base
	//ahora hay que ver q hay contiguo.
	list_iterate(tabla_aux,(void*) mostrar_tabla_aux);
	int base_minima = datos.tam_segmento;
	for(int i = list_size(tabla_general); i < list_size(tabla_aux); i++){
		segm_y_pid* seg = list_get(tabla_aux,i);
		memcpy(memoria_usuario + base_minima,memoria_usuario+seg->segm->direccion_base,seg->segm->tamanio);
		seg->segm->direccion_base = base_minima;
		base_minima = seg->segm->direccion_base + seg->segm->tamanio;

	}

	list_iterate(tabla_aux,(void*) mostrar_tabla_aux);
	/*
	for(int i=0;i<list_size(tabla_aux);i++){//TODO Atrapado en un bucle infinito

		segm_y_pid*seg1=list_get(tabla_aux,i);
		segm_y_pid*seg2=list_get(tabla_aux,i++);

		if(seg1->segm->id==0 && seg0==1){
			list_add(tabla_aux,seg1);
			seg0--;
		}
		else
		if(seg1->segm->direccion_base+seg1->segm->tamanio != seg2->segm->direccion_base) {
			//no estan contiguos hay q compactar
			seg2->segm->direccion_base=seg1->segm->direccion_base+seg1->segm->tamanio;
			list_add(tabla_aux,seg1);
			list_add(tabla_aux,seg2);
		}

	}*/

	//void* memoria_aux = malloc(datos.tam_memoria);

	//memcpy(memoria_aux,memoria_usuario,datos.tam_memoria);

	for(int i = 0; i < list_size(tabla_general); i++){
		tabla_de_proceso* tabla = list_get(tabla_general,i);
		for(int j = 0; j < list_size(tabla_aux);j++){
			segm_y_pid* info = list_get(tabla_aux,j);
			if(tabla->pid == info->pid){
				for(int g = 1; g < list_size(tabla->segments);g++){
					segmento* seg = list_get(tabla->segments,g);
					if(seg->id == info->segm->id){
						//int tamanio = info->segm->tamanio;
						//char* dato = malloc(tamanio);
						//memcpy(dato,memoria_usuario + seg->direccion_base,tamanio);
						//memcpy(memoria_usuario + info->segm->direccion_base,dato,tamanio);
						seg->direccion_base = info->segm->direccion_base;
						list_replace(tabla->segments,g,seg);

					}
				}
			}
		}
	}

	//free(memoria_aux);

/*
	t_list* tabla_gen_aux=list_create();
	//quedo la tabla compactada, hay que volver todo a su lugar
	for(int i=0;i<list_size(tabla_general);i++){

		t_list* listsegmentos=list_create();
		tabla_de_proceso*tabproc=malloc(sizeof(tabla_de_proceso*));

		tabproc=list_get(tabla_general,i);//obtengo una tabla de proceso

		for(int j=0;j<list_size(tabla_aux);j++){
			segm_y_pid*seg=list_get(tabla_aux,j);

			if(seg->pid==tabproc->pid){ //si el pid del segmento de tabla_aux es igual al pid de la tabla de procesos
				unseg->id=seg->segm->id;
				unseg->direccion_base=seg->segm->direccion_base;
				unseg->tamanio=seg->segm->tamanio;
				list_add(listsegmentos,unseg);
			}
		}
		list_clean_and_destroy_elements(tabproc->segments,NULL);//REVISAR SI ESTA BIEN
		list_add_all(tabproc->segments,listsegmentos);//agrego la lista de segmentos a la tabla de procesos de ese pid
		list_add(tabla_gen_aux,tabproc);//agrego tabla de proceso a tabla_gen_aux

	}
	//al agregar todas las tablas de proceso a tabla_Gen_aux, luego agrego esta tabla_gen_aux a tabla_General
	list_clean_and_destroy_elements(tabla_general,NULL);//REVISAR SI ESTA BIEN
	list_add_all(tabla_general,tabla_gen_aux);*/
}


/*
void compactar_tabla_huecos_libres(){
	int suma_huecos=0;
	hueco_libre*hueco=malloc(sizeof(hueco_libre));
	for(int i=0;i<list_size(tabla_huecos_libres);i++){
		hueco=list_get(tabla_huecos_libres,i);
		suma_huecos+=hueco->tamanio;
	}
	hueco_libre*nuevo_hueco=malloc(sizeof(hueco_libre));
	nuevo_hueco->base=0;
	nuevo_hueco->tamanio=suma_huecos;
	//list_clean_and_destroy_elements(tabla_huecos_libres,NULL);//REVISAR SI ESTA BIEN
	list_clean(tabla_huecos_libres);
	list_add(tabla_huecos_libres,nuevo_hueco);
}*/

void mostrar_tabla_aux(segm_y_pid* value){
	log_info(logger,"PID: %i | SEG: %i | BASE: %i | TAMANIO: %i",value->pid,value->segm->id,value->segm->direccion_base,value->segm->tamanio);
}

void compactar_tabla_huecos_libres(){
	int suma_huecos=0;
	//hueco_libre*hueco=malloc(sizeof(hueco_libre));
	for(int i=0;i<list_size(tabla_huecos_libres);i++){
		hueco_libre* hueco= list_get(tabla_huecos_libres,i);
		suma_huecos+=hueco->tamanio;
	}
	hueco_libre*nuevo_hueco=malloc(sizeof(hueco_libre));
	nuevo_hueco->base=datos.tam_memoria - suma_huecos;
	nuevo_hueco->tamanio=suma_huecos;
	//list_clean_and_destroy_elements(tabla_huecos_libres,NULL);//REVISAR SI ESTA BIEN
	list_clean(tabla_huecos_libres);
	list_add(tabla_huecos_libres,nuevo_hueco);
}

void combinar_huecos_libres(){
	//int cant = list_size(tabla_huecos_libres);
	for(int i = 1; i < list_size(tabla_huecos_libres);i++){
		hueco_libre* hueco1 = list_get(tabla_huecos_libres,i-1);
		hueco_libre* hueco2 = list_get(tabla_huecos_libres,i);

		if(hueco1->base + hueco1->tamanio == hueco2->base){
			hueco1->tamanio += hueco2->tamanio;

			list_remove(tabla_huecos_libres,i);
			i--;
		}
	}
}

void eliminar_proceso(int pid){
	for(int i = 0; i < list_size(tabla_general);i++){
			tabla_de_proceso* proc = list_get(tabla_general,i);
			if(proc->pid == pid){
				int cant = list_size(proc->segments);
				cant--;
				while(cant >= 1){
					segmento* seg = list_get(proc->segments,cant);
					eliminar_segmento(proc->pid,seg->id);
					cant--;

				}
				break;
			}
		}
}

void enviar_tabla_general(int conexion){
	t_paquete* paquete = crear_paquete_tabla_general();

	enviar_paquete_a(paquete,conexion);
}

t_paquete* crear_paquete_tabla_general(){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = COMPACTAR;
	paquete->buffer = serializar_tabla_general();

	return paquete;
}

t_buffer* serializar_tabla_general(){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int offset = 0;
	int cant_proc = list_size(tabla_general);
	buffer->size = sizeof(int);

	for(int i = 0; i < cant_proc; i++){
		tabla_de_proceso* proc = list_get(tabla_general,i);

		buffer->size += sizeof(int)*2 + list_size(proc->segments)*sizeof(int)*3;
	}

	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &cant_proc, sizeof(int));
	offset += sizeof(int);

	for(int i = 0; i < cant_proc; i++){
		tabla_de_proceso* proc = list_get(tabla_general,i);
		memcpy(buffer->stream + offset, &proc->pid, sizeof(int));
		offset += sizeof(int);

		int cant_seg = list_size(proc->segments);

		memcpy(buffer->stream + offset, &cant_seg, sizeof(int));
		offset += sizeof(int);

		for(int j = 0; j < cant_seg;j++){
			segmento* seg = list_get(proc->segments,j);

			memcpy(buffer->stream + offset, &seg->id, sizeof(int));
			offset += sizeof(int);
			memcpy(buffer->stream + offset, &seg->direccion_base, sizeof(int));
			offset += sizeof(int);
			memcpy(buffer->stream + offset, &seg->tamanio, sizeof(int));
			offset += sizeof(int);
		}
	}

	return buffer;
}

int hay_espacio(int tamanio_segmento){
	int suma_huecos_tamanios=0;
	int band;
	//hueco_libre*hueco=malloc(sizeof(hueco_libre));
	for(int i=0;i<list_size(tabla_huecos_libres);i++){
		hueco_libre* hueco= list_get(tabla_huecos_libres,i);
		suma_huecos_tamanios+=hueco->tamanio;
	}

	if(tamanio_segmento <= suma_huecos_tamanios){
		band = 0;
	} else {
		band = 1;
	}

	return band;

}

