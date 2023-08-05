/*
 * kernel.h
 *
 *  Created on: Apr 7, 2023
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <pthread.h>
#include <utils.h>
#include <sockets.h>
#include <compartido.h>
#include <semaphore.h>
#include <time.h>
#include <commons/collections/queue.h>
#include <serializacion.h>

typedef enum{
	FIFO,
	HRRN,
	FEEDBACK
}algoritmo;


typedef struct{
	char* ip_memoria;
	char* puerto_memoria;
	char* ip_filesystem;
	char* puerto_filesystem;
	char* ip_cpu;
	char* puerto_cpu;
	char* puerto_escucha;
	algoritmo algoritmo_planificacion;
	int est_inicial;
	float alfa;
	int multiprogramacion;
	char** recursos;
	char** instancias;
}datos_config;

typedef struct{
	t_queue* cola_new;
	t_queue* cola_ready_fifo;
	t_queue* cola_ready_hrrn;
}t_cola;

typedef struct{
	char* nombre;
	int instancias;
	t_queue* cola_recurso;
	sem_t sem_recurso;
}t_recurso;

typedef struct{
	char* directorio;
	t_queue* cola_archivo;
	sem_t sem_archivo;
}t_archivo;

void iterator(char* value);
void atender_consolas(void* data);
#endif /* KERNEL_H_ */
