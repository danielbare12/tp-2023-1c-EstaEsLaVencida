/*
 * serializacion.c
 *
 *  Created on: Apr 18, 2023
 *      Author: utnso
 */

#include "serializacion.h"

t_paquete* crear_paquete_pcb(t_pcb* pcb, op_code codigo){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo;
	paquete->buffer = serializar_pcb(pcb);

	return paquete;
}

void liberar_pcb(t_pcb* pcb){
    // Liberar la lista de instrucciones

    int tam = list_size(pcb->lista_instrucciones);

    for(int i = 0; i < tam; i++){
    	t_instruccion* ins = list_get(pcb->lista_instrucciones,i);
    	for(int j = 0; j < list_size(ins->parametros); j++){
    		char* a = list_get(ins->parametros,j);
    		free(a);
    	}
    	list_destroy(ins->parametros);
    	free(ins);
    }

    list_destroy(pcb->lista_instrucciones);

    // Liberar la lista de tabla de segmentos
    tam = list_size(pcb->tabla_segmentos);
    for(int i = 0; i < tam; i++){
    	segmento* seg = list_get(pcb->tabla_segmentos,i);
    	free(seg);
    }

    list_destroy(pcb->tabla_segmentos);
    // Liberar la lista de archivos abiertos

    tam = list_size(pcb->archivos_abiertos);
    for(int i = 0; i < tam; i++){
    	info_arch* a = list_get(pcb->archivos_abiertos,i);
    	free(a->dir);
    	free(a);
    }
    list_destroy(pcb->archivos_abiertos);

    // Liberar el nombre del archivo a abrir
    if (pcb->arch_a_abrir != NULL) {
        free(pcb->arch_a_abrir);
    }

    // Finalmente, liberar el pcb en sí
    free(pcb);
}


t_buffer* serializar_pcb(t_pcb* pcb){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	t_buffer* instrucciones = serializar_lista_de_instrucciones(pcb->lista_instrucciones);
	int offset = 0;
	int tamanioTotal = 0;

	for(int i = 0; i < list_size(pcb->archivos_abiertos); i++){
		info_arch* archi = list_get(pcb->archivos_abiertos,i);
		tamanioTotal = strlen(archi->dir) + 1 + sizeof(int)*2;
	}

	int tamanio_tabla = list_size(pcb->tabla_segmentos);
	int tam_arch = strlen(pcb->arch_a_abrir) + 1;

	buffer->size = sizeof(int)*20 + sizeof(uint32_t) + tam_arch + sizeof(float) + instrucciones->size + 136 + tamanio_tabla*sizeof(segmento) + tamanioTotal;

	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &pcb->pid, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &pcb->program_counter, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &pcb->registro.AX, 5);
	offset += 5;

	memcpy(buffer->stream + offset, &pcb->registro.BX, 5);
	offset += 5;

	memcpy(buffer->stream + offset, &pcb->registro.CX, 5);
	offset += 5;

	memcpy(buffer->stream + offset, &pcb->registro.DX, 5);
	offset += 5;

	memcpy(buffer->stream + offset, &pcb->registro.EAX, 9);
	offset += 9;

	memcpy(buffer->stream + offset, &pcb->registro.EBX, 9);
	offset += 9;

	memcpy(buffer->stream + offset, &pcb->registro.ECX, 9);
	offset += 9;

	memcpy(buffer->stream + offset, &pcb->registro.EDX, 9);
	offset += 9;

	memcpy(buffer->stream + offset, &pcb->registro.RAX, 17);
	offset += 17;

	memcpy(buffer->stream + offset, &pcb->registro.RBX, 17);
	offset += 17;

	memcpy(buffer->stream + offset, &pcb->registro.RCX, 17);
	offset += 17;

	memcpy(buffer->stream + offset, &pcb->registro.RDX, 17);
	offset += 17;

	memcpy(buffer->stream + offset, &(tamanio_tabla), sizeof(int));
	offset += sizeof(int);

	for(int i = 0; i < tamanio_tabla; i++) {
		segmento* seg = list_get(pcb->tabla_segmentos,i);

		memcpy(buffer->stream + offset, &seg->id, sizeof(int));
		offset += sizeof(int);

		memcpy(buffer->stream + offset, &seg->direccion_base, sizeof(int));
		offset += sizeof(int);

		memcpy(buffer->stream + offset, &seg->tamanio, sizeof(int));
		offset += sizeof(int);
	}

	int tam_tab_arch = list_size(pcb->archivos_abiertos);

	memcpy(buffer->stream + offset, &tam_tab_arch, sizeof(int));
	offset += sizeof(int);

	for(int i = 0; i < tam_tab_arch; i++){
		info_arch* archi = list_get(pcb->archivos_abiertos,i);
		int size = strlen(archi->dir) + 1;

		memcpy(buffer->stream + offset, &size, sizeof(int));
		offset += sizeof(int);

		memcpy(buffer->stream + offset, archi->dir, size);
		offset += size;

		memcpy(buffer->stream + offset, &archi->punt, sizeof(int));
		offset += sizeof(int);
	}

	memcpy(buffer->stream + offset, &pcb->estimacion, sizeof(float));
	offset += sizeof(float);

	memcpy(buffer->stream + offset, &pcb->llegadaExec, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &pcb->llegadaReady, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &pcb->real_ant, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &pcb->conexion_consola, sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &(pcb->dat_seg), sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &(pcb->dat_tamanio), sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, &(tam_arch), sizeof(int));
	offset += sizeof(int);

	memcpy(buffer->stream + offset, pcb->arch_a_abrir, tam_arch);
	offset += tam_arch;

    memcpy(buffer->stream + offset, &(pcb->posicion), sizeof(int));
    offset += sizeof(int);

    memcpy(buffer->stream + offset, &pcb->df_fs, sizeof(int));
    offset += sizeof(int);

    memcpy(buffer->stream + offset, &pcb->cant_bytes, sizeof(uint32_t));
    offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &(instrucciones->size), sizeof(int));
	offset += sizeof(int);

	if (instrucciones->size > 0) {
		memcpy(buffer->stream + offset, instrucciones->stream, instrucciones->size);
		offset += instrucciones->size;
	}



	return buffer;

}



