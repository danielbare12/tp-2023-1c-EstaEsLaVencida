/*
 ============================================================================
 Name        : cpu.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

t_log* logger;
t_config* config;
datos_config datos;
int server_fd;
int conexion_memoria;
pthread_t hilo_conexion_kernel;
pthread_t hilo_conexion_memoria;
t_pcb* ejecutar_pcb(t_pcb* pcb,int conexion_kernel);
t_instruccion* fetch(t_pcb* pcb);
int decode(t_instruccion* instruccion);
void execute(t_instruccion* instruccion,t_pcb* pcb,int conexion_kernel);
void mostrar(t_instruccion* inst);
void mostrar_parametro(char* value);
registros_pos devolver_registro(char* registro);
void insertar(t_pcb* pcb, registros_pos pos,char* caracteres);
void mostrar_registro(t_pcb* pcb);
int band_ejecutar;
int obtener_direccion_fisica(uint32_t dir_logica,t_pcb*pcb,int tam_dato);
char*devolver_dato(t_pcb* pcb,registros_pos registro);
void enviar_datos_para_escritura(int dir, char* valor,int tamanio);
int size_registro(registros_pos registro);
void enviar_datos_para_lectura(int dir,int tamanio);
char* devolver_parametros(t_list* lista);
char* op_instruct_to_string(op_instruct instr);
//void enviar_crear_segmento(int id_seg,int tamanio_seg,int conexion);
//mmu
//t_dl* obtenerDL(uint32_t dir_logica,t_pcb*pcb);
//t_df* traducirDLaDF(t_dl* dl,t_pcb* pcb,char*accion);
void enviar_datos_para_op_fs(t_pcb* pcb, envio_instr instrAEnviar, op_code codigo, int conexion);

int main(int argc, char** argv) {
	if (argc < 2) {
		printf ("se deben especificar la ruta del archivo de pseudocodigo");
		return EXIT_FAILURE;
	}
	logger = iniciar_logger("cpu.log","CPU");;
	config = iniciar_config(argv[1]);

	datos.ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	datos.puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	datos.puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	datos.ret_instruccion = config_get_int_value(config,"RETARDO_INSTRUCCION");
	datos.tam_max_segmento = config_get_int_value(config,"TAM_MAX_SEGMENTO");
	conexion_memoria = crear_conexion(datos.ip_memoria,datos.puerto_memoria);
	pthread_create(&hilo_conexion_kernel,NULL,(void*) atender_kernel,NULL);
	//pthread_create(&hilo_conexion_memoria,NULL,(void*) atender_memoria,NULL);

	pthread_join(hilo_conexion_kernel,NULL);
	//pthread_join(hilo_conexion_memoria,NULL);
	log_destroy(logger);
	config_destroy(config);
	close(server_fd);
	close(conexion_memoria);
	return EXIT_SUCCESS;
}

void* atender_kernel(void){
	server_fd = iniciar_servidor(logger,datos.puerto_escucha);
	log_info(logger, "CPU listo para recibir al kernel");
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int *cliente_fd = esperar_cliente(logger,server_fd);
	while(1){
		t_list* lista;
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

				case EJECUTAR:
					log_info(logger,"Paso por ejecutar");
					buffer = desempaquetar(paquete,*cliente_fd);
					t_pcb* pcb = deserializar_pcb(buffer);
					log_info(logger,"El PID ES %i",pcb->pid);
					//list_iterate(pcb->lista_instrucciones, (void*) mostrar);
					ejecutar_pcb(pcb,*cliente_fd);


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
					log_destroy(logger);
					config_destroy(config);
					close(*cliente_fd);
					close(server_fd);
					break;
			}
		}
	}
}

void* atender_memoria(void){
	conexion_memoria = crear_conexion(datos.ip_memoria,datos.puerto_memoria);
	enviar_mensaje("Hola te saludo desde el cpu",conexion_memoria);
	while(1){
		t_list* lista;
		while(1){
			int cod_op = recibir_operacion(conexion_memoria);
			switch (cod_op) {
				case MENSAJE:
					recibir_mensaje(logger,conexion_memoria);
					break;
					case PAQUETE:
					lista = recibir_paquete(conexion_memoria);
					log_info(logger, "Me llegaron los siguientes valores:\n");
					list_iterate(lista, (void*) iterator);
					break;
				case -1:
					log_error(logger, "el cliente se desconecto. Terminando servidor");
					log_destroy(logger);
					config_destroy(config);
					close(conexion_memoria);
					//return EXIT_FAILURE;
					exit(1);
				default:
					log_warning(logger,"Operacion desconocida. No quieras meter la pata");
					break;
			}
		}
	}
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}

void mostrar(t_instruccion* inst){
	log_info(logger,"La instruccion es %i",inst->nombre);
	list_iterate(inst->parametros, (void*) mostrar_parametro);
}

void mostrar_parametro(char* value){
	log_info(logger,"Parametro %s",value);
}

t_pcb* ejecutar_pcb(t_pcb* pcb,int conexion_kernel){


	t_instruccion* instruccion;

	do{

		instruccion = fetch(pcb);
			if(decode(instruccion) == 1){
				log_info(logger,"Instruccion set");
				usleep(datos.ret_instruccion*1000);

			}
			execute(instruccion,pcb,conexion_kernel);
	}while(band_ejecutar == 0);

	band_ejecutar = 0;


	log_info(logger,"TERMINO LA EJECUCION");
	return pcb;
}

t_instruccion* fetch(t_pcb* pcb){
	t_instruccion* instruccion = list_get(pcb->lista_instrucciones,pcb->program_counter);
	pcb->program_counter+=1;
	log_info(logger,"Program COutnter: %i",pcb->program_counter);
	return instruccion;
}

int decode(t_instruccion* instruccion){
	int acceso = 0;

	if(instruccion->nombre == SET){
		acceso = 1;
	}

	return acceso;
}

void execute(t_instruccion* instruccion,t_pcb* pcb,int conexion_kernel){
	uint32_t result;
	int registro;
	int df;
	int dl;
	int cod_op;
	//int cant_bytes;
	//envio_instr instrAEnviar;
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	t_buffer* buffer;
	log_info(logger,"PID: <%d> - Ejecutando: %s - Parametros: %s",pcb->pid,op_instruct_to_string(instruccion->nombre),devolver_parametros(instruccion->parametros));
	switch(instruccion->nombre){
		case SET:
			//log_info(logger,"PID: <%d> - Ejecutando: <%d> - <%s>",pcb->pid,instruccion->nombre,devolver_parametros(instruccion->parametros));
			char* reg_des = list_get(instruccion->parametros,0);
			char* caracteres = list_get(instruccion->parametros,1);
			registros_pos pos = devolver_registro(reg_des);
			insertar(pcb,pos,caracteres);
			//log_info(logger,"En %s esta %s",reg_des,caracteres);
			//mostrar_registro(pcb);
			break;
		case MOV_IN:
			//sleep(2);
			//log_info(logger,"Paso por mov_in");
			registro = devolver_registro(list_get(instruccion->parametros,0));
			dl=atoi(list_get(instruccion->parametros,1));
			df=obtener_direccion_fisica(dl,pcb,size_registro(registro));

			if(df < 0){
				enviar_pcb_a(pcb,conexion_kernel,SEG_FAULT);
				band_ejecutar = 1;
			} else {
				enviar_datos_para_lectura(df,size_registro(registro));
				char* valor = malloc(size_registro(registro)+1);
				recv(conexion_memoria, valor, size_registro(registro), MSG_WAITALL);
				insertar(pcb,registro,valor);
				//log_info(logger,"Se ingreso el valor %s",valor);
				int seg = floor(dl / datos.tam_max_segmento);
				log_info(logger, "PID: <%d> - Acción: <LEER> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%s>",pcb->pid,seg, df, valor);//TODO
				//free(valor);
			}

			//log_info(logger,"Se leyo el valor %s",pcb->registro.AX);
			/*if(result == 0){
				log_info(logger,"Algo salio mal");
				band_ejecutar = 1;
			} else {
				log_info(logger,"El programa sigue");
			}*/
			break;
		case MOV_OUT:
			//log_info(logger,"Paso por mov_out");
			registro = devolver_registro(list_get(instruccion->parametros,1));
			dl=atoi(list_get(instruccion->parametros,0));
			char *dato= devolver_dato(pcb,registro);//TODO: memoryLeak

			df=obtener_direccion_fisica(dl,pcb,size_registro(registro));

			if(df < 0) {
				enviar_pcb_a(pcb,conexion_kernel,SEG_FAULT);
				band_ejecutar = 1;
			} else {
				enviar_datos_para_escritura(df,dato,size_registro(registro));
				int seg = floor(dl / datos.tam_max_segmento);
				log_info(logger, "PID: <%d> - Acción: <ESCRIBIR> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%s>",pcb->pid,seg, df, dato);

				recv(conexion_memoria, &result, sizeof(int), MSG_WAITALL);
				if(result == 0){
					log_info(logger,"Algo salio bien");

				} else {
					log_info(logger,"El programa sigue");
				}
			}

			//free(dato);
			break;
		case F_OPEN:
			//log_info(logger,"Paso por F_OPEN");
			
			pcb->arch_a_abrir=list_get(instruccion->parametros, 0);
			//instrAEnviar.datos_aux=0;

			//enviar_datos_para_op_fs(pcb,instrAEnviar,SIN_PARAMETRO,conexion_kernel);
			enviar_pcb_a(pcb,conexion_kernel,ABRIR_ARCHIVO);
			//recv(conexion_kernel,&result,sizeof(uint32_t),MSG_WAITALL);

			cod_op = recibir_operacion(conexion_kernel);

			switch(cod_op){
				case CONTINUAR:
					buffer = desempaquetar(paquete,conexion_kernel);
					t_pcb* pcb2 = deserializar_pcb(buffer);
					pcb->archivos_abiertos = pcb2->archivos_abiertos;
					log_info(logger, "Sigue el programa");
					break;
				case DETENER:
					buffer = desempaquetar(paquete,conexion_kernel);
					pcb = deserializar_pcb(buffer);
					log_info(logger,"No se pudo abrir el archivo");
					band_ejecutar = 1;
					break;
				default:
					break;
			}
			/*if(result == 0){
				log_info(logger,"No se pudo abrir el archivo");
				band_ejecutar = 1;
			}else{
				log_info(logger, "Sigue el programa");
			}*/
			break;
		case F_CLOSE:
			//log_info(logger,"Paso por f_close");
			
			pcb->arch_a_abrir=list_get(instruccion->parametros,0);
			//instrAEnviar.datos_aux=atoi(list_get(instruccion->parametros,1));
			enviar_pcb_a(pcb,conexion_kernel,CERRAR_ARCHIVO);

			cod_op = recibir_operacion(conexion_kernel);

			switch(cod_op){
				case CONTINUAR:
					buffer = desempaquetar(paquete,conexion_kernel);
					pcb = deserializar_pcb(buffer);
					log_info(logger, "Sigue el programa");
					break;
				case DETENER:
					buffer = desempaquetar(paquete,conexion_kernel);
					pcb = deserializar_pcb(buffer);
					log_info(logger,"No se pudo abrir el archivo");
					band_ejecutar = 1;
					break;
				default:
					break;
			}
			//enviar_datos_para_op_fs(pcb,instrAEnviar,SIN_PARAMETRO,conexion_kernel);
			/*recv(conexion_kernel, &result, sizeof(uint32_t), MSG_WAITALL);
			if(result == 0){
				log_info(logger,"El proceso se bloqueo o no existe");
				band_ejecutar = 1;
			} else {
				log_info(logger,"El programa sigue");
			}*/
			break;
		case F_SEEK:
			//log_info(logger,"Paso por f_seek");
			
			pcb->arch_a_abrir=list_get(instruccion->parametros,0);
		    pcb->posicion=atoi(list_get(instruccion->parametros,1)); //Es la posicion
		    enviar_pcb_a(pcb,conexion_kernel,ACTUALIZAR_PUNTERO);
			cod_op = recibir_operacion(conexion_kernel);

			switch(cod_op){
				case CONTINUAR:
					buffer = desempaquetar(paquete,conexion_kernel);
					t_pcb* pcb2 = deserializar_pcb(buffer);
					pcb->archivos_abiertos = pcb2->archivos_abiertos;
					log_info(logger, "Sigue el programa");
					break;
				case DETENER:
					log_info(logger,"No se pudo abrir el archivo");
					band_ejecutar = 1;
					break;
				default:
					break;
			}
		    //enviar_datos_para_op_fs(pcb,instrAEnviar,CON_PARAMETRO,conexion_kernel);
			break;
		case F_READ:
			//log_info(logger,"Paso por f_read");

			dl=atoi(list_get(instruccion->parametros, 1));
		    pcb->cant_bytes = atoi(list_get(instruccion->parametros, 2));
		    pcb->df_fs=obtener_direccion_fisica(dl,pcb,pcb->cant_bytes);

		    pcb->arch_a_abrir=list_get(instruccion->parametros,0);
		    //instrAEnviar.datos_aux=atoi(list_get(instruccion->parametros,1));
		    enviar_pcb_a(pcb,conexion_kernel,LEER_ARCHIVO);
			cod_op = recibir_operacion(conexion_kernel);

			switch(cod_op){
				case CONTINUAR:
					buffer = desempaquetar(paquete,conexion_kernel);
					pcb = deserializar_pcb(buffer);
					log_info(logger, "Sigue el programa");
					break;
				case DETENER:
					buffer = desempaquetar(paquete,conexion_kernel);
					pcb = deserializar_pcb(buffer);
					log_info(logger,"El proceso se bloqueo");
					band_ejecutar = 1;
					break;
				default:
					break;
			}
		    //enviar_datos_para_op_fs(pcb,instrAEnviar,CON_PARAMETRO,conexion_kernel);
			break;
		case F_WRITE:
			//log_info(logger,"Paso por f_write");
			
			dl=atoi(list_get(instruccion->parametros, 1));
			pcb->cant_bytes=atoi(list_get(instruccion->parametros, 2));
			pcb->df_fs=obtener_direccion_fisica(dl,pcb,pcb->cant_bytes);

			pcb->arch_a_abrir=list_get(instruccion->parametros,0);
			//instrAEnviar.datos_aux=atoi(list_get(instruccion->parametros,1));
			enviar_pcb_a(pcb,conexion_kernel,ESCRIBIR_ARCHIVO);
			cod_op = recibir_operacion(conexion_kernel);

			switch(cod_op){
				case CONTINUAR:
					buffer = desempaquetar(paquete,conexion_kernel);
					pcb = deserializar_pcb(buffer);
					log_info(logger, "Sigue el programa");
					break;
				case DETENER:
					buffer = desempaquetar(paquete,conexion_kernel);
					pcb = deserializar_pcb(buffer);
					log_info(logger,"No se pudo abrir el archivo");
					band_ejecutar = 1;
					break;
				default:
					break;
			}
		    //enviar_datos_para_op_fs(pcb,instrAEnviar,CON_PARAMETRO,conexion_kernel);
			break;
		case F_TRUNCATE:
			//log_info(logger,"Paso por f_truncate");
			
			//TODO: memoryLeak
			pcb->arch_a_abrir=list_get(instruccion->parametros,0);
			pcb->dat_tamanio=atoi(list_get(instruccion->parametros,1)); //Es el tamanio
			enviar_pcb_a(pcb,conexion_kernel,MODIFICAR_TAMANIO);
			int cod_op = recibir_operacion(conexion_kernel);

			switch(cod_op){
				case CONTINUAR:
					buffer = desempaquetar(paquete,conexion_kernel);
					pcb = deserializar_pcb(buffer);
					log_info(logger, "Sigue el programa");
					break;
				case DETENER:
					buffer = desempaquetar(paquete,conexion_kernel);
					pcb = deserializar_pcb(buffer);
					log_info(logger,"No se pudo abrir el archivo");
					band_ejecutar = 1;
					break;
				default:
					break;
			}
		    //enviar_datos_para_op_fs(pcb,instrAEnviar,CON_PARAMETRO,conexion_kernel);
			break;
		case IO:
			//log_info(logger,"Pasa por I/O");
			enviar_pcb_a(pcb,conexion_kernel,EJECUTAR_IO);
			band_ejecutar = 1;
			break;
		case WAIT:
			//log_info(logger,"Pasa por Wait");
			enviar_pcb_a(pcb,conexion_kernel,EJECUTAR_WAIT);
			//recv(conexion_kernel, &result, sizeof(uint32_t), MSG_WAITALL);
			/*if(result == 0){
				log_info(logger,"El proceso se bloqueo o no existe");
				band_ejecutar = 1;
			} else {
				log_info(logger,"El programa sigue");
			}*/

			cod_op = recibir_operacion(conexion_kernel);

			switch(cod_op){
				case CONTINUAR:
					log_info(logger, "Sigue el programa");
					break;
				case DETENER:
					log_info(logger,"No se pudo abrir el archivo");
					band_ejecutar = 1;
					break;
				default:
					break;
			}
			break;
		case SIGNAL:
			//log_info(logger,"Pasa por Signal");
			enviar_pcb_a(pcb,conexion_kernel,EJECUTAR_SIGNAL);
			uint32_t a;
			recv(conexion_kernel, &a, sizeof(uint32_t), MSG_WAITALL);
			if(a == 0){
				log_info(logger,"El proceso se bloqueo o no existe");
				band_ejecutar = 1;
			} else {
				log_info(logger,"El programa sigue");
			}
			break;
		case CREATE_SEGMENT:
			//log_info(logger,"Paso por create_segment");

			pcb->dat_seg = atoi(list_get(instruccion->parametros,0));
			pcb->dat_tamanio = atoi(list_get(instruccion->parametros,1));

			log_info(logger,"Crear segmento %i de tamanio %i",pcb->dat_seg,pcb->dat_tamanio);
			//Hay que devolverle el pdb al kernel para que despues el mismo se lo envie a memoria como explica en el enuncuad
			enviar_pcb_a(pcb,conexion_kernel,CREAR_SEGMENTO);

			cod_op = recibir_operacion(conexion_kernel);
			switch(cod_op){
			case EJECUTAR:
				log_info(logger,"SIGA siga");
				buffer = desempaquetar(paquete,conexion_kernel);
				t_pcb* pcb2 = deserializar_pcb(buffer);
				pcb->tabla_segmentos = pcb2->tabla_segmentos;

				log_info(logger,"Volvio el pcb %i ", pcb->pid);
				break;
			case FINALIZAR:
				log_info(logger,"Se acabo el proceso");
				band_ejecutar = 1;
				break;
			default:
				log_info(logger,"Se acabo el proceso");
				band_ejecutar = 1;
				break;
			}

			//recv(conexion_kernel, &result, sizeof(uint32_t), MSG_WAITALL);
			//enviar_crear_segmento(id_seg,tamanio_seg,conexion_kernel);
			break;
		case DELETE_SEGMENT:
			//log_info(logger,"Paso por delete_segment");
			pcb->dat_seg = 0;
			pcb->dat_seg = atoi(list_get(instruccion->parametros,0));//TODO: memoryLeak

			//log_info(logger,"Eliminar segmento %i",pcb->dat_seg);
			//Hay que devolverle el pdb al kernel para que despues el mismo se lo envie a memoria como explica en el enuncuad
			enviar_pcb_a(pcb, conexion_kernel, ELIMINAR_SEGMENTO);
			cod_op = recibir_operacion(conexion_kernel);
				switch(cod_op){
					case EJECUTAR:
						log_info(logger,"SIGA siga");
						break;
					default:
						log_info(logger,"Se acabo el proceso");
						break;
				}
			buffer = desempaquetar(paquete,conexion_kernel);
			pcb = deserializar_pcb(buffer);

			log_info(logger,"Volvio el pcb %i ", pcb->pid);
			//recv(conexion_kernel, &result, sizeof(uint32_t), MSG_WAITALL);
			break;
		case YIELD:
			//log_info(logger,"Paso por YIELD");
			band_ejecutar = 1;
			enviar_pcb_a(pcb,conexion_kernel,DESALOJADO);
			break;
		case EXIT:
			log_info(logger,"Paso por Exit");
			band_ejecutar = 1;
			enviar_pcb_a(pcb,conexion_kernel,FINALIZAR);
			break;
		default:
			log_info(logger,"Otra cosa");
			break;
	}
}

