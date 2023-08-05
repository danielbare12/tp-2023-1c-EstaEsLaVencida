/*
 ============================================================================
 Name        : kernel.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "kernel.h"

t_log* logger;
t_config* config;
datos_config datos;
int server_fd;
int* cliente_fd;
int conexion_cpu;
int conexion_memoria;
int conexion_filesystem;
t_list* archivos_abiertos;
//t_buffer* desempaquetar(t_paquete* paquete, int cliente_fd);
int agregar_pid(t_buffer* buffer);
void mostrar(t_instruccion* inst);
void mostrar_parametro(char* value);
void mostrar_cola(t_queue*cola);
t_pcb* crear_pcb(t_buffer* buffer, int conexion_consola);


void* atender_cpu(void);
void* de_new_a_ready(void);
void de_ready_a_ejecutar_fifo(void);
void* de_ready_a_ejecutar(void);
void mostrar_recurso(t_recurso* value);
void ejecutar_signal(t_pcb* pcb);
void* ejecutar_IO(void* dato);
/////PLANIFICACION///////
//colas
t_cola cola;

////TRANSICIONES////
void agregar_a_cola_new(t_pcb*pcb);
void agregar_a_cola_ready(t_pcb*pcb);
//void agregar_a_cola_hrrn(t_pcb*pcb);
//void finalizar_proceso(t_pcb*pcb);
t_pcb* quitar_de_cola_new();
t_pcb* quitar_de_cola_ready();
algoritmo devolver_algoritmo(char*nombre);
void de_ready_a_ejecutar_hrrn(void);
//void replanificar(void);
//Recursos
void ejecutar_wait(t_pcb* pcb);
bool comparador_hrrn(void* data1,void* data2);
void mostrar_registro(t_pcb* pcb);
void* atender_fileSystem(void);

//SERIALIZACION
int deserializar_df(t_buffer* buffer);
char* deserializar_nombreArchivo(t_buffer* buffer);

//ADICIONALES
//bool archivoEnUso(char* nombreArchivo);

sem_t mutex_fs;
sem_t mutex_cola_new;
sem_t mutex_cola_ready;
sem_t sem_multiprogramacion;
sem_t sem_nuevo;
sem_t sem_ready;
sem_t sem_habilitar_exec;
sem_t mutex_compresion;

pthread_t thread_nuevo_a_ready;
pthread_t thread_atender_cpu;
pthread_t hilo_atender_consolas;
pthread_t thread_ejecutar;
pthread_t thread_recurso;
pthread_t thread_bloqueo_IO;
pthread_t thread_archivo_abiertos;
pthread_t thread_atender_fileSystem;

t_list* lista_recursos;
t_list* lista_archivos_abiertos;

int t_ahora;

void* iniciar_recurso(void* data);
void enviar_crear_segmento(int pid, int id_seg,int tamanio_seg);
void enviar_eliminar_segmento(int pid, int id_seg,int tamanio_seg);
void analizar_resultado(t_pcb* pcb,t_paquete* paquete,t_buffer* buffer);
t_list* deserializar_tabla_actualizada(t_buffer* buffer);
void enviar_eliminar_proceso(int pid);
void deserializar_tabla_general_actualizada(t_buffer* buffer);
int contiene_archivo(t_pcb* pcb, char* nombreArchivo);
void* iniciar_archivo(void* data);
void crear_archivo(char* nombreArchivo);
void cerrar_archivo(t_pcb* pcb);
void modificar_tamanio(t_pcb* pcb);
void actualizar_puntero(t_pcb* pcb);
int buscar_puntero(t_pcb* pcb);
void mostrar_list_ready();
//int enviar_datos_a_memoria(t_pcb* pcb, int conexion, op_code codigo);
//void enviar_segmento_con_cod(seg_aux* segmento, int conexion, op_code codigo);
int pid;

t_list* tabla_general;

typedef struct{
	int pid;
	t_list* segments;
}tabla_de_proceso;

void mostrar_tablas_de_segmentos();
t_pcb* actualizar_de_la_tabla_general(t_pcb* pcb);
int hayQueActualizar;

int main(int argc, char** argv) {
	if (argc < 2) {
		printf ("se deben especificar la ruta del archivo de pseudocodigo");
		return EXIT_FAILURE;
	}
	pid=0;

	lista_recursos = list_create();
	lista_archivos_abiertos = list_create();
	tabla_general = list_create();

	hayQueActualizar = 0;

	cola.cola_new = queue_create();
	cola.cola_ready_fifo=queue_create();
	cola.cola_ready_hrrn=queue_create();

	logger = iniciar_logger("kernel.log","Kernel");;
	config = iniciar_config(argv[1]);
	datos.ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	datos.puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	datos.ip_filesystem = config_get_string_value(config,"IP_FILESYSTEM");
	datos.puerto_filesystem = config_get_string_value(config,"PUERTO_FILESYSTEM");
	datos.ip_cpu = config_get_string_value(config,"IP_CPU");
	datos.puerto_cpu = config_get_string_value(config,"PUERTO_CPU");
	datos.puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	datos.algoritmo_planificacion=devolver_algoritmo(config_get_string_value(config,"ALGORITMO_PLANIFICACION"));
	datos.est_inicial = config_get_int_value(config,"ESTIMACION_INICIAL");
	datos.alfa = atof(config_get_string_value(config,"HRRN_ALFA"));
	datos.multiprogramacion=config_get_int_value(config,"GRADO_MAX_MULTIPROGRAMACION");
	datos.recursos = string_get_string_as_array(config_get_string_value(config,"RECURSOS"));
	datos.instancias = string_get_string_as_array(config_get_string_value(config,"INSTANCIAS_RECURSOS"));

    sem_init(&mutex_fs,0,1);
	sem_init(&mutex_cola_new,0,1);
	sem_init(&mutex_cola_ready,0,1);
	sem_init(&mutex_compresion,0,1);
	sem_init(&sem_multiprogramacion,0,datos.multiprogramacion);
	sem_init(&sem_nuevo,0,0);
	sem_init(&sem_ready,0,0);
	sem_init(&sem_habilitar_exec,0,1);
	log_info(logger,"El alfa es %f ",datos.alfa);
	int i = 0;
	while(datos.recursos[i] != NULL){
		t_recurso* recurso = malloc(sizeof(t_recurso));

		recurso->nombre = datos.recursos[i];
		recurso->instancias = atoi(datos.instancias[i]);
		recurso->cola_recurso = queue_create();
		sem_init(&recurso->sem_recurso,0,0);

		pthread_create(&thread_recurso,NULL,(void*) iniciar_recurso,recurso);
		list_add(lista_recursos,recurso);

		i++;

	}

	conexion_cpu = crear_conexion(datos.ip_cpu,datos.puerto_cpu);
	conexion_memoria = crear_conexion(datos.ip_memoria,datos.puerto_memoria);
	conexion_filesystem = crear_conexion(datos.ip_filesystem,datos.puerto_filesystem);


	enviar_mensaje("Hola te estoy hablando desde el kernel",conexion_cpu);
	enviar_mensaje("Hola te estoy hablando desde el kernel",conexion_memoria);
	enviar_mensaje("Hola te estoy hablando desde el kernel",conexion_filesystem);

	pthread_create(&thread_atender_cpu,NULL,(void*) atender_cpu,NULL);
	pthread_create(&thread_nuevo_a_ready,NULL,(void*) de_new_a_ready,NULL);
	pthread_create(&thread_ejecutar,NULL,(void*) de_ready_a_ejecutar,NULL);
	pthread_create(&thread_atender_fileSystem,NULL,(void*) atender_fileSystem,NULL);

	server_fd = iniciar_servidor(logger,datos.puerto_escucha);
	log_info(logger, "KERNEL listo para recibir a las consolas");

	cliente_fd = malloc(sizeof(int));//

	while(1){
		cliente_fd = esperar_cliente(logger,server_fd);
		pthread_create(&hilo_atender_consolas,NULL,(void*) atender_consolas,cliente_fd);
		pthread_detach(hilo_atender_consolas);
	}
	pthread_detach(thread_recurso);
	pthread_join(thread_atender_cpu,NULL);
	pthread_join(thread_nuevo_a_ready,NULL);
	pthread_join(thread_ejecutar,NULL);
	pthread_join(thread_atender_fileSystem,NULL);

	log_destroy(logger);
	config_destroy(config);
	close(server_fd);

}

void* atender_cpu(void){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	t_pcb* pcb;
	t_buffer* buffer;
	op_code c;
	//int result;

	//t_cola proc_bloqueados=queue_create();
    //int df;
	while(1){
		int cod_op = recibir_operacion(conexion_cpu);
			switch (cod_op) {
				case DESALOJADO:
					log_info(logger,"Paso por desalojado");

					buffer = desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					int hora = time(NULL);

					pcb->real_ant = (hora - pcb->llegadaExec)*1000;
					log_info(logger,"El pcb [%i] tardo en exec %i",pcb->pid,pcb->real_ant);
					pcb->estimacion = datos.alfa*pcb->real_ant + (1 - datos.alfa)*pcb->estimacion;
					log_info(logger,"La estimacion del pcb [%i]  es de %f en la hora %i",pcb->pid,pcb->estimacion,hora);
					log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <READY>",pcb->pid);
					agregar_a_cola_ready(pcb);
					sem_post(&sem_habilitar_exec);
					break;
				case EJECUTAR_WAIT:
					log_info(logger,"Paso por WAIT");
					buffer = desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					ejecutar_wait(pcb);
					break;
				case EJECUTAR_SIGNAL:
					log_info(logger,"Paso por signal");
					buffer = desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					ejecutar_signal(pcb);
					break;
				case EJECUTAR_IO:
					log_info(logger,"Paso por IO");

					buffer = desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					sem_post(&sem_habilitar_exec);
					pcb->real_ant = time(NULL) - pcb->llegadaExec;
					pcb->estimacion = datos.alfa*pcb->real_ant + (1 - datos.alfa)*pcb->estimacion;
					log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCK>",pcb->pid);
					pthread_create(&thread_bloqueo_IO,NULL,(void*) ejecutar_IO,pcb);
					pthread_detach(thread_bloqueo_IO);
					break;
				case ABRIR_ARCHIVO:
					log_info(logger, "Paso por Abrir_Archivo");
					buffer=desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);

					log_info(logger, "PID: %i - Abrir Archivo: %s", pcb->pid, pcb->arch_a_abrir);

					if(contiene_archivo(pcb,pcb->arch_a_abrir)){
						//enviar mensaje al cpu que debe continuar por que se bloqueo
						enviar_pcb_a(pcb,conexion_cpu,DETENER);
						sem_post(&sem_habilitar_exec);
					} else {
						//enviar mensaje al filesystem a ber si existe el archivo
						enviar_pcb_a(pcb,conexion_filesystem,ABRIR_ARCHIVO);//TODO: memoryLeak

					}

					break;
				
				case CERRAR_ARCHIVO:

					buffer=desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					log_info(logger, "PID: <%d> - Cerrar Archivo: <%s>",pcb->pid,pcb->arch_a_abrir);
					cerrar_archivo(pcb);

					enviar_pcb_a(pcb,conexion_cpu,CONTINUAR);

				    break;

				case ACTUALIZAR_PUNTERO:

					buffer=desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);

					actualizar_puntero(pcb);
					log_info(logger, "PID: <%d> - Actualizar puntero Archivo: <%s> - Puntero <%d>",pcb->pid,pcb->arch_a_abrir,pcb->posicion);

					enviar_pcb_a(pcb,conexion_cpu,CONTINUAR);//TODO: memoryLeak


					break;
				
				case LEER_ARCHIVO:

					buffer=desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					pcb->posicion = buscar_puntero(pcb);
					sem_wait(&mutex_compresion);
					log_info(logger, "PID: <%d> - Leer Archivo: <%s> - Puntero <%d> - Dirección Memoria <%d> - Tamaño <%d>",pcb->pid,pcb->arch_a_abrir,pcb->posicion,pcb->df_fs,pcb->cant_bytes);
					log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCK>",pcb->pid);
					enviar_pcb_a(pcb,conexion_filesystem,LEER_ARCHIVO);
					enviar_pcb_a(pcb,conexion_cpu,DETENER);
					sem_post(&sem_habilitar_exec);
					break;
				
				case ESCRIBIR_ARCHIVO:
					log_info(logger, "Paso por ESCRIBIR_ARCHIVO");
					buffer=desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					pcb->posicion = buscar_puntero(pcb);
					sem_wait(&mutex_compresion);
					log_info(logger, "PID: <%d> - Escribir Archivo: <%s> - Puntero <%d> - Dirección Memoria <%d> - Tamaño <%d>",pcb->pid,pcb->arch_a_abrir,pcb->posicion,pcb->df_fs,pcb->cant_bytes);
					log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCK>",pcb->pid);
					enviar_pcb_a(pcb,conexion_filesystem,ESCRIBIR_ARCHIVO);//TODO: memoryLeak

					enviar_pcb_a(pcb,conexion_cpu,DETENER);//TODO: memoryLeak

					sem_post(&sem_habilitar_exec);
					break;
				
				case MODIFICAR_TAMANIO:
					//log_info(logger, "Paso por modificar_Archivo");
					buffer=desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					sem_wait(&mutex_compresion);
					//modificar_tamanio(pcb);
					enviar_pcb_a(pcb, conexion_filesystem, MODIFICAR_TAMANIO);//TODO: memoryLeak
					
					enviar_pcb_a(pcb, conexion_cpu, DETENER);//TODO:memoryLeak
					//log_info(logger, "PID: <%d> - Bloqueado por:<%s>",pcb->pid,pcb->arch_a_abrir);
					log_info(logger, "PID: <%d> - Archivo: <%s> - Tamaño: <%d>",pcb->pid,pcb->arch_a_abrir,pcb->dat_tamanio);
					log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCK>",pcb->pid);
					sem_post(&sem_habilitar_exec);
					//enviar_pcb_a(pcb,conexion_cpu,DETENER);
					break;

				case CREAR_SEGMENTO:
					buffer = desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);

					log_info(logger,"PID: <%d> - Crear Segmento - Id: <%d> - Tamaño: <%d>",pcb->pid,pcb->dat_seg,pcb->dat_tamanio);

					enviar_crear_segmento(pcb->pid,pcb->dat_seg,pcb->dat_tamanio);

					analizar_resultado(pcb,paquete,buffer);

					break;
				case ELIMINAR_SEGMENTO:
					buffer = desempaquetar(paquete, conexion_cpu);
					pcb = deserializar_pcb(buffer);

					log_info(logger, "PID: <%d> - Eliminar Segmento - Id Segmento: <%d>",pcb->pid, pcb->dat_seg);
					enviar_eliminar_segmento(pcb->pid,pcb->dat_seg,pcb->dat_tamanio);
					//recv(conexion_memoria, &result, sizeof(uint32_t), MSG_WAITALL);

					analizar_resultado(pcb,paquete,buffer);
					//enviar_datos_a_memoria(pcb, conexion_memoria, ELIMINAR_SEGMENTO);
					break;
				case FINALIZAR:
					log_info(logger,"Paso por finzalizar");
					sem_post(&sem_habilitar_exec);
					sem_post(&sem_multiprogramacion);
					buffer = desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>",pcb->pid);
					//log_info(logger,"El pcb [%i] ha sido finalizado",pcb->pid);
					//mostrar_registro(pcb);
					log_info(logger,"Finaliza el proceso <%d> - Motivo: <SUCCESS>",pcb->pid);
					//enviar_mensaje("Tu proceso ha finalizado",pcb->conexion_consola);
					enviar_eliminar_proceso(pcb->pid);
					c = FINALIZAR;
					send(pcb->conexion_consola,&c,sizeof(op_code),0);

					break;
				case SEG_FAULT:
					log_info(logger,"Paso por finzalizar");
					sem_post(&sem_habilitar_exec);
					sem_post(&sem_multiprogramacion);
					buffer = desempaquetar(paquete,conexion_cpu);
					pcb = deserializar_pcb(buffer);
					log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>",pcb->pid);
					//log_info(logger,"El pcb [%i] ha sido finalizado",pcb->pid);
					//mostrar_registro(pcb);
					log_info(logger,"Finaliza el proceso <%d> - Motivo: <SEG_FAULT>",pcb->pid);
					//enviar_mensaje("Tu proceso ha finalizado",pcb->conexion_consola);
					enviar_eliminar_proceso(pcb->pid);
					c = FINALIZAR;
					send(pcb->conexion_consola,&c,sizeof(op_code),0);

					break;
				case -1:
					log_error(logger, "el cliente se desconecto. Terminando servidor");
					log_destroy(logger);
					config_destroy(config);
					close(conexion_cpu);
					close(server_fd);

					//return EXIT_FAILURE;
					exit(1);
				default:
					log_warning(logger,"Operacion desconocida. No quieras meter la pata");
					log_error(logger, "el cliente se desconecto. Terminando servidor");
					log_destroy(logger);
					config_destroy(config);
					close(conexion_cpu);
					close(server_fd);

					//return EXIT_FAILURE;
					exit(1);
					break;
			}
	}
	free(paquete);
	free(paquete->buffer);
	free(buffer);
}

void* atender_fileSystem(void){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	t_pcb* pcb;
	t_buffer* buffer;
	info_arch* arch_proc;
	//int result;

	//t_cola proc_bloqueados=queue_create();
    //int df;
	while(1){
		int cod_op = recibir_operacion(conexion_filesystem);
			switch (cod_op) {
				case DESBLOQUEADO:
					//log_info(logger,"Paso por desbloqueado");

					buffer = desempaquetar(paquete,conexion_filesystem);
					pcb = deserializar_pcb(buffer);
					sem_post(&mutex_compresion);
					log_info(logger,"PID: <%d> - Estado Anterior: <BLOCK> - Estado Actual: <READY>",pcb->pid);
					agregar_a_cola_ready(pcb);
					break;
				case EXISTE_ARCHIVO:
					log_info(logger,"El archivo ya existe");
					//enviar_pcb_a(pcb,conexion_cpu,CONTINUAR);
					buffer = desempaquetar(paquete,conexion_filesystem);
					pcb = deserializar_pcb(buffer);
					//ciclo = 0;
					crear_archivo(pcb->arch_a_abrir);
					//cod = CONTINUAR;
					arch_proc = malloc(sizeof(info_arch));//TODO: memoryLeak
					arch_proc->dir = pcb->arch_a_abrir;
					arch_proc->punt = 0;
					list_add(pcb->archivos_abiertos,arch_proc);
					enviar_pcb_a(pcb,conexion_cpu,CONTINUAR);

					break;
				case CREAR_ARCHIVO:
					//enviar para crear el archivo
					log_info(logger, "Paso por Crear_Archivo");
					buffer = desempaquetar(paquete,conexion_filesystem);
					pcb = deserializar_pcb(buffer);
					pcb->dat_tamanio=0;
					enviar_pcb_a(pcb,conexion_filesystem,CREAR_ARCHIVO);

					break;
				case ARCHIVO_CREADO:
					log_info(logger,"El archivo se creo");
					//enviar_pcb_a(pcb,conexion_cpu,CONTINUAR);
					buffer = desempaquetar(paquete,conexion_filesystem);
					pcb = deserializar_pcb(buffer);
					crear_archivo(pcb->arch_a_abrir);
					arch_proc = malloc(sizeof(info_arch));//TODO: memoryLeak
					arch_proc->dir = pcb->arch_a_abrir;
					arch_proc->punt = 0;
					list_add(pcb->archivos_abiertos,arch_proc);
					enviar_pcb_a(pcb,conexion_cpu,CONTINUAR);

					break;
				case -1:
					log_error(logger, "el cliente se desconecto. Terminando servidor");
					log_destroy(logger);
					config_destroy(config);
					close(conexion_cpu);
					close(server_fd);

					//return EXIT_FAILURE;
					exit(1);
				default:
					log_warning(logger,"Operacion desconocida. No quieras meter la pata");
					log_error(logger, "el cliente se desconecto. Terminando servidor");
					log_destroy(logger);
					config_destroy(config);
					close(conexion_cpu);
					close(server_fd);

					//return EXIT_FAILURE;
					exit(1);
					break;
			}
	}

	free(paquete);
	free(paquete->buffer);
	free(buffer);
	return NULL;
}

void atender_consolas(void* data){
	int cliente_fd = *((int*)data);
	t_list* lista;
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	t_buffer* buffer;
	while(1){

		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(logger,cliente_fd);
				break;
			case PAQUETE:
				lista = recibir_paquete(cliente_fd);
				log_info(logger, "Me llegaron los siguientes valores:\n");
				list_iterate(lista, (void*) iterator);
				break;

			case INSTRUCCIONES:
				buffer = desempaquetar(paquete,cliente_fd);

				t_pcb* pcb = crear_pcb(buffer,cliente_fd);

				log_info(logger,"EL PID QUE ME LLEGO ES %i",pcb->pid);//TODO: memoryLeak

				agregar_a_cola_new(pcb);

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
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				log_destroy(logger);
				config_destroy(config);
				close(cliente_fd);
				close(server_fd);
				free(data);
				//return EXIT_FAILURE;
				exit(1);
				break;
		}
	}

	list_destroy(lista);
	free(paquete->buffer);
	free(paquete);
	free(buffer);
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}



int agregar_pid(t_buffer* buffer){
	int id;
	int offset = 0;
	memcpy(&id, buffer->stream + offset, sizeof(int));
	offset += sizeof(int);
	return id;

}

void mostrar(t_instruccion* inst){
	log_info(logger,"La instruccion es %i",inst->nombre);
	list_iterate(inst->parametros, (void*) mostrar_parametro);
}

void mostrar_parametro(char* value){
	log_info(logger,"Parametro %s",value);
}

void mostrar_recurso(t_recurso* value){
	log_info(logger,"El recurso [%s] tiene [%i] instancias AHORA",value->nombre,value->instancias);
}

t_pcb* crear_pcb(t_buffer* buffer,int conexion_cliente){
	t_pcb* pcb = malloc(sizeof(t_pcb));

	pid++;
	pcb->pid = pid;

	t_list* lista_instrucciones = deserializar_lista_instrucciones(buffer);
	pcb->lista_instrucciones = lista_instrucciones;
	pcb->program_counter = 0;
	pcb->tabla_segmentos = list_create();
	int tamanio_seg_0;
	op_code cod = INICIALIZAR_ESTRUCTURA;
	send(conexion_memoria,&cod , sizeof(op_code), 0);
	recv(conexion_memoria, &tamanio_seg_0, sizeof(int), MSG_WAITALL);
	segmento* seg = malloc(sizeof(segmento));
	seg->id = 0;
	seg->direccion_base = 0;
	seg->tamanio = tamanio_seg_0;
	log_info(logger,"Semento id: %i, base: %i, tamanio: %i",seg->id,seg->direccion_base,seg->tamanio);
	list_add(pcb->tabla_segmentos,seg);
	pcb->archivos_abiertos = list_create();
	pcb->llegadaReady = 0;
	pcb->llegadaExec = 0;
	pcb->real_ant = 0;
	pcb->estimacion = datos.est_inicial;
	pcb->conexion_consola = conexion_cliente;
	pcb->dat_seg = 0;
	pcb->dat_tamanio = 0;
	pcb->arch_a_abrir = "Hola soy un archivo";
	pcb->cant_bytes = 0;
	pcb->df_fs = 0;
	pcb->posicion = 0;
	return pcb;
}



///////PLANIFICACION
void agregar_a_cola_new(t_pcb*pcb){
	sem_wait(&mutex_cola_new);
	queue_push(cola.cola_new,pcb);
	sem_post(&mutex_cola_new);
	//log_info(logger,"El proceso [%d] ingreso a la cola nuevo",pcb->pid);
	log_info(logger,"Se crea el proceso <%d> en NEW",pcb->pid);
	sem_post(&sem_nuevo);

}

t_pcb* quitar_de_cola_new(){
	sem_wait(&mutex_cola_new);
	t_pcb* pcb = queue_pop(cola.cola_new);
	sem_post(&mutex_cola_new);
	log_info(logger,"El proceso [%d] fue quitado a la cola nuevo",pcb->pid);
	return pcb;
}


void agregar_a_cola_ready(t_pcb* pcb){
	log_info(logger,"El proceso [%d] fue agregado a la cola ready",pcb->pid);
	mostrar_list_ready();
	sem_wait(&mutex_cola_ready);
	pcb->llegadaReady = time(NULL);
	log_info(logger,"Llego en ready en %i",pcb->llegadaReady);
	queue_push(cola.cola_ready_fifo,pcb);
	sem_post(&mutex_cola_ready);
	sem_post(&sem_ready);
	//sem_post(&sem_habilitar_exec);
	//sem_post(&sem_exec);
	mostrar_cola(cola.cola_ready_fifo);
}

void mostrar_list_ready(){
	log_info(logger,"----------LISTA READY------------------");
	for(int i = 0; i < list_size(cola.cola_ready_fifo->elements);i++){
		t_pcb* pcb = list_get(cola.cola_ready_fifo->elements,i);
		log_info(logger,"[%d]",pcb->pid);
	}
	log_info(logger,"------------------------------------");
}

t_pcb* quitar_de_cola_ready(){
	//t_pcb* pcb = malloc(sizeof(t_pcb));
	sem_wait(&mutex_cola_ready);
	t_pcb* pcb = queue_pop(cola.cola_ready_fifo);
	sem_post(&mutex_cola_ready);
	log_info(logger,"El proceso [%d] fue quitado a la cola ready",pcb->pid);
	return pcb;
}

void* de_new_a_ready(void){

	//log_info(logger,"nuevo a ready");

	while(1){
		//sem_wait(&sem_avisar);
		//log_info(logger,"HOLAAAA");
		sem_wait(&sem_nuevo);
		while(!queue_is_empty(cola.cola_new)){
			sem_wait(&sem_multiprogramacion);
			t_pcb* pcb =quitar_de_cola_new();

			log_info(logger,"PID: %d - Estado Anterior: <NEW> - Estado Actual: <READY>",pcb->pid);

			agregar_a_cola_ready(pcb);
		}
	}
}


void* de_ready_a_ejecutar(void){
	log_info(logger,"de ready a ejecutar");
	while(1){
		switch(datos.algoritmo_planificacion){
			case FIFO:
				log_info(logger,"Paso por fifo");
				de_ready_a_ejecutar_fifo();
				break;
			case HRRN:
				log_info(logger,"Paso por hrrn");
				de_ready_a_ejecutar_hrrn();
				break;
			default:
				log_warning(logger,"Algo esta ocurriendo en ready a ejectar");
				break;
		}
	}

}

void de_ready_a_ejecutar_fifo(void){
	while(1){
		//sem_wait(&sem_habilitar_exec);
		//log_info(logger,"HOLAAAAA");
		//sem_wait(&sem_ready);
		sem_wait(&sem_ready);
		while(!queue_is_empty(cola.cola_ready_fifo)){
			sem_wait(&sem_habilitar_exec);
			t_pcb* pcb = quitar_de_cola_ready();
			pcb = actualizar_de_la_tabla_general(pcb);
			log_info(logger,"“PID: %d - Estado Anterior: READY - Estado Actual: EXEC”",pcb->pid);
			pcb->llegadaExec = time(NULL);
			enviar_pcb_a(pcb,conexion_cpu,EJECUTAR);//TODO: memoryLeak

			//liberar_pcb(pcb);
		}
	}
}

void de_ready_a_ejecutar_hrrn(void){
	while(1){
		sem_wait(&sem_ready);
			while(!queue_is_empty(cola.cola_ready_fifo)){
			sem_wait(&sem_habilitar_exec);
			//replanificar();
			t_ahora = time(NULL);
			list_sort(cola.cola_ready_fifo->elements,comparador_hrrn);
			t_pcb* pcb = quitar_de_cola_ready();
			pcb = actualizar_de_la_tabla_general(pcb);
			log_info(logger,"“PID: %d - Estado Anterior: READY - Estado Actual: EXEC”",pcb->pid);
			//log_info(logger,"La hora en el clock es %ld",hora);
			pcb->llegadaExec = time(NULL);
			enviar_pcb_a(pcb,conexion_cpu,EJECUTAR);
		}
	}
}
/*
void replanificar(void){
    t_list* lista_desordenada = malloc(sizeof(t_list));


		t_pcb* pcb = malloc(sizeof(t_pcb));

		int est = pcb->estimacion*(1-datos.alfa) + pcb->real_ant*datos.alfa;
		int tiempoEspera = clock() - pcb->llegadaReady;
		int est_hrrn = (est+tiempoEspera)/est;

		t_est est_proc = malloc(sizeof(t_est));
		est_proc->pcb = pcb;
		est_proc->est = est_hrrn;

		list_add(lista_desordenada, est_proc);

	reorganizar_lista_hrrn(lista_desordenada);

	while(!list_is_empty(lista_desordenada)){
		//Saco el pcb de la lista y ordeno la cola
	}

	free(lista_desordenada);
}
*/
bool comparador_hrrn(void* data1,void* data2){
	t_pcb* pcb1 = ((t_pcb*) data1);
	t_pcb* pcb2 = ((t_pcb*) data2);
	//bool flag = malloc(sizeof(bool));
	bool flag = true;
	//int t_ahora = clock();
	float t1 = (t_ahora - pcb1->llegadaReady)*1000;
	float t2 = (t_ahora - pcb2->llegadaReady)*1000;
	float v1 = (pcb1->estimacion+t1)/pcb1->estimacion;
	float v2 = (pcb2->estimacion+t2)/pcb2->estimacion;
	log_info(logger,"El pcb[%i] tiene [%f] y el pcb[%i] tiene [%f]",pcb1->pid,v1,pcb2->pid,v2);
	if(v1 >= v2){
		flag = true;
	} else {
		flag = false;
	}
	return flag;
	//free(pcb1);
	//free(pcb2);
}

