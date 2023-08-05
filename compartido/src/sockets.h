/*
 * sockets.h
 *
 *  Created on: Apr 7, 2023
 *      Author: utnso
 */

#ifndef SRC_SOCKETS_H_
#define SRC_SOCKETS_H_


#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include "compartido.h"

extern t_log* logger;

void* recibir_buffer(int*, int);

int iniciar_servidor(t_log*,char* puerto);
int* esperar_cliente(t_log*,int);
t_list* recibir_paquete(int);
void recibir_mensaje(t_log*,int);
int recibir_operacion(int);


#endif /* SRC_SOCKETS_H_ */