char* devolver_parametros(t_list* lista){
	char* ins = string_new();

	for(int i = 0; i<list_size(lista);i++){
		char *p = list_get(lista,i);
		string_append(&ins, p);
		string_append(&ins, " ");
	}

	return ins;
}

registros_pos devolver_registro(char* registro){
	registros_pos v;
	if(strcmp(registro,"AX")==0){
		v = AX;
	} else if(strcmp(registro,"BX")==0){
		v = BX;
	} else if(strcmp(registro,"CX")==0){
		v = CX;
	} else if(strcmp(registro,"DX")==0){
		v = DX;
	} else if(strcmp(registro,"EAX")==0){
		v = EAX;
	} else if(strcmp(registro,"EBX")==0){
		v = EBX;
	} else if(strcmp(registro,"ECX")==0){
		v = ECX;
	} else if(strcmp(registro,"EDX")==0){
		v = EDX;
	} else if(strcmp(registro,"RAX")==0){
		v = RAX;
	} else if(strcmp(registro,"RBX")==0){
		v = RBX;
	} else if(strcmp(registro,"RCX")==0){
		v = RCX;
	} else if(strcmp(registro,"RDX")==0){
		v = RDX;
	} else {
		log_error(logger,"CUIDADO,CODIGO INVALIDO");
	}
	return v;
}

