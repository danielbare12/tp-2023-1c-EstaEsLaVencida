/*
 * cpu.h
 *
 *  Created on: Apr 7, 2023
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <pthread.h>
#include <utils.h>
#include <sockets.h>
#include <compartido.h>
#include <serializacion.h>
#include <math.h>

typedef struct{
	int ret_instruccion;
	char* ip_memoria;
	char* puerto_memoria;
	char* puerto_escucha;
	int tam_max_segmento;
}datos_config;

typedef struct{
	int pid;
	int num_segmento;
	int offset;
	bool segfault;
}t_dl;

typedef struct{
	int pid;
	int base;
	int desplazamiento_segmento;
}t_df;

void* atender_kernel(void);
void* atender_memoria(void);
void iterator(char* value);
#endif /* CPU_H_ */
