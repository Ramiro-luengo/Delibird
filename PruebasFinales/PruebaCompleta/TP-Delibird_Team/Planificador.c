#include "Team.h"

void crearHiloPlanificador(){
	pthread_create(&hiloPlanificador, NULL, (void*) planificador, NULL);

	pthread_t hiloPlanificadorLargoPlazo;
	pthread_create(&hiloPlanificadorLargoPlazo, NULL, (void*)planificadorDeLargoPlazo, NULL);
	pthread_detach(hiloPlanificadorLargoPlazo);
}

void registrarDeadlockResuelto(){
	deadlocksResueltos++;
}

e_algoritmo obtenerAlgoritmoPlanificador() {
	char* algoritmo;

	algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

	if (strcmp(algoritmo, "FIFO") == 0) {
		return FIFO;
	} else if (strcmp(algoritmo, "RR") == 0) {
		return RR;
	} else if (strcmp(algoritmo, "SJFCD") == 0) {
		return SJFCD;
	} else if (strcmp(algoritmo, "SJFSD") == 0) {
		return SJFSD;
	} else {
		log_info(logger,
				"No se ingresó un algoritmo válido en team.config. Se toma FIFO por defecto.\n");
		return FIFO;
	}
}

//Esta funcion se podria codear para que sea una funcion generica, pero por el momento solo me sirve saber si está o no en ready.
bool estaEnEspera(t_entrenador *trainer) { // @suppress("Type cannot be resolved")
	bool verifica = false;
	sem_wait(&mutexEntrenadores);
	if (trainer->estado == NUEVO || (trainer->estado == BLOQUEADO && !trainer->suspendido))
		verifica = true;

	sem_post(&mutexEntrenadores);
	return verifica;
}

float calcularDistancia(int posX1, int posY1, int posX2, int posY2) {
	int cat1, cat2;
	float distancia;

	cat1 = abs(posX2 - posX1);
	cat2 = abs(posY2 - posY1);
	distancia = sqrt(pow(cat1, 2) + pow(cat2, 2));

	return distancia;
}

//setea la distancia de todos los entrenadores del team al pokemon localizado
t_dist *setearDistanciaEntrenadores(int id, int posX, int posY) {
	//t_entrenador *entrenador = malloc(sizeof(t_entrenador));
	t_entrenador *entrenador;
	t_dist *distancia = malloc(sizeof(t_dist));

	entrenador = list_get(team->entrenadores, id);

	distancia->dist = calcularDistancia(entrenador->pos[0], entrenador->pos[1],posX, posY);
	distancia->id = id;

	return distancia;
}

bool menorDist(void *dist1, void *dist2) {
	bool verifica = false;

	if (((t_dist*) dist1)->dist < ((t_dist*) dist2)->dist)
		verifica = true;

	return verifica;
}

bool puedaAtraparPokemones(t_entrenador *entrenador){
	sem_wait(&mutexEntrenadores);
	bool puede = list_size(entrenador->pokemones) < entrenador->cantidadMaxDePokes;
	sem_post(&mutexEntrenadores);
	return puede;
}

//Retorna id -1 si no encuentra un entrenador para planificar.
int entrenadorMasCercanoEnEspera(int posX, int posY) {
	t_list* listaDistancias = list_create();
	t_dist * distancia;
	int idEntrenadorConDistMenor = -1;
	int idEntrenadorAux;
	int j = 0;

	sem_wait(&mutexEntrenadores);
	for (int i = 0; i < list_size(team->entrenadores); i++) {
		distancia = setearDistanciaEntrenadores(i, posX, posY);
		list_add(listaDistancias, distancia);
	}
	sem_post(&mutexEntrenadores);

	list_sort(listaDistancias, menorDist);

	while(j < list_size(listaDistancias)){

		idEntrenadorAux = ((t_dist*) list_get(listaDistancias, j))->id;

		if(estaEnEspera(((t_entrenador*) list_get(team->entrenadores,idEntrenadorAux))) && puedaAtraparPokemones((t_entrenador*)list_get(team->entrenadores,idEntrenadorAux))){
			idEntrenadorConDistMenor = idEntrenadorAux;
			break;
		}

		j++;
	}

	list_destroy_and_destroy_elements(listaDistancias,(void *)destructorGeneral);

	return idEntrenadorConDistMenor;

}

