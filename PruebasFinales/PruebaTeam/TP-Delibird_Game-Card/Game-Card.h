/*
 * Game-Card.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef GAME_CARD_H_
#define GAME_CARD_H_
#define _GNU_SOURCE

#include <Utils.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <commons/bitarray.h>
#include <signal.h>



//Interaccion con broker
//---------------------------------------------------------------------------------------------
typedef enum{
	NO_CONECTADO=0,
	CONECTADO=1,
	ERROR_CONEXION=-1
}estadoConexion;

char * ipServidor;
char * puertoServidor;
estadoConexion statusConexionBroker;
int socketSuscripcionNEW, socketSuscripcionGET, socketSuscripcionCATCH, socketEscuchaGameboy;
pthread_t hiloEsperaNEW, hiloEsperaGET, hiloEsperaCATCH, hiloEsperaGameboy, hiloReconexiones;
uint32_t idProceso;


//---------------------------------------------------------------------------------------------



//Gestion del FS
//---------------------------------------------------------------------------------------------
char * puntoDeMontaje;
uint32_t tamanioBloque, cantidadDeBloques, tiempoDeRetardo, tiempoDeReintentoDeAcceso, sizeBitmap;
t_bitarray * bitarrayBloques;
char * bitmap;
t_list * semaforosPokemon;

sem_t mutexListaDeSemaforos;
sem_t mutexBitmap;
sem_t archivoConsumiendoBloques;

typedef struct{
sem_t mutex;
char * ruta;
}mutexPokemon;
//---------------------------------------------------------------------------------------------



//Game-Card.c
void inicializarVariablesGlobales();
void destruirVariablesGlobales();
void inicializarFileSystem();
char * cadenasConcatenadas(char * cadena1, char * cadena2);
bool existeElArchivo(char * rutaArchivo);
int buscarBloqueLibre();
mutexPokemon * crearNuevoSemaforo(char * rutaMetadataPokemon);
sem_t * obtenerMutexPokemon (char * rutaMetadataPokemon);
char * posicionComoChar(uint32_t posx, uint32_t posy);
void inicializarArchivoMetadata(char * rutaArchivo);
char * aniadirBloqueAVectorString(int numeroBloque, char ** bloquesActuales);
void asignarBloquesAArchivo(char * rutaMetadataArchivo, int cantidadDeBloques, t_config * metadataArchivo);
int buscarBloqueLibre();
int obtenerCantidadDeBloquesAsignados(char* rutaMetadata);
int obtenerCantidadDeBloquesLibres();
bool haySuficientesBloquesLibresParaSize(int size);
int cantidadDeBloquesNecesariosParaSize(int size);
void escribirCadenaEnArchivo(char * rutaMetadataArchivo, char * cadena);
void escribirCadenaEnBloque(char * rutaBloque, char * cadena);
char * mapearArchivo(char * rutaMetadata, t_config * metadata);
char * obtenerRutaUltimoBloque(char * metadataArchivo);
int obtenerSizeOcupadoDeBloque(char * rutaBloque);
int obtenerEspacioLibreDeBloque(char * rutaBloque);
int espacioLibreEnElFS();
bool existeSemaforo(char * rutaMetadataPokemon);
mutexPokemon * crearNuevoSemaforo(char * rutaMetadataPokemon);
sem_t * obtenerMutexPokemon (char * rutaMetadataPokemon);
void obtenerParametrosDelFS(char * rutaMetadata);
//t_config* intentarAbrirMetadataPokemon(sem_t* mutexMetadata, t_config* metadataPokemon, char* rutaMetadataPokemon);
//void intentarAbrirMetadataPokemon(sem_t* mutexMetadata, t_config* metadataPokemon, char* rutaMetadataPokemon);
t_config* intentarAbrirMetadataPokemon(sem_t* mutexMetadata, char* rutaMetadataPokemon);
bool existenLasCoordenadas(char * archivoMappeado, char * coordenadas);
void desasignarBloquesAArchivo(t_config * metadataArchivo, int cantidadDeBloquesAQuitar, int cantidadDeBloquesAsignadaInicialmente);
int obtenerCantidadEnCoordenada(char * archivoMappeado, char * coordenadas);
void liberarStringSplitteado(char ** stringSplitteado);
void destroyer(void * nodo);

//Game-Card_Conexiones.c
void esperarMensajesGameboy(int* socketSuscripcion);
void esperarMensajesBroker(int* socketSuscripcion);
int crearSuscripcionesBroker();
void atenderConexiones(int *socketEscucha);
estadoConexion conectarYSuscribir();
void mantenerConexionBroker();
void cerrarConexiones();
void crearConexionGameBoy();
void crearConexionBroker();
int enviarMensajeBroker(cola colaDestino, uint32_t idCorrelativo, uint32_t sizeMensaje, void * mensaje);
void imprimirContenido(mensajeRecibido * mensaje, int * socketSuscripcion);

//Procesar
void procesarNEW(mensajeRecibido * mensaje);
void procesarCATCH(mensajeRecibido * mensaje);
void procesarGET(mensajeRecibido * mensaje);
mensajeNew * desarmarMensajeNEW(mensajeRecibido * mensajeRecibido);
mensajeGet * desarmarMensajeGET(mensajeRecibido * mensajeRecibido);
mensajeCatch * desarmarMensajeCATCH(mensajeRecibido * mensajeRecibido);
mensajeAppeared * armarMensajeAppeared(mensajeNew * msgNew);
mensajeLocalized * armarMensajeLocalized(mensajeGet * msgGet, t_list* posiciones);
mensajeCaught * armarMensajeCaught(resultado res);
void crearNuevoPokemon(char * rutaMetadataPokemon, mensajeNew * datosDelPokemon);

#endif /* GAME_CARD_H_ */
