/*
 * memoria.h
 *
 *  Created on: Apr 7, 2023
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <pthread.h>
#include <utils.h>
#include <sockets.h>
#include <compartido.h>
#include <semaphore.h>
#include <serializacion.h>

#define IP "127.0.0.1"
#define MEMORIA_LOG "memoria.log"
#define MEMORIA_CONFIG "memoria.config"
#define MEMORIA_NAME "Memoria" 


typedef enum{
	FIRST,
	BEST,
	WORST
}op_asignacion;

typedef struct{
	char* puerto_escucha;
	int tam_memoria;
	int tam_segmento;
	int cant_segmentos;
	int ret_memoria;
	int ret_compactacion;
	op_asignacion algoritmo;
}datos_config;

typedef struct{
	segmento segmento;
	int pid;
}seg_aux;

t_log* logger;
t_config* config;
datos_config datos;
int server_fd;
int* cliente_fd;

void atender_modulos(void* data);

void finalizar_memoria();
void iniciar_config_memoria();
void iterator(char* value);

#endif /* MEMORIA_H_ */
