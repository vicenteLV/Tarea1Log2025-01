#include "disco.h"
#include <string.h>
#include <assert.h>

long long accesos = 0; //contador de accesos

//vuelve el contador a 0
void contadorACero(void){
    accesos = 0;
}

//entrega accesos registrados
long long obtenerAccesos(void){
    return accesos;
}

/*
lee bloque tamaño B del archivo binario
archivo: estructura del archivo
bloqueIdx: indice o numero del bloque a leer
return: 0 si exito -1 si error
*/
int leerBloque(ArchivoBin *archivo, size_t bloqIdx){
    assert(archivo != NULL);
    assert(archivo->file != NULL);
    assert(archivo->buffer !=NULL);

    //si el bloque ya esta cargado no se hace nada
    if (archivo->bloqActual == bloqIdx){
        return 0;
    }

    //si esta sucio agregar modificaciones
    if (archivo->sucio){
        if(escribirBloque(archivo) != 0){
            return -1;
        }
    }

    //para posicionarse en bloque
    long offset = bloqIdx * BLOCK_SIZE; 

    //posicionarse en archivo
    if (fseek(archivo->file, offset, SEEK_SET) != 0){
        return -1;
    }
    
    //leer bloques tamaño B
    size_t elementosLeidos = fread(archivo->buffer, ELEMENT_SIZE, ELEMENTS_PER_BLOCK, archivo->file);
    accesos++; //incrementar contador de accesos a disco

    //luego de leer el bloque se rellena con ceros
    if (elementosLeidos < ELEMENTS_PER_BLOCK){
        memset(archivo->buffer + elementosLeidos, 0, (ELEMENTS_PER_BLOCK - elementosLeidos) * ELEMENT_SIZE);

    }

    archivo->bloqActual = bloqIdx;
    archivo->sucio = 0; //buffer se limpia luego de leer

    return 0;
}

/*
escribe un bloque tamaño B en el archivo binario
archivo: estructura del archivo
return: 0 si exito, -1 si error 
*/
int escribirBloque(ArchivoBin *archivo){
    assert(archivo != NULL);
    assert(archivo->file != NULL);
    assert(archivo->buffer != NULL);

    if (!archivo->sucio || archivo->bloqActual == SIZE_MAX) {
        return 0;  //solo escribir si el buffer esta sucio
    }

    //offset para posicionarse en el bloque
    long offset = archivo->bloqActual * BLOCK_SIZE;
    
    if (fseek(archivo->file, offset, SEEK_SET) != 0) {
        return -1;
    }
    
    //escribir bloque de tamaño B
    size_t elementosEscritos = fwrite(archivo->buffer, ELEMENT_SIZE, ELEMENTS_PER_BLOCK, archivo->file);
    accesos++;  //incrementar contador de accesos
    
    if (elementosEscritos != ELEMENTS_PER_BLOCK) {
        return -1;
    }
    
    archivo->sucio = 0;  //limpiar buffer
    return 0;
}


/*
convierte bloque binario en arreglo en memoria principal
archivo: estructura del archivo
bloqIdx: indice del bloque
return: 0 si exito, -1 si error
 */
int cargarEnMemoria(ArchivoBin *archivo, size_t bloqIdx){
     return leer_bloque(archivo, bloqIdx);
}

/*
abre un archivo binario con el modo especificado
archivo: estructura del archivo
filename: nombre del archivo
modo: modo de apertura ("rb", "wb", ...)
return 0 si exito, -1 si error
*/
int abrir_archivo(ArchivoBin *archivo, const char *filename, const char *modo) {
    assert(archivo != NULL && filename != NULL && modo != NULL);
    
    //copiar nombre del archivo
    strncpy(archivo->filename, filename, sizeof(archivo->filename) - 1);
    archivo->filename[sizeof(archivo->filename) - 1] = '\0';
    
    //abrir archivo con el modo especificado
    archivo->file = fopen(filename, modo);
    if (!archivo->file) {
        return -1;
    }
    
    //alocar buffer de tamaño B en memoria principal
    archivo->buffer = malloc(BLOCK_SIZE);
    if (!archivo->buffer) {
        fclose(archivo->file);
        return -1;
    }
    
    //inicializar estructura
    archivo->tamañoBuffer = ELEMENTS_PER_BLOCK;
    archivo->bloqActual = SIZE_MAX;  //indica que no hay bloque cargado
    archivo->sucio = 0;  //buffer no está modificado
    
    //calcular tamaño del archivo si es para lectura
    if (strchr(modo, 'r') != NULL) {
        archivo->file_elementos = obtenerTamañoArchivo(filename);
    } else {
        archivo->file_elementos = 0;
    }
    
    return 0;
}

/*
cierra un archivo y libera todos los recursos
archivo: estructura del archivo a cerrar
*/
void cerrar_archivo(ArchivoBin *archivo) {
    if (!archivo) return;
    
    //escribir buffer si está sucio antes de cerrar
    if (archivo->sucio) {
        escribirBloque(archivo);
    }
    
    //liberar buffer de memoria principal
    if (archivo->buffer) {
        free(archivo->buffer);
        archivo->buffer = NULL;
    }
    
    //cerrar archivo
    if (archivo->file) {
        fclose(archivo->file);
        archivo->file = NULL;
    }
}

/*
lee un elemento específico del archivo
archivo: estructura del archivo
posicion: posición del elemento (indice)
elemento: puntero donde almacenar el elemento leido
return 0 si exito, -1 si error
 */
int leer_elemento(ArchivoBin *archivo, size_t pos, int64_t *elemento) {
    assert(archivo != NULL && elemento != NULL);
    
    //calcular el bloque en que esta el elemento
    size_t bloqIdx = pos / ELEMENTS_PER_BLOCK;
    size_t offset_en_bloque = pos % ELEMENTS_PER_BLOCK;
    
    //cargar el bloque correspondiente si es necesario
    if (leer_bloque(archivo, bloqIdx) != 0) {
        return -1;
    }
    
    //leer el elemento del buffer en memoria
    *elemento = archivo->buffer[offset_en_bloque];
    return 0;
}

/*
Escribe un elemento específico en el archivo
archivo: estructura del archivo
pos: posición donde escribir el elemento
elemento: elemento a escribir
return 0 si exito, -1 si error
 */
int escribir_elemento(ArchivoBin *archivo, size_t pos, int64_t elemento) {
    assert(archivo != NULL);
    
    //calcular en que bloque esta el elemento
    size_t bloque_num = pos / ELEMENTS_PER_BLOCK;
    size_t offset_en_bloque = pos % ELEMENTS_PER_BLOCK;
    
    //cargar el bloque si es necesario
    if (leer_bloque(archivo, bloque_num) != 0) {
        return -1;
    }
    
    //escribir el elemento en el buffer de memoria
    archivo->buffer[offset_en_bloque] = elemento;
    archivo->bloqActual = bloque_num;
    archivo->sucio = 1;  //marcar buffer como modificado
    
    return 0;
}

/*
obtiene el tamaño de un archivo en número de elementos
filename: nombre del archivo
return: numero de elementos en el archivo
 */
size_t obtener_tamaño_archivo(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return 0;
    }
    
    //moverse al final del archivo
    fseek(file, 0, SEEK_END);
    long bytes = ftell(file);
    fclose(file);
    
    //convertir bytes a número de elementos
    return bytes / ELEMENT_SIZE;
}


