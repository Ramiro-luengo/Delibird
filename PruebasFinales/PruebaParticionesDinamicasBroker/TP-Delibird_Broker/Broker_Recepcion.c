#include "Broker.h"


void empezarAAtenderCliente(int socketEscucha) {
	pthread_t hiloAtenderCliente;
	pthread_create(&hiloAtenderCliente, NULL, (void*) atenderConexiones,
			&socketEscucha);
	pthread_detach(hiloAtenderCliente);
}

void atenderConexiones(int *socketEscucha) {
	/* Espera nuevas conexiones en el socket de escucha. Al establecerse una nueva, envía esa conexión a un nuevo hilo para que
	 * sea gestionada y vuelve a esperar nuevas conexiones.
	 */
	int backlog_server = config_get_int_value(config, "BACKLOG_SERVER");
	atenderConexionEn(*socketEscucha, backlog_server);
	while (1) {
		log_info(logger, "Esperando cliente...");
		int *socketCliente = esperarCliente(*socketEscucha);
		log_info(logger,
				"Se ha conectado un cliente. Número de socket cliente: %d",
				*socketCliente);

		log_info(loggerOficial,"Se ha conectado un proceso al broker.");

        esperarMensajes(socketCliente);
        free(socketCliente);
	}
}



void esperarMensajes(int *socketCliente) {
	/* Espera mensajes de una conexión ya establecida. Según el
	 * código de operación recibido, delega tareas a distintos modulos.
	 */

	opCode codOperacion;
	int sizeDelMensaje;
	cola tipoCola;

	recv(*socketCliente, &codOperacion, sizeof(opCode), MSG_WAITALL);
	log_info(logger, "Esperando mensaje de cliente %d...", *socketCliente);


	switch (codOperacion) {

	case NUEVA_CONEXION:{
		log_info(logger, "Hay una nueva conexión");
        uint32_t idProceso = getIDProceso();
        send(*socketCliente, &idProceso,sizeof(uint32_t),0);
        log_info(logger, "ClientID asignado al nuevo suscriptor: %d", idProceso);
        close(*socketCliente);
        log_info(logger, "Finalizó la atención a la nueva conexión");
		break;
	}

	case SUSCRIPCION: {
		log_info(logger, "Hay una nueva suscripción");
		atenderSuscripcion(socketCliente);
		log_info(logger, "Finalizó la atención a la nueva suscripción");
		break;
	}

	case NUEVO_MENSAJE: {
		log_info(logger, "Hay un nuevo mensaje");
		recv(*socketCliente, &sizeDelMensaje, sizeof(uint32_t), MSG_WAITALL);
		recv(*socketCliente, &tipoCola, sizeof(cola), MSG_WAITALL);
		atenderMensaje(*socketCliente, tipoCola);
		close(*socketCliente);
		log_info(logger, "Finalizó la atención al nuevo mensaje");
		break;
	}
	case FINALIZAR: {
		/* Finaliza la conexión con el broker de forma ordenada.
		 * No creo que tenga mucho sentido en el TP, seria para hacer pruebas.
		 */
		log_info(logger, "Hay una nueva desuscripción");
		recv(*socketCliente, &tipoCola, sizeof(cola), MSG_WAITALL);
		uint32_t clientID;
		recv(*socketCliente, &clientID, sizeof(uint32_t), MSG_WAITALL);
		desuscribir(clientID, tipoCola);
		log_info(logger, "El cliente con ID %d se ha desconectado",
				clientID);
		close(*socketCliente);
		log_info(logger, "Finalizó la atención de la nueva desuscripción");
		break;
	}
	case DUMPCACHE: {
		log_info(logger,"Hay una nueva solicitud de dump de cache");
		dumpCache();
		break;
	}
	default: {
		log_error(logger, "Se recibió un mensaje erroneo o dañado");
		close(*socketCliente);
		break;
	}
	}
}

bool yaExisteSuscriptor(uint32_t clientID, cola codSuscripcion){
    bool existeClientID(void * nodoLista){
		   suscriptor* sus = (suscriptor *) nodoLista;
		   return sus->clientID==clientID;
	}
   t_list * listaSuscriptores = getListaSuscriptoresByNum(codSuscripcion);

   return list_any_satisfy(listaSuscriptores,(void *)existeClientID);;
}

void atenderSuscripcion(int *socketSuscriptor){
	/* Recibe el código de suscripción desde el socket a suscribirse, eligiendo de esta manera la cola y agregando el socket
	 * a la lista de suscriptores de la misma.
	 */
	cola codSuscripcion;
	uint32_t sizePaquete;
	suscriptor * nuevoSuscriptor = malloc(sizeof(suscriptor));
	nuevoSuscriptor->socketCliente=*socketSuscriptor;

	recv(*socketSuscriptor, &sizePaquete, sizeof(uint32_t), MSG_WAITALL);
	recv(*socketSuscriptor, &codSuscripcion, sizeof(cola), MSG_WAITALL);
	recv(*socketSuscriptor, &(nuevoSuscriptor->clientID), sizeof(uint32_t), MSG_WAITALL);

	log_info(logger, "Cola a la que suscribir: %s", getCodeStringByNum(codSuscripcion));

	sem_wait(&mutexColas);
	if(yaExisteSuscriptor(nuevoSuscriptor->clientID,codSuscripcion)==true){
		suscriptor * suscriptorYaAlmacenado = buscarSuscriptor(nuevoSuscriptor->clientID,codSuscripcion);
        suscriptorYaAlmacenado->socketCliente=nuevoSuscriptor->socketCliente;

        log_info(logger,
        				"El cliente %d ha actualizado el socket: %d",
						suscriptorYaAlmacenado->clientID, suscriptorYaAlmacenado->socketCliente);

		enviarMensajesCacheados(suscriptorYaAlmacenado, codSuscripcion);
	}
	else
	{
		list_add(getListaSuscriptoresByNum(codSuscripcion), nuevoSuscriptor);
		log_info(logger,
				"Hay un nuevo suscriptor en la cola %s. Número de socket suscriptor: %d",
				getCodeStringByNum(codSuscripcion), *socketSuscriptor);

		log_info(loggerOficial, "El proceso %d se ha suscripto a la cola %s",nuevoSuscriptor->clientID,getCodeStringByNum(codSuscripcion));


		enviarMensajesCacheados(nuevoSuscriptor, codSuscripcion);
	}
	sem_post(&mutexColas);

}