t_list *obtenerEntrenadoresReady(){
	t_list *entrenadoresReady = list_create();
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));

	for(int i = 0;i < list_size(team->entrenadores);i++){
		entrenador = list_get(team->entrenadores,i);

		sem_wait(&mutexEntrenadores);
		if(entrenador->estado == LISTO)
			list_add(entrenadoresReady,entrenador);
		sem_post(&mutexEntrenadores);
		}

	return entrenadoresReady;
}

bool noSeCumplieronLosObjetivos(){
	bool verifica = false;
	t_entrenador *entrenador;

	int i = 0;
	sem_wait(&mutexEntrenadores);
	while(i < list_size(team->entrenadores)){
		entrenador = list_get(team->entrenadores,i);

		if(entrenador->estado != FIN){
			verifica = true;
			break;
		}
		i++;
	}
	sem_post(&mutexEntrenadores);

	return verifica;
}


int ponerEnReadyAlMasCercano(int x, int y, char* pokemon){
	t_entrenador* entrenadorMasCercano;
	int idEntrenadorMasCercano;
	idEntrenadorMasCercano = entrenadorMasCercanoEnEspera(x,y);

	if(idEntrenadorMasCercano != -1){
		entrenadorMasCercano = (t_entrenador*) list_get(team->entrenadores,idEntrenadorMasCercano);
		/*
		sem_wait(&mutexOBJETIVOS);
		list_remove_by_condition(team->objetivosNoAtendidos,esUnObjetivo);
		sem_post(&mutexOBJETIVOS);
		*/
		sem_wait(&mutexEntrenadores);
		entrenadorMasCercano->estado = LISTO;
		strcpy(entrenadorMasCercano->pokemonAAtrapar.pokemon, pokemon);
		entrenadorMasCercano->pokemonAAtrapar.pos[0] = x;
		entrenadorMasCercano->pokemonAAtrapar.pos[1] = y;
		sem_post(&mutexEntrenadores);

		sem_wait(&mutexListaDeReady);
		list_add(listaDeReady,entrenadorMasCercano);
		sem_post(&mutexListaDeReady);
	}
	else{
		log_error(logger,"No tengo entrenadores libres");
		sem_post(&posicionesPendientes);
//		sem_post(&entrenadorDisponible);
	}

	return idEntrenadorMasCercano;
}

bool menorEstimacion(void* entrenador1, void* entrenador2) {
	float estimadoEntrenador1 = ((t_entrenador*)entrenador1)->datosSjf.estimadoRafagaAct;
	float estimadoEntrenador2 = ((t_entrenador*)entrenador2)->datosSjf.estimadoRafagaAct;

	return estimadoEntrenador1 <= estimadoEntrenador2;
}

t_entrenador* entrenadorConMenorRafaga(){


		list_sort(listaDeReady,menorEstimacion);
		t_entrenador* entrenador = list_get(listaDeReady,0);
		/*
		for(int i = 0;i < list_size(listaDeReady);i++){
			t_entrenador *entr = list_get(listaDeReady,i);
			log_info(logger,"Id del entrenador en lista de ready: %d",entr->id);
		}*/
		return entrenador;
}


void destructorPosiciones(void * posicion){
	t_posicionEnMapa * posic  = (t_posicionEnMapa * ) posicion;
	free(posic->pokemon);
	free(posic);
}


bool hayNuevoEntrenadorConMenorRafaga(t_entrenador* entrenador){
	bool verifica = false;

	//verificarPokemonesEnMapaYPonerEnReady();

	sem_wait(&mutexListaDeReady);
	if(!list_is_empty(listaDeReady)){
		list_sort(listaDeReady,menorEstimacion);
		t_entrenador* entrenador2 = list_get(listaDeReady,0);
		sem_post(&mutexListaDeReady);

		log_error(logger,"Estimacion entrenador %d en ejec: %f y el entrenador en lista de ready: %f",entrenador->id,entrenador->datosSjf.estimadoRafagaAct,entrenador2->datosSjf.estimadoRafagaAct);
		if(entrenador->datosSjf.estimadoRafagaAct > entrenador2->datosSjf.estimadoRafagaAct){
            hayEntrenadorDesalojante=true;
            verifica = true;
        }
	}
	else{
		sem_post(&mutexListaDeReady);
	}
	return verifica;
}

