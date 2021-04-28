/*
 * Utils.c
 *
 *  Created on: 6 abr. 2020
 *      Author: ripped dinos
 */

#include "Utils.h"

void atenderConexionEn(int socket, int backlog) {
	listen(socket, backlog);
}

/* Crea un socket de escucha para un servidor en X puerto
 * RECORDAR HACER EL CLOSE DEL LISTENNING SOCKET EN LA FUNCION CORRESPONDIENTE
 */
int crearConexionServer(char * ip, char * puerto) {

	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(ip, puerto, &hints, &serverInfo);

	int socketEscucha;
	socketEscucha = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	int yes = 1;
	setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	bind(socketEscucha, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);

	return socketEscucha;
}

/* Crea una conexión con el servidor en la IP y puerto indicados, devolviendo el socket del servidor
 *
 */
int crearConexionCliente(char * ip, char * puerto) {
	struct addrinfo hints;
	struct addrinfo *serverInfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(ip, puerto, &hints, &serverInfo);

	int socketServidor;
	socketServidor = socket(serverInfo->ai_family, serverInfo->ai_socktype,serverInfo->ai_protocol);

	int status = connect(socketServidor, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);
	if(status==-1){
		return status;
	}
	return socketServidor;
}

/* Crea una conexión con el servidor en la IP y puerto indicados, devolviendo el socket del servidor.
 * Reintenta la conexion hasta que es exitosa.
 */
int crearConexionClienteConReintento(char * ip, char * puerto, int tiempoDeEspera) {
    struct addrinfo hints;
    struct addrinfo *serverInfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(ip, puerto, &hints, &serverInfo);

    int socketServidor;
    socketServidor = socket(serverInfo->ai_family, serverInfo->ai_socktype,
            serverInfo->ai_protocol);

    while(1){
        int status = connect(socketServidor, serverInfo->ai_addr, serverInfo->ai_addrlen);
        if(status != -1)
            break;
        else {
            usleep(tiempoDeEspera * 1000000);
        }
    }
    freeaddrinfo(serverInfo);
    return socketServidor;
}

/* Espera un cliente y cuando recibe una conexion, devuelve el socket correspondiente al cliente conectado.
 * RECORDAR HACER EL FREE AL PUNTERO SOCKETCLIENTE EN LA FUNCIÓN CORRESPONDIENTE Y EL CLOSE AL SOCKET
 */
int * esperarCliente(int socketEscucha) {
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int *socketCliente = malloc(sizeof(int));
	*socketCliente = accept(socketEscucha, (struct sockaddr *) &addr, &addrlen);
	return socketCliente;
}

/* Serializa un paquete de formato estandar (Código de operación / Tamaño de buffer / Stream)
 *
 */

