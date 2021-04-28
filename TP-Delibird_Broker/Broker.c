#include "Broker.h"


void inicializarVariablesGlobales() {
	config = config_create("broker.config");
	int imprimirPorConsolaLogOficial = config_get_int_value(config,"PRINT_OFICIAL");
	int imprimirPorConsolaLogNoOficial = config_get_int_value(config,"PRINT_NO_OFICIAL");

	logger = log_create("broker_logs", "Broker", imprimirPorConsolaLogNoOficial, LOG_LEVEL_TRACE);
	loggerOficial = log_create(config_get_string_value(config,"LOG_FILE"),"Delibird - Broker", imprimirPorConsolaLogOficial, LOG_LEVEL_TRACE);

	inicializarColasYListas();
	inicializarCache();

	sem_init(&mutexColas, 0, 1);
	sem_init(&habilitarEnvio, 0, 0);
	sem_init(&mutex_regParticiones, 0, 1);

	globalIDProceso = config_get_int_value(config,"PROCESOS_REGISTRADOS")+1;
	globalIDMensaje = config_get_int_value(config,"MENSAJES_REGISTRADOS")+1;
}


void destructorRegistrosCache(void * elemento){
	registroCache * reg = (registroCache *) elemento;
    list_destroy_and_destroy_elements(reg->procesosALosQueSeEnvio,(void *)destructorGeneral);
    list_destroy_and_destroy_elements(reg->procesosQueConfirmaronRecepcion,(void *)destructorGeneral);
    free(reg);
}
void destruirVariablesGlobales() {
	log_destroy(loggerOficial);
	config_destroy(config);

	list_destroy_and_destroy_elements(suscriptoresNEW,(void *)destructorGeneral);
	list_destroy_and_destroy_elements(suscriptoresAPP,(void *)destructorGeneral);
	list_destroy_and_destroy_elements(suscriptoresGET,(void *)destructorGeneral);
	list_destroy_and_destroy_elements(suscriptoresLOC,(void *)destructorGeneral);
	list_destroy_and_destroy_elements(suscriptoresCAT,(void *)destructorGeneral);
	list_destroy_and_destroy_elements(suscriptoresCAU,(void *)destructorGeneral);

	list_destroy_and_destroy_elements(NEW_POKEMON, (void *) destructorNodos);
	list_destroy_and_destroy_elements(APPEARED_POKEMON, (void *) destructorNodos);
	list_destroy_and_destroy_elements(CATCH_POKEMON, (void *) destructorNodos);
	list_destroy_and_destroy_elements(CAUGHT_POKEMON, (void *) destructorNodos);
	list_destroy_and_destroy_elements(GET_POKEMON, (void *) destructorNodos);
	list_destroy_and_destroy_elements(LOCALIZED_POKEMON, (void *) destructorNodos);

	list_destroy_and_destroy_elements(idCorrelativosRecibidos,(void *) destructorGeneral);

	list_destroy_and_destroy_elements(registrosDeCache,(void*)destructorRegistrosCache);

    free(cacheBroker);
}

void liberarSocket(int* socket) {
	free(socket);
}

void inicializarColasYListas() {

	NEW_POKEMON = list_create();
	APPEARED_POKEMON = list_create();
	CATCH_POKEMON = list_create();
	CAUGHT_POKEMON = list_create();
	GET_POKEMON = list_create();
	LOCALIZED_POKEMON = list_create();

	suscriptoresNEW = list_create();
	suscriptoresAPP = list_create();
	suscriptoresGET = list_create();
	suscriptoresLOC = list_create();
	suscriptoresCAT = list_create();
	suscriptoresCAU = list_create();

	idCorrelativosRecibidos = list_create();

	registrosDeCache = list_create();
	registrosDeParticiones = list_create();

}