/*
void reorganizar_lista_hrrn(t_list* lista_desorganizado){
    //Obtener de la lista la estimacion y convertirlo en un vector de enteros

    // Utilizo el algoritmo 'INSERTION SORT'

    for(int i = 1; i < v_est.length; i++){
    	int j = i;
    	int marcado = v_est[i];

		while(j>0 && v_est[j-1] > marcado){
			v_est[j] = v_est[j-1];
			j--;
		}
		v_est[j] = marcado;
    }

    //Transformar el vector en
}
*/

algoritmo devolver_algoritmo(char*nombre){
	algoritmo alg;
	if(strcmp(nombre,"FIFO")==0) alg=FIFO;
	else if(strcmp(nombre,"HRRN")==0) alg=HRRN;
	else log_info(logger,"algoritmo desconocido");
	return alg;
}

void mostrar_cola(t_queue*cola){
	for(int i=0;i<queue_size(cola);i++){
		int*id=list_get(cola->elements,i);
		//log_info(logger,"%d PID: %d",i+1,*id);
		log_info(logger,"Cola Ready: ");
		log_info(logger, "%d - ",*id);
	}
	log_info(logger,"-----------------------");
}

//------------------Encarse de los recursos compartidos-------

void* iniciar_recurso(void* data){
	t_recurso* recurso = ((t_recurso*) data);
	log_info(logger,"INICIANDO EL RECURSO [%s] con [%i] instancias",recurso->nombre,recurso->instancias);
	while(1){
		sem_wait(&recurso->sem_recurso);
		t_pcb* pcb = queue_pop(recurso->cola_recurso);
		pcb->llegadaReady = time(NULL);
		agregar_a_cola_ready(pcb);

	}
}

