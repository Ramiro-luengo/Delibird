#include "Game-Card.h"

void inicializarVariablesGlobales() {

	config = config_create("gamecard.config");

	int imprimirPorConsolaLogOficial = config_get_int_value(config,"PRINT_OFICIAL");
	logger = log_create("gamecard_logs_oficial", "GameCard", imprimirPorConsolaLogOficial, LOG_LEVEL_TRACE);

	ipServidor = malloc(strlen(config_get_string_value(config, "IP_BROKER")) + 1);
	strcpy(ipServidor, config_get_string_value(config, "IP_BROKER"));
	puertoServidor = malloc(strlen(config_get_string_value(config, "PUERTO_BROKER")) + 1);
	strcpy(puertoServidor, config_get_string_value(config, "PUERTO_BROKER"));

	puntoDeMontaje = malloc(strlen(config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS"))+ 1);
	strcpy(puntoDeMontaje,config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS"));

	tiempoDeRetardo = config_get_int_value(config, "TIEMPO_RETARDO_OPERACION");
	tiempoDeReintentoDeAcceso = config_get_int_value(config,"TIEMPO_DE_REINTENTO_OPERACION");

	semaforosPokemon = list_create();


	sem_init(&mutexListaDeSemaforos,0,1);
	sem_init(&mutexBitmap, 0, 1);
	sem_init(&archivoConsumiendoBloques, 0, 1);

	idProceso = -1;
	statusConexionBroker = 0;
}


char * posicionComoChar(uint32_t posx, uint32_t posy) {
	char * cad;
	asprintf(&cad, "%d-%d", posx, posy);
	return cad;
}

void inicializarArchivoMetadata(char * rutaArchivo) {
	t_config * metadata;
	metadata = config_create(rutaArchivo);
	config_set_value(metadata, "OPEN", "Y");
	config_set_value(metadata, "BLOCKS", "[]");
	config_set_value(metadata, "SIZE", "0");
	config_set_value(metadata, "DIRECTORY", "N");
	config_save(metadata);
	config_destroy(metadata);
}

char * aniadirBloqueAVectorString(int numeroBloque, char ** bloquesActuales) {

	char * cadenaAGuardar = string_new();
	int i = 0;
	string_append(&cadenaAGuardar, "[");
	while (bloquesActuales[i] != NULL) {
		string_append(&cadenaAGuardar, bloquesActuales[i]);
		string_append(&cadenaAGuardar, ",");
		i++;
	}
	char * numeroDeBloque = string_itoa(numeroBloque);
	string_append(&cadenaAGuardar, numeroDeBloque);
	string_append(&cadenaAGuardar, "]");

	free(numeroDeBloque);

	return cadenaAGuardar;
}

void asignarBloquesAArchivo(char * rutaMetadataArchivo, int cantidadDeBloques, t_config * metadataArchivo) {

	sem_wait(&mutexBitmap);
	for (int i = 0; i < cantidadDeBloques; i++) {

		int indexBloqueLibre = buscarBloqueLibre();
		char ** bloquesActuales = config_get_array_value(metadataArchivo,"BLOCKS");
		char * cadenaAGuardar = aniadirBloqueAVectorString(indexBloqueLibre,bloquesActuales);
		bitarray_set_bit(bitarrayBloques, indexBloqueLibre);
		msync(bitmap,sizeBitmap,MS_SYNC);
		config_set_value(metadataArchivo, "BLOCKS", cadenaAGuardar);

		free(cadenaAGuardar);
		liberarStringSplitteado(bloquesActuales);
	}
	sem_post(&mutexBitmap);
	config_save(metadataArchivo);
}

void desasignarBloquesAArchivo(t_config * metadataArchivo, int cantidadDeBloquesAQuitar, int cantidadDeBloquesAsignadaInicialmente){

	int index = cantidadDeBloquesAsignadaInicialmente-1;
	int numeroDeBloqueActual;
	int cantidadDeBloquesFinal = cantidadDeBloquesAsignadaInicialmente - cantidadDeBloquesAQuitar;
	char ** bloquesActuales = config_get_array_value(metadataArchivo,"BLOCKS");	  //["1","2","3","7","9","8"]

	sem_wait(&mutexBitmap);
	for (int i = 0; i < cantidadDeBloquesAQuitar; i++) {

		numeroDeBloqueActual=atoi(bloquesActuales[index]);
		bitarray_clean_bit(bitarrayBloques, numeroDeBloqueActual);
		msync(bitmap,sizeBitmap,MS_SYNC);
		index--;

	}
	sem_post(&mutexBitmap);

	char * bloquesAGuardar = string_new();
	string_append(&bloquesAGuardar,"[");

	for(int i = 0; i < cantidadDeBloquesFinal;i++){
		string_append(&bloquesAGuardar,bloquesActuales[i]);
		if(i!=cantidadDeBloquesFinal-1)
		   string_append(&bloquesAGuardar,",");
	}
	string_append(&bloquesAGuardar,"]");

	config_set_value(metadataArchivo,"BLOCKS",bloquesAGuardar);
	config_save(metadataArchivo);

	liberarStringSplitteado(bloquesActuales);
	free(bloquesAGuardar);
}

int buscarBloqueLibre() {
	for (int i = 0; i < cantidadDeBloques; i++) {
		int estadoBloqueActual = bitarray_test_bit(bitarrayBloques, i);
		if (estadoBloqueActual == 0) {
			return i;
		}
	}
	return -1;
}

int obtenerCantidadDeBloquesAsignados(char* rutaMetadata) {
	t_config * metadata;
	metadata = config_create(rutaMetadata);
	char ** bloquesArchivo = config_get_array_value(metadata, "BLOCKS");
	int index = 0;
	while (bloquesArchivo[index] != NULL) {
		index++;
	}
	config_destroy(metadata);
	liberarStringSplitteado(bloquesArchivo);
	return index;
}

int obtenerCantidadDeBloquesLibres() {
	int cantidadDeBloquesLibres = 0;
	sem_wait(&mutexBitmap);
	for (int i = 0; i < cantidadDeBloques; i++) {
		if (!bitarray_test_bit(bitarrayBloques, i)) {
			cantidadDeBloquesLibres++;
		}
	}
	sem_post(&mutexBitmap);
	return cantidadDeBloquesLibres;
}

bool haySuficientesBloquesLibresParaSize(int size) {
	int cantidadDeBloquesLibres = 0;
	cantidadDeBloquesLibres = obtenerCantidadDeBloquesLibres(
			cantidadDeBloquesLibres);
	return cantidadDeBloquesLibres * tamanioBloque >= size;
}

int cantidadDeBloquesNecesariosParaSize(int size) {
	int cantidad = 0;
	while (cantidad * tamanioBloque < size) {
		cantidad++;
	}
	return cantidad;
}

void escribirCadenaEnArchivo(char * rutaMetadataArchivo, char * cadena) {

	t_config * metadata;
	metadata = config_create(rutaMetadataArchivo);
	char ** bloquesArchivo = config_get_array_value(metadata, "BLOCKS");
	char * rutaBloqueActual;
	int numeroDeBloqueUsado = 0;//No es el numero de bloque, sino la posicion en el metadata. So fucking hard to explain
	asprintf(&rutaBloqueActual, "%s%s%s%s", puntoDeMontaje, "/Blocks/",
			bloquesArchivo[numeroDeBloqueUsado], ".bin");
	int caracteresEscritosEnBloqueActual = 0;
	int caracteresEscritosTotales = 0;
	char caracterActual = cadena[caracteresEscritosTotales];

	FILE * bloqueActual = fopen(rutaBloqueActual, "w");

	while (caracterActual != '\0') {
		if (caracteresEscritosEnBloqueActual == tamanioBloque) {
			fclose(bloqueActual);
			free(rutaBloqueActual);
			numeroDeBloqueUsado++;
			asprintf(&rutaBloqueActual, "%s%s%s%s", puntoDeMontaje, "/Blocks/",
					bloquesArchivo[numeroDeBloqueUsado], ".bin");
			bloqueActual = fopen(rutaBloqueActual, "w");
			caracteresEscritosEnBloqueActual = 0;
		}
		fputc(caracterActual, bloqueActual);
		caracteresEscritosTotales++;
		caracteresEscritosEnBloqueActual++;
		caracterActual = cadena[caracteresEscritosTotales];
	}

	fclose(bloqueActual);
	config_destroy(metadata);
	free(rutaBloqueActual);
	liberarStringSplitteado(bloquesArchivo);
}

void escribirCadenaEnBloque(char * rutaBloque, char * cadena) {
	FILE * bloque = fopen(rutaBloque, "a");
	fputs(cadena, bloque);
	fclose(bloque);
}

char * mapearArchivo(char * rutaMetadata, t_config * metadata) {
	FILE * bloqueActual;
	int sizeArchivo, numeroBloqueActual = 0, caracteresLeidosEnBloqueActual = 0;
	char ** bloquesArchivo;
	char * archivoMapeado;
	char * rutaBloqueActual;
	int caracterLeido;

	sizeArchivo = config_get_int_value(metadata, "SIZE");
	bloquesArchivo = config_get_array_value(metadata, "BLOCKS");
	archivoMapeado = malloc(sizeArchivo+1);
	asprintf(&rutaBloqueActual, "%s%s%s%s", puntoDeMontaje, "/Blocks/",bloquesArchivo[numeroBloqueActual], ".bin");
	bloqueActual = fopen(rutaBloqueActual, "r");

	for (int i = 0; i < sizeArchivo; i++) {
		if (caracteresLeidosEnBloqueActual == tamanioBloque) {
			fclose(bloqueActual);
			free(rutaBloqueActual);
			numeroBloqueActual++;
			asprintf(&rutaBloqueActual, "%s%s%s%s", puntoDeMontaje, "/Blocks/",
					bloquesArchivo[numeroBloqueActual], ".bin");
			bloqueActual = fopen(rutaBloqueActual, "r");
			caracteresLeidosEnBloqueActual = 0;
		}
		caracterLeido = fgetc(bloqueActual);
		archivoMapeado[i]=caracterLeido;
		caracteresLeidosEnBloqueActual++;
	}
	archivoMapeado[sizeArchivo]='\0';

	fclose(bloqueActual);
	free(rutaBloqueActual);
	liberarStringSplitteado(bloquesArchivo);

	return archivoMapeado;
}

char * obtenerRutaUltimoBloque(char * metadataArchivo) {
	t_config * metadata = config_create(metadataArchivo);
	char ** bloques = config_get_array_value(metadata, "BLOCKS");
	char * rutaUltimoBloque;
	int index = 0;

	while (bloques[index] != NULL) {
		index++;
	}
	if (index == 0)
		return NULL;

	asprintf(&rutaUltimoBloque, "%s%s%s%s", puntoDeMontaje, "/Blocks/",
			bloques[index - 1], ".bin");

	config_destroy(metadata);
	liberarStringSplitteado(bloques);

	return rutaUltimoBloque;
}

int obtenerSizeOcupadoDeBloque(char * rutaBloque) {
	struct stat stat_file;
	stat(rutaBloque, &stat_file);
	return stat_file.st_size;
}

int obtenerEspacioLibreDeBloque(char * rutaBloque) {
	return tamanioBloque - obtenerSizeOcupadoDeBloque(rutaBloque);
}

int espacioLibreEnElFS() {
	return obtenerCantidadDeBloquesLibres() * tamanioBloque;
}

bool existeSemaforo(char * rutaMetadataPokemon){
	bool coincideRuta(void * elementoLista){
		mutexPokemon * elem = (mutexPokemon *) elementoLista;
		return strcmp(elem->ruta,rutaMetadataPokemon)==0;
	}
	return list_find(semaforosPokemon, (void *) coincideRuta)!=NULL;
}

mutexPokemon * crearNuevoSemaforo(char * rutaMetadataPokemon){
	mutexPokemon * sem = malloc(sizeof(mutexPokemon));
	sem_init(&(sem->mutex), 0, 1);
	sem->ruta=malloc(strlen(rutaMetadataPokemon)+1);
	strcpy(sem->ruta,rutaMetadataPokemon);
	return sem;
}

sem_t * obtenerMutexPokemon (char * rutaMetadataPokemon){
	bool coincideRuta(void * elementoLista){
		mutexPokemon * elem = (mutexPokemon *) elementoLista;
		return strcmp(elem->ruta,rutaMetadataPokemon)==0;
	}
	mutexPokemon * mutexPokemon = list_find(semaforosPokemon,(void *) coincideRuta);
	return &(mutexPokemon->mutex);
}

bool existeElArchivo(char * rutaArchivo) {
	int fd = open(rutaArchivo, O_RDONLY);

	if (fd < 0) {
		return false;
	}

	close(fd);
	return true;
}

void obtenerParametrosDelFS(char * rutaMetadata) {
	t_config * metadata;
	metadata = config_create(rutaMetadata);
	tamanioBloque = config_get_int_value(metadata, "BLOCK_SIZE");
	cantidadDeBloques = config_get_int_value(metadata, "BLOCKS");
	config_destroy(metadata);
}


char * cadenasConcatenadas(char * cadena1, char * cadena2) {
	char * cadenaFinal = string_new();
	string_append(&cadenaFinal, cadena1);
	string_append(&cadenaFinal, cadena2);
	return cadenaFinal;
}

t_config* intentarAbrirMetadataPokemon(sem_t* mutexMetadata, char* rutaMetadataPokemon) {
	t_config* metadataPokemon;
	while (1) {
		sem_wait(mutexMetadata);
		metadataPokemon = config_create(rutaMetadataPokemon);
		if (strcmp(config_get_string_value(metadataPokemon, "OPEN"), "N") == 0) {
			config_set_value(metadataPokemon, "OPEN", "Y");
			config_save(metadataPokemon);
			sem_post(mutexMetadata);
			break;
		}
		config_destroy(metadataPokemon);
		sem_post(mutexMetadata);
		sleep(tiempoDeReintentoDeAcceso);
	}
	return metadataPokemon;
}

void liberarStringSplitteado(char ** stringSplitteado){
	int index=0;
	while(stringSplitteado[index]!=NULL){  //["asd","qwe","qweqw",NULL]
		free(stringSplitteado[index]);
		index++;
	}
	free(stringSplitteado);
}
int obtenerCantidadEnCoordenada(char * archivoMappeado, char * coordenadas){

	char ** entradas = string_split(archivoMappeado,"\n");   //["2-2=5", "3-4=6", NULL]
	int index=0, cantidad=0;

	while(entradas[index]!=NULL){

		char ** coordenadasConCantidad = string_split(entradas[index],"=");   //["2-2", "5", NULL]
		if(strcmp(coordenadasConCantidad[0],coordenadas)==0){
			cantidad=atoi(coordenadasConCantidad[0]);
			liberarStringSplitteado(coordenadasConCantidad);
			break;
		}
		index++;

		liberarStringSplitteado(coordenadasConCantidad);
	}

	liberarStringSplitteado(entradas);

	return cantidad;
}

bool existenLasCoordenadas(char * archivoMappeado, char * coordenadas){

	char ** entradas = string_split(archivoMappeado,"\n");   //["2-2=5", "3-4=6", NULL]
	int index=0;
	bool existen = false;

	while(entradas[index]!=NULL){

		char ** coordenadasConCantidad = string_split(entradas[index],"=");   //["2-2", "5", NULL]
		if(strcmp(coordenadasConCantidad[0],coordenadas)==0){
			existen=true;
			liberarStringSplitteado(coordenadasConCantidad);
			break;
		}
		index++;

		liberarStringSplitteado(coordenadasConCantidad);
	}

	liberarStringSplitteado(entradas);

	return existen;

}
void inicializarFileSystem() {

	char * rutaMetadata = cadenasConcatenadas(puntoDeMontaje,"/Metadata/metadata.bin");
	char * rutaBitmap = cadenasConcatenadas(puntoDeMontaje,"/Metadata/bitmap.bin");
	char * rutaBlocks = cadenasConcatenadas(puntoDeMontaje, "/Blocks/");

	char * rutaDirFiles = cadenasConcatenadas(puntoDeMontaje,"/Files");
	char * rutaDirBlocks = cadenasConcatenadas(puntoDeMontaje,"/Blocks");
	char * rutaMetadataFiles = cadenasConcatenadas(rutaDirFiles,"/metadata.bin");

	log_info(logger,"Inicializando filesystem...");

	if (!existeElArchivo(rutaMetadata)) {
		log_error(logger,"No se encontró el archivo metadata en el punto de montaje. El proceso GameCard no puede continuar");
		exit(0);
	}

	obtenerParametrosDelFS(rutaMetadata);

	if (existeElArchivo(rutaBitmap)){
		log_info(logger,"El filesystem ya estaba inicializado");
		int fd = open(rutaBitmap, O_RDWR);
		struct stat sb;
		fstat(fd, &sb);
		sizeBitmap = sb.st_size;
		bitmap = mmap(NULL, sizeBitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		bitarrayBloques = bitarray_create_with_mode(bitmap, sb.st_size,MSB_FIRST);
		log_info(logger,"Leida configuración preexistente");

	} else {

		mkdir(rutaDirFiles, 0777);
		mkdir(rutaDirBlocks, 0777);
		int fileDesc = open(rutaMetadataFiles, O_RDWR | O_CREAT, 0777);
		close(fileDesc);
		t_config * metadataFiles = config_create(rutaMetadataFiles);
		config_set_value(metadataFiles,"DIRECTORY","Y");
		config_save(metadataFiles);
		config_destroy(metadataFiles);


		int fd = open(rutaBitmap, O_RDWR | O_CREAT, 0777);
		ftruncate(fd, cantidadDeBloques / 8);
		struct stat sb;
		fstat(fd, &sb);
		sizeBitmap = sb.st_size;
		bitmap = mmap(NULL, sizeBitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
				0);
		bitarrayBloques = bitarray_create_with_mode(bitmap, sb.st_size,
				MSB_FIRST);
		for (int i = 0; i < cantidadDeBloques; i++) {
			bitarray_clean_bit(bitarrayBloques, i);

			char * rutaBloque;
			asprintf(&rutaBloque, "%s%d%s", rutaBlocks, i, ".bin");
			int nuevoBloque = open(rutaBloque, O_RDWR | O_CREAT, 0777);
			close(nuevoBloque);
			free(rutaBloque);
		}
	    msync(bitmap,sb.st_size,MS_SYNC);
	    log_info(logger,"Filesystem inicializado");
	}

	free(rutaBitmap);
	free(rutaMetadata);
	free(rutaBlocks);

	free(rutaDirFiles);
	free(rutaDirBlocks);
	free(rutaMetadataFiles);
}

void destructorNodos(void * nodo){
	mutexPokemon * elem = (mutexPokemon *) nodo;
	free(elem->ruta);
	free(elem);
}
void destruirVariablesGlobales() {
	list_destroy_and_destroy_elements(semaforosPokemon,(void *)destructorNodos);
	//munmap(bitmap,sizeBitmap);
	//bitarray_destroy(bitarrayBloques);
	free(puntoDeMontaje);
	free(ipServidor);
	free(puertoServidor);
	config_destroy(config);
}
void finalizar(){
	cerrarConexiones();
	destruirVariablesGlobales();
	log_info(logger, "El proceso gamecard finalizó su ejecución\n");
	log_destroy(logger);
	exit(0);
}

int main() {
	//Se setean todos los datos
	inicializarVariablesGlobales();
	inicializarFileSystem();
	signal(SIGINT,finalizar);

	log_info(logger, "Se ha iniciado el cliente GameCard\n");

	crearConexionBroker();
	crearConexionGameBoy();

	return 0;
}