t_buffer* serializar_lista_de_instrucciones(t_list* lista_instrucciones){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int i_count;
	int	p_count;
	int stream_size;
	int offset = 0;
	t_list_iterator* i_iterator;
	t_list_iterator* p_iterator;
	t_instruccion*	instruccion;
	char* parametro;
	uint32_t size_parametro;

	i_iterator = list_iterator_create(lista_instrucciones);
	stream_size = 0;
	i_count = 0;

	while (list_iterator_has_next(i_iterator)) {
			instruccion = (t_instruccion*)list_iterator_next(i_iterator);

			// codigo, cantidad de parametros
			stream_size += (sizeof(uint32_t) * 2);
			p_count = list_size(instruccion->parametros);

			if (p_count > 0) {
				p_iterator = list_iterator_create(instruccion->parametros);

				while (list_iterator_has_next(p_iterator)) {
					// longitud del parametro, valor del parametro
					stream_size += sizeof(uint32_t) + strlen((char*)list_iterator_next(p_iterator)) + 1;
				}

				list_iterator_destroy(p_iterator);
			}

			i_count++;
		}

	list_iterator_destroy(i_iterator);

	buffer->size = stream_size + sizeof(uint32_t);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &(i_count), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	i_iterator = list_iterator_create(lista_instrucciones);

	while (list_iterator_has_next(i_iterator)) {
			instruccion = (t_instruccion*)list_iterator_next(i_iterator);

			// codigo de instruccion
			memcpy(buffer->stream + offset, &(instruccion->nombre), sizeof(uint32_t));
			offset += sizeof(uint32_t);

			// cantidad de parametros
			p_count = list_size(instruccion->parametros);
			memcpy(buffer->stream + offset, &(p_count), sizeof(uint32_t));
			offset += sizeof(uint32_t);

			if (p_count > 0) {
				p_iterator = list_iterator_create(instruccion->parametros);

				while (list_iterator_has_next(p_iterator)) {
					parametro = (char*)list_iterator_next(p_iterator);
					size_parametro = strlen(parametro) + 1;

					// longitud del parametro
					memcpy(buffer->stream + offset, &(size_parametro), sizeof(uint32_t));
					offset += sizeof(uint32_t);

					// valor del parametro
					memcpy(buffer->stream + offset, parametro, size_parametro);
					offset += size_parametro;
				}

				list_iterator_destroy(p_iterator);
			}
		}

	list_iterator_destroy(i_iterator);
	if (offset != buffer->size){
			printf("serializar_lista_instrucciones: El tamaño del buffer[%d] no coincide con el offset final[%d]\n", buffer->size, offset);
	}
	return buffer;

}