void mostrar_lista_ready(){

}



void ejecutar_wait(t_pcb* pcb){
	t_list_iterator* iterador = list_iterator_create(lista_recursos);
	int index = pcb->program_counter-1;
	t_instruccion* instruccion = list_get(pcb->lista_instrucciones,index);
	char* nombre = list_get(instruccion->parametros,0);
	int j = 0;
	int encontroResultado = 1;
	while(list_iterator_has_next(iterador)){
		t_recurso* recu = (t_recurso*)list_iterator_next(iterador);
		//log_info(logger,"PID: <%d>, Pasa por [%s] con [%i] instancias y el recurso del pcb es %s", pcb->pid,recu->instancias,nombre);

		if(strcmp(nombre,recu->nombre) == 0){
			recu->instancias--;
			log_info(logger,"PID: <%d> - Wait: [%s] con [%i] instancias",pcb->pid,recu->nombre,recu->instancias);
			encontroResultado = 0;
			list_replace(lista_recursos,j,recu);
			//uint32_t resultOk;
			op_code resultOk;
			if(recu->instancias < 0){
				pcb->real_ant = time(NULL) - pcb->llegadaExec;
				pcb->estimacion = datos.alfa*pcb->real_ant + (1 - datos.alfa)*pcb->estimacion;
				log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCK>",pcb->pid);
				queue_push(recu->cola_recurso,pcb);
				resultOk = DETENER;
				send(conexion_cpu, &resultOk, sizeof(op_code), 0);
				sem_post(&sem_habilitar_exec);
				log_info(logger,"El recurso [%s] se bloqueo",recu->nombre);
				break;
			} else {
				op_code resultOk = CONTINUAR;
				send(conexion_cpu, &resultOk, sizeof(op_code), 0);
				log_info(logger,"El recurso [%s] sigue",recu->nombre);
				break;

			}
		}
		j++;
	}

	//list_iterate(lista_recursos, (void*) mostrar_recurso);

	if(encontroResultado == 1){
		log_info(logger,"El recurso no existe, el proceso finaliza");
		//Liberar datso en memoria
		op_code resultOk = CONTINUAR;
		send(conexion_cpu, &resultOk, sizeof(op_code), 0);
		log_info(logger,"Finaliza el proceso <%d> - Motivo: <SIN_RECURSOS>",pcb->pid);
		enviar_mensaje("Tu proceso ha finalizado por que no existe el recurso",pcb->conexion_consola);
		sem_post(&sem_habilitar_exec);
	}

	//free(recu); TODO: nose si liberarlo
	free(nombre);
	list_iterator_destroy(iterador);
}