void insertar(t_pcb* pcb, registros_pos pos,char* caracteres){
	switch(pos){
	case AX: memcpy(pcb->registro.AX,caracteres,5);
		break;
	case BX: memcpy(pcb->registro.BX,caracteres,5);
		break;
	case CX: memcpy(pcb->registro.CX,caracteres,5);
		break;
	case DX: memcpy(pcb->registro.DX,caracteres,5);
		break;
	case EAX: memcpy(pcb->registro.EAX,caracteres,9);
		break;
	case EBX: memcpy(pcb->registro.EBX,caracteres,9);
		break;
	case ECX: memcpy(pcb->registro.ECX,caracteres,9);
		break;
	case EDX: memcpy(pcb->registro.EDX,caracteres,9);
		break;
	case RAX: memcpy(pcb->registro.RAX,caracteres,17);
		break;
	case RBX: memcpy(pcb->registro.RBX,caracteres,17);
		break;
	case RCX: memcpy(pcb->registro.RCX,caracteres,17);
		break;
	case RDX: memcpy(pcb->registro.RDX,caracteres,17);
		break;
	}

}

char*devolver_dato(t_pcb* pcb,registros_pos registro){
	char*dato;
	switch(registro){
			case AX: dato= pcb->registro.AX;
				break;
			case BX: dato= pcb->registro.BX;
				break;
			case CX: dato= pcb->registro.CX;
				break;
			case DX: dato= pcb->registro.DX;
				break;
			case EAX: dato= pcb->registro.EAX;
				break;
			case EBX: dato= pcb->registro.EBX;
				break;
			case ECX: dato= pcb->registro.ECX;
				break;
			case EDX: dato= pcb->registro.EDX;
				break;
			case RAX: dato= pcb->registro.RAX;
				break;
			case RBX: dato= pcb->registro.RBX;
				break;
			case RCX: dato= pcb->registro.RCX;
				break;
			case RDX: dato= pcb->registro.RDX;
				break;
		}
		return dato;
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



int obtener_direccion_fisica(uint32_t dir_logica,t_pcb*pcb,int tam_dato){
	t_dl* DL = malloc(sizeof(t_dl));

	DL->pid=pcb->pid;
	DL->num_segmento = floor(dir_logica / datos.tam_max_segmento);
	DL->offset = dir_logica % datos.tam_max_segmento;
	DL->segfault = false;

	log_info(logger,"El segmento es %i con offset %i",DL->num_segmento,DL->offset);

	int df = -1;

	for(int i = 0; i < list_size(pcb->tabla_segmentos); i++){
		segmento* seg = list_get(pcb->tabla_segmentos,i);
		if(seg->id == DL->num_segmento){
			if(DL->offset+tam_dato > seg->tamanio){

				log_info(logger,"PID: <%d> - Error SEG_FAULT- Segmento: <%d> - Offset: <%d> - Tamaño: <%d>",
				pcb->pid,DL->num_segmento,DL->offset,seg->tamanio);
				DL->segfault=true;
			} else {
				df = DL->offset + seg->direccion_base;
			}
		}

	}
	//tabla_segmento* segmento = list_get(pcb->segmentos,DL->num_segmento);

	return df;
}

int size_registro(registros_pos registro){
	int tamanio;
	switch(registro){
			case AX: tamanio= 4;
			break;
			case BX: tamanio= 4;
			break;
			case CX: tamanio= 4;
			break;
			case DX: tamanio= 4;
			break;
			case EAX: tamanio= 8;
			break;
			case EBX: tamanio= 8;
			break;
			case ECX: tamanio= 8;
			break;
			case EDX: tamanio= 8;
			break;
			case RAX:tamanio= 16;
			break;
			case RBX:tamanio= 16;
			break;
			case RCX: tamanio= 16;
			break;
			case RDX: tamanio= 16;
			break;
		}
	return tamanio;
}

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

	log_info(logger, "Acción: <ESCRIBIR>, La df es %i, con tamanio %i, VALOR %s",dir,tamanio,valor);

	memcpy(buffer->stream + offset, valor, tamanio);
	offset += tamanio;

	if(offset != buffer->size){
		log_warning(logger,"No son iguales offset %i buffer %i",offset,buffer->size);
	}
	paquete->buffer = buffer; //TODO: memoryLeak (librear buffer)

	enviar_paquete(paquete,conexion_memoria);
}