t_list* deserializar_lista_instrucciones(t_buffer* buffer){

	t_list*	i_list;
	t_instruccion* instruccion;
	uint32_t i_count;
	uint32_t p_count;
	int offset;
	int	p_size;
	char* param;
	op_instruct	codigo;

	i_list = list_create();
	offset = 0;

	memcpy(&i_count, buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	//log_info(logger,"Esta deserializando %i",i_count);

	for(int i=0; i < i_count; i++) {

			// codigo de la instruccion
			memcpy(&(codigo), buffer->stream + offset, sizeof(uint32_t));
			offset += sizeof(uint32_t);
			instruccion = malloc(sizeof(t_instruccion));
			instruccion->nombre = codigo;
			instruccion->parametros = list_create();

			// cantidad de parametros
			memcpy(&(p_count), buffer->stream + offset, sizeof(uint32_t));
			offset += sizeof(uint32_t);

			if (p_count > 0) {
				for (int p = 0; p < p_count; p++) {
					// longitud del parametro
					memcpy(&p_size, buffer->stream + offset, sizeof(uint32_t));
					offset += sizeof(uint32_t);

					// valor del parametro
					param = malloc(p_size);
					memcpy(param, buffer->stream + offset, p_size);

					list_add(instruccion->parametros, param);
					offset += p_size;
				}
			}

			list_add(i_list, instruccion);

		}

		return i_list;
}

void enviar_paquete_a(t_paquete* paquete,int conexion){
	void* a_enviar = malloc(paquete->buffer->size + sizeof(op_code) + sizeof(uint32_t));//TODO: memoryLeak
	int offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_code));

	offset += sizeof(op_code);
	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
	offset += paquete->buffer->size;
	send(conexion, a_enviar, paquete->buffer->size + sizeof(op_code) + sizeof(uint32_t), 0);

	free(a_enviar);
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}
// pcb, conexion del socket, codigo para saber que va a realizar
void enviar_pcb_a(t_pcb* pcb,int conexion, op_code codigo){

	t_paquete* paquete = crear_paquete_pcb(pcb,codigo);

	enviar_paquete_a(paquete,conexion);

	//liberar_pcb(pcb);
}

t_buffer* desempaquetar(t_paquete* paquete, int cliente_fd){
	recv(cliente_fd, &(paquete->buffer->size), sizeof(uint32_t), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);//TODO: memoryLeak
	recv(cliente_fd, paquete->buffer->stream, paquete->buffer->size, 0);

	return paquete->buffer;
	//free(paquete->buffer->stream);
}


