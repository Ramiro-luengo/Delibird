#include "Team.h"

void registrarCambioDeContextoGeneral(){
	cambiosDeContexto++;
}

void registrarCambioDeContexto(t_entrenador* entrenador){
	sem_wait(&mutexEntrenadores);
	entrenador->cambiosDeContexto++;
	sem_post(&mutexEntrenadores);

	registrarCambioDeContextoGeneral();
}

void registrarCicloDeCPU(){
	ciclosDeCPUTotales++;
}

void liberarStringSplitteado(char ** stringSplitteado){
	int index=0;
	while(stringSplitteado[index]!=NULL){  //["asd","qwe","qweqw",NULL]
		free(stringSplitteado[index]);
		index++;
	}
	free(stringSplitteado);
}

/*Arma el Entrenador*/
t_entrenador* armarEntrenador(int id, char *posicionesEntrenador,char *objetivosEntrenador,
	char *pokemonesEntrenador, float estInicialEntrenador) {
	t_entrenador* nuevoEntrenador = malloc(sizeof(t_entrenador));

	t_list *posicionEntrenador = list_create();
	t_list *objetivoEntrenador = list_create();
	t_list *pokemonEntrenador = list_create();

	//"1|2"
	//["1", "2", NULL]
	//posicionEntrenador = "1" -> "2" -> NULL
	//

	//TODO - Solucionar los leaks sin que rompa nada
	//--------------------------------------------------------------------------
	char ** posicionesSplitteadas = string_split(posicionesEntrenador,"|");
	char ** objetivosSplitteados =  string_split(objetivosEntrenador, "|");

	array_iterate_element(posicionesSplitteadas,(void *) enlistar, posicionEntrenador);
	array_iterate_element(objetivosSplitteados, (void *) enlistar, objetivoEntrenador);
	//--------------------------------------------------------------------------

	if(pokemonesEntrenador != NULL){
		char ** pokemonesEntrenadorSplitteados = string_split(pokemonesEntrenador, "|");
		array_iterate_element(pokemonesEntrenadorSplitteados,(void *) enlistar, pokemonEntrenador);
		free(pokemonesEntrenadorSplitteados);
	}

	for (int i = 0; i < 2; i++) {
		nuevoEntrenador->pos[i] = atoi(list_get(posicionEntrenador, i));
	}
	nuevoEntrenador->id = id;
	nuevoEntrenador->cambiosDeContexto = 0;
	nuevoEntrenador->objetivos = objetivoEntrenador;
	nuevoEntrenador->objetivosOriginales = list_duplicate(objetivoEntrenador);
	nuevoEntrenador->pokemones = pokemonEntrenador;
	nuevoEntrenador->estado = NUEVO; // Debugeando de mi cuenta que sin esta linea de codigo solo el ultimo elemento lo pasa a new
	nuevoEntrenador->suspendido = false;
	nuevoEntrenador->datosSjf.duracionRafagaAnt = 0;
	nuevoEntrenador->datosSjf.duracionRafagaAct = 0;
	nuevoEntrenador->datosSjf.estimadoRafagaAnt = estInicialEntrenador;
	nuevoEntrenador->datosSjf.estimadoRafagaAct = estInicialEntrenador;
	//Pongo variable fue desalojado en true, asi en la primer vuelta el algoritmo toma las estimaciones iniciales.
	nuevoEntrenador->datosSjf.fueDesalojado = true;
	nuevoEntrenador->datosDeadlock.estaEnDeadlock = false;
	nuevoEntrenador->cantidadMaxDePokes = list_size(nuevoEntrenador->objetivos);
	nuevoEntrenador->pokemonAAtrapar.pokemon = malloc(MAXSIZE);

	list_destroy(posicionEntrenador);
	liberarStringSplitteado(posicionesSplitteadas);
	free(objetivosSplitteados);

	return nuevoEntrenador;
}