void ejecutar_signal(t_pcb* pcb){
	t_list_iterator* iterador = list_iterator_create(lista_recursos);
	int index = pcb->program_counter-1;
	t_instruccion* instruccion = list_get(pcb->lista_instrucciones,index);
	char* nombre = list_get(instruccion->parametros,0);
	int j = 0;
	int encontroResultado = 1;

	while(list_iterator_has_next(iterador)){
		t_recurso* recu = (t_recurso*)list_iterator_next(iterador);

		if(strcmp(nombre,recu->nombre) == 0){
			recu->instancias++;
			encontroResultado = 0;
			log_info(logger,"PID: <%d> - Signal: [%s] con [%i] instancias",pcb->pid,recu->nombre,recu->instancias);
			list_replace(lista_recursos,j,recu);
			if(!queue_is_empty(recu->cola_recurso)){
				sem_post(&recu->sem_recurso);
				//break;
			}
			uint32_t resultOk = 1;
			send(conexion_cpu, &resultOk, sizeof(uint32_t), 0);
			log_info(logger,"El recurso [%s] sigue",recu->nombre);
		}
		j++;
	}

	//list_iterate(lista_recursos, (void*) mostrar_recurso);
	if(encontroResultado == 1){
		log_info(logger,"El recurso no existe, el proceso finaliza");
		//Liberar datso en memoria
		uint32_t resultOk = 0;
		send(conexion_cpu, &resultOk, sizeof(uint32_t), 0);
		//log_info(logger,"El pcb [%i] ha sido finalizado",pcb->pid);
		log_info(logger,"Finaliza el proceso <%d> - Motivo: <SIN_RECURSOS>",pcb->pid);
		//enviar_mensaje("Tu proceso ha finalizado por que no existe el recurso",pcb->conexion_consola);
		enviar_mensaje("Tu proceso ha finalizado por que no existe el recurso",pcb->conexion_consola);
		sem_post(&sem_habilitar_exec);
	}

	//free(recu); TODO: ver si liberar
	free(instruccion);
	free(nombre);
	list_iterator_destroy(iterador);
}

