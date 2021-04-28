#include "Game-Card.h"



void procesarNEW(mensajeRecibido * mensajeRecibido) {

	log_info(logger, "Procesando mensaje NEW...");

	mensajeNew * msgNew = desarmarMensajeNEW(mensajeRecibido);

	char * posicionComoCadena = posicionComoChar(msgNew->posicionX,msgNew->posicionY); //"2-3"

	char * entradaCompletaComoCadena;
	asprintf(&entradaCompletaComoCadena, "%s%s%d%s", posicionComoCadena, "=",msgNew->cantPokemon, "\n"); //"2-3 = 5"

	char * rutaPokemon;
	asprintf(&rutaPokemon, "%s%s%s", puntoDeMontaje, "/Files/",msgNew->pokemon); //puntoDeMontaje/Files/pikachu

	char * rutaMetadataPokemon;
	asprintf(&rutaMetadataPokemon, "%s%s", rutaPokemon, "/metadata.bin"); //puntodeMontaje/Files/pikachu/metadata.bin

	int tamanioEntradaCompleta = strlen(entradaCompletaComoCadena);


	sem_wait(&mutexListaDeSemaforos);
	if(!existeSemaforo(rutaMetadataPokemon)){
		mutexPokemon * nuevoSemaforo =  crearNuevoSemaforo(rutaMetadataPokemon);
		list_add(semaforosPokemon,nuevoSemaforo);
	}
	sem_t * mutexMetadata = obtenerMutexPokemon(rutaMetadataPokemon);
	sem_post(&mutexListaDeSemaforos);

	t_config * metadataPokemon;

	bool operacionFinalizada = false;
	while(!operacionFinalizada){

		sem_wait(mutexMetadata);

		if (existeElArchivo(rutaMetadataPokemon)){
			metadataPokemon = config_create(rutaMetadataPokemon);

			log_info(logger,"Intentando abrir archivo del pokemon %s...", msgNew->pokemon);
			if (strcmp(config_get_string_value(metadataPokemon, "OPEN"), "N") == 0) {

				config_set_value(metadataPokemon, "OPEN", "Y");
				config_save(metadataPokemon);
				sem_post(mutexMetadata);

				log_info(logger,"Se accedió al archivo del pokemon %s...", msgNew->pokemon);

				char * archivoMappeado = mapearArchivo(rutaMetadataPokemon,metadataPokemon);

				if (existenLasCoordenadas(archivoMappeado, posicionComoCadena)) {

					log_info(logger,"Las coordenadas ya existen en el pokemon %s", msgNew->pokemon);

						char* aEscribirEnBloques = string_new();
						int indexEntrada = 0;
						char** arrayDeEntradas = string_split(archivoMappeado, "\n");	//["5-5=3", "5-6=16" ,"3-1=201", NULL];
						char* entradaActual = arrayDeEntradas[indexEntrada];			// "5-5=3"

						while (entradaActual != NULL) {
							char** posicionCantidad = string_split(entradaActual, "=");	//["5-5", "3", NULL]
							char* posicion = posicionCantidad[0];
							char* cantidad = posicionCantidad[1];
							if (strcmp(posicion, posicionComoCadena) == 0) { 			// "5-5" vs posicionNew
								int cantidadNum = atoi(cantidad);
								cantidadNum = cantidadNum + msgNew->cantPokemon;
								cantidad = string_itoa(cantidadNum);
								string_append(&aEscribirEnBloques, posicion);
								string_append(&aEscribirEnBloques, "=");
								string_append(&aEscribirEnBloques, cantidad);
								string_append(&aEscribirEnBloques, "\n");
								free(cantidad);
							}
							else{
							string_append(&aEscribirEnBloques, posicion);
							string_append(&aEscribirEnBloques, "=");
							string_append(&aEscribirEnBloques, cantidad);
							string_append(&aEscribirEnBloques, "\n");
							}
							indexEntrada++;
							entradaActual=arrayDeEntradas[indexEntrada];
							liberarStringSplitteado(posicionCantidad);

						}
						liberarStringSplitteado(arrayDeEntradas);

						int sizeAEscribir = strlen(aEscribirEnBloques);
						int cantidadDeBloquesAsignadosAArchivo = obtenerCantidadDeBloquesAsignados(rutaMetadataPokemon);
						if (cantidadDeBloquesAsignadosAArchivo * tamanioBloque>= sizeAEscribir) {
							log_info(logger,"No se requieren bloques adicionales para %s", msgNew->pokemon);
							log_info(logger,"Actualizando cantidad en coordenada %d-%d de %s", msgNew->posicionX,msgNew->posicionY,msgNew->pokemon);
							escribirCadenaEnArchivo(rutaMetadataPokemon,aEscribirEnBloques);

							log_info(logger,"Tamanio inicial del archivo: %d",config_get_int_value(metadataPokemon,"SIZE"));
							char * sizeFinal = string_itoa(sizeAEscribir);
							config_set_value(metadataPokemon, "SIZE", sizeFinal);
							config_set_value(metadataPokemon, "OPEN", "N");
							log_info(logger,"Tamanio final del archivo: %s", sizeFinal);

							log_info(logger,"Cerrando el archivo del pokemon %s...",msgNew->pokemon);
							sleep(tiempoDeRetardo);

							sem_wait(mutexMetadata);
							config_save(metadataPokemon);
							log_info(logger,"Se cerró el archivo del pokemon %s",msgNew->pokemon);
							sem_post(mutexMetadata);

							mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
							uint32_t sizeMensaje = msgAppeared->longPokemon + sizeof(uint32_t) * 3;
							log_info(logger, "Enviando APPEARED");
							enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje, sizeMensaje, msgAppeared);

							free(sizeFinal);
							free(msgAppeared->pokemon);
							free(msgAppeared);
						}
						else{
							sem_wait(&archivoConsumiendoBloques);
							int cantidadDisponible = cantidadDeBloquesAsignadosAArchivo * tamanioBloque + espacioLibreEnElFS();
							if(cantidadDisponible >= sizeAEscribir){

							int bloquesNecesarios = cantidadDeBloquesNecesariosParaSize(sizeAEscribir);
							sem_wait(mutexMetadata);
							asignarBloquesAArchivo(rutaMetadataPokemon, bloquesNecesarios-cantidadDeBloquesAsignadosAArchivo, metadataPokemon);
							sem_post(mutexMetadata);
							sem_post(&archivoConsumiendoBloques);
							log_info(logger,"Se asignaron %d bloques adicionales al archivo del pokemon %s", bloquesNecesarios-cantidadDeBloquesAsignadosAArchivo,msgNew->pokemon);
							log_info(logger,"Actualizando cantidad en coordenada %d-%d de %s", msgNew->posicionX,msgNew->posicionY,msgNew->pokemon);
							escribirCadenaEnArchivo(rutaMetadataPokemon, aEscribirEnBloques);

							log_info(logger,"Tamanio inicial del archivo: %d",config_get_int_value(metadataPokemon,"SIZE"));
							char * sizeFinal = string_itoa(sizeAEscribir);
							config_set_value(metadataPokemon, "SIZE", sizeFinal);
							config_set_value(metadataPokemon, "OPEN", "N");
							log_info(logger,"Tamanio final del archivo: %s", sizeFinal);

							log_info(logger,"Cerrando el archivo del pokemon %s...",msgNew->pokemon);
							sleep(tiempoDeRetardo);


							sem_wait(mutexMetadata);
							config_save(metadataPokemon);
							log_info(logger,"Se cerró el archivo del pokemon %s",msgNew->pokemon);
							sem_post(mutexMetadata);

							mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
							uint32_t sizeMensaje = msgAppeared->longPokemon + sizeof(uint32_t) * 3;
							log_info(logger, "Enviando APPEARED");
							enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje, sizeMensaje, msgAppeared);
							free(sizeFinal);
							free(msgAppeared->pokemon);
							free(msgAppeared);
							}
							else{
								sem_post(&archivoConsumiendoBloques);
								log_info(logger, "No se pudo actualizar la cantidad en la coordenada %d-%d del pokemon %s: no hay espacio suficiente",msgNew->posicionX,msgNew->posicionY,msgNew->pokemon);
								config_set_value(metadataPokemon, "OPEN", "N");
								log_info(logger,"Cerrando el archivo del pokemon %s...",msgNew->pokemon);
								sleep(tiempoDeRetardo);

								sem_wait(mutexMetadata);
								config_save(metadataPokemon);
								log_info(logger,"Se cerró el archivo del pokemon %s",msgNew->pokemon);
								sem_post(mutexMetadata);
							}
						}
					free(aEscribirEnBloques);
					} else {

						log_info(logger,"Las coordenadas no existen en el pokemon %s", msgNew->pokemon);
						char * rutaUltimoBloque = obtenerRutaUltimoBloque(rutaMetadataPokemon);
								int espacioLibreUltimoBloque = obtenerEspacioLibreDeBloque(rutaUltimoBloque);

								if (espacioLibreUltimoBloque >= tamanioEntradaCompleta) {
									log_info(logger,"No se requieren bloques adicionales para %s", msgNew->pokemon);
									log_info(logger,"Agregando la coordenada %d-%d a %s", msgNew->posicionX,msgNew->posicionY,msgNew->pokemon);
									escribirCadenaEnBloque(rutaUltimoBloque,entradaCompletaComoCadena);

									log_info(logger,"Tamanio inicial del archivo: %d",config_get_int_value(metadataPokemon,"SIZE"));
									char * sizeFinal = string_itoa(tamanioEntradaCompleta + config_get_int_value(metadataPokemon,"SIZE"));
									config_set_value(metadataPokemon, "SIZE", sizeFinal);
									config_set_value(metadataPokemon, "OPEN", "N");
									log_info(logger,"Tamanio final del archivo: %s", sizeFinal);

									log_info(logger,"Cerrando el archivo del pokemon %s...",msgNew->pokemon);
									sleep(tiempoDeRetardo);

									sem_wait(mutexMetadata);
									config_save(metadataPokemon);
									log_info(logger,"Se cerró el archivo del pokemon %s",msgNew->pokemon);
									sem_post(mutexMetadata);

									mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
									uint32_t sizeMensaje = msgAppeared->longPokemon + sizeof(uint32_t) * 3;
									log_info(logger, "Enviando APPEARED");
									enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje, sizeMensaje, msgAppeared);

									free(sizeFinal);
									free(msgAppeared->pokemon);
									free(msgAppeared);
								} else {

									sem_wait(&archivoConsumiendoBloques);
									if (espacioLibreUltimoBloque + espacioLibreEnElFS()>= tamanioEntradaCompleta) {

										int bloquesNecesarios = cantidadDeBloquesNecesariosParaSize(tamanioEntradaCompleta - espacioLibreUltimoBloque);

										sem_wait(mutexMetadata);
										asignarBloquesAArchivo(rutaMetadataPokemon,bloquesNecesarios, metadataPokemon);
										sem_post(mutexMetadata);
										sem_post(&archivoConsumiendoBloques);
										log_info(logger,"Se asignaron %d bloques adicionales al archivo del pokemon %s", bloquesNecesarios,msgNew->pokemon);
										log_info(logger,"Agregando la coordenada %d-%d a %s", msgNew->posicionX,msgNew->posicionY,msgNew->pokemon);

										char * contenidoAAlmacenar;
										asprintf(&contenidoAAlmacenar, "%s%s", archivoMappeado,entradaCompletaComoCadena);
										escribirCadenaEnArchivo(rutaMetadataPokemon,contenidoAAlmacenar);

										log_info(logger,"Tamanio inicial del archivo: %d",config_get_int_value(metadataPokemon,"SIZE"));
										char * sizeFinal = string_itoa(tamanioEntradaCompleta + config_get_int_value(metadataPokemon,"SIZE"));
										config_set_value(metadataPokemon, "SIZE", sizeFinal);
										config_set_value(metadataPokemon, "OPEN", "N");
										log_info(logger,"Tamanio final del archivo: %s", sizeFinal);

										log_info(logger,"Cerrando el archivo del pokemon %s...",msgNew->pokemon);
										sleep(tiempoDeRetardo);

										sem_wait(mutexMetadata);
										config_save(metadataPokemon);
										log_info(logger,"Se cerró el archivo del pokemon %s",msgNew->pokemon);
										sem_post(mutexMetadata);

										mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
										uint32_t sizeMensaje = msgAppeared->longPokemon+ sizeof(uint32_t) * 3;
										log_info(logger, "Enviando APPEARED");
										enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje,sizeMensaje, msgAppeared);

										free(sizeFinal);
										free(contenidoAAlmacenar);
										free(msgAppeared->pokemon);
										free(msgAppeared);

									}
									else{
										sem_post(&archivoConsumiendoBloques);
										log_info(logger,"No se pudo crear la coordenada %d-%d del pokemon %s: no hay espacio suficiente", msgNew->posicionX,msgNew->posicionY,msgNew->pokemon);
										config_set_value(metadataPokemon, "OPEN", "N");
										log_info(logger,"Cerrando el archivo del pokemon %s...",msgNew->pokemon);
										sleep(tiempoDeRetardo);

										sem_wait(mutexMetadata);
										config_save(metadataPokemon);
										log_info(logger,"Se cerró el archivo del pokemon %s",msgNew->pokemon);
										sem_post(mutexMetadata);

									}
								}
								free(rutaUltimoBloque);
							}
						free(archivoMappeado);
						operacionFinalizada=true;
				}
				else{
					sem_post(mutexMetadata);
					log_info(logger,"No se puede acceder al archivo del pokemon %s, está abierto por otro proceso. Esperando para reintentar...",msgNew->pokemon);
					sleep(tiempoDeReintentoDeAcceso);
				}
				config_destroy(metadataPokemon);
		}
		else {
			log_info(logger,"No existe el pokemon %s en el filesystem", msgNew->pokemon);
			sem_wait(&archivoConsumiendoBloques);
			if (haySuficientesBloquesLibresParaSize(tamanioEntradaCompleta)) {

				log_info(logger,"Creando el pokemon %s en el filesystem...", msgNew->pokemon);
				mkdir(rutaPokemon, 0777);

				crearNuevoPokemon(rutaMetadataPokemon, msgNew);

				int cantidadDeBloquesAAsignar = cantidadDeBloquesNecesariosParaSize(tamanioEntradaCompleta);

				metadataPokemon = config_create(rutaMetadataPokemon);

				asignarBloquesAArchivo(rutaMetadataPokemon, cantidadDeBloquesAAsignar, metadataPokemon);
				sem_post(&archivoConsumiendoBloques);

				log_info(logger,"Se asignaron %d bloques al archivo del pokemon %s", cantidadDeBloquesAAsignar,msgNew->pokemon);

				log_info(logger,"Agregando la coordenada %d-%d a %s", msgNew->posicionX,msgNew->posicionY,msgNew->pokemon);
				escribirCadenaEnArchivo(rutaMetadataPokemon,entradaCompletaComoCadena);



				char * sizeFinal = string_itoa(tamanioEntradaCompleta);
				config_set_value(metadataPokemon, "SIZE", sizeFinal);
				config_set_value(metadataPokemon, "OPEN", "N");
				log_info(logger,"Tamanio final del archivo: %s", sizeFinal);

				log_info(logger,"Cerrando el archivo del pokemon %s...",msgNew->pokemon);
				sleep(tiempoDeRetardo);

				config_save(metadataPokemon);
				log_info(logger,"Se cerró el archivo del pokemon %s",msgNew->pokemon);
				sem_post(mutexMetadata);

				config_destroy(metadataPokemon);

				mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
				uint32_t sizeMensaje = msgAppeared->longPokemon+ sizeof(uint32_t) * 3;
				log_info(logger, "Enviando APPEARED");
				enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje,sizeMensaje, msgAppeared);

				free(sizeFinal);
				free(msgAppeared->pokemon);
				free(msgAppeared);
			}
			else {
				sem_post(&archivoConsumiendoBloques);
				sem_post(mutexMetadata);
				log_info(logger,"No se puede crear el pokemon %s, no hay espacio suficiente", msgNew->pokemon);
			}
			operacionFinalizada=true;
		}
	}

	free(rutaPokemon);
	free(rutaMetadataPokemon);
	free(posicionComoCadena);
	free(entradaCompletaComoCadena);
	free(msgNew->pokemon);
	free(msgNew);
	free(mensajeRecibido->mensaje);
	free(mensajeRecibido);
}