int getSocketEscuchaBroker() {

	/*char * ipEscucha = malloc(
			strlen(config_get_string_value(config, "IP_BROKER")) + 1);*/
	char * ipEscucha = config_get_string_value(config, "IP_BROKER");

	/*char * puertoEscucha = malloc(
			strlen(config_get_string_value(config, "PUERTO_BROKER")) + 1);*/
	char * puertoEscucha = config_get_string_value(config, "PUERTO_BROKER");

	int socketEscucha = crearConexionServer(ipEscucha, puertoEscucha);

	log_info(logger, "Se ha iniciado el servidor broker\n");
	log_info(logger,
			"El servidor está configurado y a la espera de un cliente. Número de socket servidor: %d",
			socketEscucha);


	/*free(ipEscucha);
	free(puertoEscucha);*/

	return socketEscucha;

}



void* deserializarPayload(int socketSuscriptor) {
	log_debug(logger, "deserializarPayload");
	int msjLength;
	recv(socketSuscriptor, &msjLength, sizeof(int), MSG_WAITALL);
	log_debug(logger, "%d", msjLength);
	void* mensaje = malloc(msjLength);
	recv(socketSuscriptor, mensaje, msjLength, MSG_WAITALL);
	return mensaje;
}

t_list * getColaByNum(int nro) {
	t_list* lista[6] = { NEW_POKEMON, APPEARED_POKEMON, CATCH_POKEMON,
			CAUGHT_POKEMON, GET_POKEMON, LOCALIZED_POKEMON };
	return lista[nro];
}

t_list* getListaSuscriptoresByNum(opCode nro) {
	t_list* lista[6] = { suscriptoresNEW, suscriptoresAPP, suscriptoresCAT,
			suscriptoresCAU, suscriptoresGET, suscriptoresLOC };
	return lista[nro];
}
void eliminarSuscriptor(t_list* listaSuscriptores, uint32_t clientID){
	bool existeClientID(void * _suscriptor){
		   suscriptor* sus = (suscriptor *) _suscriptor;
		   return sus->clientID==clientID;
	   }
	list_remove_and_destroy_by_condition(listaSuscriptores, (void*)existeClientID,(void*)destructorGeneral);
}

void desuscribir(uint32_t clientID, cola colaSuscripcion) {

	log_info(logger, "Se procedera a desuscribir de la cola %s al suscriptor con clientID: %d", getCodeStringByNum(colaSuscripcion),clientID);
	int socketCliente = getSocketActualDelSuscriptor(clientID, colaSuscripcion);
	close(socketCliente);
	eliminarSuscriptor(getListaSuscriptoresByNum(colaSuscripcion), clientID);
}


int getSocketActualDelSuscriptor(uint32_t clientID, cola colaSuscripcion) {

	suscriptor* sus;
	sus = buscarSuscriptor(clientID, colaSuscripcion);
	return sus->socketCliente;
}


suscriptor * buscarSuscriptor(uint32_t clientID, cola codSuscripcion){
	   bool existeClientID(void * _suscriptor){
		   suscriptor* sus = (suscriptor *) _suscriptor;
		   return sus->clientID==clientID;
	   }
	   t_list * listaSuscriptores = getListaSuscriptoresByNum(codSuscripcion);
	   return list_find(listaSuscriptores,(void *)existeClientID);
}



uint32_t getIDMensaje() {
	char * stringIDMsg = string_itoa(globalIDMensaje);
	config_set_value(config,"MENSAJES_REGISTRADOS",stringIDMsg);
	config_save(config);
	free(stringIDMsg);
	return globalIDMensaje++;
}

uint32_t getIDProceso() {
	char * stringIDProceso = string_itoa(globalIDProceso);
	config_set_value(config,"PROCESOS_REGISTRADOS", stringIDProceso);
	config_save(config);
	free(stringIDProceso);
	return globalIDProceso++;
}

void finalizarEjecucion(){
	destruirVariablesGlobales();
	close(copiaSocketGlobal);
	log_info(logger,"El broker ha finalizado su ejecucion");
	log_destroy(logger);
	exit(0);
}

int main() {
	signal(SIGUSR1,dumpCache);
	signal(SIGINT,finalizarEjecucion);

	inicializarVariablesGlobales();
	log_info(logger, "PID BROKER: %d", getpid());

	int socketEscucha = getSocketEscuchaBroker();
	copiaSocketGlobal=socketEscucha;

	empezarAAtenderCliente(socketEscucha);
	atenderColas();

	return 0;

}