/* Genera los Entrenadores con los datos del Config */
void generarEntrenadores() {
	t_entrenador * unEntrenador;
		t_list* posiciones = list_create();
		t_list* objetivos = list_create();
		t_list* pokemones = list_create();

		obtenerDeConfig("POSICIONES_ENTRENADORES", posiciones);
		obtenerDeConfig("OBJETIVO_ENTRENADORES", objetivos);
		obtenerDeConfig("POKEMON_ENTRENADORES", pokemones);
		float estimacion = atof(config_get_string_value(config, "ESTIMACION_INICIAL"));

	for (int contador = 0; contador < list_size(posiciones); contador++){
		unEntrenador = armarEntrenador(contador, list_get(posiciones, contador),
				list_get(objetivos, contador), list_get(pokemones, contador), estimacion);
		list_add(team->entrenadores, unEntrenador);
	}
	log_info(logger,"Cantidad de posiciones: %d",list_size(posiciones));
	log_info(logger,"Cantidad de entrenadores: %d",list_size(team->entrenadores));
	int cant=0;
	sem_getvalue(&entrenadorDisponible,&cant);
	log_info(logger,"Entrenadores disponibles: %d",cant);

	list_destroy_and_destroy_elements(posiciones,destructorGeneral);
	list_destroy_and_destroy_elements(objetivos,destructorGeneral);
	list_destroy_and_destroy_elements(pokemones,destructorGeneral);
}

void removerObjetivosCumplidos(char *pokemon,t_list *listaObjetivos){

	bool esUnObjetivo(void *objetivo){
			bool verifica = false;

			if(string_equals_ignore_case((char *)objetivo, pokemon))
				verifica = true;

			return verifica;
		}

	list_remove_by_condition(listaObjetivos,esUnObjetivo);
}

void setearObjetivosDeTeam() {
	char *pokemon;

	bool esUnObjetivo(void *objetivo){
		bool verifica = false;

		if(string_equals_ignore_case((char *)objetivo, pokemon))
			verifica = true;

		return verifica;
	}

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		t_entrenador *entrenador;

		entrenador = list_get(team->entrenadores, i);

		for (int j = 0; j < list_size(entrenador->objetivos); j++) {
			list_add(team->objetivosNoAtendidos, list_get(entrenador->objetivos, j));
		}
	}//Usamos la misma referencia que la que tienen los entrenadores.

	for(int i = 0; i < list_size(team->entrenadores); i++){
		t_entrenador *entrenador;

		entrenador = list_get(team->entrenadores, i);

		for (int j = 0; j < list_size(entrenador->pokemones); j++) {

			pokemon = (char *)list_get(entrenador->pokemones,j);

			removerObjetivosCumplidos(pokemon,team->objetivosNoAtendidos);
			removerObjetivosCumplidos(pokemon,entrenador->objetivos);
		}

		seCumplieronLosObjetivosDelEntrenador(entrenador);
	}

	team->objetivosOriginales = list_duplicate(team->objetivosNoAtendidos);
	//Se verifica por deadlock en caso que venga ya sin objetivos globales.
	verificarDeadlock();
}

void crearHiloEntrenador(t_entrenador* entrenador) {
	pthread_t nuevoHilo;
//	t_listaHilos* nodoListaDeHilos = malloc(sizeof(t_listaHilos));

	pthread_create(&nuevoHilo, NULL, (void*) gestionarEntrenador, entrenador);

	pthread_detach(nuevoHilo);
//	nodoListaDeHilos->hilo = nuevoHilo;
//	nodoListaDeHilos->id = entrenador->id;
//
//	list_add(listaHilos, nodoListaDeHilos);

}

void loggearPosicion(t_entrenador* entrenador) {
	log_debug(logger,
			"El entrenador id: %d se movio a la posicion [%d,%d]",
			entrenador->id, entrenador->pos[0], entrenador->pos[1]);
	log_debug(loggerOficial,
			"El entrenador id: %d se movio a la posicion [%d,%d]",
			entrenador->id, entrenador->pos[0], entrenador->pos[1]);
}