void* ejecutar_IO(void* dato){
	t_pcb* pcb = ((t_pcb*) dato);
	int index = pcb->program_counter-1;
	//pcb->program_counter--;
	t_instruccion* instruccion = list_get(pcb->lista_instrucciones,index);

	int tiempoBloqueado = atoi(list_get(instruccion->parametros,0));
	log_info(logger,"PID: <%d> - Bloqueado por: <IO>",pcb->pid);
	log_info(logger,"PID: <%d> - Ejecuta IO: <%d>",pcb->pid,tiempoBloqueado);

	sleep(tiempoBloqueado);
	agregar_a_cola_ready(pcb);
	pthread_cancel(pthread_self());

	return NULL;
	free(instruccion);
}

void mostrar_registro(t_pcb* pcb){


	log_info(logger,"En el registro AX esta los caracteres: %s",pcb->registro.AX);
	log_info(logger,"En el registro BX esta los caracteres: %s",pcb->registro.BX);
	log_info(logger,"En el registro CX esta los caracteres: %s",pcb->registro.CX);
	log_info(logger,"En el registro DX esta los caracteres: %s",pcb->registro.DX);
	log_info(logger,"En el registro EAX esta los caracteres: %s",pcb->registro.EAX);
	log_info(logger,"En el registro EBX esta los caracteres: %s",pcb->registro.EBX);
	log_info(logger,"En el registro ECX esta los caracteres: %s",pcb->registro.ECX);
	log_info(logger,"En el registro EDX esta los caracteres: %s",pcb->registro.EDX);
	log_info(logger,"En el registro RAX esta los caracteres: %s",pcb->registro.RAX);
	log_info(logger,"En el registro RBX esta los caracteres: %s",pcb->registro.RBX);
	log_info(logger,"En el registro RCX esta los caracteres: %s",pcb->registro.RCX);
	log_info(logger,"En el registro RDX esta los caracteres: %s",pcb->registro.RDX);

}

