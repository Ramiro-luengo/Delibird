#include "Team.h"

void enviarGetDePokemon(char *ip, char *puerto, char *pokemon) {
	int *socketBroker = malloc(sizeof(int));
	*socketBroker = crearConexionCliente(ip, puerto);

	if(*socketBroker != -1){
		uint32_t * idRespuesta = malloc(sizeof(uint32_t));

		mensajeGet *msg = malloc(sizeof(mensajeGet));

		msg->longPokemon = strlen(pokemon) + 1;
		msg->pokemon = malloc(msg->longPokemon);
		strcpy(msg->pokemon, pokemon);

		enviarMensajeABroker(*socketBroker, GET, -1, sizeof(uint32_t) + msg->longPokemon, msg);
		recv(*socketBroker,idRespuesta,sizeof(uint32_t),MSG_WAITALL);

		sem_wait(&mutexidsGet);
		list_add(idsDeGet,idRespuesta);
		sem_post(&mutexidsGet);

		free(msg->pokemon);
		free(msg);
		close(*socketBroker);
		free(socketBroker);
		conexionInicial = true;
	}
	else{
		log_info(logger,"No se pudo enviar el get");
		conexionInicial = false;
	}
}

bool validarIDCorrelativoCatch(uint32_t id){

	bool esUnIDCatch(void* elemento) {
		t_catchEnEspera* unCatch = elemento;

		if (unCatch->idCorrelativo == id) {
			return true;
		}
		return false;
	}

	sem_wait(&mutexCATCH);
	bool valido = list_any_satisfy(idsDeCatch, esUnIDCatch);
	sem_post(&mutexCATCH);
	return valido;
}


t_catchEnEspera* buscarCatch(uint32_t idCorrelativo){

	bool encontrarCatch(void* elemento){
		t_catchEnEspera* unCatch = elemento;

		return unCatch->idCorrelativo == idCorrelativo;
	}

	sem_wait(&mutexCATCH);
	t_catchEnEspera * catchEncontrado = list_find(idsDeCatch, encontrarCatch);
	sem_post(&mutexCATCH);

	return catchEncontrado;
}

void loggearPokemonCapturado(t_entrenador* entrenador, bool resultado){
	if(resultado){
	log_info(logger, "El entrenador %d capturo un %s!", entrenador->id,
					entrenador->pokemonAAtrapar.pokemon);
	log_info(loggerOficial, "El entrenador %d capturo un %s en la posicion [%d,%d].",
			entrenador->id, entrenador->pokemonAAtrapar.pokemon,
			entrenador->pos[0], entrenador->pos[1]);
	}
	else{
		log_info(logger, "El entrenador %d no pudo atrapar a %s!", entrenador->id,
							entrenador->pokemonAAtrapar.pokemon);
		log_info(loggerOficial, "El entrenador %d no logro capturar un %s en la posicion [%d,%d].",
				entrenador->id, entrenador->pokemonAAtrapar.pokemon,
				entrenador->pos[0], entrenador->pos[1]);
	}
}

void agregarInfoDeEspecie(char *especie){
	char *pokemon = malloc(strlen(especie) +1);

	strcpy(pokemon,especie);
	sem_wait(&mutexEspeciesRecibidas);
	list_add(especiesRecibidas,pokemon);
	sem_post(&mutexEspeciesRecibidas);

}

bool recibiInfoDe(char *especie){

	bool coincideCon(void *elemento){
		return string_equals_ignore_case(especie, (char*)elemento);
	}
	sem_wait(&mutexEspeciesRecibidas);
	bool recibi = list_any_satisfy(especiesRecibidas,coincideCon);
	sem_post(&mutexEspeciesRecibidas);
	return recibi;
}

void imprimirListaDePosiciones(){
	log_info(logger,"Posiciones pendientes: ");
	for(int i=0; i<list_size(listaPosicionesInternas);i++){
		t_posicionEnMapa* posic =list_get(listaPosicionesInternas,i);
		log_info(logger,"%s",posic->pokemon);
		log_info(logger,"%d-%d",posic->pos[0],posic->pos[1]);
	}
}

