/*
 * Game-Card_Conexiones.c
 *
 *  Created on: 15 jun. 2020
 *      Author: utnso
 */
#include "Game-Card.h"

void crearConexionBroker() {
	log_info(logger, "Creando hilo de conexión con el broker...");
	pthread_create(&hiloReconexiones, NULL, (void*) mantenerConexionBroker, NULL);
	pthread_detach(hiloReconexiones);
	log_info(logger, "Hilo de conexion con el broker creado");
}

void mantenerConexionBroker(){
	int tiempoReintento = config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	statusConexionBroker = conectarYSuscribir();
	while(1){
		if(statusConexionBroker!=CONECTADO){
			log_info(logger, "Reintentando conectar con el broker...");
			statusConexionBroker = conectarYSuscribir();
		}
		sleep(tiempoReintento);
	}
}

int enviarMensajeBroker(cola colaDestino, uint32_t idCorrelativo,uint32_t sizeMensaje, void * mensaje) {
	int socketBroker = crearConexionCliente(ipServidor, puertoServidor);
	if (socketBroker < 0) {
		log_error(logger, "No se pudo enviar el mensaje al broker: conexion fallida.");
		return socketBroker;
	}
	enviarMensajeABroker(socketBroker, colaDestino, idCorrelativo, sizeMensaje, mensaje);
	uint32_t respuestaBroker;
	recv(socketBroker, &respuestaBroker, sizeof(uint32_t), MSG_WAITALL);
	close(socketBroker);
	return respuestaBroker;
}


int crearSuscripcionesBroker(){

	log_info(logger, "Intentando crear suscripciones a broker");
	socketSuscripcionNEW = crearConexionCliente(ipServidor, puertoServidor);
	if(socketSuscripcionNEW==-1)
		return ERROR_CONEXION;
	suscribirseACola(socketSuscripcionNEW, NEW, idProceso);
	log_info(logger, "Suscripcion a NEW_POKEMON realizada");

	socketSuscripcionCATCH = crearConexionCliente(ipServidor, puertoServidor);
	if(socketSuscripcionCATCH==-1)
		return ERROR_CONEXION;
	suscribirseACola(socketSuscripcionCATCH, CATCH, idProceso);
	log_info(logger, "Suscripcion a CATCH_POKEMON realizada");

	socketSuscripcionGET = crearConexionCliente(ipServidor, puertoServidor);
	if(socketSuscripcionGET==-1)
		return ERROR_CONEXION;
	suscribirseACola(socketSuscripcionGET, GET, idProceso);
	log_info(logger, "Suscripcion a GET_POKEMON realizada");

	pthread_create(&hiloEsperaNEW, NULL, (void*) esperarMensajesBroker, &socketSuscripcionNEW);
	pthread_create(&hiloEsperaGET, NULL, (void*) esperarMensajesBroker, &socketSuscripcionGET);
	pthread_create(&hiloEsperaCATCH, NULL, (void*) esperarMensajesBroker, &socketSuscripcionCATCH);

	pthread_detach(hiloEsperaNEW);
	pthread_detach(hiloEsperaGET);
	pthread_detach(hiloEsperaCATCH);

	log_info(logger, "Suscripciones a colas realizadas exitosamente");

	return CONECTADO;

}

estadoConexion conectarYSuscribir(){
	if(idProceso==-1){
		log_info(logger, "Intentando establecer conexion inicial con broker...");
		idProceso=obtenerIdDelProceso(ipServidor,puertoServidor);

		if(idProceso==-1){
			log_info(logger, "No se pudo establecer la conexion inicial con el broker");
			return ERROR_CONEXION;
		}
		log_info(logger, "ID de proceso asignado a GameCard: %d", idProceso);
	}
	estadoConexion statusConexion = crearSuscripcionesBroker();
	if(statusConexion==ERROR_CONEXION)
		log_info(logger,"No se pudieron realizar las suscripciones. Error de conexión");
	return statusConexion;
}


void cerrarConexiones(){
	log_info(logger,"Cerrando conexiones...");
	close(socketEscuchaGameboy);
	close(socketSuscripcionNEW);
	close(socketSuscripcionCATCH);
	close(socketSuscripcionCATCH);
}

void esperarMensajesBroker(int* socketSuscripcion) {
	uint32_t ack = 1;
	mensajeRecibido * mensaje;

	while (1) {
		mensaje = recibirMensajeDeBroker(*socketSuscripcion);
		pthread_t atencionDeMensaje;

		if(mensaje->codeOP==FINALIZAR){
			log_info(logger,"Hay problemas de conexión con el broker...");
			free(mensaje);
			statusConexionBroker=ERROR_CONEXION;
			close(*socketSuscripcion);
			break;
		}
		log_info(logger,"Se recibió un mensaje del broker");
		send(*socketSuscripcion, &ack, sizeof(uint32_t), 0);
		log_info(logger,"Enviando confirmación...");

		switch (mensaje->colaEmisora) {
		case NEW: {
			//Procesar mensaje NEW
			log_info(logger, "Mensaje de la cola: NEW_POKEMON");
			pthread_create(&atencionDeMensaje, NULL, (void *)procesarNEW,mensaje);
			pthread_detach(atencionDeMensaje);
			//procesarNEW(mensaje);
			break;
		}
		case GET: {
			//Procesar mensaje GET
			log_info(logger, "Mensaje de la cola: GET_POKEMON");
			pthread_create(&atencionDeMensaje, NULL, (void *)procesarGET,mensaje);
			pthread_detach(atencionDeMensaje);
			//procesarGET(mensaje);
			break;
		}
		case CATCH: {
			//Procesar mensaje CATCH
			log_info(logger, "Mensaje de la cola: CATCH_POKEMON");
			pthread_create(&atencionDeMensaje, NULL, (void *)procesarCATCH,mensaje);
			pthread_detach(atencionDeMensaje);
			//procesarCATCH(mensaje);
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
}