void moverXDelEntrenador(t_entrenador *entrenador){

	if(entrenador->pos[0] < entrenador->pokemonAAtrapar.pos[0]){
		entrenador->pos[0]++;
		loggearPosicion(entrenador);
		}
	else if(entrenador->pos[0] > entrenador->pokemonAAtrapar.pos[0]){
		entrenador->pos[0]--;
		loggearPosicion(entrenador);
		}
	else{
		//Si el entrenador ya esta en el X del pokemon, que mueva la Y.
		moverYDelEntrenador(entrenador);
	}

}

void moverYDelEntrenador(t_entrenador *entrenador){

	if(entrenador->pos[1] < entrenador->pokemonAAtrapar.pos[1]){
		entrenador->pos[1]++;
		loggearPosicion(entrenador);
	}
	else if(entrenador->pos[1] > entrenador->pokemonAAtrapar.pos[1]){
		entrenador->pos[1]--;
		loggearPosicion(entrenador);
	}
	else{
		//Si el entrenador ya esta en el Y del pokemon, que mueva la X
		moverXDelEntrenador(entrenador);
	}


}

bool estaEnLosObjetivos(char *pokemon){
	bool esUnObjetivo(void *elemento) {
		bool verifica = false;

		if (string_equals_ignore_case(pokemon, (char *)elemento)) {
			verifica = true;
		}
		else{
		}
		return verifica;
	}

	sem_wait(&mutexOBJETIVOS);
	bool esta = list_any_satisfy(team->objetivosNoAtendidos,esUnObjetivo);
	sem_post(&mutexOBJETIVOS);

	return esta;
}

bool estaEnLosObjetivosOriginales(char *pokemon){
	bool esUnObjetivo(void *elemento) {
			bool verifica = false;

			if (string_equals_ignore_case(pokemon, (char *)elemento)) {
				verifica = true;
			}
			else{
			}
			return verifica;
		}

	sem_wait(&mutexListaObjetivosOriginales);
	bool esta = list_any_satisfy(team->objetivosOriginales,esUnObjetivo);
	sem_post(&mutexListaObjetivosOriginales);
	return esta;
}

void removerPokemonDeListaSegunCondicion(t_list* lista,char *pokemon){

	bool esElPokemonAAtrapar(void *poke){
		bool verifica = false;

		if(string_equals_ignore_case((char*)poke, pokemon))
			verifica = true;

		return verifica;
	}

	list_remove_by_condition(lista,esElPokemonAAtrapar);
}

void removerYDestruirPokemonDeListaSegunCondicion(t_list* lista,char *pokemon){

	bool esElPokemonAAtrapar(void *poke){
		bool verifica = false;

		if(string_equals_ignore_case((char*)poke, pokemon))
			verifica = true;

		return verifica;
	}

	list_remove_and_destroy_by_condition(lista,esElPokemonAAtrapar,destructorGeneral);
}

void verificarDeadlock() {
	sem_wait(&mutexListaObjetivosOriginales);
	if(list_is_empty(team->objetivosOriginales)){
		sem_post(&mutexListaObjetivosOriginales);
		log_info(loggerOficial, "Se inicia la deteccion de deadlock.");
		escaneoDeDeadlock();
	}
	else{
		sem_post(&mutexListaObjetivosOriginales);
	}
}
void seCumplieronLosObjetivosDelEntrenador(t_entrenador* entrenador) {

	sem_wait(&mutexEntrenadores);
	if(list_is_empty(entrenador->objetivos)){
		entrenador->estado = FIN;
		sem_post(&mutexEntrenadores);
	}
	else{
		sem_post(&mutexEntrenadores);
		if(puedaAtraparPokemones(entrenador))
			sem_post(&entrenadorDisponible);
	}
}


