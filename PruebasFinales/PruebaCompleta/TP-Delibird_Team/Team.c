#include "Team.h"

void inicializarVariablesGlobales() {
	config = config_create("team.config");
	int imprimirPorConsolaLogOficial = config_get_int_value(config,"PRINT_OFICIAL");
	int imprimirPorConsolaLogNoOficial = config_get_int_value(config,"PRINT_NO_OFICIAL");
	logger = log_create("team_logs", "Team", imprimirPorConsolaLogNoOficial, LOG_LEVEL_TRACE);
	loggerOficial = log_create(config_get_string_value(config, "LOG_FILE"),"Delibird - Team",imprimirPorConsolaLogOficial, LOG_LEVEL_TRACE);
	loggerResultadosFinales = log_create(config_get_string_value(config, "LOG_FILE"), "Delibird - Team", 1, LOG_LEVEL_TRACE);
	listaHilos = list_create();
	team = malloc(sizeof(t_team));
	team->entrenadores = list_create();
	team->objetivosNoAtendidos = list_create();
	listaDeReady = list_create(); //esto se necesita para el FIFO.
	listaDeBloqued = list_create(); //esto es necesario??
	ipServidor = config_get_string_value(config, "IP_BROKER");
	puertoServidor = config_get_string_value(config, "PUERTO");
	tiempoDeEspera = atoi(config_get_string_value(config, "CONNECTION_RETRY"));
	team->algoritmoPlanificacion = obtenerAlgoritmoPlanificador();
	listaCondsEntrenadores = list_create();
	listaPosicionesInternas = list_create();
	listaPosicionesBackUp =list_create();
	especiesRecibidas = list_create();
	idsDeCatch = list_create();
	idsDeGet = list_create();
	alfa =(float)atof(config_get_string_value(config, "ALFA"));
	socketBrokerApp = malloc(sizeof(int));
	socketBrokerLoc = malloc(sizeof(int));
	socketBrokerCau = malloc(sizeof(int));
	socketGameboy = malloc(sizeof(int));
	brokerConectado = false;

	ciclosDeCPUTotales = 0;
	cambiosDeContexto = 0;
	deadlocksResueltos = 0;

	hayEntrenadorDesalojante = false;
	yaTengoID = false;

	//inicializo el mutex para los mensajes que llegan del broker
	sem_init(&mutexMensajes, 0, 1);
	sem_init(&mutexEntrenadores,0,1);
	sem_init(&mutexAPPEARED, 0, 1);
	sem_init(&mutexLOCALIZED, 0, 1);
	sem_init(&mutexAPPEARED_LOCALIZED, 0, 1);
	sem_init(&mutexCAUGHT, 0, 1);
	sem_init(&mutexCATCH, 0, 1);
	sem_init(&mutexOBJETIVOS, 0, 1);
	sem_init(&mutexListaPosiciones, 0, 1);
	sem_init(&mutexListaPosicionesBackup, 0, 1);
	sem_init(&mutexListaObjetivosOriginales, 0, 1);
	sem_init(&mutexEspeciesRecibidas, 0, 1);
	sem_init(&mutexidsGet, 0, 1);
	sem_init(&mutexListaDeReady, 0, 1);
	sem_init(&mutexListaCatch, 0, 1);

	sem_init(&ejecutando, 0, 0);
	sem_init(&procesoEnReady,0,0);
	sem_init(&conexionCreada, 0, 0);
	sem_init(&reconexion, 0, 0);
	sem_init(&resolviendoDeadlock,0,0);
	sem_init(&posicionesPendientes, 0, 0);
	sem_init(&entrenadorDisponible,0,0);
	sem_init(&semGetsEnviados, 0, 0);



	log_debug(logger, "Se ha iniciado un Team.");
}

void array_iterate_element(char** strings, void (*closure)(char*, t_list*),t_list *lista) {

	while (*strings != NULL) {
		closure(*strings, lista);
		strings++;
	}
}

void enlistar(char *elemento, t_list *lista) {
	if (elemento != NULL)
		list_add(lista, elemento);
}

//implementacion generica para obtener de configs
void obtenerDeConfig(char *clave, t_list *lista) {
	char** listaDeConfig;
	listaDeConfig = config_get_array_value(config, clave);

	array_iterate_element(listaDeConfig, enlistar, lista);
	free(listaDeConfig);
}

void destruirEntrenadores() {
	for (int i = 0; i < list_size(team->entrenadores); i++) {
		t_entrenador* entrenadorABorrar = (t_entrenador*) (list_get(team->entrenadores, i));

		sem_destroy(&semEntrenadores[i]);
		sem_destroy(&semEntrenadoresRR[i]);
		list_destroy_and_destroy_elements(entrenadorABorrar->objetivosOriginales,destructorGeneral);
		list_destroy_and_destroy_elements(entrenadorABorrar->pokemones,	destructorGeneral);
		free(entrenadorABorrar->pokemonAAtrapar.pokemon);

		if(entrenadorABorrar->datosDeadlock.pokemonAIntercambiar != NULL)
			free(entrenadorABorrar->datosDeadlock.pokemonAIntercambiar);
	}

	list_destroy_and_destroy_elements(team->entrenadores, destructorGeneral);
}