void activarHiloDe(int id){
	sem_post(&semEntrenadores[id]);
}

void activarHiloDeRR(int id, int quantum){
	activarHiloDe(id);

	int valorSemRR;
	sem_getvalue(&semEntrenadoresRR[id],&valorSemRR);

	for(int i = 0;i < valorSemRR;i++)
		sem_wait(&semEntrenadoresRR[id]);

	for(int i = 0; i<quantum; i++)
		sem_post(&semEntrenadoresRR[id]);
}

void verificarPokemonesEnMapaYPonerEnReady(){
	sem_wait(&mutexListaPosiciones);
	if(!list_is_empty(listaPosicionesInternas)){
		t_posicionEnMapa* pos;
		pos = list_remove(listaPosicionesInternas, 0);
		sem_post(&mutexListaPosiciones);

		if(estaEnLosObjetivosOriginales(pos->pokemon)){
			ponerEnReadyAlMasCercano(pos->pos[0], pos->pos[1], pos->pokemon);
			sem_post(&procesoEnReady);
		}
		else{
			sem_post(&entrenadorDisponible);
		}

		free(pos->pokemon);
		free(pos);
	}
	else{
		sem_post(&mutexListaPosiciones);
	}
}

void planificadorDeLargoPlazo(){
	//sem_wait(&semGetsEnviados); //si no mandamos los gets, el planificador deberia poder funcionar para gameboy
//	int cant=0;
//	sem_getvalue(&entrenadorDisponible,&cant);
//	log_info(logger,"Entrenadores disponibles: %d",cant);
	while(1){

//		log_error(logger,"Entrando al semaforo de entrenadores disponibles");
		sem_wait(&entrenadorDisponible);

//		log_error(logger,"Entrando al semaforo de posiciones pendientes");
		sem_wait(&posicionesPendientes);

		verificarPokemonesEnMapaYPonerEnReady();
	}
}

void planificarFifo(){
		log_debug(logger,"Se activa el planificador");

		while(1){
			t_entrenador *entrenador;

			sem_wait(&procesoEnReady);

			if(noSeCumplieronLosObjetivos()){
				sem_wait(&mutexListaDeReady);

				if(!list_is_empty(listaDeReady)){
					sem_post(&mutexListaDeReady);
					log_debug(logger,"Hurra, tengo algo en ready");

					sem_wait(&mutexEntrenadores);
					entrenador = list_get(listaDeReady,0);
					entrenador->estado = EJEC;
					sem_post(&mutexEntrenadores);

					sem_wait(&mutexListaDeReady);
					list_remove(listaDeReady,0);
					sem_post(&mutexListaDeReady);

					activarHiloDe(entrenador->id);
					sem_wait(&ejecutando);
				}
				else{
					sem_post(&mutexListaDeReady);
				}
			}
			else{
				log_debug(logger,"Se cumplieron los objetivos, finaliza el Planificador");
				break;
			}
		}
}

void planificarRR(){
	int quantum = atoi(config_get_string_value(config, "QUANTUM"));
	log_debug(logger,"Se activa el planificador");
//	int cant=0;
	t_entrenador *entrenador;

	while(1){

//			sem_getvalue(&procesoEnReady,&cant);
			sem_wait(&procesoEnReady);

			if(noSeCumplieronLosObjetivos()){

				sem_wait(&mutexListaDeReady);
				if(!list_is_empty(listaDeReady)){

					log_debug(logger,"Hurra, tengo algo en ready");

					for(int i = 0;i < list_size(listaDeReady);i++){
						log_info(logger,"id entrenador en ready: %d",((t_entrenador*)list_get(listaDeReady,i))->id);
					}

					entrenador = list_remove(listaDeReady,0);
					sem_post(&mutexListaDeReady);

					sem_wait(&mutexEntrenadores);
					entrenador->estado = EJEC;
					sem_post(&mutexEntrenadores);

					activarHiloDeRR(entrenador->id, quantum);
					sem_wait(&ejecutando);
				}
				else{
					sem_post(&mutexListaDeReady);
				}
			}
			else{
				log_debug(logger,"Se cumplieron los objetivos, finaliza el Planificador");
				break;
			}
		}
}