void intercambiar(t_entrenador *entrenador){
	t_entrenador *entrenadorParejaIntercambio = (t_entrenador*)list_get(team->entrenadores,entrenador->datosDeadlock.idEntrenadorAIntercambiar);
	char*pokemonAAtrapar = malloc(strlen(entrenador->pokemonAAtrapar.pokemon) + 1 );
	strcpy(pokemonAAtrapar, entrenador->pokemonAAtrapar.pokemon);

	log_debug(logger,"Estoy intercambiando el pokemon %s por %s",entrenador->datosDeadlock.pokemonAIntercambiar,entrenador->pokemonAAtrapar.pokemon);
	log_info(loggerOficial, "Se comienza el intercambio entre el entrenador %d y el entrenador %d", entrenador->id, entrenadorParejaIntercambio->id);

	sem_wait(&mutexEntrenadores);
	list_add(entrenador->pokemones,pokemonAAtrapar);
	removerYDestruirPokemonDeListaSegunCondicion(entrenadorParejaIntercambio->pokemones,pokemonAAtrapar);
	list_add(entrenadorParejaIntercambio->pokemones,entrenador->datosDeadlock.pokemonAIntercambiar);
	removerYDestruirPokemonDeListaSegunCondicion(entrenador->pokemones,entrenador->datosDeadlock.pokemonAIntercambiar);
	sem_post(&mutexEntrenadores);

	removerPokemonDeListaSegunCondicion(entrenador->objetivos,entrenador->pokemonAAtrapar.pokemon);
	removerPokemonDeListaSegunCondicion(entrenadorParejaIntercambio->objetivos,entrenador->datosDeadlock.pokemonAIntercambiar);

	seCumplieronLosObjetivosDelEntrenador(entrenador);
	seCumplieronLosObjetivosDelEntrenador(entrenadorParejaIntercambio);

	//Corresponde al requerimiento de que el intercambio debe demorar 5 ciclos de CPU.
	usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 5000000);

	for(int i = 0; i < 5; i++){
		registrarCicloDeCPU();
	}

	log_debug(logger,"Se finalizo el intercambio entre el entrenador %d y el entrenador %d",entrenador->id,entrenadorParejaIntercambio->id);
	log_info(loggerOficial,"Se finalizo el intercambio entre el entrenador %d y el entrenador %d",entrenador->id,entrenadorParejaIntercambio->id);
}

void gestionarEntrenadorFIFO(t_entrenador *entrenador){
	 while(1){
		sem_wait(&semEntrenadores[entrenador->id]);
		registrarCambioDeContexto(entrenador);

		if(entrenador->estado != FIN){

			if(entrenador->estado == EJEC){

				bool alternadorXY = true;

				while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
					sem_wait(&mutexEntrenadores);

					if(alternadorXY){
						moverXDelEntrenador(entrenador);
					}
					else{
						moverYDelEntrenador(entrenador);
					}
					sem_post(&mutexEntrenadores);

					alternadorXY = !alternadorXY;

					registrarCicloDeCPU();
					usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
				}
				if(!entrenador->datosDeadlock.estaEnDeadlock){

					log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: alcanzo su objetivo y envio un CATCH.", entrenador->id);

					sem_wait(&mutexEntrenadores);
					entrenador->estado = BLOQUEADO;
					entrenador->suspendido = true;
					sem_post(&mutexEntrenadores);

					registrarCambioDeContexto(entrenador);

					enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
				}
				else{
					intercambiar(entrenador);
					sem_post(&resolviendoDeadlock);//Semaforo de finalizacion de deadlock.
				}
				sem_post(&ejecutando);
			 }
		}
		else{
			break;
		}
	}

}

