# tp-2020-1c-Ripped-Dinos

![](Diagrama%20mensajes.png)


**Commit 01/06**

Avancé con buddy system, implementé varias funciones.

Agregué la variable global para el tamaño mínimo de partición. 


**Commit 31/05**

Cambie la firma de las siguientes funciones porque necesitan saber el ID del mensaje. Quedaron así:
* void * usarBestFit(estructuraMensaje mensaje);
* void * usarFirstFit(estructuraMensaje mensaje);
* void * cachearConBuddySystem(estructuraMensaje mensaje);
* void * cachearConParticionesDinamicas(estructuraMensaje mensaje);

Implemente:
* bool hayEspacioLibrePara(int sizeMensaje) 
* void vaciarParticion()
* compactarCache()
* usarFirstFit(estructuraMensaje mensaje)
* Varias otras funciones auxiliares. 

Creé los enums para los algoritmos sacados de config.


**Commit 24/05**

Bug fixes:

- En la funcion suscribirseACola de la utils faltaba tomar en cuenta el sizeof del buffer->size, no estaba mandando bien el ID de proceso.
- Al final de atender suscripcion habia un free(nuevoSuscriptor). Como lo que se añade a la lista de suscriptores es ese puntero, basicamente estabamos destruyendo el nodo despues de agregarlo.
- Solucionado bug en la busqueda de espacio libre cuando se quería agregar un nuevo mensaje a la cache (nunca encontraba).
- Saqué el cachearMensaje de adentro del bucle creador de nodos, porque si no había ningún suscriptor en la cola, no se cacheaba. Además debería cachearse una sola vez, y no tantas como suscriptores haya. 


Otros:

- En esperarMensajes cambié el tipo de codOperacion de int a opCode. 
- Agregué un free(socketCliente) en el while de atenderConexiones para que libere la memoria y la variable, por las dudas.
- Agregué parametros que faltaban al archivo config del broker (no se por qué).


Post cambios: 

- Probé hacer dos suscripciones a la misma cola desde el mismo ID de suscriptor, y actualiza el socket correctamente.
- Probé enviar un mensaje a una cola. El enviador entra a la cola que tiene al menos un mensaje, envia los mensajes (falta probar si alguien los recibe bien)  y vacia la cola.
- Probé que al recibir un mensaje, se agregue a la cache con PD y FF (parece andar, hay que ver que onda con el envío).
