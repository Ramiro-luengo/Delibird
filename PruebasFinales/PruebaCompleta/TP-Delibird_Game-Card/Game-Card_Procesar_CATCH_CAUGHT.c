#include "Game-Card.h"

void procesarCATCH(mensajeRecibido * mensajeRecibido) {

	log_info(logger, "Procesando mensaje CATCH...");

	mensajeCatch * msgCatch = desarmarMensajeCATCH(mensajeRecibido);

	char * posicionComoCadena = posicionComoChar(msgCatch->posicionX,msgCatch->posicionY); //"2-3"

	char * rutaPokemon;
	asprintf(&rutaPokemon, "%s%s%s", puntoDeMontaje, "/Files/",msgCatch->pokemon); //puntoDeMontaje/Files/pikachu

	char * rutaMetadataPokemon;
	asprintf(&rutaMetadataPokemon, "%s%s", rutaPokemon, "/metadata.bin"); //puntodeMontaje/Files/pikachu/metadata.bin

	int sizeMensaje = sizeof(resultado);

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
		metadataPokemon=config_create(rutaMetadataPokemon);

		if (existeElArchivo(rutaMetadataPokemon)) {

			log_info(logger,"Intentando abrir archivo del pokemon %s...", msgCatch->pokemon);
			if (strcmp(config_get_string_value(metadataPokemon, "OPEN"), "N") == 0) {

				config_set_value(metadataPokemon, "OPEN", "Y");
				config_save(metadataPokemon);
				sem_post(mutexMetadata);
				log_info(logger,"Se accedió al archivo del pokemon %s...", msgCatch->pokemon);

				char * archivoMappeado = mapearArchivo(rutaMetadataPokemon,metadataPokemon);

				if (existenLasCoordenadas(archivoMappeado, posicionComoCadena)) {

					//Armar la cadena habiendo aplicado el catch y obtener su size
					char* aEscribirEnBloques = string_new();
					int indexEntrada = 0;
					char** arrayDeEntradas = string_split(archivoMappeado, "\n");	//["5-5=3", "5-6=16" ,"3-1=201", NULL];
					char* entradaActual = arrayDeEntradas[indexEntrada];			// "5-5=3
					int cantidadNum=0;

					while (entradaActual != NULL) {
						char** posicionCantidad = string_split(entradaActual, "=");	//["5-5", "3", NULL]
						char* posicion = posicionCantidad[0];
						char* cantidad = posicionCantidad[1];

						if (strcmp(posicion, posicionComoCadena) == 0) { 			// "5-5" vs posicionCatch
							cantidadNum = atoi(cantidad);
							cantidadNum = cantidadNum - 1;
							cantidad = string_itoa(cantidadNum);
							if(cantidadNum>0){
							string_append(&aEscribirEnBloques, posicion);
							string_append(&aEscribirEnBloques, "=");
							string_append(&aEscribirEnBloques, cantidad);
							string_append(&aEscribirEnBloques, "\n");
							}
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
					//---------------------------------------------------------

					int cantidadDeBloquesAsignadosActualmente = obtenerCantidadDeBloquesAsignados(rutaMetadataPokemon);

					int cantidadDeBloquesSobrantes = cantidadDeBloquesAsignadosActualmente - cantidadDeBloquesNecesariosParaSize(sizeAEscribir);
					desasignarBloquesAArchivo(metadataPokemon,cantidadDeBloquesSobrantes,cantidadDeBloquesAsignadosActualmente);
					log_info(logger,"Se quitaron %d bloques al pokemon %s", cantidadDeBloquesSobrantes, msgCatch->pokemon);
					cantidadDeBloquesAsignadosActualmente=obtenerCantidadDeBloquesAsignados(rutaMetadataPokemon);
					log_info(logger,"El pokemon %s tiene asignados %d bloques actualmente", msgCatch->pokemon, cantidadDeBloquesAsignadosActualmente);
					if(cantidadDeBloquesAsignadosActualmente==0){
						remove(rutaMetadataPokemon);
						rmdir(rutaPokemon);
						log_info(logger,"Se eliminó el Pokemon del Filesystem dado que no tenia contenido");
					}
					else{
						log_info(logger,"Tamanio inicial del archivo: %d",config_get_int_value(metadataPokemon,"SIZE"));
						escribirCadenaEnArchivo(rutaMetadataPokemon,aEscribirEnBloques);
						char * tamanio = string_itoa(sizeAEscribir);
						config_set_value(metadataPokemon,"SIZE",tamanio);
						log_info(logger,"Tamanio final del archivo: %s", tamanio);
						free(tamanio);
					}

					config_set_value(metadataPokemon, "OPEN", "N");
					log_info(logger,"Cerrando el archivo del pokemon %s...",msgCatch->pokemon);
					sleep(tiempoDeRetardo);

					sem_wait(mutexMetadata);
					config_save(metadataPokemon);
					log_info(logger,"Se cerró el archivo del pokemon %s",msgCatch->pokemon);
					sem_post(mutexMetadata);

					mensajeCaught * msgCaught = armarMensajeCaught(OK);
					enviarMensajeBroker(CAUGHT, mensajeRecibido->idMensaje,sizeMensaje, msgCaught);
					log_info(logger, "Enviando CAUGHT (OK)");
					free(msgCaught);
					free(aEscribirEnBloques);
				}
				else{
					log_info(logger, "No existe %s en la posicion requerida", msgCatch->pokemon);
					config_set_value(metadataPokemon, "OPEN", "N");
					log_info(logger,"Cerrando el archivo del pokemon %s...",msgCatch->pokemon);
					sleep(tiempoDeRetardo);

					sem_wait(mutexMetadata);
					config_save(metadataPokemon);
					log_info(logger,"Se cerró el archivo del pokemon %s",msgCatch->pokemon);
					sem_post(mutexMetadata);

					mensajeCaught * msgCaught = armarMensajeCaught(FAIL);
					log_info(logger, "Enviando CAUGHT (FAIL)");
					enviarMensajeBroker(CAUGHT, mensajeRecibido->idMensaje,sizeMensaje, msgCaught);
					free(msgCaught);
				}
				free(archivoMappeado);
				operacionFinalizada=true;
			}
			else{
				log_info(logger,"No se puede acceder al archivo del pokemon %s, está abierto por otro proceso. Esperando para reintentar...",msgCatch->pokemon);
				sem_post(mutexMetadata);
				sleep(tiempoDeReintentoDeAcceso);
			}
			config_destroy(metadataPokemon);
		}
		else{
			sem_post(mutexMetadata);
			log_info(logger, "No existe el pokemon %s en el filesystem", msgCatch->pokemon);
			mensajeCaught * msgCaught = armarMensajeCaught(FAIL);
			log_info(logger, "Enviando CAUGHT (FAIL)");
			enviarMensajeBroker(CAUGHT, mensajeRecibido->idMensaje,sizeMensaje, msgCaught);
			free(msgCaught);
			operacionFinalizada=true;
		}
	}
	free(msgCatch->pokemon);
	free(msgCatch);
	free(posicionComoCadena);
	free(rutaPokemon);
	free(rutaMetadataPokemon);
	free(mensajeRecibido->mensaje);
	free(mensajeRecibido);

	log_info(logger, "Mensaje CAUGHT procesado correctamente");
}

mensajeCatch * desarmarMensajeCATCH(mensajeRecibido * mensajeRecibido) {
	mensajeCatch * mensaje = malloc(sizeof(mensajeCatch));
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

	return mensaje;
}

mensajeCaught * armarMensajeCaught(resultado res){
	mensajeCaught * msgCaught = malloc(sizeof(mensajeCaught));
	msgCaught->resultado = res;
	return msgCaught;
}
