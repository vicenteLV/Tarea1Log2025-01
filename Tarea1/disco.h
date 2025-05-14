#ifndef DISCO_H
#define DISCO_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define BLOCK_SIZE 4096                //4kb por bloque
#define ELEMENT_SIZE sizeof(int64_t)   //8 bytes por elemento
#define ELEMENTS_PER_BLOCK (BLOCK_SIZE/ELEMENT_SIZE)  //elementos por bloque
#define MEMORY_LIMIT (50 * 1024 * 1024)  //limite de 50MB

typedef struct{
    FILE *file;
    int64_t *buffer;
    size_t tamañoBuffer;
    size_t bloqActual;
    int sucio ; //indica si buffer ha sido modificado pero aun no se escribe en disco (si está 'sucio')
    char filename[256];
    size_t file_elementos; //tamaño del archivo en elementos

} ArchivoBin;

//funciones principales
int leerBloque(ArchivoBin *archivo, size_t bloqIdx);
int escribirBloque(ArchivoBin *archivo);
int cargarEnMemoria(ArchivoBin *archivo, size_t bloqIdx);

//auxiliares
int abrirArchivo(ArchivoBin *archivo, const char *filename, const char *modo);
void cerrarArchivo(ArchivoBin *archivo);
int leerElemento(ArchivoBin *archivo, size_t pos, int64_t *elemento);
int escribirElemento(ArchivoBin *archivo, size_t pos, int64_t elemento);

size_t obtenerTamañoArchivo(const char *filename);


#endif