//--------------------------------- SERIALIZACION --------------------------------------//
void enviar_crear_segmento(int pid, int id_seg,int tamanio_seg){
	t_paquete* paquete = malloc(sizeof(t_paquete)); //TODO: memoryLeak
	paquete->codigo_operacion = CREAR_SEGMENTO;
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int offset = 0;
	buffer->size = sizeof(int)*3;
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &pid, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &id_seg, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &tamanio_seg, sizeof(int));
	offset += sizeof(int);

	paquete->buffer = buffer;

	enviar_paquete(paquete,conexion_memoria);

	free(buffer->stream);
	free(buffer);
	free(paquete);
}

void enviar_eliminar_segmento(int pid, int id_seg,int tamanio_seg){
	t_paquete* paquete = malloc(sizeof(t_paquete)); //TODO: memoryLeak
	paquete->codigo_operacion = ELIMINAR_SEGMENTO;
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int offset = 0;
	buffer->size = sizeof(int)*2;
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &pid, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &id_seg, sizeof(int));
	offset += sizeof(int);

	paquete->buffer = buffer;

	enviar_paquete(paquete,conexion_memoria);

	free(paquete);
	free(buffer->stream);
	free(buffer);
}

void enviar_eliminar_proceso(int pid){
	t_paquete* paquete = malloc(sizeof(t_paquete));//TODO: memoryLeak
	paquete->codigo_operacion = FINALIZAR;
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int offset = 0;
	buffer->size = sizeof(int);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &pid, sizeof(int));
	offset += sizeof(int);

	paquete->buffer = buffer;

	enviar_paquete(paquete,conexion_memoria);

	free(paquete);
	free(buffer->stream);
	free(buffer);
}