void planificarSJFsinDesalojo(){
	log_debug(logger,"Se activa el planificador");
	while(1){
		t_entrenador *entrenador;

		sem_wait(&procesoEnReady);

		if(noSeCumplieronLosObjetivos()){

			sem_wait(&mutexListaDeReady);							//<-- Hasta que no defino bien la lista de ready nadie la toca
			if(!list_is_empty(listaDeReady)){

				log_debug(logger,"Hurra, tengo algo en ready");

				sem_wait(&mutexEntrenadores);
				entrenador = entrenadorConMenorRafaga();
				entrenador->estado = EJEC;
				sem_post(&mutexEntrenadores);

				list_remove(listaDeReady,0);


				sem_post(&mutexListaDeReady);						//<--
				activarHiloDe(entrenador->id);
				sem_wait(&ejecutando);
			}
			else{
				sem_post(&mutexListaDeReady);						//<--
			}
		}
		else{
			log_debug(logger,"Se cumplieron los objetivos, finaliza el Planificador");
			break;
	}   }
}

void planificarSJFconDesalojo(){
	log_debug(logger,"Se activa el planificador");
	while(1){
			t_entrenador *entrenador;

			sem_wait(&procesoEnReady);

			if(noSeCumplieronLosObjetivos()){

				sem_wait(&mutexListaDeReady);							//<-- Hasta que no defino bien la lista de ready nadie la toca
				if(!list_is_empty(listaDeReady)){

					log_debug(logger,"Hurra, tengo algo en ready");
                    if(!hayEntrenadorDesalojante){
                        sem_wait(&mutexEntrenadores);
                        entrenador = entrenadorConMenorRafaga();
                        entrenador->estado = EJEC;
                        sem_post(&mutexEntrenadores);
                    }
                    else{
                        sem_wait(&mutexEntrenadores);
                        entrenador = list_get(listaDeReady,0);
                        entrenador->estado = EJEC;
                        sem_post(&mutexEntrenadores);
                        hayEntrenadorDesalojante = false;
                    }

                        list_remove(listaDeReady,0);
                        sem_post(&mutexListaDeReady);					//<--
                        activarHiloDe(entrenador->id);
                        sem_wait(&ejecutando);
				}
				else{
					sem_post(&mutexListaDeReady);						//<--
				}
			}
			else{
				log_debug(logger,"Se cumplieron los objetivos, finaliza el Planificador");
				break;
		}  }
}

bool puedeExistirDeadlock(){
	t_entrenador *entrenador;
	bool verifica = false;

	for(int i = 0; i < list_size(team->entrenadores);i++){
		entrenador = list_get(team->entrenadores,i);

		sem_wait(&mutexEntrenadores);
		if(entrenador->estado != FIN){
			sem_post(&mutexEntrenadores);
			verifica = true;
			break;
		}
		sem_post(&mutexEntrenadores);
	}

	return verifica;
}

//Obtiene una lista con los elementos de la primera lista que no estan en la segunda lista
//
//[Pikachu, Charmander] - [Charmander - Squirtle - Bulbasaur]
//
t_list *pokesNoObjetivoEnDeadlock(t_list *pokemonesPosibles,t_list *pokemonesObjetivoEntrenador){
	t_list *copiaPokemonesObjetivoEntrenador = list_duplicate(pokemonesObjetivoEntrenador);
	t_list *listaFiltrada;

	bool noLoNecesita(void *pokemon){
		bool verifica = true;
		char* objetivo;

		bool coincideCon(void *elemento){
			return string_equals_ignore_case((char*)elemento,objetivo);
		}

		for(int i = 0; i < list_size(copiaPokemonesObjetivoEntrenador);i++){
			objetivo = (char*)list_get(copiaPokemonesObjetivoEntrenador,i);

			if(string_equals_ignore_case((char*)pokemon, objetivo))
			{
				list_remove_by_condition(copiaPokemonesObjetivoEntrenador,coincideCon);
				verifica = false;
				break;
			}

		}
		return verifica;
		}

	listaFiltrada = list_filter(pokemonesPosibles,noLoNecesita);
	list_destroy(copiaPokemonesObjetivoEntrenador);

	return listaFiltrada;
}

