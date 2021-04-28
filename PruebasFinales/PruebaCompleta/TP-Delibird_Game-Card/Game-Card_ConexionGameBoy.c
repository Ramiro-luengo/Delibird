/*
 * Game-Card_ConexionGameBoy.c
 *
 *  Created on: 15 jun. 2020
 *      Author: utnso
 */
#include "Game-Card.h"

void crearConexionGameBoy() {
	log_info(logger, "Creando hilo de conexión con GameBoy");
	socketEscuchaGameboy = crearConexionServer(config_get_string_value(config,"IP_GAMECARD"), config_get_string_value(config,"PUERTO_GAMECARD"));
	atenderConexiones(&socketEscuchaGameboy);
	log_info(logger, "Hilo de conexión con GameBoy creado. GameCard está a la espera de mensajes de GameBoy.");
}

void atenderConexiones(int *socketEscucha) {
	int backlog_server = 10;
	atenderConexionEn(*socketEscucha, backlog_server);
	while (1) {
		log_info(logger, "Esperando cliente GameBoy...");
		int *socketCliente = esperarCliente(*socketEscucha);
		log_info(logger,
				"Se ha conectado un cliente. Número de socket: %d",*socketCliente);
		esperarMensajesGameboy(socketCliente);
		free(socketCliente);
	}
}

void imprimirContenido(mensajeRecibido * mensaje, int * socketSuscripcion){
	log_debug(logger, "Recibiendo mensaje del socket cliente: %d",*socketSuscripcion);
	log_debug(logger, "codeOP %d",(int) mensaje->codeOP);
	log_debug(logger, "sizePayload %d", mensaje->sizePayload);
	log_debug(logger, "colaEmisora %s", getCodeStringByNum(mensaje->colaEmisora));
	log_debug(logger, "idMensaje %d", mensaje->idMensaje);
	log_debug(logger, "idCorrelativo %d", mensaje->idCorrelativo);
	log_debug(logger, "sizeMensaje %d", mensaje->sizeMensaje);
	log_debug(logger, "Mensaje recibido: %c", (char*) mensaje->mensaje);
}

void esperarMensajesGameboy(int* socketSuscripcion) {


	mensajeRecibido * mensaje = recibirMensajeDeBroker(*socketSuscripcion);
	log_info(logger,"Se recibió un mensaje de GameBoy con socket cliente %d",*socketSuscripcion);
	//imprimirContenido(mensaje, socketSuscripcion);
	switch (mensaje->colaEmisora) {
	case NEW: {
		//Procesar mensaje NEW
		log_info(logger, "Mensaje de la cola: NEW_POKEMON");
		procesarNEW(mensaje);
		break;
	}
	case GET: {
		//Procesar mensaje GET
		log_info(logger, "Mensaje de la cola: GET_POKEMON");
		procesarGET(mensaje);

		break;
	}
	case CATCH: {
		//Procesar mensaje CATCH
		log_info(logger, "Mensaje de la cola: CATCH_POKEMON");
		procesarCATCH(mensaje);
		break;
	}
	default: {

		log_error(logger, "Se recibió un mensaje dañado o de una cola no correspondiente");
		log_error(logger, "Informando error de conexión...");
		statusConexionBroker = ERROR_CONEXION;
		break;
	}

	}
}