void analizar_resultado(t_pcb* pcb,t_paquete* paquete,t_buffer* buffer){
	int seguir = 1;
	while(seguir == 1){
		int cod_op = recibir_operacion(conexion_memoria);
		switch(cod_op){
			case CREACION_EXITOSA:
				log_info(logger,"Creacion exitosa");
				buffer = desempaquetar(paquete,conexion_memoria);
				t_list* lista = deserializar_tabla_actualizada(buffer);//TODO:memoryLeak
				pcb->tabla_segmentos = lista;
				enviar_pcb_a(pcb,conexion_cpu,EJECUTAR);
				seguir = 0;

				free(lista);
				break;
			case REALIZAR_COMPACTACION:
				log_info(logger,"Compactación: <Se solicitó compactación>");
				log_info(logger,"Compactación: <Esperando Fin de Operaciones de FS>");
				sem_wait(&mutex_compresion);

				op_code codigo = REALIZAR_COMPACTACION;
				send(conexion_memoria,&codigo,sizeof(op_code),0);
				break;
			case COMPACTAR:
				log_info(logger,"Creando de nuevo");
				buffer = desempaquetar(paquete,conexion_memoria);
				deserializar_tabla_general_actualizada(buffer);
				hayQueActualizar = 1;
				//mostrar_tablas_de_segmentos();
				log_info(logger,"Se finalizó el proceso de compactación");
				pcb = actualizar_de_la_tabla_general(pcb);
				enviar_crear_segmento(pcb->pid,pcb->dat_seg,pcb->dat_tamanio);
				sem_post(&mutex_compresion);
				break;
			case SIN_MEMORIA:
				log_info(logger,"No hay mas espacio en memoria, se termina el proceso");

				log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <EXIT>",pcb->pid);

				log_info(logger,"Finaliza el proceso <%d> - Motivo: <OUT_OF_MEMORY>",pcb->pid);


				enviar_eliminar_proceso(pcb->pid);
				op_code c = FINALIZAR;
				send(conexion_cpu,&c,sizeof(op_code),0);
				seguir = 0;
				send(pcb->conexion_consola,&c,sizeof(op_code),0);
				sem_post(&sem_habilitar_exec);
				sem_post(&sem_multiprogramacion);
				break;
			}
	}

}

t_list* deserializar_tabla_actualizada(t_buffer* buffer){
	int tamanio_tabla = 0;
	int offset = 0;
	memcpy(&tamanio_tabla,buffer->stream + offset,sizeof(int));
	offset += sizeof(int);
	t_list* lista = list_create();
	for(int i = 0; i < tamanio_tabla; i++){

		segmento* seg = malloc(sizeof(segmento));//TODO: memoryLeak
		memcpy(&(seg->id),buffer->stream + offset,sizeof(int));
		offset += sizeof(int);
		memcpy(&(seg->direccion_base),buffer->stream + offset,sizeof(int));
		offset += sizeof(int);
		memcpy(&(seg->tamanio),buffer->stream + offset,sizeof(int));
		offset += sizeof(int);
		list_add(lista,seg);
		//log_info(logger,"Segmento ID %i BASE %i TAMANIO %i",seg->id,seg->direccion_base,seg->tamanio);
	}

	return lista;
}