bool estaEnDeadlock(void *entrenador){
	bool verifica = false;

	if(((t_entrenador*)entrenador)->cantidadMaxDePokes == list_size(((t_entrenador*)entrenador)->pokemones) && ((t_entrenador*)entrenador)->estado != FIN){
		verifica = true;
		((t_entrenador*)entrenador)->datosDeadlock.estaEnDeadlock = true;
	}
	else{
		((t_entrenador*)entrenador)->datosDeadlock.estaEnDeadlock = false;
	}

	return verifica;
}

bool tieneAlgunoQueNecesita(t_list *lista1, t_list *lista2){
	char *elemento;
	bool verifica = false;

	bool estaEn(void *elem){
		return string_equals_ignore_case((char*)elem,elemento);
	}

	for(int i = 0 ; i < list_size(lista2);i++){
		elemento = list_get(lista2,i);

		if(list_any_satisfy(lista1, estaEn)){
			verifica = true;
			break;
		}
	}

	return verifica;
}



void realizarIntercambio(t_entrenador *entrenador, t_entrenador *entrenadorAIntercambiar){


//		log_info(logger,"Entrenador id: %d",entrenador->id);
//		log_info(logger,"Objetivos:");
//		imprimirListaDeCadenas(entrenador->objetivosOriginales, logger);
//		log_info(logger,"Pokemones:");
//		imprimirListaDeCadenas(entrenador->pokemones, logger);


		t_list *pokemonesNoRequeridos = pokesNoObjetivoEnDeadlock(entrenador->pokemones,entrenador->objetivosOriginales);
//		log_info(logger,"Pokemones no requeridos:");
//		imprimirListaDeCadenas(pokemonesNoRequeridos, logger);

		t_list *pokemonesNoRequeridosAIntercambiar = pokesNoObjetivoEnDeadlock(entrenadorAIntercambiar->pokemones,entrenadorAIntercambiar->objetivosOriginales);

//		log_info(logger,"Entrenador id: %d",entrenadorAIntercambiar->id);
//		log_info(logger,"Objetivos:");
//		imprimirListaDeCadenas(entrenadorAIntercambiar->objetivosOriginales, logger);
//		log_info(logger,"Pokemones:");
//		imprimirListaDeCadenas(entrenadorAIntercambiar->pokemones, logger);
//		log_info(logger,"Pokemones no requeridos:");
//		imprimirListaDeCadenas(pokemonesNoRequeridosAIntercambiar, logger);

		bool pokemonAIntercambiar(void * elemento){
			bool verifica = false;

			for(int i = 0; i < list_size(pokemonesNoRequeridosAIntercambiar);i++){

				if(string_equals_ignore_case((char*)elemento, (char*)list_get(pokemonesNoRequeridosAIntercambiar,i))){
					verifica = true;
					break;
				}
			}
			return verifica;
		}

		char *pokeAIntercambiar = list_find(entrenador->objetivos,pokemonAIntercambiar);
		char *pokeADarQueNoQuiero= malloc(strlen(list_get(pokemonesNoRequeridos,0)) + 1);

		strcpy(pokeADarQueNoQuiero,list_get(pokemonesNoRequeridos,0));
		//Vos tenes uno que yo necesito, ahora tengo yo alguno que vos necesitas?

		log_info(logger,"Pokemon a intercambiar del entrenador %d: %s",entrenador->id,list_get(pokemonesNoRequeridos,0));
		log_info(logger,"Pokemon a intercambiar del entrenador %d: %s",entrenadorAIntercambiar->id,pokeAIntercambiar);

		sem_wait(&mutexEntrenadores);
		entrenador->estado = LISTO;
		strcpy(entrenador->pokemonAAtrapar.pokemon,pokeAIntercambiar);
		entrenador->pokemonAAtrapar.pos[0] = entrenadorAIntercambiar->pos[0];
		entrenador->pokemonAAtrapar.pos[1] = entrenadorAIntercambiar->pos[1];

		entrenador->datosDeadlock.pokemonAIntercambiar = malloc(strlen(pokeADarQueNoQuiero) + 1);

		strcpy(entrenador->datosDeadlock.pokemonAIntercambiar,pokeADarQueNoQuiero);

		entrenador->datosDeadlock.idEntrenadorAIntercambiar = entrenadorAIntercambiar->id;
		sem_post(&mutexEntrenadores);

		sem_wait(&mutexListaDeReady);
		list_add(listaDeReady,entrenador);
		sem_post(&mutexListaDeReady);

		sem_post(&procesoEnReady);
		free(pokeADarQueNoQuiero);
		list_destroy(pokemonesNoRequeridos);
		list_destroy(pokemonesNoRequeridosAIntercambiar);
}