mensajeNew * desarmarMensajeNEW(mensajeRecibido * mensajeRecibido) {
	mensajeNew * mensaje = malloc(sizeof(mensajeNew));
	int offset = 0;
	char finDeCadena='\0';
	memcpy(&mensaje->longPokemon, mensajeRecibido->mensaje + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje->pokemon = malloc(mensaje->longPokemon+1);

	memcpy(mensaje->pokemon, mensajeRecibido->mensaje + offset,mensaje->longPokemon);
	offset += mensaje->longPokemon;

	memcpy(mensaje->pokemon+mensaje->longPokemon,&finDeCadena,1);

	memcpy(&mensaje->posicionX, mensajeRecibido->mensaje + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&mensaje->posicionY, mensajeRecibido->mensaje + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&mensaje->cantPokemon, mensajeRecibido->mensaje + offset,sizeof(uint32_t));

	return mensaje;
}


void crearNuevoPokemon(char * rutaMetadataPokemon, mensajeNew * datosDelPokemon) {
	int pokemonFD = open(rutaMetadataPokemon, O_RDWR | O_CREAT, 0777);
	close(pokemonFD);
	inicializarArchivoMetadata(rutaMetadataPokemon);
}

mensajeAppeared * armarMensajeAppeared(mensajeNew * msgNew) {
	mensajeAppeared * msgAppeared = malloc(sizeof(mensajeAppeared));
	msgAppeared->longPokemon = msgNew->longPokemon;
	msgAppeared->pokemon = malloc(msgAppeared->longPokemon);
	memcpy(msgAppeared->pokemon, msgNew->pokemon, msgAppeared->longPokemon);
	msgAppeared->posicionX = msgNew->posicionX;
	msgAppeared->posicionY = msgNew->posicionY;
	return msgAppeared;
}