/* Liberar memoria al finalizar el programa */
void liberarMemoria() {
	//destruirEntrenadores();
	//asumo que el list_destroy hace los free correspondientes.
	list_destroy(team->objetivosNoAtendidos);
	list_destroy_and_destroy_elements(listaHilos, destructorGeneral);
	list_destroy_and_destroy_elements(listaDeReady, destructorGeneral);
	list_destroy_and_destroy_elements(listaDeBloqued, destructorGeneral);
	list_destroy_and_destroy_elements(idsDeCatch, destructorGeneral);
	list_destroy_and_destroy_elements(listaPosicionesInternas, destructorGeneral);
	list_destroy_and_destroy_elements(listaCondsEntrenadores, destructorGeneral);

	config_destroy(config);
	log_destroy(loggerOficial);
	log_info(logger, "Memoria liberada.");
	log_destroy(logger);
}
void destruirSemaforos(){
	sem_destroy(&mutexMensajes);
	sem_destroy(&mutexEntrenadores);
	sem_destroy(&mutexAPPEARED);
	sem_destroy(&mutexLOCALIZED);
	sem_destroy(&mutexCAUGHT);
	sem_destroy(&mutexOBJETIVOS);
	sem_destroy(&ejecutando);
	sem_destroy(&procesoEnReady);
	sem_destroy(&conexionCreada);
	sem_destroy(&reconexion);
	sem_destroy(&resolviendoDeadlock);
}
void liberarConexiones(){
	close(*socketBrokerApp);
	close(*socketBrokerLoc);
	close(*socketBrokerCau);
	close(*socketGameboy);
	free(socketBrokerApp);
	free(socketBrokerLoc);
	free(socketBrokerCau);
	free(socketGameboy);
}
void imprimirResultadosDelTeam(){
	log_info(loggerResultadosFinales,"El proceso Team ha finalizado.");
	imprimirEstadoFinalEntrenadores(loggerResultadosFinales);
	log_info(loggerOficial,"Ciclos de CPU totales = %d", ciclosDeCPUTotales);
	log_info(loggerOficial,"Cambios de contexto realizados = %d", cambiosDeContexto);
	
	for(int id = 0; id < list_size(team->entrenadores);id++){
		t_entrenador* entrenador = list_get(team->entrenadores, id);
		log_info(loggerOficial, "Cambios de contexto del entrenador %d = %d",
				 entrenador->id, entrenador->cambiosDeContexto);
	}
	
	log_info(loggerOficial, "Deadlocks detectados y resueltos = %d", deadlocksResueltos);

	log_info(logger,"El proceso Team ha finalizado.");
	log_info(logger,"Ciclos de CPU totales = %d", ciclosDeCPUTotales);
	log_info(logger,"Cambios de contexto realizados = %d", cambiosDeContexto);
	//TODO - loggear cambios de contexto por entrenador.
	log_info(logger, "Deadlocks detectados y resueltos = %d", deadlocksResueltos);

}

bool elementoEstaEnLista(t_list *lista, char *elemento) {
	bool verifica = false;
	for (int i = 0; i < list_size(lista); i++) {
		if (strcmp(list_get(lista, i), elemento))
			verifica = true;
	}
	return verifica;
}

void inicializarSemEntrenadores() {
	semEntrenadores = malloc(list_size(team->entrenadores) * sizeof(sem_t));
	semEntrenadoresRR = malloc(list_size(team->entrenadores) * sizeof(sem_t));

	for (int j = 0; j < list_size(team->entrenadores); j++) {
		sem_init(&(semEntrenadores[j]), 0, 0);
		sem_init(&(semEntrenadoresRR[j]), 0, 0);
	}
}

void crearHilosDeEntrenadores() {
	for (int i = 0; i < list_size(team->entrenadores); i++) {
		crearHiloEntrenador(list_get(team->entrenadores, i));
     	//Que cada hilo se bloquee a penas empieza.
	}
}

void finalizarProceso(){
	imprimirResultadosDelTeam();
	liberarConexiones();
	destruirSemaforos();
	//liberarMemoria();
	exit(0);
}

void imprimirEstadoFinalEntrenadores(t_log* loggerUsado) {
	log_info(loggerUsado, "Estado final de los entrenadores:");
	for (int i = 0; i < list_size(team->entrenadores); i++) {
		t_entrenador* entrenador = list_get(team->entrenadores, i);
		log_info(loggerUsado, "Entrenador id: %d", entrenador->id);
		log_info(loggerUsado, "  Objetivos Originales:");
		imprimirListaDeCadenas(entrenador->objetivosOriginales, loggerUsado);
		log_info(loggerUsado, "  Pokemones:");
		imprimirListaDeCadenas(entrenador->pokemones, loggerUsado);
	}
}

int main() {
	signal(SIGINT,finalizarProceso);

	inicializarVariablesGlobales();

	generarEntrenadores();

	setearObjetivosDeTeam();

	inicializarSemEntrenadores();

	crearHilosDeEntrenadores();

	crearHiloPlanificador();

	if(noSeCumplieronLosObjetivos()){

		//Hasta aca es thread safe

		//Creo conexion con Gameboy
		conectarGameboy();

		//Se suscribe el Team a las colas

		pthread_t hiloConexiones;
		pthread_create(&hiloConexiones,NULL,(void*)crearConexionesYSuscribirseALasColas,NULL);
		pthread_detach(hiloConexiones);

		enviarGetSegunObjetivo(ipServidor,puertoServidor);
	}

	//El TEAM finaliza cuando termine de ejecutarse el planificador, que sera cuando se cumplan todos los objetivos.
	pthread_join(hiloPlanificador, NULL);

	imprimirEstadoFinalEntrenadores(logger);

	log_info(logger, "Finalizó la conexión con el servidor\n");

	finalizarProceso();
	return 0;
}