//------------------------------------------------------------------------//

/*
void enviar_crear_segmento(int id_seg,int tamanio_seg,int conexion){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = CREAR_SEGMENTO;
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int offset = 0;
	buffer->size = sizeof(int)*2;
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &id_seg, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &tamanio_seg, sizeof(int));
	offset += sizeof(int);

	paquete->buffer = buffer;

	enviar_paquete(paquete,conexion);
}
*/

//------------------------------------------------------------------------//
void enviar_datos_para_op_fs(t_pcb* pcb, envio_instr instrAEnviar, op_code codigo, int conexion){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion=codigo;
	t_buffer* buffer = malloc(sizeof(t_buffer));
	t_buffer* s_pcb = serializar_pcb(pcb);
	int offset = 0;

	buffer->size = s_pcb->size + sizeof(char*);

	switch(codigo){
	 case SIN_PARAMETRO:
	     buffer->stream = malloc(buffer->size);

	     memcpy(buffer->stream + offset, s_pcb->stream, s_pcb->size);
	     offset += s_pcb->size;

	     memcpy(buffer->stream + offset, &(instrAEnviar.nombreArchivo), strlen(instrAEnviar.nombreArchivo) + 1);
	     offset += strlen(instrAEnviar.nombreArchivo) + 1;

		 break;

	 case CON_PARAMETRO:
	     buffer->size += sizeof(int);
	     buffer->stream = malloc(buffer->size);

	     //"dato_aux" depende del contexto de la instruccion:
	     // Para: LEER_ARCHIVO, EXCRIBIR_ARCHIVO  => df (direccion fisica).
         // Para: ACTUALIZAR_PUNTERO => posicion a actualizar.
	     // Para: MODIFICAR_TAMANIO => tamanio a modificar.

	     memcpy(buffer->stream + offset, s_pcb->stream, s_pcb->size);
	     offset += s_pcb->size;

	     memcpy(buffer->stream + offset, &(instrAEnviar.nombreArchivo), strlen(instrAEnviar.nombreArchivo) + 1);
	     offset += strlen(instrAEnviar.nombreArchivo) + 1;

	     memcpy(buffer->stream + offset, &(instrAEnviar.datos_aux), sizeof(int));
	     offset += sizeof(int);

	     break;
	 default: log_info(logger,"Hubo un error");
	 	 break;
	}

	paquete->buffer = buffer;

	enviar_paquete(paquete, conexion);
}

char* op_instruct_to_string(op_instruct instr) {
    switch (instr) {
        case SET:
            return "SET";
        case MOV_IN:
            return "MOV_IN";
        case MOV_OUT:
            return "MOV_OUT";
        case IO:
            return "IO";
        case F_OPEN:
            return "F_OPEN";
        case F_CLOSE:
            return "F_CLOSE";
        case F_SEEK:
            return "F_SEEK";
        case F_READ:
            return "F_READ";
        case F_WRITE:
            return "F_WRITE";
        case F_TRUNCATE:
            return "F_TRUNCATE";
        case WAIT:
            return "WAIT";
        case SIGNAL:
            return "SIGNAL";
        case CREATE_SEGMENT:
            return "CREATE_SEGMENT";
        case DELETE_SEGMENT:
            return "DELETE_SEGMENT";
        case YIELD:
            return "YIELD";
        case EXIT:
            return "EXIT";
        default:
            return "UNKNOWN";
    }
}