void deserializar_tabla_general_actualizada(t_buffer* buffer){
	int tamanio_tabla;
	int offset = 0;
	memcpy(&tamanio_tabla,buffer->stream + offset,sizeof(int));
	offset += sizeof(int);
	//t_list* lista = list_create();
	for(int i = 0; i < tamanio_tabla; i++){
		tabla_de_proceso* proc = malloc(sizeof(tabla_de_proceso));//TODO: memoryLeak
		memcpy(&proc->pid,buffer->stream + offset,sizeof(int));
		offset += sizeof(int);
		proc->segments = list_create();
		int tam_seg;


		memcpy(&tam_seg,buffer->stream + offset,sizeof(int));
		offset += sizeof(int);

		for(int j = 0; j < tam_seg;j++){
			segmento* seg = malloc(sizeof(segmento));//TODO: memoryLeak
			memcpy(&(seg->id),buffer->stream + offset,sizeof(int));
			offset += sizeof(int);
			memcpy(&(seg->direccion_base),buffer->stream + offset,sizeof(int));
			offset += sizeof(int);
			memcpy(&(seg->tamanio),buffer->stream + offset,sizeof(int));
			offset += sizeof(int);

			list_add(proc->segments,seg);
		}

		list_add(tabla_general,proc);

		//list_add(lista,seg);
		//log_info(logger,"Segmento ID %i BASE %i TAMANIO %i",seg->id,seg->direccion_base,seg->tamanio);
	}


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

t_pcb* actualizar_de_la_tabla_general(t_pcb* pcb){
	if(hayQueActualizar == 1){
		for(int i = 0; i < list_size(tabla_general);i++){
			tabla_de_proceso* proc = list_get(tabla_general,i);
			if(pcb->pid == proc->pid){
				pcb->tabla_segmentos = proc->segments;
			}
		}
	}
	return pcb;
}

int deserializar_df(t_buffer* buffer){
	int df;

	int offset = buffer->size - sizeof(int);

	memcpy(&df, buffer->stream + offset, sizeof(int));
	offset += sizeof(int);

	return df;
}

char* deserializar_nombreArchivo(t_buffer* buffer){
	char* nombreArchivo = malloc(sizeof(char*));

	int offset = buffer->size - sizeof(char*) - sizeof(int);

	memcpy(&nombreArchivo, buffer->stream + offset, strlen(nombreArchivo) + 1);

	return nombreArchivo;
	free(nombreArchivo);
}
//-------------------------------------ADICIONALES----------------------------------//
int contiene_archivo(t_pcb* pcb, char* nombreArchivo){
	int band = 0;
	for(int i = 0; i < list_size(lista_archivos_abiertos); i++){
		t_archivo* archi = list_get(lista_archivos_abiertos,i);

		if(strcmp(nombreArchivo,archi->directorio) == 0){
			info_arch* arch_proc = malloc(sizeof(info_arch));//TODO: memoryLeak
			arch_proc->dir = nombreArchivo;
			arch_proc->punt = 0;
			list_add(pcb->archivos_abiertos,arch_proc);
			queue_push(archi->cola_archivo,pcb);
			log_info(logger,"PID: <%d> - Estado Anterior: <EXEC> - Estado Actual: <BLOCK>",pcb->pid);
			band = 1;
		}
	}
	return band;
}

void crear_archivo(char* nombreArchivo){
	t_archivo* archi = malloc(sizeof(t_archivo));//TODO: memoryLeak
	archi->directorio = nombreArchivo;
	archi->cola_archivo = queue_create();
	sem_init(&archi->sem_archivo,0,0);

	pthread_create(&thread_archivo_abiertos,NULL,(void*) iniciar_archivo,archi);
	list_add(lista_archivos_abiertos,archi);
}

void* iniciar_archivo(void* data){
	t_archivo* archi = ((t_archivo*) data);
	log_info(logger,"INICIANDO EL ARCHIVO [%s] ",archi->directorio);
	while(1){
		sem_wait(&archi->sem_archivo);
		t_pcb* pcb = queue_pop(archi->cola_archivo);
		agregar_a_cola_ready(pcb);

	}
}

void cerrar_archivo(t_pcb* pcb){
	char* nombreArchivo = pcb->arch_a_abrir;
	for(int i = 0; i < list_size(lista_archivos_abiertos);i++){
		t_archivo* archi = list_get(lista_archivos_abiertos,i);
		if(strcmp(archi->directorio,nombreArchivo) == 0){
			if(queue_size(archi->cola_archivo) == 0){
				free(archi);//TODO: ver si esta bien
				list_remove(lista_archivos_abiertos,i);
			} else {
				sem_post(&archi->sem_archivo);
			}
		}
	}
	free(nombreArchivo);
}

void actualizar_puntero(t_pcb* pcb){
	char* nombreArchivo = pcb->arch_a_abrir;
	for(int i = 0; i < list_size(pcb->archivos_abiertos);i++){
		info_arch* archi = list_get(pcb->archivos_abiertos,i); //TODO: memoryLeak

		if(strcmp(archi->dir,nombreArchivo)==0){
			archi->punt = pcb->posicion;
			list_replace(pcb->archivos_abiertos,i,archi);
			break;
		}
	}
	free(nombreArchivo);
}

int buscar_puntero(t_pcb* pcb){
	int puntero;
	char* nombreArchivo = pcb->arch_a_abrir;
		for(int i = 0; i < list_size(pcb->archivos_abiertos);i++){
			info_arch* archi = list_get(pcb->archivos_abiertos,i);//TODO: memoryLeak

			if(strcmp(archi->dir,nombreArchivo)==0){
				puntero = archi->punt;
				break;
			}

		}
	return puntero;
	free(nombreArchivo);
}

void modificar_tamanio(t_pcb* pcb){
	sem_wait(&mutex_fs);
	int result;
	enviar_pcb_a(pcb, conexion_filesystem, MODIFICAR_TAMANIO);
	recv(conexion_filesystem,&result,sizeof(int),MSG_WAITALL);
	if(result==0){
		log_info(logger, "Ocurrio un error al modificar el tamanio. PID: %i - Archivo: %s", pcb->pid, pcb->arch_a_abrir);
		enviar_pcb_a(pcb, conexion_cpu, DETENER);
	}
	sem_post(&mutex_fs);
}