void procesarObjetivoCumplido(t_catchEnEspera* catchProcesado, uint32_t resultado){
	sem_wait(&mutexEntrenadores);
	catchProcesado->entrenadorConCatch->suspendido = false;
	sem_post(&mutexEntrenadores);

	if(resultado){
		t_list* pokemonesDelEntrenador = catchProcesado->entrenadorConCatch->pokemones;
		char* pokemonAtrapado = malloc(strlen(catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon) + 1);
		strcpy(pokemonAtrapado,catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon);

		sem_wait(&mutexEntrenadores);
		enlistar(pokemonAtrapado, pokemonesDelEntrenador);
		sem_post(&mutexEntrenadores);

		bool esUnObjetivo(void *objetivo){
			bool verifica = false;

			if(string_equals_ignore_case((char *)objetivo, pokemonAtrapado))
				verifica = true;

			return verifica;
		}

		sem_wait(&mutexEntrenadores);
		list_remove_by_condition(catchProcesado->entrenadorConCatch->objetivos,esUnObjetivo);
		sem_post(&mutexEntrenadores);

		sem_wait(&mutexListaObjetivosOriginales);
		list_remove_by_condition(team->objetivosOriginales,esUnObjetivo);
		sem_post(&mutexListaObjetivosOriginales);

		//Marca objetivos cumplidos de entrenador.

		loggearPokemonCapturado(catchProcesado->entrenadorConCatch, true);
	}
	else{
		loggearPokemonCapturado(catchProcesado->entrenadorConCatch, false);
		char* pokemonNoAtrapado = malloc(strlen(catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon) + 1);
		strcpy(pokemonNoAtrapado,catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon);


		bool tieneElPokemon(void *elemento){
			return string_equals_ignore_case(pokemonNoAtrapado,((t_posicionEnMapa*)elemento)->pokemon);
		}

		sem_wait(&mutexListaPosicionesBackup);
		t_posicionEnMapa *backUp = list_remove_by_condition(listaPosicionesBackUp,tieneElPokemon);
		sem_post(&mutexListaPosicionesBackup);

		if(backUp != NULL){
			sem_wait(&mutexListaPosiciones);
			list_add(listaPosicionesInternas,backUp);
			sem_post(&mutexListaPosiciones);
			log_info(logger,"Posicion de back up restaurada: [%d,%d]",backUp->pos[0],backUp->pos[1]);

			sem_post(&posicionesPendientes);
		}
		else{
			//Tener en cuenta que estamos alterando la idea de guardar las mismas referencias en objetivos originales y objetivos no atendidos.
			//En teoria, solo podria traer leaks en casos border, pero si rompe algo tenerlo en cuenta
			sem_wait(&mutexOBJETIVOS);
			list_add(team->objetivosNoAtendidos,pokemonNoAtrapado);
			sem_post(&mutexOBJETIVOS);
		}

//		sem_post(&entrenadorDisponible);
	}


//	log_info(logger,"Pokemones: ");
//	imprimirListaDeCadenas(catchProcesado->entrenadorConCatch->pokemones, logger);
//
//	log_info(logger,"Objetivos: ");
//	imprimirListaDeCadenas(catchProcesado->entrenadorConCatch->objetivos, logger);
//
//	log_info(logger,"Estado Actual: %d",catchProcesado->entrenadorConCatch->estado);
//
//	log_info(logger,"Objetivos generales:");
//	imprimirListaDeCadenas(team->objetivosOriginales, logger);
//
//	sem_wait(&mutexListaPosiciones);
//	imprimirListaDePosiciones();
//	sem_post(&mutexListaPosiciones);

	seCumplieronLosObjetivosDelEntrenador(catchProcesado->entrenadorConCatch);

	//Verifica si estan en deadlock, SOLO cuando se acabaron los objetivos generales.
	verificarDeadlock();
}