void gestionarEntrenadorRR(t_entrenador* entrenador){

	while(1){
		sem_wait(&semEntrenadores[entrenador->id]);
		registrarCambioDeContexto(entrenador);

		sem_wait(&mutexEntrenadores);
		if(entrenador->estado != FIN){
			sem_post(&mutexEntrenadores);

			sem_wait(&mutexEntrenadores);
			if(entrenador->estado == EJEC){
				sem_post(&mutexEntrenadores);

				bool alternadorXY = true;
				int quantum = atoi(config_get_string_value(config, "QUANTUM"));
				int contadorQuantum = quantum;

				while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
					if(contadorQuantum == 0){
						contadorQuantum = quantum;
						log_debug(logger, "El entrenador %d se quedó sin Quantum. Vuelve a la cola de ready.", entrenador->id);
						log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: Quantum agotado.", entrenador->id);
						registrarCambioDeContexto(entrenador);

						sem_wait(&mutexEntrenadores);
						entrenador->estado = LISTO;
						sem_post(&mutexEntrenadores);

						sem_wait(&mutexListaDeReady);
						list_add(listaDeReady,entrenador);
						sem_post(&mutexListaDeReady);
						sem_post(&procesoEnReady);
						sem_post(&ejecutando);
					}
					sem_wait(&semEntrenadoresRR[entrenador->id]);

					sem_wait(&mutexEntrenadores);

					if(alternadorXY){
						moverXDelEntrenador(entrenador);
					}
					else{
						moverYDelEntrenador(entrenador);
					}
					sem_post(&mutexEntrenadores);

					alternadorXY = !alternadorXY;

					registrarCicloDeCPU();
					usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
					contadorQuantum--;
				}

				if(!entrenador->datosDeadlock.estaEnDeadlock){
					sem_wait(&mutexEntrenadores);
					entrenador->estado = BLOQUEADO;
					entrenador->suspendido = true;
					sem_post(&mutexEntrenadores);

					enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
					registrarCambioDeContexto(entrenador);
					log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: alcanzo su objetivo y envio un CATCH.", entrenador->id);

				}
				else{
					sem_wait(&mutexEntrenadores);
					entrenador->estado = BLOQUEADO;
					sem_post(&mutexEntrenadores);

					intercambiar(entrenador);
					sem_post(&resolviendoDeadlock);//Semaforo de finalizacion de deadlock.
				}
				sem_post(&ejecutando);

			}
			else{
				sem_post(&mutexEntrenadores);
			}
		}
		else{
			sem_post(&mutexEntrenadores);
			break;
		}
	}
}

void gestionarEntrenadorSJFsinDesalojo(t_entrenador* entrenador){
	while(1){
		sem_wait(&semEntrenadores[entrenador->id]);
		registrarCambioDeContexto(entrenador);

		if(entrenador->estado != FIN){

			if(entrenador->estado == EJEC){

					bool alternadorXY = true;
					while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
						sem_wait(&mutexEntrenadores);

						if(alternadorXY){
							moverXDelEntrenador(entrenador);
						}
						else{
							moverYDelEntrenador(entrenador);
						}
						sem_post(&mutexEntrenadores);

						alternadorXY = !alternadorXY;

						registrarCicloDeCPU();
						usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
					//Actualizo estimacion
					entrenador->datosSjf.estimadoRafagaAct-=1;
//					log_error(logger,"Estimado de rafaga actual: %f",entrenador->datosSjf.estimadoRafagaAct);

					entrenador->datosSjf.duracionRafagaAct+=1;
//					log_error(logger,"Duracion de rafaga actual: %f",entrenador->datosSjf.duracionRafagaAct);

					}
					if(!entrenador->datosDeadlock.estaEnDeadlock){
						sem_wait(&mutexEntrenadores);
						entrenador->estado = BLOQUEADO;
						entrenador->suspendido = true;
						sem_post(&mutexEntrenadores);

						enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
						registrarCambioDeContexto(entrenador);
						log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: alcanzo su objetivo y envio un CATCH.", entrenador->id);
					}
					else{
					intercambiar(entrenador);
					sem_post(&resolviendoDeadlock);//Semaforo de finalizacion de deadlock.
				}

					entrenador->datosSjf.duracionRafagaAnt = entrenador->datosSjf.duracionRafagaAct;
					entrenador->datosSjf.duracionRafagaAct = 0;
					entrenador->datosSjf.estimadoRafagaAct = alfa*entrenador->datosSjf.duracionRafagaAnt+(1-alfa)*entrenador->datosSjf.estimadoRafagaAnt;
					entrenador->datosSjf.estimadoRafagaAnt = entrenador->datosSjf.estimadoRafagaAct;

					sem_post(&ejecutando);
			}

		}
	}
}