bool seRecibioElIDCorrelativo(uint32_t idCConsultado){
	bool yaExisteIDCorrelativo(void* elementoIDC){
		int * idCYaRecibido = (int *) elementoIDC;
		return *idCYaRecibido==idCConsultado;
	}
	return list_any_satisfy(idCorrelativosRecibidos, (void *)yaExisteIDCorrelativo);
}

void atenderMensaje(int socketEmisor, cola tipoCola) {
	uint32_t idMensaje=-1;
	uint32_t* idCorrelativo = malloc(sizeof(uint32_t));
	recv(socketEmisor, idCorrelativo, sizeof(uint32_t), MSG_WAITALL);

	if(!seRecibioElIDCorrelativo(*idCorrelativo)){

		if (tipoCola >= 0 && tipoCola <= 5) {
			idMensaje = agregarMensajeACola(socketEmisor, tipoCola, *idCorrelativo);
			send(socketEmisor, &idMensaje, sizeof(uint32_t), 0);

			if(*idCorrelativo != -1)
				list_add(idCorrelativosRecibidos, idCorrelativo);
			else
				free(idCorrelativo);

		} else {
			send(socketEmisor, &idMensaje, sizeof(uint32_t), 0);
			log_error(logger, "No pudo obtenerse el tipo de cola en el mensaje recibido");
			free(idCorrelativo);
		}

	}else{
		send(socketEmisor, &idMensaje, sizeof(uint32_t), 0);
		log_info(logger, "Se ignoro mensaje de proceso con socket %d. ID Correlativo %d ya recibido.", socketEmisor, *idCorrelativo);
		free(idCorrelativo);
	}


}

void imprimirEstructuraDeDatos(estructuraMensaje mensaje) {
	log_info(logger, "[NUEVO MENSAJE RECIBIDO]");
	log_info(logger, "ID: %d", mensaje.id);
	log_info(logger, "ID correlativo: %d", mensaje.idCorrelativo);
	log_info(logger, "Tamaño de mensaje: %d", mensaje.sizeMensaje);
	log_info(logger, "Tipo mensaje: %s", getCodeStringByNum(mensaje.colaMensajeria));
}

estructuraMensaje * generarNodo(estructuraMensaje mensaje) {

	estructuraMensaje * nodo = malloc(sizeof(estructuraMensaje));
	nodo->mensaje = malloc(mensaje.sizeMensaje);

	nodo->id = mensaje.id;
	nodo->idCorrelativo = mensaje.idCorrelativo;
	nodo->estado = mensaje.estado;
	nodo->sizeMensaje = mensaje.sizeMensaje;
	memcpy(nodo->mensaje, mensaje.mensaje, mensaje.sizeMensaje);
	nodo->colaMensajeria = mensaje.colaMensajeria;
	nodo->clientID = mensaje.clientID;
	return nodo;
}

int agregarMensajeACola(int socketEmisor, cola tipoCola, int idCorrelativo) {

	estructuraMensaje mensajeNuevo;

	recv(socketEmisor, &mensajeNuevo.sizeMensaje, sizeof(int), MSG_WAITALL);
	mensajeNuevo.mensaje = malloc(mensajeNuevo.sizeMensaje);
	recv(socketEmisor, mensajeNuevo.mensaje, mensajeNuevo.sizeMensaje, MSG_WAITALL);

	uint32_t id = getIDMensaje();

	mensajeNuevo.id = id;
	mensajeNuevo.idCorrelativo = idCorrelativo;
	mensajeNuevo.estado = ESTADO_NUEVO;
	mensajeNuevo.colaMensajeria = tipoCola;

	imprimirEstructuraDeDatos(mensajeNuevo);

	sem_wait(&mutexColas);
	for (int i = 0; i < list_size(getListaSuscriptoresByNum(tipoCola)); i++) {
		suscriptor* sus;
		sus = (suscriptor *) (list_get(getListaSuscriptoresByNum(tipoCola), i));
		mensajeNuevo.clientID = sus->clientID;
		list_add(getColaByNum(tipoCola), generarNodo(mensajeNuevo));
	}

	log_info(loggerOficial, "Llegó un nuevo mensaje a la cola %s. Se le asignó el ID: %d", getCodeStringByNum(tipoCola), id);

	cachearMensaje(mensajeNuevo);

	sem_post(&mutexColas);
	sem_post(&habilitarEnvio);

	free(mensajeNuevo.mensaje);

	return id;
}