void emularCaught(t_entrenador *entrenador){
	sem_wait(&mutexEntrenadores);
	entrenador->suspendido = false;
	sem_post(&mutexEntrenadores);


	char* pokemonAtrapado = malloc(strlen(entrenador->pokemonAAtrapar.pokemon) + 1);
	strcpy(pokemonAtrapado,entrenador->pokemonAAtrapar.pokemon);

	sem_wait(&mutexEntrenadores);
	enlistar(pokemonAtrapado, entrenador->pokemones);
	sem_post(&mutexEntrenadores);

	bool esUnObjetivo(void *objetivo){
		bool verifica = false;

		if(string_equals_ignore_case((char *)objetivo, pokemonAtrapado))
			verifica = true;

		return verifica;
	}

	sem_wait(&mutexEntrenadores);
	list_remove_by_condition(entrenador->objetivos,esUnObjetivo);
	sem_post(&mutexEntrenadores);

	sem_wait(&mutexListaObjetivosOriginales);
	list_remove_by_condition(team->objetivosOriginales,esUnObjetivo);
	sem_post(&mutexListaObjetivosOriginales);

	//Marca objetivos cumplidos de entrenador.

	loggearPokemonCapturado(entrenador, true);

	seCumplieronLosObjetivosDelEntrenador(entrenador);

//	log_info(logger,"Pokemones: ");
//	imprimirListaDeCadenas(entrenador->pokemones, logger);
//
//	log_info(logger,"Objetivos: ");
//	imprimirListaDeCadenas(entrenador->objetivos, logger);
//
//	log_info(logger,"Estado Actual: %d",entrenador->estado, logger);
//	log_info(logger,"Objetivos generales:");
//	imprimirListaDeCadenas(team->objetivosOriginales, logger);
//
//	sem_wait(&mutexListaPosiciones);
//	imprimirListaDePosiciones();
//	sem_post(&mutexListaPosiciones);

	//Verifica si estan en deadlock, SOLO cuando se acabaron los objetivos generales.
	verificarDeadlock();
}