void imprimirListaDeCadenas(t_list * listaDeCadenas, t_log* loggerUsado){
	for(int i=0; i<list_size(listaDeCadenas);i++){
		log_info(loggerUsado,"	%s",(char*)list_get(listaDeCadenas,i));
	}
}
void resolverDeadlock(t_list *entrenadoresEnDeadlock){
	t_entrenador *entrenador = list_get(entrenadoresEnDeadlock,0);
	t_entrenador *entrenadorAIntercambiar;

	bool swapDePokemonesValidos(void *entrPotencial){
		bool verifica;
		t_entrenador *entrPotencial1 = (t_entrenador*)entrPotencial;

		t_list *pokemonesNoRequeridosDeEntrPotencial = pokesNoObjetivoEnDeadlock(entrPotencial1->pokemones,entrPotencial1->objetivosOriginales);

		if(entrPotencial1->id != entrenador->id && tieneAlgunoQueNecesita(entrenador->objetivos,pokemonesNoRequeridosDeEntrPotencial))
			verifica = true;

		list_destroy(pokemonesNoRequeridosDeEntrPotencial);
		return verifica;
	}

	entrenadorAIntercambiar = list_find(entrenadoresEnDeadlock,swapDePokemonesValidos);



	realizarIntercambio(entrenador,entrenadorAIntercambiar);

}


void escaneoDeDeadlock(){
	log_debug(logger,"Se comienza el analisis de deadlock");


	imprimirEstadoFinalEntrenadores(logger);
	if(puedeExistirDeadlock()){
		sem_wait(&mutexEntrenadores);
		t_list *entrenadoresEnDeadlock = list_filter(team->entrenadores,estaEnDeadlock);
		sem_post(&mutexEntrenadores);
		int contadorDeDeadlocks = 0;

		while(!list_is_empty(entrenadoresEnDeadlock)){

			resolverDeadlock(entrenadoresEnDeadlock);
			sem_wait(&resolviendoDeadlock);

			list_destroy(entrenadoresEnDeadlock);

			sem_wait(&mutexEntrenadores);
			entrenadoresEnDeadlock = list_filter(team->entrenadores,estaEnDeadlock);
			sem_post(&mutexEntrenadores);

			contadorDeDeadlocks++;
			registrarDeadlockResuelto();
		}

		list_destroy(entrenadoresEnDeadlock);
		log_info(logger,"Se termino la resolucion de deadlocks");
		log_info(loggerOficial, "Finalizo la resolucion de deadlocks. Deadlocks resueltos = %d.", contadorDeDeadlocks);

		sem_post(&procesoEnReady);
	}
	else{
		log_info(logger,"No hay deadlocks pendientes");
		sem_post(&procesoEnReady);
	}


}

void planificador(){
	switch(team->algoritmoPlanificacion){
			case FIFO:{
				planificarFifo();
				break;
			}
			case RR:{
				planificarRR();
				break;
			}
			case SJFSD:{
				planificarSJFsinDesalojo();
				break;
			}
			case SJFCD:{
				planificarSJFconDesalojo();
				break;
			}
			//en caso que tengamos otro algoritmo usamos la funcion de ese algoritmo
			default:{
				planificarFifo();
				break;
			}
	}
}