void * serializarPaquete(tPaquete* paquete, int tamanioAEnviar) {
	int offset = 0;
	void* aEnviar = malloc(tamanioAEnviar);

	memcpy(aEnviar + offset, &(paquete->codOperacion), sizeof(opCode));
	offset += sizeof(opCode);
	memcpy(aEnviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(aEnviar + offset, paquete->buffer->stream, paquete->buffer->size);

	return aEnviar;
}
void armarPaqueteNew(int offset, void* mensaje, tBuffer* buffer) {

	mensajeNew* msg = mensaje;
	memcpy(buffer->stream + offset, &(msg->longPokemon), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, msg->pokemon, msg->longPokemon);
	offset += msg->longPokemon;
	memcpy(buffer->stream + offset, &(msg->posicionX), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &(msg->posicionY), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &(msg->cantPokemon), sizeof(uint32_t));
}

void armarPaqueteAppeared(int offset, void* mensaje, tBuffer* buffer) {
	mensajeAppeared* msg = mensaje;
	memcpy(buffer->stream + offset, &(msg->longPokemon), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, msg->pokemon, msg->longPokemon);
	offset += msg->longPokemon;
	memcpy(buffer->stream + offset, &(msg->posicionX), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &(msg->posicionY), sizeof(uint32_t));
	offset += sizeof(uint32_t);
}

void armarPaqueteCatch(int offset, void* mensaje, tBuffer* buffer) {
	mensajeCatch* msg = mensaje;
	memcpy(buffer->stream + offset, &(msg->longPokemon), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, msg->pokemon, msg->longPokemon);
	offset += msg->longPokemon;
	memcpy(buffer->stream + offset, &(msg->posicionX), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &(msg->posicionY), sizeof(uint32_t));
	offset += sizeof(uint32_t);
}

void armarPaqueteCaught(int offset, void* mensaje, tBuffer* buffer) {
	mensajeCaught* msg = mensaje;
	memcpy(buffer->stream + offset, &(msg->resultado), sizeof(resultado));
	offset += sizeof(resultado);
}

void armarPaqueteGet(int offset, void* mensaje, tBuffer* buffer) {
	mensajeGet* msg = mensaje;
	memcpy(buffer->stream + offset, &(msg->longPokemon), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, msg->pokemon, msg->longPokemon);
	offset += sizeof(msg->longPokemon);
}

void armarPaqueteLocalized(int offset, void* mensaje, tBuffer* buffer) {
	mensajeLocalized* msg = mensaje;
	posiciones variableAuxiliar;
	memcpy(buffer->stream + offset, &(msg->longPokemon), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, msg->pokemon, msg->longPokemon);
	offset += msg->longPokemon;
	memcpy(buffer->stream + offset, &(msg->listSize), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	for (int i = 0; i < msg->listSize; i++) {
		variableAuxiliar = *(posiciones*) (list_get(msg->paresDeCoordenada, i));
		memcpy(buffer->stream + offset, &(variableAuxiliar.posicionX),
				sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer->stream + offset, &(variableAuxiliar.posicionY),
				sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}
}

//EDIT GONZALO 23/04
//------------------
/* Permite envíar un mensaje desde cualquier cliente, que el broker sepa interpretar. El size del mensaje que requiere como
 * parametro es el del contenido del struct GET, CATCH, etc., que se esta mandando.
 */
void enviarMensajeABroker(int socketBroker, cola colaDestino,
		uint32_t idCorrelativo, uint32_t sizeMensaje, void * mensaje) {
	int offset = 0;
	int sizeTotal;
	void *mensajeSerializado;

	tPaquete *paquete = malloc(sizeof(tPaquete));
	tBuffer *buffer = malloc(sizeof(tBuffer));
	buffer->size = sizeMensaje + sizeof(cola) + sizeof(uint32_t) * 2;
	buffer->stream = malloc(buffer->size);

	paquete->codOperacion = NUEVO_MENSAJE;

	memcpy(buffer->stream + offset, &(colaDestino), sizeof(cola));
	offset += sizeof(cola);
	memcpy(buffer->stream + offset, &(idCorrelativo), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &(sizeMensaje), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	switch (colaDestino) {
	case NEW: {
		armarPaqueteNew(offset, mensaje, buffer);
		break;
	}
	case APPEARED: {
		armarPaqueteAppeared(offset, mensaje, buffer);
		break;
	}
	case CATCH: {
		armarPaqueteCatch(offset, mensaje, buffer);
		break;
	}
	case CAUGHT: {
		armarPaqueteCaught(offset, mensaje, buffer);
		break;
	}
	case GET: {
		armarPaqueteGet(offset, mensaje, buffer);
		break;
	}
	case LOCALIZED: {
		armarPaqueteLocalized(offset, mensaje, buffer);
		break;
	}
	default: {
		log_error(logger, "[ERROR]");
		log_error(logger, "No se pudo leer la cola destino");
	}
	}
	paquete->buffer = buffer;
	sizeTotal = buffer->size + sizeof(uint32_t) + sizeof(opCode);
	mensajeSerializado = serializarPaquete(paquete, sizeTotal);
	send(socketBroker, mensajeSerializado, sizeTotal, 0);
	free(mensajeSerializado);
	free(paquete);
	free(buffer->stream);
	free(buffer);
}

/* Permite enviar un mensaje a cualquier cliente, de forma que estos lo puedan interpretar
 *
 */
int enviarMensajeASuscriptor(estructuraMensaje datosMensaje, int socketSuscriptor) {

	int returnValueSend;

	tPaquete * paquete = malloc(sizeof(tPaquete));
	tBuffer * buffer = malloc(sizeof(tBuffer));
	void * paqueteSerializado;

	int offset = 0;
	int sizeTotal;

	paquete->codOperacion = NUEVO_MENSAJE;

	buffer->size = sizeof(cola) + sizeof(uint32_t) * 3
			+ datosMensaje.sizeMensaje;
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream + offset, &(datosMensaje.colaMensajeria),
			sizeof(cola));
	offset += sizeof(cola);
	memcpy(buffer->stream + offset, &(datosMensaje.id), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &(datosMensaje.idCorrelativo),
			sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, &(datosMensaje.sizeMensaje),
			sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer->stream + offset, datosMensaje.mensaje,
			datosMensaje.sizeMensaje);

	paquete->buffer = buffer;

	sizeTotal = sizeof(opCode) + sizeof(uint32_t) + buffer->size;

	paqueteSerializado = serializarPaquete(paquete, sizeTotal);

	returnValueSend = send(socketSuscriptor, paqueteSerializado, sizeTotal, 0);
	free(paqueteSerializado);
	free(paquete);
	free(buffer->stream);
	free(buffer);

	return returnValueSend;
}

/* Recibe un mensaje del broker y lo guarda en un struct con formato mensajeRecibido
 *
 */
mensajeRecibido * recibirMensajeDeBroker(int socketBroker) {
	int status;
	mensajeRecibido * mensaje = malloc(sizeof(mensajeRecibido));

	status=recv(socketBroker, &(mensaje->codeOP), sizeof(opCode), MSG_WAITALL);
	if(status<=0){
		mensaje->codeOP=FINALIZAR;
		return mensaje;
	}
	recv(socketBroker, &(mensaje->sizePayload), sizeof(uint32_t), MSG_WAITALL);
	recv(socketBroker, &(mensaje->colaEmisora), sizeof(cola), MSG_WAITALL);
	recv(socketBroker, &(mensaje->idMensaje), sizeof(uint32_t), MSG_WAITALL);
	recv(socketBroker, &(mensaje->idCorrelativo), sizeof(uint32_t),MSG_WAITALL);
	recv(socketBroker, &(mensaje->sizeMensaje), sizeof(uint32_t), MSG_WAITALL);
	mensaje->mensaje = malloc(mensaje->sizeMensaje);
	recv(socketBroker, mensaje->mensaje, mensaje->sizeMensaje, MSG_WAITALL);

	return mensaje;
}

/*
 * Luego de creada la conexión con el broker, esta función envía el código de la cola a la que se va a suscribir.
 */
void suscribirseACola(int socketBroker, cola tipoCola, uint32_t idSuscriptor) {

	tPaquete * paquete = malloc(sizeof(tPaquete));
	paquete->buffer = malloc(sizeof(tBuffer));
	paquete->codOperacion = SUSCRIPCION;

	paquete->buffer->size = sizeof(cola) + sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &(tipoCola), sizeof(cola));
	memcpy(paquete->buffer->stream + sizeof(cola), &(idSuscriptor),sizeof(uint32_t));
	int sizeTotal = sizeof(opCode) + sizeof(uint32_t) + sizeof(cola) + sizeof(uint32_t);
	void * paqueteSerializado = serializarPaquete(paquete, sizeTotal);
	send(socketBroker, paqueteSerializado, sizeTotal, 0);
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
	free(paqueteSerializado);
	log_info(logger, "Se ha suscrito a la cola %s",
	getCodeStringByNum(tipoCola));
}

//------------------

/* Envía un string al socket destino
 *
 */
void enviarString(int socketDestino, char * mensaje) {

	int longMensaje = strlen(mensaje);
	tBuffer *buffer = malloc(sizeof(tBuffer));
	tPaquete *paquete = malloc(sizeof(tPaquete));

	buffer->size = longMensaje + 1;
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, mensaje, buffer->size);

	paquete->buffer = buffer;
	if (strcmp(mensaje, "exit") == 0)
		paquete->codOperacion = FINALIZAR;
	else
		paquete->codOperacion = NUEVO_MENSAJE;

	int tamanioAEnviar = 2 * sizeof(int) + buffer->size;
	void* aEnviar = serializarPaquete(paquete, tamanioAEnviar);

	send(socketDestino, aEnviar, tamanioAEnviar, 0);

	free(buffer->stream);
	free(buffer);
	free(paquete);
	free(aEnviar);
}


uint32_t obtenerIdDelProceso(char* ip, char* puerto) {
	int socketBroker = crearConexionCliente(ip, puerto);
	if(socketBroker==-1){
		return -1;
	}
	uint32_t idProceso;

	opCode codigoOP = NUEVA_CONEXION;
	send(socketBroker, &codigoOP, sizeof(opCode), 0);
	recv(socketBroker, &idProceso, sizeof(uint32_t), MSG_WAITALL);
	close(socketBroker);


	return idProceso;
}

uint32_t obtenerIdDelProcesoConReintento(char* ip, char* puerto, int tiempoDeEspera) {
	int socketBroker = crearConexionClienteConReintento(ip, puerto, tiempoDeEspera);
	uint32_t idProceso;


	opCode codigoOP = NUEVA_CONEXION;
	send(socketBroker, &codigoOP, sizeof(opCode), 0);
	uint32_t statusRecv=recv(socketBroker, &idProceso, sizeof(uint32_t), MSG_WAITALL);
	close(socketBroker);

	log_debug(logger, "ID obtenido = %d", idProceso);

	if(statusRecv<0)
		return -1;

	return idProceso;
}

void destructorGeneral(void * elemento){
	free(elemento);
}

//Funciones auxiliares para logs
//---------------------------------------------------

//----------------------- [GETTERS] -------------------------//

char* getCodeStringByNum(int nro) {
	char* codigo[6] = { "NEW_POKEMON", "APPEARED_POKEMON", "CATCH_POKEMON",
			"CAUGHT_POKEMON", "GET_POKEMON", "LOCALIZED_POKEMON" };
	return codigo[nro];
}


//---------------------------------------------------