void gestionarEntrenadorSJFconDesalojo(t_entrenador* entrenador){
	while(1){
		sem_wait(&semEntrenadores[entrenador->id]);
		registrarCambioDeContexto(entrenador);

		if(entrenador->estado != FIN){

			if(entrenador->estado == EJEC){

				bool alternadorXY = true;
				while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){

					sem_wait(&mutexEntrenadores);
					if(alternadorXY){
						moverXDelEntrenador(entrenador);
					}
					else{
						moverYDelEntrenador(entrenador);
					}
					sem_post(&mutexEntrenadores);

					alternadorXY = !alternadorXY;
					//Los parciales son temporales, los amigos y el tp de operativos son eternos.

					registrarCicloDeCPU();
					usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);

					//Actualizo estimacion
					entrenador->datosSjf.estimadoRafagaAct-=1;
//					log_error(logger,"Estimado de rafaga actual: %f",entrenador->datosSjf.estimadoRafagaAct);

					entrenador->datosSjf.duracionRafagaAct+=1;
//					log_error(logger,"Duracion de rafaga actual: %f",entrenador->datosSjf.duracionRafagaAct);

					//Chequear funcionamiento del desalojo. Necesitamos tener mas de un entrenador en ready.
					if(hayNuevoEntrenadorConMenorRafaga(entrenador)){
						t_entrenador* entrenadorDesalojante = list_get(listaDeReady,0);
						log_debug(logger, "El entrenador %d fue desalojado por el entrenador %d. Vuelve a la cola de ready.", entrenador->id, entrenadorDesalojante->id);
						log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: fue desalojado por el entrenador %d.", entrenador->id, entrenadorDesalojante->id);
						entrenador->estado = LISTO; //Nico | Podría primero mandarlo a blocked y dps a ready, para respetar el modelo.
						list_add(listaDeReady,entrenador);
						entrenador->datosSjf.fueDesalojado = true;

						registrarCambioDeContexto(entrenador);
						sem_post(&procesoEnReady);
						sem_post(&ejecutando);
					}
					else{
						sem_post(&semEntrenadores[entrenador->id]);
					}
					sem_wait(&semEntrenadores[entrenador->id]);
				}
					entrenador->datosSjf.fueDesalojado = false;

			if(!entrenador->datosDeadlock.estaEnDeadlock){
				sem_wait(&mutexEntrenadores);
				entrenador->estado = BLOQUEADO;
				entrenador->suspendido = true;
				sem_post(&mutexEntrenadores);
				enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
				log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: alcanzo su objetivo y envio un CATCH.", entrenador->id);
				registrarCambioDeContexto(entrenador);
			}
			else{
				intercambiar(entrenador);
				sem_post(&resolviendoDeadlock);//Semaforo de finalizacion de deadlock.
			}

			entrenador->datosSjf.duracionRafagaAnt = entrenador->datosSjf.duracionRafagaAct;
			entrenador->datosSjf.duracionRafagaAct = 0;
			entrenador->datosSjf.estimadoRafagaAct = alfa*entrenador->datosSjf.duracionRafagaAnt+(1-alfa)*entrenador->datosSjf.estimadoRafagaAnt;
			entrenador->datosSjf.estimadoRafagaAnt = entrenador->datosSjf.estimadoRafagaAct;

			sem_post(&ejecutando);
			}

		}
	}
}

/*MANEJA EL FUNCIONAMIENTO INTERNO DE CADA ENTRENADOR(trabajo en un hilo separado)*/
void gestionarEntrenador(t_entrenador *entrenador) {
	switch(team->algoritmoPlanificacion){
		case FIFO:{
			gestionarEntrenadorFIFO(entrenador);
			break;
		}
		case RR:{
			gestionarEntrenadorRR(entrenador);
			break;
		}
		case SJFSD:{
			gestionarEntrenadorSJFsinDesalojo(entrenador);
			break;
		}
		case SJFCD:{
			gestionarEntrenadorSJFconDesalojo(entrenador);
			break;
		}
		default:{
			gestionarEntrenadorFIFO(entrenador);
			break;
		}
	}
}