void enviarCatchDePokemon(char *ip, char *puerto, t_entrenador* entrenador){

	//if(brokerConectado){

//		if(estaEnLosObjetivos(entrenador->pokemonAAtrapar.pokemon)){
		int *socketBroker = malloc(sizeof(int));
		*socketBroker = crearConexionCliente(ip, puerto);
		uint32_t idRespuesta;

//		if(conexionInicial){
			if(*socketBroker!=-1){
				mensajeCatch *msg = malloc(sizeof(mensajeCatch));

				msg->longPokemon = strlen(entrenador->pokemonAAtrapar.pokemon) + 1;
				msg->pokemon = malloc(msg->longPokemon);
				strcpy(msg->pokemon, entrenador->pokemonAAtrapar.pokemon);
				msg->posicionX = entrenador->pokemonAAtrapar.pos[0];
				msg->posicionY = entrenador->pokemonAAtrapar.pos[1];

				log_debug(logger,"Enviando mensaje CATCH...");
				enviarMensajeABroker(*socketBroker, CATCH, -1, sizeof(uint32_t)*3 + msg->longPokemon, msg);
				recv(*socketBroker,&idRespuesta,sizeof(uint32_t),MSG_WAITALL);                              //Recibo el ID que envia automaticamente el Broker

				//Me guardo el ID del CATCH. Es necesario para procesar el CAUGHT
				t_catchEnEspera* elIdCorrelativo = malloc(sizeof(t_catchEnEspera));
				elIdCorrelativo->idCorrelativo = idRespuesta;
				elIdCorrelativo->entrenadorConCatch = entrenador;

				sem_wait(&mutexCATCH);
				list_add(idsDeCatch, elIdCorrelativo);
				sem_post(&mutexCATCH);

				free(msg->pokemon);
				free(msg);
				close(*socketBroker);
				free(socketBroker);
			}
			else{
//				log_info(logger,"No se pudo enviar el CATCH (broker desonectado)");
//				sem_wait(&mutexEntrenadores);
//				entrenador->suspendido = false;
//				sem_post(&mutexEntrenadores);
//
//				char* pokemonNoAtrapado = malloc(strlen(entrenador->pokemonAAtrapar.pokemon) + 1);
//				strcpy(pokemonNoAtrapado,entrenador->pokemonAAtrapar.pokemon);
//
//				bool tieneElPokemon(void *elemento){
//					return string_equals_ignore_case(pokemonNoAtrapado,((t_posicionEnMapa*)elemento)->pokemon);
//				}
//
//				sem_wait(&mutexOBJETIVOS);
//				list_add(team->objetivosNoAtendidos,pokemonNoAtrapado);
//				sem_post(&mutexOBJETIVOS);
//
//				sem_wait(&mutexListaPosicionesBackup);
//				t_posicionEnMapa *backUp = list_remove_by_condition(listaPosicionesBackUp,tieneElPokemon);
//				sem_post(&mutexListaPosicionesBackup);
//
//				if(backUp != NULL){
//					sem_wait(&mutexListaPosiciones);
//					list_add(listaPosicionesInternas,backUp);
//					sem_post(&mutexListaPosiciones);
//
//					log_info(logger,"Posicion de back up restaurada: [%d,%d]",backUp->pos[0],backUp->pos[1]);
//
//					sem_post(&posicionesPendientes);
//				}
//
//				sem_post(&entrenadorDisponible);
				pthread_t hiloCaughtEmulado;
				pthread_create(&hiloCaughtEmulado, NULL, (void*)emularCaught, entrenador);
				pthread_detach(hiloCaughtEmulado);
			}

//		}
//		else{//Si no tengo conexion inicial asumo que el catch es satisfactorio.
//			sem_wait(&mutexEntrenadores);
//			entrenador->suspendido = false;
//			sem_post(&mutexEntrenadores);
//
//			char* pokemonAtrapado = malloc(strlen(entrenador->pokemonAAtrapar.pokemon) + 1);
//			strcpy(pokemonAtrapado,entrenador->pokemonAAtrapar.pokemon);
//
//			sem_wait(&mutexEntrenadores);
//			enlistar(pokemonAtrapado, entrenador->pokemones);
//			sem_post(&mutexEntrenadores);
//
//			bool esUnObjetivo(void *objetivo){
//				bool verifica = false;
//
//				if(string_equals_ignore_case((char *)objetivo, pokemonAtrapado))
//					verifica = true;
//
//				return verifica;
//			}
//
//			sem_wait(&mutexEntrenadores);
//			list_remove_by_condition(entrenador->objetivos,esUnObjetivo);
//			sem_post(&mutexEntrenadores);
//
//			sem_wait(&mutexListaObjetivosOriginales);
//			list_remove_by_condition(team->objetivosOriginales,esUnObjetivo);
//			sem_post(&mutexListaObjetivosOriginales);
//
//			//Marca objetivos cumplidos de entrenador.
//
//			loggearPokemonCapturado(entrenador, true);
//
//			seCumplieronLosObjetivosDelEntrenador(entrenador);
//
//			//Verifica si estan en deadlock, SOLO cuando se acabaron los objetivos generales.
//			verificarDeadlock();
//		}
	//}
}

void procesarCAUGHT(mensajeRecibido* miMensajeRecibido) {
	sem_wait(&mutexCAUGHT);
	log_info(logger,"Procesando CAUGHT...");

	mensajeCaught* miCaught = malloc(sizeof(mensajeCaught));
	char* resultado = malloc(5); //OK o FAIL
	memcpy(&miCaught->resultado,miMensajeRecibido->mensaje,sizeof(resultado));

	strcpy(resultado,(miCaught->resultado ? "OK" : "FAIL"));

	if(validarIDCorrelativoCatch(miMensajeRecibido->idCorrelativo)) {
//		log_debug(logger, "Se recibio un CAUGHT valido");
		procesarObjetivoCumplido(buscarCatch(miMensajeRecibido->idCorrelativo),miCaught->resultado);
		log_info(logger, "Mensaje recibido: CAUGHT_POKEMON %s", resultado);
	}


	log_info(loggerOficial, "Mensaje recibido: CAUGHT_POKEMON %s", resultado);

	free(resultado);
	free(miCaught);
	free(miMensajeRecibido->mensaje);
	free(miMensajeRecibido);

	sem_post(&mutexCAUGHT);
}