t_pcb* deserializar_pcb(t_buffer* buffer){
	t_pcb* pcb = malloc(sizeof(t_pcb));

	int offset = 0;

	memcpy(&(pcb->pid),buffer->stream + offset,sizeof(int));
	offset += sizeof(int);
	memcpy(&(pcb->program_counter),buffer->stream + offset,sizeof(int));
	offset += sizeof(int);
	memcpy(&(pcb->registro.AX),buffer->stream + offset,5);
	offset += 5;
	memcpy(&(pcb->registro.BX),buffer->stream + offset,5);
	offset += 5;
	memcpy(&(pcb->registro.CX),buffer->stream + offset,5);
	offset += 5;
	memcpy(&(pcb->registro.DX),buffer->stream + offset,5);
	offset += 5;
	memcpy(&(pcb->registro.EAX),buffer->stream + offset,9);
	offset += 9;
	memcpy(&(pcb->registro.EBX),buffer->stream + offset,9);
	offset += 9;
	memcpy(&(pcb->registro.ECX),buffer->stream + offset,9);
	offset += 9;
	memcpy(&(pcb->registro.EDX),buffer->stream + offset,9);
	offset += 9;
	memcpy(&(pcb->registro.RAX),buffer->stream + offset,17);
	offset += 17;
	memcpy(&(pcb->registro.RBX),buffer->stream + offset,17);
	offset += 17;
	memcpy(&(pcb->registro.RCX),buffer->stream + offset,17);
	offset += 17;
	memcpy(&(pcb->registro.RDX),buffer->stream + offset,17);
	offset += 17;
	int tamanio_tabla;
	memcpy(&tamanio_tabla,buffer->stream + offset,sizeof(int));
	offset += sizeof(int);
	pcb->tabla_segmentos = list_create();
	for(int i = 0; i < tamanio_tabla; i++){

		segmento* seg = malloc(sizeof(segmento));
		memcpy(&(seg->id),buffer->stream + offset,sizeof(int));
		offset += sizeof(int);
		memcpy(&(seg->direccion_base),buffer->stream + offset,sizeof(int));
		offset += sizeof(int);
		memcpy(&(seg->tamanio),buffer->stream + offset,sizeof(int));
		offset += sizeof(int);
		list_add(pcb->tabla_segmentos,seg);
	}
	int tam;
	pcb->archivos_abiertos = list_create();

	memcpy(&tam, buffer->stream + offset, sizeof(int));
	offset += sizeof(int);

	for(int i = 0; i < tam; i++){
		info_arch* archi = malloc(sizeof(info_arch));
		//char* dir;
		int size;

		memcpy(&size, buffer->stream + offset,  sizeof(int));
		offset += sizeof(int);

		archi->dir = malloc(size);

		memcpy(archi->dir,buffer->stream + offset,  size);
		offset += size;

		memcpy(&archi->punt,buffer->stream + offset,  sizeof(int));
		offset += sizeof(int);

		list_add(pcb->archivos_abiertos,archi);


	}

	memcpy(&(pcb->estimacion),buffer->stream + offset,sizeof(float));
	offset += sizeof(float);
	memcpy(&(pcb->llegadaExec),buffer->stream + offset,sizeof(int));
	offset += sizeof(int);
	memcpy(&(pcb->llegadaReady),buffer->stream + offset,sizeof(int));
	offset += sizeof(int);
	memcpy(&(pcb->real_ant),buffer->stream + offset,sizeof(int));
	offset += sizeof(int);

	memcpy(&(pcb->conexion_consola),buffer->stream + offset,sizeof(int));
	offset += sizeof(int);

	memcpy(&(pcb->dat_seg),buffer->stream + offset, sizeof(int));
	offset += sizeof(int);

	memcpy(&(pcb->dat_tamanio),buffer->stream + offset, sizeof(int));
	offset += sizeof(int);

	int tam_arch;

	memcpy(&(tam_arch), buffer->stream + offset, sizeof(int));
	offset += sizeof(int);

	pcb->arch_a_abrir = malloc(tam_arch);

	memcpy(pcb->arch_a_abrir, buffer->stream + offset, tam_arch);
	offset += tam_arch;

    memcpy(&(pcb->posicion), buffer->stream + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&pcb->df_fs, buffer->stream + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&pcb->cant_bytes, buffer->stream + offset,sizeof(uint32_t));
    offset += sizeof(uint32_t);

	t_buffer* buffer_i = malloc(sizeof(t_buffer));

	memcpy(&(buffer_i->size), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	buffer_i->stream = malloc(buffer_i->size);
	memcpy(buffer_i->stream, buffer->stream + offset, buffer_i->size);
	offset += buffer_i->size;

	t_list* lista_instrucciones = deserializar_lista_instrucciones(buffer_i);

	pcb->lista_instrucciones=lista_instrucciones;

	return pcb;
}

t_paquete* crear_paquete_segmento(segmento* seg, op_code codigo){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo;
	paquete->buffer = serializar_segmento(seg);

	return paquete;
}

t_buffer* serializar_segmento(segmento* seg){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	int offset = 0;
	buffer->size = sizeof(int)*3;
	buffer->stream = malloc(buffer->size);
	memcpy(buffer->stream + offset, &seg->id, sizeof(int));
	offset += sizeof(int);
	memcpy(buffer->stream + offset, &seg->direccion_base, sizeof(int));
	offset += sizeof(int);
	memcpy(buffer->stream + offset, &seg->tamanio, sizeof(int));
	offset += sizeof(int);

	return buffer;
}