void procesarLOCALIZED(mensajeRecibido* miMensajeRecibido) {
	sem_wait(&mutexAPPEARED_LOCALIZED);
	//sem_wait(&mutexLOCALIZED);
	log_info(logger,"Procesando LOCALIZED...");

	uint32_t cantPokes, longPokemon;
	int offset = 0;

	memcpy(&longPokemon, miMensajeRecibido->mensaje, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	char* pokemon = malloc(longPokemon + 1);
	memcpy(pokemon, miMensajeRecibido->mensaje + offset, longPokemon);
	offset += longPokemon;

	pokemon[longPokemon]='\0';

	log_debug(logger, "Pokemon: %s", pokemon);

	memcpy(&cantPokes, miMensajeRecibido->mensaje + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	bool esDeEstaEspecie(void *elemento){
		return string_equals_ignore_case(pokemon, (char*)elemento);
	}
	bool respondeAUnMensajeNuestro(void * id){
		return miMensajeRecibido->idCorrelativo==*(uint32_t *)id;
	}


	sem_wait(&mutexidsGet);
	if(list_any_satisfy(idsDeGet,respondeAUnMensajeNuestro))
	{
		sem_post(&mutexidsGet);
		if(!recibiInfoDe(pokemon)){
			agregarInfoDeEspecie(pokemon);

			if(estaEnLosObjetivos(pokemon) && cantPokes>0){
				log_debug(logger, "El pokemon %s es un objetivo", pokemon);

				sem_wait(&mutexOBJETIVOS);
				int cantQueNecesito = list_count_satisfying(team->objetivosNoAtendidos, esDeEstaEspecie);
				sem_post(&mutexOBJETIVOS);
				int cantAuxQueNecesito = cantQueNecesito;


				for(int i = 0; i < cantPokes; i++){
					t_posicionEnMapa* posicion = malloc(sizeof(t_posicionEnMapa));
					posicion->pokemon=malloc(strlen(pokemon)+1);
					strcpy(posicion->pokemon, pokemon);

					memcpy(&(posicion->pos[0]), miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);

					memcpy(&(posicion->pos[1]), miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);

					if(cantQueNecesito > 0){

						sem_wait(&mutexListaPosiciones);
						list_add(listaPosicionesInternas, posicion);
						sem_post(&mutexListaPosiciones);

						sem_wait(&mutexOBJETIVOS);
						list_remove_by_condition(team->objetivosNoAtendidos,esDeEstaEspecie);
						sem_post(&mutexOBJETIVOS);

//						sem_post(&posicionesPendientes);
						cantQueNecesito--;
					}
					else{
						sem_wait(&mutexListaPosicionesBackup);
						list_add(listaPosicionesBackUp, posicion);
						sem_post(&mutexListaPosicionesBackup);
					}
				}

				for(int i = 0; i < ((cantAuxQueNecesito <= cantPokes) ? cantAuxQueNecesito : cantPokes);i++){
					sem_post(&posicionesPendientes);
				}
			}
		}else{
			log_debug(logger,"El localized se ingora dado que ya recibi informacion de este pokemon");
		}
	}
	else{
		sem_post(&mutexidsGet);
		log_debug(logger,"El localized se ignoró ya que no corresponde con un GET enviado por el team");
	}

	log_info(logger, "Mensaje recibido: LOCALIZED_POKEMON %s con %d pokemones", pokemon, cantPokes);
	log_info(loggerOficial, "Mensaje recibido: LOCALIZED_POKEMON %s con %d pokemones", pokemon, cantPokes);

	free(pokemon);
	free(miMensajeRecibido->mensaje);
	free(miMensajeRecibido);
	//sem_post(&mutexLOCALIZED);
	sem_post(&mutexAPPEARED_LOCALIZED);
}

void procesarAPPEARED(mensajeRecibido* miMensajeRecibido) {
	sem_wait(&mutexAPPEARED_LOCALIZED);
	//sem_wait(&mutexAPPEARED);
	log_info(logger,"Procesando APPEARED...");

	char * pokemonRecibido;
	uint32_t longPokemon;
	int offset=0;

	memcpy(&longPokemon, miMensajeRecibido->mensaje+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	pokemonRecibido=malloc(longPokemon+1);
	memcpy(pokemonRecibido,miMensajeRecibido->mensaje+offset,longPokemon);
	pokemonRecibido[longPokemon]='\0';
	offset += longPokemon;

	t_posicionEnMapa* posicion = malloc(sizeof(t_posicionEnMapa));
	posicion->pokemon = pokemonRecibido;

	memcpy(&(posicion->pos[0]), miMensajeRecibido->mensaje+offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(posicion->pos[1]), miMensajeRecibido->mensaje+offset, sizeof(uint32_t));

	bool esDeEstaEspecie(void *elemento){
		return string_equals_ignore_case(pokemonRecibido, (char*)elemento);
	}

	if(estaEnLosObjetivosOriginales(pokemonRecibido)){
		if(!recibiInfoDe(pokemonRecibido))
			agregarInfoDeEspecie(pokemonRecibido);

		if(estaEnLosObjetivos(pokemonRecibido)){
			log_debug(logger, "El pokemon esta en nuestro objetivo");
			log_info(logger, "Pokemon: %s", pokemonRecibido);

			sem_wait(&mutexListaPosiciones);
			list_add(listaPosicionesInternas, posicion);
			sem_post(&mutexListaPosiciones);

			sem_wait(&mutexOBJETIVOS);
			list_remove_by_condition(team->objetivosNoAtendidos,esDeEstaEspecie);
			sem_post(&mutexOBJETIVOS);

			log_info(logger, "Mensaje recibido: APPEARED_POKEMON %s %d %d.", pokemonRecibido, posicion->pos[0], posicion->pos[1]);
			log_info(loggerOficial, "Mensaje recibido: APPEARED_POKEMON %s %d %d.", pokemonRecibido, posicion->pos[0], posicion->pos[1]);

			sem_post(&posicionesPendientes);
		}
		else{
			sem_wait(&mutexListaPosicionesBackup);
			list_add(listaPosicionesBackUp,posicion);
			sem_post(&mutexListaPosicionesBackup);
		}
	}
	else{
		log_error(logger,"No me interesa el pokemon %s",pokemonRecibido);
		log_info(loggerOficial, "Se recibio mensaje APPEARED_POKEMON para un %s. Ese pokemon no esta en mis objetivos.",pokemonRecibido);
	}

	free(miMensajeRecibido->mensaje);
	free(miMensajeRecibido);

	sem_post(&mutexAPPEARED_LOCALIZED);
	//sem_post(&mutexAPPEARED);
}

void enviarACK(int* socketServidor) {
	uint32_t ack=1;
	send(*socketServidor, &ack, sizeof(uint32_t), 0);
}

void levantarHiloDeRecepcionAPPEARED(mensajeRecibido* miMensajeRecibido){
	pthread_t hiloAPPEARED;

	pthread_create(&hiloAPPEARED, NULL, (void*) procesarAPPEARED, miMensajeRecibido);
	pthread_detach(hiloAPPEARED);
}

void levantarHiloDeRecepcionLOCALIZED(mensajeRecibido* miMensajeRecibido){
	pthread_t hiloLOCALIZED;

	pthread_create(&hiloLOCALIZED, NULL, (void*) procesarLOCALIZED, miMensajeRecibido);
	pthread_detach(hiloLOCALIZED);
}

void levantarHiloDeRecepcionCAUGHT(mensajeRecibido* miMensajeRecibido){
	pthread_t hiloCAUGHT;

	pthread_create(&hiloCAUGHT, NULL, (void*) procesarCAUGHT, miMensajeRecibido);
	pthread_detach(hiloCAUGHT);
}

/* Atender al Broker y Gameboy */
void atenderServidor(int *socketServidor) {
	while (1) {
		log_info(logger,"Esperando mensajes de broker...");
		mensajeRecibido *miMensajeRecibido = recibirMensajeDeBroker(*socketServidor);

		if (miMensajeRecibido->codeOP == FINALIZAR) {
			free(miMensajeRecibido);

			brokerConectado = false;
			log_error(logger, "Se perdio la conexión con el Broker.");
			close(*socketServidor);
			close(*socketBrokerApp);
			close(*socketBrokerLoc);
			close(*socketBrokerCau);
			log_debug(logger, "Reintentando conexion...");

			log_info(loggerOficial, "Se perdio la conexion con el Broker");
			while(1){
				*socketServidor = crearConexionCliente(ipServidor, puertoServidor);
				if(*socketServidor == -1){
					log_info(loggerOficial, "No se pudo reestablecer la conexion con el Broker. Reintentando...");
					log_info(logger, "No se pudo reestablecer la conexion con el Broker. Reintentando...");
					usleep(tiempoDeEspera * 1000000);
				}
				else{
					log_info(logger,"Reconexion con el Broker realizada correctamente.");
					log_info(loggerOficial,"Reconexion con el Broker realizada correctamente.");
					break;
				}
			}
			sem_post(&reconexion);
			break;

		}
		else{
			log_info(logger,"Recibido un mensaje de broker");
			enviarACK(socketServidor);
			if(miMensajeRecibido->codeOP > 0 && miMensajeRecibido->codeOP <= 6) {
				switch(miMensajeRecibido->colaEmisora){
					case APPEARED:{
						levantarHiloDeRecepcionAPPEARED(miMensajeRecibido);
						break;
					}
					case LOCALIZED:{
						levantarHiloDeRecepcionLOCALIZED(miMensajeRecibido);
						break;
					}
					case CAUGHT:{
						levantarHiloDeRecepcionCAUGHT(miMensajeRecibido);
						break;
					}
					default:{
						log_error(logger, "Cola de Mensaje erronea.");
						log_info(loggerOficial, "Se recibio un mensaje invalido.");
						break;
					}
				}
			}
		}
	}

}

void crearHiloParaAtenderServidor(int *socketServidor) {
	pthread_t hiloAtenderServidor;
	pthread_create(&hiloAtenderServidor, NULL, (void*) atenderServidor,
				socketServidor);
	pthread_detach(hiloAtenderServidor);
}

void reconectar(){
	sem_wait(&reconexion);
	sem_wait(&reconexion);
	sem_wait(&reconexion);
	log_info(logger,"Reconectando...");
	usleep(1 * 1000000);
	if(!list_is_empty(team->objetivosOriginales)){
		crearConexionesYSuscribirseALasColas();
	}
	else{
		log_info(logger,"Los objetivos globales ya están cumplido: se omite la reconexión");
	}

}

void crearHiloDeReconexion(){
	pthread_t hiloDeReconexion;

	pthread_create(&hiloDeReconexion, NULL, (void*) reconectar, NULL);
	pthread_detach(hiloDeReconexion);
}
void crearHilosParaAtenderBroker() {
	crearHiloParaAtenderServidor(socketBrokerApp);
	crearHiloParaAtenderServidor(socketBrokerLoc);
	crearHiloParaAtenderServidor(socketBrokerCau);

	crearHiloDeReconexion();
}

void crearConexion(int* socket){
	*socket = crearConexionClienteConReintento(ipServidor, puertoServidor, tiempoDeEspera);
}

//Obtengo el ID del proceso

void obtenerID(){
	idDelProceso = obtenerIdDelProcesoConReintento(ipServidor, puertoServidor, tiempoDeEspera);
}

/* Se suscribe a las colas del Broker */
void crearConexionesYSuscribirseALasColas() {
	//pthread_t hiloObtenerID;
	pthread_t hiloSocketLoc;
	pthread_t hiloSocketApp;
	pthread_t hiloSocketCau;

	//pthread_create(&hiloObtenerID, NULL, (void*) obtenerID, &idDelProceso); //No funciona pq le paso la direccion y la funcion recibe un entero. TODO - Revisar
	if(!yaTengoID){
		obtenerID();
		yaTengoID=true;
		pthread_create(&hiloSocketLoc, NULL, (void*) crearConexion, socketBrokerLoc);
		pthread_create(&hiloSocketApp, NULL, (void*) crearConexion, socketBrokerApp);
		pthread_create(&hiloSocketCau, NULL, (void*) crearConexion, socketBrokerCau);
		pthread_join(hiloSocketLoc, NULL);
		pthread_join(hiloSocketApp, NULL);
		pthread_join(hiloSocketCau, NULL);
	}

	//Espero a que el socket este conectado antes de utilizarlo

	suscribirseACola(*socketBrokerLoc,LOCALIZED, idDelProceso);
	crearHiloParaAtenderServidor(socketBrokerLoc);


	suscribirseACola(*socketBrokerApp,APPEARED, idDelProceso);
	crearHiloParaAtenderServidor(socketBrokerApp);


	suscribirseACola(*socketBrokerCau,CAUGHT, idDelProceso);
	crearHiloParaAtenderServidor(socketBrokerCau);

	brokerConectado = true;
//	sem_post(&conexionCreada);

	crearHiloDeReconexion();

}

void crearConexionEscuchaGameboy(int* socketGameboy) {

	char * ipEscucha;
	ipEscucha = config_get_string_value(config, "IP_TEAM");

	char * puertoEscucha;
	puertoEscucha = config_get_string_value(config, "PUERTO_TEAM");

	*socketGameboy = crearConexionServer(ipEscucha, puertoEscucha);

	
	log_info(logger, "Socket de escucha para Gameboy creado");

	atenderGameboy(socketGameboy);
}

void conectarGameboy(){
	pthread_t hiloConectarGameboy;

	pthread_create(&hiloConectarGameboy, NULL,(void*) crearConexionEscuchaGameboy, socketGameboy);
}

void atenderGameboy(int *socketEscucha) {
	int backlogGameboy = config_get_int_value(config, "BACKLOG_GAMEBOY");
	atenderConexionEn(*socketEscucha, backlogGameboy);

	while (1) {
		int *socketCliente = esperarCliente(*socketEscucha);
		esperarMensajesGameboy(socketCliente);
		free(socketCliente);
		}
}

void esperarMensajesGameboy(int* socketSuscripcion) {

	log_info(logger,"[GAMEBOY] Recibiendo mensaje del socket cliente: %d",*socketSuscripcion);
	mensajeRecibido * mensaje = recibirMensajeDeBroker(*socketSuscripcion);
	log_info(logger,"[GAMEBOY] Mensaje recibido");

	//imprimirContenido(mensaje, socketSuscripcion);

	switch (mensaje->colaEmisora) {
	case APPEARED: {
		//Procesar mensaje NEW
		log_debug(logger, "[GAMEBOY] Llegó un mensaje de la cola APPEARED");
		procesarAPPEARED(mensaje);
		break;
	}
	case LOCALIZED: {
		//Procesar mensaje GET
		log_debug(logger, "[GAMEBOY] Llegó un mensaje de la cola LOCALIZED");
		procesarLOCALIZED(mensaje);

		break;
	}
	case CAUGHT: {
		//Procesar mensaje CATCH
		log_debug(logger, "[GAMEBOY] Llegó un mensaje de la cola CAUGHT");
		procesarCAUGHT(mensaje);
		break;
	}
	default: {

		log_error(logger, "[GAMEBOY] Mensaje recibido de una cola no correspondiente");
		break;
	}

	}
}

void enviarGetSegunObjetivo(char *ip, char *puerto) {
//	sem_wait(&conexionCreada);
	char *pokemon;
	t_list *getsEnviados = list_create();

	bool fueEnviado(void *elemento){
		return string_equals_ignore_case(pokemon, (char*)elemento);
	}

	log_info(logger, "Enviando GETs...");
	//sem_wait(&mutexOBJETIVOS); //-> Puede traer problemas si llega un appeared en el medio de esto
	for (int i = 0; i < list_size(team->objetivosOriginales); i++) {
		sem_wait(&mutexListaObjetivosOriginales);
		pokemon = list_get(team->objetivosOriginales, i);
		sem_post(&mutexListaObjetivosOriginales);

		if(!list_any_satisfy(getsEnviados, fueEnviado)){
			enviarGetDePokemon(ip, puerto, pokemon);
			list_add(getsEnviados,pokemon);
		}
	}
	//sem_post(&mutexOBJETIVOS);
	sem_post(&semGetsEnviados);
	log_info(logger, "GETs enviados");
	list_destroy(getsEnviados);
}
